// qtractorMidiMeter.cpp
//
/****************************************************************************
   Copyright (C) 2005-2017, rncbc aka Rui Nuno Capela. All rights reserved.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

*****************************************************************************/

#include "qtractorAbout.h"
#include "qtractorMidiMeter.h"
#include "qtractorMidiMonitor.h"

#include "qtractorObserverWidget.h"

#include <QResizeEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPixmap>
#include <QLayout>
#include <QLabel>


// The decay rates (magic goes here :).
// - value decay rate (faster)
#define QTRACTOR_MIDI_METER_DECAY_RATE1		(1.0f - 1E-5f)
// - peak decay rate (slower)
#define QTRACTOR_MIDI_METER_DECAY_RATE2		(1.0f - 1E-6f)

// Number of cycles the peak stays on hold before fall-off.
#define QTRACTOR_MIDI_METER_PEAK_FALLOFF	16

// Number of cycles the MIDI LED stays on before going off.
#define QTRACTOR_MIDI_METER_HOLD_LEDON  	4


// MIDI On/Off LED pixmap resource.
int      qtractorMidiMeterLed::g_iLedRefCount = 0;
QPixmap *qtractorMidiMeterLed::g_pLedPixmap[qtractorMidiMeterLed::LedCount];

// MIDI meter color arrays.
QColor qtractorMidiMeter::g_defaultColors[ColorCount] = {
	QColor(160,220, 20),	// ColorPeak
	QColor(160,160, 40),	// ColorOver
	QColor( 20, 40, 20),	// ColorBack
	QColor( 80, 80, 80) 	// ColorFore
};

QColor qtractorMidiMeter::g_currentColors[ColorCount] = {
	g_defaultColors[ColorPeak],
	g_defaultColors[ColorOver],
	g_defaultColors[ColorBack],
	g_defaultColors[ColorFore]
};

//----------------------------------------------------------------------------
// qtractorMidiMeterScale -- Meter bridge scale widget.

// Constructor.
qtractorMidiMeterScale::qtractorMidiMeterScale (
	qtractorMidiMeter *pMidiMeter, QWidget *pParent )
	: qtractorMeterScale(pMidiMeter, pParent)
{
	// Nothing much to do...
}


// Actual scale drawing method.
void qtractorMidiMeterScale::paintScale ( QPainter *pPainter )
{
	qtractorMidiMeter *pMidiMeter
		= static_cast<qtractorMidiMeter *> (meter());
	if (pMidiMeter == NULL)
		return;

	const int h = QWidget::height() - 4;
	const int d = (h / 5);

	int y = h;
	int n = 100;

	while (y > 0) {
		drawLineLabel(pPainter, y, QString::number(n));
		y -= d; n -= 20;
	}
}


//----------------------------------------------------------------------------
// qtractorMidiMeterValue -- MIDI meter bridge value widget.

// Constructor.
qtractorMidiMeterValue::qtractorMidiMeterValue (
	qtractorMidiMeter *pMidiMeter, QWidget *pParent )
	: qtractorMeterValue(pMidiMeter, pParent)
{
	// Avoid intensively annoying repaints...
	QWidget::setAttribute(Qt::WA_StaticContents);
	QWidget::setAttribute(Qt::WA_OpaquePaintEvent);

	QWidget::setBackgroundRole(QPalette::NoRole);

	m_iValue      = 0;
	m_fValueDecay = QTRACTOR_MIDI_METER_DECAY_RATE1;

	m_iPeak       = 0;
	m_iPeakHold   = 0;
	m_fPeakDecay  = QTRACTOR_MIDI_METER_DECAY_RATE2;

//	QWidget::setFixedWidth(14);
	QWidget::setMaximumWidth(14);
}


// Value refreshment.
void qtractorMidiMeterValue::refresh ( unsigned long iStamp )
{
	qtractorMidiMeter *pMidiMeter
		= static_cast<qtractorMidiMeter *> (meter());
	if (pMidiMeter == NULL)
		return;

	qtractorMidiMonitor *pMidiMonitor = pMidiMeter->midiMonitor();
	if (pMidiMonitor == NULL)
		return;

	const float fValue = pMidiMonitor->value_stamp(iStamp);
	if (fValue < 0.001f && m_iPeak < 1)
		return;

	int iValue = pMidiMeter->scale(fValue);
	if (iValue < m_iValue) {
		iValue = int(m_fValueDecay * float(m_iValue));
		m_fValueDecay *= m_fValueDecay;
	} else {
		m_fValueDecay = QTRACTOR_MIDI_METER_DECAY_RATE1;
	}

	int iPeak = m_iPeak;
	if (iPeak < iValue) {
		iPeak = iValue;
		m_iPeakHold = 0;
		m_fPeakDecay = QTRACTOR_MIDI_METER_DECAY_RATE2;
	} else if (++m_iPeakHold > pMidiMeter->peakFalloff()) {
		iPeak = int(m_fPeakDecay * float(iPeak));
		if (iPeak < iValue) {
			iPeak = iValue;
		} else {
			m_fPeakDecay *= m_fPeakDecay;
		}
	}

	if (iValue == m_iValue && iPeak == m_iPeak)
		return;

	m_iValue = iValue;
	m_iPeak  = iPeak;

	update();
}


// Paint event handler.
void qtractorMidiMeterValue::paintEvent ( QPaintEvent * )
{
	qtractorMidiMeter *pMidiMeter
		= static_cast<qtractorMidiMeter *> (meter());
	if (pMidiMeter == NULL)
		return;

	QPainter painter(this);

	const int w = QWidget::width();
	const int h = QWidget::height();

	if (isEnabled()) {
		painter.fillRect(0, 0, w, h,
			pMidiMeter->color(qtractorMidiMeter::ColorBack));
	} else {
		painter.fillRect(0, 0, w, h, Qt::gray);
	}

#ifdef CONFIG_GRADIENT
	painter.drawPixmap(0, h - m_iValue,
		pMidiMeter->pixmap(), 0, h - m_iValue, w, m_iValue);
#else
	painter.fillRect(0, h - m_iValue, w, m_iValue,
		MidiMeter->color(qtractorMidiMeter::ColorOver));
#endif

	painter.setPen(pMidiMeter->color(qtractorMidiMeter::ColorPeak));
	painter.drawLine(0, h - m_iPeak, w, h - m_iPeak);
}


// Resize event handler.
void qtractorMidiMeterValue::resizeEvent ( QResizeEvent *pResizeEvent )
{
	m_iPeak = 0;

	QWidget::resizeEvent(pResizeEvent);
//	QWidget::repaint();
}


//----------------------------------------------------------------------------
// qtractorMidiMeterLed -- MIDI meter bridge LED widget.

// Constructor.
qtractorMidiMeterLed::qtractorMidiMeterLed (
	qtractorMidiMeter *pMidiMeter, QWidget *pParent )
	: qtractorMeterValue(pMidiMeter, pParent)
{
	if (++g_iLedRefCount == 1) {
		g_pLedPixmap[LedOff] = new QPixmap(":/images/trackMidiOff.png");
		g_pLedPixmap[LedOn]  = new QPixmap(":/images/trackMidiOn.png");
	}

	m_iMidiCount = 0;

	m_pMidiLabel = new QLabel();
	m_pMidiLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
	m_pMidiLabel->setPixmap(*g_pLedPixmap[LedOff]);

	QHBoxLayout *pHBoxLayout = new QHBoxLayout();
	pHBoxLayout->setMargin(2);
	pHBoxLayout->addWidget(m_pMidiLabel);
	qtractorMeterValue::setLayout(pHBoxLayout);
}


// Default destructor.
qtractorMidiMeterLed::~qtractorMidiMeterLed (void)
{
	delete m_pMidiLabel;

	if (--g_iLedRefCount == 0) {
		delete g_pLedPixmap[LedOff];
		delete g_pLedPixmap[LedOn];
	}
}


// Value refreshment.
void qtractorMidiMeterLed::refresh ( unsigned long iStamp )
{
	qtractorMidiMeter *pMidiMeter
		= static_cast<qtractorMidiMeter *> (meter());
	if (pMidiMeter == NULL)
		return;

	qtractorMidiMonitor *pMidiMonitor = pMidiMeter->midiMonitor();
	if (pMidiMonitor == NULL)
		return;

	// Take care of the MIDI LED status...
	const bool bMidiOn = (pMidiMonitor->count_stamp(iStamp) > 0);
	if (bMidiOn) {
		if (m_iMidiCount == 0)
			m_pMidiLabel->setPixmap(*g_pLedPixmap[LedOn]);
		m_iMidiCount = QTRACTOR_MIDI_METER_HOLD_LEDON;
	} else if (m_iMidiCount > 0) {
		if (--m_iMidiCount == 0)
			m_pMidiLabel->setPixmap(*g_pLedPixmap[LedOff]);
	}
}


//----------------------------------------------------------------------------
// qtractorMidiMeter -- MIDI meter bridge slot widget.

// Constructor.
qtractorMidiMeter::qtractorMidiMeter (
	qtractorMidiMonitor *pMidiMonitor, QWidget *pParent )
	: qtractorMeter(pParent)
{
	m_pMidiMonitor = pMidiMonitor;

#ifdef CONFIG_GRADIENT
	m_pPixmap = new QPixmap();
#endif

	m_pMidiValue = new qtractorMidiMeterValue(this);

	boxLayout()->addWidget(m_pMidiValue);

	setPeakFalloff(QTRACTOR_MIDI_METER_PEAK_FALLOFF);

	reset();
}


// Default destructor.
qtractorMidiMeter::~qtractorMidiMeter (void)
{
#ifdef CONFIG_GRADIENT
	delete m_pPixmap;
#endif

	// No need to delete child widgets, Qt does it all for us
	delete m_pMidiValue;
}


// MIDI monitor reset
void qtractorMidiMeter::reset (void)
{
	// Nothing really to do?
}


#ifdef CONFIG_GRADIENT
// Gradient pixmap accessor.
const QPixmap& qtractorMidiMeter::pixmap (void) const
{
	return *m_pPixmap;
}

void qtractorMidiMeter::updatePixmap (void)
{
	const int w = QWidget::width();
	const int h = QWidget::height();

	QLinearGradient grad(0, 0, 0, h);
	grad.setColorAt(0.0f, color(ColorPeak));
	grad.setColorAt(0.4f, color(ColorOver));

	*m_pPixmap = QPixmap(w, h);

	QPainter(m_pPixmap).fillRect(0, 0, w, h, grad);
}
#endif


// Resize event handler.
void qtractorMidiMeter::resizeEvent ( QResizeEvent * )
{
	qtractorMeter::setScale(float(QWidget::height()));

#ifdef CONFIG_GRADIENT
	updatePixmap();
#endif
}


// Virtual monitor accessor.
void qtractorMidiMeter::setMonitor ( qtractorMonitor *pMonitor )
{
	setMidiMonitor(static_cast<qtractorMidiMonitor *> (pMonitor));
}

qtractorMonitor *qtractorMidiMeter::monitor (void) const
{
	return midiMonitor();
}

// MIDI monitor accessor.
void qtractorMidiMeter::setMidiMonitor ( qtractorMidiMonitor *pMidiMonitor )
{
	m_pMidiMonitor = pMidiMonitor;

	reset();
}

qtractorMidiMonitor *qtractorMidiMeter::midiMonitor (void) const
{
	return m_pMidiMonitor;
}


// Common resource accessor.
void qtractorMidiMeter::setColor ( int iIndex, const QColor& color )
{
	g_currentColors[iIndex] = color;
}

const QColor& qtractorMidiMeter::color ( int iIndex )
{
	return g_currentColors[iIndex];
}

const QColor& qtractorMidiMeter::defaultColor ( int iIndex )
{
	return g_defaultColors[iIndex];
}


//----------------------------------------------------------------------
// class qtractorMidiMixerMeter::GainSpinBoxInterface -- Observer interface.
//

// Local converter interface.
class qtractorMidiMixerMeter::GainSpinBoxInterface
	: public qtractorObserverSpinBox::Interface
{
public:

	// Constructor.
	GainSpinBoxInterface ( qtractorObserverSpinBox *pSpinBox )
		: qtractorObserverSpinBox::Interface(pSpinBox) {}

	// Formerly Pure virtuals.
	float scaleFromValue ( float fValue ) const
		{ return 100.0f * fValue; }

	float valueFromScale ( float fScale ) const
		{ return 0.01f * fScale; }
};


//----------------------------------------------------------------------
// class qtractorMidiMixerMeter::GainSliderInterface -- Observer interface.
//

// Local converter interface.
class qtractorMidiMixerMeter::GainSliderInterface
	: public qtractorObserverSlider::Interface
{
public:

	// Constructor.
	GainSliderInterface ( qtractorObserverSlider *pSlider )
		: qtractorObserverSlider::Interface(pSlider) {}

	// Formerly Pure virtuals.
	float scaleFromValue ( float fValue ) const
		{ return 10000.0f * fValue; }

	float valueFromScale ( float fScale ) const
		{ return 0.0001f * fScale; }
};


//----------------------------------------------------------------------------
// qtractorMidiMixerMeter -- MIDI mixer-strip meter bridge widget.

// Constructor.
qtractorMidiMixerMeter::qtractorMidiMixerMeter (
	qtractorMidiMonitor *pMidiMonitor, QWidget *pParent )
	: qtractorMixerMeter(pParent)
{
	m_pMidiMeter = new qtractorMidiMeter(pMidiMonitor);
	m_pMidiScale = new qtractorMidiMeterScale(m_pMidiMeter);
	m_pMidiLed   = new qtractorMidiMeterLed(m_pMidiMeter);

	topLayout()->addStretch();
	topLayout()->addWidget(m_pMidiLed);

	boxLayout()->addWidget(m_pMidiScale);
	boxLayout()->addWidget(m_pMidiMeter);

	gainSlider()->setInterface(new GainSliderInterface(gainSlider()));
	gainSpinBox()->setInterface(new GainSpinBoxInterface(gainSpinBox()));

	gainSpinBox()->setMinimum(0.0f);
	gainSpinBox()->setMaximum(100.0f);
	gainSpinBox()->setToolTip(tr("Volume (%)"));
	gainSpinBox()->setSuffix(tr(" %"));

	reset();

	updatePanning();
	updateGain();
}


// Default destructor.
qtractorMidiMixerMeter::~qtractorMidiMixerMeter (void)
{
	delete m_pMidiLed;
	delete m_pMidiScale;
	delete m_pMidiMeter;

	// No need to delete child widgets, Qt does it all for us
}


// MIDI monitor reset
void qtractorMidiMixerMeter::reset (void)
{
	qtractorMidiMonitor *pMidiMonitor = m_pMidiMeter->midiMonitor();
	if (pMidiMonitor == NULL)
		return;

	m_pMidiMeter->reset();

	setPanningSubject(pMidiMonitor->panningSubject());
	setGainSubject(pMidiMonitor->gainSubject());
}


// Virtual monitor accessor.
void qtractorMidiMixerMeter::setMonitor ( qtractorMonitor *pMonitor )
{
	m_pMidiMeter->setMonitor(pMonitor);
}

qtractorMonitor *qtractorMidiMixerMeter::monitor (void) const
{
	return m_pMidiMeter->monitor();
}

// MIDI monitor accessor.
void qtractorMidiMixerMeter::setMidiMonitor ( qtractorMidiMonitor *pMidiMonitor )
{
	m_pMidiMeter->setMidiMonitor(pMidiMonitor);

	reset();
}

qtractorMidiMonitor *qtractorMidiMixerMeter::midiMonitor (void) const
{
	return m_pMidiMeter->midiMonitor();
}


// Pan-slider value change method.
void qtractorMidiMixerMeter::updatePanning (void)
{
//	setPanning(m_pMidiMonitor->panning());

	panSlider()->setToolTip(
		tr("Pan: %1").arg(panning(), 0, 'g', 1));
}


// Gain-slider value change method.
void qtractorMidiMixerMeter::updateGain (void)
{
//	setGain(m_pMidiMonitor->gain());

	gainSlider()->setToolTip(
		tr("Volume: %1%").arg(gainSpinBox()->value(), 0, 'g', 3));
}


// end of qtractorMidiMeter.cpp

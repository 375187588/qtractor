// qtractorTempoAdjustForm.h
//
/****************************************************************************
   Copyright (C) 2005-2019, rncbc aka Rui Nuno Capela. All rights reserved.

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

#ifndef __qtractorTempoAdjustForm_h
#define __qtractorTempoAdjustForm_h

#include "ui_qtractorTempoAdjustForm.h"

// Forward declarations.
class QTime;


//----------------------------------------------------------------------------
// qtractorTempoAdjustForm -- UI wrapper form.

class qtractorTempoAdjustForm : public QDialog
{
	Q_OBJECT

public:

	// Constructor.
	qtractorTempoAdjustForm(QWidget *pParent = 0, Qt::WindowFlags wflags = 0);
	// Destructor.
	~qtractorTempoAdjustForm();

	// Range accessors.
	void setRangeStart(unsigned long iRangeStart);
	unsigned long rangeStart() const;

	void setRangeLength(unsigned long iRangeLength);
	unsigned long rangeLength() const;

	void setRangeBeats(unsigned short iRangeBeats);
	unsigned short rangeBeats() const;
	
	// Accepted results accessors.
	float tempo() const;
	unsigned short beatsPerBar() const;
	unsigned short beatDivisor() const;
	
protected slots:

	void tempoChanged();
	void tempoTap();

	void rangeStartChanged(unsigned long);
	void rangeLengthChanged(unsigned long);
	void rangeBeatsChanged(int);
	void formatChanged(int);
	void adjust();
	void changed();

	void accept();
	void reject();

protected:

	void updateRangeLength(unsigned long iRangeLength);
	void updateRangeSelect();

	void stabilizeForm();

private:

	// The Qt-designer UI struct...
	Ui::qtractorTempoAdjustForm m_ui;

	// Instance variables...
	qtractorTimeScale *m_pTimeScale;

	QTime *m_pTempoTap;
	int    m_iTempoTap;
	float  m_fTempoTap;

	int m_iDirtySetup;
	int m_iDirtyCount;
};


#endif	// __qtractorTempoAdjustForm_h


// end of qtractorTempoAdjustForm.h

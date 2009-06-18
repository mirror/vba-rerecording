// -*- C++ -*-
// VisualBoyAdvance - Nintendo Gameboy/GameboyAdvance (TM) emulator.
// Copyright (C) 1999-2003 Forgotten
// Copyright (C) 2004 Forgotten and the VBA development team

// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2, or(at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

#if !defined(AFX_REWINDINTERVAL_H__C95AFF44_1F64_44C8_BAAB_A54B982D28EA__INCLUDED_)
#define AFX_REWINDINTERVAL_H__C95AFF44_1F64_44C8_BAAB_A54B982D28EA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// RewindInterval.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// RewindInterval dialog

class RewindInterval : public CDialog
{
	// Construction
public:
	float interval;
	int   slots;
	RewindInterval(float interval, int slots, CWnd*pParent = NULL);  // standard constructor

	// Dialog Data
	//{{AFX_DATA(RewindInterval)
	enum { IDD = IDD_REWIND_INTERVAL };
	CEdit m_interval;
	CEdit m_slots;
	//}}AFX_DATA

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(RewindInterval)
protected:
	virtual void DoDataExchange(CDataExchange*pDX);   // DDX/DDV support
	//}}AFX_VIRTUAL

	// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(RewindInterval)
	afx_msg void OnCancel();
	afx_msg void OnOk();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_REWINDINTERVAL_H__C95AFF44_1F64_44C8_BAAB_A54B982D28EA__INCLUDED_)

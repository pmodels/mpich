/* -*- Mode: C++; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* style:c++ header */

#if !defined(AFX_ZOOMDLG_H__E425FB6A_FDA5_4359_AA8F_FF1283FF7B1B__INCLUDED_)
#define AFX_ZOOMDLG_H__E425FB6A_FDA5_4359_AA8F_FF1283FF7B1B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ZoomDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CZoomDlg dialog

class CZoomDlg : public CDialog
{
// Construction
public:
	CZoomDlg(CWnd* pParent = NULL);   // standard constructor

	bool m_bLR, m_bWidth, m_bPerPixel;
// Dialog Data
	//{{AFX_DATA(CZoomDlg)
	enum { IDD = IDD_ZOOM_DLG };
	CEdit	m_width_edit;
	CEdit	m_right_edit;
	CEdit	m_perpixel_edit;
	CEdit	m_left_edit;
	double	m_dLeft;
	double	m_dPerPixel;
	double	m_dRight;
	double	m_dWidth;
	CButton m_first_radio;
	CButton m_second_radio;
	CButton m_third_radio;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CZoomDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CZoomDlg)
	afx_msg void OnLeftRightRadio();
	afx_msg void OnWidthRadio();
	afx_msg void OnPerpixelRadio();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ZOOMDLG_H__E425FB6A_FDA5_4359_AA8F_FF1283FF7B1B__INCLUDED_)

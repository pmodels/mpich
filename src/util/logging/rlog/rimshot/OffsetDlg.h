/* -*- Mode: C++; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* style:c++ header */

#ifndef OFFSETDLG_H_INCLUDED
#define OFFSETDLG_H_INCLUDED

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// OffsetDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// COffsetDlg dialog

class COffsetDlg:public CDialog {
// Construction
  public:
    COffsetDlg(CWnd * pParent = NULL);  // standard constructor

    bool m_bUseGUI;
// Dialog Data
    //{{AFX_DATA(COffsetDlg)
    enum { IDD = IDD_OFFSET_DLG };
    double m_offset;
    int m_row;
    //}}AFX_DATA


// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(COffsetDlg)
  protected:
     virtual void DoDataExchange(CDataExchange * pDX);  // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
  protected:

    // Generated message map functions
    //{{AFX_MSG(COffsetDlg)
     afx_msg void OnGuiBtn();
    //}}AFX_MSG
     DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif /* OFFSETDLG_H_INCLUDED */

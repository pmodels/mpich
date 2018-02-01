/* -*- Mode: C++; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* style:c++ header*/
// rimshot.h : main header file for the RIMSHOT application
//

#ifndef RIMSHOT_H_INCLUDED
#define RIMSHOT_H_INCLUDED

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"   // main symbols

/////////////////////////////////////////////////////////////////////////////
// CRimshotApp:
// See rimshot.cpp for the implementation of this class
//

class CRimshotApp:public CWinApp {
  public:
    CRimshotApp();

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CRimshotApp)
  public:
    virtual BOOL InitInstance();
    //}}AFX_VIRTUAL

// Implementation
    //{{AFX_MSG(CRimshotApp)
    afx_msg void OnAppAbout();
    //}}AFX_MSG
     DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif /* RIMSHOT_H_INCLUDED */

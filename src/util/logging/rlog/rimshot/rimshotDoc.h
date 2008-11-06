/* -*- Mode: C++; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* style:c++ header */

// rimshotDoc.h : interface of the CRimshotDoc class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_RIMSHOTDOC_H__C4934ACC_DBF0_46BF_8A88_6FAA09F6A9CE__INCLUDED_)
#define AFX_RIMSHOTDOC_H__C4934ACC_DBF0_46BF_8A88_6FAA09F6A9CE__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "rlog.h"

struct StateNode
{
    RLOG_STATE state;
    int id;
    COLORREF color;
    CBrush brush;
    StateNode *pNext;
};

class CRimshotDoc : public CDocument
{
protected: // create from serialization only
	CRimshotDoc();
	DECLARE_DYNCREATE(CRimshotDoc)

// Attributes
public:
    RLOG_IOStruct *m_pInput;
    double m_dFirst, m_dLast;
    double m_dLeft, m_dRight;
    StateNode *m_pStateList;

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CRimshotDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
	//}}AFX_VIRTUAL

// Implementation
public:
	CString GetEventDescription(int event);
	CBrush* GetEventBrush(int event);
	COLORREF GetEventColor(int event);
	virtual ~CRimshotDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CRimshotDoc)
	afx_msg void OnFileOpen();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RIMSHOTDOC_H__C4934ACC_DBF0_46BF_8A88_6FAA09F6A9CE__INCLUDED_)

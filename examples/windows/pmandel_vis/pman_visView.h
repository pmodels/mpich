/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
// pman_visView.h : interface of the Cpman_visView class
//


#pragma once

#include "ExampleNode.h"

class Cpman_visView : public CView
{
protected: // create from serialization only
	Cpman_visView();
	DECLARE_DYNCREATE(Cpman_visView)

// Attributes
public:
	Cpman_visDoc* GetDocument() const;
	CPoint m_p1, m_p2;
	HANDLE m_hThread;
	RECT m_rLast;
	bool bConnected;

// Operations
public:

// Overrides
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
protected:
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);

// Implementation
public:
	virtual ~Cpman_visView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	DECLARE_MESSAGE_MAP()
public:
    afx_msg void OnFileConnect();
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
    afx_msg void OnFileEnterpoint();
    afx_msg void OnFileEnterdemopoints();
    afx_msg void OnFileStopdemo();
};

#ifndef _DEBUG  // debug version in pman_visView.cpp
inline Cpman_visDoc* Cpman_visView::GetDocument() const
   { return reinterpret_cast<Cpman_visDoc*>(m_pDocument); }
#endif


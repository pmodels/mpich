/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#pragma once
#include "afxwin.h"

#include "ExampleNode.h"

// CDemoPointsDlg dialog

class CDemoPointsDlg : public CDialog
{
	DECLARE_DYNAMIC(CDemoPointsDlg)

public:
	CDemoPointsDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDemoPointsDlg();

	CExampleNode *m_node_list;

// Dialog Data
	enum { IDD = IDD_DEMO_POINTS_DLG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
    afx_msg void OnLbnDblclkPointsList();
    CString m_cur_selected;
    afx_msg void OnBnClickedAddBtn();
    CListBox m_list;
    afx_msg void OnBnClickedLoadBtn();
};

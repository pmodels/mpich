/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

// OffsetDlg.cpp : implementation file
//

#include "stdafx.h"
#include "rimshot.h"
#include "OffsetDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// COffsetDlg dialog


COffsetDlg::COffsetDlg(CWnd* pParent /*=NULL*/)
	: CDialog(COffsetDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(COffsetDlg)
	m_offset = 0.0;
	m_row = 0;
	//}}AFX_DATA_INIT
	m_bUseGUI = false;
}


void COffsetDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(COffsetDlg)
	DDX_Text(pDX, IDC_OFFSET, m_offset);
	DDX_Text(pDX, IDC_ROW, m_row);
	DDV_MinMaxInt(pDX, m_row, 0, 100000);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(COffsetDlg, CDialog)
	//{{AFX_MSG_MAP(COffsetDlg)
	ON_BN_CLICKED(IDC_GUI_BTN, OnGuiBtn)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// COffsetDlg message handlers

void COffsetDlg::OnGuiBtn() 
{
    m_bUseGUI = true;
    EndDialog(IDOK);
}

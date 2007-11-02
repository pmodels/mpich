/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
// BoundsDlg.cpp : implementation file
//

#include "stdafx.h"
#include "pman_vis.h"
#include "BoundsDlg.h"


// CBoundsDlg dialog

IMPLEMENT_DYNAMIC(CBoundsDlg, CDialog)
CBoundsDlg::CBoundsDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CBoundsDlg::IDD, pParent)
	, m_xmin(-1.0)
	, m_ymin(-1.0)
	, m_xmax(1.0)
	, m_ymax(1.0)
	, m_max_iter(100)
{
}

CBoundsDlg::~CBoundsDlg()
{
}

void CBoundsDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Text(pDX, IDC_XMIN_EDIT, m_xmin);
    DDX_Text(pDX, IDC_YMIN_EDIT, m_ymin);
    DDX_Text(pDX, IDC_XMAX_EDIT, m_xmax);
    DDX_Text(pDX, IDC_YMAX_EDIT, m_ymax);
    DDX_Text(pDX, IDC_MAX_ITER_EDIT, m_max_iter);
	DDV_MinMaxInt(pDX, m_max_iter, 1, 5000);
}


BEGIN_MESSAGE_MAP(CBoundsDlg, CDialog)
END_MESSAGE_MAP()


// CBoundsDlg message handlers

/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

// ZoomDlg.cpp : implementation file
//

#include "stdafx.h"
#include "rimshot.h"
#include "ZoomDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CZoomDlg dialog


CZoomDlg::CZoomDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CZoomDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CZoomDlg)
	m_dLeft = 0.0;
	m_dPerPixel = 1.0;
	m_dRight = 1.0;
	m_dWidth = 1.0;
	//}}AFX_DATA_INIT
	m_bLR = true;
	m_bWidth = false;
	m_bPerPixel = false;
}


void CZoomDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CZoomDlg)
	DDX_Control(pDX, IDC_WIDTH, m_width_edit);
	DDX_Control(pDX, IDC_RIGHT, m_right_edit);
	DDX_Control(pDX, IDC_PERPIXEL, m_perpixel_edit);
	DDX_Control(pDX, IDC_LEFT, m_left_edit);
	DDX_Text(pDX, IDC_LEFT, m_dLeft);
	DDX_Text(pDX, IDC_PERPIXEL, m_dPerPixel);
	DDX_Text(pDX, IDC_RIGHT, m_dRight);
	DDX_Text(pDX, IDC_WIDTH, m_dWidth);
	DDX_Control(pDX, IDC_LEFT_RIGHT_RADIO, m_first_radio);
	DDX_Control(pDX, IDC_WIDTH_RADIO, m_second_radio);
	DDX_Control(pDX, IDC_PERPIXEL_RADIO, m_third_radio);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CZoomDlg, CDialog)
	//{{AFX_MSG_MAP(CZoomDlg)
	ON_BN_CLICKED(IDC_LEFT_RIGHT_RADIO, OnLeftRightRadio)
	ON_BN_CLICKED(IDC_WIDTH_RADIO, OnWidthRadio)
	ON_BN_CLICKED(IDC_PERPIXEL_RADIO, OnPerpixelRadio)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CZoomDlg message handlers

void CZoomDlg::OnLeftRightRadio() 
{
    m_right_edit.EnableWindow();
    m_left_edit.EnableWindow();
    m_first_radio.SetCheck(1);
    m_bLR = true;
    
    m_bWidth = false;
    m_bPerPixel = false;
    m_width_edit.EnableWindow(FALSE);
    m_perpixel_edit.EnableWindow(FALSE);
    m_second_radio.SetCheck(0);
    m_third_radio.SetCheck(0);
}

void CZoomDlg::OnWidthRadio() 
{
    m_width_edit.EnableWindow();
    m_second_radio.SetCheck(1);
    m_bWidth = true;

    m_bLR = false;
    m_bPerPixel = false;
    m_right_edit.EnableWindow(FALSE);
    m_left_edit.EnableWindow(FALSE);
    m_perpixel_edit.EnableWindow(FALSE);
    m_first_radio.SetCheck(0);
    m_third_radio.SetCheck(0);
}

void CZoomDlg::OnPerpixelRadio() 
{
    m_perpixel_edit.EnableWindow();
    m_third_radio.SetCheck(1);
    m_bPerPixel = true;

    m_bWidth = false;
    m_bLR = false;
    m_width_edit.EnableWindow(FALSE);
    m_right_edit.EnableWindow(FALSE);
    m_left_edit.EnableWindow(FALSE);
    m_first_radio.SetCheck(0);
    m_second_radio.SetCheck(0);
}

BOOL CZoomDlg::OnInitDialog() 
{
    CDialog::OnInitDialog();

    m_first_radio.SetCheck(1);
    OnLeftRightRadio();

    return TRUE;  // return TRUE unless you set the focus to a control
    // EXCEPTION: OCX Property Pages should return FALSE
}

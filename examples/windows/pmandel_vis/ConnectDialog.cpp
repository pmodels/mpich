/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
// ConnectDialog.cpp : implementation file
//

#include "stdafx.h"
#include "pman_vis.h"
#include "ConnectDialog.h"
#include ".\connectdialog.h"


// CConnectDialog dialog

IMPLEMENT_DYNAMIC(CConnectDialog, CDialog)
CConnectDialog::CConnectDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CConnectDialog::IDD, pParent)
	, m_nPort(0)
	, m_pszHost(_T(""))
	, m_pszMPIPort(_T(""))
{
    m_type = MPI_CONNECT;
}

CConnectDialog::~CConnectDialog()
{
}

void CConnectDialog::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Text(pDX, IDC_PORT, m_nPort);
    DDX_Text(pDX, IDC_HOSTNAME, m_pszHost);
    DDX_Text(pDX, IDC_MPIPORT, m_pszMPIPort);
    DDX_Control(pDX, IDC_MPI_RADIO, m_MPI_Radio);
    DDX_Control(pDX, IDC_TCP_RADIO, m_TCP_Radio);
    DDX_Control(pDX, IDC_MPIPORT, m_mpi_port_edit);
    DDX_Control(pDX, IDC_HOSTNAME, m_host_edit);
    DDX_Control(pDX, IDC_PORT, m_port_edit);
}


BEGIN_MESSAGE_MAP(CConnectDialog, CDialog)
    ON_BN_CLICKED(IDC_MPI_RADIO, OnBnClickedMpiRadio)
    ON_BN_CLICKED(IDC_TCP_RADIO, OnBnClickedTcpRadio)
END_MESSAGE_MAP()


// CConnectDialog message handlers

void CConnectDialog::OnBnClickedMpiRadio()
{
    m_MPI_Radio.SetCheck(1);
    m_TCP_Radio.SetCheck(0);
    m_type = MPI_CONNECT;
    m_mpi_port_edit.EnableWindow();
    m_host_edit.EnableWindow(FALSE);
    m_port_edit.EnableWindow(FALSE);
}

void CConnectDialog::OnBnClickedTcpRadio()
{
    m_MPI_Radio.SetCheck(0);
    m_TCP_Radio.SetCheck(1);
    m_type = TCP_CONNECT;
    m_mpi_port_edit.EnableWindow(FALSE);
    m_host_edit.EnableWindow();
    m_port_edit.EnableWindow();
}

BOOL CConnectDialog::OnInitDialog()
{
    CDialog::OnInitDialog();

    OnBnClickedMpiRadio();

    return TRUE;  // return TRUE unless you set the focus to a control
    // EXCEPTION: OCX Property Pages should return FALSE
}

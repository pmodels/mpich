/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#pragma once
#include "afxwin.h"


// CConnectDialog dialog

class CConnectDialog : public CDialog
{
	DECLARE_DYNAMIC(CConnectDialog)

public:
	CConnectDialog(CWnd* pParent = NULL);   // standard constructor
	virtual ~CConnectDialog();

// Dialog Data
	enum { IDD = IDD_CONNECT_DIALOG };
	enum CONNECT_TYPE { MPI_CONNECT, TCP_CONNECT };
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
    int m_nPort;
    CString m_pszHost;
    CString m_pszMPIPort;
    afx_msg void OnBnClickedMpiRadio();
    afx_msg void OnBnClickedTcpRadio();
    CButton m_MPI_Radio;
    CButton m_TCP_Radio;
    CONNECT_TYPE m_type;
    CEdit m_mpi_port_edit;
    CEdit m_host_edit;
    CEdit m_port_edit;
    virtual BOOL OnInitDialog();
};

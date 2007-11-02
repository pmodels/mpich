/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
// DemoPointsDlg.cpp : implementation file
//

#include "stdafx.h"
#include "pman_vis.h"
#include "DemoPointsDlg.h"
#include ".\demopointsdlg.h"
#include "BoundsDlg.h"

// CDemoPointsDlg dialog

IMPLEMENT_DYNAMIC(CDemoPointsDlg, CDialog)
CDemoPointsDlg::CDemoPointsDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CDemoPointsDlg::IDD, pParent)
	, m_cur_selected(_T(""))
{
    m_node_list = NULL;
}

CDemoPointsDlg::~CDemoPointsDlg()
{
}

void CDemoPointsDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_LBString(pDX, IDC_POINTS_LIST, m_cur_selected);
    DDX_Control(pDX, IDC_POINTS_LIST, m_list);
}


BEGIN_MESSAGE_MAP(CDemoPointsDlg, CDialog)
    ON_LBN_DBLCLK(IDC_POINTS_LIST, OnLbnDblclkPointsList)
    ON_BN_CLICKED(IDC_ADD_BTN, OnBnClickedAddBtn)
    ON_BN_CLICKED(IDC_LOAD_BTN, OnBnClickedLoadBtn)
END_MESSAGE_MAP()


// CDemoPointsDlg message handlers

void CDemoPointsDlg::OnLbnDblclkPointsList()
{
    int index;
    UpdateData();
    MessageBox(m_cur_selected, "current", MB_OK);
    index = m_list.GetCurSel();
}

void CDemoPointsDlg::OnBnClickedAddBtn()
{
    CBoundsDlg dlg;
    CExampleNode *node;
    if (dlg.DoModal() == IDOK)
    {
	CString str;
	str.Format("%f %f %f %f %d", dlg.m_xmin, dlg.m_ymin, dlg.m_xmax, dlg.m_ymax, dlg.m_max_iter);
	m_list.AddString(str);
	node = new CExampleNode();
	node->xmin = dlg.m_xmin;
	node->xmax = dlg.m_xmax;
	node->ymin = dlg.m_ymin;
	node->ymax = dlg.m_ymax;
	node->max_iter = dlg.m_max_iter;
	node->next = m_node_list;
	m_node_list = node;
    }
}

void CDemoPointsDlg::OnBnClickedLoadBtn()
{
    CFileDialog f(TRUE);
    if (f.DoModal() == IDOK)
    {
	CStdioFile fin;
	if (fin.Open(f.GetFileName(), CFile::modeRead))
	{
	    CString str;
	    CString token, val;
	    char buffer[1024];
	    int index = 0;
	    double xmin=0, ymin=0, xmax=0, ymax=0;
	    double xcenter=0, ycenter=0, radius=0;
	    int max_iter=0;
	    CExampleNode *node, *list = NULL;
	    while (fin.ReadString(str))
	    {
		str.Trim();
		if (str.GetLength() == 0)
		    continue;
		if (str[0] == '#')
		    continue;
		strcpy(buffer, str);
		xmin=0, ymin=0, xmax=0, ymax=0;
		xcenter=0, ycenter=0, radius=0;
		max_iter=100;
		token = strtok(buffer, " \t\r\n");
		while (token != "")
		{
		    val = strtok(NULL, " \t\r\n");
		    if (token == "-rmin")
			xmin = atof(val);
		    if (token == "-rmax")
			xmax = atof(val);
		    if (token == "-imin")
			ymin = atof(val);
		    if (token == "-imax")
			ymax = atof(val);
		    if (token == "-maxiter")
			max_iter = atoi(val);
		    if (token == "-rcenter")
			xcenter = atof(val);
		    if (token == "-icenter")
			ycenter = atof(val);
		    if (token == "-radius")
			radius = atof(val);
		    token = strtok(NULL, " \t\r\n");
		}
		if (xmin != xmax && ymin != ymax)
		{
		    node = new CExampleNode();
		    node->xmin = xmin;
		    node->xmax = xmax;
		    node->ymin = ymin;
		    node->ymax = ymax;
		    node->max_iter = max_iter;
		    node->next = list;
		    list = node;
		}
		if (radius != 0)
		{
		    node = new CExampleNode();
		    node->xmin = xcenter - radius;
		    node->xmax = xcenter + radius;
		    node->ymin = ycenter - radius;
		    node->ymax = ycenter + radius;
		    node->max_iter = max_iter;
		    node->next = list;
		    list = node;
		}
	    }
	    fin.Close();
	    if (list)
	    {
		node = list;
		while (node)
		{
		    CString str;
		    if (node->max_iter == 0)
			str.Format("%f %f %f %f", node->xmin, node->ymin, node->xmax, node->ymax);
		    else
			str.Format("%f %f %f %f %d", node->xmin, node->ymin, node->xmax, node->ymax, node->max_iter);
		    m_list.AddString(str);
		    if (node->next == NULL)
		    {
			node->next = m_node_list;
			m_node_list = list;
			node = NULL;
		    }
		    else
		    {
			node = node->next;
		    }
		}
	    }
	}
    }
}

/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
// pman_visDoc.cpp : implementation of the Cpman_visDoc class
//

#include "stdafx.h"
#include "pman_vis.h"

#include "pman_visDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// Cpman_visDoc

IMPLEMENT_DYNCREATE(Cpman_visDoc, CDocument)

BEGIN_MESSAGE_MAP(Cpman_visDoc, CDocument)
END_MESSAGE_MAP()


// Cpman_visDoc construction/destruction

Cpman_visDoc::Cpman_visDoc()
{
	// TODO: add one-time construction code here

}

Cpman_visDoc::~Cpman_visDoc()
{
}

BOOL Cpman_visDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	return TRUE;
}




// Cpman_visDoc serialization

void Cpman_visDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}
}


// Cpman_visDoc diagnostics

#ifdef _DEBUG
void Cpman_visDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void Cpman_visDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG


// Cpman_visDoc commands

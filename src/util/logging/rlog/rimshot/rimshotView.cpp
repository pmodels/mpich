/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

// rimshotView.cpp : implementation of the CRimshotView class
//

#include "stdafx.h"
#include "rimshot.h"

#include "rimshotDoc.h"
#include "rimshotView.h"
#include "rimshot_draw.h"
#include "zoomdlg.h"
#include "OffsetDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CRimshotView

IMPLEMENT_DYNCREATE(CRimshotView, CView)

BEGIN_MESSAGE_MAP(CRimshotView, CView)
	//{{AFX_MSG_MAP(CRimshotView)
	ON_COMMAND(ID_NEXT, OnNext)
	ON_COMMAND(ID_PREVIOUS, OnPrevious)
	ON_COMMAND(ID_RESET_ZOOM, OnResetZoom)
	ON_COMMAND(ID_ZOOM_IN, OnZoomIn)
	ON_COMMAND(ID_ZOOM_OUT, OnZoomOut)
	ON_WM_KEYDOWN()
	ON_COMMAND(ID_ZOOM_TO, OnZoomTo)
	ON_WM_SIZE()
	ON_WM_ERASEBKGND()
	ON_COMMAND(ID_TOGGLE_ARROWS, OnToggleArrows)
	ON_WM_LBUTTONDOWN()
	ON_COMMAND(ID_VIEW_UNIFORM, OnViewUniform)
	ON_COMMAND(ID_VIEW_ZOOM, OnViewZoom)
	ON_COMMAND(ID_VIEW_SLIDERANKOFFSET, OnSlideRankOffset)
	//}}AFX_MSG_MAP
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, CView::OnFilePrintPreview)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRimshotView construction/destruction

CRimshotView::CRimshotView()
{
    m_Draw.pCanvas = NULL;
    m_Draw.pBitmap = NULL;
    m_Draw.pCopyCanvas = NULL;
    m_Draw.pCopyBitmap = NULL;
    m_hDrawThread = NULL;
    m_Draw.max_rect_size.cx = 0;
    m_Draw.max_rect_size.cy = 0;
    m_Draw.max_copy_size.cx = 0;
    m_Draw.max_copy_size.cy = 0;
    m_Draw.bDrawUniform = false;
    m_Draw.nUniformWidth = 15;
    m_Draw.pCursorRanks = NULL;
    m_Draw.ppUniRecursionColor = NULL;
    m_Draw.bSliding = false;
    m_Draw.pSlidingBitmap = NULL;
}

CRimshotView::~CRimshotView()
{
    int i;

    if (m_Draw.pCursorRanks)
	delete [] m_Draw.pCursorRanks;
    if (m_Draw.ppUniRecursionColor)
    {
	for (i=0; i<m_Draw.nUniNumRanks; i++)
	{
	    delete m_Draw.ppUniRecursionColor[i];
	}
	delete m_Draw.ppUniRecursionColor;
	m_Draw.ppUniRecursionColor = NULL;
    }
}

BOOL CRimshotView::PreCreateWindow(CREATESTRUCT& cs)
{
    return CView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CRimshotView drawing

void CRimshotView::OnDraw(CDC* pDC)
{
    CRect rect;

    GetClientRect(&rect);
    if (rect.Width() == m_Draw.copy_size.cx && rect.Height() == m_Draw.copy_size.cy)
    {
	pDC->BitBlt(0, 0, rect.Width(), rect.Height(), m_Draw.pCopyCanvas, 0, 0, SRCCOPY);
    }
    else
    {
	pDC->StretchBlt(
	    0, 0, rect.Width(), rect.Height(), 
	    m_Draw.pCopyCanvas, 0, 0, m_Draw.copy_size.cx, m_Draw.copy_size.cy, SRCCOPY);
    }
}

void CRimshotView::DrawToCanvas(CDC *pDC)
{
    CRect rect, big_rect, client_rect;
    RLOG_EVENT event;
    RLOG_ARROW arrow;
    double dx;
    int height, lip;
    CBrush *pBrush;
    int i, j, k, x, y;
    HCURSOR hOldCursor;
    double dwPixel, *pdNextPixel;
    int nMaxRecursions = 0;
    CBrush white_brush;
    CPen line_pen, cursor_pen;
    CString str;
    CSize size;
    double duration;

    CRimshotDoc* pDoc = GetDocument();
    ASSERT_VALID(pDoc);

    GetClientRect(&client_rect);
    GetClientRect(&big_rect);
    big_rect.DeflateRect(25, 25);

    line_pen.CreatePen(PS_SOLID, 1, RGB(100,100,100));
    cursor_pen.CreatePen(PS_SOLID, 1, RGB(255,255,255));

    pDC->FillSolidRect(client_rect, RGB(255,255,255));
    pDC->FillSolidRect(big_rect, RGB(0,0,0));

    if (pDoc->m_pInput)
    {
	hOldCursor = SetCursor( LoadCursor(NULL, IDC_WAIT) );

	dx = ((double)big_rect.right - (double)big_rect.left) / (pDoc->m_dRight - pDoc->m_dLeft);
	height = (big_rect.bottom - big_rect.top) / pDoc->m_pInput->nNumRanks;

	for (i=0; i<pDoc->m_pInput->nNumRanks; i++)
	    nMaxRecursions = max(pDoc->m_pInput->pNumEventRecursions[i], nMaxRecursions);
	lip = (height / 2) / (nMaxRecursions + 1);

	dwPixel = 1.0 / dx;
	pdNextPixel = new double[pDoc->m_pInput->nNumRanks];

	// draw the horizontal rank lines
	pDC->SelectObject(&line_pen);
	for (i=0; i<pDoc->m_pInput->nNumRanks; i++)
	{
	    y = big_rect.top + (height * i) + (height / 2);
	    pDC->MoveTo(big_rect.left, y);
	    pDC->LineTo(big_rect.right, y);
	}

	// draw the event boxes
	for (j=0; j<pDoc->m_pInput->nNumRanks; j++)
	{
	    for (i=0; i<pDoc->m_pInput->pNumEventRecursions[j]; i++)
	    {
		if (RLOG_FindEventBeforeTimestamp(pDoc->m_pInput, 
		    pDoc->m_pInput->header.nMinRank + j, i, pDoc->m_dLeft, &event, NULL) == 0)
		{
		    if (event.end_time < pDoc->m_dLeft)
			RLOG_GetNextEvent(pDoc->m_pInput, pDoc->m_pInput->header.nMinRank + j, i, &event);
		    else
		    {
			if (event.start_time < pDoc->m_dLeft)
			    event.start_time = pDoc->m_dLeft;
		    }
		    for (k=0; k<pDoc->m_pInput->nNumRanks; k++)
			pdNextPixel[k] = event.start_time;
		    while (event.start_time < pDoc->m_dRight)
		    {
			if (event.end_time > pdNextPixel[event.rank])
			{
			    pdNextPixel[event.rank] = event.end_time + dwPixel;
			    rect.top = big_rect.top + (event.rank * height) + (lip * (i+1));
			    rect.bottom = rect.top + height - (lip * (i+1) * 2);
			    rect.left = big_rect.left + (long)(dx * (event.start_time - pDoc->m_dLeft));
			    rect.right = rect.left + (long)(dx * (event.end_time - event.start_time));
			    if (rect.left == rect.right) rect.right++;
			    if (rect.right > big_rect.right)
				rect.right = big_rect.right;
			    
			    pDC->FillSolidRect(rect, pDoc->GetEventColor(event.event));
			}
			
			if (RLOG_GetNextEvent(pDoc->m_pInput, 
			    pDoc->m_pInput->header.nMinRank + j, i, &event) != 0)
			    event.start_time = pDoc->m_dRight + 1;
		    }
		}
	    }
	}

	// draw the arrows
	// This isn't exactly right because I don't think the arrows are stored in end time order.
	if (RLOG_FindArrowBeforeTimestamp(pDoc->m_pInput, pDoc->m_dLeft, &arrow, NULL) == 0)
	{
	    if (arrow.end_time < pDoc->m_dLeft)
		RLOG_GetNextArrow(pDoc->m_pInput, &arrow);
	    //pDC->SelectObject(&line_pen);
	    pDC->SelectObject(GetStockObject(WHITE_PEN));
	    while (arrow.end_time < pDoc->m_dRight)
	    {
		x = big_rect.left + (long)(dx * (arrow.start_time - pDoc->m_dLeft));
		y = big_rect.top + (height * arrow.src) + (height / 2);
		pDC->MoveTo(x, y);
		pDC->Ellipse(x-5, y-5, x+5, y+5);
		x = x + (long)(dx * (arrow.end_time - arrow.start_time));
		y = big_rect.top + (height * arrow.dest) + (height / 2);
		pDC->LineTo(x, y);
		if (RLOG_GetNextArrow(pDoc->m_pInput, &arrow) != 0)
		    arrow.end_time = pDoc->m_dRight + 1;
	    }
	}

	// draw the vertical cursor line
	pDC->SelectObject(&cursor_pen);
	pDC->MoveTo((big_rect.right + big_rect.left) / 2, big_rect.top);
	pDC->LineTo((big_rect.right + big_rect.left) / 2, big_rect.bottom);

	pDC->SetBkMode(OPAQUE);
	pDC->SetBkColor(RGB(255,255,255));
	pDC->SetTextColor(RGB(0,0,0));

	// draw the ranks
	for (i=0; i<pDoc->m_pInput->nNumRanks; i++)
	{
	    y = big_rect.top + (height * i) + (height / 2);
	    str.Format("%d", i);
	    size = pDC->GetTextExtent(str);
	    pDC->TextOut((big_rect.left - size.cx - 7) >= 0 ? (big_rect.left - size.cx - 7) : 0, y - (size.cy / 2), str);
	}

	// draw the box, event description and duration
	if (RLOG_GetCurrentGlobalEvent(pDoc->m_pInput, &event) == 0)
	{
	    pBrush = pDoc->GetEventBrush(event.event);
	    rect.left = (big_rect.left + big_rect.right) / 2;
	    rect.right = rect.left + 13;
	    rect.top = 7;
	    rect.bottom = 20;
	    pDC->FillRect(&rect, pBrush);

	    //str.Format("%3d: %s    duration: %.6f", event.rank, pDoc->GetEventDescription(event.event), event.end_time - event.start_time);
	    //pDC->TextOut(rect.left + 20, 7, str);
	    str.Format("%3d: %s ", event.rank, pDoc->GetEventDescription(event.event));
	    size = pDC->GetTextExtent(str);
	    pDC->TextOut(rect.left + 13, 7, str);
	    //pDC->TextOut(rect.left - size.cx, 7, str);

	    duration = event.end_time - event.start_time;
	    str.Format("%.9f", duration);
	    size = pDC->GetTextExtent(str);
	    x = rect.left - 7 - size.cx;
	    // write the milliseconds
	    str.Format("%.3f", duration);
	    size = pDC->GetTextExtent(str);
	    CRgn rgn;
	    rgn.CreateRectRgn(x, 7, x + size.cx, 7 + size.cy);
	    pDC->SelectClipRgn(&rgn, RGN_COPY);
	    str.Format("%.6f", duration);
	    pDC->SetTextColor(RGB(0,0,0));
	    pDC->TextOut(x, 7, str);
	    // write the microseconds
	    str.Format("%.6f", duration);
	    size = pDC->GetTextExtent(str);
	    rgn.CreateRectRgnIndirect(&client_rect);
	    pDC->SelectClipRgn(&rgn, RGN_XOR);
	    rgn.CreateRectRgn(x, 7, x + size.cx, 7 + size.cy);
	    str.Format("%.9f", duration);
	    pDC->SetTextColor(RGB(255,0,0));
	    pDC->TextOut(x, 7, str);
	    // write the nanoseconds
	    pDC->SelectClipRgn(&rgn, RGN_COPY);
	    rgn.CreateRectRgnIndirect(&client_rect);
	    pDC->SelectClipRgn(&rgn, RGN_XOR);
	    str.Format("%.9f", duration);
	    pDC->SetTextColor(RGB(0,0,255));
	    pDC->TextOut(x, 7, str);
	    pDC->SetTextColor(RGB(0,0,0));

	    rect.top = big_rect.top + (event.rank * height) + (lip * (event.recursion+1));
	    rect.bottom = rect.top + height - (lip * (event.recursion+1) * 2);
	    rect.left = big_rect.left + (long)(dx * (event.start_time - pDoc->m_dLeft));
	    rect.right = rect.left + (long)(dx * (event.end_time - event.start_time));
	    if (rect.left == rect.right) rect.right++;
	    if (rect.right > big_rect.right)
		rect.right = big_rect.right;
	    white_brush.CreateStockObject(WHITE_BRUSH);
	    pDC->FrameRect(&rect, &white_brush);

	    // draw the current start time
	    str.Format("%.0f", event.start_time);
	    size = pDC->GetTextExtent(str);
	    x = ((big_rect.left + big_rect.right) / 2) - size.cx;
	    y = big_rect.bottom + 7;
	    str.Format("%.3f", event.start_time);
	    size = pDC->GetTextExtent(str);
	    rgn.CreateRectRgn(x, y, x + size.cx, y + size.cy);
	    pDC->SelectClipRgn(&rgn, RGN_COPY);
	    str.Format("%.6f", event.start_time);
	    pDC->SetTextColor(RGB(0,0,0));
	    pDC->TextOut(x, y, str);
	    rgn.CreateRectRgnIndirect(&client_rect);
	    pDC->SelectClipRgn(&rgn, RGN_XOR);
	    pDC->SetTextColor(RGB(255,0,0));
	    pDC->TextOut(x, y, str);
	    pDC->SetTextColor(RGB(0,0,0));
	}
	else
	{
	    str.Format("%.6f", (pDoc->m_dLeft + pDoc->m_dRight) / 2.0);
	    size = pDC->GetTextExtent(str);
	    pDC->TextOut(((big_rect.left + big_rect.right) / 2) - (size.cx / 2), big_rect.bottom + 7, str);
	}

	// draw the left and right timestamps
	str.Format("%.6f", pDoc->m_dLeft);
	pDC->TextOut(big_rect.left, big_rect.bottom + 7, str);
	str.Format("%.6f", pDoc->m_dRight);
	size = pDC->GetTextExtent(str);
	pDC->TextOut(big_rect.right - size.cx, big_rect.bottom + 7, str);

	delete pdNextPixel;
	SetCursor(hOldCursor);
    }
}

/////////////////////////////////////////////////////////////////////////////
// CRimshotView printing

BOOL CRimshotView::OnPreparePrinting(CPrintInfo* pInfo)
{
    return DoPreparePrinting(pInfo);
}

void CRimshotView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
}

void CRimshotView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
}

void CRimshotView::OnPrepareDC(CDC* pDC, CPrintInfo* pInfo) 
{
    CView::OnPrepareDC(pDC, pInfo);
    
    if (pDC->IsPrinting())
    {
	/*
	CRect rect;
	GetClientRect (&rect);
	
	int oldMapMode = pDC->SetMapMode(MM_ISOTROPIC);
	CSize ptOldWinExt = pDC->SetWindowExt(1000, 1000);
	CSize ptOldViewportExt = pDC->SetViewportExt(rect.Width(), -rect.Height());
	CPoint ptOldOrigin = pDC->SetViewportOrg(rect.Width()/2, rect.Height()/2);
	*/
    }
}

/////////////////////////////////////////////////////////////////////////////
// CRimshotView diagnostics

#ifdef _DEBUG
void CRimshotView::AssertValid() const
{
	CView::AssertValid();
}

void CRimshotView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CRimshotDoc* CRimshotView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CRimshotDoc)));
	return (CRimshotDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CRimshotView message handlers

void CRimshotView::OnNext() 
{
    double d;
    RLOG_EVENT event1, event2;
    CRimshotDoc* pDoc = GetDocument();

    StopDrawing();
    d = (pDoc->m_dRight - pDoc->m_dLeft) / 6.0;
    RLOG_GetCurrentGlobalEvent(pDoc->m_pInput, &event1);
    RLOG_FindGlobalEventBeforeTimestamp(pDoc->m_pInput, (pDoc->m_dLeft + pDoc->m_dRight) / 2.0 + d, &event2);
    if (event1.start_time == event2.start_time)
	RLOG_GetNextGlobalEvent(pDoc->m_pInput, &event2);
    pDoc->m_dLeft = event2.start_time - (3.0 * d);
    pDoc->m_dRight = event2.start_time + (3.0 * d);
    StartDrawing();
}

void CRimshotView::OnPrevious() 
{
    double d;
    RLOG_EVENT event;
    CRimshotDoc* pDoc = GetDocument();

    StopDrawing();
    d = (pDoc->m_dRight - pDoc->m_dLeft) / 6.0;
    RLOG_FindGlobalEventBeforeTimestamp(pDoc->m_pInput, (pDoc->m_dLeft + pDoc->m_dRight) / 2.0 - d, &event);
    pDoc->m_dLeft = event.start_time - (3.0 * d);
    pDoc->m_dRight = event.start_time + (3.0 * d);
    StartDrawing();
}

void CRimshotView::OnResetZoom() 
{
    CRimshotDoc* pDoc = GetDocument();
    StopDrawing();
    pDoc->m_dLeft = pDoc->m_dFirst;
    pDoc->m_dRight = pDoc->m_dLast;
    StartDrawing();
}

void CRimshotView::OnZoomIn() 
{
    double d;
    CRimshotDoc* pDoc = GetDocument();

    StopDrawing();
    d = (pDoc->m_dRight - pDoc->m_dLeft) / 4.0;
    pDoc->m_dLeft += d;
    pDoc->m_dRight -= d;
    StartDrawing();
}

void CRimshotView::OnZoomOut() 
{
    double d;
    CRimshotDoc* pDoc = GetDocument();

    StopDrawing();
    d = (pDoc->m_dRight - pDoc->m_dLeft) / 2.0;
    pDoc->m_dLeft -= d;
    pDoc->m_dRight += d;
    StartDrawing();
}

void CRimshotView::OnZoomTo() 
{
    CRect rect;
    RLOG_EVENT event;
    CRimshotDoc* pDoc = GetDocument();
    double width;

    if (pDoc->m_pInput)
    {
	StopDrawing();
	RLOG_GetCurrentGlobalEvent(pDoc->m_pInput, &event);
	GetClientRect(&rect);
	width = event.end_time - event.start_time;
	pDoc->m_dLeft = event.start_time - (10.0 * width);
	pDoc->m_dRight = event.start_time + (10.0 * width);
	StartDrawing();
    }
}

void CRimshotView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
    RLOG_EVENT event;
    double d;
    CRimshotDoc* pDoc = GetDocument();

    if (pDoc->m_pInput)
    {
	switch (nChar)
	{
	case VK_RIGHT:
	    if (RLOG_GetNextGlobalEvent(pDoc->m_pInput, &event) == 0)
	    {
		while (!m_Draw.pCursorRanks[event.rank].active)
		{
		    if (RLOG_GetNextGlobalEvent(pDoc->m_pInput, &event) != 0)
			break;
		}
		StopDrawing();
		d = (pDoc->m_dRight - pDoc->m_dLeft) / 2;
		pDoc->m_dLeft = event.start_time - d;
		pDoc->m_dRight = event.start_time + d;
		StartDrawing();
	    }
	    break;
	case VK_LEFT:
	    if (RLOG_GetPreviousGlobalEvent(pDoc->m_pInput, &event) == 0)
	    {
		while (!m_Draw.pCursorRanks[event.rank].active)
		{
		    if (RLOG_GetPreviousGlobalEvent(pDoc->m_pInput, &event) != 0)
			break;
		}
		StopDrawing();
		d = (pDoc->m_dRight - pDoc->m_dLeft) / 2;
		pDoc->m_dLeft = event.start_time - d;
		pDoc->m_dRight = event.start_time + d;
		StartDrawing();
	    }
	    break;
	default:
	    break;
	}
    }

    CView::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CRimshotView::OnSize(UINT nType, int cx, int cy) 
{
    CView::OnSize(nType, cx, cy);

    if (m_Draw.pCanvas != NULL)
    {
	// stop any current drawing
	StopDrawing();

	if (m_Draw.max_rect_size.cx < cx || m_Draw.max_rect_size.cy < cy)
	{
	    m_Draw.max_rect_size.cx = max(m_Draw.max_rect_size.cx, cx);
	    m_Draw.max_rect_size.cy = max(m_Draw.max_rect_size.cy, cy);
	    // create a new resized canvas bitmap
	    CBitmap *pBmp = new CBitmap();
	    pBmp->CreateDiscardableBitmap(GetDC(), m_Draw.max_rect_size.cx, m_Draw.max_rect_size.cy);
	    
	    if (m_Draw.pBitmap)
	    {
		m_Draw.pCanvas->SelectObject(pBmp);
		delete m_Draw.pBitmap;
		m_Draw.pBitmap = pBmp;
	    }
	    else
	    {
		m_Draw.pOriginalBmp = m_Draw.pCanvas->SelectObject(pBmp);
		m_Draw.pBitmap = pBmp;
	    }
	}
	m_Draw.rect_size.cx = cx;
	m_Draw.rect_size.cy = cy;

	// create the copy bitmap if this is the first size event
	if (m_Draw.pCopyBitmap == NULL)
	{
	    CBitmap *pBmpCopy = new CBitmap();
	    pBmpCopy->CreateDiscardableBitmap(GetDC(), cx, cy);

	    m_Draw.pCopyOriginalBmp = m_Draw.pCopyCanvas->SelectObject(pBmpCopy);
	    m_Draw.pCopyBitmap = pBmpCopy;
	    m_Draw.copy_size.cx = cx;
	    m_Draw.copy_size.cy = cy;
	    m_Draw.max_copy_size.cx = cx;
	    m_Draw.max_copy_size.cy = cy;
	}

	// start redrawing
	StartDrawing();
    }
}

void CRimshotView::OnInitialUpdate() 
{
    DWORD dwThreadID;
    CRimshotDoc* pDoc = GetDocument();
    CView::OnInitialUpdate();

    m_Draw.pCanvas = new CDC();
    m_Draw.pCanvas->CreateCompatibleDC(GetDC());
    m_Draw.pCanvas->SetStretchBltMode(COLORONCOLOR);
    m_Draw.pCopyCanvas = new CDC();
    m_Draw.pCopyCanvas->CreateCompatibleDC(GetDC());
    m_Draw.pCopyCanvas->SetStretchBltMode(COLORONCOLOR);
    m_Draw.hWnd = m_hWnd;
    m_Draw.pDoc = pDoc;
    m_Draw.hDrawEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    m_Draw.hStoppedEvent = CreateEvent(NULL, TRUE, TRUE, NULL);
    m_Draw.hMutex = CreateMutex(NULL, FALSE, NULL);
    m_Draw.bStop = false;
    m_Draw.nCmd = -1;
    m_Draw.bDrawArrows = true;
    m_hDrawThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)RimshotDrawThread, &m_Draw, 0, &dwThreadID);
}

BOOL CRimshotView::DestroyWindow() 
{
    StopDrawing();
    m_Draw.nCmd = EXIT_CMD;
    SetEvent(m_Draw.hDrawEvent);
    WaitForSingleObject(m_hDrawThread, 10000);
    CloseHandle(m_Draw.hDrawEvent);
    CloseHandle(m_Draw.hStoppedEvent);
    CloseHandle(m_hDrawThread);

    if (m_Draw.pCanvas)
    {
	if (m_Draw.pBitmap)
	{
	    // select out the bitmap
	    m_Draw.pCanvas->SelectObject(m_Draw.pOriginalBmp);
	}
	delete m_Draw.pCanvas;
	m_Draw.pCanvas = NULL;
    }
    if (m_Draw.pBitmap)
    {
	delete m_Draw.pBitmap;
	m_Draw.pBitmap = NULL;
    }

    if (m_Draw.pCopyCanvas)
    {
	if (m_Draw.pCopyBitmap)
	{
	    m_Draw.pCopyCanvas->SelectObject(m_Draw.pCopyOriginalBmp);
	}
	delete m_Draw.pCopyCanvas;
	m_Draw.pCopyCanvas = NULL;
    }
    if (m_Draw.pCopyBitmap)
    {
	delete m_Draw.pCopyBitmap;
	m_Draw.pCopyBitmap = NULL;
    }

    return CView::DestroyWindow();
}

LRESULT CRimshotView::WindowProc(UINT message, WPARAM wParam, LPARAM lParam) 
{
    CRect rect;
    CBitmap *pBmp;

    switch (message)
    {
    case DRAW_COMPLETE_MSG:
	GetClientRect(&rect);
	if (m_Draw.max_copy_size.cx < rect.Width() || m_Draw.max_copy_size.cy < rect.Height())
	{
	    m_Draw.max_copy_size.cx = max(m_Draw.max_copy_size.cx, rect.Width());
	    m_Draw.max_copy_size.cy = max(m_Draw.max_copy_size.cy, rect.Height());
	    pBmp = new CBitmap();
	    pBmp->CreateDiscardableBitmap(GetDC(), m_Draw.max_copy_size.cx, m_Draw.max_copy_size.cy);
	    m_Draw.pCopyCanvas->SelectObject(pBmp);
	    delete m_Draw.pCopyBitmap;
	    m_Draw.pCopyBitmap = pBmp;
	}
	m_Draw.copy_size.cx = rect.Width();
	m_Draw.copy_size.cy = rect.Height();
	if (WaitForSingleObject(m_Draw.hMutex, 2000) == WAIT_OBJECT_0)
	{
	    m_Draw.pCopyCanvas->BitBlt(0, 0, rect.Width(), rect.Height(), m_Draw.pCanvas, 0, 0, SRCCOPY);
	    ReleaseMutex(m_Draw.hMutex);
	}
	Invalidate(FALSE);
	break;
    }
    return CView::WindowProc(message, wParam, lParam);
}

void CRimshotView::StopDrawing()
{
    m_Draw.bStop = true;
    WaitForSingleObject(m_Draw.hStoppedEvent, 10000);
    m_Draw.bStop = false;
    ResetEvent(m_Draw.hStoppedEvent);
}

void CRimshotView::StartDrawing()
{
    m_Draw.nCmd = REDRAW_CMD;
    SetEvent(m_Draw.hDrawEvent);
    Invalidate(FALSE);
}

BOOL CRimshotView::OnEraseBkgnd(CDC* pDC) 
{
    return TRUE;
}

void CRimshotView::OnToggleArrows() 
{
    StopDrawing();
    m_Draw.bDrawArrows = !m_Draw.bDrawArrows;
    StartDrawing();
}

#define PointInside(point, rect) (point.x >= rect.left && point.x <= rect.right && point.y >= rect.top && point.y <= rect.bottom)

void CRimshotView::OnLButtonDown(UINT nFlags, CPoint point) 
{
    bool bHit = false;
    CRimshotDoc* pDoc = GetDocument();
    if (pDoc->m_pInput)
    {
	for (int i=0; i<pDoc->m_pInput->nNumRanks; i++)
	{
	    if (PointInside(point, m_Draw.pCursorRanks[i].rect))
	    {
		m_Draw.pCursorRanks[i].active = !m_Draw.pCursorRanks[i].active;
		//InvalidateRect(&m_Draw.pCursorRanks[i].rect);
		bHit = true;
	    }
	}
    }
    if (bHit)
    {
	StopDrawing();
	StartDrawing();
    }
    CView::OnLButtonDown(nFlags, point);
}

void CRimshotView::OnViewUniform() 
{
    CMenu *pMenu = GetParentFrame()->GetMenu();

    m_Draw.bDrawUniform = !m_Draw.bDrawUniform;

    if (pMenu)
	pMenu->CheckMenuItem(ID_VIEW_UNIFORM, MF_BYCOMMAND | m_Draw.bDrawUniform ? MF_CHECKED : MF_UNCHECKED);
    StopDrawing();
    StartDrawing();
}

void CRimshotView::OnViewZoom() 
{
    CRimshotDoc* pDoc = GetDocument();
    if (pDoc->m_pInput)
    {
	CZoomDlg dlg;
	CRect rect;
	
	GetClientRect(&rect);
	dlg.m_dLeft = pDoc->m_dLeft;
	dlg.m_dRight = pDoc->m_dRight;
	dlg.m_dWidth = pDoc->m_dRight - pDoc->m_dLeft;
	dlg.m_dPerPixel = dlg.m_dWidth / (double)rect.Width();

	if (dlg.DoModal() == IDOK)
	{
	    if (dlg.m_bLR)
	    {
		if (dlg.m_dLeft < dlg.m_dRight)
		{
		    RLOG_EVENT event;
		    pDoc->m_dLeft = dlg.m_dLeft;
		    pDoc->m_dRight = dlg.m_dRight;
		    RLOG_FindGlobalEventBeforeTimestamp(pDoc->m_pInput, (dlg.m_dLeft + dlg.m_dRight) / 2.0, &event);
		    StopDrawing();
		    StartDrawing();
		}
	    }
	    if (dlg.m_bWidth)
	    {
		if (dlg.m_dWidth > 0.0)
		{
		    double middle = (pDoc->m_dLeft + pDoc->m_dRight) / 2.0;
		    pDoc->m_dLeft = middle - (dlg.m_dWidth / 2.0);
		    pDoc->m_dRight = middle + (dlg.m_dWidth / 2.0);
		    StopDrawing();
		    StartDrawing();
		}
	    }
	    if (dlg.m_bPerPixel)
	    {
		dlg.m_dWidth = dlg.m_dPerPixel * (double)rect.Width();
		if (dlg.m_dWidth > 0.0)
		{
		    double middle = (pDoc->m_dLeft + pDoc->m_dRight) / 2.0;
		    pDoc->m_dLeft = middle - (dlg.m_dWidth / 2.0);
		    pDoc->m_dRight = middle + (dlg.m_dWidth / 2.0);
		    StopDrawing();
		    StartDrawing();
		}
	    }
	}
    }
}

void CRimshotView::OnSlideRankOffset() 
{
    CRimshotDoc* pDoc = GetDocument();
    if (pDoc->m_pInput)
    {
	COffsetDlg dlg;

	if (dlg.DoModal() == IDOK)
	{
	    if (dlg.m_row < pDoc->m_pInput->nNumRanks)
	    {
		if (dlg.m_bUseGUI)
		{
		    m_Draw.bSliding = true;
		    m_Draw.nSlidingRank = dlg.m_row;
		    // setup bitmap and stuff
		}
		else
		{
		    // do file offset stuff
		}
	    }
	}
    }
}

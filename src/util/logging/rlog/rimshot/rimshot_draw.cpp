/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "stdafx.h"
#include "rimshot.h"
#include "rimshotDoc.h"
#include "rimshotView.h"
#include "rimshot_draw.h"

void RimshotDrawThread(RimshotDrawStruct *pArg)
{
    CRect rect, big_rect, client_rect;
    RLOG_EVENT event;
    RLOG_ARROW arrow;
    double dx;
    int height, lip;
    CBrush *pBrush;
    int i, j, k, x, y;
    double dwPixel, *pdNextPixel;
    int nMaxRecursions = 0;
    CBrush white_brush;
    HPEN line_pen, cursor_pen, hOldPen;
    CString str;
    CSize size;
    double duration;
    double dLeft, dRight;

    while (true)
    {
	WaitForSingleObject(pArg->hDrawEvent, INFINITE);
	ResetEvent(pArg->hDrawEvent);
	switch (pArg->nCmd)
	{
	case EXIT_CMD:
	    ExitThread(0);
	    break;
	case REDRAW_CMD:
	    if (WaitForSingleObject(pArg->hMutex, 10000) != WAIT_OBJECT_0)
		break;
	    client_rect.SetRect(0, 0, pArg->rect_size.cx, pArg->rect_size.cy);
	    big_rect.SetRect(0, 0, pArg->rect_size.cx, pArg->rect_size.cy);
	    
	    big_rect.DeflateRect(25, 25);
	    
	    line_pen = CreatePen(PS_SOLID, 1, RGB(100,100,100));
	    cursor_pen = CreatePen(PS_SOLID, 1, RGB(255,255,255));
	    
	    pArg->pCanvas->FillSolidRect(0,0,client_rect.Width(), big_rect.top, RGB(255,255,255));
	    if (pArg->bStop)
		break;
	    pArg->pCanvas->FillSolidRect(0,big_rect.bottom, client_rect.Width(), big_rect.top, RGB(255,255,255));
	    if (pArg->bStop)
		break;
	    pArg->pCanvas->FillSolidRect(0,0,big_rect.left, client_rect.Height(), RGB(255,255,255));
	    if (pArg->bStop)
		break;
	    pArg->pCanvas->FillSolidRect(big_rect.right, 0, big_rect.left, client_rect.Height(), RGB(255,255,255));
	    if (pArg->bStop)
		break;
	    pArg->pCanvas->FillSolidRect(big_rect, RGB(0,0,0));
	    if (pArg->bStop)
		break;
	    
	    if (pArg->pDoc->m_pInput)
	    {
		dx = ((double)big_rect.right - (double)big_rect.left) / (pArg->pDoc->m_dRight - pArg->pDoc->m_dLeft);
		height = (big_rect.bottom - big_rect.top) / pArg->pDoc->m_pInput->nNumRanks;
		
		for (i=0; i<pArg->pDoc->m_pInput->nNumRanks; i++)
		    nMaxRecursions = max(pArg->pDoc->m_pInput->pNumEventRecursions[i], nMaxRecursions);
		lip = (height / 2) / (nMaxRecursions + 1);
		
		dwPixel = 1.0 / dx;
		pdNextPixel = new double[pArg->pDoc->m_pInput->nNumRanks];
		
		// draw the horizontal rank lines
		hOldPen = (HPEN) pArg->pCanvas->SelectObject(line_pen);
		for (i=0; i<pArg->pDoc->m_pInput->nNumRanks; i++)
		{
		    y = big_rect.top + (height * i) + (height / 2);
		    pArg->pCanvas->MoveTo(big_rect.left, y);
		    pArg->pCanvas->LineTo(big_rect.right, y);
		}
		
		if (pArg->bDrawUniform)
		{
		    int num_moved = 0;
		    int num_left = 0;
		    int cur_pos;

		    // blacken the recursion colors
		    for (i=0; i<pArg->pDoc->m_pInput->nNumRanks; i++)
		    {
			for (j=0; j<pArg->pDoc->m_pInput->pNumEventRecursions[i]; j++)
			    pArg->ppUniRecursionColor[i][j] = RGB(0,0,0);
		    }

		    // move to the left
		    dLeft = 0.0;
		    cur_pos = (big_rect.left + big_rect.right) / 2;
		    while (cur_pos > big_rect.left)
		    {
			if (RLOG_GetPreviousGlobalEvent(pArg->pDoc->m_pInput, &event) != 0)
			    break;
			num_left++;
			cur_pos -= pArg->nUniformWidth;
			if (pArg->bStop)
			    break;
		    }
		    dLeft = event.start_time;

		    // prime the color pump
		    for (i=0; i<50; i++)
		    {
			if (RLOG_GetPreviousGlobalEvent(pArg->pDoc->m_pInput, &event) != 0)
			    break;
		    }
		    for (; i>0; i--)
		    {
			if (RLOG_GetNextGlobalEvent(pArg->pDoc->m_pInput, &event) != 0)
			    break;
			pArg->ppUniRecursionColor[event.rank][event.recursion] = pArg->pDoc->GetEventColor(event.event);
		    }

		    // draw to the right
		    while (cur_pos < big_rect.right)
		    {
			pArg->ppUniRecursionColor[event.rank][event.recursion] = pArg->pDoc->GetEventColor(event.event);
			for (i=0; i<=event.recursion; i++)
			{
			    //rect.top = big_rect.top + (event.rank * height) + (lip * (event.recursion + 1));
			    //rect.bottom = rect.top + height - (lip * (event.recursion + 1) * 2);
			    rect.top = big_rect.top + (event.rank * height) + (lip * (i + 1));
			    rect.bottom = rect.top + height - (lip * (i + 1) * 2);
			    rect.left = cur_pos;
			    rect.right = cur_pos + pArg->nUniformWidth;
			    if (rect.left < big_rect.left)
				rect.left = big_rect.left;
			    if (rect.right > big_rect.right)
				rect.right = big_rect.right;
			    //pArg->pCanvas->FillSolidRect(rect, pArg->pDoc->GetEventColor(event.event));
			    pArg->pCanvas->FillSolidRect(rect, pArg->ppUniRecursionColor[event.rank][i]);
			}
			if (pArg->bStop)
			    break;
			if (RLOG_GetNextGlobalEvent(pArg->pDoc->m_pInput, &event) != 0)
			    break;
			num_moved++;
			cur_pos += pArg->nUniformWidth;
		    }
		    dRight = event.end_time;

		    // return to the middle
		    for (i=0; i<num_moved - num_left; i++)
			RLOG_GetPreviousGlobalEvent(pArg->pDoc->m_pInput, &event);
		    if (pArg->bStop)
			break;

		}
		else
		{
		    dLeft = pArg->pDoc->m_dLeft;
		    dRight = pArg->pDoc->m_dRight;

		    // draw the event boxes
		    for (j=0; j<pArg->pDoc->m_pInput->nNumRanks; j++)
		    {
			if (pArg->bStop)
			    break;
			for (i=0; i<pArg->pDoc->m_pInput->pNumEventRecursions[j]; i++)
			{
			    if (pArg->bStop)
				break;
			    if (RLOG_FindEventBeforeTimestamp(pArg->pDoc->m_pInput, 
				pArg->pDoc->m_pInput->header.nMinRank + j, i, pArg->pDoc->m_dLeft, &event, NULL) == 0)
			    {
				if (event.end_time < pArg->pDoc->m_dLeft)
				    RLOG_GetNextEvent(pArg->pDoc->m_pInput, pArg->pDoc->m_pInput->header.nMinRank + j, i, &event);
				else
				{
				    if (event.start_time < pArg->pDoc->m_dLeft)
					event.start_time = pArg->pDoc->m_dLeft;
				}
				for (k=0; k<pArg->pDoc->m_pInput->nNumRanks; k++)
				    pdNextPixel[k] = event.start_time;
				while (event.start_time < pArg->pDoc->m_dRight)
				{
				    if (event.end_time > pdNextPixel[event.rank])
				    {
					pdNextPixel[event.rank] = event.end_time + dwPixel;
					rect.top = big_rect.top + (event.rank * height) + (lip * (i+1));
					rect.bottom = rect.top + height - (lip * (i+1) * 2);
					rect.left = big_rect.left + (long)(dx * (event.start_time - pArg->pDoc->m_dLeft));
					rect.right = rect.left + (long)(dx * (event.end_time - event.start_time));
					if (rect.left == rect.right) rect.right++;
					if (rect.right > big_rect.right)
					    rect.right = big_rect.right;
					
					pArg->pCanvas->FillSolidRect(rect, pArg->pDoc->GetEventColor(event.event));
				    }
				    
				    if (RLOG_GetNextEvent(pArg->pDoc->m_pInput, 
					pArg->pDoc->m_pInput->header.nMinRank + j, i, &event) != 0)
					event.start_time = pArg->pDoc->m_dRight + 1;
				}
			    }
			}
		    }
		    if (pArg->bStop)
			break;
		    
		    if (pArg->bDrawArrows)
		    {
			// draw the arrows
			if (RLOG_FindArrowBeforeTimestamp(pArg->pDoc->m_pInput, pArg->pDoc->m_dLeft, &arrow, NULL) == 0)
			{
			    if (arrow.end_time < pArg->pDoc->m_dLeft)
				RLOG_GetNextArrow(pArg->pDoc->m_pInput, &arrow);
			    pArg->pCanvas->SelectObject(GetStockObject(WHITE_PEN));
			    while (arrow.start_time < pArg->pDoc->m_dRight)
			    {
				if (pArg->bStop)
				    break;
				if (arrow.leftright == RLOG_ARROW_LEFT)
				{
				    x = big_rect.left + (long)(dx * (arrow.start_time - pArg->pDoc->m_dLeft));
				    y = big_rect.top + (height * arrow.dest) + (height / 2);
				    pArg->pCanvas->MoveTo(x, y);
				    x = x + (long)(dx * (arrow.end_time - arrow.start_time));
				    y = big_rect.top + (height * arrow.src) + (height / 2);
				    pArg->pCanvas->LineTo(x, y);
				    pArg->pCanvas->Ellipse(x-5, y-5, x+5, y+5);
				}
				else
				{
				    x = big_rect.left + (long)(dx * (arrow.start_time - pArg->pDoc->m_dLeft));
				    y = big_rect.top + (height * arrow.src) + (height / 2);
				    pArg->pCanvas->Ellipse(x-5, y-5, x+5, y+5);
				    pArg->pCanvas->MoveTo(x, y);
				    x = x + (long)(dx * (arrow.end_time - arrow.start_time));
				    y = big_rect.top + (height * arrow.dest) + (height / 2);
				    pArg->pCanvas->LineTo(x, y);
				}
				if (RLOG_GetNextArrow(pArg->pDoc->m_pInput, &arrow) != 0)
				    break;
			    }
			}
		    }
		}
		
		// draw the vertical cursor line
		pArg->pCanvas->SelectObject(cursor_pen);
		pArg->pCanvas->MoveTo((big_rect.right + big_rect.left) / 2, big_rect.top);
		pArg->pCanvas->LineTo((big_rect.right + big_rect.left) / 2, big_rect.bottom);

		pArg->pCanvas->SelectObject(hOldPen);
		DeleteObject(cursor_pen);
		DeleteObject(line_pen);

		pArg->pCanvas->SetBkMode(OPAQUE);
		pArg->pCanvas->SetBkColor(RGB(255,255,255));
		pArg->pCanvas->SetTextColor(RGB(0,0,0));
		
		// draw the ranks
		for (i=0; i<pArg->pDoc->m_pInput->nNumRanks; i++)
		{
		    y = big_rect.top + (height * i) + (height / 2);
		    str.Format("%d", i);
		    size = pArg->pCanvas->GetTextExtent(str);
		    //pArg->pCanvas->GetOutputTextExtent(str);
		    
		    pArg->pCursorRanks[i].rect.left = (big_rect.left - size.cx - 7) >= 0 ? (big_rect.left - size.cx - 7) : 0;
		    pArg->pCursorRanks[i].rect.top = y - (size.cy / 2);
		    pArg->pCursorRanks[i].rect.right = pArg->pCursorRanks[i].rect.left + size.cx;
		    pArg->pCursorRanks[i].rect.bottom = pArg->pCursorRanks[i].rect.top + size.cy;
		    pArg->pCursorRanks[i].rect.InflateRect(5,5);
		    if (pArg->pCursorRanks[i].active)
		    {
			pArg->pCanvas->Rectangle(&pArg->pCursorRanks[i].rect);
		    }

		    pArg->pCanvas->TextOut(
			(big_rect.left - size.cx - 7) >= 0 ? (big_rect.left - size.cx - 7) : 0, 
			y - (size.cy / 2), 
			str);
		}
		
		// draw the box, event description and duration
		if (RLOG_GetCurrentGlobalEvent(pArg->pDoc->m_pInput, &event) == 0)
		{
		    pBrush = pArg->pDoc->GetEventBrush(event.event);
		    rect.left = (big_rect.left + big_rect.right) / 2;
		    rect.right = rect.left + 13;
		    rect.top = 7;
		    rect.bottom = 20;
		    if (pBrush != NULL)
			pArg->pCanvas->FillRect(&rect, pBrush);
		    else
		    {
			CBrush brush;
			brush.CreateSolidBrush(RGB(0,0,0));
			pArg->pCanvas->FillRect(&rect, &brush);
		    }
		    //pArg->pCanvas->FillSolidRect(&rect, pArg->pDoc->GetEventColor(event.event));
		    
		    //str.Format("%3d: %s    duration: %.6f", event.rank, pDoc->GetEventDescription(event.event), event.end_time - event.start_time);
		    //pCanvas->TextOut(rect.left + 20, 7, str);
		    str.Format("%3d: %s ", event.rank, pArg->pDoc->GetEventDescription(event.event));
		    size = pArg->pCanvas->GetTextExtent(str);
		    pArg->pCanvas->TextOut(rect.left + 13, 7, str);
		    //pCanvas->TextOut(rect.left - size.cx, 7, str);
		    
		    duration = event.end_time - event.start_time;
		    str.Format("%.9f", duration);
		    size = pArg->pCanvas->GetTextExtent(str);
		    x = rect.left - 7 - size.cx;
		    // write the milliseconds
		    str.Format("%.3f", duration);
		    size = pArg->pCanvas->GetTextExtent(str);
		    CRgn rgn;
		    rgn.CreateRectRgn(x, 7, x + size.cx, 7 + size.cy);
		    pArg->pCanvas->SelectClipRgn(&rgn, RGN_COPY);
		    str.Format("%.6f", duration);
		    pArg->pCanvas->SetTextColor(RGB(0,0,0));
		    pArg->pCanvas->TextOut(x, 7, str);
		    // write the microseconds
		    str.Format("%.6f", duration);
		    size = pArg->pCanvas->GetTextExtent(str);
		    rgn.DeleteObject();
		    CRgn rgn2;
		    rgn2.CreateRectRgnIndirect(&client_rect);
		    pArg->pCanvas->SelectClipRgn(&rgn2, RGN_XOR);
		    rgn2.DeleteObject();
		    CRgn rgn3;
		    rgn3.CreateRectRgn(x, 7, x + size.cx, 7 + size.cy);
		    str.Format("%.9f", duration);
		    pArg->pCanvas->SetTextColor(RGB(255,0,0));
		    pArg->pCanvas->TextOut(x, 7, str);
		    // write the nanoseconds
		    pArg->pCanvas->SelectClipRgn(&rgn3, RGN_COPY);
		    rgn3.DeleteObject();
		    CRgn rgn4;
		    rgn4.CreateRectRgnIndirect(&client_rect);
		    pArg->pCanvas->SelectClipRgn(&rgn4, RGN_XOR);
		    str.Format("%.9f", duration);
		    pArg->pCanvas->SetTextColor(RGB(0,0,255));
		    pArg->pCanvas->TextOut(x, 7, str);
		    pArg->pCanvas->SetTextColor(RGB(0,0,0));
		    
		    rect.top = big_rect.top + (event.rank * height) + (lip * (event.recursion+1));
		    rect.bottom = rect.top + height - (lip * (event.recursion+1) * 2);
		    rect.left = big_rect.left + (long)(dx * (event.start_time - pArg->pDoc->m_dLeft));
		    rect.right = rect.left + (long)(dx * (event.end_time - event.start_time));
		    if (rect.left == rect.right) rect.right++;
		    if (rect.right > big_rect.right)
			rect.right = big_rect.right;
		    white_brush.CreateStockObject(WHITE_BRUSH);
		    pArg->pCanvas->FrameRect(&rect, &white_brush);
		    
		    // draw the current start time
		    str.Format("%.0f", event.start_time);
		    size = pArg->pCanvas->GetTextExtent(str);
		    x = ((big_rect.left + big_rect.right) / 2) - size.cx;
		    y = big_rect.bottom + 7;
		    str.Format("%.3f", event.start_time);
		    size = pArg->pCanvas->GetTextExtent(str);
		    rgn4.DeleteObject();
		    CRgn rgn5;
		    rgn5.CreateRectRgn(x, y, x + size.cx, y + size.cy);
		    pArg->pCanvas->SelectClipRgn(&rgn5, RGN_COPY);
		    str.Format("%.6f", event.start_time);
		    pArg->pCanvas->SetTextColor(RGB(0,0,0));
		    pArg->pCanvas->TextOut(x, y, str);
		    rgn5.DeleteObject();
		    CRgn rgn6;
		    rgn6.CreateRectRgnIndirect(&client_rect);
		    pArg->pCanvas->SelectClipRgn(&rgn6, RGN_XOR);
		    pArg->pCanvas->SetTextColor(RGB(255,0,0));
		    pArg->pCanvas->TextOut(x, y, str);
		    pArg->pCanvas->SetTextColor(RGB(0,0,0));
		    rgn6.DeleteObject();
		}
		else
		{
		    str.Format("%.6f", (pArg->pDoc->m_dLeft + pArg->pDoc->m_dRight) / 2.0);
		    size = pArg->pCanvas->GetTextExtent(str);
		    pArg->pCanvas->TextOut(((big_rect.left + big_rect.right) / 2) - (size.cx / 2), big_rect.bottom + 7, str);
		}
		
		// draw the left and right timestamps
		//str.Format("%.6f", pArg->pDoc->m_dLeft);
		str.Format("%.6f", dLeft);
		pArg->pCanvas->TextOut(big_rect.left, big_rect.bottom + 7, str);
		//str.Format("%.6f", pArg->pDoc->m_dRight);
		str.Format("%.6f", dRight);
		size = pArg->pCanvas->GetTextExtent(str);
		pArg->pCanvas->TextOut(big_rect.right - size.cx, big_rect.bottom + 7, str);
		
		delete pdNextPixel;
	    }
	    PostMessage(pArg->hWnd, DRAW_COMPLETE_MSG, 0, 0);
	    break;
	}
	ReleaseMutex(pArg->hMutex);
	pArg->bStop = false;
	SetEvent(pArg->hStoppedEvent);
    }
}


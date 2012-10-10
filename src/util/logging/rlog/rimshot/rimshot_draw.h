/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef RIMSHOT_DRAW_H
#define RIMSHOT_DRAW_H

#include "RimshotDoc.h"

#define REDRAW_CMD  1
#define EXIT_CMD    2

#define DRAW_COMPLETE_MSG     WM_USER + 1

struct CursorRank
{
    int rank;
    bool active;
    CRect rect;
};

struct RimshotDrawStruct
{
    CRimshotDoc* pDoc;
    CSize rect_size, copy_size;
    CSize max_rect_size, max_copy_size;
    HWND hWnd;
    HANDLE hDrawEvent, hStoppedEvent, hMutex;
    bool bStop;
    int nCmd;
    bool bDrawArrows;
    bool bDrawUniform;
    int nUniformWidth;
    int nUniNumRanks;
    COLORREF **ppUniRecursionColor;
    CursorRank *pCursorRanks;
    bool bSliding;
    int nSlidingRank;
    CBitmap *pSlidingBitmap;
    double dSlideStart, dSlideFinish;
    CDC *pCanvas;
    CBitmap *pBitmap, *pOriginalBmp;
    CDC *pCopyCanvas;
    CBitmap *pCopyBitmap, *pCopyOriginalBmp;
};

void RimshotDrawThread(RimshotDrawStruct *pArg);

#endif

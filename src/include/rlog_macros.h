/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef RLOG_MACROS_H_INCLUDED
#define RLOG_MACROS_H_INCLUDED

#include "rlog.h"
#include "mpiu_timer.h"

#ifndef MPIDM_Wtime_todouble
#error Failed to select a defintion for MPIDM_Wtime_todouble
#endif

/* structures, global variables */
/* FIXME: All global names should follow the prefix rules to ensure that 
   there are no collisions with user-defined global names.  g_pRLOG should be
   RLOG_something */
extern RLOG_Struct *g_pRLOG;

/* state declaration macros */
#define MPIDU_STATE_DECL(a) MPID_Time_t time_stamp_in##a , time_stamp_out##a
#define MPIDU_INIT_STATE_DECL(a)
#define MPIDU_FINALIZE_STATE_DECL(a)

/* function enter and exit macros */
#define MPIDU_FUNC_ENTER(a) \
if (g_pRLOG) \
{ \
    g_pRLOG->nRecursion++; \
    MPID_Wtime( &time_stamp_in##a ); \
}

#define RLOG_MACRO_HEADER_CAST() ((RLOG_HEADER*)g_pRLOG->pOutput->pCurHeader)
#define RLOG_MACRO_EVENT_CAST()  ((RLOG_EVENT*)((char*)g_pRLOG->pOutput->pCurHeader + sizeof(RLOG_HEADER)))

#define MPIDU_FUNC_EXIT(a) \
if (g_pRLOG) \
{ \
    if (g_pRLOG->bLogging) \
    { \
	double d1, d2; \
	MPID_Wtime( &time_stamp_out##a ); \
	MPIDM_Wtime_todouble( ( &time_stamp_in##a ), &d1); \
	MPIDM_Wtime_todouble( ( &time_stamp_out##a ), &d2); \
	g_pRLOG->nRecursion--; \
	if (g_pRLOG->pOutput->pCurHeader + sizeof(RLOG_HEADER) + sizeof(RLOG_EVENT) > g_pRLOG->pOutput->pEnd) \
	{ \
	    WriteCurrentDataAndLogEvent(g_pRLOG, a , d1, d2, g_pRLOG->nRecursion); \
	} \
	else \
	{ \
	    RLOG_MACRO_HEADER_CAST()->type = RLOG_EVENT_TYPE; \
	    RLOG_MACRO_HEADER_CAST()->length = sizeof(RLOG_HEADER) + sizeof(RLOG_EVENT); \
	    RLOG_MACRO_EVENT_CAST()->rank = g_pRLOG->nRank; \
	    RLOG_MACRO_EVENT_CAST()->end_time = d2 - g_pRLOG->dFirstTimestamp; \
	    RLOG_MACRO_EVENT_CAST()->start_time = d1 - g_pRLOG->dFirstTimestamp; \
	    RLOG_MACRO_EVENT_CAST()->event = a ; \
	    RLOG_MACRO_EVENT_CAST()->recursion = g_pRLOG->nRecursion; \
	    /* advance the current position pointer */ \
	    g_pRLOG->pOutput->pCurHeader += sizeof(RLOG_HEADER) + sizeof(RLOG_EVENT); \
	} \
    } \
}

#define MPIDU_PT2PT_FUNC_ENTER(a)     MPIDU_FUNC_ENTER(a)
#define MPIDU_PT2PT_FUNC_EXIT(a)      MPIDU_FUNC_EXIT(a)
#define MPIDU_COLL_FUNC_ENTER(a)      MPIDU_FUNC_ENTER(a)
#define MPIDU_COLL_FUNC_EXIT(a)       MPIDU_FUNC_EXIT(a)
#define MPIDU_RMA_FUNC_ENTER(a)       MPIDU_FUNC_ENTER(a)
#define MPIDU_RMA_FUNC_EXIT(a)        MPIDU_FUNC_EXIT(a)
#define MPIDU_INIT_FUNC_ENTER(a)
#define MPIDU_INIT_FUNC_EXIT(a)
#define MPIDU_FINALIZE_FUNC_ENTER(a)
#define MPIDU_FINALIZE_FUNC_EXIT(a)

/* arrow generating enter and exit macros */
#define MPIDU_PT2PT_FUNC_ENTER_FRONT(a) \
if (g_pRLOG) \
{ \
    g_pRLOG->nRecursion++; \
    MPID_Wtime( &time_stamp_in##a ); \
    RLOG_LogSend( g_pRLOG, dest, tag, count ); \
}

#define MPIDU_PT2PT_FUNC_ENTER_BACK(a) \
if (g_pRLOG) \
{ \
    g_pRLOG->nRecursion++; \
    MPID_Wtime( &time_stamp_in##a ); \
    RLOG_LogRecv( g_pRLOG, source, tag, count ); \
}

#ifdef MPID_LOG_RECV_FROM_BEGINNING
#define MPIDU_PT2PT_FUNC_ENTER_BOTH(a) \
if (g_pRLOG) \
{ \
    g_pRLOG->nRecursion++; \
    MPID_Wtime( &time_stamp_in##a ); \
    RLOG_LogSend( g_pRLOG, dest, sendtag, sendcount ); \
    RLOG_LogRecv( g_pRLOG, source, recvtag, recvcount ); \
}
#else
#define MPIDU_PT2PT_FUNC_ENTER_BOTH(a) \
if (g_pRLOG) \
{ \
    g_pRLOG->nRecursion++; \
    MPID_Wtime( &time_stamp_in##a ); \
    RLOG_LogSend( g_pRLOG, dest, sendtag, sendcount ); \
}
#endif

#define MPIDU_PT2PT_FUNC_EXIT_BACK(a) \
if (g_pRLOG) \
{ \
    RLOG_LogRecv( g_pRLOG, source, tag, count ); \
    MPIDU_PT2PT_FUNC_EXIT(a) \
}

#define MPIDU_PT2PT_FUNC_EXIT_BOTH(a) \
if (g_pRLOG) \
{ \
    RLOG_LogRecv( g_pRLOG, source, recvtag, recvcount ); \
    MPIDU_PT2PT_FUNC_EXIT(a) \
}

#endif

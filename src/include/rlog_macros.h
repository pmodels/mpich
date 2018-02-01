/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef RLOG_MACROS_H_INCLUDED
#define RLOG_MACROS_H_INCLUDED

#include "rlog.h"

#ifndef MPIDM_Wtime_todouble
#error Failed to select a defintion for MPIDM_Wtime_todouble
#endif

/* prototype the initialization/finalization functions */
int MPII_Timer_init(int rank, int size);
int MPII_Timer_finalize(void);
int MPII_Describe_timer_states(void);

/* structures, global variables */
/* FIXME: All global names should follow the prefix rules to ensure that
   there are no collisions with user-defined global names.  g_pRLOG should be
   RLOGI_something */
extern RLOGI_Struct *g_pRLOG;

/* state declaration macros */
#define RLOG_STATE_DECL(a) MPID_Time_t time_stamp_in##a , time_stamp_out##a
#define RLOG_INIT_STATE_DECL(a)
#define RLOG_FINALIZE_STATE_DECL(a)

/* function enter and exit macros */
#define RLOG_FUNC_ENTER(a)                      \
    if (g_pRLOG)                                \
    {                                           \
        g_pRLOG->nRecursion++;                  \
        MPID_Wtime(&time_stamp_in##a);          \
    }

#define RLOGI_MACRO_HEADER_CAST() ((RLOGI_HEADER*)g_pRLOG->pOutput->pCurHeader)
#define RLOGI_MACRO_EVENT_CAST()  ((RLOGI_EVENT*)((char*)g_pRLOG->pOutput->pCurHeader + sizeof(RLOGI_HEADER)))

#define RLOG_FUNC_EXIT(a)                                               \
    if (g_pRLOG)                                                        \
    {                                                                   \
        if (g_pRLOG->bLogging)                                          \
        {                                                               \
            double d1, d2;                                              \
            MPID_Wtime(&time_stamp_out##a);                             \
            MPIDM_Wtime_todouble((&time_stamp_in##a), &d1);             \
            MPIDM_Wtime_todouble((&time_stamp_out##a), &d2);            \
            g_pRLOG->nRecursion--;                                      \
            if (g_pRLOG->pOutput->pCurHeader + sizeof(RLOGI_HEADER) + sizeof(RLOGI_EVENT) > g_pRLOG->pOutput->pEnd) \
            {                                                           \
                WriteCurrentDataAndLogEvent(g_pRLOG, a , d1, d2, g_pRLOG->nRecursion); \
            }                                                           \
            else                                                        \
            {                                                           \
                RLOGI_MACRO_HEADER_CAST()->type = RLOGI_EVENT_TYPE;     \
                RLOGI_MACRO_HEADER_CAST()->length = sizeof(RLOGI_HEADER) + sizeof(RLOGI_EVENT); \
                RLOGI_MACRO_EVENT_CAST()->rank = g_pRLOG->nRank;        \
                RLOGI_MACRO_EVENT_CAST()->end_time = d2 - g_pRLOG->dFirstTimestamp; \
                RLOGI_MACRO_EVENT_CAST()->start_time = d1 - g_pRLOG->dFirstTimestamp; \
                RLOGI_MACRO_EVENT_CAST()->event = a ;                   \
                RLOGI_MACRO_EVENT_CAST()->recursion = g_pRLOG->nRecursion; \
                /* advance the current position pointer */              \
                g_pRLOG->pOutput->pCurHeader += sizeof(RLOGI_HEADER) + sizeof(RLOGI_EVENT); \
            }                                                           \
        }                                                               \
    }

#define RLOG_PT2PT_FUNC_ENTER(a)     RLOG_FUNC_ENTER(a)
#define RLOG_PT2PT_FUNC_EXIT(a)      RLOG_FUNC_EXIT(a)
#define RLOG_COLL_FUNC_ENTER(a)      RLOG_FUNC_ENTER(a)
#define RLOG_COLL_FUNC_EXIT(a)       RLOG_FUNC_EXIT(a)
#define RLOG_RMA_FUNC_ENTER(a)       RLOG_FUNC_ENTER(a)
#define RLOG_RMA_FUNC_EXIT(a)        RLOG_FUNC_EXIT(a)
#define RLOG_INIT_FUNC_ENTER(a)
#define RLOG_INIT_FUNC_EXIT(a)
#define RLOG_FINALIZE_FUNC_ENTER(a)
#define RLOG_FINALIZE_FUNC_EXIT(a)

/* arrow generating enter and exit macros */
#define RLOG_PT2PT_FUNC_ENTER_FRONT(a)                  \
    if (g_pRLOG)                                        \
    {                                                   \
        g_pRLOG->nRecursion++;                          \
        MPID_Wtime(&time_stamp_in##a);                  \
        RLOGI_LogSend(g_pRLOG, dest, tag, count);       \
    }

#define RLOG_PT2PT_FUNC_ENTER_BACK(a)                   \
    if (g_pRLOG)                                        \
    {                                                   \
        g_pRLOG->nRecursion++;                          \
        MPID_Wtime(&time_stamp_in##a);                  \
        RLOGI_LogRecv(g_pRLOG, source, tag, count);     \
    }

#define RLOG_PT2PT_FUNC_ENTER_BOTH(a)                           \
    if (g_pRLOG)                                                \
    {                                                           \
        g_pRLOG->nRecursion++;                                  \
        MPID_Wtime(&time_stamp_in##a);                          \
        RLOGI_LogSend(g_pRLOG, dest, sendtag, sendcount);       \
    }

#define RLOG_PT2PT_FUNC_EXIT_BACK(a)                    \
    if (g_pRLOG)                                        \
    {                                                   \
        RLOGI_LogRecv(g_pRLOG, source, tag, count);     \
        RLOG_PT2PT_FUNC_EXIT(a);                        \
    }

#define RLOG_PT2PT_FUNC_EXIT_BOTH(a)                            \
    if (g_pRLOG)                                                \
    {                                                           \
        RLOGI_LogRecv(g_pRLOG, source, recvtag, recvcount);     \
        RLOG_PT2PT_FUNC_EXIT(a);                                \
    }


/* MPI layer definitions */
#define MPIR_FUNC_TERSE_STATE_DECL(a)                RLOG_STATE_DECL(a)
#define MPIR_FUNC_TERSE_INIT_STATE_DECL(a)           RLOG_INIT_STATE_DECL(a)
#define MPIR_FUNC_TERSE_FINALIZE_STATE_DECL(a)       RLOG_FINALIZE_STATE_DECL(a)

#define MPIR_FUNC_TERSE_ENTER(a)                RLOG_FUNC_ENTER(a)
#define MPIR_FUNC_TERSE_EXIT(a)                 RLOG_FUNC_EXIT(a)
#define MPIR_FUNC_TERSE_PT2PT_ENTER(a)          RLOG_PT2PT_FUNC_ENTER(a)
#define MPIR_FUNC_TERSE_PT2PT_EXIT(a)           RLOG_PT2PT_FUNC_EXIT(a)
#define MPIR_FUNC_TERSE_COLL_ENTER(a)           RLOG_COLL_FUNC_ENTER(a)
#define MPIR_FUNC_TERSE_COLL_EXIT(a)            RLOG_COLL_FUNC_EXIT(a)
#define MPIR_FUNC_TERSE_RMA_ENTER(a)            RLOG_RMA_FUNC_ENTER(a)
#define MPIR_FUNC_TERSE_RMA_EXIT(a)             RLOG_RMA_FUNC_EXIT(a)
#define MPIR_FUNC_TERSE_INIT_ENTER(a)           RLOG_INIT_FUNC_ENTER(a)
#define MPIR_FUNC_TERSE_INIT_EXIT(a)            RLOG_INIT_FUNC_EXIT(a)
#define MPIR_FUNC_TERSE_FINALIZE_ENTER(a)       RLOG_FINALIZE_FUNC_ENTER(a)
#define MPIR_FUNC_TERSE_FINALIZE_EXIT(a)        RLOG_FINALIZE_FUNC_EXIT(a)

#define MPIR_FUNC_TERSE_PT2PT_ENTER_FRONT(a)    RLOG_PT2PT_FUNC_ENTER_FRONT(a)
#define MPIR_FUNC_TERSE_PT2PT_EXIT_FRONT(a)     RLOG_PT2PT_FUNC_EXIT(a)
#define MPIR_FUNC_TERSE_PT2PT_ENTER_BACK(a)     RLOG_PT2PT_FUNC_ENTER(a)
#define MPIR_FUNC_TERSE_PT2PT_EXIT_BACK(a)      RLOG_PT2PT_FUNC_EXIT_BACK(a)
#define MPIR_FUNC_TERSE_PT2PT_EXIT_BOTH(a)      RLOG_PT2PT_FUNC_EXIT_BOTH(a)
#define MPIR_FUNC_TERSE_PT2PT_ENTER_BOTH(a)     RLOG_PT2PT_FUNC_ENTER_BOTH(a)

#if defined(HAVE_TIMING) && (HAVE_TIMING == MPICH_TIMING_KIND__LOG_DETAILED || HAVE_TIMING == MPICH_TIMING_KIND__ALL)

/* device layer definitions */
#define MPIR_FUNC_VERBOSE_STATE_DECL(a)           RLOG_STATE_DECL(a)
#define MPIR_FUNC_VERBOSE_ENTER(a)                RLOG_FUNC_ENTER(a)
#define MPIR_FUNC_VERBOSE_EXIT(a)                 RLOG_FUNC_EXIT(a)
#define MPIR_FUNC_VERBOSE_PT2PT_ENTER(a)          RLOG_PT2PT_FUNC_ENTER(a)
#define MPIR_FUNC_VERBOSE_PT2PT_EXIT(a)           RLOG_PT2PT_FUNC_EXIT(a)
#define MPIR_FUNC_VERBOSE_RMA_ENTER(a)            RLOG_RMA_FUNC_ENTER(a)
#define MPIR_FUNC_VERBOSE_RMA_EXIT(a)             RLOG_RMA_FUNC_EXIT(a)

#else

#define MPIR_FUNC_VERBOSE_STATE_DECL(a)
#define MPIR_FUNC_VERBOSE_ENTER(a)
#define MPIR_FUNC_VERBOSE_EXIT(a)
#define MPIR_FUNC_VERBOSE_PT2PT_ENTER(a)
#define MPIR_FUNC_VERBOSE_PT2PT_EXIT(a)
#define MPIR_FUNC_VERBOSE_RMA_ENTER(a)
#define MPIR_FUNC_VERBOSE_RMA_EXIT(a)

#endif /* (HAVE_TIMING == MPICH_TIMING_KIND__LOG_DETAILED || HAVE_TIMING == MPICH_TIMING_KIND__ALL) */

#endif /* RLOG_MACROS_H_INCLUDED */

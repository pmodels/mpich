/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MM_MPE_PROF_H
#define MM_MPE_PROF_H

/*#include "mpe_log.h"*/
#include "dlog.h"
#include "mm_timer_states.h"

typedef struct MM_Timer_state
{
    int in_id, out_id;
    int num_calls;
    unsigned long color;
    /*char color_str[12];*/ /* perfect for '255 255 255' */
    char color_str[40];
    char *name;
} MM_Timer_state;

extern MM_Timer_state g_timer_state[MM_NUM_TIMER_STATES];
extern DLOG_Struct *g_pDLOG;

#define MPID_FUNC_ENTER(a) DLOG_LogEvent( g_pDLOG, g_timer_state[ a##_INDEX ].in_id, g_timer_state[ a##_INDEX ].num_calls++ )
#define MPID_FUNC_EXIT(a) DLOG_LogEvent( g_pDLOG, g_timer_state[ a##_INDEX ].out_id, g_timer_state[ a##_INDEX ].num_calls++ )

int prof_init(int rank, int size);
int prof_finalize();

#endif

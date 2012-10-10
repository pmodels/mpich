/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPID_NEM_DEBUG_H
#define MPID_NEM_DEBUG_H

/*#define YIELD_IN_SKIP*/
#ifdef YIELD_IN_SKIP
#define SKIP MPIU_PW_Sched_yield()
#warning "SKIP is yield"
#else /* YIELD_IN_SKIP */
#define SKIP do{}while(0)
/*#warning "SKIP is do ...while" */
#endif /* YIELD_IN_SKIP */

/* debugging message functions */
struct MPID_nem_cell;
void MPID_nem_dbg_dump_cell (volatile struct MPID_nem_cell *cell);

#endif /* MPID_NEM_DEBUG_H */

/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPITIMERIMPL_H_INCLUDED
#define MPITIMERIMPL_H_INCLUDED

/* Possible values for timing */
#define MPID_TIMING_KIND_NONE 0
#define MPID_TIMING_KIND_TIME 1
#define MPID_TIMING_KIND_LOG 2
#define MPID_TIMING_KIND_LOG_DETAILED 3
#define MPID_TIMING_KIND_ALL 4
#define MPID_TIMING_KIND_RUNTIME 5

/* Routine tracing (see --enable-timing for control of this) */
#if defined(HAVE_TIMING) && (HAVE_TIMING == MPID_TIMING_KIND_LOG || \
    HAVE_TIMING == MPID_TIMING_KIND_LOG_DETAILED || \
    HAVE_TIMING == MPID_TIMING_KIND_ALL || \
    HAVE_TIMING == MPID_TIMING_KIND_RUNTIME)

/* define MPID_LOG_RECV_FROM_BEGINNING to log arrows from the beginning of 
   send operations to the beginning of the corresponding receive operations.  
   Otherwise, arrows are logged from the beginning of the send to the end of 
   the receive. */
/* FIXME: Document this and/or make it a runtime feature or decide on a 
   single approach. */
#undef MPID_LOG_RECV_FROM_BEGINNING
/*#define MPID_LOG_RECV_FROM_BEGINNING*/

/* This include file contains the static state definitions */
#include "mpiallstates.h"

/* Possible values for USE_LOGGING */
#define MPID_LOGGING_NONE 0
#define MPID_LOGGING_RLOG 1
#define MPID_LOGGING_EXTERNAL 4

/* Include the macros specific to the selected logging library */
#if (USE_LOGGING == MPID_LOGGING_RLOG)
#include "rlog_macros.h"
#elif (USE_LOGGING == MPID_LOGGING_EXTERNAL)
#include "mpilogging.h"
#else
#error You must select a logging library if timing is enabled
#endif

/* MPI layer definitions */
#define MPID_MPI_STATE_DECL(a)                MPIDU_STATE_DECL(a)
#define MPID_MPI_INIT_STATE_DECL(a)           MPIDU_INIT_STATE_DECL(a)
#define MPID_MPI_FINALIZE_STATE_DECL(a)       MPIDU_FINALIZE_STATE_DECL(a)

#define MPID_MPI_FUNC_ENTER(a)                MPIDU_FUNC_ENTER(a)
#define MPID_MPI_FUNC_EXIT(a)                 MPIDU_FUNC_EXIT(a)
#define MPID_MPI_PT2PT_FUNC_ENTER(a)          MPIDU_PT2PT_FUNC_ENTER(a)
#define MPID_MPI_PT2PT_FUNC_EXIT(a)           MPIDU_PT2PT_FUNC_EXIT(a)
#define MPID_MPI_COLL_FUNC_ENTER(a)           MPIDU_COLL_FUNC_ENTER(a)
#define MPID_MPI_COLL_FUNC_EXIT(a)            MPIDU_COLL_FUNC_EXIT(a)
#define MPID_MPI_RMA_FUNC_ENTER(a)            MPIDU_RMA_FUNC_ENTER(a)
#define MPID_MPI_RMA_FUNC_EXIT(a)             MPIDU_RMA_FUNC_EXIT(a)
#define MPID_MPI_INIT_FUNC_ENTER(a)           MPIDU_INIT_FUNC_ENTER(a)
#define MPID_MPI_INIT_FUNC_EXIT(a)            MPIDU_INIT_FUNC_EXIT(a)
#define MPID_MPI_FINALIZE_FUNC_ENTER(a)       MPIDU_FINALIZE_FUNC_ENTER(a)
#define MPID_MPI_FINALIZE_FUNC_EXIT(a)        MPIDU_FINALIZE_FUNC_EXIT(a)

#define MPID_LOG_ARROWS
#ifdef MPID_LOG_ARROWS
#define MPID_MPI_PT2PT_FUNC_ENTER_FRONT(a)    MPIDU_PT2PT_FUNC_ENTER_FRONT(a)
#define MPID_MPI_PT2PT_FUNC_EXIT_FRONT(a)     MPIDU_PT2PT_FUNC_EXIT(a)
#ifdef MPID_LOG_RECV_FROM_BEGINNING
#define MPID_MPI_PT2PT_FUNC_ENTER_BACK(a)     MPIDU_PT2PT_FUNC_ENTER_BACK(a)
#define MPID_MPI_PT2PT_FUNC_EXIT_BACK(a)      MPIDU_PT2PT_FUNC_EXIT(a)
#define MPID_MPI_PT2PT_FUNC_EXIT_BOTH(a)      MPIDU_PT2PT_FUNC_EXIT(a)
#else
#define MPID_MPI_PT2PT_FUNC_ENTER_BACK(a)     MPIDU_PT2PT_FUNC_ENTER(a)
#define MPID_MPI_PT2PT_FUNC_EXIT_BACK(a)      MPIDU_PT2PT_FUNC_EXIT_BACK(a)
#define MPID_MPI_PT2PT_FUNC_EXIT_BOTH(a)      MPIDU_PT2PT_FUNC_EXIT_BOTH(a)
#endif
#define MPID_MPI_PT2PT_FUNC_ENTER_BOTH(a)     MPIDU_PT2PT_FUNC_ENTER_BOTH(a)
#else
#define MPID_MPI_PT2PT_FUNC_ENTER_FRONT(a)    MPIDU_PT2PT_FUNC_ENTER(a)
#define MPID_MPI_PT2PT_FUNC_EXIT_FRONT(a)     MPIDU_PT2PT_FUNC_EXIT(a)
#define MPID_MPI_PT2PT_FUNC_ENTER_BACK(a)     MPIDU_PT2PT_FUNC_ENTER(a)
#define MPID_MPI_PT2PT_FUNC_ENTER_BOTH(a)     MPIDU_PT2PT_FUNC_ENTER(a)
#define MPID_MPI_PT2PT_FUNC_EXIT_BACK(a)      MPIDU_PT2PT_FUNC_EXIT(a)
#define MPID_MPI_PT2PT_FUNC_EXIT_BOTH(a)      MPIDU_PT2PT_FUNC_EXIT(a)
#endif

#if defined(HAVE_TIMING) && (HAVE_TIMING == MPID_TIMING_KIND_LOG_DETAILED || HAVE_TIMING == MPID_TIMING_KIND_ALL)

/* device layer definitions */
#define MPIDI_STATE_DECL(a)                MPIDU_STATE_DECL(a)
#define MPIDI_INIT_STATE_DECL(a)           MPIDU_INIT_STATE_DECL(a)
#define MPIDI_FINALIZE_STATE_DECL(a)       MPIDU_FINALIZE_STATE_DECL(a)

#define MPIDI_FUNC_ENTER(a)                MPIDU_FUNC_ENTER(a)
#define MPIDI_FUNC_EXIT(a)                 MPIDU_FUNC_EXIT(a)
#define MPIDI_PT2PT_FUNC_ENTER(a)          MPIDU_PT2PT_FUNC_ENTER(a)
#define MPIDI_PT2PT_FUNC_EXIT(a)           MPIDU_PT2PT_FUNC_EXIT(a)
#define MPIDI_COLL_FUNC_ENTER(a)           MPIDU_COLL_FUNC_ENTER(a)
#define MPIDI_COLL_FUNC_EXIT(a)            MPIDU_COLL_FUNC_EXIT(a)
#define MPIDI_RMA_FUNC_ENTER(a)            MPIDU_RMA_FUNC_ENTER(a)
#define MPIDI_RMA_FUNC_EXIT(a)             MPIDU_RMA_FUNC_EXIT(a)
#define MPIDI_INIT_FUNC_ENTER(a)           MPIDU_INIT_FUNC_ENTER(a)
#define MPIDI_INIT_FUNC_EXIT(a)            MPIDU_INIT_FUNC_EXIT(a)
#define MPIDI_FINALIZE_FUNC_ENTER(a)       MPIDU_FINALIZE_FUNC_ENTER(a)
#define MPIDI_FINALIZE_FUNC_EXIT(a)        MPIDU_FINALIZE_FUNC_EXIT(a)

#define MPID_LOG_MPID_ARROWS
#ifdef MPID_LOG_MPID_ARROWS
#define MPIDI_PT2PT_FUNC_ENTER_FRONT(a)    MPIDU_PT2PT_FUNC_ENTER_FRONT(a)
#define MPIDI_PT2PT_FUNC_EXIT_FRONT(a)     MPIDU_PT2PT_FUNC_EXIT(a)
#ifdef MPID_LOG_RECV_FROM_BEGINNING
#define MPIDI_PT2PT_FUNC_ENTER_BACK(a)     MPIDU_PT2PT_FUNC_ENTER_BACK(a)
#define MPIDI_PT2PT_FUNC_EXIT_BACK(a)      MPIDU_PT2PT_FUNC_EXIT(a)
#define MPIDI_PT2PT_FUNC_EXIT_BOTH(a)      MPIDU_PT2PT_FUNC_EXIT(a)
#else
#define MPIDI_PT2PT_FUNC_ENTER_BACK(a)     MPIDU_PT2PT_FUNC_ENTER(a)
#define MPIDI_PT2PT_FUNC_EXIT_BACK(a)      MPIDU_PT2PT_FUNC_EXIT_BACK(a)
#define MPIDI_PT2PT_FUNC_EXIT_BOTH(a)      MPIDU_PT2PT_FUNC_EXIT_BOTH(a)
#endif
#define MPIDI_PT2PT_FUNC_ENTER_BOTH(a)     MPIDU_PT2PT_FUNC_ENTER_BOTH(a)
#else
#define MPIDI_PT2PT_FUNC_ENTER_FRONT(a)    MPIDU_PT2PT_FUNC_ENTER(a)
#define MPIDI_PT2PT_FUNC_EXIT_FRONT(a)     MPIDU_PT2PT_FUNC_EXIT(a)
#define MPIDI_PT2PT_FUNC_ENTER_BACK(a)     MPIDU_PT2PT_FUNC_ENTER(a)
#define MPIDI_PT2PT_FUNC_ENTER_BOTH(a)     MPIDU_PT2PT_FUNC_ENTER(a)
#define MPIDI_PT2PT_FUNC_EXIT_BACK(a)      MPIDU_PT2PT_FUNC_EXIT(a)
#define MPIDI_PT2PT_FUNC_EXIT_BOTH(a)      MPIDU_PT2PT_FUNC_EXIT(a)
#endif

#else

#define MPIDI_STATE_DECL(a)
#define MPIDI_FUNC_ENTER(a)
#define MPIDI_FUNC_EXIT(a)
#define MPIDI_PT2PT_FUNC_ENTER(a)
#define MPIDI_PT2PT_FUNC_ENTER_FRONT(a)
#define MPIDI_PT2PT_FUNC_EXIT_FRONT(a)
#define MPIDI_PT2PT_FUNC_ENTER_BACK(a)
#define MPIDI_PT2PT_FUNC_ENTER_BOTH(a)
#define MPIDI_PT2PT_FUNC_EXIT_BACK(a)
#define MPIDI_PT2PT_FUNC_EXIT_BOTH(a)
#define MPIDI_PT2PT_FUNC_EXIT(a)
#define MPIDI_COLL_FUNC_ENTER(a)
#define MPIDI_COLL_FUNC_EXIT(a)
#define MPIDI_RMA_FUNC_ENTER(a)
#define MPIDI_RMA_FUNC_EXIT(a)
#define MPIDI_INIT_FUNC_ENTER(a)
#define MPIDI_INIT_FUNC_EXIT(a)
#define MPIDI_FINALIZE_FUNC_ENTER(a)
#define MPIDI_FINALIZE_FUNC_EXIT(a)

#endif /* (HAVE_TIMING == MPID_TIMING_KIND_LOG_DETAILED || HAVE_TIMING == MPID_TIMING_KIND_ALL) */

/* prototype the initialization/finalization functions */
int MPIU_Timer_init(int rank, int size);
int MPIU_Timer_finalize(void);
int MPIR_Describe_timer_states(void);

/* Statistics macros aren't defined yet */
/* All uses of these are protected by the symbol COLLECT_STATS, so they
   do not need to be defined in the non-HAVE_TIMING branch. */
#define MPID_STAT_BEGIN
#define MPID_STAT_END
#define MPID_STAT_ACC(statid,val)
#define MPID_STAT_ACC_RANGE(statid,rng)
#define MPID_STAT_ACC_SIMPLE(statid,val)
#define MPID_STAT_MISC(a) a

#else /* HAVE_TIMING and doing logging */

/* evaporate all the timing macros if timing is not selected */
#define MPIU_Timer_init(rank, size)
#define MPIU_Timer_finalize()
/* MPI layer */
#define MPID_MPI_STATE_DECL(a)
#define MPID_MPI_INIT_STATE_DECL(a)
#define MPID_MPI_FINALIZE_STATE_DECL(a)
#define MPID_MPI_FUNC_EXIT(a)
#define MPID_MPI_FUNC_ENTER(a)
#define MPID_MPI_PT2PT_FUNC_ENTER(a)
#define MPID_MPI_PT2PT_FUNC_ENTER_FRONT(a)
#define MPID_MPI_PT2PT_FUNC_EXIT_FRONT(a)
#define MPID_MPI_PT2PT_FUNC_ENTER_BACK(a)
#define MPID_MPI_PT2PT_FUNC_ENTER_BOTH(a)
#define MPID_MPI_PT2PT_FUNC_EXIT(a)
#define MPID_MPI_PT2PT_FUNC_EXIT_BACK(a)
#define MPID_MPI_PT2PT_FUNC_EXIT_BOTH(a)
#define MPID_MPI_COLL_FUNC_ENTER(a)
#define MPID_MPI_COLL_FUNC_EXIT(a)
#define MPID_MPI_RMA_FUNC_ENTER(a)
#define MPID_MPI_RMA_FUNC_EXIT(a)
#define MPID_MPI_INIT_FUNC_ENTER(a)
#define MPID_MPI_INIT_FUNC_EXIT(a)
#define MPID_MPI_FINALIZE_FUNC_ENTER(a)
#define MPID_MPI_FINALIZE_FUNC_EXIT(a)
/* device layer */
#define MPIDI_STATE_DECL(a)
#define MPIDI_FUNC_ENTER(a)
#define MPIDI_FUNC_EXIT(a)
#define MPIDI_PT2PT_FUNC_ENTER(a)
#define MPIDI_PT2PT_FUNC_ENTER_FRONT(a)
#define MPIDI_PT2PT_FUNC_EXIT_FRONT(a)
#define MPIDI_PT2PT_FUNC_ENTER_BACK(a)
#define MPIDI_PT2PT_FUNC_ENTER_BOTH(a)
#define MPIDI_PT2PT_FUNC_EXIT(a)
#define MPIDI_PT2PT_FUNC_EXIT_BACK(a)
#define MPIDI_PT2PT_FUNC_EXIT_BOTH(a)
#define MPIDI_COLL_FUNC_ENTER(a)
#define MPIDI_COLL_FUNC_EXIT(a)
#define MPIDI_RMA_FUNC_ENTER(a)
#define MPIDI_RMA_FUNC_EXIT(a)
#define MPIDI_INIT_FUNC_ENTER(a)
#define MPIDI_INIT_FUNC_EXIT(a)
#define MPIDI_FINALIZE_FUNC_ENTER(a)
#define MPIDI_FINALIZE_FUNC_EXIT(a)

#endif /* HAVE_TIMING */

#endif

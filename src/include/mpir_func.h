/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPIR_FUNC_H_INCLUDED
#define MPIR_FUNC_H_INCLUDED

/* In MPICH, each function has an "enter" and "exit" macro.  These can be
 * used to add various features to each function at compile time, or they
 * can be set to empty to provide the fastest possible production version.
 *
 * There are at this time three choices of features (beyond the empty choice)
 * 1. timing
 *    These collect data on when each function began and finished; the
 *    resulting data can be displayed using special programs
 * 2. Debug logging (selected with --enable-g=log)
 *    Invokes MPL_DBG_MSG at the entry and exit for each routine
 * 3. Additional memory validation of the memory arena (--enable-g=memarena)
 */

/* state declaration macros */
#if defined(MPL_USE_DBG_LOGGING) || defined(MPICH_DEBUG_MEMARENA)
#define MPID_MPI_STATE_DECL(a)
#define MPID_MPI_INIT_STATE_DECL(a)
#define MPID_MPI_FINALIZE_STATE_DECL(a)
#define MPIDI_STATE_DECL(a)

/* Tell the package to define the rest of the enter/exit macros in
   terms of these */
#define NEEDS_FUNC_ENTER_EXIT_DEFS 1
#endif /* MPL_USE_DBG_LOGGING || MPICH_DEBUG_MEMARENA */

/* function enter and exit macros */
#if defined(MPL_USE_DBG_LOGGING)
#define MPIR_FUNC_ENTER(a) MPL_DBG_MSG(MPL_DBG_ROUTINE_ENTER,TYPICAL,"Entering "#a)
#elif defined(MPICH_DEBUG_MEMARENA)
#define MPIR_FUNC_ENTER(a) MPL_trvalid("Entering " #a)
#endif

#if defined(MPL_USE_DBG_LOGGING)
#define MPIR_FUNC_EXIT(a) MPL_DBG_MSG(MPL_DBG_ROUTINE_EXIT,TYPICAL,"Leaving "#a)
#elif defined(MPICH_DEBUG_MEMARENA)
#define MPIR_FUNC_EXIT(a) MPL_trvalid("Leaving " #a)
#endif


#if defined(NEEDS_FUNC_ENTER_EXIT_DEFS)

#define MPID_MPI_FUNC_ENTER(a)			MPIR_FUNC_ENTER(a)
#define MPID_MPI_FUNC_EXIT(a)			MPIR_FUNC_EXIT(a)
#define MPID_MPI_PT2PT_FUNC_ENTER(a)		MPIR_FUNC_ENTER(a)
#define MPID_MPI_PT2PT_FUNC_EXIT(a)		MPIR_FUNC_EXIT(a)
#define MPID_MPI_PT2PT_FUNC_ENTER_FRONT(a)	MPIR_FUNC_ENTER(a)
#define MPID_MPI_PT2PT_FUNC_EXIT_FRONT(a)	MPIR_FUNC_EXIT(a)
#define MPID_MPI_PT2PT_FUNC_ENTER_BACK(a)	MPIR_FUNC_ENTER(a)
#define MPID_MPI_PT2PT_FUNC_ENTER_BOTH(a)	MPIR_FUNC_ENTER(a)
#define MPID_MPI_PT2PT_FUNC_EXIT_BACK(a)	MPIR_FUNC_EXIT(a)
#define MPID_MPI_PT2PT_FUNC_EXIT_BOTH(a)	MPIR_FUNC_EXIT(a)
#define MPID_MPI_COLL_FUNC_ENTER(a)		MPIR_FUNC_ENTER(a)
#define MPID_MPI_COLL_FUNC_EXIT(a)		MPIR_FUNC_EXIT(a)
#define MPID_MPI_RMA_FUNC_ENTER(a)		MPIR_FUNC_ENTER(a)
#define MPID_MPI_RMA_FUNC_EXIT(a)		MPIR_FUNC_EXIT(a)
#define MPID_MPI_INIT_FUNC_ENTER(a)		MPIR_FUNC_ENTER(a)
#define MPID_MPI_INIT_FUNC_EXIT(a)		MPIR_FUNC_EXIT(a)
#define MPID_MPI_FINALIZE_FUNC_ENTER(a)		MPIR_FUNC_ENTER(a)
#define MPID_MPI_FINALIZE_FUNC_EXIT(a)		MPIR_FUNC_EXIT(a)

/* device layer definitions */
#define MPIDI_FUNC_ENTER(a)			MPIR_FUNC_ENTER(a)
#define MPIDI_FUNC_EXIT(a)			MPIR_FUNC_EXIT(a)
#define MPIDI_PT2PT_FUNC_ENTER(a)		MPIR_FUNC_ENTER(a)
#define MPIDI_PT2PT_FUNC_EXIT(a)		MPIR_FUNC_EXIT(a)
#define MPIDI_RMA_FUNC_ENTER(a)			MPIR_FUNC_ENTER(a)
#define MPIDI_RMA_FUNC_EXIT(a)			MPIR_FUNC_EXIT(a)

/* evaporate the timing macros since timing is not selected */
#define MPIU_Timer_init(rank, size)
#define MPIU_Timer_finalize()

#else   /* ! NEEDS_FUNC_ENTER_EXIT_DEFS */

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

#define MPID_MPI_PT2PT_FUNC_ENTER_FRONT(a)    MPIDU_PT2PT_FUNC_ENTER_FRONT(a)
#define MPID_MPI_PT2PT_FUNC_EXIT_FRONT(a)     MPIDU_PT2PT_FUNC_EXIT(a)
#define MPID_MPI_PT2PT_FUNC_ENTER_BACK(a)     MPIDU_PT2PT_FUNC_ENTER(a)
#define MPID_MPI_PT2PT_FUNC_EXIT_BACK(a)      MPIDU_PT2PT_FUNC_EXIT_BACK(a)
#define MPID_MPI_PT2PT_FUNC_EXIT_BOTH(a)      MPIDU_PT2PT_FUNC_EXIT_BOTH(a)
#define MPID_MPI_PT2PT_FUNC_ENTER_BOTH(a)     MPIDU_PT2PT_FUNC_ENTER_BOTH(a)

#if defined(HAVE_TIMING) && (HAVE_TIMING == MPID_TIMING_KIND_LOG_DETAILED || HAVE_TIMING == MPID_TIMING_KIND_ALL)

/* device layer definitions */
#define MPIDI_STATE_DECL(a)                MPIDU_STATE_DECL(a)
#define MPIDI_FUNC_ENTER(a)                MPIDU_FUNC_ENTER(a)
#define MPIDI_FUNC_EXIT(a)                 MPIDU_FUNC_EXIT(a)
#define MPIDI_PT2PT_FUNC_ENTER(a)          MPIDU_PT2PT_FUNC_ENTER(a)
#define MPIDI_PT2PT_FUNC_EXIT(a)           MPIDU_PT2PT_FUNC_EXIT(a)
#define MPIDI_RMA_FUNC_ENTER(a)            MPIDU_RMA_FUNC_ENTER(a)
#define MPIDI_RMA_FUNC_EXIT(a)             MPIDU_RMA_FUNC_EXIT(a)

#else

#define MPIDI_STATE_DECL(a)
#define MPIDI_FUNC_ENTER(a)
#define MPIDI_FUNC_EXIT(a)
#define MPIDI_PT2PT_FUNC_ENTER(a)
#define MPIDI_PT2PT_FUNC_EXIT(a)
#define MPIDI_RMA_FUNC_ENTER(a)
#define MPIDI_RMA_FUNC_EXIT(a)

#endif /* (HAVE_TIMING == MPID_TIMING_KIND_LOG_DETAILED || HAVE_TIMING == MPID_TIMING_KIND_ALL) */

/* prototype the initialization/finalization functions */
int MPIU_Timer_init(int rank, int size);
int MPIU_Timer_finalize(void);
int MPIR_Describe_timer_states(void);

/* The original statistics macros (see the design documentation)
   have been superceeded by the MPIR_T_PVAR_* macros (see mpit.h) */

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
#define MPIDI_PT2PT_FUNC_EXIT(a)
#define MPIDI_RMA_FUNC_ENTER(a)
#define MPIDI_RMA_FUNC_EXIT(a)

#endif /* HAVE_TIMING */

#endif /* NEEDS_FUNC_ENTER_EXIT_DEFS */

#endif /* MPIR_FUNC_H_INCLUDED */

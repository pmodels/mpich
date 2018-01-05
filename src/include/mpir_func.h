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

/* state declaration macros */
#if defined(MPL_USE_DBG_LOGGING) || defined(MPICH_DEBUG_MEMARENA)

#define MPIR_FUNC_TERSE_STATE_DECL(a)
#define MPIR_FUNC_TERSE_INIT_STATE_DECL(a)
#define MPIR_FUNC_TERSE_FINALIZE_STATE_DECL(a)
#define MPIR_FUNC_TERSE_ENTER(a)             MPIR_FUNC_ENTER(a)
#define MPIR_FUNC_TERSE_EXIT(a)              MPIR_FUNC_EXIT(a)
#define MPIR_FUNC_TERSE_PT2PT_ENTER(a)       MPIR_FUNC_ENTER(a)
#define MPIR_FUNC_TERSE_PT2PT_EXIT(a)        MPIR_FUNC_EXIT(a)
#define MPIR_FUNC_TERSE_PT2PT_ENTER_FRONT(a) MPIR_FUNC_ENTER(a)
#define MPIR_FUNC_TERSE_PT2PT_EXIT_FRONT(a)  MPIR_FUNC_EXIT(a)
#define MPIR_FUNC_TERSE_PT2PT_ENTER_BACK(a)  MPIR_FUNC_ENTER(a)
#define MPIR_FUNC_TERSE_PT2PT_ENTER_BOTH(a)  MPIR_FUNC_ENTER(a)
#define MPIR_FUNC_TERSE_PT2PT_EXIT_BACK(a)   MPIR_FUNC_EXIT(a)
#define MPIR_FUNC_TERSE_PT2PT_EXIT_BOTH(a)   MPIR_FUNC_EXIT(a)
#define MPIR_FUNC_TERSE_COLL_ENTER(a)        MPIR_FUNC_ENTER(a)
#define MPIR_FUNC_TERSE_COLL_EXIT(a)         MPIR_FUNC_EXIT(a)
#define MPIR_FUNC_TERSE_RMA_ENTER(a)         MPIR_FUNC_ENTER(a)
#define MPIR_FUNC_TERSE_RMA_EXIT(a)          MPIR_FUNC_EXIT(a)
#define MPIR_FUNC_TERSE_REQUEST_ENTER(a)	 MPIR_FUNC_ENTER(a)
#define MPIR_FUNC_TERSE_REQUEST_EXIT(a)		 MPIR_FUNC_EXIT(a)
#define MPIR_FUNC_TERSE_INIT_ENTER(a)        MPIR_FUNC_ENTER(a)
#define MPIR_FUNC_TERSE_INIT_EXIT(a)         MPIR_FUNC_EXIT(a)
#define MPIR_FUNC_TERSE_FINALIZE_ENTER(a)    MPIR_FUNC_ENTER(a)
#define MPIR_FUNC_TERSE_FINALIZE_EXIT(a)     MPIR_FUNC_EXIT(a)

#define MPIR_FUNC_VERBOSE_STATE_DECL(a)
#define MPIR_FUNC_VERBOSE_ENTER(a)           MPIR_FUNC_ENTER(a)
#define MPIR_FUNC_VERBOSE_EXIT(a)            MPIR_FUNC_EXIT(a)
#define MPIR_FUNC_VERBOSE_PT2PT_ENTER(a)     MPIR_FUNC_ENTER(a)
#define MPIR_FUNC_VERBOSE_PT2PT_EXIT(a)      MPIR_FUNC_EXIT(a)
#define MPIR_FUNC_VERBOSE_RMA_ENTER(a)       MPIR_FUNC_ENTER(a)
#define MPIR_FUNC_VERBOSE_RMA_EXIT(a)        MPIR_FUNC_EXIT(a)

#define MPII_Timer_init(rank, size)
#define MPII_Timer_finalize()

#else /* ! defined(MPL_USE_DBG_LOGGING) && ! defined(MPICH_DEBUG_MEMARENA) */

/* Routine tracing (see --enable-timing for control of this) */
#if defined(HAVE_TIMING) && (HAVE_TIMING == MPICH_TIMING_KIND__LOG || \
    HAVE_TIMING == MPICH_TIMING_KIND__LOG_DETAILED || \
    HAVE_TIMING == MPICH_TIMING_KIND__ALL || \
    HAVE_TIMING == MPICH_TIMING_KIND__RUNTIME)

#include "mpiallstates.h"

#if (USE_LOGGING == MPICH_LOGGING__RLOG)
#include "rlog_macros.h"
#elif (USE_LOGGING == MPICH_LOGGING__EXTERNAL)
#include "mpilogging.h"
#else
#error You must select a logging library if timing is enabled
#endif

#else /* HAVE_TIMING and doing logging */

#define MPII_Timer_init(rank, size)
#define MPII_Timer_finalize()

#define MPIR_FUNC_TERSE_STATE_DECL(a)
#define MPIR_FUNC_TERSE_INIT_STATE_DECL(a)
#define MPIR_FUNC_TERSE_FINALIZE_STATE_DECL(a)
#define MPIR_FUNC_TERSE_EXIT(a)
#define MPIR_FUNC_TERSE_ENTER(a)
#define MPIR_FUNC_TERSE_PT2PT_ENTER(a)
#define MPIR_FUNC_TERSE_PT2PT_ENTER_FRONT(a)
#define MPIR_FUNC_TERSE_PT2PT_EXIT_FRONT(a)
#define MPIR_FUNC_TERSE_PT2PT_ENTER_BACK(a)
#define MPIR_FUNC_TERSE_PT2PT_ENTER_BOTH(a)
#define MPIR_FUNC_TERSE_PT2PT_EXIT(a)
#define MPIR_FUNC_TERSE_PT2PT_EXIT_BACK(a)
#define MPIR_FUNC_TERSE_PT2PT_EXIT_BOTH(a)
#define MPIR_FUNC_TERSE_COLL_ENTER(a)
#define MPIR_FUNC_TERSE_COLL_EXIT(a)
#define MPIR_FUNC_TERSE_RMA_ENTER(a)
#define MPIR_FUNC_TERSE_RMA_EXIT(a)
#define MPIR_FUNC_TERSE_REQUEST_ENTER(a)
#define MPIR_FUNC_TERSE_REQUEST_EXIT(a)
#define MPIR_FUNC_TERSE_INIT_ENTER(a)
#define MPIR_FUNC_TERSE_INIT_EXIT(a)
#define MPIR_FUNC_TERSE_FINALIZE_ENTER(a)
#define MPIR_FUNC_TERSE_FINALIZE_EXIT(a)

#define MPIR_FUNC_VERBOSE_STATE_DECL(a)
#define MPIR_FUNC_VERBOSE_ENTER(a)
#define MPIR_FUNC_VERBOSE_EXIT(a)
#define MPIR_FUNC_VERBOSE_PT2PT_ENTER(a)
#define MPIR_FUNC_VERBOSE_PT2PT_EXIT(a)
#define MPIR_FUNC_VERBOSE_RMA_ENTER(a)
#define MPIR_FUNC_VERBOSE_RMA_EXIT(a)

#endif /* HAVE_TIMING */

#endif /* ! defined(MPL_USE_DBG_LOGGING) && ! defined(MPICH_DEBUG_MEMARENA) */

#endif /* MPIR_FUNC_H_INCLUDED */

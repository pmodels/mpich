/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
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
#define MPIR_FUNC_ENTER_(level) MPL_DBG_MSG_S(MPL_DBG_ROUTINE_ENTER, level, "Entering %s", __func__)
#define MPIR_FUNC_EXIT_(level)  MPL_DBG_MSG_S(MPL_DBG_ROUTINE_EXIT, level, "Leaving %s", __func__)
#elif defined(MPICH_DEBUG_MEMARENA)
#define MPIR_FUNC_ENTER MPL_trvalid2("line %d - entering %s\n", __LINE__, __func__)
#define MPIR_FUNC_EXIT MPL_trvalid2("line %d - leaving %s\n", __LINE__, __func__)
#endif

/* state declaration macros */
#if defined(MPL_USE_DBG_LOGGING) || defined(MPICH_DEBUG_MEMARENA)

#define MPIR_FUNC_ENTER                   MPIR_FUNC_ENTER_(TYPICAL)
#define MPIR_FUNC_EXIT                    MPIR_FUNC_EXIT_(TYPICAL)
#define MPIR_FUNC_TERSE_ENTER             MPIR_FUNC_ENTER_(TERSE)
#define MPIR_FUNC_TERSE_EXIT              MPIR_FUNC_EXIT_(TERSE)
#define MPIR_FUNC_VERBOSE_ENTER           MPIR_FUNC_ENTER_(VERBOSE)
#define MPIR_FUNC_VERBOSE_EXIT            MPIR_FUNC_EXIT_(VERBOSE)

#else /* ! defined(MPL_USE_DBG_LOGGING) && ! defined(MPICH_DEBUG_MEMARENA) */

/* Routine tracing (see --enable-timing for control of this) */
#if defined(HAVE_TIMING) && (HAVE_TIMING == MPICH_TIMING_KIND__LOG || \
    HAVE_TIMING == MPICH_TIMING_KIND__LOG_DETAILED || \
    HAVE_TIMING == MPICH_TIMING_KIND__ALL || \
    HAVE_TIMING == MPICH_TIMING_KIND__RUNTIME)

#if (USE_LOGGING == MPICH_LOGGING__EXTERNAL)
#include "mpilogging.h"
#else
#error You must select a logging library or enable logging if timing is enabled
#endif

#else /* HAVE_TIMING and doing logging */

#define MPIR_FUNC_ENTER
#define MPIR_FUNC_EXIT
#define MPIR_FUNC_TERSE_ENTER
#define MPIR_FUNC_TERSE_EXIT
#define MPIR_FUNC_VERBOSE_ENTER
#define MPIR_FUNC_VERBOSE_EXIT

#endif /* HAVE_TIMING */

#endif /* ! defined(MPL_USE_DBG_LOGGING) && ! defined(MPICH_DEBUG_MEMARENA) */

#endif /* MPIR_FUNC_H_INCLUDED */

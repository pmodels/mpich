/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPIFUNC_H_INCLUDED
#define MPIFUNC_H_INCLUDED

/* state declaration macros */
#if defined(MPL_USE_DBG_LOGGING) || defined(MPICH_DEBUG_MEMARENA)
#define MPIR_STATE_DECL(a)
#define MPID_MPI_STATE_DECL(a)		MPIR_STATE_DECL(a)
#define MPID_MPI_INIT_STATE_DECL(a)	MPIR_STATE_DECL(a)
#define MPID_MPI_FINALIZE_STATE_DECL(a)	MPIR_STATE_DECL(a)
#define MPIDI_STATE_DECL(a)		MPIR_STATE_DECL(a)
#define MPIDI_INIT_STATE_DECL(a)	MPIR_STATE_DECL(a)
#define MPIDI_FINALIZE_STATE_DECL(a)	MPIR_STATE_DECL(a)

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

#endif /* MPIFUNC_H_INCLUDED */

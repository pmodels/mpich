/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2007 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if !defined(MPIFUNCMEM_H_INCLUDED)
#define MPIFUNCMEM_H_INCLUDED

/* state declaration macros */
#define MPIR_STATE_DECL(a)		
#define MPID_MPI_STATE_DECL(a)		MPIR_STATE_DECL(a)
#define MPID_MPI_INIT_STATE_DECL(a)	MPIR_STATE_DECL(a)
#define MPID_MPI_FINALIZE_STATE_DECL(a)	MPIR_STATE_DECL(a)
#define MPIDI_STATE_DECL(a)		MPIR_STATE_DECL(a)
#define MPIDI_INIT_STATE_DECL(a)	MPIR_STATE_DECL(a)
#define MPIDI_FINALIZE_STATE_DECL(a)	MPIR_STATE_DECL(a)

/* function enter and exit macros */
#define MPIR_FUNC_ENTER(a) MPIU_trvalid( "Entering " #a )

#define MPIR_FUNC_EXIT(a) MPIU_trvalid( "Exiting " #a )

/* Tell the package to define the rest of the enter/exit macros in 
   terms of these */
#define NEEDS_FUNC_ENTER_EXIT_DEFS 1

#endif /* defined(MPIFUNCMEM_H_INCLUDED) */

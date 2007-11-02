/* -*- Mode: C; c-basic-offset:4 ; -*- */
/* 
 *  (C) 2005 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPIFUNCLOG_H_INCLUDED
#define MPIFUNCLOG_H_INCLUDED

/* state declaration macros */
/* FIXME: To make MPIR_STATE_DECL() and the other xxx_DECL macros
   work with a mix of declarations, they need to have a dummy
   declations, e.g., int _sdummy=0, with no trailing semi-colon (this
   declaration then eats the semicolon after the xxx_DECL macro) */
#define MPIR_STATE_DECL(a)		
#define MPID_MPI_STATE_DECL(a)		MPIR_STATE_DECL(a)
#define MPID_MPI_INIT_STATE_DECL(a)	MPIR_STATE_DECL(a)
#define MPID_MPI_FINALIZE_STATE_DECL(a)	MPIR_STATE_DECL(a)
#define MPIDI_STATE_DECL(a)		MPIR_STATE_DECL(a)
#define MPIDI_INIT_STATE_DECL(a)	MPIR_STATE_DECL(a)
#define MPIDI_FINALIZE_STATE_DECL(a)	MPIR_STATE_DECL(a)
#define MPIDI_NEMTCP_STATE_DECL	MPIR_STATE_DECL(FCNAME)

/* FIXME: it would be nice to have a nesting level for the function call, 
   and use 
   MPIU_DBG_MSG_D(ROUTINE_ENTER,TYPICAL,"[%d]Entering "#a,MPIR_DBG_nestlevel++)
   and a similar value for the exit (not thread-safe, but useful in the
   single threaded case).
*/
/* function enter and exit macros */
#define MPIR_FUNC_ENTER(a) \
    MPIU_DBG_MSG(ROUTINE_ENTER,TYPICAL,"Entering "#a )

#define MPIR_FUNC_EXIT(a) \
    MPIU_DBG_MSG(ROUTINE_EXIT,TYPICAL,"Leaving "#a )

#define MPIDI_NEMTCP_FUNC_ENTER                                 \
    MPIU_DBG_MSG(NEM_SOCK_FUNC, TYPICAL, "Entering " FCNAME )

#define MPIDI_NEMTCP_FUNC_EXIT                                  \
    MPIU_DBG_MSG(NEM_SOCK_FUNC, TYPICAL, "Leaving " FCNAME )

/* Tell the package to define the rest of the enter/exit macros in 
   terms of these */
#define NEEDS_FUNC_ENTER_EXIT_DEFS 1

#endif 

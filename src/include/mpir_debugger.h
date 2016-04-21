/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */

#ifndef MPIR_DEBUGGER_H_INCLUDED
#define MPIR_DEBUGGER_H_INCLUDED

/* These macros allow us to implement a sendq when debugger support is
   selected.  As there is extra overhead for this, we only do this
   when specifically requested
*/
#ifdef HAVE_DEBUGGER_SUPPORT
void MPIR_WaitForDebugger( void );
void MPIR_DebuggerSetAborting( const char * );
void MPIR_Sendq_remember(MPIR_Request *, int, int, int );
void MPIR_Sendq_forget(MPIR_Request *);
void MPIR_CommL_remember( MPIR_Comm * );
void MPIR_CommL_forget( MPIR_Comm * );

#define MPIR_SENDQ_REMEMBER(_a,_b,_c,_d) MPIR_Sendq_remember(_a,_b,_c,_d)
#define MPIR_SENDQ_FORGET(_a) MPIR_Sendq_forget(_a)
#define MPIR_COMML_REMEMBER(_a) MPIR_CommL_remember( _a )
#define MPIR_COMML_FORGET(_a) MPIR_CommL_forget( _a )
#else
#define MPIR_SENDQ_REMEMBER(a,b,c,d)
#define MPIR_SENDQ_FORGET(a)
#define MPIR_COMML_REMEMBER(_a)
#define MPIR_COMML_FORGET(_a)
#endif

#endif /* MPIR_DEBUGGER_H_INCLUDED */

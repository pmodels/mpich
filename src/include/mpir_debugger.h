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
void MPIR_Debugger_set_aborting(const char *);

/* internal functions */
void MPII_Wait_for_debugger(void);
void MPII_Sendq_remember(MPIR_Request *, int, int, int);
void MPII_Sendq_forget(MPIR_Request *);
void MPII_CommL_remember(MPIR_Comm *);
void MPII_CommL_forget(MPIR_Comm *);

#define MPII_SENDQ_REMEMBER(_a,_b,_c,_d) MPII_Sendq_remember(_a,_b,_c,_d)
#define MPII_SENDQ_FORGET(_a) MPII_Sendq_forget(_a)
#define MPII_COMML_REMEMBER(_a) MPII_CommL_remember(_a)
#define MPII_COMML_FORGET(_a) MPII_CommL_forget(_a)
#define MPII_REQUEST_CLEAR_DBG(_r) ((_r)->u.send.dbg_next = NULL)
#else
#define MPII_SENDQ_REMEMBER(a,b,c,d)
#define MPII_SENDQ_FORGET(a)
#define MPII_COMML_REMEMBER(_a)
#define MPII_COMML_FORGET(_a)
#define MPII_REQUEST_CLEAR_DBG(_r)
#endif

#endif /* MPIR_DEBUGGER_H_INCLUDED */

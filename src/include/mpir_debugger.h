/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIR_DEBUGGER_H_INCLUDED
#define MPIR_DEBUGGER_H_INCLUDED

/* These macros allow us to implement a sendq when debugger support is
   selected.  As there is extra overhead for this, we only do this
   when specifically requested
*/

#ifdef HAVE_DEBUGGER_SUPPORT
void MPII_Wait_for_debugger(void);
void MPIR_Debugger_set_aborting(const char *msg);
/* internal functions */
void MPII_Debugq_remember(MPIR_Request * req, int rank, int tag, int context_id);
void MPII_Debugq_forget(MPIR_Request * req);
void MPII_CommL_remember(MPIR_Comm * comm);
void MPII_CommL_forget(MPIR_Comm * comm);

#define MPII_SENDQ_REMEMBER(_a,_b,_c,_d) MPII_Debugq_remember(_a,_b,_c,_d)
#define MPII_SENDQ_FORGET(_a) MPII_Debugq_forget(_a)
#define MPII_COMML_REMEMBER(_a) MPII_CommL_remember(_a)
#define MPII_COMML_FORGET(_a) MPII_CommL_forget(_a)
#define MPII_REQUEST_CLEAR_DBG(_r)                                      \
    if ((_r)->kind == MPIR_REQUEST_KIND__SEND) {                        \
        ((_r)->u.send.dbg = NULL);                                      \
    } else if ((_r)->kind == MPIR_REQUEST_KIND__PREQUEST_SEND) {        \
        ((_r)->u.persist.dbg = NULL);                                   \
    } else {                                                            \
        MPIR_Assert(0);                                                 \
    }

#else
static inline void MPII_Wait_for_debugger(void)
{
}

static inline void MPIR_Debugger_set_aborting(const char *dummy)
{
}

#define MPII_SENDQ_REMEMBER(a,b,c,d)
#define MPII_SENDQ_FORGET(a)
#define MPII_COMML_REMEMBER(_a)
#define MPII_COMML_FORGET(_a)
#define MPII_REQUEST_CLEAR_DBG(_r)
#endif

#endif /* MPIR_DEBUGGER_H_INCLUDED */

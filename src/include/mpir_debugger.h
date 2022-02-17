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
typedef struct MPIR_Debugq {
    MPIR_Request *req;
    int tag, rank, context_id;
    const void *buf;
    MPI_Aint count;
    struct MPIR_Debugq *next;
    struct MPIR_Debugq *prev;
} MPIR_Debugq;
extern MPIR_Debugq *MPIR_Sendq_head;
extern MPIR_Debugq *MPIR_Recvq_head;
extern MPIR_Debugq *MPIR_Unexpq_head;

void MPII_Wait_for_debugger(void);
void MPIR_Debugger_set_aborting(const char *msg);
/* internal functions */
void MPII_Debugq_remember(MPIR_Request * req, int rank, int tag, int context_id, const void *buf,
                          MPI_Aint count, MPIR_Debugq ** queue);
void MPII_Debugq_forget(MPIR_Request * req, MPIR_Debugq ** queue);
void MPII_CommL_remember(MPIR_Comm * comm);
void MPII_CommL_forget(MPIR_Comm * comm);

#define MPII_SENDQ_REMEMBER(_a,_b,_c,_d,_e,_f) MPII_Debugq_remember(_a,_b,_c,_d,_e,_f,&MPIR_Sendq_head)
#define MPII_SENDQ_FORGET(_a) MPII_Debugq_forget(_a,&MPIR_Sendq_head)
#define MPII_RECVQ_REMEMBER(_a,_b,_c,_d,_e,_f) MPII_Debugq_remember(_a,_b,_c,_d,_e,_f,&MPIR_Recvq_head)
#define MPII_RECVQ_FORGET(_a) MPII_Debugq_forget(_a,&MPIR_Recvq_head)
#define MPII_UNEXPQ_REMEMBER(_a,_b,_c,_d) MPII_Debugq_remember(_a,_b,_c,_d,NULL,0,&MPIR_Unexpq_head)
#define MPII_UNEXPQ_FORGET(_a) MPII_Debugq_forget(_a,&MPIR_Unexpq_head)
#define MPII_COMML_REMEMBER(_a) MPII_CommL_remember(_a)
#define MPII_COMML_FORGET(_a) MPII_CommL_forget(_a)
#define MPII_REQUEST_CLEAR_DBG(_r)                                      \
    (_r)->send = NULL;                                                  \
    (_r)->recv = NULL;                                                  \
    (_r)->unexp = NULL;

#else
static inline void MPII_Wait_for_debugger(void)
{
}

static inline void MPIR_Debugger_set_aborting(const char *dummy)
{
}

#define MPII_SENDQ_REMEMBER(a,b,c,d,e,f)
#define MPII_SENDQ_FORGET(a)
#define MPII_RECVQ_REMEMBER(a,b,c,d,e,f)
#define MPII_RECVQ_FORGET(a)
#define MPII_UNEXPQ_REMEMBER(a,b,c,d)
#define MPII_UNEXPQ_FORGET(a)
#define MPII_COMML_REMEMBER(_a)
#define MPII_COMML_FORGET(_a)
#define MPII_REQUEST_CLEAR_DBG(_r)
#endif

#endif /* MPIR_DEBUGGER_H_INCLUDED */

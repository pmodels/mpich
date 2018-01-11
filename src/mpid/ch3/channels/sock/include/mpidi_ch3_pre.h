/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPIDI_CH3_PRE_H_INCLUDED
#define MPIDI_CH3_PRE_H_INCLUDED

/* These macros unlock shared code */
#define MPIDI_CH3_USES_SOCK
#define MPIDI_CH3_USES_ACCEPTQ

/*
 * Features needed or implemented by the channel
 */
#undef MPID_USE_SEQUENCE_NUMBERS

/* FIXME: These should be removed */
#define MPIDI_DEV_IMPLEMENTS_KVS

/* FIXME: Are the following packet extensions?  Can the socket connect/accept
   packets be made part of the util/sock support? */
#define MPIDI_CH3_PKT_ENUM			\
MPIDI_CH3I_PKT_SC_OPEN_REQ,			\
MPIDI_CH3I_PKT_SC_CONN_ACCEPT,		        \
MPIDI_CH3I_PKT_SC_OPEN_RESP,			\
MPIDI_CH3I_PKT_SC_CLOSE

typedef struct {
    int dummy;  /* dummy variable to ensure we don't have an empty structure */
} MPIDI_CH3I_CH_comm_t;

/* This channel has no special channel data for the process group structure */

/* FIXME: Explain these; why is this separate from the VC state? */
typedef enum MPIDI_CH3I_VC_state
{
    MPIDI_CH3I_VC_STATE_UNCONNECTED,
    MPIDI_CH3I_VC_STATE_CONNECTING,
    MPIDI_CH3I_VC_STATE_CONNECTED,
    MPIDI_CH3I_VC_STATE_FAILED
}
MPIDI_CH3I_VC_state_t;

/* channel-specific fields for the VC structure */
typedef struct MPIDI_CH3I_VC
{
    struct MPIR_Request * sendq_head;
    struct MPIR_Request * sendq_tail;
    MPIDI_CH3I_VC_state_t state;
    struct MPIDI_CH3I_Sock *sock;
    struct MPIDI_CH3I_Connection * conn;
}
MPIDI_CH3I_VC;

#define MPIDI_CH3_VC_DECL struct MPIDI_CH3I_VC ch;

/*
 * MPIDI_CH3_REQUEST_DECL (additions to MPIR_Request)
 * The socket channel makes no additions
 */

/*
 * MPID_Progress_state - device/channel dependent state to be passed between
 * MPID_Progress_{start,wait,end}
 *
 */
typedef struct MPIDI_CH3I_Progress_state
{
    int completion_count;
}
MPIDI_CH3I_Progress_state;

#define MPIDI_CH3_PROGRESS_STATE_DECL MPIDI_CH3I_Progress_state ch;

/* This variable is used in the definitions of the MPID_Progress_xxx macros,
   and must be available to the routines in src/mpi */
extern volatile unsigned int MPIDI_CH3I_progress_completion_count;

/* MPICH_IS_THREADED isn't defined yet */
#if (MPICH_THREAD_LEVEL == MPI_THREAD_MULTIPLE)
#define MPIDI_CH3I_PROGRESS_WAKEUP                                                                \
    do {                                                                                          \
        if (MPIDI_CH3I_progress_blocked == TRUE && MPIDI_CH3I_progress_wakeup_signalled == FALSE) \
        {                                                                                         \
            MPIDI_CH3I_progress_wakeup_signalled = TRUE;                                          \
            MPIDI_CH3I_Progress_wakeup();                                                         \
        }                                                                                         \
    } while (0)
#endif

#endif /* MPIDI_CH3_PRE_H_INCLUDED */

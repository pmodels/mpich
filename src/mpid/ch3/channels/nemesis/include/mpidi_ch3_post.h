/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if !defined(MPICH_MPIDI_CH3_POST_H_INCLUDED)
#define MPICH_MPIDI_CH3_POST_H_INCLUDED

#define MPIDI_CH3_Progress_start(progress_state_)                                                       \
        (progress_state_)->ch.completion_count = OPA_load_int(&MPIDI_CH3I_progress_completion_count);
#define MPIDI_CH3_Progress_end(progress_state_)

enum {
    MPIDI_CH3_start_packet_handler_id = 128,
    MPIDI_CH3_continue_packet_handler_id = 129,
    MPIDI_CH3_CTS_packet_handler_id = 130,
    MPIDI_CH3_reload_IOV_or_done_handler_id = 131,
    MPIDI_CH3_reload_IOV_reply_handler_id = 132
};


int MPIDI_CH3I_Progress(MPID_Progress_state *progress_state, int blocking);
#define MPIDI_CH3_Progress_test() MPIDI_CH3I_Progress(NULL, FALSE)
#define MPIDI_CH3_Progress_wait(progress_state) MPIDI_CH3I_Progress(progress_state, TRUE)
#define MPIDI_CH3_Progress_poke() MPIDI_CH3I_Progress(NULL, FALSE)

void MPIDI_CH3I_Posted_recv_enqueued(MPID_Request *rreq);
/* returns non-zero when req has been matched by channel */
int  MPIDI_CH3I_Posted_recv_dequeued(MPID_Request *rreq);

/*
 * Enable optional functionality
 */
#define MPIDI_CH3_Comm_Spawn MPIDI_CH3_Comm_Spawn

#include "mpid_nem_post.h"

int MPIDI_CH3I_Register_anysource_notification(void (*enqueue_fn)(MPID_Request *rreq), int (*dequeue_fn)(MPID_Request *rreq));

#endif /* !defined(MPICH_MPIDI_CH3_POST_H_INCLUDED) */

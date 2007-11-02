/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if !defined(MPICH_MPIDI_CH3_POST_H_INCLUDED)
#define MPICH_MPIDI_CH3_POST_H_INCLUDED

/* #define MPIDI_CH3_EAGER_MAX_MSG_SIZE (1500 - sizeof(MPIDI_CH3_Pkt_t)) */
#define MPIDI_CH3_EAGER_MAX_MSG_SIZE 128000

/*
 * CH3 Progress routines (implemented as macros for performanace)
 */

#define MPIDI_CH3_Progress_start(progress_state_)			\
{									\
    (progress_state_)->ch.completion_count = MPIDI_CH3I_progress_completion_count;	\
}
#define MPIDI_CH3_Progress_end(progress_state_)
#define MPIDI_CH3_Progress_poke() (MPIDI_CH3_Progress_test())


#if defined(USE_FIXED_SPIN_WAITS) || !defined(MPID_CPU_TICK) || defined(USE_FIXED_ACTIVE_PROGRESS)
int MPIDI_CH3I_Progress(int blocking, MPID_Progress_state *state);
#define MPIDI_CH3_Progress_test() MPIDI_CH3I_Progress(FALSE, NULL)
#define MPIDI_CH3_Progress_wait(state) MPIDI_CH3I_Progress(TRUE, state)
#endif

#endif /* !defined(MPICH_MPIDI_CH3_POST_H_INCLUDED) */

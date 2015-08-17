/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if !defined(MPICH_MPIDI_CH3_POST_H_INCLUDED)
#define MPICH_MPIDI_CH3_POST_H_INCLUDED

/* FIXME: We need to document all of these parameters.  There should be no 
   ifdef that is not documented, other than those set by configure */
/*
 * MPIDI_CH3_EAGER_MAX_MSG_SIZE - threshold for switch between the eager and 
 * rendezvous protocolsa
 */
#if !defined(MPIDI_CH3_EAGER_MAX_MSG_SIZE)
#   define MPIDI_CH3_EAGER_MAX_MSG_SIZE (256 * 1024)
#endif


/*
 * CH3 Progress routines (implemented as macros for performanace)
 */
#define MPIDI_CH3_Progress_start(progress_state_)			\
{									\
    MPID_THREAD_CS_ENTER(POBJ, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);\
    (progress_state_)->ch.completion_count = MPIDI_CH3I_progress_completion_count;\
}
#define MPIDI_CH3_Progress_end(progress_state_) MPID_THREAD_CS_EXIT(POBJ, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX)
#define MPIDI_CH3_Progress_poke() (MPIDI_CH3_Progress_test())

int MPIDI_CH3I_Progress(int blocking, MPID_Progress_state *state);
#define MPIDI_CH3_Progress_test() MPIDI_CH3I_Progress(FALSE, NULL)
#define MPIDI_CH3_Progress_wait(state) MPIDI_CH3I_Progress(TRUE, state)

#endif /* !defined(MPICH_MPIDI_CH3_POST_H_INCLUDED) */


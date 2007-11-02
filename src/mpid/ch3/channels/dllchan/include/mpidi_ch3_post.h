/* -*- Mode: C; c-basic-offset:4 ; -*- */
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
 * rendezvous protocols
 */
#if !defined(MPIDI_CH3_EAGER_MAX_MSG_SIZE)
#   define MPIDI_CH3_EAGER_MAX_MSG_SIZE (256 * 1024)
#endif


/*
 * CH3 Progress routines (implemented as macros for performanace)
 * This is slightly different from the usual channel becuase we need 
 * to us a pointer to the completion count (so that we can dynamically
 * access if from the loaded channel library)
 */
#define MPIDI_CH3_Progress_start(progress_state_)			\
{									\
    (progress_state_)->ch.completion_count = *MPIDI_CH3I_progress_completion_count_ptr;\
}
#define MPIDI_CH3_Progress_end(progress_state_)
#define MPIDI_CH3_Progress_poke() (MPIDI_CH3_Progress_test())

/* The newer channels all define a single progress routine */
/*#define MPIDI_CH3_Progress_test() MPIU_CALL_MPIDI_CH3.Progress(FALSE, NULL)*/
/* #define MPIDI_CH3_Progress_wait(state) MPIU_CALL_MPIDI_CH3.Progress(TRUE, state) */

#endif /* !defined(MPICH_MPIDI_CH3_POST_H_INCLUDED) */


/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if !defined(MPICH_MPIDI_CH3_POST_H_INCLUDED)
#define MPICH_MPIDI_CH3_POST_H_INCLUDED

/*
 * MPIDI_CH3_EAGER_MAX_MSG_SIZE - threshold for switch between the eager and rendezvous protocolsa
 */
#if !defined(MPIDI_CH3_EAGER_MAX_MSG_SIZE)
#	ifndef USE_NO_PIN_CACHE
#		define MPIDI_CH3_EAGER_MAX_MSG_SIZE 4*(IBU_EAGER_PACKET_SIZE)
#	else
#		define MPIDI_CH3_EAGER_MAX_MSG_SIZE 8*(IBU_EAGER_PACKET_SIZE)
#	endif
#endif

/*
 * Channel level request management macros
 */
#define MPIDI_CH3_Request_add_ref(req)				\
{								\
    MPIU_Assert(HANDLE_GET_MPI_KIND(req->handle) == MPID_REQUEST);	\
    MPIR_Request_add_ref(req);					\
}

#define MPIDI_CH3_Request_release_ref(req, req_ref_count)	\
{								\
    MPIU_Assert(HANDLE_GET_MPI_KIND(req->handle) == MPID_REQUEST);	\
    MPIR_Request_release_ref(req, req_ref_count);		\
    MPIU_Assert(req->ref_count >= 0);				\
}

/*
 * MPIDI_CH3_Progress_signal_completion() is used to notify the progress
 * engine that a completion has occurred.  The multi-threaded version will need
 * to wake up any (and all) threads blocking in MPIDI_CH3_Progress().
 */
extern volatile unsigned int MPIDI_CH3I_progress_completion_count;
#if (MPICH_THREAD_LEVEL == MPI_THREAD_MULTIPLE)
    extern volatile int MPIDI_CH3I_progress_blocked;
    extern volatile int MPIDI_CH3I_progress_wakeup_signalled;

    void MPIDI_CH3I_Progress_wakeup(void);
#endif

#if (MPICH_THREAD_LEVEL != MPI_THREAD_MULTIPLE)
#   define MPIDI_CH3_Progress_signal_completion()	\
    {							\
        MPIDI_CH3I_progress_completion_count++;		\
    }
#else
#   define MPIDI_CH3_Progress_signal_completion()			\
    {									\
	MPIDI_CH3I_progress_completion_count++;				\
	if (MPIDI_CH3I_progress_blocked == TRUE && MPIDI_CH3I_progress_wakeup_signalled == FALSE)\
	{								\
	    MPIDI_CH3I_progress_wakeup_signalled = TRUE;		\
	    MPIDI_CH3I_Progress_wakeup();				\
	}								\
    }
#endif


/*
 * CH3 Progress routines (implemented as macros for performanace)
 */
#define MPIDI_CH3_Progress_start(progress_state_)			\
{									\
    (progress_state_)->ch.completion_count = MPIDI_CH3I_progress_completion_count;\
}
#define MPIDI_CH3_Progress_end(progress_state_)
#define MPIDI_CH3_Progress_poke() (MPIDI_CH3_Progress_test())

/*
#if defined(MPICH_SINGLE_THREADED)
#define MPIDI_CH3_Progress_start(state)
#define MPIDI_CH3_Progress_end(state)
#endif
*/

int MPIDI_CH3I_Progress(int blocking, MPID_Progress_state *state);
#define MPIDI_CH3_Progress_test() MPIDI_CH3I_Progress(FALSE, NULL)
#define MPIDI_CH3_Progress_wait(state) MPIDI_CH3I_Progress(TRUE, state)

#endif /* !defined(MPICH_MPIDI_CH3_POST_H_INCLUDED) */

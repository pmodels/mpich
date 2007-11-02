/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if !defined(MPICH_MPIDI_CH3_PRE_H_INCLUDED)
#define MPICH_MPIDI_CH3_PRE_H_INCLUDED

#include "mpidi_ch3i_shm_conf.h"
#include "mpid_locksconf.h"

/* Feature and capabilities */
/* shm does not support any of the dynamic process operations (ports,
   spawn, etc.) */
#define MPIDI_CH3_HAS_NO_DYNAMIC_PROCESS

/*#define MPICH_DBG_OUTPUT*/

#if defined (HAVE_SHM_OPEN) && defined (HAVE_MMAP)
#define USE_POSIX_SHM
#elif defined (HAVE_SHMGET) && defined (HAVE_SHMAT) && defined (HAVE_SHMCTL) && defined (HAVE_SHMDT)
#define USE_SYSV_SHM
#elif defined (HAVE_WINDOWS_H)
#define USE_WINDOWS_SHM
#else
#error No shared memory subsystem defined
#endif

#define MPIDI_MAX_SHM_NAME_LENGTH 100

#if 0
/*
 * MPIDI_CH3_REQUEST_DECL (additions to MPID_Request)
 */
#define MPIDI_CH3_REQUEST_DECL						\
struct MPIDI_CH3I_Request						\
{									\
    /* iov_offset points to the current head element in the IOV */	\
    int iov_offset;			\
} ch;
/* Use MPIDI_CH3_REQUEST_INIT to initialize the channel-specific fields
   in the request */
#define MPIDI_CH3_REQUEST_INIT(req_) \
    (req_)->ch.iov_offset=0

#endif
/*
 * MPID_Progress_state - device/channel dependent state to be passed between 
 * MPID_Progress_{start,wait,end}
 */
typedef struct MPIDI_CH3I_Progress_state
{
    int completion_count;
} MPIDI_CH3I_Progress_state;

#define MPIDI_CH3_PROGRESS_STATE_DECL MPIDI_CH3I_Progress_state ch;

#define MPIDI_CH3_REQUEST_KIND_DECL \
MPIDI_CH3I_IOV_WRITE_REQUEST, \
MPIDI_CH3I_IOV_READ_REQUEST, \
MPIDI_CH3I_RTS_IOV_READ_REQUEST

/*
#define MPIDI_CH3I_IOV_WRITE_REQUEST MPID_LAST_REQUEST_KIND + 1
#define MPIDI_CH3I_IOV_READ_REQUEST MPID_LAST_REQUEST_KIND + 2
#define MPIDI_CH3I_RTS_IOV_READ_REQUEST MPID_LAST_REQUEST_KIND + 3
*/

#endif /* !defined(MPICH_MPIDI_CH3_PRE_H_INCLUDED) */

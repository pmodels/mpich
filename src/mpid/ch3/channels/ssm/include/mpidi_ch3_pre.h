/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if !defined(MPICH_MPIDI_CH3_PRE_H_INCLUDED)
#define MPICH_MPIDI_CH3_PRE_H_INCLUDED

/* mpidu_sock.h includes its own header file, so it may come before the 
   channel conf file */
/*#include "mpidu_sock.h" */
#include "mpidi_ch3i_ssm_conf.h"
#include "mpidi_ch3_conf.h"

/* These macros unlock shared code */
#define MPIDI_CH3_USES_SOCK
#define MPIDI_CH3_USES_SSHM
#define MPIDI_CH3_USES_ACCEPTQ
#ifdef USE_MQSHM
#define MPIDI_CH3_USES_SHM_NAME
#endif

/*
 * Features needed or implemented by the channel
 */
#define MPIDI_DEV_IMPLEMENTS_KVS
#define MPIDI_DEV_IMPLEMENTS_ABORT

#if defined (HAVE_SHM_OPEN) && defined (HAVE_MMAP)
#define USE_POSIX_SHM
#elif defined (HAVE_SHMGET) && defined (HAVE_SHMAT) && \
      defined (HAVE_SHMCTL) && defined (HAVE_SHMDT) && \
      defined (HAVE_WORKING_SHMGET)
#define USE_SYSV_SHM
#elif defined (HAVE_WINDOWS_H)
#define USE_WINDOWS_SHM
#else
#error No shared memory subsystem defined
#endif

#ifndef USE_MQSHM
#ifdef HAVE_MQ_OPEN
#define USE_POSIX_MQ
#elif defined(HAVE_MSGGET)
#define USE_SYSV_MQ
#endif
#endif

#define MPIDI_MAX_SHM_NAME_LENGTH 100

#define MPIDI_CH3_PKT_ENUM			\
MPIDI_CH3I_PKT_SC_OPEN_REQ,			\
MPIDI_CH3I_PKT_SC_CONN_ACCEPT,		        \
MPIDI_CH3I_PKT_SC_OPEN_RESP,			\
MPIDI_CH3I_PKT_SC_CLOSE,                        \
MPIDI_CH3_PKT_RTS_IOV,                          \
MPIDI_CH3_PKT_CTS_IOV,                          \
MPIDI_CH3_PKT_RELOAD,                           \
MPIDI_CH3_PKT_IOV

#define MPIDI_CH3_PKT_RELOAD_SEND 1
#define MPIDI_CH3_PKT_RELOAD_RECV 0

typedef enum MPIDI_CH3I_VC_state
{
    MPIDI_CH3I_VC_STATE_UNCONNECTED,
    MPIDI_CH3I_VC_STATE_CONNECTING,
    MPIDI_CH3I_VC_STATE_CONNECTED,
    MPIDI_CH3I_VC_STATE_FAILED
}
MPIDI_CH3I_VC_state_t;

#define MPIDI_CH3_REQUEST_KIND_DECL \
MPIDI_CH3I_IOV_WRITE_REQUEST, \
MPIDI_CH3I_IOV_READ_REQUEST, \
MPIDI_CH3I_RTS_IOV_READ_REQUEST

typedef struct MPIDI_CH3I_Progress_state
{
    int completion_count;
}
MPIDI_CH3I_Progress_state;

#define MPIDI_CH3_PROGRESS_STATE_DECL MPIDI_CH3I_Progress_state ch;

#endif /* !defined(MPICH_MPIDI_CH3_PRE_H_INCLUDED) */

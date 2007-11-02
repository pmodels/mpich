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

#if 0

/* for MAXHOSTNAMELEN under Linux ans OSX */
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 256
#endif

typedef struct MPIDI_CH3I_BootstrapQ_struct * MPIDI_CH3I_BootstrapQ;

typedef struct MPIDI_Process_group_s
{
    int nShmEagerLimit;
#ifdef HAVE_SHARED_PROCESS_READ
    int nShmRndvLimit;
#endif
    int nShmWaitSpinCount;
    int nShmWaitYieldCount;
    MPIDI_CH3I_BootstrapQ bootstrapQ;
    char shm_hostname[MAXHOSTNAMELEN];
#ifdef MPIDI_CH3_USES_SHM_NAME
    char * shm_name;
#endif
    /*struct MPIDI_Process_group_s *next;*/
}
MPIDI_CH3I_Process_group_t;

#define MPIDI_CH3_PG_DECL MPIDI_CH3I_Process_group_t ch;
#endif /* 0 */

#define MPIDI_CH3_PKT_ENUM			\
MPIDI_CH3I_PKT_SC_OPEN_REQ,			\
MPIDI_CH3I_PKT_SC_CONN_ACCEPT,		        \
MPIDI_CH3I_PKT_SC_OPEN_RESP,			\
MPIDI_CH3I_PKT_SC_CLOSE,                        \
MPIDI_CH3_PKT_RTS_IOV,                          \
MPIDI_CH3_PKT_CTS_IOV,                          \
MPIDI_CH3_PKT_RELOAD,                           \
MPIDI_CH3_PKT_IOV

#if 0
#define MPIDI_CH3_PKT_DEFS						\
typedef struct								\
{									\
    MPIDI_CH3_Pkt_type_t type;						\
    /* FIXME - We need a little security here to avoid having a random \
            port scan crash the process.  Perhaps a "secret" value for \
       each process could be published in the key-val space and        \
       subsequently sent in the open pkt. */		  	        \
									\
    /* FIXME - We need some notion of a global process group ID so that \
         we can tell the remote process which process is		\
       connecting to it */						\
    int pg_id_len;							\
    int pg_rank;							\
}									\
MPIDI_CH3I_Pkt_sc_open_req_t;						\
									\
typedef struct								\
{									\
    MPIDI_CH3_Pkt_type_t type;						\
    int ack;								\
}									\
MPIDI_CH3I_Pkt_sc_open_resp_t;						\
									\
typedef struct								\
{									\
    MPIDI_CH3_Pkt_type_t type;						\
}									\
MPIDI_CH3I_Pkt_sc_close_t;                                              \
                                                                        \
typedef struct								\
{									\
    MPIDI_CH3_Pkt_type_t type;						\
    int port_name_tag; 							\
}									\
MPIDI_CH3I_Pkt_sc_conn_accept_t;					\
									\
typedef struct MPIDI_CH3_Pkt_rdma_rts_iov_t				\
{									\
    MPIDI_CH3_Pkt_type_t type;						\
    MPI_Request sreq;							\
    int iov_len;							\
} MPIDI_CH3_Pkt_rdma_rts_iov_t;						\
typedef struct MPIDI_CH3_Pkt_rdma_cts_iov_t				\
{									\
    MPIDI_CH3_Pkt_type_t type;						\
    MPI_Request sreq, rreq;						\
    int iov_len;							\
} MPIDI_CH3_Pkt_rdma_cts_iov_t;						\
typedef struct MPIDI_CH3_Pkt_rdma_reload_t				\
{									\
    MPIDI_CH3_Pkt_type_t type;						\
    int send_recv;							\
    MPI_Request sreq, rreq;						\
} MPIDI_CH3_Pkt_rdma_reload_t;						\
typedef struct MPIDI_CH3_Pkt_rdma_iov_t					\
{									\
    MPIDI_CH3_Pkt_type_t type;						\
    MPI_Request req;							\
    int send_recv;							\
    int iov_len;							\
} MPIDI_CH3_Pkt_rdma_iov_t;

#define MPIDI_CH3_PKT_DECL			    \
MPIDI_CH3I_Pkt_sc_open_req_t sc_open_req;	    \
MPIDI_CH3I_Pkt_sc_conn_accept_t sc_conn_accept;	    \
MPIDI_CH3I_Pkt_sc_open_resp_t sc_open_resp;	    \
MPIDI_CH3I_Pkt_sc_close_t sc_close;		    \
MPIDI_CH3_Pkt_rdma_rts_iov_t rts_iov;		    \
MPIDI_CH3_Pkt_rdma_cts_iov_t cts_iov;		    \
MPIDI_CH3_Pkt_rdma_reload_t reload;		    \
MPIDI_CH3_Pkt_rdma_iov_t iov;

#endif 

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

#if 0
/* This structure requires the iovec structure macros to be defined */
typedef struct MPIDI_CH3I_SHM_Buffer_t
{
    int use_iov;
    unsigned int num_bytes;
    void *buffer;
    unsigned int bufflen;
#ifdef USE_SHM_IOV_COPY
    MPID_IOV iov[MPID_IOV_LIMIT];
#else
    MPID_IOV *iov;
#endif
    int iovlen;
    int index;
    int total;
} MPIDI_CH3I_SHM_Buffer_t;

typedef struct MPIDI_CH3I_Shmem_block_request_result
{
    int error;
    void *addr;
    unsigned int size;
#ifdef USE_POSIX_SHM
    char key[MPIDI_MAX_SHM_NAME_LENGTH];
    int id;
#elif defined (USE_SYSV_SHM)
    int key;
    int id;
#elif defined (USE_WINDOWS_SHM)
    char key[MPIDI_MAX_SHM_NAME_LENGTH];
    HANDLE id;
#else
#error *** No shared memory mapping variables specified ***
#endif
    char name[MPIDI_MAX_SHM_NAME_LENGTH];
} MPIDI_CH3I_Shmem_block_request_result;

typedef struct MPIDI_CH3I_VC
{
    struct MPIDI_CH3I_SHM_Queue_t * shm, * read_shmq, * write_shmq;
    struct MPID_Request * sendq_head;
    struct MPID_Request * sendq_tail;
    struct MPID_Request * send_active;
    struct MPID_Request * recv_active;
    struct MPID_Request * req;
    MPIDI_CH3I_VC_state_t state;
    int shm_read_connected;
    struct MPIDU_Sock *sock;
    struct MPIDI_CH3I_Connection * conn;
    int port_name_tag;
    BOOL bShm;
    MPIDI_CH3I_Shmem_block_request_result shm_write_queue_info, shm_read_queue_info;
    int shm_reading_pkt;
    int shm_state;
    MPIDI_CH3I_SHM_Buffer_t read;
#ifdef HAVE_SHARED_PROCESS_READ
#ifdef HAVE_WINDOWS_H
    HANDLE hSharedProcessHandle;
#else
    int nSharedProcessID;
    int nSharedProcessFileDescriptor;
#endif
#endif
    struct MPIDI_VC *shm_next_reader, *shm_next_writer;
} MPIDI_CH3I_VC;

#endif

#if 0
#define MPIDI_CH3_VC_DECL MPIDI_CH3I_VC ch;
#endif

#define MPIDI_CH3_REQUEST_KIND_DECL \
MPIDI_CH3I_IOV_WRITE_REQUEST, \
MPIDI_CH3I_IOV_READ_REQUEST, \
MPIDI_CH3I_RTS_IOV_READ_REQUEST

#if 0
/*
 * MPIDI_CH3_REQUEST_DECL (additions to MPID_Request)
 */
#define MPIDI_CH3_REQUEST_DECL						\
struct MPIDI_CH3I_Request						\
{									\
    /* iov_offset points to the current head element in the IOV */	\
    int iov_offset;							\
									\
    struct MPID_Request *req;					\
} ch;

/* Use MPIDI_CH3_REQUEST_INIT to initialize the channel-specific fields
   in the request */
#define MPIDI_CH3_REQUEST_INIT(req_) \
    (req_)->ch.iov_offset=0;\
    (req_)->ch.req=NULL

#endif /* 0 */

typedef struct MPIDI_CH3I_Progress_state
{
    int completion_count;
}
MPIDI_CH3I_Progress_state;

#define MPIDI_CH3_PROGRESS_STATE_DECL MPIDI_CH3I_Progress_state ch;

#endif /* !defined(MPICH_MPIDI_CH3_PRE_H_INCLUDED) */

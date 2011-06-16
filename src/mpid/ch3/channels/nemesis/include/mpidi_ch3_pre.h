/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if !defined(MPICH_MPIDI_CH3_PRE_H_INCLUDED)
#define MPICH_MPIDI_CH3_PRE_H_INCLUDED
#include "mpid_nem_pre.h"

#if defined(HAVE_NETINET_IN_H)
    #include <netinet/in.h>
#elif defined(HAVE_WINSOCK2_H)
    #include <winsock2.h>
    #include <windows.h>
#endif
#include "opa_primitives.h"

/*#define MPID_USE_SEQUENCE_NUMBERS*/
/*#define HAVE_CH3_PRE_INIT*/
/* #define MPIDI_CH3_HAS_NO_DYNAMIC_PROCESS */
#define MPIDI_DEV_IMPLEMENTS_KVS
    
typedef enum MPIDI_CH3I_VC_state
{
    MPIDI_CH3I_VC_STATE_UNCONNECTED,
    MPIDI_CH3I_VC_STATE_CONNECTING,
    MPIDI_CH3I_VC_STATE_CONNECTED,
    MPIDI_CH3I_VC_STATE_FAILED
}
MPIDI_CH3I_VC_state_t;

/* size of private data area in vc and req for network modules */
#define MPID_NEM_VC_NETMOD_AREA_LEN 128
#define MPID_NEM_REQ_NETMOD_AREA_LEN 64

struct MPIDI_CH3I_Request
{
    struct MPIDI_VC     *vc;
    int                  noncontig;
    MPIDI_msg_sz_t       header_sz;

    MPI_Request          lmt_req_id;     /* request id of remote side */
    struct MPID_Request *lmt_req;        /* pointer to original send/recv request */
    MPIDI_msg_sz_t       lmt_data_sz;    /* data size to be transferred, after checking for truncation */
    MPID_IOV             lmt_tmp_cookie; /* temporary storage for received cookie */
    void                *s_cookie;       /* temporary storage for the cookie data in case the packet can't be sent immediately */

    struct
    {
        char padding[MPID_NEM_REQ_NETMOD_AREA_LEN];
    } netmod_area;
};

/*
 * MPIDI_CH3_REQUEST_DECL (additions to MPID_Request)
 */
#define MPIDI_CH3_REQUEST_DECL struct MPIDI_CH3I_Request ch;


#if 0
#define DUMP_REQUEST(req) do {                                                          \
        int i;                                                                          \
        MPIDI_DBG_PRINTF((55, FCNAME, "request %p\n", (req)));                          \
        MPIDI_DBG_PRINTF((55, FCNAME, "  handle = %d\n", (req)->handle));		\
        MPIDI_DBG_PRINTF((55, FCNAME, "  ref_count = %d\n", (req)->ref_count));         \
        MPIDI_DBG_PRINTF((55, FCNAME, "  cc = %d\n", (req)->cc));			\
        for (i = 0; i < (req)->iov_count; ++i)                                          \
            MPIDI_DBG_PRINTF((55, FCNAME, "  dev.iov[%d] = (%p, %d)\n", i,		\
                              (req)->dev.iov[i+(req)->dev.iov_offset].MPID_IOV_BUF,     \
                              (req)->dev.iov[i+(req)->dev.iov_offset].MPID_IOV_LEN));  \
    MPIDI_DBG_PRINTF((55, FCNAME, "  dev.iov_count = %d\n",                             \
                      (req)->dev.iov_count));                                           \
    MPIDI_DBG_PRINTF((55, FCNAME, "  dev.state = 0x%x\n", (req)->dev.state));           \
    MPIDI_DBG_PRINTF((55, FCNAME, "    type = %d\n",                                    \
		      MPIDI_Request_get_type(req)));                                    \
    } while (0)
#else
#define DUMP_REQUEST(req) do { } while (0)
#endif


#define MPIDI_POSTED_RECV_ENQUEUE_HOOK(req) MPIDI_CH3I_Posted_recv_enqueued(req)
#define MPIDI_POSTED_RECV_DEQUEUE_HOOK(req) MPIDI_CH3I_Posted_recv_dequeued(req)


typedef struct MPIDI_CH3I_comm
{
    /* FIXME we should really use the copy of these values that is stored in the
       MPID_Comm structure */
    int local_size;      /* number of local procs in this comm */
    int local_rank;      /* my rank among local procs in this comm */
    int *local_ranks;    /* list of ranks of procs local to this node */
    int external_size;   /* number of procs in external set */
    int external_rank;   /* my rank among external set, or -1 if I'm not in external set */
    int *external_ranks; /* list of ranks of procs in external set */
    int *intranode_table;
    int *internode_table;
    struct MPID_nem_barrier_vars *barrier_vars; /* shared memory variables used in barrier */
}
MPIDI_CH3I_comm_t;

#ifdef ENABLED_SHM_COLLECTIVES
#define HAVE_DEV_COMM_HOOK
#define MPID_Dev_comm_create_hook(comm_) do {           \
        int _mpi_errno;                                 \
        _mpi_errno = MPIDI_CH3I_comm_create (comm_);    \
        if (_mpi_errno) MPIU_ERR_POP (_mpi_errno);      \
    } while(0)


#define MPID_Dev_comm_destroy_hook(comm_) do {          \
        int _mpi_errno;                                 \
        _mpi_errno = MPIDI_CH3I_comm_destroy (comm_);   \
        if (_mpi_errno) MPIU_ERR_POP (_mpi_errno);      \
    } while(0)

#endif
#define MPID_DEV_COMM_DECL MPIDI_CH3I_comm_t ch;

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

extern OPA_int_t MPIDI_CH3I_progress_completion_count;
#define MPIDI_CH3I_INCR_PROGRESS_COMPLETION_COUNT do {                                  \
        OPA_write_barrier();                                                            \
        OPA_incr_int(&MPIDI_CH3I_progress_completion_count);                            \
        MPIU_DBG_MSG_D(CH3_PROGRESS,VERBOSE,                                            \
                       "just incremented MPIDI_CH3I_progress_completion_count=%d",      \
                       OPA_load_int(&MPIDI_CH3I_progress_completion_count));            \
    } while(0)

#endif /* !defined(MPICH_MPIDI_CH3_PRE_H_INCLUDED) */


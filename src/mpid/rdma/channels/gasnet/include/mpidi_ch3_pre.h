/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if !defined(MPICH_MPIDI_CH3_PRE_H_INCLUDED)
#define MPICH_MPIDI_CH3_PRE_H_INCLUDED

#if ! defined (NDEBUG) && ! defined (DEBUG)
#define NDEBUG
#define GASNET_SEQ 
#include "gasnet.h"
#undef NDEBUG
#else
#define GASNET_SEQ 
#include "gasnet.h"
#endif

/*#define MPID_USE_SEQUENCE_NUMBERS*/

typedef struct MPIDI_CH3I_Process_group_s
{
    volatile int ref_count;
    char * kvs_name;
    int size;
    struct MPIDI_VC * vc_table;
}
MPIDI_CH3I_Process_group_t;

typedef enum MPIDI_CH3I_VC_state
{
    MPIDI_CH3I_VC_STATE_UNCONNECTED,
    MPIDI_CH3I_VC_STATE_CONNECTING,
    MPIDI_CH3I_VC_STATE_CONNECTED,
    MPIDI_CH3I_VC_STATE_FAILED
}
MPIDI_CH3I_VC_state_t;

typedef struct MPIDI_CH3I_VC
{
    MPIDI_CH3I_Process_group_t * pg;
    int pg_rank;
    int data_sz;
    void * data;
    struct MPID_Request *recv_active;
} MPIDI_CH3I_VC;

#define MPIDI_CH3_VC_DECL MPIDI_CH3I_VC gasnet;


/*
 * MPIDI_CH3_CA_ENUM (additions to MPIDI_CA_t)
 */
#define MPIDI_CH3_CA_ENUM			\
MPIDI_CH3I_CA_END_GASNET_CHANNEL

typedef enum MPIDI_CH3I_RNDV_state
{
    MPIDI_CH3_RNDV_NEW,
    MPIDI_CH3_RNDV_CURRENT,
    MPIDI_CH3_RNDV_WAIT
}
MPIDI_CH3I_RNDV_state_t;

/*
 * MPIDI_CH3_REQUEST_DECL (additions to MPID_Request)
 */
#define MPIDI_CH3_REQUEST_DECL						\
struct MPIDI_CH3I_Request						\
{									\
    MPIDI_VC *vc;							\
    /* iov_offset points to the current head element in the IOV */	\
    MPIDI_CH3I_RNDV_state_t rndv_state;					\
    gasnet_handle_t rndv_handle;					\
    int remote_req_id;							\
    MPID_IOV remote_iov[MPID_IOV_LIMIT];				\
    int iov_bytes;							\
    int remote_iov_bytes;						\
    int iov_offset;							\
    int remote_iov_offset;						\
    int remote_iov_count;						\
    MPIDI_CH3_Pkt_t pkt;						\
} gasnet;

#if 0
#define DUMP_REQUEST(req) do {							\
    int i;									\
    printf_d ("request %p\n", (req));						\
    printf_d ("  handle = %d\n", (req)->handle);				\
    printf_d ("  ref_count = %d\n", (req)->ref_count);				\
    printf_d ("  cc = %d\n", (req)->cc);					\
    for (i = 0; i < (req)->iov_count; ++i)					\
        printf_d ("  dev.iov[%d] = (%p, %d)\n", i,				\
                (req)->dev.iov[i].MPID_IOV_BUF,					\
                (req)->dev.iov[i].MPID_IOV_LEN);				\
    printf_d ("  dev.iov_count = %d\n", (req)->dev.iov_count);			\
    printf_d ("  dev.ca = %d\n", (req)->dev.ca);				\
    printf_d ("  dev.state = 0x%x\n", (req)->dev.state);			\
    printf_d ("    type = %d\n", MPIDI_Request_get_type(req));			\
    printf_d ("  gasnet.rndv_state = %d\n", (req)->gasnet.rndv_state);		\
    printf_d ("  gasnet.remote_req_id = %d\n", (req)->gasnet.remote_req_id);	\
} while (0)
#else
#define DUMP_REQUEST(req) do { } while (0)
#endif

#define MPID_STATE_LIST_CH3 \
MPID_STATE_MPIDI_CH3_CANCEL_SEND, \
MPID_STATE_MPIDI_CH3_COMM_SPAWN, \
MPID_STATE_MPIDI_CH3_FINALIZE, \
MPID_STATE_MPIDI_CH3_INIT, \
MPID_STATE_MPIDI_CH3_IREAD, \
MPID_STATE_MPIDI_CH3_ISEND, \
MPID_STATE_MPIDI_CH3_ISENDV, \
MPID_STATE_MPIDI_CH3_ISTARTMSG, \
MPID_STATE_MPIDI_CH3_ISTARTMSGV, \
MPID_STATE_MPIDI_CH3_IWRITE, \
MPID_STATE_MPIDI_CH3_PROGRESS_INIT, \
MPID_STATE_MPIDI_CH3_PROGRESS_FINALIZE, \
MPID_STATE_MPIDI_CH3_PROGRESS_START, \
MPID_STATE_MPIDI_CH3_PROGRESS_END, \
MPID_STATE_MPIDI_CH3_PROGRESS, \
MPID_STATE_MPIDI_CH3_PROGRESS_POKE, \
MPID_STATE_MPIDI_CH3_REQUEST_CREATE, \
MPID_STATE_MPIDI_CH3_REQUEST_ADD_REF, \
MPID_STATE_MPIDI_CH3_REQUEST_RELEASE_REF, \
MPID_STATE_MPIDI_CH3_REQUEST_DESTROY, \
MPID_STATE_MPIDI_CH3I_LISTENER_GET_PORT, \
MPID_STATE_MPIDI_CH3I_PROGRESS_FINALIZE, \
MPID_STATE_MPIDI_CH3I_PROGRESS_INIT, \
MPID_STATE_MPIDI_CH3I_VC_POST_CONNECT, \
MPID_STATE_MPIDI_CH3I_VC_POST_READ, \
MPID_STATE_MPIDI_CH3I_VC_POST_WRITE, \
MPID_STATE_MPIDI_CH3I_GET_BUSINESS_CARD, \
MPID_STATE_MPIDI_CH3U_BUFFER_COPY, \
MPID_STATE_CONNECTION_ALLOC, \
MPID_STATE_CONNECTION_FREE, \
MPID_STATE_CONNECTION_POST_SENDQ_REQ, \
MPID_STATE_CONNECTION_POST_SEND_PKT, \
MPID_STATE_CONNECTION_POST_RECV_PKT, \
MPID_STATE_CONNECTION_SEND_FAIL, \
MPID_STATE_CONNECTION_RECV_FAIL, \
MPID_STATE_UPDATE_REQUEST, \
SOCK_STATE_LIST

#endif /* !defined(MPICH_MPIDI_CH3_PRE_H_INCLUDED) */

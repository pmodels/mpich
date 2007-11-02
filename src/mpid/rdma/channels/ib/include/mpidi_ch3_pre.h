/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if !defined(MPICH_MPIDI_CH3_PRE_H_INCLUDED)
#define MPICH_MPIDI_CH3_PRE_H_INCLUDED

/*#define MPID_USE_SEQUENCE_NUMBERS*/

typedef struct MPIDI_CH3I_Process_group_s
{
    volatile int ref_count;
    char * kvs_name;
    char * pg_id;
    int size;
    struct MPIDI_VC * vc_table;
    struct MPIDI_CH3I_Process_group_s *next;
}
MPIDI_CH3I_Process_group_t;

typedef enum MPIDI_CH3I_VC_state
{
    MPIDI_CH3I_VC_STATE_UNCONNECTED,
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
    struct MPID_Request * sendq_head;
    struct MPID_Request * sendq_tail;
    ibu_t ibu;
    struct MPID_Request * send_active;
    struct MPID_Request * recv_active;
    struct MPID_Request * req;
    int reading_pkt;
    MPIDI_CH3I_VC_state_t state;
} MPIDI_CH3I_VC;

#define MPIDI_CH3_VC_DECL MPIDI_CH3I_VC ch;


/*
 * MPIDI_CH3_CA_ENUM (additions to MPIDI_CA_t)
 */
#define MPIDI_CH3_CA_ENUM			\
MPIDI_CH3I_CA_HANDLE_PKT,			\
MPIDI_CH3I_CA_END_IB_CHANNEL

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
} ch;

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

#endif /* !defined(MPICH_MPIDI_CH3_PRE_H_INCLUDED) */

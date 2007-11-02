/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if !defined(MPICH_MPIDI_CH3_PRE_H_INCLUDED)
#define MPICH_MPIDI_CH3_PRE_H_INCLUDED

#if ! GASNET_NDEBUG && ! GASNET_DEBUG
#define GASNET_NDEBUG 1
#endif

#define GASNET_SEQ 

#include "gasnet.h"

/*#define MPID_USE_SEQUENCE_NUMBERS*/

/*#define MPIDI_CH3_CHANNEL_RNDV*/

#define HAVE_CH3_PRE_INIT
#define MPIDI_CH3_HAS_NO_DYNAMIC_PROCESS

typedef struct MPIDI_CH3I_PG
{
    char *kvs_name;
}
MPIDI_CH3I_PG_t;
#define MPIDI_CH3_PG_DECL MPIDI_CH3I_PG_t ch;

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
    int pg_rank;
    int data_sz;
    void * data;
    struct MPID_Request *recv_active;
} MPIDI_CH3I_VC;

#define MPIDI_CH3_VC_DECL MPIDI_CH3I_VC gasnet;


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
    MPIDI_VC_t *vc;							\
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
#define DUMP_REQUEST(req) do {						\
    int i;								\
    MPIDI_DBG_PRINTF((55, FCNAME, "request %p\n", (req)));		\
    MPIDI_DBG_PRINTF((55, FCNAME, "  handle = %d\n", (req)->handle));	\
    MPIDI_DBG_PRINTF((55, FCNAME, "  ref_count = %d\n", (req)->ref_count));\
    MPIDI_DBG_PRINTF((55, FCNAME, "  cc = %d\n", (req)->cc));		\
    for (i = 0; i < (req)->iov_count; ++i)				\
        MPIDI_DBG_PRINTF((55, FCNAME, "  dev.iov[%d] = (%p, %d)\n", i,	\
                (req)->dev.iov[i].MPID_IOV_BUF,				\
                (req)->dev.iov[i].MPID_IOV_LEN));			\
    MPIDI_DBG_PRINTF((55, FCNAME, "  dev.iov_count = %d\n",		\
			 (req)->dev.iov_count));			\
    MPIDI_DBG_PRINTF((55, FCNAME, "  dev.ca = %d\n", (req)->dev.ca));	\
    MPIDI_DBG_PRINTF((55, FCNAME, "  dev.state = 0x%x\n", (req)->dev.state));\
    MPIDI_DBG_PRINTF((55, FCNAME, "    type = %d\n",			\
		      MPIDI_Request_get_type(req)));			\
    MPIDI_DBG_PRINTF((55, FCNAME, "  gasnet.rndv_state = %d\n",		\
		      (req)->gasnet.rndv_state));			\
    MPIDI_DBG_PRINTF((55, FCNAME, "  gasnet.remote_req_id = %d\n",	\
		      (req)->gasnet.remote_req_id));			\
} while (0)
#else
#define DUMP_REQUEST(req) do { } while (0)
#endif

#endif /* !defined(MPICH_MPIDI_CH3_PRE_H_INCLUDED) */

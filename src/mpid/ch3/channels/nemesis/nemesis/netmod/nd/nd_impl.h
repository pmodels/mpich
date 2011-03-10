/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef ND_IMPL_H_INCLUDED
#define ND_IMPL_H_INCLUDED

#define MPICH_SKIP_MPICXX
/* FIXME: Declare all MPICH headers under extern, if possible */
/* FIXME: Change #define const to const int */
#include <intrin.h>
extern "C"{
#include "mpid_nem_impl.h"
#include "mpiu_os_wrappers.h"
#include "pmi.h"
#include "mpiu_ex.h"
}
/* Include headers to handle COM error codes */
#include <comdef.h>
/* Include headers from the MS HPC SDK for Network direct API */
#define INITGUID
#include <guiddef.h>
#include <ndspi.h>
#include <ws2spi.h>

/* FIXME: Check if we can pack the structures in ND more tightly */
typedef struct MPID_Nem_nd_dev_hnd_{
    INDAdapter      *p_ad;
    INDListen       *p_listen;
    ND_ADAPTER_INFO ad_info;
    struct sockaddr_in s_addr_in;
    /* FIXME: We might need a list of completion queues here
     * if we exhaust the number of endpoints on a single
     * completion queue
     */
    INDCompletionQueue *p_cq;
    int npending_rds;
    volatile int zcp_pending;
} *MPID_Nem_nd_dev_hnd_t;

#define MPID_NEM_ND_DEV_HND_INVALID    NULL
#define MPID_NEM_ND_DEV_RDMA_RD_MAX 2
#define MPID_NEM_ND_DEV_IO_LIMIT(_dev_hnd) (_dev_hnd->ad_info.MaxOutboundLength - 1)

/* Checks whether dev handle is initialized */
#define MPID_NEM_ND_DEV_HND_IS_INIT(hnd)    ((hnd) != NULL)
/* Checks whether dev handle is valid */
#define MPID_NEM_ND_DEV_HND_IS_VALID(hnd)   (((hnd) != NULL) && \
    ((hnd)->p_ad != NULL) && ((hnd)->p_cq != NULL))

/* Connection queue sizes */
/* FIXME: Decide on these SENDQ/RECVQ sizes more judiciously */
/* Max number of flow controller conn bufs */
#define MPID_NEM_ND_CONN_FC_BUFS_MAX    10
#define MPID_NEM_ND_CONN_FC_RW (MPID_NEM_ND_CONN_FC_BUFS_MAX/2)
/* FIXME: Change this value */
/* Maximum number of outstanding RDMA reads - Not flow controlled */
#define MPID_NEM_ND_CONN_RDMA_RD_MAX    1
/* Maximum number of outstanding flow control msgs - Not flow controlled */
#define MPID_NEM_ND_CONN_FC_MSG_MAX    1

#define MPID_NEM_ND_CONN_SENDQ_SZ (MPID_NEM_ND_CONN_FC_BUFS_MAX+MPID_NEM_ND_CONN_RDMA_RD_MAX+MPID_NEM_ND_CONN_FC_MSG_MAX)
#define MPID_NEM_ND_CONN_RECVQ_SZ (MPID_NEM_ND_CONN_FC_BUFS_MAX+MPID_NEM_ND_CONN_RDMA_RD_MAX)
#define MPID_NEM_ND_CONN_SGE_MAX 16

/* We use bcopy for upto 1K of upper layer data - pkt + user data 
 * FIXME: Tune this value after some runtime exp
 */
#define MPID_NEM_ND_CONN_UDATA_SZ   2048
typedef struct MPID_Nem_nd_msg_mw_{
    /* The memory window descriptor of next data
     * i.e., upper layer data > MPID_NEM_ND_CONN_UDATA_SZ
     * - if any 
     */
    /* FIXME: Only use/send the valid mw_datas */
    ND_MW_DESCRIPTOR mw_data;
    /* The memory window descriptor containing 
     * memory window descriptors of subsequent user data
     * eg: Non contig sends
     * - if any
     */
    /* FIXME: Use this for multi-mws */
    ND_MW_DESCRIPTOR mw_mws;

} MPID_Nem_nd_msg_mw_t;
/* ND Message */
typedef struct MPID_Nem_nd_msg_hdr_{
    uint32_t type;
    uint32_t credits;
} MPID_Nem_nd_msg_hdr_t;

typedef struct MPID_Nem_nd_msg_{
    /* See the packet types in nd_sm.h */
    MPID_Nem_nd_msg_hdr_t hdr;
    char buf[MPID_NEM_ND_CONN_UDATA_SZ + sizeof(MPID_Nem_nd_msg_mw_t)];
    /* The memory window descriptor of next data
     * i.e., upper layer data > MPID_NEM_ND_CONN_UDATA_SZ
     * - if any 
     */
    /* ND_MW_DESCRIPTOR mw_data; - appended to the end of the buf */
    /* The memory window descriptor containing 
     * memory window descriptors of subsequent user data
     * eg: Non contig sends
     * - if any
     */
    /* ND_MW_DESCRIPTOR mw_mws; - appended to the end of the buf */
} MPID_Nem_nd_msg_t;

struct MPID_Nem_nd_conn_hnd_;
struct MPID_Nem_nd_msg_result_;
typedef int (*MPID_Nem_nd_msg_handler_t) (MPID_Nem_nd_msg_result_ *pmsg_result);
typedef struct MPID_Nem_nd_msg_result_{
    MPID_Nem_nd_msg_handler_t   succ_fn;
    MPID_Nem_nd_msg_handler_t   fail_fn;
    ND_RESULT result;
} MPID_Nem_nd_msg_result_t;

typedef struct MPID_Nem_nd_msg_ele_{
    MPID_Nem_nd_msg_t   msg;
    MPID_Nem_nd_conn_hnd_ *conn_hnd;
    MPID_Nem_nd_msg_result_t msg_result;
}MPID_Nem_nd_msg_ele_t;

/* Max num of CQ entries used by a conn */
#define MPID_NEM_ND_CONN_CQE_MAX    (MPID_NEM_ND_CONN_SENDQ_SZ+MPID_NEM_ND_CONN_RECVQ_SZ+MPID_NEM_ND_CONN_RDMA_RD_MAX)
/* FIXME: Define the number of max conn dynamically */
#define MPID_NEM_ND_CONN_NUM_MAX    10

typedef enum{
    MPID_NEM_ND_LISTEN_CONN,
    MPID_NEM_ND_ACCEPT_CONN,
    MPID_NEM_ND_CONNECT_CONN
}MPID_Nem_nd_conn_type_t;

typedef struct MPID_Nem_nd_conn_hnd_{
    INDEndpoint     *p_ep;
    INDConnector    *p_conn;
    MPIDI_VC_t      *vc;
    /* Set if this conn loses in H-H */
    int is_orphan;
    /* Used by conns to store vc till 3-way handshake - i.e., LACK/CACK */
    MPIDI_VC_t *tmp_vc;
    /* EX OV for Connect() */
    /* FIXME: Use this for Send() etc after extending Executive */
    MPIU_EXOVERLAPPED send_ov;
    /* EX OV for Accept() */
    MPIU_EXOVERLAPPED recv_ov;
    int state;
    /* Pre-registered scratch buffers for send/recv 
     * RDMA reads/writes are not controlled by flow control algo
     */
    MPID_Nem_nd_msg_ele_t   ssbuf[MPID_NEM_ND_CONN_SENDQ_SZ];
    MPID_Nem_nd_msg_ele_t   rsbuf[MPID_NEM_ND_CONN_RECVQ_SZ];
    /* MW handles for send and recv scratch buffers */
    ND_MR_HANDLE ssbuf_hmr;
    ND_MR_HANDLE rsbuf_hmr;
    struct{
        int head;
        int nbuf;
    } ssbuf_freeq;
    /* All the elements in rsbuf are pre-posted and re-posted
     * by the progress engine - we don't keep track of them
     * individually
     */
    /* ND flow control fields */
    /* Credits available to send nd msg */
    int send_credits;
    /* Credits available to recv nd msg - Sent to the remote proc
     * in a credit packet
     */
    int recv_credits;
	/* Currently tracking only pending sends...
	 * FIXME: Can we get this info from send_credits ?
	 */
	int npending_ops;

    /* FIXME : REMOVE ME ! */
    int npending_rds;
    /* Is a Flow control pkt pending ? */
    int fc_pkt_pending;

    /* RDMA Send side fields */
    int zcp_send_offset;
    int send_in_progress;
    int zcp_in_progress;
    ND_SGE  zcp_send_sge;
    /* MPID_Request *zcp_sreq; */
    /* The ND memory window */
    INDMemoryWindow *zcp_send_mw;
    /* The memory window desc sent in the ND message */
    MPID_Nem_nd_msg_mw_t zcp_msg_send_mw;
    ND_MR_HANDLE zcp_send_mr_hnd;
    MPID_Nem_nd_msg_result_t zcp_send_result;

    /* RDMA Recv side fields*/
    int cache_credits;
    int zcp_recv_sge_count;
    ND_SGE zcp_recv_sge[MPID_IOV_LIMIT];
    MPID_Request *zcp_rreqp;
    MPID_Nem_nd_msg_mw_t zcp_msg_recv_mw;
    /* int zcp_recv_mw_offset; */
    /* MPID_Nem_nd_msg_result_t zcp_recv_result; */
} *MPID_Nem_nd_conn_hnd_t;

/* FIXME: Currently one cannot block for specific ops */
typedef struct MPID_Nem_nd_block_op_hnd_{
    /* For EX blocking ops */
    MPIU_EXOVERLAPPED ex_ov;
	MPID_Nem_nd_conn_hnd_t conn_hnd;
    /* The number of blocking ops to wait before finalizing the hnd */
    int npending_ops;
} *MPID_Nem_nd_block_op_hnd_t;
#define MPID_NEM_ND_BLOCK_OP_HND_INVALID NULL
#define MPID_NEM_ND_BLOCK_OP_GET_OVERLAPPED_PTR(hnd) (MPIU_EX_GET_OVERLAPPED_PTR(&(hnd->ex_ov)))

#define MPID_NEM_ND_CONN_HND_INVALID    NULL
/* Checks whether conn handle is initialized */
#define MPID_NEM_ND_CONN_HND_IS_INIT(_hnd)    ((_hnd) != NULL)
/* Checks whether conn handle is valid */
#define MPID_NEM_ND_CONN_HND_IS_VALID(_hnd)   (((_hnd) != NULL) && \
    ((_hnd)->p_conn != NULL) && ((_hnd)->p_ep != NULL))
#define MPID_NEM_ND_CONN_STATE_SET(_hnd, _state)  do{   \
    MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "conn[%p] %d - %d", (_hnd), (_hnd)->state, _state));    \
    (_hnd->state = _state); \
}while(0);
/* Using an unused IANA protocol family */
#define MPID_NEM_ND_PROT_FAMILY 234

/* ND conn states */
enum{
    MPID_NEM_ND_CONN_QUIESCENT=0,
    /* Connecting through connect - posted a connect() */
    MPID_NEM_ND_CONN_C_CONNECTING,
    /* Waiting for ACK from listen side - LACK */
    MPID_NEM_ND_CONN_WAIT_LACK,
    /* Waiting for ACK from connect side - CACK */
    MPID_NEM_ND_CONN_WAIT_CACK,
    /* If CONN is ACTIVE the corresponding VC is CONNECTED */
    MPID_NEM_ND_CONN_ACTIVE
};

#define MPID_NEM_ND_CONN_IS_CONNECTING(_conn_hnd) (_conn_hnd && ( (_conn_hnd->state > MPID_NEM_ND_CONN_QUIESCENT) && (_conn_hnd->state < MPID_NEM_ND_CONN_ACTIVE) ))
#define MPID_NEM_ND_CONN_IS_CONNECTED(_conn_hnd) (_conn_hnd && (_conn_hnd->state == MPID_NEM_ND_CONN_ACTIVE))
/* VC states */
typedef enum{
    MPID_NEM_ND_VC_STATE_DISCONNECTED=0,
    MPID_NEM_ND_VC_STATE_CONNECTING,
    MPID_NEM_ND_VC_STATE_CONNECTED
} MPID_Nem_nd_vc_state_t;

/* The vc provides a generic buffer in which network modules can store
   private fields This removes all dependencies from the VC struct
   on the network module */
typedef struct {
    MPID_Nem_nd_conn_hnd_t conn_hnd;
    /* Used by connect() to temperorily store the conn handle */
    MPID_Nem_nd_conn_hnd_t tmp_conn_hnd;
    struct{
        struct MPID_Request *head;
        struct MPID_Request *tail;
    } posted_sendq;
    struct{
        struct MPID_Request *head;
        struct MPID_Request *tail;
    } pending_sendq;
    MPID_Nem_nd_vc_state_t state;
} MPID_Nem_nd_vc_area;

#define MPID_NEM_ND_IS_BLOCKING_REQ(_reqp) ((_reqp)->dev.OnDataAvail != NULL)
#define MPID_NEM_ND_VCCH_GET_ACTIVE_RECV_REQ(_vc) (VC_CH(((_vc)))->recv_active)
#define MPID_NEM_ND_VCCH_SET_ACTIVE_RECV_REQ(_vc, _req) (VC_CH(((_vc)))->recv_active = _req)
#define MPID_NEM_ND_VCCH_NETMOD_CONN_HND_INIT(_vc) ((((MPID_Nem_nd_vc_area *)VC_CH((_vc))->netmod_area.padding)->conn_hnd) = MPID_NEM_ND_CONN_HND_INVALID)
#define MPID_NEM_ND_VCCH_NETMOD_TMP_CONN_HND_INIT(_vc) ((((MPID_Nem_nd_vc_area *)VC_CH((_vc))->netmod_area.padding)->tmp_conn_hnd) = MPID_NEM_ND_CONN_HND_INVALID)
#define MPID_NEM_ND_VCCH_NETMOD_POSTED_SENDQ_INIT(_vc)  do{\
    (((MPID_Nem_nd_vc_area *)VC_CH((_vc))->netmod_area.padding)->posted_sendq).head = NULL;        \
    (((MPID_Nem_nd_vc_area *)VC_CH((_vc))->netmod_area.padding)->posted_sendq).tail = NULL;        \
}while(0)

#define MPID_NEM_ND_VCCH_NETMOD_POSTED_SENDQ_EMPTY(_vc) GENERIC_Q_EMPTY (((MPID_Nem_nd_vc_area *)VC_CH((_vc))->netmod_area.padding)->posted_sendq)
#define MPID_NEM_ND_VCCH_NETMOD_POSTED_SENDQ_HEAD(_vc) GENERIC_Q_HEAD (((MPID_Nem_nd_vc_area *)VC_CH((_vc))->netmod_area.padding)->posted_sendq)
#define MPID_NEM_ND_VCCH_NETMOD_POSTED_SENDQ_TAIL(_vc) ((((MPID_Nem_nd_vc_area *)VC_CH((_vc))->netmod_area.padding)->posted_sendq).tail)
#define MPID_NEM_ND_VCCH_NETMOD_POSTED_SENDQ_ENQUEUE(_vc, _reqp) GENERIC_Q_ENQUEUE (&(((MPID_Nem_nd_vc_area *)VC_CH((_vc))->netmod_area.padding)->posted_sendq), _reqp, dev.next)
#define MPID_NEM_ND_VCCH_NETMOD_POSTED_SENDQ_DEQUEUE(_vc, _reqp) GENERIC_Q_DEQUEUE (&(((MPID_Nem_nd_vc_area *)VC_CH((_vc))->netmod_area.padding)->posted_sendq), _reqp, dev.next)
#define MPID_NEM_ND_VCCH_NETMOD_POSTED_SENDQ_REM_TAIL(_vc, _reqp) GENERIC_Q_SEARCH_REMOVE (&(((MPID_Nem_nd_vc_area *)VC_CH((_vc))->netmod_area.padding)->posted_sendq), ( (_reqp) && ((*_reqp)->dev.next == NULL) ), _reqp, MPID_Request, dev.next)

#define MPID_NEM_ND_VCCH_NETMOD_PENDING_SENDQ_INIT(_vc)  do{\
    (((MPID_Nem_nd_vc_area *)VC_CH((_vc))->netmod_area.padding)->pending_sendq).head = NULL;        \
    (((MPID_Nem_nd_vc_area *)VC_CH((_vc))->netmod_area.padding)->pending_sendq).tail = NULL;        \
}while(0)

#define MPID_NEM_ND_VCCH_NETMOD_PENDING_SENDQ_EMPTY(_vc) GENERIC_Q_EMPTY (((MPID_Nem_nd_vc_area *)VC_CH((_vc))->netmod_area.padding)->pending_sendq)
#define MPID_NEM_ND_VCCH_NETMOD_PENDING_SENDQ_HEAD(_vc) GENERIC_Q_HEAD (((MPID_Nem_nd_vc_area *)VC_CH((_vc))->netmod_area.padding)->pending_sendq)
#define MPID_NEM_ND_VCCH_NETMOD_PENDING_SENDQ_ENQUEUE(_vc, _reqp) GENERIC_Q_ENQUEUE (&(((MPID_Nem_nd_vc_area *)VC_CH((_vc))->netmod_area.padding)->pending_sendq), _reqp, dev.next)
#define MPID_NEM_ND_VCCH_NETMOD_PENDING_SENDQ_DEQUEUE(_vc, _reqp) GENERIC_Q_DEQUEUE (&(((MPID_Nem_nd_vc_area *)VC_CH((_vc))->netmod_area.padding)->pending_sendq), _reqp, dev.next)

/* Accessor macro to channel private fields in VC */
/* Netmod area accessors */
#define MPID_NEM_ND_VCCH_NETMOD_FIELD_GET(_vc, _field) (((MPID_Nem_nd_vc_area *)VC_CH((_vc))->netmod_area.padding)->_field)
#define MPID_NEM_ND_VCCH_NETMOD_CONN_HND_SET(_vc, _conn_hnd) ((((MPID_Nem_nd_vc_area *)VC_CH((_vc))->netmod_area.padding)->conn_hnd) = _conn_hnd)
#define MPID_NEM_ND_VCCH_NETMOD_CONN_HND_GET(_vc) (((MPID_Nem_nd_vc_area *)VC_CH((_vc))->netmod_area.padding)->conn_hnd)
#define MPID_NEM_ND_VCCH_NETMOD_TMP_CONN_HND_SET(_vc, _conn_hnd) ((((MPID_Nem_nd_vc_area *)VC_CH((_vc))->netmod_area.padding)->tmp_conn_hnd) = _conn_hnd)
#define MPID_NEM_ND_VCCH_NETMOD_TMP_CONN_HND_GET(_vc) (((MPID_Nem_nd_vc_area *)VC_CH((_vc))->netmod_area.padding)->tmp_conn_hnd)
#define MPID_NEM_ND_VCCH_NETMOD_STATE_SET(_vc, _state) ((((MPID_Nem_nd_vc_area *)VC_CH((_vc))->netmod_area.padding)->state) = _state)
#define MPID_NEM_ND_VCCH_NETMOD_STATE_GET(_vc) (((MPID_Nem_nd_vc_area *)VC_CH((_vc))->netmod_area.padding)->state)

/* VC Netmod util funcs */
#define MPID_NEM_ND_VC_IS_CONNECTED(_vc) (  \
    (_vc) &&                                \
    (MPID_NEM_ND_VCCH_NETMOD_STATE_GET(_vc) == MPID_NEM_ND_VC_STATE_CONNECTED) &&   \
    (MPID_NEM_ND_CONN_HND_IS_VALID(MPID_NEM_ND_VCCH_NETMOD_FIELD_GET(_vc, conn_hnd))) &&     \
    (MPID_NEM_ND_VCCH_NETMOD_CONN_HND_GET(_vc)->state == MPID_NEM_ND_CONN_ACTIVE)     \
)

#define MPID_NEM_ND_VC_IS_CONNECTING(_vc) (\
    (_vc) &&    \
    (MPID_NEM_ND_VCCH_NETMOD_STATE_GET(_vc) == MPID_NEM_ND_VC_STATE_CONNECTING) \
)

/* CONN is orphan if
 * - conn is not valid
 * - conn is related to a VC that is no longer related to it (eg: lost in head to head)
 */
/*
#define MPID_NEM_ND_CONN_IS_ORPHAN(_hnd) (\
    !MPID_NEM_ND_CONN_HND_IS_VALID(_hnd) ||                  \
    ((_hnd->vc) && (MPID_NEM_ND_VCCH_NETMOD_CONN_HND_GET(_hnd->vc) != _hnd)) \
)
*/
#define MPID_NEM_ND_CONN_IS_ORPHAN(_hnd) (_hnd->is_orphan)
#define MPID_NEM_ND_CONN_HAS_SCREDITS(_hnd) (_hnd->send_credits > 0)
#define MPID_NEM_ND_CONN_DECR_SCREDITS(_hnd) (_hnd->send_credits--)
#define MPID_NEM_ND_CONN_DECR_CACHE_SCREDITS(_hnd) (_hnd->cache_credits--)
/* #define MPID_NEM_ND_CONN_INCR_SCREDITS(_hnd) (_hnd->send_credits++) */

/* #define MPID_NEM_ND_CONN_DECR_RCREDITS(_hnd) (_hnd->recv_credits--) */
#define MPID_NEM_ND_CONN_INCR_RCREDITS(_hnd) (_hnd->recv_credits++)

/* FC Recv window available/free */
#define MPID_NEM_ND_CONN_FC_RW_AVAIL(_hnd) (_hnd->recv_credits >= MPID_NEM_ND_CONN_FC_RW)

/* Business card key */
#define MPIDI_CH3I_ND_INTERFACE_KEY   "nd_interface"
#define MPIDI_CH3I_ND_PORT_KEY  "nd_port"

/* Func prototypes */
int MPID_Nem_nd_provider_hnd_init(void );
int MPID_Nem_nd_provider_hnd_finalize(void );
int MPID_Nem_nd_open_ad(MPID_Nem_nd_dev_hnd_t hnd);
int MPID_Nem_nd_decode_pg_info (char *pg_id, int pg_rank, struct MPIDI_VC **pvc, MPIDI_PG_t **ppg);
int MPID_Nem_nd_resolve_head_to_head(int remote_rank, MPIDI_PG_t *remote_pg, char *remote_pg_id, int *local_conn_won_hh);

int MPID_Nem_nd_dev_hnd_init(MPID_Nem_nd_dev_hnd_t *phnd, MPIU_ExSetHandle_t ex_hnd);
int MPID_Nem_nd_dev_hnd_finalize(MPID_Nem_nd_dev_hnd_t *phnd);

int MPID_Nem_nd_conn_hnd_init(MPID_Nem_nd_dev_hnd_t dev_hnd, MPID_Nem_nd_conn_type_t conn_type, INDConnector *p_conn, MPIDI_VC_t *vc, MPID_Nem_nd_conn_hnd_t *pconn_hnd);
int MPID_Nem_nd_conn_hnd_finalize(MPID_Nem_nd_dev_hnd_t dev_hnd, MPID_Nem_nd_conn_hnd_t *p_conn_hnd);

int MPID_Nem_nd_sm_init(void );
int MPID_Nem_nd_sm_finalize(void );
int MPID_Nem_nd_sm_poll(int in_blocking_poll);
int MPID_Nem_nd_conn_block_op_init(MPID_Nem_nd_conn_hnd_t conn_hnd);
int MPID_Nem_nd_conn_msg_bufs_init(MPID_Nem_nd_conn_hnd_t conn_hnd);
int MPID_Nem_nd_listen_for_conn(int pg_rank, char **bc_val_p, int *val_max_sz_p);
int MPID_Nem_nd_conn_disc(MPID_Nem_nd_conn_hnd_t conn_hnd);
int MPID_Nem_nd_conn_est(MPIDI_VC_t *vc);
int MPID_Nem_nd_post_sendv(MPID_Nem_nd_conn_hnd_t conn_hnd, MPID_Request *sreqp);
int MPID_Nem_nd_post_sendbv(MPID_Nem_nd_conn_hnd_t conn_hnd, MPID_Request *sreqp);


int MPID_Nem_nd_init(MPIDI_PG_t *pg_p, int pg_rank, char **bc_val_p, int *val_max_sz_p);
int MPID_Nem_nd_vc_init(MPIDI_VC_t *vc);
int MPID_Nem_nd_finalize();
int MPID_Nem_nd_ckpt_shutdown();
int MPID_Nem_nd_vc_destroy(MPIDI_VC_t *vc);
int MPID_Nem_nd_vc_terminate (MPIDI_VC_t *vc);
int MPID_Nem_nd_poll(int in_blocking_poll);
int MPID_Nem_nd_get_business_card(int my_rank, char **bc_val_p, int *val_max_sz_p);
int MPID_Nem_nd_connect_to_root(const char *business_card, MPIDI_VC_t *new_vc);

int MPID_Nem_nd_istart_contig_msg(MPIDI_VC_t *vc, void *hdr, MPIDI_msg_sz_t hdr_sz, void *data, MPIDI_msg_sz_t data_sz,
                                    MPID_Request **sreq_ptr);
int MPID_Nem_nd_send_contig(MPIDI_VC_t *vc, MPID_Request *sreq, void *hdr, MPIDI_msg_sz_t hdr_sz,
                                void *data, MPIDI_msg_sz_t data_sz);
int MPID_Nem_nd_send_noncontig(MPIDI_VC_t *vc, MPID_Request *sreq, void *header, MPIDI_msg_sz_t hdr_sz);
#endif /* ND_IMPL_H_INCLUDED */

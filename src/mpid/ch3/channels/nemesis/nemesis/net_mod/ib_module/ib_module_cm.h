/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef _IB_MODULE_CM_H
#define _IB_MODULE_CM_H

#include <infiniband/verbs.h>
#include <time.h>
#include <pthread.h>
#include <errno.h>

#include "ib_utils.h"

typedef struct {
    uint32_t                    cm_n_buf;
    uint32_t                    cm_max_send;
    uint64_t                    cm_max_spin_count;
    uint32_t                    ud_overhead;
    uint32_t                    cm_hash_size;
    uint64_t                    cm_timeout_ms;
} MPID_nem_ib_cm_param_t, *MPID_nem_ib_cm_param_ptr_t;

extern MPID_nem_ib_cm_param_t *MPID_nem_ib_cm_param_ptr;

typedef enum {
     MPID_NEM_IB_CONN_NONE = 1,
     MPID_NEM_IB_CONN_IN_PROGRESS = 2,
     MPID_NEM_IB_CONN_RC = 3,
} MPID_nem_ib_cm_conn_type_t;

typedef enum {
    MPID_NEM_IB_CM_UD_RECV_WR_ID = 91,
    MPID_NEM_IB_CM_UD_SEND_WR_ID = 101
} MPID_nem_ib_cm_wr_t;

typedef enum {
    MPID_NEM_IB_CM_CONN_STATE_C_IDLE = 127,
    MPID_NEM_IB_CM_CONN_STATE_C_REQUESTING = 255
} MPID_nem_ib_cm_client_state_t;

typedef enum {
    MPID_NEM_IB_CM_CONN_STATE_S_IDLE = 111,
    MPID_NEM_IB_CM_CONN_STATE_S_REQUESTED = 222
} MPID_nem_ib_cm_server_state_t;

typedef enum {
    MPID_NEM_IB_CM_MSG_TYPE_REQ = 77,
    MPID_NEM_IB_CM_MSG_TYPE_REP = 88
} MPID_nem_ib_cm_msg_type_t;

typedef enum {
    MPID_NEM_IB_CM_PENDING_CLIENT = 11,
    MPID_NEM_IB_CM_PENDING_SERVER = 21
} MPID_nem_ib_cm_pending_cli_or_srv_t;

typedef struct {
    uint8_t                     msg_type;
    uint32_t                    req_id;
    uint32_t                    rc_qpn;
    uint16_t                    lid;
    uint64_t                    src_guid;
    uint32_t                    src_ud_qpn;
} MPID_nem_ib_cm_msg_t, *MPID_nem_ib_cm_msg_ptr_t;

typedef struct {
    struct timeval              timestamp;
    MPID_nem_ib_cm_msg_t        payload;
} MPID_nem_ib_cm_packet_t, *MPID_nem_ib_cm_packet_ptr_t;

typedef struct MPID_nem_ib_cm_pending_t {
    uint64_t                            peer_guid;
    uint32_t                            peer_ud_qpn;
    struct ibv_ah                       *peer_ah;
    MPID_nem_ib_cm_packet_t             *packet;
    struct MPID_nem_ib_cm_pending_t     *next;
    struct MPID_nem_ib_cm_pending_t     *prev;
} MPID_nem_ib_cm_pending_t, *MPID_nem_ib_cm_pending_ptr_t;

/* This structure provides an identification
 * of any remote process */
typedef struct {
    uint64_t                    guid;
    uint32_t                    qp_num;
    void                        *vc;
} MPID_nem_ib_cm_remote_id_t, *MPID_nem_ib_cm_remote_id_ptr_t;

typedef struct {
    int                         rank;
    struct ibv_device           *nic;
    uint64_t                    guid;
    struct ibv_context          *context;
    uint8_t                     hca_port;
    struct ibv_port_attr        port_attr;
    struct ibv_pd               *prot_domain;
    struct ibv_comp_channel     *comp_channel;
    struct ibv_cq               *send_cq;
    struct ibv_cq               *recv_cq;
    struct ibv_qp               *ud_qp;

    void                        *cm_buf;
    struct ibv_mr               *cm_buf_mr;
    void                        *cm_send_buf;
    void                        *cm_recv_buf;
    uint32_t                    cm_recv_buf_index;
    MPID_nem_ib_module_hash_table_t hash_table;
    int                         cm_pending_num;
    uint32_t                    cm_req_id_global;
    pthread_mutex_t             cm_conn_state_lock;
    pthread_cond_t              cm_cond_new_pending;
    MPID_nem_ib_cm_pending_t    *cm_pending_head;

    struct ibv_qp_init_attr     rc_qp_init_attr;
    struct ibv_qp_attr          rc_qp_attr_to_init;
    enum ibv_qp_attr_mask       rc_qp_mask_to_init;
    struct ibv_qp_attr          rc_qp_attr_to_rtr;
    enum ibv_qp_attr_mask       rc_qp_mask_to_rtr;
    struct ibv_qp_attr          rc_qp_attr_to_rts;
    enum ibv_qp_attr_mask       rc_qp_mask_to_rts;

    pthread_t                   cm_comp_thread;
    pthread_t                   cm_timeout_thread;

} MPID_nem_ib_cm_ctxt_t, *MPID_nem_ib_cm_ctxt_ptr_t;

extern MPID_nem_ib_cm_ctxt_ptr_t MPID_nem_ib_cm_ctxt_ptr;
extern MPID_nem_ib_module_queue_ptr_t MPID_nem_ib_module_qp_queue;

int MPID_nem_ib_cm_init_cm();

int MPID_nem_ib_cm_start_connect(
        MPID_nem_ib_cm_remote_id_ptr_t r,
        MPIDI_VC_t *vc);

int MPID_nem_ib_cm_finalize_cm();

int MPID_nem_ib_cm_finalize_rc_qps();

#endif  /* _IB_MODULE_CM_H */

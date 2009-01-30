/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef IB_DEVICE_H
#define IB_DEVICE_H

#include <infiniband/verbs.h>

typedef enum  {
    IB_PROGRESS_POLL,
    IB_PROGRESS_BLOCK
} MPID_nem_ib_progress_type_t;

typedef struct {
    int                         path_mtu;
    unsigned long               max_rdma_size;
    uint8_t                     max_qp_ous_rd_atom;
    uint32_t                    max_hcas;
    uint32_t                    psn;
    uint16_t                    pkey_ix;
    uint32_t                    max_num_cqe;
    MPID_nem_ib_progress_type_t progress_type;
    uint32_t                    max_srq_wr;
    uint32_t                    max_send_sge;
    uint32_t                    max_srq_recv_sge;
    uint32_t                    max_send_wr;
    uint32_t                    srq_limit;
    uint32_t                    srq_n_preserve;
    uint32_t                    max_inline_size;
    uint8_t                     max_dst_rd_atomic;
    uint8_t                     min_rnr_timer;
    uint8_t                     sl;
    uint8_t                     src_path_bits;
    uint8_t                     static_rate;
    uint8_t                     timeout;
    uint8_t                     retry_cnt;
    uint8_t                     rnr_retry;
    uint8_t                     sq_psn;
    uint8_t                     max_rd_atomic;
    uint32_t                    max_cell_elem;
    uint32_t                    sec_pool_size;
    uint32_t                    max_vc_queue;
} MPID_nem_ib_dev_param_t, *MPID_nem_ib_dev_param_ptr_t;

extern MPID_nem_ib_dev_param_t *MPID_nem_ib_dev_param_ptr;


/**
 * MPID_nem_ib_port_t
 *
 * Data structures to hold all the properties of a single
 * InfiniBand HCA port. One HCA may have multiple ports
 * some of which may be active, some may not be.
 */

typedef struct {
    uint8_t                             port_num;
    struct ibv_port_attr                port_attr;
} MPID_nem_ib_port_t, *MPID_nem_ib_port_ptr_t;

/**
 * MPID_nem_ib_device_t
 *
 * Defines an InfiniBand device.
 */
typedef struct {
    struct ibv_device                   *nic;
    struct ibv_context                  *context;
    struct ibv_device_attr              dev_attr;
    uint8_t                             n_active_ports; 
    MPID_nem_ib_port_ptr_t              ib_port;
    struct ibv_comp_channel             *comp_channel;
    struct ibv_pd                       *prot_domain;
    struct ibv_cq                       *cq;
    struct ibv_srq                      *srq;
    uint32_t                            srq_n_posted;
    pthread_t                           async_thread;
    pthread_spinlock_t                  srq_post_lock;
} MPID_nem_ib_device_t, *MPID_nem_ib_device_ptr_t;

/**
 * MPID_nem_ib_ctxt_t
 *
 * Defines the InfiniBand context related datastructures for any
 * number of underlying InfiniBand devices.
 */

typedef struct {
    uint16_t                            n_ib_dev;
    MPID_nem_ib_device_ptr_t            ib_dev;
} MPID_nem_ib_ctxt_t, *MPID_nem_ib_ctxt_ptr_t;

extern MPID_nem_ib_ctxt_ptr_t  MPID_nem_ib_ctxt_ptr;


/* TODO: For multi-rail implementation,
 * we need to fill in policies in the
 * macro
 */

#define MPID_NEM_IB_CHOSEN_DEV (MPID_nem_ib_ctxt_ptr->ib_dev[0])

void async_thread(void *context);

#endif /* IB_DEVICE_H */

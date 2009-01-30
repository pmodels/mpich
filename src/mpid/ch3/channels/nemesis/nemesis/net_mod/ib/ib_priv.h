/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef IB_PRIV_H
#define IB_PRIV_H

#include <infiniband/verbs.h>


int MPID_nem_ib_open_hca(struct ibv_device *dev, 
        struct ibv_context **ctxt);

int MPID_nem_ib_alloc_pd(struct ibv_context *ctxt, 
        struct ibv_pd **pd,
        struct ibv_device *dev);

int MPID_nem_ib_create_cq(struct ibv_context *ctxt,
        struct ibv_cq **cq,
        struct ibv_comp_channel **comp_channel,
        int, int,
        struct ibv_device *dev);

int MPID_nem_ib_create_srq(struct ibv_context *ctxt,
        struct ibv_pd *pd,
        struct ibv_srq **srq,
        uint32_t, uint32_t, uint32_t,
        struct ibv_device *dev);

int MPID_nem_ib_get_dev_attr(struct ibv_context *ctxt, 
        struct ibv_device_attr *dev_attr,
        struct ibv_device *dev);

int MPID_nem_ib_is_port_active(struct ibv_context *ctxt,
        uint8_t port_num);

int MPID_nem_ib_get_port_prop(struct ibv_context *ctxt,
        uint8_t port_num, struct ibv_port_attr *attr);

int MPID_nem_ib_reg_mem(struct ibv_pd *pd,
        void *addr,
        size_t length,
        int acl,
        struct ibv_mr **mr);

int MPID_nem_ib_post_srq(struct ibv_srq *srq,
        struct ibv_recv_wr *r_wr);

int MPID_nem_ib_post_send(struct ibv_qp *qp,
        struct ibv_send_wr *s_wr);

int MPID_nem_ib_poll_cq(struct ibv_cq *cq, 
        struct ibv_wc *wc);

int MPID_nem_ib_modify_srq(
            struct ibv_srq *srq,
            uint32_t max_wr,
            uint32_t srq_limit);

#endif /* IB_PRIV_H */

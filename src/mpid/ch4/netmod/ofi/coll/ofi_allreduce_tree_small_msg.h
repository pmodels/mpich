/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  Copyright (C) by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */
#ifndef OFI_ALLREDUCE_TREE_SMALL_MSG_H_INCLUDED
#define OFI_ALLREDUCE_TREE_SMALL_MSG_H_INCLUDED

#include "ofi_triggered.h"
#include "utarray.h"
#include <math.h>
#include "ofi_impl.h"
#include "ofi_coll_util.h"

/*
 * This function performs a blocking kary tree based allreduce using rma for small messages.
 *
 * Since triggered ops require RTR, to hide the latency of RTR with small messages, this
 * function overlaps the RTR for the subsequent iteration with the execution of the current
 * iteration. To minimze the overhead of resource allocation, we allocate resources per
 * communicator during the first iteration and re-use them for subsequent iterations.
 */
static inline int MPIDI_OFI_Allreduce_intra_small_msg_triggered(const void *sendbuf, void *recvbuf,
                                                                int count, MPI_Datatype datatype,
                                                                MPI_Op op, MPIR_Comm * comm_ptr,
                                                                int K, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    size_t data_sz;
    enum fi_datatype fi_dt;
    enum fi_op fi_op;
    struct fi_cntr_attr cntr_attr = {
        .events = FI_CNTR_EVENTS_COMP,
    };
    int i = 0, ret;
    int nranks = MPIR_Comm_size(comm_ptr);
    int myrank = MPIR_Comm_rank(comm_ptr);

    int is_root = 0, j = 0, k = 0;
    int leaf = 0, first_child, parent = -1, intermediate;
    int num_children = 0;
    int threshold, context_offset, expected_recv = 0;
    struct fid_cntr *dummy_cntr;
    MPIDI_OFI_trig_allred_blocking_small_msg *blk_sml_allred;
    uint64_t requested_key;
    uint64_t s_match_bits;

    MPIDI_Datatype_check_size(datatype, 1, data_sz);

    if (nranks == 1) {
        if (sendbuf != MPI_IN_PLACE) {
            memcpy(recvbuf, (void *) sendbuf, data_sz * count);
        }
        goto fn_exit;
    }

    if (sendbuf != MPI_IN_PLACE) {
        memcpy(recvbuf, (void *) sendbuf, data_sz * count);
    }

    /* Build a kary tree */
    MPIDI_OFI_build_kary_tree(myrank, nranks, 0, K, &first_child,
                              &parent, &num_children, &is_root, &leaf, &intermediate);

    MPIDI_mpi_to_ofi(datatype, &fi_dt, op, &fi_op);

    /* only works for SUM reduce op */
    MPIR_Assert(fi_op == FI_SUM);

    blk_sml_allred = MPIDI_OFI_COMM(comm_ptr).blk_sml_allred;

    if (blk_sml_allred) {
        context_offset = (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) ?
            MPIR_CONTEXT_INTRA_COLL : MPIR_CONTEXT_INTER_COLL;
        s_match_bits =
            MPIDI_OFI_init_sendtag(comm_ptr->context_id + context_offset, blk_sml_allred->rtr_tag,
                                   0);
    } else if (blk_sml_allred == NULL) {

        /* Initialize per-communicator data structures for small message blocking collectives */
        blk_sml_allred = (MPIDI_OFI_trig_allred_blocking_small_msg *)
            MPL_malloc(sizeof(MPIDI_OFI_trig_allred_blocking_small_msg), MPL_MEM_BUFFER);
        MPIR_ERR_CHKANDJUMP(blk_sml_allred == NULL, mpi_errno, MPI_ERR_OTHER, "**nomem");
        MPIDI_OFI_COMM(comm_ptr).blk_sml_allred = blk_sml_allred;

        mpi_errno = MPIR_Sched_next_tag(comm_ptr, &blk_sml_allred->rtr_tag);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
        context_offset = (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) ?
            MPIR_CONTEXT_INTRA_COLL : MPIR_CONTEXT_INTER_COLL;
        s_match_bits =
            MPIDI_OFI_init_sendtag(comm_ptr->context_id + context_offset, blk_sml_allred->rtr_tag,
                                   0);

        MPIDI_OFI_COMM(comm_ptr).blk_sml_allred->recv_buf =
            MPL_malloc(MPIDI_OFI_TRIGGERED_WINDOW * sizeof(void *), MPL_MEM_BUFFER);
        MPIR_ERR_CHKANDJUMP1(MPIDI_OFI_COMM(comm_ptr).blk_sml_allred->recv_buf == NULL, mpi_errno,
                             MPI_ERR_OTHER, "**nomem", "**nomem %s",
                             "Triggered allred blocking small message array of recv buf alloc");

        MPIDI_OFI_COMM(comm_ptr).blk_sml_allred->rcv_mr =
            (struct fid_mr **) MPL_malloc(MPIDI_OFI_TRIGGERED_WINDOW * sizeof(struct fid_mr *),
                                          MPL_MEM_BUFFER);
        MPIR_ERR_CHKANDJUMP1(MPIDI_OFI_COMM(comm_ptr).blk_sml_allred->rcv_mr == NULL, mpi_errno,
                             MPI_ERR_OTHER, "**nomem", "**nomem %s",
                             "Triggered allred blocking small message RMA key alloc");

        for (i = 0; i < MPIDI_OFI_TRIGGERED_WINDOW; i++) {
            blk_sml_allred->recv_buf[i] = MPL_malloc(1024, MPL_MEM_BUFFER);
            MPIR_ERR_CHKANDJUMP1(MPIDI_OFI_COMM(comm_ptr).blk_sml_allred->recv_buf[i] == NULL,
                                 mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s",
                                 "Triggered allred blocking small message recv buf alloc");
            memset(blk_sml_allred->recv_buf[i], 0, 1024);

            /* requested_key is 64 bit, which is big enough to have context_id */
            requested_key = (comm_ptr->context_id + context_offset) << 4 | (i + 1);
            MPIDI_OFI_CALL_RETRY(fi_mr_reg
                                 (MPIDI_OFI_global.ctx[0].domain,
                                  blk_sml_allred->recv_buf[i], 1024,
                                  FI_REMOTE_WRITE, 0ULL, requested_key, FI_RMA_EVENT,
                                  &blk_sml_allred->rcv_mr[i], NULL), 0, mr_reg, false);
        }

        blk_sml_allred->recv_cntr =
            (struct fid_cntr **) MPL_malloc(MPIDI_OFI_TRIGGERED_WINDOW * sizeof(struct fid_cntr *),
                                            MPL_MEM_BUFFER);
        MPIR_ERR_CHKANDJUMP1(MPIDI_OFI_COMM(comm_ptr).blk_sml_allred->recv_cntr == NULL, mpi_errno,
                             MPI_ERR_OTHER, "**nomem", "**nomem %s",
                             "Triggered bcast blocking small message recv counter alloc");

        MPIDI_OFI_CALL_RETRY(fi_cntr_open(MPIDI_OFI_global.ctx[0].domain, &cntr_attr,
                                          &blk_sml_allred->send_cntr, NULL), 0, fi_cntr_open,
                             false);

        for (i = 0; i < MPIDI_OFI_TRIGGERED_WINDOW; i++) {
            MPIDI_OFI_CALL_RETRY(fi_cntr_open(MPIDI_OFI_global.ctx[0].domain, &cntr_attr,
                                              &blk_sml_allred->recv_cntr[i], NULL), 0, fi_cntr_open,
                                 false);
            /* bind rma mr to recv_cntr */
            MPIDI_OFI_CALL_RETRY(fi_mr_bind(blk_sml_allred->rcv_mr[i],
                                            &blk_sml_allred->recv_cntr[i]->fid,
                                            FI_REMOTE_WRITE), 0, mr_bind, false);
        }

        blk_sml_allred->works =
            (struct fi_deferred_work *) MPL_malloc((3 * (num_children + 1) + 1) *
                                                   sizeof(struct fi_deferred_work), MPL_MEM_BUFFER);
        MPIR_ERR_CHKANDJUMP1(blk_sml_allred->works == NULL, mpi_errno,
                             MPI_ERR_OTHER, "**nomem", "**nomem %s",
                             "Triggered allred blocking small message deferred work alloc");

        blk_sml_allred->iter = 0;
        blk_sml_allred->size = MPIDI_OFI_TRIGGERED_WINDOW;
        blk_sml_allred->num_works = 0;
        blk_sml_allred->indx = 0;
        blk_sml_allred->mult = 1;

        MPIDI_OFI_CALL_RETRY(fi_cntr_open
                             (MPIDI_OFI_global.ctx[0].domain, &cntr_attr,
                              &blk_sml_allred->atomic_cntr, NULL), 0, fi_cntr_open, false);

        if (!is_root) { /* Post recv to get RTR from parent */
            ret =
                MPIDI_OFI_prepare_tagged_control_cmd(MPIDI_OFI_global.ctx[0].domain,
                                                     MPIDI_OFI_global.ctx[0].rx,
                                                     &blk_sml_allred->works[blk_sml_allred->
                                                                            num_works++],
                                                     MPIDI_OFI_comm_to_phys(comm_ptr, parent, 0, 0,
                                                                            0),
                                                     MPIDI_OFI_TRIGGERED_TAGGED_RECV, NULL, 0,
                                                     blk_sml_allred->rtr_tag, comm_ptr, 0,
                                                     blk_sml_allred->
                                                     recv_cntr[blk_sml_allred->indx],
                                                     blk_sml_allred->
                                                     recv_cntr[blk_sml_allred->indx], false);
        }
        if (!leaf) {    /* Send RTR to children */
            for (j = 0; j < num_children; j++) {
                MPIDI_OFI_CALL_RETRY(fi_tinject(MPIDI_OFI_global.ctx[0].tx, NULL, 0,
                                                MPIDI_OFI_comm_to_phys(comm_ptr, first_child + j,
                                                                       0, 0, 0), s_match_bits),
                                     0, tinject, FALSE);
            }
        }

    }

    if (blk_sml_allred->iter >= MPIDI_OFI_TRIGGERED_WINDOW && (blk_sml_allred->iter % MPIDI_OFI_TRIGGERED_WINDOW) == 0) {       /* buffers are re-used after every N iterations */
        blk_sml_allred->mult++;
    }

    MPIDI_OFI_CALL_RETRY(fi_cntr_open(MPIDI_OFI_global.ctx[0].domain, &cntr_attr,
                                      &dummy_cntr, NULL), 0, fi_cntr_open, false);

    /* contribute to itself */
    if (!leaf) {
        ret =
            MPIDI_OFI_prepare_atomic_control_cmd(MPIDI_OFI_global.ctx[0].domain,
                                                 MPIDI_OFI_global.ctx[0].tx,
                                                 &blk_sml_allred->works[blk_sml_allred->
                                                                        num_works++],
                                                 MPIDI_OFI_comm_to_phys(comm_ptr, myrank, 0, 0, 0),
                                                 recvbuf, recvbuf,
                                                 blk_sml_allred->rcv_mr[blk_sml_allred->indx],
                                                 fi_dt, fi_op, count, 0, dummy_cntr,
                                                 blk_sml_allred->atomic_cntr, false);
    }

    if (!leaf) {
        if (is_root)
            expected_recv = num_children;
        else
            expected_recv = num_children + 1 + 1;       /* rtr + get result */
        expected_recv++;        /* contribute to your self */
    } else
        expected_recv = 2;      /* rtr from parent + result */

    /* send contribution to parent with atomic operation when all received */
    if (!is_root) {
        void *buf = leaf ? recvbuf : blk_sml_allred->recv_buf[blk_sml_allred->indx];
        threshold = num_children + 1;   /* with rtr */
        if (!leaf)
            threshold++;        /* self contribution */
        threshold += (blk_sml_allred->mult - 1) * expected_recv;

        ret =
            MPIDI_OFI_prepare_atomic_control_cmd(MPIDI_OFI_global.ctx[0].domain,
                                                 MPIDI_OFI_global.ctx[0].tx,
                                                 &blk_sml_allred->works[blk_sml_allred->
                                                                        num_works++],
                                                 MPIDI_OFI_comm_to_phys(comm_ptr, parent, 0, 0, 0),
                                                 buf, buf,
                                                 blk_sml_allred->rcv_mr[blk_sml_allred->indx],
                                                 fi_dt, fi_op, count, threshold,
                                                 blk_sml_allred->recv_cntr[blk_sml_allred->indx],
                                                 blk_sml_allred->atomic_cntr, false);
    }


    if (!leaf) {        /* send result data to all children with rma */
        threshold = expected_recv * blk_sml_allred->mult;

        for (k = 0; k < num_children; k++) {
            ret =
                MPIDI_OFI_prepare_rma_control_cmd(MPIDI_OFI_global.ctx[0].domain,
                                                  MPIDI_OFI_global.ctx[0].tx,
                                                  &blk_sml_allred->
                                                  works[blk_sml_allred->num_works++],
                                                  MPIDI_OFI_comm_to_phys(comm_ptr, first_child + k,
                                                                         0, 0, 0),
                                                  blk_sml_allred->recv_buf[blk_sml_allred->indx],
                                                  fi_mr_key(blk_sml_allred->rcv_mr
                                                            [blk_sml_allred->indx]),
                                                  count * data_sz, threshold,
                                                  blk_sml_allred->recv_cntr[blk_sml_allred->indx],
                                                  blk_sml_allred->send_cntr,
                                                  MPIDI_OFI_TRIGGERED_RMA_WRITE, false);
        }
    }

    /* blocking */
    if (!leaf) {
        if (MPIDI_OFI_global.using_cxi) {
            do {
                ret =
                    fi_cntr_wait(blk_sml_allred->send_cntr,
                                 (blk_sml_allred->iter + 1) * num_children, 1);
                if (ret == -FI_ETIMEDOUT)
                    MPIDI_OFI_PROGRESS(0);
            } while (ret == -FI_ETIMEDOUT);
        } else {
            /* FIXME: This is a workaround for the CXI provider. The performance here will be worse
             * because we're going to be kicking the progress engine much more often than
             * needed. */
            uint64_t donecount = 0;
            while (donecount < (blk_sml_allred->iter + 1) * num_children) {
                donecount = fi_cntr_read(blk_sml_allred->send_cntr);
                MPID_Progress_test(NULL);
            }
        }
    } else {    /* leaf */
        if (MPIDI_OFI_global.using_cxi) {
            do {
                /* rtr + result */
                ret =
                    fi_cntr_wait(blk_sml_allred->recv_cntr[blk_sml_allred->indx],
                                 blk_sml_allred->mult * 2, -1);
                if (ret == -FI_ETIMEDOUT)
                    MPIDI_OFI_PROGRESS(0);
            } while (ret == -FI_ETIMEDOUT);
        } else {
            /* FIXME: This is a workaround for the CXI provider. The performance here will be worse
             * because we're going to be kicking the progress engine much more often than
             * needed. */
            uint64_t donecount = 0;
            while (donecount < blk_sml_allred->mult * 2) {
                donecount = fi_cntr_read(blk_sml_allred->recv_cntr[blk_sml_allred->indx]);
                MPID_Progress_test(NULL);
            }
        }
    }

    /* copy result */
    memcpy(recvbuf, blk_sml_allred->recv_buf[blk_sml_allred->indx], data_sz * count);
    /* clear buffer */
    memset(blk_sml_allred->recv_buf[blk_sml_allred->indx], 0, 1024);

    /* clean up */
    MPIDI_OFI_free_deferred_works(blk_sml_allred->works, blk_sml_allred->num_works, false);
    blk_sml_allred->num_works = 0;

    if (!is_root) {     /* Post recv to get RTR from parent */
        int thres = fi_cntr_read(blk_sml_allred->recv_cntr[blk_sml_allred->indx]);
        ret =
            MPIDI_OFI_prepare_tagged_control_cmd(MPIDI_OFI_global.ctx[0].domain,
                                                 MPIDI_OFI_global.ctx[0].rx,
                                                 &blk_sml_allred->
                                                 works[blk_sml_allred->num_works++],
                                                 MPIDI_OFI_comm_to_phys(comm_ptr, parent, 0, 0, 0),
                                                 MPIDI_OFI_TRIGGERED_TAGGED_RECV, NULL, 0,
                                                 blk_sml_allred->rtr_tag, comm_ptr, thres,
                                                 blk_sml_allred->recv_cntr[blk_sml_allred->indx],
                                                 blk_sml_allred->recv_cntr[(blk_sml_allred->indx +
                                                                            1) %
                                                                           MPIDI_OFI_TRIGGERED_WINDOW],
                                                 false);
    }
    if (!leaf) {        /* Send RTR to children */
        for (j = 0; j < num_children; j++) {
            MPIDI_OFI_CALL_RETRY(fi_tinject(MPIDI_OFI_global.ctx[0].tx, NULL, 0,
                                            MPIDI_OFI_comm_to_phys(comm_ptr, first_child + j, 0, 0,
                                                                   0), s_match_bits), 0, tinject,
                                 FALSE);
        }
    }

    fi_close(&dummy_cntr->fid);

    blk_sml_allred->indx = (blk_sml_allred->indx + 1) % MPIDI_OFI_TRIGGERED_WINDOW;
    blk_sml_allred->iter++;

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;

  fn_fail:
    goto fn_exit;

}


#endif /* OFI_ALLREDUCE_TREE_SMALL_MSG_H_INCLUDED */

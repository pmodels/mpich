/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  Copyright (C) by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */
#ifndef OFI_BCAST_TREE_SMALL_MSG_H_INCLUDED
#define OFI_BCAST_TREE_SMALL_MSG_H_INCLUDED

#include "ofi_triggered.h"
#include "utarray.h"
#include <math.h>
#include "ofi_impl.h"
#include "ofi_coll_util.h"

/*
 * This function performs a blocking kary tree based bcast using rma for small messages.
 *
 * Since triggered ops require RTR, to hide the latency of RTR with small messages, this
 * function overlaps the RTR for the subsequent iteration with the execution of the current
 * iteration. To minimze the overhead of resource allocation, we allocate resources per
 * communicator during the first iteration and re-use them for subsequent iterations.
 */
MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_Bcast_intra_triggered_small_msg(void *buffer, int count,
                                                                       MPI_Datatype datatype,
                                                                       int root,
                                                                       MPIR_Comm * comm_ptr,
                                                                       int branching_factor)
{
    int mpi_errno = MPI_SUCCESS;
    size_t data_sz;
    struct fi_cntr_attr cntr_attr = {
        .events = FI_CNTR_EVENTS_COMP,
    };
    int is_root = 0, ret, nranks, is_contig;
    int i = 0, j = 0, k = 0;
    int intermediate = 0, leaf = 0, first_child, parent = -1;
    int num_children = 0, myrank = -1;
    int threshold;
    MPI_Aint true_lb, true_extent, actual_packed_unpacked_bytes;
    uint64_t requested_key;
    uint64_t s_match_bits;
    MPIDI_OFI_trig_bcast_blocking_small_msg *blk_sml_bcast;
    void *sendbuf = NULL;
    int context_offset = (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) ?
        MPIR_CONTEXT_INTRA_COLL : MPIR_CONTEXT_INTER_COLL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_BCAST_INTRA_SMALL_MSG_TRIGGERED);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_BCAST_INTRA_SMALL_MSG_TRIGGERED);

    nranks = MPIR_Comm_size(comm_ptr);
    myrank = MPIR_Comm_rank(comm_ptr);

    if (nranks == 1) {
        goto fn_exit;
    }

    /* Build a kary tree */
    MPIDI_OFI_build_kary_tree(myrank, nranks, root, branching_factor, &first_child,
                              &parent, &num_children, &is_root, &leaf, &intermediate);

    MPIR_Datatype_is_contig(datatype, &is_contig);
    MPIDI_Datatype_check_size(datatype, 1, data_sz);

    MPIR_Assert(data_sz * count <= 1024);

    if (is_contig) {
        MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);
        sendbuf = (char *) buffer + true_lb;
    } else {
        sendbuf = MPL_malloc(data_sz * count, MPL_MEM_COLL);
        if (myrank == root) {
            mpi_errno = MPIR_Typerep_pack(buffer, count, datatype, 0, sendbuf, data_sz * count,
                                          &actual_packed_unpacked_bytes);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
        }
    }

    blk_sml_bcast = MPIDI_OFI_COMM(comm_ptr).blk_sml_bcast;

    if (blk_sml_bcast) {
        s_match_bits =
            MPIDI_OFI_init_sendtag(comm_ptr->context_id + context_offset, blk_sml_bcast->rtr_tag,
                                   0);
    } else {
        /* Initialize per-communicator resources for small message blocking collectives */
        blk_sml_bcast = (MPIDI_OFI_trig_bcast_blocking_small_msg *)
            MPL_malloc(sizeof(MPIDI_OFI_trig_bcast_blocking_small_msg), MPL_MEM_BUFFER);
        MPIR_ERR_CHKANDJUMP(blk_sml_bcast == NULL, mpi_errno, MPI_ERR_OTHER, "**nomem");
        MPIDI_OFI_COMM(comm_ptr).blk_sml_bcast = blk_sml_bcast;

        mpi_errno = MPIR_Sched_next_tag(comm_ptr, &blk_sml_bcast->rtr_tag);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
        s_match_bits =
            MPIDI_OFI_init_sendtag(comm_ptr->context_id + context_offset, blk_sml_bcast->rtr_tag,
                                   0);

        blk_sml_bcast->recv_buf =
            MPL_malloc(MPIDI_OFI_TRIGGERED_WINDOW * sizeof(void *), MPL_MEM_BUFFER);
        MPIR_ERR_CHKANDJUMP1(blk_sml_bcast->recv_buf == NULL, mpi_errno,
                             MPI_ERR_OTHER, "**nomem",
                             "**nomem %s",
                             "Triggered bcast blocking small message array of recv buf alloc");

        blk_sml_bcast->rcv_mr =
            (struct fid_mr **) MPL_malloc(MPIDI_OFI_TRIGGERED_WINDOW * sizeof(struct fid_mr *),
                                          MPL_MEM_BUFFER);
        MPIR_ERR_CHKANDJUMP1(blk_sml_bcast->rcv_mr == NULL, mpi_errno,
                             MPI_ERR_OTHER, "**nomem", "**nomem %s",
                             "Triggered bcast blocking small message RMA key alloc");

        for (i = 0; i < MPIDI_OFI_TRIGGERED_WINDOW; i++) {
            blk_sml_bcast->recv_buf[i] = MPL_malloc(1024, MPL_MEM_BUFFER);
            MPIR_ERR_CHKANDJUMP1(blk_sml_bcast->recv_buf[i] == NULL,
                                 mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s",
                                 "Triggered bcast blocking small message recv buf alloc");

            memset(blk_sml_bcast->recv_buf[i], 0, 1024);

            /* requested_key is 64 bit, which is big enough to have context_id */
            requested_key = comm_ptr->context_id << 16 | (i + 1);
            MPIDI_OFI_CALL(fi_mr_reg
                           (MPIDI_OFI_global.ctx[0].domain,
                            blk_sml_bcast->recv_buf[i], 1024,
                            FI_REMOTE_WRITE, 0ULL, requested_key, FI_RMA_EVENT,
                            &blk_sml_bcast->rcv_mr[i], NULL), mr_reg);
        }

        blk_sml_bcast->recv_cntr =
            (struct fid_cntr **) MPL_malloc(MPIDI_OFI_TRIGGERED_WINDOW * sizeof(struct fid_cntr *),
                                            MPL_MEM_BUFFER);
        MPIR_ERR_CHKANDJUMP1(blk_sml_bcast->recv_cntr == NULL, mpi_errno,
                             MPI_ERR_OTHER, "**nomem", "**nomem %s",
                             "Triggered bcast blocking small message recv counter alloc");

        MPIDI_OFI_CALL_RETURN(fi_cntr_open
                              (MPIDI_OFI_global.ctx[0].domain, &cntr_attr,
                               &blk_sml_bcast->send_cntr, NULL), ret);
        if (ret < 0) {
            MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                            (MPL_DBG_FDEST, "fi_cntr_open (%s)\n", fi_strerror(ret)));
            mpi_errno = MPIDI_OFI_ENAVAIL;
            goto fn_fail;
        }

        for (i = 0; i < MPIDI_OFI_TRIGGERED_WINDOW; i++) {
            MPIDI_OFI_CALL_RETURN(fi_cntr_open(MPIDI_OFI_global.ctx[0].domain, &cntr_attr,
                                               &blk_sml_bcast->recv_cntr[i], NULL), ret);
            if (ret < 0) {
                MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                                (MPL_DBG_FDEST, "fi_cntr_open (%s)\n", fi_strerror(ret)));
                mpi_errno = MPIDI_OFI_ENAVAIL;
                goto fn_fail;
            }
            MPIDI_OFI_CALL(fi_mr_bind(blk_sml_bcast->rcv_mr[i],
                                      &blk_sml_bcast->recv_cntr[i]->fid, FI_REMOTE_WRITE), mr_bind);
        }

        blk_sml_bcast->works = (struct fi_deferred_work *) MPL_malloc((3 * num_children + 2) *
                                                                      sizeof(struct
                                                                             fi_deferred_work),
                                                                      MPL_MEM_BUFFER);
        MPIR_ERR_CHKANDJUMP1(blk_sml_bcast->works == NULL, mpi_errno, MPI_ERR_OTHER, "**nomem",
                             "**nomem %s",
                             "Triggered bcast blocking small message deferred work alloc");

        blk_sml_bcast->iter = 0;
        blk_sml_bcast->size = MPIDI_OFI_TRIGGERED_WINDOW;
        blk_sml_bcast->num_works = 0;
        blk_sml_bcast->indx = 0;
        blk_sml_bcast->mult = 1;
        blk_sml_bcast->root = root;

        if (!leaf) {    /* Post recv for RTR from the children */
            for (j = 0; j < num_children; j++) {
                ret =
                    MPIDI_OFI_prepare_tagged_control_cmd(MPIDI_OFI_global.ctx[0].domain,
                                                         MPIDI_OFI_global.ctx[0].rx,
                                                         &blk_sml_bcast->works[blk_sml_bcast->
                                                                               num_works++],
                                                         MPIDI_OFI_comm_to_phys(comm_ptr,
                                                                                first_child + j, 0,
                                                                                0, 0),
                                                         MPIDI_OFI_TRIGGERED_TAGGED_RECV, NULL, 0,
                                                         blk_sml_bcast->rtr_tag, comm_ptr, 0,
                                                         blk_sml_bcast->recv_cntr[0],
                                                         blk_sml_bcast->recv_cntr[0], false);
            }
        }

        if (!is_root) { /* Send RTR for the first iteration */
            MPIDI_OFI_CALL_RETRY(fi_tinject(MPIDI_OFI_global.ctx[0].tx, NULL, 0,
                                            MPIDI_OFI_comm_to_phys(comm_ptr, parent, 0, 0, 0),
                                            s_match_bits), 0, tinject, FALSE);
        }
    }

    if (blk_sml_bcast->iter >= MPIDI_OFI_TRIGGERED_WINDOW && (blk_sml_bcast->iter % MPIDI_OFI_TRIGGERED_WINDOW) == 0) { /* buffers are re-used after every MPIDI_OFI_TRIGGERED_WINDOW iterations */
        blk_sml_bcast->mult++;
    }

    if (is_root) {
        memcpy(blk_sml_bcast->recv_buf[blk_sml_bcast->indx], sendbuf, data_sz * count);
    }

    if (is_root) {
        threshold = blk_sml_bcast->mult * num_children;

    } else if (!leaf) {
        threshold = blk_sml_bcast->mult * (1 + num_children);
    }

    if (!leaf) {        /* Send data to children */
        for (k = 0; k < num_children; k++) {
            ret =
                MPIDI_OFI_prepare_rma_control_cmd(MPIDI_OFI_global.ctx[0].domain,
                                                  MPIDI_OFI_global.ctx[0].tx,
                                                  &blk_sml_bcast->works[MPIDI_OFI_COMM
                                                                        (comm_ptr).blk_sml_bcast->
                                                                        num_works++],
                                                  MPIDI_OFI_comm_to_phys(comm_ptr, first_child + k,
                                                                         0, 0, 0),
                                                  blk_sml_bcast->recv_buf[blk_sml_bcast->indx],
                                                  fi_mr_key(blk_sml_bcast->rcv_mr
                                                            [blk_sml_bcast->indx]), count * data_sz,
                                                  threshold,
                                                  blk_sml_bcast->recv_cntr[blk_sml_bcast->indx],
                                                  blk_sml_bcast->send_cntr,
                                                  MPIDI_OFI_TRIGGERED_RMA_WRITE, false);
        }
    }

    /* wait for the counters to reach desired values */
    /* still call progress engine periodically */
    if (leaf) {
        if (0 != strcmp(MPIDI_OFI_global.prov_use[0]->fabric_attr->prov_name, "cxi")) {
            do {
                ret =
                    fi_cntr_wait(blk_sml_bcast->recv_cntr[blk_sml_bcast->indx],
                                 blk_sml_bcast->mult * 1, 1);
                if (ret == -FI_ETIMEDOUT)
                    MPID_Progress_test(NULL);
            } while (ret == -FI_ETIMEDOUT);
        } else {
            /* FIXEM: This is a workaround for the CXI provider. The performance here will be worse
             * because we're going to be kicking the progress engine much more often than
             * needed. */
            uint64_t donecount = 0;
            while (donecount < blk_sml_bcast->mult * 1) {
                donecount = fi_cntr_read(blk_sml_bcast->recv_cntr[blk_sml_bcast->indx]);
                MPID_Progress_test(NULL);
            }
        }
        MPIDI_OFI_ERR(ret < 0 &&
                      ret != -FI_ETIMEDOUT, mpi_errno, MPI_ERR_RMA_RANGE, "**ofid_cntr_wait",
                      "**ofid_cntr_wait %s %d %s %s", __SHORT_FILE__, __LINE__, __func__,
                      fi_strerror(-ret));
        memcpy(sendbuf, blk_sml_bcast->recv_buf[blk_sml_bcast->indx], data_sz * count);

    } else {
        if (0 != strcmp(MPIDI_OFI_global.prov_use[0]->fabric_attr->prov_name, "cxi")) {
            do {
                ret =
                    fi_cntr_wait(blk_sml_bcast->send_cntr, (blk_sml_bcast->iter + 1) * num_children,
                                 1);
                if (ret == -FI_ETIMEDOUT)
                    MPID_Progress_test(NULL);
            } while (ret == -FI_ETIMEDOUT);
        } else {
            /* FIXME: This is a workaround for the CXI provider. The performance here will be worse
             * because we're going to be kicking the progress engine much more often than
             * needed. */
            uint64_t donecount = 0;
            while (donecount < (blk_sml_bcast->iter + 1) * num_children) {
                donecount = fi_cntr_read(blk_sml_bcast->send_cntr);
                MPID_Progress_test(NULL);
            }
        }
        MPIDI_OFI_ERR(ret < 0 &&
                      ret != -FI_ETIMEDOUT, mpi_errno, MPI_ERR_RMA_RANGE, "**ofid_cntr_wait",
                      "**ofid_cntr_wait %s %d %s %s", __SHORT_FILE__, __LINE__, __func__,
                      fi_strerror(-ret));

        if (!is_root)
            memcpy(sendbuf, blk_sml_bcast->recv_buf[blk_sml_bcast->indx], data_sz * count);

    }

    if (!is_contig) {
        if (myrank != root) {
            mpi_errno = MPIR_Typerep_unpack(sendbuf, data_sz * count, buffer, count, datatype, 0,
                                            &actual_packed_unpacked_bytes);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);

        }
        MPL_free(sendbuf);
    }

    /* clean up */
    MPIDI_OFI_free_deferred_works(blk_sml_bcast->works, blk_sml_bcast->num_works, false);
    blk_sml_bcast->num_works = 0;

    if (!leaf) {        /* Post recv for RTR for the next iteration */
        threshold = fi_cntr_read(blk_sml_bcast->recv_cntr[blk_sml_bcast->indx]);
        for (j = 0; j < num_children; j++) {
            ret =
                MPIDI_OFI_prepare_tagged_control_cmd(MPIDI_OFI_global.ctx[0].domain,
                                                     MPIDI_OFI_global.ctx[0].rx,
                                                     &blk_sml_bcast->
                                                     works[blk_sml_bcast->num_works++],
                                                     MPIDI_OFI_comm_to_phys(comm_ptr,
                                                                            first_child + j, 0, 0,
                                                                            0),
                                                     MPIDI_OFI_TRIGGERED_TAGGED_RECV, NULL, 0,
                                                     blk_sml_bcast->rtr_tag, comm_ptr, threshold,
                                                     blk_sml_bcast->recv_cntr[blk_sml_bcast->indx],
                                                     blk_sml_bcast->recv_cntr[(blk_sml_bcast->indx +
                                                                               1) %
                                                                              MPIDI_OFI_TRIGGERED_WINDOW],
                                                     false);
        }
    }

    if (!is_root) {     /* Send RTR */
        MPIDI_OFI_CALL_RETRY(fi_tinject
                             (MPIDI_OFI_global.ctx[0].tx, NULL, 0,
                              MPIDI_OFI_comm_to_phys(comm_ptr, parent, 0, 0, 0),
                              s_match_bits), 0, tinject, FALSE);
    }

    blk_sml_bcast->indx = (blk_sml_bcast->indx + 1) % MPIDI_OFI_TRIGGERED_WINDOW;
    blk_sml_bcast->iter++;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_BCAST_INTRA_SMALL_MSG_TRIGGERED);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* OFI_BCAST_TREE_SMALL_MSG_H_INCLUDED */

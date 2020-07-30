/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "ofi_impl.h"
#include "ofi_am_impl.h"
#include "ofi_noinline.h"


void MPIDI_OFI_deferred_am_isend_dequeue(MPIDI_OFI_deferred_am_isend_req_t * dreq)
{
    MPIDI_OFI_deferred_am_isend_req_t *curr_req = dreq;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_DEFERRED_AM_ISEND_DEQUEUE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_DEFERRED_AM_ISEND_DEQUEUE);

    DL_DELETE(MPIDI_OFI_global.deferred_am_isend_q, curr_req);
    MPL_free(dreq);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_DEFERRED_AM_ISEND_DEQUEUE);
}

int MPIDI_OFI_deferred_am_isend_issue(MPIDI_OFI_deferred_am_isend_req_t * dreq)
{
    int mpi_errno = MPI_SUCCESS;
    int dt_contig = 0;
    MPI_Aint data_sz;
    MPI_Aint dt_true_lb, last;
    MPIR_Datatype *dt_ptr;
    char *send_buf;
    bool need_packing = false;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_DEFERRED_AM_ISEND_ISSUE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_DEFERRED_AM_ISEND_ISSUE);

    MPIDI_Datatype_get_info(dreq->count, dreq->datatype, dt_contig, data_sz, dt_ptr, dt_true_lb);
    need_packing = dt_contig ? false : true;
    send_buf = (char *) dreq->buf + dt_true_lb;

    MPL_pointer_attr_t attr;
    MPIR_GPU_query_pointer_attr(dreq->buf, &attr);
    if (attr.type == MPL_GPU_POINTER_DEV && !MPIDI_OFI_ENABLE_HMEM) {
        /* Force packing of GPU buffer in host memory */
        need_packing = true;
    }

    if (need_packing) {
        /* FIXME: currently we always do packing, also for high density types. However,
         * we should not do packing unless needed. Also, for large low-density types
         * we should not allocate the entire buffer and do the packing at once. */
        /* TODO: (1) Skip packing for high-density datatypes;
         *       (2) Pipeline allocation for low-density datatypes; */
        if (dreq->am_hdr_sz + dreq->data_sz + sizeof(MPIDI_OFI_am_header_t) <=
            MPIDI_OFI_DEFAULT_SHORT_SEND_SIZE) {
            mpi_errno = MPIDU_genq_private_pool_alloc_cell(MPIDI_OFI_global.pack_buf_pool,
                                                           (void **) &send_buf);
            if (send_buf == NULL) {
                /* no buffer available, suppress error and exit */
                mpi_errno = MPI_SUCCESS;
                goto fn_exit;
            }
            MPIR_ERR_CHECK(mpi_errno);
        } else {
            /* only for large message RDMA read */
            MPL_gpu_malloc_host((void **) &send_buf, data_sz);
        }
        mpi_errno = MPIR_Typerep_pack(dreq->buf, dreq->count, dreq->datatype, 0, send_buf,
                                      dreq->data_sz, &last);
        MPIR_ERR_CHECK(mpi_errno);

        MPIDI_OFI_AMREQUEST_HDR(dreq->sreq, pack_buffer) = send_buf;
    } else {
        MPIDI_OFI_AMREQUEST_HDR(dreq->sreq, pack_buffer) = NULL;
    }

    if (dreq->am_hdr_sz + dreq->data_sz + sizeof(MPIDI_OFI_am_header_t) <=
        MPIDI_OFI_DEFAULT_SHORT_SEND_SIZE) {
        mpi_errno =
            MPIDI_OFI_am_isend_short(dreq->rank, dreq->comm, dreq->handler_id,
                                     MPIDI_OFI_AMREQUEST_HDR(dreq->sreq, am_hdr), dreq->am_hdr_sz,
                                     send_buf, dreq->data_sz, dreq->sreq);
    } else {
        mpi_errno =
            MPIDI_OFI_am_isend_long(dreq->rank, dreq->comm, dreq->handler_id,
                                    MPIDI_OFI_AMREQUEST_HDR(dreq->sreq, am_hdr), dreq->am_hdr_sz,
                                    send_buf, dreq->data_sz, dreq->sreq);
    }
    MPIR_ERR_CHECK(mpi_errno);

    MPIDI_OFI_deferred_am_isend_dequeue(dreq);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_DEFERRED_AM_ISEND_ISSUE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

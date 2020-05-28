/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "ofi_impl.h"
#include "ofi_noinline.h"

int MPIDI_OFI_nopack_putget(const void *origin_addr, int origin_count,
                            MPI_Datatype origin_datatype, int target_rank,
                            MPI_Aint target_disp, int target_count,
                            MPI_Datatype target_datatype, MPIR_Win * win,
                            MPIDI_av_entry_t * addr, int rma_type, MPIR_Request ** sigreq)
{
    int mpi_errno = MPI_SUCCESS;
    uint64_t flags;
    struct fi_msg_rma msg;
    struct fi_rma_iov riov;
    struct iovec iov;
    size_t target_bytes, origin_bytes;

    MPIR_Datatype_get_size_macro(origin_datatype, origin_bytes);
    origin_bytes *= origin_count;
    MPIR_Datatype_get_size_macro(target_datatype, target_bytes);
    target_bytes *= target_count;

    /* allocate request */
    MPIDI_OFI_win_request_t *req = MPIDI_OFI_win_request_create();
    MPIR_ERR_CHKANDSTMT((req) == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
    req->event_id = MPIDI_OFI_EVENT_ABORT;
    req->next = MPIDI_OFI_WIN(win).syncQ;
    MPIDI_OFI_WIN(win).syncQ = req;
    req->sigreq = sigreq;

    /* allocate target iovecs */
    struct iovec *target_iov;
    void *target_base;
    MPI_Aint total_target_iov_len;
    MPI_Aint target_len;
    MPI_Aint target_iov_offset = 0;
    MPIR_Typerep_iov_len(target_count, target_datatype, target_bytes, &total_target_iov_len);
    target_len = MPL_MIN(total_target_iov_len, MPIR_CVAR_CH4_OFI_RMA_IOVEC_MAX);
    target_iov = MPL_malloc(sizeof(struct iovec) * target_len, MPL_MEM_RMA);
    target_base =
        (void *) (MPIDI_OFI_winfo_base(win, target_rank) +
                  (target_disp * MPIDI_OFI_winfo_disp_unit(win, target_rank)));

    /* allocate origin iovecs */
    struct iovec *origin_iov;
    MPI_Aint total_origin_iov_len;
    MPI_Aint origin_len;
    MPI_Aint origin_iov_offset = 0;
    MPIR_Typerep_iov_len(origin_count, origin_datatype, origin_bytes, &total_origin_iov_len);
    origin_len = MPL_MIN(total_origin_iov_len, MPIR_CVAR_CH4_OFI_RMA_IOVEC_MAX);
    origin_iov = MPL_malloc(sizeof(struct iovec) * origin_len, MPL_MEM_RMA);

    MPIDI_OFI_INIT_SIGNAL_REQUEST(win, sigreq, &flags);
    int i = 0, j = 0;
    size_t msg_len;
    while (i < total_origin_iov_len && j < total_target_iov_len) {
        MPI_Aint origin_cur = i % origin_len;
        MPI_Aint target_cur = j % target_len;
        if (i == origin_iov_offset)
            MPIDI_OFI_load_iov(origin_addr, origin_count, origin_datatype, origin_len,
                               &origin_iov_offset, origin_iov);
        if (j == target_iov_offset)
            MPIDI_OFI_load_iov(target_base, target_count, target_datatype, target_len,
                               &target_iov_offset, target_iov);

        msg_len = MPL_MIN(origin_iov[origin_cur].iov_len, target_iov[target_cur].iov_len);

        msg.desc = NULL;
        msg.addr = MPIDI_OFI_av_to_phys(addr);
        msg.context = NULL;
        msg.data = 0;
        msg.msg_iov = &iov;
        msg.iov_count = 1;
        msg.rma_iov = &riov;
        msg.rma_iov_count = 1;
        iov.iov_base = origin_iov[origin_cur].iov_base;
        iov.iov_len = msg_len;
        riov.addr = (uint64_t) target_iov[target_cur].iov_base;
        riov.len = msg_len;
        riov.key = MPIDI_OFI_winfo_mr_key(win, target_rank);
        MPIDI_OFI_INIT_CHUNK_CONTEXT(win, sigreq);
        if (rma_type == MPIDI_OFI_PUT) {
            MPIDI_OFI_CALL_RETRY(fi_writemsg(MPIDI_OFI_WIN(win).ep, &msg, flags), rdma_write,
                                 FALSE);
            req->rma_type = MPIDI_OFI_PUT;
        } else {        /* MPIDI_OFI_GET */
            MPIDI_OFI_CALL_RETRY(fi_readmsg(MPIDI_OFI_WIN(win).ep, &msg, flags), rdma_write, FALSE);
            req->rma_type = MPIDI_OFI_GET;
        }

        if (msg_len < origin_iov[origin_cur].iov_len) {
            origin_iov[origin_cur].iov_base = (char *) origin_iov[origin_cur].iov_base + msg_len;
            origin_iov[origin_cur].iov_len -= msg_len;
        } else {
            i++;
        }

        if (msg_len < target_iov[target_cur].iov_len) {
            target_iov[target_cur].iov_base = (char *) target_iov[target_cur].iov_base + msg_len;
            target_iov[target_cur].iov_len -= msg_len;
        } else {
            j++;
        }
    }
    MPIR_Assert(i == total_origin_iov_len);
    MPIR_Assert(j == total_target_iov_len);
    MPL_free(origin_iov);
    MPL_free(target_iov);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_OFI_issue_deferred_rma(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(0);

    return mpi_errno;
}

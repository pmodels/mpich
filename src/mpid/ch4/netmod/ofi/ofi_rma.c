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

    if (sigreq) {
#ifdef MPIDI_CH4_USE_WORK_QUEUES
        if (*sigreq) {
            MPIR_Request_add_ref(*sigreq);
        } else
#endif
        {
            MPIDI_OFI_REQUEST_CREATE(*sigreq, MPIR_REQUEST_KIND__RMA, 0);
        }
        flags = FI_COMPLETION | FI_DELIVERY_COMPLETE;
    } else {
        flags = FI_DELIVERY_COMPLETE;
    }

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
        msg.addr = MPIDI_OFI_av_to_phys(addr, 0, 0);
        msg.context = NULL;
        msg.data = 0;
        msg.msg_iov = &iov;
        msg.iov_count = 1;
        msg.rma_iov = &riov;
        msg.rma_iov_count = 1;
        iov.iov_base = origin_iov[origin_cur].iov_base;
        iov.iov_len = msg_len;
        riov.addr = (uintptr_t) target_iov[target_cur].iov_base;
        riov.len = msg_len;
        riov.key = MPIDI_OFI_winfo_mr_key(win, target_rank);
        MPIDI_OFI_INIT_CHUNK_CONTEXT(win, sigreq);
        if (rma_type == MPIDI_OFI_PUT) {
            MPIDI_OFI_CALL_RETRY(fi_writemsg(MPIDI_OFI_WIN(win).ep, &msg, flags), 0, rdma_write,
                                 FALSE);
            req->rma_type = MPIDI_OFI_PUT;
            req->noncontig.put.origin.pack_buffer = NULL;
        } else {        /* MPIDI_OFI_GET */
            MPIDI_OFI_CALL_RETRY(fi_readmsg(MPIDI_OFI_WIN(win).ep, &msg, flags), 0, rdma_write,
                                 FALSE);
            req->rma_type = MPIDI_OFI_GET;
            req->noncontig.get.origin.pack_buffer = NULL;
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

static int issue_packed_put(MPIR_Win * win, MPIDI_OFI_win_request_t * req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request **sigreq = req->sigreq;
    MPI_Aint actual_pack_bytes;
    struct fi_msg_rma msg;
    struct iovec iov;
    struct fi_rma_iov riov;
    uint64_t flags;

    /* load the pack buffer */
    MPIR_Typerep_pack(req->noncontig.put.origin.addr, req->noncontig.put.origin.count,
                      req->noncontig.put.origin.datatype,
                      req->noncontig.put.origin.pack_offset, req->noncontig.put.origin.pack_buffer,
                      req->noncontig.put.origin.pack_size, &actual_pack_bytes);

    if (sigreq)
        flags = FI_COMPLETION | FI_DELIVERY_COMPLETE;
    else
        flags = FI_DELIVERY_COMPLETE;

    int i = 0, j = req->noncontig.put.target.iov_cur;
    size_t msg_len;
    while (i < actual_pack_bytes) {
        MPI_Aint target_cur = j % req->noncontig.put.target.iov_len;
        if (j == req->noncontig.put.target.iov_offset)
            MPIDI_OFI_load_iov(req->noncontig.put.target.base,
                               req->noncontig.put.target.count,
                               req->noncontig.put.target.datatype,
                               req->noncontig.put.target.iov_len,
                               &req->noncontig.put.target.iov_offset,
                               req->noncontig.put.target.iov);

        msg_len = MPL_MIN(actual_pack_bytes - i, req->noncontig.put.target.iov[target_cur].iov_len);

        msg.desc = NULL;
        msg.addr = MPIDI_OFI_av_to_phys(req->noncontig.put.target.addr, 0, 0);
        msg.context = NULL;
        msg.data = 0;
        msg.msg_iov = &iov;
        msg.iov_count = 1;
        msg.rma_iov = &riov;
        msg.rma_iov_count = 1;
        iov.iov_base = (char *) req->noncontig.put.origin.pack_buffer + i;
        iov.iov_len = msg_len;
        riov.addr = (uintptr_t) req->noncontig.put.target.iov[target_cur].iov_base;
        riov.len = msg_len;
        riov.key = req->noncontig.put.target.key;
        MPIDI_OFI_INIT_CHUNK_CONTEXT(win, sigreq);
        MPIDI_OFI_CALL_RETRY(fi_writemsg(MPIDI_OFI_WIN(win).ep, &msg, flags), 0, rdma_write, FALSE);
        i += msg_len;

        if (msg_len < req->noncontig.put.target.iov[target_cur].iov_len) {
            req->noncontig.put.target.iov[target_cur].iov_base =
                (char *) req->noncontig.put.target.iov[target_cur].iov_base + msg_len;
            req->noncontig.put.target.iov[target_cur].iov_len -= msg_len;
        } else {
            j++;
        }
    }
    MPIR_Assert(i == actual_pack_bytes);
    req->noncontig.put.origin.pack_offset += actual_pack_bytes;

    /* not finished. update our place in the target iov array for later. */
    if (req->noncontig.put.origin.pack_offset < req->noncontig.put.origin.total_bytes) {
        req->noncontig.put.target.iov_cur = j;
    } else {
        /* finished issuing. move from deferredQ to syncQ. */
        DL_DELETE(MPIDI_OFI_WIN(win).deferredQ, req);
        req->next = MPIDI_OFI_WIN(win).syncQ;
        MPIDI_OFI_WIN(win).syncQ = req;
        MPL_free(req->noncontig.put.target.iov);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int issue_packed_get(MPIR_Win * win, MPIDI_OFI_win_request_t * req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request **sigreq = req->sigreq;
    MPI_Aint actual_unpack_bytes;
    struct fi_msg_rma msg;
    struct iovec iov;
    struct fi_rma_iov riov;
    uint64_t flags;

    /* unpack from pack buffer */
    if (req->noncontig.put.origin.pack_offset > 0) {
        MPIR_Typerep_unpack(req->noncontig.get.origin.pack_buffer,
                            req->noncontig.get.origin.pack_size, req->noncontig.get.origin.addr,
                            req->noncontig.get.origin.count, req->noncontig.get.origin.datatype,
                            req->noncontig.get.origin.pack_offset -
                            req->noncontig.get.origin.pack_size, &actual_unpack_bytes);
        MPIR_Assert(req->noncontig.get.origin.pack_size == actual_unpack_bytes);
    }

    if (sigreq)
        flags = FI_COMPLETION | FI_DELIVERY_COMPLETE;
    else
        flags = FI_DELIVERY_COMPLETE;

    int i = 0, j = req->noncontig.get.target.iov_cur;
    MPI_Aint get_bytes = MPL_MIN(req->noncontig.get.origin.pack_size,
                                 req->noncontig.get.origin.total_bytes -
                                 req->noncontig.get.origin.pack_offset);
    size_t msg_len;
    while (i < get_bytes) {
        MPI_Aint target_cur = j % req->noncontig.get.target.iov_len;
        if (j == req->noncontig.get.target.iov_offset)
            MPIDI_OFI_load_iov(req->noncontig.get.target.base,
                               req->noncontig.get.target.count,
                               req->noncontig.get.target.datatype,
                               req->noncontig.get.target.iov_len,
                               &req->noncontig.get.target.iov_offset,
                               req->noncontig.get.target.iov);

        msg_len = MPL_MIN(get_bytes - i, req->noncontig.get.target.iov[target_cur].iov_len);

        msg.desc = NULL;
        msg.addr = MPIDI_OFI_av_to_phys(req->noncontig.get.target.addr, 0, 0);
        msg.context = NULL;
        msg.data = 0;
        msg.msg_iov = &iov;
        msg.iov_count = 1;
        msg.rma_iov = &riov;
        msg.rma_iov_count = 1;
        iov.iov_base = (char *) req->noncontig.get.origin.pack_buffer + i;
        iov.iov_len = msg_len;
        riov.addr = (uintptr_t) req->noncontig.get.target.iov[target_cur].iov_base;
        riov.len = msg_len;
        riov.key = req->noncontig.get.target.key;
        MPIDI_OFI_INIT_CHUNK_CONTEXT(win, sigreq);
        MPIDI_OFI_CALL_RETRY(fi_readmsg(MPIDI_OFI_WIN(win).ep, &msg, flags), 0, rdma_write, FALSE);
        i += msg_len;

        if (msg_len < req->noncontig.get.target.iov[target_cur].iov_len) {
            req->noncontig.get.target.iov[target_cur].iov_base =
                (char *) req->noncontig.get.target.iov[target_cur].iov_base + msg_len;
            req->noncontig.get.target.iov[target_cur].iov_len -= msg_len;
        } else {
            j++;
        }
    }
    MPIR_Assert(i == get_bytes);
    req->noncontig.get.origin.pack_offset += get_bytes;

    /* not finished. update our place in the target iov array for later. */
    if (req->noncontig.get.origin.pack_offset < req->noncontig.get.origin.total_bytes) {
        req->noncontig.get.target.iov_cur = j;
    } else {
        /* finished issuing. move from deferredQ to syncQ. */
        DL_DELETE(MPIDI_OFI_WIN(win).deferredQ, req);
        req->next = MPIDI_OFI_WIN(win).syncQ;
        MPIDI_OFI_WIN(win).syncQ = req;
        req->noncontig.get.origin.pack_size = get_bytes;
        MPL_free(req->noncontig.get.target.iov);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_OFI_pack_put(const void *origin_addr, int origin_count,
                       MPI_Datatype origin_datatype, int target_rank,
                       MPI_Aint target_disp, int target_count,
                       MPI_Datatype target_datatype, MPIR_Win * win,
                       MPIDI_av_entry_t * addr, MPIR_Request ** sigreq)
{
    int mpi_errno = MPI_SUCCESS;
    size_t target_bytes, origin_bytes;

    MPIR_Datatype_get_size_macro(origin_datatype, origin_bytes);
    origin_bytes *= origin_count;
    MPIR_Datatype_get_size_macro(target_datatype, target_bytes);
    target_bytes *= target_count;

    /* allocate request */
    MPIDI_OFI_win_request_t *req = MPIDI_OFI_win_request_create();
    MPIR_ERR_CHKANDSTMT((req) == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
    req->sigreq = sigreq;

    /* allocate target iovecs */
    struct iovec *target_iov;
    void *target_base;
    MPI_Aint total_target_iov_len;
    MPI_Aint target_len;
    MPIR_Typerep_iov_len(target_count, target_datatype, target_bytes, &total_target_iov_len);
    target_len = MPL_MIN(total_target_iov_len, MPIR_CVAR_CH4_OFI_RMA_IOVEC_MAX);
    target_iov = MPL_malloc(sizeof(struct iovec) * target_len, MPL_MEM_RMA);
    target_base =
        (void *) (MPIDI_OFI_winfo_base(win, target_rank) +
                  +(target_disp * MPIDI_OFI_winfo_disp_unit(win, target_rank)));

    /* allocate pack buffer */
    /* FIXME: use genq API to get pack buffer(s) */
    MPI_Aint pack_bytes = MPL_MIN(origin_bytes, 64 * 1024);
    void *pack_buffer = MPL_malloc(pack_bytes, MPL_MEM_BUFFER);

    /* put on deferred list */
    DL_APPEND(MPIDI_OFI_WIN(win).deferredQ, req);
    req->rma_type = MPIDI_OFI_PUT;

    /* origin */
    req->noncontig.put.origin.addr = origin_addr;
    req->noncontig.put.origin.count = origin_count;
    req->noncontig.put.origin.datatype = origin_datatype;
    req->noncontig.put.origin.pack_buffer = pack_buffer;
    req->noncontig.put.origin.pack_offset = 0;
    req->noncontig.put.origin.pack_size = pack_bytes;
    req->noncontig.put.origin.total_bytes = origin_bytes;

    /* target */
    req->noncontig.put.target.base = target_base;
    req->noncontig.put.target.count = target_count;
    req->noncontig.put.target.datatype = target_datatype;
    req->noncontig.put.target.iov = target_iov;
    req->noncontig.put.target.iov_len = target_len;
    req->noncontig.put.target.iov_offset = 0;
    req->noncontig.put.target.iov_cur = 0;
    req->noncontig.put.target.addr = addr;
    req->noncontig.put.target.key = MPIDI_OFI_winfo_mr_key(win, target_rank);

    mpi_errno = issue_packed_put(win, req);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_OFI_pack_get(void *origin_addr, int origin_count,
                       MPI_Datatype origin_datatype, int target_rank,
                       MPI_Aint target_disp, int target_count,
                       MPI_Datatype target_datatype, MPIR_Win * win,
                       MPIDI_av_entry_t * addr, MPIR_Request ** sigreq)
{
    int mpi_errno = MPI_SUCCESS;
    size_t target_bytes, origin_bytes;

    MPIR_Datatype_get_size_macro(origin_datatype, origin_bytes);
    origin_bytes *= origin_count;
    MPIR_Datatype_get_size_macro(target_datatype, target_bytes);
    target_bytes *= target_count;

    /* allocate request */
    MPIDI_OFI_win_request_t *req = MPIDI_OFI_win_request_create();
    MPIR_ERR_CHKANDSTMT((req) == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
    req->sigreq = sigreq;

    /* allocate target iovecs */
    struct iovec *target_iov;
    void *target_base;
    MPI_Aint total_target_iov_len;
    MPI_Aint target_len;
    MPIR_Typerep_iov_len(target_count, target_datatype, target_bytes, &total_target_iov_len);
    target_len = MPL_MIN(total_target_iov_len, MPIR_CVAR_CH4_OFI_RMA_IOVEC_MAX);
    target_iov = MPL_malloc(sizeof(struct iovec) * target_len, MPL_MEM_RMA);
    target_base =
        (void *) (MPIDI_OFI_winfo_base(win, target_rank) +
                  +(target_disp * MPIDI_OFI_winfo_disp_unit(win, target_rank)));

    /* allocate pack buffer */
    /* FIXME: use genq API to get pack buffer(s) */
    MPI_Aint pack_bytes = MPL_MIN(origin_bytes, 64 * 1024);
    void *pack_buffer = MPL_malloc(pack_bytes, MPL_MEM_BUFFER);

    /* put on deferred list */
    DL_APPEND(MPIDI_OFI_WIN(win).deferredQ, req);
    req->rma_type = MPIDI_OFI_GET;

    /* origin */
    req->noncontig.put.origin.addr = origin_addr;
    req->noncontig.put.origin.count = origin_count;
    req->noncontig.put.origin.datatype = origin_datatype;
    req->noncontig.put.origin.pack_buffer = pack_buffer;
    req->noncontig.put.origin.pack_offset = 0;
    req->noncontig.put.origin.pack_size = pack_bytes;
    req->noncontig.put.origin.total_bytes = origin_bytes;

    /* target */
    req->noncontig.put.target.base = target_base;
    req->noncontig.put.target.count = target_count;
    req->noncontig.put.target.datatype = target_datatype;
    req->noncontig.put.target.iov = target_iov;
    req->noncontig.put.target.iov_len = target_len;
    req->noncontig.put.target.iov_offset = 0;
    req->noncontig.put.target.iov_cur = 0;
    req->noncontig.put.target.addr = addr;
    req->noncontig.put.target.key = MPIDI_OFI_winfo_mr_key(win, target_rank);

    mpi_errno = issue_packed_get(win, req);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_OFI_issue_deferred_rma(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_OFI_win_request_t *req = MPIDI_OFI_WIN(win).deferredQ;

    while (req) {
        switch (req->rma_type) {
            case MPIDI_OFI_PUT:
                mpi_errno = issue_packed_put(win, req);
                MPIR_ERR_CHECK(mpi_errno);
                break;
            case MPIDI_OFI_GET:
                mpi_errno = issue_packed_get(win, req);
                MPIR_ERR_CHECK(mpi_errno);
                break;
            default:
                MPIR_Assert(0);
        }
        req = req->next;
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

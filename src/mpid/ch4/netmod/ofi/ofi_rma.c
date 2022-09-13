/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "ofi_impl.h"
#include "ofi_noinline.h"

static MPIDI_OFI_pack_chunk *create_chunk(void *pack_buffer, MPI_Aint unpack_size,
                                          MPI_Aint unpack_offset, MPIDI_OFI_win_request_t * winreq)
{
    MPIDI_OFI_pack_chunk *chunk = MPL_malloc(sizeof(MPIDI_OFI_chunk_request), MPL_MEM_RMA);
    if (chunk == NULL)
        return NULL;

    chunk->pack_buffer = pack_buffer;
    chunk->unpack_size = unpack_size;
    chunk->unpack_offset = unpack_offset;

    /* prepend to chunk list */
    chunk->next = winreq->chunks;
    winreq->chunks = chunk;

    return chunk;
}

void MPIDI_OFI_complete_chunks(MPIDI_OFI_win_request_t * winreq)
{
    MPIDI_OFI_pack_chunk *chunk = winreq->chunks;
    int vni = winreq->vni;

    while (chunk) {
        if (chunk->unpack_size > 0) {
            MPI_Aint actual_unpack_bytes;
            MPIR_Typerep_unpack(chunk->pack_buffer,
                                chunk->unpack_size, winreq->noncontig.get.origin.addr,
                                winreq->noncontig.get.origin.count,
                                winreq->noncontig.get.origin.datatype, chunk->unpack_offset,
                                &actual_unpack_bytes, MPIR_TYPEREP_FLAG_NONE);
            MPIR_Assert(chunk->unpack_size == actual_unpack_bytes);
        }

        MPIDI_OFI_pack_chunk *next = chunk->next;
        MPIDU_genq_private_pool_free_cell(MPIDI_global.per_vci[vni].pack_buf_pool,
                                          chunk->pack_buffer);
        MPL_free(chunk);
        chunk = next;
    }

    winreq->chunks = NULL;
}

int MPIDI_OFI_nopack_putget(const void *origin_addr, MPI_Aint origin_count,
                            MPI_Datatype origin_datatype, int target_rank,
                            MPI_Aint target_count, MPI_Datatype target_datatype,
                            MPIDI_OFI_target_mr_t target_mr, MPIR_Win * win,
                            MPIDI_av_entry_t * addr, int rma_type, MPIR_Request ** sigreq)
{
    int mpi_errno = MPI_SUCCESS;
    uint64_t flags;
    struct fi_msg_rma msg;
    struct fi_rma_iov riov;
    struct iovec iov;

    /* allocate request */
    MPIDI_OFI_win_request_t *req = MPIDI_OFI_win_request_create();
    MPIR_ERR_CHKANDSTMT((req) == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
    req->vni = MPIDI_WIN(win, am_vci);
    req->next = MPIDI_OFI_WIN(win).syncQ;
    MPIDI_OFI_WIN(win).syncQ = req;
    req->sigreq = sigreq;
    req->chunks = NULL;
    if (rma_type == MPIDI_OFI_PUT) {
        req->noncontig.put.origin.datatype = MPI_DATATYPE_NULL;
        req->noncontig.put.target.datatype = MPI_DATATYPE_NULL;
    } else {
        req->noncontig.get.origin.datatype = MPI_DATATYPE_NULL;
        req->noncontig.get.target.datatype = MPI_DATATYPE_NULL;
    }

    /* allocate target iovecs */
    struct iovec *target_iov;
    MPI_Aint total_target_iov_len;
    MPI_Aint target_len;
    MPI_Aint target_iov_offset = 0;
    MPIR_Typerep_get_iov_len(target_count, target_datatype, &total_target_iov_len);
    target_len = MPL_MIN(total_target_iov_len, MPIR_CVAR_CH4_OFI_RMA_IOVEC_MAX);
    target_iov = MPL_malloc(sizeof(struct iovec) * target_len, MPL_MEM_RMA);

    /* allocate origin iovecs */
    struct iovec *origin_iov;
    MPI_Aint total_origin_iov_len;
    MPI_Aint origin_len;
    MPI_Aint origin_iov_offset = 0;
    MPIR_Typerep_get_iov_len(origin_count, origin_datatype, &total_origin_iov_len);
    origin_len = MPL_MIN(total_origin_iov_len, MPIR_CVAR_CH4_OFI_RMA_IOVEC_MAX);
    origin_iov = MPL_malloc(sizeof(struct iovec) * origin_len, MPL_MEM_RMA);

    if (sigreq) {
        MPIDI_OFI_REQUEST_CREATE(*sigreq, MPIR_REQUEST_KIND__RMA, 0);
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
            MPIDI_OFI_load_iov((const void *) (uintptr_t) target_mr.addr, target_count,
                               target_datatype, target_len, &target_iov_offset, target_iov);

        msg_len = MPL_MIN(origin_iov[origin_cur].iov_len, target_iov[target_cur].iov_len);

        int vni = MPIDI_WIN(win, am_vci);
        int nic = 0;
        msg.desc = NULL;
        msg.addr = MPIDI_OFI_av_to_phys(addr, nic, vni, vni);
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
        riov.key = target_mr.mr_key;
        MPIDI_OFI_INIT_CHUNK_CONTEXT(win, sigreq);
        if (rma_type == MPIDI_OFI_PUT) {
            MPIDI_OFI_CALL_RETRY(fi_writemsg(MPIDI_OFI_WIN(win).ep, &msg, flags), vni, rdma_write,
                                 FALSE);
            req->rma_type = MPIDI_OFI_PUT;
        } else {        /* MPIDI_OFI_GET */
            MPIDI_OFI_CALL_RETRY(fi_readmsg(MPIDI_OFI_WIN(win).ep, &msg, flags), vni, rdma_write,
                                 FALSE);
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

static int issue_packed_put(MPIR_Win * win, MPIDI_OFI_win_request_t * req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request **sigreq = req->sigreq;
    MPI_Aint actual_pack_bytes;
    struct fi_msg_rma msg;
    struct iovec iov;
    struct fi_rma_iov riov;
    uint64_t flags;
    void *pack_buffer;
    int vni = req->vni;

    if (sigreq)
        flags = FI_COMPLETION | FI_DELIVERY_COMPLETE;
    else
        flags = FI_DELIVERY_COMPLETE;

    int j = req->noncontig.put.target.iov_cur;
    size_t msg_len;
    while (req->noncontig.put.origin.pack_offset < req->noncontig.put.origin.total_bytes) {
        MPIDU_genq_private_pool_alloc_cell(MPIDI_global.per_vci[vni].pack_buf_pool, &pack_buffer);
        if (pack_buffer == NULL)
            break;

        MPI_Aint target_cur = j % req->noncontig.put.target.iov_len;
        if (j == req->noncontig.put.target.iov_offset)
            MPIDI_OFI_load_iov(req->noncontig.put.target.base,
                               req->noncontig.put.target.count,
                               req->noncontig.put.target.datatype,
                               req->noncontig.put.target.iov_len,
                               &req->noncontig.put.target.iov_offset,
                               req->noncontig.put.target.iov);

        msg_len =
            MPL_MIN(MPIDI_OFI_DEFAULT_SHORT_SEND_SIZE,
                    req->noncontig.put.target.iov[target_cur].iov_len);
        /* load the pack buffer */
        MPIR_Typerep_pack(req->noncontig.put.origin.addr, req->noncontig.put.origin.count,
                          req->noncontig.put.origin.datatype,
                          req->noncontig.put.origin.pack_offset, pack_buffer,
                          msg_len, &actual_pack_bytes, MPIR_TYPEREP_FLAG_NONE);
        MPIR_Assert(msg_len == actual_pack_bytes);

        MPIDI_OFI_pack_chunk *chunk = create_chunk(pack_buffer, 0, 0, req);
        MPIR_ERR_CHKANDSTMT(chunk == NULL, mpi_errno, MPI_ERR_NO_MEM, goto fn_fail, "**nomem");

        int nic = 0;
        msg.desc = NULL;
        msg.addr = MPIDI_OFI_av_to_phys(req->noncontig.put.target.addr, nic, vni, vni);
        msg.context = NULL;
        msg.data = 0;
        msg.msg_iov = &iov;
        msg.iov_count = 1;
        msg.rma_iov = &riov;
        msg.rma_iov_count = 1;
        iov.iov_base = pack_buffer;
        iov.iov_len = msg_len;
        riov.addr = (uintptr_t) req->noncontig.put.target.iov[target_cur].iov_base;
        riov.len = msg_len;
        riov.key = req->noncontig.put.target.key;
        MPIDI_OFI_INIT_CHUNK_CONTEXT(win, sigreq);
        MPIDI_OFI_CALL_RETRY(fi_writemsg(MPIDI_OFI_WIN(win).ep, &msg, flags), vni, rdma_write,
                             FALSE);
        req->noncontig.put.origin.pack_offset += msg_len;

        if (msg_len < req->noncontig.put.target.iov[target_cur].iov_len) {
            req->noncontig.put.target.iov[target_cur].iov_base =
                (char *) req->noncontig.put.target.iov[target_cur].iov_base + msg_len;
            req->noncontig.put.target.iov[target_cur].iov_len -= msg_len;
        } else {
            j++;
        }
    }

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
    struct fi_msg_rma msg;
    struct iovec iov;
    struct fi_rma_iov riov;
    uint64_t flags;
    void *pack_buffer;
    int vni = req->vni;

    if (sigreq)
        flags = FI_COMPLETION | FI_DELIVERY_COMPLETE;
    else
        flags = FI_DELIVERY_COMPLETE;

    int j = req->noncontig.get.target.iov_cur;
    size_t msg_len;
    while (req->noncontig.get.origin.pack_offset < req->noncontig.get.origin.total_bytes) {
        MPIDU_genq_private_pool_alloc_cell(MPIDI_global.per_vci[vni].pack_buf_pool, &pack_buffer);
        if (pack_buffer == NULL)
            break;

        MPI_Aint target_cur = j % req->noncontig.get.target.iov_len;
        if (j == req->noncontig.get.target.iov_offset)
            MPIDI_OFI_load_iov(req->noncontig.get.target.base,
                               req->noncontig.get.target.count,
                               req->noncontig.get.target.datatype,
                               req->noncontig.get.target.iov_len,
                               &req->noncontig.get.target.iov_offset,
                               req->noncontig.get.target.iov);

        msg_len =
            MPL_MIN(MPIDI_OFI_DEFAULT_SHORT_SEND_SIZE,
                    req->noncontig.get.target.iov[target_cur].iov_len);

        MPIDI_OFI_pack_chunk *chunk =
            create_chunk(pack_buffer, msg_len, req->noncontig.get.origin.pack_offset, req);
        MPIR_ERR_CHKANDSTMT(chunk == NULL, mpi_errno, MPI_ERR_NO_MEM, goto fn_fail, "**nomem");

        int nic = 0;
        msg.desc = NULL;
        msg.addr = MPIDI_OFI_av_to_phys(req->noncontig.get.target.addr, nic, vni, vni);
        msg.context = NULL;
        msg.data = 0;
        msg.msg_iov = &iov;
        msg.iov_count = 1;
        msg.rma_iov = &riov;
        msg.rma_iov_count = 1;
        iov.iov_base = pack_buffer;
        iov.iov_len = msg_len;
        riov.addr = (uintptr_t) req->noncontig.get.target.iov[target_cur].iov_base;
        riov.len = msg_len;
        riov.key = req->noncontig.get.target.key;
        MPIDI_OFI_INIT_CHUNK_CONTEXT(win, sigreq);
        MPIDI_OFI_CALL_RETRY(fi_readmsg(MPIDI_OFI_WIN(win).ep, &msg, flags), vni, rdma_write,
                             FALSE);
        req->noncontig.get.origin.pack_offset += msg_len;

        if (msg_len < req->noncontig.get.target.iov[target_cur].iov_len) {
            req->noncontig.get.target.iov[target_cur].iov_base =
                (char *) req->noncontig.get.target.iov[target_cur].iov_base + msg_len;
            req->noncontig.get.target.iov[target_cur].iov_len -= msg_len;
        } else {
            j++;
        }
    }

    /* not finished. update our place in the target iov array for later. */
    if (req->noncontig.get.origin.pack_offset < req->noncontig.get.origin.total_bytes) {
        req->noncontig.get.target.iov_cur = j;
    } else {
        /* finished issuing. move from deferredQ to syncQ. */
        DL_DELETE(MPIDI_OFI_WIN(win).deferredQ, req);
        req->next = MPIDI_OFI_WIN(win).syncQ;
        MPIDI_OFI_WIN(win).syncQ = req;
        MPL_free(req->noncontig.get.target.iov);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_OFI_pack_put(const void *origin_addr, MPI_Aint origin_count,
                       MPI_Datatype origin_datatype, int target_rank,
                       MPI_Aint target_count, MPI_Datatype target_datatype,
                       MPIDI_OFI_target_mr_t target_mr, MPIR_Win * win,
                       MPIDI_av_entry_t * addr, MPIR_Request ** sigreq)
{
    int mpi_errno = MPI_SUCCESS;
    size_t origin_bytes;

    MPIR_Datatype_get_size_macro(origin_datatype, origin_bytes);
    origin_bytes *= origin_count;

    /* allocate request */
    MPIDI_OFI_win_request_t *req = MPIDI_OFI_win_request_create();
    MPIR_ERR_CHKANDSTMT((req) == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
    req->vni = MPIDI_WIN(win, am_vci);
    req->sigreq = sigreq;

    /* allocate target iovecs */
    struct iovec *target_iov;
    MPI_Aint total_target_iov_len;
    MPI_Aint target_len;
    MPIR_Typerep_get_iov_len(target_count, target_datatype, &total_target_iov_len);
    target_len = MPL_MIN(total_target_iov_len, MPIR_CVAR_CH4_OFI_RMA_IOVEC_MAX);
    target_iov = MPL_malloc(sizeof(struct iovec) * target_len, MPL_MEM_RMA);

    /* put on deferred list */
    DL_APPEND(MPIDI_OFI_WIN(win).deferredQ, req);
    req->rma_type = MPIDI_OFI_PUT;
    req->chunks = NULL;

    /* origin */
    req->noncontig.put.origin.addr = origin_addr;
    req->noncontig.put.origin.count = origin_count;
    req->noncontig.put.origin.datatype = origin_datatype;
    MPIR_Datatype_add_ref_if_not_builtin(origin_datatype);
    req->noncontig.put.origin.pack_offset = 0;
    req->noncontig.put.origin.total_bytes = origin_bytes;

    /* target */
    req->noncontig.put.target.base = (void *) (uintptr_t) target_mr.addr;
    req->noncontig.put.target.count = target_count;
    req->noncontig.put.target.datatype = target_datatype;
    MPIR_Datatype_add_ref_if_not_builtin(target_datatype);
    req->noncontig.put.target.iov = target_iov;
    req->noncontig.put.target.iov_len = target_len;
    req->noncontig.put.target.iov_offset = 0;
    req->noncontig.put.target.iov_cur = 0;
    req->noncontig.put.target.addr = addr;
    req->noncontig.put.target.key = target_mr.mr_key;

    mpi_errno = issue_packed_put(win, req);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_OFI_pack_get(void *origin_addr, MPI_Aint origin_count,
                       MPI_Datatype origin_datatype, int target_rank,
                       MPI_Aint target_count, MPI_Datatype target_datatype,
                       MPIDI_OFI_target_mr_t target_mr, MPIR_Win * win,
                       MPIDI_av_entry_t * addr, MPIR_Request ** sigreq)
{
    int mpi_errno = MPI_SUCCESS;
    size_t origin_bytes;

    MPIR_Datatype_get_size_macro(origin_datatype, origin_bytes);
    origin_bytes *= origin_count;

    /* allocate request */
    MPIDI_OFI_win_request_t *req = MPIDI_OFI_win_request_create();
    MPIR_ERR_CHKANDSTMT((req) == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
    req->vni = MPIDI_WIN(win, am_vci);
    req->sigreq = sigreq;

    /* allocate target iovecs */
    struct iovec *target_iov;
    MPI_Aint total_target_iov_len;
    MPI_Aint target_len;
    MPIR_Typerep_get_iov_len(target_count, target_datatype, &total_target_iov_len);
    target_len = MPL_MIN(total_target_iov_len, MPIR_CVAR_CH4_OFI_RMA_IOVEC_MAX);
    target_iov = MPL_malloc(sizeof(struct iovec) * target_len, MPL_MEM_RMA);

    /* put on deferred list */
    DL_APPEND(MPIDI_OFI_WIN(win).deferredQ, req);
    req->rma_type = MPIDI_OFI_GET;
    req->chunks = NULL;

    /* origin */
    req->noncontig.get.origin.addr = origin_addr;
    req->noncontig.get.origin.count = origin_count;
    req->noncontig.get.origin.datatype = origin_datatype;
    MPIR_Datatype_add_ref_if_not_builtin(origin_datatype);
    req->noncontig.get.origin.pack_offset = 0;
    req->noncontig.get.origin.total_bytes = origin_bytes;

    /* target */
    req->noncontig.get.target.base = (void *) (uintptr_t) target_mr.addr;
    req->noncontig.get.target.count = target_count;
    req->noncontig.get.target.datatype = target_datatype;
    MPIR_Datatype_add_ref_if_not_builtin(target_datatype);
    req->noncontig.get.target.iov = target_iov;
    req->noncontig.get.target.iov_len = target_len;
    req->noncontig.get.target.iov_offset = 0;
    req->noncontig.get.target.iov_cur = 0;
    req->noncontig.get.target.addr = addr;
    req->noncontig.get.target.key = target_mr.mr_key;

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
        /* free temporary buffers */
        MPIDI_OFI_complete_chunks(req);

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

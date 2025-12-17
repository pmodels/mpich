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
    int vci = winreq->vci_local;

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
        MPIDU_genq_private_pool_free_cell(MPIDI_global.per_vci[vci].pack_buf_pool,
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
    size_t origin_bytes;
    int vci = MPIDI_WIN(win, am_vci);
    int vci_target = MPIDI_WIN_TARGET_VCI(win, target_rank);

    /* used for GPU buffer registration */
    MPIR_Datatype_get_size_macro(origin_datatype, origin_bytes);
    origin_bytes *= origin_count;

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
        MPIDI_OFI_REQUEST_CREATE(*sigreq, MPIR_REQUEST_KIND__RMA, vci);
        flags = FI_COMPLETION | FI_DELIVERY_COMPLETE;
    } else {
        flags = FI_DELIVERY_COMPLETE;
    }

    void *desc = NULL;
    int nic_target = MPIDI_OFI_get_pref_nic(win->comm_ptr, target_rank);;

    MPIDI_OFI_gpu_rma_register(origin_addr, origin_bytes, NULL, win, nic_target, &desc);

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

        msg.desc = desc;
        msg.addr = MPIDI_OFI_av_to_phys(addr, vci, 0, vci_target, nic_target);
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
            MPIDI_OFI_CALL_RETRY(fi_writemsg(MPIDI_OFI_WIN(win).ep, &msg, flags), vci, rdma_write);
        } else {        /* MPIDI_OFI_GET */
            MPIDI_OFI_CALL_RETRY(fi_readmsg(MPIDI_OFI_WIN(win).ep, &msg, flags), vci, rdma_write);
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
    int vci = req->vci_local;
    int vci_target = req->vci_target;
    int nic_target = req->nic_target;

    if (sigreq)
        flags = FI_COMPLETION | FI_DELIVERY_COMPLETE;
    else
        flags = FI_DELIVERY_COMPLETE;

    int j = req->noncontig.put.target.iov_cur;
    size_t msg_len;
    while (req->noncontig.put.origin.pack_offset < req->noncontig.put.origin.total_bytes) {
        MPIDU_genq_private_pool_alloc_cell(MPIDI_global.per_vci[vci].pack_buf_pool, &pack_buffer);
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

        msg.desc = NULL;
        msg.addr =
            MPIDI_OFI_av_to_phys(req->noncontig.put.target.addr, vci, 0, vci_target, nic_target);
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
        MPIDI_OFI_CALL_RETRY(fi_writemsg(MPIDI_OFI_WIN(win).ep, &msg, flags), vci, rdma_write);
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
    int vci = req->vci_local;
    int vci_target = req->vci_target;
    int nic_target = req->nic_target;

    if (sigreq)
        flags = FI_COMPLETION | FI_DELIVERY_COMPLETE;
    else
        flags = FI_DELIVERY_COMPLETE;

    int j = req->noncontig.get.target.iov_cur;
    size_t msg_len;
    while (req->noncontig.get.origin.pack_offset < req->noncontig.get.origin.total_bytes) {
        MPIDU_genq_private_pool_alloc_cell(MPIDI_global.per_vci[vci].pack_buf_pool, &pack_buffer);
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

        msg.desc = NULL;
        msg.addr =
            MPIDI_OFI_av_to_phys(req->noncontig.get.target.addr, vci, 0, vci_target, nic_target);
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
        MPIDI_OFI_CALL_RETRY(fi_readmsg(MPIDI_OFI_WIN(win).ep, &msg, flags), vci, rdma_write);
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
    req->vci_local = MPIDI_WIN(win, am_vci);
    req->vci_target = MPIDI_WIN_TARGET_VCI(win, target_rank);
    req->nic_target = MPIDI_OFI_get_pref_nic(win->comm_ptr, target_rank);
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
    req->vci_local = MPIDI_WIN(win, am_vci);
    req->vci_target = MPIDI_WIN_TARGET_VCI(win, target_rank);
    req->nic_target = MPIDI_OFI_get_pref_nic(win->comm_ptr, target_rank);
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

/* -- active message fallback using mirror buffers -- */

/* assumptions:
 * 1. both origin and target datatypes are contig
 * 2. data_sz <= MPIDI_OFI_global.max_msg_size
 */

/* Get using AM mirror buffer -
 * 1. Origin send am MPIDI_OFI_GET_REQ
 * 2. Target async localcopy to mirror buffer
 * 3. Target send am MPIDI_OFI_GET_ACK
 * 4. Origin RDMA read
 * 5. Origin complete
 */

struct get_context {
    MPIR_Win *win;
    int target_rank;
    MPI_Aint data_sz;
    void *origin_addr;
    MPI_Aint target_offset;
    MPIR_Request *req;
};

struct get_hdr {
    uint64_t win_id;
    int origin_rank;
    void *origin_context;
    MPI_Aint target_offset;
    MPI_Aint data_sz;
};

/* origin side - issue AM req */
int MPIDI_OFI_mirror_get(void *origin_addr, MPI_Aint origin_count, MPI_Datatype origin_datatype,
                         int target_rank, MPI_Aint target_disp, MPI_Aint target_count,
                         MPI_Datatype target_datatype, MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    /* query target datatype */
    int is_contig;
    MPIR_Datatype_is_contig(target_datatype, &is_contig);

    MPI_Aint data_sz;
    MPIR_Datatype_get_size_macro(origin_datatype, data_sz);
    data_sz *= origin_count;

    MPI_Aint origin_true_lb, target_true_lb;
    MPIR_Datatype_get_true_lb(target_datatype, &target_true_lb);
    MPIR_Datatype_get_true_lb(origin_datatype, &origin_true_lb);

    int vci = MPIDI_WIN(win, am_vci);
    int vci_target = MPIDI_WIN_TARGET_VCI(win, target_rank);

    /* fill origin context */
    struct get_context *origin_context;
    origin_context = MPL_malloc(sizeof(struct get_context), MPL_MEM_OTHER);
    MPIR_ERR_CHKANDJUMP((origin_context == NULL), mpi_errno, MPI_ERR_OTHER, "**nomem");

    origin_context->win = win;
    origin_context->target_rank = target_rank;
    origin_context->data_sz = data_sz;
    origin_context->origin_addr = (char *) origin_addr + origin_true_lb;
    origin_context->target_offset = target_disp * win->disp_unit + target_true_lb;

    /* allocate a request, used for reuse the code from ofi_rndv_read. */
    MPIR_Request *req;
    MPIDI_OFI_REQUEST_CREATE(req, MPIR_REQUEST_KIND__RMA, vci);
    if (1) {
        MPIDI_CH4_REQUEST_FREE(req);
    }
    origin_context->req = req;

    /* fill am_hdr */
    struct get_hdr am_hdr;
    am_hdr.win_id = MPIDIG_WIN(win, win_id);
    am_hdr.origin_rank = win->comm_ptr->rank;
    am_hdr.origin_context = origin_context;
    am_hdr.data_sz = origin_context->data_sz;
    am_hdr.target_offset = origin_context->target_offset;

    MPIDIG_win_cmpl_cnts_incr(win, target_rank, NULL);

    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI_LOCK(vci));
    mpi_errno = MPIDI_NM_am_send_hdr(target_rank, win->comm_ptr, MPIDI_OFI_GET_REQ,
                                     &am_hdr, sizeof(am_hdr), vci, vci_target);
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI_LOCK(vci));
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

struct target_mirror_copy {
    MPIR_Win *win;
    int origin_rank;
    void *origin_context;
    int vci_origin;
    int vci_target;
    MPIR_gpu_req async_req;
};

static int target_mirror_copy_poll(MPIX_Async_thing thing);

/* target side - AM callback */
int MPIDI_OFI_get_handler(void *am_hdr, void *data, MPI_Aint in_data_sz,
                          uint32_t attr, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    struct get_hdr *msg_hdr = am_hdr;

    MPIR_Win *win;
    win = (MPIR_Win *) MPIDIU_map_lookup(MPIDI_global.win_map, msg_hdr->win_id);
    MPIR_Assert(win);

    void *mirror_buf = MPIDI_OFI_WIN(win).mirror_buf;
    void *mirror_attr = &(MPIDI_OFI_WIN(win).mirror_attr);
    void *base_buf = win->base;
    void *base_attr = &(MPIDI_OFI_WIN(win).base_attr);

    /* async localcopy */
    MPIR_gpu_req async_req;
    int engine_type = MPIDI_OFI_gpu_get_send_engine_type();
    mpi_errno = MPIR_Ilocalcopy_gpu(base_buf, msg_hdr->data_sz, MPIR_BYTE_INTERNAL,
                                    msg_hdr->target_offset, base_attr,
                                    mirror_buf, msg_hdr->data_sz, MPIR_BYTE_INTERNAL,
                                    msg_hdr->target_offset, mirror_attr,
                                    engine_type, 1, &async_req);
    MPIR_ERR_CHECK(mpi_errno);

    /* add async things */
    struct target_mirror_copy *p = MPL_malloc(sizeof(struct target_mirror_copy), MPL_MEM_OTHER);
    p->win = win;
    p->origin_rank = msg_hdr->origin_rank;
    p->origin_context = msg_hdr->origin_context;
    p->vci_origin = MPIDIG_AM_ATTR_SRC_VCI(attr);
    p->vci_target = MPIDIG_AM_ATTR_DST_VCI(attr);
    p->async_req = async_req;

    mpi_errno = MPIR_Async_things_add(target_mirror_copy_poll, p, NULL);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

struct getack_hdr {
    void *origin_context;
    uint64_t rkey;
    uint64_t remote_base;
};

/* target side - async callback */
static int target_mirror_copy_poll(MPIX_Async_thing thing)
{
    struct target_mirror_copy *p = MPIR_Async_thing_get_state(thing);
    int is_done;
    MPIR_async_test(&p->async_req, &is_done);

    if (is_done) {
        /* send get_ack */
        struct getack_hdr am_hdr;
        am_hdr.origin_context = p->origin_context;
        am_hdr.rkey = fi_mr_key(MPIDI_OFI_WIN(p->win).mr);
        am_hdr.remote_base = (uintptr_t) MPIDI_OFI_WIN(p->win).mirror_buf;

        int rc = MPIDI_NM_am_send_hdr(p->origin_rank, p->win->comm_ptr, MPIDI_OFI_GET_ACK,
                                      &am_hdr, sizeof(am_hdr), p->vci_target, p->vci_origin);
        MPIR_Assertp(rc == MPI_SUCCESS);

        MPL_free(p);

        return MPIX_ASYNC_DONE;
    }

    return MPIX_ASYNC_NOPROGRESS;
}

struct read_req {
    char pad[MPIDI_REQUEST_HDR_SIZE];
    struct fi_context context[MPIDI_OFI_CONTEXT_STRUCTS];
    int event_id;
    struct get_context *origin_context;
};

static int rdmaread_completion(void *context);

/* origin side - AM callback */
int MPIDI_OFI_getack_handler(void *am_hdr, void *data, MPI_Aint in_data_sz,
                             uint32_t attr, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;

    struct getack_hdr *msg_hdr = am_hdr;
    struct get_context *origin_context = msg_hdr->origin_context;
    MPIR_Win *win = origin_context->win;
    int target_rank = origin_context->target_rank;
    MPI_Aint target_offset = origin_context->target_offset;

    MPIDI_OFI_rndvread_t *p = &MPIDI_OFI_AMREQ_READ(origin_context->req);
    p->buf = origin_context->origin_addr;
    p->count = origin_context->data_sz;
    p->datatype = MPIR_BYTE_INTERNAL;

    MPIR_GPU_query_pointer_attr(p->buf, &p->attr);
    p->need_pack = MPL_gpu_attr_is_dev(&p->attr);

    p->data_sz = p->remote_data_sz = origin_context->data_sz;
    p->vci_local = MPIDI_WIN(win, am_vci);
    p->vci_remote = MPIDI_WIN_TARGET_VCI(win, target_rank);
    p->av = MPIDIU_win_rank_to_av(win, target_rank, MPIDI_WIN(win, winattr));

    p->num_nics = 1;
    if (MPIDI_OFI_ENABLE_MR_VIRT_ADDRESS) {
        p->u.recv.remote_base = msg_hdr->remote_base + target_offset;
    } else {
        p->u.recv.remote_base = target_offset;
    }
    p->u.recv.rkeys = &p->u.recv.rkey0;
    p->u.recv.rkey0 = msg_hdr->rkey;
    p->u.recv.cmpl_cb = rdmaread_completion;
    p->u.recv.context = origin_context;

    mpi_errno = MPIR_Async_things_add(MPIDI_OFI_rdmaread_poll, origin_context->req, NULL);

    return mpi_errno;
}

static int rdmaread_completion(void *context)
{
    struct get_context *origin_context = context;

    MPIR_Win *win = origin_context->win;
    int target_rank = origin_context->target_rank;

    MPIDIG_win_cmpl_cnts_decr(win, target_rank);

    MPIDI_Request_complete_fast(origin_context->req);
    MPL_free(origin_context);

    return MPI_SUCCESS;
}

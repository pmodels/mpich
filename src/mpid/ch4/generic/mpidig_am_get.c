#include "mpidimpl.h"

MPIR_T_pvar_timer_t PVAR_TIMER_rma_targetcb_get ATTRIBUTE((unused));
MPIR_T_pvar_timer_t PVAR_TIMER_rma_targetcb_get_ack ATTRIBUTE((unused));

static int MPIDIG_get_origin_cb(MPIR_Request * sreq);
static int MPIDIG_get_target_msg_cb(int handler_id, void *am_hdr, void **data, size_t * p_data_sz,
                                    int is_local, int *is_contig,
                                    MPIDIG_am_target_cmpl_cb * target_cmpl_cb, MPIR_Request ** req);
static int do_get_ack(MPIR_Request * req);
static int MPIDIG_get_ack_target_msg_cb(int handler_id, void *am_hdr,
                                        void **data, size_t * p_data_sz,
                                        int is_local, int *is_contig,
                                        MPIDIG_am_target_cmpl_cb * target_cmpl_cb,
                                        MPIR_Request ** req);
static int get_ack_target_cmpl_cb(MPIR_Request * get_req);

int MPIDIG_am_get_init(void)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIDIG_am_reg_cb(MPIDIG_GET_REQ, &MPIDIG_get_origin_cb, &MPIDIG_get_target_msg_cb);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIDIG_am_reg_cb(MPIDIG_GET_ACK, NULL, &MPIDIG_get_ack_target_msg_cb);
    MPIR_ERR_CHECK(mpi_errno);

    /* rma_targetcb_get */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(RMA,
                                      MPI_DOUBLE,
                                      rma_targetcb_get,
                                      MPI_T_VERBOSITY_MPIDEV_DETAIL,
                                      MPI_T_BIND_NO_OBJECT,
                                      MPIR_T_PVAR_FLAG_READONLY,
                                      "RMA", "RMA:TARGETCB for Get (in seconds)");

    /* rma_targetcb_get_ack */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(RMA,
                                      MPI_DOUBLE,
                                      rma_targetcb_get_ack,
                                      MPI_T_VERBOSITY_MPIDEV_DETAIL,
                                      MPI_T_BIND_NO_OBJECT,
                                      MPIR_T_PVAR_FLAG_READONLY,
                                      "RMA", "RMA:TARGETCB for Get ACK (in seconds)");

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* MPIDIG_GET_REQ */
struct am_get_hdr {
    int src_rank;
    uint64_t win_id;
    MPIR_Request *greq_ptr;
    MPI_Aint target_disp;
    uint64_t count;
    MPI_Datatype datatype;
    int n_iov;
};

int MPIDIG_do_get(void *origin_addr, int origin_count, MPI_Datatype origin_datatype,
                  int target_rank, MPI_Aint target_disp, int target_count,
                  MPI_Datatype target_datatype, MPIR_Win * win, MPIR_Request ** sreq_ptr)
{
    int mpi_errno = MPI_SUCCESS, n_iov, c;
    size_t offset;
    MPIR_Request *sreq = NULL;
    size_t data_sz;
    MPI_Aint num_iov;
    struct iovec *dt_iov;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_DO_GET);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_DO_GET);

    MPIDIG_RMA_OP_CHECK_SYNC(target_rank, win);

    MPIDI_Datatype_check_size(origin_datatype, origin_count, data_sz);
    if (data_sz == 0)
        goto immed_cmpl;

    if (target_rank == win->comm_ptr->rank) {
        offset = win->disp_unit * target_disp;
        mpi_errno = MPIR_Localcopy((char *) win->base + offset,
                                   target_count,
                                   target_datatype, origin_addr, origin_count, origin_datatype);
        MPIR_ERR_CHECK(mpi_errno);
        goto immed_cmpl;
    }

    /* Only create request when issuing is not completed.
     * We initialize two ref_count for progress engine and request-based OP,
     * then get needs to free the second ref_count.*/
    sreq = MPIDIG_request_create(MPIR_REQUEST_KIND__RMA, 2);
    MPIR_ERR_CHKANDSTMT(sreq == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");

    MPIDIG_REQUEST(sreq, req->greq.win_ptr) = win;
    MPIDIG_REQUEST(sreq, req->greq.addr) = origin_addr;
    MPIDIG_REQUEST(sreq, req->greq.count) = origin_count;
    MPIDIG_REQUEST(sreq, req->greq.datatype) = origin_datatype;
    MPIDIG_REQUEST(sreq, rank) = target_rank;
    MPIR_Datatype_add_ref_if_not_builtin(origin_datatype);

    MPIR_cc_incr(sreq->cc_ptr, &c);
    MPIR_T_PVAR_TIMER_START(RMA, rma_amhdr_set);

    struct am_get_hdr am_hdr;
    am_hdr.target_disp = target_disp;
    am_hdr.count = target_count;
    am_hdr.datatype = target_datatype;
    am_hdr.greq_ptr = sreq;
    am_hdr.win_id = MPIDIG_WIN(win, win_id);
    am_hdr.src_rank = win->comm_ptr->rank;

    /* Increase local and remote completion counters and set the local completion
     * counter in request, thus it can be decreased at request completion. */
    MPIDIG_win_cmpl_cnts_incr(win, target_rank, &sreq->completion_notification);

#ifndef MPIDI_CH4_DIRECT_NETMOD
    int is_local = MPIDI_rank_is_local(target_rank, win->comm_ptr);
#endif

    if (HANDLE_IS_BUILTIN(target_datatype)) {
        am_hdr.n_iov = 0;
        MPIR_T_PVAR_TIMER_END(RMA, rma_amhdr_set);
        MPIDIG_REQUEST(sreq, req->greq.dt_iov) = NULL;

        MPIDIG_AM_SEND(is_local, target_rank, win->comm_ptr, MPIDIG_GET_REQ,
                       NULL, 0, MPI_DATATYPE_NULL, sreq);
        goto fn_exit;
    }

    MPIR_Typerep_iov_len(NULL, target_count, target_datatype, 0, data_sz, &num_iov);
    n_iov = (int) num_iov;
    MPIR_Assert(n_iov > 0);
    am_hdr.n_iov = n_iov;
    dt_iov = (struct iovec *) MPL_malloc(n_iov * sizeof(struct iovec), MPL_MEM_BUFFER);
    MPIR_Assert(dt_iov);

    int actual_iov_len;
    MPI_Aint actual_iov_bytes;
    MPIR_Typerep_to_iov(NULL, target_count, target_datatype, 0, dt_iov, n_iov, data_sz,
                        &actual_iov_len, &actual_iov_bytes);
    n_iov = actual_iov_len;

    MPIR_Assert(actual_iov_bytes == (MPI_Aint) data_sz);
    MPIR_T_PVAR_TIMER_END(RMA, rma_amhdr_set);

    MPIDIG_REQUEST(sreq, req->greq.dt_iov) = dt_iov;

    MPIDIG_AM_SEND(is_local, target_rank, win->comm_ptr, MPIDIG_GET_REQ,
                   dt_iov, sizeof(struct iovec) * am_hdr.n_iov, MPI_BYTE, sreq);

  fn_exit:
    if (sreq_ptr)
        *sreq_ptr = sreq;
    else if (sreq != NULL)
        MPIR_Request_free(sreq);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_DO_GET);
    return mpi_errno;

  immed_cmpl:
    if (sreq_ptr)
        MPIDI_RMA_REQUEST_CREATE_COMPLETE(sreq);
    goto fn_exit;

  fn_fail:
    goto fn_exit;
}

static int MPIDIG_get_origin_cb(MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Request_complete(sreq);
    return mpi_errno;
}

static int MPIDIG_get_target_msg_cb(int handler_id, void *am_hdr, void **data, size_t * p_data_sz,
                                    int is_local, int *is_contig,
                                    MPIDIG_am_target_cmpl_cb * target_cmpl_cb, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *rreq = NULL;
    struct am_get_hdr *hdr = am_hdr;
    struct iovec *iov;
    MPIR_Win *win;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_GET_TARGET_MSG_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_GET_TARGET_MSG_CB);
    MPIR_T_PVAR_TIMER_START(RMA, rma_targetcb_get);

    rreq = MPIDIG_request_create(MPIR_REQUEST_KIND__RMA, 1);
    MPIR_ERR_CHKANDSTMT(rreq == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");

    *req = rreq;
    *target_cmpl_cb = do_get_ack;
#ifndef MPIDI_CH4_DIRECT_NETMOD
    MPIDI_REQUEST(rreq, is_local) = is_local;
#endif

    win = (MPIR_Win *) MPIDIU_map_lookup(MPIDI_global.win_map, hdr->win_id);
    MPIR_Assert(win);

    uintptr_t base = MPIDIG_win_base_at_target(win);
    size_t offset = win->disp_unit * hdr->target_disp;

    MPIDIG_REQUEST(rreq, req->greq.win_ptr) = win;
    MPIDIG_REQUEST(rreq, req->greq.n_iov) = hdr->n_iov;
    MPIDIG_REQUEST(rreq, req->greq.addr) = (char *) base + offset;
    MPIDIG_REQUEST(rreq, req->greq.count) = hdr->count;
    MPIDIG_REQUEST(rreq, req->greq.datatype) = hdr->datatype;
    MPIDIG_REQUEST(rreq, req->greq.dt_iov) = NULL;
    MPIDIG_REQUEST(rreq, req->greq.greq_ptr) = hdr->greq_ptr;
    MPIDIG_REQUEST(rreq, rank) = hdr->src_rank;

    if (hdr->n_iov) {
        iov = (struct iovec *) MPL_malloc(hdr->n_iov * sizeof(*iov), MPL_MEM_RMA);
        MPIR_Assert(iov);

        *data = (void *) iov;
        *is_contig = 1;
        *p_data_sz = hdr->n_iov * sizeof(*iov);
        MPIDIG_REQUEST(rreq, req->greq.dt_iov) = iov;
    }

  fn_exit:
    MPIR_T_PVAR_TIMER_END(RMA, rma_targetcb_get);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_GET_TARGET_MSG_CB);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* MPIDIG_GET_ACK */
struct am_get_ack_hdr {
    MPIR_Request *greq_ptr;
};

static int do_get_ack(MPIR_Request * req)
{
    int mpi_errno = MPI_SUCCESS, i, c;
    size_t data_sz, offset;
    struct iovec *iov;
    char *p_data;
    MPIR_Win *win;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_GET_TARGET_CMPL_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_GET_TARGET_CMPL_CB);

    uintptr_t base = (uintptr_t) MPIDIG_REQUEST(req, req->greq.addr);

    MPIR_cc_incr(req->cc_ptr, &c);

    struct am_get_ack_hdr am_hdr;
    am_hdr.greq_ptr = MPIDIG_REQUEST(req, req->greq.greq_ptr);
    win = MPIDIG_REQUEST(req, req->greq.win_ptr);

#ifndef MPIDI_CH4_DIRECT_NETMOD
    int is_local = MPIDI_REQUEST(req, is_local);
#endif
    int rank = MPIDIG_REQUEST(req, rank);
    if (MPIDIG_REQUEST(req, req->greq.n_iov) == 0) {
        MPIDIG_AM_SEND(is_local, rank, win->comm_ptr, MPIDIG_GET_ACK,
                       MPIDIG_REQUEST(req, req->greq.addr), MPIDIG_REQUEST(req, req->greq.count),
                       MPIDIG_REQUEST(req, req->greq.datatype), req);

        MPID_Request_complete(req);
        goto fn_exit;
    }

    iov = (struct iovec *) MPIDIG_REQUEST(req, req->greq.dt_iov);

    data_sz = 0;
    for (i = 0; i < MPIDIG_REQUEST(req, req->greq.n_iov); i++) {
        data_sz += iov[i].iov_len;
    }

    p_data = (char *) MPL_malloc(data_sz, MPL_MEM_RMA);
    MPIR_Assert(p_data);

    offset = 0;
    for (i = 0; i < MPIDIG_REQUEST(req, req->greq.n_iov); i++) {
        /* Adjust a window base address */
        iov[i].iov_base = (char *) iov[i].iov_base + base;
        MPIR_Memcpy(p_data + offset, iov[i].iov_base, iov[i].iov_len);
        offset += iov[i].iov_len;
    }

    MPL_free(MPIDIG_REQUEST(req, req->greq.dt_iov));
    MPIDIG_REQUEST(req, req->greq.dt_iov) = (void *) p_data;

    MPIDIG_AM_SEND(is_local, rank, win->comm_ptr, MPIDIG_GET_ACK, p_data, data_sz, MPI_BYTE, req);
    MPID_Request_complete(req);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_GET_TARGET_CMPL_CB);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int MPIDIG_get_ack_target_msg_cb(int handler_id, void *am_hdr,
                                        void **data, size_t * p_data_sz,
                                        int is_local, int *is_contig,
                                        MPIDIG_am_target_cmpl_cb * target_cmpl_cb,
                                        MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *get_req;
    size_t data_sz;

    int dt_contig, n_iov;
    MPI_Aint dt_true_lb, num_iov;
    MPIR_Datatype *dt_ptr;

    struct am_get_ack_hdr *hdr = am_hdr;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_GET_ACK_TARGET_MSG_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_GET_ACK_TARGET_MSG_CB);
    MPIR_T_PVAR_TIMER_START(RMA, rma_targetcb_get_ack);

    get_req = (MPIR_Request *) hdr->greq_ptr;
    MPIR_Assert(get_req->kind == MPIR_REQUEST_KIND__RMA);
    *req = get_req;

    MPL_free(MPIDIG_REQUEST(get_req, req->greq.dt_iov));

    *target_cmpl_cb = get_ack_target_cmpl_cb;
#ifndef MPIDI_CH4_DIRECT_NETMOD
    MPIDI_REQUEST(get_req, is_local) = is_local;
#endif

    MPIDI_Datatype_get_info(MPIDIG_REQUEST(get_req, req->greq.count),
                            MPIDIG_REQUEST(get_req, req->greq.datatype),
                            dt_contig, data_sz, dt_ptr, dt_true_lb);

    *is_contig = dt_contig;

    if (dt_contig) {
        *p_data_sz = data_sz;
        *data = (char *) MPIDIG_REQUEST(get_req, req->greq.addr) + dt_true_lb;
    } else {
        MPIR_Typerep_iov_len((void *) MPIDIG_REQUEST(get_req, req->greq.addr),
                             MPIDIG_REQUEST(get_req, req->greq.count),
                             MPIDIG_REQUEST(get_req, req->greq.datatype), 0, data_sz, &num_iov);
        n_iov = (int) num_iov;
        MPIR_Assert(n_iov > 0);
        MPIDIG_REQUEST(get_req, req->iov) =
            (struct iovec *) MPL_malloc(n_iov * sizeof(struct iovec), MPL_MEM_RMA);
        MPIR_Assert(MPIDIG_REQUEST(get_req, req->iov));

        int actual_iov_len;
        MPI_Aint actual_iov_bytes;
        MPIR_Typerep_to_iov((void *) MPIDIG_REQUEST(get_req, req->greq.addr),
                            MPIDIG_REQUEST(get_req, req->greq.count),
                            MPIDIG_REQUEST(get_req, req->greq.datatype),
                            0, MPIDIG_REQUEST(get_req, req->iov), n_iov, data_sz,
                            &actual_iov_len, &actual_iov_bytes);
        n_iov = actual_iov_len;

        MPIR_Assert(actual_iov_bytes == (MPI_Aint) data_sz);
        *data = MPIDIG_REQUEST(get_req, req->iov);
        *p_data_sz = n_iov;
        MPIDIG_REQUEST(get_req, req->status) |= MPIDIG_REQ_RCV_NON_CONTIG;
    }

    MPIR_T_PVAR_TIMER_END(RMA, rma_targetcb_get_ack);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_GET_ACK_TARGET_MSG_CB);
    return mpi_errno;
}

static int get_ack_target_cmpl_cb(MPIR_Request * get_req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Win *win;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_GET_ACK_TARGET_CMPL_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_GET_ACK_TARGET_CMPL_CB);

    if (MPIDIG_REQUEST(get_req, req->status) & MPIDIG_REQ_RCV_NON_CONTIG) {
        MPL_free(MPIDIG_REQUEST(get_req, req->iov));
    }

    win = MPIDIG_REQUEST(get_req, req->greq.win_ptr);
    MPIDIG_win_remote_cmpl_cnt_decr(win, MPIDIG_REQUEST(get_req, rank));

    MPIR_Datatype_release_if_not_builtin(MPIDIG_REQUEST(get_req, req->greq.datatype));
    MPID_Request_complete(get_req);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_GET_ACK_TARGET_CMPL_CB);
    return mpi_errno;
}

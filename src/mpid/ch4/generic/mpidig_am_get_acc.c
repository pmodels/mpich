#include "mpidimpl.h"

static int MPIDIG_get_acc_origin_cb(MPIR_Request * sreq);
static int MPIDIG_get_acc_target_msg_cb(int handler_id, void *am_hdr,
                                        void **data, size_t * p_data_sz,
                                        int is_local, int *is_contig,
                                        MPIDIG_am_target_cmpl_cb * target_cmpl_cb,
                                        MPIR_Request ** req);
static int get_acc_target_cmpl_cb(MPIR_Request * rreq);
static int ack_get_acc(MPIR_Request * rreq);
static int MPIDIG_get_acc_ack_target_msg_cb(int handler_id, void *am_hdr,
                                            void **data, size_t * p_data_sz,
                                            int is_local, int *is_contig,
                                            MPIDIG_am_target_cmpl_cb * target_cmpl_cb,
                                            MPIR_Request ** req);
static int get_acc_ack_target_cmpl_cb(MPIR_Request * areq);
static int MPIDIG_get_acc_iov_origin_cb(MPIR_Request * sreq);
static int MPIDIG_get_acc_iov_target_msg_cb(int handler_id, void *am_hdr,
                                            void **data, size_t * p_data_sz,
                                            int is_local, int *is_contig,
                                            MPIDIG_am_target_cmpl_cb * target_cmpl_cb,
                                            MPIR_Request ** req);
static int do_get_acc_iov_ack(MPIR_Request * rreq);
static int MPIDIG_get_acc_iov_ack_target_msg_cb(int handler_id, void *am_hdr,
                                                void **data, size_t * p_data_sz,
                                                int is_local, int *is_contig,
                                                MPIDIG_am_target_cmpl_cb * target_cmpl_cb,
                                                MPIR_Request ** req);
static int do_get_acc_data(MPIR_Request * target_req, MPIR_Request * origin_req,
                           MPIR_Request * rreq, int is_local);
static int MPIDIG_get_acc_data_origin_cb(MPIR_Request * sreq);
static int MPIDIG_get_acc_data_target_msg_cb(int handler_id, void *am_hdr,
                                             void **data, size_t * p_data_sz,
                                             int is_local, int *is_contig,
                                             MPIDIG_am_target_cmpl_cb * target_cmpl_cb,
                                             MPIR_Request ** req);

MPIR_T_pvar_timer_t PVAR_TIMER_rma_targetcb_get_acc ATTRIBUTE((unused));
MPIR_T_pvar_timer_t PVAR_TIMER_rma_targetcb_get_acc_ack ATTRIBUTE((unused));
MPIR_T_pvar_timer_t PVAR_TIMER_rma_targetcb_get_acc_iov ATTRIBUTE((unused));
MPIR_T_pvar_timer_t PVAR_TIMER_rma_targetcb_get_acc_iov_ack ATTRIBUTE((unused));
MPIR_T_pvar_timer_t PVAR_TIMER_rma_targetcb_get_acc_data ATTRIBUTE((unused));

int MPIDIG_am_get_acc_init(void)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIDIG_am_reg_cb(MPIDIG_GET_ACC_REQ,
                                 &MPIDIG_get_acc_origin_cb, &MPIDIG_get_acc_target_msg_cb);
    MPIR_ERR_CHECK(mpi_errno);
    mpi_errno = MPIDIG_am_reg_cb(MPIDIG_GET_ACC_ACK, NULL, &MPIDIG_get_acc_ack_target_msg_cb);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIDIG_am_reg_cb(MPIDIG_GET_ACC_IOV_REQ,
                                 &MPIDIG_get_acc_iov_origin_cb, &MPIDIG_get_acc_iov_target_msg_cb);
    MPIR_ERR_CHECK(mpi_errno);
    mpi_errno = MPIDIG_am_reg_cb(MPIDIG_GET_ACC_IOV_ACK,
                                 NULL, &MPIDIG_get_acc_iov_ack_target_msg_cb);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIDIG_am_reg_cb(MPIDIG_GET_ACC_DAT_REQ,
                                 &MPIDIG_get_acc_data_origin_cb,
                                 &MPIDIG_get_acc_data_target_msg_cb);
    MPIR_ERR_CHECK(mpi_errno);

    /* rma_targetcb_get_acc */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(RMA,
                                      MPI_DOUBLE,
                                      rma_targetcb_get_acc,
                                      MPI_T_VERBOSITY_MPIDEV_DETAIL,
                                      MPI_T_BIND_NO_OBJECT,
                                      MPIR_T_PVAR_FLAG_READONLY,
                                      "RMA", "RMA:TARGETCB for Get-Accumulate (in seconds)");

    /* rma_targetcb_get_acc_ack */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(RMA,
                                      MPI_DOUBLE,
                                      rma_targetcb_get_acc_ack,
                                      MPI_T_VERBOSITY_MPIDEV_DETAIL,
                                      MPI_T_BIND_NO_OBJECT,
                                      MPIR_T_PVAR_FLAG_READONLY,
                                      "RMA", "RMA:TARGETCB for Get-Accumulate ACK (in seconds)");

    /* rma_targetcb_get_acc_iov */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(RMA,
                                      MPI_DOUBLE,
                                      rma_targetcb_get_acc_iov,
                                      MPI_T_VERBOSITY_MPIDEV_DETAIL,
                                      MPI_T_BIND_NO_OBJECT,
                                      MPIR_T_PVAR_FLAG_READONLY,
                                      "RMA", "RMA:TARGETCB for GET ACC IOV (in seconds)");

    /* rma_targetcb_get_acc_iov_ack */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(RMA,
                                      MPI_DOUBLE,
                                      rma_targetcb_get_acc_iov_ack,
                                      MPI_T_VERBOSITY_MPIDEV_DETAIL,
                                      MPI_T_BIND_NO_OBJECT,
                                      MPIR_T_PVAR_FLAG_READONLY,
                                      "RMA", "RMA:TARGETCB for GET ACC IOV ACK (in seconds)");

    /* rma_targetcb_get_acc_data */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(RMA,
                                      MPI_DOUBLE,
                                      rma_targetcb_get_acc_data,
                                      MPI_T_VERBOSITY_MPIDEV_DETAIL,
                                      MPI_T_BIND_NO_OBJECT,
                                      MPIR_T_PVAR_FLAG_READONLY,
                                      "RMA", "RMA:TARGETCB for GET ACC DATA (in seconds)");

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* MPIDIG_GET_ACC_REQ */
struct am_get_acc_req_hdr {
    int src_rank;
    uint64_t win_id;
    MPIR_Request *req_ptr;
    int origin_count;
    MPI_Datatype origin_datatype;
    int target_count;
    MPI_Datatype target_datatype;
    MPI_Op op;
    MPI_Aint target_disp;
    uint64_t result_data_sz;
    int n_iov;
};

int MPIDIG_do_get_accumulate(const void *origin_addr, int origin_count,
                             MPI_Datatype origin_datatype, void *result_addr, int result_count,
                             MPI_Datatype result_datatype, int target_rank, MPI_Aint target_disp,
                             int target_count, MPI_Datatype target_datatype,
                             MPI_Op op, MPIR_Win * win, MPIR_Request ** sreq_ptr)
{
    int mpi_errno = MPI_SUCCESS, c, n_iov;
    MPIR_Request *sreq = NULL;
    size_t basic_type_size;
    uint64_t data_sz, result_data_sz, target_data_sz;
    MPI_Aint num_iov;
    struct iovec *dt_iov, am_iov[2];
    MPIR_Datatype *dt_ptr;
    int am_hdr_max_sz;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_DO_GET_ACCUMULATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_DO_GET_ACCUMULATE);

    MPIDIG_RMA_OP_CHECK_SYNC(target_rank, win);

    MPIDI_Datatype_get_size_dt_ptr(origin_count, origin_datatype, data_sz, dt_ptr);
    MPIDI_Datatype_check_size(target_datatype, target_count, target_data_sz);
    MPIDI_Datatype_check_size(result_datatype, result_count, result_data_sz);

    if (target_data_sz == 0 || (data_sz == 0 && result_data_sz == 0)) {
        goto immed_cmpl;
    }

    /* Only create request when issuing is not completed.
     * We initialize two ref_count for progress engine and request-based OP,
     * then get_acc needs to free the second ref_count.*/
    sreq = MPIDIG_request_create(MPIR_REQUEST_KIND__RMA, 2);
    MPIR_ERR_CHKANDSTMT(sreq == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");

    MPIDIG_REQUEST(sreq, req->areq.win_ptr) = win;
    MPIDIG_REQUEST(sreq, req->areq.result_addr) = result_addr;
    MPIDIG_REQUEST(sreq, req->areq.result_count) = result_count;
    MPIDIG_REQUEST(sreq, req->areq.result_datatype) = result_datatype;
    MPIR_Datatype_add_ref_if_not_builtin(result_datatype);
    MPIR_cc_incr(sreq->cc_ptr, &c);

    /* TODO: have common routine for accumulate/get_accumulate */
    MPIR_T_PVAR_TIMER_START(RMA, rma_amhdr_set);

    struct am_get_acc_req_hdr am_hdr;
    am_hdr.req_ptr = sreq;
    am_hdr.origin_count = origin_count;

    if (HANDLE_IS_BUILTIN(origin_datatype)) {
        am_hdr.origin_datatype = origin_datatype;
    } else {
        am_hdr.origin_datatype = (dt_ptr) ? dt_ptr->basic_type : MPI_DATATYPE_NULL;
        MPIR_Datatype_get_size_macro(am_hdr.origin_datatype, basic_type_size);
        am_hdr.origin_count = (basic_type_size > 0) ? data_sz / basic_type_size : 0;
    }

    am_hdr.target_count = target_count;
    am_hdr.target_datatype = target_datatype;
    am_hdr.target_disp = target_disp;
    am_hdr.op = op;
    am_hdr.win_id = MPIDIG_WIN(win, win_id);
    am_hdr.src_rank = win->comm_ptr->rank;

    am_hdr.result_data_sz = result_data_sz;

    /* Increase local and remote completion counters and set the local completion
     * counter in request, thus it can be decreased at request completion. */
    MPIDIG_win_cmpl_cnts_incr(win, target_rank, &sreq->completion_notification);
    /* Increase remote completion counter for acc. */
    MPIDIG_win_remote_acc_cmpl_cnt_incr(win, target_rank);

#ifndef MPIDI_CH4_DIRECT_NETMOD
    int is_local = MPIDI_rank_is_local(target_rank, win->comm_ptr);
#endif
    MPIDIG_REQUEST(sreq, rank) = target_rank;
    MPIDIG_REQUEST(sreq, req->areq.data_sz) = data_sz;
    if (HANDLE_IS_BUILTIN(target_datatype)) {
        am_hdr.n_iov = 0;
        MPIR_T_PVAR_TIMER_END(RMA, rma_amhdr_set);
        MPIDIG_REQUEST(sreq, req->areq.dt_iov) = NULL;

        MPIDIG_AM_SEND(is_local, target_rank, win->comm_ptr, MPIDIG_GET_ACC_REQ,
                       origin_addr, (op == MPI_NO_OP) ? 0 : origin_count, origin_datatype, sreq);

        goto fn_exit;
    }

    MPIDI_Datatype_get_size_dt_ptr(target_count, target_datatype, data_sz, dt_ptr);
    am_hdr.target_datatype = dt_ptr->basic_type;
    am_hdr.target_count = dt_ptr->n_builtin_elements;

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

    am_iov[0].iov_base = &am_hdr;
    am_iov[0].iov_len = sizeof(am_hdr);
    am_iov[1].iov_base = dt_iov;
    am_iov[1].iov_len = sizeof(struct iovec) * am_hdr.n_iov;
    MPIR_T_PVAR_TIMER_END(RMA, rma_amhdr_set);
    MPIDIG_REQUEST(sreq, req->areq.dt_iov) = dt_iov;

#ifndef MPIDI_CH4_DIRECT_NETMOD
    am_hdr_max_sz = is_local ? MPIDI_SHM_am_hdr_max_sz() : MPIDI_NM_am_hdr_max_sz();
#else
    am_hdr_max_sz = MPIDI_NM_am_hdr_max_sz();
#endif

    if ((am_iov[0].iov_len + am_iov[1].iov_len) <= am_hdr_max_sz) {
        MPIDIG_AM_SENDV(is_local, target_rank, win->comm_ptr, MPIDIG_GET_ACC_REQ,
                        am_iov, 2, origin_addr,
                        (op == MPI_NO_OP) ? 0 : origin_count, origin_datatype, sreq);

    } else {
        MPIDIG_REQUEST(sreq, req->areq.origin_addr) = (void *) origin_addr;
        MPIDIG_REQUEST(sreq, req->areq.origin_count) = origin_count;
        MPIDIG_REQUEST(sreq, req->areq.origin_datatype) = origin_datatype;
        MPIR_Datatype_add_ref_if_not_builtin(origin_datatype);

        MPIDIG_AM_SEND(is_local, target_rank, win->comm_ptr, MPIDIG_GET_ACC_IOV_REQ,
                       am_iov[1].iov_base, am_iov[1].iov_len, MPI_BYTE, sreq);
    }

  fn_exit:
    if (sreq_ptr)
        *sreq_ptr = sreq;
    else if (sreq != NULL)
        MPIR_Request_free(sreq);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_DO_GET_ACCUMULATE);
    return mpi_errno;

  immed_cmpl:
    if (sreq_ptr)
        MPIDI_RMA_REQUEST_CREATE_COMPLETE(sreq);
    goto fn_exit;

  fn_fail:
    goto fn_exit;
}

static int MPIDIG_get_acc_origin_cb(MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Request_complete(sreq);
    return mpi_errno;
}

int MPIDIG_acc_target_msg_cb(int handler_id, void *am_hdr, void **data, size_t * p_data_sz,
                             int is_local, int *is_contig,
                             MPIDIG_am_target_cmpl_cb * target_cmpl_cb, MPIR_Request ** req);
static int MPIDIG_get_acc_target_msg_cb(int handler_id, void *am_hdr,
                                        void **data, size_t * p_data_sz,
                                        int is_local, int *is_contig,
                                        MPIDIG_am_target_cmpl_cb * target_cmpl_cb,
                                        MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_GET_ACC_TARGET_MSG_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_GET_ACC_TARGET_MSG_CB);
    MPIR_T_PVAR_TIMER_START(RMA, rma_targetcb_get_acc);

    /* the same handling processing as ACC except the completion handler function. */
    mpi_errno = MPIDIG_acc_target_msg_cb(handler_id, am_hdr, data, p_data_sz, is_local, is_contig,
                                         target_cmpl_cb, req);

    *target_cmpl_cb = get_acc_target_cmpl_cb;

    MPIR_T_PVAR_TIMER_END(RMA, rma_targetcb_get_acc);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_GET_ACC_TARGET_MSG_CB);
    return mpi_errno;
}

static int handle_get_acc_cmpl(MPIR_Request * rreq);
static int get_acc_target_cmpl_cb(MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_GET_ACC_TARGET_CMPL_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_GET_ACC_TARGET_CMPL_CB);

    if (!MPIDIG_check_cmpl_order(rreq, get_acc_target_cmpl_cb))
        return mpi_errno;

    mpi_errno = handle_get_acc_cmpl(rreq);
    MPIR_ERR_CHECK(mpi_errno);

    MPIDIG_progress_compl_list();
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_GET_ACC_TARGET_CMPL_CB);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* MPIDIG_GET_ACC_ACK */
struct am_get_acc_ack_hdr {
    MPIR_Request *req_ptr;
};

static int ack_get_acc(MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS, c;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_ACK_GET_ACC);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_ACK_GET_ACC);

    MPIR_cc_incr(rreq->cc_ptr, &c);

    struct am_get_acc_ack_hdr am_hdr;
    am_hdr.req_ptr = MPIDIG_REQUEST(rreq, req->areq.req_ptr);

    int rank = MPIDIG_REQUEST(rreq, rank);
    MPIR_Comm *comm = MPIDIG_REQUEST(rreq, req->areq.win_ptr)->comm_ptr;
    MPIDIG_AM_SEND(MPIDI_REQUEST(rreq, is_local), rank, comm, MPIDIG_GET_ACC_ACK,
                   MPIDIG_REQUEST(rreq, req->areq.data),
                   MPIDIG_REQUEST(rreq, req->areq.data_sz), MPI_BYTE, rreq);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_ACK_GET_ACC);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int MPIDIG_get_acc_ack_target_msg_cb(int handler_id, void *am_hdr,
                                            void **data, size_t * p_data_sz,
                                            int is_local, int *is_contig,
                                            MPIDIG_am_target_cmpl_cb * target_cmpl_cb,
                                            MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *areq;

    size_t data_sz;
    int dt_contig, n_iov;
    MPI_Aint dt_true_lb, num_iov;
    MPIR_Datatype *dt_ptr;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_GET_ACC_ACK_TARGET_MSG_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_GET_ACC_ACK_TARGET_MSG_CB);
    MPIR_T_PVAR_TIMER_START(RMA, rma_targetcb_get_acc_ack);

    struct am_get_acc_ack_hdr *hdr = am_hdr;
    areq = (MPIR_Request *) hdr->req_ptr;

    MPL_free(MPIDIG_REQUEST(areq, req->areq.dt_iov));

    MPIDI_Datatype_get_info(MPIDIG_REQUEST(areq, req->areq.result_count),
                            MPIDIG_REQUEST(areq, req->areq.result_datatype),
                            dt_contig, data_sz, dt_ptr, dt_true_lb);
    *is_contig = dt_contig;

    if (dt_contig) {
        *p_data_sz = data_sz;
        *data = (char *) MPIDIG_REQUEST(areq, req->areq.result_addr) + dt_true_lb;
    } else {
        MPIR_Typerep_iov_len(MPIDIG_REQUEST(areq, req->areq.result_addr),
                             MPIDIG_REQUEST(areq, req->areq.result_count),
                             MPIDIG_REQUEST(areq, req->areq.result_datatype), 0, data_sz, &num_iov);
        n_iov = (int) num_iov;
        MPIR_Assert(n_iov > 0);
        MPIDIG_REQUEST(areq, req->iov) =
            (struct iovec *) MPL_malloc(n_iov * sizeof(struct iovec), MPL_MEM_RMA);
        MPIR_Assert(MPIDIG_REQUEST(areq, req->iov));

        int actual_iov_len;
        MPI_Aint actual_iov_bytes;
        MPIR_Typerep_to_iov(MPIDIG_REQUEST(areq, req->areq.result_addr),
                            MPIDIG_REQUEST(areq, req->areq.result_count),
                            MPIDIG_REQUEST(areq, req->areq.result_datatype),
                            0, MPIDIG_REQUEST(areq, req->iov), n_iov, data_sz,
                            &actual_iov_len, &actual_iov_bytes);
        n_iov = actual_iov_len;

        MPIR_Assert(actual_iov_bytes == (MPI_Aint) data_sz);
        *data = MPIDIG_REQUEST(areq, req->iov);
        *p_data_sz = n_iov;
        MPIDIG_REQUEST(areq, req->status) |= MPIDIG_REQ_RCV_NON_CONTIG;
    }

    *req = areq;
    *target_cmpl_cb = get_acc_ack_target_cmpl_cb;

    MPIR_T_PVAR_TIMER_END(RMA, rma_targetcb_get_acc_ack);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_GET_ACC_ACK_TARGET_MSG_CB);
    return mpi_errno;
}

static int get_acc_ack_target_cmpl_cb(MPIR_Request * areq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Win *win;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_GET_ACC_ACK_TARGET_CMPL_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_GET_ACC_ACK_TARGET_CMPL_CB);

    if (MPIDIG_REQUEST(areq, req->status) & MPIDIG_REQ_RCV_NON_CONTIG) {
        MPL_free(MPIDIG_REQUEST(areq, req->iov));
    }

    win = MPIDIG_REQUEST(areq, req->areq.win_ptr);
    MPIDIG_win_remote_cmpl_cnt_decr(win, MPIDIG_REQUEST(areq, rank));
    MPIDIG_win_remote_acc_cmpl_cnt_decr(win, MPIDIG_REQUEST(areq, rank));

    MPIR_Datatype_release_if_not_builtin(MPIDIG_REQUEST(areq, req->areq.result_datatype));
    MPID_Request_complete(areq);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_GET_ACC_ACK_TARGET_CMPL_CB);
    return mpi_errno;
}

/* MPIDIG_GET_ACC_IOV_REQ */
static int MPIDIG_get_acc_iov_origin_cb(MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_GET_ACC_IOV_ORIGIN_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_GET_ACC_IOV_ORIGIN_CB);
    MPID_Request_complete(sreq);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_GET_ACC_IOV_ORIGIN_CB);
    return mpi_errno;
}

int MPIDIG_acc_iov_target_msg_cb(int handler_id, void *am_hdr,
                                 void **data, size_t * p_data_sz,
                                 int is_local, int *is_contig,
                                 MPIDIG_am_target_cmpl_cb * target_cmpl_cb, MPIR_Request ** req);
static int MPIDIG_get_acc_iov_target_msg_cb(int handler_id, void *am_hdr,
                                            void **data, size_t * p_data_sz,
                                            int is_local, int *is_contig,
                                            MPIDIG_am_target_cmpl_cb * target_cmpl_cb,
                                            MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_GET_ACC_IOV_TARGET_MSG_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_GET_ACC_IOV_TARGET_MSG_CB);
    MPIR_T_PVAR_TIMER_START(RMA, rma_targetcb_get_acc_iov);

    /* the same handling processing as ACC except the completion handler function. */
    mpi_errno = MPIDIG_acc_iov_target_msg_cb(handler_id, am_hdr, data,
                                             p_data_sz, is_local, is_contig, target_cmpl_cb, req);

    *target_cmpl_cb = do_get_acc_iov_ack;

    MPIR_T_PVAR_TIMER_END(RMA, rma_targetcb_get_acc_iov);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_GET_ACC_IOV_TARGET_MSG_CB);
    return mpi_errno;
}

/* MPIDIG_GET_ACC_IOV_ACK */
struct am_get_acc_iov_ack_hdr {
    int src_rank;
    MPIR_Request *target_preq_ptr;
    MPIR_Request *origin_preq_ptr;
};

static int do_get_acc_iov_ack(MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_GET_ACC_IOV_TARGET_CMPL_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_GET_ACC_IOV_TARGET_CMPL_CB);

    struct am_get_acc_iov_ack_hdr am_hdr;
    am_hdr.origin_preq_ptr = MPIDIG_REQUEST(rreq, req->areq.req_ptr);
    am_hdr.target_preq_ptr = rreq;

    int rank = MPIDIG_REQUEST(rreq, rank);
    MPIR_Comm *comm = MPIDIG_REQUEST(rreq, req->areq.win_ptr)->comm_ptr;
    MPIDIG_AM_SEND_HDR(MPIDI_REQUEST(rreq, is_local), rank, comm, MPIDIG_GET_ACC_IOV_ACK);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_GET_ACC_IOV_TARGET_CMPL_CB);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int MPIDIG_get_acc_iov_ack_target_msg_cb(int handler_id, void *am_hdr,
                                                void **data, size_t * p_data_sz,
                                                int is_local, int *is_contig,
                                                MPIDIG_am_target_cmpl_cb * target_cmpl_cb,
                                                MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_GET_ACC_IOV_ACK_TARGET_MSG_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_GET_ACC_IOV_ACK_TARGET_MSG_CB);
    MPIR_T_PVAR_TIMER_START(RMA, rma_targetcb_get_acc_iov_ack);

    MPIR_Request *rreq = MPIDIG_request_create(MPIR_REQUEST_KIND__RMA, 1);
    MPIR_ERR_CHKANDSTMT(rreq == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");

    struct am_get_acc_iov_ack_hdr *hdr = am_hdr;
    mpi_errno = do_get_acc_data(hdr->target_preq_ptr, hdr->origin_preq_ptr, rreq, is_local);

    *target_cmpl_cb = NULL;
    *req = NULL;

  fn_exit:
    MPIR_T_PVAR_TIMER_END(RMA, rma_targetcb_get_acc_iov_ack);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_GET_ACC_IOV_ACK_TARGET_MSG_CB);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


/* MPIDIG_GET_ACC_DAT_REQ */
struct am_get_acc_dat_hdr {
    MPIR_Request *preq_ptr;
};

static int do_get_acc_data(MPIR_Request * target_req, MPIR_Request * origin_req,
                           MPIR_Request * rreq, int is_local)
{
    int mpi_errno;

    struct am_get_acc_dat_hdr am_hdr;
    am_hdr.preq_ptr = target_req;

    MPIR_Win *win = MPIDIG_REQUEST(origin_req, req->areq.win_ptr);
    int rank = MPIDIG_REQUEST(origin_req, rank);
    MPIDIG_AM_SEND(is_local, rank, win->comm_ptr, MPIDIG_GET_ACC_DAT_REQ,
                   MPIDIG_REQUEST(origin_req, req->areq.origin_addr),
                   MPIDIG_REQUEST(origin_req, req->areq.origin_count),
                   MPIDIG_REQUEST(origin_req, req->areq.origin_datatype), rreq);

    MPIR_Datatype_release_if_not_builtin(MPIDIG_REQUEST(origin_req, req->areq.origin_datatype));

    return mpi_errno;
}

static int MPIDIG_get_acc_data_origin_cb(MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_GET_ACC_DATA_ORIGIN_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_GET_ACC_DATA_ORIGIN_CB);
    MPID_Request_complete(sreq);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_GET_ACC_DATA_ORIGIN_CB);
    return mpi_errno;
}

static void handle_acc_data(void **data, size_t * p_data_sz, int *is_contig, MPIR_Request * rreq);
static int MPIDIG_get_acc_data_target_msg_cb(int handler_id, void *am_hdr,
                                             void **data, size_t * p_data_sz,
                                             int is_local, int *is_contig,
                                             MPIDIG_am_target_cmpl_cb * target_cmpl_cb,
                                             MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *rreq;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_GET_ACC_DATA_TARGET_MSG_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_GET_ACC_DATA_TARGET_MSG_CB);
    MPIR_T_PVAR_TIMER_START(RMA, rma_targetcb_get_acc_data);

    struct am_get_acc_dat_hdr *hdr = am_hdr;
    rreq = (MPIR_Request *) hdr->preq_ptr;
    handle_acc_data(data, p_data_sz, is_contig, rreq);

    *req = rreq;
    *target_cmpl_cb = get_acc_target_cmpl_cb;

    MPIR_T_PVAR_TIMER_END(RMA, rma_targetcb_get_acc_data);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_GET_ACC_DATA_TARGET_MSG_CB);
    return mpi_errno;
}

/* Internal routines */
static int handle_get_acc_cmpl(MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS, i;
    MPI_Aint basic_sz, count, offset = 0;
    struct iovec *iov;
    char *src_ptr, *original = NULL;
    size_t data_sz;
    int shm_locked ATTRIBUTE((unused)) = 0;
    MPIR_Win *win ATTRIBUTE((unused)) = MPIDIG_REQUEST(rreq, req->areq.win_ptr);

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_HANDLE_GET_ACC_CMPL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_HANDLE_GET_ACC_CMPL);

    MPIR_Datatype_get_size_macro(MPIDIG_REQUEST(rreq, req->areq.target_datatype), basic_sz);
    MPIR_Assert(basic_sz);
    data_sz = MPIDIG_REQUEST(rreq, req->areq.data_sz);

    /* MPIDI_CS_ENTER(); */

    original = (char *) MPL_malloc(data_sz, MPL_MEM_RMA);
    MPIR_Assert(original);

    if (MPIDIG_REQUEST(rreq, req->areq.op) == MPI_NO_OP) {
        MPIDIG_REQUEST(rreq, req->areq.origin_count) = MPIDIG_REQUEST(rreq, req->areq.target_count);
        MPIDIG_REQUEST(rreq, req->areq.data_sz) = data_sz;
    }
#ifndef MPIDI_CH4_DIRECT_NETMOD
    if (MPIDIG_WIN(win, shm_allocated)) {
        mpi_errno = MPIDI_SHM_rma_op_cs_enter_hook(win);
        MPIR_ERR_CHECK(mpi_errno);
        shm_locked = 1;
    }
#endif

    if (MPIDIG_REQUEST(rreq, req->areq.dt_iov) == NULL) {

        MPIR_Memcpy(original, MPIDIG_REQUEST(rreq, req->areq.target_addr),
                    basic_sz * MPIDIG_REQUEST(rreq, req->areq.target_count));

        mpi_errno = MPIDIG_compute_acc_op(MPIDIG_REQUEST(rreq, req->areq.data),
                                          MPIDIG_REQUEST(rreq, req->areq.origin_count),
                                          MPIDIG_REQUEST(rreq, req->areq.origin_datatype),
                                          MPIDIG_REQUEST(rreq, req->areq.target_addr),
                                          MPIDIG_REQUEST(rreq, req->areq.target_count),
                                          MPIDIG_REQUEST(rreq, req->areq.target_datatype),
                                          MPIDIG_REQUEST(rreq, req->areq.op),
                                          MPIDIG_ACC_SRCBUF_DEFAULT);
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        iov = (struct iovec *) MPIDIG_REQUEST(rreq, req->areq.dt_iov);
        src_ptr = (char *) MPIDIG_REQUEST(rreq, req->areq.data);
        for (i = 0; i < MPIDIG_REQUEST(rreq, req->areq.n_iov); i++) {
            count = iov[i].iov_len / basic_sz;
            MPIR_Assert(count > 0);

            MPIR_Memcpy(original + offset, iov[i].iov_base, count * basic_sz);
            offset += count * basic_sz;

            mpi_errno = MPIDIG_compute_acc_op(src_ptr, count,
                                              MPIDIG_REQUEST(rreq, req->areq.origin_datatype),
                                              iov[i].iov_base, count,
                                              MPIDIG_REQUEST(rreq, req->areq.target_datatype),
                                              MPIDIG_REQUEST(rreq, req->areq.op),
                                              MPIDIG_ACC_SRCBUF_DEFAULT);
            MPIR_ERR_CHECK(mpi_errno);
            src_ptr += count * basic_sz;
        }
        MPL_free(iov);
    }

#ifndef MPIDI_CH4_DIRECT_NETMOD
    if (MPIDIG_WIN(win, shm_allocated)) {
        mpi_errno = MPIDI_SHM_rma_op_cs_exit_hook(win);
        MPIR_ERR_CHECK(mpi_errno);
    }
#endif

    /* MPIDI_CS_EXIT(); */
    MPL_free(MPIDIG_REQUEST(rreq, req->areq.data));

    MPIDIG_REQUEST(rreq, req->areq.data) = original;
    mpi_errno = ack_get_acc(rreq);
    MPIR_ERR_CHECK(mpi_errno);

    MPID_Request_complete(rreq);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_HANDLE_GET_ACC_CMPL);
    return mpi_errno;
  fn_fail:
#ifndef MPIDI_CH4_DIRECT_NETMOD
    /* release lock if error is reported inside CS. */
    if (shm_locked)
        MPIDI_SHM_rma_op_cs_exit_hook(win);
#endif
    goto fn_exit;
}

static void handle_acc_data(void **data, size_t * p_data_sz, int *is_contig, MPIR_Request * rreq)
{
    void *p_data = NULL;
    size_t data_sz;
    uintptr_t base;
    int i;
    struct iovec *iov;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_HANDLE_ACC_DATA);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_HANDLE_ACC_DATA);

    base = (uintptr_t) MPIDIG_REQUEST(rreq, req->areq.target_addr);
    MPIDI_Datatype_check_size(MPIDIG_REQUEST(rreq, req->areq.origin_datatype),
                              MPIDIG_REQUEST(rreq, req->areq.origin_count), data_sz);

    /* The origin_data can be NULL only with no-op.
     * TODO: when no-op is set, we do not need send origin_data at all. */
    if (data_sz) {
        p_data = MPL_malloc(data_sz, MPL_MEM_RMA);
        MPIR_Assert(p_data);
    } else {
        MPIR_Assert(MPIDIG_REQUEST(rreq, req->areq.op) == MPI_NO_OP);
    }

    MPIDIG_REQUEST(rreq, req->areq.data) = p_data;

    /* Adjust the target iov addresses using the base address
     * (window base + target_disp) */
    iov = (struct iovec *) MPIDIG_REQUEST(rreq, req->areq.dt_iov);
    for (i = 0; i < MPIDIG_REQUEST(rreq, req->areq.n_iov); i++)
        iov[i].iov_base = (char *) iov[i].iov_base + base;

    /* Progress engine may pass NULL here if received data sizeis zero */
    if (data)
        *data = p_data;
    if (is_contig)
        *is_contig = 1;
    if (p_data_sz)
        *p_data_sz = data_sz;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_HANDLE_ACC_DATA);
}

#include "mpidimpl.h"

static int MPIDIG_cswap_origin_cb(MPIR_Request * sreq);
static int MPIDIG_cswap_target_msg_cb(int handler_id, void *am_hdr,
                                      void **data, size_t * p_data_sz,
                                      int is_local, int *is_contig,
                                      MPIDIG_am_target_cmpl_cb * target_cmpl_cb,
                                      MPIR_Request ** req);
static int cswap_target_cmpl_cb(MPIR_Request * rreq);
static int ack_cswap(MPIR_Request * rreq);
static int MPIDIG_cswap_ack_target_msg_cb(int handler_id, void *am_hdr,
                                          void **data, size_t * p_data_sz,
                                          int is_local, int *is_contig,
                                          MPIDIG_am_target_cmpl_cb * target_cmpl_cb,
                                          MPIR_Request ** req);
static int cswap_ack_target_cmpl_cb(MPIR_Request * rreq);

MPIR_T_pvar_timer_t PVAR_TIMER_rma_targetcb_cas ATTRIBUTE((unused));
MPIR_T_pvar_timer_t PVAR_TIMER_rma_targetcb_cas_ack ATTRIBUTE((unused));

int MPIDIG_am_cswap_init(void)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIDIG_am_reg_cb(MPIDIG_CSWAP_REQ,
                                 &MPIDIG_cswap_origin_cb, &MPIDIG_cswap_target_msg_cb);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIDIG_am_reg_cb(MPIDIG_CSWAP_ACK, NULL, &MPIDIG_cswap_ack_target_msg_cb);
    MPIR_ERR_CHECK(mpi_errno);

    /* rma_targetcb_cas */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(RMA,
                                      MPI_DOUBLE,
                                      rma_targetcb_cas,
                                      MPI_T_VERBOSITY_MPIDEV_DETAIL,
                                      MPI_T_BIND_NO_OBJECT,
                                      MPIR_T_PVAR_FLAG_READONLY,
                                      "RMA", "RMA:TARGETCB for Compare-and-swap (in seconds)");

    /* rma_targetcb_cas_ack */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(RMA,
                                      MPI_DOUBLE,
                                      rma_targetcb_cas_ack,
                                      MPI_T_VERBOSITY_MPIDEV_DETAIL,
                                      MPI_T_BIND_NO_OBJECT,
                                      MPIR_T_PVAR_FLAG_READONLY,
                                      "RMA", "RMA:TARGETCB for Compare-and-swap ACK (in seconds)");

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* MPIDIG_CSWAP_REQ */
struct am_cswap_req_hdr {
    int src_rank;
    uint64_t win_id;
    MPIR_Request *req_ptr;
    MPI_Aint target_disp;
    MPI_Datatype datatype;
};

int MPIDIG_do_cswap(const void *origin_addr, const void *compare_addr,
                    void *result_addr, MPI_Datatype datatype,
                    int target_rank, MPI_Aint target_disp, MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS, c;
    MPIR_Request *sreq = NULL;
    size_t data_sz;
    void *p_data;

    MPIDIG_RMA_OP_CHECK_SYNC(target_rank, win);

    MPIDI_Datatype_check_size(datatype, 1, data_sz);
    if (data_sz == 0)
        goto fn_exit;

    p_data = MPL_malloc(data_sz * 2, MPL_MEM_BUFFER);
    MPIR_Assert(p_data);
    MPIR_Memcpy(p_data, (char *) origin_addr, data_sz);
    MPIR_Memcpy((char *) p_data + data_sz, (char *) compare_addr, data_sz);

    sreq = MPIDIG_request_create(MPIR_REQUEST_KIND__RMA, 1);
    MPIR_ERR_CHKANDSTMT(sreq == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");

    MPIDIG_REQUEST(sreq, req->creq.win_ptr) = win;
    MPIDIG_REQUEST(sreq, req->creq.addr) = result_addr;
    MPIDIG_REQUEST(sreq, req->creq.datatype) = datatype;
    MPIDIG_REQUEST(sreq, req->creq.result_addr) = result_addr;
    MPIDIG_REQUEST(sreq, req->creq.data) = p_data;
    MPIDIG_REQUEST(sreq, rank) = target_rank;
    MPIR_cc_incr(sreq->cc_ptr, &c);

    MPIR_T_PVAR_TIMER_START(RMA, rma_amhdr_set);
    struct am_cswap_req_hdr am_hdr;
    am_hdr.target_disp = target_disp;
    am_hdr.datatype = datatype;
    am_hdr.req_ptr = sreq;
    am_hdr.win_id = MPIDIG_WIN(win, win_id);
    am_hdr.src_rank = win->comm_ptr->rank;
    MPIR_T_PVAR_TIMER_END(RMA, rma_amhdr_set);

    MPIDIG_win_cmpl_cnts_incr(win, target_rank, &sreq->completion_notification);
    /* Increase remote completion counter for acc. */
    MPIDIG_win_remote_acc_cmpl_cnt_incr(win, target_rank);

    MPIDIG_AM_SEND(MPIDI_rank_is_local(target_rank, win->comm_ptr),
                   target_rank, win->comm_ptr, MPIDIG_CSWAP_REQ,
                   (void *) p_data, 2, datatype, sreq);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int MPIDIG_cswap_origin_cb(MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Request_complete(sreq);
    return mpi_errno;
}

static int MPIDIG_cswap_target_msg_cb(int handler_id, void *am_hdr,
                                      void **data, size_t * p_data_sz,
                                      int is_local, int *is_contig,
                                      MPIDIG_am_target_cmpl_cb * target_cmpl_cb,
                                      MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *rreq = NULL;
    size_t data_sz;
    MPIR_Win *win;

    int dt_contig;
    void *p_data;

    struct am_cswap_req_hdr *hdr = am_hdr;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_CSWAP_TARGET_MSG_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_CSWAP_TARGET_MSG_CB);
    MPIR_T_PVAR_TIMER_START(RMA, rma_targetcb_cas);

    rreq = MPIDIG_request_create(MPIR_REQUEST_KIND__RMA, 1);
    MPIR_ERR_CHKANDSTMT(rreq == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
    *req = rreq;

    *target_cmpl_cb = cswap_target_cmpl_cb;
    MPIDIG_REQUEST(rreq, req->seq_no) = MPL_atomic_fetch_add_uint64(&MPIDI_global.nxt_seq_no, 1);
#ifndef MPIDI_CH4_DIRECT_NETMOD
    MPIDI_REQUEST(rreq, is_local) = is_local;
#endif

    MPIDI_Datatype_check_contig_size(hdr->datatype, 1, dt_contig, data_sz);
    *is_contig = dt_contig;

    win = (MPIR_Win *) MPIDIU_map_lookup(MPIDI_global.win_map, hdr->win_id);
    MPIR_Assert(win);

    uintptr_t base = MPIDIG_win_base_at_target(win);
    size_t offset = win->disp_unit * hdr->target_disp;

    MPIDIG_REQUEST(*req, req->creq.win_ptr) = win;
    MPIDIG_REQUEST(*req, req->creq.creq_ptr) = hdr->req_ptr;
    MPIDIG_REQUEST(*req, req->creq.datatype) = hdr->datatype;
    MPIDIG_REQUEST(*req, req->creq.addr) = (char *) base + offset;
    MPIDIG_REQUEST(*req, rank) = hdr->src_rank;

    MPIR_Assert(dt_contig == 1);
    p_data = MPL_malloc(data_sz * 2, MPL_MEM_RMA);
    MPIR_Assert(p_data);

    *p_data_sz = data_sz * 2;
    *data = p_data;
    MPIDIG_REQUEST(*req, req->creq.data) = p_data;

  fn_exit:
    MPIR_T_PVAR_TIMER_END(RMA, rma_targetcb_cas);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_CSWAP_TARGET_MSG_CB);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int cswap_target_cmpl_cb(MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    void *compare_addr;
    void *origin_addr;
    size_t data_sz;
    MPIR_Win *win ATTRIBUTE((unused)) = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_CSWAP_TARGET_CMPL_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_CSWAP_TARGET_CMPL_CB);

    if (!MPIDIG_check_cmpl_order(rreq, cswap_target_cmpl_cb))
        return mpi_errno;

    MPIDI_Datatype_check_size(MPIDIG_REQUEST(rreq, req->creq.datatype), 1, data_sz);
    origin_addr = MPIDIG_REQUEST(rreq, req->creq.data);
    compare_addr = ((char *) MPIDIG_REQUEST(rreq, req->creq.data)) + data_sz;

    /* MPIDI_CS_ENTER(); */
#ifndef MPIDI_CH4_DIRECT_NETMOD
    win = MPIDIG_REQUEST(rreq, req->creq.win_ptr);
    if (MPIDIG_WIN(win, shm_allocated)) {
        mpi_errno = MPIDI_SHM_rma_op_cs_enter_hook(win);
        MPIR_ERR_CHECK(mpi_errno);
    }
#endif

    if (MPIR_Compare_equal((void *) MPIDIG_REQUEST(rreq, req->creq.addr), compare_addr,
                           MPIDIG_REQUEST(rreq, req->creq.datatype))) {
        MPIR_Memcpy(compare_addr, (void *) MPIDIG_REQUEST(rreq, req->creq.addr), data_sz);
        MPIR_Memcpy((void *) MPIDIG_REQUEST(rreq, req->creq.addr), origin_addr, data_sz);
    } else {
        MPIR_Memcpy(compare_addr, (void *) MPIDIG_REQUEST(rreq, req->creq.addr), data_sz);
    }

#ifndef MPIDI_CH4_DIRECT_NETMOD
    if (MPIDIG_WIN(win, shm_allocated)) {
        mpi_errno = MPIDI_SHM_rma_op_cs_exit_hook(win);
        MPIR_ERR_CHECK(mpi_errno);
    }
#endif
    /* MPIDI_CS_EXIT(); */

    mpi_errno = ack_cswap(rreq);
    MPIR_ERR_CHECK(mpi_errno);
    MPID_Request_complete(rreq);
    MPIDIG_progress_compl_list();
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_CSWAP_TARGET_CMPL_CB);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* MPIDIG_CSWAP_ACK */
struct am_cswap_ack_hdr {
    MPIR_Request *req_ptr;
};

static int ack_cswap(MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS, c;
    void *result_addr;
    size_t data_sz;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_ACK_CSWAP);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_ACK_CSWAP);

    MPIDI_Datatype_check_size(MPIDIG_REQUEST(rreq, req->creq.datatype), 1, data_sz);
    result_addr = ((char *) MPIDIG_REQUEST(rreq, req->creq.data)) + data_sz;

    MPIR_cc_incr(rreq->cc_ptr, &c);

    struct am_cswap_ack_hdr am_hdr;
    am_hdr.req_ptr = MPIDIG_REQUEST(rreq, req->creq.creq_ptr);

    int rank = MPIDIG_REQUEST(rreq, rank);
    MPIR_Comm *comm = MPIDIG_REQUEST(rreq, req->creq.win_ptr)->comm_ptr;
    MPI_Datatype datatype = MPIDIG_REQUEST(rreq, req->creq.datatype);
    MPIDIG_AM_SEND(MPIDI_REQUEST(rreq, is_local), rank, comm, MPIDIG_CSWAP_ACK,
                   result_addr, 1, datatype, rreq);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_ACK_CSWAP);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int MPIDIG_cswap_ack_target_msg_cb(int handler_id, void *am_hdr,
                                          void **data, size_t * p_data_sz,
                                          int is_local, int *is_contig,
                                          MPIDIG_am_target_cmpl_cb * target_cmpl_cb,
                                          MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    struct am_cswap_ack_hdr *hdr = am_hdr;
    MPIR_Request *creq;
    uint64_t data_sz;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_CSWAP_ACK_TARGET_MSG_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_CSWAP_ACK_TARGET_MSG_CB);
    MPIR_T_PVAR_TIMER_START(RMA, rma_targetcb_cas_ack);

    creq = (MPIR_Request *) hdr->req_ptr;
    MPIDI_Datatype_check_size(MPIDIG_REQUEST(creq, req->creq.datatype), 1, data_sz);
    *data = MPIDIG_REQUEST(creq, req->creq.result_addr);
    *p_data_sz = data_sz;
    *is_contig = 1;

    *req = creq;
    *target_cmpl_cb = cswap_ack_target_cmpl_cb;

    MPIR_T_PVAR_TIMER_END(RMA, rma_targetcb_cas_ack);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_CSWAP_ACK_TARGET_MSG_CB);
    return mpi_errno;
}

static int cswap_ack_target_cmpl_cb(MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Win *win;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_CSWAP_ACK_TARGET_CMPL_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_CSWAP_ACK_TARGET_CMPL_CB);

    win = MPIDIG_REQUEST(rreq, req->creq.win_ptr);
    MPIDIG_win_remote_cmpl_cnt_decr(win, MPIDIG_REQUEST(rreq, rank));
    MPIDIG_win_remote_acc_cmpl_cnt_decr(win, MPIDIG_REQUEST(rreq, rank));

    MPL_free(MPIDIG_REQUEST(rreq, req->creq.data));
    MPID_Request_complete(rreq);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_CSWAP_ACK_TARGET_CMPL_CB);
    return mpi_errno;
}

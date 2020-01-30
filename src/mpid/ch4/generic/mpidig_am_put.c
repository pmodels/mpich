#include "mpidimpl.h"

static int MPIDIG_put_origin_cb(MPIR_Request * sreq);
static int MPIDIG_put_target_msg_cb(int handler_id, void *am_hdr, void **data, size_t * p_data_sz,
                                    int is_local, int *is_contig,
                                    MPIDIG_am_target_cmpl_cb * target_cmpl_cb, MPIR_Request ** req);
static int put_target_cmpl_cb(MPIR_Request * rreq);
static int ack_put(MPIR_Request * rreq);
static int MPIDIG_put_ack_target_msg_cb(int handler_id, void *am_hdr,
                                        void **data, size_t * p_data_sz,
                                        int is_local, int *is_contig,
                                        MPIDIG_am_target_cmpl_cb * target_cmpl_cb,
                                        MPIR_Request ** req);
static int MPIDIG_put_iov_origin_cb(MPIR_Request * sreq);
static int MPIDIG_put_iov_target_msg_cb(int handler_id, void *am_hdr,
                                        void **data, size_t * p_data_sz,
                                        int is_local, int *is_contig,
                                        MPIDIG_am_target_cmpl_cb * target_cmpl_cb,
                                        MPIR_Request ** req);
static int do_put_iov_ack(MPIR_Request * rreq);
static int MPIDIG_put_iov_ack_target_msg_cb(int handler_id, void *am_hdr,
                                            void **data, size_t * p_data_sz,
                                            int is_local, int *is_contig,
                                            MPIDIG_am_target_cmpl_cb * target_cmpl_cb,
                                            MPIR_Request ** req);
static int do_put_data(MPIR_Request * preq, MPIR_Context_id_t context_id, int rank,
                       MPIDI_av_entry_t * addr, MPI_Aint count, MPI_Datatype datatype,
                       MPIR_Request * rreq, int is_local);
static int MPIDIG_put_data_origin_cb(MPIR_Request * sreq);
static int MPIDIG_put_data_target_msg_cb(int handler_id, void *am_hdr,
                                         void **data, size_t * p_data_sz,
                                         int is_local, int *is_contig,
                                         MPIDIG_am_target_cmpl_cb * target_cmpl_cb,
                                         MPIR_Request ** req);

MPIR_T_pvar_timer_t PVAR_TIMER_rma_targetcb_put ATTRIBUTE((unused));
MPIR_T_pvar_timer_t PVAR_TIMER_rma_targetcb_put_ack ATTRIBUTE((unused));
MPIR_T_pvar_timer_t PVAR_TIMER_rma_targetcb_put_iov ATTRIBUTE((unused));
MPIR_T_pvar_timer_t PVAR_TIMER_rma_targetcb_put_iov_ack ATTRIBUTE((unused));
MPIR_T_pvar_timer_t PVAR_TIMER_rma_targetcb_put_data ATTRIBUTE((unused));

int MPIDIG_am_put_init(void)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIDIG_am_reg_cb(MPIDIG_PUT_REQ, &MPIDIG_put_origin_cb, &MPIDIG_put_target_msg_cb);
    MPIR_ERR_CHECK(mpi_errno);
    mpi_errno = MPIDIG_am_reg_cb(MPIDIG_PUT_ACK, NULL, &MPIDIG_put_ack_target_msg_cb);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIDIG_am_reg_cb(MPIDIG_PUT_IOV_REQ,
                                 &MPIDIG_put_iov_origin_cb, &MPIDIG_put_iov_target_msg_cb);
    MPIR_ERR_CHECK(mpi_errno);
    mpi_errno = MPIDIG_am_reg_cb(MPIDIG_PUT_IOV_ACK, NULL, &MPIDIG_put_iov_ack_target_msg_cb);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIDIG_am_reg_cb(MPIDIG_PUT_DAT_REQ,
                                 &MPIDIG_put_data_origin_cb, &MPIDIG_put_data_target_msg_cb);
    MPIR_ERR_CHECK(mpi_errno);

    /* rma_targetcb_put */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(RMA,
                                      MPI_DOUBLE,
                                      rma_targetcb_put,
                                      MPI_T_VERBOSITY_MPIDEV_DETAIL,
                                      MPI_T_BIND_NO_OBJECT,
                                      MPIR_T_PVAR_FLAG_READONLY,
                                      "RMA", "RMA:TARGETCB for Put (in seconds)");

    /* rma_targetcb_put_ack */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(RMA,
                                      MPI_DOUBLE,
                                      rma_targetcb_put_ack,
                                      MPI_T_VERBOSITY_MPIDEV_DETAIL,
                                      MPI_T_BIND_NO_OBJECT,
                                      MPIR_T_PVAR_FLAG_READONLY,
                                      "RMA", "RMA:TARGETCB for Put ACK (in seconds)");

    /* rma_targetcb_put_iov */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(RMA,
                                      MPI_DOUBLE,
                                      rma_targetcb_put_iov,
                                      MPI_T_VERBOSITY_MPIDEV_DETAIL,
                                      MPI_T_BIND_NO_OBJECT,
                                      MPIR_T_PVAR_FLAG_READONLY,
                                      "RMA", "RMA:TARGETCB for PUT IOV (in seconds)");

    /* rma_targetcb_put_iov_ack */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(RMA,
                                      MPI_DOUBLE,
                                      rma_targetcb_put_iov_ack,
                                      MPI_T_VERBOSITY_MPIDEV_DETAIL,
                                      MPI_T_BIND_NO_OBJECT,
                                      MPIR_T_PVAR_FLAG_READONLY,
                                      "RMA", "RMA:TARGETCB for PUT IOV ACK (in seconds)");

    /* rma_targetcb_put_data */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(RMA,
                                      MPI_DOUBLE,
                                      rma_targetcb_put_data,
                                      MPI_T_VERBOSITY_MPIDEV_DETAIL,
                                      MPI_T_BIND_NO_OBJECT,
                                      MPIR_T_PVAR_FLAG_READONLY,
                                      "RMA", "RMA:TARGETCB for PUT DATA (in seconds)");

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* MPIDIG_PUT_REQ */
struct am_put_hdr {
    int src_rank;
    uint64_t win_id;
    MPIR_Request *preq_ptr;
    MPI_Aint target_disp;
    uint64_t count;
    MPI_Datatype datatype;
    int n_iov;
};

int MPIDIG_do_put(const void *origin_addr, int origin_count,
                  MPI_Datatype origin_datatype, int target_rank,
                  MPI_Aint target_disp, int target_count,
                  MPI_Datatype target_datatype, MPIR_Win * win, MPIR_Request ** sreq_ptr)
{
    int mpi_errno = MPI_SUCCESS, n_iov, c;
    MPIR_Request *sreq = NULL;
    uint64_t offset;
    size_t data_sz;
    MPI_Aint num_iov;
    struct iovec *dt_iov, am_iov[2];
    size_t am_hdr_max_size;
#ifndef MPIDI_CH4_DIRECT_NETMOD
    int is_local;
#endif

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_DO_PUT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_DO_PUT);

#ifndef MPIDI_CH4_DIRECT_NETMOD
    is_local = MPIDI_rank_is_local(target_rank, win->comm_ptr);
#endif

    MPIDIG_RMA_OP_CHECK_SYNC(target_rank, win);

    MPIDI_Datatype_check_size(origin_datatype, origin_count, data_sz);
    if (data_sz == 0)
        goto immed_cmpl;

    if (target_rank == win->comm_ptr->rank) {
        offset = win->disp_unit * target_disp;
        mpi_errno = MPIR_Localcopy(origin_addr,
                                   origin_count,
                                   origin_datatype,
                                   (char *) win->base + offset, target_count, target_datatype);
        MPIR_ERR_CHECK(mpi_errno);
        goto immed_cmpl;
    }

    /* Only create request when issuing is not completed.
     * We initialize two ref_count for progress engine and request-based OP,
     * then put needs to free the second ref_count.*/
    sreq = MPIDIG_request_create(MPIR_REQUEST_KIND__RMA, 2);
    MPIR_ERR_CHKANDSTMT(sreq == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
    MPIDIG_REQUEST(sreq, req->preq.win_ptr) = win;

    MPIR_cc_incr(sreq->cc_ptr, &c);
    MPIR_T_PVAR_TIMER_START(RMA, rma_amhdr_set);

    struct am_put_hdr am_hdr;
    am_hdr.src_rank = win->comm_ptr->rank;
    am_hdr.target_disp = target_disp;
    am_hdr.count = target_count;
    am_hdr.datatype = target_datatype;
    am_hdr.preq_ptr = sreq;
    am_hdr.win_id = MPIDIG_WIN(win, win_id);

    /* Increase local and remote completion counters and set the local completion
     * counter in request, thus it can be decreased at request completion. */
    MPIDIG_win_cmpl_cnts_incr(win, target_rank, &sreq->completion_notification);
    MPIDIG_REQUEST(sreq, rank) = target_rank;

    if (HANDLE_IS_BUILTIN(target_datatype)) {
        am_hdr.n_iov = 0;
        MPIR_T_PVAR_TIMER_END(RMA, rma_amhdr_set);
        MPIDIG_REQUEST(sreq, req->preq.dt_iov) = NULL;

#ifndef MPIDI_CH4_DIRECT_NETMOD
        if (is_local)
            mpi_errno = MPIDI_SHM_am_isend(target_rank, win->comm_ptr, MPIDIG_PUT_REQ,
                                           &am_hdr, sizeof(am_hdr), origin_addr,
                                           origin_count, origin_datatype, sreq);
        else
#endif
        {
            mpi_errno = MPIDI_NM_am_isend(target_rank, win->comm_ptr, MPIDIG_PUT_REQ,
                                          &am_hdr, sizeof(am_hdr), origin_addr,
                                          origin_count, origin_datatype, sreq);
        }

        MPIR_ERR_CHECK(mpi_errno);
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

    am_iov[0].iov_base = &am_hdr;
    am_iov[0].iov_len = sizeof(am_hdr);
    am_iov[1].iov_base = dt_iov;
    am_iov[1].iov_len = sizeof(struct iovec) * am_hdr.n_iov;
    MPIR_T_PVAR_TIMER_END(RMA, rma_amhdr_set);

    MPIDIG_REQUEST(sreq, req->preq.dt_iov) = dt_iov;

#ifndef MPIDI_CH4_DIRECT_NETMOD
    am_hdr_max_size = is_local ? MPIDI_SHM_am_hdr_max_sz() : MPIDI_NM_am_hdr_max_sz();
#else
    am_hdr_max_size = MPIDI_NM_am_hdr_max_sz();
#endif

    if ((am_iov[0].iov_len + am_iov[1].iov_len) <= am_hdr_max_size) {
#ifndef MPIDI_CH4_DIRECT_NETMOD
        if (is_local)
            mpi_errno = MPIDI_SHM_am_isendv(target_rank, win->comm_ptr, MPIDIG_PUT_REQ,
                                            &am_iov[0], 2, origin_addr, origin_count,
                                            origin_datatype, sreq);
        else
#endif
        {
            mpi_errno = MPIDI_NM_am_isendv(target_rank, win->comm_ptr, MPIDIG_PUT_REQ,
                                           &am_iov[0], 2, origin_addr, origin_count,
                                           origin_datatype, sreq);
        }
    } else {
        MPIDIG_REQUEST(sreq, req->preq.origin_addr) = (void *) origin_addr;
        MPIDIG_REQUEST(sreq, req->preq.origin_count) = origin_count;
        MPIDIG_REQUEST(sreq, req->preq.origin_datatype) = origin_datatype;
        MPIR_Datatype_add_ref_if_not_builtin(origin_datatype);

#ifndef MPIDI_CH4_DIRECT_NETMOD
        if (is_local)
            mpi_errno = MPIDI_SHM_am_isend(target_rank, win->comm_ptr, MPIDIG_PUT_IOV_REQ,
                                           &am_hdr, sizeof(am_hdr), am_iov[1].iov_base,
                                           am_iov[1].iov_len, MPI_BYTE, sreq);
        else
#endif
        {
            mpi_errno = MPIDI_NM_am_isend(target_rank, win->comm_ptr, MPIDIG_PUT_IOV_REQ,
                                          &am_hdr, sizeof(am_hdr), am_iov[1].iov_base,
                                          am_iov[1].iov_len, MPI_BYTE, sreq);
        }
    }
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    if (sreq_ptr)
        *sreq_ptr = sreq;
    else if (sreq != NULL)
        MPIR_Request_free(sreq);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_DO_PUT);
    return mpi_errno;

  immed_cmpl:
    if (sreq_ptr)
        MPIDI_RMA_REQUEST_CREATE_COMPLETE(sreq);
    goto fn_exit;

  fn_fail:
    goto fn_exit;
}

static int MPIDIG_put_origin_cb(MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Request_complete(sreq);
    return mpi_errno;
}

static int MPIDIG_put_target_msg_cb(int handler_id, void *am_hdr, void **data, size_t * p_data_sz,
                                    int is_local, int *is_contig,
                                    MPIDIG_am_target_cmpl_cb * target_cmpl_cb, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;

    struct am_put_hdr *hdr = am_hdr;
    MPIR_T_PVAR_TIMER_START(RMA, rma_targetcb_put);

    MPIR_Win *win;
    win = (MPIR_Win *) MPIDIU_map_lookup(MPIDI_global.win_map, hdr->win_id);
    MPIR_Assert(win);

    MPIR_Request *rreq;
    rreq = MPIDIG_request_create(MPIR_REQUEST_KIND__RMA, 1);
    MPIR_ERR_CHKANDSTMT(rreq == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
    *req = rreq;

    MPIDIG_REQUEST(rreq, rank) = hdr->src_rank;
#ifndef MPIDI_CH4_DIRECT_NETMOD
    MPIDI_REQUEST(rreq, is_local) = is_local;
#endif
    MPIDIG_REQUEST(rreq, req->preq.preq_ptr) = hdr->preq_ptr;
    MPIDIG_REQUEST(rreq, req->preq.win_ptr) = win;

    uintptr_t base = MPIDIG_win_base_at_target(win);
    size_t offset = win->disp_unit * hdr->target_disp;

    *target_cmpl_cb = put_target_cmpl_cb;

    if (hdr->n_iov) {
        struct iovec *iov, *dt_iov;
        dt_iov = (struct iovec *) MPL_malloc(sizeof(struct iovec) * hdr->n_iov, MPL_MEM_RMA);
        MPIR_Assert(dt_iov);

        iov = (struct iovec *) ((char *) am_hdr + sizeof(*hdr));
        for (int i = 0; i < hdr->n_iov; i++) {
            dt_iov[i].iov_base = (char *) iov[i].iov_base + base + offset;
            dt_iov[i].iov_len = iov[i].iov_len;
        }

        MPIDIG_REQUEST(rreq, req->preq.dt_iov) = dt_iov;
        MPIDIG_REQUEST(rreq, req->preq.n_iov) = hdr->n_iov;
        *is_contig = 0;
        *data = dt_iov;
        *p_data_sz = hdr->n_iov;
        goto fn_exit;
    }

    MPIDIG_REQUEST(rreq, req->preq.dt_iov) = NULL;

    int dt_contig;
    MPI_Aint dt_true_lb;
    size_t data_sz;
    MPIR_Datatype *dt_ptr;
    MPIDI_Datatype_get_info(hdr->count, hdr->datatype, dt_contig, data_sz, dt_ptr, dt_true_lb);
    *is_contig = dt_contig;

    if (dt_contig) {
        *p_data_sz = data_sz;
        *data = (char *) (offset + base + dt_true_lb);
    } else {
        MPI_Aint num_iov;
        MPIR_Typerep_iov_len((void *) (offset + base), hdr->count, hdr->datatype,
                             0, data_sz, &num_iov);
        int n_iov = (int) num_iov;
        MPIR_Assert(n_iov > 0);
        MPIDIG_REQUEST(rreq, req->iov) =
            (struct iovec *) MPL_malloc(n_iov * sizeof(struct iovec), MPL_MEM_RMA);
        MPIR_Assert(MPIDIG_REQUEST(rreq, req->iov));

        int actual_iov_len;
        MPI_Aint actual_iov_bytes;
        MPIR_Typerep_to_iov((void *) (offset + base), hdr->count, hdr->datatype,
                            0, MPIDIG_REQUEST(rreq, req->iov), n_iov, data_sz,
                            &actual_iov_len, &actual_iov_bytes);
        n_iov = actual_iov_len;

        MPIR_Assert(actual_iov_bytes == (MPI_Aint) data_sz);
        *data = MPIDIG_REQUEST(rreq, req->iov);
        *p_data_sz = n_iov;
        MPIDIG_REQUEST(rreq, req->status) |= MPIDIG_REQ_RCV_NON_CONTIG;
    }

  fn_exit:
    MPIR_T_PVAR_TIMER_END(RMA, rma_targetcb_put);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_PUT_TARGET_MSG_CB);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int put_target_cmpl_cb(MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_PUT_TARGET_CMPL_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_PUT_TARGET_CMPL_CB);

    if (MPIDIG_REQUEST(rreq, req->status) & MPIDIG_REQ_RCV_NON_CONTIG) {
        MPL_free(MPIDIG_REQUEST(rreq, req->iov));
    }

    MPL_free(MPIDIG_REQUEST(rreq, req->preq.dt_iov));

    mpi_errno = ack_put(rreq);
    MPIR_ERR_CHECK(mpi_errno);

    MPID_Request_complete(rreq);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_PUT_TARGET_CMPL_CB);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* MPIDIG_PUT_ACK */
struct am_put_ack_hdr {
    MPIR_Request *preq_ptr;
};

static int ack_put(MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    struct am_put_ack_hdr am_hdr;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_ACK_PUT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_ACK_PUT);

    am_hdr.preq_ptr = MPIDIG_REQUEST(rreq, req->preq.preq_ptr);
#ifndef MPIDI_CH4_DIRECT_NETMOD
    if (MPIDI_REQUEST(rreq, is_local))
        mpi_errno =
            MPIDI_SHM_am_send_hdr_reply(MPIDIG_win_to_context
                                        (MPIDIG_REQUEST(rreq, req->preq.win_ptr)),
                                        MPIDIG_REQUEST(rreq, rank), MPIDIG_PUT_ACK,
                                        &am_hdr, sizeof(am_hdr));
    else
#endif
    {
        mpi_errno =
            MPIDI_NM_am_send_hdr_reply(MPIDIG_win_to_context
                                       (MPIDIG_REQUEST(rreq, req->preq.win_ptr)),
                                       MPIDIG_REQUEST(rreq, rank), MPIDIG_PUT_ACK,
                                       &am_hdr, sizeof(am_hdr));
    }

    MPIR_ERR_CHECK(mpi_errno);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_ACK_PUT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int MPIDIG_put_ack_target_msg_cb(int handler_id, void *am_hdr,
                                        void **data, size_t * p_data_sz,
                                        int is_local, int *is_contig,
                                        MPIDIG_am_target_cmpl_cb * target_cmpl_cb,
                                        MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_T_PVAR_TIMER_START(RMA, rma_targetcb_put_ack);

    struct am_put_ack_hdr *hdr = am_hdr;
    MPIR_Request *preq = hdr->preq_ptr;
    MPIR_Win *win = MPIDIG_REQUEST(preq, req->preq.win_ptr);

    MPL_free(MPIDIG_REQUEST(preq, req->preq.dt_iov));
    MPIDIG_win_remote_cmpl_cnt_decr(win, MPIDIG_REQUEST(preq, rank));
    MPID_Request_complete(preq);

    if (req)
        *req = NULL;
    if (target_cmpl_cb)
        *target_cmpl_cb = NULL;

    MPIR_T_PVAR_TIMER_END(RMA, rma_targetcb_put_ack);
    return mpi_errno;
}

/* MPIDIG_PUT_IOV_REQ */
static int MPIDIG_put_iov_origin_cb(MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_PUT_IOV_ORIGIN_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_PUT_IOV_ORIGIN_CB);
    MPID_Request_complete(sreq);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_PUT_IOV_ORIGIN_CB);
    return mpi_errno;
}

static int MPIDIG_put_iov_target_msg_cb(int handler_id, void *am_hdr,
                                        void **data, size_t * p_data_sz,
                                        int is_local, int *is_contig,
                                        MPIDIG_am_target_cmpl_cb * target_cmpl_cb,
                                        MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *rreq = NULL;
    struct iovec *dt_iov;
    uintptr_t base;
    size_t offset;

    MPIR_Win *win;
    struct am_put_hdr *hdr = am_hdr;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_PUT_IOV_TARGET_MSG_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_PUT_IOV_TARGET_MSG_CB);
    MPIR_T_PVAR_TIMER_START(RMA, rma_targetcb_put_iov);

    rreq = MPIDIG_request_create(MPIR_REQUEST_KIND__RMA, 1);
    MPIR_ERR_CHKANDSTMT(rreq == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
    *req = rreq;

    MPIDIG_REQUEST(*req, req->preq.preq_ptr) = hdr->preq_ptr;
    MPIDIG_REQUEST(*req, rank) = hdr->src_rank;

    win = (MPIR_Win *) MPIDIU_map_lookup(MPIDI_global.win_map, hdr->win_id);
    MPIR_Assert(win);

    MPIDIG_REQUEST(rreq, req->preq.win_ptr) = win;

    offset = win->disp_unit * hdr->target_disp;
    base = MPIDIG_win_base_at_target(win);

    *target_cmpl_cb = do_put_iov_ack;
#ifndef MPIDI_CH4_DIRECT_NETMOD
    MPIDI_REQUEST(rreq, is_local) = is_local;
#endif

    /* Base adjustment for iov will be done after we get the entire iovs,
     * at MPIDIG_put_data_target_msg_cb */
    MPIR_Assert(hdr->n_iov);
    dt_iov = (struct iovec *) MPL_malloc(sizeof(struct iovec) * hdr->n_iov, MPL_MEM_RMA);
    MPIR_Assert(dt_iov);

    MPIDIG_REQUEST(rreq, req->preq.dt_iov) = dt_iov;
    MPIDIG_REQUEST(rreq, req->preq.n_iov) = hdr->n_iov;
    MPIDIG_REQUEST(*req, req->preq.target_addr) = (void *) (offset + base);
    *is_contig = 1;
    *data = dt_iov;
    *p_data_sz = hdr->n_iov * sizeof(struct iovec);

  fn_exit:
    MPIR_T_PVAR_TIMER_END(RMA, rma_targetcb_put_iov);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_PUT_IOV_TARGET_MSG_CB);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* MPIDIG_PUT_IOV_ACK */
struct am_put_iov_ack_hdr {
    int src_rank;
    MPIR_Request *target_preq_ptr;
    MPIR_Request *origin_preq_ptr;
};

static int do_put_iov_ack(MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_PUT_IOV_TARGET_CMPL_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_PUT_IOV_TARGET_CMPL_CB);

    struct am_put_iov_ack_hdr am_hdr;
    am_hdr.src_rank = MPIDIG_REQUEST(rreq, rank);
    am_hdr.origin_preq_ptr = MPIDIG_REQUEST(rreq, req->preq.preq_ptr);
    am_hdr.target_preq_ptr = rreq;

#ifndef MPIDI_CH4_DIRECT_NETMOD
    if (MPIDI_REQUEST(rreq, is_local))
        mpi_errno =
            MPIDI_SHM_am_send_hdr_reply(MPIDIG_win_to_context
                                        (MPIDIG_REQUEST(rreq, req->preq.win_ptr)),
                                        MPIDIG_REQUEST(rreq, rank), MPIDIG_PUT_IOV_ACK,
                                        &am_hdr, sizeof(am_hdr));
    else
#endif
    {
        mpi_errno =
            MPIDI_NM_am_send_hdr_reply(MPIDIG_win_to_context
                                       (MPIDIG_REQUEST(rreq, req->preq.win_ptr)),
                                       MPIDIG_REQUEST(rreq, rank), MPIDIG_PUT_IOV_ACK,
                                       &am_hdr, sizeof(am_hdr));
    }

    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_PUT_IOV_TARGET_CMPL_CB);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int MPIDIG_put_iov_ack_target_msg_cb(int handler_id, void *am_hdr,
                                            void **data, size_t * p_data_sz,
                                            int is_local, int *is_contig,
                                            MPIDIG_am_target_cmpl_cb * target_cmpl_cb,
                                            MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    struct am_put_iov_ack_hdr *hdr = am_hdr;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_PUT_IOV_ACK_TARGET_MSG_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_PUT_IOV_ACK_TARGET_MSG_CB);
    MPIR_T_PVAR_TIMER_START(RMA, rma_targetcb_put_iov_ack);

    MPIR_Request *rreq = MPIDIG_request_create(MPIR_REQUEST_KIND__RMA, 1);
    MPIR_ERR_CHKANDSTMT(rreq == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");

    MPIR_Request *origin_req = (MPIR_Request *) hdr->origin_preq_ptr;
    MPIR_Win *win = MPIDIG_REQUEST(origin_req, req->preq.win_ptr);

    mpi_errno = do_put_data(hdr->target_preq_ptr,
                            MPIDIG_win_to_context(win),
                            MPIDIG_REQUEST(origin_req, rank),
                            MPIDIG_REQUEST(origin_req, req->preq.origin_addr),
                            MPIDIG_REQUEST(origin_req, req->preq.origin_count),
                            MPIDIG_REQUEST(origin_req, req->preq.origin_datatype), rreq, is_local);
    MPIR_ERR_CHECK(mpi_errno);
    MPIR_Datatype_release_if_not_builtin(MPIDIG_REQUEST(origin_req, req->preq.origin_datatype));

    *target_cmpl_cb = NULL;
    *req = NULL;

  fn_exit:
    MPIR_T_PVAR_TIMER_END(RMA, rma_targetcb_put_iov_ack);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_PUT_IOV_ACK_TARGET_MSG_CB);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* MPIDIG_PUT_DAT_REQ */
struct am_put_dat_hdr {
    MPIR_Request *preq_ptr;
};

static int do_put_data(MPIR_Request * preq, MPIR_Context_id_t context_id, int rank,
                       MPIDI_av_entry_t * addr, MPI_Aint count, MPI_Datatype datatype,
                       MPIR_Request * rreq, int is_local)
{
    int mpi_errno;

    struct am_put_dat_hdr am_hdr;
    am_hdr.preq_ptr = preq;
#ifndef MPIDI_CH4_DIRECT_NETMOD
    if (is_local)
        mpi_errno = MPIDI_SHM_am_isend_reply(context_id, rank, MPIDIG_PUT_DAT_REQ,
                                             &am_hdr, sizeof(am_hdr), addr, count, datatype, rreq);
    else
#endif
    {
        mpi_errno = MPIDI_NM_am_isend_reply(context_id, rank, MPIDIG_PUT_DAT_REQ,
                                            &am_hdr, sizeof(am_hdr), addr, count, datatype, rreq);
    }
    return mpi_errno;
}

static int MPIDIG_put_data_origin_cb(MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_PUT_DATA_ORIGIN_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_PUT_DATA_ORIGIN_CB);
    MPID_Request_complete(sreq);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_PUT_DATA_ORIGIN_CB);
    return mpi_errno;
}

static int MPIDIG_put_data_target_msg_cb(int handler_id, void *am_hdr,
                                         void **data, size_t * p_data_sz,
                                         int is_local, int *is_contig,
                                         MPIDIG_am_target_cmpl_cb * target_cmpl_cb,
                                         MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *rreq;
    struct am_put_dat_hdr *hdr = am_hdr;
    struct iovec *iov;
    uintptr_t base;
    int i;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_PUT_DATA_TARGET_MSG_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_PUT_DATA_TARGET_MSG_CB);
    MPIR_T_PVAR_TIMER_START(RMA, rma_targetcb_put_data);

    rreq = (MPIR_Request *) hdr->preq_ptr;
    base = (uintptr_t) MPIDIG_REQUEST(rreq, req->preq.target_addr);

    /* Adjust the target iov addresses using the base address
     * (window base + target_disp) */
    iov = (struct iovec *) MPIDIG_REQUEST(rreq, req->preq.dt_iov);
    for (i = 0; i < MPIDIG_REQUEST(rreq, req->preq.n_iov); i++)
        iov[i].iov_base = (char *) iov[i].iov_base + base;

    *data = MPIDIG_REQUEST(rreq, req->preq.dt_iov);
    *is_contig = 0;
    *p_data_sz = MPIDIG_REQUEST(rreq, req->preq.n_iov);
    *req = rreq;
    *target_cmpl_cb = put_target_cmpl_cb;

    MPIR_T_PVAR_TIMER_END(RMA, rma_targetcb_put_data);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_PUT_DATA_TARGET_MSG_CB);
    return mpi_errno;
}

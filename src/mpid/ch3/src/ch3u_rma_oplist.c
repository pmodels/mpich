/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#include "mpidrma.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_CH3_RMA_ACC_IMMED
      category    : CH3
      type        : boolean
      default     : true
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Use the immediate accumulate optimization

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

static int send_flush_msg(int dest, MPID_Win *win_ptr);
static int send_rma_msg(MPIDI_RMA_Op_t * rma_op, MPID_Win * win_ptr, MPIDI_CH3_Pkt_flags_t flags);
static int send_contig_acc_msg(MPIDI_RMA_Op_t * rma_op,
                               MPID_Win * win_ptr, MPIDI_CH3_Pkt_flags_t flags);
static int recv_rma_msg(MPIDI_RMA_Op_t * rma_op, MPID_Win * win_ptr, MPIDI_CH3_Pkt_flags_t flags);
static int send_immed_rmw_msg(MPIDI_RMA_Op_t * rma_op,
                              MPID_Win * win_ptr, MPIDI_CH3_Pkt_flags_t flags);

/* create_datatype() creates a new struct datatype for the dtype_info
   and the dataloop of the target datatype together with the user data */
#undef FUNCNAME
#define FUNCNAME create_datatype
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int create_datatype(const MPIDI_RMA_dtype_info * dtype_info,
                           const void *dataloop, MPI_Aint dataloop_sz,
                           const void *o_addr, int o_count, MPI_Datatype o_datatype,
                           MPID_Datatype ** combined_dtp)
{
    int mpi_errno = MPI_SUCCESS;
    /* datatype_set_contents wants an array 'ints' which is the
     * blocklens array with count prepended to it.  So blocklens
     * points to the 2nd element of ints to avoid having to copy
     * blocklens into ints later. */
    int ints[4];
    int *blocklens = &ints[1];
    MPI_Aint displaces[3];
    MPI_Datatype datatypes[3];
    const int count = 3;
    MPI_Datatype combined_datatype;
    MPIDI_STATE_DECL(MPID_STATE_CREATE_DATATYPE);

    MPIDI_FUNC_ENTER(MPID_STATE_CREATE_DATATYPE);

    /* create datatype */
    displaces[0] = MPIU_PtrToAint(dtype_info);
    blocklens[0] = sizeof(*dtype_info);
    datatypes[0] = MPI_BYTE;

    displaces[1] = MPIU_PtrToAint(dataloop);
    MPIU_Assign_trunc(blocklens[1], dataloop_sz, int);
    datatypes[1] = MPI_BYTE;

    displaces[2] = MPIU_PtrToAint(o_addr);
    blocklens[2] = o_count;
    datatypes[2] = o_datatype;

    mpi_errno = MPID_Type_struct(count, blocklens, displaces, datatypes, &combined_datatype);
    if (mpi_errno)
        MPIU_ERR_POP(mpi_errno);

    ints[0] = count;

    MPID_Datatype_get_ptr(combined_datatype, *combined_dtp);
    mpi_errno = MPID_Datatype_set_contents(*combined_dtp, MPI_COMBINER_STRUCT, count + 1,       /* ints (cnt,blklen) */
                                           count,       /* aints (disps) */
                                           count,       /* types */
                                           ints, displaces, datatypes);
    if (mpi_errno)
        MPIU_ERR_POP(mpi_errno);

    /* Commit datatype */

    MPID_Dataloop_create(combined_datatype,
                         &(*combined_dtp)->dataloop,
                         &(*combined_dtp)->dataloop_size,
                         &(*combined_dtp)->dataloop_depth, MPID_DATALOOP_HOMOGENEOUS);

    /* create heterogeneous dataloop */
    MPID_Dataloop_create(combined_datatype,
                         &(*combined_dtp)->hetero_dloop,
                         &(*combined_dtp)->hetero_dloop_size,
                         &(*combined_dtp)->hetero_dloop_depth, MPID_DATALOOP_HETEROGENEOUS);

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_CREATE_DATATYPE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME send_rma_msg
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int send_rma_msg(MPIDI_RMA_Op_t * rma_op, MPID_Win * win_ptr,
                        MPIDI_CH3_Pkt_flags_t flags,
                        MPI_Win source_win_handle,
                        MPI_Win target_win_handle,
                        MPIDI_RMA_dtype_info * dtype_info, void **dataloop, MPID_Request ** request)
{
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_CH3_Pkt_put_t *put_pkt = &upkt.put;
    MPIDI_CH3_Pkt_accum_t *accum_pkt = &upkt.accum;
    MPID_IOV iov[MPID_IOV_LIMIT];
    int mpi_errno = MPI_SUCCESS;
    int origin_dt_derived, target_dt_derived, iovcnt;
    MPI_Aint origin_type_size;
    MPIDI_VC_t *vc;
    MPID_Comm *comm_ptr;
    MPID_Datatype *target_dtp = NULL, *origin_dtp = NULL;
    MPID_Request *resp_req = NULL;
    MPIU_CHKPMEM_DECL(1);
    MPIDI_STATE_DECL(MPID_STATE_SEND_RMA_MSG);
    MPIDI_STATE_DECL(MPID_STATE_MEMCPY);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_SEND_RMA_MSG);

    *request = NULL;

    if (rma_op->type == MPIDI_RMA_PUT) {
        MPIDI_Pkt_init(put_pkt, MPIDI_CH3_PKT_PUT);
        put_pkt->addr = (char *) win_ptr->base_addrs[rma_op->target_rank] +
            win_ptr->disp_units[rma_op->target_rank] * rma_op->target_disp;
        put_pkt->flags = flags;
        put_pkt->count = rma_op->target_count;
        put_pkt->datatype = rma_op->target_datatype;
        put_pkt->dataloop_size = 0;
        put_pkt->target_win_handle = target_win_handle;
        put_pkt->source_win_handle = source_win_handle;

        iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST) put_pkt;
        iov[0].MPID_IOV_LEN = sizeof(*put_pkt);
    }
    else if (rma_op->type == MPIDI_RMA_GET_ACCUMULATE) {
        /* Create a request for the GACC response.  Store the response buf, count, and
         * datatype in it, and pass the request's handle in the GACC packet. When the
         * response comes from the target, it will contain the request handle. */
        resp_req = MPID_Request_create();
        MPIU_ERR_CHKANDJUMP(resp_req == NULL, mpi_errno, MPI_ERR_OTHER, "**nomemreq");

        MPIU_Object_set_ref(resp_req, 2);

        resp_req->dev.user_buf = rma_op->result_addr;
        resp_req->dev.user_count = rma_op->result_count;
        resp_req->dev.datatype = rma_op->result_datatype;
        resp_req->dev.target_win_handle = target_win_handle;
        resp_req->dev.source_win_handle = source_win_handle;

        if (!MPIR_DATATYPE_IS_PREDEFINED(resp_req->dev.datatype)) {
            MPID_Datatype *result_dtp = NULL;
            MPID_Datatype_get_ptr(resp_req->dev.datatype, result_dtp);
            resp_req->dev.datatype_ptr = result_dtp;
            /* this will cause the datatype to be freed when the
             * request is freed. */
        }

        /* Note: Get_accumulate uses the same packet type as accumulate */
        MPIDI_Pkt_init(accum_pkt, MPIDI_CH3_PKT_GET_ACCUM);
        accum_pkt->addr = (char *) win_ptr->base_addrs[rma_op->target_rank] +
            win_ptr->disp_units[rma_op->target_rank] * rma_op->target_disp;
        accum_pkt->flags = flags;
        accum_pkt->count = rma_op->target_count;
        accum_pkt->datatype = rma_op->target_datatype;
        accum_pkt->dataloop_size = 0;
        accum_pkt->op = rma_op->op;
        accum_pkt->target_win_handle = target_win_handle;
        accum_pkt->source_win_handle = source_win_handle;
        accum_pkt->request_handle = resp_req->handle;

        iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST) accum_pkt;
        iov[0].MPID_IOV_LEN = sizeof(*accum_pkt);
    }
    else {
        MPIDI_Pkt_init(accum_pkt, MPIDI_CH3_PKT_ACCUMULATE);
        accum_pkt->addr = (char *) win_ptr->base_addrs[rma_op->target_rank] +
            win_ptr->disp_units[rma_op->target_rank] * rma_op->target_disp;
        accum_pkt->flags = flags;
        accum_pkt->count = rma_op->target_count;
        accum_pkt->datatype = rma_op->target_datatype;
        accum_pkt->dataloop_size = 0;
        accum_pkt->op = rma_op->op;
        accum_pkt->target_win_handle = target_win_handle;
        accum_pkt->source_win_handle = source_win_handle;

        iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST) accum_pkt;
        iov[0].MPID_IOV_LEN = sizeof(*accum_pkt);
    }

    /*    printf("send pkt: type %d, addr %d, count %d, base %d\n", rma_pkt->type,
     * rma_pkt->addr, rma_pkt->count, win_ptr->base_addrs[rma_op->target_rank]);
     * fflush(stdout);
     */

    comm_ptr = win_ptr->comm_ptr;
    MPIDI_Comm_get_vc_set_active(comm_ptr, rma_op->target_rank, &vc);

    if (!MPIR_DATATYPE_IS_PREDEFINED(rma_op->origin_datatype)) {
        origin_dt_derived = 1;
        MPID_Datatype_get_ptr(rma_op->origin_datatype, origin_dtp);
    }
    else {
        origin_dt_derived = 0;
    }

    if (!MPIR_DATATYPE_IS_PREDEFINED(rma_op->target_datatype)) {
        target_dt_derived = 1;
        MPID_Datatype_get_ptr(rma_op->target_datatype, target_dtp);
    }
    else {
        target_dt_derived = 0;
    }

    if (target_dt_derived) {
        /* derived datatype on target. fill derived datatype info */
        dtype_info->is_contig = target_dtp->is_contig;
        dtype_info->max_contig_blocks = target_dtp->max_contig_blocks;
        dtype_info->size = target_dtp->size;
        dtype_info->extent = target_dtp->extent;
        dtype_info->dataloop_size = target_dtp->dataloop_size;
        dtype_info->dataloop_depth = target_dtp->dataloop_depth;
        dtype_info->eltype = target_dtp->eltype;
        dtype_info->dataloop = target_dtp->dataloop;
        dtype_info->ub = target_dtp->ub;
        dtype_info->lb = target_dtp->lb;
        dtype_info->true_ub = target_dtp->true_ub;
        dtype_info->true_lb = target_dtp->true_lb;
        dtype_info->has_sticky_ub = target_dtp->has_sticky_ub;
        dtype_info->has_sticky_lb = target_dtp->has_sticky_lb;

        MPIU_CHKPMEM_MALLOC(*dataloop, void *, target_dtp->dataloop_size, mpi_errno, "dataloop");

        MPIDI_FUNC_ENTER(MPID_STATE_MEMCPY);
        MPIU_Memcpy(*dataloop, target_dtp->dataloop, target_dtp->dataloop_size);
        MPIDI_FUNC_EXIT(MPID_STATE_MEMCPY);
        /* the dataloop can have undefined padding sections, so we need to let
         * valgrind know that it is OK to pass this data to writev later on */
        MPL_VG_MAKE_MEM_DEFINED(*dataloop, target_dtp->dataloop_size);

        if (rma_op->type == MPIDI_RMA_PUT) {
            put_pkt->dataloop_size = target_dtp->dataloop_size;
        }
        else {
            accum_pkt->dataloop_size = target_dtp->dataloop_size;
        }
    }

    MPID_Datatype_get_size_macro(rma_op->origin_datatype, origin_type_size);

    if (!target_dt_derived) {
        /* basic datatype on target */
        if (!origin_dt_derived) {
            /* basic datatype on origin */
            iov[1].MPID_IOV_BUF = (MPID_IOV_BUF_CAST) rma_op->origin_addr;
            iov[1].MPID_IOV_LEN = rma_op->origin_count * origin_type_size;
            iovcnt = 2;
            MPIU_THREAD_CS_ENTER(CH3COMM, vc);
            mpi_errno = MPIDI_CH3_iStartMsgv(vc, iov, iovcnt, request);
            MPIU_THREAD_CS_EXIT(CH3COMM, vc);
            MPIU_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");
        }
        else {
            /* derived datatype on origin */
            *request = MPID_Request_create();
            MPIU_ERR_CHKANDJUMP(*request == NULL, mpi_errno, MPI_ERR_OTHER, "**nomemreq");

            MPIU_Object_set_ref(*request, 2);
            (*request)->kind = MPID_REQUEST_SEND;

            (*request)->dev.segment_ptr = MPID_Segment_alloc();
            MPIU_ERR_CHKANDJUMP1((*request)->dev.segment_ptr == NULL, mpi_errno, MPI_ERR_OTHER,
                                 "**nomem", "**nomem %s", "MPID_Segment_alloc");

            (*request)->dev.datatype_ptr = origin_dtp;
            /* this will cause the datatype to be freed when the request
             * is freed. */
            MPID_Segment_init(rma_op->origin_addr, rma_op->origin_count,
                              rma_op->origin_datatype, (*request)->dev.segment_ptr, 0);
            (*request)->dev.segment_first = 0;
            (*request)->dev.segment_size = rma_op->origin_count * origin_type_size;

            (*request)->dev.OnFinal = 0;
            (*request)->dev.OnDataAvail = 0;

            MPIU_THREAD_CS_ENTER(CH3COMM, vc);
            mpi_errno =
                vc->sendNoncontig_fn(vc, *request, iov[0].MPID_IOV_BUF, iov[0].MPID_IOV_LEN);
            MPIU_THREAD_CS_EXIT(CH3COMM, vc);
            MPIU_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");
        }
    }
    else {
        /* derived datatype on target */
        MPID_Datatype *combined_dtp = NULL;

        *request = MPID_Request_create();
        if (*request == NULL) {
            MPIU_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**nomemreq");
        }

        MPIU_Object_set_ref(*request, 2);
        (*request)->kind = MPID_REQUEST_SEND;

        (*request)->dev.segment_ptr = MPID_Segment_alloc();
        MPIU_ERR_CHKANDJUMP1((*request)->dev.segment_ptr == NULL, mpi_errno, MPI_ERR_OTHER,
                             "**nomem", "**nomem %s", "MPID_Segment_alloc");

        /* create a new datatype containing the dtype_info, dataloop, and origin data */

        mpi_errno =
            create_datatype(dtype_info, *dataloop, target_dtp->dataloop_size, rma_op->origin_addr,
                            rma_op->origin_count, rma_op->origin_datatype, &combined_dtp);
        if (mpi_errno)
            MPIU_ERR_POP(mpi_errno);

        (*request)->dev.datatype_ptr = combined_dtp;
        /* combined_datatype will be freed when request is freed */

        MPID_Segment_init(MPI_BOTTOM, 1, combined_dtp->handle, (*request)->dev.segment_ptr, 0);
        (*request)->dev.segment_first = 0;
        (*request)->dev.segment_size = combined_dtp->size;

        (*request)->dev.OnFinal = 0;
        (*request)->dev.OnDataAvail = 0;

        MPIU_THREAD_CS_ENTER(CH3COMM, vc);
        mpi_errno = vc->sendNoncontig_fn(vc, *request, iov[0].MPID_IOV_BUF, iov[0].MPID_IOV_LEN);
        MPIU_THREAD_CS_EXIT(CH3COMM, vc);
        MPIU_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");

        /* we're done with the datatypes */
        if (origin_dt_derived)
            MPID_Datatype_release(origin_dtp);
        MPID_Datatype_release(target_dtp);
    }

    /* This operation can generate two requests; one for inbound and one for
     * outbound data. */
    if (resp_req != NULL) {
        if (*request != NULL) {
            /* If we have both inbound and outbound requests (i.e. GACC
             * operation), we need to ensure that the source buffer is
             * available and that the response data has been received before
             * informing the origin that this operation is complete.  Because
             * the update needs to be done atomically at the target, they will
             * not send back data until it has been received.  Therefore,
             * completion of the response request implies that the send request
             * has completed.
             *
             * Therefore: refs on the response request are set to two: one is
             * held by the progress engine and the other by the RMA op
             * completion code.  Refs on the outbound request are set to one;
             * it will be completed by the progress engine.
             */

            MPID_Request_release(*request);
            *request = resp_req;

        }
        else {
            *request = resp_req;
        }

        /* For error checking */
        resp_req = NULL;
    }

  fn_exit:
    MPIU_CHKPMEM_COMMIT();
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_SEND_RMA_MSG);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    if (resp_req) {
        MPID_Request_release(resp_req);
    }
    if (*request) {
        MPIU_CHKPMEM_REAP();
        if ((*request)->dev.datatype_ptr)
            MPID_Datatype_release((*request)->dev.datatype_ptr);
        MPID_Request_release(*request);
    }
    *request = NULL;
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

/*
 * Use this for contiguous accumulate operations
 */
#undef FUNCNAME
#define FUNCNAME send_contig_acc_msg
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int send_contig_acc_msg(MPIDI_RMA_Op_t * rma_op,
                               MPID_Win * win_ptr,
                               MPIDI_CH3_Pkt_flags_t flags,
                               MPI_Win source_win_handle,
                               MPI_Win target_win_handle, MPID_Request ** request)
{
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_CH3_Pkt_accum_t *accum_pkt = &upkt.accum;
    MPID_IOV iov[MPID_IOV_LIMIT];
    int mpi_errno = MPI_SUCCESS;
    int iovcnt;
    MPI_Aint origin_type_size;
    MPIDI_VC_t *vc;
    MPID_Comm *comm_ptr;
    size_t len;
    MPIDI_STATE_DECL(MPID_STATE_SEND_CONTIG_ACC_MSG);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_SEND_CONTIG_ACC_MSG);

    *request = NULL;

    MPID_Datatype_get_size_macro(rma_op->origin_datatype, origin_type_size);
    /* FIXME: Make this size check efficient and match the packet type */
    MPIU_Assign_trunc(len, rma_op->origin_count * origin_type_size, size_t);
    if (MPIR_CVAR_CH3_RMA_ACC_IMMED && len <= MPIDI_RMA_IMMED_INTS * sizeof(int)) {
        MPIDI_CH3_Pkt_accum_immed_t *accumi_pkt = &upkt.accum_immed;
        void *dest = accumi_pkt->data, *src = rma_op->origin_addr;

        MPIDI_Pkt_init(accumi_pkt, MPIDI_CH3_PKT_ACCUM_IMMED);
        accumi_pkt->addr = (char *) win_ptr->base_addrs[rma_op->target_rank] +
            win_ptr->disp_units[rma_op->target_rank] * rma_op->target_disp;
        accumi_pkt->flags = flags;
        accumi_pkt->count = rma_op->target_count;
        accumi_pkt->datatype = rma_op->target_datatype;
        accumi_pkt->op = rma_op->op;
        accumi_pkt->target_win_handle = target_win_handle;
        accumi_pkt->source_win_handle = source_win_handle;

        switch (len) {
        case 1:
            *(uint8_t *) dest = *(uint8_t *) src;
            break;
        case 2:
            *(uint16_t *) dest = *(uint16_t *) src;
            break;
        case 4:
            *(uint32_t *) dest = *(uint32_t *) src;
            break;
        case 8:
            *(uint64_t *) dest = *(uint64_t *) src;
            break;
        default:
            MPIU_Memcpy(accumi_pkt->data, (void *) rma_op->origin_addr, len);
        }
        comm_ptr = win_ptr->comm_ptr;
        MPIDI_Comm_get_vc_set_active(comm_ptr, rma_op->target_rank, &vc);
        MPIU_THREAD_CS_ENTER(CH3COMM, vc);
        mpi_errno = MPIDI_CH3_iStartMsg(vc, accumi_pkt, sizeof(*accumi_pkt), request);
        MPIU_THREAD_CS_EXIT(CH3COMM, vc);
        MPIU_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");
        goto fn_exit;
    }

    MPIDI_Pkt_init(accum_pkt, MPIDI_CH3_PKT_ACCUMULATE);
    accum_pkt->addr = (char *) win_ptr->base_addrs[rma_op->target_rank] +
        win_ptr->disp_units[rma_op->target_rank] * rma_op->target_disp;
    accum_pkt->flags = flags;
    accum_pkt->count = rma_op->target_count;
    accum_pkt->datatype = rma_op->target_datatype;
    accum_pkt->dataloop_size = 0;
    accum_pkt->op = rma_op->op;
    accum_pkt->target_win_handle = target_win_handle;
    accum_pkt->source_win_handle = source_win_handle;

    iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST) accum_pkt;
    iov[0].MPID_IOV_LEN = sizeof(*accum_pkt);

    /*    printf("send pkt: type %d, addr %d, count %d, base %d\n", rma_pkt->type,
     * rma_pkt->addr, rma_pkt->count, win_ptr->base_addrs[rma_op->target_rank]);
     * fflush(stdout);
     */

    comm_ptr = win_ptr->comm_ptr;
    MPIDI_Comm_get_vc_set_active(comm_ptr, rma_op->target_rank, &vc);


    /* basic datatype on target */
    /* basic datatype on origin */
    /* FIXME: This is still very heavyweight for a small message operation,
     * such as a single word update */
    /* One possibility is to use iStartMsg with a buffer that is just large
     * enough, though note that nemesis has an optimization for this */
    iov[1].MPID_IOV_BUF = (MPID_IOV_BUF_CAST) rma_op->origin_addr;
    iov[1].MPID_IOV_LEN = rma_op->origin_count * origin_type_size;
    iovcnt = 2;
    MPIU_THREAD_CS_ENTER(CH3COMM, vc);
    mpi_errno = MPIDI_CH3_iStartMsgv(vc, iov, iovcnt, request);
    MPIU_THREAD_CS_EXIT(CH3COMM, vc);
    MPIU_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");

  fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_SEND_CONTIG_ACC_MSG);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    if (*request) {
        MPID_Request_release(*request);
    }
    *request = NULL;
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


/*
 * Initiate an immediate RMW accumulate operation
 */
#undef FUNCNAME
#define FUNCNAME send_immed_rmw_msg
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int send_immed_rmw_msg(MPIDI_RMA_Op_t * rma_op,
                              MPID_Win * win_ptr,
                              MPIDI_CH3_Pkt_flags_t flags,
                              MPI_Win source_win_handle,
                              MPI_Win target_win_handle, MPID_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Request *rmw_req = NULL, *resp_req = NULL;
    MPIDI_VC_t *vc;
    MPID_Comm *comm_ptr;
    MPI_Aint len;
    MPIDI_STATE_DECL(MPID_STATE_SEND_IMMED_RMW_MSG);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_SEND_IMMED_RMW_MSG);

    *request = NULL;

    /* Create a request for the RMW response.  Store the origin buf, count, and
     * datatype in it, and pass the request's handle RMW packet. When the
     * response comes from the target, it will contain the request handle. */
    resp_req = MPID_Request_create();
    MPIU_ERR_CHKANDJUMP(resp_req == NULL, mpi_errno, MPI_ERR_OTHER, "**nomemreq");
    *request = resp_req;

    /* Set refs on the request to 2: one for the response message, and one for
     * the partial completion handler */
    MPIU_Object_set_ref(resp_req, 2);

    resp_req->dev.user_buf = rma_op->result_addr;
    resp_req->dev.user_count = rma_op->result_count;
    resp_req->dev.datatype = rma_op->result_datatype;
    resp_req->dev.target_win_handle = target_win_handle;
    resp_req->dev.source_win_handle = source_win_handle;

    /* REQUIRE: All datatype arguments must be of the same, builtin
     * type and counts must be 1. */
    MPID_Datatype_get_size_macro(rma_op->origin_datatype, len);
    comm_ptr = win_ptr->comm_ptr;

    if (rma_op->type == MPIDI_RMA_COMPARE_AND_SWAP) {
        MPIDI_CH3_Pkt_t upkt;
        MPIDI_CH3_Pkt_cas_t *cas_pkt = &upkt.cas;

        MPIU_Assert(len <= sizeof(MPIDI_CH3_CAS_Immed_u));

        MPIDI_Pkt_init(cas_pkt, MPIDI_CH3_PKT_CAS);

        cas_pkt->addr = (char *) win_ptr->base_addrs[rma_op->target_rank] +
            win_ptr->disp_units[rma_op->target_rank] * rma_op->target_disp;
        cas_pkt->flags = flags;
        cas_pkt->datatype = rma_op->target_datatype;
        cas_pkt->target_win_handle = target_win_handle;
        cas_pkt->request_handle = resp_req->handle;

        MPIU_Memcpy((void *) &cas_pkt->origin_data, rma_op->origin_addr, len);
        MPIU_Memcpy((void *) &cas_pkt->compare_data, rma_op->compare_addr, len);

        MPIDI_Comm_get_vc_set_active(comm_ptr, rma_op->target_rank, &vc);
        MPIU_THREAD_CS_ENTER(CH3COMM, vc);
        mpi_errno = MPIDI_CH3_iStartMsg(vc, cas_pkt, sizeof(*cas_pkt), &rmw_req);
        MPIU_THREAD_CS_EXIT(CH3COMM, vc);
        MPIU_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");

        if (rmw_req != NULL) {
            MPID_Request_release(rmw_req);
        }
    }

    else if (rma_op->type == MPIDI_RMA_FETCH_AND_OP) {
        MPIDI_CH3_Pkt_t upkt;
        MPIDI_CH3_Pkt_fop_t *fop_pkt = &upkt.fop;

        MPIU_Assert(len <= sizeof(MPIDI_CH3_FOP_Immed_u));

        MPIDI_Pkt_init(fop_pkt, MPIDI_CH3_PKT_FOP);

        fop_pkt->addr = (char *) win_ptr->base_addrs[rma_op->target_rank] +
            win_ptr->disp_units[rma_op->target_rank] * rma_op->target_disp;
        fop_pkt->flags = flags;
        fop_pkt->datatype = rma_op->target_datatype;
        fop_pkt->target_win_handle = target_win_handle;
        fop_pkt->request_handle = resp_req->handle;
        fop_pkt->op = rma_op->op;

        if (len <= sizeof(fop_pkt->origin_data) || rma_op->op == MPI_NO_OP) {
            /* Embed FOP data in the packet header */
            if (rma_op->op != MPI_NO_OP) {
                MPIU_Memcpy(fop_pkt->origin_data, rma_op->origin_addr, len);
            }

            MPIDI_Comm_get_vc_set_active(comm_ptr, rma_op->target_rank, &vc);
            MPIU_THREAD_CS_ENTER(CH3COMM, vc);
            mpi_errno = MPIDI_CH3_iStartMsg(vc, fop_pkt, sizeof(*fop_pkt), &rmw_req);
            MPIU_THREAD_CS_EXIT(CH3COMM, vc);
            MPIU_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");

            if (rmw_req != NULL) {
                MPID_Request_release(rmw_req);
            }
        }
        else {
            /* Data is too big to copy into the FOP header, use an IOV to send it */
            MPID_IOV iov[MPID_IOV_LIMIT];

            rmw_req = MPID_Request_create();
            MPIU_ERR_CHKANDJUMP(rmw_req == NULL, mpi_errno, MPI_ERR_OTHER, "**nomemreq");
            MPIU_Object_set_ref(rmw_req, 1);

            rmw_req->dev.OnFinal = 0;
            rmw_req->dev.OnDataAvail = 0;

            iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST) fop_pkt;
            iov[0].MPID_IOV_LEN = sizeof(*fop_pkt);
            iov[1].MPID_IOV_BUF = (MPID_IOV_BUF_CAST) rma_op->origin_addr;
            iov[1].MPID_IOV_LEN = len;  /* count == 1 */

            MPIDI_Comm_get_vc_set_active(comm_ptr, rma_op->target_rank, &vc);
            MPIU_THREAD_CS_ENTER(CH3COMM, vc);
            mpi_errno = MPIDI_CH3_iSendv(vc, rmw_req, iov, 2);
            MPIU_THREAD_CS_EXIT(CH3COMM, vc);

            MPIU_ERR_CHKANDJUMP(mpi_errno != MPI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");
        }
    }
    else {
        MPIU_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");
    }

  fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_SEND_IMMED_RMW_MSG);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    if (*request) {
        MPID_Request_release(*request);
    }
    *request = NULL;
    if (rmw_req) {
        MPID_Request_release(rmw_req);
    }
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}



#undef FUNCNAME
#define FUNCNAME recv_rma_msg
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int recv_rma_msg(MPIDI_RMA_Op_t * rma_op, MPID_Win * win_ptr,
                        MPIDI_CH3_Pkt_flags_t flags,
                        MPI_Win source_win_handle,
                        MPI_Win target_win_handle,
                        MPIDI_RMA_dtype_info * dtype_info, void **dataloop, MPID_Request ** request)
{
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_CH3_Pkt_get_t *get_pkt = &upkt.get;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_VC_t *vc;
    MPID_Comm *comm_ptr;
    MPID_Request *req = NULL;
    MPID_Datatype *dtp;
    MPID_IOV iov[MPID_IOV_LIMIT];
    MPIU_CHKPMEM_DECL(1);
    MPIDI_STATE_DECL(MPID_STATE_RECV_RMA_MSG);
    MPIDI_STATE_DECL(MPID_STATE_MEMCPY);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_RECV_RMA_MSG);

    /* create a request, store the origin buf, cnt, datatype in it,
     * and pass a handle to it in the get packet. When the get
     * response comes from the target, it will contain the request
     * handle. */
    req = MPID_Request_create();
    if (req == NULL) {
        MPIU_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**nomemreq");
    }

    *request = req;

    MPIU_Object_set_ref(req, 2);

    req->dev.user_buf = rma_op->origin_addr;
    req->dev.user_count = rma_op->origin_count;
    req->dev.datatype = rma_op->origin_datatype;
    req->dev.target_win_handle = MPI_WIN_NULL;
    req->dev.source_win_handle = source_win_handle;
    if (!MPIR_DATATYPE_IS_PREDEFINED(req->dev.datatype)) {
        MPID_Datatype_get_ptr(req->dev.datatype, dtp);
        req->dev.datatype_ptr = dtp;
        /* this will cause the datatype to be freed when the
         * request is freed. */
    }

    MPIDI_Pkt_init(get_pkt, MPIDI_CH3_PKT_GET);
    get_pkt->addr = (char *) win_ptr->base_addrs[rma_op->target_rank] +
        win_ptr->disp_units[rma_op->target_rank] * rma_op->target_disp;
    get_pkt->flags = flags;
    get_pkt->count = rma_op->target_count;
    get_pkt->datatype = rma_op->target_datatype;
    get_pkt->request_handle = req->handle;
    get_pkt->target_win_handle = target_win_handle;
    get_pkt->source_win_handle = source_win_handle;

/*    printf("send pkt: type %d, addr %d, count %d, base %d\n", rma_pkt->type,
           rma_pkt->addr, rma_pkt->count, win_ptr->base_addrs[rma_op->target_rank]);
    fflush(stdout);
*/

    comm_ptr = win_ptr->comm_ptr;
    MPIDI_Comm_get_vc_set_active(comm_ptr, rma_op->target_rank, &vc);

    if (MPIR_DATATYPE_IS_PREDEFINED(rma_op->target_datatype)) {
        /* basic datatype on target. simply send the get_pkt. */
        MPIU_THREAD_CS_ENTER(CH3COMM, vc);
        mpi_errno = MPIDI_CH3_iStartMsg(vc, get_pkt, sizeof(*get_pkt), &req);
        MPIU_THREAD_CS_EXIT(CH3COMM, vc);
    }
    else {
        /* derived datatype on target. fill derived datatype info and
         * send it along with get_pkt. */

        MPID_Datatype_get_ptr(rma_op->target_datatype, dtp);
        dtype_info->is_contig = dtp->is_contig;
        dtype_info->max_contig_blocks = dtp->max_contig_blocks;
        dtype_info->size = dtp->size;
        dtype_info->extent = dtp->extent;
        dtype_info->dataloop_size = dtp->dataloop_size;
        dtype_info->dataloop_depth = dtp->dataloop_depth;
        dtype_info->eltype = dtp->eltype;
        dtype_info->dataloop = dtp->dataloop;
        dtype_info->ub = dtp->ub;
        dtype_info->lb = dtp->lb;
        dtype_info->true_ub = dtp->true_ub;
        dtype_info->true_lb = dtp->true_lb;
        dtype_info->has_sticky_ub = dtp->has_sticky_ub;
        dtype_info->has_sticky_lb = dtp->has_sticky_lb;

        MPIU_CHKPMEM_MALLOC(*dataloop, void *, dtp->dataloop_size, mpi_errno, "dataloop");

        MPIDI_FUNC_ENTER(MPID_STATE_MEMCPY);
        MPIU_Memcpy(*dataloop, dtp->dataloop, dtp->dataloop_size);
        MPIDI_FUNC_EXIT(MPID_STATE_MEMCPY);

        /* the dataloop can have undefined padding sections, so we need to let
         * valgrind know that it is OK to pass this data to writev later on */
        MPL_VG_MAKE_MEM_DEFINED(*dataloop, dtp->dataloop_size);

        get_pkt->dataloop_size = dtp->dataloop_size;

        iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST) get_pkt;
        iov[0].MPID_IOV_LEN = sizeof(*get_pkt);
        iov[1].MPID_IOV_BUF = (MPID_IOV_BUF_CAST) dtype_info;
        iov[1].MPID_IOV_LEN = sizeof(*dtype_info);
        iov[2].MPID_IOV_BUF = (MPID_IOV_BUF_CAST) * dataloop;
        iov[2].MPID_IOV_LEN = dtp->dataloop_size;

        MPIU_THREAD_CS_ENTER(CH3COMM, vc);
        mpi_errno = MPIDI_CH3_iStartMsgv(vc, iov, 3, &req);
        MPIU_THREAD_CS_EXIT(CH3COMM, vc);

        /* release the target datatype */
        MPID_Datatype_release(dtp);
    }

    if (mpi_errno != MPI_SUCCESS) {
        MPIU_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");
    }

    /* release the request returned by iStartMsg or iStartMsgv */
    if (req != NULL) {
        MPID_Request_release(req);
    }

  fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_RECV_RMA_MSG);
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME send_flush_msg
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int send_flush_msg(int dest, MPID_Win * win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_CH3_Pkt_flush_t *flush_pkt = &upkt.flush;
    MPID_Request *req = NULL;
    MPIDI_VC_t *vc;
    MPIDI_STATE_DECL(MPID_STATE_SEND_FLUSH_MSG);
    MPIDI_RMA_FUNC_ENTER(MPID_STATE_SEND_FLUSH_MSG);

    MPIDI_Comm_get_vc_set_active(win_ptr->comm_ptr, dest, &vc);

    MPIDI_Pkt_init(flush_pkt, MPIDI_CH3_PKT_FLUSH);
    flush_pkt->target_win_handle = win_ptr->all_win_handles[dest];
    flush_pkt->source_win_handle = win_ptr->handle;

    MPIU_THREAD_CS_ENTER(CH3COMM, vc);
    mpi_errno = MPIDI_CH3_iStartMsg(vc, flush_pkt, sizeof(*flush_pkt), &req);
    MPIU_THREAD_CS_EXIT(CH3COMM, vc);
    MPIU_ERR_CHKANDJUMP(mpi_errno != MPI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**ch3|rma_msg");

    /* Release the request returned by iStartMsg */
    if (req != NULL) {
        MPID_Request_release(req);
    }

  fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_SEND_FLUSH_MSG);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


int MPIDI_CH3I_Issue_rma_op(MPIDI_RMA_Op_t * op_ptr, MPID_Win * win_ptr,
                            MPIDI_CH3_Pkt_flags_t flags, MPI_Win source_win_handle,
                            MPI_Win target_win_handle)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_ISSUE_RMA_OP);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_ISSUE_RMA_OP);

    switch (op_ptr->type) {
    case (MPIDI_RMA_PUT):
    case (MPIDI_RMA_ACCUMULATE):
        mpi_errno = send_rma_msg(op_ptr, win_ptr, flags, source_win_handle,
                                 target_win_handle, &op_ptr->dtype_info,
                                 &op_ptr->dataloop, &op_ptr->request);
        if (mpi_errno)
            MPIU_ERR_POP(mpi_errno);
        break;
    case (MPIDI_RMA_GET_ACCUMULATE):
        if (op_ptr->op == MPI_NO_OP) {
            /* Note: Origin arguments are ignored for NO_OP, so we don't
             * need to release a ref to the origin datatype. */

            /* Convert the GAcc to a Get */
            op_ptr->type = MPIDI_RMA_GET;
            op_ptr->origin_addr = op_ptr->result_addr;
            op_ptr->origin_count = op_ptr->result_count;
            op_ptr->origin_datatype = op_ptr->result_datatype;

            mpi_errno = recv_rma_msg(op_ptr, win_ptr, flags, source_win_handle,
                                     target_win_handle, &op_ptr->dtype_info,
                                     &op_ptr->dataloop, &op_ptr->request);
        }
        else {
            mpi_errno = send_rma_msg(op_ptr, win_ptr, flags, source_win_handle,
                                     target_win_handle, &op_ptr->dtype_info,
                                     &op_ptr->dataloop, &op_ptr->request);
        }
        if (mpi_errno)
            MPIU_ERR_POP(mpi_errno);
        break;
    case (MPIDI_RMA_ACC_CONTIG):
        mpi_errno = send_contig_acc_msg(op_ptr, win_ptr, flags,
                                        source_win_handle, target_win_handle, &op_ptr->request);
        if (mpi_errno)
            MPIU_ERR_POP(mpi_errno);
        break;
    case (MPIDI_RMA_GET):
        mpi_errno = recv_rma_msg(op_ptr, win_ptr, flags,
                                 source_win_handle, target_win_handle,
                                 &op_ptr->dtype_info, &op_ptr->dataloop, &op_ptr->request);
        if (mpi_errno)
            MPIU_ERR_POP(mpi_errno);
        break;
    case (MPIDI_RMA_COMPARE_AND_SWAP):
    case (MPIDI_RMA_FETCH_AND_OP):
        mpi_errno = send_immed_rmw_msg(op_ptr, win_ptr, flags,
                                       source_win_handle, target_win_handle, &op_ptr->request);
        if (mpi_errno)
            MPIU_ERR_POP(mpi_errno);
        break;
    default:
        MPIU_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**winInvalidOp");
    }

  fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_ISSUE_RMA_OP);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

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
static int send_rma_msg(MPIDI_RMA_Op_t * rma_op, MPID_Win * win_ptr, MPIDI_CH3_Pkt_flags_t flags)
{
    MPIDI_CH3_Pkt_put_t *put_pkt = &rma_op->pkt.put;
    MPIDI_CH3_Pkt_accum_t *accum_pkt = &rma_op->pkt.accum;
    MPID_IOV iov[MPID_IOV_LIMIT];
    int mpi_errno = MPI_SUCCESS;
    int origin_dt_derived, target_dt_derived, iovcnt;
    MPI_Aint origin_type_size;
    MPIDI_VC_t *vc;
    MPID_Comm *comm_ptr;
    MPID_Datatype *target_dtp = NULL, *origin_dtp = NULL;
    MPI_Datatype target_datatype;
    MPID_Request *resp_req = NULL;
    MPIU_CHKPMEM_DECL(1);
    MPIDI_STATE_DECL(MPID_STATE_SEND_RMA_MSG);
    MPIDI_STATE_DECL(MPID_STATE_MEMCPY);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_SEND_RMA_MSG);

    rma_op->request = NULL;

    if (rma_op->pkt.type == MPIDI_CH3_PKT_PUT) {
        put_pkt->flags = flags;

        iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST) put_pkt;
        iov[0].MPID_IOV_LEN = sizeof(*put_pkt);
    }
    else if (rma_op->pkt.type == MPIDI_CH3_PKT_GET_ACCUM) {
        /* Create a request for the GACC response.  Store the response buf, count, and
         * datatype in it, and pass the request's handle in the GACC packet. When the
         * response comes from the target, it will contain the request handle. */
        resp_req = MPID_Request_create();
        MPIU_ERR_CHKANDJUMP(resp_req == NULL, mpi_errno, MPI_ERR_OTHER, "**nomemreq");

        MPIU_Object_set_ref(resp_req, 2);

        resp_req->dev.user_buf = rma_op->result_addr;
        resp_req->dev.user_count = rma_op->result_count;
        resp_req->dev.datatype = rma_op->result_datatype;
        resp_req->dev.target_win_handle = accum_pkt->target_win_handle;
        resp_req->dev.source_win_handle = accum_pkt->source_win_handle;

        if (!MPIR_DATATYPE_IS_PREDEFINED(resp_req->dev.datatype)) {
            MPID_Datatype *result_dtp = NULL;
            MPID_Datatype_get_ptr(resp_req->dev.datatype, result_dtp);
            resp_req->dev.datatype_ptr = result_dtp;
            /* this will cause the datatype to be freed when the
             * request is freed. */
        }

        /* Note: Get_accumulate uses the same packet type as accumulate */
        accum_pkt->request_handle = resp_req->handle;

        accum_pkt->flags = flags;
        iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST) accum_pkt;
        iov[0].MPID_IOV_LEN = sizeof(*accum_pkt);
    }
    else {
        accum_pkt->flags = flags;

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

    MPIDI_CH3_PKT_RMA_GET_TARGET_DATATYPE(rma_op->pkt, target_datatype, mpi_errno);
    if (!MPIR_DATATYPE_IS_PREDEFINED(target_datatype)) {
        target_dt_derived = 1;
        MPID_Datatype_get_ptr(target_datatype, target_dtp);
    }
    else {
        target_dt_derived = 0;
    }

    if (target_dt_derived) {
        /* derived datatype on target. fill derived datatype info */
        rma_op->dtype_info.is_contig = target_dtp->is_contig;
        rma_op->dtype_info.max_contig_blocks = target_dtp->max_contig_blocks;
        rma_op->dtype_info.size = target_dtp->size;
        rma_op->dtype_info.extent = target_dtp->extent;
        rma_op->dtype_info.dataloop_size = target_dtp->dataloop_size;
        rma_op->dtype_info.dataloop_depth = target_dtp->dataloop_depth;
        rma_op->dtype_info.eltype = target_dtp->eltype;
        rma_op->dtype_info.dataloop = target_dtp->dataloop;
        rma_op->dtype_info.ub = target_dtp->ub;
        rma_op->dtype_info.lb = target_dtp->lb;
        rma_op->dtype_info.true_ub = target_dtp->true_ub;
        rma_op->dtype_info.true_lb = target_dtp->true_lb;
        rma_op->dtype_info.has_sticky_ub = target_dtp->has_sticky_ub;
        rma_op->dtype_info.has_sticky_lb = target_dtp->has_sticky_lb;

        MPIU_CHKPMEM_MALLOC(rma_op->dataloop, void *, target_dtp->dataloop_size,
                            mpi_errno, "dataloop");

        MPIDI_FUNC_ENTER(MPID_STATE_MEMCPY);
        MPIU_Memcpy(rma_op->dataloop, target_dtp->dataloop, target_dtp->dataloop_size);
        MPIDI_FUNC_EXIT(MPID_STATE_MEMCPY);
        /* the dataloop can have undefined padding sections, so we need to let
         * valgrind know that it is OK to pass this data to writev later on */
        MPL_VG_MAKE_MEM_DEFINED(rma_op->dataloop, target_dtp->dataloop_size);

        if (rma_op->pkt.type == MPIDI_CH3_PKT_PUT) {
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
            mpi_errno = MPIDI_CH3_iStartMsgv(vc, iov, iovcnt, &rma_op->request);
            MPIU_THREAD_CS_EXIT(CH3COMM, vc);
            MPIU_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");
        }
        else {
            /* derived datatype on origin */
            rma_op->request = MPID_Request_create();
            MPIU_ERR_CHKANDJUMP(rma_op->request == NULL, mpi_errno, MPI_ERR_OTHER, "**nomemreq");

            MPIU_Object_set_ref(rma_op->request, 2);
            rma_op->request->kind = MPID_REQUEST_SEND;

            rma_op->request->dev.segment_ptr = MPID_Segment_alloc();
            MPIU_ERR_CHKANDJUMP1(rma_op->request->dev.segment_ptr == NULL, mpi_errno, MPI_ERR_OTHER,
                                 "**nomem", "**nomem %s", "MPID_Segment_alloc");

            rma_op->request->dev.datatype_ptr = origin_dtp;
            /* this will cause the datatype to be freed when the request
             * is freed. */
            MPID_Segment_init(rma_op->origin_addr, rma_op->origin_count,
                              rma_op->origin_datatype, rma_op->request->dev.segment_ptr, 0);
            rma_op->request->dev.segment_first = 0;
            rma_op->request->dev.segment_size = rma_op->origin_count * origin_type_size;

            rma_op->request->dev.OnFinal = 0;
            rma_op->request->dev.OnDataAvail = 0;

            MPIU_THREAD_CS_ENTER(CH3COMM, vc);
            mpi_errno =
                vc->sendNoncontig_fn(vc, rma_op->request, iov[0].MPID_IOV_BUF, iov[0].MPID_IOV_LEN);
            MPIU_THREAD_CS_EXIT(CH3COMM, vc);
            MPIU_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");
        }
    }
    else {
        /* derived datatype on target */
        MPID_Datatype *combined_dtp = NULL;

        rma_op->request = MPID_Request_create();
        if (rma_op->request == NULL) {
            MPIU_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**nomemreq");
        }

        MPIU_Object_set_ref(rma_op->request, 2);
        rma_op->request->kind = MPID_REQUEST_SEND;

        rma_op->request->dev.segment_ptr = MPID_Segment_alloc();
        MPIU_ERR_CHKANDJUMP1(rma_op->request->dev.segment_ptr == NULL, mpi_errno, MPI_ERR_OTHER,
                             "**nomem", "**nomem %s", "MPID_Segment_alloc");

        /* create a new datatype containing the dtype_info, dataloop, and origin data */

        mpi_errno =
            create_datatype(&rma_op->dtype_info, rma_op->dataloop, target_dtp->dataloop_size,
                            rma_op->origin_addr, rma_op->origin_count, rma_op->origin_datatype,
                            &combined_dtp);
        if (mpi_errno)
            MPIU_ERR_POP(mpi_errno);

        rma_op->request->dev.datatype_ptr = combined_dtp;
        /* combined_datatype will be freed when request is freed */

        MPID_Segment_init(MPI_BOTTOM, 1, combined_dtp->handle, rma_op->request->dev.segment_ptr, 0);
        rma_op->request->dev.segment_first = 0;
        rma_op->request->dev.segment_size = combined_dtp->size;

        rma_op->request->dev.OnFinal = 0;
        rma_op->request->dev.OnDataAvail = 0;

        MPIU_THREAD_CS_ENTER(CH3COMM, vc);
        mpi_errno =
            vc->sendNoncontig_fn(vc, rma_op->request, iov[0].MPID_IOV_BUF, iov[0].MPID_IOV_LEN);
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
        if (rma_op->request != NULL) {
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

            MPID_Request_release(rma_op->request);
            rma_op->request = resp_req;

        }
        else {
            rma_op->request = resp_req;
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
    if (rma_op->request) {
        MPIU_CHKPMEM_REAP();
        if (rma_op->request->dev.datatype_ptr)
            MPID_Datatype_release(rma_op->request->dev.datatype_ptr);
        MPID_Request_release(rma_op->request);
    }
    rma_op->request = NULL;
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
                               MPID_Win * win_ptr, MPIDI_CH3_Pkt_flags_t flags)
{
    MPIDI_CH3_Pkt_accum_t *accum_pkt = &rma_op->pkt.accum;
    MPID_IOV iov[MPID_IOV_LIMIT];
    int mpi_errno = MPI_SUCCESS;
    int iovcnt;
    MPI_Aint origin_type_size;
    MPIDI_VC_t *vc;
    MPID_Comm *comm_ptr;
    size_t len;
    MPIDI_STATE_DECL(MPID_STATE_SEND_CONTIG_ACC_MSG);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_SEND_CONTIG_ACC_MSG);

    rma_op->request = NULL;

    MPID_Datatype_get_size_macro(rma_op->origin_datatype, origin_type_size);
    /* FIXME: Make this size check efficient and match the packet type */
    MPIU_Assign_trunc(len, rma_op->origin_count * origin_type_size, size_t);
    if (MPIR_CVAR_CH3_RMA_ACC_IMMED && len <= MPIDI_RMA_IMMED_INTS * sizeof(int)) {
        MPIDI_CH3_Pkt_accum_immed_t *accumi_pkt = &rma_op->pkt.accum_immed;
        void *dest = accumi_pkt->data, *src = rma_op->origin_addr;

        accumi_pkt->flags = flags;

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
        mpi_errno = MPIDI_CH3_iStartMsg(vc, accumi_pkt, sizeof(*accumi_pkt), &rma_op->request);
        MPIU_THREAD_CS_EXIT(CH3COMM, vc);
        MPIU_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");
        goto fn_exit;
    }

    accum_pkt->flags = flags;

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
    mpi_errno = MPIDI_CH3_iStartMsgv(vc, iov, iovcnt, &rma_op->request);
    MPIU_THREAD_CS_EXIT(CH3COMM, vc);
    MPIU_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");

  fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_SEND_CONTIG_ACC_MSG);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    if (rma_op->request) {
        MPID_Request_release(rma_op->request);
    }
    rma_op->request = NULL;
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
                              MPID_Win * win_ptr, MPIDI_CH3_Pkt_flags_t flags)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Request *rmw_req = NULL;
    MPIDI_VC_t *vc;
    MPID_Comm *comm_ptr;
    MPI_Aint len;
    MPIDI_STATE_DECL(MPID_STATE_SEND_IMMED_RMW_MSG);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_SEND_IMMED_RMW_MSG);

    /* Create a request for the RMW response.  Store the origin buf, count, and
     * datatype in it, and pass the request's handle RMW packet. When the
     * response comes from the target, it will contain the request handle. */
    rma_op->request = MPID_Request_create();
    MPIU_ERR_CHKANDJUMP(rma_op->request == NULL, mpi_errno, MPI_ERR_OTHER, "**nomemreq");

    /* Set refs on the request to 2: one for the response message, and one for
     * the partial completion handler */
    MPIU_Object_set_ref(rma_op->request, 2);

    rma_op->request->dev.user_buf = rma_op->result_addr;
    rma_op->request->dev.user_count = rma_op->result_count;
    rma_op->request->dev.datatype = rma_op->result_datatype;

    /* REQUIRE: All datatype arguments must be of the same, builtin
     * type and counts must be 1. */
    MPID_Datatype_get_size_macro(rma_op->origin_datatype, len);
    comm_ptr = win_ptr->comm_ptr;

    if (rma_op->pkt.type == MPIDI_CH3_PKT_CAS) {
        MPIDI_CH3_Pkt_cas_t *cas_pkt = &rma_op->pkt.cas;

        MPIU_Assert(len <= sizeof(MPIDI_CH3_CAS_Immed_u));

        rma_op->request->dev.target_win_handle = cas_pkt->target_win_handle;
        rma_op->request->dev.source_win_handle = cas_pkt->source_win_handle;

        cas_pkt->request_handle = rma_op->request->handle;
        cas_pkt->flags = flags;

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

    else if (rma_op->pkt.type == MPIDI_CH3_PKT_FOP) {
        MPIDI_CH3_Pkt_fop_t *fop_pkt = &rma_op->pkt.fop;

        MPIU_Assert(len <= sizeof(MPIDI_CH3_FOP_Immed_u));

        rma_op->request->dev.target_win_handle = fop_pkt->target_win_handle;
        rma_op->request->dev.source_win_handle = fop_pkt->source_win_handle;

        fop_pkt->request_handle = rma_op->request->handle;
        fop_pkt->flags = flags;

        if (len <= sizeof(fop_pkt->origin_data) || fop_pkt->op == MPI_NO_OP) {
            /* Embed FOP data in the packet header */
            if (fop_pkt->op != MPI_NO_OP) {
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
    if (rma_op->request) {
        MPID_Request_release(rma_op->request);
    }
    rma_op->request = NULL;
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
static int recv_rma_msg(MPIDI_RMA_Op_t * rma_op, MPID_Win * win_ptr, MPIDI_CH3_Pkt_flags_t flags)
{
    MPIDI_CH3_Pkt_get_t *get_pkt = &rma_op->pkt.get;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_VC_t *vc;
    MPID_Comm *comm_ptr;
    MPID_Datatype *dtp;
    MPI_Datatype target_datatype;
    MPID_Request *req = NULL;
    MPID_IOV iov[MPID_IOV_LIMIT];
    MPIU_CHKPMEM_DECL(1);
    MPIDI_STATE_DECL(MPID_STATE_RECV_RMA_MSG);
    MPIDI_STATE_DECL(MPID_STATE_MEMCPY);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_RECV_RMA_MSG);

    /* create a request, store the origin buf, cnt, datatype in it,
     * and pass a handle to it in the get packet. When the get
     * response comes from the target, it will contain the request
     * handle. */
    rma_op->request = MPID_Request_create();
    if (rma_op->request == NULL) {
        MPIU_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**nomemreq");
    }

    MPIU_Object_set_ref(rma_op->request, 2);

    rma_op->request->dev.user_buf = rma_op->origin_addr;
    rma_op->request->dev.user_count = rma_op->origin_count;
    rma_op->request->dev.datatype = rma_op->origin_datatype;
    rma_op->request->dev.target_win_handle = MPI_WIN_NULL;
    rma_op->request->dev.source_win_handle = get_pkt->source_win_handle;
    if (!MPIR_DATATYPE_IS_PREDEFINED(rma_op->request->dev.datatype)) {
        MPID_Datatype_get_ptr(rma_op->request->dev.datatype, dtp);
        rma_op->request->dev.datatype_ptr = dtp;
        /* this will cause the datatype to be freed when the
         * request is freed. */
    }

    get_pkt->request_handle = rma_op->request->handle;
    get_pkt->flags = flags;

/*    printf("send pkt: type %d, addr %d, count %d, base %d\n", rma_pkt->type,
           rma_pkt->addr, rma_pkt->count, win_ptr->base_addrs[rma_op->target_rank]);
    fflush(stdout);
*/

    comm_ptr = win_ptr->comm_ptr;
    MPIDI_Comm_get_vc_set_active(comm_ptr, rma_op->target_rank, &vc);

    MPIDI_CH3_PKT_RMA_GET_TARGET_DATATYPE(rma_op->pkt, target_datatype, mpi_errno);
    if (MPIR_DATATYPE_IS_PREDEFINED(target_datatype)) {
        /* basic datatype on target. simply send the get_pkt. */
        MPIU_THREAD_CS_ENTER(CH3COMM, vc);
        mpi_errno = MPIDI_CH3_iStartMsg(vc, get_pkt, sizeof(*get_pkt), &req);
        MPIU_THREAD_CS_EXIT(CH3COMM, vc);
    }
    else {
        /* derived datatype on target. fill derived datatype info and
         * send it along with get_pkt. */

        MPID_Datatype_get_ptr(target_datatype, dtp);
        rma_op->dtype_info.is_contig = dtp->is_contig;
        rma_op->dtype_info.max_contig_blocks = dtp->max_contig_blocks;
        rma_op->dtype_info.size = dtp->size;
        rma_op->dtype_info.extent = dtp->extent;
        rma_op->dtype_info.dataloop_size = dtp->dataloop_size;
        rma_op->dtype_info.dataloop_depth = dtp->dataloop_depth;
        rma_op->dtype_info.eltype = dtp->eltype;
        rma_op->dtype_info.dataloop = dtp->dataloop;
        rma_op->dtype_info.ub = dtp->ub;
        rma_op->dtype_info.lb = dtp->lb;
        rma_op->dtype_info.true_ub = dtp->true_ub;
        rma_op->dtype_info.true_lb = dtp->true_lb;
        rma_op->dtype_info.has_sticky_ub = dtp->has_sticky_ub;
        rma_op->dtype_info.has_sticky_lb = dtp->has_sticky_lb;

        MPIU_CHKPMEM_MALLOC(rma_op->dataloop, void *, dtp->dataloop_size, mpi_errno, "dataloop");

        MPIDI_FUNC_ENTER(MPID_STATE_MEMCPY);
        MPIU_Memcpy(rma_op->dataloop, dtp->dataloop, dtp->dataloop_size);
        MPIDI_FUNC_EXIT(MPID_STATE_MEMCPY);

        /* the dataloop can have undefined padding sections, so we need to let
         * valgrind know that it is OK to pass this data to writev later on */
        MPL_VG_MAKE_MEM_DEFINED(rma_op->dataloop, dtp->dataloop_size);

        get_pkt->dataloop_size = dtp->dataloop_size;

        iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST) get_pkt;
        iov[0].MPID_IOV_LEN = sizeof(*get_pkt);
        iov[1].MPID_IOV_BUF = (MPID_IOV_BUF_CAST) & rma_op->dtype_info;
        iov[1].MPID_IOV_LEN = sizeof(rma_op->dtype_info);
        iov[2].MPID_IOV_BUF = (MPID_IOV_BUF_CAST) rma_op->dataloop;
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
                            MPIDI_CH3_Pkt_flags_t flags)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_ISSUE_RMA_OP);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_ISSUE_RMA_OP);

    switch (op_ptr->pkt.type) {
    case (MPIDI_CH3_PKT_PUT):
    case (MPIDI_CH3_PKT_ACCUMULATE):
    case (MPIDI_CH3_PKT_GET_ACCUM):
        mpi_errno = send_rma_msg(op_ptr, win_ptr, flags);
        break;
    case (MPIDI_CH3_PKT_ACCUM_IMMED):
        mpi_errno = send_contig_acc_msg(op_ptr, win_ptr, flags);
        break;
    case (MPIDI_CH3_PKT_GET):
        mpi_errno = recv_rma_msg(op_ptr, win_ptr, flags);
        break;
    case (MPIDI_CH3_PKT_CAS):
    case (MPIDI_CH3_PKT_FOP):
        mpi_errno = send_immed_rmw_msg(op_ptr, win_ptr, flags);
        break;
    default:
        MPIU_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**winInvalidOp");
    }

    if (mpi_errno)
        MPIU_ERR_POP(mpi_errno);

  fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_ISSUE_RMA_OP);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

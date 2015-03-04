/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2014 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if !defined(MPID_RMA_ISSUE_H_INCLUDED)
#define MPID_RMA_ISSUE_H_INCLUDED

#include "mpl_utlist.h"
#include "mpid_rma_types.h"

/* define ACC stream size as the SRBuf size */
#define MPIDI_CH3U_Acc_stream_size MPIDI_CH3U_SRBuf_size

/* =========================================================== */
/*                    auxiliary functions                      */
/* =========================================================== */

/* immed_copy() copys data from origin buffer to
   IMMED packet header. */
#undef FUNCNAME
#define FUNCNAME immed_copy
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline int immed_copy(void *src, void *dest, size_t len)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_IMMED_COPY);

    MPIDI_FUNC_ENTER(MPID_STATE_IMMED_COPY);

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
        MPIU_Memcpy(dest, (void *) src, len);
    }

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_IMMED_COPY);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


/* fill_in_derived_dtp_info() fills derived datatype information
   into RMA operation structure. */
#undef FUNCNAME
#define FUNCNAME fill_in_derived_dtp_info
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int fill_in_derived_dtp_info(MPIDI_RMA_Op_t * rma_op, MPID_Datatype * dtp)
{
    int mpi_errno = MPI_SUCCESS;
    MPIU_CHKPMEM_DECL(1);
    MPIDI_STATE_DECL(MPID_STATE_FILL_IN_DERIVED_DTP_INFO);

    MPIDI_FUNC_ENTER(MPID_STATE_FILL_IN_DERIVED_DTP_INFO);

    /* Derived datatype on target, fill derived datatype info. */
    rma_op->dtype_info.is_contig = dtp->is_contig;
    rma_op->dtype_info.max_contig_blocks = dtp->max_contig_blocks;
    rma_op->dtype_info.size = dtp->size;
    rma_op->dtype_info.extent = dtp->extent;
    rma_op->dtype_info.dataloop_size = dtp->dataloop_size;
    rma_op->dtype_info.dataloop_depth = dtp->dataloop_depth;
    rma_op->dtype_info.basic_type = dtp->basic_type;
    rma_op->dtype_info.dataloop = dtp->dataloop;
    rma_op->dtype_info.ub = dtp->ub;
    rma_op->dtype_info.lb = dtp->lb;
    rma_op->dtype_info.true_ub = dtp->true_ub;
    rma_op->dtype_info.true_lb = dtp->true_lb;
    rma_op->dtype_info.has_sticky_ub = dtp->has_sticky_ub;
    rma_op->dtype_info.has_sticky_lb = dtp->has_sticky_lb;

    MPIU_Assert(rma_op->dataloop == NULL);
    MPIU_CHKPMEM_MALLOC(rma_op->dataloop, void *, dtp->dataloop_size, mpi_errno, "dataloop");

    MPIU_Memcpy(rma_op->dataloop, dtp->dataloop, dtp->dataloop_size);
    /* The dataloop can have undefined padding sections, so we need to let
     * valgrind know that it is OK to pass this data to writev later on. */
    MPL_VG_MAKE_MEM_DEFINED(rma_op->dataloop, dtp->dataloop_size);

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_FILL_IN_DERIVED_DTP_INFO);
    MPIU_CHKPMEM_COMMIT();
    return mpi_errno;
  fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME create_datatype
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int create_datatype(int *ints, MPI_Aint * displaces, MPI_Datatype * datatypes,
                           MPID_Datatype ** combined_dtp)
{
    int mpi_errno = MPI_SUCCESS;
    /* datatype_set_contents wants an array 'ints' which is the
     * blocklens array with count prepended to it.  So blocklens
     * points to the 2nd element of ints to avoid having to copy
     * blocklens into ints later. */
    int *blocklens = &ints[1];
    MPI_Datatype combined_datatype;
    int count = ints[0];
    MPIDI_STATE_DECL(MPID_STATE_CREATE_DATATYPE);

    MPIDI_FUNC_ENTER(MPID_STATE_CREATE_DATATYPE);

    mpi_errno = MPID_Type_struct(count, blocklens, displaces, datatypes, &combined_datatype);
    if (mpi_errno)
        MPIU_ERR_POP(mpi_errno);

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


/* =========================================================== */
/*                      issuinng functions                     */
/* =========================================================== */

/* issue_from_origin_buffer() issues data from origin
   buffer (i.e. non-IMMED operation). */
#undef FUNCNAME
#define FUNCNAME issue_from_origin_buffer
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int issue_from_origin_buffer(MPIDI_RMA_Op_t * rma_op, MPIDI_VC_t * vc,
                                    MPID_Request ** req_ptr)
{
    MPI_Aint origin_type_size;
    MPI_Datatype target_datatype;
    MPID_Datatype *target_dtp = NULL, *origin_dtp = NULL;
    int is_origin_contig;
    MPID_IOV iov[MPID_IOV_LIMIT];
    MPID_Request *req = NULL;
    int count;
    int *ints = NULL;
    int *blocklens = NULL;
    MPI_Aint *displaces = NULL;
    MPI_Datatype *datatypes = NULL;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_ISSUE_FROM_ORIGIN_BUFFER);

    MPIDI_FUNC_ENTER(MPID_STATE_ISSUE_FROM_ORIGIN_BUFFER);

    /* Judge if target datatype is derived datatype. */
    MPIDI_CH3_PKT_RMA_GET_TARGET_DATATYPE(rma_op->pkt, target_datatype, mpi_errno);
    if (!MPIR_DATATYPE_IS_PREDEFINED(target_datatype)) {
        MPID_Datatype_get_ptr(target_datatype, target_dtp);

        /* Fill derived datatype info. */
        mpi_errno = fill_in_derived_dtp_info(rma_op, target_dtp);
        if (mpi_errno != MPI_SUCCESS)
            MPIU_ERR_POP(mpi_errno);

        /* Set dataloop size in pkt header */
        MPIDI_CH3_PKT_RMA_SET_DATALOOP_SIZE(rma_op->pkt, target_dtp->dataloop_size, mpi_errno);
    }

    /* Judge if origin datatype is derived datatype. */
    if (!MPIR_DATATYPE_IS_PREDEFINED(rma_op->origin_datatype)) {
        MPID_Datatype_get_ptr(rma_op->origin_datatype, origin_dtp);
    }

    MPID_Datatype_get_size_macro(rma_op->origin_datatype, origin_type_size);
    MPID_Datatype_is_contig(rma_op->origin_datatype, &is_origin_contig);

    iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST) & (rma_op->pkt);
    iov[0].MPID_IOV_LEN = sizeof(rma_op->pkt);

    if (target_dtp == NULL) {
        /* basic datatype on target */
        if (is_origin_contig) {
            /* basic datatype on origin */
            int iovcnt = 2;

            iov[1].MPID_IOV_BUF = (MPID_IOV_BUF_CAST) ((char *) rma_op->origin_addr);
            iov[1].MPID_IOV_LEN = rma_op->origin_count * origin_type_size;

            MPIU_THREAD_CS_ENTER(CH3COMM, vc);
            mpi_errno = MPIDI_CH3_iStartMsgv(vc, iov, iovcnt, &req);
            MPIU_THREAD_CS_EXIT(CH3COMM, vc);
            MPIU_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");

            if (origin_dtp != NULL) {
                if (req == NULL) {
                    MPID_Datatype_release(origin_dtp);
                }
                else {
                    /* this will cause the datatype to be freed when the request
                     * is freed. */
                    req->dev.datatype_ptr = origin_dtp;
                }
            }
        }
        else {
            /* derived datatype on origin */
            req = MPID_Request_create();
            MPIU_ERR_CHKANDJUMP(req == NULL, mpi_errno, MPI_ERR_OTHER, "**nomemreq");

            MPIU_Object_set_ref(req, 2);
            req->kind = MPID_REQUEST_SEND;

            req->dev.segment_ptr = MPID_Segment_alloc();
            MPIU_ERR_CHKANDJUMP1(req->dev.segment_ptr == NULL, mpi_errno,
                                 MPI_ERR_OTHER, "**nomem", "**nomem %s", "MPID_Segment_alloc");

            if (origin_dtp != NULL) {
                req->dev.datatype_ptr = origin_dtp;
                /* this will cause the datatype to be freed when the request
                 * is freed. */
            }
            MPID_Segment_init(rma_op->origin_addr, rma_op->origin_count,
                              rma_op->origin_datatype, req->dev.segment_ptr, 0);
            req->dev.segment_first = 0;
            req->dev.segment_size = rma_op->origin_count * origin_type_size;

            req->dev.OnFinal = 0;
            req->dev.OnDataAvail = 0;

            MPIU_THREAD_CS_ENTER(CH3COMM, vc);
            mpi_errno = vc->sendNoncontig_fn(vc, req, iov[0].MPID_IOV_BUF, iov[0].MPID_IOV_LEN);
            MPIU_THREAD_CS_EXIT(CH3COMM, vc);
            MPIU_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");
        }
    }
    else {
        /* derived datatype on target */
        MPID_Datatype *combined_dtp = NULL;

        req = MPID_Request_create();
        if (req == NULL) {
            MPIU_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**nomemreq");
        }

        MPIU_Object_set_ref(req, 2);
        req->kind = MPID_REQUEST_SEND;

        req->dev.segment_ptr = MPID_Segment_alloc();
        MPIU_ERR_CHKANDJUMP1(req->dev.segment_ptr == NULL, mpi_errno, MPI_ERR_OTHER,
                             "**nomem", "**nomem %s", "MPID_Segment_alloc");

        /* create a new datatype containing the dtype_info, dataloop, and origin data */

        count = 3;
        ints = (int *) MPIU_Malloc(sizeof(int) * (count + 1));
        blocklens = &ints[1];
        displaces = (MPI_Aint *) MPIU_Malloc(sizeof(MPI_Aint) * count);
        datatypes = (MPI_Datatype *) MPIU_Malloc(sizeof(MPI_Datatype) * count);

        ints[0] = count;

        displaces[0] = MPIU_PtrToAint(&(rma_op->dtype_info));
        blocklens[0] = sizeof(MPIDI_RMA_dtype_info);
        datatypes[0] = MPI_BYTE;

        displaces[1] = MPIU_PtrToAint(rma_op->dataloop);
        MPIU_Assign_trunc(blocklens[1], target_dtp->dataloop_size, int);
        datatypes[1] = MPI_BYTE;

        displaces[2] = MPIU_PtrToAint(rma_op->origin_addr);
        blocklens[2] = rma_op->origin_count;
        datatypes[2] = rma_op->origin_datatype;

        mpi_errno = create_datatype(ints, displaces, datatypes, &combined_dtp);
        if (mpi_errno)
            MPIU_ERR_POP(mpi_errno);

        req->dev.datatype_ptr = combined_dtp;
        /* combined_datatype will be freed when request is freed */

        MPID_Segment_init(MPI_BOTTOM, 1, combined_dtp->handle, req->dev.segment_ptr, 0);
        req->dev.segment_first = 0;
        req->dev.segment_size = combined_dtp->size;

        req->dev.OnFinal = 0;
        req->dev.OnDataAvail = 0;

        MPIU_THREAD_CS_ENTER(CH3COMM, vc);
        mpi_errno = vc->sendNoncontig_fn(vc, req, iov[0].MPID_IOV_BUF, iov[0].MPID_IOV_LEN);
        MPIU_THREAD_CS_EXIT(CH3COMM, vc);
        MPIU_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");

        MPIU_Free(ints);
        MPIU_Free(displaces);
        MPIU_Free(datatypes);

        /* we're done with the datatypes */
        if (origin_dtp != NULL)
            MPID_Datatype_release(origin_dtp);
        MPID_Datatype_release(target_dtp);
    }

    (*req_ptr) = req;

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_ISSUE_FROM_ORIGIN_BUFFER);
    return mpi_errno;
  fn_fail:
    if ((*req_ptr)) {
        if ((*req_ptr)->dev.datatype_ptr)
            MPID_Datatype_release((*req_ptr)->dev.datatype_ptr);
        MPID_Request_release((*req_ptr));
    }
    (*req_ptr) = NULL;
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME issue_from_origin_buffer_stream
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int issue_from_origin_buffer_stream(MPIDI_RMA_Op_t * rma_op, MPIDI_VC_t * vc,
                                           MPIDI_msg_sz_t stream_offset, MPIDI_msg_sz_t stream_size,
                                           MPID_Request ** req_ptr)
{
    MPI_Datatype target_datatype;
    MPID_Datatype *target_dtp = NULL, *origin_dtp = NULL;
    int is_origin_contig;
    MPID_IOV iov[MPID_IOV_LIMIT];
    MPID_Request *req = NULL;
    int count;
    int *ints = NULL;
    int *blocklens = NULL;
    MPI_Aint *displaces = NULL;
    MPI_Datatype *datatypes = NULL;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_ISSUE_FROM_ORIGIN_BUFFER_STREAM);

    MPIDI_FUNC_ENTER(MPID_STATE_ISSUE_FROM_ORIGIN_BUFFER_STREAM);

    /* Judge if target datatype is derived datatype. */
    MPIDI_CH3_PKT_RMA_GET_TARGET_DATATYPE(rma_op->pkt, target_datatype, mpi_errno);
    if (!MPIR_DATATYPE_IS_PREDEFINED(target_datatype)) {
        MPID_Datatype_get_ptr(target_datatype, target_dtp);

        if (rma_op->dataloop == NULL) {
            /* Fill derived datatype info. */
            mpi_errno = fill_in_derived_dtp_info(rma_op, target_dtp);
            if (mpi_errno != MPI_SUCCESS)
                MPIU_ERR_POP(mpi_errno);

            /* Set dataloop size in pkt header */
            MPIDI_CH3_PKT_RMA_SET_DATALOOP_SIZE(rma_op->pkt, target_dtp->dataloop_size, mpi_errno);
        }
    }

    /* Judge if origin datatype is derived datatype. */
    if (!MPIR_DATATYPE_IS_PREDEFINED(rma_op->origin_datatype)) {
        MPID_Datatype_get_ptr(rma_op->origin_datatype, origin_dtp);
    }

    MPID_Datatype_is_contig(rma_op->origin_datatype, &is_origin_contig);

    iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST) & (rma_op->pkt);
    iov[0].MPID_IOV_LEN = sizeof(rma_op->pkt);

    if (target_dtp == NULL) {
        /* basic datatype on target */
        if (is_origin_contig) {
            /* basic datatype on origin */
            int iovcnt = 2;

            iov[1].MPID_IOV_BUF =
                (MPID_IOV_BUF_CAST) ((char *) rma_op->origin_addr + stream_offset);
            iov[1].MPID_IOV_LEN = stream_size;

            MPIU_THREAD_CS_ENTER(CH3COMM, vc);
            mpi_errno = MPIDI_CH3_iStartMsgv(vc, iov, iovcnt, &req);
            MPIU_THREAD_CS_EXIT(CH3COMM, vc);
            MPIU_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");

            if (origin_dtp != NULL) {
                if (req == NULL) {
                    MPID_Datatype_release(origin_dtp);
                }
                else {
                    /* this will cause the datatype to be freed when the request
                     * is freed. */
                    req->dev.datatype_ptr = origin_dtp;
                }
            }
        }
        else {
            /* derived datatype on origin */
            req = MPID_Request_create();
            MPIU_ERR_CHKANDJUMP(req == NULL, mpi_errno, MPI_ERR_OTHER, "**nomemreq");

            MPIU_Object_set_ref(req, 2);
            req->kind = MPID_REQUEST_SEND;

            req->dev.segment_ptr = MPID_Segment_alloc();
            MPIU_ERR_CHKANDJUMP1(req->dev.segment_ptr == NULL, mpi_errno,
                                 MPI_ERR_OTHER, "**nomem", "**nomem %s", "MPID_Segment_alloc");

            if (origin_dtp != NULL) {
                req->dev.datatype_ptr = origin_dtp;
                /* this will cause the datatype to be freed when the request
                 * is freed. */
            }
            MPID_Segment_init(rma_op->origin_addr, rma_op->origin_count,
                              rma_op->origin_datatype, req->dev.segment_ptr, 0);
            req->dev.segment_first = stream_offset;
            req->dev.segment_size = stream_offset + stream_size;

            req->dev.OnFinal = 0;
            req->dev.OnDataAvail = 0;

            MPIU_THREAD_CS_ENTER(CH3COMM, vc);
            mpi_errno = vc->sendNoncontig_fn(vc, req, iov[0].MPID_IOV_BUF, iov[0].MPID_IOV_LEN);
            MPIU_THREAD_CS_EXIT(CH3COMM, vc);
            MPIU_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");
        }
    }
    else {
        /* derived datatype on target */
        MPID_Datatype *combined_dtp = NULL;
        MPID_Segment *segp = NULL;
        DLOOP_VECTOR *dloop_vec = NULL;
        MPID_Datatype *dtp = NULL;
        int vec_len, i;
        MPIDI_msg_sz_t first = stream_offset;
        MPIDI_msg_sz_t last = stream_offset + stream_size;

        req = MPID_Request_create();
        if (req == NULL) {
            MPIU_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**nomemreq");
        }

        MPIU_Object_set_ref(req, 2);
        req->kind = MPID_REQUEST_SEND;

        req->dev.segment_ptr = MPID_Segment_alloc();
        MPIU_ERR_CHKANDJUMP1(req->dev.segment_ptr == NULL, mpi_errno, MPI_ERR_OTHER,
                             "**nomem", "**nomem %s", "MPID_Segment_alloc");

        /* create a new datatype containing the dtype_info, dataloop, and origin data */
        segp = MPID_Segment_alloc();
        MPIU_ERR_CHKANDJUMP1(segp == NULL, mpi_errno, MPI_ERR_OTHER,
                             "**nomem", "**nomem %s", "MPID_Segment_alloc");

        MPID_Segment_init(rma_op->origin_addr, rma_op->origin_count, rma_op->origin_datatype, segp,
                          0);

        MPID_Datatype_get_ptr(rma_op->origin_datatype, dtp);
        vec_len = dtp->max_contig_blocks * rma_op->origin_count + 1;
        dloop_vec = (DLOOP_VECTOR *) MPIU_Malloc(vec_len * sizeof(DLOOP_VECTOR));
        /* --BEGIN ERROR HANDLING-- */
        if (!dloop_vec) {
            mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                             FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
            goto fn_fail;
        }
        /* --END ERROR HANDLING-- */

        MPID_Segment_pack_vector(segp, first, &last, dloop_vec, &vec_len);

        count = 2 + vec_len;

        ints = (int *) MPIU_Malloc(sizeof(int) * (count + 1));
        blocklens = &ints[1];
        displaces = (MPI_Aint *) MPIU_Malloc(sizeof(MPI_Aint) * count);
        datatypes = (MPI_Datatype *) MPIU_Malloc(sizeof(MPI_Datatype) * count);

        ints[0] = count;

        displaces[0] = MPIU_PtrToAint(&(rma_op->dtype_info));
        blocklens[0] = sizeof(MPIDI_RMA_dtype_info);
        datatypes[0] = MPI_BYTE;

        displaces[1] = MPIU_PtrToAint(rma_op->dataloop);
        MPIU_Assign_trunc(blocklens[1], target_dtp->dataloop_size, int);
        datatypes[1] = MPI_BYTE;

        for (i = 0; i < vec_len; i++) {
            displaces[i + 2] = MPIU_PtrToAint(dloop_vec[i].DLOOP_VECTOR_BUF);
            MPIU_Assign_trunc(blocklens[i + 2], dloop_vec[i].DLOOP_VECTOR_LEN, int);
            datatypes[i + 2] = MPI_BYTE;
        }

        MPID_Segment_free(segp);
        MPIU_Free(dloop_vec);

        mpi_errno = create_datatype(ints, displaces, datatypes, &combined_dtp);
        if (mpi_errno)
            MPIU_ERR_POP(mpi_errno);

        req->dev.datatype_ptr = combined_dtp;
        /* combined_datatype will be freed when request is freed */

        MPID_Segment_init(MPI_BOTTOM, 1, combined_dtp->handle, req->dev.segment_ptr, 0);
        req->dev.segment_first = 0;
        req->dev.segment_size = combined_dtp->size;

        req->dev.OnFinal = 0;
        req->dev.OnDataAvail = 0;

        MPIU_THREAD_CS_ENTER(CH3COMM, vc);
        mpi_errno = vc->sendNoncontig_fn(vc, req, iov[0].MPID_IOV_BUF, iov[0].MPID_IOV_LEN);
        MPIU_THREAD_CS_EXIT(CH3COMM, vc);
        MPIU_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");

        MPIU_Free(ints);
        MPIU_Free(displaces);
        MPIU_Free(datatypes);

        /* we're done with the datatypes */
        if (origin_dtp != NULL)
            MPID_Datatype_release(origin_dtp);
        MPID_Datatype_release(target_dtp);
    }

    (*req_ptr) = req;

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_ISSUE_FROM_ORIGIN_BUFFER_STREAM);
    return mpi_errno;
  fn_fail:
    if ((*req_ptr)) {
        if ((*req_ptr)->dev.datatype_ptr)
            MPID_Datatype_release((*req_ptr)->dev.datatype_ptr);
        MPID_Request_release((*req_ptr));
    }
    (*req_ptr) = NULL;
    goto fn_exit;
}


/* issue_put_op() issues PUT packet header and data. */
#undef FUNCNAME
#define FUNCNAME issue_put_op
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int issue_put_op(MPIDI_RMA_Op_t * rma_op, MPID_Win * win_ptr,
                        MPIDI_RMA_Target_t * target_ptr, MPIDI_CH3_Pkt_flags_t flags)
{
    MPIDI_VC_t *vc = NULL;
    MPID_Comm *comm_ptr = win_ptr->comm_ptr;
    MPIDI_CH3_Pkt_put_t *put_pkt = &rma_op->pkt.put;
    MPID_Request *curr_req = NULL;
    int i, curr_req_index = 0;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_ISSUE_PUT_OP);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_ISSUE_PUT_OP);

    put_pkt->flags |= flags;

    MPIDI_Comm_get_vc_set_active(comm_ptr, rma_op->target_rank, &vc);

    if (rma_op->pkt.type == MPIDI_CH3_PKT_PUT_IMMED) {
        /* All origin data is in packet header, issue the header. */
        MPIU_THREAD_CS_ENTER(CH3COMM, vc);
        mpi_errno = MPIDI_CH3_iStartMsg(vc, put_pkt, sizeof(*put_pkt), &curr_req);
        MPIU_THREAD_CS_EXIT(CH3COMM, vc);
        MPIU_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");
    }
    else {
        mpi_errno = issue_from_origin_buffer(rma_op, vc, &curr_req);
        if (mpi_errno != MPI_SUCCESS)
            MPIU_ERR_POP(mpi_errno);
    }

    if (curr_req != NULL) {
        rma_op->reqs_size = 1;

        rma_op->reqs = (MPID_Request **) MPIU_Malloc(sizeof(MPID_Request *) * rma_op->reqs_size);
        for (i = 0; i < rma_op->reqs_size; i++)
            rma_op->reqs[i] = NULL;

        rma_op->reqs[curr_req_index] = curr_req;
        win_ptr->active_req_cnt++;
    }

  fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_ISSUE_PUT_OP);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    if (rma_op->reqs != NULL) {
        MPIU_Free(rma_op->reqs);
    }
    rma_op->reqs = NULL;
    rma_op->reqs_size = 0;
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


/* issue_acc_op() send ACC packet header and data. */
#undef FUNCNAME
#define FUNCNAME issue_acc_op
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int issue_acc_op(MPIDI_RMA_Op_t * rma_op, MPID_Win * win_ptr,
                        MPIDI_RMA_Target_t * target_ptr, MPIDI_CH3_Pkt_flags_t flags)
{
    MPIDI_VC_t *vc = NULL;
    MPID_Comm *comm_ptr = win_ptr->comm_ptr;
    MPIDI_CH3_Pkt_accum_t *accum_pkt = &rma_op->pkt.accum;
    int i, j;
    MPI_Aint stream_elem_count, stream_unit_count;
    MPI_Aint predefined_dtp_size, predefined_dtp_extent, predefined_dtp_count;
    MPI_Aint total_len, rest_len;
    MPI_Aint origin_dtp_size;
    MPID_Datatype *origin_dtp_ptr = NULL;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_ISSUE_ACC_OP);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_ISSUE_ACC_OP);

    MPIDI_Comm_get_vc_set_active(comm_ptr, rma_op->target_rank, &vc);

    if (rma_op->pkt.type == MPIDI_CH3_PKT_ACCUMULATE_IMMED) {
        MPID_Request *curr_req = NULL;

        accum_pkt->flags |= flags;

        /* All origin data is in packet header, issue the header. */
        MPIU_THREAD_CS_ENTER(CH3COMM, vc);
        mpi_errno = MPIDI_CH3_iStartMsg(vc, accum_pkt, sizeof(*accum_pkt), &curr_req);
        MPIU_THREAD_CS_EXIT(CH3COMM, vc);
        MPIU_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");

        if (curr_req != NULL) {
            MPIU_Assert(rma_op->reqs_size == 0 && rma_op->reqs == NULL);

            rma_op->reqs_size = 1;

            rma_op->reqs =
                (MPID_Request **) MPIU_Malloc(sizeof(MPID_Request *) * rma_op->reqs_size);
            for (i = 0; i < rma_op->reqs_size; i++)
                rma_op->reqs[i] = NULL;

            rma_op->reqs[0] = curr_req;
            win_ptr->active_req_cnt++;
        }
        goto fn_exit;
    }

    /* Get total length of origin data */
    MPID_Datatype_get_size_macro(rma_op->origin_datatype, origin_dtp_size);
    total_len = origin_dtp_size * rma_op->origin_count;

    /* Get size and count for predefined datatype elements */
    if (MPIR_DATATYPE_IS_PREDEFINED(rma_op->origin_datatype)) {
        predefined_dtp_size = origin_dtp_size;
        predefined_dtp_count = rma_op->origin_count;
        MPID_Datatype_get_extent_macro(rma_op->origin_datatype, predefined_dtp_extent);
    }
    else {
        MPID_Datatype_get_ptr(rma_op->origin_datatype, origin_dtp_ptr);
        MPIU_Assert(origin_dtp_ptr != NULL && origin_dtp_ptr->basic_type != MPI_DATATYPE_NULL);
        MPID_Datatype_get_size_macro(origin_dtp_ptr->basic_type, predefined_dtp_size);
        predefined_dtp_count = total_len / predefined_dtp_size;
        MPID_Datatype_get_extent_macro(origin_dtp_ptr->basic_type, predefined_dtp_extent);
    }
    MPIU_Assert(predefined_dtp_count > 0 && predefined_dtp_size > 0 && predefined_dtp_extent > 0);

    /* Calculate number of predefined elements in each stream unit, and
     * total number of stream units. */
    stream_elem_count = MPIDI_CH3U_Acc_stream_size / predefined_dtp_extent;
    stream_unit_count = (predefined_dtp_count - 1) / stream_elem_count + 1;
    MPIU_Assert(stream_elem_count > 0 && stream_unit_count > 0);

    rest_len = total_len;
    MPIU_Assert(rma_op->issued_stream_count >= 0);
    for (j = 0; j < stream_unit_count; j++) {
        MPIDI_msg_sz_t stream_offset, stream_size;
        MPID_Request *curr_req = NULL;

        if (j < rma_op->issued_stream_count)
            continue;

        accum_pkt->flags |= flags;

        if (j != 0) {
            accum_pkt->flags &= ~MPIDI_CH3_PKT_FLAG_RMA_LOCK_SHARED;
            accum_pkt->flags &= ~MPIDI_CH3_PKT_FLAG_RMA_LOCK_EXCLUSIVE;
        }
        if (j != stream_unit_count - 1) {
            accum_pkt->flags &= ~MPIDI_CH3_PKT_FLAG_RMA_UNLOCK;
            accum_pkt->flags &= ~MPIDI_CH3_PKT_FLAG_RMA_FLUSH;
            accum_pkt->flags &= ~MPIDI_CH3_PKT_FLAG_RMA_DECR_AT_COUNTER;
        }

        stream_offset = j * stream_elem_count * predefined_dtp_size;
        stream_size = MPIR_MIN(stream_elem_count * predefined_dtp_size, rest_len);
        rest_len -= stream_size;

        accum_pkt->info.metadata.stream_offset = stream_offset;

        mpi_errno =
            issue_from_origin_buffer_stream(rma_op, vc, stream_offset, stream_size, &curr_req);
        if (mpi_errno != MPI_SUCCESS)
            MPIU_ERR_POP(mpi_errno);

        if (curr_req != NULL) {
            if (rma_op->reqs_size == 0) {
                MPIU_Assert(rma_op->reqs == NULL);
                rma_op->reqs_size = stream_unit_count;

                rma_op->reqs =
                    (MPID_Request **) MPIU_Malloc(sizeof(MPID_Request *) * rma_op->reqs_size);
                for (i = 0; i < rma_op->reqs_size; i++)
                    rma_op->reqs[i] = NULL;
            }

            rma_op->reqs[j] = curr_req;
            win_ptr->active_req_cnt++;
        }

        rma_op->issued_stream_count++;

        if (accum_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK_SHARED ||
            accum_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK_EXCLUSIVE) {
            /* if piggybacked with LOCK flag, we
             * only issue the first streaming unit */
            MPIU_Assert(j == 0);
            break;
        }
    }   /* end of for loop */

    /* Mark that all stream units have been issued */
    rma_op->issued_stream_count = -1;

  fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_ISSUE_ACC_OP);
    return mpi_errno;
  fn_fail:
    if (rma_op->reqs != NULL) {
        MPIU_Free(rma_op->reqs);
    }
    rma_op->reqs = NULL;
    rma_op->reqs_size = 0;
    goto fn_exit;
}


/* issue_get_acc_op() send GACC packet header and data. */
#undef FUNCNAME
#define FUNCNAME issue_get_acc_op
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int issue_get_acc_op(MPIDI_RMA_Op_t * rma_op, MPID_Win * win_ptr,
                            MPIDI_RMA_Target_t * target_ptr, MPIDI_CH3_Pkt_flags_t flags)
{
    MPIDI_VC_t *vc = NULL;
    MPID_Comm *comm_ptr = win_ptr->comm_ptr;
    MPIDI_CH3_Pkt_get_accum_t *get_accum_pkt = &rma_op->pkt.get_accum;
    int i, j;
    MPI_Aint stream_elem_count, stream_unit_count;
    MPI_Aint predefined_dtp_size, predefined_dtp_count, predefined_dtp_extent;
    MPI_Aint total_len, rest_len;
    MPI_Aint origin_dtp_size;
    MPID_Datatype *origin_dtp_ptr = NULL;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_ISSUE_GET_ACC_OP);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_ISSUE_GET_ACC_OP);

    MPIDI_Comm_get_vc_set_active(comm_ptr, rma_op->target_rank, &vc);

    if (rma_op->pkt.type == MPIDI_CH3_PKT_GET_ACCUM_IMMED) {
        MPID_Request *resp_req = NULL;
        MPID_Request *curr_req = NULL;

        get_accum_pkt->flags |= flags;

        rma_op->reqs_size = 1;

        rma_op->reqs = (MPID_Request **) MPIU_Malloc(sizeof(MPID_Request *) * rma_op->reqs_size);
        for (i = 0; i < rma_op->reqs_size; i++)
            rma_op->reqs[i] = NULL;

        /* Create a request for the GACC response.  Store the response buf, count, and
         * datatype in it, and pass the request's handle in the GACC packet. When the
         * response comes from the target, it will contain the request handle. */
        resp_req = MPID_Request_create();
        MPIU_ERR_CHKANDJUMP(resp_req == NULL, mpi_errno, MPI_ERR_OTHER, "**nomemreq");

        MPIU_Object_set_ref(resp_req, 2);

        resp_req->dev.user_buf = rma_op->result_addr;
        resp_req->dev.user_count = rma_op->result_count;
        resp_req->dev.datatype = rma_op->result_datatype;
        resp_req->dev.target_win_handle = MPI_WIN_NULL;
        resp_req->dev.source_win_handle = win_ptr->handle;

        /* Note: Get_accumulate uses the same packet type as accumulate */
        get_accum_pkt->request_handle = resp_req->handle;

        /* All origin data is in packet header, issue the header. */
        MPIU_THREAD_CS_ENTER(CH3COMM, vc);
        mpi_errno = MPIDI_CH3_iStartMsg(vc, get_accum_pkt, sizeof(*get_accum_pkt), &curr_req);
        MPIU_THREAD_CS_EXIT(CH3COMM, vc);
        MPIU_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");

        if (curr_req != NULL) {
            MPID_Request_release(curr_req);
            curr_req = resp_req;
        }
        else {
            curr_req = resp_req;
        }

        /* For error checking */
        resp_req = NULL;

        rma_op->reqs[0] = curr_req;
        win_ptr->active_req_cnt++;

        goto fn_exit;
    }

    /* Get total length of origin data */
    MPID_Datatype_get_size_macro(rma_op->origin_datatype, origin_dtp_size);
    total_len = origin_dtp_size * rma_op->origin_count;

    /* Get size and count for predefined datatype elements */
    if (MPIR_DATATYPE_IS_PREDEFINED(rma_op->origin_datatype)) {
        predefined_dtp_size = origin_dtp_size;
        predefined_dtp_count = rma_op->origin_count;
        MPID_Datatype_get_extent_macro(rma_op->origin_datatype, predefined_dtp_extent);
    }
    else {
        MPID_Datatype_get_ptr(rma_op->origin_datatype, origin_dtp_ptr);
        MPIU_Assert(origin_dtp_ptr != NULL && origin_dtp_ptr->basic_type != MPI_DATATYPE_NULL);
        MPID_Datatype_get_size_macro(origin_dtp_ptr->basic_type, predefined_dtp_size);
        predefined_dtp_count = total_len / predefined_dtp_size;
        MPID_Datatype_get_extent_macro(origin_dtp_ptr->basic_type, predefined_dtp_extent);
    }
    MPIU_Assert(predefined_dtp_count > 0 && predefined_dtp_size > 0 && predefined_dtp_extent > 0);

    /* Calculate number of predefined elements in each stream unit, and
     * total number of stream units. */
    stream_elem_count = MPIDI_CH3U_Acc_stream_size / predefined_dtp_extent;
    stream_unit_count = (predefined_dtp_count - 1) / stream_elem_count + 1;
    MPIU_Assert(stream_elem_count > 0 && stream_unit_count > 0);

    rest_len = total_len;

    rma_op->reqs_size = stream_unit_count;

    rma_op->reqs = (MPID_Request **) MPIU_Malloc(sizeof(MPID_Request *) * rma_op->reqs_size);
    for (i = 0; i < rma_op->reqs_size; i++)
        rma_op->reqs[i] = NULL;

    MPIU_Assert(rma_op->issued_stream_count >= 0);

    for (j = 0; j < stream_unit_count; j++) {
        MPIDI_msg_sz_t stream_offset, stream_size;
        MPID_Request *resp_req = NULL;
        MPID_Request *curr_req = NULL;

        if (j < rma_op->issued_stream_count)
            continue;

        get_accum_pkt->flags |= flags;

        if (j != 0) {
            get_accum_pkt->flags &= ~MPIDI_CH3_PKT_FLAG_RMA_LOCK_SHARED;
            get_accum_pkt->flags &= ~MPIDI_CH3_PKT_FLAG_RMA_LOCK_EXCLUSIVE;
        }
        if (j != stream_unit_count - 1) {
            get_accum_pkt->flags &= ~MPIDI_CH3_PKT_FLAG_RMA_UNLOCK;
            get_accum_pkt->flags &= ~MPIDI_CH3_PKT_FLAG_RMA_FLUSH;
            get_accum_pkt->flags &= ~MPIDI_CH3_PKT_FLAG_RMA_DECR_AT_COUNTER;
        }

        /* Create a request for the GACC response.  Store the response buf, count, and
         * datatype in it, and pass the request's handle in the GACC packet. When the
         * response comes from the target, it will contain the request handle. */
        resp_req = MPID_Request_create();
        MPIU_ERR_CHKANDJUMP(resp_req == NULL, mpi_errno, MPI_ERR_OTHER, "**nomemreq");

        MPIU_Object_set_ref(resp_req, 2);

        resp_req->dev.user_buf = rma_op->result_addr;
        resp_req->dev.user_count = rma_op->result_count;
        resp_req->dev.datatype = rma_op->result_datatype;
        resp_req->dev.target_win_handle = MPI_WIN_NULL;
        resp_req->dev.source_win_handle = win_ptr->handle;

        if (!MPIR_DATATYPE_IS_PREDEFINED(resp_req->dev.datatype)) {
            MPID_Datatype *result_dtp = NULL;
            MPID_Datatype_get_ptr(resp_req->dev.datatype, result_dtp);
            resp_req->dev.datatype_ptr = result_dtp;
            /* this will cause the datatype to be freed when the
             * request is freed. */
        }

        /* Note: Get_accumulate uses the same packet type as accumulate */
        get_accum_pkt->request_handle = resp_req->handle;

        stream_offset = j * stream_elem_count * predefined_dtp_size;
        stream_size = MPIR_MIN(stream_elem_count * predefined_dtp_size, rest_len);
        rest_len -= stream_size;

        get_accum_pkt->info.metadata.stream_offset = stream_offset;

        resp_req->dev.stream_offset = stream_offset;

        mpi_errno =
            issue_from_origin_buffer_stream(rma_op, vc, stream_offset, stream_size, &curr_req);
        if (mpi_errno != MPI_SUCCESS)
            MPIU_ERR_POP(mpi_errno);

        /* This operation can generate two requests; one for inbound and one for
         * outbound data. */
        if (curr_req != NULL) {
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

            MPID_Request_release(curr_req);
            curr_req = resp_req;
        }
        else {
            curr_req = resp_req;
        }

        /* For error checking */
        resp_req = NULL;

        rma_op->reqs[j] = curr_req;
        win_ptr->active_req_cnt++;

        rma_op->issued_stream_count++;

        if (get_accum_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK_SHARED ||
            get_accum_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK_EXCLUSIVE) {
            /* if piggybacked with LOCK flag, we
             * only issue the first streaming unit */
            MPIU_Assert(j == 0);
            break;
        }
    }   /* end of for loop */

    /* Mark that all stream units have been issued */
    rma_op->issued_stream_count = -1;

  fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_ISSUE_GET_ACC_OP);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    for (i = 0; i < rma_op->reqs_size; i++) {
        if (rma_op->reqs[i] != NULL) {
            MPIDI_CH3_Request_destroy(rma_op->reqs[i]);
        }
    }
    if (rma_op->reqs != NULL) {
        MPIU_Free(rma_op->reqs);
    }
    rma_op->reqs = NULL;
    rma_op->reqs_size = 0;
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME issue_get_op
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int issue_get_op(MPIDI_RMA_Op_t * rma_op, MPID_Win * win_ptr,
                        MPIDI_RMA_Target_t * target_ptr, MPIDI_CH3_Pkt_flags_t flags)
{
    MPIDI_CH3_Pkt_get_t *get_pkt = &rma_op->pkt.get;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_VC_t *vc;
    MPID_Comm *comm_ptr;
    MPID_Datatype *dtp;
    MPI_Datatype target_datatype;
    MPID_Request *req = NULL;
    MPID_Request *curr_req = NULL;
    int i, curr_req_index = 0;
    MPID_IOV iov[MPID_IOV_LIMIT];
    MPIDI_STATE_DECL(MPID_STATE_ISSUE_GET_OP);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_ISSUE_GET_OP);

    rma_op->reqs_size = 1;

    rma_op->reqs = (MPID_Request **) MPIU_Malloc(sizeof(MPID_Request *) * rma_op->reqs_size);
    for (i = 0; i < rma_op->reqs_size; i++)
        rma_op->reqs[i] = NULL;

    /* create a request, store the origin buf, cnt, datatype in it,
     * and pass a handle to it in the get packet. When the get
     * response comes from the target, it will contain the request
     * handle. */
    curr_req = MPID_Request_create();
    if (curr_req == NULL) {
        MPIU_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**nomemreq");
    }

    MPIU_Object_set_ref(curr_req, 2);

    curr_req->dev.user_buf = rma_op->origin_addr;
    curr_req->dev.user_count = rma_op->origin_count;
    curr_req->dev.datatype = rma_op->origin_datatype;
    curr_req->dev.target_win_handle = MPI_WIN_NULL;
    curr_req->dev.source_win_handle = win_ptr->handle;
    if (!MPIR_DATATYPE_IS_PREDEFINED(curr_req->dev.datatype)) {
        MPID_Datatype_get_ptr(curr_req->dev.datatype, dtp);
        curr_req->dev.datatype_ptr = dtp;
        /* this will cause the datatype to be freed when the
         * request is freed. */
    }

    get_pkt->request_handle = curr_req->handle;

    get_pkt->flags |= flags;

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

        mpi_errno = fill_in_derived_dtp_info(rma_op, dtp);
        if (mpi_errno != MPI_SUCCESS)
            MPIU_ERR_POP(mpi_errno);

        /* Set dataloop size in pkt header */
        MPIDI_CH3_PKT_RMA_SET_DATALOOP_SIZE(rma_op->pkt, dtp->dataloop_size, mpi_errno);

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

    rma_op->reqs[curr_req_index] = curr_req;
    win_ptr->active_req_cnt++;

  fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_ISSUE_GET_OP);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    for (i = 0; i < rma_op->reqs_size; i++) {
        if (rma_op->reqs[i] != NULL) {
            MPIDI_CH3_Request_destroy(rma_op->reqs[i]);
        }
    }
    if (rma_op->reqs != NULL) {
        MPIU_Free(rma_op->reqs);
    }
    rma_op->reqs = NULL;
    rma_op->reqs_size = 0;
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME issue_cas_op
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int issue_cas_op(MPIDI_RMA_Op_t * rma_op,
                        MPID_Win * win_ptr, MPIDI_RMA_Target_t * target_ptr,
                        MPIDI_CH3_Pkt_flags_t flags)
{
    MPIDI_VC_t *vc = NULL;
    MPID_Comm *comm_ptr = win_ptr->comm_ptr;
    MPIDI_CH3_Pkt_cas_t *cas_pkt = &rma_op->pkt.cas;
    MPID_Request *rmw_req = NULL;
    MPID_Request *curr_req = NULL;
    int i, curr_req_index = 0;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_ISSUE_CAS_OP);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_ISSUE_CAS_OP);

    rma_op->reqs_size = 1;

    rma_op->reqs = (MPID_Request **) MPIU_Malloc(sizeof(MPID_Request *) * rma_op->reqs_size);
    for (i = 0; i < rma_op->reqs_size; i++)
        rma_op->reqs[i] = NULL;

    /* Create a request for the RMW response.  Store the origin buf, count, and
     * datatype in it, and pass the request's handle RMW packet. When the
     * response comes from the target, it will contain the request handle. */
    curr_req = MPID_Request_create();
    MPIU_ERR_CHKANDJUMP(curr_req == NULL, mpi_errno, MPI_ERR_OTHER, "**nomemreq");

    /* Set refs on the request to 2: one for the response message, and one for
     * the partial completion handler */
    MPIU_Object_set_ref(curr_req, 2);

    curr_req->dev.user_buf = rma_op->result_addr;
    curr_req->dev.datatype = rma_op->result_datatype;

    curr_req->dev.target_win_handle = MPI_WIN_NULL;
    curr_req->dev.source_win_handle = win_ptr->handle;

    cas_pkt->request_handle = curr_req->handle;
    cas_pkt->flags |= flags;

    MPIDI_Comm_get_vc_set_active(comm_ptr, rma_op->target_rank, &vc);
    MPIU_THREAD_CS_ENTER(CH3COMM, vc);
    mpi_errno = MPIDI_CH3_iStartMsg(vc, cas_pkt, sizeof(*cas_pkt), &rmw_req);
    MPIU_THREAD_CS_EXIT(CH3COMM, vc);
    MPIU_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");

    if (rmw_req != NULL) {
        MPID_Request_release(rmw_req);
    }

    rma_op->reqs[curr_req_index] = curr_req;
    win_ptr->active_req_cnt++;

  fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_ISSUE_CAS_OP);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    for (i = 0; i < rma_op->reqs_size; i++) {
        if (rma_op->reqs[i] != NULL) {
            MPIDI_CH3_Request_destroy(rma_op->reqs[i]);
        }
    }
    if (rma_op->reqs != NULL) {
        MPIU_Free(rma_op->reqs);
    }
    rma_op->reqs = NULL;
    rma_op->reqs_size = 0;
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME issue_fop_op
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int issue_fop_op(MPIDI_RMA_Op_t * rma_op,
                        MPID_Win * win_ptr, MPIDI_RMA_Target_t * target_ptr,
                        MPIDI_CH3_Pkt_flags_t flags)
{
    MPIDI_VC_t *vc = NULL;
    MPID_Comm *comm_ptr = win_ptr->comm_ptr;
    MPIDI_CH3_Pkt_fop_t *fop_pkt = &rma_op->pkt.fop;
    MPID_Request *resp_req = NULL;
    MPID_Request *curr_req = NULL;
    int i, curr_req_index = 0;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_ISSUE_FOP_OP);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_ISSUE_FOP_OP);

    rma_op->reqs_size = 1;

    rma_op->reqs = (MPID_Request **) MPIU_Malloc(sizeof(MPID_Request *) * rma_op->reqs_size);
    for (i = 0; i < rma_op->reqs_size; i++)
        rma_op->reqs[i] = NULL;

    /* Create a request for the GACC response.  Store the response buf, count, and
     * datatype in it, and pass the request's handle in the GACC packet. When the
     * response comes from the target, it will contain the request handle. */
    resp_req = MPID_Request_create();
    MPIU_ERR_CHKANDJUMP(resp_req == NULL, mpi_errno, MPI_ERR_OTHER, "**nomemreq");

    MPIU_Object_set_ref(resp_req, 2);

    resp_req->dev.user_buf = rma_op->result_addr;
    resp_req->dev.datatype = rma_op->result_datatype;
    resp_req->dev.target_win_handle = MPI_WIN_NULL;
    resp_req->dev.source_win_handle = win_ptr->handle;

    fop_pkt->request_handle = resp_req->handle;

    fop_pkt->flags |= flags;

    MPIDI_Comm_get_vc_set_active(comm_ptr, rma_op->target_rank, &vc);

    if (rma_op->pkt.type == MPIDI_CH3_PKT_FOP_IMMED) {
        /* All origin data is in packet header, issue the header. */
        MPIU_THREAD_CS_ENTER(CH3COMM, vc);
        mpi_errno = MPIDI_CH3_iStartMsg(vc, fop_pkt, sizeof(*fop_pkt), &curr_req);
        MPIU_THREAD_CS_EXIT(CH3COMM, vc);
        MPIU_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");
    }
    else {
        mpi_errno = issue_from_origin_buffer(rma_op, vc, &curr_req);
        if (mpi_errno != MPI_SUCCESS)
            MPIU_ERR_POP(mpi_errno);
    }

    /* This operation can generate two requests; one for inbound and one for
     * outbound data. */
    if (curr_req != NULL) {
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

        MPID_Request_release(curr_req);
        curr_req = resp_req;
    }
    else {
        curr_req = resp_req;
    }

    /* For error checking */
    resp_req = NULL;

    rma_op->reqs[curr_req_index] = curr_req;
    win_ptr->active_req_cnt++;

  fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_ISSUE_FOP_OP);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    for (i = 0; i < rma_op->reqs_size; i++) {
        if (rma_op->reqs[i] != NULL) {
            MPIDI_CH3_Request_destroy(rma_op->reqs[i]);
        }
    }
    if (rma_op->reqs != NULL) {
        MPIU_Free(rma_op->reqs);
    }
    rma_op->reqs = NULL;
    rma_op->reqs_size = 0;
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


/* issue_rma_op() is called by ch3u_rma_oplist.c, it triggers
   proper issuing functions according to packet type. */
#undef FUNCNAME
#define FUNCNAME issue_rma_op
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline int issue_rma_op(MPIDI_RMA_Op_t * op_ptr, MPID_Win * win_ptr,
                               MPIDI_RMA_Target_t * target_ptr, MPIDI_CH3_Pkt_flags_t flags)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_ISSUE_RMA_OP);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_ISSUE_RMA_OP);

    switch (op_ptr->pkt.type) {
    case (MPIDI_CH3_PKT_PUT):
    case (MPIDI_CH3_PKT_PUT_IMMED):
        mpi_errno = issue_put_op(op_ptr, win_ptr, target_ptr, flags);
        break;
    case (MPIDI_CH3_PKT_ACCUMULATE):
    case (MPIDI_CH3_PKT_ACCUMULATE_IMMED):
        mpi_errno = issue_acc_op(op_ptr, win_ptr, target_ptr, flags);
        break;
    case (MPIDI_CH3_PKT_GET_ACCUM):
    case (MPIDI_CH3_PKT_GET_ACCUM_IMMED):
        mpi_errno = issue_get_acc_op(op_ptr, win_ptr, target_ptr, flags);
        break;
    case (MPIDI_CH3_PKT_GET):
        mpi_errno = issue_get_op(op_ptr, win_ptr, target_ptr, flags);
        break;
    case (MPIDI_CH3_PKT_CAS_IMMED):
        mpi_errno = issue_cas_op(op_ptr, win_ptr, target_ptr, flags);
        break;
    case (MPIDI_CH3_PKT_FOP):
    case (MPIDI_CH3_PKT_FOP_IMMED):
        mpi_errno = issue_fop_op(op_ptr, win_ptr, target_ptr, flags);
        break;
    default:
        MPIU_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**winInvalidOp");
    }

    if (mpi_errno)
        MPIU_ERR_POP(mpi_errno);

  fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_ISSUE_RMA_OP);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

#undef FUNCNAME
#define FUNCNAME set_user_req_after_issuing_op
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline int set_user_req_after_issuing_op(MPIDI_RMA_Op_t * op)
{
    int i, incomplete_req_cnt = 0;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_SET_USER_REQ_AFTER_ISSUING_OP);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_SET_USER_REQ_AFTER_ISSUING_OP);

    if (op->ureq == NULL)
        goto fn_exit;

    if (op->reqs_size == 0) {
        MPIU_Assert(op->reqs == NULL);
        /* Sending is completed immediately, complete user request
         * and release ch3 ref. */

        /* Complete user request and release the ch3 ref */
        MPID_Request_set_completed(op->ureq);
        MPID_Request_release(op->ureq);
    }
    else {
        /* Sending is not completed immediately. */

        for (i = 0; i < op->reqs_size; i++) {
            if (op->reqs[i] == NULL || MPID_Request_is_complete(op->reqs[i]))
                continue;

            /* Setup user request info in order to be completed following send request. */
            incomplete_req_cnt++;
            MPID_cc_set(&(op->ureq->cc), incomplete_req_cnt);   /* increment CC counter */

            op->reqs[i]->dev.request_handle = op->ureq->handle;

            /* Setup user request completion handler.
             *
             * The handler is triggered when send request is completed at
             * following places:
             * - progress engine: complete PUT/ACC req.
             * - GET/GET_ACC packet handler: complete GET/GET_ACC reqs.
             *
             * We always set OnFinal which should be called when sending or
             * receiving the last segment. However, short put/acc ops are
             * issued in one packet and the lower layer only check OnDataAvail
             * so we have to set OnDataAvail as well.
             *
             * Note that a noncontig send also uses OnDataAvail to loop all
             * segments but it must be changed to OnFinal when sending the
             * last segment, so it is also correct for us.
             *
             * TODO: implement stack for overriding functions*/
            if (op->reqs[i]->dev.OnDataAvail == NULL) {
                op->reqs[i]->dev.OnDataAvail = MPIDI_CH3_ReqHandler_ReqOpsComplete;
            }
            op->reqs[i]->dev.OnFinal = MPIDI_CH3_ReqHandler_ReqOpsComplete;
        }       /* end of for loop */

        if (incomplete_req_cnt) {
            /* Increase ref for completion handler */
            MPIU_Object_add_ref(op->ureq);
        }
        else {
            /* all requests are completed */
            /* Complete user request and release ch3 ref */
            MPID_Request_set_completed(op->ureq);
            MPID_Request_release(op->ureq);
            op->ureq = NULL;
        }
    }

  fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_SET_USER_REQ_AFTER_ISSUING_OP);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* MPID_RMA_ISSUE_H_INCLUDED */

/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2012 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "ptl_impl.h"
#include "rptl.h"

#undef FUNCNAME
#define FUNCNAME dequeue_req
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static void dequeue_req(const ptl_event_t *e)
{
    int found;
    MPID_Request *const rreq = e->user_ptr;
    MPI_Aint s_len, r_len;

    /* At this point we know the ME is unlinked. Invalidate the handle to
       prevent further accesses, e.g. an attempted cancel. */
    REQ_PTL(rreq)->put_me = PTL_INVALID_HANDLE;

    found = MPIDI_CH3U_Recvq_DP(rreq);
    /* an MPI_ANY_SOURCE request may have been previously removed from the
       CH3 queue by an FDP (find and dequeue posted) operation */
    if (rreq->dev.match.parts.rank != MPI_ANY_SOURCE)
        MPIU_Assert(found);

    rreq->status.MPI_ERROR = MPI_SUCCESS;
    rreq->status.MPI_SOURCE = NPTL_MATCH_GET_RANK(e->match_bits);
    rreq->status.MPI_TAG = NPTL_MATCH_GET_TAG(e->match_bits);

    MPID_Datatype_get_size_macro(rreq->dev.datatype, r_len);
    r_len *= rreq->dev.user_count;

    s_len = NPTL_HEADER_GET_LENGTH(e->hdr_data);

    if (s_len > r_len) {
        /* truncated data */
        MPIR_STATUS_SET_COUNT(rreq->status, r_len);
        MPIR_ERR_SET2(rreq->status.MPI_ERROR, MPI_ERR_TRUNCATE, "**truncate", "**truncate %d %d", s_len, r_len);
    } else {
        MPIR_STATUS_SET_COUNT(rreq->status, s_len);
    }
}

#undef FUNCNAME
#define FUNCNAME handler_recv_complete
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int handler_recv_complete(const ptl_event_t *e)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Request *const rreq = e->user_ptr;
    int ret;
    int i;
    MPIDI_STATE_DECL(MPID_STATE_HANDLER_RECV_COMPLETE);

    MPIDI_FUNC_ENTER(MPID_STATE_HANDLER_RECV_COMPLETE);
    
    MPIU_Assert(e->type == PTL_EVENT_REPLY || e->type == PTL_EVENT_PUT || e->type == PTL_EVENT_PUT_OVERFLOW);

    if (REQ_PTL(rreq)->md != PTL_INVALID_HANDLE) {
        ret = PtlMDRelease(REQ_PTL(rreq)->md);
        MPIR_ERR_CHKANDJUMP1(ret, mpi_errno, MPI_ERR_OTHER, "**ptlmdrelease", "**ptlmdrelease %s", MPID_nem_ptl_strerror(ret));
    }

    for (i = 0; i < MPID_NEM_PTL_NUM_CHUNK_BUFFERS; ++i)
        if (REQ_PTL(rreq)->chunk_buffer[i])
            MPIU_Free(REQ_PTL(rreq)->chunk_buffer[i]);
    
    mpi_errno = MPID_Request_complete(rreq);
    if (mpi_errno) {
        MPIR_ERR_POP(mpi_errno);
    }

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_HANDLER_RECV_COMPLETE);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME handler_recv_dequeue_complete
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int handler_recv_dequeue_complete(const ptl_event_t *e)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Request *const rreq = e->user_ptr;
    int is_contig;
    MPI_Aint last;
    MPI_Aint dt_true_lb;
    MPIDI_msg_sz_t data_sz;
    MPID_Datatype *dt_ptr ATTRIBUTE((unused));

    MPIDI_STATE_DECL(MPID_STATE_HANDLER_RECV_DEQUEUE_COMPLETE);

    MPIDI_FUNC_ENTER(MPID_STATE_HANDLER_RECV_DEQUEUE_COMPLETE);

    MPIU_Assert(e->type == PTL_EVENT_PUT || e->type == PTL_EVENT_PUT_OVERFLOW);

    MPIDI_Datatype_get_info(rreq->dev.user_count, rreq->dev.datatype, is_contig, data_sz, dt_ptr, dt_true_lb);
    
    dequeue_req(e);

    if (e->type == PTL_EVENT_PUT_OVERFLOW) {
        /* unpack the data from unexpected buffer */
        MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "is_contig = %d", is_contig);

        if (is_contig) {
            MPIU_Memcpy((char *)rreq->dev.user_buf + dt_true_lb, e->start, e->mlength);
        } else {
            last = e->mlength;
            MPID_Segment_unpack(rreq->dev.segment_ptr, rreq->dev.segment_first, &last, e->start);
            if (last != e->mlength)
                MPIR_ERR_SET(rreq->status.MPI_ERROR, MPI_ERR_TYPE, "**dtypemismatch");
        }
    } else {
        /* Data was placed directly into the user buffer, so datatype mismatch
           is harder to detect. We use a simple check ensuring the received bytes
           are a multiple of a single basic element. Currently, we do not detect
           mismatches with datatypes constructed of more than one basic type */
        MPI_Datatype dt_basic_type;
        MPID_Datatype_get_basic_type(rreq->dev.datatype, dt_basic_type);
        if (dt_basic_type != MPI_DATATYPE_NULL && (e->mlength % MPID_Datatype_get_basic_size(dt_basic_type)) != 0)
            MPIR_ERR_SET(rreq->status.MPI_ERROR, MPI_ERR_TYPE, "**dtypemismatch");
    }
    
    mpi_errno = handler_recv_complete(e);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_HANDLER_RECV_DEQUEUE_COMPLETE);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME handler_recv_big_get
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int handler_recv_big_get(const ptl_event_t *e)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Request *const rreq = e->user_ptr;
    MPI_Aint last;

    MPIDI_STATE_DECL(MPID_STATE_HANDLER_RECV_UNPACK);

    MPIDI_FUNC_ENTER(MPID_STATE_HANDLER_RECV_UNPACK);

    MPIU_Assert(e->type == PTL_EVENT_REPLY);

    /* decrement the number of remaining gets */
    REQ_PTL(rreq)->num_gets--;
    if (REQ_PTL(rreq)->num_gets == 0) {
        /* if we used a temporary buffer, unpack the data */
        if (REQ_PTL(rreq)->chunk_buffer[0]) {
            last = rreq->dev.segment_size;
            MPID_Segment_unpack(rreq->dev.segment_ptr, rreq->dev.segment_first, &last, REQ_PTL(rreq)->chunk_buffer[0]);
            MPIU_Assert(last == rreq->dev.segment_size);
        }
        mpi_errno = handler_recv_complete(e);
    }

    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_HANDLER_RECV_UNPACK);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME big_get
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static void big_get(void *buf, ptl_size_t left_to_get, MPIDI_VC_t *vc, ptl_match_bits_t match_bits, MPID_Request *rreq)
{
    int ret;
    MPID_nem_ptl_vc_area *vc_ptl;
    ptl_size_t start, get_sz;

    vc_ptl = VC_PTL(vc);
    start = (ptl_size_t)buf;

    /* we need to handle all events */
    REQ_PTL(rreq)->event_handler = handler_recv_big_get;

    while (left_to_get > 0) {
        /* get up to the maximum allowed by the portals interface */
        if (left_to_get > MPIDI_nem_ptl_ni_limits.max_msg_size)
            get_sz = MPIDI_nem_ptl_ni_limits.max_msg_size;
        else
            get_sz = left_to_get;

        ret = MPID_nem_ptl_rptl_get(MPIDI_nem_ptl_global_md, start, get_sz, vc_ptl->id, vc_ptl->ptg, match_bits, 0, rreq);
        DBG_MSG_GET("global", get_sz, vc->pg_rank, match_bits);
        MPIU_DBG_MSG_P(CH3_CHANNEL, VERBOSE, "   buf=%p", (char *)start);
        MPIU_Assert(ret == 0);

        /* account for what has been sent */
        start += get_sz;
        left_to_get -= get_sz;
        REQ_PTL(rreq)->num_gets++;
    }
}

#undef FUNCNAME
#define FUNCNAME handler_recv_unpack_complete
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int handler_recv_unpack_complete(const ptl_event_t *e)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Request *const rreq = e->user_ptr;
    void *buf;
    MPI_Aint last;

    MPIDI_STATE_DECL(MPID_STATE_HANDLER_RECV_UNPACK_COMPLETE);

    MPIDI_FUNC_ENTER(MPID_STATE_HANDLER_RECV_UNPACK_COMPLETE);
    
    MPIU_Assert(e->type == PTL_EVENT_REPLY || e->type == PTL_EVENT_PUT || e->type == PTL_EVENT_PUT_OVERFLOW);

    if (e->type == PTL_EVENT_PUT_OVERFLOW)
        buf = e->start;
    else
        buf = REQ_PTL(rreq)->chunk_buffer[0];

    last = rreq->dev.segment_first + e->mlength;
    MPID_Segment_unpack(rreq->dev.segment_ptr, rreq->dev.segment_first, &last, buf);
    MPIU_Assert(last == rreq->dev.segment_first + e->mlength);
    
    mpi_errno = handler_recv_complete(e);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_HANDLER_RECV_UNPACK_COMPLETE);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME handler_recv_dequeue_unpack_complete
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int handler_recv_dequeue_unpack_complete(const ptl_event_t *e)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_HANDLER_RECV_DEQUEUE_UNPACK_COMPLETE);

    MPIDI_FUNC_ENTER(MPID_STATE_HANDLER_RECV_DEQUEUE_UNPACK_COMPLETE);
    
    MPIU_Assert(e->type == PTL_EVENT_PUT || e->type == PTL_EVENT_PUT_OVERFLOW);

    dequeue_req(e);
    mpi_errno = handler_recv_unpack_complete(e);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_HANDLER_RECV_DEQUEUE_UNPACK_COMPLETE);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME handler_recv_dequeue_large
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int handler_recv_dequeue_large(const ptl_event_t *e)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Request *const rreq = e->user_ptr;
    MPIDI_VC_t *vc;
    MPID_nem_ptl_vc_area *vc_ptl;
    int ret;
    int dt_contig;
    MPIDI_msg_sz_t data_sz;
    MPID_Datatype *dt_ptr;
    MPI_Aint dt_true_lb;
    MPI_Aint last;
    MPIU_CHKPMEM_DECL(1);
    MPIDI_STATE_DECL(MPID_STATE_HANDLER_RECV_DEQUEUE_LARGE);

    MPIDI_FUNC_ENTER(MPID_STATE_HANDLER_RECV_DEQUEUE_LARGE);
    
    MPIU_Assert(e->type == PTL_EVENT_PUT || e->type == PTL_EVENT_PUT_OVERFLOW);

    MPIDI_Comm_get_vc(rreq->comm, NPTL_MATCH_GET_RANK(e->match_bits), &vc);
    vc_ptl = VC_PTL(vc);
    
    dequeue_req(e);

    MPIDI_Datatype_get_info(rreq->dev.user_count, rreq->dev.datatype, dt_contig, data_sz, dt_ptr, dt_true_lb);

    /* unpack data from unexpected buffer first */
    if (e->type == PTL_EVENT_PUT_OVERFLOW) {
        if (dt_contig) {
            MPIU_Memcpy((char *)rreq->dev.user_buf + dt_true_lb, e->start, e->mlength);
        } else {
            last = e->mlength;
            MPID_Segment_unpack(rreq->dev.segment_ptr, 0, &last, e->start);
            MPIU_Assert(last == e->mlength);
            rreq->dev.segment_first = e->mlength;
        }
    }
    
    if (!(e->hdr_data & NPTL_LARGE)) {
        /* all data has already been received; we're done */
        mpi_errno = handler_recv_complete(e);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        goto fn_exit;
    }
        
    MPIU_Assert (e->mlength == PTL_LARGE_THRESHOLD);

    /* we need to GET the rest of the data from the sender's buffer */
    if (dt_contig) {
        big_get((char *)rreq->dev.user_buf + dt_true_lb + PTL_LARGE_THRESHOLD, data_sz - PTL_LARGE_THRESHOLD,
                vc, e->match_bits, rreq);
        goto fn_exit;
    }

    /* noncontig recv buffer */
    
    last = rreq->dev.segment_size;
    rreq->dev.iov_count = MPL_IOV_LIMIT;
    MPID_Segment_pack_vector(rreq->dev.segment_ptr, rreq->dev.segment_first, &last, rreq->dev.iov, &rreq->dev.iov_count);

    if (last == rreq->dev.segment_size && rreq->dev.segment_size <= MPIDI_nem_ptl_ni_limits.max_msg_size + PTL_LARGE_THRESHOLD) {
        /* Rest of message fits in one IOV */
        ptl_md_t md;

        md.start = rreq->dev.iov;
        md.length = rreq->dev.iov_count;
        md.options = PTL_IOVEC;
        md.eq_handle = MPIDI_nem_ptl_origin_eq;
        md.ct_handle = PTL_CT_NONE;
        ret = PtlMDBind(MPIDI_nem_ptl_ni, &md, &REQ_PTL(rreq)->md);
        MPIR_ERR_CHKANDJUMP1(ret, mpi_errno, MPI_ERR_OTHER, "**ptlmdbind", "**ptlmdbind %s", MPID_nem_ptl_strerror(ret));

        REQ_PTL(rreq)->event_handler = handler_recv_complete;
        ret = MPID_nem_ptl_rptl_get(REQ_PTL(rreq)->md, 0, rreq->dev.segment_size - rreq->dev.segment_first, vc_ptl->id, vc_ptl->ptg,
                     e->match_bits, 0, rreq);
        MPIR_ERR_CHKANDJUMP1(ret, mpi_errno, MPI_ERR_OTHER, "**ptlget", "**ptlget %s", MPID_nem_ptl_strerror(ret));
        goto fn_exit;
    }
        
    /* message won't fit in a single IOV, allocate buffer and unpack when received */
    /* FIXME: For now, allocate a single large buffer to hold entire message */
    MPIU_CHKPMEM_MALLOC(REQ_PTL(rreq)->chunk_buffer[0], void *, data_sz - PTL_LARGE_THRESHOLD,
                        mpi_errno, "chunk_buffer");
    big_get(REQ_PTL(rreq)->chunk_buffer[0], data_sz - PTL_LARGE_THRESHOLD, vc, e->match_bits, rreq);

 fn_exit:
    MPIU_CHKPMEM_COMMIT();
 fn_exit2:
    MPIDI_FUNC_EXIT(MPID_STATE_HANDLER_RECV_DEQUEUE_LARGE);
    return mpi_errno;
 fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_exit2;
}


#undef FUNCNAME
#define FUNCNAME handler_recv_dequeue_unpack_large
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int handler_recv_dequeue_unpack_large(const ptl_event_t *e)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Request *const rreq = e->user_ptr;
    MPIDI_VC_t *vc;
    MPI_Aint last;
    void *buf;
    MPIU_CHKPMEM_DECL(1);
    MPIDI_STATE_DECL(MPID_STATE_HANDLER_RECV_DEQUEUE_UNPACK_LARGE);

    MPIDI_FUNC_ENTER(MPID_STATE_HANDLER_RECV_DEQUEUE_UNPACK_LARGE);
    MPIU_Assert(e->type == PTL_EVENT_PUT || e->type == PTL_EVENT_PUT_OVERFLOW);

    MPIDI_Comm_get_vc(rreq->comm, NPTL_MATCH_GET_RANK(e->match_bits), &vc);

    dequeue_req(e);

    if (!(e->hdr_data & NPTL_LARGE)) {
        /* all data has already been received; we're done */
        mpi_errno = handler_recv_unpack_complete(e);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        goto fn_exit;
    }

    if (e->type == PTL_EVENT_PUT_OVERFLOW)
        buf = e->start;
    else
        buf = REQ_PTL(rreq)->chunk_buffer[0];

    MPIU_Assert(e->mlength == PTL_LARGE_THRESHOLD);
    last = PTL_LARGE_THRESHOLD;
    MPID_Segment_unpack(rreq->dev.segment_ptr, rreq->dev.segment_first, &last, buf);
    MPIU_Assert(last == PTL_LARGE_THRESHOLD);
    rreq->dev.segment_first += PTL_LARGE_THRESHOLD;
    MPIU_Free(REQ_PTL(rreq)->chunk_buffer[0]);

    MPIU_CHKPMEM_MALLOC(REQ_PTL(rreq)->chunk_buffer[0], void *, rreq->dev.segment_size - rreq->dev.segment_first,
                        mpi_errno, "chunk_buffer");
    big_get(REQ_PTL(rreq)->chunk_buffer[0], rreq->dev.segment_size - rreq->dev.segment_first, vc, e->match_bits, rreq);

 fn_exit:
    MPIU_CHKPMEM_COMMIT();
 fn_exit2:
    MPIDI_FUNC_EXIT(MPID_STATE_HANDLER_RECV_DEQUEUE_UNPACK_LARGE);
    return mpi_errno;
 fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_exit2;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_ptl_recv_posted
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_nem_ptl_recv_posted(MPIDI_VC_t *vc, MPID_Request *rreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_nem_ptl_vc_area *const vc_ptl = VC_PTL(vc);
    ptl_me_t me;
    int dt_contig;
    MPIDI_msg_sz_t data_sz;
    MPID_Datatype *dt_ptr;
    MPI_Aint dt_true_lb;
    MPI_Aint last;
    ptl_process_t id_any;
    int ret;
    MPIU_CHKPMEM_DECL(1);
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_PTL_RECV_POSTED);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_PTL_RECV_POSTED);

    id_any.phys.nid = PTL_NID_ANY;
    id_any.phys.pid = PTL_PID_ANY;

    MPID_nem_ptl_init_req(rreq);
    
    me.ct_handle = PTL_CT_NONE;
    me.uid = PTL_UID_ANY;
    me.options = ( PTL_ME_OP_PUT | PTL_ME_IS_ACCESSIBLE | PTL_ME_EVENT_LINK_DISABLE |
                   PTL_ME_EVENT_UNLINK_DISABLE | PTL_ME_USE_ONCE );
    if (vc == NULL) {
        /* MPI_ANY_SOURCE receive */
        me.match_id = id_any;
    } else {
        if (!vc_ptl->id_initialized) {
            mpi_errno = MPID_nem_ptl_init_id(vc);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        }
        me.match_id = vc_ptl->id;
    }

    MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "tag=%#x ctx=%#x rank=%#x", rreq->dev.match.parts.tag, rreq->dev.match.parts.context_id, rreq->dev.match.parts.rank));
    me.match_bits = NPTL_MATCH(rreq->dev.match.parts.tag, rreq->dev.match.parts.context_id,
                               rreq->dev.match.parts.rank);
    if (rreq->dev.match.parts.tag == MPI_ANY_TAG)
        me.ignore_bits = NPTL_MATCH_IGNORE_ANY_TAG;
    else
        me.ignore_bits = NPTL_MATCH_IGNORE;

    me.min_free = 0;

    MPIDI_Datatype_get_info(rreq->dev.user_count, rreq->dev.datatype, dt_contig, data_sz, dt_ptr, dt_true_lb);
    MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "count="MPI_AINT_FMT_DEC_SPEC" datatype=%#x contig=%d data_sz=%lu", rreq->dev.user_count, rreq->dev.datatype, dt_contig, data_sz));

    if (data_sz <= PTL_LARGE_THRESHOLD) {
        if (dt_contig) {
            /* small contig message */
            void *start = (char *)rreq->dev.user_buf + dt_true_lb;
            MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "Small contig message");
            if (start == NULL)
                me.start = &dummy;
            else
                me.start = start;
            me.length = data_sz;
            REQ_PTL(rreq)->event_handler = handler_recv_dequeue_complete;
        } else {
            /* small noncontig */
            MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "Small noncontig message");
            rreq->dev.segment_ptr = MPID_Segment_alloc();
            MPIR_ERR_CHKANDJUMP1(rreq->dev.segment_ptr == NULL, mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "MPID_Segment_alloc");
            MPID_Segment_init(rreq->dev.user_buf, rreq->dev.user_count, rreq->dev.datatype, rreq->dev.segment_ptr, 0);
            rreq->dev.segment_first = 0;
            rreq->dev.segment_size = data_sz;

            last = rreq->dev.segment_size;
            rreq->dev.iov_count = MPL_IOV_LIMIT;
            MPID_Segment_pack_vector(rreq->dev.segment_ptr, rreq->dev.segment_first, &last, rreq->dev.iov, &rreq->dev.iov_count);

            if (last == rreq->dev.segment_size) {
                /* entire message fits in IOV */
                MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "    entire message fits in IOV");
                me.start = rreq->dev.iov;
                me.length = rreq->dev.iov_count;
                me.options |= PTL_IOVEC;
                REQ_PTL(rreq)->event_handler = handler_recv_dequeue_complete;
            } else {
                /* IOV is not long enough to describe entire message: recv into
                   buffer and unpack later */
                MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "    IOV too long: using bounce buffer");
                MPIU_CHKPMEM_MALLOC(REQ_PTL(rreq)->chunk_buffer[0], void *, data_sz, mpi_errno, "chunk_buffer");
                me.start = REQ_PTL(rreq)->chunk_buffer[0];
                me.length = data_sz;
                REQ_PTL(rreq)->event_handler = handler_recv_dequeue_unpack_complete;
            }
        }
    } else {
        /* Large message: Create an ME for the first chunk of data, then do a GET for the rest */
        if (dt_contig) {
            /* large contig message */
            MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "Large contig message");
            me.start = (char *)rreq->dev.user_buf + dt_true_lb;
            me.length = PTL_LARGE_THRESHOLD;
            REQ_PTL(rreq)->event_handler = handler_recv_dequeue_large;
        } else {
            /* large noncontig */
            MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "Large noncontig message");
            rreq->dev.segment_ptr = MPID_Segment_alloc();
            MPIR_ERR_CHKANDJUMP1(rreq->dev.segment_ptr == NULL, mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "MPID_Segment_alloc");
            MPID_Segment_init(rreq->dev.user_buf, rreq->dev.user_count, rreq->dev.datatype, rreq->dev.segment_ptr, 0);
            rreq->dev.segment_first = 0;
            rreq->dev.segment_size = data_sz;

            last = PTL_LARGE_THRESHOLD;
            rreq->dev.iov_count = MPL_IOV_LIMIT;
            MPID_Segment_pack_vector(rreq->dev.segment_ptr, rreq->dev.segment_first, &last, rreq->dev.iov, &rreq->dev.iov_count);

            if (last == PTL_LARGE_THRESHOLD) {
                /* first chunk fits in IOV */
                MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "    first chunk fits in IOV");
                rreq->dev.segment_first = last;
                me.start = rreq->dev.iov;
                me.length = rreq->dev.iov_count;
                me.options |= PTL_IOVEC;
                REQ_PTL(rreq)->event_handler = handler_recv_dequeue_large;
            } else {
                /* IOV is not long enough to describe the first chunk: recv into
                   buffer and unpack later */
                MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "    IOV too long: using bounce buffer for first chunk");
                MPIU_CHKPMEM_MALLOC(REQ_PTL(rreq)->chunk_buffer[0], void *, PTL_LARGE_THRESHOLD, mpi_errno, "chunk_buffer");
                me.start = REQ_PTL(rreq)->chunk_buffer[0];
                me.length = PTL_LARGE_THRESHOLD;
                REQ_PTL(rreq)->event_handler = handler_recv_dequeue_unpack_large;
            }
        }
        
    }

    ret = PtlMEAppend(MPIDI_nem_ptl_ni, MPIDI_nem_ptl_pt, &me, PTL_PRIORITY_LIST, rreq, &REQ_PTL(rreq)->put_me);
    MPIR_ERR_CHKANDJUMP1(ret, mpi_errno, MPI_ERR_OTHER, "**ptlmeappend", "**ptlmeappend %s", MPID_nem_ptl_strerror(ret));
    DBG_MSG_MEAPPEND("REG", vc ? vc->pg_rank : MPI_ANY_SOURCE, me, rreq);
    MPIU_DBG_MSG_P(CH3_CHANNEL, VERBOSE, "    buf=%p", me.start);
    MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "MPIDI_nem_ptl_pt = %d", MPIDI_nem_ptl_pt);

 fn_exit:
    MPIU_CHKPMEM_COMMIT();
 fn_exit2:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_PTL_RECV_POSTED);
    return mpi_errno;
 fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_exit2;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_ptl_anysource_posted
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
void MPID_nem_ptl_anysource_posted(MPID_Request *rreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_PTL_ANYSOURCE_POSTED);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_PTL_ANYSOURCE_POSTED);

    mpi_errno = MPID_nem_ptl_recv_posted(NULL, rreq);

    /* FIXME: This function is void, so we can't return an error.  This function
       cannot return an error because the queue functions (where the posted_recv
       hooks are called) return no error code. */
    MPIU_Assertp(mpi_errno == MPI_SUCCESS);

    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_PTL_ANYSOURCE_POSTED);
}

#undef FUNCNAME
#define FUNCNAME cancel_recv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int cancel_recv(MPID_Request *rreq, int *cancelled)
{
    int mpi_errno = MPI_SUCCESS;
    int ptl_err   = PTL_OK;
    MPIDI_STATE_DECL(MPID_STATE_CANCEL_RECV);

    MPIDI_FUNC_ENTER(MPID_STATE_CANCEL_RECV);

    *cancelled = FALSE;

    /* An invalid handle indicates the operation has been completed
       and the matching list entry unlinked. At that point, the operation
       cannot be cancelled. */
    if (REQ_PTL(rreq)->put_me == PTL_INVALID_HANDLE)
        goto fn_exit;

    ptl_err = PtlMEUnlink(REQ_PTL(rreq)->put_me);
    if (ptl_err == PTL_OK)
        *cancelled = TRUE;

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_CANCEL_RECV);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_ptl_anysource_matched
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_nem_ptl_anysource_matched(MPID_Request *rreq)
{
    int mpi_errno, cancelled;

    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_PTL_ANYSOURCE_MATCHED);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_PTL_ANYSOURCE_MATCHED);

    mpi_errno = cancel_recv(rreq, &cancelled);
    /* FIXME: This function is does not return an error because the queue
       functions (where the posted_recv hooks are called) return no error
       code. See also comment on cancel_recv. */
    MPIU_Assertp(mpi_errno == MPI_SUCCESS);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_PTL_ANYSOURCE_MATCHED);
    return !cancelled;
 fn_fail:
    goto fn_exit;
}



#undef FUNCNAME
#define FUNCNAME MPID_nem_ptl_cancel_recv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_nem_ptl_cancel_recv(MPIDI_VC_t *vc,  MPID_Request *rreq)
{
    int mpi_errno, cancelled;

    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_PTL_CANCEL_RECV);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_PTL_CANCEL_RECV);

    mpi_errno = cancel_recv(rreq, &cancelled);
    /* FIXME: This function is does not return an error because the queue
       functions (where the posted_recv hooks are called) return no error
       code. */
    MPIU_Assertp(mpi_errno == MPI_SUCCESS);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_PTL_CANCEL_RECV);
    return !cancelled;
 fn_fail:
    goto fn_exit;
}



#undef FUNCNAME
#define FUNCNAME MPID_nem_ptl_lmt_start_recv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_nem_ptl_lmt_start_recv(MPIDI_VC_t *vc,  MPID_Request *rreq, MPL_IOV s_cookie)
{
    /* This function should only be called as a result of an Mrecv because of the CH3 protocol for
       Rendezvous Mrecvs. The regular CH3 protocol is not optimal for portals, since we don't need
       to exchange CTS/RTS. We need this code here because at the time of the Mprobe we don't know
       the target buffer, but we dequeue (and lose) the portals entry. This doesn't happen on
       regular large transfers because we handle them directly on the netmod. */
    int mpi_errno = MPI_SUCCESS;
    int dt_contig;
    MPIDI_msg_sz_t data_sz;
    MPID_Datatype *dt_ptr;
    MPI_Aint dt_true_lb;
    ptl_match_bits_t match_bits;
    int was_incomplete;
    int ret;
    MPID_nem_ptl_vc_area *vc_ptl = VC_PTL(vc);
    MPIU_CHKPMEM_DECL(1);

    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_PTL_LMT_START_RECV);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_PTL_LMT_START_RECV);

    /* This Rendezvous protocol does not do RTS-CTS. Since we have all the data, we limit to get it */
    /* The following code is inspired on handler_recv_dqueue_large */

    match_bits = NPTL_MATCH(rreq->dev.match.parts.tag, rreq->dev.match.parts.context_id,
                            rreq->dev.match.parts.rank);
    MPIDI_CH3U_Request_increment_cc(rreq, &was_incomplete);
    MPIU_Assert(was_incomplete == 0);
    MPIR_Request_add_ref(rreq);

    MPIDI_Datatype_get_info(rreq->dev.user_count, rreq->dev.datatype, dt_contig, data_sz, dt_ptr,
                            dt_true_lb);
    if (dt_contig) {
        void * real_user_buf = (char *)rreq->dev.user_buf + dt_true_lb;

        big_get((char *)real_user_buf + PTL_LARGE_THRESHOLD, data_sz - PTL_LARGE_THRESHOLD, vc, match_bits, rreq);

        /* The memcpy is done after the get purposely for overlapping */
        MPIU_Memcpy(real_user_buf, rreq->dev.tmpbuf, PTL_LARGE_THRESHOLD);
    }
    else {
        MPI_Aint last;

        rreq->dev.segment_ptr = MPID_Segment_alloc();
        MPIR_ERR_CHKANDJUMP1(rreq->dev.segment_ptr == NULL, mpi_errno, MPI_ERR_OTHER, "**nomem",
                             "**nomem %s", "MPID_Segment_alloc");
        MPID_Segment_init(rreq->dev.user_buf, rreq->dev.user_count, rreq->dev.datatype,
                          rreq->dev.segment_ptr, 0);
        rreq->dev.segment_first = 0;
        rreq->dev.segment_size = data_sz;
        last = PTL_LARGE_THRESHOLD;
        MPID_Segment_unpack(rreq->dev.segment_ptr, rreq->dev.segment_first, &last, rreq->dev.tmpbuf);
        MPIU_Assert(last == PTL_LARGE_THRESHOLD);
        rreq->dev.segment_first = PTL_LARGE_THRESHOLD;
        last = rreq->dev.segment_size;
        rreq->dev.iov_count = MPL_IOV_LIMIT;
        MPID_Segment_pack_vector(rreq->dev.segment_ptr, rreq->dev.segment_first, &last, rreq->dev.iov,
                                 &rreq->dev.iov_count);
        if (last == rreq->dev.segment_size && last <= MPIDI_nem_ptl_ni_limits.max_msg_size + PTL_LARGE_THRESHOLD) {
            /* Rest of message fits in one IOV */
            ptl_md_t md;

            md.start = rreq->dev.iov;
            md.length = rreq->dev.iov_count;
            md.options = PTL_IOVEC;
            md.eq_handle = MPIDI_nem_ptl_origin_eq;
            md.ct_handle = PTL_CT_NONE;
            ret = PtlMDBind(MPIDI_nem_ptl_ni, &md, &REQ_PTL(rreq)->md);
            MPIR_ERR_CHKANDJUMP1(ret, mpi_errno, MPI_ERR_OTHER, "**ptlmdbind", "**ptlmdbind %s",
                                 MPID_nem_ptl_strerror(ret));

            REQ_PTL(rreq)->event_handler = handler_recv_complete;
            ret = MPID_nem_ptl_rptl_get(REQ_PTL(rreq)->md, 0, rreq->dev.segment_size - rreq->dev.segment_first,
                                        vc_ptl->id, vc_ptl->ptg, match_bits, 0, rreq);
            MPIR_ERR_CHKANDJUMP1(ret, mpi_errno, MPI_ERR_OTHER, "**ptlget", "**ptlget %s",
                                 MPID_nem_ptl_strerror(ret));
        }
        else {
            /* message won't fit in a single IOV, allocate buffer and unpack when received */
            /* FIXME: For now, allocate a single large buffer to hold entire message */
            MPIU_CHKPMEM_MALLOC(REQ_PTL(rreq)->chunk_buffer[0], void *, rreq->dev.segment_size - rreq->dev.segment_first,
                                mpi_errno, "chunk_buffer");
            big_get(REQ_PTL(rreq)->chunk_buffer[0], rreq->dev.segment_size - rreq->dev.segment_first, vc, match_bits, rreq);
        }
    }
    MPIU_Free(rreq->dev.tmpbuf);
    rreq->ch.lmt_tmp_cookie.MPL_IOV_LEN = 0;  /* Required for do_cts in mpid_nem_lmt.c */

 fn_exit:
    MPIU_CHKPMEM_COMMIT();
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_PTL_LMT_START_RECV);
    return mpi_errno;
 fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
}

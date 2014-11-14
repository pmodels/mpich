/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2012 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "ptl_impl.h"
#include "rptl.h"

#undef FUNCNAME
#define FUNCNAME big_meappend
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
static void big_meappend(void *buf, ptl_size_t left_to_send, MPIDI_VC_t *vc, ptl_match_bits_t match_bits, MPID_Request *sreq)
{
    int i, ret;
    MPID_nem_ptl_vc_area *vc_ptl;
    ptl_me_t me;

    vc_ptl = VC_PTL(vc);

    me.start = buf;
    me.ct_handle = PTL_CT_NONE;
    me.uid = PTL_UID_ANY;
    me.options = ( PTL_ME_OP_PUT | PTL_ME_OP_GET | PTL_ME_USE_ONCE | PTL_ME_IS_ACCESSIBLE | PTL_ME_EVENT_LINK_DISABLE |
                   PTL_ME_EVENT_UNLINK_DISABLE );
    me.match_id = vc_ptl->id;
    me.match_bits = match_bits;
    me.ignore_bits = 0;
    me.min_free = 0;

    /* allocate enough handles to cover all get operations */
    REQ_PTL(sreq)->get_me_p = MPIU_Malloc(sizeof(ptl_handle_me_t) *
                                        ((left_to_send / MPIDI_nem_ptl_ni_limits.max_msg_size) + 1));

    /* queue up as many entries as necessary to describe the entire message */
    for (i = 0; left_to_send > 0; i++) {
        /* send up to the maximum allowed by the portals interface */
        if (left_to_send > MPIDI_nem_ptl_ni_limits.max_msg_size)
            me.length = MPIDI_nem_ptl_ni_limits.max_msg_size;
        else
            me.length = left_to_send;

        ret = MPID_nem_ptl_me_append(MPIDI_nem_ptl_ni, MPIDI_nem_ptl_get_pt, &me, PTL_PRIORITY_LIST, sreq,
                                     &REQ_PTL(sreq)->get_me_p[i]);
        DBG_MSG_MEAPPEND("CTL", vc->pg_rank, me, sreq);
        MPIU_Assert(ret == 0);

        /* account for what has been sent */
        me.start = (char *)me.start + me.length;
        left_to_send -= me.length;
        REQ_PTL(sreq)->num_gets++;
    }
}

#undef FUNCNAME
#define FUNCNAME handler_send_complete
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
static int handler_send_complete(const ptl_event_t *e)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Request *const sreq = e->user_ptr;
    int ret;
    int i;
    MPIDI_STATE_DECL(MPID_STATE_HANDLER_SEND_COMPLETE);

    MPIDI_FUNC_ENTER(MPID_STATE_HANDLER_SEND_COMPLETE);

    MPIU_Assert(e->type == PTL_EVENT_SEND || e->type == PTL_EVENT_GET);

    if (REQ_PTL(sreq)->md != PTL_INVALID_HANDLE) {
        ret = PtlMDRelease(REQ_PTL(sreq)->md);
        MPIU_ERR_CHKANDJUMP1(ret, mpi_errno, MPI_ERR_OTHER, "**ptlmdrelease", "**ptlmdrelease %s", MPID_nem_ptl_strerror(ret));
    }

    for (i = 0; i < MPID_NEM_PTL_NUM_CHUNK_BUFFERS; ++i)
        if (REQ_PTL(sreq)->chunk_buffer[i])
            MPIU_Free(REQ_PTL(sreq)->chunk_buffer[i]);

    if (REQ_PTL(sreq)->get_me_p)
        MPIU_Free(REQ_PTL(sreq)->get_me_p);
    
    MPIDI_CH3U_Request_complete(sreq);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_HANDLER_SEND_COMPLETE);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME handler_large
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
static int handler_large(const ptl_event_t *e)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Request *const sreq = e->user_ptr;
    MPIDI_STATE_DECL(MPID_STATE_HANDLER_LARGE);

    MPIDI_FUNC_ENTER(MPID_STATE_HANDLER_LARGE);

    MPIU_Assert(e->type == PTL_EVENT_SEND || e->type == PTL_EVENT_GET);

    if (e->type == PTL_EVENT_SEND) {
        REQ_PTL(sreq)->put_done = 1;
    } else if (e->type == PTL_EVENT_GET) {
        /* decrement the remaining get operations */
        REQ_PTL(sreq)->num_gets--;
    }

    if (REQ_PTL(sreq)->num_gets == 0 && REQ_PTL(sreq)->put_done)
        mpi_errno = handler_send_complete(e);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_HANDLER_LARGE);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#if 0

#undef FUNCNAME
#define FUNCNAME handler_pack_chunk
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
static int handler_pack_chunk(const ptl_event_t *e)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Request *const sreq = e->user_ptr;
    MPIDI_STATE_DECL(MPID_STATE_HANDLER_PACK_CHUNK);

    MPIDI_FUNC_ENTER(MPID_STATE_HANDLER_PACK_CHUNK);

    MPIU_Assert(e->type == PTL_EVENT_GET || e->type == PTL_EVENT_PUT);

    if (e->type == PTL_EVENT_PUT) {
        mpi_errno = handler_send_complete(e);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        goto fn_exit;
    }

    /* pack next chunk */
    MPI_nem_ptl_pack_byte(sreq->dev.segment_ptr, sreq->dev.segment_first, sreq->dev.segment_first + PTL_LARGE_THRESHOLD,
              REQ_PTL(sreq_)->chunk_buffer[1], &REQ_PTL(sreq)->overflow[1]);
    sreq->dev.segment_first += PTL_LARGE_THRESHOLD;

    /* notify receiver */
    ret = MPID_nem_ptl_rptl_put(MPIDI_nem_ptl_global_md, 0, 0, PTL_ACK_REQ, vc_ptl->id,
                 vc_ptl->pt, ?????, 0, sreq,
                 NPTL_HEADER(?????, MPIDI_Process.my_pg_rank, me.match_bits));


 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_HANDLER_PACK_CHUNK);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}
#undef FUNCNAME
#define FUNCNAME handler_multi_put
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
static int handler_multi_put(const ptl_event_t *e)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Request *const sreq = e->user_ptr;
    MPIDI_STATE_DECL(MPID_STATE_HANDLER_MULTI_PUT);

    MPIDI_FUNC_ENTER(MPID_STATE_HANDLER_MULTI_PUT);

    
    

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_HANDLER_MULTI_PUT);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME handler_large_multi
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
static int handler_large_multi(const ptl_event_t *e)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Request *const sreq = e->user_ptr;
    MPIDI_STATE_DECL(MPID_STATE_HANDLER_LARGE_MULTI);

    MPIU_Assert(e->type == PTL_EVENT_ACK);

    MPIDI_FUNC_ENTER(MPID_STATE_HANDLER_LARGE_MULTI);
    if (e->mlength < PTL_LARGE_THRESHOLD) {
        /* truncated message */
        mpi_errno = handler_send_complete(e);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    } else {
        REQ_PTL(sreq)->event_handler = handler_pack_chunk;
    }
    
 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_HANDLER_LARGE_MULTI);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#endif


/* Send message for either isend or issend */
#undef FUNCNAME
#define FUNCNAME send_msg
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
static int send_msg(ptl_hdr_data_t ssend_flag, struct MPIDI_VC *vc, const void *buf, int count, MPI_Datatype datatype, int dest,
                    int tag, MPID_Comm *comm, int context_offset, struct MPID_Request **request)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_nem_ptl_vc_area *const vc_ptl = VC_PTL(vc);
    int ret;
    MPIDI_msg_sz_t data_sz;
    int dt_contig;
    MPI_Aint dt_true_lb;
    MPID_Datatype *dt_ptr;
    MPID_Request *sreq = NULL;
    ptl_me_t me;
    int initial_iov_count, remaining_iov_count;
    ptl_md_t md;
    MPI_Aint last;
    MPIU_CHKPMEM_DECL(2);
    MPIDI_STATE_DECL(MPID_STATE_SEND_MSG);

    MPIDI_FUNC_ENTER(MPID_STATE_SEND_MSG);

    MPID_nem_ptl_request_create_sreq(sreq, mpi_errno, comm);
    sreq->dev.match.parts.rank = dest;
    sreq->dev.match.parts.tag = tag;
    sreq->dev.match.parts.context_id = comm->context_id + context_offset;

    if (!vc_ptl->id_initialized) {
        mpi_errno = MPID_nem_ptl_init_id(vc);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }

    MPIDI_Datatype_get_info(count, datatype, dt_contig, data_sz, dt_ptr, dt_true_lb);
    MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "count=%d datatype=%#x contig=%d data_sz=%lu", count, datatype, dt_contig, data_sz));

    if (data_sz <= PTL_LARGE_THRESHOLD) {
        /* Small message.  Send all data eagerly */
        if (dt_contig) {
            MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "Small contig message");
            REQ_PTL(sreq)->event_handler = handler_send_complete;
            MPIU_DBG_MSG_P(CH3_CHANNEL, VERBOSE, "&REQ_PTL(sreq)->event_handler = %p", &(REQ_PTL(sreq)->event_handler));
            ret = MPID_nem_ptl_rptl_put(MPIDI_nem_ptl_global_md, (ptl_size_t)((char *)buf + dt_true_lb), data_sz, PTL_NO_ACK_REQ, vc_ptl->id, vc_ptl->pt,
                         NPTL_MATCH(tag, comm->context_id + context_offset, comm->rank), 0, sreq,
                                        NPTL_HEADER(ssend_flag, data_sz));
            MPIU_ERR_CHKANDJUMP1(ret, mpi_errno, MPI_ERR_OTHER, "**ptlput", "**ptlput %s", MPID_nem_ptl_strerror(ret));
            DBG_MSG_PUT("global", data_sz, vc->pg_rank, NPTL_MATCH(tag, comm->context_id + context_offset, comm->rank), NPTL_HEADER(ssend_flag, data_sz));
            MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "id.nid = %#x", vc_ptl->id.phys.nid);
            MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "id.pid = %#x", vc_ptl->id.phys.pid);
            MPIU_DBG_MSG_P(CH3_CHANNEL, VERBOSE, "sreq = %p", sreq);
            MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "vc_ptl->pt = %d", vc_ptl->pt);
            MPIU_DBG_MSG_P(CH3_CHANNEL, VERBOSE, "REQ_PTL(sreq)->event_handler = %p", REQ_PTL(sreq)->event_handler);
           goto fn_exit;
        }
        
        /* noncontig data */
        MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "Small noncontig message");
        sreq->dev.segment_ptr = MPID_Segment_alloc();
        MPIU_ERR_CHKANDJUMP1(sreq->dev.segment_ptr == NULL, mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "MPID_Segment_alloc");
        MPID_Segment_init(buf, count, datatype, sreq->dev.segment_ptr, 0);
        sreq->dev.segment_first = 0;
        sreq->dev.segment_size = data_sz;

        last = sreq->dev.segment_size;
        sreq->dev.iov_count = MPID_IOV_LIMIT;
        MPID_Segment_pack_vector(sreq->dev.segment_ptr, sreq->dev.segment_first, &last, sreq->dev.iov, &sreq->dev.iov_count);

        if (last == sreq->dev.segment_size) {
            /* IOV is able to describe entire message */
            MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "    entire message fits in IOV");
            md.start = sreq->dev.iov;
            md.length = sreq->dev.iov_count;
            md.options = PTL_IOVEC;
            md.eq_handle = MPIDI_nem_ptl_origin_eq;
            md.ct_handle = PTL_CT_NONE;
            ret = PtlMDBind(MPIDI_nem_ptl_ni, &md, &REQ_PTL(sreq)->md);
            MPIU_ERR_CHKANDJUMP1(ret, mpi_errno, MPI_ERR_OTHER, "**ptlmdbind", "**ptlmdbind %s", MPID_nem_ptl_strerror(ret));
                
            REQ_PTL(sreq)->event_handler = handler_send_complete;
            ret = MPID_nem_ptl_rptl_put(REQ_PTL(sreq)->md, 0, data_sz, PTL_NO_ACK_REQ, vc_ptl->id, vc_ptl->pt,
                         NPTL_MATCH(tag, comm->context_id + context_offset, comm->rank), 0, sreq,
                                        NPTL_HEADER(ssend_flag, data_sz));
            MPIU_ERR_CHKANDJUMP1(ret, mpi_errno, MPI_ERR_OTHER, "**ptlput", "**ptlput %s", MPID_nem_ptl_strerror(ret));
            DBG_MSG_PUT("sreq", data_sz, vc->pg_rank, NPTL_MATCH(tag, comm->context_id + context_offset, comm->rank), NPTL_HEADER(ssend_flag, data_sz));
            goto fn_exit;
        }
        
        /* IOV is not long enough to describe entire message */
        MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "    IOV too long: using bounce buffer");
        MPIU_CHKPMEM_MALLOC(REQ_PTL(sreq)->chunk_buffer[0], void *, data_sz, mpi_errno, "chunk_buffer");
        MPID_Segment_init(buf, count, datatype, sreq->dev.segment_ptr, 0);
        sreq->dev.segment_first = 0;
        last = data_sz;
        MPID_Segment_pack(sreq->dev.segment_ptr, sreq->dev.segment_first, &last, REQ_PTL(sreq)->chunk_buffer[0]);
        MPIU_Assert(last == sreq->dev.segment_size);
        REQ_PTL(sreq)->event_handler = handler_send_complete;
        ret = MPID_nem_ptl_rptl_put(MPIDI_nem_ptl_global_md, (ptl_size_t)REQ_PTL(sreq)->chunk_buffer[0], data_sz, PTL_NO_ACK_REQ,
                     vc_ptl->id, vc_ptl->pt, NPTL_MATCH(tag, comm->context_id + context_offset, comm->rank), 0, sreq,
                                    NPTL_HEADER(ssend_flag, data_sz));
        MPIU_ERR_CHKANDJUMP1(ret, mpi_errno, MPI_ERR_OTHER, "**ptlput", "**ptlput %s", MPID_nem_ptl_strerror(ret));
        DBG_MSG_PUT("global", data_sz, vc->pg_rank, NPTL_MATCH(tag, comm->context_id + context_offset, comm->rank), NPTL_HEADER(ssend_flag, data_sz));
        goto fn_exit;
    }
        
    /* Large message.  Send first chunk of data and let receiver get the rest */
    if (dt_contig) {
        /* create ME for buffer so receiver can issue a GET for the data */
        MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "Large contig message");
        big_meappend((char *)buf + dt_true_lb + PTL_LARGE_THRESHOLD, data_sz - PTL_LARGE_THRESHOLD, vc,
                     NPTL_MATCH(tag, comm->context_id + context_offset, comm->rank), sreq);
        REQ_PTL(sreq)->large = TRUE;

        REQ_PTL(sreq)->event_handler = handler_large;
        ret = MPID_nem_ptl_rptl_put(MPIDI_nem_ptl_global_md, (ptl_size_t)((char *)buf + dt_true_lb), PTL_LARGE_THRESHOLD, PTL_NO_ACK_REQ, vc_ptl->id, vc_ptl->pt,
                     NPTL_MATCH(tag, comm->context_id + context_offset, comm->rank), 0, sreq,
                                    NPTL_HEADER(ssend_flag | NPTL_LARGE, data_sz));
        MPIU_ERR_CHKANDJUMP1(ret, mpi_errno, MPI_ERR_OTHER, "**ptlput", "**ptlput %s", MPID_nem_ptl_strerror(ret));
        DBG_MSG_PUT("global", PTL_LARGE_THRESHOLD, vc->pg_rank, NPTL_MATCH(tag, comm->context_id + context_offset, comm->rank), NPTL_HEADER(ssend_flag | NPTL_LARGE, data_sz));
        goto fn_exit;
    }
    
    /* Large noncontig data */
    MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "Large noncontig message");
    sreq->dev.segment_ptr = MPID_Segment_alloc();
    MPIU_ERR_CHKANDJUMP1(sreq->dev.segment_ptr == NULL, mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "MPID_Segment_alloc");
    MPID_Segment_init(buf, count, datatype, sreq->dev.segment_ptr, 0);
    sreq->dev.segment_first = 0;
    sreq->dev.segment_size = data_sz;

    last = PTL_LARGE_THRESHOLD;
    sreq->dev.iov_count = MPID_IOV_LIMIT;
    MPID_Segment_pack_vector(sreq->dev.segment_ptr, sreq->dev.segment_first, &last, sreq->dev.iov, &sreq->dev.iov_count);

    initial_iov_count = sreq->dev.iov_count;
    sreq->dev.segment_first = last;

    if (last == PTL_LARGE_THRESHOLD) {
        /* first chunk of message fits into IOV */
        MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "    first chunk fits in IOV");
        if (initial_iov_count < MPID_IOV_LIMIT) {
            /* There may be space for the rest of the message in this IOV */
            sreq->dev.iov_count = MPID_IOV_LIMIT - sreq->dev.iov_count;
            last = sreq->dev.segment_size;
                    
            MPID_Segment_pack_vector(sreq->dev.segment_ptr, sreq->dev.segment_first, &last,
                                     &sreq->dev.iov[initial_iov_count], &sreq->dev.iov_count);
            remaining_iov_count = sreq->dev.iov_count;

            if (last == sreq->dev.segment_size && last <= MPIDI_nem_ptl_ni_limits.max_msg_size + PTL_LARGE_THRESHOLD) {
                /* Entire message fit in one IOV */
                MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "    rest of message fits in one IOV");
                /* Create ME for remaining data */
                me.start = &sreq->dev.iov[initial_iov_count];
                me.length = remaining_iov_count;
                me.ct_handle = PTL_CT_NONE;
                me.uid = PTL_UID_ANY;
                me.options = ( PTL_ME_OP_PUT | PTL_ME_OP_GET | PTL_ME_USE_ONCE | PTL_ME_IS_ACCESSIBLE | PTL_ME_EVENT_LINK_DISABLE |
                               PTL_ME_EVENT_UNLINK_DISABLE | PTL_IOVEC );
                me.match_id = vc_ptl->id;
                me.match_bits = NPTL_MATCH(tag, comm->context_id + context_offset, comm->rank);
                me.ignore_bits = 0;
                me.min_free = 0;

                MPIU_CHKPMEM_MALLOC(REQ_PTL(sreq)->get_me_p, ptl_handle_me_t *, sizeof(ptl_handle_me_t), mpi_errno, "get_me_p");

                REQ_PTL(sreq)->num_gets = 1;
                ret = MPID_nem_ptl_me_append(MPIDI_nem_ptl_ni, MPIDI_nem_ptl_get_pt, &me, PTL_PRIORITY_LIST, sreq,
                                             &REQ_PTL(sreq)->get_me_p[0]);
                MPIU_ERR_CHKANDJUMP1(ret, mpi_errno, MPI_ERR_OTHER, "**ptlmeappend", "**ptlmeappend %s", MPID_nem_ptl_strerror(ret));
                DBG_MSG_MEAPPEND("CTL", vc->pg_rank, me, sreq);

                /* Create MD for first chunk */
                md.start = sreq->dev.iov;
                md.length = initial_iov_count;
                md.options = PTL_IOVEC;
                md.eq_handle = MPIDI_nem_ptl_origin_eq;
                md.ct_handle = PTL_CT_NONE;
                ret = PtlMDBind(MPIDI_nem_ptl_ni, &md, &REQ_PTL(sreq)->md);
                MPIU_ERR_CHKANDJUMP1(ret, mpi_errno, MPI_ERR_OTHER, "**ptlmdbind", "**ptlmdbind %s", MPID_nem_ptl_strerror(ret));

                REQ_PTL(sreq)->large = TRUE;

                REQ_PTL(sreq)->event_handler = handler_large;
                ret = MPID_nem_ptl_rptl_put(REQ_PTL(sreq)->md, 0, PTL_LARGE_THRESHOLD, PTL_NO_ACK_REQ, vc_ptl->id, vc_ptl->pt,
                             NPTL_MATCH(tag, comm->context_id + context_offset, comm->rank), 0, sreq,
                                            NPTL_HEADER(ssend_flag | NPTL_LARGE, data_sz));
                MPIU_ERR_CHKANDJUMP1(ret, mpi_errno, MPI_ERR_OTHER, "**ptlput", "**ptlput %s", MPID_nem_ptl_strerror(ret));
                DBG_MSG_PUT("req", PTL_LARGE_THRESHOLD, vc->pg_rank, NPTL_MATCH(tag, comm->context_id + context_offset, comm->rank), NPTL_HEADER(ssend_flag | NPTL_LARGE, data_sz));
                goto fn_exit;
            }
        }
        /* First chunk of message fits, but the rest doesn't */
        /* Don't handle this case separately */
    }

    /* allocate a temporary buffer and copy all the data to send */
    MPIU_CHKPMEM_MALLOC(REQ_PTL(sreq)->chunk_buffer[0], void *, data_sz, mpi_errno, "tmpbuf");

    last = data_sz;
    MPID_Segment_pack(sreq->dev.segment_ptr, 0, &last, REQ_PTL(sreq)->chunk_buffer[0]);
    MPIU_Assert(last == data_sz);

    big_meappend((char *)REQ_PTL(sreq)->chunk_buffer[0] + PTL_LARGE_THRESHOLD, data_sz - PTL_LARGE_THRESHOLD, vc,
                 NPTL_MATCH(tag, comm->context_id + context_offset, comm->rank), sreq);
    REQ_PTL(sreq)->large = TRUE;

    REQ_PTL(sreq)->event_handler = handler_large;
    ret = MPID_nem_ptl_rptl_put(MPIDI_nem_ptl_global_md, (ptl_size_t)REQ_PTL(sreq)->chunk_buffer[0], PTL_LARGE_THRESHOLD,
                                PTL_NO_ACK_REQ, vc_ptl->id, vc_ptl->pt, NPTL_MATCH(tag, comm->context_id + context_offset, comm->rank),
                                0, sreq, NPTL_HEADER(ssend_flag | NPTL_LARGE, data_sz));
    MPIU_ERR_CHKANDJUMP1(ret, mpi_errno, MPI_ERR_OTHER, "**ptlput", "**ptlput %s", MPID_nem_ptl_strerror(ret));
    DBG_MSG_PUT("global", PTL_LARGE_THRESHOLD, vc->pg_rank, NPTL_MATCH(tag, comm->context_id + context_offset, comm->rank), NPTL_HEADER(ssend_flag | NPTL_LARGE, data_sz));
    
 fn_exit:
    *request = sreq;
    MPIU_CHKPMEM_COMMIT();
    MPIDI_FUNC_EXIT(MPID_STATE_SEND_MSG);
    return mpi_errno;
 fn_fail:
    if (sreq) {
        MPID_Request_release(sreq);
        sreq = NULL;
    }
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_ptl_isend
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPID_nem_ptl_isend(struct MPIDI_VC *vc, const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
                       MPID_Comm *comm, int context_offset, struct MPID_Request **request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_PTL_ISEND);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_PTL_ISEND);

    mpi_errno = send_msg(0, vc, buf, count, datatype, dest, tag, comm, context_offset, request);

    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_PTL_ISEND);
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_ptl_issend
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPID_nem_ptl_issend(struct MPIDI_VC *vc, const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
                        MPID_Comm *comm, int context_offset, struct MPID_Request **request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_PTL_ISSEND);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_PTL_ISSEND);

    mpi_errno = send_msg(NPTL_SSEND, vc, buf, count, datatype, dest, tag, comm, context_offset, request);

    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_PTL_ISSEND);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_ptl_cancel_send
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPID_nem_ptl_cancel_send(struct MPIDI_VC *vc,  struct MPID_Request *sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_PKT_DECL_CAST(upkt, MPIDI_nem_ptl_pkt_cancel_send_req_t, csr_pkt);
    MPID_Request *csr_sreq;
    int was_incomplete;

    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_PTL_CANCEL_SEND);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_PTL_CANCEL_SEND);

    /* The completion counter and reference count are incremented to keep
       the request around long enough to receive a
       response regardless of what the user does (free the request before
       waiting, etc.). */
    MPIDI_CH3U_Request_increment_cc(sreq, &was_incomplete);
    if (!was_incomplete) {
        /* The reference count is incremented only if the request was
           complete before the increment. */
        MPIR_Request_add_ref(sreq);
    }

    csr_pkt->type = MPIDI_NEM_PKT_NETMOD;
    csr_pkt->subtype = MPIDI_NEM_PTL_PKT_CANCEL_SEND_REQ;
    csr_pkt->match.parts.rank = sreq->dev.match.parts.rank;
    csr_pkt->match.parts.tag = sreq->dev.match.parts.tag;
    csr_pkt->match.parts.context_id = sreq->dev.match.parts.context_id;
    csr_pkt->sender_req_id = sreq->handle;

    MPID_nem_ptl_iStartContigMsg(vc, csr_pkt, sizeof(*csr_pkt), NULL,
                                 0, &csr_sreq);

    if (csr_sreq != NULL)
        MPID_Request_release(csr_sreq);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_PTL_CANCEL_SEND);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

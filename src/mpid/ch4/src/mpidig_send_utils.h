/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIDIG_SEND_UTILS_H_INCLUDED
#define MPIDIG_SEND_UTILS_H_INCLUDED

/* This file is for supporting routines used for pipelined data send. These routines mainly is for
 * managing the send request counters, completion counters and DT refcount */

/* prepare the send counters for size of data and datatype that we need to refcount */
MPL_STATIC_INLINE_PREFIX void MPIDIG_am_send_async_init(MPIR_Request * sreq, MPI_Datatype datatype,
                                                        MPI_Aint data_sz)
{
    MPIDIG_sreq_async_t *send_async = &MPIDIG_REQUEST(sreq, req->send_async);
    send_async->datatype = datatype;
    send_async->data_sz_left = data_sz;
    send_async->offset = 0;
    send_async->seg_issued = 0;
    send_async->seg_completed = 0;
    /* hold reference to the sreq and datatype at the start of send operation */
    MPIR_cc_inc(sreq->cc_ptr);
    MPIR_Datatype_add_ref_if_not_builtin(send_async->datatype);
}

MPL_STATIC_INLINE_PREFIX MPI_Aint MPIDIG_am_send_async_get_data_sz_left(MPIR_Request * sreq)
{
    return MPIDIG_REQUEST(sreq, req->send_async).data_sz_left;
}

MPL_STATIC_INLINE_PREFIX MPI_Aint MPIDIG_am_send_async_get_offset(MPIR_Request * sreq)
{
    return MPIDIG_REQUEST(sreq, req->send_async).offset;
}

MPL_STATIC_INLINE_PREFIX bool MPIDIG_am_send_async_is_done(MPIR_Request * sreq)
{
    return MPIDIG_REQUEST(sreq, req->send_async).data_sz_left == 0;
}

/* indicating one segment is issued, update counter with actual send_size */
MPL_STATIC_INLINE_PREFIX void MPIDIG_am_send_async_issue_seg(MPIR_Request * sreq,
                                                             MPI_Aint send_size)
{
    MPIDIG_sreq_async_t *send_async = &MPIDIG_REQUEST(sreq, req->send_async);
    send_async->data_sz_left -= send_size;
    MPIR_Assert(send_async->data_sz_left >= 0);
    send_async->offset += send_size;
    send_async->seg_issued += 1;
}

/* origin side completion of one issued segment, release cc and refcount if all data is set and all
 * segments are completed
 * The send is considered completed if 1. data_sz_left is zero, 2. all issued segment are finished
 * */
MPL_STATIC_INLINE_PREFIX bool MPIDIG_am_send_async_finish_seg(MPIR_Request * sreq)
{
    bool is_done = false;
    MPIDIG_sreq_async_t *send_async = &MPIDIG_REQUEST(sreq, req->send_async);
    send_async->seg_completed += 1;
    if (send_async->data_sz_left == 0 && send_async->seg_issued == send_async->seg_completed) {
        MPIR_Datatype_release_if_not_builtin(send_async->datatype);
        MPID_Request_complete(sreq);
        is_done = true;
    }
    return is_done;
}

#endif /* MPIDIG_SEND_UTILS_H_INCLUDED */

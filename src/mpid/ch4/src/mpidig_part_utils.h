/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */
#ifndef MPIDIG_PART_UTILS_H_INCLUDED
#define MPIDIG_PART_UTILS_H_INCLUDED

#include <stdio.h>
#include "ch4_wait.h"
#include "mpi.h"
#include "mpidpre.h"
#include "mpiimpl.h"
#include "ch4_impl.h"
#include "mpir_assert.h"
#include "mpir_request.h"
#include "mpir_tags.h"
#include "mpl_base.h"

/*------------------------------------------------------------------------------------------------*/
/* definition of the send status which depends from the code path and the iteation */
#define MPIDIG_PART_STATUS_SEND_READY 0
/* must go through the CTS reception (-1) and the Pready (-1) before being ready to send */
#define MPIDIG_PART_STATUS_SEND_AM_INIT 2
// tag-based messaging must go through the CTS reception (-1) and the Pready (-1) at the FIRST
// iteration only. other iteration, the value is not used
#define MPIDIG_PART_STATUS_SEND_TAG_INIT 2
/*------------------------------------------------------------------------------------------------*/
/* definition of the send status which depends from the code path and the iteation */
#define MPIDIG_PART_STATUS_RECV_READY 0
#define MPIDIG_PART_STATUS_RECV_INIT 1
/*------------------------------------------------------------------------------------------------*/

/* functions defined in .c */
void MPIDIG_part_sreq_create(MPIR_Request ** req);
void MPIDIG_part_rreq_create(MPIR_Request ** req);

void MPIDIG_part_sreq_set_cc_part(MPIR_Request * rqst);
void MPIDIG_part_rreq_reset_cc_part(MPIR_Request * rqst);

void MPIDIG_part_rreq_matched(MPIR_Request * rreq);
void MPIDIG_part_rreq_update_sinfo(MPIR_Request * rreq, MPIDIG_part_send_init_msg_t * msg_hdr);
void MPIDIG_Part_rreq_allocate(MPIR_Request * rreq);

MPL_STATIC_INLINE_PREFIX int MPIDIG_Part_get_vci(const int im)
{
    return (im % MPIDI_global.n_vcis);
}

/* given the bit encoding strategy returns the maximum tag id (included) */
MPL_STATIC_INLINE_PREFIX int MPIDIG_Part_get_max_tag(void)
{
    // in the VCI_tag approach we can use all the remaining bits (usable-15) + the 5 bits in front
    // of the tag
    return (MPIR_TAG_USABLE_BITS >> (15 - 5));
}

MPL_STATIC_INLINE_PREFIX int MPIDIG_Part_get_tag(const int im)
{
#if MPIDI_CH4_VCI_METHOD == MPICH_VCI__TAG
    const int mask = 0x1f;
    /* get the VCI id, we use symmetric VCI ids for the moment */
    const int vci = MPIDIG_Part_get_vci(im);
    /* encode the src and destination vci on bit [5-10[ and bit [10-15[. If the VCI doesn't fit on 5
     * bits then we only select the information that will. This is not an error we just only use the
     * VCI from 0 to 31 */
    int tag = 0;
    tag |= ((vci & 0x1f) << 5);
    tag |= ((vci & 0x1f) << 10);
    /* encode the msg id on the rest of the bits, if the message id is too big for the remaining
     * bits then it's an error to keep going as we will have conflicting tag ids on the network */
    MPIR_Assert(im <= MPIDIG_Part_get_max_tag());
    // register the first 5 bits of the partition id
    tag |= (im & mask);
    tag |= (im & ~mask) << 15;
    /* finally add the TAG bit to the tag */
    MPIR_TAG_SET_PART_BIT(tag);
    return tag;
#else
    MPIR_Assert(im <= MPIR_TAG_USABLE_BITS);
    return im;
#endif
}

/* Convert the 'origin' indexing layout into the 'arget' indexing layout.
 * For a given index in the origin (io) return the lower index to vising in the target indexing layout.
 * no = the number of indexes in the origin indexing layout
 * nt = the number of indexes in the target indexing layout
 */
MPL_STATIC_INLINE_PREFIX int MPIDIG_part_idx_lb(const int io, const int no, const int nt)
{
    return (io * nt) / no;
}

/* Convert the 'origin' indexing layout into the 'arget' indexing layout.
 * For a given index in the origin (io) return the upper index (not included!) to vising in the target indexing layout.
 * no = the number of indexes in the origin indexing layout
 * nt = the number of indexes in the target indexing layout
 */
MPL_STATIC_INLINE_PREFIX int MPIDIG_part_idx_ub(const int io, const int no, const int nt)
{
    const int tmp = (io + 1) * nt;
    const int it = tmp / no + (tmp % no > 0);
    return MPL_MIN(it, nt);
}


MPL_STATIC_INLINE_PREFIX void MPIDIG_Part_rreq_status_matched(MPIR_Request * rreq)
{
    MPIR_Assert(rreq->kind == MPIR_REQUEST_KIND__PART_RECV);
    MPIR_Assert(MPIR_cc_get(MPIDIG_PART_RREQUEST(rreq, status_matched)) == 0);
    MPIR_cc_inc(&MPIDIG_PART_RREQUEST(rreq, status_matched));
}

MPL_STATIC_INLINE_PREFIX void MPIDIG_Part_rreq_status_first_cts(MPIR_Request * rreq)
{
    MPIR_Assert(rreq->kind == MPIR_REQUEST_KIND__PART_RECV);
    MPIR_Assert(MPIR_cc_get(MPIDIG_PART_RREQUEST(rreq, status_matched)) == 1);
    MPIR_cc_inc(&MPIDIG_PART_RREQUEST(rreq, status_matched));
}

MPL_STATIC_INLINE_PREFIX bool MPIDIG_Part_rreq_status_has_matched(MPIR_Request * rreq)
{
    MPIR_Assert(rreq->kind == MPIR_REQUEST_KIND__PART_RECV);
    return MPIR_cc_get(MPIDIG_PART_RREQUEST(rreq, status_matched)) >= 1;
}

MPL_STATIC_INLINE_PREFIX bool MPIDIG_Part_rreq_status_has_first_cts(MPIR_Request * rreq)
{
    MPIR_Assert(rreq->kind == MPIR_REQUEST_KIND__PART_RECV);
    return MPIR_cc_get(MPIDIG_PART_RREQUEST(rreq, status_matched)) >= 2;
}

/* Receiver issues a CTS to sender once reach MPIDIG_PART_REQ_CTS */
MPL_STATIC_INLINE_PREFIX int MPIDIG_part_issue_cts(MPIR_Request * rreq_ptr)
{
    MPIR_Assert(rreq_ptr->kind == MPIR_REQUEST_KIND__PART_RECV);
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    MPIDIG_part_cts_msg_t am_hdr;
    am_hdr.msg_part = MPIDIG_PART_REQUEST(rreq_ptr, msg_part);
    am_hdr.sreq_ptr = MPIDIG_PART_REQUEST(rreq_ptr, peer_req_ptr);
    am_hdr.rreq_ptr = rreq_ptr;

    int source = MPIDI_PART_REQUEST(rreq_ptr, u.recv.source);
    CH4_CALL(am_send_hdr_reply(rreq_ptr->comm, source, MPIDIG_PART_CTS, &am_hdr, sizeof(am_hdr),
                               0, 0), MPIDI_REQUEST(rreq_ptr, is_local), mpi_errno);

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

/* Issue the receive request for the receiver for tag-matching approach
 * the non-blocking receive are issued at start time*/
MPL_STATIC_INLINE_PREFIX int MPIDIG_part_issue_recv(MPIR_Request * rreq)
{
    MPIR_Assert(rreq->kind == MPIR_REQUEST_KIND__PART_RECV);
    MPIR_Assert(MPIDIG_PART_DO_TAG(rreq));

    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    /* get the count per the msg partition and the shift in the buffer from the datatype */
    const int msg_part = MPIDIG_PART_REQUEST(rreq, msg_part);
    MPI_Aint count = MPIDI_PART_REQUEST(rreq, count) * rreq->u.part.partitions;
    MPIR_Assert(count % msg_part == 0);
    count /= msg_part;
    MPIR_Assert(count < MPIR_AINT_MAX);

    MPI_Aint part_offset;
    MPI_Datatype dtype_recv = MPIDI_PART_REQUEST(rreq, datatype);
    MPIR_Datatype_get_extent_macro(dtype_recv, part_offset);
    part_offset *= count;

    //const int count_recv = MPIDI_PART_REQUEST(rreq, count);
    const int source_rank = MPIDI_PART_REQUEST(rreq, u.recv.source);
    MPIR_cc_t *cc_ptr = rreq->cc_ptr;
    MPIR_Comm *comm = rreq->comm;
    MPIR_Assert(MPIR_cc_get(*cc_ptr) == msg_part);

    MPIR_Request **child_req = MPIDIG_PART_RREQUEST(rreq, tag_req_ptr);
    for (int im = 0; im < msg_part; ++im) {
        int source_tag = MPIDIG_Part_get_tag(im);
        void *buf_recv = (char *) MPIDI_PART_REQUEST(rreq, buffer) + im * part_offset;

        /* attr = 1 isolates the traffic of internal vs external communications */
        const int attr = 0;
        /* initialize the next request, the ref count should be 2 here: one for mpich, one for me */
        MPIR_Assert(child_req[im] == NULL);
        MPID_Irecv_parent(buf_recv, count, dtype_recv, source_rank, source_tag, comm, attr, cc_ptr,
                          child_req + im);
    }

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDIG_part_issue_send(const int imsg, MPIR_Request * sreq)
{
    MPIR_Assert(sreq->kind == MPIR_REQUEST_KIND__PART_SEND);
    MPIR_Assert(MPIDIG_PART_DO_TAG(sreq));

    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    /* get the count per the msg partition and the shift in the buffer from the datatype */
    const int msg_part = MPIDIG_PART_REQUEST(sreq, msg_part);
    MPI_Aint count = MPIDI_PART_REQUEST(sreq, count) * sreq->u.part.partitions;
    MPIR_Assert(count % msg_part == 0);
    count /= msg_part;
    MPIR_Assert(count < MPIR_AINT_MAX);

    MPI_Aint part_offset;
    MPI_Datatype *dtype_send = &MPIDI_PART_REQUEST(sreq, datatype);
    MPIR_Datatype_get_extent_macro(*dtype_send, part_offset);
    part_offset *= count;

    const int dest_rank = MPIDI_PART_REQUEST(sreq, u.send.dest);
    MPIR_cc_t *cc_ptr = sreq->cc_ptr;
    MPIR_Comm *comm = sreq->comm;

    int dest_tag = MPIDIG_Part_get_tag(imsg);
    void *buf_send = (char *) MPIDI_PART_REQUEST(sreq, buffer) + imsg * part_offset;
    MPIR_Request **child_req = MPIDIG_PART_SREQUEST(sreq, tag_req_ptr);

    /* attr = 1 isolates the traffic of internal vs external communications */
    const int attr = 0;
    MPIR_Assert(child_req[imsg] == NULL);
    MPID_Isend_parent(buf_send, count, *dtype_send, dest_rank, dest_tag, comm, attr, cc_ptr,
                      child_req + imsg);

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

enum MPIDIG_part_issue_mode {
    MPIDIG_PART_REGULAR,        /* when called outside of the progress engine */
    MPIDIG_PART_REPLY,          /* when called from the progress engine */
};

/* Sender issues data after all partitions are ready and CTS; called by pready functions. */
MPL_STATIC_INLINE_PREFIX int MPIDIG_part_issue_data(const int imsg, MPIR_Request * part_sreq,
                                                    enum MPIDIG_part_issue_mode mode)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Assert(MPIDIG_PART_DO_AM(part_sreq));
    MPIR_FUNC_ENTER;

    // TG: why is the user-request created with MPIDI_CH4_REQUEST_CREATE and this one a normal request on the VCI (target + source)

    // create a request specifically for this partition
    //  TODO change the VCI
    MPIR_Request *sreq = MPIDIG_request_create(MPIR_REQUEST_KIND__PART, 1, 0, 0);
    MPIR_ERR_CHKANDSTMT(sreq == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
    sreq->comm = part_sreq->comm;
    MPIR_Comm_add_ref(sreq->comm);

    /* we already know which request is assigned at the receiver
     * so there is no need to match anything here
     */
    const int msg_part = MPIDIG_PART_REQUEST(part_sreq, msg_part);
    MPIR_Assert(msg_part >= 0);

    MPIDIG_part_send_data_msg_t am_hdr;
    am_hdr.rreq_ptr = MPIDIG_PART_REQUEST(part_sreq, peer_req_ptr);
    am_hdr.imsg = imsg;
    MPIR_Assert(imsg >= 0);
    MPIR_Assert(imsg < msg_part);

    /* Complete partitioned req at am request completes */
    sreq->dev.completion_notification = &part_sreq->cc;
    /* Will update part_sreq status when the AM request completes
     * TODO: can we get rid of the pointer? */
    MPIDIG_REQUEST(sreq, req->part_am_req.part_req_ptr) = part_sreq;

    /* get the count per the msg partition and the shift in the buffer from the datatype */
    MPI_Aint count = MPIDI_PART_REQUEST(part_sreq, count) * part_sreq->u.part.partitions;
    MPIR_Assert(count % msg_part == 0);
    count /= msg_part;
    MPIR_Assert(count < MPIR_AINT_MAX);

    MPI_Aint part_offset;
    MPIR_Datatype_get_extent_macro(MPIDI_PART_REQUEST(part_sreq, datatype), part_offset);
    part_offset *= count;
    const void *part_buf = (char *) MPIDI_PART_REQUEST(part_sreq, buffer) + imsg * part_offset;

    if (mode == MPIDIG_PART_REGULAR) {
        CH4_CALL(am_isend
                 (/*matching data */ MPIDI_PART_REQUEST(part_sreq, u.send.dest), part_sreq->comm,
                  MPIDIG_PART_SEND_DATA,
                  /* header */ &am_hdr, sizeof(am_hdr),
                  /* buff */ part_buf, count, MPIDI_PART_REQUEST(part_sreq, datatype), 0, 0, sreq),
                 MPIDI_REQUEST(part_sreq, is_local), mpi_errno);
    } else {    /* MPIDIG_PART_REPLY */
        CH4_CALL(am_isend_reply(part_sreq->comm, MPIDI_PART_REQUEST(part_sreq, u.send.dest),
                                MPIDIG_PART_SEND_DATA, &am_hdr, sizeof(am_hdr), part_buf, count,
                                MPIDI_PART_REQUEST(part_sreq, datatype), 0, 0, sreq),
                 MPIDI_REQUEST(part_sreq, is_local), mpi_errno);
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}



MPL_STATIC_INLINE_PREFIX int MPIDIG_part_issue_msg_if_ready(const int msg_id,
                                                            MPIR_Request * sreq,
                                                            enum MPIDIG_part_issue_mode mode)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;
    MPIR_Assert(sreq->kind == MPIR_REQUEST_KIND__PART_SEND);

    const bool do_tag = MPIDIG_PART_DO_TAG(sreq);
    MPIR_cc_t *cc_msg = MPIDIG_PART_SREQUEST(sreq, cc_msg);
    /* decrement the counter of the specific msg */

    int incomplete;
    MPIR_cc_decr(cc_msg + msg_id, &incomplete);
    if (!incomplete) {
        // actually send the data
        if (do_tag) {
            // no need to reset the cc_send, it will be used over the entire simulation to
            // acknowledge CTS reception sending data
            // the lock in the VCI happens inside the send function
            mpi_errno = MPIDIG_part_issue_send(msg_id, sreq);
        } else {
            // decrease the counter of the number of msgs requested to be sent, must be done BEFORE
            // the last send to avoid data races with the MPI_Start resetting the cc_send value
            int no_reset;
            MPIR_cc_decr(&MPIDIG_PART_SREQUEST(sreq, cc_send), &no_reset);
            // if we have requested the send of every msgs then we can reset the counters, must be
            // done BEFORE the last send to avoid conflict with an early CTS arriving from the
            // receiver
            if (!no_reset) {
                const int n_part = sreq->u.part.partitions;
                MPIR_cc_t *cc_part = MPIDIG_PART_SREQUEST(sreq, cc_part);
                const int init_value = MPIDIG_PART_STATUS_SEND_AM_INIT;
                for (int ip = 0; ip < n_part; ++ip) {
                    MPIR_cc_set(&cc_part[ip], init_value);
                }
            }
            // sending data
            MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(0).lock);
            mpi_errno = MPIDIG_part_issue_data(msg_id, sreq, mode);
            MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(0).lock);
        }
        MPIR_ERR_CHECK(mpi_errno);
        // we have to explicitly increment the progress counter to ensure progress on the main
        // thread when using multiple VCIs
        const int vci_id = MPIDI_Request_get_vci(sreq);
        MPL_atomic_fetch_add_int(&MPIDI_VCI(vci_id).progress_count, 1);
    }
  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* MPIDIG_PART_UTILS_H_INCLUDED */

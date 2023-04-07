/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "mpidig_part_utils.h"
#include "mpidpre.h"
#include "mpl_base.h"

/* creates a MPIDIG SEND request */
void MPIDIG_part_sreq_create(MPIR_Request ** req)
{
    MPIR_Request *sreq = req[0];
    MPIR_Assert(sreq->kind == MPIR_REQUEST_KIND__PART_SEND);

    /*allocate the cc_part array to the send_part size */
    const int n_part = sreq->u.part.partitions;
    MPIDIG_PART_REQUEST(sreq, msg_part) = -1;
    MPIDIG_PART_REQUEST(sreq, peer_req_ptr) = NULL;
    MPIR_cc_set(&MPIDIG_PART_SREQUEST(sreq, cc_send), 0);
    MPIDIG_PART_SREQUEST(sreq, cc_msg) = NULL;
    MPIDIG_PART_SREQUEST(sreq, tag_req_ptr) = NULL;
    MPIDIG_PART_SREQUEST(sreq, cc_part) = MPL_malloc(sizeof(MPIR_cc_t) * n_part, MPL_MEM_OTHER);
}

/* creates a MPIDIG RECV request */
void MPIDIG_part_rreq_create(MPIR_Request ** req)
{
    MPIR_Request *rreq = req[0];
    MPIR_Assert(rreq->kind == MPIR_REQUEST_KIND__PART_RECV);

    MPIDIG_PART_REQUEST(rreq, msg_part) = -1;
    MPIDIG_PART_REQUEST(rreq, peer_req_ptr) = NULL;
    MPIR_cc_set(&MPIDIG_PART_REQUEST(rreq, u.recv.status_matched), 0);

    MPIDIG_PART_RREQUEST(rreq, tag_req_ptr) = NULL;
}

void MPIDIG_Part_rreq_allocate(MPIR_Request * rreq)
{
    MPIR_Assert(rreq->kind == MPIR_REQUEST_KIND__PART_RECV);

    /* allocate partitions tracking */
    const int msg_part = MPIDIG_PART_REQUEST(rreq, msg_part);
    MPIR_Assert(msg_part > 0);

    /* if we do the tag matching then we need an array of request */
    const bool do_tag = MPIDIG_PART_DO_TAG(rreq);
    if (do_tag) {
        MPIDIG_PART_RREQUEST(rreq, tag_req_ptr) =
            MPL_malloc(sizeof(MPIR_Request *) * msg_part, MPL_MEM_OTHER);
        for (int i = 0; i < msg_part; ++i) {
            MPIDIG_PART_RREQUEST(rreq, tag_req_ptr[i]) = NULL;
        }
    } else {
        MPIDIG_PART_RREQUEST(rreq, cc_part) =
            MPL_malloc(sizeof(MPIR_cc_t) * msg_part, MPL_MEM_OTHER);
    }
}


/* return the greatest divisor of a, smaller than the limit L */
MPL_STATIC_INLINE_PREFIX int gdst(int a, int L)
{
    MPIR_Assert(L >= 1);
    int res = MPL_MIN(a, L);
    while (a % res) {
        res--;
    }
    return res;
}

/* called when a receive Request has been matched
 * - set the status
 * - allocate the cc_part
 * */
void MPIDIG_part_rreq_matched(MPIR_Request * rreq)
{
    MPIR_Assert(rreq->kind == MPIR_REQUEST_KIND__PART_RECV);

    /* Set status for partitioned req */
    MPI_Aint sdata_size = MPIDIG_PART_REQUEST(rreq, u.recv.send_dsize);
    MPIR_STATUS_SET_COUNT(rreq->status, sdata_size);
    rreq->status.MPI_SOURCE = MPIDI_PART_REQUEST(rreq, u.recv.source);
    rreq->status.MPI_TAG = MPIDI_PART_REQUEST(rreq, u.recv.tag);
    rreq->status.MPI_ERROR = MPI_SUCCESS;

    /* now that we have matched the receiver decides how many msgs are used
     * if the total number of datatypes can be divided by the number of send partition
     * we use the number of send partitions to communicate datat
     * if the modulo is not null then we use the gcd approach */
    const int send_part = MPIDIG_PART_REQUEST(rreq, msg_part);
    const int recv_part = rreq->u.part.partitions;

    /* we must guarantee that the one partition on both the send and recv side corresponds to only
     * one actual msgs */
    //MPIDIG_PART_REQUEST(rreq, msg_part) = MPL_gcd(send_part, recv_part);
    const int n_msg = MPL_gcd(send_part, recv_part);

    /* try to gather messages together to reduce the latency: get the number byte in 1 msg and then
     * get the GCD(number of msgs, number of msgs/chunk). This we are sure that we always send
     * multiples of datatypes*/

    /* gather the messages to try to match the size of a chunk, the results must be a multiple of
     * the number of partitions (and therefore a multiple of the number of msgs)
     * - get the number of bytes in 1 msg
     * - get the MAX number of msgs that fits within one chunk of size MPIR_CVAR_PART_AGGR_SIZE
     * - find the largest factor of the number of msgs that is lower then the max number of msgs
     * - the number of messages is the original msg one divided by that factor*/
    MPI_Aint part_byte;
    MPIR_Datatype_get_size_macro(MPIDI_PART_REQUEST(rreq, datatype), part_byte);
    part_byte *= MPIDI_PART_REQUEST(rreq, count) * recv_part / n_msg;
    const int max_msg_per_chunk = MPL_MAX(1, MPIR_CVAR_PART_AGGR_SIZE / part_byte);
    MPIDIG_PART_REQUEST(rreq, msg_part) =
        (MPIR_CVAR_PART_AGGR_SIZE > 0) ? n_msg / gdst(n_msg, max_msg_per_chunk) : n_msg;

    /* 0 partition is illegual so at least one message must happen */
    MPIR_Assert(MPIDIG_PART_REQUEST(rreq, msg_part) > 0);

    /* Additional check for partitioned pt2pt: require identical buffer size */
    if (rreq->status.MPI_ERROR == MPI_SUCCESS) {
        MPI_Aint rdata_size;
        MPIR_Datatype_get_size_macro(MPIDI_PART_REQUEST(rreq, datatype), rdata_size);
        rdata_size *= MPIDI_PART_REQUEST(rreq, count) * rreq->u.part.partitions;
        if (sdata_size != rdata_size) {
            rreq->status.MPI_ERROR =
                MPIR_Err_create_code(rreq->status.MPI_ERROR, MPIR_ERR_RECOVERABLE, __FUNCTION__,
                                     __LINE__, MPI_ERR_OTHER, "**ch4|partmismatchsize",
                                     "**ch4|partmismatchsize %d %d",
                                     (int) rdata_size, (int) sdata_size);
        }
    }

    /* indicate that we have matched */
    MPIDIG_Part_rreq_status_matched(rreq);
}

/* partition recv request - reset cc_part array of an activated request */
void MPIDIG_part_rreq_reset_cc_part(MPIR_Request * rqst)
{
    MPIR_Assert(MPIDIG_Part_rreq_status_has_matched(rqst));
    MPIR_Assert(rqst->kind == MPIR_REQUEST_KIND__PART_RECV);
    MPIR_Assert(!MPIDIG_PART_DO_TAG(rqst));

    /* reset the counters to the init value */
    const int msg_part = MPIDIG_PART_REQUEST(rqst, msg_part);
    MPIR_cc_t *cc_part = MPIDIG_PART_RREQUEST(rqst, cc_part);
    MPIR_Assert(msg_part >= 0);
    for (int i = 0; i < msg_part; ++i) {
        MPIR_cc_set(&cc_part[i], MPIDIG_PART_STATUS_RECV_INIT);
    }
}

/* partition send requests - resets the cc_part */
void MPIDIG_part_sreq_set_cc_part(MPIR_Request * rqst)
{
    MPIR_Assert(rqst->kind == MPIR_REQUEST_KIND__PART_SEND);

    const int init_value = MPIDIG_PART_DO_TAG(rqst) ? MPIDIG_PART_STATUS_SEND_TAG_INIT
        : MPIDIG_PART_STATUS_SEND_AM_INIT;
    const int send_part = rqst->u.part.partitions;
    MPIR_cc_t *cc_part = MPIDIG_PART_SREQUEST(rqst, cc_part);
    MPIR_Assert(send_part >= 0);
    for (int i = 0; i < send_part; ++i) {
        MPIR_cc_set(&cc_part[i], init_value);
    }
}

/* partition recv requests - update the request with header information*/
void MPIDIG_part_rreq_update_sinfo(MPIR_Request * rreq, MPIDIG_part_send_init_msg_t * msg_hdr)
{
    MPIR_Assert(rreq->kind == MPIR_REQUEST_KIND__PART_RECV);

    MPIDIG_PART_REQUEST(rreq, u.recv.send_dsize) = msg_hdr->data_sz;
    /* if do_tag in the header, mode is 1 */
    MPIDIG_PART_REQUEST(rreq, mode) = msg_hdr->do_tag;
    /* store the sender number of partitions in the msg_part temporarily */
    MPIDIG_PART_REQUEST(rreq, msg_part) = msg_hdr->send_npart;
    MPIR_Assert(MPIDIG_PART_REQUEST(rreq, msg_part) > 0);
    MPIDIG_PART_REQUEST(rreq, peer_req_ptr) = msg_hdr->sreq_ptr;
}

/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2019 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#include "rndv.h"
#include "shm_control.h"

/* Construct the LMT RTS information and send it to the receiver
 *
 * buf - The data to be sent via the LMT protocol
 * grank - The global rank information for the receiver
 * tag - The tag of the message being sent
 * error_bits - The error bits information used by collective to track whether an error occurred
 * comm - The communicator used to send the message
 * data_sz - The size of the data stored in buf
 * sreq - The send request being used to store all of the metadata
 */
int MPIDI_POSIX_lmt_send_rts(void *buf, int grank, int tag, int error_bits, MPIR_Comm * comm,
                             size_t data_sz, int handler_id, MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_SHM_ctrl_hdr_t ctrl_hdr;
    MPIDI_SHM_lmt_rndv_long_msg_t *long_msg_hdr = &ctrl_hdr.lmt_rndv_long_msg;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_LMT_SEND_RTS);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_LMT_SEND_RTS);

    MPIR_Datatype_add_ref_if_not_builtin(MPIDIG_REQUEST(sreq, datatype));

    MPIDI_Datatype_check_size(MPIDIG_REQUEST(sreq, datatype), MPIDIG_REQUEST(sreq, count),
                              MPIDI_POSIX_AMREQUEST(sreq, lmt.lmt_data_sz));
    MPIDI_POSIX_AMREQUEST(sreq, lmt.lmt_msg_offset) = 0;
    MPIDI_POSIX_AMREQUEST(sreq, lmt.lmt_buf_num) = 0;

    /* Construct and send the RTS message with the control message protocol. */

    long_msg_hdr->sreq_ptr = (uint64_t) sreq;

    long_msg_hdr->rank = comm->rank;
    long_msg_hdr->tag = tag;
    long_msg_hdr->context_id = MPIDIG_REQUEST(sreq, context_id);
    long_msg_hdr->error_bits = error_bits;
    long_msg_hdr->data_sz = data_sz;
    long_msg_hdr->handler_id = handler_id;

    mpi_errno = MPIDI_SHM_do_ctrl_send(MPIDIG_REQUEST(sreq, rank), comm, MPIDI_SHM_SEND_LMT_MSG,
                                       &ctrl_hdr);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_LMT_SEND_RTS);
    return mpi_errno;
}

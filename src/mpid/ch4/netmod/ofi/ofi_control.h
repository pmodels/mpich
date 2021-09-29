/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef OFI_CONTROL_H_INCLUDED
#define OFI_CONTROL_H_INCLUDED

#include "ofi_am_impl.h"

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_do_control_send(MPIDI_OFI_send_control_t * control,
                                                       char *send_buf,
                                                       size_t msgsize,
                                                       int rank, MPIR_Comm * comm_ptr,
                                                       MPIR_Request * ackreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    control->origin_rank = comm_ptr->rank;
    control->send_buf = (uintptr_t) send_buf;
    control->msgsize = msgsize;
    control->comm_id = comm_ptr->context_id;
    control->ackreq = ackreq;

    mpi_errno = MPIDI_OFI_do_inject(rank, comm_ptr,
                                    MPIDI_OFI_INTERNAL_HANDLER_CONTROL,
                                    (void *) control, sizeof(*control), 0, 0);

    MPIR_FUNC_EXIT;
    return mpi_errno;
}


#endif /* OFI_CONTROL_H_INCLUDED */

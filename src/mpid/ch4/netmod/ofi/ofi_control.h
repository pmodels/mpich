/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef OFI_CONTROL_H_INCLUDED
#define OFI_CONTROL_H_INCLUDED

#include "ofi_am_impl.h"

#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_do_control_win
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_OFI_do_control_win(MPIDI_OFI_win_control_t * control,
                                           int rank, MPIR_Win * win, int use_comm, int use_lock)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_DO_CONTROL_WIN);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_DO_CONTROL_WIN);

    control->win_id = MPIDI_OFI_WIN(win).win_id;
    control->origin_rank = win->comm_ptr->rank;

    mpi_errno = MPIDI_OFI_do_inject(rank,
                                    win->comm_ptr,
                                    MPIDI_OFI_INTERNAL_HANDLER_CONTROL,
                                    (void *) control,
                                    sizeof(*control), FALSE, use_comm, use_lock);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_DO_CONTROL_WIN);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_do_control_send
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_OFI_do_control_send(MPIDI_OFI_send_control_t * control,
                                            char *send_buf,
                                            size_t msgsize,
                                            int rank,
                                            MPIR_Comm * comm_ptr,
                                            MPIR_Request * ackreq, int need_lock)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_DO_CONTROL_SEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_DO_CONTROL_SEND);

    control->origin_rank = comm_ptr->rank;
    control->send_buf = (uintptr_t) send_buf;
    control->msgsize = msgsize;
    control->comm_id = comm_ptr->context_id;
    control->endpoint_id = MPIDI_OFI_comm_to_ep(comm_ptr, comm_ptr->rank);
    control->ackreq = ackreq;

    mpi_errno = MPIDI_OFI_do_inject(rank, comm_ptr,
                                    MPIDI_OFI_INTERNAL_HANDLER_CONTROL,
                                    (void *) control,
                                    sizeof(*control), FALSE, TRUE, need_lock);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_DO_CONTROL_SEND);
    return mpi_errno;
}


#endif /* OFI_CONTROL_H_INCLUDED */

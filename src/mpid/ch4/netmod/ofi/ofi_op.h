/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2012 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef OFI_OP_H_INCLUDED
#define OFI_OP_H_INCLUDED

#include "ofi_impl.h"

static inline int MPIDI_NM_mpi_op_free_hook(MPIR_Op * op_p)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_OP_FREE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_OP_FREE_HOOK);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_OP_FREE_HOOK);
    return 0;
}

static inline int MPIDI_NM_mpi_op_commit_hook(MPIR_Op * op_p)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_OP_COMMIT_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_OP_COMMIT_HOOK);

    MPIDI_OFI_op_t *ofi_op = &MPIDI_OFI_OP(op_p);
    ofi_op->op_mpich_kary.tsp_op.mpi_op     = op_p->handle;
    ofi_op->op_mpich_knomial.tsp_op.mpi_op  = op_p->handle;
    ofi_op->op_mpich_dissem.tsp_op.mpi_op = op_p->handle;
    ofi_op->op_mpich_recexch.tsp_op.mpi_op = op_p->handle;

    ofi_op->op_triggered_kary.tsp_op.mpi_op     = op_p->handle;
    ofi_op->op_triggered_knomial.tsp_op.mpi_op  = op_p->handle;
    ofi_op->op_triggered_dissem.tsp_op.mpi_op = op_p->handle;
    ofi_op->op_triggered_recexch.tsp_op.mpi_op = op_p->handle;

    ofi_op->op_stub_kary.tsp_op.dummy     = -1;
    ofi_op->op_stub_knomial.tsp_op.dummy  = -1;
    ofi_op->op_stub_dissem.tsp_op.dummy = -1;
    ofi_op->op_stub_recexch.tsp_op.dummy = -1;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_OP_COMMIT_HOOK);
    return 0;
}

#endif /* OFI_OP_H_INCLUDED */


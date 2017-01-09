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
#ifndef OFI_DATATYPE_H_INCLUDED
#define OFI_DATATYPE_H_INCLUDED

#include "ofi_impl.h"

static inline int MPIDI_NM_mpi_type_free_hook(MPIR_Datatype * datatype_p)
{
    return 0;
}

static inline int MPIDI_NM_mpi_type_commit_hook(MPIR_Datatype * datatype_p)
{
    MPIDI_OFI_dt_t *ofi_dt = &MPIDI_OFI_DT(datatype_p);
    ofi_dt->dt_mpich_kary.tsp_dt.mpi_dt     = datatype_p->handle;
    ofi_dt->dt_mpich_knomial.tsp_dt.mpi_dt  = datatype_p->handle;
    ofi_dt->dt_mpich_dissem.tsp_dt.mpi_dt = datatype_p->handle;
    ofi_dt->dt_mpich_recexch.tsp_dt.mpi_dt = datatype_p->handle;

//    ofi_dt->dt_triggered_kary.tsp_dt.mpi_dt     = datatype_p->handle;
//    ofi_dt->dt_triggered_knomial.tsp_dt.mpi_dt  = datatype_p->handle;
//    ofi_dt->dt_triggered_dissem.tsp_dt.mpi_dt = datatype_p->handle;
//    ofi_dt->dt_triggered_recexch.tsp_dt.mpi_dt = datatype_p->handle;

    ofi_dt->dt_stub_kary.tsp_dt.dummy     = -1;
    ofi_dt->dt_stub_knomial.tsp_dt.dummy  = -1;
    ofi_dt->dt_stub_dissem.tsp_dt.dummy = -1;
    ofi_dt->dt_stub_recexch.tsp_dt.dummy = -1;
    return 0;
}

#endif /* OFI_DATATYPE_H_INCLUDED */

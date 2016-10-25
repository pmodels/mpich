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
#ifndef OFI_COMM_H_INCLUDED
#define OFI_COMM_H_INCLUDED

#include "ofi_impl.h"
#include "ofi_coll_impl.h"
#include "mpl_utlist.h"

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_comm_create_hook
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_comm_create_hook(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_COMM_CREATE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_COMM_CREATE_HOOK);

    MPIDI_OFI_map_create(&MPIDI_OFI_COMM(comm).huge_send_counters);
    MPIDI_OFI_map_create(&MPIDI_OFI_COMM(comm).huge_recv_counters);
    MPIDI_OFI_index_allocator_create(&MPIDI_OFI_COMM(comm).win_id_allocator,0);
    MPIDI_OFI_index_allocator_create(&MPIDI_OFI_COMM(comm).rma_id_allocator,1);
    MPIDI_OFI_COMM(comm).issued_collectives = 0;
    MPIDI_OFI_COMM(comm).use_tag = 0;
    mpi_errno = MPIDI_CH4U_init_comm(comm);

    /* no connection for non-dynamic or non-root-rank of intercomm */
    MPIDI_OFI_COMM(comm).conn_id = -1;

    /* Do not handle intercomms */
    if (comm->comm_kind == MPIR_COMM_KIND__INTERCOMM)
        goto fn_exit;

    MPIR_Assert(comm->coll_fns != NULL);
    MPIDI_OFI_COMM(comm).comm_mpich_kary.tsp_comm.mpid_comm         = comm;
    MPIDI_OFI_COMM(comm).comm_mpich_knomial.tsp_comm.mpid_comm      = comm;
    MPIDI_OFI_COMM(comm).comm_mpich_dissem.tsp_comm.mpid_comm       = comm;
    MPIDI_OFI_COMM(comm).comm_mpich_recexch.tsp_comm.mpid_comm      = comm;

    MPIDI_OFI_COMM(comm).comm_triggered_kary.tsp_comm.mpid_comm     = comm;
    MPIDI_OFI_COMM(comm).comm_triggered_knomial.tsp_comm.mpid_comm  = comm;
    MPIDI_OFI_COMM(comm).comm_triggered_dissem.tsp_comm.mpid_comm   = comm;
    MPIDI_OFI_COMM(comm).comm_triggered_recexch.tsp_comm.mpid_comm  = comm;

    MPIDI_OFI_COMM(comm).comm_stub_kary.tsp_comm.dummy              = -1;
    MPIDI_OFI_COMM(comm).comm_stub_knomial.tsp_comm.dummy           = -1;
    MPIDI_OFI_COMM(comm).comm_stub_dissem.tsp_comm.dummy            = -1;
    MPIDI_OFI_COMM(comm).comm_mpich_stub.tsp_comm.mpid_comm         = comm;
    MPIDI_OFI_COMM(comm).comm_stub_stub.tsp_comm.dummy              = -1;
    MPIDI_OFI_COMM(comm).comm_stub_recexch.tsp_comm.dummy           = -1;

    MPIDI_OFI_COMM(comm).comm_shm_gr.dummy                          = -1;

    MPIDI_OFI_COLL_MPICH_KARY_comm_init(&MPIDI_OFI_COMM(comm).comm_mpich_kary,
                                        &MPIDI_OFI_COMM(comm).use_tag);
    MPIDI_OFI_COLL_MPICH_KNOMIAL_comm_init(&MPIDI_OFI_COMM(comm).comm_mpich_knomial,
                                           &MPIDI_OFI_COMM(comm).use_tag);
    MPIDI_OFI_COLL_MPICH_DISSEM_comm_init(&MPIDI_OFI_COMM(comm).comm_mpich_dissem,
                                          &MPIDI_OFI_COMM(comm).use_tag);
    MPIDI_OFI_COLL_MPICH_RECEXCH_comm_init(&MPIDI_OFI_COMM(comm).comm_mpich_recexch,
                                           &MPIDI_OFI_COMM(comm).use_tag);

    MPIDI_OFI_COLL_TRIGGERED_KARY_comm_init(&MPIDI_OFI_COMM(comm).comm_triggered_kary,
                                            &MPIDI_OFI_COMM(comm).use_tag);
    MPIDI_OFI_COLL_TRIGGERED_KNOMIAL_comm_init(&MPIDI_OFI_COMM(comm).comm_triggered_knomial,
                                               &MPIDI_OFI_COMM(comm).use_tag);
    MPIDI_OFI_COLL_TRIGGERED_DISSEM_comm_init(&MPIDI_OFI_COMM(comm).comm_triggered_dissem,
                                              &MPIDI_OFI_COMM(comm).use_tag);
    MPIDI_OFI_COLL_TRIGGERED_RECEXCH_comm_init(&MPIDI_OFI_COMM(comm).comm_triggered_recexch,
                                               &MPIDI_OFI_COMM(comm).use_tag);

    MPIDI_OFI_COLL_STUB_KARY_comm_init(&MPIDI_OFI_COMM(comm).comm_stub_kary,
                                       &MPIDI_OFI_COMM(comm).use_tag);
    MPIDI_OFI_COLL_STUB_KNOMIAL_comm_init(&MPIDI_OFI_COMM(comm).comm_stub_knomial,
                                          &MPIDI_OFI_COMM(comm).use_tag);
    MPIDI_OFI_COLL_STUB_DISSEM_comm_init(&MPIDI_OFI_COMM(comm).comm_stub_dissem,
                                         &MPIDI_OFI_COMM(comm).use_tag);
    MPIDI_OFI_COLL_STUB_RECEXCH_comm_init(&MPIDI_OFI_COMM(comm).comm_stub_recexch,
                                          &MPIDI_OFI_COMM(comm).use_tag);

    MPIDI_OFI_COLL_STUB_STUB_comm_init(&MPIDI_OFI_COMM(comm).comm_stub_stub);
    MPIDI_OFI_COLL_MPICH_STUB_comm_init(&MPIDI_OFI_COMM(comm).comm_mpich_stub);

    MPIDI_OFI_COLL_SHM_GR_comm_init(&MPIDI_OFI_COMM(comm).comm_shm_gr);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_COMM_CREATE_HOOK);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_comm_free_hook
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_comm_free_hook(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_COMM_FREE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_COMM_FREE_HOOK);

    mpi_errno = MPIDI_CH4U_destroy_comm(comm);
    MPIDI_OFI_map_destroy(MPIDI_OFI_COMM(comm).huge_send_counters);
    MPIDI_OFI_map_destroy(MPIDI_OFI_COMM(comm).huge_recv_counters);
    MPIDI_OFI_index_allocator_destroy(MPIDI_OFI_COMM(comm).win_id_allocator);
    MPIDI_OFI_index_allocator_destroy(MPIDI_OFI_COMM(comm).rma_id_allocator);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_COMM_FREE_HOOK);
    return mpi_errno;
}


#endif /* OFI_COMM_H_INCLUDED */

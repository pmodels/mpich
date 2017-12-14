/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2010 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "collutil.h"


struct MPII_Ibcast_status{
    int curr_bytes;
    int n_bytes;
    MPI_Status status;
};

int MPII_sched_test_length(MPIR_Comm * comm, int tag, void *state);
int MPII_sched_test_curr_length(MPIR_Comm * comm, int tag, void *state);
int MPII_sched_add_length(MPIR_Comm * comm, int tag, void *state);
int MPII_Ibcast_scatter_ring_allgather_sched(void *buffer, int count, MPI_Datatype datatype, int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s);


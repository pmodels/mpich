/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2010 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef IBCAST_H_INCLUDED
#define IBCAST_H_INCLUDED

#include "mpiimpl.h"


struct MPII_Ibcast_status{
    int curr_bytes;
    int n_bytes;
    MPI_Status status;
};

int MPII_sched_test_length(MPIR_Comm * comm, int tag, void *state);
int MPII_sched_test_curr_length(MPIR_Comm * comm, int tag, void *state);
int MPII_sched_add_length(MPIR_Comm * comm, int tag, void *state);
int MPII_Iscatter_for_bcast_sched(void *tmp_buf, int root, MPIR_Comm *comm_ptr, int nbytes, MPIR_Sched_t s);

#endif /* IBCAST_H_INCLUDED */

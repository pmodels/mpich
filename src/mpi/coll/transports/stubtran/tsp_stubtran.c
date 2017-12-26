/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2017 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */


#include "mpiimpl.h"
#include "tsp_stubtran.h"

int MPII_Stubutil_sched_create(MPII_Stubutil_sched_t * sched, int tag)
{
    return MPI_SUCCESS;
}

int MPII_Stubutil_sched_isend(const void *buf, int count, MPI_Datatype dt, int dest, int tag,
                              MPIR_Comm * comm_ptr, MPII_Stubutil_sched_t * sched,
                              int n_invtcs, int *invtcs)
{
    return MPI_SUCCESS;
}

int MPII_Stubutil_sched_imcast(const void *buf, int count, MPI_Datatype dt, int *destinations,
                               int num_destinations, int tag, MPIR_Comm * comm_ptr,
                               MPII_Stubutil_sched_t * sched, int n_invtcs, int *invtcs)
{
    return MPI_SUCCESS;
}

int MPII_Stubutil_sched_irecv(void *buf, int count, MPI_Datatype datatype, int source, int tag,
                              MPIR_Comm * comm_ptr, MPII_Stubutil_sched_t * sched,
                              int n_invtcs, int *invtcs)
{
    return MPI_SUCCESS;
}

int MPII_Stubutil_sched_start(MPII_Stubutil_sched_t * sched, MPIR_Comm * comm,
                              MPII_Coll_req_t ** request)
{
    return MPI_SUCCESS;
}

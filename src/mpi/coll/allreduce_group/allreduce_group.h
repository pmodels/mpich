/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

#ifndef ALLREDUCE_GROUP_H_INCLUDED
#define ALLREDUCE_GROUP_H_INCLUDED

int MPII_Allreduce_group(void *sendbuf, void *recvbuf, MPI_Aint count,
                         MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                         MPIR_Group * group_ptr, int tag, MPIR_Errflag_t * errflag);
int MPII_Allreduce_group_intra(void *sendbuf, void *recvbuf, MPI_Aint count,
                               MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                               MPIR_Group * group_ptr, int tag, MPIR_Errflag_t * errflag);

#endif /* ALLREDUCE_GROUP_H_INCLUDED */

/*
 * Copyright 2026 Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mpiimpl.h"

#ifndef ALLREDUCE_GROUP_H_INCLUDED
#define ALLREDUCE_GROUP_H_INCLUDED

int MPII_Allreduce_group(void *sendbuf, void *recvbuf, MPI_Aint count,
                         MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                         MPIR_Group * group_ptr, int tag, int coll_attr);
int MPII_Allreduce_group_intra(void *sendbuf, void *recvbuf, MPI_Aint count,
                               MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                               MPIR_Group * group_ptr, int tag, int coll_attr);

#endif /* ALLREDUCE_GROUP_H_INCLUDED */

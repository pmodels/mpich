/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef BCAST_H_INCLUDED
#define BCAST_H_INCLUDED

#include "mpiimpl.h"

int MPII_Scatter_for_bcast(void *buffer, MPI_Aint count, MPI_Datatype datatype,
                           int root, MPIR_Comm * comm_ptr, MPI_Aint nbytes, void *tmp_buf,
                           int is_contig, MPIR_Errflag_t errflag);
bool find_local_rank(int* group, int group_size, int rank, int* group_rank);

int MPII_Scatter_for_bcast_group(void *buffer, MPI_Aint count, MPI_Datatype datatype,
                           int root, MPIR_Comm * comm_ptr, int* group, int group_size, MPI_Aint nbytes, void *tmp_buf,
                           int is_contig, MPIR_Errflag_t errflag);
#endif /* BCAST_H_INCLUDED */

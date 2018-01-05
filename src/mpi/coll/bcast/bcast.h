/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

int MPII_Scatter_for_bcast( void *buffer, int count, MPI_Datatype datatype,
        int root, MPIR_Comm *comm_ptr, int nbytes, void *tmp_buf,
        int is_contig, int is_homogeneous, MPIR_Errflag_t *errflag);

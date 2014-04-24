
/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>

#include "mpidimpl.h"

int main(int argc, char **argv)
{
    int rc, size, rank, ec;
    MPID_Comm *comm_ptr;
    uint32_t *mask;

    MPI_Init(&argc, &argv);

    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);

    if (size != 16) {
        fprintf(stderr, "Requires 16 ranks\n");
        exit(1);
    }

    if (rank == 1  ||
        rank == 5  ||
        rank == 6  ||
        rank == 9  ||
        rank == 12)
        exit(1);

    rc = MPI_Barrier(MPI_COMM_WORLD);
    ec = MPI_Error_class(rc, &ec);
    if (MPI_SUCCESS == rc) {
        fprintf(stderr, "[%d] ERROR CLASS: %d\n", rank, rc);
    }

    MPID_Comm_get_ptr(MPI_COMM_WORLD, comm_ptr);

    MPID_Comm_failed_bitarray(comm_ptr, &mask, 0);

    if (mask[0] != (uint32_t) 0x46480000) {
        fprintf(stderr, "[%d] Unexpected failure bitmask: 0x%x\n", rank, mask[0]);
        exit(1);
    } else {
        fprintf(stdout, " No errors\n");
    }

    MPIU_Free(mask);

    MPI_Finalize();

    return 0;
}

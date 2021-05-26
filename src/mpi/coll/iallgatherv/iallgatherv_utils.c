/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "iallgatherv.h"

/* This function checks whether the displacements are in increasing order and
 * that there is no overlap or gap between the data of successive ranks. Some
 * algorithms can only handle ordered array of data and hence this function for
 * checking whether the data is ordered.
 */
int MPII_Iallgatherv_is_displs_ordered(int size, const MPI_Aint recvcounts[],
                                       const MPI_Aint displs[])
{
    int i, pos = 0;
    for (i = 0; i < size; i++) {
        if (pos != displs[i]) {
            return 0;
        }
        pos += recvcounts[i];
    }
    return 1;
}

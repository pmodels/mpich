/*
 * Copyright 2026 Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef IALLGATHERV_H_INCLUDED
#define IALLGATHERV_H_INCLUDED

#include "mpiimpl.h"

int MPII_Iallgatherv_is_displs_ordered(int size, const MPI_Aint recvcounts[],
                                       const MPI_Aint displs[]);

#endif /* IALLGATHERV_H_INCLUDED */

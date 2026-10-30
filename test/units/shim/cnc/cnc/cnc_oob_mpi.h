/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef CNC_OOB_MPI_SHIM_H_INCLUDED
#define CNC_OOB_MPI_SHIM_H_INCLUDED
#define CNC_OOB_MPI_H

#include "mpiimpl.h"
#include <cnc/collective.h>

cnc_status_t cnc_oob_mpi_create(cnc_oob_t * oob, MPI_Comm comm);
void cnc_oob_mpi_destroy(cnc_oob_t * oob);

#endif /* CNC_OOB_MPI_SHIM_H_INCLUDED */

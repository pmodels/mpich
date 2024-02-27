/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpi.h"

void c_f08_status_(MPI_F08_status * f08_status)
{
    MPI_Status c_status;
    MPI_Fint f_status[MPI_F_STATUS_SIZE];

    MPI_Status_f082c(f08_status, &c_status);
    c_status.MPI_SOURCE += 1;
    c_status.MPI_TAG += 10;
    c_status.MPI_ERROR += 100;
    MPI_Status_c2f(&c_status, f_status);
    f_status[MPI_F_SOURCE] += 1;
    f_status[MPI_F_TAG] += 10;
    f_status[MPI_F_ERROR] += 100;
    MPI_Status_f2c(f_status, &c_status);
    c_status.MPI_SOURCE += 1;
    c_status.MPI_TAG += 10;
    c_status.MPI_ERROR += 100;
    MPI_Status_c2f08(&c_status, f08_status);
    f08_status->MPI_SOURCE += 1;
    f08_status->MPI_TAG += 10;
    f08_status->MPI_ERROR += 100;
    MPI_Status_f082f(f08_status, f_status);
    f_status[MPI_F_SOURCE] += 1;
    f_status[MPI_F_TAG] += 10;
    f_status[MPI_F_ERROR] += 100;
    MPI_Status_f2f08(f_status, f08_status);
}

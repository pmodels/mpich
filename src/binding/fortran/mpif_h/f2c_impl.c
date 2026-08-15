/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpi_fortimpl.h"

int MPIR_Status_f2c_impl(const MPI_Fint * f_status, MPI_Status * c_status)
{
    if (f_status == MPI_F_STATUS_IGNORE || f_status == MPI_F_STATUSES_IGNORE) {
        return MPI_ERR_ARG;
    }
#ifdef HAVE_FINT_IS_INT
    *c_status = *(MPI_Status *) f_status;
#else
    /* cast c_status as int array */
    int *p = (int *) c_status;
    for (int i = 0; i < MPI_F_STATUS_SIZE; i++) {
        p[i] = (int) f_status[i];
    }
#endif
    return MPI_SUCCESS;
}

int MPIR_Status_c2f_impl(const MPI_Status * c_status, MPI_Fint * f_status)
{
    if (c_status == MPI_STATUS_IGNORE || c_status == MPI_STATUSES_IGNORE) {
        return MPI_ERR_ARG;
    }
#ifdef HAVE_FINT_IS_INT
    *(MPI_Status *) f_status = *c_status;
#else
    /* cast c_status as int array */
    int *p = (int *) c_status;
    for (int i = 0; i < MPI_F_STATUS_SIZE; i++) {
        f_status[i] = (MPI_Fint) p[i];
    }
#endif
    return MPI_SUCCESS;
}

int MPIR_Status_f2f08_impl(const MPI_Fint * f_status, MPI_F08_status * f08_status)
{
    if (f_status == MPI_F_STATUS_IGNORE || f_status == MPI_F_STATUSES_IGNORE) {
        return MPI_ERR_ARG;
    }
    /* f_status and f08_status are always byte-equivalent */
    *f08_status = *(MPI_F08_status *) f_status;
    return MPI_SUCCESS;
}

int MPIR_Status_f082f_impl(const MPI_F08_status * f08_status, MPI_Fint * f_status)
{
    if (f08_status == MPI_F08_STATUS_IGNORE || f08_status == MPI_F08_STATUSES_IGNORE) {
        return MPI_ERR_ARG;
    }
    /* f_status and f08_status are always byte-equivalent */
    *(MPI_F08_status *) f_status = *f08_status;
    return MPI_SUCCESS;
}

int MPIR_Status_f082c_impl(const MPI_F08_status * f08_status, MPI_Status * c_status)
{
    if (f08_status == MPI_F08_STATUS_IGNORE || f08_status == MPI_F08_STATUSES_IGNORE) {
        return MPI_ERR_ARG;
    }
#ifdef HAVE_FINT_IS_INT
    *c_status = *(MPI_Status *) f08_status;
#else
    /* cast c_status as int array, f08_status as MPI_Fint array */
    int *p = (int *) c_status;
    int *q = (MPI_Fint *) f08_status;
    for (int i = 0; i < MPI_F_STATUS_SIZE; i++) {
        p[i] = (int) q[i];
    }
#endif
    return MPI_SUCCESS;
}

int MPIR_Status_c2f08_impl(const MPI_Status * c_status, MPI_F08_status * f08_status)
{
    if (c_status == MPI_STATUS_IGNORE || c_status == MPI_STATUSES_IGNORE) {
        return MPI_ERR_ARG;
    }
#ifdef HAVE_FINT_IS_INT
    *(MPI_Status *) f08_status = *c_status;
#else
    /* cast c_status as int array, f08_status as MPI_Fint array */
    int *p = (int *) c_status;
    int *q = (MPI_Fint *) f08_status;
    for (int i = 0; i < MPI_F_STATUS_SIZE; i++) {
        q[i] = (MPI_Fint) p[i];
    }
#endif
    return MPI_SUCCESS;
}

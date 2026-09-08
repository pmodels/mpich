/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpi_fortimpl.h"

MPI_Fint *MPI_F_STATUS_IGNORE MPICH_API_PUBLIC = 0;
MPI_Fint *MPI_F_STATUSES_IGNORE MPICH_API_PUBLIC = 0;

MPI_F08_status MPIR_F08_MPI_STATUS_IGNORE_OBJ MPICH_API_PUBLIC;
MPI_F08_status MPIR_F08_MPI_STATUSES_IGNORE_OBJ[1] MPICH_API_PUBLIC;
MPI_F08_status *MPI_F08_STATUS_IGNORE MPICH_API_PUBLIC = &MPIR_F08_MPI_STATUS_IGNORE_OBJ;
MPI_F08_status *MPI_F08_STATUSES_IGNORE MPICH_API_PUBLIC = &MPIR_F08_MPI_STATUSES_IGNORE_OBJ[0];

int MPIR_Status_f2c_impl(const MPI_Fint * f_status, MPI_Status * c_status)
{
    if (f_status == MPI_F_STATUS_IGNORE || f_status == MPI_F_STATUSES_IGNORE) {
        return MPI_ERR_ARG;
    }
#ifdef HAVE_FINT_IS_INT
    *c_status = *(MPI_Status *) f_status;
#else
    int *c = (void *) c_status;
    for (int i = 0; i < MPI_F_STATUS_SIZE; i++) {
        c[i] = (int) f_status[i];
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
    MPI_Fint *c = (void *) c_status;
    for (int i = 0; i < MPI_F_STATUS_SIZE; i++) {
        f_status[i] = (MPI_Fint) c_status[i];
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
    int *c = (void *) c_status;
    const MPI_Fint *f = (void *) f08_status;
    for (int i = 0; i < MPI_F_STATUS_SIZE; i++) {
        c[i] = (int) f[i];
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
    const int *c = (void *) c_status;
    MPI_Fint *f = (void *) f08_status;
    for (int i = 0; i < MPI_F_STATUS_SIZE; i++) {
        f[i] = (MPI_Fint) c[i];
    }
#endif
    return MPI_SUCCESS;
}

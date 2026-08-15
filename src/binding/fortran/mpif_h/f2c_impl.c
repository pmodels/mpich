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
    /* copy the whole dummy array */
    for (int i = 0; i < MPI_F_STATUS_SIZE; i++) {
        f08_status->dummy[i] = f_status[i];
    }
    /* synchronize the f08 fields */
    f08_status->MPI_SOURCE = f08_status->dummy[MPI_F_SOURCE];
    f08_status->MPI_TAG = f08_status->dummy[MPI_F_TAG];
    f08_status->MPI_ERROR = f08_status->dummy[MPI_F_ERROR];
    return MPI_SUCCESS;
}

int MPIR_Status_f082f_impl(const MPI_F08_status * f08_status, MPI_Fint * f_status)
{
    if (f08_status == MPI_F08_STATUS_IGNORE || f08_status == MPI_F08_STATUSES_IGNORE) {
        return MPI_ERR_ARG;
    }
    /* synchronize f08 fields */
    MPI_Fint *dummy = ((MPI_F08_status *) f08_status)->dummy;
    dummy[MPI_F_SOURCE] = f08_status->MPI_SOURCE;
    dummy[MPI_F_TAG] = f08_status->MPI_TAG;
    dummy[MPI_F_ERROR] = f08_status->MPI_ERROR;
    /* copy the whole dummy array */
    for (int i = 0; i < MPI_F_STATUS_SIZE; i++) {
        f_status[i] = f08_status->dummy[i];
    }
    return MPI_SUCCESS;
}

int MPIR_Status_f082c_impl(const MPI_F08_status * f08_status, MPI_Status * c_status)
{
    if (f08_status == MPI_F08_STATUS_IGNORE || f08_status == MPI_F08_STATUSES_IGNORE) {
        return MPI_ERR_ARG;
    }
    /* synchronize f08 fields */
    MPI_Fint *dummy = ((MPI_F08_status *) f08_status)->dummy;
    dummy[MPI_F_SOURCE] = f08_status->MPI_SOURCE;
    dummy[MPI_F_TAG] = f08_status->MPI_TAG;
    dummy[MPI_F_ERROR] = f08_status->MPI_ERROR;
    /* copy the whole dummy array */
    int *p = (int *) c_status;
    for (int i = 0; i < MPI_F_STATUS_SIZE; i++) {
        p[i] = f08_status->dummy[i];
    }
    return MPI_SUCCESS;
}

int MPIR_Status_c2f08_impl(const MPI_Status * c_status, MPI_F08_status * f08_status)
{
    if (c_status == MPI_STATUS_IGNORE || c_status == MPI_STATUSES_IGNORE) {
        return MPI_ERR_ARG;
    }
    /* copy the whole dummy array */
    int *p = (int *) c_status;
    for (int i = 0; i < MPI_F_STATUS_SIZE; i++) {
        f08_status->dummy[i] = (MPI_Fint) p[i];
    }
    /* synchronize the f08 fields */
    f08_status->MPI_SOURCE = f08_status->dummy[MPI_F_SOURCE];
    f08_status->MPI_TAG = f08_status->dummy[MPI_F_TAG];
    f08_status->MPI_ERROR = f08_status->dummy[MPI_F_ERROR];
    return MPI_SUCCESS;
}

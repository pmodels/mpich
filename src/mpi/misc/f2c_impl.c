/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

int MPIR_Status_f2c_impl(const MPI_Fint * f_status, MPI_Status * c_status)
{
    if (f_status == MPI_F_STATUS_IGNORE || f_status == MPI_F_STATUSES_IGNORE) {
        return MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                    __func__, __LINE__, MPI_ERR_OTHER, "**notfstatignore", 0);
    }
#ifdef HAVE_FINT_IS_INT
    *c_status = *(MPI_Status *) f_status;
#else
    c_status->count_lo = (int) f_status[0];
    c_status->count_hi_and_cancelled = (int) f_status[1];
    c_status->MPI_SOURCE = (int) f_status[2];
    c_status->MPI_TAG = (int) f_status[3];
    c_status->MPI_ERROR = (int) f_status[4];
#endif
    return MPI_SUCCESS;
}

int MPIR_Status_c2f_impl(const MPI_Status * c_status, MPI_Fint * f_status)
{
    if (c_status == MPI_STATUS_IGNORE || c_status == MPI_STATUSES_IGNORE) {
        return MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                    __func__, __LINE__, MPI_ERR_OTHER, "**notcstatignore", 0);
    }
#ifdef HAVE_FINT_IS_INT
    *(MPI_Status *) f_status = *c_status;
#else
    f_status[0] = (MPI_Fint) c_status->count_lo;
    f_status[1] = (MPI_Fint) c_status->count_hi_and_cancelled;
    f_status[2] = (MPI_Fint) c_status->MPI_SOURCE;
    f_status[3] = (MPI_Fint) c_status->MPI_TAG;
    f_status[4] = (MPI_Fint) c_status->MPI_ERROR;
#endif
    return MPI_SUCCESS;
}

int MPIR_Status_f2f08_impl(const MPI_Fint * f_status, MPI_F08_status * f08_status)
{
    if (f_status == MPI_F_STATUS_IGNORE || f_status == MPI_F_STATUSES_IGNORE) {
        return MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                    __func__, __LINE__, MPI_ERR_OTHER, "**notfstatignore", 0);
    }
    /* f_status and f08_status are always byte-equivalent */
    *f08_status = *(MPI_F08_status *) f_status;
    return MPI_SUCCESS;
}

int MPIR_Status_f082f_impl(const MPI_F08_status * f08_status, MPI_Fint * f_status)
{
    if (f08_status == MPI_F08_STATUS_IGNORE || f08_status == MPI_F08_STATUSES_IGNORE) {
        return MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                    __func__, __LINE__, MPI_ERR_OTHER, "**notcstatignore", 0);
    }
    /* f_status and f08_status are always byte-equivalent */
    *(MPI_F08_status *) f_status = *f08_status;
    return MPI_SUCCESS;
}

int MPIR_Status_f082c_impl(const MPI_F08_status * f08_status, MPI_Status * c_status)
{
    if (f08_status == MPI_F08_STATUS_IGNORE || f08_status == MPI_F08_STATUSES_IGNORE) {
        return MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                    __func__, __LINE__, MPI_ERR_OTHER, "**notfstatignore", 0);
    }
#ifdef HAVE_FINT_IS_INT
    *c_status = *(MPI_Status *) f08_status;
#else
    c_status->count_lo = (int) f08_status->count_lo;
    c_status->count_hi_and_cancelled = (int) f08_status->count_hi_and_cancelled;
    c_status->MPI_SOURCE = (int) f08_status->MPI_SOURCE;
    c_status->MPI_TAG = (int) f08_status->MPI_TAG;
    c_status->MPI_ERROR = (int) f08_status->MPI_ERROR;
#endif
    return MPI_SUCCESS;
}

int MPIR_Status_c2f08_impl(const MPI_Status * c_status, MPI_F08_status * f08_status)
{
    if (c_status == MPI_STATUS_IGNORE || c_status == MPI_STATUSES_IGNORE) {
        return MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                    __func__, __LINE__, MPI_ERR_OTHER, "**notcstatignore", 0);
    }
#ifdef HAVE_FINT_IS_INT
    *(MPI_Status *) f08_status = *c_status;
#else
    f08_status->count_lo = (MPI_Fint) c_status->count_lo;
    f08_status->count_hi_and_cancelled = (MPI_Fint) c_status->count_hi_and_cancelled;
    f08_status->MPI_SOURCE = (MPI_Fint) c_status->MPI_SOURCE;
    f08_status->MPI_TAG = (MPI_Fint) c_status->MPI_TAG;
    f08_status->MPI_ERROR = (MPI_Fint) c_status->MPI_ERROR;
#endif
    return MPI_SUCCESS;
}

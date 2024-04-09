/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpioimpl.h"
#include "adio_extern.h"
#include "mpir_ext.h"

#include <stdarg.h>
#include <stdio.h>


int MPIO_Err_create_code(int lastcode, int fatal, const char fcname[],
                         int line, int error_class, const char generic_msg[],
                         const char specific_msg[], ...)
{
    va_list Argp;
    int error_code;

    va_start(Argp, specific_msg);

    error_code = MPIR_Err_create_code_valist(lastcode, fatal, fcname, line,
                                             error_class, generic_msg, specific_msg, Argp);

    va_end(Argp);

    return error_code;
}

int MPIO_Err_return_file(MPI_File mpi_fh, int error_code)
{
    MPI_Errhandler e;
    char error_msg[4096];
    int len;

    /* If the file pointer is not valid, we use the handler on
     * MPI_FILE_NULL (MPI-2, section 9.7).  For now, this code assumes that
     * MPI_FILE_NULL has the default handler (return).  FIXME.  See
     * below - the set error handler uses ADIOI_DFLT_ERR_HANDLER;
     */

    /* First, get the handler and the corresponding function */
    if (mpi_fh == MPI_FILE_NULL) {
        e = ADIOI_DFLT_ERR_HANDLER;
    } else {
        ADIO_File fh;

        fh = MPIO_File_resolve(mpi_fh);
        e = fh->err_handler;
    }

    if (MPIR_Err_is_fatal(error_code) || e == MPI_ERRORS_ARE_FATAL || e == MPI_ERRORS_ABORT) {
        ADIO_File fh = MPIO_File_resolve(mpi_fh);

        snprintf(error_msg, 4096, "I/O error: ");
        len = (int) strlen(error_msg);
        MPIR_Err_get_string(error_code, &error_msg[len], 4096 - len, NULL);
        MPIR_Abort(fh->comm, MPI_SUCCESS, error_code, error_msg);

        goto fn_exit;
    } else if (e == MPI_ERRORS_RETURN || e == MPIR_ERRORS_THROW_EXCEPTIONS || !e) {
        /* FIXME: This is a hack in case no error handler was set */
        goto fn_exit;
    } else {
        error_code = MPIR_Call_file_errhandler(e, error_code, mpi_fh);
    }

  fn_exit:
    return error_code;
}

int MPIO_Err_return_comm(MPI_Comm mpi_comm, int error_code)
{
    /* note: MPI calls inside the MPICH implementation are prefixed
     * with an "N", indicating a nested call.
     */
    MPI_Comm_call_errhandler(mpi_comm, error_code);
    return error_code;
}

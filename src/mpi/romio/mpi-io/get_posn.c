/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpioimpl.h"
#include "adioi.h"

#ifdef HAVE_WEAK_SYMBOLS

#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_File_get_position = PMPI_File_get_position
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_File_get_position MPI_File_get_position
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_File_get_position as PMPI_File_get_position
/* end of weak pragmas */
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_File_get_position(MPI_File fh, MPI_Offset * offset)
    __attribute__ ((weak, alias("PMPI_File_get_position")));
#endif

/* Include mapping from MPI->PMPI */
#define MPIO_BUILD_PROFILING
#include "mpioprof.h"
#endif

/*@
    MPI_File_get_position - Returns the current position of the
                individual file pointer in etype units relative to
                the current view

Input Parameters:
. fh - file handle (handle)

Output Parameters:
. offset - offset of individual file pointer (nonnegative integer)

.N fortran
@*/
int MPI_File_get_position(MPI_File fh, MPI_Offset * offset)
{
    int error_code;
    ROMIO_THREAD_CS_ENTER();

    error_code = MPIR_File_get_position_impl(fh, offset);
    if (error_code) {
        goto fn_fail;
    }

  fn_exit:
    ROMIO_THREAD_CS_EXIT();
    return error_code;
  fn_fail:
    error_code = MPIO_Err_return_file(fh, error_code);
    goto fn_exit;
}

/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpioimpl.h"
#include <limits.h>
#include <assert.h>

#ifdef HAVE_WEAK_SYMBOLS

#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_File_write = PMPI_File_write
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_File_write MPI_File_write
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_File_write as PMPI_File_write
/* end of weak pragmas */
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_File_write(MPI_File fh, const void *buf, int count, MPI_Datatype datatype,
                   MPI_Status * status) __attribute__ ((weak, alias("PMPI_File_write")));
#endif

#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_File_write_c = PMPI_File_write_c
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_File_write_c MPI_File_write_c
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_File_write_c as PMPI_File_write_c
/* end of weak pragmas */
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_File_write_c(MPI_File fh, const void *buf, MPI_Count count, MPI_Datatype datatype,
                     MPI_Status * status) __attribute__ ((weak, alias("PMPI_File_write_c")));
#endif

/* Include mapping from MPI->PMPI */
#define MPIO_BUILD_PROFILING
#include "mpioprof.h"
#endif

/* status object not filled currently */

/*@
    MPI_File_write - Write using individual file pointer

Input Parameters:
. fh - file handle (handle)
. buf - initial address of buffer (choice)
. count - number of elements in buffer (nonnegative integer)
. datatype - datatype of each buffer element (handle)

Output Parameters:
. status - status object (Status)

.N fortran
@*/
int MPI_File_write(MPI_File fh, ROMIO_CONST void *buf, int count,
                   MPI_Datatype datatype, MPI_Status * status)
{
    int error_code;
    ROMIO_THREAD_CS_ENTER();

    error_code = MPIR_File_write_impl(fh, buf, count, datatype, status);
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

/* large count function */



/*@
    MPI_File_write_c - Write using individual file pointer

Input Parameters:
. fh - file handle (handle)
. buf - initial address of buffer (choice)
. count - number of elements in buffer (nonnegative integer)
. datatype - datatype of each buffer element (handle)

Output Parameters:
. status - status object (Status)

.N fortran
@*/
int MPI_File_write_c(MPI_File fh, ROMIO_CONST void *buf, MPI_Count count,
                     MPI_Datatype datatype, MPI_Status * status)
{
    int error_code;
    ROMIO_THREAD_CS_ENTER();

    error_code = MPIR_File_write_impl(fh, buf, count, datatype, status);
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

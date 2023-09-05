/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpioimpl.h"
#include <limits.h>
#include <assert.h>

#ifdef HAVE_WEAK_SYMBOLS

#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_File_write_at = PMPI_File_write_at
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_File_write_at MPI_File_write_at
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_File_write_at as PMPI_File_write_at
/* end of weak pragmas */
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_File_write_at(MPI_File fh, MPI_Offset offset, const void *buf, int count,
                      MPI_Datatype datatype, MPI_Status * status)
    __attribute__ ((weak, alias("PMPI_File_write_at")));
#endif

#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_File_write_at_c = PMPI_File_write_at_c
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_File_write_at_c MPI_File_write_at_c
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_File_write_at_c as PMPI_File_write_at_c
/* end of weak pragmas */
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_File_write_at_c(MPI_File fh, MPI_Offset offset, const void *buf, MPI_Count count,
                        MPI_Datatype datatype, MPI_Status * status)
    __attribute__ ((weak, alias("PMPI_File_write_at_c")));
#endif

/* Include mapping from MPI->PMPI */
#define MPIO_BUILD_PROFILING
#include "mpioprof.h"
#endif

/* status object not filled currently */

/*@
    MPI_File_write_at - Write using explicit offset

Input Parameters:
. fh - file handle (handle)
. offset - file offset (nonnegative integer)
. buf - initial address of buffer (choice)
. count - number of elements in buffer (nonnegative integer)
. datatype - datatype of each buffer element (handle)

Output Parameters:
. status - status object (Status)

.N fortran
@*/
int MPI_File_write_at(MPI_File fh, MPI_Offset offset, ROMIO_CONST void *buf,
                      int count, MPI_Datatype datatype, MPI_Status * status)
{
    int error_code;
    ROMIO_THREAD_CS_ENTER();

    error_code = MPIR_File_write_at_impl(fh, offset, buf, count, datatype, status);
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
    MPI_File_write_at_c - Write using explicit offset

Input Parameters:
. fh - file handle (handle)
. offset - file offset (nonnegative integer)
. buf - initial address of buffer (choice)
. count - number of elements in buffer (nonnegative integer)
. datatype - datatype of each buffer element (handle)

Output Parameters:
. status - status object (Status)

.N fortran
@*/
int MPI_File_write_at_c(MPI_File fh, MPI_Offset offset, ROMIO_CONST void *buf,
                        MPI_Count count, MPI_Datatype datatype, MPI_Status * status)
{
    int error_code;
    ROMIO_THREAD_CS_ENTER();

    error_code = MPIR_File_write_at_impl(fh, offset, buf, count, datatype, status);
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

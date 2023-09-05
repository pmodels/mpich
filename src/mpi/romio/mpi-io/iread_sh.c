/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpioimpl.h"
#include <limits.h>
#include <assert.h>

#ifdef HAVE_WEAK_SYMBOLS

#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_File_iread_shared = PMPI_File_iread_shared
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_File_iread_shared MPI_File_iread_shared
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_File_iread_shared as PMPI_File_iread_shared
/* end of weak pragmas */
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_File_iread_shared(MPI_File fh, void *buf, int count, MPI_Datatype datatype,
                          MPIO_Request * request)
    __attribute__ ((weak, alias("PMPI_File_iread_shared")));
#endif

#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_File_iread_shared_c = PMPI_File_iread_shared_c
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_File_iread_shared_c MPI_File_iread_shared_c
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_File_iread_shared_c as PMPI_File_iread_shared_c
/* end of weak pragmas */
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_File_iread_shared_c(MPI_File fh, void *buf, MPI_Count count, MPI_Datatype datatype,
                            MPIO_Request * request)
    __attribute__ ((weak, alias("PMPI_File_iread_shared_c")));
#endif

/* Include mapping from MPI->PMPI */
#define MPIO_BUILD_PROFILING
#include "mpioprof.h"
#endif

#ifdef HAVE_MPI_GREQUEST
#include "mpiu_greq.h"
#endif

/*@
    MPI_File_iread_shared - Nonblocking read using shared file pointer

Input Parameters:
. fh - file handle (handle)
. count - number of elements in buffer (nonnegative integer)
. datatype - datatype of each buffer element (handle)

Output Parameters:
. buf - initial address of buffer (choice)
. request - request object (handle)

.N fortran
@*/
int MPI_File_iread_shared(MPI_File fh, void *buf, int count,
                          MPI_Datatype datatype, MPI_Request * request)
{
    int error_code;
    ROMIO_THREAD_CS_ENTER();

    error_code = MPIR_File_iread_shared_impl(fh, buf, count, datatype, request);
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
    MPI_File_iread_shared_c - Nonblocking read using shared file pointer

Input Parameters:
. fh - file handle (handle)
. count - number of elements in buffer (nonnegative integer)
. datatype - datatype of each buffer element (handle)

Output Parameters:
. buf - initial address of buffer (choice)
. request - request object (handle)

.N fortran
@*/
int MPI_File_iread_shared_c(MPI_File fh, void *buf, MPI_Count count,
                            MPI_Datatype datatype, MPI_Request * request)
{
    int error_code;
    ROMIO_THREAD_CS_ENTER();

    error_code = MPIR_File_iread_shared_impl(fh, buf, count, datatype, request);
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

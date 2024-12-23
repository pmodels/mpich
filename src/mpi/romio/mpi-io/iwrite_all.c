/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpioimpl.h"
#include <limits.h>
#include <assert.h>

#ifdef HAVE_WEAK_SYMBOLS

#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_File_iwrite_all = PMPI_File_iwrite_all
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_File_iwrite_all MPI_File_iwrite_all
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_File_iwrite_all as PMPI_File_iwrite_all
/* end of weak pragmas */
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_File_iwrite_all(MPI_File fh, const void *buf, int count, MPI_Datatype datatype,
                        MPI_Request * request)
    __attribute__ ((weak, alias("PMPI_File_iwrite_all")));
#endif

#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_File_iwrite_all_c = PMPI_File_iwrite_all_c
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_File_iwrite_all_c MPI_File_iwrite_all_c
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_File_iwrite_all_c as PMPI_File_iwrite_all_c
/* end of weak pragmas */
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_File_iwrite_all_c(MPI_File fh, const void *buf, MPI_Count count, MPI_Datatype datatype,
                          MPI_Request * request)
    __attribute__ ((weak, alias("PMPI_File_iwrite_all_c")));
#endif

/* Include mapping from MPI->PMPI */
#define MPIO_BUILD_PROFILING
#include "mpioprof.h"
#endif

#ifdef HAVE_MPI_GREQUEST
#include "mpiu_greq.h"
#endif

/*@
    MPI_File_iwrite_all - Nonblocking collective write using individual file pointer

Input Parameters:
. fh - file handle (handle)
. buf - initial address of buffer (choice)
. count - number of elements in buffer (nonnegative integer)
. datatype - datatype of each buffer element (handle)

Output Parameters:
. request - request object (handle)

.N fortran
@*/
int MPI_File_iwrite_all(MPI_File fh, ROMIO_CONST void *buf, int count,
                        MPI_Datatype datatype, MPI_Request * request)
{
    int error_code;
    ROMIO_THREAD_CS_ENTER();

    error_code = MPIR_File_iwrite_all_impl(fh, buf, count, datatype, request);
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


#ifdef HAVE_MPI_GREQUEST
#include "mpiu_greq.h"
#endif

/*@
    MPI_File_iwrite_all_c - Nonblocking collective write using individual file pointer

Input Parameters:
. fh - file handle (handle)
. buf - initial address of buffer (choice)
. count - number of elements in buffer (nonnegative integer)
. datatype - datatype of each buffer element (handle)

Output Parameters:
. request - request object (handle)

.N fortran
@*/
int MPI_File_iwrite_all_c(MPI_File fh, ROMIO_CONST void *buf, MPI_Count count,
                          MPI_Datatype datatype, MPI_Request * request)
{
    int error_code;
    ROMIO_THREAD_CS_ENTER();

    error_code = MPIR_File_iwrite_all_impl(fh, buf, count, datatype, request);
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

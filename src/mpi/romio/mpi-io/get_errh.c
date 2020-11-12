/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpioimpl.h"
#include "adio_extern.h"

/* -- Begin Profiling Symbol Block */
#ifdef HAVE_WEAK_SYMBOLS

#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_File_get_errhandler = PMPI_File_get_errhandler
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_File_get_errhandler MPI_File_get_errhandler
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_File_get_errhandler as PMPI_File_get_errhandler
/* end of weak pragmas */
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_File_get_errhandler(MPI_File file, MPI_Errhandler * errhandler)
    __attribute__ ((weak, alias("PMPI_File_get_errhandler")));
#endif
/* -- End Profiling Symbol Block */

/* Include mapping from MPI->PMPI */
#define MPIO_BUILD_PROFILING
#include "mpioprof.h"
#endif

int MPI_File_get_errhandler(MPI_File mpi_fh, MPI_Errhandler * errhandler)
{
    QMPI_Context context;
    QMPI_File_get_errhandler_t *fn_ptr;

    context.storage_stack = NULL;

    if (MPIR_QMPI_num_tools == 0)
        return QMPI_File_get_errhandler(context, 0, mpi_fh, errhandler);

    fn_ptr = (QMPI_File_get_errhandler_t *) MPIR_QMPI_first_fn_ptrs[MPI_FILE_GET_ERRHANDLER_T];

    return (*fn_ptr) (context, MPIR_QMPI_first_tool_ids[MPI_FILE_GET_ERRHANDLER_T], mpi_fh,
                      errhandler);
}

/*@
    MPI_File_get_errhandler - Returns the error handler for a file

Input Parameters:
. fh - file handle (handle)

Output Parameters:
. errhandler - error handler (handle)

.N fortran
@*/
int QMPI_File_get_errhandler(QMPI_Context context, int tool_id, MPI_File mpi_fh,
                             MPI_Errhandler * errhandler)
{
    int error_code = MPI_SUCCESS;
    ADIO_File fh;
    static char myname[] = "MPI_FILE_GET_ERRHANDLER";

    ROMIO_THREAD_CS_ENTER();

    if (mpi_fh == MPI_FILE_NULL) {
        *errhandler = ADIOI_DFLT_ERR_HANDLER;
    } else {
        fh = MPIO_File_resolve(mpi_fh);
        /* --BEGIN ERROR HANDLING-- */
        if ((fh <= (MPI_File) 0) || ((fh)->cookie != ADIOI_FILE_COOKIE)) {
            error_code = MPIO_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                              myname, __LINE__, MPI_ERR_ARG, "**iobadfh", 0);
            error_code = MPIO_Err_return_file(MPI_FILE_NULL, error_code);
            goto fn_exit;
        }
        /* --END ERROR HANDLING-- */

        *errhandler = fh->err_handler;
    }

  fn_exit:
    ROMIO_THREAD_CS_EXIT();
    return MPI_SUCCESS;
}

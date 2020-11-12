/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpioimpl.h"

/* -- Begin Profiling Symbol Block */
#ifdef HAVE_WEAK_SYMBOLS

#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_File_sync = PMPI_File_sync
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_File_sync MPI_File_sync
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_File_sync as PMPI_File_sync
/* end of weak pragmas */
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_File_sync(MPI_File fh) __attribute__ ((weak, alias("PMPI_File_sync")));
#endif
/* -- End Profiling Symbol Block */

/* Include mapping from MPI->PMPI */
#define MPIO_BUILD_PROFILING
#include "mpioprof.h"
#endif

int MPI_File_sync(MPI_File fh)
{
    QMPI_Context context;
    QMPI_File_sync_t *fn_ptr;

    context.storage_stack = NULL;

    if (MPIR_QMPI_num_tools == 0)
        return QMPI_File_sync(context, 0, fh);

    fn_ptr = (QMPI_File_sync_t *) MPIR_QMPI_first_fn_ptrs[MPI_FILE_SYNC_T];

    return (*fn_ptr) (context, MPIR_QMPI_first_tool_ids[MPI_FILE_SYNC_T], fh);
}

/*@
    MPI_File_sync - Causes all previous writes to be transferred
                    to the storage device

Input Parameters:
. fh - file handle (handle)

.N fortran
@*/
int QMPI_File_sync(QMPI_Context context, int tool_id, MPI_File fh)
{
    int error_code;
    ADIO_File adio_fh;
    static char myname[] = "MPI_FILE_SYNC";
#ifdef MPI_hpux
    int fl_xmpi;

    HPMP_IO_START(fl_xmpi, BLKMPIFILESYNC, TRDTBLOCK, adio_fh, MPI_DATATYPE_NULL, -1);
#endif /* MPI_hpux */
    ROMIO_THREAD_CS_ENTER();

    adio_fh = MPIO_File_resolve(fh);
    /* --BEGIN ERROR HANDLING-- */
    if ((adio_fh == NULL) || ((adio_fh)->cookie != ADIOI_FILE_COOKIE)) {
        error_code = MPIO_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                          myname, __LINE__, MPI_ERR_ARG, "**iobadfh", 0);
        error_code = MPIO_Err_return_file(MPI_FILE_NULL, error_code);
        goto fn_exit;
    }
    MPIO_CHECK_WRITABLE(fh, myname, error_code);
    /* --END ERROR HANDLING-- */

    ADIO_Flush(adio_fh, &error_code);
    /* --BEGIN ERROR HANDLING-- */
    if (error_code != MPI_SUCCESS)
        error_code = MPIO_Err_return_file(adio_fh, error_code);
    /* --END ERROR HANDLING-- */

#ifdef MPI_hpux
    HPMP_IO_END(fl_xmpi, adio_fh, MPI_DATATYPE_NULL, -1);
#endif /* MPI_hpux */

  fn_exit:
    ROMIO_THREAD_CS_EXIT();
    return error_code;
}

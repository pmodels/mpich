/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpioimpl.h"

/* -- Begin Profiling Symbol Block */
#ifdef HAVE_WEAK_SYMBOLS

#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_File_iread_at = PMPI_File_iread_at
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_File_iread_at MPI_File_iread_at
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_File_iread_at as PMPI_File_iread_at
/* end of weak pragmas */
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_File_iread_at(MPI_File fh, MPI_Offset offset, void *buf, int count, MPI_Datatype datatype,
                      MPIO_Request * request) __attribute__ ((weak, alias("PMPI_File_iread_at")));
#endif
/* -- End Profiling Symbol Block */

/* Include mapping from MPI->PMPI */
#define MPIO_BUILD_PROFILING
#include "mpioprof.h"
#endif

#ifdef HAVE_MPI_GREQUEST
#include "mpiu_greq.h"
#endif

int MPI_File_iread_at(MPI_File fh, MPI_Offset offset, void *buf, int count, MPI_Datatype datatype,
                      MPIO_Request * request)
{
    QMPI_Context context;
    QMPI_File_iread_at_t *fn_ptr;

    context.storage_stack = NULL;

    if (MPIR_QMPI_num_tools == 0)
        return QMPI_File_iread_at(context, 0, fh, offset, buf, count, datatype, request);

    fn_ptr = (QMPI_File_iread_at_t *) MPIR_QMPI_first_fn_ptrs[MPI_FILE_IREAD_AT_T];

    return (*fn_ptr) (context, MPIR_QMPI_first_tool_ids[MPI_FILE_IREAD_AT_T], fh, offset, buf,
                      count, datatype, request);
}

/*@
    MPI_File_iread_at - Nonblocking read using explicit offset

Input Parameters:
. fh - file handle (handle)
. offset - file offset (nonnegative integer)
. count - number of elements in buffer (nonnegative integer)
. datatype - datatype of each buffer element (handle)

Output Parameters:
. buf - initial address of buffer (choice)
. request - request object (handle)

.N fortran
@*/
int QMPI_File_iread_at(QMPI_Context context, int tool_id, MPI_File fh, MPI_Offset offset, void *buf,
                       int count, MPI_Datatype datatype, MPIO_Request * request)
{
    int error_code;
    static char myname[] = "MPI_FILE_IREAD_AT";

#ifdef MPI_hpux
    int fl_xmpi;

    HPMP_IO_START(fl_xmpi, BLKMPIFILEIREADAT, TRDTSYSTEM, fh, datatype, count);
#endif /* MPI_hpux */


    error_code = MPIOI_File_iread(fh, offset, ADIO_EXPLICIT_OFFSET, buf,
                                  count, datatype, myname, request);

    /* --BEGIN ERROR HANDLING-- */
    if (error_code != MPI_SUCCESS)
        error_code = MPIO_Err_return_file(fh, error_code);
    /* --END ERROR HANDLING-- */

#ifdef MPI_hpux
    HPMP_IO_END(fl_xmpi, fh, datatype, count);
#endif /* MPI_hpux */

    return error_code;
}

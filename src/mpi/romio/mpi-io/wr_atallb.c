/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpioimpl.h"

/* -- Begin Profiling Symbol Block */
#ifdef HAVE_WEAK_SYMBOLS

#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_File_write_at_all_begin = PMPI_File_write_at_all_begin
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_File_write_at_all_begin MPI_File_write_at_all_begin
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_File_write_at_all_begin as PMPI_File_write_at_all_begin
/* end of weak pragmas */
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_File_write_at_all_begin(MPI_File fh, MPI_Offset offset, const void *buf, int count,
                                MPI_Datatype datatype)
    __attribute__ ((weak, alias("PMPI_File_write_at_all_begin")));
#endif
/* -- End Profiling Symbol Block */

/* Include mapping from MPI->PMPI */
#define MPIO_BUILD_PROFILING
#include "mpioprof.h"
#endif

int MPI_File_write_at_all_begin(MPI_File fh, MPI_Offset offset, ROMIO_CONST void *buf,
                                int count, MPI_Datatype datatype)
{
    QMPI_Context context;
    QMPI_File_write_at_all_begin_t *fn_ptr;

    context.storage_stack = NULL;

    if (MPIR_QMPI_num_tools == 0)
        return QMPI_File_write_at_all_begin(context, 0, fh, offset, buf, count, datatype);

    fn_ptr =
        (QMPI_File_write_at_all_begin_t *) MPIR_QMPI_first_fn_ptrs[MPI_FILE_WRITE_AT_ALL_BEGIN_T];

    return (*fn_ptr) (context, MPIR_QMPI_first_tool_ids[MPI_FILE_WRITE_AT_ALL_BEGIN_T], fh, offset,
                      buf, count, datatype);
}

/*@
    MPI_File_write_at_all_begin - Begin a split collective write using
    explicit offset

Input Parameters:
. fh - file handle (handle)
. offset - file offset (nonnegative integer)
. buf - initial address of buffer (choice)
. count - number of elements in buffer (nonnegative integer)
. datatype - datatype of each buffer element (handle)

.N fortran
@*/
int QMPI_File_write_at_all_begin(QMPI_Context context, int tool_id, MPI_File fh, MPI_Offset offset,
                                 ROMIO_CONST void *buf, int count, MPI_Datatype datatype)
{
    int error_code;
    static char myname[] = "MPI_FILE_WRITE_AT_ALL_BEGIN";

    error_code = MPIOI_File_write_all_begin(fh, offset,
                                            ADIO_EXPLICIT_OFFSET, buf, count, datatype, myname);

    return error_code;
}

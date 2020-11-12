/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpioimpl.h"

/* -- Begin Profiling Symbol Block */
#ifdef HAVE_WEAK_SYMBOLS

#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_File_read_at = PMPI_File_read_at
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_File_read_at MPI_File_read_at
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_File_read_at as PMPI_File_read_at
/* end of weak pragmas */
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_File_read_at(MPI_File fh, MPI_Offset offset, void *buf, int count, MPI_Datatype datatype,
                     MPI_Status * status) __attribute__ ((weak, alias("PMPI_File_read_at")));
#endif
/* -- End Profiling Symbol Block */

/* Include mapping from MPI->PMPI */
#define MPIO_BUILD_PROFILING
#include "mpioprof.h"
#endif

/* status object not filled currently */

int MPI_File_read_at(MPI_File fh, MPI_Offset offset, void *buf,
                     int count, MPI_Datatype datatype, MPI_Status * status)
{
    QMPI_Context context;
    QMPI_File_read_at_t *fn_ptr;

    context.storage_stack = NULL;

    if (MPIR_QMPI_num_tools == 0)
        return QMPI_File_read_at(context, 0, fh, offset, buf, count, datatype, status);

    fn_ptr = (QMPI_File_read_at_t *) MPIR_QMPI_first_fn_ptrs[MPI_FILE_READ_AT_T];

    return (*fn_ptr) (context, MPIR_QMPI_first_tool_ids[MPI_FILE_READ_AT_T], fh, offset, buf, count,
                      datatype, status);
}

/*@
    MPI_File_read_at - Read using explicit offset

Input Parameters:
. fh - file handle (handle)
. offset - file offset (nonnegative integer)
. count - number of elements in buffer (nonnegative integer)
. datatype - datatype of each buffer element (handle)

Output Parameters:
. buf - initial address of buffer (choice)
. status - status object (Status)

.N fortran
@*/
int QMPI_File_read_at(QMPI_Context context, int tool_id, MPI_File fh, MPI_Offset offset, void *buf,
                      int count, MPI_Datatype datatype, MPI_Status * status)
{
    int error_code;
    static char myname[] = "MPI_FILE_READ_AT";
#ifdef MPI_hpux
    int fl_xmpi;

    HPMP_IO_START(fl_xmpi, BLKMPIFILEREADAT, TRDTBLOCK, fh, datatype, count);
#endif /* MPI_hpux */

    /* ADIOI_File_read() defined in mpi-io/read.c */
    error_code = MPIOI_File_read(fh, offset, ADIO_EXPLICIT_OFFSET, buf,
                                 count, datatype, myname, status);

#ifdef MPI_hpux
    HPMP_IO_END(fl_xmpi, fh, datatype, count);
#endif /* MPI_hpux */

    return error_code;
}

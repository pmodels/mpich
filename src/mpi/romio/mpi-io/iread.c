/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpioimpl.h"
#include <limits.h>
#include <assert.h>

#ifdef HAVE_WEAK_SYMBOLS

#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_File_iread = PMPI_File_iread
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_File_iread MPI_File_iread
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_File_iread as PMPI_File_iread
/* end of weak pragmas */
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_File_iread(MPI_File fh, void *buf, int count, MPI_Datatype datatype, MPIO_Request * request)
    __attribute__ ((weak, alias("PMPI_File_iread")));
#endif

#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_File_iread_c = PMPI_File_iread_c
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_File_iread_c MPI_File_iread_c
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_File_iread_c as PMPI_File_iread_c
/* end of weak pragmas */
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_File_iread_c(MPI_File fh, void *buf, MPI_Count count, MPI_Datatype datatype,
                     MPIO_Request * request)
    __attribute__ ((weak, alias("PMPI_File_iread_c")));
#endif

/* Include mapping from MPI->PMPI */
#define MPIO_BUILD_PROFILING
#include "mpioprof.h"
#endif

#ifdef HAVE_MPI_GREQUEST
#include "mpiu_greq.h"
#endif

/*@
    MPI_File_iread - Nonblocking read using individual file pointer

Input Parameters:
. fh - file handle (handle)
. count - number of elements in buffer (nonnegative integer)
. datatype - datatype of each buffer element (handle)

Output Parameters:
. buf - initial address of buffer (choice)
. request - request object (handle)

.N fortran
@*/
int MPI_File_iread(MPI_File fh, void *buf, int count, MPI_Datatype datatype, MPI_Request * request)
{
    int error_code = MPI_SUCCESS;
    static char myname[] = "MPI_FILE_IREAD";
#ifdef MPI_hpux
    int fl_xmpi;

    HPMP_IO_START(fl_xmpi, BLKMPIFILEIREAD, TRDTSYSTEM, fh, datatype, count);
#endif /* MPI_hpux */


    error_code = MPIOI_File_iread(fh, (MPI_Offset) 0, ADIO_INDIVIDUAL,
                                  buf, count, datatype, myname, request);

    /* --BEGIN ERROR HANDLING-- */
    if (error_code != MPI_SUCCESS)
        error_code = MPIO_Err_return_file(fh, error_code);
    /* --END ERROR HANDLING-- */

#ifdef MPI_hpux
    HPMP_IO_END(fl_xmpi, fh, datatype, count);
#endif /* MPI_hpux */

    return error_code;
}

/* large count function */


#ifdef HAVE_MPI_GREQUEST
#include "mpiu_greq.h"
#endif

/*@
    MPI_File_iread_c - Nonblocking read using individual file pointer

Input Parameters:
. fh - file handle (handle)
. count - number of elements in buffer (nonnegative integer)
. datatype - datatype of each buffer element (handle)

Output Parameters:
. buf - initial address of buffer (choice)
. request - request object (handle)

.N fortran
@*/
int MPI_File_iread_c(MPI_File fh, void *buf, MPI_Count count, MPI_Datatype datatype,
                     MPI_Request * request)
{
    int error_code = MPI_SUCCESS;
    static char myname[] = "MPI_FILE_IREAD";
#ifdef MPI_hpux
    int fl_xmpi;

    HPMP_IO_START(fl_xmpi, BLKMPIFILEIREAD, TRDTSYSTEM, fh, datatype, count);
#endif /* MPI_hpux */


    error_code = MPIOI_File_iread(fh, (MPI_Offset) 0, ADIO_INDIVIDUAL,
                                  buf, count, datatype, myname, request);

    /* --BEGIN ERROR HANDLING-- */
    if (error_code != MPI_SUCCESS)
        error_code = MPIO_Err_return_file(fh, error_code);
    /* --END ERROR HANDLING-- */

#ifdef MPI_hpux
    HPMP_IO_END(fl_xmpi, fh, datatype, count);
#endif /* MPI_hpux */

    return error_code;
}

/* prevent multiple definitions of this routine */
#ifdef MPIO_BUILD_PROFILING
int MPIOI_File_iread(MPI_File fh, MPI_Offset offset, int file_ptr_type, void *buf, MPI_Aint count,
                     MPI_Datatype datatype, char *myname, MPI_Request * request)
{
    int error_code, buftype_is_contig, filetype_is_contig;
    MPI_Count datatype_size;
    ADIO_Status status;
    ADIO_File adio_fh;
    ADIO_Offset off, bufsize;
    MPI_Offset nbytes = 0;
    void *xbuf = NULL, *e32_buf = NULL, *host_buf = NULL;

    ROMIO_THREAD_CS_ENTER();

    adio_fh = MPIO_File_resolve(fh);

    /* --BEGIN ERROR HANDLING-- */
    MPIO_CHECK_FILE_HANDLE(adio_fh, myname, error_code);
    MPIO_CHECK_COUNT(adio_fh, count, myname, error_code);
    MPIO_CHECK_DATATYPE(adio_fh, datatype, myname, error_code);

    if (file_ptr_type == ADIO_EXPLICIT_OFFSET && offset < 0) {
        error_code = MPIO_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                          myname, __LINE__, MPI_ERR_ARG, "**iobadoffset", 0);
        error_code = MPIO_Err_return_file(adio_fh, error_code);
        goto fn_exit;
    }
    /* --END ERROR HANDLING-- */

    MPI_Type_size_x(datatype, &datatype_size);

    /* --BEGIN ERROR HANDLING-- */
    MPIO_CHECK_INTEGRAL_ETYPE(adio_fh, count, datatype_size, myname, error_code);
    MPIO_CHECK_READABLE(adio_fh, myname, error_code);
    MPIO_CHECK_NOT_SEQUENTIAL_MODE(adio_fh, myname, error_code);
    MPIO_CHECK_COUNT_SIZE(adio_fh, count, datatype_size, myname, error_code);
    /* --END ERROR HANDLING-- */

    ADIOI_Datatype_iscontig(datatype, &buftype_is_contig);
    ADIOI_Datatype_iscontig(adio_fh->filetype, &filetype_is_contig);

    ADIOI_TEST_DEFERRED(adio_fh, myname, &error_code);

    xbuf = buf;
    if (adio_fh->is_external32) {
        MPI_Aint e32_size = 0;
        error_code = MPIU_datatype_full_size(datatype, &e32_size);
        if (error_code != MPI_SUCCESS)
            goto fn_exit;

        e32_buf = ADIOI_Malloc(e32_size * count);
        xbuf = e32_buf;
    } else {
        MPIO_GPU_HOST_ALLOC(host_buf, buf, count, datatype);
        if (host_buf != NULL) {
            xbuf = host_buf;
        }
    }

    if (buftype_is_contig && filetype_is_contig) {
        /* convert count and offset to bytes */
        bufsize = datatype_size * count;

        if (file_ptr_type == ADIO_EXPLICIT_OFFSET) {
            off = adio_fh->disp + adio_fh->etype_size * offset;
        } else {
            off = adio_fh->fp_ind;
        }

        if (!(adio_fh->atomicity))
            ADIO_IreadContig(adio_fh, xbuf, count, datatype, file_ptr_type,
                             off, request, &error_code);
        else {
            /* to maintain strict atomicity semantics with other concurrent
             * operations, lock (exclusive) and call blocking routine */
            if (ADIO_Feature(adio_fh, ADIO_LOCKS)) {
                ADIOI_WRITE_LOCK(adio_fh, off, SEEK_SET, bufsize);
            }

            ADIO_ReadContig(adio_fh, xbuf, count, datatype, file_ptr_type,
                            off, &status, &error_code);

            if (ADIO_Feature(adio_fh, ADIO_LOCKS)) {
                ADIOI_UNLOCK(adio_fh, off, SEEK_SET, bufsize);
            }
            if (error_code == MPI_SUCCESS) {
                nbytes = count * datatype_size;
            }
            MPIO_Completed_request_create(&adio_fh, nbytes, &error_code, request);
        }
    } else
        ADIO_IreadStrided(adio_fh, xbuf, count, datatype, file_ptr_type,
                          offset, request, &error_code);

    if (e32_buf != NULL) {
        error_code = MPIU_read_external32_conversion_fn(buf, datatype, count, e32_buf);
        ADIOI_Free(e32_buf);
    }

    MPIO_GPU_SWAP_BACK(host_buf, buf, count, datatype);

  fn_exit:
    ROMIO_THREAD_CS_EXIT();

    return error_code;
}
#endif

/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpioimpl.h"

/* for user-definde reduce operator */
#include "adio_extern.h"

static int MPIOI_File_iread(MPI_File fh, MPI_Offset offset, int file_ptr_type,
                            void *buf, MPI_Aint count, MPI_Datatype datatype,
                            MPI_Request * request);
static int MPIOI_File_iread_all(MPI_File fh, MPI_Offset offset, int file_ptr_type,
                                void *buf, MPI_Aint count, MPI_Datatype datatype,
                                MPI_Request * request);
static int MPIOI_File_iread_shared(MPI_File fh, void *buf, MPI_Aint count,
                                   MPI_Datatype datatype, MPI_Request * request);
static int MPIOI_File_iwrite(MPI_File fh, MPI_Offset offset, int file_ptr_type,
                             const void *buf, MPI_Aint count, MPI_Datatype datatype,
                             MPI_Request * request);
static int MPIOI_File_iwrite_all(MPI_File fh, MPI_Offset offset, int file_ptr_type,
                                 const void *buf, MPI_Aint count, MPI_Datatype datatype,
                                 MPI_Request * request);
static int MPIOI_File_iwrite_shared(MPI_File fh, const void *buf, MPI_Aint count,
                                    MPI_Datatype datatype, MPIO_Request * request);
static int MPIOI_File_read(MPI_File fh, MPI_Offset offset, int file_ptr_type,
                           void *buf, MPI_Aint count, MPI_Datatype datatype, MPI_Status * status);
static int MPIOI_File_read_all(MPI_File fh, MPI_Offset offset, int file_ptr_type,
                               void *buf, MPI_Aint count, MPI_Datatype datatype,
                               MPI_Status * status);
static int MPIOI_File_read_all_begin(MPI_File fh, MPI_Offset offset, int file_ptr_type,
                                     void *buf, MPI_Aint count, MPI_Datatype datatype);
static int MPIOI_File_read_all_end(MPI_File fh, void *buf, MPI_Status * status);
static int MPIOI_File_read_ordered(MPI_File fh, void *buf, MPI_Aint count, MPI_Datatype datatype,
                                   MPI_Status * status);
static int MPIOI_File_read_ordered_begin(MPI_File fh,
                                         void *buf, MPI_Aint count, MPI_Datatype datatype);
static int MPIOI_File_read_ordered_end(MPI_File fh, void *buf, MPI_Status * status);
static int MPIOI_File_read_shared(MPI_File fh, void *buf, MPI_Aint count, MPI_Datatype datatype,
                                  MPI_Status * status);
static int MPIOI_File_write(MPI_File fh, MPI_Offset offset, int file_ptr_type,
                            const void *buf, MPI_Aint count, MPI_Datatype datatype,
                            MPI_Status * status);
static int MPIOI_File_write_all(MPI_File fh, MPI_Offset offset, int file_ptr_type,
                                const void *buf, MPI_Aint count, MPI_Datatype datatype,
                                MPI_Status * status);
static int MPIOI_File_write_all_begin(MPI_File fh, MPI_Offset offset, int file_ptr_type,
                                      const void *buf, MPI_Aint count, MPI_Datatype datatype);
static int MPIOI_File_write_all_end(MPI_File fh, const void *buf, MPI_Status * status);
static int MPIOI_File_write_ordered(MPI_File fh, const void *buf, MPI_Aint count,
                                    MPI_Datatype datatype, MPI_Status * status);
static int MPIOI_File_write_ordered_begin(MPI_File fh, const void *buf, MPI_Aint count,
                                          MPI_Datatype datatype);
static int MPIOI_File_write_ordered_end(MPI_File fh, const void *buf, MPI_Status * status);
static int MPIOI_File_write_shared(MPI_File fh, const void *buf, MPI_Aint count,
                                   MPI_Datatype datatype, MPI_Status * status);

typedef void (*MPIOI_VOID_FN) (void *);
static int MPIOI_Register_datarep(const char *datarep,
                                  MPIOI_VOID_FN * read_conversion_fn,
                                  MPIOI_VOID_FN * write_conversion_fn,
                                  MPI_Datarep_extent_function * dtype_file_extent_fn,
                                  void *extra_state, int is_large);

int MPIR_File_open_impl(MPI_Comm comm, ROMIO_CONST char *filename, int amode,
                        MPI_Info info, MPI_File * fh)
{
    int error_code = MPI_SUCCESS;
    MPI_Comm dupcomm = MPI_COMM_NULL;

    MPIO_CHECK_COMM(comm, __func__, error_code);
    MPIO_CHECK_INFO_ALL(info, __func__, error_code, comm);

    int flag;
    error_code = MPI_Comm_test_inter(comm, &flag);
    if (error_code || flag) {
        error_code = MPIO_Err_create_code(error_code, MPIR_ERR_RECOVERABLE,
                                          __func__, __LINE__, MPI_ERR_COMM, "**commnotintra", 0);
        goto fn_fail;
    }

    if (((amode & MPI_MODE_RDONLY) ? 1 : 0) + ((amode & MPI_MODE_RDWR) ? 1 : 0) +
        ((amode & MPI_MODE_WRONLY) ? 1 : 0) != 1) {
        error_code = MPIO_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                          __func__, __LINE__, MPI_ERR_AMODE, "**fileamodeone", 0);
        goto fn_fail;
    }

    if ((amode & MPI_MODE_RDONLY) && ((amode & MPI_MODE_CREATE) || (amode & MPI_MODE_EXCL))) {
        error_code = MPIO_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                          __func__, __LINE__, MPI_ERR_AMODE, "**fileamoderead", 0);
        goto fn_fail;
    }

    if ((amode & MPI_MODE_RDWR) && (amode & MPI_MODE_SEQUENTIAL)) {
        error_code = MPIO_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                          __func__, __LINE__, MPI_ERR_AMODE, "**fileamodeseq", 0);
        goto fn_fail;
    }

    MPI_Comm_dup(comm, &dupcomm);

    /* Check if ADIO has been initialized. If not, initialize it */
    MPIR_MPIOInit(&error_code);
    if (error_code != MPI_SUCCESS)
        goto fn_fail;

    /* check if amode is the same on all processes: at first glance, one might try
     * to use a built-in operator like MPI_BAND, but we need every mpi process to
     * agree the amode was not the same.  Consider process A with
     * MPI_MODE_CREATE|MPI_MODE_RDWR, and B with MPI_MODE_RDWR:  MPI_BAND yields
     * MPI_MODE_RDWR.  A determines amodes are different, but B proceeds having not
     * detected an error */
    int tmp_amode = 0;
    MPI_Allreduce(&amode, &tmp_amode, 1, MPI_INT, ADIO_same_amode, dupcomm);

    if (tmp_amode == ADIO_AMODE_NOMATCH) {
        error_code = MPIO_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                          __func__, __LINE__, MPI_ERR_NOT_SAME, "**fileamodediff",
                                          0);
        goto fn_fail;
    }

    int file_system = -1, known_fstype;
    ADIOI_Fns *fsops;
    /* resolve file system type from file name; this is a collective call */
    known_fstype = ADIO_ResolveFileType(dupcomm, filename, &file_system, &fsops, &error_code);
    if (error_code != MPI_SUCCESS) {
        /* ADIO_ResolveFileType() will print as informative a message as it
         * possibly can or call MPIO_Err_setmsg.  We just need to propagate
         * the error up.
         */
        goto fn_fail;
    }

    if (known_fstype) {
        /* filename contains a known file system type prefix, such as "ufs:".
         * strip off prefix if there is one, but only skip prefixes
         * if they are greater than length one to allow for windows
         * drive specifications (e.g. c:\...)
         */
        char *tmp = strchr(filename, ':');
        if (tmp > filename + 1) {
            filename = tmp + 1;
        }
    }

    /* use default values for disp, etype, filetype */
    *fh = ADIO_Open(comm, dupcomm, filename, file_system, fsops, amode, 0,
                    MPI_BYTE, MPI_BYTE, info, ADIO_PERM_NULL, &error_code);

    if (error_code != MPI_SUCCESS) {
        goto fn_fail;
    }

    /* if MPI_MODE_SEQUENTIAL requested, file systems cannot do explicit offset
     * or independent file pointer accesses, leaving not much else aside from
     * shared file pointer accesses. */
    if (!ADIO_Feature((*fh), ADIO_SHARED_FP) && (amode & MPI_MODE_SEQUENTIAL)) {
        error_code = MPIO_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                          __func__, __LINE__,
                                          MPI_ERR_UNSUPPORTED_OPERATION, "**iosequnsupported", 0);
        ADIO_Close(*fh, &error_code);
        goto fn_fail;
    }

    /* determine name of file that will hold the shared file pointer */
    /* can't support shared file pointers on a file system that doesn't
     * support file locking. */
    if ((error_code == MPI_SUCCESS) && ADIO_Feature((*fh), ADIO_SHARED_FP)) {
        int rank;
        MPI_Comm_rank(dupcomm, &rank);
        ADIOI_Shfp_fname(*fh, rank, &error_code);
        if (error_code != MPI_SUCCESS)
            goto fn_fail;

        /* if MPI_MODE_APPEND, set the shared file pointer to end of file.
         * indiv. file pointer already set to end of file in ADIO_Open.
         * Here file view is just bytes. */
        if ((*fh)->access_mode & MPI_MODE_APPEND) {
            if (rank == (*fh)->hints->ranklist[0])      /* only one person need set the sharedfp */
                ADIO_Set_shared_fp(*fh, (*fh)->fp_ind, &error_code);
            MPI_Barrier(dupcomm);
        }
    }

  fn_exit:
    return error_code;
  fn_fail:
    if (dupcomm != MPI_COMM_NULL)
        MPI_Comm_free(&dupcomm);
    goto fn_exit;
}

int MPIR_File_close_impl(MPI_File * fh)
{
    int error_code = MPI_SUCCESS;

    ADIO_File adio_fh;
    adio_fh = MPIO_File_resolve(*fh);

    /* --BEGIN ERROR HANDLING-- */
    MPIO_CHECK_FILE_HANDLE(adio_fh, __func__, error_code);
    /* --END ERROR HANDLING-- */

    if (ADIO_Feature(adio_fh, ADIO_SHARED_FP)) {
        ADIOI_Free((adio_fh)->shared_fp_fname);
        /* POSIX semantics say a deleted file remains available until all
         * processes close the file.  But since when was NFS posix-compliant?
         */
        /* this used to be gated by the lack of the UNLINK_AFTER_CLOSE feature,
         * but a race condition in GPFS necessated this.  See ticket #2214 */
        MPI_Barrier((adio_fh)->comm);
        if ((adio_fh)->shared_fp_fd != ADIO_FILE_NULL) {
            MPI_File *fh_shared = &(adio_fh->shared_fp_fd);
            ADIO_Close((adio_fh)->shared_fp_fd, &error_code);
            MPIO_File_free(fh_shared);
            /* --BEGIN ERROR HANDLING-- */
            if (error_code != MPI_SUCCESS)
                goto fn_fail;
            /* --END ERROR HANDLING-- */
        }
    }

    /* Because ROMIO expects the MPI library to provide error handler management
     * routines but it doesn't ever participate in MPI_File_close, we have to
     * somehow inform the MPI library that we no longer hold a reference to any
     * user defined error handler.  We do this by setting the errhandler at this
     * point to MPI_ERRORS_RETURN. */
    error_code = PMPI_File_set_errhandler(*fh, MPI_ERRORS_RETURN);
    if (error_code != MPI_SUCCESS)
        goto fn_fail;

    ADIO_Close(adio_fh, &error_code);
    MPIO_File_free(fh);

  fn_exit:
    return error_code;
  fn_fail:
    goto fn_exit;
}

int MPIR_File_delete_impl(const char *filename, MPI_Info info)
{
    int error_code = MPI_SUCCESS;
    int file_system, known_fstype;
    char *tmp;
    ADIOI_Fns *fsops;

    MPL_UNREFERENCED_ARG(info);

    MPIR_MPIOInit(&error_code);
    if (error_code != MPI_SUCCESS)
        goto fn_exit;

    /* resolve file system type from file name; this is a collective call */
    known_fstype = ADIO_ResolveFileType(MPI_COMM_SELF, filename, &file_system, &fsops, &error_code);

    /* --BEGIN ERROR HANDLING-- */
    if (error_code != MPI_SUCCESS) {
        /* ADIO_ResolveFileType() will print as informative a message as it
         * possibly can or call MPIR_Err_setmsg.  We just need to propagate
         * the error up.  In the PRINT_ERR_MSG case MPI_Abort has already
         * been called as well, so we probably didn't even make it this far.
         */
        goto fn_fail;
    }
    /* --END ERROR HANDLING-- */

    if (known_fstype) {
        /* filename contains a known file system type prefix, such as "ufs:".
         * skip prefixes on file names if they have more than one character;
         * single-character prefixes are assumed to be windows drive
         * specifications (e.g. c:\foo) and are left alone.
         */
        tmp = strchr(filename, ':');
        if (tmp > filename + 1)
            filename = tmp + 1;
    }

    /* call the fs-specific delete function */
    (fsops->ADIOI_xxx_Delete) (filename, &error_code);

  fn_exit:
    return error_code;
  fn_fail:
    goto fn_exit;
}

int MPIR_File_get_amode_impl(MPI_File fh, int *amode)
{
    int error_code = MPI_SUCCESS;

    ADIO_File adio_fh = MPIO_File_resolve(fh);
    MPIO_CHECK_FILE_HANDLE(adio_fh, __func__, error_code);

    *amode = adio_fh->orig_access_mode;

  fn_exit:
    return error_code;
  fn_fail:
    goto fn_exit;
}

int MPIR_File_get_atomicity_impl(MPI_File fh, int *flag)
{
    int error_code = MPI_SUCCESS;

    ADIO_File adio_fh = MPIO_File_resolve(fh);
    MPIO_CHECK_FILE_HANDLE(adio_fh, __func__, error_code);

    *flag = adio_fh->atomicity;

  fn_exit:
    return error_code;
  fn_fail:
    goto fn_exit;
}

int MPIR_File_get_byte_offset_impl(MPI_File fh, MPI_Offset offset, MPI_Offset * disp)
{
    int error_code = MPI_SUCCESS;

    ADIO_File adio_fh = MPIO_File_resolve(fh);
    MPIO_CHECK_FILE_HANDLE(adio_fh, __func__, error_code);

    if (offset < 0) {
        error_code = MPIO_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                          __func__, __LINE__, MPI_ERR_ARG, "**iobadoffset", 0);
        goto fn_fail;
    }

    MPIO_CHECK_NOT_SEQUENTIAL_MODE(adio_fh, __func__, error_code);
    /* --END ERROR HANDLING-- */

    ADIOI_Get_byte_offset(adio_fh, offset, disp);

  fn_exit:
    return error_code;
  fn_fail:
    goto fn_exit;
}

int MPIR_File_get_group_impl(MPI_File fh, MPI_Group * group)
{
    int error_code = MPI_SUCCESS;

    ADIO_File adio_fh = MPIO_File_resolve(fh);
    MPIO_CHECK_FILE_HANDLE(adio_fh, __func__, error_code);

    /* note: this will return the group of processes that called open, but
     * with deferred open this might not be the group of processes that
     * actually opened the file from the file system's perspective
     */
    error_code = MPI_Comm_group(adio_fh->comm, group);

  fn_exit:
    return error_code;
  fn_fail:
    goto fn_exit;
}

int MPIR_File_get_info_impl(MPI_File fh, MPI_Info * info_used)
{
    int error_code = MPI_SUCCESS;

    ADIO_File adio_fh = MPIO_File_resolve(fh);
    MPIO_CHECK_FILE_HANDLE(adio_fh, __func__, error_code);

    error_code = MPI_Info_dup(adio_fh->info, info_used);

  fn_exit:
    return error_code;
  fn_fail:
    goto fn_exit;
}

int MPIR_File_get_position_impl(MPI_File fh, MPI_Offset * offset)
{
    int error_code = MPI_SUCCESS;

    ADIO_File adio_fh = MPIO_File_resolve(fh);
    MPIO_CHECK_FILE_HANDLE(adio_fh, __func__, error_code);

    MPIO_CHECK_NOT_SEQUENTIAL_MODE(adio_fh, __func__, error_code);

    ADIOI_Get_position(adio_fh, offset);

  fn_exit:
    return error_code;
  fn_fail:
    goto fn_exit;
}

int MPIR_File_get_position_shared_impl(MPI_File fh, MPI_Offset * offset)
{
    int error_code = MPI_SUCCESS;

    ADIO_File adio_fh = MPIO_File_resolve(fh);
    MPIO_CHECK_FILE_HANDLE(adio_fh, __func__, error_code);

    MPIO_CHECK_NOT_SEQUENTIAL_MODE(adio_fh, __func__, error_code);
    MPIO_CHECK_FS_SUPPORTS_SHARED(adio_fh, __func__, error_code);

    ADIOI_TEST_DEFERRED(adio_fh, __func__, &error_code);

    ADIO_Get_shared_fp(adio_fh, 0, offset, &error_code);

  fn_exit:
    return error_code;
  fn_fail:
    goto fn_exit;
}

int MPIR_File_get_size_impl(MPI_File fh, MPI_Offset * size)
{
    int error_code = MPI_SUCCESS;

    ADIO_File adio_fh = MPIO_File_resolve(fh);
    MPIO_CHECK_FILE_HANDLE(adio_fh, __func__, error_code);

    if (size == NULL) {
        error_code = MPIO_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                          __func__, __LINE__, MPI_ERR_ARG,
                                          "**nullptr", "**nullptr %s", "size");
        goto fn_fail;
    }

    ADIOI_TEST_DEFERRED(adio_fh, __func__, &error_code);

    ADIO_Fcntl_t *fcntl_struct;
    fcntl_struct = (ADIO_Fcntl_t *) ADIOI_Malloc(sizeof(ADIO_Fcntl_t));
    ADIO_Fcntl(adio_fh, ADIO_FCNTL_GET_FSIZE, fcntl_struct, &error_code);
    if (error_code != MPI_SUCCESS) {
        goto fn_fail;
    }

    *size = fcntl_struct->fsize;
    ADIOI_Free(fcntl_struct);

  fn_exit:
    return error_code;
  fn_fail:
    goto fn_exit;
}

int MPIR_File_get_type_extent_impl(MPI_File fh, MPI_Datatype datatype, MPI_Aint * extent)
{
    int error_code = MPI_SUCCESS;

    ADIO_File adio_fh = MPIO_File_resolve(fh);
    MPIO_CHECK_FILE_HANDLE(adio_fh, __func__, error_code);

    MPIO_CHECK_DATATYPE(adio_fh, datatype, __func__, error_code);

    /* FIXME: handle other file data representations */

    MPI_Aint lb_i, extent_i;
    error_code = PMPI_Type_get_extent(datatype, &lb_i, &extent_i);
    *extent = extent_i;

  fn_exit:
    return error_code;
  fn_fail:
    goto fn_exit;
}

int MPIR_File_get_view_impl(MPI_File fh, MPI_Offset * disp, MPI_Datatype * etype,
                            MPI_Datatype * filetype, char *datarep)
{
    int error_code = MPI_SUCCESS;
    int is_predef;
    MPI_Datatype copy_etype, copy_filetype;

    ADIO_File adio_fh = MPIO_File_resolve(fh);
    MPIO_CHECK_FILE_HANDLE(adio_fh, __func__, error_code);

    if (datarep == NULL) {
        error_code = MPIO_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                          __func__, __LINE__, MPI_ERR_ARG, "**iodatarepnomem", 0);
        goto fn_exit;
    }
    /* --END ERROR HANDLING-- */

    *disp = adio_fh->disp;
    ADIOI_Strncpy(datarep,
                  (adio_fh->is_external32 ? "external32" : "native"), MPI_MAX_DATAREP_STRING);

    ADIOI_Type_ispredef(adio_fh->etype, &is_predef);
    if (is_predef)
        *etype = adio_fh->etype;
    else {
#ifdef MPIIMPL_HAVE_MPI_COMBINER_DUP
        MPI_Type_dup(adio_fh->etype, &copy_etype);
#else
        MPI_Type_contiguous(1, adio_fh->etype, &copy_etype);
#endif

        MPI_Type_commit(&copy_etype);
        *etype = copy_etype;
    }
    ADIOI_Type_ispredef(adio_fh->filetype, &is_predef);
    if (is_predef)
        *filetype = adio_fh->filetype;
    else {
#ifdef MPIIMPL_HAVE_MPI_COMBINER_DUP
        MPI_Type_dup(adio_fh->filetype, &copy_filetype);
#else
        MPI_Type_contiguous(1, adio_fh->filetype, &copy_filetype);
#endif

        MPI_Type_commit(&copy_filetype);
        *filetype = copy_filetype;
    }

  fn_exit:
    return error_code;
  fn_fail:
    goto fn_exit;
}

int MPIR_File_iread_impl(MPI_File fh, void *buf, MPI_Aint count, MPI_Datatype datatype,
                         MPI_Request * request)
{
    return MPIOI_File_iread(fh, (MPI_Offset) 0, ADIO_INDIVIDUAL, buf, count, datatype, request);
}

int MPIR_File_iread_all_impl(MPI_File fh, void *buf, MPI_Aint count, MPI_Datatype datatype,
                             MPI_Request * request)
{
    return MPIOI_File_iread_all(fh, (MPI_Offset) 0, ADIO_INDIVIDUAL, buf, count, datatype, request);
}

int MPIR_File_iread_at_impl(MPI_File fh, MPI_Offset offset, void *buf, MPI_Aint count,
                            MPI_Datatype datatype, MPI_Request * request)
{
    return MPIOI_File_iread(fh, offset, ADIO_EXPLICIT_OFFSET, buf, count, datatype, request);
}

int MPIR_File_iread_at_all_impl(MPI_File fh, MPI_Offset offset, void *buf, MPI_Aint count,
                                MPI_Datatype datatype, MPI_Request * request)
{
    return MPIOI_File_iread_all(fh, offset, ADIO_EXPLICIT_OFFSET, buf, count, datatype, request);
}

int MPIR_File_iread_shared_impl(MPI_File fh, void *buf, MPI_Aint count, MPI_Datatype datatype,
                                MPI_Request * request)
{
    return MPIOI_File_iread_shared(fh, buf, count, datatype, request);
}

int MPIR_File_iwrite_impl(MPI_File fh, const void *buf, MPI_Aint count, MPI_Datatype datatype,
                          MPI_Request * request)
{
    return MPIOI_File_iwrite(fh, (MPI_Offset) 0, ADIO_INDIVIDUAL, buf, count, datatype, request);
}

int MPIR_File_iwrite_all_impl(MPI_File fh, const void *buf, MPI_Aint count, MPI_Datatype datatype,
                              MPI_Request * request)
{
    return MPIOI_File_iwrite_all(fh, (MPI_Offset) 0, ADIO_INDIVIDUAL,
                                 buf, count, datatype, request);
}

int MPIR_File_iwrite_at_impl(MPI_File fh, MPI_Offset offset, const void *buf, MPI_Aint count,
                             MPI_Datatype datatype, MPI_Request * request)
{
    return MPIOI_File_iwrite(fh, offset, ADIO_EXPLICIT_OFFSET, buf, count, datatype, request);
}

int MPIR_File_iwrite_at_all_impl(MPI_File fh, MPI_Offset offset, const void *buf, MPI_Aint count,
                                 MPI_Datatype datatype, MPI_Request * request)
{
    return MPIOI_File_iwrite_all(fh, offset, ADIO_EXPLICIT_OFFSET, buf, count, datatype, request);
}

int MPIR_File_iwrite_shared_impl(MPI_File fh, const void *buf, MPI_Aint count,
                                 MPI_Datatype datatype, MPI_Request * request)
{
    return MPIOI_File_iwrite_shared(fh, buf, count, datatype, request);
}

int MPIR_File_preallocate_impl(MPI_File fh, MPI_Offset size)
{
    int error_code = MPI_SUCCESS;

    ADIO_File adio_fh = MPIO_File_resolve(fh);
    MPIO_CHECK_FILE_HANDLE(adio_fh, __func__, error_code);

    if (size < 0) {
        error_code = MPIO_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                          __func__, __LINE__, MPI_ERR_ARG, "**iobadsize", 0);
        goto fn_fail;
    }

    MPI_Offset tmp_sz, max_sz, min_sz;
    tmp_sz = size;
    MPI_Allreduce(&tmp_sz, &max_sz, 1, ADIO_OFFSET, MPI_MAX, adio_fh->comm);
    MPI_Allreduce(&tmp_sz, &min_sz, 1, ADIO_OFFSET, MPI_MIN, adio_fh->comm);

    if (max_sz != min_sz) {
        error_code = MPIO_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                          __func__, __LINE__, MPI_ERR_ARG, "**notsame", 0);
        goto fn_fail;
    }
    /* --END ERROR HANDLING-- */

    if (size == 0)
        goto fn_exit;

    ADIOI_TEST_DEFERRED(adio_fh, __func__, &error_code);

    int mynod = 0;
    MPI_Comm_rank(adio_fh->comm, &mynod);
    if (!mynod) {
        ADIO_Fcntl_t *fcntl_struct;
        fcntl_struct = (ADIO_Fcntl_t *) ADIOI_Malloc(sizeof(ADIO_Fcntl_t));
        fcntl_struct->diskspace = size;
        ADIO_Fcntl(adio_fh, ADIO_FCNTL_SET_DISKSPACE, fcntl_struct, &error_code);
        ADIOI_Free(fcntl_struct);
    }
    MPI_Barrier(adio_fh->comm);

  fn_exit:
    return error_code;
  fn_fail:
    goto fn_exit;
}

int MPIR_File_read_impl(MPI_File fh, void *buf, MPI_Aint count, MPI_Datatype datatype,
                        MPI_Status * status)
{
    return MPIOI_File_read(fh, (MPI_Offset) 0, ADIO_INDIVIDUAL, buf, count, datatype, status);
}

int MPIR_File_read_all_impl(MPI_File fh, void *buf, MPI_Aint count, MPI_Datatype datatype,
                            MPI_Status * status)
{
    return MPIOI_File_read_all(fh, (MPI_Offset) 0, ADIO_INDIVIDUAL, buf, count, datatype, status);
}

int MPIR_File_read_all_begin_impl(MPI_File fh, void *buf, MPI_Aint count, MPI_Datatype datatype)
{
    return MPIOI_File_read_all_begin(fh, (MPI_Offset) 0, ADIO_INDIVIDUAL, buf, count, datatype);
}

int MPIR_File_read_all_end_impl(MPI_File fh, void *buf, MPI_Status * status)
{
    return MPIOI_File_read_all_end(fh, buf, status);
}

int MPIR_File_read_at_impl(MPI_File fh, MPI_Offset offset, void *buf, MPI_Aint count,
                           MPI_Datatype datatype, MPI_Status * status)
{
    return MPIOI_File_read(fh, offset, ADIO_EXPLICIT_OFFSET, buf, count, datatype, status);
}

int MPIR_File_read_at_all_impl(MPI_File fh, MPI_Offset offset, void *buf, MPI_Aint count,
                               MPI_Datatype datatype, MPI_Status * status)
{
    return MPIOI_File_read_all(fh, offset, ADIO_EXPLICIT_OFFSET, buf, count, datatype, status);
}

int MPIR_File_read_at_all_begin_impl(MPI_File fh, MPI_Offset offset, void *buf, MPI_Aint count,
                                     MPI_Datatype datatype)
{
    return MPIOI_File_read_all_begin(fh, offset, ADIO_EXPLICIT_OFFSET, buf, count, datatype);
}

int MPIR_File_read_at_all_end_impl(MPI_File fh, void *buf, MPI_Status * status)
{
    return MPIOI_File_read_all_end(fh, buf, status);
}

int MPIR_File_read_ordered_impl(MPI_File fh, void *buf, MPI_Aint count, MPI_Datatype datatype,
                                MPI_Status * status)
{
    return MPIOI_File_read_ordered(fh, buf, count, datatype, status);
}

int MPIR_File_read_ordered_begin_impl(MPI_File fh, void *buf, MPI_Aint count, MPI_Datatype datatype)
{
    return MPIOI_File_read_ordered_begin(fh, buf, count, datatype);
}

int MPIR_File_read_ordered_end_impl(MPI_File fh, void *buf, MPI_Status * status)
{
    return MPIOI_File_read_ordered_end(fh, buf, status);
}

int MPIR_File_read_shared_impl(MPI_File fh, void *buf, MPI_Aint count, MPI_Datatype datatype,
                               MPI_Status * status)
{
    return MPIOI_File_read_shared(fh, buf, count, datatype, status);
}

int MPIR_File_seek_impl(MPI_File fh, MPI_Offset offset, int whence)
{
    int error_code = MPI_SUCCESS;
    MPI_Offset curr_offset, eof_offset;

    ADIO_File adio_fh = MPIO_File_resolve(fh);
    MPIO_CHECK_FILE_HANDLE(adio_fh, __func__, error_code);

    MPIO_CHECK_NOT_SEQUENTIAL_MODE(adio_fh, __func__, error_code);

    switch (whence) {
        case MPI_SEEK_SET:
            /* --BEGIN ERROR HANDLING-- */
            if (offset < 0) {
                error_code = MPIO_Err_create_code(MPI_SUCCESS,
                                                  MPIR_ERR_RECOVERABLE, __func__,
                                                  __LINE__, MPI_ERR_ARG, "**iobadoffset", 0);
                goto fn_fail;
            }
            /* --END ERROR HANDLING-- */
            break;
        case MPI_SEEK_CUR:
            /* find offset corr. to current location of file pointer */
            ADIOI_Get_position(adio_fh, &curr_offset);
            offset += curr_offset;

            /* --BEGIN ERROR HANDLING-- */
            if (offset < 0) {
                error_code = MPIO_Err_create_code(MPI_SUCCESS,
                                                  MPIR_ERR_RECOVERABLE, __func__,
                                                  __LINE__, MPI_ERR_ARG, "**ionegoffset", 0);
                goto fn_fail;
            }
            /* --END ERROR HANDLING-- */

            break;
        case MPI_SEEK_END:
            /* we can in many cases do seeks w/o a file actually opened, but not in
             * the MPI_SEEK_END case */
            ADIOI_TEST_DEFERRED(adio_fh, "MPI_File_seek", &error_code);

            /* find offset corr. to end of file */
            ADIOI_Get_eof_offset(adio_fh, &eof_offset);
            offset += eof_offset;

            /* --BEGIN ERROR HANDLING-- */
            if (offset < 0) {
                error_code = MPIO_Err_create_code(MPI_SUCCESS,
                                                  MPIR_ERR_RECOVERABLE, __func__,
                                                  __LINE__, MPI_ERR_ARG, "**ionegoffset", 0);
                goto fn_fail;
            }
            /* --END ERROR HANDLING-- */

            break;
        default:
            /* --BEGIN ERROR HANDLING-- */
            error_code = MPIO_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                              __func__, __LINE__, MPI_ERR_ARG, "**iobadwhence", 0);
            goto fn_fail;
            /* --END ERROR HANDLING-- */
    }

    ADIO_SeekIndividual(adio_fh, offset, ADIO_SEEK_SET, &error_code);

  fn_exit:
    return error_code;
  fn_fail:
    goto fn_exit;
}

int MPIR_File_seek_shared_impl(MPI_File fh, MPI_Offset offset, int whence)
{
    int error_code = MPI_SUCCESS;
    MPI_Offset curr_offset, eof_offset;

    ADIO_File adio_fh = MPIO_File_resolve(fh);
    MPIO_CHECK_FILE_HANDLE(adio_fh, __func__, error_code);

    MPIO_CHECK_NOT_SEQUENTIAL_MODE(adio_fh, __func__, error_code);
    MPIO_CHECK_FS_SUPPORTS_SHARED(adio_fh, __func__, error_code);
    /* --END ERROR HANDLING-- */

    MPI_Offset tmp_offset;
    tmp_offset = offset;
    MPI_Bcast(&tmp_offset, 1, ADIO_OFFSET, 0, adio_fh->comm);
    /* --BEGIN ERROR HANDLING-- */
    if (tmp_offset != offset) {
        error_code = MPIO_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                          __func__, __LINE__, MPI_ERR_ARG, "**notsame", 0);
        goto fn_fail;
    }
    /* --END ERROR HANDLING-- */

    int tmp_whence;
    tmp_whence = whence;
    MPI_Bcast(&tmp_whence, 1, MPI_INT, 0, adio_fh->comm);
    /* --BEGIN ERROR HANDLING-- */
    if (tmp_whence != whence) {
        error_code = MPIO_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                          __func__, __LINE__, MPI_ERR_ARG, "**iobadwhence", 0);
        goto fn_fail;
    }
    /* --END ERROR HANDLING-- */

    ADIOI_TEST_DEFERRED(adio_fh, "MPI_File_seek_shared", &error_code);

    int myrank;
    MPI_Comm_rank(adio_fh->comm, &myrank);

    if (!myrank) {
        switch (whence) {
            case MPI_SEEK_SET:
                /* --BEGIN ERROR HANDLING-- */
                if (offset < 0) {
                    error_code = MPIO_Err_create_code(MPI_SUCCESS,
                                                      MPIR_ERR_RECOVERABLE,
                                                      __func__, __LINE__,
                                                      MPI_ERR_ARG, "**iobadoffset", 0);
                    goto fn_fail;
                }
                /* --END ERROR HANDLING-- */
                break;
            case MPI_SEEK_CUR:
                /* get current location of shared file pointer */
                ADIO_Get_shared_fp(adio_fh, 0, &curr_offset, &error_code);
                /* --BEGIN ERROR HANDLING-- */
                if (error_code != MPI_SUCCESS) {
                    error_code = MPIO_Err_create_code(MPI_SUCCESS,
                                                      MPIR_ERR_FATAL,
                                                      __func__, __LINE__,
                                                      MPI_ERR_INTERN, "**iosharedfailed", 0);
                    goto fn_fail;
                }
                /* --END ERROR HANDLING-- */
                offset += curr_offset;
                /* --BEGIN ERROR HANDLING-- */
                if (offset < 0) {
                    error_code = MPIO_Err_create_code(MPI_SUCCESS,
                                                      MPIR_ERR_RECOVERABLE,
                                                      __func__, __LINE__,
                                                      MPI_ERR_ARG, "**ionegoffset", 0);
                    goto fn_fail;
                }
                /* --END ERROR HANDLING-- */
                break;
            case MPI_SEEK_END:
                /* find offset corr. to end of file */
                ADIOI_Get_eof_offset(adio_fh, &eof_offset);
                offset += eof_offset;
                /* --BEGIN ERROR HANDLING-- */
                if (offset < 0) {
                    error_code = MPIO_Err_create_code(MPI_SUCCESS,
                                                      MPIR_ERR_RECOVERABLE,
                                                      __func__, __LINE__,
                                                      MPI_ERR_ARG, "**ionegoffset", 0);
                    goto fn_fail;
                }
                /* --END ERROR HANDLING-- */
                break;
            default:
                /* --BEGIN ERROR HANDLING-- */
                error_code = MPIO_Err_create_code(MPI_SUCCESS,
                                                  MPIR_ERR_RECOVERABLE,
                                                  __func__, __LINE__, MPI_ERR_ARG,
                                                  "**iobadwhence", 0);
                goto fn_fail;
                /* --END ERROR HANDLING-- */
        }

        ADIO_Set_shared_fp(adio_fh, offset, &error_code);
        /* --BEGIN ERROR HANDLING-- */
        if (error_code != MPI_SUCCESS) {
            error_code = MPIO_Err_create_code(MPI_SUCCESS,
                                              MPIR_ERR_FATAL,
                                              __func__, __LINE__,
                                              MPI_ERR_INTERN, "**iosharedfailed", 0);
            goto fn_fail;
        }
        /* --END ERROR HANDLING-- */

    }

    /* FIXME: explain why the barrier is necessary */
    MPI_Barrier(adio_fh->comm);

  fn_exit:
    return error_code;
  fn_fail:
    goto fn_exit;
}

int MPIR_File_set_atomicity_impl(MPI_File fh, int flag)
{
    int error_code = MPI_SUCCESS;

    ADIO_File adio_fh = MPIO_File_resolve(fh);
    MPIO_CHECK_FILE_HANDLE(adio_fh, __func__, error_code);

    ADIOI_TEST_DEFERRED(adio_fh, __func__, &error_code);

    if (flag)
        flag = 1;       /* take care of non-one values! */

    /* check if flag is the same on all processes */
    int tmp_flag;
    tmp_flag = flag;
    MPI_Bcast(&tmp_flag, 1, MPI_INT, 0, adio_fh->comm);

    /* --BEGIN ERROR HANDLING-- */
    if (tmp_flag != flag) {
        error_code = MPIO_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                          __func__, __LINE__, MPI_ERR_ARG, "**notsame", 0);
        goto fn_fail;
    }
    /* --END ERROR HANDLING-- */

    if (adio_fh->atomicity == flag) {
        error_code = MPI_SUCCESS;
        goto fn_exit;
    }

    ADIO_Fcntl_t *fcntl_struct;
    fcntl_struct = (ADIO_Fcntl_t *) ADIOI_Malloc(sizeof(ADIO_Fcntl_t));
    fcntl_struct->atomicity = flag;
    ADIO_Fcntl(adio_fh, ADIO_FCNTL_SET_ATOMICITY, fcntl_struct, &error_code);
    /* TODO: what do we do with this error code? */

    /* --BEGIN ERROR HANDLING-- */
    if (error_code != MPI_SUCCESS)
        goto fn_fail;
    /* --END ERROR HANDLING-- */

    ADIOI_Free(fcntl_struct);
  fn_exit:
    return error_code;
  fn_fail:
    goto fn_exit;
}

int MPIR_File_set_info_impl(MPI_File fh, MPI_Info info)
{
    int error_code = MPI_SUCCESS;

    ADIO_File adio_fh = MPIO_File_resolve(fh);
    MPIO_CHECK_FILE_HANDLE(adio_fh, __func__, error_code);

    MPIO_CHECK_INFO_ALL(info, __func__, error_code, fh->comm);

    /* set new info */
    ADIO_SetInfo(adio_fh, info, &error_code);

  fn_exit:
    return error_code;
  fn_fail:
    goto fn_exit;
}

int MPIR_File_set_size_impl(MPI_File fh, MPI_Offset size)
{
    int error_code = MPI_SUCCESS;

    ADIO_File adio_fh = MPIO_File_resolve(fh);
    MPIO_CHECK_FILE_HANDLE(adio_fh, __func__, error_code);

    MPIO_CHECK_NOT_SEQUENTIAL_MODE(adio_fh, __func__, error_code);

    if (size < 0) {
        error_code = MPIO_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                          __func__, __LINE__, MPI_ERR_ARG, "**iobadsize", 0);
        goto fn_fail;
    }
    MPIO_CHECK_WRITABLE(fh, __func__, error_code);

    MPI_Offset tmp_sz, max_sz, min_sz;
    tmp_sz = size;
    MPI_Allreduce(&tmp_sz, &max_sz, 1, ADIO_OFFSET, MPI_MAX, adio_fh->comm);
    MPI_Allreduce(&tmp_sz, &min_sz, 1, ADIO_OFFSET, MPI_MIN, adio_fh->comm);

    /* --BEGIN ERROR HANDLING-- */
    if (max_sz != min_sz) {
        error_code = MPIO_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                          __func__, __LINE__, MPI_ERR_ARG, "**notsame", 0);
        goto fn_fail;
    }
    /* --END ERROR HANDLING-- */

    if (!ADIO_Feature(adio_fh, ADIO_SCALABLE_RESIZE)) {
        /* rare stupid file systems (like NFS) need to carry out resize on all
         * processes */
        ADIOI_TEST_DEFERRED(adio_fh, "MPI_File_set_size", &error_code);
    }

    ADIO_Resize(adio_fh, size, &error_code);

  fn_exit:
    return error_code;
  fn_fail:
    goto fn_exit;
}

int MPIR_File_set_view_impl(MPI_File fh, MPI_Offset disp, MPI_Datatype etype, MPI_Datatype filetype,
                            const char *datarep, MPI_Info info)
{
    int error_code = MPI_SUCCESS;

    ADIO_File adio_fh = MPIO_File_resolve(fh);
    MPIO_CHECK_FILE_HANDLE(adio_fh, __func__, error_code);

    if ((disp < 0) && (disp != MPI_DISPLACEMENT_CURRENT)) {
        error_code = MPIO_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                          __func__, __LINE__, MPI_ERR_ARG, "**iobaddisp", 0);
        goto fn_fail;
    }

    /* rudimentary checks for incorrect etype/filetype. */
    if (etype == MPI_DATATYPE_NULL) {
        error_code = MPIO_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                          __func__, __LINE__, MPI_ERR_ARG, "**ioetype", 0);
        goto fn_fail;
    }
    MPIO_DATATYPE_ISCOMMITTED(etype, error_code);
    if (error_code != MPI_SUCCESS) {
        goto fn_fail;
    }

    if (filetype == MPI_DATATYPE_NULL) {
        error_code = MPIO_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                          __func__, __LINE__, MPI_ERR_ARG, "**iofiletype", 0);
        goto fn_fail;
    }
    MPIO_DATATYPE_ISCOMMITTED(filetype, error_code);
    if (error_code != MPI_SUCCESS) {
        goto fn_fail;
    }

    if ((adio_fh->access_mode & MPI_MODE_SEQUENTIAL) && (disp != MPI_DISPLACEMENT_CURRENT)) {
        error_code = MPIO_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                          __func__, __LINE__, MPI_ERR_ARG, "**iodispifseq", 0);
        goto fn_fail;
    }

    if ((disp == MPI_DISPLACEMENT_CURRENT) && !(adio_fh->access_mode & MPI_MODE_SEQUENTIAL)) {
        error_code = MPIO_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                          __func__, __LINE__, MPI_ERR_ARG, "**iodispifseq", 0);
        goto fn_fail;
    }
    MPIO_CHECK_INFO_ALL(info, __func__, error_code, adio_fh->comm);
    /* --END ERROR HANDLING-- */

    MPI_Count filetype_size, etype_size;
    MPI_Type_size_x(filetype, &filetype_size);
    MPI_Type_size_x(etype, &etype_size);

    /* --BEGIN ERROR HANDLING-- */
    if (etype_size != 0 && filetype_size % etype_size != 0) {
        error_code = MPIO_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                          __func__, __LINE__, MPI_ERR_ARG, "**iofiletype", 0);
        goto fn_fail;
    }

    if ((datarep == NULL) || (strcmp(datarep, "native") &&
                              strcmp(datarep, "NATIVE") &&
                              strcmp(datarep, "external32") &&
                              strcmp(datarep, "EXTERNAL32") &&
                              strcmp(datarep, "internal") && strcmp(datarep, "INTERNAL"))) {
        error_code = MPIO_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                          __func__, __LINE__,
                                          MPI_ERR_UNSUPPORTED_DATAREP, "**unsupporteddatarep", 0);
        goto fn_fail;
    }
    /* --END ERROR HANDLING-- */

    if (disp == MPI_DISPLACEMENT_CURRENT) {
        ADIO_Offset shared_fp, byte_off;

        MPI_Barrier(adio_fh->comm);
        ADIO_Get_shared_fp(adio_fh, 0, &shared_fp, &error_code);
        /* TODO: check error code */

        MPI_Barrier(adio_fh->comm);
        ADIOI_Get_byte_offset(adio_fh, shared_fp, &byte_off);
        /* TODO: check error code */

        disp = byte_off;
    }

    ADIO_Set_view(adio_fh, disp, etype, filetype, info, &error_code);

    /* --BEGIN ERROR HANDLING-- */
    if (error_code != MPI_SUCCESS) {
        goto fn_fail;
    }
    /* --END ERROR HANDLING-- */

    /* reset shared file pointer to zero */
    if (ADIO_Feature(adio_fh, ADIO_SHARED_FP) && (adio_fh->shared_fp_fd != ADIO_FILE_NULL)) {
        /* only one process needs to set it to zero, but I don't want to
         * create the shared-file-pointer file if shared file pointers have
         * not been used so far. Therefore, every process that has already
         * opened the shared-file-pointer file sets the shared file pointer
         * to zero. If the file was not opened, the value is automatically
         * zero. Note that shared file pointer is stored as no. of etypes
         * relative to the current view, whereas indiv. file pointer is
         * stored in bytes. */

        ADIO_Set_shared_fp(adio_fh, 0, &error_code);
        /* --BEGIN ERROR HANDLING-- */
        if (error_code != MPI_SUCCESS)
            goto fn_fail;
        /* --END ERROR HANDLING-- */
    }

    if (ADIO_Feature(adio_fh, ADIO_SHARED_FP)) {
        MPI_Barrier(adio_fh->comm);     /* for above to work correctly */
    }
    if (strcmp(datarep, "external32") && strcmp(datarep, "EXTERNAL32"))
        adio_fh->is_external32 = 0;
    else
        adio_fh->is_external32 = 1;

  fn_exit:
    return error_code;
  fn_fail:
    goto fn_exit;
}

int MPIR_File_sync_impl(MPI_File fh)
{
    int error_code = MPI_SUCCESS;

    ADIO_File adio_fh = MPIO_File_resolve(fh);
    MPIO_CHECK_FILE_HANDLE(adio_fh, __func__, error_code);

    if ((adio_fh == NULL) || ((adio_fh)->cookie != ADIOI_FILE_COOKIE)) {
        error_code = MPIO_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                          __func__, __LINE__, MPI_ERR_ARG, "**iobadfh", 0);
        goto fn_fail;
    }

    MPIO_CHECK_WRITABLE(fh, __func__, error_code);

    ADIO_Flush(adio_fh, &error_code);

  fn_exit:
    return error_code;
  fn_fail:
    goto fn_exit;
}

int MPIR_File_write_impl(MPI_File fh, const void *buf, MPI_Aint count, MPI_Datatype datatype,
                         MPI_Status * status)
{
    return MPIOI_File_write(fh, (MPI_Offset) 0, ADIO_INDIVIDUAL, buf, count, datatype, status);
}

int MPIR_File_write_all_impl(MPI_File fh, const void *buf, MPI_Aint count, MPI_Datatype datatype,
                             MPI_Status * status)
{
    return MPIOI_File_write_all(fh, (MPI_Offset) 0, ADIO_INDIVIDUAL, buf, count, datatype, status);
}

int MPIR_File_write_all_begin_impl(MPI_File fh, const void *buf, MPI_Aint count,
                                   MPI_Datatype datatype)
{
    return MPIOI_File_write_all_begin(fh, (MPI_Offset) 0, ADIO_INDIVIDUAL, buf, count, datatype);
}

int MPIR_File_write_all_end_impl(MPI_File fh, const void *buf, MPI_Status * status)
{
    return MPIOI_File_write_all_end(fh, buf, status);
}

int MPIR_File_write_at_impl(MPI_File fh, MPI_Offset offset, const void *buf, MPI_Aint count,
                            MPI_Datatype datatype, MPI_Status * status)
{
    return MPIOI_File_write(fh, offset, ADIO_EXPLICIT_OFFSET, buf, count, datatype, status);
}

int MPIR_File_write_at_all_impl(MPI_File fh, MPI_Offset offset, const void *buf, MPI_Aint count,
                                MPI_Datatype datatype, MPI_Status * status)
{
    return MPIOI_File_write_all(fh, offset, ADIO_EXPLICIT_OFFSET, buf, count, datatype, status);
}

int MPIR_File_write_at_all_begin_impl(MPI_File fh, MPI_Offset offset, const void *buf,
                                      MPI_Aint count, MPI_Datatype datatype)
{
    return MPIOI_File_write_all_begin(fh, offset, ADIO_EXPLICIT_OFFSET, buf, count, datatype);
}

int MPIR_File_write_at_all_end_impl(MPI_File fh, const void *buf, MPI_Status * status)
{
    return MPIOI_File_write_all_end(fh, buf, status);
}

int MPIR_File_write_ordered_impl(MPI_File fh, const void *buf, MPI_Aint count,
                                 MPI_Datatype datatype, MPI_Status * status)
{
    return MPIOI_File_write_ordered(fh, buf, count, datatype, status);
}

int MPIR_File_write_ordered_begin_impl(MPI_File fh, const void *buf, MPI_Aint count,
                                       MPI_Datatype datatype)
{
    return MPIOI_File_write_ordered_begin(fh, buf, count, datatype);
}

int MPIR_File_write_ordered_end_impl(MPI_File fh, const void *buf, MPI_Status * status)
{
    return MPIOI_File_write_ordered_end(fh, buf, status);
}

int MPIR_File_write_shared_impl(MPI_File fh, const void *buf, MPI_Aint count, MPI_Datatype datatype,
                                MPI_Status * status)
{
    return MPIOI_File_write_shared(fh, buf, count, datatype, status);
}

int MPIR_Register_datarep_impl(ROMIO_CONST char *datarep,
                               MPI_Datarep_conversion_function * read_conversion_fn,
                               MPI_Datarep_conversion_function * write_conversion_fn,
                               MPI_Datarep_extent_function * dtype_file_extent_fn,
                               void *extra_state)
{
    int is_large = false;
    return MPIOI_Register_datarep(datarep, (MPIOI_VOID_FN *) read_conversion_fn,
                                  (MPIOI_VOID_FN *) write_conversion_fn,
                                  dtype_file_extent_fn, extra_state, is_large);
}

int MPIR_Register_datarep_large_impl(ROMIO_CONST char *datarep,
                                     MPI_Datarep_conversion_function_c * read_conversion_fn,
                                     MPI_Datarep_conversion_function_c * write_conversion_fn,
                                     MPI_Datarep_extent_function * dtype_file_extent_fn,
                                     void *extra_state)
{
    int is_large = true;
    return MPIOI_Register_datarep(datarep, (MPIOI_VOID_FN *) read_conversion_fn,
                                  (MPIOI_VOID_FN *) write_conversion_fn,
                                  dtype_file_extent_fn, extra_state, is_large);
}

MPI_Fint MPIR_File_c2f_impl(MPI_File fh)
{
    return MPIO_File_c2f(fh);
}

MPI_File MPIR_File_f2c_impl(MPI_Fint fh)
{
    return MPIO_File_f2c(fh);
}

/* internal routines */

static int MPIOI_File_iread(MPI_File fh, MPI_Offset offset, int file_ptr_type,
                            void *buf, MPI_Aint count, MPI_Datatype datatype, MPI_Request * request)
{
    int error_code, buftype_is_contig, filetype_is_contig;
    MPI_Count datatype_size;
    ADIO_Status status;
    ADIO_File adio_fh;
    ADIO_Offset off, bufsize;
    MPI_Offset nbytes = 0;
    void *xbuf = NULL, *e32_buf = NULL, *host_buf = NULL;

    adio_fh = MPIO_File_resolve(fh);

    /* --BEGIN ERROR HANDLING-- */
    MPIO_CHECK_FILE_HANDLE(adio_fh, __func__, error_code);
    MPIO_CHECK_COUNT(adio_fh, count, __func__, error_code);
    MPIO_CHECK_DATATYPE(adio_fh, datatype, __func__, error_code);

    if (file_ptr_type == ADIO_EXPLICIT_OFFSET && offset < 0) {
        error_code = MPIO_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                          __func__, __LINE__, MPI_ERR_ARG, "**iobadoffset", 0);
        goto fn_fail;
    }
    /* --END ERROR HANDLING-- */

    MPI_Type_size_x(datatype, &datatype_size);

    /* --BEGIN ERROR HANDLING-- */
    MPIO_CHECK_INTEGRAL_ETYPE(adio_fh, count, datatype_size, __func__, error_code);
    MPIO_CHECK_READABLE(adio_fh, __func__, error_code);
    MPIO_CHECK_NOT_SEQUENTIAL_MODE(adio_fh, __func__, error_code);
    MPIO_CHECK_COUNT_SIZE(adio_fh, count, datatype_size, __func__, error_code);
    /* --END ERROR HANDLING-- */

    ADIOI_Datatype_iscontig(datatype, &buftype_is_contig);
    ADIOI_Datatype_iscontig(adio_fh->filetype, &filetype_is_contig);

    ADIOI_TEST_DEFERRED(adio_fh, __func__, &error_code);

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
    return error_code;
  fn_fail:
    goto fn_exit;
}

static int MPIOI_File_iread_all(MPI_File fh, MPI_Offset offset, int file_ptr_type,
                                void *buf, MPI_Aint count, MPI_Datatype datatype,
                                MPI_Request * request)
{
    int error_code;
    MPI_Count datatype_size;
    ADIO_File adio_fh;
    void *xbuf = NULL, *e32_buf = NULL, *host_buf = NULL;

    adio_fh = MPIO_File_resolve(fh);

    /* --BEGIN ERROR HANDLING-- */
    MPIO_CHECK_FILE_HANDLE(adio_fh, __func__, error_code);
    MPIO_CHECK_COUNT(adio_fh, count, __func__, error_code);
    MPIO_CHECK_DATATYPE(adio_fh, datatype, __func__, error_code);

    if (file_ptr_type == ADIO_EXPLICIT_OFFSET && offset < 0) {
        error_code = MPIO_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                          __func__, __LINE__, MPI_ERR_ARG, "**iobadoffset", 0);
        goto fn_fail;
    }
    /* --END ERROR HANDLING-- */

    MPI_Type_size_x(datatype, &datatype_size);

    /* --BEGIN ERROR HANDLING-- */
    MPIO_CHECK_INTEGRAL_ETYPE(adio_fh, count, datatype_size, __func__, error_code);
    MPIO_CHECK_READABLE(adio_fh, __func__, error_code);
    MPIO_CHECK_NOT_SEQUENTIAL_MODE(adio_fh, __func__, error_code);
    MPIO_CHECK_COUNT_SIZE(adio_fh, count, datatype_size, __func__, error_code);
    /* --END ERROR HANDLING-- */

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

    ADIO_IreadStridedColl(adio_fh, xbuf, count, datatype, file_ptr_type,
                          offset, request, &error_code);

    /* --BEGIN ERROR HANDLING-- */
    if (error_code != MPI_SUCCESS)
        goto fn_fail;
    /* --END ERROR HANDLING-- */

    if (e32_buf != NULL) {
        error_code = MPIU_read_external32_conversion_fn(buf, datatype, count, e32_buf);
        ADIOI_Free(e32_buf);
    }

    MPIO_GPU_SWAP_BACK(host_buf, buf, count, datatype);

  fn_exit:
    return error_code;
  fn_fail:
    goto fn_exit;
}

static int MPIOI_File_iread_shared(MPI_File fh, void *buf, MPI_Aint count,
                                   MPI_Datatype datatype, MPI_Request * request)
{
    int error_code, buftype_is_contig, filetype_is_contig;
    ADIO_Offset bufsize;
    ADIO_File adio_fh;
    MPI_Count datatype_size, incr;
    MPI_Status status;
    ADIO_Offset off, shared_fp;
    MPI_Offset nbytes = 0;
    void *xbuf = NULL, *e32_buf = NULL, *host_buf = NULL;

    adio_fh = MPIO_File_resolve(fh);

    /* --BEGIN ERROR HANDLING-- */
    MPIO_CHECK_FILE_HANDLE(adio_fh, __func__, error_code);
    MPIO_CHECK_COUNT(adio_fh, count, __func__, error_code);
    MPIO_CHECK_DATATYPE(adio_fh, datatype, __func__, error_code);
    /* --END ERROR HANDLING-- */

    MPI_Type_size_x(datatype, &datatype_size);

    /* --BEGIN ERROR HANDLING-- */
    MPIO_CHECK_INTEGRAL_ETYPE(adio_fh, count, datatype_size, __func__, error_code);
    MPIO_CHECK_FS_SUPPORTS_SHARED(adio_fh, __func__, error_code);
    MPIO_CHECK_COUNT_SIZE(adio_fh, count, datatype_size, __func__, error_code);
    /* --END ERROR HANDLING-- */

    ADIOI_Datatype_iscontig(datatype, &buftype_is_contig);
    ADIOI_Datatype_iscontig(adio_fh->filetype, &filetype_is_contig);

    ADIOI_TEST_DEFERRED(adio_fh, __func__, &error_code);

    incr = (count * datatype_size) / adio_fh->etype_size;
    ADIO_Get_shared_fp(adio_fh, incr, &shared_fp, &error_code);

    /* --BEGIN ERROR HANDLING-- */
    if (error_code != MPI_SUCCESS) {
        /* note: ADIO_Get_shared_fp should have set up error code already? */
        goto fn_fail;
    }
    /* --END ERROR HANDLING-- */

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
        /* convert count and shared_fp to bytes */
        bufsize = datatype_size * count;
        off = adio_fh->disp + adio_fh->etype_size * shared_fp;
        if (!(adio_fh->atomicity)) {
            ADIO_IreadContig(adio_fh, xbuf, count, datatype, ADIO_EXPLICIT_OFFSET,
                             off, request, &error_code);
        } else {
            /* to maintain strict atomicity semantics with other concurrent
             * operations, lock (exclusive) and call blocking routine */

            if (adio_fh->file_system != ADIO_NFS) {
                ADIOI_WRITE_LOCK(adio_fh, off, SEEK_SET, bufsize);
            }

            ADIO_ReadContig(adio_fh, xbuf, count, datatype, ADIO_EXPLICIT_OFFSET,
                            off, &status, &error_code);

            if (adio_fh->file_system != ADIO_NFS) {
                ADIOI_UNLOCK(adio_fh, off, SEEK_SET, bufsize);
            }
            if (error_code == MPI_SUCCESS) {
                nbytes = count * datatype_size;
            }
            MPIO_Completed_request_create(&adio_fh, nbytes, &error_code, request);
        }
    } else {
        ADIO_IreadStrided(adio_fh, xbuf, count, datatype, ADIO_EXPLICIT_OFFSET,
                          shared_fp, request, &error_code);
    }

    /* --BEGIN ERROR HANDLING-- */
    if (error_code != MPI_SUCCESS)
        goto fn_fail;
    /* --END ERROR HANDLING-- */

    if (e32_buf != NULL) {
        error_code = MPIU_read_external32_conversion_fn(buf, datatype, count, e32_buf);
        ADIOI_Free(e32_buf);
    }

    MPIO_GPU_SWAP_BACK(host_buf, buf, count, datatype);

  fn_exit:
    return error_code;
  fn_fail:
    goto fn_exit;
}

static int MPIOI_File_iwrite(MPI_File fh, MPI_Offset offset, int file_ptr_type,
                             const void *buf, MPI_Aint count, MPI_Datatype datatype,
                             MPI_Request * request)
{
    int error_code, buftype_is_contig, filetype_is_contig;
    MPI_Count datatype_size;
    ADIO_Status status;
    ADIO_Offset off, bufsize;
    ADIO_File adio_fh;
    MPI_Offset nbytes = 0;
    void *e32buf = NULL;
    const void *xbuf = NULL;
    void *host_buf = NULL;

    adio_fh = MPIO_File_resolve(fh);

    /* --BEGIN ERROR HANDLING-- */
    MPIO_CHECK_FILE_HANDLE(adio_fh, __func__, error_code);
    MPIO_CHECK_COUNT(adio_fh, count, __func__, error_code);
    MPIO_CHECK_DATATYPE(adio_fh, datatype, __func__, error_code);

    if (file_ptr_type == ADIO_EXPLICIT_OFFSET && offset < 0) {
        error_code = MPIO_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                          __func__, __LINE__, MPI_ERR_ARG, "**iobadoffset", 0);
        goto fn_fail;
    }
    /* --END ERROR HANDLING-- */

    MPI_Type_size_x(datatype, &datatype_size);

    /* --BEGIN ERROR HANDLING-- */
    MPIO_CHECK_INTEGRAL_ETYPE(adio_fh, count, datatype_size, __func__, error_code);
    MPIO_CHECK_WRITABLE(adio_fh, __func__, error_code);
    MPIO_CHECK_NOT_SEQUENTIAL_MODE(adio_fh, __func__, error_code);
    MPIO_CHECK_COUNT_SIZE(adio_fh, count, datatype_size, __func__, error_code);
    /* --END ERROR HANDLING-- */

    ADIOI_Datatype_iscontig(datatype, &buftype_is_contig);
    ADIOI_Datatype_iscontig(adio_fh->filetype, &filetype_is_contig);

    ADIOI_TEST_DEFERRED(adio_fh, __func__, &error_code);

    xbuf = buf;
    if (adio_fh->is_external32) {
        error_code = MPIU_external32_buffer_setup(buf, count, datatype, &e32buf);
        if (error_code != MPI_SUCCESS)
            goto fn_exit;

        xbuf = e32buf;
    } else {
        MPIO_GPU_HOST_SWAP(host_buf, buf, count, datatype);
        if (host_buf != NULL) {
            xbuf = host_buf;
        }
    }

    if (buftype_is_contig && filetype_is_contig) {
        /* convert sizes to bytes */
        bufsize = datatype_size * count;
        if (file_ptr_type == ADIO_EXPLICIT_OFFSET) {
            off = adio_fh->disp + adio_fh->etype_size * offset;
        } else {
            off = adio_fh->fp_ind;
        }

        if (!(adio_fh->atomicity)) {
            ADIO_IwriteContig(adio_fh, xbuf, count, datatype, file_ptr_type,
                              off, request, &error_code);
        } else {
            /* to maintain strict atomicity semantics with other concurrent
             * operations, lock (exclusive) and call blocking routine */
            if (ADIO_Feature(adio_fh, ADIO_LOCKS)) {
                ADIOI_WRITE_LOCK(adio_fh, off, SEEK_SET, bufsize);
            }

            ADIO_WriteContig(adio_fh, xbuf, count, datatype, file_ptr_type, off,
                             &status, &error_code);

            if (ADIO_Feature(adio_fh, ADIO_LOCKS)) {
                ADIOI_UNLOCK(adio_fh, off, SEEK_SET, bufsize);
            }
            if (error_code == MPI_SUCCESS) {
                nbytes = count * datatype_size;
            }

            MPIO_Completed_request_create(&adio_fh, nbytes, &error_code, request);
        }
    } else {
        ADIO_IwriteStrided(adio_fh, xbuf, count, datatype, file_ptr_type,
                           offset, request, &error_code);
    }

    MPIO_GPU_HOST_FREE(host_buf, count, datatype);

  fn_exit:
    if (e32buf != NULL)
        ADIOI_Free(e32buf);
    return error_code;
  fn_fail:
    goto fn_exit;
}

static int MPIOI_File_iwrite_all(MPI_File fh, MPI_Offset offset, int file_ptr_type,
                                 const void *buf, MPI_Aint count, MPI_Datatype datatype,
                                 MPI_Request * request)
{
    int error_code;
    MPI_Count datatype_size;
    ADIO_File adio_fh;
    void *e32buf = NULL;
    const void *xbuf = NULL;
    void *host_buf = NULL;

    adio_fh = MPIO_File_resolve(fh);

    /* --BEGIN ERROR HANDLING-- */
    MPIO_CHECK_FILE_HANDLE(adio_fh, __func__, error_code);
    MPIO_CHECK_COUNT(adio_fh, count, __func__, error_code);
    MPIO_CHECK_DATATYPE(adio_fh, datatype, __func__, error_code);

    if (file_ptr_type == ADIO_EXPLICIT_OFFSET && offset < 0) {
        error_code = MPIO_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                          __func__, __LINE__, MPI_ERR_ARG, "**iobadoffset", 0);
        goto fn_fail;
    }
    /* --END ERROR HANDLING-- */

    MPI_Type_size_x(datatype, &datatype_size);

    /* --BEGIN ERROR HANDLING-- */
    MPIO_CHECK_INTEGRAL_ETYPE(adio_fh, count, datatype_size, __func__, error_code);
    MPIO_CHECK_WRITABLE(adio_fh, __func__, error_code);
    MPIO_CHECK_NOT_SEQUENTIAL_MODE(adio_fh, __func__, error_code);
    MPIO_CHECK_COUNT_SIZE(adio_fh, count, datatype_size, __func__, error_code);
    /* --END ERROR HANDLING-- */

    xbuf = buf;
    if (adio_fh->is_external32) {
        error_code = MPIU_external32_buffer_setup(buf, count, datatype, &e32buf);
        if (error_code != MPI_SUCCESS)
            goto fn_exit;

        xbuf = e32buf;
    } else {
        MPIO_GPU_HOST_SWAP(host_buf, buf, count, datatype);
        if (host_buf != NULL) {
            xbuf = host_buf;
        }
    }

    ADIO_IwriteStridedColl(adio_fh, xbuf, count, datatype, file_ptr_type,
                           offset, request, &error_code);

    /* --BEGIN ERROR HANDLING-- */
    if (error_code != MPI_SUCCESS)
        goto fn_fail;
    /* --END ERROR HANDLING-- */

    MPIO_GPU_HOST_FREE(host_buf, count, datatype);

  fn_exit:
    if (e32buf != NULL)
        ADIOI_Free(e32buf);

    return error_code;
  fn_fail:
    goto fn_exit;
}

static int MPIOI_File_iwrite_shared(MPI_File fh, const void *buf, MPI_Aint count,
                                    MPI_Datatype datatype, MPIO_Request * request)
{
    int error_code, buftype_is_contig, filetype_is_contig;
    ADIO_File adio_fh;
    ADIO_Offset incr, bufsize;
    MPI_Count datatype_size;
    ADIO_Status status;
    ADIO_Offset off, shared_fp;
    void *e32buf = NULL;
    const void *xbuf = NULL;
    void *host_buf = NULL;

    adio_fh = MPIO_File_resolve(fh);

    /* --BEGIN ERROR HANDLING-- */
    MPIO_CHECK_FILE_HANDLE(adio_fh, __func__, error_code);
    MPIO_CHECK_COUNT(adio_fh, count, __func__, error_code);
    MPIO_CHECK_DATATYPE(adio_fh, datatype, __func__, error_code);
    /* --END ERROR HANDLING-- */

    MPI_Type_size_x(datatype, &datatype_size);

    /* --BEGIN ERROR HANDLING-- */
    MPIO_CHECK_INTEGRAL_ETYPE(adio_fh, count, datatype_size, __func__, error_code);
    MPIO_CHECK_FS_SUPPORTS_SHARED(adio_fh, __func__, error_code);
    MPIO_CHECK_COUNT_SIZE(adio_fh, count, datatype_size, __func__, error_code);
    /* --END ERROR HANDLING-- */

    ADIOI_Datatype_iscontig(datatype, &buftype_is_contig);
    ADIOI_Datatype_iscontig(adio_fh->filetype, &filetype_is_contig);

    ADIOI_TEST_DEFERRED(adio_fh, __func__, &error_code);

    incr = (count * datatype_size) / adio_fh->etype_size;
    ADIO_Get_shared_fp(adio_fh, incr, &shared_fp, &error_code);
    if (error_code != MPI_SUCCESS) {
        /* note: ADIO_Get_shared_fp should have set up error code already? */
        goto fn_fail;
    }

    xbuf = buf;
    if (adio_fh->is_external32) {
        error_code = MPIU_external32_buffer_setup(buf, count, datatype, &e32buf);
        if (error_code != MPI_SUCCESS)
            goto fn_exit;

        xbuf = e32buf;
    } else {
        MPIO_GPU_HOST_SWAP(host_buf, buf, count, datatype);
        if (host_buf != NULL) {
            xbuf = host_buf;
        }
    }

    /* contiguous or strided? */
    if (buftype_is_contig && filetype_is_contig) {
        /* convert sizes to bytes */
        bufsize = datatype_size * count;
        off = adio_fh->disp + adio_fh->etype_size * shared_fp;
        if (!(adio_fh->atomicity))
            ADIO_IwriteContig(adio_fh, xbuf, count, datatype, ADIO_EXPLICIT_OFFSET,
                              off, request, &error_code);
        else {
            /* to maintain strict atomicity semantics with other concurrent
             * operations, lock (exclusive) and call blocking routine */

            if (adio_fh->file_system != ADIO_NFS)
                ADIOI_WRITE_LOCK(adio_fh, off, SEEK_SET, bufsize);

            ADIO_WriteContig(adio_fh, xbuf, count, datatype, ADIO_EXPLICIT_OFFSET,
                             off, &status, &error_code);

            if (adio_fh->file_system != ADIO_NFS)
                ADIOI_UNLOCK(adio_fh, off, SEEK_SET, bufsize);

            MPIO_Completed_request_create(&adio_fh, bufsize, &error_code, request);
        }
    } else
        ADIO_IwriteStrided(adio_fh, xbuf, count, datatype, ADIO_EXPLICIT_OFFSET,
                           shared_fp, request, &error_code);

    MPIO_GPU_HOST_FREE(host_buf, count, datatype);

  fn_exit:
    if (e32buf != NULL)
        ADIOI_Free(e32buf);

    return error_code;
  fn_fail:
    goto fn_exit;
}

static int MPIOI_File_read(MPI_File fh, MPI_Offset offset, int file_ptr_type,
                           void *buf, MPI_Aint count, MPI_Datatype datatype, MPI_Status * status)
{
    int error_code, buftype_is_contig, filetype_is_contig;
    MPI_Count datatype_size;
    ADIO_File adio_fh;
    ADIO_Offset off, bufsize;
    void *xbuf = NULL, *e32_buf = NULL, *host_buf = NULL;

    adio_fh = MPIO_File_resolve(fh);

    /* --BEGIN ERROR HANDLING-- */
    MPIO_CHECK_FILE_HANDLE(adio_fh, __func__, error_code);
    MPIO_CHECK_COUNT(adio_fh, count, __func__, error_code);
    MPIO_CHECK_DATATYPE(adio_fh, datatype, __func__, error_code);

    if (file_ptr_type == ADIO_EXPLICIT_OFFSET && offset < 0) {
        error_code = MPIO_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                          __func__, __LINE__, MPI_ERR_ARG, "**iobadoffset", 0);
        goto fn_fail;
    }
    /* --END ERROR HANDLING-- */

    MPI_Type_size_x(datatype, &datatype_size);

    /* --BEGIN ERROR HANDLING-- */
    MPIO_CHECK_COUNT_SIZE(adio_fh, count, datatype_size, __func__, error_code);
    /* --END ERROR HANDLING-- */

    if (count * datatype_size == 0) {
#ifdef HAVE_STATUS_SET_BYTES
        MPIR_Status_set_bytes(status, datatype, 0);
#endif
        error_code = MPI_SUCCESS;
        goto fn_exit;
    }

    /* --BEGIN ERROR HANDLING-- */
    MPIO_CHECK_INTEGRAL_ETYPE(adio_fh, count, datatype_size, __func__, error_code);
    MPIO_CHECK_READABLE(adio_fh, __func__, error_code);
    MPIO_CHECK_NOT_SEQUENTIAL_MODE(adio_fh, __func__, error_code);
    /* --END ERROR HANDLING-- */

    ADIOI_Datatype_iscontig(datatype, &buftype_is_contig);
    ADIOI_Datatype_iscontig(adio_fh->filetype, &filetype_is_contig);

    ADIOI_TEST_DEFERRED(adio_fh, __func__, &error_code);

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
        } else {        /* ADIO_INDIVIDUAL */

            off = adio_fh->fp_ind;
        }

        /* if atomic mode requested, lock (exclusive) the region, because
         * there could be a concurrent noncontiguous request.
         */
        if ((adio_fh->atomicity) && ADIO_Feature(adio_fh, ADIO_LOCKS)) {
            ADIOI_WRITE_LOCK(adio_fh, off, SEEK_SET, bufsize);
        }

        ADIO_ReadContig(adio_fh, xbuf, count, datatype, file_ptr_type, off, status, &error_code);

        if ((adio_fh->atomicity) && ADIO_Feature(adio_fh, ADIO_LOCKS)) {
            ADIOI_UNLOCK(adio_fh, off, SEEK_SET, bufsize);
        }
    } else {
        ADIO_ReadStrided(adio_fh, xbuf, count, datatype, file_ptr_type,
                         offset, status, &error_code);
        /* For strided and atomic mode, locking is done in ADIO_ReadStrided */
    }

    /* --BEGIN ERROR HANDLING-- */
    if (error_code != MPI_SUCCESS)
        goto fn_fail;
    /* --END ERROR HANDLING-- */

    if (e32_buf != NULL) {
        error_code = MPIU_read_external32_conversion_fn(buf, datatype, count, e32_buf);
        ADIOI_Free(e32_buf);
    }

    MPIO_GPU_SWAP_BACK(host_buf, buf, count, datatype);

  fn_exit:
    return error_code;
  fn_fail:
    goto fn_exit;
}

static int MPIOI_File_read_all(MPI_File fh, MPI_Offset offset, int file_ptr_type,
                               void *buf, MPI_Aint count, MPI_Datatype datatype,
                               MPI_Status * status)
{
    int error_code;
    MPI_Count datatype_size;
    ADIO_File adio_fh;
    void *xbuf = NULL, *e32_buf = NULL, *host_buf = NULL;

    adio_fh = MPIO_File_resolve(fh);

    /* --BEGIN ERROR HANDLING-- */
    MPIO_CHECK_FILE_HANDLE(adio_fh, __func__, error_code);
    MPIO_CHECK_COUNT(adio_fh, count, __func__, error_code);
    MPIO_CHECK_DATATYPE(adio_fh, datatype, __func__, error_code);

    if (file_ptr_type == ADIO_EXPLICIT_OFFSET && offset < 0) {
        error_code = MPIO_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                          __func__, __LINE__, MPI_ERR_ARG, "**iobadoffset", 0);
        goto fn_fail;
    }
    /* --END ERROR HANDLING-- */

    MPI_Type_size_x(datatype, &datatype_size);

    /* --BEGIN ERROR HANDLING-- */
    MPIO_CHECK_INTEGRAL_ETYPE(adio_fh, count, datatype_size, __func__, error_code);
    MPIO_CHECK_READABLE(adio_fh, __func__, error_code);
    MPIO_CHECK_NOT_SEQUENTIAL_MODE(adio_fh, __func__, error_code);
    MPIO_CHECK_COUNT_SIZE(adio_fh, count, datatype_size, __func__, error_code);
    /* --END ERROR HANDLING-- */

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

    ADIO_ReadStridedColl(adio_fh, xbuf, count, datatype, file_ptr_type,
                         offset, status, &error_code);

    /* --BEGIN ERROR HANDLING-- */
    if (error_code != MPI_SUCCESS)
        goto fn_fail;
    /* --END ERROR HANDLING-- */

    if (e32_buf != NULL) {
        error_code = MPIU_read_external32_conversion_fn(buf, datatype, count, e32_buf);
        ADIOI_Free(e32_buf);
    }

    MPIO_GPU_SWAP_BACK(host_buf, buf, count, datatype);

  fn_exit:
    return error_code;
  fn_fail:
    goto fn_exit;
}

static int MPIOI_File_read_all_begin(MPI_File fh, MPI_Offset offset, int file_ptr_type,
                                     void *buf, MPI_Aint count, MPI_Datatype datatype)
{
    int error_code;
    MPI_Count datatype_size;
    ADIO_File adio_fh;
    void *xbuf = NULL, *e32_buf = NULL, *host_buf = NULL;

    adio_fh = MPIO_File_resolve(fh);

    /* --BEGIN ERROR HANDLING-- */
    MPIO_CHECK_FILE_HANDLE(adio_fh, __func__, error_code);
    MPIO_CHECK_COUNT(adio_fh, count, __func__, error_code);
    MPIO_CHECK_DATATYPE(adio_fh, datatype, __func__, error_code);

    if (file_ptr_type == ADIO_EXPLICIT_OFFSET && offset < 0) {
        error_code = MPIO_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                          __func__, __LINE__, MPI_ERR_ARG, "**iobadoffset", 0);
        goto fn_fail;
    }
    /* --END ERROR HANDLING-- */

    MPI_Type_size_x(datatype, &datatype_size);

    /* --BEGIN ERROR HANDLING-- */
    MPIO_CHECK_INTEGRAL_ETYPE(adio_fh, count, datatype_size, __func__, error_code);
    MPIO_CHECK_READABLE(adio_fh, __func__, error_code);
    MPIO_CHECK_NOT_SEQUENTIAL_MODE(adio_fh, __func__, error_code);

    if (adio_fh->split_coll_count) {
        error_code = MPIO_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                          __func__, __LINE__, MPI_ERR_IO, "**iosplitcoll", 0);
        goto fn_fail;
    }
    MPIO_CHECK_COUNT_SIZE(adio_fh, count, datatype_size, __func__, error_code);
    /* --END ERROR HANDLING-- */

    adio_fh->split_coll_count = 1;

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

    ADIO_ReadStridedColl(adio_fh, xbuf, count, datatype, file_ptr_type,
                         offset, &adio_fh->split_status, &error_code);

    /* --BEGIN ERROR HANDLING-- */
    if (error_code != MPI_SUCCESS)
        goto fn_fail;
    /* --END ERROR HANDLING-- */

    if (e32_buf != NULL) {
        error_code = MPIU_read_external32_conversion_fn(buf, datatype, count, e32_buf);
        ADIOI_Free(e32_buf);
    }

    MPIO_GPU_SWAP_BACK(host_buf, buf, count, datatype);

  fn_exit:
    return error_code;
  fn_fail:
    goto fn_exit;
}

static int MPIOI_File_read_all_end(MPI_File fh, void *buf, MPI_Status * status)
{
    int error_code = MPI_SUCCESS;
    ADIO_File adio_fh;

    MPL_UNREFERENCED_ARG(buf);

    adio_fh = MPIO_File_resolve(fh);

    /* --BEGIN ERROR HANDLING-- */
    MPIO_CHECK_FILE_HANDLE(adio_fh, __func__, error_code);

    if (!(adio_fh->split_coll_count)) {
        error_code = MPIO_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                          __func__, __LINE__, MPI_ERR_IO, "**iosplitcollnone", 0);
        goto fn_fail;
    }
    /* --END ERROR HANDLING-- */

#ifdef HAVE_STATUS_SET_BYTES
    if (status != MPI_STATUS_IGNORE)
        *status = adio_fh->split_status;
#endif
    adio_fh->split_coll_count = 0;

  fn_exit:
    return error_code;
  fn_fail:
    goto fn_exit;
}

static int MPIOI_File_read_ordered(MPI_File fh, void *buf, MPI_Aint count, MPI_Datatype datatype,
                                   MPI_Status * status)
{
    int error_code, nprocs, myrank;
    ADIO_Offset incr;
    MPI_Count datatype_size;
    int source, dest;
    ADIO_Offset shared_fp = 0;
    ADIO_File adio_fh;

    adio_fh = MPIO_File_resolve(fh);

    /* --BEGIN ERROR HANDLING-- */
    MPIO_CHECK_FILE_HANDLE(adio_fh, __func__, error_code);
    MPIO_CHECK_COUNT(adio_fh, count, __func__, error_code);
    MPIO_CHECK_DATATYPE(adio_fh, datatype, __func__, error_code);
    /* --END ERROR HANDLING-- */

    MPI_Type_size_x(datatype, &datatype_size);

    /* --BEGIN ERROR HANDLING-- */
    MPIO_CHECK_INTEGRAL_ETYPE(adio_fh, count, datatype_size, __func__, error_code);
    MPIO_CHECK_FS_SUPPORTS_SHARED(adio_fh, __func__, error_code);
    MPIO_CHECK_COUNT_SIZE(adio_fh, count, datatype_size, __func__, error_code);
    /* --END ERROR HANDLING-- */

    ADIOI_TEST_DEFERRED(adio_fh, "MPI_File_read_ordered", &error_code);

    MPI_Comm_size(adio_fh->comm, &nprocs);
    MPI_Comm_rank(adio_fh->comm, &myrank);

    incr = (count * datatype_size) / adio_fh->etype_size;

    /* Use a message as a 'token' to order the operations */
    source = myrank - 1;
    dest = myrank + 1;
    if (source < 0)
        source = MPI_PROC_NULL;
    if (dest >= nprocs)
        dest = MPI_PROC_NULL;
    MPI_Recv(NULL, 0, MPI_BYTE, source, 0, adio_fh->comm, MPI_STATUS_IGNORE);

    ADIO_Get_shared_fp(adio_fh, incr, &shared_fp, &error_code);
    /* --BEGIN ERROR HANDLING-- */
    if (error_code != MPI_SUCCESS) {
        goto fn_fail;
    }
    /* --END ERROR HANDLING-- */

    MPI_Send(NULL, 0, MPI_BYTE, dest, 0, adio_fh->comm);

    ADIO_ReadStridedColl(adio_fh, buf, count, datatype, ADIO_EXPLICIT_OFFSET,
                         shared_fp, status, &error_code);

    /* --BEGIN ERROR HANDLING-- */
    if (error_code != MPI_SUCCESS)
        goto fn_fail;
    /* --END ERROR HANDLING-- */

  fn_exit:
    /* FIXME: Check for error code from ReadStridedColl? */
    return error_code;
  fn_fail:
    goto fn_exit;
}

static int MPIOI_File_read_ordered_begin(MPI_File fh,
                                         void *buf, MPI_Aint count, MPI_Datatype datatype)
{
    int error_code, nprocs, myrank;
    MPI_Count datatype_size;
    int source, dest;
    ADIO_Offset shared_fp, incr;
    ADIO_File adio_fh;
    void *xbuf = NULL, *e32_buf = NULL, *host_buf = NULL;

    adio_fh = MPIO_File_resolve(fh);

    /* --BEGIN ERROR HANDLING-- */
    MPIO_CHECK_FILE_HANDLE(adio_fh, __func__, error_code);
    MPIO_CHECK_COUNT(adio_fh, count, __func__, error_code);
    MPIO_CHECK_DATATYPE(adio_fh, datatype, __func__, error_code);

    if (adio_fh->split_coll_count) {
        error_code = MPIO_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                          __func__, __LINE__, MPI_ERR_IO, "**iosplitcoll", 0);
        goto fn_fail;
    }
    /* --END ERROR HANDLING-- */

    adio_fh->split_coll_count = 1;


    MPI_Type_size_x(datatype, &datatype_size);
    /* --BEGIN ERROR HANDLING-- */
    MPIO_CHECK_INTEGRAL_ETYPE(adio_fh, count, datatype_size, __func__, error_code);
    MPIO_CHECK_FS_SUPPORTS_SHARED(adio_fh, __func__, error_code);
    MPIO_CHECK_COUNT_SIZE(adio_fh, count, datatype_size, __func__, error_code);
    /* --END ERROR HANDLING-- */

    ADIOI_TEST_DEFERRED(adio_fh, __func__, &error_code);

    MPI_Comm_size(adio_fh->comm, &nprocs);
    MPI_Comm_rank(adio_fh->comm, &myrank);

    incr = (count * datatype_size) / adio_fh->etype_size;
    /* Use a message as a 'token' to order the operations */
    source = myrank - 1;
    dest = myrank + 1;
    if (source < 0)
        source = MPI_PROC_NULL;
    if (dest >= nprocs)
        dest = MPI_PROC_NULL;
    MPI_Recv(NULL, 0, MPI_BYTE, source, 0, adio_fh->comm, MPI_STATUS_IGNORE);

    ADIO_Get_shared_fp(adio_fh, incr, &shared_fp, &error_code);
    /* --BEGIN ERROR HANDLING-- */
    if (error_code != MPI_SUCCESS) {
        goto fn_fail;
    }
    /* --END ERROR HANDLING-- */

    MPI_Send(NULL, 0, MPI_BYTE, dest, 0, adio_fh->comm);

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


    ADIO_ReadStridedColl(adio_fh, xbuf, count, datatype, ADIO_EXPLICIT_OFFSET,
                         shared_fp, &adio_fh->split_status, &error_code);

    /* --BEGIN ERROR HANDLING-- */
    if (error_code != MPI_SUCCESS)
        goto fn_fail;
    /* --END ERROR HANDLING-- */

    if (e32_buf != NULL) {
        error_code = MPIU_read_external32_conversion_fn(buf, datatype, count, e32_buf);
        ADIOI_Free(e32_buf);
    }

    MPIO_GPU_SWAP_BACK(host_buf, buf, count, datatype);

  fn_exit:
    return error_code;
  fn_fail:
    goto fn_exit;
}

static int MPIOI_File_read_ordered_end(MPI_File fh, void *buf, MPI_Status * status)
{
    int error_code = MPI_SUCCESS;
    ADIO_File adio_fh;

    MPL_UNREFERENCED_ARG(buf);

    adio_fh = MPIO_File_resolve(fh);

    /* --BEGIN ERROR HANDLING-- */
    MPIO_CHECK_FILE_HANDLE(adio_fh, __func__, error_code);

    if (!(adio_fh->split_coll_count)) {
        error_code = MPIO_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                          __func__, __LINE__, MPI_ERR_IO, "**iosplitcollnone", 0);
        goto fn_fail;
    }
    /* --END ERROR HANDLING-- */

#ifdef HAVE_STATUS_SET_BYTES
    if (status != MPI_STATUS_IGNORE)
        *status = adio_fh->split_status;
#endif
    adio_fh->split_coll_count = 0;

  fn_exit:
    return error_code;
  fn_fail:
    goto fn_exit;
}

static int MPIOI_File_read_shared(MPI_File fh, void *buf, MPI_Aint count, MPI_Datatype datatype,
                                  MPI_Status * status)
{
    int error_code, buftype_is_contig, filetype_is_contig;
    MPI_Count datatype_size;
    ADIO_Offset off, shared_fp, incr, bufsize;
    ADIO_File adio_fh;
    void *xbuf = NULL, *e32_buf = NULL, *host_buf = NULL;

    adio_fh = MPIO_File_resolve(fh);

    /* --BEGIN ERROR HANDLING-- */
    MPIO_CHECK_FILE_HANDLE(adio_fh, __func__, error_code);
    MPIO_CHECK_COUNT(adio_fh, count, __func__, error_code);
    MPIO_CHECK_DATATYPE(adio_fh, datatype, __func__, error_code);
    /* --END ERROR HANDLING-- */

    MPI_Type_size_x(datatype, &datatype_size);

    /* --BEGIN ERROR HANDLING-- */
    MPIO_CHECK_COUNT_SIZE(adio_fh, count, datatype_size, __func__, error_code);
    /* --END ERROR HANDLING-- */

    if (count * datatype_size == 0) {
#ifdef HAVE_STATUS_SET_BYTES
        MPIR_Status_set_bytes(status, datatype, 0);
#endif
        error_code = MPI_SUCCESS;
        goto fn_exit;
    }

    /* --BEGIN ERROR HANDLING-- */
    MPIO_CHECK_INTEGRAL_ETYPE(adio_fh, count, datatype_size, __func__, error_code);
    MPIO_CHECK_READABLE(adio_fh, __func__, error_code);
    MPIO_CHECK_FS_SUPPORTS_SHARED(adio_fh, __func__, error_code);
    /* --END ERROR HANDLING-- */

    ADIOI_Datatype_iscontig(datatype, &buftype_is_contig);
    ADIOI_Datatype_iscontig(adio_fh->filetype, &filetype_is_contig);

    ADIOI_TEST_DEFERRED(adio_fh, __func__, &error_code);

    incr = (count * datatype_size) / adio_fh->etype_size;

    ADIO_Get_shared_fp(adio_fh, incr, &shared_fp, &error_code);
    /* --BEGIN ERROR HANDLING-- */
    if (error_code != MPI_SUCCESS) {
        goto fn_fail;
    }
    /* --END ERROR HANDLING-- */

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

    /* contiguous or strided? */
    if (buftype_is_contig && filetype_is_contig) {
        /* convert count and shared_fp to bytes */
        bufsize = datatype_size * count;
        off = adio_fh->disp + adio_fh->etype_size * shared_fp;

        /* if atomic mode requested, lock (exclusive) the region, because there
         * could be a concurrent noncontiguous request. On NFS, locking
         * is done in the ADIO_ReadContig. */

        if ((adio_fh->atomicity) && (adio_fh->file_system != ADIO_NFS))
            ADIOI_WRITE_LOCK(adio_fh, off, SEEK_SET, bufsize);

        ADIO_ReadContig(adio_fh, xbuf, count, datatype, ADIO_EXPLICIT_OFFSET,
                        off, status, &error_code);

        if ((adio_fh->atomicity) && (adio_fh->file_system != ADIO_NFS))
            ADIOI_UNLOCK(adio_fh, off, SEEK_SET, bufsize);
    } else {
        ADIO_ReadStrided(adio_fh, xbuf, count, datatype, ADIO_EXPLICIT_OFFSET,
                         shared_fp, status, &error_code);
        /* For strided and atomic mode, locking is done in ADIO_ReadStrided */
    }

    /* --BEGIN ERROR HANDLING-- */
    if (error_code != MPI_SUCCESS)
        goto fn_fail;
    /* --END ERROR HANDLING-- */

    if (e32_buf != NULL) {
        error_code = MPIU_read_external32_conversion_fn(buf, datatype, count, e32_buf);
        ADIOI_Free(e32_buf);
    }

    MPIO_GPU_SWAP_BACK(host_buf, buf, count, datatype);

  fn_exit:
    return error_code;
  fn_fail:
    goto fn_exit;
}

static int MPIOI_File_write(MPI_File fh, MPI_Offset offset, int file_ptr_type,
                            const void *buf, MPI_Aint count, MPI_Datatype datatype,
                            MPI_Status * status)
{
    int error_code, buftype_is_contig, filetype_is_contig;
    MPI_Count datatype_size;
    ADIO_Offset off, bufsize;
    ADIO_File adio_fh;
    void *e32buf = NULL;
    const void *xbuf = NULL;
    void *host_buf = NULL;

    adio_fh = MPIO_File_resolve(fh);

    /* --BEGIN ERROR HANDLING-- */
    MPIO_CHECK_FILE_HANDLE(adio_fh, __func__, error_code);
    MPIO_CHECK_COUNT(adio_fh, count, __func__, error_code);
    MPIO_CHECK_DATATYPE(adio_fh, datatype, __func__, error_code);

    if (file_ptr_type == ADIO_EXPLICIT_OFFSET && offset < 0) {
        error_code = MPIO_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                          __func__, __LINE__, MPI_ERR_ARG, "**iobadoffset", 0);
        goto fn_fail;
    }
    /* --END ERROR HANDLING-- */

    MPI_Type_size_x(datatype, &datatype_size);

    /* --BEGIN ERROR HANDLING-- */
    MPIO_CHECK_COUNT_SIZE(adio_fh, count, datatype_size, __func__, error_code);
    /* --END ERROR HANDLING-- */

    if (count * datatype_size == 0) {
#ifdef HAVE_STATUS_SET_BYTES
        MPIR_Status_set_bytes(status, datatype, 0);
#endif
        error_code = MPI_SUCCESS;
        goto fn_exit;
    }

    /* --BEGIN ERROR HANDLING-- */
    MPIO_CHECK_INTEGRAL_ETYPE(adio_fh, count, datatype_size, __func__, error_code);
    MPIO_CHECK_WRITABLE(adio_fh, __func__, error_code);
    MPIO_CHECK_NOT_SEQUENTIAL_MODE(adio_fh, __func__, error_code);
    /* --END ERROR HANDLING-- */

    ADIOI_Datatype_iscontig(datatype, &buftype_is_contig);
    ADIOI_Datatype_iscontig(adio_fh->filetype, &filetype_is_contig);

    ADIOI_TEST_DEFERRED(adio_fh, __func__, &error_code);

    xbuf = buf;
    if (adio_fh->is_external32) {
        error_code = MPIU_external32_buffer_setup(buf, count, datatype, &e32buf);
        if (error_code != MPI_SUCCESS)
            goto fn_exit;

        xbuf = e32buf;
    } else {
        MPIO_GPU_HOST_SWAP(host_buf, buf, count, datatype);
        if (host_buf != NULL) {
            xbuf = host_buf;
        }
    }

    if (buftype_is_contig && filetype_is_contig) {
        /* convert bufcount and offset to bytes */
        bufsize = datatype_size * count;
        if (file_ptr_type == ADIO_EXPLICIT_OFFSET) {
            off = adio_fh->disp + adio_fh->etype_size * offset;
        } else {        /* ADIO_INDIVIDUAL */

            off = adio_fh->fp_ind;
        }

        /* if atomic mode requested, lock (exclusive) the region, because
         * there could be a concurrent noncontiguous request. Locking doesn't
         * work on some parallel file systems, and on NFS it is done in the
         * ADIO_WriteContig.
         */

        if ((adio_fh->atomicity) && ADIO_Feature(adio_fh, ADIO_LOCKS)) {
            ADIOI_WRITE_LOCK(adio_fh, off, SEEK_SET, bufsize);
        }

        ADIO_WriteContig(adio_fh, xbuf, count, datatype, file_ptr_type, off, status, &error_code);

        if ((adio_fh->atomicity) && ADIO_Feature(adio_fh, ADIO_LOCKS)) {
            ADIOI_UNLOCK(adio_fh, off, SEEK_SET, bufsize);
        }
    } else {
        /* For strided and atomic mode, locking is done in ADIO_WriteStrided */
        ADIO_WriteStrided(adio_fh, xbuf, count, datatype, file_ptr_type,
                          offset, status, &error_code);
    }


    /* --BEGIN ERROR HANDLING-- */
    if (error_code != MPI_SUCCESS)
        goto fn_fail;
    /* --END ERROR HANDLING-- */

    MPIO_GPU_HOST_FREE(host_buf, count, datatype);

  fn_exit:
    if (e32buf != NULL)
        ADIOI_Free(e32buf);
    return error_code;
  fn_fail:
    goto fn_exit;
}

static int MPIOI_File_write_all(MPI_File fh, MPI_Offset offset, int file_ptr_type,
                                const void *buf, MPI_Aint count, MPI_Datatype datatype,
                                MPI_Status * status)
{
    int error_code;
    MPI_Count datatype_size;
    ADIO_File adio_fh;
    void *e32buf = NULL;
    const void *xbuf = NULL;
    void *host_buf = NULL;

    adio_fh = MPIO_File_resolve(fh);

    /* --BEGIN ERROR HANDLING-- */
    MPIO_CHECK_FILE_HANDLE(adio_fh, __func__, error_code);
    MPIO_CHECK_COUNT(adio_fh, count, __func__, error_code);
    MPIO_CHECK_DATATYPE(adio_fh, datatype, __func__, error_code);

    if (file_ptr_type == ADIO_EXPLICIT_OFFSET && offset < 0) {
        error_code = MPIO_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                          __func__, __LINE__, MPI_ERR_ARG, "**iobadoffset", 0);
        goto fn_fail;
    }
    /* --END ERROR HANDLING-- */

    MPI_Type_size_x(datatype, &datatype_size);

    /* --BEGIN ERROR HANDLING-- */
    MPIO_CHECK_INTEGRAL_ETYPE(adio_fh, count, datatype_size, __func__, error_code);
    MPIO_CHECK_WRITABLE(adio_fh, __func__, error_code);
    MPIO_CHECK_NOT_SEQUENTIAL_MODE(adio_fh, __func__, error_code);
    MPIO_CHECK_COUNT_SIZE(adio_fh, count, datatype_size, __func__, error_code);
    /* --END ERROR HANDLING-- */

    xbuf = buf;
    if (adio_fh->is_external32) {
        error_code = MPIU_external32_buffer_setup(buf, count, datatype, &e32buf);
        if (error_code != MPI_SUCCESS)
            goto fn_exit;

        xbuf = e32buf;
    } else {
        MPIO_GPU_HOST_SWAP(host_buf, buf, count, datatype);
        if (host_buf != NULL) {
            xbuf = host_buf;
        }
    }
    ADIO_WriteStridedColl(adio_fh, xbuf, count, datatype, file_ptr_type,
                          offset, status, &error_code);

    /* --BEGIN ERROR HANDLING-- */
    if (error_code != MPI_SUCCESS)
        goto fn_fail;
    /* --END ERROR HANDLING-- */

    MPIO_GPU_HOST_FREE(host_buf, count, datatype);

  fn_exit:
    if (e32buf != NULL)
        ADIOI_Free(e32buf);
    return error_code;
  fn_fail:
    goto fn_exit;
}

static int MPIOI_File_write_all_begin(MPI_File fh, MPI_Offset offset, int file_ptr_type,
                                      const void *buf, MPI_Aint count, MPI_Datatype datatype)
{
    int error_code;
    MPI_Count datatype_size;
    ADIO_File adio_fh;
    void *e32buf = NULL;
    const void *xbuf = NULL;
    void *host_buf = NULL;

    adio_fh = MPIO_File_resolve(fh);

    /* --BEGIN ERROR HANDLING-- */
    MPIO_CHECK_FILE_HANDLE(adio_fh, __func__, error_code);
    MPIO_CHECK_COUNT(adio_fh, count, __func__, error_code);
    MPIO_CHECK_DATATYPE(adio_fh, datatype, __func__, error_code);
    MPIO_CHECK_NOT_SEQUENTIAL_MODE(adio_fh, __func__, error_code);

    if (file_ptr_type == ADIO_EXPLICIT_OFFSET && offset < 0) {
        error_code = MPIO_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                          __func__, __LINE__, MPI_ERR_ARG, "**iobadoffset", 0);
        goto fn_fail;
    }

    if (adio_fh->split_coll_count) {
        error_code = MPIO_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                          __func__, __LINE__, MPI_ERR_IO, "**iosplitcoll", 0);
        goto fn_fail;
    }
    /* --END ERROR HANDLING-- */

    adio_fh->split_coll_count = 1;

    MPI_Type_size_x(datatype, &datatype_size);
    /* --BEGIN ERROR HANDLING-- */
    MPIO_CHECK_INTEGRAL_ETYPE(adio_fh, count, datatype_size, __func__, error_code);
    MPIO_CHECK_COUNT_SIZE(adio_fh, count, datatype_size, __func__, error_code);
    /* --END ERROR HANDLING-- */


    xbuf = buf;
    if (adio_fh->is_external32) {
        error_code = MPIU_external32_buffer_setup(buf, count, datatype, &e32buf);
        if (error_code != MPI_SUCCESS)
            goto fn_exit;

        xbuf = e32buf;
    } else {
        MPIO_GPU_HOST_SWAP(host_buf, buf, count, datatype);
        if (host_buf != NULL) {
            xbuf = host_buf;
        }
    }

    adio_fh->split_datatype = datatype;
    ADIO_WriteStridedColl(adio_fh, xbuf, count, datatype, file_ptr_type,
                          offset, &adio_fh->split_status, &error_code);

    /* --BEGIN ERROR HANDLING-- */
    if (error_code != MPI_SUCCESS)
        goto fn_fail;
    /* --END ERROR HANDLING-- */

    MPIO_GPU_HOST_FREE(host_buf, count, datatype);

  fn_exit:
    if (e32buf != NULL)
        ADIOI_Free(e32buf);
    return error_code;
  fn_fail:
    goto fn_exit;
}

static int MPIOI_File_write_all_end(MPI_File fh, const void *buf, MPI_Status * status)
{
    int error_code;
    ADIO_File adio_fh;

    MPL_UNREFERENCED_ARG(buf);

    adio_fh = MPIO_File_resolve(fh);

    /* --BEGIN ERROR HANDLING-- */
    MPIO_CHECK_FILE_HANDLE(adio_fh, __func__, error_code);

    if (!(adio_fh->split_coll_count)) {
        error_code = MPIO_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                          __func__, __LINE__, MPI_ERR_IO, "**iosplitcollnone", 0);
        goto fn_fail;
    }
    /* --END ERROR HANDLING-- */

#ifdef HAVE_STATUS_SET_BYTES
    /* FIXME - we should really ensure that the split_datatype remains
     * valid by incrementing the ref count in the write_allb.c routine
     * and decrement it here after setting the bytes */
    if (status != MPI_STATUS_IGNORE)
        *status = adio_fh->split_status;
#endif
    adio_fh->split_coll_count = 0;

    error_code = MPI_SUCCESS;

  fn_exit:
    return error_code;
  fn_fail:
    goto fn_exit;
}

static int MPIOI_File_write_ordered(MPI_File fh, const void *buf, MPI_Aint count,
                                    MPI_Datatype datatype, MPI_Status * status)
{
    int error_code, nprocs, myrank;
    ADIO_Offset incr;
    MPI_Count datatype_size;
    int source, dest;
    ADIO_Offset shared_fp;
    ADIO_File adio_fh;
    void *e32buf = NULL;
    const void *xbuf;
    void *host_buf = NULL;

    adio_fh = MPIO_File_resolve(fh);

    /* --BEGIN ERROR HANDLING-- */
    MPIO_CHECK_FILE_HANDLE(adio_fh, __func__, error_code);
    MPIO_CHECK_COUNT(adio_fh, count, __func__, error_code);
    MPIO_CHECK_DATATYPE(adio_fh, datatype, __func__, error_code);
    /* --END ERROR HANDLING-- */

    MPI_Type_size_x(datatype, &datatype_size);

    /* --BEGIN ERROR HANDLING-- */
    MPIO_CHECK_INTEGRAL_ETYPE(adio_fh, count, datatype_size, __func__, error_code);
    MPIO_CHECK_FS_SUPPORTS_SHARED(adio_fh, __func__, error_code);
    MPIO_CHECK_COUNT_SIZE(adio_fh, count, datatype_size, __func__, error_code);
    /* --END ERROR HANDLING-- */

    ADIOI_TEST_DEFERRED(adio_fh, __func__, &error_code);

    MPI_Comm_size(adio_fh->comm, &nprocs);
    MPI_Comm_rank(adio_fh->comm, &myrank);

    incr = (count * datatype_size) / adio_fh->etype_size;
    /* Use a message as a 'token' to order the operations */
    source = myrank - 1;
    dest = myrank + 1;
    if (source < 0)
        source = MPI_PROC_NULL;
    if (dest >= nprocs)
        dest = MPI_PROC_NULL;
    MPI_Recv(NULL, 0, MPI_BYTE, source, 0, adio_fh->comm, MPI_STATUS_IGNORE);

    ADIO_Get_shared_fp(adio_fh, incr, &shared_fp, &error_code);

    /* --BEGIN ERROR HANDLING-- */
    if (error_code != MPI_SUCCESS) {
        error_code = MPIO_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL,
                                          __func__, __LINE__, MPI_ERR_INTERN, "**iosharedfailed",
                                          0);
        goto fn_fail;
    }
    /* --END ERROR HANDLING-- */

    MPI_Send(NULL, 0, MPI_BYTE, dest, 0, adio_fh->comm);

    xbuf = buf;
    if (adio_fh->is_external32) {
        error_code = MPIU_external32_buffer_setup(buf, count, datatype, &e32buf);
        if (error_code != MPI_SUCCESS)
            goto fn_exit;

        xbuf = e32buf;
    } else {
        MPIO_GPU_HOST_SWAP(host_buf, buf, count, datatype);
        if (host_buf != NULL) {
            xbuf = host_buf;
        }
    }

    ADIO_WriteStridedColl(adio_fh, xbuf, count, datatype, ADIO_EXPLICIT_OFFSET,
                          shared_fp, status, &error_code);

    /* --BEGIN ERROR HANDLING-- */
    if (error_code != MPI_SUCCESS)
        goto fn_fail;
    /* --END ERROR HANDLING-- */

    MPIO_GPU_HOST_FREE(host_buf, count, datatype);

  fn_exit:
    if (e32buf != NULL)
        ADIOI_Free(e32buf);
    /* FIXME: Check for error code from WriteStridedColl? */
    return error_code;
  fn_fail:
    goto fn_exit;
}

static int MPIOI_File_write_ordered_begin(MPI_File fh, const void *buf, MPI_Aint count,
                                          MPI_Datatype datatype)
{
    int error_code, nprocs, myrank;
    ADIO_Offset incr;
    MPI_Count datatype_size;
    int source, dest;
    ADIO_Offset shared_fp;
    ADIO_File adio_fh;
    void *e32buf = NULL;
    const void *xbuf = NULL;
    void *host_buf = NULL;

    adio_fh = MPIO_File_resolve(fh);

    /* --BEGIN ERROR HANDLING-- */
    MPIO_CHECK_FILE_HANDLE(adio_fh, __func__, error_code);
    MPIO_CHECK_COUNT(adio_fh, count, __func__, error_code);
    MPIO_CHECK_DATATYPE(adio_fh, datatype, __func__, error_code);

    if (adio_fh->split_coll_count) {
        error_code = MPIO_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                          __func__, __LINE__, MPI_ERR_IO, "**iosplitcoll", 0);
        goto fn_fail;
    }
    /* --END ERROR HANDLING-- */

    adio_fh->split_coll_count = 1;

    MPI_Type_size_x(datatype, &datatype_size);
    /* --BEGIN ERROR HANDLING-- */
    MPIO_CHECK_INTEGRAL_ETYPE(adio_fh, count, datatype_size, __func__, error_code);
    MPIO_CHECK_FS_SUPPORTS_SHARED(adio_fh, __func__, error_code);
    MPIO_CHECK_COUNT_SIZE(adio_fh, count, datatype_size, __func__, error_code);
    /* --END ERROR HANDLING-- */

    ADIOI_TEST_DEFERRED(adio_fh, __func__, &error_code);

    MPI_Comm_size(adio_fh->comm, &nprocs);
    MPI_Comm_rank(adio_fh->comm, &myrank);

    incr = (count * datatype_size) / adio_fh->etype_size;
    /* Use a message as a 'token' to order the operations */
    source = myrank - 1;
    dest = myrank + 1;
    if (source < 0)
        source = MPI_PROC_NULL;
    if (dest >= nprocs)
        dest = MPI_PROC_NULL;
    MPI_Recv(NULL, 0, MPI_BYTE, source, 0, adio_fh->comm, MPI_STATUS_IGNORE);

    ADIO_Get_shared_fp(adio_fh, incr, &shared_fp, &error_code);
    /* --BEGIN ERROR HANDLING-- */
    if (error_code != MPI_SUCCESS) {
        error_code = MPIO_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL,
                                          __func__, __LINE__, MPI_ERR_INTERN, "**iosharedfailed",
                                          0);
        goto fn_fail;
    }
    /* --END ERROR HANDLING-- */

    MPI_Send(NULL, 0, MPI_BYTE, dest, 0, adio_fh->comm);

    xbuf = buf;
    if (adio_fh->is_external32) {
        error_code = MPIU_external32_buffer_setup(buf, count, datatype, &e32buf);
        if (error_code != MPI_SUCCESS)
            goto fn_exit;

        xbuf = e32buf;
    } else {
        MPIO_GPU_HOST_SWAP(host_buf, buf, count, datatype);
        if (host_buf != NULL) {
            xbuf = host_buf;
        }
    }

    ADIO_WriteStridedColl(adio_fh, xbuf, count, datatype, ADIO_EXPLICIT_OFFSET,
                          shared_fp, &adio_fh->split_status, &error_code);

    /* --BEGIN ERROR HANDLING-- */
    if (error_code != MPI_SUCCESS)
        goto fn_fail;
    /* --END ERROR HANDLING-- */

    MPIO_GPU_HOST_FREE(host_buf, count, datatype);

  fn_exit:
    /* FIXME: Check for error code from WriteStridedColl? */
    return error_code;
  fn_fail:
    goto fn_exit;
}

static int MPIOI_File_write_ordered_end(MPI_File fh, const void *buf, MPI_Status * status)
{
    int error_code = MPI_SUCCESS;
    ADIO_File adio_fh;

    MPL_UNREFERENCED_ARG(buf);

    adio_fh = MPIO_File_resolve(fh);

    /* --BEGIN ERROR HANDLING-- */
    MPIO_CHECK_FILE_HANDLE(adio_fh, __func__, error_code);

    if (!(adio_fh->split_coll_count)) {
        error_code = MPIO_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                          __func__, __LINE__, MPI_ERR_IO, "**iosplitcollnone", 0);
        goto fn_fail;
    }
    /* --END ERROR HANDLING-- */

#ifdef HAVE_STATUS_SET_BYTES
    if (status != MPI_STATUS_IGNORE)
        *status = adio_fh->split_status;
#endif
    adio_fh->split_coll_count = 0;


  fn_exit:
    return error_code;
  fn_fail:
    goto fn_exit;
}

static int MPIOI_File_write_shared(MPI_File fh, const void *buf, MPI_Aint count,
                                   MPI_Datatype datatype, MPI_Status * status)
{
    int error_code, buftype_is_contig, filetype_is_contig;
    ADIO_Offset bufsize;
    MPI_Count datatype_size, incr;
    ADIO_Offset off, shared_fp;
    ADIO_File adio_fh;
    void *e32buf = NULL;
    const void *xbuf = NULL;
    void *host_buf = NULL;

    adio_fh = MPIO_File_resolve(fh);

    /* --BEGIN ERROR HANDLING-- */
    MPIO_CHECK_FILE_HANDLE(adio_fh, __func__, error_code);
    MPIO_CHECK_COUNT(adio_fh, count, __func__, error_code);
    MPIO_CHECK_DATATYPE(adio_fh, datatype, __func__, error_code);
    /* --END ERROR HANDLING-- */

    MPI_Type_size_x(datatype, &datatype_size);

    /* --BEGIN ERROR HANDLING-- */
    MPIO_CHECK_COUNT_SIZE(adio_fh, count, datatype_size, __func__, error_code);
    /* --END ERROR HANDLING-- */

    if (count * datatype_size == 0) {
#ifdef HAVE_STATUS_SET_BYTES
        MPIR_Status_set_bytes(status, datatype, 0);
#endif
        error_code = MPI_SUCCESS;
        goto fn_exit;
    }

    /* --BEGIN ERROR HANDLING-- */
    MPIO_CHECK_INTEGRAL_ETYPE(adio_fh, count, datatype_size, __func__, error_code);
    MPIO_CHECK_FS_SUPPORTS_SHARED(adio_fh, __func__, error_code);
    /* --END ERROR HANDLING-- */

    ADIOI_Datatype_iscontig(datatype, &buftype_is_contig);
    ADIOI_Datatype_iscontig(adio_fh->filetype, &filetype_is_contig);

    ADIOI_TEST_DEFERRED(adio_fh, __func__, &error_code);

    incr = (count * datatype_size) / adio_fh->etype_size;

    ADIO_Get_shared_fp(adio_fh, incr, &shared_fp, &error_code);
    /* --BEGIN ERROR HANDLING-- */
    if (error_code != MPI_SUCCESS) {
        error_code = MPIO_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL,
                                          __func__, __LINE__, MPI_ERR_INTERN, "**iosharedfailed",
                                          0);
        goto fn_fail;
    }
    /* --END ERROR HANDLING-- */

    xbuf = buf;
    if (adio_fh->is_external32) {
        error_code = MPIU_external32_buffer_setup(buf, count, datatype, &e32buf);
        if (error_code != MPI_SUCCESS)
            goto fn_exit;

        xbuf = e32buf;
    } else {
        MPIO_GPU_HOST_SWAP(host_buf, buf, count, datatype);
        if (host_buf != NULL) {
            xbuf = host_buf;
        }
    }

    if (buftype_is_contig && filetype_is_contig) {
        /* convert bufocunt and shared_fp to bytes */
        bufsize = datatype_size * count;
        off = adio_fh->disp + adio_fh->etype_size * shared_fp;

        /* if atomic mode requested, lock (exclusive) the region, because there
         * could be a concurrent noncontiguous request. On NFS, locking is
         * done in the ADIO_WriteContig. */

        if ((adio_fh->atomicity) && (adio_fh->file_system != ADIO_NFS))
            ADIOI_WRITE_LOCK(adio_fh, off, SEEK_SET, bufsize);

        ADIO_WriteContig(adio_fh, xbuf, count, datatype, ADIO_EXPLICIT_OFFSET,
                         off, status, &error_code);

        if ((adio_fh->atomicity) && (adio_fh->file_system != ADIO_NFS))
            ADIOI_UNLOCK(adio_fh, off, SEEK_SET, bufsize);
    } else {
        ADIO_WriteStrided(adio_fh, xbuf, count, datatype, ADIO_EXPLICIT_OFFSET,
                          shared_fp, status, &error_code);
        /* For strided and atomic mode, locking is done in ADIO_WriteStrided */
    }

    /* --BEGIN ERROR HANDLING-- */
    if (error_code != MPI_SUCCESS)
        goto fn_fail;
    /* --END ERROR HANDLING-- */

    MPIO_GPU_HOST_FREE(host_buf, count, datatype);

  fn_exit:
    if (e32buf != NULL)
        ADIOI_Free(e32buf);
    return error_code;
  fn_fail:
    goto fn_exit;
}

static int MPIOI_Register_datarep(const char *datarep,
                                  MPIOI_VOID_FN * read_conversion_fn,
                                  MPIOI_VOID_FN * write_conversion_fn,
                                  MPI_Datarep_extent_function * dtype_file_extent_fn,
                                  void *extra_state, int is_large)
{
    int error_code;
    ADIOI_Datarep *adio_datarep;
    static char myname[] = "MPI_REGISTER_DATAREP";

    /* --BEGIN ERROR HANDLING-- */
    /* check datarep name (use strlen instead of strnlen because
     * strnlen is not portable) */
    if (datarep == NULL || strlen(datarep) < 1 || strlen(datarep) > MPI_MAX_DATAREP_STRING) {
        error_code = MPIO_Err_create_code(MPI_SUCCESS,
                                          MPIR_ERR_RECOVERABLE,
                                          myname, __LINE__, MPI_ERR_ARG, "**datarepname", 0);
        error_code = MPIO_Err_return_file(MPI_FILE_NULL, error_code);
        goto fn_exit;
    }
    /* --END ERROR HANDLING-- */

    MPIR_MPIOInit(&error_code);
    if (error_code != MPI_SUCCESS)
        goto fn_exit;

    /* --BEGIN ERROR HANDLING-- */
    /* check datarep isn't already registered */
    for (adio_datarep = ADIOI_Datarep_head; adio_datarep; adio_datarep = adio_datarep->next) {
        if (!strncmp(datarep, adio_datarep->name, MPI_MAX_DATAREP_STRING)) {
            error_code = MPIO_Err_create_code(MPI_SUCCESS,
                                              MPIR_ERR_RECOVERABLE,
                                              myname, __LINE__,
                                              MPI_ERR_DUP_DATAREP,
                                              "**datarepused", "**datarepused %s", datarep);
            error_code = MPIO_Err_return_file(MPI_FILE_NULL, error_code);
            goto fn_exit;
        }
    }

    /* Check Non-NULL Read and Write conversion function pointer */
    /* Read and Write conversions are currently not supported.   */
    if ((read_conversion_fn != NULL) || (write_conversion_fn != NULL)) {
        error_code = MPIO_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                          myname, __LINE__,
                                          MPI_ERR_CONVERSION, "**drconvnotsupported", 0);

        error_code = MPIO_Err_return_file(MPI_FILE_NULL, error_code);
        goto fn_exit;
    }

    /* check extent function pointer */
    if (dtype_file_extent_fn == NULL) {
        error_code = MPIO_Err_create_code(MPI_SUCCESS,
                                          MPIR_ERR_RECOVERABLE,
                                          myname, __LINE__, MPI_ERR_ARG, "**datarepextent", 0);
        error_code = MPIO_Err_return_file(MPI_FILE_NULL, error_code);
        goto fn_exit;
    }
    /* --END ERROR HANDLING-- */

    adio_datarep = ADIOI_Malloc(sizeof(ADIOI_Datarep));
    adio_datarep->name = ADIOI_Strdup(datarep);
    adio_datarep->state = extra_state;
    adio_datarep->is_large = is_large;
    if (is_large) {
        adio_datarep->u.large.read_conv_fn =
            (MPI_Datarep_conversion_function_c *) read_conversion_fn;
        adio_datarep->u.large.write_conv_fn =
            (MPI_Datarep_conversion_function_c *) write_conversion_fn;
    } else {
        adio_datarep->u.small.read_conv_fn = (MPI_Datarep_conversion_function *) read_conversion_fn;
        adio_datarep->u.small.write_conv_fn =
            (MPI_Datarep_conversion_function *) write_conversion_fn;
    }
    adio_datarep->extent_fn = dtype_file_extent_fn;
    adio_datarep->next = ADIOI_Datarep_head;

    ADIOI_Datarep_head = adio_datarep;

    error_code = MPI_SUCCESS;

  fn_exit:
    return error_code;
}

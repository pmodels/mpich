/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */


/* header file for MPI-IO implementation. not intended to be
   user-visible */

#ifndef MPIOIMPL_H_INCLUDED
#define MPIOIMPL_H_INCLUDED

#include "adio.h"

#ifdef ROMIO_INSIDE_MPICH
#include "mpir_ext.h"

#define ROMIO_THREAD_CS_ENTER() MPIR_Ext_cs_enter()
#define ROMIO_THREAD_CS_EXIT() MPIR_Ext_cs_exit()
#define ROMIO_THREAD_CS_YIELD() MPL_thread_yield()

/* committed datatype checking support in ROMIO */
#define MPIO_DATATYPE_ISCOMMITTED(dtype_, err_)        \
    do {                                               \
        err_ =  MPIR_Ext_datatype_iscommitted(dtype_); \
    } while (0)

#define MPIO_GPU_HOST_ALLOC(host_buf, buf, count, datatype)             \
    do {                                                                \
        host_buf = MPIR_Ext_gpu_host_alloc(buf, count, datatype);       \
    } while (0)
#define MPIO_GPU_HOST_FREE(host_buf, count, datatype)                   \
    do {                                                                \
        if (host_buf != NULL) {                                         \
            MPIR_Ext_gpu_host_free(host_buf, count, datatype);          \
        }                                                               \
    } while (0)
#define MPIO_GPU_HOST_SWAP(host_buf, buf, count, datatype)              \
    do {                                                                \
        host_buf = MPIR_Ext_gpu_host_swap(buf, count, datatype);        \
    } while (0)
#define MPIO_GPU_SWAP_BACK(host_buf, buf, count, datatype)              \
    do {                                                                \
        if (host_buf != NULL) {                                         \
            MPIR_Ext_gpu_swap_back(host_buf, buf, count, datatype);     \
        }                                                               \
    } while (0)

#else /* not ROMIO_INSIDE_MPICH */
/* Any MPI implementation that wishes to follow the thread-safety and
   error reporting features provided by MPICH must implement these
   four functions.  Defining these as empty should not change the behavior
   of correct programs */
#define ROMIO_THREAD_CS_ENTER()
#define ROMIO_THREAD_CS_EXIT()
#define ROMIO_THREAD_CS_YIELD()
#define MPIO_DATATYPE_ISCOMMITTED(dtype_, err_) do {} while (0)
/* functions for GPU-awareness */
#define MPIO_GPU_HOST_ALLOC(...) do {} while (0)
#define MPIO_GPU_HOST_FREE(...) do {} while (0)
#define MPIO_GPU_HOST_SWAP(...) do {} while (0)
#define MPIO_GPU_SWAP_BACK(...) do {} while (0)
#endif /* ROMIO_INSIDE_MPICH */

/* info is a linked list of these structures */
struct MPIR_Info {
    int cookie;
    char *key, *value;
    struct MPIR_Info *next;
};

#define MPIR_INFO_COOKIE 5835657

MPI_Delete_function ADIOI_End_call;

/* common initialization routine */
void MPIR_MPIOInit(int *error_code);

#if MPI_VERSION >= 3
#define ROMIO_CONST const
#else
#define ROMIO_CONST
#endif

#include "mpiu_external32.h"

#ifdef MPIO_BUILD_PROFILING
#include "mpioprof.h"
#endif /* MPIO_BUILD_PROFILING */

int MPIR_File_close_impl(MPI_File * fh);
int MPIR_File_delete_impl(const char *filename, MPI_Info info);
int MPIR_File_get_amode_impl(MPI_File fh, int *amode);
int MPIR_File_get_atomicity_impl(MPI_File fh, int *flag);
int MPIR_File_get_byte_offset_impl(MPI_File fh, MPI_Offset offset, MPI_Offset * disp);
int MPIR_File_get_group_impl(MPI_File fh, MPI_Group * group);
int MPIR_File_get_info_impl(MPI_File fh, MPI_Info * info_used);
int MPIR_File_get_position_impl(MPI_File fh, MPI_Offset * offset);
int MPIR_File_get_position_shared_impl(MPI_File fh, MPI_Offset * offset);
int MPIR_File_get_size_impl(MPI_File fh, MPI_Offset * size);
int MPIR_File_get_type_extent_impl(MPI_File fh, MPI_Datatype datatype, MPI_Aint * extent);
int MPIR_File_get_view_impl(MPI_File fh, MPI_Offset * disp, MPI_Datatype * etype,
                            MPI_Datatype * filetype, char *datarep);
int MPIR_File_iread_impl(MPI_File fh, void *buf, MPI_Aint count, MPI_Datatype datatype,
                         MPI_Request * request);
int MPIR_File_iread_all_impl(MPI_File fh, void *buf, MPI_Aint count, MPI_Datatype datatype,
                             MPI_Request * request);
int MPIR_File_iread_at_impl(MPI_File fh, MPI_Offset offset, void *buf, MPI_Aint count,
                            MPI_Datatype datatype, MPI_Request * request);
int MPIR_File_iread_at_all_impl(MPI_File fh, MPI_Offset offset, void *buf, MPI_Aint count,
                                MPI_Datatype datatype, MPI_Request * request);
int MPIR_File_iread_shared_impl(MPI_File fh, void *buf, MPI_Aint count, MPI_Datatype datatype,
                                MPI_Request * request);
int MPIR_File_iwrite_impl(MPI_File fh, const void *buf, MPI_Aint count, MPI_Datatype datatype,
                          MPI_Request * request);
int MPIR_File_iwrite_all_impl(MPI_File fh, const void *buf, MPI_Aint count, MPI_Datatype datatype,
                              MPI_Request * request);
int MPIR_File_iwrite_at_impl(MPI_File fh, MPI_Offset offset, const void *buf, MPI_Aint count,
                             MPI_Datatype datatype, MPI_Request * request);
int MPIR_File_iwrite_at_all_impl(MPI_File fh, MPI_Offset offset, const void *buf, MPI_Aint count,
                                 MPI_Datatype datatype, MPI_Request * request);
int MPIR_File_iwrite_shared_impl(MPI_File fh, const void *buf, MPI_Aint count,
                                 MPI_Datatype datatype, MPI_Request * request);
int MPIR_File_open_impl(MPI_Comm comm, const char *filename, int amode, MPI_Info info,
                        MPI_File * fh);
int MPIR_File_preallocate_impl(MPI_File fh, MPI_Offset size);
int MPIR_File_read_impl(MPI_File fh, void *buf, MPI_Aint count, MPI_Datatype datatype,
                        MPI_Status * status);
int MPIR_File_read_all_impl(MPI_File fh, void *buf, MPI_Aint count, MPI_Datatype datatype,
                            MPI_Status * status);
int MPIR_File_read_all_begin_impl(MPI_File fh, void *buf, MPI_Aint count, MPI_Datatype datatype);
int MPIR_File_read_all_end_impl(MPI_File fh, void *buf, MPI_Status * status);
int MPIR_File_read_at_impl(MPI_File fh, MPI_Offset offset, void *buf, MPI_Aint count,
                           MPI_Datatype datatype, MPI_Status * status);
int MPIR_File_read_at_all_impl(MPI_File fh, MPI_Offset offset, void *buf, MPI_Aint count,
                               MPI_Datatype datatype, MPI_Status * status);
int MPIR_File_read_at_all_begin_impl(MPI_File fh, MPI_Offset offset, void *buf, MPI_Aint count,
                                     MPI_Datatype datatype);
int MPIR_File_read_at_all_end_impl(MPI_File fh, void *buf, MPI_Status * status);
int MPIR_File_read_ordered_impl(MPI_File fh, void *buf, MPI_Aint count, MPI_Datatype datatype,
                                MPI_Status * status);
int MPIR_File_read_ordered_begin_impl(MPI_File fh, void *buf, MPI_Aint count,
                                      MPI_Datatype datatype);
int MPIR_File_read_ordered_end_impl(MPI_File fh, void *buf, MPI_Status * status);
int MPIR_File_read_shared_impl(MPI_File fh, void *buf, MPI_Aint count, MPI_Datatype datatype,
                               MPI_Status * status);
int MPIR_File_seek_impl(MPI_File fh, MPI_Offset offset, int whence);
int MPIR_File_seek_shared_impl(MPI_File fh, MPI_Offset offset, int whence);
int MPIR_File_set_atomicity_impl(MPI_File fh, int flag);
int MPIR_File_set_info_impl(MPI_File fh, MPI_Info info);
int MPIR_File_set_size_impl(MPI_File fh, MPI_Offset size);
int MPIR_File_set_view_impl(MPI_File fh, MPI_Offset disp, MPI_Datatype etype, MPI_Datatype filetype,
                            const char *datarep, MPI_Info info);
int MPIR_File_sync_impl(MPI_File fh);
int MPIR_File_write_impl(MPI_File fh, const void *buf, MPI_Aint count, MPI_Datatype datatype,
                         MPI_Status * status);
int MPIR_File_write_all_impl(MPI_File fh, const void *buf, MPI_Aint count, MPI_Datatype datatype,
                             MPI_Status * status);
int MPIR_File_write_all_begin_impl(MPI_File fh, const void *buf, MPI_Aint count,
                                   MPI_Datatype datatype);
int MPIR_File_write_all_end_impl(MPI_File fh, const void *buf, MPI_Status * status);
int MPIR_File_write_at_impl(MPI_File fh, MPI_Offset offset, const void *buf, MPI_Aint count,
                            MPI_Datatype datatype, MPI_Status * status);
int MPIR_File_write_at_all_impl(MPI_File fh, MPI_Offset offset, const void *buf, MPI_Aint count,
                                MPI_Datatype datatype, MPI_Status * status);
int MPIR_File_write_at_all_begin_impl(MPI_File fh, MPI_Offset offset, const void *buf,
                                      MPI_Aint count, MPI_Datatype datatype);
int MPIR_File_write_at_all_end_impl(MPI_File fh, const void *buf, MPI_Status * status);
int MPIR_File_write_ordered_impl(MPI_File fh, const void *buf, MPI_Aint count,
                                 MPI_Datatype datatype, MPI_Status * status);
int MPIR_File_write_ordered_begin_impl(MPI_File fh, const void *buf, MPI_Aint count,
                                       MPI_Datatype datatype);
int MPIR_File_write_ordered_end_impl(MPI_File fh, const void *buf, MPI_Status * status);
int MPIR_File_write_shared_impl(MPI_File fh, const void *buf, MPI_Aint count, MPI_Datatype datatype,
                                MPI_Status * status);
int MPIR_Register_datarep_impl(const char *datarep,
                               MPI_Datarep_conversion_function * read_conversion_fn,
                               MPI_Datarep_conversion_function * write_conversion_fn,
                               MPI_Datarep_extent_function * dtype_file_extent_fn,
                               void *extra_state);
int MPIR_Register_datarep_large_impl(const char *datarep,
                                     MPI_Datarep_conversion_function_c * read_conversion_fn,
                                     MPI_Datarep_conversion_function_c * write_conversion_fn,
                                     MPI_Datarep_extent_function * dtype_file_extent_fn,
                                     void *extra_state);

#endif /* MPIOIMPL_H_INCLUDED */

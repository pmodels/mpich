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


#endif /* MPIOIMPL_H_INCLUDED */

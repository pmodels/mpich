/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPID_DATALOOP_H
#define MPID_DATALOOP_H

#include <mpi.h>

/* Note: this is where you define the prefix that will be prepended on
 * all externally visible generic dataloop and segment functions.
 */
#define PREPEND_PREFIX(fn) MPID_ ## fn

/* These following dataloop-specific types will be used throughout the DLOOP
 * instance:
 */
#define DLOOP_Offset     MPI_Aint
#define DLOOP_Count      MPI_Aint
#define DLOOP_Handle     MPI_Datatype
#define DLOOP_Type       MPI_Datatype
#define DLOOP_Buffer     void *
#define DLOOP_VECTOR     MPL_IOV
#define DLOOP_VECTOR_LEN MPL_IOV_LEN
#define DLOOP_VECTOR_BUF MPL_IOV_BUF
#define DLOOP_Size       MPI_Aint

/* The following accessor functions must also be defined:
 *
 * DLOOP_Handle_extent()
 * DLOOP_Handle_size()
 * DLOOP_Handle_loopptr()
 * DLOOP_Handle_loopdepth()
 * DLOOP_Handle_hasloop()
 *
 */

/* USE THE NOTATION THAT BILL USED IN MPIIMPL.H AND MAKE THESE MACROS */

/* NOTE: put get size into mpiimpl.h; the others go here until such time
 * as we see that we need them elsewhere.
 */
#define DLOOP_Handle_get_loopdepth_macro(handle_,depth_,flag_) \
    MPID_Datatype_get_loopdepth_macro(handle_,depth_,flag_)

#define DLOOP_Handle_get_loopsize_macro(handle_,size_,flag_) \
    MPID_Datatype_get_loopsize_macro(handle_,size_,flag_)

#define DLOOP_Handle_set_loopptr_macro(handle_,lptr_,flag_) \
    MPID_Datatype_set_loopptr_macro(handle_,lptr_,flag_)

#define DLOOP_Handle_set_loopdepth_macro(handle_,depth_,flag_) \
    MPID_Datatype_set_loopdepth_macro(handle_,depth_,flag_)

#define DLOOP_Handle_set_loopsize_macro(handle_,size_,flag_) \
    MPID_Datatype_set_loopsize_macro(handle_,size_,flag_)

#define DLOOP_Handle_get_loopptr_macro(handle_,lptr_,flag_) \
    MPID_Datatype_get_loopptr_macro(handle_,lptr_,flag_)

#define DLOOP_Handle_get_size_macro(handle_,size_) \
    MPID_Datatype_get_size_macro(handle_,size_)

#define DLOOP_Handle_get_basic_type_macro(handle_,basic_type_) \
    MPID_Datatype_get_basic_type(handle_, basic_type_)

#define DLOOP_Handle_get_extent_macro(handle_,extent_) \
    MPID_Datatype_get_extent_macro(handle_,extent_)

#define DLOOP_Handle_hasloop_macro(handle_)                           \
    ((HANDLE_GET_KIND(handle_) == HANDLE_KIND_BUILTIN) ? 0 : 1)

#define DLOOP_Ensure_Offset_fits_in_pointer(value_) \
    MPIU_Ensure_Aint_fits_in_pointer(value_)

/* allocate and free functions must also be defined. */
#define DLOOP_Malloc MPIU_Malloc
#define DLOOP_Free   MPIU_Free

/* debugging output function */
#define DLOOP_dbg_printf MPIU_dbg_printf

/* assert function */
#define DLOOP_Assert MPIU_Assert

/* memory copy function */
#define DLOOP_Memcpy MPIU_Memcpy

/* casting macros */
#define DLOOP_OFFSET_CAST_TO_VOID_PTR MPIU_AINT_CAST_TO_VOID_PTR
#define DLOOP_VOID_PTR_CAST_TO_OFFSET MPIU_VOID_PTR_CAST_TO_MPI_AINT
#define DLOOP_PTR_DISP_CAST_TO_OFFSET MPIU_PTR_DISP_CAST_TO_MPI_AINT

/* printing macros */
#define DLOOP_OFFSET_FMT_DEC_SPEC MPI_AINT_FMT_DEC_SPEC
#define DLOOP_OFFSET_FMT_HEX_SPEC MPI_AINT_FMT_HEX_SPEC

/* Include dataloop_parts.h at the end to get the rest of the prototypes
 * and defines, in terms of the prefixes and types above.
 */
#include "dataloop/dataloop_parts.h"
#include "dataloop/dataloop_create.h"

/* These values are defined by DLOOP code.
 *
 * Note: DLOOP_DATALOOP_ALL_BYTES is used only when the device
 * defines MPID_NEEDS_DLOOP_ALL_BYTES.
 */
#define MPID_DATALOOP_HETEROGENEOUS DLOOP_DATALOOP_HETEROGENEOUS
#define MPID_DATALOOP_HOMOGENEOUS   DLOOP_DATALOOP_HOMOGENEOUS
#define MPID_DATALOOP_ALL_BYTES     DLOOP_DATALOOP_ALL_BYTES

#include <mpiimpl.h>

#endif

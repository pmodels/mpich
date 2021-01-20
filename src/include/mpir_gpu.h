/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIR_GPU_H_INCLUDED
#define MPIR_GPU_H_INCLUDED

#include "mpir_err.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

categories:
    - name        : GPU
      description : GPU related cvars

cvars:
    - name        : MPIR_CVAR_ENABLE_GPU
      category    : GPU
      type        : int
      default     : 1
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Control MPICH GPU support. If set to 0, all GPU support is disabled
        and we do not query the buffer type internally because we assume
        no GPU buffer is use.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

extern int MPIR_CVAR_ENABLE_GPU;

#undef ENABLE_GPU

#ifdef MPL_HAVE_GPU
#define ENABLE_GPU MPIR_CVAR_ENABLE_GPU
#else
#define ENABLE_GPU FALSE
#endif

MPL_STATIC_INLINE_PREFIX int MPIR_GPU_query_pointer_attr(const void *ptr, MPL_pointer_attr_t * attr)
{
    int mpi_errno = MPI_SUCCESS, mpl_err = MPL_SUCCESS;

    /* Skip query if GPU support is disabled by CVAR. Because we assume
     * no GPU buffer is used. If the user disables GPU at configure time
     * (e.g., --without-cuda), then MPL fallback will handle the query. */
    if (ENABLE_GPU) {
        mpl_err = MPL_gpu_query_pointer_attr(ptr, attr);
        MPIR_ERR_CHKANDJUMP(mpl_err != MPL_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**gpu_query_ptr");
    } else {
        attr->type = MPL_GPU_POINTER_UNREGISTERED_HOST;
        attr->device = MPL_GPU_DEVICE_INVALID;
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX bool MPIR_GPU_query_pointer_is_dev(const void *ptr)
{
    if (ENABLE_GPU && ptr != NULL) {
        MPL_pointer_attr_t attr;
        MPL_gpu_query_pointer_attr(ptr, &attr);

        return attr.type == MPL_GPU_POINTER_DEV;
    }

    return false;
}

#endif /* MPIR_GPU_H_INCLUDED */

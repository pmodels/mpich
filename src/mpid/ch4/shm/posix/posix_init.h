/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2017 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef POSIX_INIT_H_INCLUDED
#define POSIX_INIT_H_INCLUDED

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_CH4_SHM_POSIX_EAGER
      category    : CH4
      type        : string
      default     : ""
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        If non-empty, this cvar specifies which shm posix eager module to use

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

#include "posix_types.h"
#include "ch4_types.h"

#include "posix_eager.h"

#undef FUNCNAME
#define FUNCNAME MPIDI_choose_posix_eager
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_choose_posix_eager(void)
{
    int mpi_errno = MPI_SUCCESS;
    int i;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4_CHOOSE_POSIX_EAGER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4_CHOOSE_POSIX_EAGER);

    MPIR_Assert(MPIR_CVAR_CH4_SHM_POSIX_EAGER != NULL);

    if (strcmp(MPIR_CVAR_CH4_SHM_POSIX_EAGER, "") == 0) {
        /* posix_eager not specified, using the default */
        MPIDI_POSIX_eager_func = MPIDI_POSIX_eager_funcs[0];
        goto fn_exit;
    }

    for (i = 0; i < MPIDI_num_posix_eager_fabrics; ++i) {
        /* use MPL variant of strncasecmp if we get one */
        if (!strncasecmp
            (MPIR_CVAR_CH4_SHM_POSIX_EAGER, MPIDI_POSIX_eager_strings[i],
             MPIDI_MAX_POSIX_EAGER_STRING_LEN)) {
            MPIDI_POSIX_eager_func = MPIDI_POSIX_eager_funcs[i];
            goto fn_exit;
        }
    }

    MPIR_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**ch4|invalid_shm_posix_eager",
                         "**ch4|invalid_shm_posix_eager %s", MPIR_CVAR_CH4_SHM_POSIX_EAGER);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4_CHOOSE_POSIX_EAGER);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_mpi_init_hook
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_POSIX_mpi_init_hook(int rank, int size, int *n_vnis_provided, int *tag_ub)
{
    int mpi_errno = MPI_SUCCESS;
    int i;

#ifdef MPL_USE_DBG_LOGGING
    MPIDI_CH4_SHM_POSIX_GENERAL = MPL_dbg_class_alloc("SHM_POSIX", "shm_posix");
#endif /* MPL_USE_DBG_LOGGING */

    MPIDI_POSIX_global.am_buf_pool =
        MPIDI_CH4U_create_buf_pool(MPIDI_POSIX_BUF_POOL_NUM, MPIDI_POSIX_BUF_POOL_SIZE);

    MPIR_CHKPMEM_DECL(3);

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_INIT_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_INIT_HOOK);

    *n_vnis_provided = 1;

    MPIDI_POSIX_global.postponed_queue = NULL;

    MPIR_CHKPMEM_MALLOC(MPIDI_POSIX_global.active_rreq,
                        MPIR_Request **,
                        size * sizeof(MPIR_Request *), mpi_errno, "active recv req", MPL_MEM_SHM);

    for (i = 0; i < size; i++) {
        MPIDI_POSIX_global.active_rreq[i] = NULL;
    }

    MPIDI_choose_posix_eager();

    mpi_errno = MPIDI_POSIX_eager_init(rank, size);

    /* There is no restriction on the tag_ub from the posix shmod side */
    *tag_ub = MPIR_TAG_USABLE_BITS;

    MPIR_CHKPMEM_COMMIT();

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_INIT_HOOK);
    return mpi_errno;
  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    MPIR_CHKPMEM_REAP();
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

#undef FCNAME
#define FCNAME MPL_QUOTE(MPIDI_POSIX_mpi_finalize_hook)
MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_mpi_finalize_hook(void)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_FINALIZE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_FINALIZE_HOOK);

    mpi_errno = MPIDI_POSIX_eager_finalize();

    MPIDI_CH4R_destroy_buf_pool(MPIDI_POSIX_global.am_buf_pool);

    MPL_free(MPIDI_POSIX_global.active_rreq);

    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_FINALIZE_HOOK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_get_vni_attr(int vni)
{
    MPIR_Assert(0 <= vni && vni < 1);
    return MPIDI_VNI_TX | MPIDI_VNI_RX;
}

MPL_STATIC_INLINE_PREFIX void *MPIDI_POSIX_mpi_alloc_mem(size_t size, MPIR_Info * info_ptr)
{
    MPIR_Assert(0);
    return NULL;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_mpi_free_mem(void *ptr)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_comm_get_lpid(MPIR_Comm * comm_ptr, int idx, int *lpid_ptr,
                                                       MPL_bool is_remote)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_get_node_id(MPIR_Comm * comm, int rank, int *id_p)
{
    *id_p = 0;
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_get_max_node_id(MPIR_Comm * comm, int *max_id_p)
{
    *max_id_p = 0;
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_get_local_upids(MPIR_Comm * comm,
                                                         size_t ** local_upid_size,
                                                         char **local_upids)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_upids_to_lupids(int size,
                                                         size_t * remote_upid_size,
                                                         char *remote_upids, int **remote_lupids)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_create_intercomm_from_lpids(MPIR_Comm * newcomm_ptr,
                                                                     int size, const int lpids[])
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

#endif /* POSIX_INIT_H_INCLUDED */

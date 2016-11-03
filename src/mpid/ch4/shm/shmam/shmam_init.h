/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef SHM_SHMAM_INIT_H_INCLUDED
#define SHM_SHMAM_INIT_H_INCLUDED

#include "shmam_impl.h"
#include "shmam_types.h"
#include "ch4_types.h"

#include "shmam_eager.h"

#undef FUNCNAME
#define FUNCNAME MPIDI_choose_shmam_eager
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_choose_shmam_eager(void)
{

    int mpi_errno = MPI_SUCCESS;
    int i;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4_CHOOSE_SHMAM_EAGER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4_CHOOSE_SHMAM_EAGER);


    MPIR_Assert(MPIR_CVAR_CH4_SHMAM_EAGER != NULL);

    if (strcmp(MPIR_CVAR_CH4_SHMAM_EAGER, "") == 0) {
        /* shmam_eager not specified, using the default */
        MPIDI_SHMAM_eager_func = MPIDI_SHMAM_eager_funcs[0];
        goto fn_exit;
    }

    for (i = 0; i < MPIDI_num_shmam_eager_fabrics; ++i) {
        /* use MPL variant of strncasecmp if we get one */
        if (!strncasecmp
            (MPIR_CVAR_CH4_SHMAM_EAGER, MPIDI_SHMAM_eager_strings[i],
             MPIDI_MAX_SHMAM_EAGER_STRING_LEN)) {
            MPIDI_SHMAM_eager_func = MPIDI_SHMAM_eager_funcs[i];
            goto fn_exit;
        }
    }

    MPIR_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**ch4|invalid_shmam_eager",
                         "**ch4|invalid_shmam_eager %s", MPIR_CVAR_CH4_SHMAM_EAGER);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4_CHOOSE_SHMAM_EAGER);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_SHM_mpi_init_hook
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_init_hook(int rank, int size)
{
    int mpi_errno = MPI_SUCCESS;

    int i;

    int num_local = 0;

#ifdef SHM_AM_DEBUG
    char logfile_name_prefix[] = "/dev/shm/trace_";
    char logfile_name[256];
    sprintf(logfile_name, "%s%d", logfile_name_prefix, getpid());
    MPIDI_SHM_Shmam_global.logfile = fopen(logfile_name, "w+");
#endif /* SHM_AM_DEBUG */

    MPIDI_SHM_Shmam_global.am_buf_pool =
        MPIDI_CH4U_create_buf_pool(MPIDI_SHM_BUF_POOL_NUM, MPIDI_SHM_BUF_POOL_SIZE);

    MPIR_CHKPMEM_DECL(3);

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_INIT);

    MPIDI_SHM_Shmam_global.postponed_queue = NULL;

    for (i = 0; i < size; i++) {
        if (MPIDI_CH4_rank_is_local(i, MPIR_Process.comm_world)) {
            num_local++;
        }
    }

    MPIR_CHKPMEM_MALLOC(MPIDI_SHM_Shmam_global.active_rreq,
                        MPIR_Request **,
                        num_local * sizeof(MPIR_Request *), mpi_errno, "active recv req");

    for (i = 0; i < num_local; i++) {
        MPIDI_SHM_Shmam_global.active_rreq[i] = NULL;
    }

    MPIDI_choose_shmam_eager();

    int grank = MPIDI_CH4U_rank_to_lpid(rank, MPIR_Process.comm_world);

    mpi_errno = MPIDI_SHMAM_eager_init(rank, grank, num_local);

    MPIR_CHKPMEM_COMMIT();

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_INIT);
    return mpi_errno;
  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    MPIR_CHKPMEM_REAP();
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

#undef FUNCNAME
#define FUNCNAME MPIDI_SHM_mpi_finalize_hook
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_finalize_hook(void)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_FINALIZE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_FINALIZE);

    mpi_errno = MPIDI_SHMAM_eager_finalize();

    MPIDI_CH4R_destroy_buf_pool(MPIDI_SHM_Shmam_global.am_buf_pool);

    MPL_free(MPIDI_SHM_Shmam_global.active_rreq);

    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

#ifdef SHM_AM_DEBUG
    fclose(MPIDI_SHM_Shmam_global.logfile);
#endif /* SHM_AM_DEBUG */

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_FINALIZE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX void *MPIDI_SHM_mpi_alloc_mem(size_t size, MPIR_Info * info_ptr)
{
    MPIR_Assert(0);
    return NULL;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_free_mem(void *ptr)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int
MPIDI_SHM_comm_get_lpid(MPIR_Comm * comm_ptr, int idx, int *lpid_ptr, MPL_bool is_remote)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_gpid_get(MPIR_Comm * comm_ptr, int rank, MPIR_Gpid * gpid)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int
MPIDI_SHM_get_node_id(MPIR_Comm * comm, int rank, MPID_Node_id_t * id_p)
{
    *id_p = (MPID_Node_id_t) 0;
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_get_max_node_id(MPIR_Comm * comm, MPID_Node_id_t * max_id_p)
{
    *max_id_p = (MPID_Node_id_t) 1;
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int
MPIDI_SHM_getallincomm(MPIR_Comm * comm_ptr,
                       int local_size, MPIR_Gpid local_gpids[], int *singleAVT)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_gpid_tolpidarray(int size, MPIR_Gpid gpid[], int lpid[])
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int
MPIDI_SHM_create_intercomm_from_lpids(MPIR_Comm * newcomm_ptr, int size, const int lpids[])
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

#endif /* SHM_SHMAM_INIT_H_INCLUDED */

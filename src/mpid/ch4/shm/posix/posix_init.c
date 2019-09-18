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

#include "mpidimpl.h"
#include "posix_types.h"
#include "ch4_types.h"

#include "posix_eager.h"
#include "posix_noinline.h"
extern MPL_atomic_uint64_t *MPIDI_POSIX_shm_limit_counter;

static int choose_posix_eager(void);

static int choose_posix_eager(void)
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

int MPIDI_POSIX_mpi_init_hook(int rank, int size, int *n_vcis_provided, int *tag_bits)
{
    int mpi_errno = MPI_SUCCESS;
    int i, num_local = 0, local_rank_0 = -1, my_local_rank = -1;

#ifdef MPL_USE_DBG_LOGGING
    MPIDI_CH4_SHM_POSIX_GENERAL = MPL_dbg_class_alloc("SHM_POSIX", "shm_posix");
#endif /* MPL_USE_DBG_LOGGING */

    MPIDI_POSIX_global.am_buf_pool =
        MPIDIU_create_buf_pool(MPIDI_POSIX_BUF_POOL_NUM, MPIDI_POSIX_BUF_POOL_SIZE);

    MPIR_CHKPMEM_DECL(3);

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_INIT_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_INIT_HOOK);

    /* Populate these values with transformation information about each rank and its original
     * information in MPI_COMM_WORLD. */

    /* mpi_errno = MPIR_Find_local(MPIR_Process.comm_world, &num_local, &my_local_rank, */
    /*                             /1* comm_world rank of each local process *1/ */
    /*                             &MPIDI_POSIX_global.local_procs, */
    /*                             /1* local rank of each process in comm_world if it is on the same node *1/ */
    /*                             &MPIDI_POSIX_global.local_ranks); */

    MPIDI_POSIX_global.local_procs = MPIR_Process.node_local_map;
    MPIDI_POSIX_global.local_ranks = (int *) MPL_malloc(MPIR_Process.size * sizeof(int),
                                                        MPL_MEM_SHM);
    memset(MPIDI_POSIX_global.local_ranks, -1, MPIR_Process.size);
    for (i = 0; i < MPIR_Process.local_size; i++) {
        MPIDI_POSIX_global.local_ranks[MPIDI_POSIX_global.local_procs[i]] = i;
    }
    local_rank_0 = MPIR_Process.node_local_map[0];
    MPIDI_POSIX_global.num_local = MPIR_Process.local_size;
    MPIDI_POSIX_global.my_local_rank = MPIR_Process.local_rank;

    MPIDI_POSIX_global.local_rank_0 = local_rank_0;
    *n_vcis_provided = 1;

    /* This is used to track messages that the eager submodule was not ready to send. */
    MPIDI_POSIX_global.postponed_queue = NULL;

    MPIR_CHKPMEM_MALLOC(MPIDI_POSIX_global.active_rreq,
                        MPIR_Request **,
                        size * sizeof(MPIR_Request *), mpi_errno, "active recv req", MPL_MEM_SHM);

    for (i = 0; i < size; i++) {
        MPIDI_POSIX_global.active_rreq[i] = NULL;
    }

    choose_posix_eager();

    mpi_errno = MPIDI_POSIX_eager_init(rank, size);
    MPIR_ERR_CHECK(mpi_errno);

    /* There is no restriction on the tag_bits from the posix shmod side */
    *tag_bits = MPIR_TAG_BITS_DEFAULT;

    mpi_errno = MPIDI_POSIX_coll_init(rank, size);
    MPIR_ERR_CHECK(mpi_errno);

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

int MPIDI_POSIX_mpi_finalize_hook(void)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_FINALIZE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_FINALIZE_HOOK);

    mpi_errno = MPIDI_POSIX_eager_finalize();
    MPIR_ERR_CHECK(mpi_errno);

    MPIDIU_destroy_buf_pool(MPIDI_POSIX_global.am_buf_pool);

    MPL_free(MPIDI_POSIX_global.active_rreq);

    mpi_errno = MPIDI_POSIX_coll_finalize();
    MPIR_ERR_CHECK(mpi_errno);

    MPL_free(MPIDI_POSIX_global.local_ranks);
    /* MPL_free(MPIDI_POSIX_global.local_procs); */

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_FINALIZE_HOOK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_POSIX_coll_init(int rank, int size)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_COLL_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_COLL_INIT);

    /* Allocate a shared counter to track the amount of shared memory created per node for
     * intra-node collectives */
    mpi_errno =
        MPIDU_shm_seg_alloc(sizeof(int), (void **) &MPIDI_POSIX_shm_limit_counter, MPL_MEM_SHM);
    MPIR_ERR_CHECK(mpi_errno);

    /* Actually allocate the segment and assign regions to the pointers */
    mpi_errno =
        MPIDU_shm_seg_commit(&MPIDI_POSIX_global.memory, &MPIDI_POSIX_global.barrier,
                             MPIDI_POSIX_global.num_local, MPIDI_POSIX_global.my_local_rank,
                             MPIDI_POSIX_global.local_rank_0, rank, MPL_MEM_SHM);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIDU_shm_barrier(MPIDI_POSIX_global.barrier, MPIDI_POSIX_global.num_local);
    MPIR_ERR_CHECK(mpi_errno);

    /* Set the counter to 0 */
    MPL_atomic_relaxed_store_uint64(MPIDI_POSIX_shm_limit_counter, 0);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_COLL_INIT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_POSIX_coll_finalize(void)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_COLL_FINALIZE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_COLL_FINALIZE);

    /* Destroy the shared counter which was used to track the amount of shared memory created
     * per node for intra-node collectives */
    mpi_errno = MPIDU_shm_seg_destroy(&MPIDI_POSIX_global.memory, MPIDI_POSIX_global.num_local);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_COLL_FINALIZE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_POSIX_get_vci_attr(int vci)
{
    MPIR_Assert(0 <= vci && vci < 1);
    return MPIDI_VCI_TX | MPIDI_VCI_RX;
}

void *MPIDI_POSIX_mpi_alloc_mem(size_t size, MPIR_Info * info_ptr)
{
    MPIR_Assert(0);
    return NULL;
}

int MPIDI_POSIX_mpi_free_mem(void *ptr)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

int MPIDI_POSIX_get_local_upids(MPIR_Comm * comm, size_t ** local_upid_size, char **local_upids)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

int MPIDI_POSIX_upids_to_lupids(int size, size_t * remote_upid_size, char *remote_upids,
                                int **remote_lupids)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

int MPIDI_POSIX_create_intercomm_from_lpids(MPIR_Comm * newcomm_ptr, int size, const int lpids[])
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

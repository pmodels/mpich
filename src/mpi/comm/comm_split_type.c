/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "mpicomm.h"

static const char *SHMEM_INFO_KEY = "shmem_topo";

static int node_split(MPIR_Comm * comm_ptr, int key, const char *hint_str,
                      MPIR_Comm ** newcomm_ptr);

int MPIR_Comm_split_type(MPIR_Comm * user_comm_ptr, int split_type, int key,
                         MPIR_Info * info_ptr, MPIR_Comm ** newcomm_ptr)
{
    MPIR_Comm *comm_ptr = NULL;
    int mpi_errno = MPI_SUCCESS;

    /* split out the undefined processes */
    mpi_errno = MPIR_Comm_split_impl(user_comm_ptr, split_type == MPI_UNDEFINED ? MPI_UNDEFINED : 0,
                                     key, &comm_ptr);
    MPIR_ERR_CHECK(mpi_errno);

    if (split_type == MPI_UNDEFINED) {
        *newcomm_ptr = NULL;
        goto fn_exit;
    }

    if (split_type == MPI_COMM_TYPE_SHARED) {
        mpi_errno = MPIR_Comm_split_type_self(comm_ptr, key, newcomm_ptr);
        MPIR_ERR_CHECK(mpi_errno);
    } else if (split_type == MPI_COMM_TYPE_HW_GUIDED) {
        mpi_errno = MPIR_Comm_split_type_hw_guided(comm_ptr, key, info_ptr, newcomm_ptr);
        MPIR_ERR_CHECK(mpi_errno);
    } else if (split_type == MPI_COMM_TYPE_HW_UNGUIDED) {
        mpi_errno = MPIR_Comm_split_type_hw_unguided(comm_ptr, key, info_ptr, newcomm_ptr);
        MPIR_ERR_CHECK(mpi_errno);
    } else if (split_type == MPIX_COMM_TYPE_NEIGHBORHOOD) {
        mpi_errno =
            MPIR_Comm_split_type_neighborhood(comm_ptr, split_type, key, info_ptr, newcomm_ptr);
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_ARG, "**arg");
    }

  fn_exit:
    if (comm_ptr)
        MPIR_Comm_free_impl(comm_ptr);
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

int MPIR_Comm_split_type_impl(MPIR_Comm * comm_ptr, int split_type, int key,
                              MPIR_Info * info_ptr, MPIR_Comm ** newcomm_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    if (MPIR_Comm_fns != NULL && MPIR_Comm_fns->split_type != NULL) {
        mpi_errno = MPIR_Comm_fns->split_type(comm_ptr, split_type, key, info_ptr, newcomm_ptr);
    } else {
        mpi_errno = MPIR_Comm_split_type(comm_ptr, split_type, key, info_ptr, newcomm_ptr);
    }
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIR_Comm_set_info_impl(*newcomm_ptr, info_ptr);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Comm_split_type_self(MPIR_Comm * comm_ptr, int key, MPIR_Comm ** newcomm_ptr)
{
    MPIR_Comm *comm_self_ptr;
    int mpi_errno = MPI_SUCCESS;

    MPIR_Comm_get_ptr(MPI_COMM_SELF, comm_self_ptr);
    mpi_errno = MPIR_Comm_dup_impl(comm_self_ptr, newcomm_ptr);

    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

int MPIR_Comm_split_type_hw_guided(MPIR_Comm * comm_ptr, int key, MPIR_Info * info_ptr,
                                   MPIR_Comm ** newcomm_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    *newcomm_ptr = NULL;

    return mpi_errno;
}

int MPIR_Comm_split_type_hw_unguided(MPIR_Comm * comm_ptr, int key, MPIR_Info * info_ptr,
                                     MPIR_Comm ** newcomm_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    *newcomm_ptr = NULL;

    return mpi_errno;
}

int MPIR_Comm_split_type_by_node(MPIR_Comm * comm_ptr, int key, MPIR_Comm ** newcomm_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    int color;

    mpi_errno = MPID_Get_node_id(comm_ptr, comm_ptr->rank, &color);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIR_Comm_split_impl(comm_ptr, color, key, newcomm_ptr);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

int MPIR_Comm_split_type_node_topo(MPIR_Comm * user_comm_ptr, int key,
                                   MPIR_Info * info_ptr, MPIR_Comm ** newcomm_ptr)
{
    MPIR_Comm *comm_ptr;
    int mpi_errno = MPI_SUCCESS;
    int flag = 0;
    char hint_str[MPI_MAX_INFO_VAL + 1];
    int info_args_are_equal;
    *newcomm_ptr = NULL;

    mpi_errno = MPIR_Comm_split_type_by_node(user_comm_ptr, key, &comm_ptr);
    MPIR_ERR_CHECK(mpi_errno);

    if (info_ptr) {
        MPIR_Info_get_impl(info_ptr, SHMEM_INFO_KEY, MPI_MAX_INFO_VAL, hint_str, &flag);
    }

    if (!flag) {
        hint_str[0] = '\0';
    }

    /* make sure all processes are using the same hint_str */
    mpi_errno = MPII_compare_info_hint(hint_str, comm_ptr, &info_args_are_equal);

    MPIR_ERR_CHECK(mpi_errno);

    /* if all processes do not have the same hint_str, skip
     * topology-aware comm split */
    if (!info_args_are_equal)
        goto use_node_comm;

    /* if no info key is given, skip topology-aware comm split */
    if (!info_ptr)
        goto use_node_comm;

    /* if hw topology is not initialized, skip topology-aware comm split */
    if (!MPIR_hwtopo_is_initialized())
        goto use_node_comm;

    if (flag) {
        mpi_errno = node_split(comm_ptr, key, hint_str, newcomm_ptr);

        MPIR_ERR_CHECK(mpi_errno);

        MPIR_Comm_free_impl(comm_ptr);

        goto fn_exit;
    }

  use_node_comm:
    *newcomm_ptr = comm_ptr;

  fn_exit:
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

static int node_split(MPIR_Comm * comm_ptr, int key, const char *hint_str, MPIR_Comm ** newcomm_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_hwtopo_gid_t gid;

    gid = MPIR_hwtopo_get_obj_by_name(hint_str);

    mpi_errno = MPIR_Comm_split_impl(comm_ptr, gid, key, newcomm_ptr);

    return mpi_errno;
}

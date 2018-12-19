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
#ifndef CH4_COMM_H_INCLUDED
#define CH4_COMM_H_INCLUDED

#include "pmi.h"
#include "utarray.h"

#include "ch4_impl.h"
#include "ch4r_comm.h"
#include "ch4i_comm.h"

#ifdef MPIDI_CH4_ULFM
#define parse_rank(r_p) do {                                                                \
        while (isspace(*c)) /* skip spaces */                                               \
            ++c;                                                                            \
        MPIR_ERR_CHKINTERNAL(!isdigit(*c), mpi_errno, "error parsing failed process list"); \
        *(r_p) = (int)strtol(c, &c, 0);                                                     \
        while (isspace(*c)) /* skip spaces */                                               \
            ++c;                                                                            \
    } while (0)
#endif

MPL_STATIC_INLINE_PREFIX int MPID_Comm_AS_enabled(MPIR_Comm * comm)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_COMM_AS_ENABLED);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_COMM_AS_ENABLED);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_COMM_AS_ENABLED);
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPID_Comm_reenable_anysource(MPIR_Comm * comm,
                                                          MPIR_Group ** failed_group_ptr)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_COMM_REENABLE_ANYSOURCE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_COMM_REENABLE_ANYSOURCE);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_COMM_REENABLE_ANYSOURCE);
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPID_Comm_remote_group_failed(MPIR_Comm * comm,
                                                           MPIR_Group ** failed_group_ptr)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_COMM_REMOTE_GROUP_FAILED);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_COMM_REMOTE_GROUP_FAILED);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_COMM_REMOTE_GROUP_FAILED);
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPID_Comm_group_failed(MPIR_Comm * comm_ptr,
                                                    MPIR_Group ** failed_group_ptr)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_COMM_GROUP_FAILED);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_COMM_GROUP_FAILED);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_COMM_GROUP_FAILED);
    return MPI_SUCCESS;
}

#ifdef MPIDI_CH4_ULFM
#undef FUNCNAME
#define FUNCNAME MPIDI_CH4I_get_failed_group
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4I_get_failed_group(int last_rank, MPIR_Group **failed_group)
{
    char *c;
    int i, mpi_errno = MPI_SUCCESS, rank;
    UT_array *failed_procs = NULL;
    MPIR_Group *world_group;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4I_GET_FAILED_GROUP);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4I_GET_FAILED_GROUP);

    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_COMM, VERBOSE,
                    (MPL_DBG_FDEST, "Getting failed group with %d as last acknowledged\n",
                     last_rank));

    if (-1 == last_rank) {
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_COMM, VERBOSE, (MPL_DBG_FDEST, "No failure acknowledged"));

        *failed_group = MPIR_Group_empty;
        goto fn_exit;
    }

    if (*MPIDI_failed_procs_string == '\0') {
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_COMM, VERBOSE, (MPL_DBG_FDEST, "Found no failed ranks"));
        *failed_group = MPIR_Group_empty;
        goto fn_exit;
    }

    utarray_new(failed_procs, &ut_int_icd, MPL_MEM_OTHER);

    /* parse list of failed processes.  This is a comma separated list
       of ranks or ranges of ranks (e.g., "1, 3-5, 11") */
    i = 0;
    c = MPIDI_failed_procs_string;
    while(1) {
        parse_rank(&rank);
        ++i;
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_COMM, VERBOSE,
                        (MPL_DBG_FDEST, "Found failed rank: %d", rank));

        utarray_push_back(failed_procs, &rank, MPL_MEM_OTHER);
        MPIDI_last_known_failed = rank;
        MPIR_ERR_CHKINTERNAL(*c != ',' && *c != '\0', mpi_errno,
                             "error parsing failed process list");
        if (*c == '\0' || last_rank == rank)
            break;
        ++c; /* skip ',' */
    }

    /* Create group of failed processes for comm_world.  Failed groups for other
       communicators can be created from this one using group_intersection. */
    mpi_errno = MPIR_Comm_group_impl(MPIR_Process.comm_world, &world_group);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    mpi_errno = MPIR_Group_incl_impl(world_group, i, ut_int_array(failed_procs), failed_group);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    mpi_errno = MPIR_Group_release(world_group);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4I_GET_FAILED_GROUP);
    if (failed_procs)
        utarray_free(failed_procs);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME nonempty_intersection
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int nonempty_intersection(MPIR_Comm *comm, MPIR_Group *group, int *flag)
{
    int mpi_errno = MPI_SUCCESS;
    int i_g, i_c;
    int lpid_g, lpid_c;



    /* handle common case fast */
    if (comm == MPIR_Process.comm_world || comm == MPIR_Process.icomm_world) {
        *flag = TRUE;
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                         (MPL_DBG_FDEST, "comm is comm_world or icomm_world"));
        goto fn_exit;
    }
    *flag = FALSE;

    /* FIXME: This algorithm assumes that the number of processes in group is
       very small (like 1).  So doing a linear search for them in comm is better
       than sorting the procs in comm and group then doing a binary search */

    for (i_g = 0; i_g < group->size; ++i_g) {
        lpid_g = group->lrank_to_lpid[i_g].lpid;
        for (i_c = 0; i_c < comm->remote_size; ++i_c) {
            MPID_Comm_get_lpid(comm, i_c, &lpid_c, FALSE);
            if (lpid_g == lpid_c) {
                *flag = TRUE;
                goto fn_exit;
            }
        }
    }

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4I_comm_handle_failed_procs
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4I_comm_handle_failed_procs(MPIR_Group *new_failed_procs)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm;
    int flag = FALSE;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4I_COMM_HANDLE_FAILED_PROCS);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4I_COMM_HANDLE_FAILED_PROCS);

    /* mark communicators with new failed processes as collectively inactive and
       disable posting anysource receives */
    MPIDIU_COMM_LIST_FOREACH(comm) {
        /* if this comm is already collectively inactive and
           anysources are disabled, there's no need to check */
        if (!MPIDI_COMM(comm, anysource_enabled))
            continue;

        mpi_errno = nonempty_intersection(comm, new_failed_procs, &flag);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);

        if (flag) {
            MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                             (MPL_DBG_FDEST, "disabling AS on communicator %p (%#08x)",
                              comm, comm->handle));
            MPIDI_COMM(comm, anysource_enabled) = FALSE;
        }
    }

    /* Signal that something completed here to allow the progress engine to
     * break out and return control to the user. */
    // MPIDI_CH3_Progress_signal_completion();

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4I_COMM_HANDLE_FAILED_PROCS);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4I_check_for_failed_procs
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4I_check_for_failed_procs(void)
{
    int mpi_errno = MPI_SUCCESS;
    int pmi_errno;
    int len;
    char *kvsname;
    MPIR_Group *prev_failed_group, *new_failed_group;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4I_CHECK_FOR_FAILED_PROCS);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4I_CHECK_FOR_FAILED_PROCS);

#ifdef USE_PMI2_API
    int vallen = 0;
    pmi_errno+ PMI2_KVS_Get(MPICH_CH4_Global.jobid, "PMI_dead_processes",
                            MPIDI_failed_procs_string, PMI2_MAX_VALLEN, &vallen);
    MPIR_ERR_CHKANDJUMP(pmi_errno, mpi_errno, MPI_ERR_OTHER, "**pmi_kvs_get");
#else
    pmi_errno = PMI_KVS_Get_value_length_max(&len);
    MPIR_ERR_CHKANDJUMP(pmi_errno, mpi_errno, MPI_ERR_OTHER, "**pmi_kvs_get_value_length_max");
    pmi_errno = PMI_KVS_Get(MPIDI_CH4_Global.jobid, "PMI_dead_processes",
                            MPIDI_failed_procs_string, len);
    MPIR_ERR_CHKANDJUMP(pmi_errno, mpi_errno, MPI_ERR_OTHER, "**pmi_kvs_get");
#endif

    if (*MPIDI_failed_procs_string == '\0') {
        /* there are no failed processes */
        MPIDI_failed_procs_group = MPIR_Group_empty;
        goto fn_exit;
    }

    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_COMM, VERBOSE,
                    (MPL_DBG_FDEST, "Received proc fail notification: %d",
                     MPIDI_failed_procs_string));


    /* save reference to previous group so we can identify new failures */
    prev_failed_group = MPIDI_failed_procs_group;

    /* Parse the list of failed processes */
    MPIDI_CH4I_get_failed_group(-2, &MPIDI_failed_procs_group);

    /* get group of newly failed processes */
    mpi_errno = MPIR_Group_difference_impl(MPIDI_failed_procs_group, prev_failed_group, &new_failed_group);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    if (new_failed_group != MPIR_Group_empty) {
        mpi_errno = MPIDI_CH4I_comm_handle_failed_procs(new_failed_group);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);

        // mpi_errno = terminate_failed_VCs(new_failed_group);
        // if (mpi_errno)
        //     MPIR_ERR_POP(mpi_errno);

        mpi_errno = MPIR_Group_release(new_failed_group);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }

    /* free prev group */
    if (prev_failed_group != MPIR_Group_empty) {
        mpi_errno = MPIR_Group_release(prev_failed_group);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }

 fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4I_CHECK_FOR_FAILED_PROCS);
    return mpi_errno;
 fn_oom: /* out-of-memory handler for utarray operations */
    MPIR_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "utarray");
 fn_fail:
    goto fn_exit;
}
#endif

#undef FUNCNAME
#define FUNCNAME MPID_Comm_failure_ack
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPID_Comm_failure_ack(MPIR_Comm * comm_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_COMM_FAILURE_ACK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_COMM_FAILURE_ACK);

#ifdef MPIDI_CH4_ULFM
    MPIDI_CH4I_check_for_failed_procs();

    MPIDI_COMM(comm_ptr, last_ack_rank) = MPIDI_last_known_failed;
    MPIDI_COMM(comm_ptr, anysource_enabled) = 1;
#else
    MPIR_Assert(0);
#endif

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_COMM_FAILURE_ACK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_Comm_failure_get_acked
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPID_Comm_failure_get_acked(MPIR_Comm * comm_ptr,
                                                         MPIR_Group ** failed_group_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Group *failed_group, *comm_group;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_COMM_FAILURE_GET_ACKED);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_COMM_FAILURE_GET_ACKED);

#ifdef MPIDI_CH4_ULFM
    /* Get the group of all failed processes */
    MPIDI_CH4I_check_for_failed_procs();
    MPIDI_CH4I_get_failed_group(MPIDI_COMM(comm_ptr, last_ack_rank), &failed_group);
    if (failed_group == MPIR_Group_empty) {
        *failed_group_ptr = MPIR_Group_empty;
        goto fn_exit;
    }

    MPIR_Comm_group_impl(comm_ptr, &comm_group);

    /* Get the intersection of all falied processes in this communicator */
    MPIR_Group_intersection_impl(failed_group, comm_group, failed_group_ptr);

    MPIR_Group_release(comm_group);
    MPIR_Group_release(failed_group);
#else
    MPIR_Assert(0);
#endif

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_COMM_FAILURE_GET_ACKED);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_Comm_revoke
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPID_Comm_revoke(MPIR_Comm * comm_ptr, int is_remote)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_COMM_REVOKE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_COMM_REVOKE);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_COMM_REVOKE);
    return 0;
}

#undef FUNCNAME
#define FUNCNAME MPID_Comm_get_all_failed_procs
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPID_Comm_get_all_failed_procs(MPIR_Comm * comm_ptr,
                                                            MPIR_Group ** failed_group, int tag)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_COMM_GET_ALL_FAILED_PROCS);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_COMM_GET_ALL_FAILED_PROCS);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_COMM_GET_ALL_FAILED_PROCS);
    return 0;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_Comm_split_type
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_Comm_split_type(MPIR_Comm * user_comm_ptr,
                                                   int split_type,
                                                   int key, MPIR_Info * info_ptr,
                                                   MPIR_Comm ** newcomm_ptr)
{
    MPIR_Comm *comm_ptr = NULL;
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_COMM_SPLIT_TYPE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_COMM_SPLIT_TYPE);

    mpi_errno = MPIR_Comm_split_impl(user_comm_ptr, split_type == MPI_UNDEFINED ? MPI_UNDEFINED : 0,
                                     key, &comm_ptr);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    if (split_type == MPI_UNDEFINED) {
        *newcomm_ptr = NULL;
        goto fn_exit;
    }

    if (split_type != MPI_COMM_TYPE_SHARED) {
        /* we don't know how to handle other split types; hand it back
         * to the upper layer */
        mpi_errno = MPIR_Comm_split_type(comm_ptr, split_type, key, info_ptr, newcomm_ptr);
        goto fn_exit;
    }

    mpi_errno = MPIR_Comm_split_type_node_topo(comm_ptr, split_type, key, info_ptr, newcomm_ptr);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    if (comm_ptr)
        MPIR_Comm_free_impl(comm_ptr);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_COMM_SPLIT_TYPE);
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

#undef FUNCNAME
#define FUNCNAME MPIDI_Comm_create_hook
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_Comm_create_hook(MPIR_Comm * comm)
{
    int mpi_errno;
    int i, *uniq_avtids;
    int max_n_avts;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_COMM_CREATE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_COMM_CREATE_HOOK);

    /* comm_world and comm_self are already initialized */
    if (comm != MPIR_Process.comm_world && comm != MPIR_Process.comm_self) {
        MPIDI_comm_create_rank_map(comm);
        /* add ref to avts */
        switch (MPIDI_COMM(comm, map).mode) {
            case MPIDI_RANK_MAP_NONE:
                break;
            case MPIDI_RANK_MAP_MLUT:
                max_n_avts = MPIDIU_get_max_n_avts();
                uniq_avtids = (int *) MPL_malloc(max_n_avts * sizeof(int), MPL_MEM_ADDRESS);
                memset(uniq_avtids, 0, max_n_avts * sizeof(int));
                for (i = 0; i < MPIDI_COMM(comm, map).size; i++) {
                    if (uniq_avtids[MPIDI_COMM(comm, map).irreg.mlut.gpid[i].avtid] == 0) {
                        uniq_avtids[MPIDI_COMM(comm, map).irreg.mlut.gpid[i].avtid] = 1;
                        MPIDIU_avt_add_ref(MPIDI_COMM(comm, map).irreg.mlut.gpid[i].avtid);
                    }
                }
                MPL_free(uniq_avtids);
                break;
            default:
                MPIDIU_avt_add_ref(MPIDI_COMM(comm, map).avtid);
        }

        switch (MPIDI_COMM(comm, local_map).mode) {
            case MPIDI_RANK_MAP_NONE:
                break;
            case MPIDI_RANK_MAP_MLUT:
                max_n_avts = MPIDIU_get_max_n_avts();
                uniq_avtids = (int *) MPL_malloc(max_n_avts * sizeof(int), MPL_MEM_ADDRESS);
                memset(uniq_avtids, 0, max_n_avts * sizeof(int));
                for (i = 0; i < MPIDI_COMM(comm, local_map).size; i++) {
                    if (uniq_avtids[MPIDI_COMM(comm, local_map).irreg.mlut.gpid[i].avtid] == 0) {
                        uniq_avtids[MPIDI_COMM(comm, local_map).irreg.mlut.gpid[i].avtid] = 1;
                        MPIDIU_avt_add_ref(MPIDI_COMM(comm, local_map).irreg.mlut.gpid[i].avtid);
                    }
                }
                MPL_free(uniq_avtids);
                break;
            default:
                MPIDIU_avt_add_ref(MPIDI_COMM(comm, local_map).avtid);
        }
    }

    mpi_errno = MPIDI_NM_mpi_comm_create_hook(comm);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
#ifndef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_SHM_mpi_comm_create_hook(comm);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
#endif

#ifdef MPIDI_CH4_ULFM
    MPIDI_COMM(comm, last_ack_rank) = -1;
    MPIDI_COMM(comm, anysource_enabled) = 1;

    MPIDIU_COMM_LIST_ADD(comm);
#endif

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_COMM_CREATE_HOOK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_Comm_free_hook
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_Comm_free_hook(MPIR_Comm * comm)
{
    int mpi_errno;
    int i, *uniq_avtids;
    int max_n_avts;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_COMM_FREE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_COMM_FREE_HOOK);
    /* release ref to avts */
    switch (MPIDI_COMM(comm, map).mode) {
        case MPIDI_RANK_MAP_NONE:
            break;
        case MPIDI_RANK_MAP_MLUT:
            max_n_avts = MPIDIU_get_max_n_avts();
            uniq_avtids = (int *) MPL_malloc(max_n_avts * sizeof(int), MPL_MEM_ADDRESS);
            memset(uniq_avtids, 0, max_n_avts * sizeof(int));
            for (i = 0; i < MPIDI_COMM(comm, map).size; i++) {
                if (uniq_avtids[MPIDI_COMM(comm, map).irreg.mlut.gpid[i].avtid] == 0) {
                    uniq_avtids[MPIDI_COMM(comm, map).irreg.mlut.gpid[i].avtid] = 1;
                    MPIDIU_avt_release_ref(MPIDI_COMM(comm, map).irreg.mlut.gpid[i].avtid);
                }
            }
            MPL_free(uniq_avtids);
            break;
        default:
            MPIDIU_avt_release_ref(MPIDI_COMM(comm, map).avtid);
    }

    switch (MPIDI_COMM(comm, local_map).mode) {
        case MPIDI_RANK_MAP_NONE:
            break;
        case MPIDI_RANK_MAP_MLUT:
            max_n_avts = MPIDIU_get_max_n_avts();
            uniq_avtids = (int *) MPL_malloc(max_n_avts * sizeof(int), MPL_MEM_ADDRESS);
            memset(uniq_avtids, 0, max_n_avts * sizeof(int));
            for (i = 0; i < MPIDI_COMM(comm, local_map).size; i++) {
                if (uniq_avtids[MPIDI_COMM(comm, local_map).irreg.mlut.gpid[i].avtid] == 0) {
                    uniq_avtids[MPIDI_COMM(comm, local_map).irreg.mlut.gpid[i].avtid] = 1;
                    MPIDIU_avt_release_ref(MPIDI_COMM(comm, local_map).irreg.mlut.gpid[i].avtid);
                }
            }
            MPL_free(uniq_avtids);
            break;
        default:
            MPIDIU_avt_release_ref(MPIDI_COMM(comm, local_map).avtid);
    }

    if (MPIDI_COMM(comm, map).mode == MPIDI_RANK_MAP_LUT
        || MPIDI_COMM(comm, map).mode == MPIDI_RANK_MAP_LUT_INTRA) {
        MPIDIU_release_lut(MPIDI_COMM(comm, map).irreg.lut.t);
    }
    if (MPIDI_COMM(comm, local_map).mode == MPIDI_RANK_MAP_LUT
        || MPIDI_COMM(comm, local_map).mode == MPIDI_RANK_MAP_LUT_INTRA) {
        MPIDIU_release_lut(MPIDI_COMM(comm, local_map).irreg.lut.t);
    }
    if (MPIDI_COMM(comm, map).mode == MPIDI_RANK_MAP_MLUT) {
        MPIDIU_release_mlut(MPIDI_COMM(comm, map).irreg.mlut.t);
    }
    if (MPIDI_COMM(comm, local_map).mode == MPIDI_RANK_MAP_MLUT) {
        MPIDIU_release_mlut(MPIDI_COMM(comm, local_map).irreg.mlut.t);
    }

    mpi_errno = MPIDI_NM_mpi_comm_free_hook(comm);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
#ifndef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_SHM_mpi_comm_free_hook(comm);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
#endif

#ifdef MPIDI_CH4_ULFM
    MPIDIU_COMM_LIST_DEL(comm);
#endif

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_COMM_FREE_HOOK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_Intercomm_exchange_map
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPID_Intercomm_exchange_map(MPIR_Comm * local_comm,
                                                         int local_leader,
                                                         MPIR_Comm * peer_comm,
                                                         int remote_leader,
                                                         int *remote_size,
                                                         int **remote_lupids, int *is_low_group)
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    int avtid = 0, lpid = -1;
    int local_avtid = 0, remote_avtid = 0;
    int local_size_send = 0, remote_size_recv = 0;
    int cts_tag = 0;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    int pure_intracomm = 1;
    int local_size = 0;
    int *local_node_ids = NULL, *remote_node_ids = NULL;
    int *local_lupids = NULL;
    size_t *local_upid_size = NULL, *remote_upid_size = NULL;
    int upid_send_size = 0, upid_recv_size = 0;
    char *local_upids = NULL, *remote_upids = NULL;

    /*
     * CH4 only cares about LUPID. GUPID extraction and exchange should be done
     * by netmod
     */
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_INTERCOMM_EXCHANGE_MAP);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_INTERCOMM_EXCHANGE_MAP);

    MPIR_CHKPMEM_DECL(1);
    MPIR_CHKLMEM_DECL(5);

    cts_tag = 0 | MPIR_TAG_COLL_BIT;
    local_size = local_comm->local_size;

    /*
     * Stage 1: UPID exchange and LUPID conversion in leaders
     */
    if (local_comm->rank == local_leader) {
        /* We need to check all processes in local group to decide there
         * is no dynamic spawned process. */
        for (i = 0; i < local_size; i++) {
            MPIDIU_comm_rank_to_pid(local_comm, i, &lpid, &local_avtid);
            if (local_avtid > 0) {
                pure_intracomm = 0;
                break;
            }
        }
        if (pure_intracomm) {
            /* check if remote leader is dynamic spawned process */
            MPIDIU_comm_rank_to_pid(peer_comm, remote_leader, &lpid, &remote_avtid);
            if (remote_avtid > 0)
                pure_intracomm = 0;
        }
        local_size_send = local_size;
        if (!pure_intracomm) {
            /* embedded dynamic process info in size */
            local_size_send |= MPIDI_DYNPROC_MASK;
        }

        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_COMM, VERBOSE,
                        (MPL_DBG_FDEST, "rank %d sendrecv to rank %d",
                         peer_comm->rank, remote_leader));
        mpi_errno = MPIC_Sendrecv(&local_size_send, 1, MPI_INT,
                                  remote_leader, cts_tag,
                                  &remote_size_recv, 1, MPI_INT,
                                  remote_leader, cts_tag, peer_comm, MPI_STATUS_IGNORE, &errflag);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);

        if (remote_size_recv & MPIDI_DYNPROC_MASK)
            pure_intracomm = 0;
        (*remote_size) = remote_size_recv & (~MPIDI_DYNPROC_MASK);


        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_COMM, VERBOSE,
                        (MPL_DBG_FDEST, "local size = %d, remote size = %d, pure_intracomm = %d",
                         local_size, *remote_size, pure_intracomm));

        MPIR_CHKPMEM_MALLOC((*remote_lupids), int *, (*remote_size) * sizeof(int),
                            mpi_errno, "remote_lupids", MPL_MEM_ADDRESS);
        MPIR_CHKLMEM_MALLOC(local_lupids, int *, local_size * sizeof(int),
                            mpi_errno, "local_lupids", MPL_MEM_ADDRESS);
        for (i = 0; i < local_size; i++) {
            MPIDIU_comm_rank_to_pid(local_comm, i, &lpid, &avtid);
            local_lupids[i] = MPIDIU_LUPID_CREATE(avtid, lpid);
        }

        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_COMM, VERBOSE,
                        (MPL_DBG_FDEST, "Intercomm map exchange stage 1: leaders"));
        if (!pure_intracomm) {
            /* Stage 1.1 UPID exchange between leaders */
            MPIR_CHKLMEM_MALLOC(remote_upid_size, size_t *, (*remote_size) * sizeof(size_t),
                                mpi_errno, "remote_upid_size", MPL_MEM_ADDRESS);

            mpi_errno = MPIDI_NM_get_local_upids(local_comm, &local_upid_size, &local_upids);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
            mpi_errno = MPIC_Sendrecv(local_upid_size, local_size, MPI_UNSIGNED_LONG,
                                      remote_leader, cts_tag,
                                      remote_upid_size, *remote_size, MPI_UNSIGNED_LONG,
                                      remote_leader, cts_tag,
                                      peer_comm, MPI_STATUS_IGNORE, &errflag);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
            upid_send_size = 0;
            for (i = 0; i < local_size; i++)
                upid_send_size += local_upid_size[i];
            upid_recv_size = 0;
            for (i = 0; i < *remote_size; i++)
                upid_recv_size += remote_upid_size[i];
            MPIR_CHKLMEM_MALLOC(remote_upids, char *, upid_recv_size * sizeof(char),
                                mpi_errno, "remote_upids", MPL_MEM_ADDRESS);
            mpi_errno = MPIC_Sendrecv(local_upids, upid_send_size, MPI_BYTE,
                                      remote_leader, cts_tag,
                                      remote_upids, upid_recv_size, MPI_BYTE,
                                      remote_leader, cts_tag,
                                      peer_comm, MPI_STATUS_IGNORE, &errflag);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);

            MPIR_CHKLMEM_MALLOC(local_node_ids, int *,
                                local_size * sizeof(int), mpi_errno, "local_node_ids",
                                MPL_MEM_ADDRESS);
            MPIR_CHKLMEM_MALLOC(remote_node_ids, int *, (*remote_size) * sizeof(int), mpi_errno,
                                "remote_node_ids", MPL_MEM_ADDRESS);
            for (i = 0; i < local_size; i++) {
                MPIDI_CH4U_get_node_id(local_comm, i, &local_node_ids[i]);
            }
            mpi_errno = MPIC_Sendrecv(local_node_ids, local_size * sizeof(int), MPI_BYTE,
                                      remote_leader, cts_tag,
                                      remote_node_ids, (*remote_size) * sizeof(int),
                                      MPI_BYTE, remote_leader, cts_tag, peer_comm,
                                      MPI_STATUS_IGNORE, &errflag);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);

            /* Stage 1.2 convert remote UPID to LUPID and get LUPID for local group */
            MPIDIU_upids_to_lupids(*remote_size, remote_upid_size, remote_upids,
                                   remote_lupids, remote_node_ids);
        } else {
            /* Stage 1.1f only exchange LUPIDS if no dynamic process involved */
            mpi_errno = MPIC_Sendrecv(local_lupids, local_size, MPI_INT,
                                      remote_leader, cts_tag,
                                      *remote_lupids, *remote_size, MPI_INT,
                                      remote_leader, cts_tag,
                                      peer_comm, MPI_STATUS_IGNORE, &errflag);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
        }
        /* Stage 1.3 check if local/remote groups are disjoint */

        /*
         * Error checking for this routine requires care.  Because this
         * routine is collective over two different sets of processes,
         * it is relatively easy for the user to try to create an
         * intercommunicator from two overlapping groups of processes.
         * This is made more likely by inconsistencies in the MPI-1
         * specification (clarified in MPI-2) that seemed to allow
         * the groups to overlap.  Because of that, we first check that the
         * groups are in fact disjoint before performing any collective
         * operations.
         */

#ifdef HAVE_ERROR_CHECKING
        {
            MPID_BEGIN_ERROR_CHECKS;
            {
                /* Now that we have both the local and remote processes,
                 * check for any overlap */
                mpi_errno = MPIDI_check_disjoint_lupids(local_lupids, local_size,
                                                        *remote_lupids, *remote_size);
                if (mpi_errno)
                    MPIR_ERR_POP(mpi_errno);
            }
            MPID_END_ERROR_CHECKS;
        }
#endif /* HAVE_ERROR_CHECKING */

        /*
         * Make an arbitrary decision about which group of processs is
         * the low group.  The LEADERS do this by comparing the
         * local process ids of the 0th member of the two groups
         * LUPID itself is not enough for determine is_low_group because both
         * local group is always smaller than remote
         */
        if (pure_intracomm) {
            *is_low_group = local_lupids[0] < (*remote_lupids)[0];
        } else {
            if (local_upid_size[0] == remote_upid_size[0]) {
                *is_low_group = memcmp(local_upids, remote_upids, local_upid_size[0]);
                MPIR_Assert(*is_low_group != 0);
                if (*is_low_group < 0)
                    *is_low_group = 0;
                else
                    *is_low_group = 1;
            } else {
                *is_low_group = local_upid_size[0] < remote_upid_size[0];
            }
        }

        /* At this point, we're done with the local lpids; they'll
         * be freed with the other local memory on exit */
        local_lupids = NULL;
    }

    /*
     * Stage 2. Bcast UPID to non-leaders (intra-group)
     */
    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_COMM, VERBOSE,
                    (MPL_DBG_FDEST, "Intercomm map exchange stage 2: intra-group"));
    mpi_errno = MPIDIU_Intercomm_map_bcast_intra(local_comm, local_leader,
                                                 remote_size, is_low_group, pure_intracomm,
                                                 remote_upid_size, remote_upids,
                                                 remote_lupids, remote_node_ids);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    MPIR_CHKPMEM_COMMIT();
  fn_exit:
    MPL_free(local_upid_size);
    MPL_free(local_upids);
    MPIR_CHKLMEM_FREEALL();
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_INTERCOMM_EXCHANGE_MAP);
    return mpi_errno;
  fn_fail:
    MPIR_CHKPMEM_REAP();
    goto fn_exit;
}

#endif /* CH4_COMM_H_INCLUDED */

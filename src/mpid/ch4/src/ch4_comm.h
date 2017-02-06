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

#include "ch4_impl.h"
#include "ch4r_comm.h"
#include "ch4i_comm.h"

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

MPL_STATIC_INLINE_PREFIX int MPID_Comm_failure_ack(MPIR_Comm * comm_ptr)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_COMM_FAILURE_ACK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_COMM_FAILURE_ACK);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_COMM_FAILURE_ACK);
    return 0;
}

MPL_STATIC_INLINE_PREFIX int MPID_Comm_failure_get_acked(MPIR_Comm * comm_ptr,
                                                          MPIR_Group ** failed_group_ptr)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_COMM_FAILURE_GET_ACKED);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_COMM_FAILURE_GET_ACKED);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_COMM_FAILURE_GET_ACKED);
    return 0;
}

MPL_STATIC_INLINE_PREFIX int MPID_Comm_revoke(MPIR_Comm * comm_ptr, int is_remote)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_COMM_REVOKE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_COMM_REVOKE);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_COMM_REVOKE);
    return 0;
}

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
MPL_STATIC_INLINE_PREFIX int MPIDI_Comm_split_type(MPIR_Comm * comm_ptr,
                                                   int split_type,
                                                   int key, MPIR_Info * info_ptr,
                                                   MPIR_Comm ** newcomm_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    int idx;
    MPID_Node_id_t node_id;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_COMM_SPLIT_TYPE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_COMM_SPLIT_TYPE);

    if (split_type == MPI_COMM_TYPE_SHARED) {
        MPID_Comm_get_lpid(comm_ptr, comm_ptr->rank, &idx, FALSE);
        MPID_Get_node_id(comm_ptr, comm_ptr->rank, &node_id);
        mpi_errno = MPIR_Comm_split_impl(comm_ptr, node_id, key, newcomm_ptr);
    }
    else
        mpi_errno = MPIR_Comm_split_impl(comm_ptr, MPI_UNDEFINED, key, newcomm_ptr);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_COMM_SPLIT_TYPE);
    return mpi_errno;
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
    mpi_errno = MPIDI_NM_mpi_comm_create_hook(comm);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
#ifdef MPIDI_BUILD_CH4_SHM
    mpi_errno = MPIDI_SHM_mpi_comm_create_hook(comm);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
#endif

    /* comm_world and comm_self are already initialized */
    if (comm != MPIR_Process.comm_world && comm != MPIR_Process.comm_self) {
        MPIDI_comm_create_rank_map(comm);
        /* add ref to avts */
        switch (MPIDI_COMM(comm, map).mode) {
        case MPIDI_RANK_MAP_NONE:
            break;
        case MPIDI_RANK_MAP_MLUT:
            max_n_avts = MPIDIU_get_max_n_avts();
            uniq_avtids = (int *) MPL_malloc(max_n_avts * sizeof(int));
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
            uniq_avtids = (int *) MPL_malloc(max_n_avts * sizeof(int));
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
        uniq_avtids = (int *) MPL_malloc(max_n_avts * sizeof(int));
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
        uniq_avtids = (int *) MPL_malloc(max_n_avts * sizeof(int));
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

    mpi_errno = MPIDI_NM_mpi_comm_free_hook(comm);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
#ifdef MPIDI_BUILD_CH4_SHM
    mpi_errno = MPIDI_SHM_mpi_comm_free_hook(comm);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
#endif

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
    int avtid = 0, lpid;
    int local_avtid = 0, remote_avtid = 0;
    int local_size_send = 0, remote_size_recv = 0;
    int cts_tag = 0;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    int pure_intracomm = 0;
    int local_size = 0;
    MPID_Node_id_t *local_node_ids = NULL, *remote_node_ids = NULL;
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

    cts_tag = 0 | MPIR_Process.tagged_coll_mask;
    local_size = local_comm->local_size;

    /*
     * Stage 1: UPID exchange and LUPID conversion in leaders
     */
    if (local_comm->rank == local_leader) {
        /* embedded dynamic process info in size */
        MPIDIU_comm_rank_to_pid(local_comm, local_leader, &lpid, &local_avtid);
        MPIDIU_comm_rank_to_pid(peer_comm, remote_leader, &lpid, &remote_avtid);
        if (local_avtid == 0 && remote_avtid == 0) {
            pure_intracomm = 1;
            local_size_send = local_size;
        }
        else {
            local_size_send = local_size | MPIDI_DYNPROC_MASK;
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
                            mpi_errno, "remote_lupids");
        MPIR_CHKLMEM_MALLOC(local_lupids, int *, local_size * sizeof(int),
                            mpi_errno, "local_lupids");
        for (i = 0; i < local_size; i++) {
            MPIDIU_comm_rank_to_pid(local_comm, i, &lpid, &avtid);
            local_lupids[i] = MPIDIU_LUPID_CREATE(avtid, lpid);
        }

        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_COMM, VERBOSE,
                        (MPL_DBG_FDEST, "Intercomm map exchange stage 1: leaders"));
        if (!pure_intracomm) {
            /* Stage 1.1 UPID exchange between leaders */
            MPIR_CHKLMEM_MALLOC(remote_upid_size, size_t *, (*remote_size) * sizeof(size_t),
                                mpi_errno, "remote_upid_size");

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
                                mpi_errno, "remote_upids");
            mpi_errno = MPIC_Sendrecv(local_upids, upid_send_size, MPI_BYTE,
                                      remote_leader, cts_tag,
                                      remote_upids, upid_recv_size, MPI_BYTE,
                                      remote_leader, cts_tag,
                                      peer_comm, MPI_STATUS_IGNORE, &errflag);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);

            MPIR_CHKLMEM_MALLOC(local_node_ids, MPID_Node_id_t *,
                                local_size * sizeof(MPID_Node_id_t), mpi_errno, "local_node_ids");
            MPIR_CHKLMEM_MALLOC(remote_node_ids, MPID_Node_id_t *,
                                (*remote_size) * sizeof(MPID_Node_id_t),
                                mpi_errno, "remote_node_ids");
            for (i = 0; i < local_size; i++) {
                MPIDI_CH4U_get_node_id(local_comm, i, &local_node_ids[i]);
            }
            mpi_errno = MPIC_Sendrecv(local_node_ids, local_size * sizeof(MPID_Node_id_t), MPI_BYTE,
                                      remote_leader, cts_tag,
                                      remote_node_ids, (*remote_size) * sizeof(MPID_Node_id_t),
                                      MPI_BYTE, remote_leader, cts_tag, peer_comm,
                                      MPI_STATUS_IGNORE, &errflag);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);

            /* Stage 1.2 convert remote UPID to LUPID and get LUPID for local group */
            MPIDIU_upids_to_lupids(*remote_size, remote_upid_size, remote_upids,
                                  remote_lupids, remote_node_ids);
        }
        else {
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
        }
        else {
            if (local_upid_size[0] == remote_upid_size[0]) {
                *is_low_group = memcmp(local_upids, remote_upids, local_upid_size[0]);
            }
            else {
                *is_low_group = local_upid_size[0] < remote_upid_size[0];
            }
            (*is_low_group) |= MPIDI_DYNPROC_MASK;
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

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
#include "ch4i_comm.h"

MPL_STATIC_INLINE_PREFIX int MPIDI_Comm_AS_enabled(MPIR_Comm * comm)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Comm_reenable_anysource(MPIR_Comm * comm,
                                                           MPIR_Group ** failed_group_ptr)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Comm_remote_group_failed(MPIR_Comm * comm,
                                                            MPIR_Group ** failed_group_ptr)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Comm_group_failed(MPIR_Comm * comm_ptr,
                                                     MPIR_Group ** failed_group_ptr)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Comm_failure_ack(MPIR_Comm * comm_ptr)
{
    MPIR_Assert(0);
    return 0;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Comm_failure_get_acked(MPIR_Comm * comm_ptr,
                                                          MPIR_Group ** failed_group_ptr)
{
    MPIR_Assert(0);
    return 0;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Comm_revoke(MPIR_Comm * comm_ptr, int is_remote)
{
    MPIR_Assert(0);
    return 0;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Comm_get_all_failed_procs(MPIR_Comm * comm_ptr,
                                                             MPIR_Group ** failed_group, int tag)
{
    MPIR_Assert(0);
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

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4_COMM_SPLIT_TYPE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4_COMM_SPLIT_TYPE);

    if (split_type == MPI_COMM_TYPE_SHARED) {
        MPIDI_Comm_get_lpid(comm_ptr, comm_ptr->rank, &idx, FALSE);
        MPIDI_Get_node_id(comm_ptr, comm_ptr->rank, &node_id);
        mpi_errno = MPIR_Comm_split_impl(comm_ptr, node_id, key, newcomm_ptr);
    }
    else
        mpi_errno = MPIR_Comm_split_impl(comm_ptr, MPI_UNDEFINED, key, newcomm_ptr);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4_COMM_SPLIT_TYPE);
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
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4_COMM_CREATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4_COMM_CREATE);
    mpi_errno = MPIDI_NM_mpi_comm_create_hook(comm);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
#if defined(MPIDI_BUILD_CH4_SHM)
    mpi_errno = MPIDI_SHM_mpi_comm_create_hook(comm);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
#endif

    /* comm_world and comm_self are already initialized */
    if (comm != MPIR_Process.comm_world && comm != MPIR_Process.comm_self) {
        MPIDII_comm_create_rank_map(comm);
        /* add ref to avts */
        switch (MPIDII_COMM(comm, map).mode) {
        case MPIDII_RANK_MAP_NONE:
            break;
        case MPIDII_RANK_MAP_MLUT:
            max_n_avts = MPIDIU_get_max_n_avts();
            uniq_avtids = (int *) MPL_malloc(max_n_avts * sizeof(int));
            memset(uniq_avtids, 0, max_n_avts);
            for (i = 0; i < MPIDII_COMM(comm, map).size; i++) {
                if (uniq_avtids[MPIDII_COMM(comm, map).irreg.mlut.gpid[i].avtid] == 0) {
                    uniq_avtids[MPIDII_COMM(comm, map).irreg.mlut.gpid[i].avtid] = 1;
                    MPIDIU_avt_add_ref(MPIDII_COMM(comm, map).irreg.mlut.gpid[i].avtid);
                }
            }
            MPL_free(uniq_avtids);
            break;
        default:
            MPIDIU_avt_add_ref(MPIDII_COMM(comm, map).avtid);
        }

        switch (MPIDII_COMM(comm, local_map).mode) {
        case MPIDII_RANK_MAP_NONE:
            break;
        case MPIDII_RANK_MAP_MLUT:
            max_n_avts = MPIDIU_get_max_n_avts();
            uniq_avtids = (int *) MPL_malloc(max_n_avts * sizeof(int));
            memset(uniq_avtids, 0, max_n_avts);
            for (i = 0; i < MPIDII_COMM(comm, local_map).size; i++) {
                if (uniq_avtids[MPIDII_COMM(comm, local_map).irreg.mlut.gpid[i].avtid] == 0) {
                    uniq_avtids[MPIDII_COMM(comm, local_map).irreg.mlut.gpid[i].avtid] = 1;
                    MPIDIU_avt_add_ref(MPIDII_COMM(comm, local_map).irreg.mlut.gpid[i].avtid);
                }
            }
            MPL_free(uniq_avtids);
            break;
        default:
            MPIDIU_avt_add_ref(MPIDII_COMM(comm, local_map).avtid);
        }
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4_COMM_CREATE);
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
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4_COMM_DESTROY);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4_COMM_DESTROY);
    /* release ref to avts */
    switch (MPIDII_COMM(comm, map).mode) {
    case MPIDII_RANK_MAP_NONE:
        break;
    case MPIDII_RANK_MAP_MLUT:
        max_n_avts = MPIDIU_get_max_n_avts();
        uniq_avtids = (int *) MPL_malloc(max_n_avts * sizeof(int));
        memset(uniq_avtids, 0, max_n_avts);
        for (i = 0; i < MPIDII_COMM(comm, map).size; i++) {
            if (uniq_avtids[MPIDII_COMM(comm, map).irreg.mlut.gpid[i].avtid] == 0) {
                uniq_avtids[MPIDII_COMM(comm, map).irreg.mlut.gpid[i].avtid] = 1;
                MPIDIU_avt_release_ref(MPIDII_COMM(comm, map).irreg.mlut.gpid[i].avtid);
            }
        }
        MPL_free(uniq_avtids);
        break;
    default:
        MPIDIU_avt_release_ref(MPIDII_COMM(comm, map).avtid);
    }

    switch (MPIDII_COMM(comm, local_map).mode) {
    case MPIDII_RANK_MAP_NONE:
        break;
    case MPIDII_RANK_MAP_MLUT:
        max_n_avts = MPIDIU_get_max_n_avts();
        uniq_avtids = (int *) MPL_malloc(max_n_avts * sizeof(int));
        memset(uniq_avtids, 0, max_n_avts);
        for (i = 0; i < MPIDII_COMM(comm, local_map).size; i++) {
            if (uniq_avtids[MPIDII_COMM(comm, local_map).irreg.mlut.gpid[i].avtid] == 0) {
                uniq_avtids[MPIDII_COMM(comm, local_map).irreg.mlut.gpid[i].avtid] = 1;
                MPIDIU_avt_release_ref(MPIDII_COMM(comm, local_map).irreg.mlut.gpid[i].avtid);
            }
        }
        MPL_free(uniq_avtids);
        break;
    default:
        MPIDIU_avt_release_ref(MPIDII_COMM(comm, local_map).avtid);
    }

    mpi_errno = MPIDI_NM_mpi_comm_free_hook(comm);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
#if defined(MPIDI_BUILD_CH4_SHM)
    mpi_errno = MPIDI_SHM_mpi_comm_free_hook(comm);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
#endif

    if (MPIDII_COMM(comm, map).mode == MPIDII_RANK_MAP_LUT
        || MPIDII_COMM(comm, map).mode == MPIDII_RANK_MAP_LUT_INTRA) {
        MPIDIU_release_lut(MPIDII_COMM(comm, map).irreg.lut.t);
    }
    if (MPIDII_COMM(comm, local_map).mode == MPIDII_RANK_MAP_LUT
        || MPIDII_COMM(comm, local_map).mode == MPIDII_RANK_MAP_LUT_INTRA) {
        MPIDIU_release_lut(MPIDII_COMM(comm, local_map).irreg.lut.t);
    }
    if (MPIDII_COMM(comm, map).mode == MPIDII_RANK_MAP_MLUT) {
        MPIDIU_release_mlut(MPIDII_COMM(comm, map).irreg.mlut.t);
    }
    if (MPIDII_COMM(comm, local_map).mode == MPIDII_RANK_MAP_MLUT) {
        MPIDIU_release_mlut(MPIDII_COMM(comm, local_map).irreg.mlut.t);
    }
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4_COMM_DESTROY);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_intercomm_upid_lupid_bcast_intra
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_intercomm_map_bcast_intra(MPIR_Comm *local_comm,
                                                             int local_leader,
                                                             int *remote_size,
                                                             int *is_low_group,
                                                             int pure_intracomm,
                                                             size_t *remote_upid_size,
                                                             char *remote_upids,
                                                             int **remote_lupids,
                                                             MPID_Node_id_t *remote_node_ids)
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    int upid_recv_size = 0;
    int map_info[4];
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;

    if (local_comm->rank == local_leader) {
        if (!pure_intracomm) {
            for (i = 0; i < (*remote_size); i++) {
                upid_recv_size += remote_upid_size[i];
            }
        }
        map_info[0] = *remote_size;
        map_info[1] = upid_recv_size;
        map_info[2] = *is_low_group;
        map_info[3] = pure_intracomm;
        mpi_errno = MPIR_Bcast_intra(map_info, 4, MPI_INT, local_leader, local_comm, &errflag );
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);

        if (!pure_intracomm) {
            mpi_errno = MPIR_Bcast_intra(remote_upid_size, *remote_size, MPI_UNSIGNED_LONG,
                                         local_leader, local_comm, &errflag );
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            mpi_errno = MPIR_Bcast_intra(remote_upids, upid_recv_size, MPI_BYTE,
                                         local_leader, local_comm, &errflag );
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            mpi_errno = MPIR_Bcast_intra(remote_node_ids, (*remote_size)*sizeof(MPID_Node_id_t), MPI_BYTE,
                                         local_leader, local_comm, &errflag );
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        } else {
            mpi_errno = MPIR_Bcast_intra(*remote_lupids, *remote_size, MPI_INT,
                                         local_leader, local_comm, &errflag);
        }
    } else {
        mpi_errno = MPIR_Bcast_intra(map_info, 4, MPI_INT, local_leader, local_comm, &errflag );
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        *remote_size = map_info[0];
        upid_recv_size = map_info[1];
        *is_low_group = map_info[2];
        pure_intracomm = map_info[3];

        (*remote_lupids) = (int*) MPL_malloc((*remote_size) * sizeof(int));
        if (!pure_intracomm) {
            size_t *_remote_upid_size = NULL;
            _remote_upid_size = (size_t*) MPL_malloc((*remote_size) * sizeof(size_t));
            mpi_errno = MPIR_Bcast_intra(_remote_upid_size, *remote_size, MPI_UNSIGNED_LONG,
                                         local_leader, local_comm, &errflag );
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            remote_upids = (char*) MPL_malloc(upid_recv_size * sizeof(char));
            mpi_errno = MPIR_Bcast_intra(remote_upids, upid_recv_size, MPI_BYTE,
                                         local_leader, local_comm, &errflag );
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            mpi_errno = MPIR_Bcast_intra(remote_node_ids, (*remote_size)*sizeof(MPID_Node_id_t), MPI_BYTE,
                                         local_leader, local_comm, &errflag );
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);

            MPIDI_upids_to_lupids(*remote_size, _remote_upid_size, remote_upids,
                                  remote_lupids, remote_node_ids);
            MPL_free(_remote_upid_size);
        } else {
            mpi_errno = MPIR_Bcast_intra(*remote_lupids, *remote_size, MPI_INT,
                                         local_leader, local_comm, &errflag);
        }
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_intercomm_exchange_map
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_intercomm_exchange_map(MPIR_Comm *local_comm,
                                                          int local_leader,
                                                          MPIR_Comm *peer_comm,
                                                          int remote_leader,
                                                          int *remote_size,
                                                          int **remote_lupids,
                                                          int *is_low_group)
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    int avtid = 0, lpid;
    int local_avtid = 0, remote_avtid = 0;
    int local_size_send, remote_size_recv;
    int cts_tag;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    int pure_intracomm = 0;
    int local_size;
    MPID_Node_id_t *local_node_ids, *remote_node_ids;
    int *local_lupids = NULL;
    size_t *local_upid_size = NULL, *remote_upid_size = NULL;
    int upid_send_size = 0, upid_recv_size = 0;
    char *local_upids = NULL, *remote_upids = NULL;

    /*
     * CH4 only cares about LUPID. GUPID extraction and exchange should be done
     * by netmod
     */
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4_INTERCOMM_XCHG_MAP);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4_INTERCOMM_XCHG_MAP);

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
        } else {
            local_size_send = local_size | MPIDII_DYNPROC_MASK;
        }

        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_COMM,VERBOSE,
                        (MPL_DBG_FDEST,"rank %d sendrecv to rank %d",
                         peer_comm->rank, remote_leader));
        mpi_errno = MPIC_Sendrecv(&local_size_send, 1, MPI_INT,
                                  remote_leader, cts_tag,
                                  &remote_size_recv, 1, MPI_INT,
                                  remote_leader, cts_tag,
                                  peer_comm, MPI_STATUS_IGNORE, &errflag);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);

        if (remote_size_recv & MPIDII_DYNPROC_MASK) pure_intracomm = 0;
        (*remote_size) = remote_size_recv & (~MPIDII_DYNPROC_MASK);


        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_COMM,VERBOSE,
                        (MPL_DBG_FDEST, "local size = %d, remote size = %d, pure_intracomm = %d",
                         local_size, *remote_size, pure_intracomm));

        (*remote_lupids) = (int*) MPL_malloc((*remote_size) * sizeof(int));
        local_lupids = (int*) MPL_malloc(local_size * sizeof(int));
        for (i = 0; i < local_size; i++) {
            MPIDIU_comm_rank_to_pid(local_comm, i, &lpid, &avtid);
            local_lupids[i] = MPIDIU_LUPID_CREATE(avtid, lpid);
        }

        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_COMM,VERBOSE,
                        (MPL_DBG_FDEST, "Intercomm map exchange stage 1: leaders"));
        if (!pure_intracomm) {
            /* Stage 1.1 UPID exchange between leaders */
            remote_upid_size = (size_t*) MPL_malloc((*remote_size) * sizeof(size_t));

            mpi_errno = MPIDI_NM_get_local_upids(local_comm, &local_upid_size, &local_upids);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            mpi_errno = MPIC_Sendrecv(local_upid_size, local_size, MPI_UNSIGNED_LONG,
                                      remote_leader, cts_tag,
                                      remote_upid_size, *remote_size, MPI_UNSIGNED_LONG,
                                      remote_leader, cts_tag,
                                      peer_comm, MPI_STATUS_IGNORE, &errflag);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            upid_send_size = 0;
            for (i = 0; i < local_size; i++) upid_send_size += local_upid_size[i];
            upid_recv_size = 0;
            for (i = 0; i < *remote_size; i++) upid_recv_size += remote_upid_size[i];
            remote_upids = (char*) MPL_malloc(upid_recv_size * sizeof(char));
            mpi_errno = MPIC_Sendrecv(local_upids, upid_send_size, MPI_BYTE,
                                      remote_leader, cts_tag,
                                      remote_upids, upid_recv_size, MPI_BYTE,
                                      remote_leader, cts_tag,
                                      peer_comm, MPI_STATUS_IGNORE, &errflag);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);

            local_node_ids = (MPID_Node_id_t*) MPL_malloc(local_size * sizeof(MPID_Node_id_t));
            remote_node_ids = (MPID_Node_id_t*) MPL_malloc((*remote_size) * sizeof(MPID_Node_id_t));
            for (i = 0; i < local_size; i++) {
                MPIDI_CH4U_get_node_id(local_comm, i, &local_node_ids[i]);
            }
            mpi_errno = MPIC_Sendrecv(local_node_ids, local_size*sizeof(MPID_Node_id_t), MPI_BYTE,
                                      remote_leader, cts_tag,
                                      remote_node_ids, (*remote_size)*sizeof(MPID_Node_id_t), MPI_BYTE,
                                      remote_leader, cts_tag,
                                      peer_comm, MPI_STATUS_IGNORE, &errflag);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);

            /* Stage 1.2 convert remote UPID to LUPID and get LUPID for local group */
            MPIDI_upids_to_lupids(*remote_size, remote_upid_size, remote_upids,
                                     remote_lupids, remote_node_ids);
            MPL_free(local_node_ids);
            MPL_free(remote_node_ids);
        } else {
            /* Stage 1.1f only exchange LUPIDS if no dynamic process involved */
            mpi_errno = MPIC_Sendrecv(local_lupids, local_size, MPI_INT,
                                      remote_leader, cts_tag,
                                      *remote_lupids, *remote_size, MPI_INT,
                                      remote_leader, cts_tag,
                                      peer_comm, MPI_STATUS_IGNORE, &errflag);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
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
                   check for any overlap */
                mpi_errno = MPIDII_check_disjoint_lupids(local_lupids, local_size,
                                                         *remote_lupids, *remote_size);
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);
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
            }
            else {
                *is_low_group = local_upid_size[0] < remote_upid_size[0];
            }
            (*is_low_group) |= MPIDII_DYNPROC_MASK;
        }

        /* At this point, we're done with the local lpids; they'll
           be freed with the other local memory on exit */
        MPL_free(local_lupids);
        local_lupids = NULL;
    }

    /*
     * Stage 2. Bcast UPID to non-leaders (intra-group)
     */
    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_COMM,VERBOSE,
                    (MPL_DBG_FDEST, "Intercomm map exchange stage 2: intra-group"));
    mpi_errno = MPIDI_intercomm_map_bcast_intra(local_comm, local_leader,
                                                remote_size, is_low_group, pure_intracomm,
                                                remote_upid_size, remote_upids,
                                                remote_lupids, remote_node_ids);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

  fn_exit:
    /* free resource */
    if (local_upid_size) MPL_free(local_upid_size);
    if (remote_upid_size) MPL_free(remote_upid_size);
    if (local_upids) MPL_free(local_upids);
    if (remote_upids) MPL_free(remote_upids);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4_INTERCOMM_XCHG_MAP);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* CH4_COMM_H_INCLUDED */

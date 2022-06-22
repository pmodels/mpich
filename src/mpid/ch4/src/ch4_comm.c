/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "mpidch4r.h"
#include "ch4_comm.h"
#include "ch4i_comm.h"


int MPID_Comm_reenable_anysource(MPIR_Comm * comm, MPIR_Group ** failed_group_ptr)
{
    MPIR_FUNC_ENTER;

    MPIR_Assert(0);

    MPIR_FUNC_EXIT;
    return MPI_SUCCESS;
}

int MPID_Comm_remote_group_failed(MPIR_Comm * comm, MPIR_Group ** failed_group_ptr)
{
    MPIR_FUNC_ENTER;

    MPIR_Assert(0);

    MPIR_FUNC_EXIT;
    return MPI_SUCCESS;
}

int MPID_Comm_group_failed(MPIR_Comm * comm_ptr, MPIR_Group ** failed_group_ptr)
{
    MPIR_FUNC_ENTER;

    MPIR_Assert(0);

    MPIR_FUNC_EXIT;
    return MPI_SUCCESS;
}

int MPID_Comm_failure_ack(MPIR_Comm * comm_ptr)
{
    MPIR_FUNC_ENTER;

    MPIR_Assert(0);

    MPIR_FUNC_EXIT;
    return 0;
}

int MPID_Comm_failure_get_acked(MPIR_Comm * comm_ptr, MPIR_Group ** failed_group_ptr)
{
    MPIR_FUNC_ENTER;

    MPIR_Assert(0);

    MPIR_FUNC_EXIT;
    return 0;
}

int MPID_Comm_revoke(MPIR_Comm * comm_ptr, int is_remote)
{
    MPIR_FUNC_ENTER;

    MPIR_Assert(0);

    MPIR_FUNC_EXIT;
    return 0;
}

int MPID_Comm_get_all_failed_procs(MPIR_Comm * comm_ptr, MPIR_Group ** failed_group, int tag)
{
    MPIR_FUNC_ENTER;

    MPIR_Assert(0);

    MPIR_FUNC_EXIT;
    return 0;
}

int MPIDI_Comm_split_type(MPIR_Comm * user_comm_ptr, int split_type, int key, MPIR_Info * info_ptr,
                          MPIR_Comm ** newcomm_ptr)
{
    MPIR_Comm *comm_ptr = NULL;
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    mpi_errno = MPIR_Comm_split_impl(user_comm_ptr, split_type == MPI_UNDEFINED ? MPI_UNDEFINED : 0,
                                     key, &comm_ptr);
    MPIR_ERR_CHECK(mpi_errno);

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

    mpi_errno = MPIR_Comm_split_type_node_topo(comm_ptr, key, info_ptr, newcomm_ptr);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    if (comm_ptr)
        MPIR_Comm_free_impl(comm_ptr);
    MPIR_FUNC_EXIT;
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

static void mlut_update_avt_reference(int size, MPIDI_gpid_t * gpid, bool is_release)
{
    int n_avts = MPIDIU_get_n_avts();
    int *uniq_avtids = (int *) MPL_calloc(n_avts, sizeof(int), MPL_MEM_ADDRESS);
    for (int i = 0; i < size; i++) {
        if (uniq_avtids[gpid[i].avtid] == 0) {
            uniq_avtids[gpid[i].avtid] = 1;
            if (is_release) {
                MPIDIU_avt_release_ref(gpid[i].avtid);
            } else {
                MPIDIU_avt_add_ref(gpid[i].avtid);
            }
        }
    }
    MPL_free(uniq_avtids);
}

int MPID_Comm_commit_pre_hook(MPIR_Comm * comm)
{
    int mpi_errno;
    MPIR_FUNC_ENTER;

    if (comm == MPIR_Process.comm_world) {
        MPIDI_COMM(comm, map).mode = MPIDI_RANK_MAP_DIRECT_INTRA;
        MPIDI_COMM(comm, map).avtid = 0;
        MPIDI_COMM(comm, map).size = MPIR_Process.size;
        MPIDI_COMM(comm, local_map).mode = MPIDI_RANK_MAP_NONE;
        MPIDIU_avt_add_ref(0);

        mpi_errno = MPIDU_Init_shm_init();
        MPIR_ERR_CHECK(mpi_errno);

#ifndef MPIDI_CH4_DIRECT_NETMOD
        mpi_errno = MPIDI_SHM_init_world();
        MPIR_ERR_CHECK(mpi_errno);
#endif
        mpi_errno = MPIDI_NM_init_world();
        MPIR_ERR_CHECK(mpi_errno);
    } else if (comm == MPIR_Process.comm_self) {
        MPIDI_COMM(comm, map).mode = MPIDI_RANK_MAP_OFFSET_INTRA;
        MPIDI_COMM(comm, map).avtid = 0;
        MPIDI_COMM(comm, map).size = 1;
        MPIDI_COMM(comm, map).reg.offset = MPIR_Process.rank;
        MPIDI_COMM(comm, local_map).mode = MPIDI_RANK_MAP_NONE;
        MPIDIU_avt_add_ref(0);
    } else {
        MPIDI_comm_create_rank_map(comm);
        /* add ref to avts */
        switch (MPIDI_COMM(comm, map).mode) {
            case MPIDI_RANK_MAP_NONE:
                break;
            case MPIDI_RANK_MAP_MLUT:
                mlut_update_avt_reference(MPIDI_COMM(comm, map).size,
                                          MPIDI_COMM(comm, map).irreg.mlut.gpid, false);
                break;
            default:
                MPIDIU_avt_add_ref(MPIDI_COMM(comm, map).avtid);
        }

        switch (MPIDI_COMM(comm, local_map).mode) {
            case MPIDI_RANK_MAP_NONE:
                break;
            case MPIDI_RANK_MAP_MLUT:
                mlut_update_avt_reference(MPIDI_COMM(comm, local_map).size,
                                          MPIDI_COMM(comm, local_map).irreg.mlut.gpid, false);
                break;
            default:
                MPIDIU_avt_add_ref(MPIDI_COMM(comm, local_map).avtid);
        }
    }

    MPIDI_COMM(comm, multi_leads_comm) = NULL;
    MPIDI_COMM(comm, inter_node_leads_comm) = NULL;
    MPIDI_COMM(comm, sub_node_comm) = NULL;
    MPIDI_COMM(comm, intra_node_leads_comm) = NULL;
    MPIDI_COMM(comm, spanned_num_nodes) = -1;
    MPIDI_COMM(comm, alltoall_comp_info) = NULL;
    MPIDI_COMM(comm, allgather_comp_info) = NULL;
    MPIDI_COMM(comm, allreduce_comp_info) = NULL;

    mpi_errno = MPIDIG_init_comm(comm);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIDI_NM_mpi_comm_commit_pre_hook(comm);
    MPIR_ERR_CHECK(mpi_errno);
#ifndef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_SHM_mpi_comm_commit_pre_hook(comm);
    MPIR_ERR_CHECK(mpi_errno);
#endif

#ifdef HAVE_DEBUGGER_SUPPORT
    MPIDIG_COMM(comm, posted_head_ptr) = &(MPIDI_global.per_vci[0].posted_list);
    MPIDIG_COMM(comm, unexp_head_ptr) = &(MPIDI_global.per_vci[0].unexp_list);
#endif
  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPID_Comm_commit_post_hook(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    if (comm == MPIR_Process.comm_world) {
        mpi_errno = MPIDI_NM_post_init();
        MPIR_ERR_CHECK(mpi_errno);

        MPIDI_global.is_initialized = 1;
    }

    mpi_errno = MPIDI_NM_mpi_comm_commit_post_hook(comm);
    MPIR_ERR_CHECK(mpi_errno);
#ifndef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_SHM_mpi_comm_commit_post_hook(comm);
    MPIR_ERR_CHECK(mpi_errno);
#endif

    /* prune selection tree */
    mpi_errno = MPIR_Csel_prune(MPIDI_global.csel_root, comm, &MPIDI_COMM(comm, csel_comm));
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPID_Comm_free_hook(MPIR_Comm * comm)
{
    int mpi_errno;
    MPIR_FUNC_ENTER;

    if (MPIDI_COMM(comm, multi_leads_comm) != NULL) {
        MPIR_Comm_release(MPIDI_COMM(comm, multi_leads_comm));
    }

    if (MPIDI_COMM(comm, inter_node_leads_comm) != NULL) {
        MPIR_Comm_release(MPIDI_COMM(comm, inter_node_leads_comm));
    }

    if (MPIDI_COMM(comm, sub_node_comm) != NULL) {
        MPIR_Comm_release(MPIDI_COMM(comm, sub_node_comm));
    }

    if (MPIDI_COMM(comm, intra_node_leads_comm) != NULL) {
        MPIR_Comm_release(MPIDI_COMM(comm, intra_node_leads_comm));
    }


    if (MPIDI_COMM(comm, alltoall_comp_info) != NULL) {
        /* Destroy the associated shared memory region used by multi-leads Alltoall */
        if (MPIDI_COMM_ALLTOALL(comm, shm_addr) != NULL) {
            mpi_errno = MPIDU_shm_free(MPIDI_COMM_ALLTOALL(comm, shm_addr));
            if (mpi_errno != MPI_SUCCESS) {
                MPIR_ERR_POP(mpi_errno);
            }
        }
        MPL_free(MPIDI_COMM(comm, alltoall_comp_info));
    }
    if (MPIDI_COMM(comm, allgather_comp_info) != NULL) {
        /* Destroy the associated shared memory region used by multi-leads Allgather */
        if (MPIDI_COMM_ALLGATHER(comm, shm_addr) != NULL) {
            mpi_errno = MPIDU_shm_free(MPIDI_COMM_ALLGATHER(comm, shm_addr));
            if (mpi_errno != MPI_SUCCESS) {
                MPIR_ERR_POP(mpi_errno);
            }
        }
        MPL_free(MPIDI_COMM(comm, allgather_comp_info));
    }
    if (MPIDI_COMM(comm, allreduce_comp_info) != NULL) {
        /* Destroy the associated shared memory region used by multi-leads Allreduce */
        if (MPIDI_COMM_ALLREDUCE(comm, shm_addr) != NULL) {
            mpi_errno = MPIDU_shm_free(MPIDI_COMM_ALLREDUCE(comm, shm_addr));
            if (mpi_errno != MPI_SUCCESS) {
                MPIR_ERR_POP(mpi_errno);
            }
        }
        MPL_free(MPIDI_COMM(comm, allreduce_comp_info));
    }



    /* release ref to avts */
    switch (MPIDI_COMM(comm, map).mode) {
        case MPIDI_RANK_MAP_NONE:
            break;
        case MPIDI_RANK_MAP_MLUT:
            mlut_update_avt_reference(MPIDI_COMM(comm, map).size,
                                      MPIDI_COMM(comm, map).irreg.mlut.gpid, true);
            break;
        default:
            MPIDIU_avt_release_ref(MPIDI_COMM(comm, map).avtid);
    }

    switch (MPIDI_COMM(comm, local_map).mode) {
        case MPIDI_RANK_MAP_NONE:
            break;
        case MPIDI_RANK_MAP_MLUT:
            mlut_update_avt_reference(MPIDI_COMM(comm, local_map).size,
                                      MPIDI_COMM(comm, local_map).irreg.mlut.gpid, true);
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
    MPIR_ERR_CHECK(mpi_errno);
#ifndef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_SHM_mpi_comm_free_hook(comm);
    MPIR_ERR_CHECK(mpi_errno);
#endif

    if (MPIDI_COMM(comm, csel_comm)) {
        mpi_errno = MPIR_Csel_free(MPIDI_COMM(comm, csel_comm));
        MPIR_ERR_CHECK(mpi_errno);
    }

    mpi_errno = MPIDIG_destroy_comm(comm);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPID_Comm_set_hints(MPIR_Comm * comm_ptr, MPIR_Info * info_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIDI_NM_comm_set_hints(comm_ptr, info_ptr);
    MPIR_ERR_CHECK(mpi_errno);
#ifndef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_SHM_comm_set_hints(comm_ptr, info_ptr);
    MPIR_ERR_CHECK(mpi_errno);
#endif

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPID_Intercomm_exchange_map(MPIR_Comm * local_comm, int local_leader, MPIR_Comm * peer_comm,
                                int remote_leader, int *remote_size, uint64_t ** remote_gpids,
                                int *is_low_group)
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
    uint64_t *local_gpids = NULL;
    int *local_upid_size = NULL, *remote_upid_size = NULL;
    int upid_send_size = 0, upid_recv_size = 0;
    char *local_upids = NULL, *remote_upids = NULL;

    /*
     * CH4 only cares about GPID. UPID extraction and exchange should be done
     * by netmod
     */
    MPIR_FUNC_ENTER;

    MPIR_CHKPMEM_DECL(1);
    MPIR_CHKLMEM_DECL(5);

    cts_tag = 0 | MPIR_TAG_COLL_BIT;
    local_size = local_comm->local_size;

    /*
     * Stage 1: UPID exchange and GPID conversion in leaders
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
        MPIR_ERR_CHECK(mpi_errno);

        if (remote_size_recv & MPIDI_DYNPROC_MASK)
            pure_intracomm = 0;
        (*remote_size) = remote_size_recv & (~MPIDI_DYNPROC_MASK);

        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_COMM, VERBOSE,
                        (MPL_DBG_FDEST, "local size = %d, remote size = %d, pure_intracomm = %d",
                         local_size, *remote_size, pure_intracomm));

        MPIR_CHKPMEM_MALLOC((*remote_gpids), uint64_t *, (*remote_size) * sizeof(uint64_t),
                            mpi_errno, "remote_gpids", MPL_MEM_ADDRESS);
        MPIR_CHKLMEM_MALLOC(local_gpids, uint64_t *, local_size * sizeof(uint64_t),
                            mpi_errno, "local_gpids", MPL_MEM_ADDRESS);
        for (i = 0; i < local_size; i++) {
            MPIDIU_comm_rank_to_pid(local_comm, i, &lpid, &avtid);
            local_gpids[i] = MPIDIU_GPID_CREATE(avtid, lpid);
        }

        /* TODO: optimizations --
         *       if local_size is 1, we can skip send and local bcast;
         *       if remote_size is 1, we can skip recv.
         */
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_COMM, VERBOSE,
                        (MPL_DBG_FDEST, "Intercomm map exchange stage 1: leaders"));
        if (!pure_intracomm) {
            /* Stage 1.1 UPID exchange between leaders */
            MPIR_CHKLMEM_MALLOC(remote_upid_size, int *, (*remote_size) * sizeof(int),
                                mpi_errno, "remote_upid_size", MPL_MEM_ADDRESS);

            mpi_errno = MPIDI_NM_get_local_upids(local_comm, &local_upid_size, &local_upids);
            MPIR_ERR_CHECK(mpi_errno);
            mpi_errno = MPIC_Sendrecv(local_upid_size, local_size, MPI_INT,
                                      remote_leader, cts_tag,
                                      remote_upid_size, *remote_size, MPI_INT,
                                      remote_leader, cts_tag,
                                      peer_comm, MPI_STATUS_IGNORE, &errflag);
            MPIR_ERR_CHECK(mpi_errno);
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
            MPIR_ERR_CHECK(mpi_errno);

            /* Stage 1.2 convert remote UPID to GPID and get GPID for local group */
            MPIDIU_upids_to_gpids(*remote_size, remote_upid_size, remote_upids, *remote_gpids);
        } else {
            /* Stage 1.1f only exchange GPIDS if no dynamic process involved */
            mpi_errno = MPIC_Sendrecv(local_gpids, local_size, MPI_UINT64_T,
                                      remote_leader, cts_tag,
                                      *remote_gpids, *remote_size, MPI_UINT64_T,
                                      remote_leader, cts_tag,
                                      peer_comm, MPI_STATUS_IGNORE, &errflag);
            MPIR_ERR_CHECK(mpi_errno);
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
                mpi_errno = MPIDI_check_disjoint_gpids(local_gpids, local_size,
                                                       *remote_gpids, *remote_size);
                MPIR_ERR_CHECK(mpi_errno);
            }
            MPID_END_ERROR_CHECKS;
        }
#endif /* HAVE_ERROR_CHECKING */

        /*
         * Make an arbitrary decision about which group of process is
         * the low group.  The LEADERS do this by comparing the
         * local process ids of the 0th member of the two groups
         * GPID itself is not enough for determine is_low_group because both
         * local group is always smaller than remote
         */
        if (pure_intracomm) {
            *is_low_group = local_gpids[0] < (*remote_gpids)[0];
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
        local_gpids = NULL;
    }

    /*
     * Stage 2. Bcast UPID to non-leaders (intra-group)
     */
    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_COMM, VERBOSE,
                    (MPL_DBG_FDEST, "Intercomm map exchange stage 2: intra-group"));
    mpi_errno = MPIDIU_Intercomm_map_bcast_intra(local_comm, local_leader,
                                                 remote_size, is_low_group, pure_intracomm,
                                                 remote_upid_size, remote_upids, remote_gpids);
    MPIR_ERR_CHECK(mpi_errno);

    MPIR_CHKPMEM_COMMIT();
  fn_exit:
    MPL_free(local_upid_size);
    MPL_free(local_upids);
    MPIR_CHKLMEM_FREEALL();
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    MPIR_CHKPMEM_REAP();
    *remote_gpids = NULL;
    goto fn_exit;
}

int MPIDIU_Intercomm_map_bcast_intra(MPIR_Comm * local_comm, int local_leader, int *remote_size,
                                     int *is_low_group, int pure_intracomm,
                                     int *remote_upid_size, char *remote_upids,
                                     uint64_t ** remote_gpids)
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    int upid_recv_size = 0;
    int map_info[4];
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    int *_remote_upid_size = NULL;
    char *_remote_upids = NULL;

    MPIR_CHKPMEM_DECL(1);
    MPIR_CHKLMEM_DECL(3);

    MPIR_FUNC_ENTER;

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
        mpi_errno =
            MPIR_Bcast_allcomm_auto(map_info, 4, MPI_INT, local_leader, local_comm, &errflag);
        MPIR_ERR_CHECK(mpi_errno);

        if (!pure_intracomm) {
            mpi_errno = MPIR_Bcast_allcomm_auto(remote_upid_size, *remote_size, MPI_INT,
                                                local_leader, local_comm, &errflag);
            MPIR_ERR_CHECK(mpi_errno);
            mpi_errno = MPIR_Bcast_allcomm_auto(remote_upids, upid_recv_size, MPI_BYTE,
                                                local_leader, local_comm, &errflag);
            MPIR_ERR_CHECK(mpi_errno);
        } else {
            mpi_errno = MPIR_Bcast_allcomm_auto(*remote_gpids, *remote_size, MPI_UINT64_T,
                                                local_leader, local_comm, &errflag);
        }
    } else {
        mpi_errno =
            MPIR_Bcast_allcomm_auto(map_info, 4, MPI_INT, local_leader, local_comm, &errflag);
        MPIR_ERR_CHECK(mpi_errno);
        *remote_size = map_info[0];
        upid_recv_size = map_info[1];
        *is_low_group = map_info[2];
        pure_intracomm = map_info[3];

        MPIR_CHKPMEM_MALLOC((*remote_gpids), uint64_t *, (*remote_size) * sizeof(uint64_t),
                            mpi_errno, "remote_gpids", MPL_MEM_COMM);
        if (!pure_intracomm) {
            MPIR_CHKLMEM_MALLOC(_remote_upid_size, int *, (*remote_size) * sizeof(int),
                                mpi_errno, "_remote_upid_size", MPL_MEM_COMM);
            mpi_errno = MPIR_Bcast_allcomm_auto(_remote_upid_size, *remote_size, MPI_INT,
                                                local_leader, local_comm, &errflag);
            MPIR_ERR_CHECK(mpi_errno);
            MPIR_CHKLMEM_MALLOC(_remote_upids, char *, upid_recv_size * sizeof(char),
                                mpi_errno, "_remote_upids", MPL_MEM_COMM);
            mpi_errno = MPIR_Bcast_allcomm_auto(_remote_upids, upid_recv_size, MPI_BYTE,
                                                local_leader, local_comm, &errflag);
            MPIR_ERR_CHECK(mpi_errno);

            MPIDIU_upids_to_gpids(*remote_size, _remote_upid_size, _remote_upids, *remote_gpids);
        } else {
            mpi_errno = MPIR_Bcast_allcomm_auto(*remote_gpids, *remote_size, MPI_UINT64_T,
                                                local_leader, local_comm, &errflag);
        }
    }

    MPIR_CHKPMEM_COMMIT();
  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    MPIR_CHKPMEM_REAP();
    *remote_gpids = NULL;
    goto fn_exit;
}

int MPID_Create_intercomm_from_lpids(MPIR_Comm * newcomm_ptr, int size, const uint64_t lpids[])
{
    int mpi_errno = MPI_SUCCESS, i;
    MPIR_FUNC_ENTER;

    MPIDI_rank_map_mlut_t *mlut = NULL;
    MPIDI_COMM(newcomm_ptr, map).mode = MPIDI_RANK_MAP_MLUT;
    MPIDI_COMM(newcomm_ptr, map).avtid = -1;
    mpi_errno = MPIDIU_alloc_mlut(&mlut, size);
    MPIR_ERR_CHECK(mpi_errno);
    MPIDI_COMM(newcomm_ptr, map).size = size;
    MPIDI_COMM(newcomm_ptr, map).irreg.mlut.t = mlut;
    MPIDI_COMM(newcomm_ptr, map).irreg.mlut.gpid = mlut->gpid;

    for (i = 0; i < size; i++) {
        MPIDI_COMM(newcomm_ptr, map).irreg.mlut.gpid[i].avtid = MPIDIU_GPID_GET_AVTID(lpids[i]);
        MPIDI_COMM(newcomm_ptr, map).irreg.mlut.gpid[i].lpid = MPIDIU_GPID_GET_LPID(lpids[i]);
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                        (MPL_DBG_FDEST, " remote rank=%d, avtid=%d, lpid=%d", i,
                         MPIDI_COMM(newcomm_ptr, map).irreg.mlut.gpid[i].avtid,
                         MPIDI_COMM(newcomm_ptr, map).irreg.mlut.gpid[i].lpid));
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* Create multi-leaders communicator */
/* Create a comm with rank 0 of each node. A comm with rank 1 of each node and so on. Since these
 * new comms do no overlap, it uses the same context id */
int MPIDI_Comm_create_multi_leaders(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;
    int num_local = -1, num_external = -1;
    int local_rank = -1, external_rank = -1, rank;
    int i = 0;
    int *local_procs = NULL, *external_procs = NULL;
    int *intranode_table = NULL, *internode_table = NULL;
    MPIR_FUNC_TERSE_ENTER;

    mpi_errno = MPIR_Find_local(comm, &num_local, &local_rank, &local_procs, &intranode_table);
    if (mpi_errno) {
        if (MPIR_Err_is_fatal(mpi_errno))
            MPIR_ERR_POP(mpi_errno);

        MPL_DBG_MSG_P(MPIR_DBG_COMM, VERBOSE,
                      "MPIR_Find_local_and_external failed for comm_ptr=%p", comm);
        if (intranode_table)
            MPL_free(intranode_table);

        mpi_errno = MPI_SUCCESS;
        goto fn_exit;
    }

    mpi_errno = MPIR_Find_external(comm, &num_external, &external_rank, &external_procs,
                                   &internode_table);
    if (mpi_errno) {
        if (MPIR_Err_is_fatal(mpi_errno))
            MPIR_ERR_POP(mpi_errno);

        MPL_DBG_MSG_P(MPIR_DBG_COMM, VERBOSE,
                      "MPIR_Find_local_and_external failed for comm_ptr=%p", comm);
        if (internode_table)
            MPL_free(internode_table);

        mpi_errno = MPI_SUCCESS;
        goto fn_exit;
    }

    MPIR_Assert(num_local > 0);
    MPIR_Assert(num_local > 1 || external_rank >= 0);
    MPIR_Assert(external_rank < 0 || external_procs != NULL);
    rank = MPIR_Comm_rank(comm);

    external_rank = comm->internode_table[rank];
    for (i = 0; i < num_external; ++i) {
        external_procs[i] = i * num_local + local_rank;
    }

    for (i = 0; i < num_local; i++) {
        if (local_rank == i) {
            mpi_errno = MPIR_Comm_create(&MPIDI_COMM(comm, multi_leads_comm));
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);

            MPIDI_COMM(comm, multi_leads_comm)->context_id =
                comm->context_id + MPIR_CONTEXT_MULTILEADS_OFFSET;
            MPIDI_COMM(comm, multi_leads_comm)->recvcontext_id =
                MPIDI_COMM(comm, multi_leads_comm)->context_id;
            MPIDI_COMM(comm, multi_leads_comm)->rank = internode_table[rank];
            MPIDI_COMM(comm, multi_leads_comm)->comm_kind = MPIR_COMM_KIND__INTRACOMM;
            MPIDI_COMM(comm, multi_leads_comm)->hierarchy_kind =
                MPIR_COMM_HIERARCHY_KIND__MULTI_LEADS;
            MPIDI_COMM(comm, multi_leads_comm)->local_comm = NULL;
            MPIDI_COMM(comm, multi_leads_comm)->node_comm = NULL;
            MPIDI_COMM(comm, multi_leads_comm)->node_roots_comm = NULL;
            MPL_DBG_MSG_D(MPIR_DBG_COMM, VERBOSE, "Create multi-leaders_comm=%p\n",
                          MPIDI_COMM(comm, multi_leads_comm));

            MPIDI_COMM(comm, multi_leads_comm)->local_size = num_external;
            MPIDI_COMM(comm, multi_leads_comm)->coll.pof2 =
                MPL_pof2(MPIDI_COMM(comm, multi_leads_comm)->local_size);
            MPIDI_COMM(comm, multi_leads_comm)->remote_size = num_external;

            MPIR_Comm_map_irregular(MPIDI_COMM(comm, multi_leads_comm), comm,
                                    external_procs, num_external, MPIR_COMM_MAP_DIR__L2L, NULL);

            /* Notify device of communicator creation */
            mpi_errno = MPID_Comm_commit_pre_hook(MPIDI_COMM(comm, multi_leads_comm));
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
            /* don't call MPIR_Comm_commit here */

            /* Create collectives-specific infrastructure */
            mpi_errno = MPIR_Coll_comm_init(MPIDI_COMM(comm, multi_leads_comm));
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);

            MPIR_Comm_map_free(MPIDI_COMM(comm, multi_leads_comm));
        }
    }

  fn_exit:
    if (external_procs != NULL)
        MPL_free(external_procs);
    if (local_procs != NULL)
        MPL_free(local_procs);
    if (intranode_table != NULL)
        MPL_free(intranode_table);
    if (internode_table != NULL)
        MPL_free(internode_table);

    MPIR_FUNC_TERSE_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* Example: Ranks 0-3 on node 0 and 4-7 on node 1. Num_leaders = 2 */
/* This function creates 3 kinds of communicators -
 * 1. Inter-node sub-comms. A comm out of first leaders from each node, next comm out of second
 *    leaders from each node and so on. (Example: (0,4), (2,6))
 * 2. Intra-node sub-communicators. A leader and its followers. (Example: (0,1), (2,3), (4,5), (6,7))
 * 3. Intra-node sub-comm consisting of all the leaders on that node. (Example: (0,2), (4,6))
 */
int MPIDI_Comm_create_multi_leader_subcomms(MPIR_Comm * comm, int num_leaders)
{
    int mpi_errno = MPI_SUCCESS;
    int is_leader_color = MPI_UNDEFINED;
    int sub_node_comm_size;

    MPIR_FUNC_ENTER;

    sub_node_comm_size = MPIR_Comm_size(comm->node_comm) / num_leaders;
    /* If node_comm_size is same as number of leaders, each rank should be a leader */
    if (MPIR_Comm_size(comm->node_comm) == num_leaders)
        is_leader_color = MPIR_Comm_rank(comm->node_comm);
    else if (MPIR_Comm_rank(comm->node_comm) % sub_node_comm_size == 0)
        is_leader_color = MPIR_Comm_rank(comm->node_comm) / sub_node_comm_size;

    /* Create the inter-node leaders sub comms */
    mpi_errno = MPIR_Comm_split_impl(comm, is_leader_color, MPIR_Comm_rank(comm->node_comm),
                                     &(MPIDI_COMM(comm, inter_node_leads_comm)));
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    /* Create the intra-node sub comms */
    mpi_errno =
        MPIR_Comm_split_impl(comm->node_comm, MPIR_Comm_rank(comm->node_comm) / sub_node_comm_size,
                             MPIR_Comm_rank(comm->node_comm), &(MPIDI_COMM(comm, sub_node_comm)));
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    if (MPIR_Comm_rank(comm->node_comm) % sub_node_comm_size == 0)
        is_leader_color = 1;
    else
        is_leader_color = MPI_UNDEFINED;

    /* Create an intra-node comm consisting of all leader ranks on that node */
    mpi_errno = MPIR_Comm_split_impl(comm->node_comm, is_leader_color,
                                     MPIR_Comm_rank(comm->node_comm),
                                     &(MPIDI_COMM(comm, intra_node_leads_comm)));
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

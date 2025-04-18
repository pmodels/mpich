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

int MPID_Comm_commit_pre_hook(MPIR_Comm * comm)
{
    int mpi_errno;
    MPIR_FUNC_ENTER;

    MPIR_Assert(comm->local_group);
    MPIR_Assert(comm->comm_kind == MPIR_COMM_KIND__INTRACOMM || comm->remote_group);

    if (comm == MPIR_Process.comm_world) {
        mpi_errno = MPIDI_world_pre_init();
        MPIR_ERR_CHECK(mpi_errno);
    }

    MPIDI_COMM(comm, multi_leads_comm) = NULL;
    MPIDI_COMM(comm, inter_node_leads_comm) = NULL;
    MPIDI_COMM(comm, sub_node_comm) = NULL;
    MPIDI_COMM(comm, intra_node_leads_comm) = NULL;
    MPIDI_COMM(comm, alltoall_comp_info) = NULL;
    MPIDI_COMM(comm, allgather_comp_info) = NULL;
    MPIDI_COMM(comm, allreduce_comp_info) = NULL;

    mpi_errno = MPIDIG_init_comm(comm);
    MPIR_ERR_CHECK(mpi_errno);

#ifndef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_SHM_mpi_comm_commit_pre_hook(comm);
    MPIR_ERR_CHECK(mpi_errno);
#endif
    mpi_errno = MPIDI_NM_mpi_comm_commit_pre_hook(comm);
    MPIR_ERR_CHECK(mpi_errno);

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
        mpi_errno = MPIDI_world_post_init();
        MPIR_ERR_CHECK(mpi_errno);
    }

    mpi_errno = MPIDI_NM_mpi_comm_commit_post_hook(comm);
    MPIR_ERR_CHECK(mpi_errno);
#ifndef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_SHM_mpi_comm_commit_post_hook(comm);
    MPIR_ERR_CHECK(mpi_errno);
#endif

    /* set_vcis for comm_world.
     * TODO: expose MPIX_Comm_set_vcis to allow vcis for arbitrary comm
     */
    if (comm == MPIR_Process.comm_world) {
        /* we always need call set_vcis even when n_total_vcis is 1 because -
         * 1. in case netmod need support multi-nics.
         * 2. remote processes may have multiple vcis.
         */
        mpi_errno = MPIDI_Comm_set_vcis(comm, MPIR_CVAR_CH4_NUM_VCIS, MPIR_CVAR_CH4_RESERVE_VCIS);
        MPIR_ERR_CHECK(mpi_errno);
    }

    /* prune selection tree */
    if (MPIDI_global.csel_root) {
        mpi_errno = MPIR_Csel_prune(MPIDI_global.csel_root, comm, &MPIDI_COMM(comm, csel_comm));
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        MPIDI_COMM(comm, csel_comm) = NULL;
    }

    /* prune selection tree for gpu */
    if (MPIDI_global.csel_root_gpu) {
        mpi_errno = MPIR_Csel_prune(MPIDI_global.csel_root_gpu, comm,
                                    &MPIDI_COMM(comm, csel_comm_gpu));
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        MPIDI_COMM(comm, csel_comm_gpu) = NULL;
    }

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
        MPIR_Subcomm_free(MPIDI_COMM(comm, multi_leads_comm));
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

    if (MPIDI_COMM(comm, csel_comm_gpu)) {
        mpi_errno = MPIR_Csel_free(MPIDI_COMM(comm, csel_comm_gpu));
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

/* Stages of forming inter communicator:
 * 0. establish leader communication - get dynamic_av via PMI, peer_comm, or connect/accept.
 * 1. leader exchange data.
 * 2. leader broadcast over local_comm.
 */
static int leader_exchange(MPIR_Comm * local_comm, MPIR_Lpid remote_lpid, int tag,
                           int context_id, int *remote_data_size_out, void **remote_data_out,
                           int timeout);
static int prepare_local_lpids(MPIR_Comm * local_comm, MPIR_Lpid ** lpids_out,
                               int *num_worlds_out, int **worlds_out);
static void convert_local_lpids(int local_size, MPIR_Lpid * lpids, int num_worlds, int *worlds);
static int prepare_local_data(int local_size, int context_id, MPIR_Lpid * lpids,
                              int num_worlds, int *world_idx_array,
                              int *upid_sizes, char *upids, int *data_size_out, void **data_out);
static int extract_remote_data(void *remote_data, int *remote_size_out,
                               int *remote_context_id_out, MPIR_Lpid ** remote_lpids_out,
                               int **remote_upid_sizes_out, char **remote_upids_out);

int MPID_Intercomm_exchange(MPIR_Comm * local_comm, int local_leader,
                            MPIR_Comm * peer_comm, int remote_leader, int tag,
                            int context_id, int *remote_context_id_out,
                            int *remote_size_out, MPIR_Lpid ** remote_lpids_out, int timeout)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    bool is_local_leader = (local_comm->rank == local_leader);
    struct bcast_data_t {
        int mpi_errno;
        int remote_data_size;
    };
    struct bcast_data_t bcast_data;

    /* Stage 1: exchange between leaders */
    int remote_data_size = 0;
    void *remote_data = NULL;
    if (is_local_leader) {
        MPIR_Lpid remote_lpid = MPIR_comm_rank_to_lpid(peer_comm, remote_leader);
        mpi_errno = leader_exchange(local_comm, remote_lpid, tag, context_id,
                                    &remote_data_size, &remote_data, timeout);
    }

    /* Stage 2: Broadcast inside local_group */
    if (is_local_leader) {
        bcast_data.mpi_errno = mpi_errno;
        bcast_data.remote_data_size = remote_data_size;
    }
    mpi_errno = MPIR_Bcast_impl(&bcast_data, 2, MPIR_INT_INTERNAL,
                                local_leader, local_comm, MPIR_ERR_NONE);
    MPIR_ERR_CHECK(mpi_errno);

    /* error checking of previous leader exchange */
    if (is_local_leader) {
        mpi_errno = bcast_data.mpi_errno;
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        MPIR_ERR_CHKANDJUMP(bcast_data.mpi_errno, mpi_errno, MPI_ERR_PORT, "**spawn");
        remote_data_size = bcast_data.remote_data_size;
    }

    /* bcast remote data */
    if (!is_local_leader) {
        remote_data = MPL_malloc(remote_data_size, MPL_MEM_OTHER);
        MPIR_ERR_CHKANDJUMP(!remote_data, mpi_errno, MPI_ERR_OTHER, "**nomem");
    }

    mpi_errno = MPIR_Bcast_impl(remote_data, remote_data_size, MPIR_BYTE_INTERNAL,
                                local_leader, local_comm, MPIR_ERR_NONE);
    MPIR_ERR_CHECK(mpi_errno);

    /* Stage 3: Each process extract data (if necessary: add worlds, convert lpids) */
    MPIR_Lpid *remote_lpids;
    int *remote_upid_sizes;
    char *remote_upids;
    /* need be inside CS because we are potentially introducing new worlds */
    mpi_errno = extract_remote_data(remote_data, remote_size_out, remote_context_id_out,
                                    &remote_lpids, &remote_upid_sizes, &remote_upids);
    MPIR_ERR_CHECK(mpi_errno);

#ifdef HAVE_ERROR_CHECKING
    /* Now that we have both the local and remote processes,
     * check for any overlap */
    MPIR_Lpid *local_lpids;
    local_lpids = MPL_malloc(local_comm->local_size * sizeof(MPIR_Lpid), MPL_MEM_GROUP);
    MPIR_ERR_CHKANDJUMP(!local_lpids, mpi_errno, MPI_ERR_OTHER, "**nomem");
    for (int i = 0; i < local_comm->local_size; i++) {
        local_lpids[i] = MPIR_Group_rank_to_lpid(local_comm->local_group, i);
    }

    mpi_errno = MPIDI_check_disjoint_lpids(local_lpids, local_comm->local_size,
                                           remote_lpids, *remote_size_out);
    MPL_free(local_lpids);
    MPIR_ERR_CHECK(mpi_errno);
#endif

    /* insert upids */
    char *upid = remote_upids;
    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI_LOCK(0));
    for (int i = 0; i < *remote_size_out; i++) {
        mpi_errno = MPIDI_NM_insert_upid(remote_lpids[i], upid, remote_upid_sizes[i]);
        if (mpi_errno) {
            break;
        }
        upid += remote_upid_sizes[i];
    }
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI_LOCK(0));
    MPIR_ERR_CHECK(mpi_errno);

    /* make a copy of remote_lpids (because it points to remote_data and it will freed) */
    *remote_lpids_out = MPL_malloc((*remote_size_out) * sizeof(MPIR_Lpid), MPL_MEM_GROUP);
    MPIR_ERR_CHKANDJUMP(!(*remote_lpids_out), mpi_errno, MPI_ERR_OTHER, "**nomem");
    memcpy(*remote_lpids_out, remote_lpids, (*remote_size_out) * sizeof(MPIR_Lpid));

    MPL_free(remote_data);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* Allocate and fill local lpids data. We assume remote will be from
 * different worlds, so we need worlds info so remote can match worlds
 * and convert lpids.
 */
static int prepare_local_lpids(MPIR_Comm * local_comm, MPIR_Lpid ** lpids_out,
                               int *num_worlds_out, int **worlds_out)
{
    int mpi_errno = MPI_SUCCESS;

    int local_size = local_comm->local_size;

    MPIR_Lpid *lpids;
    lpids = MPL_malloc(local_size * sizeof(MPIR_Lpid), MPL_MEM_GROUP);
    MPIR_ERR_CHKANDJUMP(!lpids, mpi_errno, MPI_ERR_OTHER, "**nomem");

    /* a make-shift hash for world_idx's, consider typically only a few worlds (or just 0)
     * It is OK to use static array here because the entire leader exchange will be
     * under (VCI 0) critical section.
     */
#define MAX_WORLDS 100
    static int world_hash[MAX_WORLDS] = { 0 };
    int num_worlds = 0;

    for (int i = 0; i < local_size; i++) {
        lpids[i] = MPIR_Group_rank_to_lpid(local_comm->local_group, i);
        int world_idx = MPIR_LPID_WORLD_INDEX(lpids[i]);

        bool found = false;
        for (int j = 0; j < num_worlds; j++) {
            if (world_hash[j] == world_idx) {
                found = true;
                break;
            }
        }
        if (!found) {
            world_hash[num_worlds++] = world_idx;
            MPIR_Assert(num_worlds < MAX_WORLDS);
        }
    }

  fn_exit:
    *lpids_out = lpids;
    *num_worlds_out = num_worlds;
    *worlds_out = world_hash;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static void convert_local_lpids(int local_size, MPIR_Lpid * lpids, int num_worlds, int *worlds)
{
    for (int i = 0; i < local_size; i++) {
        int world_idx = MPIR_LPID_WORLD_INDEX(lpids[i]);
        int world_rank = MPIR_LPID_WORLD_RANK(lpids[i]);
        int transit_world_idx = -1;
        for (int j = 0; j < num_worlds; j++) {
            if (worlds[j] == world_idx) {
                transit_world_idx = j;
                break;
            }
        }
        MPIR_Assert(transit_world_idx >= 0);
        lpids[i] = MPIR_LPID_FROM(transit_world_idx, world_rank);
    }
}

static int prepare_local_data(int local_size, int context_id, MPIR_Lpid * lpids,
                              int num_worlds, int *world_idx_array,
                              int *upid_sizes, char *upids, int *data_size_out, void **data_out)
{
    int mpi_errno = MPI_SUCCESS;

    /* layout:
     *    local_size
     *    context_id
     *    lpids[local_size]
     *    num_worlds
     *    namespace[num_worlds][MPIR_NAMESPACE_MAX]
     *    world_sizes[num_worlds]
     *    upid_sizes[local_size]
     *    upids[]
     */
    int total_upid_size = 0;
    for (int i = 0; i < local_size; i++) {
        total_upid_size += upid_sizes[i];
    }

    int len = 0;
    len += sizeof(int) * 2 + local_size * sizeof(MPIR_Lpid);
    len += sizeof(int) + num_worlds * MPIR_NAMESPACE_MAX + num_worlds * sizeof(int);
    len += local_size * sizeof(int);
    len += total_upid_size;

    char *data = MPL_malloc(len, MPL_MEM_OTHER);
    char *s = data;

    *(int *) (s) = local_size;
    s += sizeof(int);
    *(int *) (s) = context_id;
    s += sizeof(int);

    memcpy(s, lpids, local_size * sizeof(MPIR_Lpid));
    s += local_size * sizeof(MPIR_Lpid);

    *(int *) (s) = num_worlds;
    s += sizeof(int);
    for (int i = 0; i < num_worlds; i++) {
        strncpy(s, MPIR_Worlds[world_idx_array[i]].namespace, MPIR_NAMESPACE_MAX);
        s += MPIR_NAMESPACE_MAX;
    }
    for (int i = 0; i < num_worlds; i++) {
        *(int *) (s) = MPIR_Worlds[world_idx_array[i]].num_procs;
        s += sizeof(int);
    }

    memcpy(s, upid_sizes, local_size * sizeof(int));
    s += local_size * sizeof(int);

    memcpy(s, upids, total_upid_size);

    *data_size_out = len;
    *data_out = data;

    return mpi_errno;
}

/* NOTE: will add worlds and convert lpids if necessary */
static int extract_remote_data(void *remote_data, int *remote_size_out,
                               int *remote_context_id_out, MPIR_Lpid ** remote_lpids_out,
                               int **remote_upid_sizes_out, char **remote_upids_out)
{
    int mpi_errno = MPI_SUCCESS;
    /* lock to protect MPIR_Worlds */
    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI_LOCK(0));

    char *s = remote_data;

    *remote_size_out = *(int *) s;
    s += sizeof(int);
    int remote_size = *remote_size_out;

    *remote_context_id_out = *(int *) s;
    s += sizeof(int);

    *remote_lpids_out = (void *) s;
    s += remote_size * sizeof(MPIR_Lpid);

    int num_worlds = *(int *) s;
    s += sizeof(int);

    char *p_worlds = s;
    s += num_worlds * MPIR_NAMESPACE_MAX;

    int *p_world_sizes = (void *) s;
    s += num_worlds * sizeof(int);

    *remote_upid_sizes_out = (void *) s;
    s += remote_size * sizeof(int);

    *remote_upids_out = s;

    /* Find or add new worlds */
    int world_hash[MAX_WORLDS];
    for (int i = 0; i < num_worlds; i++) {
        char *namespace = p_worlds + i * MPIR_NAMESPACE_MAX;
        world_hash[i] = MPIR_find_world(namespace);
        if (world_hash[i] == -1) {
            world_hash[i] = MPIR_add_world(namespace, p_world_sizes[i]);
        }
    }

    /* convert remote lpids */
    for (int i = 0; i < remote_size; i++) {
        MPIR_Lpid lpid = (*remote_lpids_out)[i];
        int world_idx = MPIR_LPID_WORLD_INDEX(lpid);
        int world_rank = MPIR_LPID_WORLD_RANK(lpid);
        (*remote_lpids_out)[i] = MPIR_LPID_FROM(world_hash[world_idx], world_rank);
    }

    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI_LOCK(0));
    return mpi_errno;
}

/* exchange data between leaders */
static int leader_exchange(MPIR_Comm * local_comm, MPIR_Lpid remote_lpid, int tag, int context_id,
                           int *remote_data_size_out, void **remote_data_out, int timeout)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_CHKLMEM_DECL();
    MPIR_FUNC_ENTER;
    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI_LOCK(0));

    /* I am the leader of local_comm, remote_lpid is the remote leader of remote_comm.
     *
     * 1. Send data sizes
     * 2. Send data
     *
     * Future optimizations
     * * Eager mode
     * * Optionally skip upids exchange
     */

    /* local prepare */
    int local_size = local_comm->local_size;
    MPIR_Lpid *local_lpids;
    int num_local_worlds;
    int *local_worlds;
    mpi_errno = prepare_local_lpids(local_comm, &local_lpids, &num_local_worlds, &local_worlds);
    MPIR_ERR_CHECK(mpi_errno);
    MPIR_CHKLMEM_REGISTER(local_lpids);

    /* convert local world_idx to transit world_idx */
    convert_local_lpids(local_size, local_lpids, num_local_worlds, local_worlds);

    int *local_upid_sizes;
    char *local_upids;
    mpi_errno = MPIDI_NM_get_local_upids(local_comm, &local_upid_sizes, &local_upids);
    MPIR_ERR_CHECK(mpi_errno);
    MPIR_CHKLMEM_REGISTER(local_upid_sizes);
    MPIR_CHKLMEM_REGISTER(local_upids);

    int local_data_size;
    void *local_data;
    mpi_errno = prepare_local_data(local_size, context_id, local_lpids,
                                   num_local_worlds, local_worlds, local_upid_sizes, local_upids,
                                   &local_data_size, &local_data);
    MPIR_ERR_CHECK(mpi_errno);
    MPIR_CHKLMEM_REGISTER(local_data);

    /* exchange */
    int remote_data_size;
    void *remote_data;
    mpi_errno = MPIDI_NM_dynamic_sendrecv(remote_lpid, tag, &local_data_size, sizeof(int),
                                          &remote_data_size, sizeof(int), timeout);
    MPIR_ERR_CHECK(mpi_errno);

    remote_data = MPL_malloc(remote_data_size, MPL_MEM_OTHER);
    MPIR_ERR_CHKANDJUMP(!remote_data, mpi_errno, MPI_ERR_OTHER, "**nomem");

    mpi_errno = MPIDI_NM_dynamic_sendrecv(remote_lpid, tag, local_data, local_data_size,
                                          remote_data, remote_data_size, timeout);
    MPIR_ERR_CHECK(mpi_errno);

    *remote_data_size_out = remote_data_size;
    *remote_data_out = remote_data;

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI_LOCK(0));
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* ---- */
int MPID_Create_intercomm_from_lpids(MPIR_Comm * newcomm_ptr, int size, const MPIR_Lpid lpids[])
{
    int mpi_errno = MPI_SUCCESS;

    /* Assuming MPID_Intercomm_exchange already called, nothing to do here. */

    return mpi_errno;
}

/* Create multi-leaders communicator */
/* This routines assume a balanced and consecutive comm.
 * Create a comm with rank 0 of each node. A comm with rank 1 of each node and so on. Since these
 * new comms do no overlap, it uses the same context id */
int MPIDI_Comm_create_multi_leaders(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_CHKLMEM_DECL();
    MPIR_FUNC_TERSE_ENTER;

    int num_local = comm->num_local;
    int num_external = comm->num_external;
    int local_rank = comm->local_rank;
    int external_rank = comm->external_rank;

    /* balanced and node-consecutive */
    MPIR_Assert(comm->local_size == num_local * num_external);
    MPIR_Assert(local_rank == comm->rank % num_local);
    MPIR_Assert(external_rank = comm->rank / num_local);

    int *external_procs;
    MPIR_CHKLMEM_MALLOC(external_procs, num_external * sizeof(int));
    for (int i = 0; i < num_external; ++i) {
        external_procs[i] = i * num_local + local_rank;
    }

    mpi_errno = MPIR_Subcomm_create(comm, num_external, external_rank, external_procs,
                                    MPIR_CONTEXT_MULTILEADS_OFFSET,
                                    &MPIDI_COMM(comm, multi_leads_comm));
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
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

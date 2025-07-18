/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpid_nem_impl.h"
#include "mpidimpl.h"
#include "mpir_info.h"
#include "mpidrma.h"

/* FIXME: get this from OS */
#define MPIDI_CH3_PAGESIZE ((MPI_Aint)4096)

extern MPIR_T_pvar_timer_t PVAR_TIMER_rma_wincreate_allgather ATTRIBUTE((unused));

MPIDI_SHM_Wins_list_t shm_wins_list;

static int MPIDI_CH3I_Win_init(MPI_Aint size, int disp_unit, int create_flavor, int model,
                               MPIR_Info * info, MPIR_Comm * comm_ptr, MPIR_Win ** win_ptr);

static int MPIDI_CH3I_Win_allocate_shm(MPI_Aint size, int disp_unit, MPIR_Info * info,
                                       MPIR_Comm * comm_ptr, void *base_ptr, MPIR_Win ** win_ptr,
                                       int must);

static int MPIDI_CH3I_Win_detect_shm(MPIR_Win ** win_ptr);

static int MPIDI_CH3I_Win_gather_info(void *base, MPI_Aint size, int disp_unit, MPIR_Info * info,
                                      MPIR_Comm * comm_ptr, MPIR_Win ** win_ptr);

int MPIDI_CH3_Win_fns_init(MPIDI_CH3U_Win_fns_t * win_fns)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    if (MPIDI_CH3I_Shm_supported()) {
        win_fns->allocate_shm = MPIDI_CH3I_Win_allocate_shm;
        win_fns->detect_shm = MPIDI_CH3I_Win_detect_shm;
        win_fns->gather_info = MPIDI_CH3I_Win_gather_info;
        win_fns->shared_query = MPIDI_CH3_SHM_Win_shared_query;
    }

    MPIR_FUNC_EXIT;

    return mpi_errno;
}

int MPIDI_CH3_Win_hooks_init(MPIDI_CH3U_Win_hooks_t * win_hooks)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    if (MPIDI_CH3I_Shm_supported()) {
        win_hooks->win_init = MPIDI_CH3I_Win_init;
        win_hooks->win_free = MPIDI_CH3_SHM_Win_free;
    }

    MPIR_FUNC_EXIT;

    return mpi_errno;
}


int MPIDI_CH3_Win_pkt_orderings_init(MPIDI_CH3U_Win_pkt_ordering_t * win_pkt_orderings)
{
    int mpi_errno = MPI_SUCCESS;
    int netmod_ordering = 0;

    MPIR_FUNC_ENTER;

    win_pkt_orderings->am_flush_ordered = 0;

    if (MPID_nem_netmod_func && MPID_nem_netmod_func->get_ordering) {
        mpi_errno = MPID_nem_netmod_func->get_ordering(&netmod_ordering);
        MPIR_ERR_CHECK(mpi_errno);
    }

    if (netmod_ordering > 0) {
        /* Guarantees ordered AM flush only on ordered network.
         * In other words, it is ordered only when both intra-node and inter-node
         * connections are ordered. Otherwise we have to maintain the ordering per
         * connection, which causes expensive O(P) structure or per-OP function calls.*/
        win_pkt_orderings->am_flush_ordered = 1;
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int MPIDI_CH3I_Win_init(MPI_Aint size, int disp_unit, int create_flavor, int model,
                               MPIR_Info * info, MPIR_Comm * comm_ptr, MPIR_Win ** win_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    (*win_ptr)->shm_base_addr = NULL;
    (*win_ptr)->shm_segment_len = 0;
    (*win_ptr)->shm_segment_handle = 0;
    (*win_ptr)->shm_mutex = NULL;
    (*win_ptr)->shm_mutex_segment_handle = 0;

    (*win_ptr)->info_shm_base_addr = NULL;
    (*win_ptr)->info_shm_segment_len = 0;
    (*win_ptr)->info_shm_segment_handle = 0;

    MPIR_FUNC_EXIT;
    return mpi_errno;
}


static int MPIDI_CH3I_SHM_Wins_match(MPIR_Win ** win_ptr, MPIR_Win ** matched_win,
                                     MPI_Aint ** base_shm_offs_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    int i, comm_size;
    int node_size, node_rank, shm_node_size;
    int group_diff;
    int base_diff;

    MPIR_Comm *node_comm_ptr = NULL, *shm_node_comm_ptr = NULL;
    int *node_ranks = NULL, *node_ranks_in_shm_node = NULL;
    MPIR_Group *node_group_ptr = NULL, *shm_node_group_ptr = NULL;
    MPI_Aint *base_shm_offs;

    MPIDI_SHM_Win_t *elem = shm_wins_list;

    MPIR_CHKLMEM_DECL();
    MPIR_FUNC_ENTER;

    *matched_win = NULL;
    base_shm_offs = *base_shm_offs_ptr;
    node_comm_ptr = (*win_ptr)->comm_ptr->node_comm;
    MPIR_Assert(node_comm_ptr != NULL);
    node_size = node_comm_ptr->local_size;
    node_rank = node_comm_ptr->rank;

    comm_size = (*win_ptr)->comm_ptr->local_size;

    MPIR_CHKLMEM_MALLOC(node_ranks, node_size * sizeof(int));
    MPIR_CHKLMEM_MALLOC(node_ranks_in_shm_node, node_size * sizeof(int));

    for (i = 0; i < node_size; i++) {
        node_ranks[i] = i;
    }

    mpi_errno = MPIR_Comm_group_impl(node_comm_ptr, &node_group_ptr);
    MPIR_ERR_CHECK(mpi_errno);

    while (elem != NULL) {
        MPIR_Win *shm_win = elem->win;
        if (!shm_win)
            MPIDI_SHM_Wins_next_and_continue(elem);

        /* Compare node_comm.
         *
         * Only support shm if new node_comm is equal to or a subset of shm node_comm.
         * Shm node_comm == a subset of node_comm is not supported, because it means
         * some processes of node_comm cannot be shared, but RMA operation simply checks
         * the node_id of a target process for distinguishing shm target.  */
        shm_node_comm_ptr = shm_win->comm_ptr->node_comm;
        shm_node_size = shm_node_comm_ptr->local_size;

        if (node_size > shm_node_size)
            MPIDI_SHM_Wins_next_and_continue(elem);

        mpi_errno = MPIR_Comm_group_impl(shm_win->comm_ptr, &shm_node_group_ptr);
        MPIR_ERR_CHECK(mpi_errno);

        mpi_errno = MPIR_Group_translate_ranks_impl(node_group_ptr, node_size,
                                                    node_ranks, shm_node_group_ptr,
                                                    node_ranks_in_shm_node);
        MPIR_ERR_CHECK(mpi_errno);

        mpi_errno = MPIR_Group_free_impl(shm_node_group_ptr);
        MPIR_ERR_CHECK(mpi_errno);
        shm_node_group_ptr = NULL;

        group_diff = 0;
        for (i = 0; i < node_size; i++) {
            /* not exist in shm_comm->node_comm */
            if (node_ranks_in_shm_node[i] == MPI_UNDEFINED) {
                group_diff = 1;
                break;
            }
        }
        if (group_diff)
            MPIDI_SHM_Wins_next_and_continue(elem);

        /* Gather the offset of base_addr from all local processes. Match only
         * when all of them are included in the shm segment in current shm_win.
         *
         * Note that this collective call must be called after checking the
         * group match in order to guarantee all the local processes can perform
         * this call. */
        base_shm_offs[node_rank] = (MPI_Aint) ((*win_ptr)->base)
            - (MPI_Aint) (shm_win->shm_base_addr);
        mpi_errno = MPIR_Allgather(MPI_IN_PLACE, 0, MPI_DATATYPE_NULL,
                                   base_shm_offs, 1, MPIR_AINT_INTERNAL,
                                   node_comm_ptr, 0);
        MPIR_ERR_CHECK(mpi_errno);

        base_diff = 0;
        for (i = 0; i < comm_size; ++i) {
            int i_node_rank = MPIR_Get_intranode_rank((*win_ptr)->comm_ptr, i);
            if (i_node_rank >= 0) {
                MPIR_Assert(i_node_rank < node_size);

                if (base_shm_offs[i_node_rank] < 0 ||
                    base_shm_offs[i_node_rank] + (*win_ptr)->basic_info_table[i].size >
                    shm_win->shm_segment_len) {
                    base_diff = 1;
                    break;
                }
            }
        }

        if (base_diff)
            MPIDI_SHM_Wins_next_and_continue(elem);

        /* Found the first matched shm_win */
        *matched_win = shm_win;
        break;
    }

  fn_exit:
    if (node_group_ptr != NULL)
        mpi_errno = MPIR_Group_free_impl(node_group_ptr);
    /* Only free it here when group_translate_ranks fails. */
    if (shm_node_group_ptr != NULL)
        mpi_errno = MPIR_Group_free_impl(shm_node_group_ptr);

    MPIR_CHKLMEM_FREEALL();
    MPIR_FUNC_EXIT;
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

static int MPIDI_CH3I_Win_detect_shm(MPIR_Win ** win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Win *shm_win_ptr = NULL;
    int i, node_size;
    MPI_Aint *base_shm_offs;

    MPIR_CHKPMEM_DECL();
    MPIR_CHKLMEM_DECL();
    MPIR_FUNC_ENTER;

    if ((*win_ptr)->comm_ptr->node_comm == NULL) {
        goto fn_exit;
    }

    node_size = (*win_ptr)->comm_ptr->node_comm->local_size;

    MPIR_CHKLMEM_MALLOC(base_shm_offs, node_size * sizeof(MPI_Aint));

    /* Return the first matched shared window.
     * It is noted that the shared windows including all local processes are
     * stored in every local process in the same order, hence the first matched
     * shared window on every local process should be the same. */
    mpi_errno = MPIDI_CH3I_SHM_Wins_match(win_ptr, &shm_win_ptr, &base_shm_offs);
    MPIR_ERR_CHECK(mpi_errno);
    if (shm_win_ptr == NULL)
        goto fn_exit;

    (*win_ptr)->shm_allocated = TRUE;
    MPIR_CHKPMEM_MALLOC((*win_ptr)->shm_base_addrs, node_size * sizeof(void *), MPL_MEM_SHM);

    /* Compute the base address of shm buffer on each process.
     * shm_base_addrs[i] = my_shm_base_addr + off[i] */
    for (i = 0; i < node_size; i++) {
        (*win_ptr)->shm_base_addrs[i] =
            (void *) ((MPI_Aint) shm_win_ptr->shm_base_addr + base_shm_offs[i]);
    }

    /* TODO: should we use the same mutex or create a new one ?
     * It causes unnecessary synchronization.*/
    (*win_ptr)->shm_mutex = shm_win_ptr->shm_mutex;

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    MPIR_FUNC_EXIT;
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    MPIR_CHKPMEM_REAP();
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

static int MPIDI_CH3I_Win_gather_info(void *base, MPI_Aint size, int disp_unit, MPIR_Info * info,
                                      MPIR_Comm * comm_ptr, MPIR_Win ** win_ptr)
{
    MPIR_Comm *node_comm_ptr = NULL;
    int node_rank;
    int comm_rank, comm_size;
    MPI_Aint *tmp_buf = NULL;
    int i, k;
    int mpi_errno = MPI_SUCCESS;
    int mpl_err = 0;
    MPIR_CHKLMEM_DECL();

    MPIR_FUNC_ENTER;

    if ((*win_ptr)->comm_ptr->node_comm == NULL) {
        mpi_errno = MPIDI_CH3U_Win_gather_info(base, size, disp_unit, info, comm_ptr, win_ptr);
        goto fn_exit;
    }

    comm_size = (*win_ptr)->comm_ptr->local_size;
    comm_rank = (*win_ptr)->comm_ptr->rank;

    node_comm_ptr = (*win_ptr)->comm_ptr->node_comm;
    MPIR_Assert(node_comm_ptr != NULL);
    node_rank = node_comm_ptr->rank;

    (*win_ptr)->info_shm_segment_len = comm_size * sizeof(MPIDI_Win_basic_info_t);

    mpl_err = MPL_shm_hnd_init(&(*win_ptr)->info_shm_segment_handle);
    MPIR_ERR_CHKANDJUMP(mpl_err, mpi_errno, MPI_ERR_OTHER, "**alloc_shar_mem");

    if (node_rank == 0) {
        char *serialized_hnd_ptr = NULL;

        /* create shared memory region for all processes in win and map. */
        mpl_err = MPL_shm_seg_create_and_attach((*win_ptr)->info_shm_segment_handle,
                                                    (*win_ptr)->info_shm_segment_len,
                                                    &(*win_ptr)->info_shm_base_addr, 0);
        MPIR_ERR_CHKANDJUMP(mpl_err, mpi_errno, MPI_ERR_OTHER, "**alloc_shar_mem");

        /* serialize handle and broadcast it to the other processes in win */
        mpi_errno =
            MPL_shm_hnd_get_serialized_by_ref((*win_ptr)->info_shm_segment_handle,
                                                &serialized_hnd_ptr);
        MPIR_ERR_CHECK(mpi_errno);

        mpi_errno =
            MPIR_Bcast(serialized_hnd_ptr, MPL_SHM_GHND_SZ, MPIR_CHAR_INTERNAL, 0, node_comm_ptr,
                       0);
        MPIR_ERR_CHECK(mpi_errno);

        /* wait for other processes to attach to win */
        mpi_errno = MPIR_Barrier(node_comm_ptr, 0);
        MPIR_ERR_CHECK(mpi_errno);

        /* unlink shared memory region so it gets deleted when all processes exit */
        mpl_err = MPL_shm_seg_remove((*win_ptr)->info_shm_segment_handle);
        MPIR_ERR_CHKANDJUMP(mpl_err, mpi_errno, MPI_ERR_OTHER, "**remove_shar_mem");
    }
    else {
        char serialized_hnd[MPL_SHM_GHND_SZ] = { 0 };

        /* get serialized handle from rank 0 and deserialize it */
        mpi_errno =
            MPIR_Bcast(serialized_hnd, MPL_SHM_GHND_SZ, MPIR_CHAR_INTERNAL, 0, node_comm_ptr,
                       0);
        MPIR_ERR_CHECK(mpi_errno);

        mpl_err = MPL_shm_hnd_deserialize((*win_ptr)->info_shm_segment_handle, serialized_hnd,
                                              strlen(serialized_hnd));
        MPIR_ERR_CHKANDJUMP(mpl_err, mpi_errno, MPI_ERR_OTHER, "**alloc_shar_mem");

        /* attach to shared memory region created by rank 0 */
        mpl_err = MPL_shm_seg_attach((*win_ptr)->info_shm_segment_handle,
                                     (*win_ptr)->info_shm_segment_len,
                                     &(*win_ptr)->info_shm_base_addr, 0);
        MPIR_ERR_CHKANDJUMP(mpl_err, mpi_errno, MPI_ERR_OTHER, "**attach_shar_mem");

        mpi_errno = MPIR_Barrier(node_comm_ptr, 0);
        MPIR_ERR_CHECK(mpi_errno);
    }

    (*win_ptr)->basic_info_table = (MPIDI_Win_basic_info_t *) ((*win_ptr)->info_shm_base_addr);

    MPIR_CHKLMEM_MALLOC(tmp_buf, 4 * comm_size * sizeof(MPI_Aint));

    tmp_buf[4 * comm_rank] = MPIR_Ptr_to_aint(base);
    tmp_buf[4 * comm_rank + 1] = size;
    tmp_buf[4 * comm_rank + 2] = (MPI_Aint) disp_unit;
    tmp_buf[4 * comm_rank + 3] = (MPI_Aint) (*win_ptr)->handle;

    mpi_errno = MPIR_Allgather(MPI_IN_PLACE, 0, MPI_DATATYPE_NULL, tmp_buf, 4, MPIR_AINT_INTERNAL,
                               (*win_ptr)->comm_ptr, 0);
    MPIR_ERR_CHECK(mpi_errno);

    if (node_rank == 0) {
        /* only node_rank == 0 writes results to basic_info_table on shared memory region. */
        k = 0;
        for (i = 0; i < comm_size; i++) {
            (*win_ptr)->basic_info_table[i].base_addr = MPIR_Aint_to_ptr(tmp_buf[k++]);
            (*win_ptr)->basic_info_table[i].size = tmp_buf[k++];
            (*win_ptr)->basic_info_table[i].disp_unit = (int) tmp_buf[k++];
            (*win_ptr)->basic_info_table[i].win_handle = (MPI_Win) tmp_buf[k++];
        }
    }

    /* Make sure that all local processes see the results written by node_rank == 0 */
    mpi_errno = MPIR_Barrier(node_comm_ptr, 0);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    MPIR_FUNC_EXIT;
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

static int MPIDI_CH3I_Win_allocate_shm(MPI_Aint size, int disp_unit, MPIR_Info * info,
                                       MPIR_Comm * comm_ptr, void *base_ptr, MPIR_Win ** win_ptr,
                                       int must)
{
    int mpi_errno = MPI_SUCCESS;
    int mpl_err = 0;
    void **base_pp = (void **) base_ptr;
    int i, node_size, node_rank;
    MPIR_Comm *node_comm_ptr;
    MPI_Aint *node_sizes;
    int noncontig = FALSE;
    MPIR_CHKPMEM_DECL();
    MPIR_CHKLMEM_DECL();

    MPIR_FUNC_ENTER;

    if (must && (*win_ptr)->comm_ptr->local_size > 1) {
        /* this is MPI_Win_allocate_shared, return error if not all processes
         * are on the same shared memory domain */
        if ((*win_ptr)->comm_ptr->node_comm == NULL ||
            (*win_ptr)->comm_ptr->node_comm->local_size < (*win_ptr)->comm_ptr->local_size) {
            MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**winallocnotshared");
        }
    }

    if ((*win_ptr)->comm_ptr->node_comm == NULL) {
        mpi_errno =
            MPIDI_CH3U_Win_allocate_no_shm(size, disp_unit, info, comm_ptr, base_ptr, win_ptr);
        goto fn_exit;
    }

    /* see if we can allocate all windows contiguously */
    noncontig = (*win_ptr)->info_args.alloc_shared_noncontig;

    (*win_ptr)->shm_allocated = TRUE;

    /* When allocating shared memory region segment, we need comm of processes
     * that are on the same node as this process (node_comm).
     * If node_comm == NULL, this process is the only one on this node, therefore
     * we use comm_self as node comm. */
    node_comm_ptr = (*win_ptr)->comm_ptr->node_comm;
    MPIR_Assert(node_comm_ptr != NULL);
    node_size = node_comm_ptr->local_size;
    node_rank = node_comm_ptr->rank;

    MPIR_T_PVAR_TIMER_START(RMA, rma_wincreate_allgather);
    /* allocate memory for the base addresses, disp_units, and
     * completion counters of all processes */
    MPIR_CHKPMEM_MALLOC((*win_ptr)->shm_base_addrs, node_size * sizeof(void *), MPL_MEM_SHM);

    /* get the sizes of the windows and window objectsof
     * all processes.  allocate temp. buffer for communication */
    MPIR_CHKLMEM_MALLOC(node_sizes, node_size * sizeof(MPI_Aint));

    node_sizes[node_rank] = (MPI_Aint) size;

    mpi_errno = MPIR_Allgather(MPI_IN_PLACE, 0, MPI_DATATYPE_NULL,
                               node_sizes, sizeof(MPI_Aint), MPIR_BYTE_INTERNAL,
                               node_comm_ptr, 0);
    MPIR_T_PVAR_TIMER_END(RMA, rma_wincreate_allgather);
    MPIR_ERR_CHECK(mpi_errno);

    (*win_ptr)->shm_segment_len = 0;

    for (i = 0; i < node_size; i++) {
        if (noncontig)
            /* Round up to next page size */
            (*win_ptr)->shm_segment_len += MPL_ROUND_UP_ALIGN(node_sizes[i], MPIDI_CH3_PAGESIZE);
        else
            (*win_ptr)->shm_segment_len += node_sizes[i];
    }

    if ((*win_ptr)->shm_segment_len == 0) {
        (*win_ptr)->base = NULL;
    }

    else {
        mpl_err = MPL_shm_hnd_init(&(*win_ptr)->shm_segment_handle);
        MPIR_ERR_CHKANDJUMP(mpl_err, mpi_errno, MPI_ERR_OTHER, "**alloc_shar_mem");

        if (node_rank == 0) {
            char *serialized_hnd_ptr = NULL;

            /* create shared memory region for all processes in win and map */
            mpi_errno =
                MPL_shm_seg_create_and_attach((*win_ptr)->shm_segment_handle,
                                                (*win_ptr)->shm_segment_len,
                                                &(*win_ptr)->shm_base_addr, 0);
            MPIR_ERR_CHECK(mpi_errno);

            /* serialize handle and broadcast it to the other processes in win */
            mpi_errno =
                MPL_shm_hnd_get_serialized_by_ref((*win_ptr)->shm_segment_handle,
                                                    &serialized_hnd_ptr);
            MPIR_ERR_CHECK(mpi_errno);

            mpi_errno = MPIR_Bcast(serialized_hnd_ptr, MPL_SHM_GHND_SZ, MPIR_CHAR_INTERNAL, 0,
                                   node_comm_ptr, 0);
            MPIR_ERR_CHECK(mpi_errno);

            /* wait for other processes to attach to win */
            mpi_errno = MPIR_Barrier(node_comm_ptr, 0);
            MPIR_ERR_CHECK(mpi_errno);

            /* unlink shared memory region so it gets deleted when all processes exit */
            mpl_err = MPL_shm_seg_remove((*win_ptr)->shm_segment_handle);
            MPIR_ERR_CHKANDJUMP(mpl_err, mpi_errno, MPI_ERR_OTHER, "**remove_shar_mem");
        }
        else {
            char serialized_hnd[MPL_SHM_GHND_SZ] = { 0 };

            /* get serialized handle from rank 0 and deserialize it */
            mpi_errno = MPIR_Bcast(serialized_hnd, MPL_SHM_GHND_SZ, MPIR_CHAR_INTERNAL, 0,
                                   node_comm_ptr, 0);
            MPIR_ERR_CHECK(mpi_errno);

            mpi_errno =
                MPL_shm_hnd_deserialize((*win_ptr)->shm_segment_handle, serialized_hnd,
                                          strlen(serialized_hnd));
            MPIR_ERR_CHECK(mpi_errno);

            /* attach to shared memory region created by rank 0 */
            mpl_err = MPL_shm_seg_attach((*win_ptr)->shm_segment_handle,
                                         (*win_ptr)->shm_segment_len,
                                         &(*win_ptr)->shm_base_addr, 0);
            MPIR_ERR_CHKANDJUMP(mpl_err, mpi_errno, MPI_ERR_OTHER, "**attach_shar_mem");

            mpi_errno = MPIR_Barrier(node_comm_ptr, 0);
            MPIR_ERR_CHECK(mpi_errno);
        }

        /* Allocated the interprocess mutex segment. */
        mpl_err = MPL_shm_hnd_init(&(*win_ptr)->shm_mutex_segment_handle);
        MPIR_ERR_CHKANDJUMP(mpl_err, mpi_errno, MPI_ERR_OTHER, "**alloc_shar_mem");

        if (node_rank == 0) {
            char *serialized_hnd_ptr = NULL;

            /* create shared memory region for all processes in win and map */
            mpi_errno =
                MPL_shm_seg_create_and_attach((*win_ptr)->shm_mutex_segment_handle,
                                                sizeof(MPIDI_CH3I_SHM_MUTEX),
                                                (void **) &(*win_ptr)->shm_mutex, 0);
            MPIR_ERR_CHECK(mpi_errno);

            MPIDI_CH3I_SHM_MUTEX_INIT(*win_ptr);

            /* serialize handle and broadcast it to the other processes in win */
            mpi_errno =
                MPL_shm_hnd_get_serialized_by_ref((*win_ptr)->shm_mutex_segment_handle,
                                                    &serialized_hnd_ptr);
            MPIR_ERR_CHECK(mpi_errno);

            mpi_errno = MPIR_Bcast(serialized_hnd_ptr, MPL_SHM_GHND_SZ, MPIR_CHAR_INTERNAL, 0,
                                   node_comm_ptr, 0);
            MPIR_ERR_CHECK(mpi_errno);

            /* wait for other processes to attach to win */
            mpi_errno = MPIR_Barrier(node_comm_ptr, 0);
            MPIR_ERR_CHECK(mpi_errno);

            /* unlink shared memory region so it gets deleted when all processes exit */
            mpl_err = MPL_shm_seg_remove((*win_ptr)->shm_mutex_segment_handle);
            MPIR_ERR_CHKANDJUMP(mpl_err, mpi_errno, MPI_ERR_OTHER, "**remove_shar_mem");
        }
        else {
            char serialized_hnd[MPL_SHM_GHND_SZ] = { 0 };

            /* get serialized handle from rank 0 and deserialize it */
            mpi_errno = MPIR_Bcast(serialized_hnd, MPL_SHM_GHND_SZ, MPIR_CHAR_INTERNAL, 0,
                                   node_comm_ptr, 0);
            MPIR_ERR_CHECK(mpi_errno);

            mpi_errno =
                MPL_shm_hnd_deserialize((*win_ptr)->shm_mutex_segment_handle, serialized_hnd,
                                          strlen(serialized_hnd));
            MPIR_ERR_CHECK(mpi_errno);

            /* attach to shared memory region created by rank 0 */
            mpl_err = MPL_shm_seg_attach((*win_ptr)->shm_mutex_segment_handle,
                                         sizeof(MPIDI_CH3I_SHM_MUTEX),
                                         (void **) &(*win_ptr)->shm_mutex, 0);
            MPIR_ERR_CHKANDJUMP(mpl_err, mpi_errno, MPI_ERR_OTHER, "**attach_shar_mem");

            mpi_errno = MPIR_Barrier(node_comm_ptr, 0);
            MPIR_ERR_CHECK(mpi_errno);
        }

        /* compute the base addresses of each process within the shared memory segment */
        {
            char *cur_base;
            int cur_rank;

            cur_base = (*win_ptr)->shm_base_addr;
            cur_rank = 0;
            ((*win_ptr)->shm_base_addrs)[0] = (*win_ptr)->shm_base_addr;
            for (i = 1; i < node_size; ++i) {
                if (node_sizes[i]) {
                    /* For the base addresses, we track the previous
                     * process that has allocated non-zero bytes of shared
                     * memory.  We can not simply use "i-1" for the
                     * previous process because rank "i-1" might not have
                     * allocated any memory. */
                    if (noncontig) {
                        ((*win_ptr)->shm_base_addrs)[i] =
                            cur_base + MPL_ROUND_UP_ALIGN(node_sizes[cur_rank], MPIDI_CH3_PAGESIZE);
                    }
                    else {
                        ((*win_ptr)->shm_base_addrs)[i] = cur_base + node_sizes[cur_rank];
                    }
                    cur_base = ((*win_ptr)->shm_base_addrs)[i];
                    cur_rank = i;
                }
                else {
                    ((*win_ptr)->shm_base_addrs)[i] = NULL;
                }
            }
        }

        (*win_ptr)->base = (*win_ptr)->shm_base_addrs[node_rank];
    }

    *base_pp = (*win_ptr)->base;

    /* gather window information among processes via shared memory region. */
    mpi_errno = MPIDI_CH3I_Win_gather_info((*base_pp), size, disp_unit, info, comm_ptr, win_ptr);
    MPIR_ERR_CHECK(mpi_errno);

    /* Cache SHM windows */
    MPIDI_CH3I_SHM_Wins_append(&shm_wins_list, (*win_ptr));

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    MPIR_FUNC_EXIT;
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    MPIR_CHKPMEM_REAP();
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

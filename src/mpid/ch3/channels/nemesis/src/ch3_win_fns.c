/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpid_nem_impl.h"
#include "mpidimpl.h"
#include "mpiinfo.h"
#include "mpidrma.h"

/* FIXME: get this from OS */
#define MPIDI_CH3_PAGESIZE ((MPI_Aint)4096)
#define MPIDI_CH3_PAGESIZE_MASK (~(MPIDI_CH3_PAGESIZE-1))
#define MPIDI_CH3_ROUND_UP_PAGESIZE(x) ((((MPI_Aint)x)+(~MPIDI_CH3_PAGESIZE_MASK)) & MPIDI_CH3_PAGESIZE_MASK)

MPIR_T_PVAR_DOUBLE_TIMER_DECL_EXTERN(RMA, rma_wincreate_allgather);

MPIDI_SHM_Wins_list_t shm_wins_list;

static int MPIDI_CH3I_Win_allocate_shm(MPI_Aint size, int disp_unit, MPID_Info *info, MPID_Comm *comm_ptr,
                                       void *base_ptr, MPID_Win **win_ptr);

static int MPIDI_CH3I_Win_detect_shm(MPID_Win ** win_ptr);

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Win_fns_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Win_fns_init(MPIDI_CH3U_Win_fns_t *win_fns)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_WIN_FNS_INIT);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_CH3_WIN_FNS_INIT);

    if (MPIDI_CH3I_Shm_supported()) {
        win_fns->allocate_shm = MPIDI_CH3I_Win_allocate_shm;
        win_fns->detect_shm = MPIDI_CH3I_Win_detect_shm;
    }

    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_CH3_WIN_FNS_INIT);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_SHM_Wins_match
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int MPIDI_CH3I_SHM_Wins_match(MPID_Win ** win_ptr, MPID_Win ** matched_win,
                                     MPI_Aint ** base_shm_offs_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    int i, comm_size;
    int node_size, node_rank, shm_node_size;
    int group_diff;
    int base_diff;

    MPID_Comm *node_comm_ptr = NULL, *shm_node_comm_ptr = NULL;
    int *node_ranks = NULL, *node_ranks_in_shm_node = NULL;
    MPID_Group *node_group_ptr = NULL, *shm_node_group_ptr = NULL;
    int errflag = FALSE;
    MPI_Aint *base_shm_offs;

    MPIDI_SHM_Win_t *elem = shm_wins_list;

    MPIU_CHKLMEM_DECL(2);
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SHM_WINS_MATCH);
    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SHM_WINS_MATCH);

    *matched_win = NULL;
    base_shm_offs = *base_shm_offs_ptr;
    node_comm_ptr = (*win_ptr)->comm_ptr->node_comm;
    MPIU_Assert(node_comm_ptr != NULL);
    node_size = node_comm_ptr->local_size;
    node_rank = node_comm_ptr->rank;

    comm_size = (*win_ptr)->comm_ptr->local_size;

    MPIU_CHKLMEM_MALLOC(node_ranks, int *, node_size * sizeof(int), mpi_errno, "node_ranks");
    MPIU_CHKLMEM_MALLOC(node_ranks_in_shm_node, int *, node_size * sizeof(int),
                        mpi_errno, "node_ranks_in_shm_comm");

    for (i = 0; i < node_size; i++) {
        node_ranks[i] = i;
    }

    mpi_errno = MPIR_Comm_group_impl(node_comm_ptr, &node_group_ptr);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    while (elem != NULL) {
        MPID_Win *shm_win = elem->win;
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
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);

        mpi_errno = MPIR_Group_translate_ranks_impl(node_group_ptr, node_size,
                                                    node_ranks, shm_node_group_ptr,
                                                    node_ranks_in_shm_node);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);

        mpi_errno = MPIR_Group_free_impl(shm_node_group_ptr);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
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
        mpi_errno = MPIR_Allgather_impl(MPI_IN_PLACE, 0, MPI_DATATYPE_NULL,
                                        base_shm_offs, 1, MPI_AINT, node_comm_ptr, &errflag);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        MPIU_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");

        base_diff = 0;
        for (i = 0; i < comm_size; ++i) {
            int i_node_rank = (*win_ptr)->comm_ptr->intranode_table[i];
            if (i_node_rank >= 0) {
                MPIU_Assert(i_node_rank < node_size);

                if (base_shm_offs[i_node_rank] < 0 ||
                    base_shm_offs[i_node_rank] + (*win_ptr)->sizes[i] > shm_win->shm_segment_len) {
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

    MPIU_CHKLMEM_FREEALL();
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_WINS_MATCH);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Win_detect_shm
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int MPIDI_CH3I_Win_detect_shm(MPID_Win ** win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Win *shm_win_ptr = NULL;
    int i, comm_size, node_size;
    MPI_Aint *base_shm_offs;

    MPIU_CHKPMEM_DECL(1);
    MPIU_CHKLMEM_DECL(1);
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_WIN_DETECT_SHM);
    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_WIN_DETECT_SHM);

    if ((*win_ptr)->comm_ptr->node_comm == NULL) {
        goto fn_exit;
    }

    node_size = (*win_ptr)->comm_ptr->node_comm->local_size;
    comm_size = (*win_ptr)->comm_ptr->local_size;

    MPIU_CHKLMEM_MALLOC(base_shm_offs, MPI_Aint *, node_size * sizeof(MPI_Aint),
                        mpi_errno, "base_shm_offs");

    /* Return the first matched shared window.
     * It is noted that the shared windows including all local processes are
     * stored in every local process in the same order, hence the first matched
     * shared window on every local process should be the same. */
    mpi_errno = MPIDI_CH3I_SHM_Wins_match(win_ptr, &shm_win_ptr, &base_shm_offs);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    if (shm_win_ptr == NULL)
        goto fn_exit;

    (*win_ptr)->shm_allocated = TRUE;
    MPIU_CHKPMEM_MALLOC((*win_ptr)->shm_base_addrs, void **,
                        comm_size * sizeof(void *), mpi_errno, "(*win_ptr)->shm_base_addrs");

    /* Compute the base address of shm buffer on each process.
     * shm_base_addrs[i] = my_shm_base_addr + off[i] */
    for (i = 0; i < comm_size; i++) {
        int i_node_rank;
        i_node_rank = (*win_ptr)->comm_ptr->intranode_table[i];
        if (i_node_rank >= 0) {
            MPIU_Assert(i_node_rank < node_size);

            (*win_ptr)->shm_base_addrs[i] =
                (void *) ((MPI_Aint) shm_win_ptr->shm_base_addr + base_shm_offs[i_node_rank]);
        }
        else {
            (*win_ptr)->shm_base_addrs[i] = NULL;
        }
    }

    /* TODO: should we use the same mutex or create a new one ?
     * It causes unnecessary synchronization.*/
    (*win_ptr)->shm_mutex = shm_win_ptr->shm_mutex;
    (*win_ptr)->RMAFns.Win_free = MPIDI_CH3_SHM_Win_free;

  fn_exit:
    MPIU_CHKLMEM_FREEALL();
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_WIN_DETECT_SHM);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Win_allocate_shm
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int MPIDI_CH3I_Win_allocate_shm(MPI_Aint size, int disp_unit, MPID_Info *info,
                                       MPID_Comm *comm_ptr, void *base_ptr, MPID_Win **win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    void **base_pp = (void **) base_ptr;
    int i, k, comm_size, rank;
    int  node_size, node_rank;
    MPID_Comm *node_comm_ptr;
    MPI_Aint *node_sizes;
    void **node_shm_base_addrs;
    MPI_Aint *tmp_buf;
    int errflag = FALSE;
    int noncontig = FALSE;
    MPIU_CHKPMEM_DECL(6);
    MPIU_CHKLMEM_DECL(3);
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_WIN_ALLOCATE_SHM);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_WIN_ALLOCATE_SHM);

    if ((*win_ptr)->comm_ptr->node_comm == NULL) {
        mpi_errno = MPIDI_CH3U_Win_allocate_no_shm(size, disp_unit, info, comm_ptr, base_ptr, win_ptr);
        goto fn_exit;
    }

    /* If create flavor is MPI_WIN_FLAVOR_ALLOCATE, alloc_shared_noncontig is set to 1 by default. */
    if ((*win_ptr)->create_flavor == MPI_WIN_FLAVOR_ALLOCATE)
        (*win_ptr)->info_args.alloc_shared_noncontig = 1;

    /* Check if we are allowed to allocate space non-contiguously */
    if (info != NULL) {
        int alloc_shared_nctg_flag = 0;
        char alloc_shared_nctg_value[MPI_MAX_INFO_VAL+1];
        MPIR_Info_get_impl(info, "alloc_shared_noncontig", MPI_MAX_INFO_VAL,
                           alloc_shared_nctg_value, &alloc_shared_nctg_flag);
        if (alloc_shared_nctg_flag == 1) {
            if (!strncmp(alloc_shared_nctg_value, "true", strlen("true")))
                (*win_ptr)->info_args.alloc_shared_noncontig = 1;
            if (!strncmp(alloc_shared_nctg_value, "false", strlen("false")))
                (*win_ptr)->info_args.alloc_shared_noncontig = 0;
        }
    }

    /* see if we can allocate all windows contiguously */
    noncontig = (*win_ptr)->info_args.alloc_shared_noncontig;

    (*win_ptr)->shm_allocated = TRUE;

    comm_size = (*win_ptr)->comm_ptr->local_size;
    rank      = (*win_ptr)->comm_ptr->rank;

    /* When allocating shared memory region segment, we need comm of processes
       that are on the same node as this process (node_comm).
       If node_comm == NULL, this process is the only one on this node, therefore
       we use comm_self as node comm. */
    if ((*win_ptr)->comm_ptr->node_comm != NULL)
        node_comm_ptr = (*win_ptr)->comm_ptr->node_comm;
    else
        node_comm_ptr = MPIR_Process.comm_self;
    MPIU_Assert(node_comm_ptr != NULL);
    node_size = node_comm_ptr->local_size;
    node_rank = node_comm_ptr->rank;

    MPIR_T_PVAR_TIMER_START(RMA, rma_wincreate_allgather);
    /* allocate memory for the base addresses, disp_units, and
       completion counters of all processes */
    MPIU_CHKPMEM_MALLOC((*win_ptr)->base_addrs, void **,
                        comm_size*sizeof(void *),
                        mpi_errno, "(*win_ptr)->base_addrs");

    MPIU_CHKPMEM_MALLOC((*win_ptr)->shm_base_addrs, void **,
                        comm_size*sizeof(void *),
                        mpi_errno, "(*win_ptr)->shm_base_addrs");

    MPIU_CHKPMEM_MALLOC((*win_ptr)->sizes, MPI_Aint *, comm_size*sizeof(MPI_Aint),
                        mpi_errno, "(*win_ptr)->sizes");

    MPIU_CHKPMEM_MALLOC((*win_ptr)->disp_units, int *, comm_size*sizeof(int),
                        mpi_errno, "(*win_ptr)->disp_units");

    MPIU_CHKPMEM_MALLOC((*win_ptr)->all_win_handles, MPI_Win *,
                        comm_size*sizeof(MPI_Win),
                        mpi_errno, "(*win_ptr)->all_win_handles");

    MPIU_CHKPMEM_MALLOC((*win_ptr)->pt_rma_puts_accs, int *,
                        comm_size*sizeof(int),
                        mpi_errno, "(*win_ptr)->pt_rma_puts_accs");
    for (i=0; i<comm_size; i++)	(*win_ptr)->pt_rma_puts_accs[i] = 0;

    /* get the sizes of the windows and window objectsof
       all processes.  allocate temp. buffer for communication */
    MPIU_CHKLMEM_MALLOC(tmp_buf, MPI_Aint *, 3*comm_size*sizeof(MPI_Aint), mpi_errno, "tmp_buf");

    /* FIXME: This needs to be fixed for heterogeneous systems */
    tmp_buf[3*rank]   = (MPI_Aint) size;
    tmp_buf[3*rank+1] = (MPI_Aint) disp_unit;
    tmp_buf[3*rank+2] = (MPI_Aint) (*win_ptr)->handle;

    mpi_errno = MPIR_Allgather_impl(MPI_IN_PLACE, 0, MPI_DATATYPE_NULL,
                                    tmp_buf, 3 * sizeof(MPI_Aint), MPI_BYTE,
                                    (*win_ptr)->comm_ptr, &errflag);
    MPIR_T_PVAR_TIMER_END(RMA, rma_wincreate_allgather);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    MPIU_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");

    if ((*win_ptr)->create_flavor != MPI_WIN_FLAVOR_SHARED) {
        MPIU_CHKLMEM_MALLOC(node_sizes, MPI_Aint *, node_size*sizeof(MPI_Aint), mpi_errno, "node_sizes");
        for (i = 0; i < node_size; i++) node_sizes[i] = 0;
    }
    else {
        node_sizes = (*win_ptr)->sizes;
    }

    (*win_ptr)->shm_segment_len = 0;
    k = 0;
    for (i = 0; i < comm_size; ++i) {
        (*win_ptr)->sizes[i]           = tmp_buf[k++];
        (*win_ptr)->disp_units[i]      = (int) tmp_buf[k++];
        (*win_ptr)->all_win_handles[i] = (MPI_Win) tmp_buf[k++];

        if ((*win_ptr)->create_flavor != MPI_WIN_FLAVOR_SHARED) {
            /* If create flavor is not MPI_WIN_FLAVOR_SHARED, all processes on this
               window may not be on the same node. Because we only need the sizes of local
               processes (in order), we copy their sizes to a seperate array and keep them
               in order, fur purpose of future use of calculating shm_base_addrs. */
            if ((*win_ptr)->comm_ptr->intranode_table[i] >= 0) {
                MPIU_Assert((*win_ptr)->comm_ptr->intranode_table[i] < node_size);
                node_sizes[(*win_ptr)->comm_ptr->intranode_table[i]] = (*win_ptr)->sizes[i];
            }
        }
    }

    for (i = 0; i < node_size; i++) {
        if (noncontig)
            /* Round up to next page size */
            (*win_ptr)->shm_segment_len += MPIDI_CH3_ROUND_UP_PAGESIZE(node_sizes[i]);
        else
            (*win_ptr)->shm_segment_len += node_sizes[i];
    }

    if ((*win_ptr)->shm_segment_len == 0) {
        (*win_ptr)->base = NULL;
    }

    else {
    mpi_errno = MPIU_SHMW_Hnd_init(&(*win_ptr)->shm_segment_handle);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    if (node_rank == 0) {
        char *serialized_hnd_ptr = NULL;

        /* create shared memory region for all processes in win and map */
        mpi_errno = MPIU_SHMW_Seg_create_and_attach((*win_ptr)->shm_segment_handle, (*win_ptr)->shm_segment_len,
                                                    (char **)&(*win_ptr)->shm_base_addr, 0);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);

        /* serialize handle and broadcast it to the other processes in win */
        mpi_errno = MPIU_SHMW_Hnd_get_serialized_by_ref((*win_ptr)->shm_segment_handle, &serialized_hnd_ptr);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);

        mpi_errno = MPIR_Bcast_impl(serialized_hnd_ptr, MPIU_SHMW_GHND_SZ, MPI_CHAR, 0, node_comm_ptr, &errflag);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        MPIU_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");

        /* wait for other processes to attach to win */
        mpi_errno = MPIR_Barrier_impl(node_comm_ptr, &errflag);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        MPIU_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");

        /* unlink shared memory region so it gets deleted when all processes exit */
        mpi_errno = MPIU_SHMW_Seg_remove((*win_ptr)->shm_segment_handle);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    } else {
        char serialized_hnd[MPIU_SHMW_GHND_SZ] = {0};

        /* get serialized handle from rank 0 and deserialize it */
        mpi_errno = MPIR_Bcast_impl(serialized_hnd, MPIU_SHMW_GHND_SZ, MPI_CHAR, 0, node_comm_ptr, &errflag);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        MPIU_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");

        mpi_errno = MPIU_SHMW_Hnd_deserialize((*win_ptr)->shm_segment_handle, serialized_hnd, strlen(serialized_hnd));
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);

        /* attach to shared memory region created by rank 0 */
        mpi_errno = MPIU_SHMW_Seg_attach((*win_ptr)->shm_segment_handle, (*win_ptr)->shm_segment_len,
                                         (char **)&(*win_ptr)->shm_base_addr, 0);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);

        mpi_errno = MPIR_Barrier_impl(node_comm_ptr, &errflag);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        MPIU_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");
    }

    /* Allocated the interprocess mutex segment. */
    mpi_errno = MPIU_SHMW_Hnd_init(&(*win_ptr)->shm_mutex_segment_handle);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    if (node_rank == 0) {
        char *serialized_hnd_ptr = NULL;

        /* create shared memory region for all processes in win and map */
        mpi_errno = MPIU_SHMW_Seg_create_and_attach((*win_ptr)->shm_mutex_segment_handle, sizeof(MPIDI_CH3I_SHM_MUTEX),
                                                    (char **)&(*win_ptr)->shm_mutex, 0);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);

        MPIDI_CH3I_SHM_MUTEX_INIT(*win_ptr);

        /* serialize handle and broadcast it to the other processes in win */
        mpi_errno = MPIU_SHMW_Hnd_get_serialized_by_ref((*win_ptr)->shm_mutex_segment_handle, &serialized_hnd_ptr);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);

        mpi_errno = MPIR_Bcast_impl(serialized_hnd_ptr, MPIU_SHMW_GHND_SZ, MPI_CHAR, 0, node_comm_ptr, &errflag);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        MPIU_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");

        /* wait for other processes to attach to win */
        mpi_errno = MPIR_Barrier_impl(node_comm_ptr, &errflag);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        MPIU_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");

        /* unlink shared memory region so it gets deleted when all processes exit */
        mpi_errno = MPIU_SHMW_Seg_remove((*win_ptr)->shm_mutex_segment_handle);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    } else {
        char serialized_hnd[MPIU_SHMW_GHND_SZ] = {0};

        /* get serialized handle from rank 0 and deserialize it */
        mpi_errno = MPIR_Bcast_impl(serialized_hnd, MPIU_SHMW_GHND_SZ, MPI_CHAR, 0, node_comm_ptr, &errflag);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        MPIU_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");

        mpi_errno = MPIU_SHMW_Hnd_deserialize((*win_ptr)->shm_mutex_segment_handle, serialized_hnd, strlen(serialized_hnd));
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);

        /* attach to shared memory region created by rank 0 */
        mpi_errno = MPIU_SHMW_Seg_attach((*win_ptr)->shm_mutex_segment_handle, sizeof(MPIDI_CH3I_SHM_MUTEX),
                                         (char **)&(*win_ptr)->shm_mutex, 0);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);

        mpi_errno = MPIR_Barrier_impl(node_comm_ptr, &errflag);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        MPIU_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");
    }

    /* compute the base addresses of each process within the shared memory segment */
    {
        char *cur_base;
        int cur_rank;
        if ((*win_ptr)->create_flavor != MPI_WIN_FLAVOR_SHARED) {
            /* If create flavor is not MPI_WIN_FLAVOR_SHARED, all processes on this
               window may not be on the same node. Because we only need to calculate
               local processes' shm_base_addrs using local processes's sizes,
               we allocate a temporary array to place results and copy results
               back to shm_base_addrs on the window at last. */
            MPIU_CHKLMEM_MALLOC(node_shm_base_addrs, void **, node_size*sizeof(void*),
                                mpi_errno, "node_shm_base_addrs");
        }
        else {
            node_shm_base_addrs = (*win_ptr)->shm_base_addrs;
        }

        cur_base = (*win_ptr)->shm_base_addr;
        cur_rank = 0;
        node_shm_base_addrs[0] = (*win_ptr)->shm_base_addr;
        for (i = 1; i < node_size; ++i) {
            if (node_sizes[i]) {
                /* For the base addresses, we track the previous
                 * process that has allocated non-zero bytes of shared
                 * memory.  We can not simply use "i-1" for the
                 * previous process because rank "i-1" might not have
                 * allocated any memory. */
                if (noncontig) {
                    node_shm_base_addrs[i] = cur_base + MPIDI_CH3_ROUND_UP_PAGESIZE(node_sizes[cur_rank]);
                } else {
                    node_shm_base_addrs[i] = cur_base + node_sizes[cur_rank];
                }
                cur_base = node_shm_base_addrs[i];
                cur_rank = i;
            } else {
                node_shm_base_addrs[i] = NULL;
            }
        }

        if ((*win_ptr)->create_flavor != MPI_WIN_FLAVOR_SHARED) {
            /* if MPI_WIN_FLAVOR_SHARED is not set, copy from node_shm_base_addrs to
               (*win_ptr)->shm_base_addrs */
            for (i = 0; i < comm_size; i++) {
                if ((*win_ptr)->comm_ptr->intranode_table[i] >= 0) {
                    MPIU_Assert((*win_ptr)->comm_ptr->intranode_table[i] < node_size);
                    (*win_ptr)->shm_base_addrs[i] = node_shm_base_addrs[(*win_ptr)->comm_ptr->intranode_table[i]];
                }
                else
                    (*win_ptr)->shm_base_addrs[i] = NULL;
            }
        }
    }

    (*win_ptr)->base = (*win_ptr)->shm_base_addrs[rank];
    }

    /* get the base addresses of the windows.  Note we reuse tmp_buf from above
       since it's at least as large as we need it for this allgather. */
    tmp_buf[rank] = MPIU_PtrToAint((*win_ptr)->base);

    mpi_errno = MPIR_Allgather_impl(MPI_IN_PLACE, 0, MPI_DATATYPE_NULL,
                                    tmp_buf, 1, MPI_AINT,
                                    (*win_ptr)->comm_ptr, &errflag);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    MPIU_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");

    for (i = 0; i < comm_size; ++i)
        (*win_ptr)->base_addrs[i] = MPIU_AintToPtr(tmp_buf[i]);

    *base_pp = (*win_ptr)->base;

    /* Provide operation overrides for this window flavor */
    (*win_ptr)->RMAFns.Win_shared_query = MPIDI_CH3_SHM_Win_shared_query;
    (*win_ptr)->RMAFns.Win_free         = MPIDI_CH3_SHM_Win_free;

    /* Cache SHM windows */
    MPIDI_CH3I_SHM_Wins_append(&shm_wins_list, (*win_ptr));

fn_exit:
    MPIU_CHKLMEM_FREEALL();
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_WIN_ALLOCATE_SHM);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

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

static int MPIDI_CH3I_Win_allocate_shm(MPI_Aint size, int disp_unit, MPID_Info *info, MPID_Comm *comm_ptr,
                                       void *base_ptr, MPID_Win **win_ptr);

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Win_fns_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Win_fns_init(MPIDI_CH3U_Win_fns_t *win_fns)
{
    int mpi_errno = MPI_SUCCESS;
    int mutex_err;
    pthread_mutexattr_t attr;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_WIN_FNS_INIT);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_CH3_WIN_FNS_INIT);

    /* Test for PTHREAD_PROCESS_SHARED support.  Some platforms do not support
     * this capability even though it is a part of the pthreads core API (e.g.,
     * FreeBSD does not support this as of version 9.1) */
    pthread_mutexattr_init(&attr);
    mutex_err = pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutexattr_destroy(&attr);

    /* Only allocate shared memory windows if we can use
     * pthread_mutexattr_setpshared correctly. */
    if (!mutex_err)
        win_fns->allocate_shm = MPIDI_CH3I_Win_allocate_shm;

    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_CH3_WIN_FNS_INIT);

    return mpi_errno;
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

        char *cur_base = (*win_ptr)->shm_base_addr;
        node_shm_base_addrs[0] = (*win_ptr)->shm_base_addr;
        for (i = 1; i < node_size; ++i) {
            if (node_sizes[i]) {
                if (noncontig) {
                    node_shm_base_addrs[i] = cur_base + MPIDI_CH3_ROUND_UP_PAGESIZE(node_sizes[i-1]);
                } else {
                    node_shm_base_addrs[i] = cur_base + node_sizes[i-1];
                }
                cur_base = node_shm_base_addrs[i];
            } else {
                node_shm_base_addrs[i] = NULL; /* FIXME: Is this right? */
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

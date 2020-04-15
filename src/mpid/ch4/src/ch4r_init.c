/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "ch4r_init.h"

int MPIDIG_init_comm(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS, comm_idx, subcomm_type, is_localcomm;
    MPIDIG_rreq_t **uelist;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_INIT_COMM);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_INIT_COMM);

    MPIR_Assert(MPIDI_global.is_ch4u_initialized);

    if (MPIR_CONTEXT_READ_FIELD(DYNAMIC_PROC, comm->recvcontext_id))
        goto fn_exit;

    comm_idx = MPIDIG_get_context_index(comm->recvcontext_id);
    subcomm_type = MPIR_CONTEXT_READ_FIELD(SUBCOMM, comm->recvcontext_id);
    is_localcomm = MPIR_CONTEXT_READ_FIELD(IS_LOCALCOMM, comm->recvcontext_id);

    MPIR_Assert(subcomm_type <= 3);
    MPIR_Assert(is_localcomm <= 1);

    /* There is a potential race between this code (likely called by a user/main thread)
     * and an MPIDIG callback handler (called by a progress thread, when async progress
     * is turned on).
     * Thus we take a lock here to make sure the following operations are atomically done.
     * (transferring unexpected messages from a global queue to the newly created communicator) */
    MPID_THREAD_CS_ENTER(VCI, MPIDIU_THREAD_MPIDIG_GLOBAL_MUTEX);
    MPIDI_global.comm_req_lists[comm_idx].comm[is_localcomm][subcomm_type] = comm;
    MPIDIG_COMM(comm, posted_list) = NULL;
    MPIDIG_COMM(comm, unexp_list) = NULL;

    uelist = MPIDIG_context_id_to_uelist(comm->context_id);
    if (*uelist) {
        MPIDIG_rreq_t *curr, *tmp;
        DL_FOREACH_SAFE(*uelist, curr, tmp) {
            DL_DELETE(*uelist, curr);
            MPIR_Comm_add_ref(comm);    /* +1 for each entry in unexp_list */
            DL_APPEND(MPIDIG_COMM(comm, unexp_list), curr);
        }
        *uelist = NULL;
    }
    MPID_THREAD_CS_EXIT(VCI, MPIDIU_THREAD_MPIDIG_GLOBAL_MUTEX);

    MPIDIG_COMM(comm, window_instance) = 0;
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_INIT_COMM);
    return mpi_errno;
}

int MPIDIG_destroy_comm(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS, comm_idx, subcomm_type, is_localcomm;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_DESTROY_COMM);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_DESTROY_COMM);

    if (MPIR_CONTEXT_READ_FIELD(DYNAMIC_PROC, comm->recvcontext_id))
        goto fn_exit;
    comm_idx = MPIDIG_get_context_index(comm->recvcontext_id);
    subcomm_type = MPIR_CONTEXT_READ_FIELD(SUBCOMM, comm->recvcontext_id);
    is_localcomm = MPIR_CONTEXT_READ_FIELD(IS_LOCALCOMM, comm->recvcontext_id);

    MPIR_Assert(subcomm_type <= 3);
    MPIR_Assert(is_localcomm <= 1);

    MPID_THREAD_CS_ENTER(VCI, MPIDIU_THREAD_MPIDIG_GLOBAL_MUTEX);
    MPIR_Assert(MPIDI_global.comm_req_lists[comm_idx].comm[is_localcomm][subcomm_type] != NULL);

    if (MPIDI_global.comm_req_lists[comm_idx].comm[is_localcomm][subcomm_type]) {
        MPIR_Assert(MPIDIG_COMM
                    (MPIDI_global.comm_req_lists[comm_idx].comm[is_localcomm][subcomm_type],
                     posted_list) == NULL);
        MPIR_Assert(MPIDIG_COMM
                    (MPIDI_global.comm_req_lists[comm_idx].comm[is_localcomm][subcomm_type],
                     unexp_list) == NULL);
    }
    MPIDI_global.comm_req_lists[comm_idx].comm[is_localcomm][subcomm_type] = NULL;
    MPID_THREAD_CS_EXIT(VCI, MPIDIU_THREAD_MPIDIG_GLOBAL_MUTEX);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_DESTROY_COMM);
    return mpi_errno;
}

/* Linked list internally used to keep track of
 * allocated memory for which memory binding is
 * requested by the user. */
typedef struct mem_node {
    void *ptr;
    size_t size;
    struct mem_node *next;
} mem_node_t;

static mem_node_t *mem_list_head = NULL;
static mem_node_t *mem_list_tail = NULL;

void *MPIDIG_mpi_alloc_mem(size_t size, MPIR_Info * info_ptr)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_MPI_ALLOC_MEM);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_MPI_ALLOC_MEM);
    void *p;
    MPIR_hwtopo_type_e mem_type = MPIR_HWTOPO_TYPE__DDR;
    MPIR_hwtopo_gid_t mem_gid = MPIR_HWTOPO_GID_ROOT;
    int flag = 0;
    char hint_str[MPI_MAX_INFO_VAL + 1];

    /* retrieve requested memory type for allocation */
    if (info_ptr) {
        MPIR_Info_get_impl(info_ptr, "bind_memory", MPI_MAX_INFO_VAL, hint_str, &flag);
    }

    if (flag) {
        if (!strcmp(hint_str, "ddr"))
            mem_type = MPIR_HWTOPO_TYPE__DDR;
        else if (!strcmp(hint_str, "hbm")) {
            mem_type = MPIR_HWTOPO_TYPE__HBM;
        } else {
            mem_type = MPIR_HWTOPO_TYPE__DDR;
        }
        mem_gid = MPIR_hwtopo_get_obj_by_type(mem_type);
    }

    if (mem_gid != MPIR_HWTOPO_GID_ROOT) {
        /* requested memory type is available in the system and process is bound
         * to the corresponding device; allocate memory and bind it to device. */
        p = MPL_mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0,
                     MPL_MEM_USER);
        MPIR_hwtopo_mem_bind(p, size, mem_gid);

        /* keep track of bound memory for freeing it later */
        mem_node_t *el = MPL_malloc(sizeof(*el), MPL_MEM_OTHER);
        el->ptr = p;
        el->size = size;
        LL_APPEND(mem_list_head, mem_list_tail, el);
    } else if (mem_type != MPIR_HWTOPO_TYPE__DDR) {
        /* if mem_gid = MPIR_HWTOPO_GID_ROOT and mem_type is non-default (DDR)
         * it can mean either that the requested memory type is not available
         * in the system or the requested memory type is available but there
         * are many devices of such type and the process requesting memory is
         * not bound to any of them. Regardless the reason we do not fall back
         * to the default allocation and return a NULL pointer to the upper layer
         * instead. */
        p = NULL;
    } else {
        /* if mem_gid = MPIR_HWTOPO_GID_ROOT and mem_type is default (DDR) it
         * means that we cannot bind memory to a single device explicitly. In
         * this case we still allocate memory and leave the binding to the OS
         * (first touch policy in Linux). */
        p = MPL_malloc(size, MPL_MEM_USER);
    }
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_MPI_ALLOC_MEM);
    return p;
}

int MPIDIG_mpi_free_mem(void *ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_MPI_FREE_MEM);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_MPI_FREE_MEM);
    mem_node_t *el = NULL;

    /* scan memory list for allocations */
    LL_FOREACH(mem_list_head, el) {
        if (el->ptr == ptr) {
            LL_DELETE(mem_list_head, mem_list_tail, el);
            break;
        }
    }

    if (el) {
        MPL_munmap(el->ptr, el->size, MPL_MEM_USER);
        MPL_free(el);
    } else {
        MPL_free(ptr);
    }
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_MPI_FREE_MEM);
    return mpi_errno;
}

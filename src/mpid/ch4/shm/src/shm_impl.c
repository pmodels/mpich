/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "posix_impl.h"
MPL_SUPPRESS_OSX_HAS_NO_SYMBOLS_WARNING;

#ifndef SHM_INLINE
#ifndef SHM_DISABLE_INLINES

int MPIDI_SHM_mpi_init_hook(int rank, int size, int *tag_bits)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_MPI_INIT_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_MPI_INIT_HOOK);

    ret = MPIDI_SHM_src_funcs.mpi_init(rank, size, tag_bits);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_MPI_INIT_HOOK);
    return ret;
}

int MPIDI_SHM_mpi_finalize_hook(void)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_MPI_FINALIZE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_MPI_FINALIZE_HOOK);

    ret = MPIDI_SHM_src_funcs.mpi_finalize();

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_MPI_FINALIZE_HOOK);
    return ret;
}

int MPIDI_SHM_get_vci_attr(int vci)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_QUERY_VCI);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_QUERY_VCI);

    ret = MPIDI_SHM_src_funcs.get_vci_attr(vci);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_QUERY_VCI);
    return ret;
}

int MPIDI_SHM_mpi_comm_create_hook(MPIR_Comm * comm)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_MPI_COMM_CREATE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_MPI_COMM_CREATE_HOOK);

    ret = MPIDI_SHM_src_funcs.mpi_comm_create_hook(comm);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_MPI_COMM_CREATE_HOOK);
    return ret;
}

int MPIDI_SHM_mpi_comm_free_hook(MPIR_Comm * comm)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_MPI_COMM_FREE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_MPI_COMM_FREE_HOOK);

    ret = MPIDI_SHM_src_funcs.mpi_comm_free_hook(comm);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_MPI_COMM_FREE_HOOK);
    return ret;
}

int MPIDI_SHM_mpi_win_create_hook(MPIR_Win * win)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_MPI_WIN_CREATE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_MPI_WIN_CREATE_HOOK);

    ret = MPIDI_SHM_src_funcs.mpi_win_create_hook(win);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_MPI_WIN_CREATE_HOOK);
    return ret;
}

int MPIDI_SHM_mpi_win_allocate_hook(MPIR_Win * win)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_MPI_WIN_ALLOCATE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_MPI_WIN_ALLOCATE_HOOK);

    ret = MPIDI_SHM_src_funcs.mpi_win_allocate_hook(win);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_MPI_WIN_ALLOCATE_HOOK);
    return ret;
}

int MPIDI_SHM_mpi_win_allocate_shared_hook(MPIR_Win * win)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_MPI_WIN_ALLOCATE_SHARED_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_MPI_WIN_ALLOCATE_SHARED_HOOK);

    ret = MPIDI_SHM_src_funcs.mpi_win_allocate_shared_hook(win);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_MPI_WIN_ALLOCATE_SHARED_HOOK);
    return ret;
}

int MPIDI_SHM_mpi_win_create_dynamic_hook(MPIR_Win * win)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_MPI_WIN_CREATE_DYNAMIC_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_MPI_WIN_CREATE_DYNAMIC_HOOK);

    ret = MPIDI_SHM_src_funcs.mpi_win_create_dynamic_hook(win);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_MPI_WIN_CREATE_DYNAMIC_HOOK);
    return ret;
}

int MPIDI_SHM_mpi_win_attach_hook(MPIR_Win * win, void *base, MPI_Aint size)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_MPI_WIN_ATTACH_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_MPI_WIN_ATTACH_HOOK);

    ret = MPIDI_SHM_src_funcs.mpi_win_attach_hook(win, base, size);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_MPI_WIN_ATTACH_HOOK);
    return ret;
}

int MPIDI_SHM_mpi_win_detach_hook(MPIR_Win * win, const void *base)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_MPI_WIN_DETACH_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_MPI_WIN_DETACH_HOOK);

    ret = MPIDI_SHM_src_funcs.mpi_win_detach_hook(win, base);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_MPI_WIN_DETACH_HOOK);
    return ret;
}

int MPIDI_SHM_mpi_win_free_hook(MPIR_Win * win)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_MPI_WIN_FREE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_MPI_WIN_FREE_HOOK);

    ret = MPIDI_SHM_src_funcs.mpi_win_free_hook(win);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_MPI_WIN_FREE_HOOK);
    return ret;
}

int MPIDI_SHM_mpi_comm_connect(const char *port_name, MPIR_Info * info, int root, int timeout,
                               MPIR_Comm * comm, MPIR_Comm ** newcomm_ptr)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_MPI_COMM_CONNECT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_MPI_COMM_CONNECT);

    ret = MPIDI_SHM_src_funcs.mpi_comm_connect(port_name, info, root, timeout, comm, newcomm_ptr);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_MPI_COMM_CONNECT);
    return ret;
}

int MPIDI_SHM_mpi_comm_disconnect(MPIR_Comm * comm_ptr)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_MPI_COMM_DISCONNECT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_MPI_COMM_DISCONNECT);

    ret = MPIDI_SHM_src_funcs.mpi_comm_disconnect(comm_ptr);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_MPI_COMM_DISCONNECT);
    return ret;
}

int MPIDI_SHM_mpi_open_port(MPIR_Info * info_ptr, char *port_name)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_MPI_OPEN_PORT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_MPI_OPEN_PORT);

    ret = MPIDI_SHM_src_funcs.mpi_open_port(info_ptr, port_name);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_MPI_OPEN_PORT);
    return ret;
}

int MPIDI_SHM_mpi_close_port(const char *port_name)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_MPI_CLOSE_PORT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_MPI_CLOSE_PORT);

    ret = MPIDI_SHM_src_funcs.mpi_close_port(port_name);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_MPI_CLOSE_PORT);
    return ret;
}

int MPIDI_SHM_mpi_comm_accept(const char *port_name, MPIR_Info * info, int root, MPIR_Comm * comm,
                              MPIR_Comm ** newcomm_ptr)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_MPI_COMM_ACCEPT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_MPI_COMM_ACCEPT);

    ret = MPIDI_SHM_src_funcs.mpi_comm_accept(port_name, info, root, comm, newcomm_ptr);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_MPI_COMM_ACCEPT);
    return ret;
}

int MPIDI_SHM_mpi_win_set_info(MPIR_Win * win, MPIR_Info * info)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_MPI_WIN_SET_INFO);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_MPI_WIN_SET_INFO);

    ret = MPIDI_SHM_native_src_funcs.mpi_win_set_info(win, info);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_MPI_WIN_SET_INFO);
    return ret;
}

int MPIDI_SHM_mpi_win_get_info(MPIR_Win * win, MPIR_Info ** info_p_p)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_MPI_WIN_GET_INFO);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_MPI_WIN_GET_INFO);

    ret = MPIDI_SHM_native_src_funcs.mpi_win_get_info(win, info_p_p);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_MPI_WIN_GET_INFO);
    return ret;
}

int MPIDI_SHM_mpi_win_free(MPIR_Win ** win_ptr)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_MPI_WIN_FREE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_MPI_WIN_FREE);

    ret = MPIDI_SHM_native_src_funcs.mpi_win_free(win_ptr);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_MPI_WIN_FREE);
    return ret;
}

int MPIDI_SHM_mpi_win_create(void *base, MPI_Aint length, int disp_unit, MPIR_Info * info,
                             MPIR_Comm * comm_ptr, MPIR_Win ** win_ptr)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_MPI_WIN_CREATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_MPI_WIN_CREATE);

    ret =
        MPIDI_SHM_native_src_funcs.mpi_win_create(base, length, disp_unit, info, comm_ptr, win_ptr);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_MPI_WIN_CREATE);
    return ret;
}

int MPIDI_SHM_mpi_win_attach(MPIR_Win * win, void *base, MPI_Aint size)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_MPI_WIN_ATTACH);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_MPI_WIN_ATTACH);

    ret = MPIDI_SHM_native_src_funcs.mpi_win_attach(win, base, size);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_MPI_WIN_ATTACH);
    return ret;
}

int MPIDI_SHM_mpi_win_allocate_shared(MPI_Aint size, int disp_unit, MPIR_Info * info_ptr,
                                      MPIR_Comm * comm_ptr, void **base_ptr, MPIR_Win ** win_ptr)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_MPI_WIN_ALLOCATE_SHARED);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_MPI_WIN_ALLOCATE_SHARED);

    ret = MPIDI_SHM_native_src_funcs.mpi_win_allocate_shared(size, disp_unit, info_ptr, comm_ptr,
                                                             base_ptr, win_ptr);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_MPI_WIN_ALLOCATE_SHARED);
    return ret;
}

int MPIDI_SHM_mpi_win_detach(MPIR_Win * win, const void *base)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_MPI_WIN_DETACH);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_MPI_WIN_DETACH);

    ret = MPIDI_SHM_native_src_funcs.mpi_win_detach(win, base);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_MPI_WIN_DETACH);
    return ret;
}

int MPIDI_SHM_mpi_win_allocate(MPI_Aint size, int disp_unit, MPIR_Info * info, MPIR_Comm * comm,
                               void *baseptr, MPIR_Win ** win)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_MPI_WIN_ALLOCATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_MPI_WIN_ALLOCATE);

    ret = MPIDI_SHM_native_src_funcs.mpi_win_allocate(size, disp_unit, info, comm, baseptr, win);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_MPI_WIN_ALLOCATE);
    return ret;
}

int MPIDI_SHM_mpi_win_create_dynamic(MPIR_Info * info, MPIR_Comm * comm, MPIR_Win ** win)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_MPI_WIN_CREATE_DYNAMIC);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_MPI_WIN_CREATE_DYNAMIC);

    ret = MPIDI_SHM_native_src_funcs.mpi_win_create_dynamic(info, comm, win);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_MPI_WIN_CREATE_DYNAMIC);
    return ret;
}

int MPIDI_SHM_get_local_upids(MPIR_Comm * comm, size_t ** local_upid_size, char **local_upids)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_GET_LOCAL_UPIDS);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_GET_LOCAL_UPIDS);

    ret = MPIDI_SHM_src_funcs.get_local_upids(comm, local_upid_size, local_upids);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_GET_LOCAL_UPIDS);
    return ret;
}

int MPIDI_SHM_upids_to_lupids(int size, size_t * remote_upid_size, char *remote_upids,
                              int **remote_lupids)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_UPIDS_TO_LUPIDS);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_UPIDS_TO_LUPIDS);

    ret = MPIDI_SHM_src_funcs.upids_to_lupids(size, remote_upid_size, remote_upids, remote_lupids);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_UPIDS_TO_LUPIDS);
    return ret;
}

int MPIDI_SHM_create_intercomm_from_lpids(MPIR_Comm * newcomm_ptr, int size, const int lpids[])
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_CREATE_INTERCOMM_FROM_LPIDS);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_CREATE_INTERCOMM_FROM_LPIDS);

    ret = MPIDI_SHM_src_funcs.create_intercomm_from_lpids(newcomm_ptr, size, lpids);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_CREATE_INTERCOMM_FROM_LPIDS);
    return ret;
}

void *MPIDI_SHM_mpi_alloc_mem(size_t size, MPIR_Info * info_ptr)
{
    void *ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_MPI_ALLOC_MEM);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_MPI_ALLOC_MEM);

    ret = MPIDI_SHM_native_src_funcs.mpi_alloc_mem(size, info_ptr);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_MPI_ALLOC_MEM);
    return ret;
}

int MPIDI_SHM_mpi_free_mem(void *ptr)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_MPI_FREE_MEM);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_MPI_FREE_MEM);

    ret = MPIDI_SHM_native_src_funcs.mpi_free_mem(ptr);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_MPI_FREE_MEM);
    return ret;
}

int MPIDI_SHM_mpi_type_commit_hook(MPIR_Datatype * datatype_p)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_MPI_TYPE_CREATE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_MPI_TYPE_CREATE_HOOK);

    ret = MPIDI_SHM_native_src_funcs.mpi_type_commit_hook(datatype_p);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_MPI_TYPE_CREATE_HOOK);
    return ret;
}

int MPIDI_SHM_mpi_type_free_hook(MPIR_Datatype * datatype_p)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_MPI_TYPE_FREE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_MPI_TYPE_FREE_HOOK);

    ret = MPIDI_SHM_native_src_funcs.mpi_type_free_hook(datatype_p);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_MPI_TYPE_FREE_HOOK);
    return ret;
}

int MPIDI_SHM_mpi_op_commit_hook(MPIR_Op * op_p)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_MPI_OP_COMMIT_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_MPI_OP_COMMIT_HOOK);

    ret = MPIDI_SHM_native_src_funcs.mpi_op_commit_hook(op_p);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_MPI_OP_COMMIT_HOOK);
    return ret;
}

int MPIDI_SHM_mpi_op_free_hook(MPIR_Op * op_p)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_MPI_OP_FREE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_MPI_OP_FREE_HOOK);

    ret = MPIDI_SHM_native_src_funcs.mpi_op_free_hook(op_p);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_MPI_OP_FREE_HOOK);
    return ret;
}

int MPIDI_SHM_progress(int vci, int blocking)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_PROGRESS);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_PROGRESS);

    ret = MPIDI_SHM_src_funcs.progress(vci, blocking);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_PROGRESS);
    return ret;
}

#endif
#endif

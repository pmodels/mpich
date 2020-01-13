/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "posix_impl.h"
MPL_SUPPRESS_OSX_HAS_NO_SYMBOLS_WARNING;

#ifndef SHM_INLINE
#ifndef SHM_DISABLE_INLINES

int MPIDI_SHM_mpi_init_hook(int rank, int size, int *n_vcis_provided, int *tag_bits)
{
    int ret;


    ret = MPIDI_SHM_src_funcs.mpi_init(rank, size, n_vcis_provided, tag_bits);

    return ret;
}

int MPIDI_SHM_mpi_finalize_hook(void)
{
    int ret;


    ret = MPIDI_SHM_src_funcs.mpi_finalize();

    return ret;
}

int MPIDI_SHM_get_vci_attr(int vci)
{
    int ret;


    ret = MPIDI_SHM_src_funcs.get_vci_attr(vci);

    return ret;
}

int MPIDI_SHM_mpi_comm_create_hook(MPIR_Comm * comm)
{
    int ret;


    ret = MPIDI_SHM_src_funcs.mpi_comm_create_hook(comm);

    return ret;
}

int MPIDI_SHM_mpi_comm_free_hook(MPIR_Comm * comm)
{
    int ret;


    ret = MPIDI_SHM_src_funcs.mpi_comm_free_hook(comm);

    return ret;
}

int MPIDI_SHM_mpi_win_create_hook(MPIR_Win * win)
{
    int ret;


    ret = MPIDI_SHM_src_funcs.mpi_win_create_hook(win);

    return ret;
}

int MPIDI_SHM_mpi_win_allocate_hook(MPIR_Win * win)
{
    int ret;


    ret = MPIDI_SHM_src_funcs.mpi_win_allocate_hook(win);

    return ret;
}

int MPIDI_SHM_mpi_win_allocate_shared_hook(MPIR_Win * win)
{
    int ret;


    ret = MPIDI_SHM_src_funcs.mpi_win_allocate_shared_hook(win);

    return ret;
}

int MPIDI_SHM_mpi_win_create_dynamic_hook(MPIR_Win * win)
{
    int ret;


    ret = MPIDI_SHM_src_funcs.mpi_win_create_dynamic_hook(win);

    return ret;
}

int MPIDI_SHM_mpi_win_attach_hook(MPIR_Win * win, void *base, MPI_Aint size)
{
    int ret;


    ret = MPIDI_SHM_src_funcs.mpi_win_attach_hook(win, base, size);

    return ret;
}

int MPIDI_SHM_mpi_win_detach_hook(MPIR_Win * win, const void *base)
{
    int ret;


    ret = MPIDI_SHM_src_funcs.mpi_win_detach_hook(win, base);

    return ret;
}

int MPIDI_SHM_mpi_win_free_hook(MPIR_Win * win)
{
    int ret;


    ret = MPIDI_SHM_src_funcs.mpi_win_free_hook(win);

    return ret;
}

int MPIDI_SHM_mpi_comm_connect(const char *port_name, MPIR_Info * info, int root, int timeout,
                               MPIR_Comm * comm, MPIR_Comm ** newcomm_ptr)
{
    int ret;


    ret = MPIDI_SHM_src_funcs.mpi_comm_connect(port_name, info, root, timeout, comm, newcomm_ptr);

    return ret;
}

int MPIDI_SHM_mpi_comm_disconnect(MPIR_Comm * comm_ptr)
{
    int ret;


    ret = MPIDI_SHM_src_funcs.mpi_comm_disconnect(comm_ptr);

    return ret;
}

int MPIDI_SHM_mpi_open_port(MPIR_Info * info_ptr, char *port_name)
{
    int ret;


    ret = MPIDI_SHM_src_funcs.mpi_open_port(info_ptr, port_name);

    return ret;
}

int MPIDI_SHM_mpi_close_port(const char *port_name)
{
    int ret;


    ret = MPIDI_SHM_src_funcs.mpi_close_port(port_name);

    return ret;
}

int MPIDI_SHM_mpi_comm_accept(const char *port_name, MPIR_Info * info, int root, MPIR_Comm * comm,
                              MPIR_Comm ** newcomm_ptr)
{
    int ret;


    ret = MPIDI_SHM_src_funcs.mpi_comm_accept(port_name, info, root, comm, newcomm_ptr);

    return ret;
}

int MPIDI_SHM_mpi_win_set_info(MPIR_Win * win, MPIR_Info * info)
{
    int ret;


    ret = MPIDI_SHM_native_src_funcs.mpi_win_set_info(win, info);

    return ret;
}

int MPIDI_SHM_mpi_win_get_info(MPIR_Win * win, MPIR_Info ** info_p_p)
{
    int ret;


    ret = MPIDI_SHM_native_src_funcs.mpi_win_get_info(win, info_p_p);

    return ret;
}

int MPIDI_SHM_mpi_win_free(MPIR_Win ** win_ptr)
{
    int ret;


    ret = MPIDI_SHM_native_src_funcs.mpi_win_free(win_ptr);

    return ret;
}

int MPIDI_SHM_mpi_win_create(void *base, MPI_Aint length, int disp_unit, MPIR_Info * info,
                             MPIR_Comm * comm_ptr, MPIR_Win ** win_ptr)
{
    int ret;


    ret =
        MPIDI_SHM_native_src_funcs.mpi_win_create(base, length, disp_unit, info, comm_ptr, win_ptr);

    return ret;
}

int MPIDI_SHM_mpi_win_attach(MPIR_Win * win, void *base, MPI_Aint size)
{
    int ret;


    ret = MPIDI_SHM_native_src_funcs.mpi_win_attach(win, base, size);

    return ret;
}

int MPIDI_SHM_mpi_win_allocate_shared(MPI_Aint size, int disp_unit, MPIR_Info * info_ptr,
                                      MPIR_Comm * comm_ptr, void **base_ptr, MPIR_Win ** win_ptr)
{
    int ret;


    ret = MPIDI_SHM_native_src_funcs.mpi_win_allocate_shared(size, disp_unit, info_ptr, comm_ptr,
                                                             base_ptr, win_ptr);

    return ret;
}

int MPIDI_SHM_mpi_win_detach(MPIR_Win * win, const void *base)
{
    int ret;


    ret = MPIDI_SHM_native_src_funcs.mpi_win_detach(win, base);

    return ret;
}

int MPIDI_SHM_mpi_win_allocate(MPI_Aint size, int disp_unit, MPIR_Info * info, MPIR_Comm * comm,
                               void *baseptr, MPIR_Win ** win)
{
    int ret;


    ret = MPIDI_SHM_native_src_funcs.mpi_win_allocate(size, disp_unit, info, comm, baseptr, win);

    return ret;
}

int MPIDI_SHM_mpi_win_create_dynamic(MPIR_Info * info, MPIR_Comm * comm, MPIR_Win ** win)
{
    int ret;


    ret = MPIDI_SHM_native_src_funcs.mpi_win_create_dynamic(info, comm, win);

    return ret;
}

int MPIDI_SHM_get_local_upids(MPIR_Comm * comm, size_t ** local_upid_size, char **local_upids)
{
    int ret;


    ret = MPIDI_SHM_src_funcs.get_local_upids(comm, local_upid_size, local_upids);

    return ret;
}

int MPIDI_SHM_upids_to_lupids(int size, size_t * remote_upid_size, char *remote_upids,
                              int **remote_lupids)
{
    int ret;


    ret = MPIDI_SHM_src_funcs.upids_to_lupids(size, remote_upid_size, remote_upids, remote_lupids);

    return ret;
}

int MPIDI_SHM_create_intercomm_from_lpids(MPIR_Comm * newcomm_ptr, int size, const int lpids[])
{
    int ret;


    ret = MPIDI_SHM_src_funcs.create_intercomm_from_lpids(newcomm_ptr, size, lpids);

    return ret;
}

void *MPIDI_SHM_mpi_alloc_mem(size_t size, MPIR_Info * info_ptr)
{
    void *ret;


    ret = MPIDI_SHM_native_src_funcs.mpi_alloc_mem(size, info_ptr);

    return ret;
}

int MPIDI_SHM_mpi_free_mem(void *ptr)
{
    int ret;


    ret = MPIDI_SHM_native_src_funcs.mpi_free_mem(ptr);

    return ret;
}

int MPIDI_SHM_mpi_type_commit_hook(MPIR_Datatype * datatype_p)
{
    int ret;


    ret = MPIDI_SHM_native_src_funcs.mpi_type_commit_hook(datatype_p);

    return ret;
}

int MPIDI_SHM_mpi_type_free_hook(MPIR_Datatype * datatype_p)
{
    int ret;


    ret = MPIDI_SHM_native_src_funcs.mpi_type_free_hook(datatype_p);

    return ret;
}

int MPIDI_SHM_mpi_op_commit_hook(MPIR_Op * op_p)
{
    int ret;


    ret = MPIDI_SHM_native_src_funcs.mpi_op_commit_hook(op_p);

    return ret;
}

int MPIDI_SHM_mpi_op_free_hook(MPIR_Op * op_p)
{
    int ret;


    ret = MPIDI_SHM_native_src_funcs.mpi_op_free_hook(op_p);

    return ret;
}

int MPIDI_SHM_progress(int vci, int blocking)
{
    int ret;


    ret = MPIDI_SHM_src_funcs.progress(vci, blocking);

    return ret;
}

#endif
#endif

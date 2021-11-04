/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */
#ifndef GPU_POST_H_INCLUDED
#define GPU_POST_H_INCLUDED

#include "gpu_pre.h"

int MPIDI_GPU_get_ipc_attr(const void *vaddr, int rank, MPIR_Comm * comm,
                           MPIDI_IPCI_ipc_attr_t * ipc_attr);
int MPIDI_GPU_ipc_get_map_dev(int remote_global_dev_id, int local_dev_id, MPI_Datatype datatype);
int MPIDI_GPU_ipc_handle_map(MPIDI_GPU_ipc_handle_t handle, int map_dev_id, void **vaddr);
int MPIDI_GPU_ipc_handle_unmap(void *vaddr, MPIDI_GPU_ipc_handle_t handle);
int MPIDI_GPU_init_local(void);
int MPIDI_GPU_init_world(void);
int MPIDI_GPU_mpi_finalize_hook(void);
int MPIDI_GPU_ipc_handle_cache_insert(int rank, MPIR_Comm * comm, MPIDI_GPU_ipc_handle_t handle);

typedef struct {
    int max_dev_id;
    int max_subdev_id;
} MPIDI_GPU_device_info_t;

#endif /* GPU_POST_H_INCLUDED */

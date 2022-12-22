/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */
#ifndef GPU_POST_H_INCLUDED
#define GPU_POST_H_INCLUDED

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_CH4_IPC_GPU_P2P_THRESHOLD
      category    : CH4
      type        : int
      default     : 1
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        If a send message size is greater than or equal to MPIR_CVAR_CH4_IPC_GPU_P2P_THRESHOLD (in
        bytes), then enable GPU-based single copy protocol for intranode communication. The
        environment variable is valid only when then GPU IPC shmmod is enabled.

    - name        : MPIR_CVAR_CH4_IPC_GPU_FAST_COPY_MAX_SIZE
      category    : CH4
      type        : int
      default     : 1024
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        If a send message size is less than or equal to MPIR_CVAR_CH4_IPC_GPU_FAST_COPY_MAX_SIZE (in
        bytes), then enable GPU-basedfast memcpy. The environment variable is valid only when then
        GPU IPC shmmod is enabled.

    - name        : MPIR_CVAR_CH4_IPC_ZE_SHAREABLE_HANDLE
      category    : CH4
      type        : enum
      default     : drmfd
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select implementation for ZE shareable IPC handle
        pidfd - use pidfd_getfd syscall to implement shareable IPC handle
        drmfd - force to use device fd-based shareable IPC handle

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

#include "gpu_pre.h"

int MPIDI_GPU_get_ipc_type(MPIDI_IPCI_type_t * ipc_type);
int MPIDI_GPU_get_ipc_threshold(size_t * threshold);
int MPIDI_GPU_get_ipc_attr(const void *vaddr, int rank, MPIR_Comm * comm,
                           MPIDI_IPCI_ipc_attr_t * ipc_attr);
int MPIDI_GPU_ipc_get_map_dev(int remote_global_dev_id, int local_dev_id, MPI_Datatype datatype);
int MPIDI_GPU_ipc_handle_map(MPIDI_GPU_ipc_handle_t handle, int map_dev_id, void **vaddr,
                             bool do_mmap);
int MPIDI_GPU_ipc_handle_unmap(void *vaddr, MPIDI_GPU_ipc_handle_t handle, int do_mmap);
int MPIDI_GPU_init_local(void);
int MPIDI_GPU_init_world(void);
int MPIDI_GPU_mpi_finalize_hook(void);
int MPIDI_GPU_ipc_handle_cache_insert(int rank, MPIR_Comm * comm, MPIDI_GPU_ipc_handle_t handle);

int MPIDI_GPU_ipc_fast_memcpy(MPIDI_IPCI_ipc_handle_t ipc_handle, void *dest_vaddr,
                              MPI_Aint src_data_sz, MPI_Datatype datatype);
int MPIDI_GPU_ipc_event_pool_handle_create(MPIDI_GPU_ipc_event_pool_handle_t *
                                           ipc_event_pool_handle);
int MPIDI_GPU_ipc_event_pool_handle_open(MPL_gpu_ipc_event_pool_handle_t ipc_event_pool_handle,
                                         MPL_gpu_event_pool_handle_t * mapped_event_pool_handle);
int MPIDI_GPU_ipc_event_pool_handle_close(MPL_gpu_event_pool_handle_t mapped_event_pool_handle);
int MPIDI_GPU_ipc_event_pool_handle_size(void);

int MPIDI_GPU_ipc_alltoall_stream_read(void **remote_bufs, void *recv_buf, int count,
                                       MPI_Datatype datatype, int comm_size,
                                       int comm_rank, int *rank_to_global_dev_id);
int MPIDI_GPU_ipc_alltoall_kernel_read(void **remote_bufs, void *recv_buf, int count,
                                       MPI_Datatype datatype, int comm_size,
                                       int comm_rank, int dev_id, const char *datatype_name,
                                       const char *kernel_location);
int MPIDI_GPU_ipc_alltoall_kernel_write(void *send_buf, void **remote_bufs, int count,
                                        MPI_Datatype datatype, int comm_size,
                                        int comm_rank, int dev_id, const char *datatype_name,
                                        const char *kernel_location);

#endif /* GPU_POST_H_INCLUDED */

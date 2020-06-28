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
      default     : 32768
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        If a send message size is greater than or equal to MPIR_CVAR_CH4_IPC_GPU_P2P_THRESHOLD (in
        bytes), then enable GPU-based single copy protocol for intranode communication. The
        environment variable is valid only when then GPU IPC shmmod is enabled.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

#include "gpu_pre.h"

int MPIDI_GPU_get_ipc_attr(const void *vaddr, int rank, MPIR_Comm * comm,
                           MPIDI_IPCI_ipc_attr_t * ipc_attr);
int MPIDI_GPU_ipc_handle_map(MPIDI_GPU_ipc_handle_t handle,
                             MPL_gpu_device_handle_t dev_handle,
                             MPI_Datatype recv_type, void **vaddr);
int MPIDI_GPU_ipc_handle_unmap(void *vaddr, MPIDI_GPU_ipc_handle_t handle);
int MPIDI_GPU_mpi_init_hook(int rank, int size, int *tag_bits);
int MPIDI_GPU_mpi_finalize_hook(void);
int MPIDI_GPU_ipc_handle_cache_insert(int rank, MPIR_Comm * comm, MPIDI_GPU_ipc_handle_t handle);

#endif /* GPU_POST_H_INCLUDED */

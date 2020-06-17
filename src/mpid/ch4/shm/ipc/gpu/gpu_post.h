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

int MPIDI_GPU_get_mem_attr(const void *vaddr, MPIDI_IPCI_mem_attr_t * attr);
int MPIDI_GPU_attach_mem(MPIDI_GPU_mem_handle_t mem_handle,
                         MPL_gpu_device_handle_t dev_handle, void **vaddr);
int MPIDI_GPU_close_mem(void *vaddr, MPL_gpu_ipc_mem_handle_t ipc_handle);
int MPIDI_GPU_mpi_init_hook(int rank, int size, int *tag_bits);
int MPIDI_GPU_mpi_finalize_hook(void);

#endif /* GPU_POST_H_INCLUDED */

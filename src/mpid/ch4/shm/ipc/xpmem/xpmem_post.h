/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */
#ifndef XPMEM_POST_H_INCLUDED
#define XPMEM_POST_H_INCLUDED

#include "ch4_impl.h"
#include "ipc_types.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_CH4_XPMEM_ENABLE
      category    : CH4
      type        : int
      default     : 1
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        To manually disable XPMEM set to 0. The environment variable is valid only when the XPMEM
        submodule is enabled.

    - name        : MPIR_CVAR_CH4_IPC_XPMEM_P2P_THRESHOLD
      category    : CH4
      type        : int
      default     : 4096
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        If a send message size is greater than or equal to MPIR_CVAR_CH4_IPC_XPMEM_P2P_THRESHOLD (in
        bytes), then enable XPMEM-based single copy protocol for intranode communication. The
        environment variable is valid only when the XPMEM submodule is enabled.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

MPL_STATIC_INLINE_PREFIX int MPIDI_XPMEM_get_mem_attr(const void *vaddr,
                                                      MPIDI_IPCI_mem_attr_t * attr)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_XPMEM_GET_MEM_ATTR);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_XPMEM_GET_MEM_ATTR);

    memset(&attr->mem_handle, 0, sizeof(MPIDI_IPCI_mem_handle_t));

#ifdef MPIDI_CH4_SHM_ENABLE_XPMEM
    attr->ipc_type = MPIDI_IPCI_TYPE__XPMEM;
    attr->mem_handle.xpmem.src_offset = (uint64_t) vaddr;
    if (MPIR_CVAR_CH4_XPMEM_ENABLE)
        attr->threshold.send_lmt_sz = MPIR_CVAR_CH4_IPC_XPMEM_P2P_THRESHOLD;
    else
        attr->threshold.send_lmt_sz = MPIR_AINT_MAX;
#else
    attr->ipc_type = MPIDI_IPCI_TYPE__NONE;
    attr->threshold.send_lmt_sz = MPIR_AINT_MAX;
#endif

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_XPMEM_GET_MEM_ATTR);
    return MPI_SUCCESS;
}

int MPIDI_XPMEM_mpi_init_hook(int rank, int size, int *tag_bits);
int MPIDI_XPMEM_mpi_finalize_hook(void);
int MPIDI_XPMEM_attach_mem(int node_rank, MPIDI_XPMEM_mem_handle_t handle, size_t size,
                           MPIDI_XPMEM_mem_seg_t * mem_seg, void **vaddr);
int MPIDI_XPMEM_close_mem(MPIDI_XPMEM_mem_seg_t mem_seg);


#endif /* XPMEM_POST_H_INCLUDED */

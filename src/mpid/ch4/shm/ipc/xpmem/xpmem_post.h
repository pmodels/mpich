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
      default     : 65536
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        If a send message size is greater than or equal to MPIR_CVAR_CH4_IPC_XPMEM_P2P_THRESHOLD (in
        bytes), then enable XPMEM-based single copy protocol for intranode communication. The
        environment variable is valid only when the XPMEM submodule is enabled.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

#ifdef MPIDI_CH4_SHM_ENABLE_XPMEM
MPL_STATIC_INLINE_PREFIX int MPIDI_XPMEM_get_ipc_attr(const void *buf, MPI_Aint count,
                                                      MPI_Datatype datatype,
                                                      MPIDI_IPCI_ipc_attr_t * ipc_attr)
{
    MPIR_FUNC_ENTER;

    ipc_attr->ipc_type = MPIDI_IPCI_TYPE__NONE;

    MPIR_Datatype *dt_ptr;
    MPI_Aint true_lb, data_sz;
    int dt_contig;
    MPIDI_Datatype_get_info(count, datatype, dt_contig, data_sz, dt_ptr, true_lb);

    if (!MPIR_CVAR_CH4_XPMEM_ENABLE || buf == MPI_BOTTOM ||
        data_sz < MPIR_CVAR_CH4_IPC_XPMEM_P2P_THRESHOLD) {
        goto fn_exit;
    } else {
        ipc_attr->ipc_type = MPIDI_IPCI_TYPE__XPMEM;
        ipc_attr->u.xpmem.buf = buf;
        ipc_attr->u.xpmem.count = count;
        ipc_attr->u.xpmem.datatype = datatype;
    }
  fn_exit:
    MPIR_FUNC_EXIT;
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX void MPIDI_XPMEM_fill_ipc_handle(MPIDI_IPCI_ipc_attr_t * ipc_attr,
                                                          MPIDI_IPCI_ipc_handle_t * ipc_handle)
{
    MPIR_Datatype *dt_ptr;
    MPI_Aint true_lb, data_sz;
    int dt_contig;
    MPIDI_Datatype_get_info(ipc_attr->u.xpmem.count, ipc_attr->u.xpmem.datatype,
                            dt_contig, data_sz, dt_ptr, true_lb);

    ipc_handle->xpmem.src_lrank = MPIR_Process.local_rank;
    ipc_handle->xpmem.is_contig = dt_contig;
    ipc_handle->xpmem.addr = ipc_attr->u.xpmem.buf;
    ipc_handle->xpmem.true_lb = true_lb;

    if (dt_contig) {
        ipc_handle->xpmem.range = data_sz;
    } else {
        MPIR_Assert(ipc_attr->u.xpmem.count > 0);
        MPI_Aint extent = dt_ptr->ub - dt_ptr->lb;
        MPI_Aint true_extent = dt_ptr->true_ub - dt_ptr->true_lb;
        if (extent >= 0) {
            ipc_handle->xpmem.range = true_extent + extent * (ipc_attr->u.xpmem.count - 1);
        } else {
            MPI_Aint neg_extent = extent * (ipc_attr->u.xpmem.count - 1);
            ipc_handle->xpmem.addr = (char *) ipc_handle->xpmem.addr + neg_extent;
            ipc_handle->xpmem.range = true_extent - neg_extent;
        }
    }
}

int MPIDI_XPMEM_init_local(void);
int MPIDI_XPMEM_init_world(void);
int MPIDI_XPMEM_mpi_finalize_hook(void);
int MPIDI_XPMEM_ipc_handle_map(MPIDI_XPMEM_ipc_handle_t mem_handle, void **vaddr);
#endif

#endif /* XPMEM_POST_H_INCLUDED */

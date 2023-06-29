/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */
#ifndef CMA_POST_H_INCLUDED
#define CMA_POST_H_INCLUDED

#include "ch4_impl.h"
#include "ipc_types.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_CH4_CMA_ENABLE
      category    : CH4
      type        : int
      default     : 1
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        To manually disable CMA set to 0. The environment variable is valid only when the CMA
        submodule is enabled.

    - name        : MPIR_CVAR_CH4_IPC_CMA_P2P_THRESHOLD
      category    : CH4
      type        : int
      default     : 16384
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        If a send message size is greater than or equal to MPIR_CVAR_CH4_IPC_CMA_P2P_THRESHOLD (in
        bytes), then enable CMA-based single copy protocol for intranode communication. The
        environment variable is valid only when the CMA submodule is enabled.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

MPL_STATIC_INLINE_PREFIX int MPIDI_CMA_get_ipc_attr(const void *buf, MPI_Aint count,
                                                    MPI_Datatype datatype,
                                                    MPIDI_IPCI_ipc_attr_t * ipc_attr)
{
    MPIR_FUNC_ENTER;

    ipc_attr->ipc_type = MPIDI_IPCI_TYPE__NONE;
    if (buf == MPI_BOTTOM) {
        goto fn_exit;
    }
#ifdef MPIDI_CH4_SHM_ENABLE_CMA
    if (MPIR_CVAR_CH4_CMA_ENABLE) {
        MPI_Aint data_sz, num_blocks;
        if (HANDLE_IS_BUILTIN(datatype)) {
            data_sz = MPIR_Datatype_get_basic_size(datatype);
            num_blocks = 1;
        } else {
            MPIR_Datatype *dt_ptr;
            MPIR_Datatype_get_ptr(datatype, dt_ptr);
            data_sz = dt_ptr->size;
            if (dt_ptr->is_contig) {
                num_blocks = 1;
            } else {
                num_blocks = count * dt_ptr->typerep.num_contig_blocks;
            }
        }

        if (data_sz < MPIR_CVAR_CH4_IPC_CMA_P2P_THRESHOLD) {
            goto fn_exit;
        } else if (num_blocks > IOV_MAX) {
            goto fn_exit;
        } else {
            ipc_attr->ipc_type = MPIDI_IPCI_TYPE__CMA;
            ipc_attr->u.cma.buf = buf;
            ipc_attr->u.cma.count = count;
            ipc_attr->u.cma.datatype = datatype;
            goto fn_exit;
        }
    }
#endif

  fn_exit:
    MPIR_FUNC_EXIT;
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX void MPIDI_CMA_fill_ipc_handle(MPIDI_IPCI_ipc_attr_t * ipc_attr,
                                                        MPIDI_IPCI_ipc_handle_t * ipc_handle)
{
#ifdef MPIDI_CH4_SHM_ENABLE_CMA
    ipc_handle->cma.pid = getpid();

    MPIR_Datatype *dt_ptr;
    MPI_Aint true_lb, data_sz;
    int dt_contig;
    MPIDI_Datatype_get_info(ipc_attr->u.cma.count, ipc_attr->u.cma.datatype,
                            dt_contig, data_sz, dt_ptr, true_lb);
    if (dt_contig) {
        ipc_handle->cma.vaddr = MPIR_get_contig_ptr(ipc_attr->u.cma.buf, true_lb);
        ipc_handle->cma.data_sz = data_sz;
    } else {
        ipc_handle->cma.vaddr = ipc_attr->u.cma.buf;
        ipc_handle->cma.data_sz = data_sz;
    }
#endif
}

int MPIDI_CMA_init_world(void);
int MPIDI_CMA_mpi_finalize_hook(void);
int MPIDI_CMA_copy_data(MPIDI_IPC_hdr * ipc_hdr, MPIR_Request * rreq, MPI_Aint src_data_sz);

#endif /* CMA_POST_H_INCLUDED */

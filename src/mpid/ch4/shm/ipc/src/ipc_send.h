/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef IPC_SEND_H_INCLUDED
#define IPC_SEND_H_INCLUDED

#include "ch4_impl.h"
#include "mpidimpl.h"
#include "ipc_types.h"
#include "../xpmem/xpmem_post.h"
#include "../gpu/gpu_post.h"
#include "ipc_p2p.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===
cvars:
    - name        : MPIR_CVAR_CH4_IPC_MAP_REPEAT_ADDR
      category    : CH4
      type        : boolean
      default     : true
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        If an address is used more than once in the last ten send operations,
        map it for IPC use even if it is below the IPC threshold.
=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

MPL_STATIC_INLINE_PREFIX bool MPIDI_IPCI_is_repeat_addr(void *addr)
{
    if (!MPIR_CVAR_CH4_IPC_MAP_REPEAT_ADDR) {
        return false;
    }

    static void *repeat_addr[10] = { 0 };
    static int addr_idx = 0;

    for (int i = 0; i < 10; i++) {
        if (addr == repeat_addr[i]) {
            return true;
        }
    }

    repeat_addr[addr_idx] = addr;
    addr_idx = (addr_idx + 1) % 10;
    return false;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_IPCI_try_lmt_isend(const void *buf, MPI_Aint count,
                                                      MPI_Datatype datatype, int rank, int tag,
                                                      MPIR_Comm * comm, int attr,
                                                      MPIDI_av_entry_t * addr,
                                                      MPIR_Request ** request, bool * done)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    int context_offset = MPIR_PT2PT_ATTR_CONTEXT_OFFSET(attr);
    MPIR_Errflag_t errflag = MPIR_PT2PT_ATTR_GET_ERRFLAG(attr);
    bool syncflag = MPIR_PT2PT_ATTR_GET_SYNCFLAG(attr);
    int vci_src, vci_dst;
    /* note: MPIDI_POSIX_SEND_VSIS defined in posix_send.h */
    MPIDI_POSIX_SEND_VSIS(vci_src, vci_dst);

    MPIDI_POSIX_THREAD_CS_ENTER_VCI(vci_src);

    MPIR_Datatype *dt_ptr;
    bool dt_contig;
    MPI_Aint true_lb;
    uintptr_t data_sz;
    MPIDI_Datatype_get_info(count, datatype, dt_contig, data_sz, dt_ptr, true_lb);

    if (!dt_contig) {
        if (dt_ptr->contents) {
            /* skip HINDEXED and STRUCT */
            int combiner = dt_ptr->contents->combiner;
            MPIR_Datatype *tmp_dtp = dt_ptr;
            while (combiner == MPI_COMBINER_DUP || combiner == MPI_COMBINER_RESIZED) {
                int *ints;
                MPI_Aint *aints, *counts;
                MPI_Datatype *types;
                MPIR_Datatype_access_contents(tmp_dtp->contents, &ints, &aints, &counts, &types);
                MPIR_Datatype_get_ptr(types[0], tmp_dtp);
                if (!tmp_dtp || !tmp_dtp->contents) {
                    break;
                }
                combiner = tmp_dtp->contents->combiner;
            }
            switch (combiner) {
                case MPI_COMBINER_HINDEXED:
                case MPI_COMBINER_STRUCT:
                    goto fn_exit;
            }
        }

        /* skip negative lb and extent */
        if (dt_ptr->true_lb < 0 || dt_ptr->extent < 0) {
            goto fn_exit;
        }
    }

    MPI_Aint mem_size;
    void *mem_addr;
    if (dt_contig) {
        mem_addr = MPIR_get_contig_ptr(buf, true_lb);
        mem_size = data_sz;
    } else {
        mem_addr = (char *) buf;
        mem_size = dt_ptr->true_ub + (count - 1) * dt_ptr->extent;
    }
    MPIDI_IPCI_ipc_attr_t ipc_attr;
    memset(&ipc_attr, 0, sizeof(ipc_attr));
    MPIR_GPU_query_pointer_attr(mem_addr, &ipc_attr.gpu_attr);

    bool do_ipc = false;
    if (!do_ipc && ipc_attr.gpu_attr.type == MPL_GPU_POINTER_DEV) {
        mpi_errno = MPIDI_GPU_get_ipc_attr(mem_addr, rank, comm, &ipc_attr);
        MPIR_ERR_CHECK(mpi_errno);
        do_ipc = (ipc_attr.ipc_type != MPIDI_IPCI_TYPE__NONE);
    }
    if (!do_ipc && !MPL_gpu_query_pointer_is_dev(buf, &ipc_attr.gpu_attr)) {
        /* The result of MPL_gpu_query_pointer_is_dev is not necessarily equivalent to
         * (gpu_attr.type == MPL_GPU_POINTER_DEV) depending on the backend. This explicit check
         * ensures the pointer can be accepted by XPMEM and work as intended. */
        mpi_errno = MPIDI_XPMEM_get_ipc_attr(mem_addr, mem_size, &ipc_attr);
        MPIR_ERR_CHECK(mpi_errno);
        do_ipc = (ipc_attr.ipc_type != MPIDI_IPCI_TYPE__NONE);
    }

    if (!do_ipc) {
        goto fn_exit;
    }

    if (!dt_contig) {
        int flattened_sz;
        void *flattened_dt;
        MPIR_Datatype_get_flattened(datatype, &flattened_dt, &flattened_sz);
        if (sizeof(MPIDI_IPC_rts_t) + flattened_sz > MPIDI_POSIX_am_hdr_max_sz()) {
            do_ipc = false;
        }
    }

    if (do_ipc) {
        mpi_errno = MPIDI_IPCI_send_lmt(buf, count, datatype, data_sz, dt_contig,
                                        rank, tag, comm, context_offset, addr, ipc_attr,
                                        vci_src, vci_dst, request, syncflag, errflag);
        MPIR_ERR_CHECK(mpi_errno);
        *done = true;
    }
  fn_exit:
    MPIDI_POSIX_THREAD_CS_EXIT_VCI(vci_src);
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_IPC_mpi_isend(const void *buf, MPI_Aint count,
                                                 MPI_Datatype datatype, int rank, int tag,
                                                 MPIR_Comm * comm, int attr,
                                                 MPIDI_av_entry_t * addr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    bool done = false;
    mpi_errno = MPIDI_IPCI_try_lmt_isend(buf, count, datatype, rank, tag, comm,
                                         attr, addr, request, &done);
    MPIR_ERR_CHECK(mpi_errno);

    if (!done) {
        mpi_errno = MPIDI_POSIX_mpi_isend(buf, count, datatype, rank, tag, comm,
                                          attr, addr, request);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* IPC_SEND_H_INCLUDED */

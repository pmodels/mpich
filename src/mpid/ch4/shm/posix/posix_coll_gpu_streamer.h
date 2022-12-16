/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_BCAST_STREAMER_READ_MSG_SIZE_THRESHOLD
      category    : COLLECTIVE
      type        : int
      default     : 1024
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Use gpu streamer read bcast only when the message size is more than this
        threshold.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

#ifndef POSIX_COLL_GPU_STREAMER_H_INCLUDED
#define POSIX_COLL_GPU_STREAMER_H_INCLUDED

#include "../ipc/src/ipc_types.h"
#include "../ipc/src/ipc_p2p.h"
#include "../gpu/gpu_post.h"
#include "../../../include/mpir_err.h"

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_mpi_bcast_gpu_stream_read(void *buffer,
                                                                   MPI_Aint count,
                                                                   MPI_Datatype datatype,
                                                                   int root, MPIR_Comm * comm_ptr,
                                                                   MPIR_Errflag_t * errflag)
{
    MPIR_FUNC_ENTER;
    MPIR_CHKLMEM_DECL(1);

    int mpi_errno = MPI_SUCCESS, mpi_errno_ret = MPI_SUCCESS;

    MPI_Aint true_lb;
    MPIR_Datatype *dt_ptr;
    bool dt_contig;
    uintptr_t data_sz;
    MPIDI_Datatype_get_info(count, datatype, dt_contig, data_sz, dt_ptr, true_lb);
    /* fallback if datatype is not contigous or data size is not large enough or data has been
     * switched to CPU buffer in MPIDI_Bcast_intra_composition_*
     */
    if (!dt_contig || data_sz <= MPIR_CVAR_BCAST_STREAMER_READ_MSG_SIZE_THRESHOLD ||
        data_sz <= MPIR_CVAR_CH4_GPU_COLL_SWAP_BUFFER_SZ) {
        goto fallback;
    }

    void *mem_addr = MPIR_get_contig_ptr(buffer, true_lb);
    int my_rank = MPIR_Comm_rank(comm_ptr);
    int comm_size = MPIR_Comm_size(comm_ptr);

    MPIDI_IPCI_ipc_attr_t ipc_attr;
    memset(&ipc_attr, 0, sizeof(ipc_attr));
    MPIR_GPU_query_pointer_attr(mem_addr, &ipc_attr.gpu_attr);

    /* set up ipc_handles */
    MPIDI_IPCI_ipc_handle_t *ipc_handles = NULL;
    MPIR_CHKLMEM_MALLOC(ipc_handles, MPIDI_IPCI_ipc_handle_t *,
                        sizeof(MPIDI_IPCI_ipc_handle_t) * comm_size, mpi_errno, "IPC handles",
                        MPL_MEM_COLL);

    MPIDI_IPCI_ipc_handle_t my_ipc_handle;
    memset(&my_ipc_handle, 0, sizeof(my_ipc_handle));
    int dev_id = MPL_gpu_get_dev_id_from_attr(&ipc_attr.gpu_attr);
    /* CPU memory, registered host memory and USM fallback to P2P bcast,
     * GPU memory will use stream read bcast
     */
    if (dev_id >= 0 && ipc_attr.gpu_attr.type == MPL_GPU_POINTER_DEV) {
        mpi_errno = MPIDI_GPU_get_ipc_attr(mem_addr, my_rank, comm_ptr, &ipc_attr);
        MPIR_ERR_CHECK(mpi_errno);
        my_ipc_handle = ipc_attr.ipc_handle;
    } else {
        my_ipc_handle.gpu.global_dev_id = -1;
    }

    /* allgather is needed to exchange all the IPC handles */
    mpi_errno =
        MPIR_Allgather_impl(&my_ipc_handle, sizeof(MPIDI_IPCI_ipc_handle_t), MPI_BYTE, ipc_handles,
                            sizeof(MPIDI_IPCI_ipc_handle_t), MPI_BYTE, comm_ptr, errflag);
    MPIR_ERR_CHECK(mpi_errno);

    /* check the ipc_handles to make sure all the buffers are on GPU */
    int errs = 0;
    for (int i = 0; i < comm_size; i++) {
        if (ipc_handles[i].gpu.global_dev_id < 0) {
            errs++;
            break;
        }
    }
    if (errs > 0) {
        goto fallback;
    }

    if (my_rank != root) {
        /* map root's ipc_handle to remote_buf */
        void *remote_buf = NULL;
        bool do_mmap = (data_sz <= MPIR_CVAR_CH4_IPC_GPU_FAST_COPY_MAX_SIZE);
        int root_dev =
            MPIDI_GPU_ipc_get_map_dev(ipc_handles[root].gpu.global_dev_id, dev_id, datatype);
        mpi_errno = MPIDI_GPU_ipc_handle_map(ipc_handles[root].gpu, root_dev, &remote_buf, do_mmap);
        MPIR_ERR_CHECK(mpi_errno);

        /* get engine type */
        MPL_gpu_engine_type_t engine_type =
            MPIDI_IPCI_choose_engine(ipc_handles[root].gpu.global_dev_id,
                                     my_ipc_handle.gpu.global_dev_id);

        /* copy data from root */
        mpi_errno = MPIR_Localcopy_gpu(remote_buf, count, datatype, 0, NULL,
                                       mem_addr, count, datatype, 0, &ipc_attr.gpu_attr,
                                       MPL_GPU_COPY_D2D_INCOMING, engine_type, true);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    MPIR_FUNC_EXIT;
    return mpi_errno_ret;
  fn_fail:
    goto fn_exit;
  fallback:
    /* Fall back to other algorithms as gpu streamer based bcast cannot be used */
    mpi_errno = MPIR_Bcast_impl(buffer, count, datatype, root, comm_ptr, errflag);
    MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, *errflag);
    goto fn_exit;

}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_mpi_alltoall_gpu_stream_read(const void *sendbuf,
                                                                      MPI_Aint sendcount,
                                                                      MPI_Datatype sendtype,
                                                                      void *recvbuf,
                                                                      MPI_Aint recvcount,
                                                                      MPI_Datatype recvtype,
                                                                      MPIR_Comm * comm_ptr,
                                                                      MPIR_Errflag_t * errflag)
{
    MPIR_FUNC_ENTER;
    MPIR_CHKLMEM_DECL(3);

    int mpi_errno = MPI_SUCCESS, mpi_errno_ret = MPI_SUCCESS;

    /* fallback if sendtype and recvtype is different */
    if (sendtype != recvtype || sendcount != recvcount) {
        goto fallback;
    }
    MPI_Aint true_lb;
    MPIR_Datatype *dt_ptr;
    bool dt_contig;
    uintptr_t data_sz;
    MPIDI_Datatype_get_info(sendcount, sendtype, dt_contig, data_sz, dt_ptr, true_lb);
    /* fallback if data_sz is 0 or datatype is not contigous */
    if (data_sz == 0 || (!dt_contig && data_sz > 0)) {
        goto fallback;
    }

    void *send_mem_addr = MPIR_get_contig_ptr(sendbuf, true_lb);
    void *recv_mem_addr = MPIR_get_contig_ptr(recvbuf, true_lb);
    MPIDI_IPCI_ipc_attr_t ipc_attr;
    memset(&ipc_attr, 0, sizeof(ipc_attr));
    MPIR_GPU_query_pointer_attr(send_mem_addr, &ipc_attr.gpu_attr);

    int my_rank = MPIR_Comm_rank(comm_ptr);
    int comm_size = MPIR_Comm_size(comm_ptr);
    /* set up ipc_handles */
    MPIDI_IPCI_ipc_handle_t *ipc_handles = NULL;
    MPIR_CHKLMEM_MALLOC(ipc_handles, MPIDI_IPCI_ipc_handle_t *,
                        sizeof(MPIDI_IPCI_ipc_handle_t) * comm_size, mpi_errno, "IPC handles",
                        MPL_MEM_COLL);
    MPIDI_IPCI_ipc_handle_t my_ipc_handle;
    memset(&my_ipc_handle, 0, sizeof(my_ipc_handle));
    int dev_id = MPL_gpu_get_dev_id_from_attr(&ipc_attr.gpu_attr);
    /* CPU memory, registered host memory and USM fallback to P2P alltoall,
     * GPU memory will use stream read alltoall
     */
    if (dev_id >= 0 && ipc_attr.gpu_attr.type == MPL_GPU_POINTER_DEV) {
        mpi_errno = MPIDI_GPU_get_ipc_attr(send_mem_addr, my_rank, comm_ptr, &ipc_attr);
        MPIR_ERR_CHECK(mpi_errno);
        my_ipc_handle = ipc_attr.ipc_handle;
    } else {
        my_ipc_handle.gpu.global_dev_id = -1;
    }
    /* allgather is needed to exchange all the IPC handles */
    mpi_errno =
        MPIR_Allgather_impl(&my_ipc_handle, sizeof(MPIDI_IPCI_ipc_handle_t), MPI_BYTE, ipc_handles,
                            sizeof(MPIDI_IPCI_ipc_handle_t), MPI_BYTE, comm_ptr, errflag);
    MPIR_ERR_CHECK(mpi_errno);

    /* check the ipc_handles to make sure all the buffers are on GPU */
    int errs = 0;
    for (int i = 0; i < comm_size; i++) {
        if (ipc_handles[i].gpu.global_dev_id < 0) {
            errs++;
            break;
        }
    }
    if (errs > 0) {
        goto fallback;
    }

    /* map ipc_handles to remote_bufs */
    void **remote_bufs = NULL;
    MPIR_CHKLMEM_MALLOC(remote_bufs, void **, sizeof(void *) * comm_size, mpi_errno, "Remote bufs",
                        MPL_MEM_COLL);
    int *rank_to_global_dev_id = NULL;
    MPIR_CHKLMEM_MALLOC(rank_to_global_dev_id, int *,
                        sizeof(int) * comm_size, mpi_errno, "Rank to global dev_id", MPL_MEM_COLL);
    for (int i = 0; i < comm_size; i++) {
        if (i != my_rank) {
            int remote_dev =
                MPIDI_GPU_ipc_get_map_dev(ipc_handles[i].gpu.global_dev_id, dev_id, sendtype);
            mpi_errno =
                MPIDI_GPU_ipc_handle_map(ipc_handles[i].gpu, remote_dev, &(remote_bufs[i]), false);
            MPIR_ERR_CHECK(mpi_errno);
        } else {
            remote_bufs[i] = send_mem_addr;
        }
        rank_to_global_dev_id[i] = ipc_handles[i].gpu.global_dev_id;
    }
    mpi_errno =
        MPIDI_GPU_ipc_alltoall_stream_read(remote_bufs, recv_mem_addr, sendcount, sendtype,
                                           MPIR_Comm_size(comm_ptr), my_rank,
                                           rank_to_global_dev_id);
    MPIR_ERR_CHECK(mpi_errno);
  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    MPIR_FUNC_EXIT;
    return mpi_errno_ret;
  fn_fail:
    goto fn_exit;
  fallback:
    /* Fall back to other algorithms as gpu streamer based alltoall cannot be used */
    mpi_errno = MPIR_Alltoall_impl(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype,
                                   comm_ptr, errflag);
    MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, *errflag);
    goto fn_exit;
}

#endif /* POSIX_COLL_GPU_STREAMER_H_INCLUDED */

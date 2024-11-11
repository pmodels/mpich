/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_BCAST_IPC_READ_MSG_SIZE_THRESHOLD
      category    : COLLECTIVE
      type        : int
      default     : 256
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Use gpu ipc read bcast only when the message size is larger than this
        threshold.

    - name        : MPIR_CVAR_ALLTOALL_IPC_READ_MSG_SIZE_THRESHOLD
      category    : COLLECTIVE
      type        : int
      default     : 256
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Use gpu ipc read alltoall only when the message size is larger than this
        threshold.

    - name        : MPIR_CVAR_ALLGATHER_IPC_READ_MSG_SIZE_THRESHOLD
      category    : COLLECTIVE
      type        : int
      default     : 256
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Use gpu ipc read allgather only when the message size is larger than this
        threshold.

    - name        : MPIR_CVAR_ALLGATHERV_IPC_READ_MSG_SIZE_THRESHOLD
      category    : COLLECTIVE
      type        : int
      default     : 256
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Use gpu ipc read allgatherv only when the message size is larger than this
        threshold.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

#ifndef POSIX_COLL_GPU_IPC_H_INCLUDED
#define POSIX_COLL_GPU_IPC_H_INCLUDED

#include "../ipc/src/ipc_types.h"
#include "../ipc/src/ipc_p2p.h"
#include "../ipc/gpu/gpu_post.h"
#include "../../../include/mpir_err.h"

#ifdef MPIDI_CH4_SHM_ENABLE_GPU
static int allgather_ipc_handles(const void *buf, MPI_Aint count, MPI_Datatype datatype,
                                 MPIR_Comm * comm, int coll_group,
                                 int threshold, MPI_Aint * data_sz_out,
                                 void **mem_addr_out, MPIDI_IPCI_ipc_handle_t ** ipc_handles_out)
{
    int mpi_errno = MPI_SUCCESS;

    MPI_Aint true_lb, data_sz;
    MPIR_Datatype *dt_ptr;
    bool dt_contig;
    MPIDI_Datatype_get_info(count, datatype, dt_contig, data_sz, dt_ptr, true_lb);
    /* fallback if datatype is not contiguous or data size is not large enough */
    if (!dt_contig || data_sz <= threshold) {
        *ipc_handles_out = NULL;
        goto fn_exit;
    }

    int comm_size = MPIR_Comm_size(comm);
    void *mem_addr = MPIR_get_contig_ptr(buf, true_lb);

    MPIDI_IPCI_ipc_attr_t ipc_attr;
    mpi_errno = MPIDI_GPU_get_ipc_attr(buf, count, datatype, MPI_PROC_NULL, comm, &ipc_attr);

    MPIDI_IPCI_ipc_handle_t *ipc_handles;
    ipc_handles = MPL_malloc(sizeof(MPIDI_IPCI_ipc_handle_t) * comm_size, MPL_MEM_COLL);
    MPIR_ERR_CHKANDJUMP(!ipc_handles, mpi_errno, MPI_ERR_OTHER, "**nomem");

    MPIDI_IPCI_ipc_handle_t my_ipc_handle;
    memset(&my_ipc_handle, 0, sizeof(my_ipc_handle));
    if (ipc_attr.ipc_type == MPIDI_IPCI_TYPE__GPU) {
        mpi_errno = MPIDI_GPU_fill_ipc_handle(&ipc_attr, &my_ipc_handle);
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        my_ipc_handle.gpu.global_dev_id = -1;
    }

    /* allgather is needed to exchange all the IPC handles */
    mpi_errno = MPIR_Allgather_impl(&my_ipc_handle, sizeof(MPIDI_IPCI_ipc_handle_t), MPI_BYTE,
                                    ipc_handles, sizeof(MPIDI_IPCI_ipc_handle_t), MPI_BYTE,
                                    comm, coll_group, MPIR_ERR_NONE);
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
        MPL_free(ipc_handles);
        *ipc_handles_out = NULL;
    } else {
        *data_sz_out = data_sz;
        *mem_addr_out = mem_addr;
        *ipc_handles_out = ipc_handles;
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_mpi_bcast_gpu_ipc_read(void *buffer,
                                                                MPI_Aint count,
                                                                MPI_Datatype datatype,
                                                                int root, MPIR_Comm * comm_ptr,
                                                                int coll_group,
                                                                MPIR_Errflag_t errflag)
{
    MPIR_FUNC_ENTER;
    int mpi_errno = MPI_SUCCESS;

    int my_rank = MPIR_Comm_rank(comm_ptr);

    MPI_Aint data_sz;
    void *mem_addr;
    MPIDI_IPCI_ipc_handle_t *ipc_handles = NULL;
    mpi_errno = allgather_ipc_handles(buffer, count, datatype, comm_ptr, coll_group,
                                      MPIR_CVAR_BCAST_IPC_READ_MSG_SIZE_THRESHOLD,
                                      &data_sz, &mem_addr, &ipc_handles);
    MPIR_ERR_CHECK(mpi_errno);

    if (!ipc_handles) {
        goto fallback;
    }

    if (my_rank != root) {
        /* map root's ipc_handle to remote_buf */
        void *remote_buf = NULL;
        bool do_mmap = (data_sz <= MPIR_CVAR_GPU_FAST_COPY_MAX_SIZE);
        int dev_id = ipc_handles[my_rank].gpu.local_dev_id;
        int root_dev =
            MPIDI_GPU_ipc_get_map_dev(ipc_handles[root].gpu.global_dev_id, dev_id, datatype);
        mpi_errno = MPIDI_GPU_ipc_handle_map(ipc_handles[root].gpu, root_dev, &remote_buf, do_mmap);
        MPIR_ERR_CHECK(mpi_errno);

        /* get engine type */
        MPL_gpu_engine_type_t engine_type =
            MPIDI_IPCI_choose_engine(ipc_handles[root].gpu.global_dev_id,
                                     ipc_handles[my_rank].gpu.global_dev_id);

        /* copy data from root */
        MPL_gpu_request req;
        mpi_errno = MPL_gpu_imemcpy(mem_addr, remote_buf, data_sz, dev_id,
                                    MPL_GPU_COPY_DIRECTION_NONE, engine_type, &req, true);
        MPIR_ERR_CHECK(mpi_errno);
        int completed = 0;
        while (!completed) {
            mpi_errno = MPL_gpu_test(&req, &completed);
            MPIR_ERR_CHECK(mpi_errno);
        }
    }

    MPL_free(ipc_handles);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
  fallback:
    /* Fall back to other algorithms as gpu ipc bcast cannot be used */
    mpi_errno = MPIR_Bcast_impl(buffer, count, datatype, root, comm_ptr, coll_group, errflag);
    MPIR_ERR_CHECK(mpi_errno);
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_mpi_alltoall_gpu_ipc_read(const void *sendbuf,
                                                                   MPI_Aint sendcount,
                                                                   MPI_Datatype sendtype,
                                                                   void *recvbuf,
                                                                   MPI_Aint recvcount,
                                                                   MPI_Datatype recvtype,
                                                                   MPIR_Comm * comm_ptr,
                                                                   int coll_group,
                                                                   MPIR_Errflag_t errflag)
{
    MPIR_FUNC_ENTER;
    MPIR_CHKLMEM_DECL(3);

    int mpi_errno = MPI_SUCCESS;

    /* fallback if sendtype and recvtype is different */
    if (sendtype != recvtype || sendcount != recvcount) {
        goto fallback;
    }

    int my_rank = MPIR_Comm_rank(comm_ptr);
    int comm_size = MPIR_Comm_size(comm_ptr);

    MPI_Aint data_sz;
    void *send_mem_addr;
    MPIDI_IPCI_ipc_handle_t *ipc_handles = NULL;
    mpi_errno = allgather_ipc_handles(sendbuf, sendcount, sendtype, comm_ptr, coll_group,
                                      MPIR_CVAR_ALLTOALL_IPC_READ_MSG_SIZE_THRESHOLD,
                                      &data_sz, &send_mem_addr, &ipc_handles);
    MPIR_ERR_CHECK(mpi_errno);

    if (!ipc_handles) {
        goto fallback;
    }

    intptr_t true_lb = (char *) send_mem_addr - (const char *) sendbuf;
    void *recv_mem_addr = (char *) recvbuf + true_lb;
    int dev_id = ipc_handles[my_rank].gpu.local_dev_id;

    /* map ipc_handles to remote_bufs */
    void **remote_bufs = NULL;
    MPIR_CHKLMEM_MALLOC(remote_bufs, void **, sizeof(void *) * comm_size, mpi_errno, "Remote bufs",
                        MPL_MEM_COLL);
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
    }
    /* use imemcpy to copy the data concurrently */
    MPL_gpu_request *reqs = NULL;
    MPIR_CHKLMEM_MALLOC(reqs, MPL_gpu_request *, sizeof(MPL_gpu_request) * comm_size, mpi_errno,
                        "Memcpy requests", MPL_MEM_COLL);
    for (int i = 0; i < comm_size; i++) {
        int target = (my_rank + 1 + i) % comm_size;
        char *temp_recv = (char *) recv_mem_addr + target * data_sz;
        char *temp_send = (char *) (remote_bufs[target]) + my_rank * data_sz;
        /* get engine type */
        MPL_gpu_engine_type_t engine_type =
            MPIDI_IPCI_choose_engine(ipc_handles[target].gpu.global_dev_id,
                                     ipc_handles[my_rank].gpu.global_dev_id);
        mpi_errno = MPL_gpu_imemcpy(temp_recv, temp_send, data_sz, dev_id,
                                    MPL_GPU_COPY_DIRECTION_NONE, engine_type, &reqs[i], true);
        MPIR_ERR_CHECK(mpi_errno);
    }
    /* wait for the imemcpy to finish */
    for (int i = 0; i < comm_size; i++) {
        int completed = 0;
        while (!completed) {
            mpi_errno = MPL_gpu_test(&reqs[i], &completed);
            MPIR_ERR_CHECK(mpi_errno);
        }
    }

    MPL_free(ipc_handles);

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
  fallback:
    /* Fall back to other algorithms as gpu ipc alltoall cannot be used */
    mpi_errno = MPIR_Alltoall_impl(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype,
                                   comm_ptr, coll_group, errflag);
    MPIR_ERR_CHECK(mpi_errno);
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_mpi_allgather_gpu_ipc_read(const void *sendbuf,
                                                                    MPI_Aint sendcount,
                                                                    MPI_Datatype sendtype,
                                                                    void *recvbuf,
                                                                    MPI_Aint recvcount,
                                                                    MPI_Datatype recvtype,
                                                                    MPIR_Comm * comm_ptr,
                                                                    int coll_group,
                                                                    MPIR_Errflag_t errflag)
{
    MPIR_FUNC_ENTER;
    MPIR_CHKLMEM_DECL(3);

    int mpi_errno = MPI_SUCCESS;

    /* fallback if sendtype and recvtype is different */
    if (sendtype != recvtype || sendcount != recvcount) {
        goto fallback;
    }

    int my_rank = MPIR_Comm_rank(comm_ptr);
    int comm_size = MPIR_Comm_size(comm_ptr);

    MPI_Aint data_sz;
    void *send_mem_addr;
    MPIDI_IPCI_ipc_handle_t *ipc_handles = NULL;
    mpi_errno = allgather_ipc_handles(sendbuf, sendcount, sendtype, comm_ptr, coll_group,
                                      MPIR_CVAR_ALLGATHER_IPC_READ_MSG_SIZE_THRESHOLD,
                                      &data_sz, &send_mem_addr, &ipc_handles);
    MPIR_ERR_CHECK(mpi_errno);

    if (!ipc_handles) {
        goto fallback;
    }

    intptr_t true_lb = (char *) send_mem_addr - (const char *) sendbuf;
    void *recv_mem_addr = (char *) recvbuf + true_lb;
    int dev_id = ipc_handles[my_rank].gpu.local_dev_id;

    /* map ipc_handles to remote_bufs */
    void **remote_bufs = NULL;
    MPIR_CHKLMEM_MALLOC(remote_bufs, void **, sizeof(void *) * comm_size, mpi_errno, "Remote bufs",
                        MPL_MEM_COLL);
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
    }
    /* use imemcpy to copy the data concurrently */
    MPL_gpu_request *reqs = NULL;
    MPIR_CHKLMEM_MALLOC(reqs, MPL_gpu_request *, sizeof(MPL_gpu_request) * comm_size, mpi_errno,
                        "Memcpy requests", MPL_MEM_COLL);
    for (int i = 0; i < comm_size; i++) {
        int target = (my_rank + 1 + i) % comm_size;
        char *temp_recv = (char *) recv_mem_addr + target * data_sz;
        char *temp_send = (char *) remote_bufs[target];
        /* get engine type */
        MPL_gpu_engine_type_t engine_type =
            MPIDI_IPCI_choose_engine(ipc_handles[target].gpu.global_dev_id,
                                     ipc_handles[my_rank].gpu.global_dev_id);
        mpi_errno = MPL_gpu_imemcpy(temp_recv, temp_send, data_sz, dev_id,
                                    MPL_GPU_COPY_DIRECTION_NONE, engine_type, &reqs[i], true);
        MPIR_ERR_CHECK(mpi_errno);
    }
    /* wait for the imemcpy to finish */
    for (int i = 0; i < comm_size; i++) {
        int completed = 0;
        while (!completed) {
            mpi_errno = MPL_gpu_test(&reqs[i], &completed);
            MPIR_ERR_CHECK(mpi_errno);
        }
    }

    MPL_free(ipc_handles);

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
  fallback:
    /* Fall back to other algorithms as gpu ipc allgather cannot be used */
    mpi_errno = MPIR_Allgather_impl(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype,
                                    comm_ptr, coll_group, errflag);
    MPIR_ERR_CHECK(mpi_errno);
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_mpi_allgatherv_gpu_ipc_read(const void *sendbuf,
                                                                     MPI_Aint sendcount,
                                                                     MPI_Datatype sendtype,
                                                                     void *recvbuf,
                                                                     const MPI_Aint * recvcounts,
                                                                     const MPI_Aint * displs,
                                                                     MPI_Datatype recvtype,
                                                                     MPIR_Comm * comm_ptr,
                                                                     int coll_group,
                                                                     MPIR_Errflag_t errflag)
{
    MPIR_FUNC_ENTER;
    MPIR_CHKLMEM_DECL(3);

    int mpi_errno = MPI_SUCCESS;

    /* fallback if sendtype and recvtype is different */
    if (sendtype != recvtype) {
        goto fallback;
    }

    int my_rank = MPIR_Comm_rank(comm_ptr);
    int comm_size = MPIR_Comm_size(comm_ptr);
    MPIR_Assert(comm_size > 0);

    MPI_Aint data_sz;
    void *send_mem_addr;
    MPIDI_IPCI_ipc_handle_t *ipc_handles = NULL;
    mpi_errno = allgather_ipc_handles(sendbuf, sendcount, sendtype, comm_ptr, coll_group,
                                      MPIR_CVAR_ALLGATHERV_IPC_READ_MSG_SIZE_THRESHOLD,
                                      &data_sz, &send_mem_addr, &ipc_handles);
    MPIR_ERR_CHECK(mpi_errno);

    if (!ipc_handles) {
        goto fallback;
    }

    intptr_t true_lb = (char *) send_mem_addr - (const char *) sendbuf;
    void *recv_mem_addr = (char *) recvbuf + true_lb;
    int dev_id = ipc_handles[my_rank].gpu.local_dev_id;

    /* map ipc_handles to remote_bufs */
    void **remote_bufs = NULL;
    MPIR_CHKLMEM_MALLOC(remote_bufs, void **, sizeof(void *) * comm_size, mpi_errno, "Remote bufs",
                        MPL_MEM_COLL);
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
    }
    /* use imemcpy to copy the data concurrently */
    MPL_gpu_request *reqs = NULL;
    MPIR_CHKLMEM_MALLOC(reqs, MPL_gpu_request *, sizeof(MPL_gpu_request) * comm_size, mpi_errno,
                        "Memcpy requests", MPL_MEM_COLL);
    MPI_Aint recvtype_extent;
    MPIR_Datatype_get_extent_macro(recvtype, recvtype_extent);
    for (int i = 0; i < comm_size; i++) {
        int target = (my_rank + 1 + i) % comm_size;
        char *temp_recv = (char *) recv_mem_addr + displs[target] * recvtype_extent;
        char *temp_send = (char *) remote_bufs[target];
        /* get engine type */
        MPL_gpu_engine_type_t engine_type =
            MPIDI_IPCI_choose_engine(ipc_handles[target].gpu.global_dev_id,
                                     ipc_handles[my_rank].gpu.global_dev_id);
        MPI_Aint temp_sz = recvcounts[target] * recvtype_extent;
        mpi_errno = MPL_gpu_imemcpy(temp_recv, temp_send, temp_sz, dev_id,
                                    MPL_GPU_COPY_DIRECTION_NONE, engine_type, &reqs[i], true);
        MPIR_ERR_CHECK(mpi_errno);
    }
    /* wait for the imemcpy to finish */
    for (int i = 0; i < comm_size; i++) {
        int completed = 0;
        while (!completed) {
            mpi_errno = MPL_gpu_test(&reqs[i], &completed);
            MPIR_ERR_CHECK(mpi_errno);
        }
    }

    MPL_free(ipc_handles);

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
  fallback:
    /* Fall back to other algorithms as gpu ipc allgatherv cannot be used */
    mpi_errno = MPIR_Allgatherv_impl(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs,
                                     recvtype, comm_ptr, coll_group, errflag);
    MPIR_ERR_CHECK(mpi_errno);
    goto fn_exit;
}

#else /* MPIDI_CH4_SHM_ENABLE_GPU */
MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_mpi_bcast_gpu_ipc_read(void *buffer,
                                                                MPI_Aint count,
                                                                MPI_Datatype datatype,
                                                                int root, MPIR_Comm * comm_ptr,
                                                                int coll_group,
                                                                MPIR_Errflag_t errflag)
{
    return MPIR_Bcast_impl(buffer, count, datatype, root, comm_ptr, coll_group, errflag);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_mpi_alltoall_gpu_ipc_read(const void *sendbuf,
                                                                   MPI_Aint sendcount,
                                                                   MPI_Datatype sendtype,
                                                                   void *recvbuf,
                                                                   MPI_Aint recvcount,
                                                                   MPI_Datatype recvtype,
                                                                   MPIR_Comm * comm_ptr,
                                                                   int coll_group,
                                                                   MPIR_Errflag_t errflag)
{
    return MPIR_Alltoall_impl(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype,
                              comm_ptr, coll_group, errflag);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_mpi_allgather_gpu_ipc_read(const void *sendbuf,
                                                                    MPI_Aint sendcount,
                                                                    MPI_Datatype sendtype,
                                                                    void *recvbuf,
                                                                    MPI_Aint recvcount,
                                                                    MPI_Datatype recvtype,
                                                                    MPIR_Comm * comm_ptr,
                                                                    int coll_group,
                                                                    MPIR_Errflag_t errflag)
{
    return MPIR_Allgather_impl(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype,
                               comm_ptr, coll_group, errflag);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_mpi_allgatherv_gpu_ipc_read(const void *sendbuf,
                                                                     MPI_Aint sendcount,
                                                                     MPI_Datatype sendtype,
                                                                     void *recvbuf,
                                                                     const MPI_Aint * recvcounts,
                                                                     const MPI_Aint * displs,
                                                                     MPI_Datatype recvtype,
                                                                     MPIR_Comm * comm_ptr,
                                                                     int coll_group,
                                                                     MPIR_Errflag_t errflag)
{
    return MPIR_Allgatherv_impl(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype,
                                comm_ptr, coll_group, errflag);
}
#endif /* !MPIDI_CH4_SHM_ENABLE_GPU */

#endif /* POSIX_COLL_GPU_IPC_H_INCLUDED */

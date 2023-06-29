/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "ipc_types.h"

static int get_iovs(void *buf, MPI_Aint count, MPI_Datatype datatype, MPI_Aint data_sz,
                    struct iovec **iovs_out, MPI_Aint * len_out);
static int copy_iovs(pid_t pid, MPI_Aint src_data_sz,
                     struct iovec *src_iovs, MPI_Aint src_iov_len,
                     struct iovec *dst_iovs, MPI_Aint dst_iov_len);

int MPIDI_CMA_copy_data(MPIDI_IPC_hdr * ipc_hdr, MPIR_Request * rreq, MPI_Aint src_data_sz)
{
    int mpi_errno = MPI_SUCCESS;

    /* local iovs */
    struct iovec *dst_iovs;
    MPI_Aint dst_iov_len;
    mpi_errno = get_iovs(MPIDIG_REQUEST(rreq, buffer), MPIDIG_REQUEST(rreq, count),
                         MPIDIG_REQUEST(rreq, datatype), src_data_sz, &dst_iovs, &dst_iov_len);
    MPIR_ERR_CHECK(mpi_errno);

    /* remote iovs */
    struct iovec static_src_iov;
    struct iovec *src_iovs;
    MPI_Aint src_iov_len;

    if (ipc_hdr->is_contig) {
        src_iovs = &static_src_iov;
        src_iov_len = 1;

        src_iovs[0].iov_base = (void *) ipc_hdr->ipc_handle.cma.vaddr;
        src_iovs[0].iov_len = src_data_sz;
    } else {
        void *flattened_type = ipc_hdr + 1;
        MPIR_Datatype *dt = (MPIR_Datatype *) MPIR_Handle_obj_alloc(&MPIR_Datatype_mem);
        MPIR_Assert(dt);
        mpi_errno = MPIR_Typerep_unflatten(dt, flattened_type);
        MPIR_ERR_CHECK(mpi_errno);

        mpi_errno = get_iovs((void *) ipc_hdr->ipc_handle.cma.vaddr, ipc_hdr->count, dt->handle,
                             src_data_sz, &src_iovs, &src_iov_len);
        MPIR_ERR_CHECK(mpi_errno);

        MPIR_Datatype_free(dt);
    }

    /* process_vm_readv */
    pid_t src_pid = ipc_hdr->ipc_handle.cma.pid;
    mpi_errno = copy_iovs(src_pid, src_data_sz, src_iovs, src_iov_len, dst_iovs, dst_iov_len);
    MPIR_ERR_CHECK(mpi_errno);

    if (src_iovs != &static_src_iov) {
        MPL_free(src_iovs);
    }
    MPL_free(dst_iovs);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int get_iovs(void *buf, MPI_Aint count, MPI_Datatype datatype, MPI_Aint data_sz,
                    struct iovec **iovs_out, MPI_Aint * len_out)
{
    int mpi_errno = MPI_SUCCESS;

    struct iovec *iovs;
    MPI_Aint len, actual;

    mpi_errno = MPIR_Typerep_iov_len(count, datatype, data_sz, &len, &actual);
    MPIR_ERR_CHECK(mpi_errno);

    iovs = MPL_malloc(len * sizeof(struct iovec), MPL_MEM_OTHER);
    MPIR_Assert(iovs);

    mpi_errno = MPIR_Typerep_to_iov_offset(buf, count, datatype, 0, iovs, len, &actual);
    MPIR_ERR_CHECK(mpi_errno);

    *iovs_out = iovs;
    *len_out = len;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* Potentially src_iov_len or dst_iov_len is bigger than IOV_MAX,
 * thus, we can just call a single process_vm_readv */
static int copy_iovs(pid_t src_pid, MPI_Aint src_data_sz,
                     struct iovec *src_iovs, MPI_Aint src_iov_len,
                     struct iovec *dst_iovs, MPI_Aint dst_iov_len)
{
    int mpi_errno = MPI_SUCCESS;

    MPI_Aint i_src = 0, i_dst = 0;
    MPI_Aint cur_offset = 0;

    while (i_src < src_iov_len && i_dst < dst_iov_len) {
        MPI_Aint n_src, n_dst;
        MPI_Aint exp_data_sz;
        /* in case we result in partial iov */
        MPI_Aint remain_len = 0;
        bool adjust_src_iov = false;

        n_src = src_iov_len - i_src;
        n_dst = dst_iov_len - i_dst;
        if (n_src <= IOV_MAX && n_dst <= IOV_MAX) {
            exp_data_sz = src_data_sz - cur_offset;
        } else {
            if (n_src > IOV_MAX) {
                n_src = IOV_MAX;
            }
            if (n_dst > IOV_MAX) {
                n_dst = IOV_MAX;
            }

            MPI_Aint cnt_src = 0;
            for (int i = 0; i < n_src; i++) {
                cnt_src += src_iovs[i_src + i].iov_len;
            }
            MPI_Aint cnt_dst = 0;
            for (int i = 0; i < n_dst; i++) {
                cnt_dst += dst_iovs[i_dst + i].iov_len;
            }
            exp_data_sz = MPL_MIN(cnt_src, cnt_dst);

            if (cnt_src > exp_data_sz) {
                cnt_src = 0;
                for (int i = 0; i < n_src; i++) {
                    cnt_src += src_iovs[i_src + i].iov_len;
                    if (cnt_src >= exp_data_sz) {
                        remain_len = cnt_src - exp_data_sz;
                        src_iovs[i_src + i].iov_len -= remain_len;
                        n_src = i + 1;
                        break;
                    }
                }
                adjust_src_iov = true;
            } else if (cnt_dst > exp_data_sz) {
                cnt_dst = 0;
                for (int i = 0; i < n_dst; i++) {
                    cnt_dst += dst_iovs[i_dst + i].iov_len;
                    if (cnt_dst >= exp_data_sz) {
                        remain_len = cnt_dst - exp_data_sz;
                        dst_iovs[i_dst + i].iov_len -= remain_len;
                        n_dst = i + 1;
                    }
                }
                adjust_src_iov = false;
            }
        }

        MPI_Aint cnt;
        cnt = process_vm_readv(src_pid, dst_iovs + i_dst, n_dst, src_iovs + i_src, n_src, 0);
        MPIR_ERR_CHKANDJUMP1(cnt == -1, mpi_errno, MPI_ERR_OTHER, "**cmaread",
                             "**cmaread %d", errno);
        MPIR_ERR_CHKANDJUMP2(cnt != exp_data_sz, mpi_errno, MPI_ERR_OTHER, "**cmadata",
                             "**cmadata %d %d", (int) cnt, (int) exp_data_sz);

        cur_offset += cnt;
        i_src += n_src;
        i_dst += n_dst;

        if (remain_len > 0) {
            if (adjust_src_iov) {
                i_src--;
                /* adjust the remaining iov */
                src_iovs[i_src].iov_base =
                    (char *) src_iovs[i_src].iov_base + src_iovs[i_src].iov_len;
                src_iovs[i_src].iov_len = remain_len;
            } else {
                i_dst--;
                /* adjust the remaining iov */
                dst_iovs[i_dst].iov_base =
                    (char *) dst_iovs[i_dst].iov_base + dst_iovs[i_dst].iov_len;
                dst_iovs[i_dst].iov_len = remain_len;
            }
        }
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

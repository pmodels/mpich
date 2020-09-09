/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef OFI_RMA_H_INCLUDED
#define OFI_RMA_H_INCLUDED

#include "ofi_impl.h"

#define MPIDI_OFI_QUERY_ATOMIC_COUNT         0
#define MPIDI_OFI_QUERY_FETCH_ATOMIC_COUNT   1
#define MPIDI_OFI_QUERY_COMPARE_ATOMIC_COUNT 2

#define MPIDI_OFI_GET_BASIC_TYPE(a,b)   \
    do {                                        \
        if (MPIR_DATATYPE_IS_PREDEFINED(a))     \
            b = a;                              \
        else {                                  \
            MPIR_Datatype *dt_ptr;              \
            MPIR_Datatype_get_ptr(a,dt_ptr);    \
            MPIR_Assert(dt_ptr != NULL);        \
            b = dt_ptr->basic_type;             \
        }                                       \
    } while (0)

/* _count: count of data elements of certain datatype
 * _datatype: the datatype
 * _bytes: total byte size
 * Note: _bytes is calculated based on _count & _datatype and passed in here for reusing. */
MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_count_iovecs(int origin_count,
                                                    int target_count,
                                                    int result_count,
                                                    MPI_Datatype origin_datatype,
                                                    MPI_Datatype target_datatype,
                                                    MPI_Datatype result_datatype,
                                                    size_t origin_bytes,
                                                    size_t target_bytes,
                                                    size_t result_bytes,
                                                    size_t max_pipe, size_t * countp)
{
    /* Count the max number of iovecs that will be generated, given the iovs    */
    /* and maximum data size.  The code adds the iovecs from all three lists    */
    /* which is an upper bound for all three lists.  This is a tradeoff because */
    /* it will be very fast to calculate the estimate;  count_iov() only        */
    /* scans two elements of the datatype to make the estimate                  */
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_COUNT_IOVECS);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_COUNT_IOVECS);
    *countp = MPIDI_OFI_count_iov(origin_count, origin_datatype, origin_bytes, max_pipe);
    *countp += MPIDI_OFI_count_iov(target_count, target_datatype, target_bytes, max_pipe);
    *countp += MPIDI_OFI_count_iov(result_count, result_datatype, result_bytes, max_pipe);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_COUNT_IOVECS);
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX void MPIDI_OFI_query_acc_atomic_support(MPI_Datatype dt, int query_type,
                                                                 MPI_Op op,
                                                                 MPIR_Win * win,
                                                                 enum fi_datatype *fi_dt,
                                                                 enum fi_op *fi_op, size_t * count,
                                                                 size_t * dtsize)
{
    MPIR_Datatype *dt_ptr;
    int op_index, dt_index;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_QUERY_ACC_ATOMIC_SUPPORT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_QUERY_ACC_ATOMIC_SUPPORT);

    MPIR_Datatype_get_ptr(dt, dt_ptr);
    MPIR_Assert(dt_ptr != NULL);

    dt_index = MPIDI_OFI_DATATYPE(dt_ptr).index;
    op_index = MPIDI_OFI_get_mpi_acc_op_index(op);
    MPIR_Assert(op_index >= 0);
    MPIR_Assert(op_index < MPIDI_OFI_OP_SIZES);

    *fi_dt = (enum fi_datatype) MPIDI_OFI_global.win_op_table[dt_index][op_index].dt;
    *fi_op = (enum fi_op) MPIDI_OFI_global.win_op_table[dt_index][op_index].op;
    *dtsize = MPIDI_OFI_global.win_op_table[dt_index][op_index].dtsize;

    switch (query_type) {
        case MPIDI_OFI_QUERY_ATOMIC_COUNT:
            *count = MPIDI_OFI_global.win_op_table[dt_index][op_index].max_atomic_count;
            break;
        case MPIDI_OFI_QUERY_FETCH_ATOMIC_COUNT:
            *count = MPIDI_OFI_global.win_op_table[dt_index][op_index].max_fetch_atomic_count;
            break;
        case MPIDI_OFI_QUERY_COMPARE_ATOMIC_COUNT:
            *count = MPIDI_OFI_global.win_op_table[dt_index][op_index].max_compare_atomic_count;
            break;
        default:
            MPIR_Assert(query_type == MPIDI_OFI_QUERY_ATOMIC_COUNT ||
                        query_type == MPIDI_OFI_QUERY_FETCH_ATOMIC_COUNT ||
                        query_type == MPIDI_OFI_QUERY_COMPARE_ATOMIC_COUNT);
            break;
    }

    /* If same_op_no_op is set, we also check other processes' atomic status.
     * We have to always fallback to AM unless all possible reduce and cswap
     * operations are supported by provider. This is because the noop process
     * does not know what the same_op is on the other processes, which might be
     * unsupported (e.g., maxloc). Thus, the noop process has to always use AM, and
     * other processes have to also only use AM in order to ensure atomicity with
     * atomic get. The user can use which_accumulate_ops hint to specify used reduce
     * and cswap operations.
     * We calculate the max count of all specified ops for each datatype at window
     * creation or info set. dtypes_max_count[dt_index] is the provider allowed
     * count for all possible ops. It is 0 if any of the op is not supported and
     * is a positive value if all ops are supported.
     * Current <datatype, op> can become hw atomic only when both local op and all
     * possible remote op are atomic. */

    /* We use the minimal max_count of local op and any possible remote op as the limit
     * to validate dt_size in later check. We assume the limit should be symmetric on
     * all processes as long as the hint correctly contains the local op.*/
    MPIR_Assert(*count >= (size_t) MPIDI_OFI_WIN(win).acc_hint->dtypes_max_count[dt_index]);

    if (MPIDIG_WIN(win, info_args).accumulate_ops == MPIDIG_ACCU_SAME_OP_NO_OP)
        *count = (size_t) MPIDI_OFI_WIN(win).acc_hint->dtypes_max_count[dt_index];

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_QUERY_ACC_ATOMIC_SUPPORT);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_do_put(const void *origin_addr,
                                              int origin_count,
                                              MPI_Datatype origin_datatype,
                                              int target_rank,
                                              MPI_Aint target_disp,
                                              int target_count,
                                              MPI_Datatype target_datatype,
                                              MPIR_Win * win, MPIDI_av_entry_t * addr,
                                              MPIDI_winattr_t winattr, MPIR_Request ** sigreq)
{
    int mpi_errno = MPI_SUCCESS;
    size_t offset;
    uint64_t flags;
    struct fi_msg_rma msg;
    int target_contig, origin_contig;
    size_t target_bytes, origin_bytes;
    MPI_Aint origin_true_lb, target_true_lb;
    struct iovec iov;
    struct fi_rma_iov riov;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_DO_PUT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_DO_PUT);

    MPIDIG_RMA_OP_CHECK_SYNC(target_rank, win);

    MPIDI_Datatype_check_origin_target_contig_size_lb(origin_datatype, target_datatype,
                                                      origin_count, target_count,
                                                      origin_contig, target_contig,
                                                      origin_bytes, target_bytes,
                                                      origin_true_lb, target_true_lb);

    MPIR_ERR_CHKANDJUMP((origin_bytes != target_bytes), mpi_errno, MPI_ERR_SIZE, "**rmasize");

    /* zero-byte messages */
    if (unlikely(origin_bytes == 0))
        goto null_op_exit;

    /* self messages */
    if (target_rank == MPIDIU_win_comm_rank(win, winattr)) {
        offset = target_disp * MPIDI_OFI_winfo_disp_unit(win, target_rank);
        mpi_errno = MPIR_Localcopy(origin_addr,
                                   origin_count,
                                   origin_datatype,
                                   (char *) win->base + offset, target_count, target_datatype);
        goto null_op_exit;
    }

    /* small contiguous messages */
    if (origin_contig && target_contig && (origin_bytes <= MPIDI_OFI_global.max_buffered_write)) {
        MPIDI_OFI_win_cntr_incr(win);
        MPIDI_OFI_CALL_RETRY(fi_inject_write(MPIDI_OFI_WIN(win).ep,
                                             (char *) origin_addr + origin_true_lb, target_bytes,
                                             MPIDI_OFI_av_to_phys(addr, 0, 0),
                                             (uint64_t) MPIDI_OFI_winfo_base(win, target_rank)
                                             + target_disp * MPIDI_OFI_winfo_disp_unit(win,
                                                                                       target_rank)
                                             + target_true_lb, MPIDI_OFI_winfo_mr_key(win,
                                                                                      target_rank)),
                             0, rdma_inject_write, FALSE);
        goto null_op_exit;
    }

    /* large contiguous messages */
    if (origin_contig && target_contig) {
        if (sigreq) {
#ifdef MPIDI_CH4_USE_WORK_QUEUES
            if (*sigreq) {
                MPIR_Request_add_ref(*sigreq);
            } else
#endif
            {
                MPIDI_OFI_REQUEST_CREATE(*sigreq, MPIR_REQUEST_KIND__RMA, 0);
            }
            flags = FI_COMPLETION | FI_DELIVERY_COMPLETE;
        } else {
            flags = FI_DELIVERY_COMPLETE;
        }
        offset = target_disp * MPIDI_OFI_winfo_disp_unit(win, target_rank);
        msg.desc = NULL;
        msg.addr = MPIDI_OFI_av_to_phys(addr, 0, 0);
        msg.context = NULL;
        msg.data = 0;
        msg.msg_iov = &iov;
        msg.iov_count = 1;
        msg.rma_iov = &riov;
        msg.rma_iov_count = 1;
        iov.iov_base = (char *) origin_addr + origin_true_lb;
        iov.iov_len = target_bytes;
        riov.addr = (uint64_t) (MPIDI_OFI_winfo_base(win, target_rank) + offset + target_true_lb);
        riov.len = target_bytes;
        riov.key = MPIDI_OFI_winfo_mr_key(win, target_rank);
        MPIDI_OFI_INIT_CHUNK_CONTEXT(win, sigreq);
        MPIDI_OFI_CALL_RETRY(fi_writemsg(MPIDI_OFI_WIN(win).ep, &msg, flags), 0, rdma_write, FALSE);
        /* Complete signal request to inform completion to user. */
        MPIDI_OFI_sigreq_complete(sigreq);
        goto fn_exit;
    }

    /* noncontiguous messages */
    MPI_Aint origin_density, target_density;
    MPIR_Datatype_get_density(origin_datatype, origin_density);
    MPIR_Datatype_get_density(target_datatype, target_density);

    if (origin_density >= MPIR_CVAR_CH4_IOV_DENSITY_MIN &&
        target_density >= MPIR_CVAR_CH4_IOV_DENSITY_MIN) {
        mpi_errno =
            MPIDI_OFI_nopack_putget(origin_addr, origin_count, origin_datatype, target_rank,
                                    target_disp, target_count, target_datatype, win, addr,
                                    MPIDI_OFI_PUT, sigreq);
        goto fn_exit;
    }

    if (origin_density < MPIR_CVAR_CH4_IOV_DENSITY_MIN &&
        target_density >= MPIR_CVAR_CH4_IOV_DENSITY_MIN) {
        mpi_errno =
            MPIDI_OFI_pack_put(origin_addr, origin_count, origin_datatype, target_rank,
                               target_disp, target_count, target_datatype, win, addr, sigreq);
        goto fn_exit;
    }

    if (sigreq)
        mpi_errno =
            MPIDIG_mpi_rput(origin_addr, origin_count, origin_datatype, target_rank, target_disp,
                            target_count, target_datatype, win, sigreq);
    else
        mpi_errno =
            MPIDIG_mpi_put(origin_addr, origin_count, origin_datatype, target_rank, target_disp,
                           target_count, target_datatype, win);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_DO_PUT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
  null_op_exit:
    mpi_errno = MPI_SUCCESS;
    if (sigreq)
        *sigreq = MPIR_Request_create_complete(MPIR_REQUEST_KIND__RMA);
    goto fn_exit;
}


MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_put(const void *origin_addr,
                                              int origin_count,
                                              MPI_Datatype origin_datatype,
                                              int target_rank,
                                              MPI_Aint target_disp,
                                              int target_count, MPI_Datatype target_datatype,
                                              MPIR_Win * win, MPIDI_av_entry_t * av,
                                              MPIDI_winattr_t winattr)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_PUT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_PUT);
    int mpi_errno = MPI_SUCCESS;

    if (!MPIDI_OFI_ENABLE_RMA || win->create_flavor == MPI_WIN_FLAVOR_DYNAMIC) {
        mpi_errno = MPIDIG_mpi_put(origin_addr, origin_count, origin_datatype, target_rank,
                                   target_disp, target_count, target_datatype, win);
        goto fn_exit;
    }

    mpi_errno = MPIDI_OFI_do_put(origin_addr,
                                 origin_count,
                                 origin_datatype,
                                 target_rank,
                                 target_disp, target_count, target_datatype, win, av,
                                 winattr, NULL);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_PUT);
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_do_get(void *origin_addr,
                                              int origin_count,
                                              MPI_Datatype origin_datatype,
                                              int target_rank,
                                              MPI_Aint target_disp,
                                              int target_count,
                                              MPI_Datatype target_datatype,
                                              MPIR_Win * win, MPIDI_av_entry_t * addr,
                                              MPIDI_winattr_t winattr, MPIR_Request ** sigreq)
{
    int mpi_errno = MPI_SUCCESS;
    size_t offset;
    uint64_t flags;
    struct fi_msg_rma msg;
    int origin_contig, target_contig;
    MPI_Aint origin_true_lb, target_true_lb;
    size_t origin_bytes, target_bytes;
    struct fi_rma_iov riov;
    struct iovec iov;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_DO_GET);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_DO_GET);

    MPIDIG_RMA_OP_CHECK_SYNC(target_rank, win);

    MPIDI_Datatype_check_origin_target_contig_size_lb(origin_datatype, target_datatype,
                                                      origin_count, target_count,
                                                      origin_contig, target_contig,
                                                      origin_bytes, target_bytes,
                                                      origin_true_lb, target_true_lb);

    MPIR_ERR_CHKANDJUMP((origin_bytes != target_bytes), mpi_errno, MPI_ERR_SIZE, "**rmasize");

    /* zero-byte messages */
    if (unlikely(origin_bytes == 0))
        goto null_op_exit;

    /* self messages */
    if (target_rank == MPIDIU_win_comm_rank(win, winattr)) {
        offset = target_disp * MPIDI_OFI_winfo_disp_unit(win, target_rank);
        mpi_errno = MPIR_Localcopy((char *) win->base + offset,
                                   target_count,
                                   target_datatype, origin_addr, origin_count, origin_datatype);
        goto null_op_exit;
    }

    /* contiguous messages */
    if (origin_contig && target_contig) {
        offset = target_disp * MPIDI_OFI_winfo_disp_unit(win, target_rank);
        if (sigreq) {
            if (sigreq) {
#ifdef MPIDI_CH4_USE_WORK_QUEUES
                if (*sigreq) {
                    MPIR_Request_add_ref(*sigreq);
                } else
#endif
                {
                    MPIDI_OFI_REQUEST_CREATE(*sigreq, MPIR_REQUEST_KIND__RMA, 0);
                }
                flags = FI_COMPLETION | FI_DELIVERY_COMPLETE;
            } else {
                flags = FI_DELIVERY_COMPLETE;
            }
        } else {
            flags = 0;
        }
        msg.desc = NULL;
        msg.msg_iov = &iov;
        msg.iov_count = 1;
        msg.addr = MPIDI_OFI_av_to_phys(addr, 0, 0);
        msg.rma_iov = &riov;
        msg.rma_iov_count = 1;
        msg.context = NULL;
        msg.data = 0;
        iov.iov_base = (char *) origin_addr + origin_true_lb;
        iov.iov_len = target_bytes;
        riov.addr = (uint64_t) (MPIDI_OFI_winfo_base(win, target_rank) + offset + target_true_lb);
        riov.len = target_bytes;
        riov.key = MPIDI_OFI_winfo_mr_key(win, target_rank);
        MPIDI_OFI_INIT_CHUNK_CONTEXT(win, sigreq);
        MPIDI_OFI_CALL_RETRY(fi_readmsg(MPIDI_OFI_WIN(win).ep, &msg, flags), 0, rdma_write, FALSE);
        /* Complete signal request to inform completion to user. */
        MPIDI_OFI_sigreq_complete(sigreq);
        goto fn_exit;
    }

    /* noncontiguous messages */
    MPI_Aint origin_density, target_density;
    MPIR_Datatype_get_density(origin_datatype, origin_density);
    MPIR_Datatype_get_density(target_datatype, target_density);

    if (origin_density >= MPIR_CVAR_CH4_IOV_DENSITY_MIN &&
        target_density >= MPIR_CVAR_CH4_IOV_DENSITY_MIN) {
        mpi_errno =
            MPIDI_OFI_nopack_putget(origin_addr, origin_count, origin_datatype, target_rank,
                                    target_disp, target_count, target_datatype, win, addr,
                                    MPIDI_OFI_GET, sigreq);
        goto fn_exit;
    }

    if (origin_density < MPIR_CVAR_CH4_IOV_DENSITY_MIN &&
        target_density >= MPIR_CVAR_CH4_IOV_DENSITY_MIN) {
        mpi_errno =
            MPIDI_OFI_pack_get(origin_addr, origin_count, origin_datatype, target_rank,
                               target_disp, target_count, target_datatype, win, addr, sigreq);
        goto fn_exit;
    }

    if (sigreq)
        mpi_errno =
            MPIDIG_mpi_rget(origin_addr, origin_count, origin_datatype, target_rank, target_disp,
                            target_count, target_datatype, win, sigreq);
    else
        mpi_errno =
            MPIDIG_mpi_get(origin_addr, origin_count, origin_datatype, target_rank, target_disp,
                           target_count, target_datatype, win);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_DO_GET);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
  null_op_exit:
    mpi_errno = MPI_SUCCESS;
    if (sigreq)
        *sigreq = MPIR_Request_create_complete(MPIR_REQUEST_KIND__RMA);
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_get(void *origin_addr,
                                              int origin_count,
                                              MPI_Datatype origin_datatype,
                                              int target_rank,
                                              MPI_Aint target_disp,
                                              int target_count, MPI_Datatype target_datatype,
                                              MPIR_Win * win, MPIDI_av_entry_t * av,
                                              MPIDI_winattr_t winattr)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_GET);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_GET);

    if (!MPIDI_OFI_ENABLE_RMA || win->create_flavor == MPI_WIN_FLAVOR_DYNAMIC) {
        mpi_errno = MPIDIG_mpi_get(origin_addr, origin_count, origin_datatype, target_rank,
                                   target_disp, target_count, target_datatype, win);
        goto fn_exit;
    }

    mpi_errno = MPIDI_OFI_do_get(origin_addr,
                                 origin_count,
                                 origin_datatype,
                                 target_rank,
                                 target_disp, target_count, target_datatype, win, av, winattr,
                                 NULL);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_GET);
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_rput(const void *origin_addr,
                                               int origin_count,
                                               MPI_Datatype origin_datatype,
                                               int target_rank,
                                               MPI_Aint target_disp,
                                               int target_count,
                                               MPI_Datatype target_datatype,
                                               MPIR_Win * win, MPIDI_av_entry_t * av,
                                               MPIDI_winattr_t winattr, MPIR_Request ** request)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_RPUT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_RPUT);
    int mpi_errno = MPI_SUCCESS;

    if (!MPIDI_OFI_ENABLE_RMA || win->create_flavor == MPI_WIN_FLAVOR_DYNAMIC) {
        mpi_errno = MPIDIG_mpi_rput(origin_addr, origin_count, origin_datatype, target_rank,
                                    target_disp, target_count, target_datatype, win, request);
        goto fn_exit;
    }

    mpi_errno = MPIDI_OFI_do_put((void *) origin_addr,
                                 origin_count,
                                 origin_datatype,
                                 target_rank,
                                 target_disp, target_count, target_datatype, win, av, winattr,
                                 request);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_RPUT);
    return mpi_errno;
}


MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_compare_and_swap(const void *origin_addr,
                                                           const void *compare_addr,
                                                           void *result_addr,
                                                           MPI_Datatype datatype,
                                                           int target_rank, MPI_Aint target_disp,
                                                           MPIR_Win * win, MPIDI_av_entry_t * av,
                                                           MPIDI_winattr_t winattr)
{
    int mpi_errno = MPI_SUCCESS;
    enum fi_op fi_op;
    enum fi_datatype fi_dt;
    size_t offset, max_count, max_size, dt_size, bytes;
    MPI_Aint true_lb;
    void *buffer, *tbuffer, *rbuffer;
    struct fi_ioc originv;
    struct fi_ioc resultv;
    struct fi_ioc comparev;
    struct fi_rma_ioc targetv;
    struct fi_msg_atomic msg;

    if (
#ifndef MPIDI_CH4_DIRECT_NETMOD
           /* We have to disable network-based atomics in auto mode.
            * Because concurrent atomics may be performed by CPU (e.g., op
            * over shared memory, or op issues to process-self.
            * If disable_shm_accumulate == TRUE is set, then all above ops are issued
            * via network thus we can safely use network-based atomics. */
           !MPIDIG_WIN(win, info_args).disable_shm_accumulate ||
#endif
           !MPIDI_OFI_ENABLE_RMA || !MPIDI_OFI_ENABLE_ATOMICS ||
           win->create_flavor == MPI_WIN_FLAVOR_DYNAMIC) {
        mpi_errno =
            MPIDIG_mpi_compare_and_swap(origin_addr, compare_addr, result_addr, datatype,
                                        target_rank, target_disp, win);
        goto fn_exit;
    }

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_COMPARE_AND_SWAP);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_COMPARE_AND_SWAP);

    MPIDIG_RMA_OP_CHECK_SYNC(target_rank, win);

    offset = target_disp * MPIDI_OFI_winfo_disp_unit(win, target_rank);

    MPIDI_Datatype_check_size_lb(datatype, 1, bytes, true_lb);

    if (bytes == 0)
        goto fn_exit;

    buffer = (char *) origin_addr + true_lb;
    rbuffer = (char *) result_addr + true_lb;
    tbuffer = (void *) (MPIDI_OFI_winfo_base(win, target_rank) + offset);

    MPIDI_OFI_query_acc_atomic_support(datatype, MPIDI_OFI_QUERY_COMPARE_ATOMIC_COUNT, MPI_OP_NULL,
                                       win, &fi_dt, &fi_op, &max_count, &dt_size);
    if (max_count == 0)
        goto am_fallback;

    max_size = MPIDI_OFI_check_acc_order_size(win, dt_size);
    /* It's impossible to transfer data if buffer size is smaller than basic datatype size.
     *  TODO: we assume all processes should use the same max_size and dt_size, true ? */
    if (max_size < dt_size)
        goto am_fallback;
    /* Compare_and_swap is READ and WRITE. */
    MPIDIG_wait_am_acc(win, target_rank, (MPIDIG_ACCU_ORDER_RAW | MPIDIG_ACCU_ORDER_WAR |
                                          MPIDIG_ACCU_ORDER_WAW));

    originv.addr = (void *) buffer;
    originv.count = 1;
    resultv.addr = (void *) rbuffer;
    resultv.count = 1;
    comparev.addr = (void *) compare_addr;
    comparev.count = 1;
    targetv.addr = (uint64_t) (uintptr_t) tbuffer;
    targetv.count = 1;
    targetv.key = MPIDI_OFI_winfo_mr_key(win, target_rank);;

    msg.msg_iov = &originv;
    msg.desc = NULL;
    msg.iov_count = 1;
    msg.addr = MPIDI_OFI_av_to_phys(av, 0, 0);
    msg.rma_iov = &targetv;
    msg.rma_iov_count = 1;
    msg.datatype = fi_dt;
    msg.op = fi_op;
    msg.context = NULL;
    msg.data = 0;
    MPIDI_OFI_win_cntr_incr(win);
    MPIDI_OFI_CALL_RETRY(fi_compare_atomicmsg(MPIDI_OFI_WIN(win).ep, &msg,
                                              &comparev, NULL, 1, &resultv, NULL, 1, 0), 0,
                         atomicto, FALSE);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_COMPARE_AND_SWAP);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
  am_fallback:
    if (MPIDIG_WIN(win, info_args).accumulate_ordering &
        (MPIDIG_ACCU_ORDER_RAW | MPIDIG_ACCU_ORDER_WAW | MPIDIG_ACCU_ORDER_WAR)) {
        /* Wait for OFI cas to complete.
         * For now, there is no FI flag to track atomic only ops, we use RMA level cntr. */
        MPIDI_OFI_win_do_progress(win);
    }
    return MPIDIG_mpi_compare_and_swap(origin_addr, compare_addr, result_addr, datatype,
                                       target_rank, target_disp, win);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_do_accumulate(const void *origin_addr,
                                                     int origin_count,
                                                     MPI_Datatype origin_datatype,
                                                     int target_rank,
                                                     MPI_Aint target_disp,
                                                     int target_count,
                                                     MPI_Datatype target_datatype,
                                                     MPI_Op op, MPIR_Win * win,
                                                     MPIDI_av_entry_t * addr,
                                                     MPIR_Request ** sigreq)
{
    int mpi_errno = MPI_SUCCESS;
    size_t origin_bytes;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_DO_ACCUMULATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_DO_ACCUMULATE);

    MPIDIG_RMA_OP_CHECK_SYNC(target_rank, win);

    MPIDI_Datatype_check_size(origin_datatype, origin_count, origin_bytes);
    if (origin_bytes == 0)
        goto null_op_exit;

    if (MPIDIG_WIN(win, info_args).accumulate_ordering &
        (MPIDIG_ACCU_ORDER_WAW | MPIDIG_ACCU_ORDER_WAR)) {
        /* Wait for OFI acc to complete.
         * For now, there is no FI flag to track atomic only ops, we use RMA level cntr. */
        MPIDI_OFI_win_do_progress(win);
    }
    if (sigreq)
        mpi_errno = MPIDIG_mpi_raccumulate(origin_addr, origin_count, origin_datatype, target_rank,
                                           target_disp, target_count, target_datatype, op, win,
                                           sigreq);
    else
        mpi_errno = MPIDIG_mpi_accumulate(origin_addr, origin_count, origin_datatype, target_rank,
                                          target_disp, target_count, target_datatype, op, win);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_DO_ACCUMULATE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
  null_op_exit:
    mpi_errno = MPI_SUCCESS;
    if (sigreq)
        *sigreq = MPIR_Request_create_complete(MPIR_REQUEST_KIND__RMA);
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_do_get_accumulate(const void *origin_addr,
                                                         int origin_count,
                                                         MPI_Datatype origin_datatype,
                                                         void *result_addr,
                                                         int result_count,
                                                         MPI_Datatype result_datatype,
                                                         int target_rank,
                                                         MPI_Aint target_disp,
                                                         int target_count,
                                                         MPI_Datatype target_datatype,
                                                         MPI_Op op, MPIR_Win * win,
                                                         MPIDI_av_entry_t * addr,
                                                         MPIR_Request ** sigreq)
{
    int mpi_errno = MPI_SUCCESS;
    size_t target_bytes;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_DO_GET_ACCUMULATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_DO_GET_ACCUMULATE);

    MPIDIG_RMA_OP_CHECK_SYNC(target_rank, win);

    MPIDI_Datatype_check_size(target_datatype, target_count, target_bytes);
    if (target_bytes == 0)
        goto null_op_exit;

    if (unlikely(op == MPI_NO_OP)) {
        if (MPIDIG_WIN(win, info_args).accumulate_ordering & MPIDIG_ACCU_ORDER_RAW) {
            /* Wait for OFI acc to complete.
             * For now, there is no FI flag to track atomic only ops, we use RMA level cntr. */
            MPIDI_OFI_win_do_progress(win);
        }
    } else {
        if (MPIDIG_WIN(win, info_args).accumulate_ordering &
            (MPIDIG_ACCU_ORDER_RAW | MPIDIG_ACCU_ORDER_WAR | MPIDIG_ACCU_ORDER_WAW)) {
            MPIDI_OFI_win_do_progress(win);
        }
    }
    if (sigreq)
        mpi_errno =
            MPIDIG_mpi_rget_accumulate(origin_addr, origin_count, origin_datatype, result_addr,
                                       result_count, result_datatype, target_rank, target_disp,
                                       target_count, target_datatype, op, win, sigreq);
    else
        mpi_errno =
            MPIDIG_mpi_get_accumulate(origin_addr, origin_count, origin_datatype, result_addr,
                                      result_count, result_datatype, target_rank, target_disp,
                                      target_count, target_datatype, op, win);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_DO_GET_ACCUMULATE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
  null_op_exit:
    mpi_errno = MPI_SUCCESS;
    if (sigreq) {
        *sigreq = MPIR_Request_create_from_pool(MPIR_REQUEST_KIND__RMA, 0);
        MPIR_ERR_CHKANDSTMT((*sigreq) == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail,
                            "**nomemreq");
        MPIR_Request_add_ref(*sigreq);
        MPIDIU_request_complete(*sigreq);
    }
    goto fn_exit;
}


MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_raccumulate(const void *origin_addr,
                                                      int origin_count,
                                                      MPI_Datatype origin_datatype,
                                                      int target_rank,
                                                      MPI_Aint target_disp,
                                                      int target_count,
                                                      MPI_Datatype target_datatype,
                                                      MPI_Op op, MPIR_Win * win,
                                                      MPIDI_av_entry_t * av,
                                                      MPIDI_winattr_t winattr,
                                                      MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_RACCUMULATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_RACCUMULATE);

    if (
#ifndef MPIDI_CH4_DIRECT_NETMOD
           /* We have to disable network-based atomics in auto mode.
            * Because concurrent atomics may be performed by CPU (e.g., op
            * over shared memory, or op issues to process-self.
            * If disable_shm_accumulate == TRUE is set, then all above ops are issued
            * via network thus we can safely use network-based atomics. */
           !MPIDIG_WIN(win, info_args).disable_shm_accumulate ||
#endif
           !MPIDI_OFI_ENABLE_RMA || !MPIDI_OFI_ENABLE_ATOMICS ||
           win->create_flavor == MPI_WIN_FLAVOR_DYNAMIC) {
        mpi_errno =
            MPIDIG_mpi_raccumulate(origin_addr, origin_count, origin_datatype, target_rank,
                                   target_disp, target_count, target_datatype, op, win, request);
        goto fn_exit;
    }

    mpi_errno = MPIDI_OFI_do_accumulate((void *) origin_addr,
                                        origin_count,
                                        origin_datatype,
                                        target_rank,
                                        target_disp,
                                        target_count, target_datatype, op, win, av, request);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_RACCUMULATE);
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_rget_accumulate(const void *origin_addr,
                                                          int origin_count,
                                                          MPI_Datatype origin_datatype,
                                                          void *result_addr,
                                                          int result_count,
                                                          MPI_Datatype result_datatype,
                                                          int target_rank,
                                                          MPI_Aint target_disp,
                                                          int target_count,
                                                          MPI_Datatype target_datatype,
                                                          MPI_Op op, MPIR_Win * win,
                                                          MPIDI_av_entry_t * av,
                                                          MPIDI_winattr_t winattr,
                                                          MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_RGET_ACCUMULATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_RGET_ACCUMULATE);

    if (
#ifndef MPIDI_CH4_DIRECT_NETMOD
           /* We have to disable network-based atomics in auto mode.
            * Because concurrent atomics may be performed by CPU (e.g., op
            * over shared memory, or op issues to process-self.
            * If disable_shm_accumulate == TRUE is set, then all above ops are issued
            * via network thus we can safely use network-based atomics. */
           !MPIDIG_WIN(win, info_args).disable_shm_accumulate ||
#endif
           !MPIDI_OFI_ENABLE_RMA || !MPIDI_OFI_ENABLE_ATOMICS ||
           win->create_flavor == MPI_WIN_FLAVOR_DYNAMIC) {
        mpi_errno =
            MPIDIG_mpi_rget_accumulate(origin_addr, origin_count, origin_datatype, result_addr,
                                       result_count, result_datatype, target_rank, target_disp,
                                       target_count, target_datatype, op, win, request);
        goto fn_exit;
    }

    mpi_errno = MPIDI_OFI_do_get_accumulate(origin_addr, origin_count, origin_datatype,
                                            result_addr, result_count, result_datatype,
                                            target_rank, target_disp, target_count,
                                            target_datatype, op, win, av, request);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_RGET_ACCUMULATE);
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_fetch_and_op(const void *origin_addr,
                                                       void *result_addr,
                                                       MPI_Datatype datatype,
                                                       int target_rank,
                                                       MPI_Aint target_disp, MPI_Op op,
                                                       MPIR_Win * win, MPIDI_av_entry_t * av,
                                                       MPIDI_winattr_t winattr)
{
    int mpi_errno = MPI_SUCCESS;
    enum fi_op fi_op;
    enum fi_datatype fi_dt;
    size_t offset, max_count, max_size, dt_size, bytes;
    MPI_Aint true_lb ATTRIBUTE((unused));
    void *buffer, *tbuffer, *rbuffer;
    struct fi_ioc originv;
    struct fi_ioc resultv;
    struct fi_rma_ioc targetv;
    struct fi_msg_atomic msg;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_FETCH_AND_OP);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_FETCH_AND_OP);

    if (
#ifndef MPIDI_CH4_DIRECT_NETMOD
           /* We have to disable network-based atomics in auto mode.
            * Because concurrent atomics may be performed by CPU (e.g., op
            * over shared memory, or op issues to process-self.
            * If disable_shm_accumulate == TRUE is set, then all above ops are issued
            * via network thus we can safely use network-based atomics. */
           !MPIDIG_WIN(win, info_args).disable_shm_accumulate ||
#endif
           !MPIDI_OFI_ENABLE_RMA || !MPIDI_OFI_ENABLE_ATOMICS ||
           win->create_flavor == MPI_WIN_FLAVOR_DYNAMIC) {
        mpi_errno =
            MPIDIG_mpi_fetch_and_op(origin_addr, result_addr, datatype, target_rank, target_disp,
                                    op, win);
        goto fn_exit;
    }

    MPIDIG_RMA_OP_CHECK_SYNC(target_rank, win);

    offset = target_disp * MPIDI_OFI_winfo_disp_unit(win, target_rank);

    MPIDI_Datatype_check_size_lb(datatype, 1, bytes, true_lb);
    if (bytes == 0)
        goto fn_exit;

    buffer = (char *) origin_addr;      /* ignore true_lb, which should be always zero */
    rbuffer = (char *) result_addr;
    tbuffer = (void *) (MPIDI_OFI_winfo_base(win, target_rank) + offset);

    MPIDI_OFI_query_acc_atomic_support(datatype, MPIDI_OFI_QUERY_FETCH_ATOMIC_COUNT,
                                       op, win, &fi_dt, &fi_op, &max_count, &dt_size);
    if (max_count == 0)
        goto am_fallback;

    max_size = MPIDI_OFI_check_acc_order_size(win, dt_size);
    /* It's impossible to transfer data if buffer size is smaller than basic datatype size.
     *  TODO: we assume all processes should use the same max_size and dt_size, true ? */
    if (max_size < dt_size)
        goto am_fallback;

    if (unlikely(op == MPI_NO_OP)) {
        /* Fetch_and_op is READ and WRITE, except NO_OP (it is READ only). */
        MPIDIG_wait_am_acc(win, target_rank, MPIDIG_ACCU_ORDER_RAW);
    } else {
        MPIDIG_wait_am_acc(win, target_rank,
                           (MPIDIG_ACCU_ORDER_RAW | MPIDIG_ACCU_ORDER_WAR | MPIDIG_ACCU_ORDER_WAW));
    }

    originv.addr = (void *) buffer;
    originv.count = 1;
    resultv.addr = (void *) rbuffer;
    resultv.count = 1;
    targetv.addr = (uint64_t) (uintptr_t) tbuffer;
    targetv.count = 1;
    targetv.key = MPIDI_OFI_winfo_mr_key(win, target_rank);

    msg.msg_iov = &originv;
    msg.desc = NULL;
    msg.iov_count = 1;
    msg.addr = MPIDI_OFI_av_to_phys(av, 0, 0);
    msg.rma_iov = &targetv;
    msg.rma_iov_count = 1;
    msg.datatype = fi_dt;
    msg.op = fi_op;
    msg.context = NULL;
    msg.data = 0;
    MPIDI_OFI_win_cntr_incr(win);
    MPIDI_OFI_CALL_RETRY(fi_fetch_atomicmsg(MPIDI_OFI_WIN(win).ep, &msg, &resultv,
                                            NULL, 1, 0), 0, rdma_readfrom, FALSE);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_FETCH_AND_OP);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
  am_fallback:
    if (unlikely(op == MPI_NO_OP)) {
        if (MPIDIG_WIN(win, info_args).accumulate_ordering & MPIDIG_ACCU_ORDER_RAW) {
            /* Wait for OFI fetch_and_op to complete.
             * For now, there is no FI flag to track atomic only ops, we use RMA level cntr. */
            MPIDI_OFI_win_do_progress(win);
        }
    } else {
        if (MPIDIG_WIN(win, info_args).accumulate_ordering &
            (MPIDIG_ACCU_ORDER_RAW | MPIDIG_ACCU_ORDER_WAR | MPIDIG_ACCU_ORDER_WAW)) {
            MPIDI_OFI_win_do_progress(win);
        }
    }
    return MPIDIG_mpi_fetch_and_op(origin_addr, result_addr, datatype, target_rank, target_disp, op,
                                   win);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_rget(void *origin_addr,
                                               int origin_count,
                                               MPI_Datatype origin_datatype,
                                               int target_rank,
                                               MPI_Aint target_disp,
                                               int target_count,
                                               MPI_Datatype target_datatype,
                                               MPIR_Win * win, MPIDI_av_entry_t * av,
                                               MPIDI_winattr_t winattr, MPIR_Request ** request)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_RGET);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_RGET);
    int mpi_errno = MPI_SUCCESS;

    if (!MPIDI_OFI_ENABLE_RMA || win->create_flavor == MPI_WIN_FLAVOR_DYNAMIC) {
        mpi_errno = MPIDIG_mpi_rget(origin_addr, origin_count, origin_datatype, target_rank,
                                    target_disp, target_count, target_datatype, win, request);
        goto fn_exit;
    }

    mpi_errno = MPIDI_OFI_do_get(origin_addr,
                                 origin_count,
                                 origin_datatype,
                                 target_rank,
                                 target_disp, target_count, target_datatype, win, av, winattr,
                                 request);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_RGET);
    return mpi_errno;
}


MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_get_accumulate(const void *origin_addr,
                                                         int origin_count,
                                                         MPI_Datatype origin_datatype,
                                                         void *result_addr,
                                                         int result_count,
                                                         MPI_Datatype result_datatype,
                                                         int target_rank,
                                                         MPI_Aint target_disp,
                                                         int target_count,
                                                         MPI_Datatype target_datatype, MPI_Op op,
                                                         MPIR_Win * win, MPIDI_av_entry_t * av,
                                                         MPIDI_winattr_t winattr)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_GET_ACCUMULATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_GET_ACCUMULATE);

    if (
#ifndef MPIDI_CH4_DIRECT_NETMOD
           /* We have to disable network-based atomics in auto mode.
            * Because concurrent atomics may be performed by CPU (e.g., op
            * over shared memory, or op issues to process-self.
            * If disable_shm_accumulate == TRUE is set, then all above ops are issued
            * via network thus we can safely use network-based atomics. */
           !MPIDIG_WIN(win, info_args).disable_shm_accumulate ||
#endif
           !MPIDI_OFI_ENABLE_RMA || !MPIDI_OFI_ENABLE_ATOMICS ||
           win->create_flavor == MPI_WIN_FLAVOR_DYNAMIC) {
        mpi_errno =
            MPIDIG_mpi_get_accumulate(origin_addr, origin_count, origin_datatype, result_addr,
                                      result_count, result_datatype, target_rank, target_disp,
                                      target_count, target_datatype, op, win);
        goto fn_exit;
    }

    mpi_errno = MPIDI_OFI_do_get_accumulate(origin_addr, origin_count, origin_datatype,
                                            result_addr, result_count, result_datatype,
                                            target_rank, target_disp, target_count,
                                            target_datatype, op, win, av, NULL);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_GET_ACCUMULATE);
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_accumulate(const void *origin_addr,
                                                     int origin_count,
                                                     MPI_Datatype origin_datatype,
                                                     int target_rank,
                                                     MPI_Aint target_disp,
                                                     int target_count,
                                                     MPI_Datatype target_datatype, MPI_Op op,
                                                     MPIR_Win * win, MPIDI_av_entry_t * av,
                                                     MPIDI_winattr_t winattr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_ACCUMULATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_ACCUMULATE);

    if (
#ifndef MPIDI_CH4_DIRECT_NETMOD
           /* We have to disable network-based atomics in auto mode.
            * Because concurrent atomics may be performed by CPU (e.g., op
            * over shared memory, or op issues to process-self.
            * If disable_shm_accumulate == TRUE is set, then all above ops are issued
            * via network thus we can safely use network-based atomics. */
           !MPIDIG_WIN(win, info_args).disable_shm_accumulate ||
#endif
           !MPIDI_OFI_ENABLE_RMA || !MPIDI_OFI_ENABLE_ATOMICS ||
           win->create_flavor == MPI_WIN_FLAVOR_DYNAMIC) {
        mpi_errno =
            MPIDIG_mpi_accumulate(origin_addr, origin_count, origin_datatype, target_rank,
                                  target_disp, target_count, target_datatype, op, win);
        goto fn_exit;
    }

    mpi_errno = MPIDI_OFI_do_accumulate(origin_addr,
                                        origin_count,
                                        origin_datatype,
                                        target_rank,
                                        target_disp, target_count, target_datatype, op, win, av,
                                        NULL);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_ACCUMULATE);
    return mpi_errno;
}

#endif /* OFI_RMA_H_INCLUDED */

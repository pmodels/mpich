/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef OFI_RMA_H_INCLUDED
#define OFI_RMA_H_INCLUDED

#include "ofi_impl.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_CH4_OFI_DISABLE_INJECT_WRITE
      category    : CH4_OFI
      type        : boolean
      default     : false
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        Avoid use fi_inject_write. For some provider, e.g. tcp;ofi_rxm,
        inject write may break the synchronization.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

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

MPL_STATIC_INLINE_PREFIX void MPIDI_OFI_query_acc_atomic_support(MPI_Datatype dt, int query_type,
                                                                 MPI_Op op,
                                                                 MPIR_Win * win,
                                                                 MPIDI_winattr_t winattr,
                                                                 enum fi_datatype *fi_dt,
                                                                 enum fi_op *fi_op,
                                                                 MPI_Aint * count,
                                                                 MPI_Aint * dtsize)
{
    int op_index, dt_index;

    MPIR_FUNC_ENTER;

    dt_index = MPIR_Datatype_predefined_get_index(dt);
    MPIR_Assert(dt_index < MPIR_DATATYPE_N_PREDEFINED);

    op_index = MPIDIU_win_acc_op_get_index(op);
    MPIR_Assert(op_index < MPIDIG_ACCU_NUM_OP);

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

    if (winattr & MPIDI_WINATTR_ACCU_SAME_OP_NO_OP)
        *count = (size_t) MPIDI_OFI_WIN(win).acc_hint->dtypes_max_count[dt_index];

    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                    (MPL_DBG_FDEST, "Query atomics support: max_count 0x%lx, dtsize 0x%lx", *count,
                     *dtsize));

    MPIR_FUNC_EXIT;
}

MPL_STATIC_INLINE_PREFIX bool MPIDI_OFI_prepare_target_mr(int target_rank,
                                                          MPI_Aint target_disp,
                                                          MPI_Aint target_extent,
                                                          MPI_Aint target_true_lb, MPIR_Win * win,
                                                          MPIDI_winattr_t winattr,
                                                          MPIDI_OFI_target_mr_t * target_mr)
{
    size_t offset;

    if (winattr & MPIDI_WINATTR_NM_DYNAMIC_MR) {
        /* Special path for dynamic window with per-attach memory registration. */
        offset = target_disp;   /* dynamic win is always with disp_unit=1 */
        void *target_mr_found = NULL;
        uint64_t target_addr = (uintptr_t) MPI_BOTTOM + offset;
        /* Return valid node only when [target_addr:target_extent] matches within a
         * single region. If it crosses two regions, NULL is returned. */
        MPL_gavl_tree_search(MPIDI_OFI_WIN(win).dwin_target_mrs[target_rank],
                             (const void *) (uintptr_t) target_addr, target_extent,
                             &target_mr_found);

        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                        (MPL_DBG_FDEST, "target_mr found %d addr 0x%" PRIx64 ", extent 0x%lx",
                         target_mr_found ? 1 : 0, target_addr, target_extent));

        if (target_mr_found) {
            if (MPIDI_OFI_ENABLE_MR_VIRT_ADDRESS) {
                target_mr->addr = target_addr;
            } else {
                /* Unlikely hit this path. It happens only when provider is without
                 * FI_VIRT_ADDRESS but fails to register MPI_BOTTOM at win creation time */
                target_mr->addr = target_addr - ((MPIDI_OFI_target_mr_t *) target_mr_found)->addr;
            }
            target_mr->mr_key = ((MPIDI_OFI_target_mr_t *) target_mr_found)->mr_key;
            return true;
        } else {
            return false;
        }
    } else {
        offset = target_disp * MPIDI_OFI_winfo_disp_unit(win, target_rank);
        target_mr->addr = MPIDI_OFI_winfo_base(win, target_rank) + offset;
        target_mr->mr_key = MPIDI_OFI_winfo_mr_key(win, target_rank);

        return true;    /* always found */
    }
}

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_do_put(const void *origin_addr,
                                              MPI_Aint origin_count,
                                              MPI_Datatype origin_datatype,
                                              int target_rank,
                                              MPI_Aint target_disp,
                                              MPI_Aint target_count,
                                              MPI_Datatype target_datatype,
                                              MPIR_Win * win, MPIDI_av_entry_t * addr,
                                              MPIDI_winattr_t winattr, MPIR_Request ** sigreq)
{
    int mpi_errno = MPI_SUCCESS;
    uint64_t flags;
    struct fi_msg_rma msg;
    int target_contig, origin_contig;
    MPI_Aint target_bytes, origin_bytes, target_extent;
    MPI_Aint origin_true_lb, target_true_lb;
    struct iovec iov;
    struct fi_rma_iov riov;
    int nic = 0;

    MPIR_FUNC_ENTER;

    MPIDIG_RMA_OP_CHECK_SYNC(target_rank, win);

    MPIDI_Datatype_check_contig_size_lb(origin_datatype, origin_count, origin_contig,
                                        origin_bytes, origin_true_lb);
    MPIDI_Datatype_check_contig_size_extent_lb(target_datatype, target_count, target_contig,
                                               target_bytes, target_extent, target_true_lb);

    int vni = MPIDI_WIN(win, am_vci);
    int vni_target = MPIDI_WIN_TARGET_VCI(win, target_rank);

    /* zero-byte messages */
    if (unlikely(origin_bytes == 0))
        goto null_op_exit;

    /* self messages */
    if (target_rank == MPIDIU_win_comm_rank(win, winattr)) {
        MPI_Aint offset = target_disp * MPIDI_OFI_winfo_disp_unit(win, target_rank);
        mpi_errno = MPIR_Localcopy(origin_addr,
                                   origin_count,
                                   origin_datatype,
                                   (char *) win->base + offset, target_count, target_datatype);
        goto null_op_exit;
    }

    /* determine preferred physical NIC number for the rank */
    MPIDI_OFI_nic_info_t *nics = MPIDI_OFI_global.nic_info;
    MPIR_T_PVAR_COUNTER_INC(MULTINIC, rma_pref_phy_nic_put_bytes_count[nics[0].id], target_bytes);

    /* prepare remote addr and mr key.
     * Continue native path only when all segments are in the same registered memory region */
    bool target_mr_found;
    MPIDI_OFI_target_mr_t target_mr;
    target_mr_found = MPIDI_OFI_prepare_target_mr(target_rank,
                                                  target_disp, target_extent, target_true_lb,
                                                  win, winattr, &target_mr);
    if (unlikely(!target_mr_found))
        goto am_fallback;

    /* small contiguous messages */
    if (origin_contig && target_contig && (origin_bytes <= MPIDI_OFI_global.max_buffered_write) &&
        !MPIR_CVAR_CH4_OFI_DISABLE_INJECT_WRITE) {
        MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vni).lock);
        MPIDI_OFI_win_cntr_incr(win);
        MPIDI_OFI_CALL_RETRY(fi_inject_write(MPIDI_OFI_WIN(win).ep,
                                             MPIR_get_contig_ptr(origin_addr, origin_true_lb),
                                             target_bytes,
                                             MPIDI_OFI_av_to_phys(addr, nic, vni, vni_target),
                                             target_mr.addr + target_true_lb,
                                             target_mr.mr_key), vni, rdma_inject_write, FALSE);
        MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vni).lock);
        goto null_op_exit;
    }

    /* large contiguous messages */
    if (origin_contig && target_contig) {
        MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vni).lock);
        if (sigreq) {
            MPIDI_OFI_REQUEST_CREATE(*sigreq, MPIR_REQUEST_KIND__RMA, 0);
            flags = FI_COMPLETION | FI_DELIVERY_COMPLETE;
        } else {
            flags = FI_DELIVERY_COMPLETE;
        }
        msg.desc = NULL;
        msg.addr = MPIDI_OFI_av_to_phys(addr, nic, vni, vni_target);
        msg.context = NULL;
        msg.data = 0;
        msg.msg_iov = &iov;
        msg.iov_count = 1;
        msg.rma_iov = &riov;
        msg.rma_iov_count = 1;
        iov.iov_base = MPIR_get_contig_ptr(origin_addr, origin_true_lb);
        iov.iov_len = target_bytes;
        riov.addr = target_mr.addr + target_true_lb;
        riov.len = target_bytes;
        riov.key = target_mr.mr_key;
        MPIDI_OFI_INIT_CHUNK_CONTEXT(win, sigreq);
        MPIDI_OFI_CALL_RETRY(fi_writemsg(MPIDI_OFI_WIN(win).ep, &msg, flags), vni, rdma_write,
                             FALSE);
        /* Complete signal request to inform completion to user. */
        MPIDI_OFI_sigreq_complete(sigreq);
        MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vni).lock);
        goto fn_exit;
    }

    /* noncontiguous messages */
    MPI_Aint origin_density, target_density;
    MPIR_Datatype_get_density(origin_datatype, origin_density);
    MPIR_Datatype_get_density(target_datatype, target_density);

    if (origin_density >= MPIR_CVAR_CH4_IOV_DENSITY_MIN &&
        target_density >= MPIR_CVAR_CH4_IOV_DENSITY_MIN) {
        MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vni).lock);
        mpi_errno =
            MPIDI_OFI_nopack_putget(origin_addr, origin_count, origin_datatype, target_rank,
                                    target_count, target_datatype, target_mr, win, addr,
                                    MPIDI_OFI_PUT, sigreq);
        MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vni).lock);
        goto fn_exit;
    }

    if (origin_density < MPIR_CVAR_CH4_IOV_DENSITY_MIN &&
        target_density >= MPIR_CVAR_CH4_IOV_DENSITY_MIN) {
        MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vni).lock);
        mpi_errno =
            MPIDI_OFI_pack_put(origin_addr, origin_count, origin_datatype, target_rank,
                               target_count, target_datatype, target_mr, win, addr, sigreq);
        MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vni).lock);
        goto fn_exit;
    }

  am_fallback:
    if (sigreq)
        mpi_errno =
            MPIDIG_mpi_rput(origin_addr, origin_count, origin_datatype, target_rank, target_disp,
                            target_count, target_datatype, win, sigreq);
    else
        mpi_errno =
            MPIDIG_mpi_put(origin_addr, origin_count, origin_datatype, target_rank, target_disp,
                           target_count, target_datatype, win);

  fn_exit:
    MPIR_FUNC_EXIT;
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
                                              MPI_Aint origin_count,
                                              MPI_Datatype origin_datatype,
                                              int target_rank,
                                              MPI_Aint target_disp,
                                              MPI_Aint target_count, MPI_Datatype target_datatype,
                                              MPIR_Win * win, MPIDI_av_entry_t * av,
                                              MPIDI_winattr_t winattr)
{
    MPIR_FUNC_ENTER;
    int mpi_errno = MPI_SUCCESS;

    if (!MPIDI_OFI_ENABLE_RMA || !(winattr & MPIDI_WINATTR_NM_REACHABLE) ||
        MPIR_GPU_query_pointer_is_dev(origin_addr)) {
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
    MPIR_FUNC_EXIT;
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_do_get(void *origin_addr,
                                              MPI_Aint origin_count,
                                              MPI_Datatype origin_datatype,
                                              int target_rank,
                                              MPI_Aint target_disp,
                                              MPI_Aint target_count,
                                              MPI_Datatype target_datatype,
                                              MPIR_Win * win, MPIDI_av_entry_t * addr,
                                              MPIDI_winattr_t winattr, MPIR_Request ** sigreq)
{
    int mpi_errno = MPI_SUCCESS;
    uint64_t flags;
    struct fi_msg_rma msg;
    int origin_contig, target_contig;
    MPI_Aint origin_true_lb, target_true_lb;
    MPI_Aint target_bytes, target_extent;
    struct fi_rma_iov riov;
    struct iovec iov;
    int nic = 0;

    MPIR_FUNC_ENTER;

    MPIDIG_RMA_OP_CHECK_SYNC(target_rank, win);

    MPIDI_Datatype_check_contig_lb(origin_datatype, origin_contig, origin_true_lb);
    MPIDI_Datatype_check_contig_size_extent_lb(target_datatype, target_count, target_contig,
                                               target_bytes, target_extent, target_true_lb);

    int vni = MPIDI_WIN(win, am_vci);
    int vni_target = MPIDI_WIN_TARGET_VCI(win, target_rank);

    /* zero-byte messages */
    if (unlikely(target_bytes == 0))
        goto null_op_exit;

    /* self messages */
    if (target_rank == MPIDIU_win_comm_rank(win, winattr)) {
        MPI_Aint offset = target_disp * MPIDI_OFI_winfo_disp_unit(win, target_rank);
        mpi_errno = MPIR_Localcopy((char *) win->base + offset,
                                   target_count,
                                   target_datatype, origin_addr, origin_count, origin_datatype);
        goto null_op_exit;
    }

    /* determine preferred physical NIC number for the rank */
    MPIDI_OFI_nic_info_t *nics = MPIDI_OFI_global.nic_info;
    MPIR_T_PVAR_COUNTER_INC(MULTINIC, rma_pref_phy_nic_get_bytes_count[nics[0].id], target_bytes);

    /* prepare remote addr and mr key.
     * Continue native path only when all segments are in the same registered memory region */
    bool target_mr_found;
    MPIDI_OFI_target_mr_t target_mr;
    target_mr_found = MPIDI_OFI_prepare_target_mr(target_rank,
                                                  target_disp, target_extent, target_true_lb,
                                                  win, winattr, &target_mr);
    if (unlikely(!target_mr_found))
        goto am_fallback;

    /* contiguous messages */
    if (origin_contig && target_contig) {
        MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vni).lock);
        if (sigreq) {
            MPIDI_OFI_REQUEST_CREATE(*sigreq, MPIR_REQUEST_KIND__RMA, 0);
            flags = FI_COMPLETION | FI_DELIVERY_COMPLETE;
        } else {
            flags = 0;
        }
        msg.desc = NULL;
        msg.msg_iov = &iov;
        msg.iov_count = 1;
        msg.addr = MPIDI_OFI_av_to_phys(addr, nic, vni, vni_target);
        msg.rma_iov = &riov;
        msg.rma_iov_count = 1;
        msg.context = NULL;
        msg.data = 0;
        iov.iov_base = MPIR_get_contig_ptr(origin_addr, origin_true_lb);
        iov.iov_len = target_bytes;
        riov.addr = target_mr.addr + target_true_lb;
        riov.len = target_bytes;
        riov.key = target_mr.mr_key;
        MPIDI_OFI_INIT_CHUNK_CONTEXT(win, sigreq);
        MPIDI_OFI_CALL_RETRY(fi_readmsg(MPIDI_OFI_WIN(win).ep, &msg, flags), vni, rdma_write,
                             FALSE);
        /* Complete signal request to inform completion to user. */
        MPIDI_OFI_sigreq_complete(sigreq);
        MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vni).lock);
        goto fn_exit;
    }

    /* noncontiguous messages */
    MPI_Aint origin_density, target_density;
    MPIR_Datatype_get_density(origin_datatype, origin_density);
    MPIR_Datatype_get_density(target_datatype, target_density);

    if (origin_density >= MPIR_CVAR_CH4_IOV_DENSITY_MIN &&
        target_density >= MPIR_CVAR_CH4_IOV_DENSITY_MIN) {
        MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vni).lock);
        mpi_errno =
            MPIDI_OFI_nopack_putget(origin_addr, origin_count, origin_datatype, target_rank,
                                    target_count, target_datatype, target_mr, win, addr,
                                    MPIDI_OFI_GET, sigreq);
        MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vni).lock);
        goto fn_exit;
    }

    if (origin_density < MPIR_CVAR_CH4_IOV_DENSITY_MIN &&
        target_density >= MPIR_CVAR_CH4_IOV_DENSITY_MIN) {
        MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vni).lock);
        mpi_errno =
            MPIDI_OFI_pack_get(origin_addr, origin_count, origin_datatype, target_rank,
                               target_count, target_datatype, target_mr, win, addr, sigreq);
        MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vni).lock);
        goto fn_exit;
    }

  am_fallback:
    if (sigreq)
        mpi_errno =
            MPIDIG_mpi_rget(origin_addr, origin_count, origin_datatype, target_rank, target_disp,
                            target_count, target_datatype, win, sigreq);
    else
        mpi_errno =
            MPIDIG_mpi_get(origin_addr, origin_count, origin_datatype, target_rank, target_disp,
                           target_count, target_datatype, win);

  fn_exit:
    MPIR_FUNC_EXIT;
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
                                              MPI_Aint origin_count,
                                              MPI_Datatype origin_datatype,
                                              int target_rank,
                                              MPI_Aint target_disp,
                                              MPI_Aint target_count, MPI_Datatype target_datatype,
                                              MPIR_Win * win, MPIDI_av_entry_t * av,
                                              MPIDI_winattr_t winattr)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    if (!MPIDI_OFI_ENABLE_RMA || !(winattr & MPIDI_WINATTR_NM_REACHABLE) ||
        MPIR_GPU_query_pointer_is_dev(origin_addr)) {
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
    MPIR_FUNC_EXIT;
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_rput(const void *origin_addr,
                                               MPI_Aint origin_count,
                                               MPI_Datatype origin_datatype,
                                               int target_rank,
                                               MPI_Aint target_disp,
                                               MPI_Aint target_count,
                                               MPI_Datatype target_datatype,
                                               MPIR_Win * win, MPIDI_av_entry_t * av,
                                               MPIDI_winattr_t winattr, MPIR_Request ** request)
{
    MPIR_FUNC_ENTER;
    int mpi_errno = MPI_SUCCESS;

    if (!MPIDI_OFI_ENABLE_RMA || !(winattr & MPIDI_WINATTR_NM_REACHABLE) ||
        MPIR_GPU_query_pointer_is_dev(origin_addr)) {
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
    MPIR_FUNC_EXIT;
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
    MPI_Aint max_count, max_size, dt_size, bytes;
    MPI_Aint true_lb;
    void *buffer, *rbuffer;
    struct fi_ioc originv;
    struct fi_ioc resultv;
    struct fi_ioc comparev;
    struct fi_rma_ioc targetv;
    struct fi_msg_atomic msg;
    int nic = 0;

    int vni = MPIDI_WIN(win, am_vci);
    int vni_target = MPIDI_WIN_TARGET_VCI(win, target_rank);

    if (
#ifndef MPIDI_CH4_DIRECT_NETMOD
           /* We have to disable network-based atomics in auto mode.
            * Because concurrent atomics may be performed by CPU (e.g., op
            * over shared memory, or op issues to process-self.
            * If ACCU_NO_SHM is set, then all above ops are issued
            * via network thus we can safely use network-based atomics. */
           !(winattr & MPIDI_WINATTR_ACCU_NO_SHM) ||
#endif
           !MPIDI_OFI_ENABLE_RMA || !MPIDI_OFI_ENABLE_ATOMICS ||
           !(winattr & MPIDI_WINATTR_NM_REACHABLE) ||
           MPIR_GPU_query_pointer_is_dev(origin_addr) ||
           MPIR_GPU_query_pointer_is_dev(compare_addr) ||
           MPIR_GPU_query_pointer_is_dev(result_addr)) {
        mpi_errno =
            MPIDIG_mpi_compare_and_swap(origin_addr, compare_addr, result_addr, datatype,
                                        target_rank, target_disp, win);
        goto fn_exit;
    }

    MPIR_FUNC_ENTER;

    MPIDIG_RMA_OP_CHECK_SYNC(target_rank, win);
    MPIDI_Datatype_check_size_lb(datatype, 1, bytes, true_lb);

    if (bytes == 0)
        goto fn_exit;

    /* prepare remote addr and mr key.
     * Continue native path only when all segments are in the same registered memory region */
    bool target_mr_found;
    MPIDI_OFI_target_mr_t target_mr;
    target_mr_found = MPIDI_OFI_prepare_target_mr(target_rank, target_disp, bytes, 0,
                                                  win, winattr, &target_mr);
    if (unlikely(!target_mr_found))
        goto am_fallback;

    buffer = MPIR_get_contig_ptr(origin_addr, true_lb);
    rbuffer = MPIR_get_contig_ptr(result_addr, true_lb);

    MPIDI_OFI_query_acc_atomic_support(datatype, MPIDI_OFI_QUERY_COMPARE_ATOMIC_COUNT, MPI_OP_NULL,
                                       win, winattr, &fi_dt, &fi_op, &max_count, &dt_size);
    if (max_count == 0)
        goto am_fallback;

    max_size = MPIDI_OFI_check_acc_order_size(win, dt_size);
    /* It's impossible to transfer data if buffer size is smaller than basic datatype size.
     *  TODO: we assume all processes should use the same max_size and dt_size, true ? */
    if (max_size < dt_size)
        goto am_fallback;
    /* Ensure completion of outstanding AMs for atomicity. */
    MPIDIG_wait_am_acc(win, target_rank);

    originv.addr = (void *) buffer;
    originv.count = 1;
    resultv.addr = (void *) rbuffer;
    resultv.count = 1;
    comparev.addr = (void *) compare_addr;
    comparev.count = 1;
    targetv.addr = target_mr.addr;
    targetv.count = 1;
    targetv.key = target_mr.mr_key;

    msg.msg_iov = &originv;
    msg.desc = NULL;
    msg.iov_count = 1;
    msg.addr = MPIDI_OFI_av_to_phys(av, nic, vni, vni_target);
    msg.rma_iov = &targetv;
    msg.rma_iov_count = 1;
    msg.datatype = fi_dt;
    msg.op = fi_op;
    msg.context = NULL;
    msg.data = 0;

    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vni).lock);
    MPIDI_OFI_win_cntr_incr(win);
    MPIDI_OFI_CALL_RETRY(fi_compare_atomicmsg(MPIDI_OFI_WIN(win).ep, &msg,
                                              &comparev, NULL, 1, &resultv, NULL, 1, 0), vni,
                         atomicto, FALSE);
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vni).lock);
  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
  am_fallback:
    /* Wait for OFI case to complete for atomicity.
     * For now, there is no FI flag to track atomic only ops, we use RMA level cntr. */
    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vni).lock);
    MPIDI_OFI_win_do_progress(win, vni);
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vni).lock);
    return MPIDIG_mpi_compare_and_swap(origin_addr, compare_addr, result_addr, datatype,
                                       target_rank, target_disp, win);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_do_accumulate(const void *origin_addr,
                                                     MPI_Aint origin_count,
                                                     MPI_Datatype origin_datatype,
                                                     int target_rank,
                                                     MPI_Aint target_disp,
                                                     MPI_Aint target_count,
                                                     MPI_Datatype target_datatype,
                                                     MPI_Op op, MPIR_Win * win,
                                                     MPIDI_av_entry_t * addr,
                                                     MPIDI_winattr_t winattr,
                                                     MPIR_Request ** sigreq)
{
    int mpi_errno = MPI_SUCCESS;
    int target_contig, origin_contig;
    MPI_Aint target_bytes, origin_bytes, target_extent;
    MPI_Aint origin_true_lb, target_true_lb;
    int nic = 0;

    MPIR_FUNC_ENTER;

    MPIDIG_RMA_OP_CHECK_SYNC(target_rank, win);

    MPIDI_Datatype_check_contig_size_lb(origin_datatype, origin_count, origin_contig,
                                        origin_bytes, origin_true_lb);
    MPIDI_Datatype_check_contig_size_extent_lb(target_datatype, target_count, target_contig,
                                               target_bytes, target_extent, target_true_lb);
    if (origin_bytes == 0)
        goto null_op_exit;

    int vni = MPIDI_WIN(win, am_vci);
    int vni_target = MPIDI_WIN_TARGET_VCI(win, target_rank);

    /* prepare remote addr and mr key.
     * Continue native path only when all segments are in the same registered memory region */
    bool target_mr_found;
    MPIDI_OFI_target_mr_t target_mr;
    target_mr_found = MPIDI_OFI_prepare_target_mr(target_rank,
                                                  target_disp, target_extent, target_true_lb,
                                                  win, winattr, &target_mr);
    if (unlikely(!target_mr_found))
        goto am_fallback;

    /* contiguous messages */
    if (origin_contig && target_contig) {
        MPI_Datatype basic_type;
        enum fi_op fi_op;
        enum fi_datatype fi_dt;
        MPI_Aint max_count, max_size, dt_size, basic_count;

        /* accept only same predefined basic datatype */
        MPIDI_OFI_GET_BASIC_TYPE(target_datatype, basic_type);
        MPIR_Assert(basic_type != MPI_DATATYPE_NULL);

        /* query atomics max count support */
        MPIDI_OFI_query_acc_atomic_support(basic_type, MPIDI_OFI_QUERY_ATOMIC_COUNT,
                                           op, win, winattr, &fi_dt, &fi_op, &max_count, &dt_size);
        if (max_count == 0)
            goto am_fallback;

        /* query ordering max size support */
        max_size = MPIDI_OFI_check_acc_order_size(win, max_count * dt_size);
        max_size = MPL_MIN(max_size, MPIDI_OFI_global.max_msg_size);
        /* fall back to active message if cannot send all data in one message */
        if (max_size < target_bytes)
            goto am_fallback;
        basic_count = target_bytes / dt_size;
        MPIR_Assert(target_bytes % dt_size == 0);

        /* Ensure completion of outstanding AMs for atomicity. */
        MPIDIG_wait_am_acc(win, target_rank);

        MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vni).lock);
        uint64_t flags;
        if (sigreq) {
            MPIDI_OFI_REQUEST_CREATE(*sigreq, MPIR_REQUEST_KIND__RMA, 0);
            flags = FI_COMPLETION | FI_DELIVERY_COMPLETE;
        } else {
            flags = FI_DELIVERY_COMPLETE;
        }

        struct fi_ioc originv;
        struct fi_rma_ioc targetv;
        struct fi_msg_atomic msg;

        originv.addr = MPIR_get_contig_ptr(origin_addr, origin_true_lb);
        originv.count = basic_count;
        targetv.addr = target_mr.addr + target_true_lb;
        targetv.count = basic_count;
        targetv.key = target_mr.mr_key;

        msg.msg_iov = &originv;
        msg.desc = NULL;
        msg.iov_count = 1;
        msg.addr = MPIDI_OFI_av_to_phys(addr, nic, vni, vni_target);
        msg.rma_iov = &targetv;
        msg.rma_iov_count = 1;
        msg.datatype = fi_dt;
        msg.op = fi_op;
        msg.context = NULL;
        msg.data = 0;
        MPIDI_OFI_INIT_CHUNK_CONTEXT(win, sigreq);
        MPIDI_OFI_CALL_RETRY(fi_atomicmsg(MPIDI_OFI_WIN(win).ep, &msg, flags), vni,
                             rdma_atomicto, FALSE);
        /* Complete signal request to inform completion to user. */
        MPIDI_OFI_sigreq_complete(sigreq);
        MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vni).lock);
        goto fn_exit;
    }

  am_fallback:
    /* Wait for OFI acc to complete for atomicity.
     * For now, there is no FI flag to track atomic only ops, we use RMA level cntr. */
    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vni).lock);
    MPIDI_OFI_win_do_progress(win, vni);
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vni).lock);
    if (sigreq)
        mpi_errno = MPIDIG_mpi_raccumulate(origin_addr, origin_count, origin_datatype, target_rank,
                                           target_disp, target_count, target_datatype, op, win,
                                           sigreq);
    else
        mpi_errno = MPIDIG_mpi_accumulate(origin_addr, origin_count, origin_datatype, target_rank,
                                          target_disp, target_count, target_datatype, op, win);

  fn_exit:
    MPIR_FUNC_EXIT;
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
                                                         MPI_Aint origin_count,
                                                         MPI_Datatype origin_datatype,
                                                         void *result_addr,
                                                         MPI_Aint result_count,
                                                         MPI_Datatype result_datatype,
                                                         int target_rank,
                                                         MPI_Aint target_disp,
                                                         MPI_Aint target_count,
                                                         MPI_Datatype target_datatype,
                                                         MPI_Op op, MPIR_Win * win,
                                                         MPIDI_av_entry_t * addr,
                                                         MPIDI_winattr_t winattr,
                                                         MPIR_Request ** sigreq)
{
    int mpi_errno = MPI_SUCCESS;
    int target_contig, origin_contig, result_contig;
    MPI_Aint target_bytes, target_extent;
    MPI_Aint origin_true_lb, target_true_lb, result_true_lb;
    int nic = 0;

    MPIR_FUNC_ENTER;

    MPIDIG_RMA_OP_CHECK_SYNC(target_rank, win);

    MPIDI_Datatype_check_contig_lb(origin_datatype, origin_contig, origin_true_lb);
    MPIDI_Datatype_check_contig_lb(result_datatype, result_contig, result_true_lb);
    MPIDI_Datatype_check_contig_size_extent_lb(target_datatype, target_count, target_contig,
                                               target_bytes, target_extent, target_true_lb);
    if (target_bytes == 0)
        goto null_op_exit;

    int vni = MPIDI_WIN(win, am_vci);
    int vni_target = MPIDI_WIN_TARGET_VCI(win, target_rank);

    /* contiguous messages */
    if (origin_contig && target_contig && result_contig) {
        /* prepare remote addr and mr key.
         * Continue native path only when all segments are in the same registered memory region */
        bool target_mr_found;
        MPIDI_OFI_target_mr_t target_mr;
        target_mr_found = MPIDI_OFI_prepare_target_mr(target_rank,
                                                      target_disp, target_extent, target_true_lb,
                                                      win, winattr, &target_mr);
        if (unlikely(!target_mr_found))
            goto am_fallback;

        MPI_Datatype basic_type;
        enum fi_op fi_op;
        enum fi_datatype fi_dt;
        MPI_Aint max_count, max_size, dt_size, basic_count;

        /* accept only same predefined basic datatype */
        MPIDI_OFI_GET_BASIC_TYPE(target_datatype, basic_type);
        MPIR_Assert(basic_type != MPI_DATATYPE_NULL);

        /* query atomics max count support */
        MPIDI_OFI_query_acc_atomic_support(basic_type, MPIDI_OFI_QUERY_FETCH_ATOMIC_COUNT,
                                           op, win, winattr, &fi_dt, &fi_op, &max_count, &dt_size);
        if (max_count == 0)
            goto am_fallback;

        /* query ordering max size support */
        max_size = MPIDI_OFI_check_acc_order_size(win, max_count * dt_size);
        max_size = MPL_MIN(max_size, MPIDI_OFI_global.max_msg_size);
        /* fall back to active message if cannot send all data in one message */
        if (max_size < target_bytes)
            goto am_fallback;
        basic_count = target_bytes / dt_size;
        MPIR_Assert(target_bytes % dt_size == 0);

        /* Ensure completion of outstanding AMs for atomicity. */
        MPIDIG_wait_am_acc(win, target_rank);

        MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vni).lock);
        uint64_t flags;
        if (sigreq) {
            MPIDI_OFI_REQUEST_CREATE(*sigreq, MPIR_REQUEST_KIND__RMA, 0);
            flags = FI_COMPLETION | FI_DELIVERY_COMPLETE;
        } else {
            flags = FI_DELIVERY_COMPLETE;
        }

        struct fi_ioc originv, resultv;
        struct fi_rma_ioc targetv;
        struct fi_msg_atomic msg;

        originv.addr = MPIR_get_contig_ptr(origin_addr, origin_true_lb);
        originv.count = basic_count;
        resultv.addr = MPIR_get_contig_ptr(result_addr, result_true_lb);
        resultv.count = basic_count;
        targetv.addr = target_mr.addr + target_true_lb;
        targetv.count = basic_count;
        targetv.key = target_mr.mr_key;

        msg.msg_iov = &originv;
        msg.desc = NULL;
        msg.iov_count = 1;
        msg.addr = MPIDI_OFI_av_to_phys(addr, nic, vni, vni_target);
        msg.rma_iov = &targetv;
        msg.rma_iov_count = 1;
        msg.datatype = fi_dt;
        msg.op = fi_op;
        msg.context = NULL;
        msg.data = 0;
        MPIDI_OFI_INIT_CHUNK_CONTEXT(win, sigreq);
        MPIDI_OFI_CALL_RETRY(fi_fetch_atomicmsg(MPIDI_OFI_WIN(win).ep, &msg, &resultv,
                                                NULL, 1, flags), vni, rdma_readfrom, FALSE);
        /* Complete signal request to inform completion to user. */
        MPIDI_OFI_sigreq_complete(sigreq);
        MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vni).lock);
        goto fn_exit;
    }

  am_fallback:
    /* Wait for OFI getacc to complete for atomicity.
     * For now, there is no FI flag to track atomic only ops, we use RMA level cntr. */
    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vni).lock);
    MPIDI_OFI_win_do_progress(win, vni);
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vni).lock);
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
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
  null_op_exit:
    mpi_errno = MPI_SUCCESS;
    if (sigreq) {
        *sigreq = MPIR_Request_create_complete(MPIR_REQUEST_KIND__RMA);
    }
    goto fn_exit;
}


MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_raccumulate(const void *origin_addr,
                                                      MPI_Aint origin_count,
                                                      MPI_Datatype origin_datatype,
                                                      int target_rank,
                                                      MPI_Aint target_disp,
                                                      MPI_Aint target_count,
                                                      MPI_Datatype target_datatype,
                                                      MPI_Op op, MPIR_Win * win,
                                                      MPIDI_av_entry_t * av,
                                                      MPIDI_winattr_t winattr,
                                                      MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    if (
#ifndef MPIDI_CH4_DIRECT_NETMOD
           /* We have to disable network-based atomics in auto mode.
            * Because concurrent atomics may be performed by CPU (e.g., op
            * over shared memory, or op issues to process-self.
            * If ACCU_NO_SHM is set, then all above ops are issued
            * via network thus we can safely use network-based atomics. */
           !(winattr & MPIDI_WINATTR_ACCU_NO_SHM) ||
#endif
           !MPIDI_OFI_ENABLE_RMA || !MPIDI_OFI_ENABLE_ATOMICS ||
           !(winattr & MPIDI_WINATTR_NM_REACHABLE) || MPIR_GPU_query_pointer_is_dev(origin_addr)) {
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
                                        target_count, target_datatype, op, win,
                                        av, winattr, request);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_rget_accumulate(const void *origin_addr,
                                                          MPI_Aint origin_count,
                                                          MPI_Datatype origin_datatype,
                                                          void *result_addr,
                                                          MPI_Aint result_count,
                                                          MPI_Datatype result_datatype,
                                                          int target_rank,
                                                          MPI_Aint target_disp,
                                                          MPI_Aint target_count,
                                                          MPI_Datatype target_datatype,
                                                          MPI_Op op, MPIR_Win * win,
                                                          MPIDI_av_entry_t * av,
                                                          MPIDI_winattr_t winattr,
                                                          MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    if (
#ifndef MPIDI_CH4_DIRECT_NETMOD
           /* We have to disable network-based atomics in auto mode.
            * Because concurrent atomics may be performed by CPU (e.g., op
            * over shared memory, or op issues to process-self.
            * If ACCU_NO_SHM is set, then all above ops are issued
            * via network thus we can safely use network-based atomics. */
           !(winattr & MPIDI_WINATTR_ACCU_NO_SHM) ||
#endif
           !MPIDI_OFI_ENABLE_RMA || !MPIDI_OFI_ENABLE_ATOMICS ||
           !(winattr & MPIDI_WINATTR_NM_REACHABLE) ||
           MPIR_GPU_query_pointer_is_dev(origin_addr) ||
           MPIR_GPU_query_pointer_is_dev(result_addr)) {
        mpi_errno =
            MPIDIG_mpi_rget_accumulate(origin_addr, origin_count, origin_datatype, result_addr,
                                       result_count, result_datatype, target_rank, target_disp,
                                       target_count, target_datatype, op, win, request);
        goto fn_exit;
    }

    mpi_errno = MPIDI_OFI_do_get_accumulate(origin_addr, origin_count, origin_datatype,
                                            result_addr, result_count, result_datatype,
                                            target_rank, target_disp, target_count,
                                            target_datatype, op, win, av, winattr, request);

  fn_exit:
    MPIR_FUNC_EXIT;
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
    MPI_Aint max_count, max_size, dt_size, bytes;
    MPI_Aint true_lb ATTRIBUTE((unused));
    void *buffer, *rbuffer;
    struct fi_ioc originv;
    struct fi_ioc resultv;
    struct fi_rma_ioc targetv;
    struct fi_msg_atomic msg;
    MPIR_FUNC_ENTER;

    int vni = MPIDI_WIN(win, am_vci);
    int vni_target = MPIDI_WIN_TARGET_VCI(win, target_rank);
    int nic = 0;

    if (
#ifndef MPIDI_CH4_DIRECT_NETMOD
           /* We have to disable network-based atomics in auto mode.
            * Because concurrent atomics may be performed by CPU (e.g., op
            * over shared memory, or op issues to process-self.
            * If ACCU_NO_SHM is set, then all above ops are issued
            * via network thus we can safely use network-based atomics. */
           !(winattr & MPIDI_WINATTR_ACCU_NO_SHM) ||
#endif
           !MPIDI_OFI_ENABLE_RMA || !MPIDI_OFI_ENABLE_ATOMICS ||
           !(winattr & MPIDI_WINATTR_NM_REACHABLE) ||
           MPIR_GPU_query_pointer_is_dev(origin_addr) ||
           MPIR_GPU_query_pointer_is_dev(result_addr)) {
        mpi_errno =
            MPIDIG_mpi_fetch_and_op(origin_addr, result_addr, datatype, target_rank, target_disp,
                                    op, win);
        goto fn_exit;
    }

    MPIDIG_RMA_OP_CHECK_SYNC(target_rank, win);
    MPIDI_Datatype_check_size_lb(datatype, 1, bytes, true_lb);
    if (bytes == 0)
        goto fn_exit;

    /* prepare remote addr and mr key.
     * Continue native path only when all segments are in the same registered memory region */
    bool target_mr_found;
    MPIDI_OFI_target_mr_t target_mr;
    target_mr_found = MPIDI_OFI_prepare_target_mr(target_rank, target_disp, bytes, 0,
                                                  win, winattr, &target_mr);
    if (unlikely(!target_mr_found))
        goto am_fallback;

    buffer = (char *) origin_addr;      /* ignore true_lb, which should be always zero */
    rbuffer = (char *) result_addr;

    MPIDI_OFI_query_acc_atomic_support(datatype, MPIDI_OFI_QUERY_FETCH_ATOMIC_COUNT,
                                       op, win, winattr, &fi_dt, &fi_op, &max_count, &dt_size);
    if (max_count == 0)
        goto am_fallback;

    max_size = MPIDI_OFI_check_acc_order_size(win, dt_size);
    /* It's impossible to transfer data if buffer size is smaller than basic datatype size.
     *  TODO: we assume all processes should use the same max_size and dt_size, true ? */
    if (max_size < dt_size)
        goto am_fallback;

    /* Ensure completion of outstanding AMs for atomicity. */
    MPIDIG_wait_am_acc(win, target_rank);

    originv.addr = (void *) buffer;
    originv.count = 1;
    resultv.addr = (void *) rbuffer;
    resultv.count = 1;
    targetv.addr = target_mr.addr;
    targetv.count = 1;
    targetv.key = target_mr.mr_key;

    msg.msg_iov = &originv;
    msg.desc = NULL;
    msg.iov_count = 1;
    msg.addr = MPIDI_OFI_av_to_phys(av, nic, vni, vni_target);
    msg.rma_iov = &targetv;
    msg.rma_iov_count = 1;
    msg.datatype = fi_dt;
    msg.op = fi_op;
    msg.context = NULL;
    msg.data = 0;

    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vni).lock);
    MPIDI_OFI_win_cntr_incr(win);
    MPIDI_OFI_CALL_RETRY(fi_fetch_atomicmsg(MPIDI_OFI_WIN(win).ep, &msg, &resultv,
                                            NULL, 1, 0), vni, rdma_readfrom, FALSE);
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vni).lock);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
  am_fallback:
    /* Wait for OFI fetch_and_op to complete for atomicity.
     * For now, there is no FI flag to track atomic only ops, we use RMA level cntr. */
    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vni).lock);
    MPIDI_OFI_win_do_progress(win, vni);
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vni).lock);
    return MPIDIG_mpi_fetch_and_op(origin_addr, result_addr, datatype, target_rank, target_disp, op,
                                   win);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_rget(void *origin_addr,
                                               MPI_Aint origin_count,
                                               MPI_Datatype origin_datatype,
                                               int target_rank,
                                               MPI_Aint target_disp,
                                               MPI_Aint target_count,
                                               MPI_Datatype target_datatype,
                                               MPIR_Win * win, MPIDI_av_entry_t * av,
                                               MPIDI_winattr_t winattr, MPIR_Request ** request)
{
    MPIR_FUNC_ENTER;
    int mpi_errno = MPI_SUCCESS;

    if (!MPIDI_OFI_ENABLE_RMA || !(winattr & MPIDI_WINATTR_NM_REACHABLE) ||
        MPIR_GPU_query_pointer_is_dev(origin_addr)) {
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
    MPIR_FUNC_EXIT;
    return mpi_errno;
}


MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_get_accumulate(const void *origin_addr,
                                                         MPI_Aint origin_count,
                                                         MPI_Datatype origin_datatype,
                                                         void *result_addr,
                                                         MPI_Aint result_count,
                                                         MPI_Datatype result_datatype,
                                                         int target_rank,
                                                         MPI_Aint target_disp,
                                                         MPI_Aint target_count,
                                                         MPI_Datatype target_datatype, MPI_Op op,
                                                         MPIR_Win * win, MPIDI_av_entry_t * av,
                                                         MPIDI_winattr_t winattr)
{
    int mpi_errno;
    MPIR_FUNC_ENTER;

    if (
#ifndef MPIDI_CH4_DIRECT_NETMOD
           /* We have to disable network-based atomics in auto mode.
            * Because concurrent atomics may be performed by CPU (e.g., op
            * over shared memory, or op issues to process-self.
            * If ACCU_NO_SHM is set, then all above ops are issued
            * via network thus we can safely use network-based atomics. */
           !(winattr & MPIDI_WINATTR_ACCU_NO_SHM) ||
#endif
           !MPIDI_OFI_ENABLE_RMA || !MPIDI_OFI_ENABLE_ATOMICS ||
           !(winattr & MPIDI_WINATTR_NM_REACHABLE) || MPIR_GPU_query_pointer_is_dev(origin_addr) ||
           MPIR_GPU_query_pointer_is_dev(result_addr)) {
        mpi_errno =
            MPIDIG_mpi_get_accumulate(origin_addr, origin_count, origin_datatype, result_addr,
                                      result_count, result_datatype, target_rank, target_disp,
                                      target_count, target_datatype, op, win);
        goto fn_exit;
    }

    mpi_errno = MPIDI_OFI_do_get_accumulate(origin_addr, origin_count, origin_datatype,
                                            result_addr, result_count, result_datatype,
                                            target_rank, target_disp, target_count,
                                            target_datatype, op, win, av, winattr, NULL);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_accumulate(const void *origin_addr,
                                                     MPI_Aint origin_count,
                                                     MPI_Datatype origin_datatype,
                                                     int target_rank,
                                                     MPI_Aint target_disp,
                                                     MPI_Aint target_count,
                                                     MPI_Datatype target_datatype, MPI_Op op,
                                                     MPIR_Win * win, MPIDI_av_entry_t * av,
                                                     MPIDI_winattr_t winattr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    if (
#ifndef MPIDI_CH4_DIRECT_NETMOD
           /* We have to disable network-based atomics in auto mode.
            * Because concurrent atomics may be performed by CPU (e.g., op
            * over shared memory, or op issues to process-self.
            * If ACCU_NO_SHM is set, then all above ops are issued
            * via network thus we can safely use network-based atomics. */
           !(winattr & MPIDI_WINATTR_ACCU_NO_SHM) ||
#endif
           !MPIDI_OFI_ENABLE_RMA || !MPIDI_OFI_ENABLE_ATOMICS ||
           !(winattr & MPIDI_WINATTR_NM_REACHABLE) || MPIR_GPU_query_pointer_is_dev(origin_addr)) {
        mpi_errno =
            MPIDIG_mpi_accumulate(origin_addr, origin_count, origin_datatype, target_rank,
                                  target_disp, target_count, target_datatype, op, win);
        goto fn_exit;
    }

    mpi_errno = MPIDI_OFI_do_accumulate(origin_addr,
                                        origin_count,
                                        origin_datatype,
                                        target_rank,
                                        target_disp, target_count, target_datatype, op, win,
                                        av, winattr, NULL);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
}

#endif /* OFI_RMA_H_INCLUDED */

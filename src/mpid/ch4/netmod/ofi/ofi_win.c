/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "ofi_impl.h"
#include "ofi_noinline.h"

static void load_acc_hint(MPIR_Win * win);
static void set_rma_fi_info(MPIR_Win * win, struct fi_info *finfo);
static int win_allgather(MPIR_Win * win, void *base, int disp_unit);
static int win_set_per_win_sync(MPIR_Win * win);
static int win_init_sep(MPIR_Win * win);
static int win_init_stx(MPIR_Win * win);
static int win_init_global(MPIR_Win * win);
static int win_init(MPIR_Win * win);
static void win_init_am(MPIR_Win * win);

static void load_acc_hint(MPIR_Win * win)
{
    int op_index = 0, i;

    MPIR_FUNC_ENTER;

    if (!MPIDI_OFI_WIN(win).acc_hint)
        MPIDI_OFI_WIN(win).acc_hint = MPL_malloc(sizeof(MPIDI_OFI_win_acc_hint_t), MPL_MEM_RMA);

    /* TODO: we assume all pes pass the same hint. Allreduce is needed to check
     * the maximal value or the spec must define it as same value on all pes.
     * Now we assume the spec requires same value on all pes. */

    /* We translate the atomic op hints to max count allowed for all possible atomics with each
     * datatype. We do not need more specific info (e.g., <datatype, op>, because any process may use
     * the op with accumulate or get_accumulate.*/
    for (i = 0; i < MPIR_DATATYPE_N_PREDEFINED; i++) {
        MPIDI_OFI_WIN(win).acc_hint->dtypes_max_count[i] = 0;
        bool first_valid_op = true;

        MPI_Datatype dt = MPIR_Datatype_predefined_get_type(i);
        if (dt == MPI_DATATYPE_NULL)
            continue;   /* skip disabled datatype */

        for (op_index = 0; op_index < MPIDIG_ACCU_NUM_OP; op_index++) {
            uint64_t max_count = 0;
            /* Calculate the max count of all possible atomics if this op is enabled.
             * If the op is disabled for the datatype, the max counts are set to 0 (see util.c).*/
            if (MPIDIG_WIN(win, info_args).which_accumulate_ops & (1 << op_index)) {
                MPI_Op op = MPIDIU_win_acc_get_op(op_index);

                /* Invalid <datatype, op> pairs should be excluded as it is never used in a
                 * correct program (e.g., <double, MAXLOC>).*/
                if (!MPIDI_OFI_global.win_op_table[i][op_index].mpi_acc_valid)
                    continue;

                if (op == MPI_NO_OP)    /* atomic get */
                    max_count = MPIDI_OFI_global.win_op_table[i][op_index].max_fetch_atomic_count;
                else if (op == MPI_OP_NULL)     /* compare and swap */
                    max_count = MPIDI_OFI_global.win_op_table[i][op_index].max_compare_atomic_count;
                else    /* atomic write and fetch_and_write */
                    max_count = MPL_MIN(MPIDI_OFI_global.win_op_table[i][op_index].max_atomic_count,
                                        MPIDI_OFI_global.
                                        win_op_table[i][op_index].max_fetch_atomic_count);

                /* calculate the minimal max_count. */
                if (first_valid_op) {
                    MPIDI_OFI_WIN(win).acc_hint->dtypes_max_count[i] = max_count;
                    first_valid_op = false;
                } else
                    MPIDI_OFI_WIN(win).acc_hint->dtypes_max_count[i] =
                        MPL_MIN(max_count, MPIDI_OFI_WIN(win).acc_hint->dtypes_max_count[i]);
            }
        }
    }

    MPIR_FUNC_EXIT;
}

/* Set OFI attributes and capabilities for RMA. */
static void set_rma_fi_info(MPIR_Win * win, struct fi_info *finfo)
{
    finfo->caps = FI_RMA | FI_WRITE | FI_READ | FI_ATOMIC;
    finfo->tx_attr->caps = FI_RMA | FI_WRITE | FI_READ | FI_ATOMIC;

    /* Update msg_order by accumulate ordering in window info.
     * Accumulate ordering cannot easily be changed once the window has been created.
     * OFI implementation ignores acc ordering hints issued in MPI_WIN_SET_INFO()
     * after window is created. */
    finfo->tx_attr->msg_order = FI_ORDER_NONE;  /* FI_ORDER_NONE is an alias for the value 0 */
    if ((MPIDIG_WIN(win, info_args).accumulate_ordering & MPIDIG_ACCU_ORDER_RAR) ==
        MPIDIG_ACCU_ORDER_RAR)
#ifdef FI_ORDER_ATOMIC_RAR
        finfo->tx_attr->msg_order |= FI_ORDER_ATOMIC_RAR;
#else
        finfo->tx_attr->msg_order |= FI_ORDER_RAR;
#endif
    if ((MPIDIG_WIN(win, info_args).accumulate_ordering & MPIDIG_ACCU_ORDER_RAW) ==
        MPIDIG_ACCU_ORDER_RAW)
#ifdef FI_ORDER_ATOMIC_RAW
        finfo->tx_attr->msg_order |= FI_ORDER_ATOMIC_RAW;
#else
        finfo->tx_attr->msg_order |= FI_ORDER_RAW;
#endif
    if ((MPIDIG_WIN(win, info_args).accumulate_ordering & MPIDIG_ACCU_ORDER_WAR) ==
        MPIDIG_ACCU_ORDER_WAR)
#ifdef FI_ORDER_ATOMIC_WAR
        finfo->tx_attr->msg_order |= FI_ORDER_ATOMIC_WAR;
#else
        finfo->tx_attr->msg_order |= FI_ORDER_WAR;
#endif
    if ((MPIDIG_WIN(win, info_args).accumulate_ordering & MPIDIG_ACCU_ORDER_WAW) ==
        MPIDIG_ACCU_ORDER_WAW)
#ifdef FI_ORDER_ATOMIC_WAW
        finfo->tx_attr->msg_order |= FI_ORDER_ATOMIC_WAW;
#else
        finfo->tx_attr->msg_order |= FI_ORDER_WAW;
#endif
}

static int win_allgather(MPIR_Win * win, void *base, int disp_unit)
{
    int i, same_disp, mpi_errno = MPI_SUCCESS;
    uint32_t first;
    uint64_t local_key = 0;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    MPIR_Comm *comm_ptr = win->comm_ptr;
    MPIDI_OFI_win_targetinfo_t *winfo;
    int nic = 0;

    MPIR_FUNC_ENTER;

    if (!MPIDI_OFI_ENABLE_MR_PROV_KEY) {
        if (MPIDIG_WIN(win, info_args).optimized_mr &&
            MPIDIG_WIN(win, info_args).accumulate_ordering == 0) {
            /* accumulate_ordering must be set to zero to support creation of optimized
             * memory region for use with lower-overhead, unordered RMA operations.
             * Attempting to create an optimized memory region key. Gets the next MR key that's
             * available to the processes involved in the RMA window. Use the current maximum + 1
             * to ensure that the key is available for all processes. */
            mpi_errno = MPIR_Allreduce(&MPIDI_OFI_global.global_max_optimized_mr_key, &local_key, 1,
                                       MPI_UNSIGNED, MPI_MAX, comm_ptr, &errflag);
            MPIR_ERR_CHECK(mpi_errno);

            if (local_key + 1 < MPIDI_OFI_NUM_OPTIMIZED_MEMORY_REGIONS) {
                MPIDI_OFI_global.global_max_optimized_mr_key = local_key + 1;
                MPIDI_OFI_WIN(win).mr_key = local_key;
            }
        }
        /* Assign regular memory registration key if the optimized one is
         * not requested or exhausted */
        if (local_key + 1 >= MPIDI_OFI_NUM_OPTIMIZED_MEMORY_REGIONS ||
            !MPIDIG_WIN(win, info_args).optimized_mr) {
            /* Makes sure that regular mr key does not fall within optimized mr key range */
            MPIDI_OFI_WIN(win).mr_key =
                MPIDI_OFI_NUM_OPTIMIZED_MEMORY_REGIONS + MPIDI_OFI_WIN(win).win_id;
        }
    } else {
        /* Expect provider to assign the key */
        MPIDI_OFI_WIN(win).mr_key = 0;
    }

    /* we need register mr on the correct domain for the vni */
    int vni = MPIDI_WIN(win, am_vci);
    int ctx_idx = MPIDI_OFI_get_ctx_index(NULL, vni, nic);

    /* Register the allocated win buffer or MPI_BOTTOM (NULL) for dynamic win.
     * It is clear that we cannot register NULL when FI_MR_ALLOCATED is set, thus
     * we skip trial and return immediately. When FI_MR_ALLOCATED is not set, however,
     * it lacks documentation defining whether the provider can accept NULL and whether
     * it means registration of the entire virtual address space, although we have observed
     * successful use in most of the existing providers. Thus, we take a try here for most
     * providers. For providers like CXI, FI_MR_ALLOCATED is not set but registration with
     * non-zero address is not supported. For such providers, registration is skipped by
     * using the MPIDI_OFI_ENABLE_MR_REGISTER_NULL capability set variable. */
    int rc = 0, allrc = 0;
    MPIDI_OFI_WIN(win).mr = NULL;

    if (base || (win->create_flavor == MPI_WIN_FLAVOR_DYNAMIC &&
                 !MPIDI_OFI_ENABLE_MR_ALLOCATED && MPIDI_OFI_ENABLE_MR_REGISTER_NULL)) {
        size_t len;
        if (win->create_flavor == MPI_WIN_FLAVOR_DYNAMIC) {
            len = UINTPTR_MAX - (uintptr_t) base;
        } else {
            len = (size_t) win->size;
        }

        /* device buffers are not currently supported */
        if (MPIR_GPU_query_pointer_is_dev(base))
            rc = -1;
        else {
            MPIDI_OFI_CALL_RETURN(fi_mr_reg(MPIDI_OFI_global.ctx[ctx_idx].domain,       /* In:  Domain Object */
                                            base,       /* In:  Lower memory address */
                                            len,        /* In:  Length              */
                                            FI_REMOTE_READ | FI_REMOTE_WRITE,   /* In:  Expose MR for read  */
                                            0ULL,       /* In:  offset(not used)    */
                                            MPIDI_OFI_WIN(win).mr_key,  /* In:  requested key       */
                                            0ULL,       /* In:  flags               */
                                            &MPIDI_OFI_WIN(win).mr,     /* Out: memregion object    */
                                            NULL), rc); /* In:  context             */
            mpi_errno = MPIDI_OFI_mr_bind(MPIDI_OFI_global.prov_use[0], MPIDI_OFI_WIN(win).mr,
                                          MPIDI_OFI_WIN(win).ep, MPIDI_OFI_WIN(win).cmpl_cntr);
            MPIR_ERR_CHECK(mpi_errno);
        }
    } else if (win->create_flavor == MPI_WIN_FLAVOR_DYNAMIC) {
        /* We may still do native atomics with collective attach, let's load acc_hint */
        load_acc_hint(win);
        goto fn_exit;
    } else {
        /* Do nothing */
    }

    /* Check if any process fails to register. If so, release local MR and force AM path. */
    MPIR_Allreduce(&rc, &allrc, 1, MPI_INT, MPI_MIN, comm_ptr, &errflag);
    if (allrc < 0) {
        if (rc >= 0 && MPIDI_OFI_WIN(win).mr)
            MPIDI_OFI_CALL(fi_close(&MPIDI_OFI_WIN(win).mr->fid), fi_close);
        MPIDI_OFI_WIN(win).mr = NULL;
        goto fn_exit;
    } else {
        MPIDI_WIN(win, winattr) |= MPIDI_WINATTR_NM_REACHABLE;  /* enable NM native RMA */
    }

    MPIDI_OFI_WIN(win).winfo = MPL_malloc(sizeof(*winfo) * comm_ptr->local_size, MPL_MEM_RMA);

    winfo = MPIDI_OFI_WIN(win).winfo;
    winfo[comm_ptr->rank].disp_unit = disp_unit;

    if ((MPIDI_OFI_ENABLE_MR_PROV_KEY || MPIDI_OFI_ENABLE_MR_VIRT_ADDRESS) && MPIDI_OFI_WIN(win).mr) {
        /* MR_BASIC */
        MPIDI_OFI_WIN(win).mr_key = fi_mr_key(MPIDI_OFI_WIN(win).mr);
        winfo[comm_ptr->rank].mr_key = MPIDI_OFI_WIN(win).mr_key;
        winfo[comm_ptr->rank].base = (uintptr_t) base;
    }

    mpi_errno = MPIR_Allgather(MPI_IN_PLACE, 0,
                               MPI_DATATYPE_NULL,
                               winfo, sizeof(*winfo), MPI_BYTE, comm_ptr, &errflag);
    MPIR_ERR_CHECK(mpi_errno);

    if (!MPIDI_OFI_ENABLE_MR_PROV_KEY && !MPIDI_OFI_ENABLE_MR_VIRT_ADDRESS) {
        first = winfo[0].disp_unit;
        same_disp = 1;
        for (i = 1; i < comm_ptr->local_size; i++) {
            if (winfo[i].disp_unit != first) {
                same_disp = 0;
                break;
            }
        }
        if (same_disp) {
            MPL_free(MPIDI_OFI_WIN(win).winfo);
            MPIDI_OFI_WIN(win).winfo = NULL;
        }
    }

    load_acc_hint(win);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/*
 * Setup per-window sync using independent counter.
 *
 * Return value:
 * MPI_SUCCESS: Endpoint is successfully initialized.
 * MPIDI_OFI_ENAVAIL: OFI resource is not available.
 * MPIDI_OFI_EPERROR: OFI endpoint related failures.
 * MPI_ERR_OTHER: Other error occurs.
 */
static int win_set_per_win_sync(MPIR_Win * win)
{
    int ret, mpi_errno = MPI_SUCCESS;
    int nic = 0;
    int vni = MPIDI_WIN(win, am_vci);
    int ctx_idx = MPIDI_OFI_get_ctx_index(NULL, vni, nic);

    MPIR_FUNC_ENTER;

    struct fi_cntr_attr cntr_attr;
    memset(&cntr_attr, 0, sizeof(cntr_attr));
    cntr_attr.events = FI_CNTR_EVENTS_COMP;
    cntr_attr.wait_obj = FI_WAIT_UNSPEC;
    MPIDI_OFI_CALL_RETURN(fi_cntr_open(MPIDI_OFI_global.ctx[ctx_idx].domain, &cntr_attr,
                                       &MPIDI_OFI_WIN(win).cmpl_cntr, NULL), ret);
    if (ret < 0) {
        MPL_DBG_MSG(MPIDI_CH4_DBG_GENERAL, VERBOSE, "Failed to open completion counter.\n");
        mpi_errno = MPIDI_OFI_ENAVAIL;
        goto fn_fail;
    }

    MPIDI_OFI_WIN(win).issued_cntr = &MPIDI_OFI_WIN(win).issued_cntr_v;

    MPIDI_OFI_CALL_RETURN(fi_ep_bind
                          (MPIDI_OFI_WIN(win).ep, &MPIDI_OFI_WIN(win).cmpl_cntr->fid,
                           FI_READ | FI_WRITE), ret);

    if (ret < 0) {
        MPL_DBG_MSG(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                    "Failed to bind completion counter to endpoint.\n");
        /* Close and release completion counter resource. */
        MPIDI_OFI_CALL(fi_close(&MPIDI_OFI_WIN(win).cmpl_cntr->fid), cntrclose);
        mpi_errno = MPIDI_OFI_EPERROR;
        goto fn_fail;
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static void win_init_am(MPIR_Win * win)
{
    MPIR_Assert(MPIDI_WIN(win, am_vci) < MPIDI_OFI_global.num_vnis);
}

/*
 * Initialize endpoint using scalable endpoint.
 *
 * Return value:
 * MPI_SUCCESS: Endpoint is successfully initialized.
 * MPIDI_OFI_ENAVAIL: OFI resource is not available.
 * MPIDI_OFI_EPERROR: OFI endpoint related failures.
 * MPI_ERR_OTHER: Other error occurs.
 */
static int win_init_sep(MPIR_Win * win)
{
    int i, ret, mpi_errno = MPI_SUCCESS;
    struct fi_info *finfo;
    int nic = 0;
    int vni = MPIDI_WIN(win, am_vci);
    int ctx_idx = MPIDI_OFI_get_ctx_index(NULL, vni, nic);

    MPIR_FUNC_ENTER;

    finfo = fi_dupinfo(MPIDI_OFI_global.prov_use[0]);
    MPIR_Assert(finfo);


    /* Initialize scalable EP when first window is created. */
    if (MPIDI_OFI_global.ctx[ctx_idx].rma_sep == NULL) {
        /* NOTE: if MPIDI_OFI_VNI_USE_SEPCTX, we could share rma_stx_ctx across vnis */

        MPIDI_OFI_global.max_rma_sep_tx_cnt =
            MPL_MIN(MPIDI_OFI_global.prov_use[0]->domain_attr->max_ep_tx_ctx,
                    MPIR_CVAR_CH4_OFI_MAX_RMA_SEP_CTX);
        finfo->ep_attr->tx_ctx_cnt = MPIDI_OFI_global.max_rma_sep_tx_cnt;

        MPIDI_OFI_CALL_RETURN(fi_scalable_ep(MPIDI_OFI_global.ctx[ctx_idx].domain, finfo,
                                             &MPIDI_OFI_global.ctx[ctx_idx].rma_sep, NULL), ret);
        if (ret < 0) {
            MPL_DBG_MSG(MPIDI_CH4_DBG_GENERAL, VERBOSE, "Failed to create scalable endpoint.\n");
            MPIDI_OFI_global.ctx[ctx_idx].rma_sep = NULL;
            mpi_errno = MPIDI_OFI_ENAVAIL;
            goto fn_fail;
        }

        MPIDI_OFI_CALL_RETURN(fi_scalable_ep_bind(MPIDI_OFI_global.ctx[ctx_idx].rma_sep,
                                                  &(MPIDI_OFI_global.ctx[ctx_idx].av->fid), 0),
                              ret);
        if (ret < 0) {
            MPL_DBG_MSG(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                        "Failed to bind scalable endpoint to address vector.\n");
            MPIDI_OFI_global.ctx[ctx_idx].rma_sep = NULL;
            mpi_errno = MPIDI_OFI_ENAVAIL;
            goto fn_fail;
        }

        /* Allocate and initialize tx index array. */
        utarray_new(MPIDI_OFI_global.ctx[ctx_idx].rma_sep_idx_array, &ut_int_icd, MPL_MEM_RMA);
        for (i = 0; i < MPIDI_OFI_global.max_rma_sep_tx_cnt; i++) {
            utarray_push_back(MPIDI_OFI_global.ctx[ctx_idx].rma_sep_idx_array, &i, MPL_MEM_RMA);
        }
    }
    /* Set per window transmit attributes. */
    set_rma_fi_info(win, finfo);
    /* Get available transmit context index. */
    int *idx = (int *) utarray_back(MPIDI_OFI_global.ctx[ctx_idx].rma_sep_idx_array);
    if (idx == NULL) {
        mpi_errno = MPIDI_OFI_ENAVAIL;
        goto fn_fail;
    }
    /* Retrieve transmit context on scalable EP. */
    MPIDI_OFI_CALL_RETURN(fi_tx_context
                          (MPIDI_OFI_global.ctx[ctx_idx].rma_sep, *idx, finfo->tx_attr,
                           &(MPIDI_OFI_WIN(win).ep), NULL), ret);
    if (ret < 0) {
        MPL_DBG_MSG(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                    "Failed to retrieve transmit context from scalable endpoint.\n");
        mpi_errno = MPIDI_OFI_ENAVAIL;
        goto fn_fail;
    }

    MPIDI_OFI_WIN(win).sep_tx_idx = *idx;
    /* Pop this index out of reserving array. */
    utarray_pop_back(MPIDI_OFI_global.ctx[ctx_idx].rma_sep_idx_array);

    MPIDI_OFI_CALL_RETURN(fi_ep_bind(MPIDI_OFI_WIN(win).ep,
                                     &MPIDI_OFI_global.ctx[ctx_idx].cq->fid,
                                     FI_TRANSMIT | FI_SELECTIVE_COMPLETION), ret);
    if (ret < 0) {
        MPL_DBG_MSG(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                    "Failed to bind endpoint to completion queue.\n");
        mpi_errno = MPIDI_OFI_EPERROR;
        goto fn_fail;
    }

    if (win_set_per_win_sync(win) == MPI_SUCCESS) {
        MPIDI_OFI_CALL_RETURN(fi_enable(MPIDI_OFI_WIN(win).ep), ret);
        if (ret < 0) {
            MPL_DBG_MSG(MPIDI_CH4_DBG_GENERAL, VERBOSE, "Failed to activate endpoint.\n");
            mpi_errno = MPIDI_OFI_EPERROR;
            /* Close the per-window counter opened by win_set_per_win_sync */
            MPIDI_OFI_CALL(fi_close(&MPIDI_OFI_WIN(win).cmpl_cntr->fid), cntrclose);
            goto fn_fail;
        }
    } else {
        /* Fail in per-window sync initialization,
         * we are not using scalable endpoint without per-win sync support. */
        mpi_errno = MPIDI_OFI_EPERROR;
        goto fn_fail;
    }

  fn_exit:
    fi_freeinfo(finfo);
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    if (MPIDI_OFI_WIN(win).sep_tx_idx != -1) {
        /* Push tx idx back into available pool. */
        utarray_push_back(MPIDI_OFI_global.ctx[ctx_idx].rma_sep_idx_array,
                          &MPIDI_OFI_WIN(win).sep_tx_idx, MPL_MEM_RMA);
        MPIDI_OFI_WIN(win).sep_tx_idx = -1;
    }
    if (MPIDI_OFI_WIN(win).ep != NULL) {
        /* Close an endpoint and release all resources associated with it. */
        MPIDI_OFI_CALL(fi_close(&MPIDI_OFI_WIN(win).ep->fid), epclose);
        MPIDI_OFI_WIN(win).ep = NULL;
    }
    goto fn_exit;
}

/*
 * Initialize endpoint using shared transmit context.
 *
 * Return value:
 * MPI_SUCCESS: Endpoint is successfully initialized.
 * MPIDI_OFI_ENAVAIL: OFI resource is not available.
 * MPIDI_OFI_EPERROR: OFI endpoint related failures.
 * MPI_ERR_OTHER: Other error occurs.
 */
static int win_init_stx(MPIR_Win * win)
{
    /* Activate per-window EP/counter using STX */
    int ret, mpi_errno = MPI_SUCCESS;
    struct fi_info *finfo;
    bool have_per_win_cntr = false;
    int nic = 0;
    int vni = MPIDI_WIN(win, am_vci);
    int ctx_idx = MPIDI_OFI_get_ctx_index(NULL, vni, nic);

    MPIR_FUNC_ENTER;

    /* WB TODO - Only setting up nic[0] for RMA right now. */
    finfo = fi_dupinfo(MPIDI_OFI_global.prov_use[0]);
    MPIR_Assert(finfo);


    /* Initialize rma shared context when first window is created. */
    if (MPIDI_OFI_global.ctx[ctx_idx].rma_stx_ctx == NULL) {
        /* NOTE: if MPIDI_OFI_VNI_USE_SEPCTX, we could share rma_stx_ctx across vnis */

        struct fi_tx_attr tx_attr;
        memset(&tx_attr, 0, sizeof(tx_attr));
        /* A shared transmit context’s attributes must be a union of all associated
         * endpoints' transmit capabilities. */
        tx_attr.caps = FI_RMA | FI_WRITE | FI_READ | FI_ATOMIC;
        tx_attr.msg_order = FI_ORDER_RAR | FI_ORDER_RAW | FI_ORDER_WAR | FI_ORDER_WAW;
        tx_attr.op_flags = FI_DELIVERY_COMPLETE | FI_COMPLETION;
        MPIDI_OFI_CALL_RETURN(fi_stx_context(MPIDI_OFI_global.ctx[ctx_idx].domain, &tx_attr,
                                             &MPIDI_OFI_global.ctx[ctx_idx].rma_stx_ctx, NULL),
                              ret);
        if (ret < 0) {
            MPL_DBG_MSG(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                        "Failed to create RMA shared TX context.\n");
            MPIDI_OFI_global.ctx[ctx_idx].rma_stx_ctx = NULL;
            mpi_errno = MPIDI_OFI_ENAVAIL;
            goto fn_fail;
        }
    }

    /* Set per window transmit attributes. */
    set_rma_fi_info(win, finfo);
    /* Still need to take out rx capabilities for shared context. */
    finfo->rx_attr->caps = 0ULL;        /* RX capabilities not needed */

    finfo->ep_attr->tx_ctx_cnt = FI_SHARED_CONTEXT;     /* Request a shared context */
    finfo->ep_attr->rx_ctx_cnt = 0;     /* We don't need RX contexts */
    MPIDI_OFI_CALL_RETURN(fi_endpoint(MPIDI_OFI_global.ctx[ctx_idx].domain,
                                      finfo, &MPIDI_OFI_WIN(win).ep, NULL), ret);
    if (ret < 0) {
        MPL_DBG_MSG(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                    "Failed to create per-window EP using shared TX context, "
                    "falling back to global EP/counter scheme");
        mpi_errno = MPIDI_OFI_ENAVAIL;
        goto fn_fail;
    }

    if (win_set_per_win_sync(win) == MPI_SUCCESS) {
        have_per_win_cntr = true;
        MPIDI_OFI_CALL_RETURN(fi_ep_bind(MPIDI_OFI_WIN(win).ep,
                                         &MPIDI_OFI_global.ctx[ctx_idx].rma_stx_ctx->fid, 0), ret);
        if (ret < 0) {
            MPL_DBG_MSG(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                        "Failed to bind endpoint to shared transmit contxt.\n");
            mpi_errno = MPIDI_OFI_EPERROR;
            goto fn_fail;
        }

        MPIDI_OFI_CALL_RETURN(fi_ep_bind(MPIDI_OFI_WIN(win).ep,
                                         &MPIDI_OFI_global.ctx[ctx_idx].cq->fid,
                                         FI_TRANSMIT | FI_SELECTIVE_COMPLETION), ret);
        if (ret < 0) {
            MPL_DBG_MSG(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                        "Failed to bind endpoint to completion queue.\n");
            mpi_errno = MPIDI_OFI_EPERROR;
            goto fn_fail;
        }

        MPIDI_OFI_CALL_RETURN(fi_ep_bind
                              (MPIDI_OFI_WIN(win).ep, &MPIDI_OFI_global.ctx[ctx_idx].av->fid, 0),
                              ret);
        if (ret < 0) {
            MPL_DBG_MSG(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                        "Failed to bind endpoint to address vector.\n");
            mpi_errno = MPIDI_OFI_EPERROR;
            goto fn_fail;
        }

        MPIDI_OFI_CALL_RETURN(fi_enable(MPIDI_OFI_WIN(win).ep), ret);
        if (ret < 0) {
            MPL_DBG_MSG(MPIDI_CH4_DBG_GENERAL, VERBOSE, "Failed to activate endpoint.\n");
            mpi_errno = MPIDI_OFI_EPERROR;
            goto fn_fail;
        }
    } else {
        /* If per-win sync initialization is failed, we are not using shared transmit context. */
        mpi_errno = MPIDI_OFI_EPERROR;
        goto fn_fail;
    }

  fn_exit:
    fi_freeinfo(finfo);
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    if (MPIDI_OFI_WIN(win).ep != NULL) {
        /* Close an endpoint and release all resources associated with it. */
        MPIDI_OFI_CALL(fi_close(&MPIDI_OFI_WIN(win).ep->fid), epclose);
        MPIDI_OFI_WIN(win).ep = NULL;
    }
    if (have_per_win_cntr) {
        /* Close the per-window counter opened by win_set_per_win_sync */
        MPIDI_OFI_CALL(fi_close(&MPIDI_OFI_WIN(win).cmpl_cntr->fid), cntrclose);
    }
    goto fn_exit;
}

static int win_init_global(MPIR_Win * win)
{
    MPIR_FUNC_ENTER;

    int vni = MPIDI_WIN(win, am_vci);
    int nic = 0;
    int ctx_idx = MPIDI_OFI_get_ctx_index(NULL, vni, nic);

    MPIDI_OFI_WIN(win).ep = MPIDI_OFI_global.ctx[ctx_idx].tx;
#ifdef MPIDI_OFI_VNI_USE_DOMAIN
    MPIDI_OFI_WIN(win).cmpl_cntr = MPIDI_OFI_global.ctx[ctx_idx].rma_cmpl_cntr;
    MPIDI_OFI_WIN(win).issued_cntr = &MPIDI_OFI_global.ctx[ctx_idx].rma_issued_cntr;
#else
    ctx_idx = MPIDI_OFI_get_ctx_index(NULL, 0, nic);
    /* NOTE: shared with ctx[0] */
    MPIDI_OFI_WIN(win).cmpl_cntr = MPIDI_OFI_global.ctx[ctx_idx].rma_cmpl_cntr;
    MPIDI_OFI_WIN(win).issued_cntr = &MPIDI_OFI_global.ctx[ctx_idx].rma_issued_cntr;
#endif

    MPIR_FUNC_EXIT;

    return 0;
}

static int win_init(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    MPL_COMPILE_TIME_ASSERT(sizeof(MPIDI_Devwin_t) >= sizeof(MPIDI_OFI_win_t));

    memset(&MPIDI_OFI_WIN(win), 0, sizeof(MPIDI_OFI_win_t));

    MPIDI_OFI_WIN(win).win_id =
        MPIDI_OFI_mr_key_alloc(MPIDI_OFI_COLL_MR_KEY, win->comm_ptr->context_id);
    MPIR_ERR_CHKANDJUMP(MPIDI_OFI_WIN(win).win_id == MPIDI_OFI_INVALID_MR_KEY, mpi_errno,
                        MPI_ERR_OTHER, "**ofid_mr_key");

    if (MPIDI_OFI_WIN(win).win_id == -1ULL) {
        MPL_DBG_MSG(MPIDI_CH4_DBG_GENERAL, VERBOSE, "Failed to get global mr key.\n");
        mpi_errno = MPIDI_OFI_ENAVAIL;
        goto fn_exit;
    }

    MPIDIU_map_set(MPIDI_OFI_global.win_map, MPIDI_OFI_WIN(win).win_id, win, MPL_MEM_RMA);

    MPIDI_OFI_WIN(win).sep_tx_idx = -1; /* By default, -1 means not using scalable EP. */
    MPIDI_OFI_WIN(win).progress_counter = 1;

    /* First, try to enable scalable EP. */
    if (MPIR_CVAR_CH4_OFI_ENABLE_SCALABLE_ENDPOINTS && MPIR_CVAR_CH4_OFI_MAX_RMA_SEP_CTX > 0) {
        /* Create tx based on scalable EP. */
        if (win_init_sep(win) == MPI_SUCCESS) {
            goto fn_exit;
        }
    }

    /* If scalable EP is not available, try shared transmit context next. */
    /* Create tx using shared transmit context. */
    if (MPIDI_OFI_ENABLE_SHARED_CONTEXTS && win_init_stx(win) == MPI_SUCCESS) {
        goto fn_exit;
    }

    /* Fall back to use global EP, without per-window sync support. */
    win_init_global(win);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* Callback of MPL_gavl_tree_create to delete local registered MR for dynamic window */
static void dwin_close_mr(void *obj)
{
    MPIR_FUNC_ENTER;

    /* Close local MR */
    struct fid_mr *mr = (struct fid_mr *) obj;
    if (mr) {
        int ret ATTRIBUTE((unused));
        if (!MPIDI_OFI_ENABLE_MR_PROV_KEY) {
            uint64_t requested_key = fi_mr_key(mr);
            MPIDI_OFI_mr_key_free(MPIDI_OFI_LOCAL_MR_KEY, requested_key);
        }
        MPIDI_OFI_CALL_RETURN(fi_close(&mr->fid), ret);
        MPIR_Assert(ret >= 0);
    }

    MPIR_FUNC_EXIT;
    return;
}

/* Callback of MPL_gavl_tree_create to delete remote registered MR for dynamic window */
static void dwin_free_target_mr(void *obj)
{
    MPIR_FUNC_ENTER;

    /* Free cached buffer for remote MR */
    MPL_free(obj);

    MPIR_FUNC_EXIT;
    return;
}

int MPIDI_OFI_mpi_win_set_info(MPIR_Win * win, MPIR_Info * info)
{
    int mpi_errno;
    MPIR_FUNC_ENTER;

    mpi_errno = MPIDIG_mpi_win_set_info(win, info);

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

int MPIDI_OFI_mpi_win_get_info(MPIR_Win * win, MPIR_Info ** info_p_p)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    mpi_errno = MPIDIG_mpi_win_get_info(win, info_p_p);

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

int MPIDI_OFI_mpi_win_free(MPIR_Win ** win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    mpi_errno = MPIDIG_mpi_win_free(win_ptr);

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

int MPIDI_OFI_mpi_win_create(void *base, MPI_Aint length, int disp_unit, MPIR_Info * info,
                             MPIR_Comm * comm_ptr, MPIR_Win ** win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    mpi_errno = MPIDIG_mpi_win_create(base, length, disp_unit, info, comm_ptr, win_ptr);

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

int MPIDI_OFI_mpi_win_attach(MPIR_Win * win, void *base, MPI_Aint size)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    mpi_errno = MPIDIG_mpi_win_attach(win, base, size);

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

int MPIDI_OFI_mpi_win_allocate_shared(MPI_Aint size, int disp_unit, MPIR_Info * info_ptr,
                                      MPIR_Comm * comm_ptr, void **base_ptr, MPIR_Win ** win_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    mpi_errno = MPIDIG_mpi_win_allocate_shared(size, disp_unit, info_ptr, comm_ptr, base_ptr,
                                               win_ptr);

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

int MPIDI_OFI_mpi_win_detach(MPIR_Win * win, const void *base)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    mpi_errno = MPIDIG_mpi_win_detach(win, base);

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

int MPIDI_OFI_mpi_win_allocate(MPI_Aint size, int disp_unit, MPIR_Info * info, MPIR_Comm * comm,
                               void *baseptr, MPIR_Win ** win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    mpi_errno = MPIDIG_mpi_win_allocate(size, disp_unit, info, comm, baseptr, win_ptr);

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

int MPIDI_OFI_mpi_win_create_dynamic(MPIR_Info * info, MPIR_Comm * comm, MPIR_Win ** win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    mpi_errno = MPIDIG_mpi_win_create_dynamic(info, comm, win_ptr);

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

int MPIDI_OFI_mpi_win_create_hook(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;
    win_init_am(win);

    /* This hook is called by CH4 generic call after CH4 initialization */
    if (MPIDI_OFI_ENABLE_RMA) {
        mpi_errno = win_init(win);
        if (mpi_errno != MPI_SUCCESS)
            goto fn_fail;

        mpi_errno = win_allgather(win, win->base, win->disp_unit);
        if (mpi_errno != MPI_SUCCESS)
            goto fn_fail;
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_OFI_mpi_win_allocate_hook(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;
    win_init_am(win);

    /* This hook is called by CH4 generic call after CH4 initialization */
    if (MPIDI_OFI_ENABLE_RMA) {
        mpi_errno = win_init(win);
        MPIR_ERR_CHECK(mpi_errno);

        mpi_errno = win_allgather(win, win->base, win->disp_unit);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_OFI_mpi_win_allocate_shared_hook(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;
    win_init_am(win);

    /* This hook is called by CH4 generic call after CH4 initialization */
    if (MPIDI_OFI_ENABLE_RMA) {
        mpi_errno = win_init(win);
        MPIR_ERR_CHECK(mpi_errno);

        mpi_errno = win_allgather(win, win->base, win->disp_unit);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_OFI_mpi_win_create_dynamic_hook(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;
    win_init_am(win);

    MPIR_CHKPMEM_DECL(1);

    /* This hook is called by CH4 generic call after CH4 initialization */
    if (MPIDI_OFI_ENABLE_RMA) {
        mpi_errno = win_init(win);
        MPIR_ERR_CHECK(mpi_errno);

        mpi_errno = win_allgather(win, win->base, win->disp_unit);
        MPIR_ERR_CHECK(mpi_errno);

        /* Initialize memory region storage for per-attach memory registration
         * if MR is not successfully registered here and all attach calls are collective */
        if (!MPIDI_OFI_WIN(win).mr && MPIDIG_WIN(win, info_args).coll_attach) {
            /* Initialize AVL tree for local registered region */
            int mpl_err = MPL_SUCCESS;
            mpl_err = MPL_gavl_tree_create(dwin_close_mr, &MPIDI_OFI_WIN(win).dwin_mrs);
            MPIR_ERR_CHKANDJUMP(mpl_err != MPL_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                                "**mpl_gavl_create");

            /* Initialize AVL trees for remote registered regions */
            MPIR_CHKPMEM_MALLOC(MPIDI_OFI_WIN(win).dwin_target_mrs, MPL_gavl_tree_t *,
                                sizeof(MPL_gavl_tree_t) * win->comm_ptr->local_size, mpi_errno,
                                "AVL tree for remote dynamic win memory regions", MPL_MEM_RMA);
            int i;
            for (i = 0; i < win->comm_ptr->local_size; i++) {
                mpl_err = MPL_gavl_tree_create(dwin_free_target_mr,
                                               &MPIDI_OFI_WIN(win).dwin_target_mrs[i]);
                MPIR_ERR_CHKANDJUMP(mpl_err != MPL_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                                    "**mpl_gavl_create");
            }

            /* Enable native RMA for now. Will be disabled if any registration fails at attach. */
            MPIDI_WIN(win, winattr) |= MPIDI_WINATTR_NM_REACHABLE;
            MPIDI_WIN(win, winattr) |= MPIDI_WINATTR_NM_DYNAMIC_MR;
        }
    }

    MPIR_CHKPMEM_COMMIT();

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    MPIR_CHKPMEM_REAP();
    goto fn_exit;
}

typedef struct dwin_target_mr {
    uint64_t mr_key;
    uintptr_t size;
    const void *base;
} dwin_target_mr_t;

int MPIDI_OFI_mpi_win_attach_hook(MPIR_Win * win, void *base, MPI_Aint size)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    MPIR_Comm *comm_ptr = win->comm_ptr;
    dwin_target_mr_t *target_mrs;
    int mpl_err = MPL_SUCCESS, i;
    int nic = 0;
    int vni = MPIDI_WIN(win, am_vci);
    int ctx_idx = MPIDI_OFI_get_ctx_index(NULL, vni, nic);

    MPIR_FUNC_ENTER;

    MPIR_CHKLMEM_DECL(1);

    if (!MPIDI_OFI_ENABLE_RMA || MPIDI_OFI_WIN(win).mr || !MPIDIG_WIN(win, info_args).coll_attach)
        goto fn_exit;

    uint64_t requested_key = 0ULL;
    if (!MPIDI_OFI_ENABLE_MR_PROV_KEY) {
        requested_key = MPIDI_OFI_mr_key_alloc(MPIDI_OFI_LOCAL_MR_KEY, MPIDI_OFI_INVALID_MR_KEY);
        MPIR_ERR_CHKANDJUMP(requested_key == MPIDI_OFI_INVALID_MR_KEY, mpi_errno,
                            MPI_ERR_OTHER, "**ofid_mr_key");
    }

    int rc = 0, allrc = 0;
    struct fid_mr *mr = NULL;
    /* device buffers are not currently supported */
    if (MPIR_GPU_query_pointer_is_dev(base))
        rc = -1;
    else {
        MPIDI_OFI_CALL_RETURN(fi_mr_reg(MPIDI_OFI_global.ctx[ctx_idx].domain,   /* In:  Domain Object */
                                        base,   /* In:  Lower memory address */
                                        size,   /* In:  Length              */
                                        FI_REMOTE_READ | FI_REMOTE_WRITE,       /* In:  Expose MR for read  */
                                        0ULL,   /* In:  offset(not used)    */
                                        requested_key,  /* In:  requested key */
                                        0ULL,   /* In:  flags               */
                                        &mr,    /* Out: memregion object    */
                                        NULL), rc);     /* In:  context             */
        mpi_errno = MPIDI_OFI_mr_bind(MPIDI_OFI_global.prov_use[0], mr,
                                      MPIDI_OFI_WIN(win).ep, MPIDI_OFI_WIN(win).cmpl_cntr);
        MPIR_ERR_CHECK(mpi_errno);
    }

    /* Check if any process fails to register. If so, release local MR and force AM path. */
    MPIR_Allreduce(&rc, &allrc, 1, MPI_INT, MPI_MIN, comm_ptr, &errflag);
    if (allrc < 0) {
        if (rc >= 0)
            MPIDI_OFI_CALL(fi_close(&mr->fid), fi_close);
        MPIDI_WIN(win, winattr) &= ~MPIDI_WINATTR_NM_REACHABLE; /* disable native RMA */
        goto fn_exit;
    }

    /* Local MR is searched only at win_detach */
    mpl_err = MPL_gavl_tree_insert(MPIDI_OFI_WIN(win).dwin_mrs, (const void *) base,
                                   (uintptr_t) size, (const void *) mr);
    MPIR_ERR_CHKANDJUMP(mpl_err != MPL_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**mpl_gavl_insert");

    MPIR_CHKLMEM_MALLOC(target_mrs, dwin_target_mr_t *,
                        sizeof(dwin_target_mr_t) * comm_ptr->local_size,
                        mpi_errno, "temp buffer for dynamic win remote memory regions",
                        MPL_MEM_RMA);

    /* Exchange remote MR across all processes because "coll_attach" info ensures
     * that all processes collectively call attach. */
    target_mrs[comm_ptr->rank].mr_key = fi_mr_key(mr);
    target_mrs[comm_ptr->rank].base = (const void *) base;
    target_mrs[comm_ptr->rank].size = (uintptr_t) size;
    mpi_errno = MPIR_Allgather(MPI_IN_PLACE, 0,
                               MPI_DATATYPE_NULL,
                               target_mrs, sizeof(dwin_target_mr_t), MPI_BYTE, comm_ptr, &errflag);
    MPIR_ERR_CHECK(mpi_errno);

    /* Insert each remote MR which will be searched when issuing an RMA operation
     * and deleted at win_detach or win_free */
    for (i = 0; i < comm_ptr->local_size; i++) {
        MPIDI_OFI_target_mr_t *target_mr =
            (MPIDI_OFI_target_mr_t *) MPL_malloc(sizeof(MPIDI_OFI_target_mr_t), MPL_MEM_RMA);
        MPIR_Assert(target_mr);
        /* Store addr only for calculating offset when !MPIDI_OFI_ENABLE_MR_VIRT_ADDRESS */
        target_mr->addr = (uintptr_t) target_mrs[i].base;
        target_mr->mr_key = target_mrs[i].mr_key;

        mpl_err = MPL_gavl_tree_insert(MPIDI_OFI_WIN(win).dwin_target_mrs[i],
                                       target_mrs[i].base, target_mrs[i].size,
                                       (const void *) target_mr);
        MPIR_ERR_CHKANDJUMP(mpl_err != MPL_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**mpl_gavl_insert");
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_OFI_mpi_win_detach_hook(MPIR_Win * win, const void *base)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    MPIR_Comm *comm_ptr = win->comm_ptr;
    const void **target_bases;
    int mpl_err = MPL_SUCCESS, i;

    MPIR_CHKLMEM_DECL(1);

    if (!MPIDI_OFI_ENABLE_RMA || MPIDI_OFI_WIN(win).mr || !MPIDIG_WIN(win, info_args).coll_attach)
        goto fn_exit;

    /* Search and delete local MR. MR may not be found if the registration fails
     * at attach. However, we don't distinguish them for code simplicity. */
    mpl_err = MPL_gavl_tree_delete_start_addr(MPIDI_OFI_WIN(win).dwin_mrs, (const void *) base);
    MPIR_ERR_CHKANDJUMP(mpl_err != MPL_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                        "**mpl_gavl_delete_start_addr");

    /* Notify remote processes to delete their local cached MR key */
    MPIR_CHKLMEM_MALLOC(target_bases, const void **,
                        sizeof(const void *) * comm_ptr->local_size,
                        mpi_errno, "temp buffer for dynamic win remote memory regions",
                        MPL_MEM_RMA);

    /* Exchange remote MR across all processes because "coll_attach" info ensures
     * that all processes collectively call detach. */
    target_bases[comm_ptr->rank] = base;
    mpi_errno = MPIR_Allgather(MPI_IN_PLACE, 0, MPI_DATATYPE_NULL,
                               target_bases, sizeof(const void *), MPI_BYTE, comm_ptr, &errflag);
    MPIR_ERR_CHECK(mpi_errno);

    /* Search and delete each remote MR */
    for (i = 0; i < comm_ptr->local_size; i++) {
        mpl_err = MPL_gavl_tree_delete_start_addr(MPIDI_OFI_WIN(win).dwin_target_mrs[i],
                                                  (const void *) target_bases[i]);
        MPIR_ERR_CHKANDJUMP(mpl_err != MPL_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                            "**mpl_gavl_delete_start_addr");
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_OFI_mpi_win_free_hook(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    int nic = 0;

    MPIR_FUNC_ENTER;

    if (MPIDI_OFI_ENABLE_RMA) {
        int vni = MPIDI_WIN(win, am_vci);
        int ctx_idx = MPIDI_OFI_get_ctx_index(NULL, vni, nic);
        MPIDI_OFI_mr_key_free(MPIDI_OFI_COLL_MR_KEY, MPIDI_OFI_WIN(win).win_id);
        MPIDIU_map_erase(MPIDI_OFI_global.win_map, MPIDI_OFI_WIN(win).win_id);
        /* For scalable EP: push transmit context index back into available pool. */
        if (MPIDI_OFI_WIN(win).sep_tx_idx != -1) {
            utarray_push_back(MPIDI_OFI_global.ctx[ctx_idx].rma_sep_idx_array,
                              &(MPIDI_OFI_WIN(win).sep_tx_idx), MPL_MEM_RMA);
        }
        if (MPIDI_OFI_WIN(win).ep != MPIDI_OFI_global.ctx[ctx_idx].tx)
            MPIDI_OFI_CALL(fi_close(&MPIDI_OFI_WIN(win).ep->fid), epclose);
        /* FIXME: comparing pointer is fragile, ctx[ctx_idx].rma_cmpl_cntr may be a dummy */
        if (MPIDI_OFI_WIN(win).cmpl_cntr != MPIDI_OFI_global.ctx[ctx_idx].rma_cmpl_cntr)
            MPIDI_OFI_CALL(fi_close(&MPIDI_OFI_WIN(win).cmpl_cntr->fid), cntrclose);
        if (MPIDI_OFI_WIN(win).mr)
            MPIDI_OFI_CALL(fi_close(&MPIDI_OFI_WIN(win).mr->fid), mr_unreg);
        MPL_free(MPIDI_OFI_WIN(win).winfo);
        MPIDI_OFI_WIN(win).winfo = NULL;
        MPL_free(MPIDI_OFI_WIN(win).acc_hint);
        MPIDI_OFI_WIN(win).acc_hint = NULL;

        /* Free storage of per-attach memory regions for dynamic window */
        if (win->create_flavor == MPI_WIN_FLAVOR_DYNAMIC &&
            !MPIDI_OFI_WIN(win).mr && MPIDIG_WIN(win, info_args).coll_attach) {
            int i;
            for (i = 0; i < win->comm_ptr->local_size; i++)
                MPL_gavl_tree_destory(MPIDI_OFI_WIN(win).dwin_target_mrs[i]);
            MPL_free(MPIDI_OFI_WIN(win).dwin_target_mrs);
            MPL_gavl_tree_destory(MPIDI_OFI_WIN(win).dwin_mrs);
        }

    }
    /* This hook is called by CH4 generic call before CH4 finalization */

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

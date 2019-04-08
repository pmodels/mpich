/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#include "mpidimpl.h"
#include "ofi_impl.h"
#include "ofi_noinline.h"

static int accu_op_hint_get_index(MPIDIG_win_info_accu_op_shift_t hint_shift);
static void load_acc_hint(MPIR_Win * win);
static void set_rma_fi_info(MPIR_Win * win, struct fi_info *finfo);
static int win_allgather(MPIR_Win * win, void *base, int disp_unit);
static int win_set_per_win_sync(MPIR_Win * win);
static int win_init_sep(MPIR_Win * win);
static int win_init_stx(MPIR_Win * win);
static int win_init_global(MPIR_Win * win);
static int win_init(MPIR_Win * win);

static int accu_op_hint_get_index(MPIDIG_win_info_accu_op_shift_t hint_shift)
{
    int op_index = 0;
    switch (hint_shift) {
        case MPIDIG_ACCU_MAX_SHIFT:
            op_index = MPIDI_OFI_get_mpi_acc_op_index(MPI_MAX);
            break;
        case MPIDIG_ACCU_MIN_SHIFT:
            op_index = MPIDI_OFI_get_mpi_acc_op_index(MPI_MIN);
            break;
        case MPIDIG_ACCU_SUM_SHIFT:
            op_index = MPIDI_OFI_get_mpi_acc_op_index(MPI_SUM);
            break;
        case MPIDIG_ACCU_PROD_SHIFT:
            op_index = MPIDI_OFI_get_mpi_acc_op_index(MPI_PROD);
            break;
        case MPIDIG_ACCU_MAXLOC_SHIFT:
            op_index = MPIDI_OFI_get_mpi_acc_op_index(MPI_MAXLOC);
            break;
        case MPIDIG_ACCU_MINLOC_SHIFT:
            op_index = MPIDI_OFI_get_mpi_acc_op_index(MPI_MINLOC);
            break;
        case MPIDIG_ACCU_BAND_SHIFT:
            op_index = MPIDI_OFI_get_mpi_acc_op_index(MPI_BAND);
            break;
        case MPIDIG_ACCU_BOR_SHIFT:
            op_index = MPIDI_OFI_get_mpi_acc_op_index(MPI_BOR);
            break;
        case MPIDIG_ACCU_BXOR_SHIFT:
            op_index = MPIDI_OFI_get_mpi_acc_op_index(MPI_BXOR);
            break;
        case MPIDIG_ACCU_LAND_SHIFT:
            op_index = MPIDI_OFI_get_mpi_acc_op_index(MPI_LAND);
            break;
        case MPIDIG_ACCU_LOR_SHIFT:
            op_index = MPIDI_OFI_get_mpi_acc_op_index(MPI_LOR);
            break;
        case MPIDIG_ACCU_LXOR_SHIFT:
            op_index = MPIDI_OFI_get_mpi_acc_op_index(MPI_LXOR);
            break;
        case MPIDIG_ACCU_REPLACE_SHIFT:
            op_index = MPIDI_OFI_get_mpi_acc_op_index(MPI_REPLACE);
            break;
        case MPIDIG_ACCU_NO_OP_SHIFT:
            op_index = MPIDI_OFI_get_mpi_acc_op_index(MPI_NO_OP);
            break;
        case MPIDIG_ACCU_CSWAP_SHIFT:
            op_index = MPIDI_OFI_get_mpi_acc_op_index(MPI_OP_NULL);
            break;
        default:
            MPIR_Assert(hint_shift < MPIDIG_ACCU_OP_SHIFT_LAST);
            break;
    }
    return op_index;
}

#undef FUNCNAME
#define FUNCNAME load_acc_hint
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static void load_acc_hint(MPIR_Win * win)
{
    int op_index = 0, i;
    MPIDIG_win_info_accu_op_shift_t hint_shift = MPIDIG_ACCU_OP_SHIFT_FIRST;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_WIN_LOAD_ATOMIC_INFO);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_WIN_LOAD_ATOMIC_INFO);

    if (!MPIDI_OFI_WIN(win).acc_hint)
        MPIDI_OFI_WIN(win).acc_hint = MPL_malloc(sizeof(MPIDI_OFI_win_acc_hint_t), MPL_MEM_RMA);

    /* TODO: we assume all pes pass the same hint. Allreduce is needed to check
     * the maximal value or the spec must define it as same value on all pes.
     * Now we assume the spec requries same value on all pes. */

    /* We translate the atomic op hints to max count allowed for all possible atomics with each
     * datatype. We do not need more specific info (e.g., <datatype, op>, because any process may use
     * the op with accumulate or get_accumulate.*/
    for (i = 0; i < MPIDI_OFI_DT_SIZES; i++) {
        MPIDI_OFI_WIN(win).acc_hint->dtypes_max_count[i] = 0;
        bool first_valid_op = true;

        for (hint_shift = MPIDIG_ACCU_OP_SHIFT_FIRST; hint_shift < MPIDIG_ACCU_OP_SHIFT_LAST;
             hint_shift++) {
            uint64_t max_count = 0;
            /* Calculate the max count of all possible atomics if this op is enabled.
             * If the op is disabled for the datatype, the max counts are set to 0 (see util.c).*/
            if (MPIDIG_WIN(win, info_args).which_accumulate_ops & (1 << hint_shift)) {
                op_index = accu_op_hint_get_index(hint_shift);

                /* Invalid <datatype, op> pairs should be excluded as it is never used in a
                 * correct program (e.g., <double, MAXLOC>).*/
                if (!MPIDI_OFI_global.win_op_table[i][op_index].mpi_acc_valid)
                    continue;

                if (hint_shift == MPIDIG_ACCU_NO_OP_SHIFT)      /* atomic get */
                    max_count = MPIDI_OFI_global.win_op_table[i][op_index].max_fetch_atomic_count;
                else if (hint_shift == MPIDIG_ACCU_CSWAP_SHIFT) /* compare and swap */
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

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_WIN_LOAD_ATOMIC_INFO);
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
        finfo->tx_attr->msg_order |= FI_ORDER_RAR;
    if ((MPIDIG_WIN(win, info_args).accumulate_ordering & MPIDIG_ACCU_ORDER_RAW) ==
        MPIDIG_ACCU_ORDER_RAW)
        finfo->tx_attr->msg_order |= FI_ORDER_RAW;
    if ((MPIDIG_WIN(win, info_args).accumulate_ordering & MPIDIG_ACCU_ORDER_WAR) ==
        MPIDIG_ACCU_ORDER_WAR)
        finfo->tx_attr->msg_order |= FI_ORDER_WAR;
    if ((MPIDIG_WIN(win, info_args).accumulate_ordering & MPIDIG_ACCU_ORDER_WAW) ==
        MPIDIG_ACCU_ORDER_WAW)
        finfo->tx_attr->msg_order |= FI_ORDER_WAW;
}

#undef FUNCNAME
#define FUNCNAME win_allgather
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int win_allgather(MPIR_Win * win, void *base, int disp_unit)
{
    int i, same_disp, mpi_errno = MPI_SUCCESS;
    uint32_t first;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    MPIR_Comm *comm_ptr = win->comm_ptr;
    MPIDI_OFI_win_targetinfo_t *winfo;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_WIN_ALLGATHER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_WIN_ALLGATHER);

    if (MPIDI_OFI_ENABLE_MR_SCALABLE) {
        MPIDI_OFI_WIN(win).mr_key = MPIDI_OFI_WIN(win).win_id;
    } else {
        MPIDI_OFI_WIN(win).mr_key = 0;
    }

    /* Don't register MR for NULL buffer, because FI_MR_BASIC mode requires
     * that all registered memory regions must be backed by physical memory
     * pages at the time the registration call is made. */
    if (MPIDI_OFI_ENABLE_MR_SCALABLE || base) {
        MPIDI_OFI_CALL(fi_mr_reg(MPIDI_OFI_global.domain,       /* In:  Domain Object       */
                                 base,  /* In:  Lower memory address */
                                 win->size,     /* In:  Length              */
                                 FI_REMOTE_READ | FI_REMOTE_WRITE,      /* In:  Expose MR for read  */
                                 0ULL,  /* In:  offset(not used)    */
                                 MPIDI_OFI_WIN(win).mr_key,     /* In:  requested key       */
                                 0ULL,  /* In:  flags               */
                                 &MPIDI_OFI_WIN(win).mr,        /* Out: memregion object    */
                                 NULL), mr_reg);        /* In:  context             */
    }
    MPIDI_OFI_WIN(win).winfo = MPL_malloc(sizeof(*winfo) * comm_ptr->local_size, MPL_MEM_RMA);

    winfo = MPIDI_OFI_WIN(win).winfo;
    winfo[comm_ptr->rank].disp_unit = disp_unit;

    if (!MPIDI_OFI_ENABLE_MR_SCALABLE && MPIDI_OFI_WIN(win).mr) {
        /* MR_BASIC */
        MPIDI_OFI_WIN(win).mr_key = fi_mr_key(MPIDI_OFI_WIN(win).mr);
        winfo[comm_ptr->rank].mr_key = MPIDI_OFI_WIN(win).mr_key;
        winfo[comm_ptr->rank].base = (uintptr_t) base;
    }

    mpi_errno = MPIR_Allgather(MPI_IN_PLACE, 0,
                               MPI_DATATYPE_NULL,
                               winfo, sizeof(*winfo), MPI_BYTE, comm_ptr, &errflag);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    if (MPIDI_OFI_ENABLE_MR_SCALABLE) {
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
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_WIN_ALLGATHER);
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
#undef FUNCNAME
#define FUNCNAME win_set_per_win_sync
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int win_set_per_win_sync(MPIR_Win * win)
{
    int ret, mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_WIN_SET_PER_WIN_SYNC);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_WIN_SET_PER_WIN_SYNC);

    struct fi_cntr_attr cntr_attr;
    memset(&cntr_attr, 0, sizeof(cntr_attr));
    cntr_attr.events = FI_CNTR_EVENTS_COMP;
    cntr_attr.wait_obj = FI_WAIT_UNSPEC;
    MPIDI_OFI_CALL_RETURN(fi_cntr_open(MPIDI_OFI_global.domain, /* In:  Domain Object        */
                                       &cntr_attr,      /* In:  Configuration object */
                                       &MPIDI_OFI_WIN(win).cmpl_cntr,   /* Out: Counter Object       */
                                       NULL), ret);     /* Context: counter events   */
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
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_WIN_SET_PER_WIN_SYNC);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
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
#undef FUNCNAME
#define FUNCNAME win_init_sep
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int win_init_sep(MPIR_Win * win)
{
    int i, ret, mpi_errno = MPI_SUCCESS;
    struct fi_info *finfo;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_WIN_INIT_SEP);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_WIN_INIT_SEP);

    finfo = fi_dupinfo(MPIDI_OFI_global.prov_use);
    MPIR_Assert(finfo);

    /* Initialize scalable EP when first window is created. */
    if (MPIDI_OFI_global.rma_sep == NULL) {
        /* Specify number of transmit context according user input and provider limit. */
        MPIDI_OFI_global.max_rma_sep_tx_cnt =
            MPL_MIN(MPIDI_OFI_global.prov_use->domain_attr->max_ep_tx_ctx,
                    MPIR_CVAR_CH4_OFI_MAX_RMA_SEP_CTX);
        finfo->ep_attr->tx_ctx_cnt = MPIDI_OFI_global.max_rma_sep_tx_cnt;

        MPIDI_OFI_CALL_RETURN(fi_scalable_ep
                              (MPIDI_OFI_global.domain, finfo, &MPIDI_OFI_global.rma_sep, NULL),
                              ret);
        if (ret < 0) {
            MPL_DBG_MSG(MPIDI_CH4_DBG_GENERAL, VERBOSE, "Failed to create scalable endpoint.\n");
            MPIDI_OFI_global.rma_sep = NULL;
            mpi_errno = MPIDI_OFI_ENAVAIL;
            goto fn_fail;
        }

        MPIDI_OFI_CALL_RETURN(fi_scalable_ep_bind
                              (MPIDI_OFI_global.rma_sep, &(MPIDI_OFI_global.av->fid), 0), ret);
        if (ret < 0) {
            MPL_DBG_MSG(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                        "Failed to bind scalable endpoint to address vector.\n");
            MPIDI_OFI_global.rma_sep = NULL;
            mpi_errno = MPIDI_OFI_ENAVAIL;
            goto fn_fail;
        }

        /* Allocate and initilize tx index array. */
        utarray_new(MPIDI_OFI_global.rma_sep_idx_array, &ut_int_icd, MPL_MEM_RMA);
        for (i = 0; i < MPIDI_OFI_global.max_rma_sep_tx_cnt; i++) {
            utarray_push_back(MPIDI_OFI_global.rma_sep_idx_array, &i, MPL_MEM_RMA);
        }
    }
    /* Set per window transmit attributes. */
    set_rma_fi_info(win, finfo);
    /* Get available transmit context index. */
    int *index = (int *) utarray_back(MPIDI_OFI_global.rma_sep_idx_array);
    if (index == NULL) {
        mpi_errno = MPIDI_OFI_ENAVAIL;
        goto fn_fail;
    }
    /* Retrieve transmit context on scalable EP. */
    MPIDI_OFI_CALL_RETURN(fi_tx_context
                          (MPIDI_OFI_global.rma_sep, *index, finfo->tx_attr,
                           &(MPIDI_OFI_WIN(win).ep), NULL), ret);
    if (ret < 0) {
        MPL_DBG_MSG(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                    "Failed to retrieve transmit context from scalable endpoint.\n");
        mpi_errno = MPIDI_OFI_ENAVAIL;
        goto fn_fail;
    }

    MPIDI_OFI_WIN(win).sep_tx_idx = *index;
    /* Pop this index out of reserving array. */
    utarray_pop_back(MPIDI_OFI_global.rma_sep_idx_array);

    MPIDI_OFI_CALL_RETURN(fi_ep_bind(MPIDI_OFI_WIN(win).ep,
                                     &MPIDI_OFI_global.ctx[0].cq->fid,
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
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_WIN_INIT_SEP);
    return mpi_errno;
  fn_fail:
    if (MPIDI_OFI_WIN(win).sep_tx_idx != -1) {
        /* Push tx idx back into available pool. */
        utarray_push_back(MPIDI_OFI_global.rma_sep_idx_array, &MPIDI_OFI_WIN(win).sep_tx_idx,
                          MPL_MEM_RMA);
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
#undef FUNCNAME
#define FUNCNAME win_init_stx
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int win_init_stx(MPIR_Win * win)
{
    /* Activate per-window EP/counter using STX */
    int ret, mpi_errno = MPI_SUCCESS;
    struct fi_info *finfo;
    bool have_per_win_cntr = false;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_WIN_INIT_STX);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_WIN_INIT_STX);

    finfo = fi_dupinfo(MPIDI_OFI_global.prov_use);
    MPIR_Assert(finfo);

    /* Set per window transmit attributes. */
    set_rma_fi_info(win, finfo);
    /* Still need to take out rx capabilities for shared context. */
    finfo->rx_attr->caps = 0ULL;        /* RX capabilities not needed */

    finfo->ep_attr->tx_ctx_cnt = FI_SHARED_CONTEXT;     /* Request a shared context */
    finfo->ep_attr->rx_ctx_cnt = 0;     /* We don't need RX contexts */
    MPIDI_OFI_CALL_RETURN(fi_endpoint(MPIDI_OFI_global.domain,
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
        MPIDI_OFI_CALL_RETURN(fi_ep_bind
                              (MPIDI_OFI_WIN(win).ep, &MPIDI_OFI_global.rma_stx_ctx->fid, 0), ret);
        if (ret < 0) {
            MPL_DBG_MSG(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                        "Failed to bind endpoint to shared transmit contxt.\n");
            mpi_errno = MPIDI_OFI_EPERROR;
            goto fn_fail;
        }

        MPIDI_OFI_CALL_RETURN(fi_ep_bind(MPIDI_OFI_WIN(win).ep,
                                         &MPIDI_OFI_global.ctx[0].cq->fid,
                                         FI_TRANSMIT | FI_SELECTIVE_COMPLETION), ret);
        if (ret < 0) {
            MPL_DBG_MSG(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                        "Failed to bind endpoint to completion queue.\n");
            mpi_errno = MPIDI_OFI_EPERROR;
            goto fn_fail;
        }

        MPIDI_OFI_CALL_RETURN(fi_ep_bind(MPIDI_OFI_WIN(win).ep, &MPIDI_OFI_global.av->fid, 0), ret);
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
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_WIN_INIT_STX);
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

#undef FUNCNAME
#define FUNCNAME win_init_global
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int win_init_global(MPIR_Win * win)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_WIN_INIT_GLOBAL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_WIN_INIT_GLOBAL);

    MPIDI_OFI_WIN(win).ep = MPIDI_OFI_global.ctx[0].tx;
    MPIDI_OFI_WIN(win).cmpl_cntr = MPIDI_OFI_global.rma_cmpl_cntr;
    MPIDI_OFI_WIN(win).issued_cntr = &MPIDI_OFI_global.rma_issued_cntr;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_WIN_INIT_GLOBAL);

    return 0;
}

#undef FUNCNAME
#define FUNCNAME win_init
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int win_init(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_WIN_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_WIN_INIT);

    MPIR_Datatype_init_names();
    MPIDI_OFI_index_datatypes();

    MPL_COMPILE_TIME_ASSERT(sizeof(MPIDI_Devwin_t) >= sizeof(MPIDI_OFI_win_t));
    MPL_COMPILE_TIME_ASSERT(sizeof(MPIDI_Devdt_t) >= sizeof(MPIDI_OFI_datatype_t));

    memset(&MPIDI_OFI_WIN(win), 0, sizeof(MPIDI_OFI_win_t));

    MPIDI_OFI_WIN(win).win_id = MPIDI_OFI_mr_key_alloc();

    MPIDIU_map_set(MPIDI_OFI_global.win_map, MPIDI_OFI_WIN(win).win_id, win, MPL_MEM_RMA);

    MPIDI_OFI_WIN(win).sep_tx_idx = -1; /* By default, -1 means not using scalable EP. */

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
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_WIN_INIT);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_mpi_win_set_info
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_OFI_mpi_win_set_info(MPIR_Win * win, MPIR_Info * info)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_MPI_WIN_SET_INFO);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_MPI_WIN_SET_INFO);

    mpi_errno = MPIDIG_mpi_win_set_info(win, info);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_MPI_WIN_SET_INFO);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_mpi_win_get_info
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_OFI_mpi_win_get_info(MPIR_Win * win, MPIR_Info ** info_p_p)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_MPI_WIN_GET_INFO);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_MPI_WIN_GET_INFO);

    mpi_errno = MPIDIG_mpi_win_get_info(win, info_p_p);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_MPI_WIN_GET_INFO);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_mpi_win_free
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_OFI_mpi_win_free(MPIR_Win ** win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_MPI_WIN_FREE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_MPI_WIN_FREE);

    mpi_errno = MPIDIG_mpi_win_free(win_ptr);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_MPI_WIN_FREE);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_mpi_win_create
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_OFI_mpi_win_create(void *base, MPI_Aint length, int disp_unit, MPIR_Info * info,
                             MPIR_Comm * comm_ptr, MPIR_Win ** win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_MPI_WIN_CREATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_MPI_WIN_CREATE);

    mpi_errno = MPIDIG_mpi_win_create(base, length, disp_unit, info, comm_ptr, win_ptr);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_MPI_WIN_CREATE);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_mpi_win_attach
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_OFI_mpi_win_attach(MPIR_Win * win, void *base, MPI_Aint size)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_MPI_WIN_ATTACH);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_MPI_WIN_ATTACH);

    mpi_errno = MPIDIG_mpi_win_attach(win, base, size);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_MPI_WIN_ATTACH);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_mpi_win_allocate_shared
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_OFI_mpi_win_allocate_shared(MPI_Aint size, int disp_unit, MPIR_Info * info_ptr,
                                      MPIR_Comm * comm_ptr, void **base_ptr, MPIR_Win ** win_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_MPI_WIN_ALLOCATE_SHARED);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_MPI_WIN_ALLOCATE_SHARED);

    mpi_errno = MPIDIG_mpi_win_allocate_shared(size, disp_unit, info_ptr, comm_ptr, base_ptr,
                                               win_ptr);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_MPI_WIN_ALLOCATE_SHARED);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_mpi_win_detach
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_OFI_mpi_win_detach(MPIR_Win * win, const void *base)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_MPI_WIN_DETACH);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_MPI_WIN_DETACH);

    mpi_errno = MPIDIG_mpi_win_detach(win, base);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_MPI_WIN_DETACH);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_mpi_win_allocate
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_OFI_mpi_win_allocate(MPI_Aint size, int disp_unit, MPIR_Info * info, MPIR_Comm * comm,
                               void *baseptr, MPIR_Win ** win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_MPI_WIN_ALLOCATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_MPI_WIN_ALLOCATE);

    mpi_errno = MPIDIG_mpi_win_allocate(size, disp_unit, info, comm, baseptr, win_ptr);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_MPI_WIN_ALLOCATE);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_mpi_win_create_dynamic
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_OFI_mpi_win_create_dynamic(MPIR_Info * info, MPIR_Comm * comm, MPIR_Win ** win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_MPI_WIN_CREATE_DYNAMIC);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_MPI_WIN_CREATE_DYNAMIC);

    mpi_errno = MPIDIG_mpi_win_create_dynamic(info, comm, win_ptr);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_MPI_WIN_CREATE_DYNAMIC);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_mpi_win_create_hook
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_OFI_mpi_win_create_hook(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_MPI_WIN_CREATE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_MPI_WIN_CREATE_HOOK);

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
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_MPI_WIN_CREATE_HOOK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_mpi_win_allocate_hook
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_OFI_mpi_win_allocate_hook(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_MPI_WIN_ALLOCATE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_MPI_WIN_ALLOCATE_HOOK);

    /* This hook is called by CH4 generic call after CH4 initialization */
    if (MPIDI_OFI_ENABLE_RMA) {
        mpi_errno = win_init(win);
        if (mpi_errno != MPI_SUCCESS)
            MPIR_ERR_POP(mpi_errno);

        mpi_errno = win_allgather(win, win->base, win->disp_unit);
        if (mpi_errno != MPI_SUCCESS)
            MPIR_ERR_POP(mpi_errno);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_MPI_WIN_ALLOCATE_HOOK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_mpi_win_allocate_shared_hook
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_OFI_mpi_win_allocate_shared_hook(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_MPI_WIN_ALLOCATE_SHARED_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_MPI_WIN_ALLOCATE_SHARED_HOOK);

    /* This hook is called by CH4 generic call after CH4 initialization */
    if (MPIDI_OFI_ENABLE_RMA) {
        mpi_errno = win_init(win);
        if (mpi_errno != MPI_SUCCESS)
            MPIR_ERR_POP(mpi_errno);

        mpi_errno = win_allgather(win, win->base, win->disp_unit);
        if (mpi_errno != MPI_SUCCESS)
            MPIR_ERR_POP(mpi_errno);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_MPI_WIN_ALLOCATE_SHARED_HOOK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_mpi_win_create_dynamic_hook
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_OFI_mpi_win_create_dynamic_hook(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_MPI_WIN_CREATE_DYNAMIC_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_MPI_WIN_CREATE_DYNAMIC_HOOK);

    /* This hook is called by CH4 generic call after CH4 initialization */
    if (MPIDI_OFI_ENABLE_RMA) {
        /* FIXME: should the OFI specific size be stored inside OFI rather
         * than overwriting CH4 info ? Now we simply followed original
         * create_dynamic routine.*/
        win->size = (uintptr_t) UINTPTR_MAX - (uintptr_t) MPI_BOTTOM;

        mpi_errno = win_init(win);
        if (mpi_errno != MPI_SUCCESS)
            MPIR_ERR_POP(mpi_errno);

        mpi_errno = win_allgather(win, win->base, win->disp_unit);
        if (mpi_errno != MPI_SUCCESS)
            MPIR_ERR_POP(mpi_errno);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_MPI_WIN_CREATE_DYNAMIC_HOOK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_mpi_win_attach_hook
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_OFI_mpi_win_attach_hook(MPIR_Win * win, void *base, MPI_Aint size)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_MPI_WIN_ATTACH_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_MPI_WIN_ATTACH_HOOK);

    /* Do nothing */

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_MPI_WIN_ATTACH_HOOK);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_mpi_win_detach_hook
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_OFI_mpi_win_detach_hook(MPIR_Win * win, const void *base)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_MPI_WIN_DETACH_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_MPI_WIN_DETACH_HOOK);

    /* Do nothing */

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_MPI_WIN_DETACH_HOOK);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_mpi_win_free_hook
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_OFI_mpi_win_free_hook(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_MPI_WIN_FREE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_MPI_WIN_FREE_HOOK);

    if (MPIDI_OFI_ENABLE_RMA) {
        MPIDI_OFI_mr_key_free(MPIDI_OFI_WIN(win).win_id);
        MPIDIU_map_erase(MPIDI_OFI_global.win_map, MPIDI_OFI_WIN(win).win_id);
        /* For scalable EP: push transmit context index back into available pool. */
        if (MPIDI_OFI_WIN(win).sep_tx_idx != -1) {
            utarray_push_back(MPIDI_OFI_global.rma_sep_idx_array, &(MPIDI_OFI_WIN(win).sep_tx_idx),
                              MPL_MEM_RMA);
        }
        if (MPIDI_OFI_WIN(win).ep != MPIDI_OFI_global.ctx[0].tx)
            MPIDI_OFI_CALL(fi_close(&MPIDI_OFI_WIN(win).ep->fid), epclose);
        if (MPIDI_OFI_WIN(win).cmpl_cntr != MPIDI_OFI_global.rma_cmpl_cntr)
            MPIDI_OFI_CALL(fi_close(&MPIDI_OFI_WIN(win).cmpl_cntr->fid), cntrclose);
        if (MPIDI_OFI_WIN(win).mr)
            MPIDI_OFI_CALL(fi_close(&MPIDI_OFI_WIN(win).mr->fid), mr_unreg);
        MPL_free(MPIDI_OFI_WIN(win).winfo);
        MPIDI_OFI_WIN(win).winfo = NULL;
        MPL_free(MPIDI_OFI_WIN(win).acc_hint);
        MPIDI_OFI_WIN(win).acc_hint = NULL;
    }
    /* This hook is called by CH4 generic call before CH4 finalization */

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_MPI_WIN_FREE_HOOK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

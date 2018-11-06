/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef OFI_WIN_H_INCLUDED
#define OFI_WIN_H_INCLUDED

#include "ofi_impl.h"
#include <opa_primitives.h>

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_accu_op_hint_get_index(MPIDI_CH4U_win_info_accu_op_shift_t
                                                              hint_shift)
{
    int op_index = 0;
    switch (hint_shift) {
        case MPIDI_CH4I_ACCU_MAX_SHIFT:
            MPIDI_OFI_MPI_ACCU_OP_INDEX(MPI_MAX, op_index);
            break;
        case MPIDI_CH4I_ACCU_MIN_SHIFT:
            MPIDI_OFI_MPI_ACCU_OP_INDEX(MPI_MIN, op_index);
            break;
        case MPIDI_CH4I_ACCU_SUM_SHIFT:
            MPIDI_OFI_MPI_ACCU_OP_INDEX(MPI_SUM, op_index);
            break;
        case MPIDI_CH4I_ACCU_PROD_SHIFT:
            MPIDI_OFI_MPI_ACCU_OP_INDEX(MPI_PROD, op_index);
            break;
        case MPIDI_CH4I_ACCU_MAXLOC_SHIFT:
            MPIDI_OFI_MPI_ACCU_OP_INDEX(MPI_MAXLOC, op_index);
            break;
        case MPIDI_CH4I_ACCU_MINLOC_SHIFT:
            MPIDI_OFI_MPI_ACCU_OP_INDEX(MPI_MINLOC, op_index);
            break;
        case MPIDI_CH4I_ACCU_BAND_SHIFT:
            MPIDI_OFI_MPI_ACCU_OP_INDEX(MPI_BAND, op_index);
            break;
        case MPIDI_CH4I_ACCU_BOR_SHIFT:
            MPIDI_OFI_MPI_ACCU_OP_INDEX(MPI_BOR, op_index);
            break;
        case MPIDI_CH4I_ACCU_BXOR_SHIFT:
            MPIDI_OFI_MPI_ACCU_OP_INDEX(MPI_BXOR, op_index);
            break;
        case MPIDI_CH4I_ACCU_LAND_SHIFT:
            MPIDI_OFI_MPI_ACCU_OP_INDEX(MPI_LAND, op_index);
            break;
        case MPIDI_CH4I_ACCU_LOR_SHIFT:
            MPIDI_OFI_MPI_ACCU_OP_INDEX(MPI_LOR, op_index);
            break;
        case MPIDI_CH4I_ACCU_LXOR_SHIFT:
            MPIDI_OFI_MPI_ACCU_OP_INDEX(MPI_LXOR, op_index);
            break;
        case MPIDI_CH4I_ACCU_REPLACE_SHIFT:
            MPIDI_OFI_MPI_ACCU_OP_INDEX(MPI_REPLACE, op_index);
            break;
        case MPIDI_CH4I_ACCU_NO_OP_SHIFT:
            MPIDI_OFI_MPI_ACCU_OP_INDEX(MPI_NO_OP, op_index);
            break;
        case MPIDI_CH4I_ACCU_CSWAP_SHIFT:
            MPIDI_OFI_MPI_ACCU_OP_INDEX(MPI_OP_NULL, op_index);
            break;
        default:
            MPIR_Assert(hint_shift < MPIDI_CH4I_ACCU_OP_SHIFT_LAST);
            break;
    }
    return op_index;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_load_acc_hint
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX void MPIDI_OFI_load_acc_hint(MPIR_Win * win)
{
    int op_index = 0, i;
    MPIDI_CH4U_win_info_accu_op_shift_t hint_shift = 0;

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

        for (hint_shift = 0; hint_shift < MPIDI_CH4I_ACCU_OP_SHIFT_LAST; hint_shift++) {
            uint64_t max_count = 0;
            /* Calculate the max count of all possible atomics if this op is enabled.
             * If the op is disabled for the datatype, the max counts are set to 0 (see util.c).*/
            if (MPIDI_CH4U_WIN(win, info_args).which_accumulate_ops & (1 << hint_shift)) {
                op_index = MPIDI_OFI_accu_op_hint_get_index(hint_shift);

                /* Invalid <datatype, op> pairs should be excluded as it is never used in a
                 * correct program (e.g., <double, MAXLOC>).*/
                if (!MPIDI_Global.win_op_table[i][op_index].mpi_acc_valid)
                    continue;

                if (hint_shift == MPIDI_CH4I_ACCU_NO_OP_SHIFT)  /* atomic get */
                    max_count = MPIDI_Global.win_op_table[i][op_index].max_fetch_atomic_count;
                else if (hint_shift == MPIDI_CH4I_ACCU_CSWAP_SHIFT)     /* compare and swap */
                    max_count = MPIDI_Global.win_op_table[i][op_index].max_compare_atomic_count;
                else    /* atomic write and fetch_and_write */
                    max_count = MPL_MIN(MPIDI_Global.win_op_table[i][op_index].max_atomic_count,
                                        MPIDI_Global.
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

#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_win_allgather
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_OFI_win_allgather(MPIR_Win * win, void *base, int disp_unit)
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
        MPIDI_OFI_CALL(fi_mr_reg(MPIDI_Global.domain,   /* In:  Domain Object       */
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

    MPIDI_OFI_load_acc_hint(win);

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
#define FUNCNAME MPIDI_OFI_win_set_per_win_sync
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_win_set_per_win_sync(MPIR_Win * win)
{
    int ret, mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_WIN_SET_PER_WIN_SYNC);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_WIN_SET_PER_WIN_SYNC);

    struct fi_cntr_attr cntr_attr;
    memset(&cntr_attr, 0, sizeof(cntr_attr));
    cntr_attr.events = FI_CNTR_EVENTS_COMP;
    cntr_attr.wait_obj = FI_WAIT_UNSPEC;
    MPIDI_OFI_CALL_RETURN(fi_cntr_open(MPIDI_Global.domain,     /* In:  Domain Object        */
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
#define FUNCNAME MPIDI_OFI_win_init_sep
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_win_init_sep(MPIR_Win * win)
{
    int i, ret, mpi_errno = MPI_SUCCESS;
    struct fi_info *finfo;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_WIN_INIT_SEP);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_WIN_INIT_SEP);

    finfo = fi_dupinfo(MPIDI_Global.prov_use);
    MPIR_Assert(finfo);

    /* Initialize scalable EP when first window is created. */
    if (MPIDI_Global.rma_sep == NULL) {
        /* Specify number of transmit context according user input and provider limit. */
        MPIDI_Global.max_rma_sep_tx_cnt = MPL_MIN(MPIDI_Global.prov_use->domain_attr->max_ep_tx_ctx,
                                                  MPIR_CVAR_CH4_OFI_MAX_RMA_SEP_CTX);
        finfo->ep_attr->tx_ctx_cnt = MPIDI_Global.max_rma_sep_tx_cnt;

        MPIDI_OFI_CALL_RETURN(fi_scalable_ep
                              (MPIDI_Global.domain, finfo, &MPIDI_Global.rma_sep, NULL), ret);
        if (ret < 0) {
            MPL_DBG_MSG(MPIDI_CH4_DBG_GENERAL, VERBOSE, "Failed to create scalable endpoint.\n");
            MPIDI_Global.rma_sep = NULL;
            mpi_errno = MPIDI_OFI_ENAVAIL;
            goto fn_fail;
        }

        MPIDI_OFI_CALL_RETURN(fi_scalable_ep_bind(MPIDI_Global.rma_sep, &(MPIDI_Global.av->fid), 0),
                              ret);
        if (ret < 0) {
            MPL_DBG_MSG(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                        "Failed to bind scalable endpoint to address vector.\n");
            MPIDI_Global.rma_sep = NULL;
            mpi_errno = MPIDI_OFI_ENAVAIL;
            goto fn_fail;
        }

        /* Allocate and initilize tx index array. */
        utarray_new(MPIDI_Global.rma_sep_idx_array, &ut_int_icd, MPL_MEM_RMA);
        for (i = 0; i < MPIDI_Global.max_rma_sep_tx_cnt; i++) {
            utarray_push_back(MPIDI_Global.rma_sep_idx_array, &i, MPL_MEM_RMA);
        }
    }
    /* Set per window transmit attributes. */
    MPIDI_OFI_set_rma_fi_info(win, finfo);
    /* Get available transmit context index. */
    int *index = (int *) utarray_back(MPIDI_Global.rma_sep_idx_array);
    if (index == NULL) {
        mpi_errno = MPIDI_OFI_ENAVAIL;
        goto fn_fail;
    }
    /* Retrieve transmit context on scalable EP. */
    MPIDI_OFI_CALL_RETURN(fi_tx_context
                          (MPIDI_Global.rma_sep, *index, finfo->tx_attr, &(MPIDI_OFI_WIN(win).ep),
                           NULL), ret);
    if (ret < 0) {
        MPL_DBG_MSG(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                    "Failed to retrieve transmit context from scalable endpoint.\n");
        mpi_errno = MPIDI_OFI_ENAVAIL;
        goto fn_fail;
    }

    MPIDI_OFI_WIN(win).sep_tx_idx = *index;
    /* Pop this index out of reserving array. */
    utarray_pop_back(MPIDI_Global.rma_sep_idx_array);

    MPIDI_OFI_CALL_RETURN(fi_ep_bind(MPIDI_OFI_WIN(win).ep,
                                     &MPIDI_Global.ctx[0].cq->fid,
                                     FI_TRANSMIT | FI_SELECTIVE_COMPLETION), ret);
    if (ret < 0) {
        MPL_DBG_MSG(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                    "Failed to bind endpoint to completion queue.\n");
        mpi_errno = MPIDI_OFI_EPERROR;
        goto fn_fail;
    }

    if (MPIDI_OFI_win_set_per_win_sync(win) == MPI_SUCCESS) {
        MPIDI_OFI_CALL_RETURN(fi_enable(MPIDI_OFI_WIN(win).ep), ret);
        if (ret < 0) {
            MPL_DBG_MSG(MPIDI_CH4_DBG_GENERAL, VERBOSE, "Failed to activate endpoint.\n");
            mpi_errno = MPIDI_OFI_EPERROR;
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
        utarray_push_back(MPIDI_Global.rma_sep_idx_array, &MPIDI_OFI_WIN(win).sep_tx_idx,
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
#define FUNCNAME MPIDI_OFI_win_init_stx
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_OFI_win_init_stx(MPIR_Win * win)
{
    /* Activate per-window EP/counter using STX */
    int ret, mpi_errno = MPI_SUCCESS;
    struct fi_info *finfo;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_WIN_INIT_STX);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_WIN_INIT_STX);

    finfo = fi_dupinfo(MPIDI_Global.prov_use);
    MPIR_Assert(finfo);

    /* Set per window transmit attributes. */
    MPIDI_OFI_set_rma_fi_info(win, finfo);
    /* Still need to take out rx capabilities for shared context. */
    finfo->rx_attr->caps = 0ULL;        /* RX capabilities not needed */

    finfo->ep_attr->tx_ctx_cnt = FI_SHARED_CONTEXT;     /* Request a shared context */
    finfo->ep_attr->rx_ctx_cnt = 0;     /* We don't need RX contexts */
    MPIDI_OFI_CALL_RETURN(fi_endpoint(MPIDI_Global.domain,
                                      finfo, &MPIDI_OFI_WIN(win).ep, NULL), ret);
    if (ret < 0) {
        MPL_DBG_MSG(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                    "Failed to create per-window EP using shared TX context, "
                    "falling back to global EP/counter scheme");
        mpi_errno = MPIDI_OFI_ENAVAIL;
        goto fn_fail;
    }

    if (MPIDI_OFI_win_set_per_win_sync(win) == MPI_SUCCESS) {
        MPIDI_OFI_CALL_RETURN(fi_ep_bind(MPIDI_OFI_WIN(win).ep, &MPIDI_Global.rma_stx_ctx->fid, 0),
                              ret);
        if (ret < 0) {
            MPL_DBG_MSG(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                        "Failed to bind endpoint to shared transmit contxt.\n");
            mpi_errno = MPIDI_OFI_EPERROR;
            goto fn_fail;
        }

        MPIDI_OFI_CALL_RETURN(fi_ep_bind(MPIDI_OFI_WIN(win).ep,
                                         &MPIDI_Global.ctx[0].cq->fid,
                                         FI_TRANSMIT | FI_SELECTIVE_COMPLETION), ret);
        if (ret < 0) {
            MPL_DBG_MSG(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                        "Failed to bind endpoint to completion queue.\n");
            mpi_errno = MPIDI_OFI_EPERROR;
            goto fn_fail;
        }

        MPIDI_OFI_CALL_RETURN(fi_ep_bind(MPIDI_OFI_WIN(win).ep, &MPIDI_Global.av->fid, 0), ret);
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
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_win_init_global
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_OFI_win_init_global(MPIR_Win * win)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_WIN_INIT_GLOBAL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_WIN_INIT_GLOBAL);

    MPIDI_OFI_WIN(win).ep = MPIDI_Global.ctx[0].tx;
    MPIDI_OFI_WIN(win).cmpl_cntr = MPIDI_Global.rma_cmpl_cntr;
    MPIDI_OFI_WIN(win).issued_cntr = &MPIDI_Global.rma_issued_cntr;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_WIN_INIT_GLOBAL);

    return 0;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_win_init
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_win_init(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    uint64_t window_instance, max_contexts_allowed;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_WIN_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_WIN_INIT);

    MPIR_Datatype_init_names();
    MPIDI_OFI_index_datatypes();

    CH4_COMPILE_TIME_ASSERT(sizeof(MPIDI_Devwin_t) >= sizeof(MPIDI_OFI_win_t));
    CH4_COMPILE_TIME_ASSERT(sizeof(MPIDI_Devdt_t) >= sizeof(MPIDI_OFI_datatype_t));

    memset(&MPIDI_OFI_WIN(win), 0, sizeof(MPIDI_OFI_win_t));

    /* context id lower bits, window instance upper bits */
    window_instance =
        MPIDI_OFI_index_allocator_alloc(MPIDI_OFI_COMM(win->comm_ptr).win_id_allocator,
                                        MPL_MEM_RMA);
    MPIR_ERR_CHKANDSTMT(window_instance >= MPIDI_Global.max_huge_rmas, mpi_errno,
                        MPI_ERR_OTHER, goto fn_fail, "**ofid_mr_reg");

    max_contexts_allowed =
        (uint64_t) 1 << (MPIDI_Global.max_rma_key_bits - MPIDI_Global.context_shift);
    MPIR_ERR_CHKANDSTMT(MPIR_CONTEXT_READ_FIELD(PREFIX, win->comm_ptr->context_id)
                        >= max_contexts_allowed, mpi_errno, MPI_ERR_OTHER,
                        goto fn_fail, "**ofid_mr_reg");

    MPIDI_OFI_WIN(win).win_id = MPIDI_OFI_rma_key_pack(win->comm_ptr->context_id,
                                                       MPIDI_OFI_KEY_TYPE_WINDOW, window_instance);

    MPIDI_CH4U_map_set(MPIDI_Global.win_map, MPIDI_OFI_WIN(win).win_id, win, MPL_MEM_RMA);

    MPIDI_OFI_WIN(win).sep_tx_idx = -1; /* By default, -1 means not using scalable EP. */

    /* First, try to enable scalable EP. */
    if (MPIR_CVAR_CH4_OFI_ENABLE_SCALABLE_ENDPOINTS && MPIR_CVAR_CH4_OFI_MAX_RMA_SEP_CTX > 0) {
        /* Create tx based on scalable EP. */
        if (MPIDI_OFI_win_init_sep(win) == MPI_SUCCESS) {
            goto fn_exit;
        }
    }

    /* If scalable EP is not available, try shared transmit context next. */
    /* Create tx using shared transmit context. */
    if (MPIDI_OFI_win_init_stx(win) == MPI_SUCCESS) {
        goto fn_exit;
    }

    /* Fall back to use global EP, without per-window sync support. */
    MPIDI_OFI_win_init_global(win);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_WIN_INIT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_win_progress_fence
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_OFI_win_progress_fence(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    int itercount = 0;
    int ret;
    uint64_t tcount, donecount;
    MPIDI_OFI_win_request_t *r;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_WIN_PROGRESS_FENCE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_WIN_PROGRESS_FENCE);

    MPID_THREAD_CS_ENTER(POBJ, MPIDI_OFI_THREAD_FI_MUTEX);
    tcount = *MPIDI_OFI_WIN(win).issued_cntr;
    donecount = fi_cntr_read(MPIDI_OFI_WIN(win).cmpl_cntr);

    MPIR_Assert(donecount <= tcount);

    while (tcount > donecount) {
        MPIR_Assert(donecount <= tcount);
        MPID_THREAD_CS_EXIT(POBJ, MPIDI_OFI_THREAD_FI_MUTEX);
        MPID_THREAD_CS_EXIT(VNI, MPIDI_CH4_Global.vni_lock);
        MPIDI_OFI_PROGRESS();
        MPID_THREAD_CS_ENTER(VNI, MPIDI_CH4_Global.vni_lock);
        MPID_THREAD_CS_ENTER(POBJ, MPIDI_OFI_THREAD_FI_MUTEX);
        donecount = fi_cntr_read(MPIDI_OFI_WIN(win).cmpl_cntr);
        itercount++;

        if (itercount == 1000) {
            ret = fi_cntr_wait(MPIDI_OFI_WIN(win).cmpl_cntr, tcount, 0);
            MPIDI_OFI_ERR(ret < 0 && ret != -FI_ETIMEDOUT,
                          mpi_errno,
                          MPI_ERR_RMA_RANGE,
                          "**ofid_cntr_wait",
                          "**ofid_cntr_wait %s %d %s %s",
                          __SHORT_FILE__, __LINE__, FCNAME, fi_strerror(-ret));
            itercount = 0;
        }
    }

    r = MPIDI_OFI_WIN(win).syncQ;

    while (r) {
        MPIDI_OFI_win_request_t *next = r->next;
        MPIDI_OFI_rma_done_event(NULL, (MPIR_Request *) r);
        r = next;
    }

    MPIDI_OFI_WIN(win).syncQ = NULL;
  fn_exit:
    MPID_THREAD_CS_EXIT(POBJ, MPIDI_OFI_THREAD_FI_MUTEX);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_WIN_PROGRESS_FENCE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_win_set_info
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_win_set_info(MPIR_Win * win, MPIR_Info * info)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_SET_INFO);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_SET_INFO);

    mpi_errno = MPIDI_CH4R_mpi_win_set_info(win, info);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_SET_INFO);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_win_start
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_win_start(MPIR_Group * group, int assert, MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_START);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_START);

    mpi_errno = MPIDI_CH4R_mpi_win_start(group, assert, win);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_START);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_win_complete
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_win_complete(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_COMPLETE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_COMPLETE);

    mpi_errno = MPIDI_CH4R_mpi_win_complete(win);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_COMPLETE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_win_post
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_win_post(MPIR_Group * group, int assert, MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_POST);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_POST);

    mpi_errno = MPIDI_CH4R_mpi_win_post(group, assert, win);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_POST);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_win_wait
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_win_wait(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_WAIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_WAIT);

    mpi_errno = MPIDI_CH4R_mpi_win_wait(win);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_WAIT);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_win_test
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_win_test(MPIR_Win * win, int *flag)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_TEST);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_TEST);

    mpi_errno = MPIDI_CH4R_mpi_win_test(win, flag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_TEST);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_win_lock
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_win_lock(int lock_type, int rank, int assert, MPIR_Win * win,
                                        MPIDI_av_entry_t * addr)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_LOCK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_LOCK);

    mpi_errno = MPIDI_CH4R_mpi_win_lock(lock_type, rank, assert, win);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_LOCK);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_win_unlock
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_win_unlock(int rank, MPIR_Win * win, MPIDI_av_entry_t * addr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_UNLOCK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_UNLOCK);

    mpi_errno = MPIDI_CH4R_mpi_win_unlock(rank, win);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_UNLOCK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;

}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_win_get_info
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_win_get_info(MPIR_Win * win, MPIR_Info ** info_p_p)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_GET_INFO);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_GET_INFO);

    mpi_errno = MPIDI_CH4R_mpi_win_get_info(win, info_p_p);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_GET_INFO);
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_win_free
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_win_free(MPIR_Win ** win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_FREE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_FREE);

    mpi_errno = MPIDI_CH4R_mpi_win_free(win_ptr);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_FREE);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_win_fence
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_win_fence(int massert, MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_FENCE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_FENCE);

    mpi_errno = MPIDI_CH4R_mpi_win_fence(massert, win);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_FENCE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_win_create
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_win_create(void *base,
                                          MPI_Aint length,
                                          int disp_unit,
                                          MPIR_Info * info, MPIR_Comm * comm_ptr,
                                          MPIR_Win ** win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_CREATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_CREATE);

    mpi_errno = MPIDI_CH4R_mpi_win_create(base, length, disp_unit, info, comm_ptr, win_ptr);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_CREATE);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_win_attach
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_win_attach(MPIR_Win * win, void *base, MPI_Aint size)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_ATTACH);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_ATTACH);

    mpi_errno = MPIDI_CH4R_mpi_win_attach(win, base, size);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_ATTACH);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_win_allocate_shared
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_win_allocate_shared(MPI_Aint size,
                                                   int disp_unit,
                                                   MPIR_Info * info_ptr,
                                                   MPIR_Comm * comm_ptr,
                                                   void **base_ptr, MPIR_Win ** win_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_ALLOCATE_SHARED);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_ALLOCATE_SHARED);

    mpi_errno = MPIDI_CH4R_mpi_win_allocate_shared(size, disp_unit, info_ptr,
                                                   comm_ptr, base_ptr, win_ptr);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_ALLOCATE_SHARED);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_win_detach
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_win_detach(MPIR_Win * win, const void *base)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_DETACH);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_DETACH);

    mpi_errno = MPIDI_CH4R_mpi_win_detach(win, base);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_DETACH);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_win_shared_query
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_win_shared_query(MPIR_Win * win,
                                                int rank,
                                                MPI_Aint * size, int *disp_unit, void *baseptr)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_SHARED_QUERY);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_SHARED_QUERY);

    mpi_errno = MPIDI_CH4R_mpi_win_shared_query(win, rank, size, disp_unit, baseptr);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_SHARED_QUERY);
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_win_allocate
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_win_allocate(MPI_Aint size,
                                            int disp_unit,
                                            MPIR_Info * info,
                                            MPIR_Comm * comm, void *baseptr, MPIR_Win ** win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_ALLOCATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_ALLOCATE);

    mpi_errno = MPIDI_CH4R_mpi_win_allocate(size, disp_unit, info, comm, baseptr, win_ptr);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_ALLOCATE);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_win_flush
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_win_flush(int rank, MPIR_Win * win, MPIDI_av_entry_t * addr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_FLUSH);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_FLUSH);

    mpi_errno = MPIDI_CH4R_mpi_win_flush(rank, win);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_FLUSH);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_win_flush_local_all
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_win_flush_local_all(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_FLUSH_LOCAL_ALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_FLUSH_LOCAL_ALL);

    mpi_errno = MPIDI_CH4R_mpi_win_flush_local_all(win);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_FLUSH_LOCAL_ALL);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_win_unlock_all
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_win_unlock_all(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_UNLOCK_ALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_UNLOCK_ALL);

    mpi_errno = MPIDI_CH4R_mpi_win_unlock_all(win);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_UNLOCK_ALL);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_win_create_dynamic
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_win_create_dynamic(MPIR_Info * info,
                                                  MPIR_Comm * comm, MPIR_Win ** win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_CREATE_DYNAMIC);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_CREATE_DYNAMIC);

    mpi_errno = MPIDI_CH4R_mpi_win_create_dynamic(info, comm, win_ptr);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_CREATE_DYNAMIC);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_win_flush_local
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_win_flush_local(int rank, MPIR_Win * win, MPIDI_av_entry_t * addr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_FLUSH_LOCAL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_FLUSH_LOCAL);

    mpi_errno = MPIDI_CH4R_mpi_win_flush_local(rank, win);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_FLUSH_LOCAL);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_win_sync
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_win_sync(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_SYNC);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_SYNC);

    mpi_errno = MPIDI_CH4R_mpi_win_sync(win);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_SYNC);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_win_flush_all
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_win_flush_all(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_FLUSH_ALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_FLUSH_ALL);

    MPIDI_OFI_MPI_CALL_POP(MPIDI_CH4R_mpi_win_flush_all(win));

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_FLUSH_ALL);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_win_lock_all
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_win_lock_all(int assert, MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_LOCK_ALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_LOCK_ALL);

    mpi_errno = MPIDI_CH4R_mpi_win_lock_all(assert, win);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_LOCK_ALL);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_win_create_hook
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_create_hook(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_CREATE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_CREATE_HOOK);

    /* This hook is called by CH4 generic call after CH4 initialization */
    if (MPIDI_OFI_ENABLE_RMA) {
        mpi_errno = MPIDI_OFI_win_init(win);
        if (mpi_errno != MPI_SUCCESS)
            goto fn_fail;

        mpi_errno = MPIDI_OFI_win_allgather(win, win->base, win->disp_unit);
        if (mpi_errno != MPI_SUCCESS)
            goto fn_fail;
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_CREATE_HOOK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_win_allocate_hook
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_allocate_hook(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_ALLOCATE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_ALLOCATE_HOOK);

    /* This hook is called by CH4 generic call after CH4 initialization */
    if (MPIDI_OFI_ENABLE_RMA) {
        mpi_errno = MPIDI_OFI_win_init(win);
        if (mpi_errno != MPI_SUCCESS)
            MPIR_ERR_POP(mpi_errno);

        mpi_errno = MPIDI_OFI_win_allgather(win, win->base, win->disp_unit);
        if (mpi_errno != MPI_SUCCESS)
            MPIR_ERR_POP(mpi_errno);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_ALLOCATE_HOOK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_win_allocate_shared_hook
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_allocate_shared_hook(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_ALLOCATE_SHARED_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_ALLOCATE_SHARED_HOOK);

    /* This hook is called by CH4 generic call after CH4 initialization */
    if (MPIDI_OFI_ENABLE_RMA) {
        mpi_errno = MPIDI_OFI_win_init(win);
        if (mpi_errno != MPI_SUCCESS)
            MPIR_ERR_POP(mpi_errno);

        mpi_errno = MPIDI_OFI_win_allgather(win, win->base, win->disp_unit);
        if (mpi_errno != MPI_SUCCESS)
            MPIR_ERR_POP(mpi_errno);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_ALLOCATE_SHARED_HOOK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_win_create_dynamic_hook
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_create_dynamic_hook(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_CREATE_DYNAMIC_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_CREATE_DYNAMIC_HOOK);

    /* This hook is called by CH4 generic call after CH4 initialization */
    if (MPIDI_OFI_ENABLE_RMA) {
        /* FIXME: should the OFI specific size be stored inside OFI rather
         * than overwriting CH4 info ? Now we simply followed original
         * create_dynamic routine.*/
        win->size = (uintptr_t) UINTPTR_MAX - (uintptr_t) MPI_BOTTOM;

        mpi_errno = MPIDI_OFI_win_init(win);
        if (mpi_errno != MPI_SUCCESS)
            MPIR_ERR_POP(mpi_errno);

        mpi_errno = MPIDI_OFI_win_allgather(win, win->base, win->disp_unit);
        if (mpi_errno != MPI_SUCCESS)
            MPIR_ERR_POP(mpi_errno);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_CREATE_DYNAMIC_HOOK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_win_attach_hook
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_attach_hook(MPIR_Win * win, void *base, MPI_Aint size)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_ATTACH_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_ATTACH_HOOK);

    /* Do nothing */

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_ATTACH_HOOK);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_win_detach_hook
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_detach_hook(MPIR_Win * win, const void *base)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_DETACH_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_DETACH_HOOK);

    /* Do nothing */

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_DETACH_HOOK);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_win_free_hook
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_free_hook(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    uint32_t window_instance;
    int key_type;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_FREE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_FREE_HOOK);

    if (MPIDI_OFI_ENABLE_RMA) {
        MPIDI_OFI_rma_key_unpack(MPIDI_OFI_WIN(win).win_id, NULL, &key_type, &window_instance);
        MPIR_Assert(key_type == MPIDI_OFI_KEY_TYPE_WINDOW);

        MPIDI_OFI_index_allocator_free(MPIDI_OFI_COMM(win->comm_ptr).win_id_allocator,
                                       window_instance);
        MPIDI_CH4U_map_erase(MPIDI_Global.win_map, MPIDI_OFI_WIN(win).win_id);
        /* For scalable EP: push transmit context index back into available pool. */
        if (MPIDI_OFI_WIN(win).sep_tx_idx != -1) {
            utarray_push_back(MPIDI_Global.rma_sep_idx_array, &(MPIDI_OFI_WIN(win).sep_tx_idx),
                              MPL_MEM_RMA);
        }
        if (MPIDI_OFI_WIN(win).ep != MPIDI_Global.ctx[0].tx)
            MPIDI_OFI_CALL(fi_close(&MPIDI_OFI_WIN(win).ep->fid), epclose);
        if (MPIDI_OFI_WIN(win).cmpl_cntr != MPIDI_Global.rma_cmpl_cntr)
            MPIDI_OFI_CALL(fi_close(&MPIDI_OFI_WIN(win).cmpl_cntr->fid), cntrclose);
        if (MPIDI_OFI_WIN(win).mr)
            MPIDI_OFI_CALL(fi_close(&MPIDI_OFI_WIN(win).mr->fid), mr_unreg);
        if (MPIDI_OFI_WIN(win).winfo) {
            MPL_free(MPIDI_OFI_WIN(win).winfo);
            MPIDI_OFI_WIN(win).winfo = NULL;
        }
        MPL_free(MPIDI_OFI_WIN(win).acc_hint);
        MPIDI_OFI_WIN(win).acc_hint = NULL;
    }
    /* This hook is called by CH4 generic call before CH4 finalization */

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_FREE_HOOK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_rma_win_cmpl_hook
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_rma_win_cmpl_hook(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_RMA_WIN_CMPL_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_RMA_WIN_CMPL_HOOK);
    if (MPIDI_OFI_ENABLE_RMA) {
        /* network completion */
        MPIDI_OFI_MPI_CALL_POP(MPIDI_OFI_win_progress_fence(win));
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_RMA_WIN_CMPL_HOOK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_rma_win_local_cmpl_hook
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_rma_win_local_cmpl_hook(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_RMA_WIN_LOCAL_CMPL_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_RMA_WIN_LOCAL_CMPL_HOOK);
    if (MPIDI_OFI_ENABLE_RMA) {
        /* network completion */
        MPIDI_OFI_MPI_CALL_POP(MPIDI_OFI_win_progress_fence(win));
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_RMA_WIN_LOCAL_CMPL_HOOK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_rma_target_cmpl_hook
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_rma_target_cmpl_hook(int rank ATTRIBUTE((unused)),
                                                           MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_RMA_TARGET_CMPL_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_RMA_TARGET_CMPL_HOOK);
    if (MPIDI_OFI_ENABLE_RMA) {
        /* network completion */
        MPIDI_OFI_MPI_CALL_POP(MPIDI_OFI_win_progress_fence(win));
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_RMA_TARGET_CMPL_HOOK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_rma_target_local_cmpl_hook
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_rma_target_local_cmpl_hook(int rank ATTRIBUTE((unused)),
                                                                 MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_RMA_TARGET_LOCAL_CMPL_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_RMA_TARGET_LOCAL_CMPL_HOOK);
    if (MPIDI_OFI_ENABLE_RMA) {
        /* network completion */
        MPIDI_OFI_MPI_CALL_POP(MPIDI_OFI_win_progress_fence(win));
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_RMA_TARGET_LOCAL_CMPL_HOOK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
#endif /* OFI_WIN_H_INCLUDED */

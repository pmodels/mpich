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
#ifndef CH4R_WIN_H_INCLUDED
#define CH4R_WIN_H_INCLUDED

#include "ch4_impl.h"
#include "ch4i_util.h"
#include <opa_primitives.h>
#include "mpir_info.h"
#include "ch4r_symheap.h"
#include "uthash.h"
#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif /* HAVE_SYS_MMAN_H */

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_CH4_RMA_MEM_EFFICIENT
      category    : CH4
      type        : boolean
      default     : false
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_GROUP_EQ
      description : >-
        If true, memory-saving mode is on, per-target object is released
        at the epoch end call.
        If false, performance-efficient mode is on, all allocated target
        objects are cached and freed at win_finalize.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

extern MPIR_T_pvar_timer_t PVAR_TIMER_rma_winlock_getlocallock ATTRIBUTE((unused));
extern MPIR_T_pvar_timer_t PVAR_TIMER_rma_wincreate_allgather ATTRIBUTE((unused));
extern MPIR_T_pvar_timer_t PVAR_TIMER_rma_amhdr_set ATTRIBUTE((unused));

MPL_STATIC_INLINE_PREFIX void MPIDI_CH4I_parse_info_accu_ops_str(const char *str,
                                                                 uint32_t * ops_ptr)
{
    uint32_t ops = 0;
    char *value, *token, *savePtr = NULL;

    value = (char *) str;
    /* str can never be NULL. */
    MPIR_Assert(value);

    /* handle special value */
    if (!strncmp(value, "none", strlen("none"))) {
        *ops_ptr = 0;
        return;
    } else if (!strncmp(value, "any_op", strlen("any_op"))) {
        MPIDI_CH4U_win_info_accu_op_shift_t op_shift;
        /* add all ops */
        for (op_shift = 0; op_shift < MPIDI_CH4I_ACCU_OP_SHIFT_LAST; op_shift++)
            ops |= (1 << op_shift);
        *ops_ptr = ops;
        return;
    }

    token = (char *) strtok_r(value, ",", &savePtr);
    while (token != NULL) {

        /* traverse op list (exclude null and last) and add the op if set */
        if (!strncmp(token, "max", strlen("max")))
            ops |= (1 << MPIDI_CH4I_ACCU_MAX_SHIFT);
        else if (!strncmp(token, "min", strlen("min")))
            ops |= (1 << MPIDI_CH4I_ACCU_MIN_SHIFT);
        else if (!strncmp(token, "sum", strlen("sum")))
            ops |= (1 << MPIDI_CH4I_ACCU_SUM_SHIFT);
        else if (!strncmp(token, "prod", strlen("prod")))
            ops |= (1 << MPIDI_CH4I_ACCU_PROD_SHIFT);
        else if (!strncmp(token, "maxloc", strlen("maxloc")))
            ops |= (1 << MPIDI_CH4I_ACCU_MAXLOC_SHIFT);
        else if (!strncmp(token, "minloc", strlen("minloc")))
            ops |= (1 << MPIDI_CH4I_ACCU_MINLOC_SHIFT);
        else if (!strncmp(token, "band", strlen("band")))
            ops |= (1 << MPIDI_CH4I_ACCU_BAND_SHIFT);
        else if (!strncmp(token, "bor", strlen("bor")))
            ops |= (1 << MPIDI_CH4I_ACCU_BOR_SHIFT);
        else if (!strncmp(token, "bxor", strlen("bxor")))
            ops |= (1 << MPIDI_CH4I_ACCU_BXOR_SHIFT);
        else if (!strncmp(token, "land", strlen("land")))
            ops |= (1 << MPIDI_CH4I_ACCU_LAND_SHIFT);
        else if (!strncmp(token, "lor", strlen("lor")))
            ops |= (1 << MPIDI_CH4I_ACCU_LOR_SHIFT);
        else if (!strncmp(token, "lxor", strlen("lxor")))
            ops |= (1 << MPIDI_CH4I_ACCU_LXOR_SHIFT);
        else if (!strncmp(token, "replace", strlen("replace")))
            ops |= (1 << MPIDI_CH4I_ACCU_REPLACE_SHIFT);
        else if (!strncmp(token, "no_op", strlen("no_op")))
            ops |= (1 << MPIDI_CH4I_ACCU_NO_OP_SHIFT);
        else if (!strncmp(token, "cswap", strlen("cswap")) ||
                 !strncmp(token, "compare_and_swap", strlen("compare_and_swap")))
            ops |= (1 << MPIDI_CH4I_ACCU_CSWAP_SHIFT);

        token = (char *) strtok_r(NULL, ",", &savePtr);
    }

    /* update info only when any valid value is set */
    if (ops)
        *ops_ptr = ops;
}

MPL_STATIC_INLINE_PREFIX void MPIDI_CH4I_get_info_accu_ops_str(uint32_t val, char *buf,
                                                               size_t maxlen)
{
    int c = 0;

    MPIR_Assert(maxlen >= strlen("max,min,sum,prod,maxloc,minloc,band,bor,"
                                 "bxor,land,lor,lxor,replace,no_op,cswap") + 1);

    if (val & (1 << MPIDI_CH4I_ACCU_MAX_SHIFT))
        c += snprintf(buf + c, maxlen - c, "max");
    if (val & (1 << MPIDI_CH4I_ACCU_MIN_SHIFT))
        c += snprintf(buf + c, maxlen - c, "%smin", (c > 0) ? "," : "");
    if (val & (1 << MPIDI_CH4I_ACCU_SUM_SHIFT))
        c += snprintf(buf + c, maxlen - c, "%ssum", (c > 0) ? "," : "");
    if (val & (1 << MPIDI_CH4I_ACCU_PROD_SHIFT))
        c += snprintf(buf + c, maxlen - c, "%sprod", (c > 0) ? "," : "");
    if (val & (1 << MPIDI_CH4I_ACCU_MAXLOC_SHIFT))
        c += snprintf(buf + c, maxlen - c, "%smaxloc", (c > 0) ? "," : "");
    if (val & (1 << MPIDI_CH4I_ACCU_MINLOC_SHIFT))
        c += snprintf(buf + c, maxlen - c, "%sminloc", (c > 0) ? "," : "");
    if (val & (1 << MPIDI_CH4I_ACCU_BAND_SHIFT))
        c += snprintf(buf + c, maxlen - c, "%sband", (c > 0) ? "," : "");
    if (val & (1 << MPIDI_CH4I_ACCU_BOR_SHIFT))
        c += snprintf(buf + c, maxlen - c, "%sbor", (c > 0) ? "," : "");
    if (val & (1 << MPIDI_CH4I_ACCU_BXOR_SHIFT))
        c += snprintf(buf + c, maxlen - c, "%sbxor", (c > 0) ? "," : "");
    if (val & (1 << MPIDI_CH4I_ACCU_LAND_SHIFT))
        c += snprintf(buf + c, maxlen - c, "%sland", (c > 0) ? "," : "");
    if (val & (1 << MPIDI_CH4I_ACCU_LOR_SHIFT))
        c += snprintf(buf + c, maxlen - c, "%slor", (c > 0) ? "," : "");
    if (val & (1 << MPIDI_CH4I_ACCU_LXOR_SHIFT))
        c += snprintf(buf + c, maxlen - c, "%slxor", (c > 0) ? "," : "");
    if (val & (1 << MPIDI_CH4I_ACCU_REPLACE_SHIFT))
        c += snprintf(buf + c, maxlen - c, "%sreplace", (c > 0) ? "," : "");
    if (val & (1 << MPIDI_CH4I_ACCU_NO_OP_SHIFT))
        c += snprintf(buf + c, maxlen - c, "%sno_op", (c > 0) ? "," : "");
    if (val & (1 << MPIDI_CH4I_ACCU_CSWAP_SHIFT))
        c += snprintf(buf + c, maxlen - c, "%scswap", (c > 0) ? "," : "");

    if (c == 0)
        strncpy(buf, "none", maxlen);
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4_RMA_Init_sync_pvars
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4R_RMA_Init_sync_pvars(void)
{
    int mpi_errno = MPI_SUCCESS;
    /* rma_winlock_getlocallock */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(RMA,
                                      MPI_DOUBLE,
                                      rma_winlock_getlocallock,
                                      MPI_T_VERBOSITY_MPIDEV_DETAIL,
                                      MPI_T_BIND_NO_OBJECT,
                                      MPIR_T_PVAR_FLAG_READONLY,
                                      "RMA", "WIN_LOCK:Get local lock (in seconds)");

    /* rma_wincreate_allgather */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(RMA,
                                      MPI_DOUBLE,
                                      rma_wincreate_allgather,
                                      MPI_T_VERBOSITY_MPIDEV_DETAIL,
                                      MPI_T_BIND_NO_OBJECT,
                                      MPIR_T_PVAR_FLAG_READONLY,
                                      "RMA", "WIN_CREATE:Allgather (in seconds)");

    /* rma_amhdr_set */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(RMA,
                                      MPI_DOUBLE,
                                      rma_amhdr_set,
                                      MPI_T_VERBOSITY_MPIDEV_DETAIL,
                                      MPI_T_BIND_NO_OBJECT,
                                      MPIR_T_PVAR_FLAG_READONLY,
                                      "RMA", "Set fields in AM Handler (in seconds)");

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4I_win_set_info
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_CH4I_win_set_info(MPIR_Win * win, MPIR_Info * info, bool is_init)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4I_WIN_SET_INFO);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4I_WIN_SET_INFO);

    MPIR_Info *curr_ptr;
    char *value, *token, *savePtr = NULL;
    int save_ordering;

    curr_ptr = info->next;

    while (curr_ptr) {
        if (!strcmp(curr_ptr->key, "no_locks")) {
            if (!strcmp(curr_ptr->value, "true"))
                MPIDI_CH4U_WIN(win, info_args).no_locks = 1;
            else if (!strcmp(curr_ptr->value, "false"))
                MPIDI_CH4U_WIN(win, info_args).no_locks = 0;
        } else if (!strcmp(curr_ptr->key, "accumulate_ordering")) {
            save_ordering = MPIDI_CH4U_WIN(win, info_args).accumulate_ordering;
            MPIDI_CH4U_WIN(win, info_args).accumulate_ordering = 0;
            if (!strcmp(curr_ptr->value, "none")) {
                /* For MPI-3, "none" means no ordering and is not default. */
                goto next;
            }

            /* value can never be NULL. */
            MPIR_Assert(curr_ptr->value);

            value = curr_ptr->value;
            token = (char *) strtok_r(value, ",", &savePtr);

            while (token) {
                if (!memcmp(token, "rar", 3))
                    MPIDI_CH4U_WIN(win, info_args).accumulate_ordering =
                        (MPIDI_CH4U_WIN(win, info_args).accumulate_ordering |
                         MPIDI_CH4I_ACCU_ORDER_RAR);
                else if (!memcmp(token, "raw", 3))
                    MPIDI_CH4U_WIN(win, info_args).accumulate_ordering =
                        (MPIDI_CH4U_WIN(win, info_args).accumulate_ordering |
                         MPIDI_CH4I_ACCU_ORDER_RAW);
                else if (!memcmp(token, "war", 3))
                    MPIDI_CH4U_WIN(win, info_args).accumulate_ordering =
                        (MPIDI_CH4U_WIN(win, info_args).accumulate_ordering |
                         MPIDI_CH4I_ACCU_ORDER_WAR);
                else if (!memcmp(token, "waw", 3))
                    MPIDI_CH4U_WIN(win, info_args).accumulate_ordering =
                        (MPIDI_CH4U_WIN(win, info_args).accumulate_ordering |
                         MPIDI_CH4I_ACCU_ORDER_WAW);
                else
                    MPIR_ERR_SETANDSTMT(mpi_errno, MPI_ERR_ARG, goto fn_fail, "**info");

                token = (char *) strtok_r(NULL, ",", &savePtr);
            }

            if (MPIDI_CH4U_WIN(win, info_args).accumulate_ordering == 0)
                MPIDI_CH4U_WIN(win, info_args).accumulate_ordering = save_ordering;
        } else if (!strcmp(curr_ptr->key, "accumulate_ops")) {
            if (!strcmp(curr_ptr->value, "same_op"))
                MPIDI_CH4U_WIN(win, info_args).accumulate_ops = MPIDI_CH4I_ACCU_SAME_OP;
            else if (!strcmp(curr_ptr->value, "same_op_no_op"))
                MPIDI_CH4U_WIN(win, info_args).accumulate_ops = MPIDI_CH4I_ACCU_SAME_OP_NO_OP;
        } else if (!strcmp(curr_ptr->key, "same_disp_unit")) {
            if (!strcmp(curr_ptr->value, "true"))
                MPIDI_CH4U_WIN(win, info_args).same_disp_unit = 1;
            else if (!strcmp(curr_ptr->value, "false"))
                MPIDI_CH4U_WIN(win, info_args).same_disp_unit = 0;
        } else if (!strcmp(curr_ptr->key, "same_size")) {
            if (!strcmp(curr_ptr->value, "true"))
                MPIDI_CH4U_WIN(win, info_args).same_size = 1;
            else if (!strcmp(curr_ptr->value, "false"))
                MPIDI_CH4U_WIN(win, info_args).same_size = 0;
        } else if (!strcmp(curr_ptr->key, "alloc_shared_noncontig")) {
            if (!strcmp(curr_ptr->value, "true"))
                MPIDI_CH4U_WIN(win, info_args).alloc_shared_noncontig = 1;
            else if (!strcmp(curr_ptr->value, "false"))
                MPIDI_CH4U_WIN(win, info_args).alloc_shared_noncontig = 0;
        } else if (!strcmp(curr_ptr->key, "alloc_shm")) {
            if (!strcmp(curr_ptr->value, "true"))
                MPIDI_CH4U_WIN(win, info_args).alloc_shm = 1;
            else if (!strcmp(curr_ptr->value, "false"))
                MPIDI_CH4U_WIN(win, info_args).alloc_shm = 0;
        }
        /* We allow the user to set the following atomics hint only at window init time,
         * all future updates by win_set_info are ignored. This is because we do not
         * have a good way to ensure all outstanding atomic ops have been completed
         * on all processes especially in passive-target epochs. */
        else if (is_init && !strcmp(curr_ptr->key, "which_accumulate_ops")) {
            MPIDI_CH4I_parse_info_accu_ops_str(curr_ptr->value,
                                               &MPIDI_CH4U_WIN(win,
                                                               info_args).which_accumulate_ops);
        } else if (is_init && !strcmp(curr_ptr->key, "accumulate_noncontig_dtype")) {
            if (!strcmp(curr_ptr->value, "true"))
                MPIDI_CH4U_WIN(win, info_args).accumulate_noncontig_dtype = true;
            else if (!strcmp(curr_ptr->value, "false"))
                MPIDI_CH4U_WIN(win, info_args).accumulate_noncontig_dtype = false;
        } else if (is_init && !strcmp(curr_ptr->key, "accumulate_max_bytes")) {
            if (!strcmp(curr_ptr->value, "unlimited") || !strcmp(curr_ptr->value, "-1"))
                MPIDI_CH4U_WIN(win, info_args).accumulate_max_bytes = -1;
            else {
                long max_bytes = atol(curr_ptr->value);
                if (max_bytes >= 0)
                    MPIDI_CH4U_WIN(win, info_args).accumulate_max_bytes = max_bytes;
            }
        } else if (is_init && !strcmp(curr_ptr->key, "disable_shm_accumulate")) {
            if (!strcmp(curr_ptr->value, "true"))
                MPIDI_CH4U_WIN(win, info_args).disable_shm_accumulate = true;
            else
                MPIDI_CH4U_WIN(win, info_args).disable_shm_accumulate = false;
        }
      next:
        curr_ptr = curr_ptr->next;
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4I_WIN_SET_INFO);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4R_mpi_win_set_info
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4R_mpi_win_set_info(MPIR_Win * win, MPIR_Info * info)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4R_MPI_WIN_SET_INFO);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4R_MPI_WIN_SET_INFO);

    mpi_errno = MPIDI_CH4I_win_set_info(win, info, FALSE /* is_init */);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);

    mpi_errno = MPIR_Barrier(win->comm_ptr, &errflag);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4R_MPI_WIN_SET_INFO);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4R_win_init
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4R_win_init(MPI_Aint length,
                                      int disp_unit,
                                      MPIR_Win ** win_ptr,
                                      MPIR_Info * info,
                                      MPIR_Comm * comm_ptr, int create_flavor, int model)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Win *win = (MPIR_Win *) MPIR_Handle_obj_alloc(&MPIR_Win_mem);
    MPIDI_CH4U_win_target_t *targets = NULL;
    MPIR_Comm *win_comm_ptr;
    MPIDI_CH4U_win_info_accu_op_shift_t op_shift;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4R_WIN_INIT);
    MPIR_FUNC_VERBOSE_RMA_ENTER(MPID_STATE_MPIDI_CH4R_WIN_INIT);

    MPIR_ERR_CHKANDSTMT(win == NULL, mpi_errno, MPI_ERR_NO_MEM, goto fn_fail, "**nomem");
    *win_ptr = win;

    memset(&win->dev.ch4u, 0, sizeof(MPIDI_CH4U_win_t));

    /* Duplicate the original communicator here to avoid having collisions
     * between internal collectives */
    mpi_errno = MPIR_Comm_dup_impl(comm_ptr, &win_comm_ptr);
    if (MPI_SUCCESS != mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    MPIDI_CH4U_WIN(win, targets) = targets;

    win->errhandler = NULL;
    win->base = NULL;
    win->size = length;
    win->disp_unit = disp_unit;
    win->create_flavor = (MPIR_Win_flavor_t) create_flavor;
    win->model = (MPIR_Win_model_t) model;
    win->copyCreateFlavor = (MPIR_Win_flavor_t) 0;
    win->copyModel = (MPIR_Win_model_t) 0;
    win->attributes = NULL;
    win->comm_ptr = win_comm_ptr;
    win->copyDispUnit = 0;
    win->copySize = 0;
    MPIDI_CH4U_WIN(win, shared_table) = NULL;
    MPIDI_CH4U_WIN(win, sync).assert_mode = 0;
    MPIDI_CH4U_WIN(win, shm_allocated) = 0;

    /* Initialize the info (hint) flags per window */
    MPIDI_CH4U_WIN(win, info_args).no_locks = 0;
    MPIDI_CH4U_WIN(win, info_args).accumulate_ordering = (MPIDI_CH4I_ACCU_ORDER_RAR |
                                                          MPIDI_CH4I_ACCU_ORDER_RAW |
                                                          MPIDI_CH4I_ACCU_ORDER_WAR |
                                                          MPIDI_CH4I_ACCU_ORDER_WAW);
    MPIDI_CH4U_WIN(win, info_args).accumulate_ops = MPIDI_CH4I_ACCU_SAME_OP_NO_OP;
    MPIDI_CH4U_WIN(win, info_args).same_size = 0;
    MPIDI_CH4U_WIN(win, info_args).same_disp_unit = 0;
    MPIDI_CH4U_WIN(win, info_args).alloc_shared_noncontig = 0;
    if (win->create_flavor == MPI_WIN_FLAVOR_ALLOCATE
        || win->create_flavor == MPI_WIN_FLAVOR_SHARED) {
        MPIDI_CH4U_WIN(win, info_args).alloc_shm = 1;
    } else {
        MPIDI_CH4U_WIN(win, info_args).alloc_shm = 0;
    }

    /* default any op */
    MPIDI_CH4U_WIN(win, info_args).which_accumulate_ops = 0;
    for (op_shift = 0; op_shift < MPIDI_CH4I_ACCU_OP_SHIFT_LAST; op_shift++)
        MPIDI_CH4U_WIN(win, info_args).which_accumulate_ops |= (1 << op_shift);
    MPIDI_CH4U_WIN(win, info_args).accumulate_noncontig_dtype = true;
    MPIDI_CH4U_WIN(win, info_args).accumulate_max_bytes = -1;
    MPIDI_CH4U_WIN(win, info_args).disable_shm_accumulate = false;

    if ((info != NULL) && ((int *) info != (int *) MPI_INFO_NULL)) {
        mpi_errno = MPIDI_CH4I_win_set_info(win, info, TRUE /* is_init */);
        if (MPI_SUCCESS != mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }


    MPIDI_CH4U_WIN(win, mmap_sz) = 0;
    MPIDI_CH4U_WIN(win, mmap_addr) = NULL;

    MPIR_cc_set(&MPIDI_CH4U_WIN(win, local_cmpl_cnts), 0);
    MPIR_cc_set(&MPIDI_CH4U_WIN(win, remote_cmpl_cnts), 0);
    MPIR_cc_set(&MPIDI_CH4U_WIN(win, remote_acc_cmpl_cnts), 0);

    MPIDI_CH4U_WIN(win, win_id) = MPIDI_CH4U_generate_win_id(comm_ptr);
    MPIDI_CH4U_map_set(MPIDI_CH4_Global.win_map, MPIDI_CH4U_WIN(win, win_id), win, MPL_MEM_RMA);

  fn_exit:
    MPIR_FUNC_VERBOSE_RMA_EXIT(MPID_STATE_MPIDI_CH4R_WIN_INIT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4I_fill_ranks_in_win_grp
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4I_fill_ranks_in_win_grp(MPIR_Win * win_ptr, MPIR_Group * group_ptr,
                                                   int *ranks_in_win_grp)
{
    int mpi_errno = MPI_SUCCESS;
    int i, *ranks_in_grp = NULL;
    MPIR_Group *win_grp_ptr;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4I_FILL_RANKS_IN_WIN_GRP);
    MPIR_FUNC_VERBOSE_RMA_ENTER(MPID_STATE_MPIDI_CH4I_FILL_RANKS_IN_WIN_GRP);

    ranks_in_grp = (int *) MPL_malloc(group_ptr->size * sizeof(int), MPL_MEM_RMA);
    MPIR_Assert(ranks_in_grp);
    for (i = 0; i < group_ptr->size; i++)
        ranks_in_grp[i] = i;

    mpi_errno = MPIR_Comm_group_impl(win_ptr->comm_ptr, &win_grp_ptr);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);

    mpi_errno = MPIR_Group_translate_ranks_impl(group_ptr, group_ptr->size,
                                                ranks_in_grp, win_grp_ptr, ranks_in_win_grp);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);

    mpi_errno = MPIR_Group_free_impl(win_grp_ptr);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    MPL_free(ranks_in_grp);

    MPIR_FUNC_VERBOSE_RMA_EXIT(MPID_STATE_MPIDI_CH4I_FILL_RANKS_IN_WIN_GRP);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4R_mpi_win_start
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4R_mpi_win_start(MPIR_Group * group, int assert, MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4R_MPI_WIN_START);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4R_MPI_WIN_START);

    MPIDI_CH4U_ACCESS_EPOCH_CHECK_NONE(win, mpi_errno, goto fn_fail);

    MPIR_Group_add_ref(group);
    if (assert & MPI_MODE_NOCHECK) {
        goto no_check;
    }

    MPIDI_CH4R_PROGRESS_WHILE(group->size != (int) MPIDI_CH4U_WIN(win, sync).pw.count);
  no_check:
    MPIDI_CH4U_WIN(win, sync).pw.count = 0;

    MPIR_ERR_CHKANDJUMP((MPIDI_CH4U_WIN(win, sync).sc.group != NULL),
                        mpi_errno, MPI_ERR_GROUP, "**group");
    MPIDI_CH4U_WIN(win, sync).sc.group = group;
    MPIDI_CH4U_WIN(win, sync).access_epoch_type = MPIDI_CH4U_EPOTYPE_START;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4R_MPI_WIN_START);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4R_mpi_win_complete
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4R_mpi_win_complete(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_CH4U_win_cntrl_msg_t msg;
    int win_grp_idx, peer;
    MPIR_Group *group;
    int *ranks_in_win_grp = NULL;
    int all_local_completed = 0;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4R_MPI_WIN_COMPLETE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4R_MPI_WIN_COMPLETE);

    MPIDI_CH4U_ACCESS_EPOCH_CHECK(win, MPIDI_CH4U_EPOTYPE_START, mpi_errno, return mpi_errno);

    group = MPIDI_CH4U_WIN(win, sync).sc.group;
    MPIR_Assert(group != NULL);

    /* Ensure op completion in netmod and shmmod */
    mpi_errno = MPIDI_NM_rma_win_cmpl_hook(win);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);

#ifndef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_SHM_rma_win_cmpl_hook(win);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);
#endif

    msg.win_id = MPIDI_CH4U_WIN(win, win_id);
    msg.origin_rank = win->comm_ptr->rank;

    /* Ensure completion of AM operations */
    ranks_in_win_grp = (int *) MPL_malloc(sizeof(int) * group->size, MPL_MEM_RMA);
    MPIR_Assert(ranks_in_win_grp);

    mpi_errno = MPIDI_CH4I_fill_ranks_in_win_grp(win, group, ranks_in_win_grp);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);

    /* FIXME: now we simply set per-target counters for PSCW, can it be optimized ? */
    do {
        MPID_THREAD_CS_EXIT(VNI, MPIDI_CH4_Global.vni_lock);
        MPIDI_CH4R_PROGRESS();
        MPID_THREAD_CS_ENTER(VNI, MPIDI_CH4_Global.vni_lock);
        MPIDI_win_check_group_local_completed(win, ranks_in_win_grp, group->size,
                                              &all_local_completed);
    } while (all_local_completed != 1);

    for (win_grp_idx = 0; win_grp_idx < group->size; ++win_grp_idx) {
        peer = ranks_in_win_grp[win_grp_idx];

#ifndef MPIDI_CH4_DIRECT_NETMOD
        if (MPIDI_CH4_rank_is_local(peer, win->comm_ptr))
            mpi_errno = MPIDI_SHM_am_send_hdr(peer, win->comm_ptr,
                                              MPIDI_CH4U_WIN_COMPLETE, &msg, sizeof(msg));
        else
#endif
        {
            mpi_errno = MPIDI_NM_am_send_hdr(peer, win->comm_ptr,
                                             MPIDI_CH4U_WIN_COMPLETE, &msg, sizeof(msg));
        }

        if (mpi_errno != MPI_SUCCESS)
            MPIR_ERR_SETANDSTMT(mpi_errno, MPI_ERR_RMA_SYNC, goto fn_fail, "**rmasync");
    }

    /* In performance-efficient mode, all allocated targets are freed at win_finalize. */
    if (MPIR_CVAR_CH4_RMA_MEM_EFFICIENT)
        MPIDI_CH4U_win_target_cleanall(win);
    MPIDI_CH4U_WIN(win, sync).access_epoch_type = MPIDI_CH4U_EPOTYPE_NONE;
    MPIR_Group_release(MPIDI_CH4U_WIN(win, sync).sc.group);
    MPIDI_CH4U_WIN(win, sync).sc.group = NULL;

  fn_exit:
    MPL_free(ranks_in_win_grp);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4R_MPI_WIN_COMPLETE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4R_mpi_win_post
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4R_mpi_win_post(MPIR_Group * group, int assert, MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_CH4U_win_cntrl_msg_t msg;
    int win_grp_idx, peer;
    int *ranks_in_win_grp = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4R_MPI_WIN_POST);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4R_MPI_WIN_POST);

    MPIDI_CH4U_EXPOSURE_EPOCH_CHECK_NONE(win, mpi_errno, goto fn_fail);

    MPIR_Group_add_ref(group);
    MPIR_ERR_CHKANDJUMP((MPIDI_CH4U_WIN(win, sync).pw.group != NULL),
                        mpi_errno, MPI_ERR_GROUP, "**group");

    MPIDI_CH4U_WIN(win, sync).pw.group = group;
    MPIR_Assert(group != NULL);
    if (assert & MPI_MODE_NOCHECK) {
        goto no_check;
    }

    msg.win_id = MPIDI_CH4U_WIN(win, win_id);
    msg.origin_rank = win->comm_ptr->rank;

    ranks_in_win_grp = (int *) MPL_malloc(sizeof(int) * group->size, MPL_MEM_RMA);
    MPIR_Assert(ranks_in_win_grp);

    mpi_errno = MPIDI_CH4I_fill_ranks_in_win_grp(win, group, ranks_in_win_grp);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);

    for (win_grp_idx = 0; win_grp_idx < group->size; ++win_grp_idx) {
        peer = ranks_in_win_grp[win_grp_idx];

#ifndef MPIDI_CH4_DIRECT_NETMOD
        if (MPIDI_CH4_rank_is_local(peer, win->comm_ptr))
            mpi_errno = MPIDI_SHM_am_send_hdr(peer, win->comm_ptr,
                                              MPIDI_CH4U_WIN_POST, &msg, sizeof(msg));
        else
#endif
        {
            mpi_errno = MPIDI_NM_am_send_hdr(peer, win->comm_ptr,
                                             MPIDI_CH4U_WIN_POST, &msg, sizeof(msg));
        }

        if (mpi_errno != MPI_SUCCESS)
            MPIR_ERR_SETANDSTMT(mpi_errno, MPI_ERR_RMA_SYNC, goto fn_fail, "**rmasync");
    }

  no_check:
    MPIDI_CH4U_WIN(win, sync).exposure_epoch_type = MPIDI_CH4U_EPOTYPE_POST;
  fn_exit:
    MPL_free(ranks_in_win_grp);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4R_MPI_WIN_POST);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4R_mpi_win_wait
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4R_mpi_win_wait(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Group *group;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4R_MPI_WIN_WAIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4R_MPI_WIN_WAIT);

    MPIDI_CH4U_EXPOSURE_EPOCH_CHECK(win, MPIDI_CH4U_EPOTYPE_POST, mpi_errno, goto fn_fail);
    group = MPIDI_CH4U_WIN(win, sync).pw.group;
    MPIDI_CH4R_PROGRESS_WHILE(group->size != (int) MPIDI_CH4U_WIN(win, sync).sc.count);

    MPIDI_CH4U_WIN(win, sync).sc.count = 0;
    MPIDI_CH4U_WIN(win, sync).pw.group = NULL;
    MPIR_Group_release(group);
    MPIDI_CH4U_WIN(win, sync).exposure_epoch_type = MPIDI_CH4U_EPOTYPE_NONE;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4R_MPI_WIN_WAIT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4R_mpi_win_test
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4R_mpi_win_test(MPIR_Win * win, int *flag)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4R_MPI_WIN_TEST);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4R_MPI_WIN_TEST);

    MPIDI_CH4U_EXPOSURE_EPOCH_CHECK(win, MPIDI_CH4U_EPOTYPE_POST, mpi_errno, goto fn_fail);

    MPIR_Group *group;
    group = MPIDI_CH4U_WIN(win, sync).pw.group;

    if (group->size == (int) MPIDI_CH4U_WIN(win, sync).sc.count) {
        MPIDI_CH4U_WIN(win, sync).sc.count = 0;
        MPIDI_CH4U_WIN(win, sync).pw.group = NULL;
        *flag = 1;
        MPIR_Group_release(group);
        MPIDI_CH4U_WIN(win, sync).exposure_epoch_type = MPIDI_CH4U_EPOTYPE_NONE;
    } else {
        MPID_THREAD_CS_EXIT(VNI, MPIDI_CH4_Global.vni_lock);
        MPIDI_CH4R_PROGRESS();
        MPID_THREAD_CS_ENTER(VNI, MPIDI_CH4_Global.vni_lock);
        *flag = 0;
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4R_MPI_WIN_TEST);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4R_mpi_win_lock
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4R_mpi_win_lock(int lock_type, int rank, int assert, MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    unsigned locked;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4R_MPI_WIN_LOCK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4R_MPI_WIN_LOCK);

    if (rank == MPI_PROC_NULL)
        goto fn_exit0;

    MPIDI_CH4U_LOCK_EPOCH_CHECK_NONE(win, rank, mpi_errno, goto fn_fail);

    MPIDI_CH4U_win_target_t *target_ptr = MPIDI_CH4U_win_target_get(win, rank);

    MPIDI_CH4U_win_target_sync_lock_t *slock = &target_ptr->sync.lock;
    MPIR_Assert(slock->locked == 0);
    if (assert & MPI_MODE_NOCHECK) {
        target_ptr->sync.assert_mode |= MPI_MODE_NOCHECK;
        slock->locked = 1;
        goto no_check;
    }

    MPIDI_CH4U_win_cntrl_msg_t msg;
    msg.win_id = MPIDI_CH4U_WIN(win, win_id);
    msg.origin_rank = win->comm_ptr->rank;
    msg.lock_type = lock_type;

    locked = slock->locked + 1;
#ifndef MPIDI_CH4_DIRECT_NETMOD
    if (MPIDI_CH4_rank_is_local(rank, win->comm_ptr))
        mpi_errno =
            MPIDI_SHM_am_send_hdr(rank, win->comm_ptr, MPIDI_CH4U_WIN_LOCK, &msg, sizeof(msg));
    else
#endif
    {
        mpi_errno =
            MPIDI_NM_am_send_hdr(rank, win->comm_ptr, MPIDI_CH4U_WIN_LOCK, &msg, sizeof(msg));
    }

    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_SETANDSTMT(mpi_errno, MPI_ERR_RMA_SYNC, goto fn_fail, "**rmasync");

    MPIDI_CH4R_PROGRESS_WHILE(slock->locked != locked);
  no_check:
    target_ptr->sync.access_epoch_type = MPIDI_CH4U_EPOTYPE_LOCK;

  fn_exit0:
    MPIDI_CH4U_WIN(win, sync).access_epoch_type = MPIDI_CH4U_EPOTYPE_LOCK;
    MPIDI_CH4U_WIN(win, sync).lock.count++;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4R_MPI_WIN_LOCK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4R_mpi_win_unlock
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4R_mpi_win_unlock(int rank, MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    unsigned unlocked;
    MPIDI_CH4U_win_cntrl_msg_t msg;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4R_MPI_WIN_UNLOCK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4R_MPI_WIN_UNLOCK);

    /* Check window lock epoch.
     * PROC_NULL does not update per-target epoch. */
    MPIDI_CH4U_ACCESS_EPOCH_CHECK(win, MPIDI_CH4U_EPOTYPE_LOCK, mpi_errno, return mpi_errno);
    if (rank == MPI_PROC_NULL)
        goto fn_exit0;

    MPIDI_CH4U_win_target_t *target_ptr = MPIDI_CH4U_win_target_find(win, rank);
    MPIR_Assert(target_ptr);

    /* Check per-target lock epoch */
    MPIDI_CH4U_EPOCH_CHECK_TARGET_LOCK(target_ptr, mpi_errno, return mpi_errno);

    MPIDI_CH4U_win_target_sync_lock_t *slock = &target_ptr->sync.lock;
    /* NOTE: lock blocking waits till granted */
    MPIR_Assert(slock->locked == 1);

    /* Ensure op completion in netmod and shmmod */
    mpi_errno = MPIDI_NM_rma_target_cmpl_hook(rank, win);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);

#ifndef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_SHM_rma_target_cmpl_hook(rank, win);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);
#endif

    /* Ensure completion of AM operations */
    do {
        MPID_THREAD_CS_EXIT(VNI, MPIDI_CH4_Global.vni_lock);
        MPIDI_CH4R_PROGRESS();
        MPID_THREAD_CS_ENTER(VNI, MPIDI_CH4_Global.vni_lock);
    } while (MPIR_cc_get(target_ptr->remote_cmpl_cnts) != 0);

    if (target_ptr->sync.assert_mode & MPI_MODE_NOCHECK) {
        target_ptr->sync.lock.locked = 0;
        goto no_check;
    }

    msg.win_id = MPIDI_CH4U_WIN(win, win_id);
    msg.origin_rank = win->comm_ptr->rank;
    unlocked = slock->locked - 1;

#ifndef MPIDI_CH4_DIRECT_NETMOD
    if (MPIDI_CH4_rank_is_local(rank, win->comm_ptr))
        mpi_errno =
            MPIDI_SHM_am_send_hdr(rank, win->comm_ptr, MPIDI_CH4U_WIN_UNLOCK, &msg, sizeof(msg));
    else
#endif
    {
        mpi_errno =
            MPIDI_NM_am_send_hdr(rank, win->comm_ptr, MPIDI_CH4U_WIN_UNLOCK, &msg, sizeof(msg));
    }

    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_SETANDSTMT(mpi_errno, MPI_ERR_RMA_SYNC, goto fn_fail, "**rmasync");

    MPIDI_CH4R_PROGRESS_WHILE(slock->locked != unlocked);
  no_check:
    /* In performance-efficient mode, all allocated targets are freed at win_finalize. */
    if (MPIR_CVAR_CH4_RMA_MEM_EFFICIENT)
        MPIDI_CH4U_win_target_delete(win, target_ptr);

  fn_exit0:
    MPIR_Assert(MPIDI_CH4U_WIN(win, sync).lock.count > 0);
    MPIDI_CH4U_WIN(win, sync).lock.count--;

    /* Reset window epoch only when all per-target lock epochs are closed. */
    if (MPIDI_CH4U_WIN(win, sync).lock.count == 0) {
        MPIDI_CH4U_WIN(win, sync).access_epoch_type = MPIDI_CH4U_EPOTYPE_NONE;
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4R_MPI_WIN_UNLOCK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;

}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4R_mpi_win_get_info
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4R_mpi_win_get_info(MPIR_Win * win, MPIR_Info ** info_p_p)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4R_MPI_WIN_GET_INFO);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4R_MPI_WIN_GET_INFO);

    mpi_errno = MPIR_Info_alloc(info_p_p);
    if (MPI_SUCCESS != mpi_errno) {
        *info_p_p = NULL;
        MPIR_ERR_POP(mpi_errno);
    }

    if (MPIDI_CH4U_WIN(win, info_args).no_locks)
        mpi_errno = MPIR_Info_set_impl(*info_p_p, "no_locks", "true");
    else
        mpi_errno = MPIR_Info_set_impl(*info_p_p, "no_locks", "false");

    if (MPI_SUCCESS != mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    {
#define BUFSIZE 32
        char buf[BUFSIZE];
        int c = 0;

        CH4_COMPILE_TIME_ASSERT(BUFSIZE >= 16); /* maximum: strlen("rar,raw,war,waw") + 1 */

        if (MPIDI_CH4U_WIN(win, info_args).accumulate_ordering & MPIDI_CH4I_ACCU_ORDER_RAR)
            c += snprintf(buf, BUFSIZE, "rar");

        if (MPIDI_CH4U_WIN(win, info_args).accumulate_ordering & MPIDI_CH4I_ACCU_ORDER_RAW)
            c += snprintf(buf + c, BUFSIZE - c, "%sraw", (c > 0) ? "," : "");

        if (MPIDI_CH4U_WIN(win, info_args).accumulate_ordering & MPIDI_CH4I_ACCU_ORDER_WAR)
            c += snprintf(buf + c, BUFSIZE - c, "%swar", (c > 0) ? "," : "");

        if (MPIDI_CH4U_WIN(win, info_args).accumulate_ordering & MPIDI_CH4I_ACCU_ORDER_WAW)
            c += snprintf(buf + c, BUFSIZE - c, "%swaw", (c > 0) ? "," : "");

        if (c == 0) {
            strncpy(buf, "none", BUFSIZE);
        }

        mpi_errno = MPIR_Info_set_impl(*info_p_p, "accumulate_ordering", buf);
        if (MPI_SUCCESS != mpi_errno)
            MPIR_ERR_POP(mpi_errno);
#undef BUFSIZE
    }

    if (MPIDI_CH4U_WIN(win, info_args).accumulate_ops == MPIDI_CH4I_ACCU_SAME_OP)
        mpi_errno = MPIR_Info_set_impl(*info_p_p, "accumulate_ops", "same_op");
    else
        mpi_errno = MPIR_Info_set_impl(*info_p_p, "accumulate_ops", "same_op_no_op");

    if (MPI_SUCCESS != mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    if (MPIDI_CH4U_WIN(win, info_args).alloc_shared_noncontig)
        mpi_errno = MPIR_Info_set_impl(*info_p_p, "alloc_shared_noncontig", "true");
    else
        mpi_errno = MPIR_Info_set_impl(*info_p_p, "alloc_shared_noncontig", "false");

    if (MPI_SUCCESS != mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    if (MPIDI_CH4U_WIN(win, info_args).same_size)
        mpi_errno = MPIR_Info_set_impl(*info_p_p, "same_size", "true");
    else
        mpi_errno = MPIR_Info_set_impl(*info_p_p, "same_size", "false");

    if (MPI_SUCCESS != mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    if (MPIDI_CH4U_WIN(win, info_args).same_disp_unit)
        mpi_errno = MPIR_Info_set_impl(*info_p_p, "same_disp_unit", "true");
    else
        mpi_errno = MPIR_Info_set_impl(*info_p_p, "same_disp_unit", "false");

    if (MPI_SUCCESS != mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    if (MPIDI_CH4U_WIN(win, info_args).alloc_shm)
        mpi_errno = MPIR_Info_set_impl(*info_p_p, "alloc_shm", "true");
    else
        mpi_errno = MPIR_Info_set_impl(*info_p_p, "alloc_shm", "false");

    if (MPI_SUCCESS != mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    {   /* Keep buf as a local variable for which_accumulate_ops key. */
        char buf[128];
        MPIDI_CH4I_get_info_accu_ops_str(MPIDI_CH4U_WIN(win, info_args).which_accumulate_ops,
                                         &buf[0], sizeof(buf));
        mpi_errno = MPIR_Info_set_impl(*info_p_p, "which_accumulate_ops", buf);
        if (MPI_SUCCESS != mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }

    if (MPIDI_CH4U_WIN(win, info_args).accumulate_noncontig_dtype)
        mpi_errno = MPIR_Info_set_impl(*info_p_p, "accumulate_noncontig_dtype", "true");
    else
        mpi_errno = MPIR_Info_set_impl(*info_p_p, "accumulate_noncontig_dtype", "false");
    if (MPI_SUCCESS != mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    if (MPIDI_CH4U_WIN(win, info_args).accumulate_max_bytes >= 0) {
        char buf[32];           /* make sure 64-bit integer can fit */
        snprintf(buf, sizeof(buf), "%ld",
                 (long) MPIDI_CH4U_WIN(win, info_args).accumulate_max_bytes);
        mpi_errno = MPIR_Info_set_impl(*info_p_p, "accumulate_max_bytes", buf);
    } else
        mpi_errno = MPIR_Info_set_impl(*info_p_p, "accumulate_max_bytes", "unlimited");
    if (MPI_SUCCESS != mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    if (MPIDI_CH4U_WIN(win, info_args).disable_shm_accumulate)
        mpi_errno = MPIR_Info_set_impl(*info_p_p, "disable_shm_accumulate", "true");
    else
        mpi_errno = MPIR_Info_set_impl(*info_p_p, "disable_shm_accumulate", "false");
    if (MPI_SUCCESS != mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4R_MPI_WIN_GET_INFO);
    return mpi_errno;
  fn_fail:
    if (*info_p_p != NULL) {
        MPIR_Info_free(*info_p_p);
        *info_p_p = NULL;
    }
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4R_win_finalize
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4R_win_finalize(MPIR_Win ** win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    int all_completed = 0;
    MPIR_Win *win = *win_ptr;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4R_WIN_FINALIZE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4R_WIN_FINALIZE);

    /* All local outstanding OPs should have been completed. */
    MPIR_Assert(MPIR_cc_get(MPIDI_CH4U_WIN(win, local_cmpl_cnts)) == 0);
    MPIR_Assert(MPIR_cc_get(MPIDI_CH4U_WIN(win, remote_cmpl_cnts)) == 0);

    /* Make progress till all OPs have been completed */
    do {
        int all_local_completed = 0, all_remote_completed = 0;

        MPIDI_CH4R_PROGRESS();

        MPIDI_win_check_all_targets_local_completed(win, &all_local_completed);
        MPIDI_win_check_all_targets_remote_completed(win, &all_remote_completed);

        /* Local completion counter might be updated later than remote completion
         * (at request completion), so we need to check it before release entire
         * window. */
        all_completed = (MPIR_cc_get(MPIDI_CH4U_WIN(win, local_cmpl_cnts)) == 0) &&
            (MPIR_cc_get(MPIDI_CH4U_WIN(win, remote_cmpl_cnts)) == 0) &&
            all_local_completed && all_remote_completed;
    } while (all_completed != 1);

    mpi_errno = MPIDI_NM_mpi_win_free_hook(win);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);

#ifndef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_SHM_mpi_win_free_hook(win);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);
#endif

    MPIDI_CH4U_win_target_cleanall(win);
    MPIDI_CH4U_win_hash_clear(win);

    if (win->create_flavor == MPI_WIN_FLAVOR_ALLOCATE ||
        win->create_flavor == MPI_WIN_FLAVOR_SHARED) {
        /* if more than one process on a node, we always use shared memory */
        if (win->comm_ptr->node_comm != NULL) {
            if (MPIDI_CH4U_WIN(win, mmap_sz) > 0) {
                /* destroy shared window memory */
                mpi_errno = MPIDI_CH4U_destroy_shm_segment(MPIDI_CH4U_WIN(win, mmap_sz),
                                                           &MPIDI_CH4U_WIN(win, shm_segment_handle),
                                                           &MPIDI_CH4U_WIN(win, mmap_addr));
                if (mpi_errno)
                    MPIR_ERR_POP(mpi_errno);
            }

            MPL_free(MPIDI_CH4U_WIN(win, shared_table));
        } else if (MPIDI_CH4U_WIN(win, mmap_sz) > 0) {
            /* if single process on the node, we use mmap with symm heap */
            MPL_munmap(MPIDI_CH4U_WIN(win, mmap_addr), MPIDI_CH4U_WIN(win, mmap_sz), MPL_MEM_RMA);
        } else
            MPL_free(win->base);
    }

    MPIDI_CH4U_map_erase(MPIDI_CH4_Global.win_map, MPIDI_CH4U_WIN(win, win_id));

    MPIR_Comm_release(win->comm_ptr);
    MPIR_Handle_obj_free(&MPIR_Win_mem, win);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4R_WIN_FINALIZE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4R_mpi_win_free
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4R_mpi_win_free(MPIR_Win ** win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    MPIR_Win *win = *win_ptr;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4R_MPI_WIN_FREE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4R_MPI_WIN_FREE);

    MPIDI_CH4U_ACCESS_EPOCH_CHECK_NONE(win, mpi_errno, return mpi_errno);
    MPIDI_CH4U_EXPOSURE_EPOCH_CHECK_NONE(win, mpi_errno, return mpi_errno);

    mpi_errno = MPIR_Barrier(win->comm_ptr, &errflag);
    if (mpi_errno != MPI_SUCCESS)
        goto fn_fail;

    MPIDI_CH4R_win_finalize(win_ptr);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4R_MPI_WIN_FREE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4R_mpi_win_fence
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4R_mpi_win_fence(int massert, MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4R_MPI_WIN_FENCE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4R_MPI_WIN_FENCE);

    MPIDI_CH4U_FENCE_EPOCH_CHECK(win, mpi_errno, goto fn_fail);

    /* Ensure op completion in netmod and shmmod */
    mpi_errno = MPIDI_NM_rma_win_cmpl_hook(win);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);

#ifndef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_SHM_rma_win_cmpl_hook(win);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);
#endif

    /* Ensure completion of AM operations */
    do {
        MPID_THREAD_CS_EXIT(VNI, MPIDI_CH4_Global.vni_lock);
        MPIDI_CH4R_PROGRESS();
        MPID_THREAD_CS_ENTER(VNI, MPIDI_CH4_Global.vni_lock);
    } while (MPIR_cc_get(MPIDI_CH4U_WIN(win, local_cmpl_cnts)) != 0);
    MPIDI_CH4U_EPOCH_FENCE_EVENT(win, massert);

    /*
     * We always make a barrier even if MPI_MODE_NOPRECEDE is specified.
     * This is necessary because we no longer defer executions of RMA ops
     * until synchronization calls as CH3 did. Otherwise, the code like
     * this won't work correctly (cf. f77/rma/wingetf)
     *
     * Rank 0                          Rank 1
     * ----                            ----
     * Store to local mem in window
     * MPI_Win_fence(MODE_NOPRECEDE)   MPI_Win_fence(MODE_NOPRECEDE)
     * MPI_Get(from rank 1)
     */
    /* MPIR_Barrier's state is protected by ALLFUNC_MUTEX.
     * In VNI granularity, individual send/recv/wait operations will take
     * the VNI lock internally. */
    MPID_THREAD_CS_EXIT(VNI, MPIDI_CH4_Global.vni_lock);
    mpi_errno = MPIR_Barrier(win->comm_ptr, &errflag);
    MPID_THREAD_CS_ENTER(VNI, MPIDI_CH4_Global.vni_lock);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4R_MPI_WIN_FENCE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4R_mpi_win_create
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4R_mpi_win_create(void *base,
                                            MPI_Aint length,
                                            int disp_unit,
                                            MPIR_Info * info, MPIR_Comm * comm_ptr,
                                            MPIR_Win ** win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    MPIR_Win *win;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4R_MPI_WIN_CREATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4R_MPI_WIN_CREATE);

    mpi_errno = MPIDI_CH4R_win_init(length,
                                    disp_unit,
                                    win_ptr,
                                    info, comm_ptr, MPI_WIN_FLAVOR_CREATE, MPI_WIN_UNIFIED);

    if (mpi_errno != MPI_SUCCESS)
        goto fn_fail;

    win = *win_ptr;
    win->base = base;

    mpi_errno = MPIDI_NM_mpi_win_create_hook(win);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);

#ifndef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_SHM_mpi_win_create_hook(win);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);
#endif

    mpi_errno = MPIR_Barrier(win->comm_ptr, &errflag);

    if (mpi_errno != MPI_SUCCESS)
        goto fn_fail;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4R_MPI_WIN_CREATE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4R_mpi_win_attach
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4R_mpi_win_attach(MPIR_Win * win, void *base, MPI_Aint size)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4R_MPI_WIN_ATTACH);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4R_MPI_WIN_ATTACH);

    MPIR_ERR_CHKANDSTMT((win->create_flavor != MPI_WIN_FLAVOR_DYNAMIC), mpi_errno,
                        MPI_ERR_RMA_FLAVOR, goto fn_fail, "**rmaflavor");

    mpi_errno = MPIDI_NM_mpi_win_attach_hook(win, base, size);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);

#ifndef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_SHM_mpi_win_attach_hook(win, base, size);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);
#endif

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4R_MPI_WIN_ATTACH);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* Allocate RMA window over shared memory region. Used by both win_allocate
 * and win_allocate_shared.
 *
 * This routine allocates window memory region on each node from shared
 * memory, and initializes the shared_table structure that stores each
 * node process's size, disp_unit, and start address for shm RMA operations
 * and query routine.*/
#undef FUNCNAME
#define FUNCNAME MPIDI_CH4I_win_shm_alloc_impl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4I_win_shm_alloc_impl(MPI_Aint size,
                                                int disp_unit,
                                                MPIR_Comm * comm_ptr,
                                                void **base_ptr, MPIR_Win ** win_ptr)
{
    int i, mpi_errno = MPI_SUCCESS;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    MPIR_Win *win = NULL;
    size_t total_shm_size = 0LL;
    MPIDI_CH4U_win_shared_info_t *shared_table = NULL;
    MPI_Aint *shm_offsets = NULL;
    MPIR_Comm *shm_comm_ptr = comm_ptr->node_comm;
    size_t page_sz = 0, mapsize;
    int mapfail_flag = 0;
    unsigned symheap_flag = 1, global_symheap_flag = 0;

    MPIR_CHKPMEM_DECL(1);
    MPIR_CHKLMEM_DECL(1);
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4I_WIN_SHM_ALLOC_IMPL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4I_WIN_SHM_ALLOC_IMPL);

    if (mpi_errno != MPI_SUCCESS)
        goto fn_fail;

    win = *win_ptr;
    *base_ptr = NULL;

    /* Check whether multiple processes exist on the local node. If so,
     * we need to count the total size on a node for shared memory allocation. */
    if (shm_comm_ptr != NULL) {
        MPIR_T_PVAR_TIMER_START(RMA, rma_wincreate_allgather);
        MPIDI_CH4U_WIN(win, shared_table) =
            (MPIDI_CH4U_win_shared_info_t *) MPL_malloc(sizeof(MPIDI_CH4U_win_shared_info_t) *
                                                        shm_comm_ptr->local_size, MPL_MEM_RMA);
        shared_table = MPIDI_CH4U_WIN(win, shared_table);
        shared_table[shm_comm_ptr->rank].size = size;
        shared_table[shm_comm_ptr->rank].disp_unit = disp_unit;
        shared_table[shm_comm_ptr->rank].shm_base_addr = NULL;

        mpi_errno = MPIR_Allgather(MPI_IN_PLACE,
                                   0,
                                   MPI_DATATYPE_NULL,
                                   shared_table,
                                   sizeof(MPIDI_CH4U_win_shared_info_t), MPI_BYTE, shm_comm_ptr,
                                   &errflag);
        MPIR_T_PVAR_TIMER_END(RMA, rma_wincreate_allgather);
        if (mpi_errno != MPI_SUCCESS)
            goto fn_fail;

        MPIR_CHKLMEM_MALLOC(shm_offsets, MPI_Aint *, shm_comm_ptr->local_size * sizeof(MPI_Aint),
                            mpi_errno, "shm offset", MPL_MEM_RMA);

        /* No allreduce here because this is a shared memory domain
         * and should be a relatively small number of processes
         * and a non performance sensitive API.
         */
        for (i = 0; i < shm_comm_ptr->local_size; i++) {
            shm_offsets[i] = (MPI_Aint) total_shm_size;
            if (MPIDI_CH4U_WIN(win, info_args).alloc_shared_noncontig)
                total_shm_size += MPIDI_CH4R_get_mapsize(shared_table[i].size, &page_sz);
            else
                total_shm_size += shared_table[i].size;
        }

        /* if all processes give zero size on a single node window, simply return. */
        if (total_shm_size == 0 && shm_comm_ptr->local_size == comm_ptr->local_size)
            goto fn_exit;

        /* if my size is not page aligned and noncontig is disabled, skip global symheap. */
        if (size != MPIDI_CH4R_get_mapsize(size, &page_sz) &&
            !MPIDI_CH4U_WIN(win, info_args).alloc_shared_noncontig)
            symheap_flag = 0;
    } else
        total_shm_size = size;

    /* try global symm heap only when multiple processes exist */
    if (comm_ptr->local_size > 1) {
        /* global symm heap can be successful only when any of the following conditions meet.
         * Thus, we can skip unnecessary global symm heap retry based on condition check.
         * - no shared memory node (i.e., single process per node)
         * - size of each process on the shared memory node is page aligned,
         *   thus all process can be assigned to a page aligned start address.
         * - user sets alloc_shared_noncontig=true, thus we can internally make
         *   the size aligned on each process. */
        mpi_errno = MPIR_Allreduce(&symheap_flag, &global_symheap_flag, 1, MPI_UNSIGNED,
                                   MPI_BAND, comm_ptr, &errflag);
        MPIR_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");
    } else
        global_symheap_flag = 0;

    /* because MPI_shm follows a create & attach mode, we need to set the
     * size of entire shared memory segment on each node as the size of
     * each process. */
    mapsize = MPIDI_CH4R_get_mapsize(total_shm_size, &page_sz);
    MPIDI_CH4U_WIN(win, mmap_sz) = mapsize;

    /* first try global symmetric heap segment allocation */
    if (global_symheap_flag) {
        mpi_errno = MPIDI_CH4R_get_shm_symheap(mapsize, shm_offsets, comm_ptr, win, &mapfail_flag);
        if (mpi_errno != MPI_SUCCESS)
            goto fn_fail;
    }

    /* if fails, try normal shm segment allocation or malloc */
    if (!global_symheap_flag || mapfail_flag) {
        if (shm_comm_ptr != NULL && mapsize) {
            mpi_errno = MPIDI_CH4U_allocate_shm_segment(shm_comm_ptr, mapsize,
                                                        &MPIDI_CH4U_WIN(win, shm_segment_handle),
                                                        &MPIDI_CH4U_WIN(win, mmap_addr));
            if (mpi_errno != MPI_SUCCESS)
                goto fn_fail;
        } else if (size > 0) {
            MPIR_CHKPMEM_MALLOC(*base_ptr, void *, size, mpi_errno, "(*win_ptr)->base",
                                MPL_MEM_RMA);
            MPL_VG_MEM_INIT(*base_ptr, size);
            MPIDI_CH4U_WIN(win, mmap_sz) = 0;   /* reset mmap_sz if use malloc */
        }
    }

    /* compute the base addresses of each process within the shared memory segment */
    if (shm_comm_ptr != NULL) {
        char *cur_base = (char *) MPIDI_CH4U_WIN(win, mmap_addr);
        for (i = 0; i < shm_comm_ptr->local_size; i++) {
            if (shared_table[i].size)
                shared_table[i].shm_base_addr = cur_base;
            else
                shared_table[i].shm_base_addr = NULL;

            if (MPIDI_CH4U_WIN(win, info_args).alloc_shared_noncontig)
                cur_base += MPIDI_CH4R_get_mapsize(shared_table[i].size, &page_sz);
            else
                cur_base += shared_table[i].size;
        }

        *base_ptr = shared_table[shm_comm_ptr->rank].shm_base_addr;
    } else if (MPIDI_CH4U_WIN(win, mmap_sz) > 0) {
        /* if symm heap is allocated without shared memory, use the mapping address */
        *base_ptr = MPIDI_CH4U_WIN(win, mmap_addr);
    }
    /* otherwise, it has already be assigned with a local memory region or NULL (zero size). */

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4I_WIN_SHM_ALLOC_IMPL);
    return mpi_errno;
  fn_fail:
    MPIR_CHKPMEM_REAP();
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4R_mpi_win_allocate_shared
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4R_mpi_win_allocate_shared(MPI_Aint size,
                                                     int disp_unit,
                                                     MPIR_Info * info_ptr,
                                                     MPIR_Comm * comm_ptr,
                                                     void **base_ptr, MPIR_Win ** win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    MPIR_Win *win = NULL;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4R_MPI_WIN_ALLOCATE_SHARED);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4R_MPI_WIN_ALLOCATE_SHARED);

    mpi_errno = MPIDI_CH4R_win_init(size, disp_unit, win_ptr, info_ptr, comm_ptr,
                                    MPI_WIN_FLAVOR_SHARED, MPI_WIN_UNIFIED);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);

    mpi_errno = MPIDI_CH4I_win_shm_alloc_impl(size, disp_unit, comm_ptr, base_ptr, win_ptr);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);

    win = *win_ptr;
    win->base = *base_ptr;
    win->size = size;

    mpi_errno = MPIDI_NM_mpi_win_allocate_shared_hook(win);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);

#ifndef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_SHM_mpi_win_allocate_shared_hook(win);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);
#endif

    mpi_errno = MPIR_Barrier(comm_ptr, &errflag);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4R_MPI_WIN_ALLOCATE_SHARED);
    return mpi_errno;
  fn_fail:
    if (win_ptr)
        MPIDI_CH4R_win_finalize(win_ptr);
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4R_mpi_win_detach
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4R_mpi_win_detach(MPIR_Win * win, const void *base)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4R_MPI_WIN_DETACH);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4R_MPI_WIN_DETACH);
    MPIR_ERR_CHKANDSTMT((win->create_flavor != MPI_WIN_FLAVOR_DYNAMIC), mpi_errno,
                        MPI_ERR_RMA_FLAVOR, goto fn_fail, "**rmaflavor");

    mpi_errno = MPIDI_NM_mpi_win_detach_hook(win, base);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);

#ifndef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_SHM_mpi_win_detach_hook(win, base);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);
#endif

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4R_MPI_WIN_DETACH);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4R_mpi_win_shared_query
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4R_mpi_win_shared_query(MPIR_Win * win,
                                                  int rank,
                                                  MPI_Aint * size, int *disp_unit, void *baseptr)
{
    int mpi_errno = MPI_SUCCESS;
    int offset = rank;
    MPIDI_CH4U_win_shared_info_t *shared_table = MPIDI_CH4U_WIN(win, shared_table);

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4R_MPI_WIN_SHARED_QUERY);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4R_MPI_WIN_SHARED_QUERY);

    /* When only single process exists on the node, should only query
     * MPI_PROC_NULL or local process. Thus, return local window's info. */
    if (win->comm_ptr->node_comm == NULL) {
        *size = win->size;
        *disp_unit = win->disp_unit;
        *((void **) baseptr) = win->base;
        goto fn_exit;
    }

    /* When rank is MPI_PROC_NULL, return the memory region belonging the lowest
     * rank that specified size > 0*/
    if (rank == MPI_PROC_NULL) {
        /* Default, if no process has size > 0. */
        *size = 0;
        *disp_unit = 0;
        *((void **) baseptr) = NULL;

        for (offset = 0; offset < win->comm_ptr->local_size; offset++) {
            if (shared_table[offset].size > 0) {
                *size = shared_table[offset].size;
                *disp_unit = shared_table[offset].disp_unit;
                *((void **) baseptr) = shared_table[offset].shm_base_addr;
                break;
            }
        }
    } else {
        *size = shared_table[offset].size;
        *disp_unit = shared_table[offset].disp_unit;
        *(void **) baseptr = shared_table[offset].shm_base_addr;
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4R_MPI_WIN_SHARED_QUERY);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4R_mpi_win_allocate
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4R_mpi_win_allocate(MPI_Aint size,
                                              int disp_unit,
                                              MPIR_Info * info,
                                              MPIR_Comm * comm, void *baseptr, MPIR_Win ** win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    MPIR_Win *win;
    void **base_ptr = (void **) baseptr;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4R_MPI_WIN_ALLOCATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4R_MPI_WIN_ALLOCATE);

    mpi_errno = MPIDI_CH4R_win_init(size, disp_unit, win_ptr, info, comm,
                                    MPI_WIN_FLAVOR_ALLOCATE, MPI_WIN_UNIFIED);

    if (mpi_errno != MPI_SUCCESS)
        goto fn_fail;

    mpi_errno = MPIDI_CH4I_win_shm_alloc_impl(size, disp_unit, comm, base_ptr, win_ptr);
    if (mpi_errno != MPI_SUCCESS)
        goto fn_fail;

    win = *win_ptr;
    win->base = *(void **) baseptr;
    win->size = size;

    mpi_errno = MPIDI_NM_mpi_win_allocate_hook(win);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);

#ifndef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_SHM_mpi_win_allocate_hook(win);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);
#endif

    mpi_errno = MPIR_Barrier(comm, &errflag);

    if (mpi_errno != MPI_SUCCESS)
        goto fn_fail;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4R_MPI_WIN_ALLOCATE);
    return mpi_errno;
  fn_fail:
    if (win_ptr)
        MPIDI_CH4R_win_finalize(win_ptr);
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4R_mpi_win_flush
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4R_mpi_win_flush(int rank, MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4R_MPI_WIN_FLUSH);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4R_MPI_WIN_FLUSH);

    /* Check window lock epoch.
     * PROC_NULL does not update per-target epoch. */
    MPIDI_CH4U_EPOCH_CHECK_PASSIVE(win, mpi_errno, return mpi_errno);
    if (rank == MPI_PROC_NULL)
        goto fn_exit;

    /* Ensure op completion in netmod and shmmod */
    mpi_errno = MPIDI_NM_rma_target_cmpl_hook(rank, win);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);

#ifndef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_SHM_rma_target_cmpl_hook(rank, win);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);
#endif

    /* Ensure completion of AM operations issued to the target.
     * If target object is not created (e.g., when all operations issued
     * to the target were via shm and in lockall), we also need trigger
     * progress once to handle remote AM. */
    MPIDI_CH4U_win_target_t *target_ptr = MPIDI_CH4U_win_target_find(win, rank);
    if (target_ptr) {
        if (MPIDI_CH4U_WIN(win, sync).access_epoch_type == MPIDI_CH4U_EPOTYPE_LOCK)
            MPIDI_CH4U_EPOCH_CHECK_TARGET_LOCK(target_ptr, mpi_errno, goto fn_fail);
    }

    do {
        MPID_THREAD_CS_EXIT(VNI, MPIDI_CH4_Global.vni_lock);
        MPIDI_CH4R_PROGRESS();
        MPID_THREAD_CS_ENTER(VNI, MPIDI_CH4_Global.vni_lock);
    } while (target_ptr && MPIR_cc_get(target_ptr->remote_cmpl_cnts) != 0);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4R_MPI_WIN_FLUSH);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4R_mpi_win_flush_local_all
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4R_mpi_win_flush_local_all(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    int all_local_completed = 0;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4R_MPI_WIN_FLUSH_LOCAL_ALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4R_MPI_WIN_FLUSH_LOCAL_ALL);

    MPIDI_CH4U_EPOCH_CHECK_PASSIVE(win, mpi_errno, goto fn_fail);

    /* Ensure op local completion in netmod and shmmod */
    mpi_errno = MPIDI_NM_rma_win_local_cmpl_hook(win);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);

#ifndef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_SHM_rma_win_local_cmpl_hook(win);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);
#endif

    /* Ensure completion of AM operations */

    /* FIXME: now we simply set per-target counters for lockall in case
     * user flushes per target, but this should be optimized. */
    do {
        MPID_THREAD_CS_EXIT(VNI, MPIDI_CH4_Global.vni_lock);
        MPIDI_CH4R_PROGRESS();
        MPID_THREAD_CS_ENTER(VNI, MPIDI_CH4_Global.vni_lock);
        MPIDI_win_check_all_targets_local_completed(win, &all_local_completed);
    } while (all_local_completed != 1);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4R_MPI_WIN_FLUSH_LOCAL_ALL);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4R_mpi_win_unlock_all
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4R_mpi_win_unlock_all(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4R_MPI_WIN_UNLOCK_ALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4R_MPI_WIN_UNLOCK_ALL);
    int i;

    int all_remote_completed = 0;

    MPIDI_CH4U_ACCESS_EPOCH_CHECK(win, MPIDI_CH4U_EPOTYPE_LOCK_ALL, mpi_errno, return mpi_errno);
    /* NOTE: lockall blocking waits till all locks granted */
    MPIR_Assert(MPIDI_CH4U_WIN(win, sync).lockall.allLocked == win->comm_ptr->local_size);

    /* Ensure op completion in netmod and shmmod */
    mpi_errno = MPIDI_NM_rma_win_cmpl_hook(win);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);

#ifndef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_SHM_rma_win_cmpl_hook(win);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);
#endif

    /* Ensure completion of AM operations */

    /* FIXME: now we simply set per-target counters for lockall in case
     * user flushes per target, but this should be optimized. */
    do {
        MPID_THREAD_CS_EXIT(VNI, MPIDI_CH4_Global.vni_lock);
        MPIDI_CH4R_PROGRESS();
        MPID_THREAD_CS_ENTER(VNI, MPIDI_CH4_Global.vni_lock);
        MPIDI_win_check_all_targets_remote_completed(win, &all_remote_completed);
    } while (all_remote_completed != 1);

    if (MPIDI_CH4U_WIN(win, sync).assert_mode & MPI_MODE_NOCHECK) {
        MPIDI_CH4U_WIN(win, sync).lockall.allLocked = 0;
        goto no_check;
    }
    for (i = 0; i < win->comm_ptr->local_size; i++) {
        MPIDI_CH4U_win_cntrl_msg_t msg;
        msg.win_id = MPIDI_CH4U_WIN(win, win_id);
        msg.origin_rank = win->comm_ptr->rank;

#ifndef MPIDI_CH4_DIRECT_NETMOD
        if (MPIDI_CH4_rank_is_local(i, win->comm_ptr))
            mpi_errno = MPIDI_SHM_am_send_hdr(i, win->comm_ptr,
                                              MPIDI_CH4U_WIN_UNLOCKALL, &msg, sizeof(msg));
        else
#endif
        {
            mpi_errno = MPIDI_NM_am_send_hdr(i, win->comm_ptr,
                                             MPIDI_CH4U_WIN_UNLOCKALL, &msg, sizeof(msg));
        }

        if (mpi_errno != MPI_SUCCESS)
            MPIR_ERR_SETANDSTMT(mpi_errno, MPI_ERR_RMA_SYNC, goto fn_fail, "**rmasync");
    }

    MPIDI_CH4R_PROGRESS_WHILE(MPIDI_CH4U_WIN(win, sync).lockall.allLocked);
  no_check:
    /* In performance-efficient mode, all allocated targets are freed at win_finalize. */
    if (MPIR_CVAR_CH4_RMA_MEM_EFFICIENT)
        MPIDI_CH4U_win_target_cleanall(win);
    MPIDI_CH4U_WIN(win, sync).access_epoch_type = MPIDI_CH4U_EPOTYPE_NONE;
    MPIDI_CH4U_WIN(win, sync).assert_mode = 0;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4R_MPI_WIN_UNLOCK_ALL);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4R_mpi_win_create_dynamic
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4R_mpi_win_create_dynamic(MPIR_Info * info,
                                                    MPIR_Comm * comm, MPIR_Win ** win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    int rc = MPI_SUCCESS;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4R_MPI_WIN_CREATE_DYNAMIC);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4R_MPI_WIN_CREATE_DYNAMIC);

    MPIR_Win *win;

    rc = MPIDI_CH4R_win_init(0, 1, win_ptr, info, comm, MPI_WIN_FLAVOR_DYNAMIC, MPI_WIN_UNIFIED);

    if (rc != MPI_SUCCESS)
        goto fn_fail;

    win = *win_ptr;
    win->base = MPI_BOTTOM;

    mpi_errno = MPIDI_NM_mpi_win_create_dynamic_hook(win);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);

#ifndef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_SHM_mpi_win_create_dynamic_hook(win);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);
#endif

    mpi_errno = MPIR_Barrier(comm, &errflag);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4R_MPI_WIN_CREATE_DYNAMIC);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4R_mpi_win_flush_local
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4R_mpi_win_flush_local(int rank, MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4R_MPI_WIN_FLUSH_LOCAL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4R_MPI_WIN_FLUSH_LOCAL);

    /* Check window lock epoch.
     * PROC_NULL does not update per-target epoch. */
    MPIDI_CH4U_EPOCH_CHECK_PASSIVE(win, mpi_errno, return mpi_errno);
    if (rank == MPI_PROC_NULL)
        goto fn_exit;

    /* Ensure op local completion in netmod and shmmod */
    mpi_errno = MPIDI_NM_rma_target_local_cmpl_hook(rank, win);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);

#ifndef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_SHM_rma_target_local_cmpl_hook(rank, win);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);
#endif

    /* Ensure completion of AM operations issued to the target.
     * If target object is not created (e.g., when all operations issued
     * to the target were via shm and in lockall), we also need trigger
     * progress once to handle remote AM. */
    MPIDI_CH4U_win_target_t *target_ptr = MPIDI_CH4U_win_target_find(win, rank);
    if (target_ptr) {
        if (MPIDI_CH4U_WIN(win, sync).access_epoch_type == MPIDI_CH4U_EPOTYPE_LOCK)
            MPIDI_CH4U_EPOCH_CHECK_TARGET_LOCK(target_ptr, mpi_errno, goto fn_fail);
    }

    do {
        MPID_THREAD_CS_EXIT(VNI, MPIDI_CH4_Global.vni_lock);
        MPIDI_CH4R_PROGRESS();
        MPID_THREAD_CS_ENTER(VNI, MPIDI_CH4_Global.vni_lock);
    } while (target_ptr && MPIR_cc_get(target_ptr->local_cmpl_cnts) != 0);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4R_MPI_WIN_FLUSH_LOCAL);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4R_mpi_win_sync
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4R_mpi_win_sync(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4R_MPI_WIN_SYNC);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4R_MPI_WIN_SYNC);

    MPIDI_CH4U_EPOCH_CHECK_PASSIVE(win, mpi_errno, goto fn_fail);
    OPA_read_write_barrier();

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4R_MPI_WIN_SYNC);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4R_mpi_win_flush_all
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4R_mpi_win_flush_all(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    int all_remote_completed = 0;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4R_MPI_WIN_FLUSH_ALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4R_MPI_WIN_FLUSH_ALL);

    MPIDI_CH4U_EPOCH_CHECK_PASSIVE(win, mpi_errno, goto fn_fail);

    /* Ensure op completion in netmod and shmmod */
    mpi_errno = MPIDI_NM_rma_win_cmpl_hook(win);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);

#ifndef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_SHM_rma_win_cmpl_hook(win);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);
#endif

    /* Ensure completion of AM operations */
    do {
        MPID_THREAD_CS_EXIT(VNI, MPIDI_CH4_Global.vni_lock);
        MPIDI_CH4R_PROGRESS();
        MPID_THREAD_CS_ENTER(VNI, MPIDI_CH4_Global.vni_lock);

        /* FIXME: now we simply set per-target counters for lockall in case
         * user flushes per target, but this should be optimized. */
        MPIDI_win_check_all_targets_remote_completed(win, &all_remote_completed);
    } while (all_remote_completed != 1);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4R_MPI_WIN_FLUSH_ALL);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4R_mpi_win_lock_all
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4R_mpi_win_lock_all(int assert, MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4R_MPI_WIN_LOCK_ALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4R_MPI_WIN_LOCK_ALL);

    MPIDI_CH4U_ACCESS_EPOCH_CHECK_NONE(win, mpi_errno, goto fn_fail);

    MPIR_Assert(MPIDI_CH4U_WIN(win, sync).lockall.allLocked == 0);

    int size;
    size = win->comm_ptr->local_size;
    if (assert & MPI_MODE_NOCHECK) {
        MPIDI_CH4U_WIN(win, sync).assert_mode |= MPI_MODE_NOCHECK;
        MPIDI_CH4U_WIN(win, sync).lockall.allLocked = size;
        goto no_check;
    }

    int i;
    for (i = 0; i < size; i++) {
        MPIDI_CH4U_win_cntrl_msg_t msg;
        msg.win_id = MPIDI_CH4U_WIN(win, win_id);
        msg.origin_rank = win->comm_ptr->rank;
        msg.lock_type = MPI_LOCK_SHARED;

#ifndef MPIDI_CH4_DIRECT_NETMOD
        if (MPIDI_CH4_rank_is_local(i, win->comm_ptr))
            mpi_errno = MPIDI_SHM_am_send_hdr(i, win->comm_ptr,
                                              MPIDI_CH4U_WIN_LOCKALL, &msg, sizeof(msg));
        else
#endif
        {
            mpi_errno = MPIDI_NM_am_send_hdr(i, win->comm_ptr,
                                             MPIDI_CH4U_WIN_LOCKALL, &msg, sizeof(msg));
        }

        if (mpi_errno != MPI_SUCCESS)
            MPIR_ERR_SETANDSTMT(mpi_errno, MPI_ERR_RMA_SYNC, goto fn_fail, "**rmasync");
    }

    MPIDI_CH4R_PROGRESS_WHILE(size != (int) MPIDI_CH4U_WIN(win, sync).lockall.allLocked);
  no_check:
    MPIDI_CH4U_WIN(win, sync).access_epoch_type = MPIDI_CH4U_EPOTYPE_LOCK_ALL;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4R_MPI_WIN_LOCK_ALL);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* CH4R_WIN_H_INCLUDED */

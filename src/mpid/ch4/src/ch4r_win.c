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
#include "mpidch4r.h"
#include "ch4r_win.h"

enum {
    SHM_WIN_OPTIONAL,
    SHM_WIN_REQUIRED,
};

static void parse_info_accu_ops_str(const char *str, uint32_t * ops_ptr);
static void get_info_accu_ops_str(uint32_t val, char *buf, size_t maxlen);
static int win_set_info(MPIR_Win * win, MPIR_Info * info, bool is_init);
static int win_init(MPI_Aint length, int disp_unit, MPIR_Win ** win_ptr, MPIR_Info * info,
                    MPIR_Comm * comm_ptr, int create_flavor, int model);
static int win_finalize(MPIR_Win ** win_ptr);
static int win_shm_alloc_impl(MPI_Aint size, int disp_unit, MPIR_Comm * comm_ptr, void **base_ptr,
                              MPIR_Win ** win_ptr, int shm_option);

static void parse_info_accu_ops_str(const char *str, uint32_t * ops_ptr)
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
        MPIDIG_win_info_accu_op_shift_t op_shift;
        /* add all ops */
        for (op_shift = MPIDIG_ACCU_OP_SHIFT_FIRST; op_shift < MPIDIG_ACCU_OP_SHIFT_LAST;
             op_shift++)
            ops |= (1 << op_shift);
        *ops_ptr = ops;
        return;
    }

    token = (char *) strtok_r(value, ",", &savePtr);
    while (token != NULL) {

        /* traverse op list (exclude null and last) and add the op if set */
        if (!strncmp(token, "max", strlen("max")))
            ops |= (1 << MPIDIG_ACCU_MAX_SHIFT);
        else if (!strncmp(token, "min", strlen("min")))
            ops |= (1 << MPIDIG_ACCU_MIN_SHIFT);
        else if (!strncmp(token, "sum", strlen("sum")))
            ops |= (1 << MPIDIG_ACCU_SUM_SHIFT);
        else if (!strncmp(token, "prod", strlen("prod")))
            ops |= (1 << MPIDIG_ACCU_PROD_SHIFT);
        else if (!strncmp(token, "maxloc", strlen("maxloc")))
            ops |= (1 << MPIDIG_ACCU_MAXLOC_SHIFT);
        else if (!strncmp(token, "minloc", strlen("minloc")))
            ops |= (1 << MPIDIG_ACCU_MINLOC_SHIFT);
        else if (!strncmp(token, "band", strlen("band")))
            ops |= (1 << MPIDIG_ACCU_BAND_SHIFT);
        else if (!strncmp(token, "bor", strlen("bor")))
            ops |= (1 << MPIDIG_ACCU_BOR_SHIFT);
        else if (!strncmp(token, "bxor", strlen("bxor")))
            ops |= (1 << MPIDIG_ACCU_BXOR_SHIFT);
        else if (!strncmp(token, "land", strlen("land")))
            ops |= (1 << MPIDIG_ACCU_LAND_SHIFT);
        else if (!strncmp(token, "lor", strlen("lor")))
            ops |= (1 << MPIDIG_ACCU_LOR_SHIFT);
        else if (!strncmp(token, "lxor", strlen("lxor")))
            ops |= (1 << MPIDIG_ACCU_LXOR_SHIFT);
        else if (!strncmp(token, "replace", strlen("replace")))
            ops |= (1 << MPIDIG_ACCU_REPLACE_SHIFT);
        else if (!strncmp(token, "no_op", strlen("no_op")))
            ops |= (1 << MPIDIG_ACCU_NO_OP_SHIFT);
        else if (!strncmp(token, "cswap", strlen("cswap")) ||
                 !strncmp(token, "compare_and_swap", strlen("compare_and_swap")))
            ops |= (1 << MPIDIG_ACCU_CSWAP_SHIFT);

        token = (char *) strtok_r(NULL, ",", &savePtr);
    }

    /* update info only when any valid value is set */
    if (ops)
        *ops_ptr = ops;
}

static void get_info_accu_ops_str(uint32_t val, char *buf, size_t maxlen)
{
    int c = 0;

    MPIR_Assert(maxlen >= strlen("max,min,sum,prod,maxloc,minloc,band,bor,"
                                 "bxor,land,lor,lxor,replace,no_op,cswap") + 1);

    if (val & (1 << MPIDIG_ACCU_MAX_SHIFT))
        c += snprintf(buf + c, maxlen - c, "max");
    if (val & (1 << MPIDIG_ACCU_MIN_SHIFT))
        c += snprintf(buf + c, maxlen - c, "%smin", (c > 0) ? "," : "");
    if (val & (1 << MPIDIG_ACCU_SUM_SHIFT))
        c += snprintf(buf + c, maxlen - c, "%ssum", (c > 0) ? "," : "");
    if (val & (1 << MPIDIG_ACCU_PROD_SHIFT))
        c += snprintf(buf + c, maxlen - c, "%sprod", (c > 0) ? "," : "");
    if (val & (1 << MPIDIG_ACCU_MAXLOC_SHIFT))
        c += snprintf(buf + c, maxlen - c, "%smaxloc", (c > 0) ? "," : "");
    if (val & (1 << MPIDIG_ACCU_MINLOC_SHIFT))
        c += snprintf(buf + c, maxlen - c, "%sminloc", (c > 0) ? "," : "");
    if (val & (1 << MPIDIG_ACCU_BAND_SHIFT))
        c += snprintf(buf + c, maxlen - c, "%sband", (c > 0) ? "," : "");
    if (val & (1 << MPIDIG_ACCU_BOR_SHIFT))
        c += snprintf(buf + c, maxlen - c, "%sbor", (c > 0) ? "," : "");
    if (val & (1 << MPIDIG_ACCU_BXOR_SHIFT))
        c += snprintf(buf + c, maxlen - c, "%sbxor", (c > 0) ? "," : "");
    if (val & (1 << MPIDIG_ACCU_LAND_SHIFT))
        c += snprintf(buf + c, maxlen - c, "%sland", (c > 0) ? "," : "");
    if (val & (1 << MPIDIG_ACCU_LOR_SHIFT))
        c += snprintf(buf + c, maxlen - c, "%slor", (c > 0) ? "," : "");
    if (val & (1 << MPIDIG_ACCU_LXOR_SHIFT))
        c += snprintf(buf + c, maxlen - c, "%slxor", (c > 0) ? "," : "");
    if (val & (1 << MPIDIG_ACCU_REPLACE_SHIFT))
        c += snprintf(buf + c, maxlen - c, "%sreplace", (c > 0) ? "," : "");
    if (val & (1 << MPIDIG_ACCU_NO_OP_SHIFT))
        c += snprintf(buf + c, maxlen - c, "%sno_op", (c > 0) ? "," : "");
    if (val & (1 << MPIDIG_ACCU_CSWAP_SHIFT))
        c += snprintf(buf + c, maxlen - c, "%scswap", (c > 0) ? "," : "");

    if (c == 0)
        strncpy(buf, "none", maxlen);
}

static int win_set_info(MPIR_Win * win, MPIR_Info * info, bool is_init)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_WIN_SET_INFO);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_WIN_SET_INFO);

    MPIR_Info *curr_ptr;
    char *value, *token, *savePtr = NULL;
    int save_ordering;

    curr_ptr = info->next;

    while (curr_ptr) {
        if (!strcmp(curr_ptr->key, "no_locks")) {
            if (!strcmp(curr_ptr->value, "true"))
                MPIDIG_WIN(win, info_args).no_locks = 1;
            else if (!strcmp(curr_ptr->value, "false"))
                MPIDIG_WIN(win, info_args).no_locks = 0;
        } else if (!strcmp(curr_ptr->key, "accumulate_ordering")) {
            save_ordering = MPIDIG_WIN(win, info_args).accumulate_ordering;
            MPIDIG_WIN(win, info_args).accumulate_ordering = 0;
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
                    MPIDIG_WIN(win, info_args).accumulate_ordering =
                        (MPIDIG_WIN(win, info_args).accumulate_ordering | MPIDIG_ACCU_ORDER_RAR);
                else if (!memcmp(token, "raw", 3))
                    MPIDIG_WIN(win, info_args).accumulate_ordering =
                        (MPIDIG_WIN(win, info_args).accumulate_ordering | MPIDIG_ACCU_ORDER_RAW);
                else if (!memcmp(token, "war", 3))
                    MPIDIG_WIN(win, info_args).accumulate_ordering =
                        (MPIDIG_WIN(win, info_args).accumulate_ordering | MPIDIG_ACCU_ORDER_WAR);
                else if (!memcmp(token, "waw", 3))
                    MPIDIG_WIN(win, info_args).accumulate_ordering =
                        (MPIDIG_WIN(win, info_args).accumulate_ordering | MPIDIG_ACCU_ORDER_WAW);
                else
                    MPIR_ERR_SETANDSTMT(mpi_errno, MPI_ERR_ARG, goto fn_fail, "**info");

                token = (char *) strtok_r(NULL, ",", &savePtr);
            }

            if (MPIDIG_WIN(win, info_args).accumulate_ordering == 0)
                MPIDIG_WIN(win, info_args).accumulate_ordering = save_ordering;
        } else if (!strcmp(curr_ptr->key, "accumulate_ops")) {
            if (!strcmp(curr_ptr->value, "same_op"))
                MPIDIG_WIN(win, info_args).accumulate_ops = MPIDIG_ACCU_SAME_OP;
            else if (!strcmp(curr_ptr->value, "same_op_no_op"))
                MPIDIG_WIN(win, info_args).accumulate_ops = MPIDIG_ACCU_SAME_OP_NO_OP;
        } else if (!strcmp(curr_ptr->key, "same_disp_unit")) {
            if (!strcmp(curr_ptr->value, "true"))
                MPIDIG_WIN(win, info_args).same_disp_unit = 1;
            else if (!strcmp(curr_ptr->value, "false"))
                MPIDIG_WIN(win, info_args).same_disp_unit = 0;
        } else if (!strcmp(curr_ptr->key, "same_size")) {
            if (!strcmp(curr_ptr->value, "true"))
                MPIDIG_WIN(win, info_args).same_size = 1;
            else if (!strcmp(curr_ptr->value, "false"))
                MPIDIG_WIN(win, info_args).same_size = 0;
        } else if (!strcmp(curr_ptr->key, "alloc_shared_noncontig")) {
            if (!strcmp(curr_ptr->value, "true"))
                MPIDIG_WIN(win, info_args).alloc_shared_noncontig = 1;
            else if (!strcmp(curr_ptr->value, "false"))
                MPIDIG_WIN(win, info_args).alloc_shared_noncontig = 0;
        } else if (!strcmp(curr_ptr->key, "alloc_shm")) {
            if (!strcmp(curr_ptr->value, "true"))
                MPIDIG_WIN(win, info_args).alloc_shm = 1;
            else if (!strcmp(curr_ptr->value, "false"))
                MPIDIG_WIN(win, info_args).alloc_shm = 0;
        }
        /* We allow the user to set the following atomics hint only at window init time,
         * all future updates by win_set_info are ignored. This is because we do not
         * have a good way to ensure all outstanding atomic ops have been completed
         * on all processes especially in passive-target epochs. */
        else if (is_init && !strcmp(curr_ptr->key, "which_accumulate_ops")) {
            parse_info_accu_ops_str(curr_ptr->value,
                                    &MPIDIG_WIN(win, info_args).which_accumulate_ops);
        } else if (is_init && !strcmp(curr_ptr->key, "accumulate_noncontig_dtype")) {
            if (!strcmp(curr_ptr->value, "true"))
                MPIDIG_WIN(win, info_args).accumulate_noncontig_dtype = true;
            else if (!strcmp(curr_ptr->value, "false"))
                MPIDIG_WIN(win, info_args).accumulate_noncontig_dtype = false;
        } else if (is_init && !strcmp(curr_ptr->key, "accumulate_max_bytes")) {
            if (!strcmp(curr_ptr->value, "unlimited") || !strcmp(curr_ptr->value, "-1"))
                MPIDIG_WIN(win, info_args).accumulate_max_bytes = -1;
            else {
                long max_bytes = atol(curr_ptr->value);
                if (max_bytes >= 0)
                    MPIDIG_WIN(win, info_args).accumulate_max_bytes = max_bytes;
            }
        } else if (is_init && !strcmp(curr_ptr->key, "disable_shm_accumulate")) {
            if (!strcmp(curr_ptr->value, "true"))
                MPIDIG_WIN(win, info_args).disable_shm_accumulate = true;
            else
                MPIDIG_WIN(win, info_args).disable_shm_accumulate = false;
        }
      next:
        curr_ptr = curr_ptr->next;
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_WIN_SET_INFO);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int win_init(MPI_Aint length, int disp_unit, MPIR_Win ** win_ptr, MPIR_Info * info,
                    MPIR_Comm * comm_ptr, int create_flavor, int model)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Win *win = (MPIR_Win *) MPIR_Handle_obj_alloc(&MPIR_Win_mem);
    MPIDIG_win_target_t *targets = NULL;
    MPIR_Comm *win_comm_ptr;
    MPIDIG_win_info_accu_op_shift_t op_shift;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_WIN_INIT);
    MPIR_FUNC_VERBOSE_RMA_ENTER(MPID_STATE_MPIDIG_WIN_INIT);

    MPIR_ERR_CHKANDSTMT(win == NULL, mpi_errno, MPI_ERR_NO_MEM, goto fn_fail, "**nomem");
    *win_ptr = win;

    memset(&win->dev.am, 0, sizeof(MPIDIG_win_t));

    /* Duplicate the original communicator here to avoid having collisions
     * between internal collectives */
    mpi_errno = MPIR_Comm_dup_impl(comm_ptr, NULL, &win_comm_ptr);
    if (MPI_SUCCESS != mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    MPIDIG_WIN(win, targets) = targets;

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
    MPIDIG_WIN(win, shared_table) = NULL;
    MPIDIG_WIN(win, sync).assert_mode = 0;
    MPIDIG_WIN(win, shm_allocated) = 0;

    /* Initialize the info (hint) flags per window */
    MPIDIG_WIN(win, info_args).no_locks = 0;
    MPIDIG_WIN(win, info_args).accumulate_ordering = (MPIDIG_ACCU_ORDER_RAR |
                                                      MPIDIG_ACCU_ORDER_RAW |
                                                      MPIDIG_ACCU_ORDER_WAR |
                                                      MPIDIG_ACCU_ORDER_WAW);
    MPIDIG_WIN(win, info_args).accumulate_ops = MPIDIG_ACCU_SAME_OP_NO_OP;
    MPIDIG_WIN(win, info_args).same_size = 0;
    MPIDIG_WIN(win, info_args).same_disp_unit = 0;
    MPIDIG_WIN(win, info_args).alloc_shared_noncontig = 0;
    if (win->create_flavor == MPI_WIN_FLAVOR_ALLOCATE
        || win->create_flavor == MPI_WIN_FLAVOR_SHARED) {
        MPIDIG_WIN(win, info_args).alloc_shm = 1;
    } else {
        MPIDIG_WIN(win, info_args).alloc_shm = 0;
    }

    /* default any op */
    MPIDIG_WIN(win, info_args).which_accumulate_ops = 0;
    for (op_shift = MPIDIG_ACCU_OP_SHIFT_FIRST; op_shift < MPIDIG_ACCU_OP_SHIFT_LAST; op_shift++)
        MPIDIG_WIN(win, info_args).which_accumulate_ops |= (1 << op_shift);
    MPIDIG_WIN(win, info_args).accumulate_noncontig_dtype = true;
    MPIDIG_WIN(win, info_args).accumulate_max_bytes = -1;
    MPIDIG_WIN(win, info_args).disable_shm_accumulate = false;

    if ((info != NULL) && ((int *) info != (int *) MPI_INFO_NULL)) {
        mpi_errno = win_set_info(win, info, TRUE /* is_init */);
        if (MPI_SUCCESS != mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }


    MPIDIG_WIN(win, mmap_sz) = 0;
    MPIDIG_WIN(win, mmap_addr) = NULL;

    MPIR_cc_set(&MPIDIG_WIN(win, local_cmpl_cnts), 0);
    MPIR_cc_set(&MPIDIG_WIN(win, remote_cmpl_cnts), 0);
    MPIR_cc_set(&MPIDIG_WIN(win, remote_acc_cmpl_cnts), 0);

    MPIDIG_WIN(win, win_id) = MPIDIG_generate_win_id(comm_ptr);
    MPIDIU_map_set(MPIDI_global.win_map, MPIDIG_WIN(win, win_id), win, MPL_MEM_RMA);

  fn_exit:
    MPIR_FUNC_VERBOSE_RMA_EXIT(MPID_STATE_MPIDIG_WIN_INIT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int win_finalize(MPIR_Win ** win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    int all_completed = 0;
    MPIR_Win *win = *win_ptr;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_WIN_FINALIZE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_WIN_FINALIZE);

    /* All local outstanding OPs should have been completed. */
    MPIR_Assert(MPIR_cc_get(MPIDIG_WIN(win, local_cmpl_cnts)) == 0);
    MPIR_Assert(MPIR_cc_get(MPIDIG_WIN(win, remote_cmpl_cnts)) == 0);

    /* Make progress till all OPs have been completed */
    do {
        int all_local_completed = 0, all_remote_completed = 0;

        MPIDIU_PROGRESS();

        MPIDIG_win_check_all_targets_local_completed(win, &all_local_completed);
        MPIDIG_win_check_all_targets_remote_completed(win, &all_remote_completed);

        /* Local completion counter might be updated later than remote completion
         * (at request completion), so we need to check it before release entire
         * window. */
        all_completed = (MPIR_cc_get(MPIDIG_WIN(win, local_cmpl_cnts)) == 0) &&
            (MPIR_cc_get(MPIDIG_WIN(win, remote_cmpl_cnts)) == 0) &&
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

    MPIDIG_win_target_cleanall(win);
    MPIDIG_win_hash_clear(win);

    if (win->create_flavor == MPI_WIN_FLAVOR_ALLOCATE ||
        win->create_flavor == MPI_WIN_FLAVOR_SHARED) {
        /* if more than one process on a node, we use shared memory by default */
        if (win->comm_ptr->node_comm != NULL && MPIDIG_WIN(win, mmap_addr)) {
            if (MPIDIG_WIN(win, mmap_sz) > 0) {
                /* destroy shared window memory */
                mpi_errno = MPIDIU_destroy_shm_segment(MPIDIG_WIN(win, mmap_sz),
                                                       &MPIDIG_WIN(win, shm_segment_handle),
                                                       &MPIDIG_WIN(win, mmap_addr));
                if (mpi_errno)
                    MPIR_ERR_POP(mpi_errno);
            }

            /* if shared memory allocation fails or zero size window, free the table at allocation. */
            MPL_free(MPIDIG_WIN(win, shared_table));
        } else if (MPIDIG_WIN(win, mmap_sz) > 0) {
            /* if single process on the node, we use mmap with symm heap */
            MPL_munmap(MPIDIG_WIN(win, mmap_addr), MPIDIG_WIN(win, mmap_sz), MPL_MEM_RMA);
        } else
            MPL_free(win->base);
    }

    MPIDIU_map_erase(MPIDI_global.win_map, MPIDIG_WIN(win, win_id));

    MPIR_Comm_release(win->comm_ptr);
    MPIR_Handle_obj_free(&MPIR_Win_mem, win);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_WIN_FINALIZE);
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
static int win_shm_alloc_impl(MPI_Aint size, int disp_unit, MPIR_Comm * comm_ptr, void **base_ptr,
                              MPIR_Win ** win_ptr, int shm_option)
{
    int i, mpi_errno = MPI_SUCCESS;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    MPIR_Win *win = NULL;
    size_t total_shm_size = 0LL;
    MPIDIG_win_shared_info_t *shared_table = NULL;
    MPI_Aint *shm_offsets = NULL;
    MPIR_Comm *shm_comm_ptr = comm_ptr->node_comm;
    size_t page_sz = 0, mapsize;
    bool symheap_mapfail_flag = false, shm_mapfail_flag = false;
    bool symheap_flag = true, global_symheap_flag = false;

    MPIR_CHKPMEM_DECL(2);
    MPIR_CHKLMEM_DECL(1);
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_WIN_SHM_ALLOC_IMPL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_WIN_SHM_ALLOC_IMPL);

    if (mpi_errno != MPI_SUCCESS)
        goto fn_fail;

    win = *win_ptr;
    *base_ptr = NULL;

    /* Check whether multiple processes exist on the local node. If so,
     * we need to count the total size on a node for shared memory allocation. */
    if (shm_comm_ptr != NULL) {
        MPIR_T_PVAR_TIMER_START(RMA, rma_wincreate_allgather);
        MPIR_CHKPMEM_MALLOC(MPIDIG_WIN(win, shared_table), MPIDIG_win_shared_info_t *,
                            sizeof(MPIDIG_win_shared_info_t) * shm_comm_ptr->local_size,
                            mpi_errno, "shared table", MPL_MEM_RMA);
        shared_table = MPIDIG_WIN(win, shared_table);
        shared_table[shm_comm_ptr->rank].size = size;
        shared_table[shm_comm_ptr->rank].disp_unit = disp_unit;
        shared_table[shm_comm_ptr->rank].shm_base_addr = NULL;

        mpi_errno = MPIR_Allgather(MPI_IN_PLACE,
                                   0,
                                   MPI_DATATYPE_NULL,
                                   shared_table,
                                   sizeof(MPIDIG_win_shared_info_t), MPI_BYTE, shm_comm_ptr,
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
            if (MPIDIG_WIN(win, info_args).alloc_shared_noncontig)
                total_shm_size += MPIDIU_get_mapsize(shared_table[i].size, &page_sz);
            else
                total_shm_size += shared_table[i].size;
        }

        /* if all processes give zero size on a single node window, simply return. */
        if (total_shm_size == 0 && shm_comm_ptr->local_size == comm_ptr->local_size)
            goto fn_no_shm;

        /* if my size is not page aligned and noncontig is disabled, skip global symheap. */
        if (size != MPIDIU_get_mapsize(size, &page_sz) &&
            !MPIDIG_WIN(win, info_args).alloc_shared_noncontig)
            symheap_flag = false;
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
        mpi_errno = MPIR_Allreduce(&symheap_flag, &global_symheap_flag, 1, MPI_C_BOOL,
                                   MPI_LAND, comm_ptr, &errflag);
        MPIR_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");
    } else
        global_symheap_flag = false;

    /* because MPI_shm follows a create & attach mode, we need to set the
     * size of entire shared memory segment on each node as the size of
     * each process. */
    mapsize = MPIDIU_get_mapsize(total_shm_size, &page_sz);

    /* first try global symmetric heap segment allocation */
    if (global_symheap_flag) {
        MPIDIG_WIN(win, mmap_sz) = mapsize;
        mpi_errno = MPIDIU_get_shm_symheap(mapsize, shm_offsets, comm_ptr,
                                           win, &symheap_mapfail_flag);
        if (mpi_errno != MPI_SUCCESS)
            goto fn_fail;

        if (symheap_mapfail_flag) {
            MPIDIG_WIN(win, mmap_sz) = 0;
            MPIDIG_WIN(win, mmap_addr) = NULL;
        }
    }

    /* if symmetric heap is disabled or fails, try normal shm segment allocation */
    if (!global_symheap_flag || symheap_mapfail_flag) {
        if (shm_comm_ptr != NULL && mapsize) {
            MPIDIG_WIN(win, mmap_sz) = mapsize;
            mpi_errno = MPIDIU_allocate_shm_segment(shm_comm_ptr, mapsize,
                                                    &MPIDIG_WIN(win, shm_segment_handle),
                                                    &MPIDIG_WIN(win, mmap_addr), &shm_mapfail_flag);
            if (mpi_errno != MPI_SUCCESS)
                goto fn_fail;

            if (shm_mapfail_flag) {
                MPIDIG_WIN(win, mmap_sz) = 0;
                MPIDIG_WIN(win, mmap_addr) = NULL;
            }

            /* throw error here if shm allocation is required but fails */
            if (shm_option == SHM_WIN_REQUIRED)
                MPIR_ERR_CHKANDJUMP(shm_mapfail_flag, mpi_errno, MPI_ERR_OTHER, "**alloc_shar_mem");
        }

        /* If only single process on a node or shm segment allocation fails, try malloc. */
        if ((shm_comm_ptr == NULL || shm_mapfail_flag) && size > 0) {
            MPIR_CHKPMEM_MALLOC(*base_ptr, void *, size, mpi_errno, "(*win_ptr)->base",
                                MPL_MEM_RMA);
            MPL_VG_MEM_INIT(*base_ptr, size);
        }
    }

    /* compute the base addresses of each process within the shared memory segment */
    if (shm_comm_ptr != NULL && MPIDIG_WIN(win, mmap_addr)) {
        char *cur_base = (char *) MPIDIG_WIN(win, mmap_addr);
        for (i = 0; i < shm_comm_ptr->local_size; i++) {
            if (shared_table[i].size)
                shared_table[i].shm_base_addr = cur_base;
            else
                shared_table[i].shm_base_addr = NULL;

            if (MPIDIG_WIN(win, info_args).alloc_shared_noncontig)
                cur_base += MPIDIU_get_mapsize(shared_table[i].size, &page_sz);
            else
                cur_base += shared_table[i].size;
        }

        *base_ptr = shared_table[shm_comm_ptr->rank].shm_base_addr;
    } else if (MPIDIG_WIN(win, mmap_sz) > 0) {
        /* if symm heap is allocated without shared memory, use the mapping address */
        *base_ptr = MPIDIG_WIN(win, mmap_addr);
    }
    /* otherwise, it has already be assigned with a local memory region or NULL (zero size). */

  fn_no_shm:
    /* free shared_table if no shm segment allocated */
    if (shared_table && !MPIDIG_WIN(win, mmap_addr)) {
        MPL_free(MPIDIG_WIN(win, shared_table));
        MPIDIG_WIN(win, shared_table) = NULL;
    }

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_WIN_SHM_ALLOC_IMPL);
    return mpi_errno;
  fn_fail:
    MPIR_CHKPMEM_REAP();
    goto fn_exit;
}

int MPIDIG_RMA_Init_sync_pvars(void)
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

int MPIDIG_mpi_win_set_info(MPIR_Win * win, MPIR_Info * info)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_MPI_WIN_SET_INFO);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_MPI_WIN_SET_INFO);

    mpi_errno = win_set_info(win, info, FALSE /* is_init */);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);

    mpi_errno = MPIR_Barrier(win->comm_ptr, &errflag);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_MPI_WIN_SET_INFO);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDIG_mpi_win_get_info(MPIR_Win * win, MPIR_Info ** info_p_p)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_MPI_WIN_GET_INFO);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_MPI_WIN_GET_INFO);

    mpi_errno = MPIR_Info_alloc(info_p_p);
    if (MPI_SUCCESS != mpi_errno) {
        *info_p_p = NULL;
        MPIR_ERR_POP(mpi_errno);
    }

    if (MPIDIG_WIN(win, info_args).no_locks)
        mpi_errno = MPIR_Info_set_impl(*info_p_p, "no_locks", "true");
    else
        mpi_errno = MPIR_Info_set_impl(*info_p_p, "no_locks", "false");

    if (MPI_SUCCESS != mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    {
#define BUFSIZE 32
        char buf[BUFSIZE];
        int c = 0;

        MPL_COMPILE_TIME_ASSERT(BUFSIZE >= 16); /* maximum: strlen("rar,raw,war,waw") + 1 */

        if (MPIDIG_WIN(win, info_args).accumulate_ordering & MPIDIG_ACCU_ORDER_RAR)
            c += snprintf(buf, BUFSIZE, "rar");

        if (MPIDIG_WIN(win, info_args).accumulate_ordering & MPIDIG_ACCU_ORDER_RAW)
            c += snprintf(buf + c, BUFSIZE - c, "%sraw", (c > 0) ? "," : "");

        if (MPIDIG_WIN(win, info_args).accumulate_ordering & MPIDIG_ACCU_ORDER_WAR)
            c += snprintf(buf + c, BUFSIZE - c, "%swar", (c > 0) ? "," : "");

        if (MPIDIG_WIN(win, info_args).accumulate_ordering & MPIDIG_ACCU_ORDER_WAW)
            c += snprintf(buf + c, BUFSIZE - c, "%swaw", (c > 0) ? "," : "");

        if (c == 0) {
            strncpy(buf, "none", BUFSIZE);
        }

        mpi_errno = MPIR_Info_set_impl(*info_p_p, "accumulate_ordering", buf);
        if (MPI_SUCCESS != mpi_errno)
            MPIR_ERR_POP(mpi_errno);
#undef BUFSIZE
    }

    if (MPIDIG_WIN(win, info_args).accumulate_ops == MPIDIG_ACCU_SAME_OP)
        mpi_errno = MPIR_Info_set_impl(*info_p_p, "accumulate_ops", "same_op");
    else
        mpi_errno = MPIR_Info_set_impl(*info_p_p, "accumulate_ops", "same_op_no_op");

    if (MPI_SUCCESS != mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    if (MPIDIG_WIN(win, info_args).alloc_shared_noncontig)
        mpi_errno = MPIR_Info_set_impl(*info_p_p, "alloc_shared_noncontig", "true");
    else
        mpi_errno = MPIR_Info_set_impl(*info_p_p, "alloc_shared_noncontig", "false");

    if (MPI_SUCCESS != mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    if (MPIDIG_WIN(win, info_args).same_size)
        mpi_errno = MPIR_Info_set_impl(*info_p_p, "same_size", "true");
    else
        mpi_errno = MPIR_Info_set_impl(*info_p_p, "same_size", "false");

    if (MPI_SUCCESS != mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    if (MPIDIG_WIN(win, info_args).same_disp_unit)
        mpi_errno = MPIR_Info_set_impl(*info_p_p, "same_disp_unit", "true");
    else
        mpi_errno = MPIR_Info_set_impl(*info_p_p, "same_disp_unit", "false");

    if (MPI_SUCCESS != mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    if (MPIDIG_WIN(win, info_args).alloc_shm)
        mpi_errno = MPIR_Info_set_impl(*info_p_p, "alloc_shm", "true");
    else
        mpi_errno = MPIR_Info_set_impl(*info_p_p, "alloc_shm", "false");

    if (MPI_SUCCESS != mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    {   /* Keep buf as a local variable for which_accumulate_ops key. */
        char buf[128];
        get_info_accu_ops_str(MPIDIG_WIN(win, info_args).which_accumulate_ops, &buf[0],
                              sizeof(buf));
        mpi_errno = MPIR_Info_set_impl(*info_p_p, "which_accumulate_ops", buf);
        if (MPI_SUCCESS != mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }

    if (MPIDIG_WIN(win, info_args).accumulate_noncontig_dtype)
        mpi_errno = MPIR_Info_set_impl(*info_p_p, "accumulate_noncontig_dtype", "true");
    else
        mpi_errno = MPIR_Info_set_impl(*info_p_p, "accumulate_noncontig_dtype", "false");
    if (MPI_SUCCESS != mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    if (MPIDIG_WIN(win, info_args).accumulate_max_bytes >= 0) {
        char buf[32];           /* make sure 64-bit integer can fit */
        snprintf(buf, sizeof(buf), "%ld", (long) MPIDIG_WIN(win, info_args).accumulate_max_bytes);
        mpi_errno = MPIR_Info_set_impl(*info_p_p, "accumulate_max_bytes", buf);
    } else
        mpi_errno = MPIR_Info_set_impl(*info_p_p, "accumulate_max_bytes", "unlimited");
    if (MPI_SUCCESS != mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    if (MPIDIG_WIN(win, info_args).disable_shm_accumulate)
        mpi_errno = MPIR_Info_set_impl(*info_p_p, "disable_shm_accumulate", "true");
    else
        mpi_errno = MPIR_Info_set_impl(*info_p_p, "disable_shm_accumulate", "false");
    if (MPI_SUCCESS != mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_MPI_WIN_GET_INFO);
    return mpi_errno;
  fn_fail:
    if (*info_p_p != NULL) {
        MPIR_Info_free(*info_p_p);
        *info_p_p = NULL;
    }
    goto fn_exit;
}

int MPIDIG_mpi_win_free(MPIR_Win ** win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    MPIR_Win *win = *win_ptr;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_MPI_WIN_FREE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_MPI_WIN_FREE);

    MPIDIG_ACCESS_EPOCH_CHECK_NONE(win, mpi_errno, return mpi_errno);
    MPIDIG_EXPOSURE_EPOCH_CHECK_NONE(win, mpi_errno, return mpi_errno);

    mpi_errno = MPIR_Barrier(win->comm_ptr, &errflag);
    if (mpi_errno != MPI_SUCCESS)
        goto fn_fail;

    win_finalize(win_ptr);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_MPI_WIN_FREE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDIG_mpi_win_create(void *base, MPI_Aint length, int disp_unit, MPIR_Info * info,
                          MPIR_Comm * comm_ptr, MPIR_Win ** win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    MPIR_Win *win;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_MPI_WIN_CREATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_MPI_WIN_CREATE);

    mpi_errno = win_init(length, disp_unit, win_ptr, info, comm_ptr, MPI_WIN_FLAVOR_CREATE,
                         MPI_WIN_UNIFIED);

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
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_MPI_WIN_CREATE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDIG_mpi_win_attach(MPIR_Win * win, void *base, MPI_Aint size)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_MPI_WIN_ATTACH);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_MPI_WIN_ATTACH);

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
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_MPI_WIN_ATTACH);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDIG_mpi_win_allocate_shared(MPI_Aint size, int disp_unit, MPIR_Info * info_ptr,
                                   MPIR_Comm * comm_ptr, void **base_ptr, MPIR_Win ** win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    MPIR_Win *win = NULL;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_MPI_WIN_ALLOCATE_SHARED);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_MPI_WIN_ALLOCATE_SHARED);

    mpi_errno = win_init(size, disp_unit, win_ptr, info_ptr, comm_ptr, MPI_WIN_FLAVOR_SHARED,
                         MPI_WIN_UNIFIED);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);

    mpi_errno = win_shm_alloc_impl(size, disp_unit, comm_ptr, base_ptr, win_ptr, SHM_WIN_REQUIRED);
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
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_MPI_WIN_ALLOCATE_SHARED);
    return mpi_errno;
  fn_fail:
    if (win_ptr)
        win_finalize(win_ptr);
    goto fn_exit;
}

int MPIDIG_mpi_win_detach(MPIR_Win * win, const void *base)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_MPI_WIN_DETACH);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_MPI_WIN_DETACH);
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
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_MPI_WIN_DETACH);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDIG_mpi_win_allocate(MPI_Aint size, int disp_unit, MPIR_Info * info, MPIR_Comm * comm,
                            void *baseptr, MPIR_Win ** win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    MPIR_Win *win;
    void **base_ptr = (void **) baseptr;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_MPI_WIN_ALLOCATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_MPI_WIN_ALLOCATE);

    mpi_errno = win_init(size, disp_unit, win_ptr, info, comm, MPI_WIN_FLAVOR_ALLOCATE,
                         MPI_WIN_UNIFIED);

    if (mpi_errno != MPI_SUCCESS)
        goto fn_fail;

    mpi_errno = win_shm_alloc_impl(size, disp_unit, comm, base_ptr, win_ptr, SHM_WIN_OPTIONAL);
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
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_MPI_WIN_ALLOCATE);
    return mpi_errno;
  fn_fail:
    if (win_ptr)
        win_finalize(win_ptr);
    goto fn_exit;
}

int MPIDIG_mpi_win_create_dynamic(MPIR_Info * info, MPIR_Comm * comm, MPIR_Win ** win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    int rc = MPI_SUCCESS;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_MPI_WIN_CREATE_DYNAMIC);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_MPI_WIN_CREATE_DYNAMIC);

    MPIR_Win *win;

    rc = win_init(0, 1, win_ptr, info, comm, MPI_WIN_FLAVOR_DYNAMIC, MPI_WIN_UNIFIED);

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
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_MPI_WIN_CREATE_DYNAMIC);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

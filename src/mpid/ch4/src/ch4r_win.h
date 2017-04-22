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
#include "mpl_uthash.h"
#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif /* HAVE_SYS_MMAN_H */

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

    MPIR_Info *curr_ptr;
    char *value, *token, *savePtr;
    int save_ordering;

    curr_ptr = info->next;

    while (curr_ptr) {
        if (!strcmp(curr_ptr->key, "no_locks")) {
            if (!strcmp(curr_ptr->value, "true"))
                MPIDI_CH4U_WIN(win, info_args).no_locks = 1;
            else if (!strcmp(curr_ptr->value, "false"))
                MPIDI_CH4U_WIN(win, info_args).no_locks = 0;
        }
        else if (!strcmp(curr_ptr->key, "accumulate_ordering")) {
            save_ordering = MPIDI_CH4U_WIN(win, info_args).accumulate_ordering;
            MPIDI_CH4U_WIN(win, info_args).accumulate_ordering = 0;
            if (!strcmp(curr_ptr->value, "none"))
                goto next;
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
        }
        else if (!strcmp(curr_ptr->key, "accumulate_ops")) {
            if (!strcmp(curr_ptr->value, "same_op"))
                MPIDI_CH4U_WIN(win, info_args).accumulate_ops = MPIDI_CH4I_ACCU_SAME_OP;
            else if (!strcmp(curr_ptr->value, "same_op_no_op"))
                MPIDI_CH4U_WIN(win, info_args).accumulate_ops = MPIDI_CH4I_ACCU_SAME_OP_NO_OP;
        }
        else if (!strcmp(curr_ptr->key, "same_disp_unit")) {
            if (!strcmp(curr_ptr->value, "true"))
                MPIDI_CH4U_WIN(win, info_args).same_disp_unit = 1;
            else if (!strcmp(curr_ptr->value, "false"))
                MPIDI_CH4U_WIN(win, info_args).same_disp_unit = 0;
        }
        else if (!strcmp(curr_ptr->key, "same_size")) {
            if (!strcmp(curr_ptr->value, "true"))
                MPIDI_CH4U_WIN(win, info_args).same_size = 1;
            else if (!strcmp(curr_ptr->value, "false"))
                MPIDI_CH4U_WIN(win, info_args).same_size = 0;
        }
        else if (!strcmp(curr_ptr->key, "alloc_shared_noncontig")) {
            if (!strcmp(curr_ptr->value, "true"))
                MPIDI_CH4U_WIN(win, info_args).alloc_shared_noncontig = 1;
            else if (!strcmp(curr_ptr->value, "false"))
                MPIDI_CH4U_WIN(win, info_args).alloc_shared_noncontig = 0;
        }
      next:
        curr_ptr = curr_ptr->next;
    }

    mpi_errno = MPIR_Barrier_impl(win->comm_ptr, &errflag);
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
    int i;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4R_WIN_INIT);
    MPIR_FUNC_VERBOSE_RMA_ENTER(MPID_STATE_MPIDI_CH4R_WIN_INIT);

    MPIR_CHKPMEM_DECL(1);

    MPIR_ERR_CHKANDSTMT(win == NULL, mpi_errno, MPI_ERR_NO_MEM, goto fn_fail, "**nomem");
    *win_ptr = win;

    memset(&win->dev.ch4u, 0, sizeof(MPIDI_CH4U_win_t));
    win->comm_ptr = comm_ptr;
    MPIR_Comm_add_ref(comm_ptr);

    /* FIXME: per-target structure should be initialized only when user starts
     * per-target synchronization. */
    MPIR_CHKPMEM_MALLOC(targets, MPIDI_CH4U_win_target_t *,
                        comm_ptr->local_size * sizeof(MPIDI_CH4U_win_target_t),
                        mpi_errno, "window targets");
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
    win->comm_ptr = comm_ptr;
    win->copyDispUnit = 0;
    win->copySize = 0;
    MPIDI_CH4U_WIN(win, shared_table) = NULL;

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

    if ((info != NULL) && ((int *) info != (int *) MPI_INFO_NULL)) {
        mpi_errno = MPIDI_CH4R_mpi_win_set_info(win, info);
        MPIR_Assert(mpi_errno == 0);
    }


    MPIDI_CH4U_WIN(win, mmap_sz) = 0;
    MPIDI_CH4U_WIN(win, mmap_addr) = NULL;

    MPIR_cc_set(&MPIDI_CH4U_WIN(win, local_cmpl_cnts), 0);
    MPIR_cc_set(&MPIDI_CH4U_WIN(win, remote_cmpl_cnts), 0);
    for (i = 0; i < win->comm_ptr->local_size; i++) {
        MPIR_cc_set(&targets[i].local_cmpl_cnts, 0);
        MPIR_cc_set(&targets[i].remote_cmpl_cnts, 0);

        targets[i].sync.lock.locked = 0;
        targets[i].sync.origin_epoch_type = MPIDI_CH4U_EPOTYPE_NONE;
    }

    MPIDI_CH4U_WIN(win, win_id) = MPIDI_CH4U_generate_win_id(comm_ptr);
    MPL_HASH_ADD(dev.ch4u.hash_handle, MPIDI_CH4_Global.win_hash,
                 dev.ch4u.win_id, sizeof(uint64_t), win);

    MPIR_CHKPMEM_COMMIT();
  fn_exit:
    MPIR_FUNC_VERBOSE_RMA_EXIT(MPID_STATE_MPIDI_CH4R_WIN_INIT);
    return mpi_errno;
  fn_fail:
    MPIR_CHKPMEM_REAP();
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
    int i, *ranks_in_grp;
    MPIR_Group *win_grp_ptr;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4I_FILL_RANKS_IN_WIN_GRP);
    MPIR_FUNC_VERBOSE_RMA_ENTER(MPID_STATE_MPIDI_CH4I_FILL_RANKS_IN_WIN_GRP);

    ranks_in_grp = (int *) MPL_malloc(group_ptr->size * sizeof(int));
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

    MPL_free(ranks_in_grp);

  fn_exit:
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

    MPIDI_CH4U_EPOCH_CHECK_TYPE(win, mpi_errno, goto fn_fail);

    MPIR_Group_add_ref(group);

    MPIDI_CH4R_PROGRESS_WHILE(group->size != (int) MPIDI_CH4U_WIN(win, sync).pw.count);
    MPIDI_CH4U_WIN(win, sync).pw.count = 0;

    MPIR_ERR_CHKANDJUMP((MPIDI_CH4U_WIN(win, sync).sc.group != NULL),
                        mpi_errno, MPI_ERR_GROUP, "**group");
    MPIDI_CH4U_WIN(win, sync).sc.group = group;
    MPIDI_CH4U_WIN(win, sync).origin_epoch_type = MPIDI_CH4U_EPOTYPE_START;

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
    int index, peer;
    MPIR_Group *group;
    int *ranks_in_win_grp;
    int all_local_completed = 0;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4R_MPI_WIN_COMPLETE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4R_MPI_WIN_COMPLETE);

    MPIDI_CH4U_EPOCH_START_CHECK2(win, mpi_errno, goto fn_fail);

    group = MPIDI_CH4U_WIN(win, sync).sc.group;
    MPIR_Assert(group != NULL);

    msg.win_id = MPIDI_CH4U_WIN(win, win_id);
    msg.origin_rank = win->comm_ptr->rank;

    ranks_in_win_grp = (int *) MPL_malloc(sizeof(int) * group->size);
    MPIR_Assert(ranks_in_win_grp);

    mpi_errno = MPIDI_CH4I_fill_ranks_in_win_grp(win, group, ranks_in_win_grp);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);

    /* FIXME: now we simply set per-target counters for PSCW, can it be optimized ? */
    do {
        MPIDI_CH4R_PROGRESS();
        MPIDI_win_check_group_local_completed(win, ranks_in_win_grp, group->size,
                                              &all_local_completed);
    } while (all_local_completed != 1);

    for (index = 0; index < group->size; ++index) {
        peer = ranks_in_win_grp[index];
        mpi_errno = MPIDI_NM_am_send_hdr(peer, win->comm_ptr,
                                         MPIDI_CH4U_WIN_COMPLETE, &msg, sizeof(msg));
        if (mpi_errno != MPI_SUCCESS)
            MPIR_ERR_SETANDSTMT(mpi_errno, MPI_ERR_RMA_SYNC, goto fn_fail, "**rmasync");
    }

    MPL_free(ranks_in_win_grp);
    MPIDI_CH4U_WIN(win, sync).origin_epoch_type = MPIDI_CH4U_EPOTYPE_NONE;
    MPIR_Group_release(MPIDI_CH4U_WIN(win, sync).sc.group);
    MPIDI_CH4U_WIN(win, sync).sc.group = NULL;

  fn_exit:
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
    int index, peer;
    int *ranks_in_win_grp;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4R_MPI_WIN_POST);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4R_MPI_WIN_POST);

    MPIDI_CH4U_EPOCH_POST_CHECK(win, mpi_errno, goto fn_fail);

    MPIR_Group_add_ref(group);
    MPIR_ERR_CHKANDJUMP((MPIDI_CH4U_WIN(win, sync).pw.group != NULL),
                        mpi_errno, MPI_ERR_GROUP, "**group");

    MPIDI_CH4U_WIN(win, sync).pw.group = group;
    MPIR_Assert(group != NULL);

    msg.win_id = MPIDI_CH4U_WIN(win, win_id);
    msg.origin_rank = win->comm_ptr->rank;

    ranks_in_win_grp = (int *) MPL_malloc(sizeof(int) * group->size);
    MPIR_Assert(ranks_in_win_grp);

    mpi_errno = MPIDI_CH4I_fill_ranks_in_win_grp(win, group, ranks_in_win_grp);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);

    for (index = 0; index < group->size; ++index) {
        peer = ranks_in_win_grp[index];
        mpi_errno = MPIDI_NM_am_send_hdr(peer, win->comm_ptr,
                                         MPIDI_CH4U_WIN_POST, &msg, sizeof(msg));
        if (mpi_errno != MPI_SUCCESS)
            MPIR_ERR_SETANDSTMT(mpi_errno, MPI_ERR_RMA_SYNC, goto fn_fail, "**rmasync");
    }

    MPL_free(ranks_in_win_grp);
    MPIDI_CH4U_WIN(win, sync).target_epoch_type = MPIDI_CH4U_EPOTYPE_POST;
  fn_exit:
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

    MPIDI_CH4U_EPOCH_TARGET_CHECK(win, MPIDI_CH4U_EPOTYPE_POST, mpi_errno, goto fn_fail);
    group = MPIDI_CH4U_WIN(win, sync).pw.group;
    MPIDI_CH4R_PROGRESS_WHILE(group->size != (int) MPIDI_CH4U_WIN(win, sync).sc.count);

    MPIDI_CH4U_WIN(win, sync).sc.count = 0;
    MPIDI_CH4U_WIN(win, sync).pw.group = NULL;
    MPIR_Group_release(group);
    MPIDI_CH4U_WIN(win, sync).target_epoch_type = MPIDI_CH4U_EPOTYPE_NONE;

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

    MPIDI_CH4U_EPOCH_TARGET_CHECK(win, MPIDI_CH4U_EPOTYPE_POST, mpi_errno, goto fn_fail);

    MPIR_Group *group;
    group = MPIDI_CH4U_WIN(win, sync).pw.group;

    if (group->size == (int) MPIDI_CH4U_WIN(win, sync).sc.count) {
        MPIDI_CH4U_WIN(win, sync).sc.count = 0;
        MPIDI_CH4U_WIN(win, sync).pw.group = NULL;
        *flag = 1;
        MPIR_Group_release(group);
        MPIDI_CH4U_WIN(win, sync).target_epoch_type = MPIDI_CH4U_EPOTYPE_NONE;
    }
    else {
        MPIDI_CH4R_PROGRESS();
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

    MPIDI_CH4U_EPOCH_CHECK_LOCK_TYPE(win, rank, mpi_errno, goto fn_fail);

    MPIDI_CH4U_win_target_t *target_ptr = &MPIDI_CH4U_WIN(win, targets)[rank];
    MPIDI_CH4U_win_target_sync_lock_t *slock = &target_ptr->sync.lock;
    MPIR_Assert(slock->locked == 0);

    MPIDI_CH4U_win_cntrl_msg_t msg;
    msg.win_id = MPIDI_CH4U_WIN(win, win_id);
    msg.origin_rank = win->comm_ptr->rank;
    msg.lock_type = lock_type;

    locked = slock->locked + 1;
    mpi_errno = MPIDI_NM_am_send_hdr(rank, win->comm_ptr,
                                     MPIDI_CH4U_WIN_LOCK, &msg, sizeof(msg));
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_SETANDSTMT(mpi_errno, MPI_ERR_RMA_SYNC, goto fn_fail, "**rmasync");

    MPIDI_CH4R_PROGRESS_WHILE(slock->locked != locked);
    target_ptr->sync.origin_epoch_type = MPIDI_CH4U_EPOTYPE_LOCK;

  fn_exit0:
    MPIDI_CH4U_WIN(win, sync).origin_epoch_type = MPIDI_CH4U_EPOTYPE_LOCK;
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
    MPIDI_CH4U_EPOCH_ORIGIN_CHECK(win, MPIDI_CH4U_EPOTYPE_LOCK, mpi_errno, return mpi_errno);
    if (rank == MPI_PROC_NULL)
        goto fn_exit0;

    /* Check per-target lock epoch */
    MPIDI_CH4U_EPOCH_ORIGIN_LOCK_CHECK(win, rank, mpi_errno, return mpi_errno);

    MPIDI_CH4U_win_target_t *target_ptr = NULL;
    target_ptr = &MPIDI_CH4U_WIN(win, targets)[rank];

    MPIDI_CH4U_win_target_sync_lock_t *slock = &target_ptr->sync.lock;
    /* NOTE: lock blocking waits till granted */
    MPIR_Assert(slock->locked == 1);

    do {
        MPIDI_CH4R_PROGRESS();
    } while (MPIR_cc_get(MPIDI_CH4U_WIN_TARGET(win, rank, remote_cmpl_cnts)) != 0);

    msg.win_id = MPIDI_CH4U_WIN(win, win_id);
    msg.origin_rank = win->comm_ptr->rank;
    unlocked = slock->locked - 1;

    mpi_errno = MPIDI_NM_am_send_hdr(rank, win->comm_ptr,
                                     MPIDI_CH4U_WIN_UNLOCK, &msg, sizeof(msg));
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_SETANDSTMT(mpi_errno, MPI_ERR_RMA_SYNC, goto fn_fail, "**rmasync");

    MPIDI_CH4R_PROGRESS_WHILE(slock->locked != unlocked);
    target_ptr->sync.origin_epoch_type = MPIDI_CH4U_EPOTYPE_NONE;

  fn_exit0:
    MPIR_Assert(MPIDI_CH4U_WIN(win, sync).lock.count > 0);
    MPIDI_CH4U_WIN(win, sync).lock.count--;

    /* Reset window epoch only when all per-target lock epochs are closed. */
    if (MPIDI_CH4U_WIN(win, sync).lock.count == 0) {
        MPIDI_CH4U_WIN(win, sync).origin_epoch_type = MPIDI_CH4U_EPOTYPE_NONE;
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
    MPIR_Assert(mpi_errno == MPI_SUCCESS);

    if (MPIDI_CH4U_WIN(win, info_args).no_locks)
        mpi_errno = MPIR_Info_set_impl(*info_p_p, "no_locks", "true");
    else
        mpi_errno = MPIR_Info_set_impl(*info_p_p, "no_locks", "false");

    MPIR_Assert(mpi_errno == MPI_SUCCESS);

    {
#define BUFSIZE 32
        char buf[BUFSIZE];
        int c = 0;

        CH4_COMPILE_TIME_ASSERT(BUFSIZE >= 16); /* maximum: strlen("rar,raw,war,waw") + 1 */

        if (MPIDI_CH4U_WIN(win, info_args).accumulate_ordering & MPIDI_CH4I_ACCU_ORDER_RAR)
            c += snprintf(buf + c, BUFSIZE - c, "%srar", (c > 0) ? "," : "");

        if (MPIDI_CH4U_WIN(win, info_args).accumulate_ordering & MPIDI_CH4I_ACCU_ORDER_RAW)
            c += snprintf(buf + c, BUFSIZE - c, "%sraw", (c > 0) ? "," : "");

        if (MPIDI_CH4U_WIN(win, info_args).accumulate_ordering & MPIDI_CH4I_ACCU_ORDER_WAR)
            c += snprintf(buf + c, BUFSIZE - c, "%swar", (c > 0) ? "," : "");

        if (MPIDI_CH4U_WIN(win, info_args).accumulate_ordering & MPIDI_CH4I_ACCU_ORDER_WAW)
            c += snprintf(buf + c, BUFSIZE - c, "%swaw", (c > 0) ? "," : "");

        if (c == 0) {
            strncpy(buf, "none", BUFSIZE);
        }

        MPIR_Info_set_impl(*info_p_p, "accumulate_ordering", buf);
        MPIR_Assert(mpi_errno == MPI_SUCCESS);
#undef BUFSIZE
    }

    if (MPIDI_CH4U_WIN(win, info_args).accumulate_ops == MPIDI_CH4I_ACCU_SAME_OP)
        mpi_errno = MPIR_Info_set_impl(*info_p_p, "accumulate_ops", "same_op");
    else
        mpi_errno = MPIR_Info_set_impl(*info_p_p, "accumulate_ops", "same_op_no_op");

    MPIR_Assert(mpi_errno == MPI_SUCCESS);

    if (MPIDI_CH4U_WIN(win, info_args).alloc_shared_noncontig)
        mpi_errno = MPIR_Info_set_impl(*info_p_p, "alloc_shared_noncontig", "true");
    else
        mpi_errno = MPIR_Info_set_impl(*info_p_p, "alloc_shared_noncontig", "false");

    MPIR_Assert(mpi_errno == MPI_SUCCESS);

    if (MPIDI_CH4U_WIN(win, info_args).same_size)
        mpi_errno = MPIR_Info_set_impl(*info_p_p, "same_size", "true");
    else
        mpi_errno = MPIR_Info_set_impl(*info_p_p, "same_size", "false");

    MPIR_Assert(mpi_errno == MPI_SUCCESS);

    if (MPIDI_CH4U_WIN(win, info_args).same_disp_unit)
        mpi_errno = MPIR_Info_set_impl(*info_p_p, "same_disp_unit", "true");
    else
        mpi_errno = MPIR_Info_set_impl(*info_p_p, "same_disp_unit", "false");

    MPIR_Assert(mpi_errno == MPI_SUCCESS);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4R_MPI_WIN_GET_INFO);
    return mpi_errno;
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

    MPL_free(MPIDI_CH4U_WIN(win, targets));

    if (win->create_flavor == MPI_WIN_FLAVOR_ALLOCATE && win->base) {
        if (MPIDI_CH4U_WIN(win, mmap_sz) > 0)
            munmap(MPIDI_CH4U_WIN(win, mmap_addr), MPIDI_CH4U_WIN(win, mmap_sz));
        else if (MPIDI_CH4U_WIN(win, mmap_sz) == -1)
            MPL_free(win->base);
    }

    if (win->create_flavor == MPI_WIN_FLAVOR_SHARED) {
        if (MPIDI_CH4U_WIN(win, mmap_addr))
            munmap(MPIDI_CH4U_WIN(win, mmap_addr), MPIDI_CH4U_WIN(win, mmap_sz));
        MPL_free(MPIDI_CH4U_WIN(win, sizes));
    }

    MPL_HASH_DELETE(dev.ch4u.hash_handle, MPIDI_CH4_Global.win_hash, win);

    if (win->create_flavor == MPI_WIN_FLAVOR_SHARED) {
        MPL_free(MPIDI_CH4U_WIN(win, shared_table));
    }

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

    MPIDI_CH4U_EPOCH_FREE_CHECK(win, mpi_errno, goto fn_fail);
    mpi_errno = MPIR_Barrier_impl(win->comm_ptr, &errflag);
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

    MPIDI_CH4U_EPOCH_FENCE_CHECK(win, mpi_errno, goto fn_fail);
    do {
        MPIDI_CH4R_PROGRESS();
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
    mpi_errno = MPIR_Barrier_impl(win->comm_ptr, &errflag);

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

    mpi_errno = MPIR_Barrier_impl(comm_ptr, &errflag);

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
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4R_MPI_WIN_ATTACH);
    return mpi_errno;
  fn_fail:
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
    int i = 0, fd = -1, rc, first = 0, mpi_errno = MPI_SUCCESS;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    void *baseP = NULL;
    MPIR_Win *win = NULL;
    ssize_t total_size = 0LL;
    MPI_Aint size_out = 0;
    MPIDI_CH4U_win_shared_info_t *shared_table = NULL;
    char shm_key[64];
    void *map_ptr;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4R_MPI_WIN_ALLOCATE_SHARED);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4R_MPI_WIN_ALLOCATE_SHARED);

    mpi_errno = MPIDI_CH4R_win_init(size, disp_unit, win_ptr, info_ptr, comm_ptr,
                                    MPI_WIN_FLAVOR_SHARED, MPI_WIN_UNIFIED);

    win = *win_ptr;
    MPIDI_CH4U_WIN(win, shared_table) =
        (MPIDI_CH4U_win_shared_info_t *) MPL_malloc(sizeof(MPIDI_CH4U_win_shared_info_t) *
                                                    comm_ptr->local_size);
    shared_table = MPIDI_CH4U_WIN(win, shared_table);
    shared_table[comm_ptr->rank].size = size;
    shared_table[comm_ptr->rank].disp_unit = disp_unit;

    mpi_errno = MPIR_Allgather_impl(MPI_IN_PLACE,
                                    0,
                                    MPI_DATATYPE_NULL,
                                    shared_table,
                                    sizeof(MPIDI_CH4U_win_shared_info_t),
                                    MPI_BYTE, comm_ptr, &errflag);
    if (mpi_errno != MPI_SUCCESS)
        goto fn_fail;

    /* No allreduce here because this is a shared memory domain
     * and should be a relatively small number of processes
     * and a non performance sensitive API.
     */
    for (i = 0; i < comm_ptr->local_size; i++)
        total_size += shared_table[i].size;

    if (total_size == 0)
        goto fn_zero;

    sprintf(shm_key, "/mpi-%X-%" PRIx64, MPIDI_CH4_Global.jobid, MPIDI_CH4U_WIN(win, win_id));

    rc = shm_open(shm_key, O_CREAT | O_EXCL | O_RDWR, 0600);
    first = (rc != -1);

    if (!first) {
        rc = shm_open(shm_key, O_RDWR, 0);

        if (rc == -1) {
            shm_unlink(shm_key);
            MPIR_ERR_SETANDSTMT(mpi_errno, MPI_ERR_NO_MEM, goto fn_fail, "**nomem");
        }
    }

    /* Make the addresses symmetric by using MAP_FIXED */
    size_t page_sz, mapsize;

    mapsize = MPIDI_CH4R_get_mapsize(total_size, &page_sz);
    fd = rc;
    rc = ftruncate(fd, mapsize);

    if (rc == -1) {
        close(fd);

        if (first)
            shm_unlink(shm_key);

        MPIR_ERR_SETANDSTMT(mpi_errno, MPI_ERR_NO_MEM, goto fn_fail, "**nomem");
    }

    if (comm_ptr->rank == 0) {
        map_ptr = MPIDI_CH4R_generate_random_addr(mapsize);
        map_ptr = mmap(map_ptr, mapsize, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, fd, 0);

        if (map_ptr == NULL || map_ptr == MAP_FAILED) {
            close(fd);

            if (first)
                shm_unlink(shm_key);

            MPIR_ERR_SETANDSTMT(mpi_errno, MPI_ERR_NO_MEM, goto fn_fail, "**nomem");
        }

        mpi_errno = MPIR_Bcast_impl(&map_ptr, 1, MPI_UNSIGNED_LONG, 0, comm_ptr, &errflag);

        if (mpi_errno != MPI_SUCCESS)
            goto fn_fail;

        MPIDI_CH4U_WIN(win, mmap_addr) = map_ptr;
        MPIDI_CH4U_WIN(win, mmap_sz) = mapsize;
    }
    else {
        mpi_errno = MPIR_Bcast_impl(&map_ptr, 1, MPI_UNSIGNED_LONG, 0, comm_ptr, &errflag);

        if (mpi_errno != MPI_SUCCESS)
            goto fn_fail;

        rc = MPIDI_CH4R_check_maprange_ok(map_ptr, mapsize);
        /* If we hit this assert, we need to iterate
         * trying more addresses
         */
        MPIR_Assert(rc == 1);
        map_ptr = mmap(map_ptr, mapsize, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, fd, 0);
        MPIDI_CH4U_WIN(win, mmap_addr) = map_ptr;
        MPIDI_CH4U_WIN(win, mmap_sz) = mapsize;

        if (map_ptr == NULL || map_ptr == MAP_FAILED) {
            close(fd);

            if (first)
                shm_unlink(shm_key);

            MPIR_ERR_SETANDSTMT(mpi_errno, MPI_ERR_NO_MEM, goto fn_fail, "**nomem");
        }
    }

    /* Scan for my offset into the buffer             */
    /* Could use exscan if this is expensive at scale */
    for (i = 0; i < comm_ptr->rank; i++)
        size_out += shared_table[i].size;

  fn_zero:

    baseP = (size == 0) ? NULL : (void *) ((char *) map_ptr + size_out);
    win->base = baseP;
    win->size = size;

    *(void **) base_ptr = (void *) win->base;
    mpi_errno = MPIR_Barrier_impl(comm_ptr, &errflag);

    if (fd >= 0)
        close(fd);

    if (first)
        shm_unlink(shm_key);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4R_MPI_WIN_ALLOCATE_SHARED);
    return mpi_errno;
  fn_fail:
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
    uintptr_t base = (uintptr_t) MPIDI_CH4U_WIN(win, mmap_addr);
    int offset = rank, i;
    MPIDI_CH4U_win_shared_info_t *shared_table = MPIDI_CH4U_WIN(win, shared_table);

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4R_MPI_WIN_SHARED_QUERY);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4R_MPI_WIN_SHARED_QUERY);

    if (rank < 0)
        offset = 0;
    *size = shared_table[offset].size;
    *disp_unit = shared_table[offset].disp_unit;
    if (*size > 0) {
        for (i = 0; i < offset; i++)
            base += shared_table[i].size;
        *(void **) baseptr = (void *) base;
    }
    else
        *(void **) baseptr = NULL;

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
    void *baseP;
    MPIR_Win *win;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4R_MPI_WIN_ALLOCATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4R_MPI_WIN_ALLOCATE);

    mpi_errno = MPIDI_CH4R_win_init(size, disp_unit, win_ptr, info, comm,
                                    MPI_WIN_FLAVOR_ALLOCATE, MPI_WIN_UNIFIED);

    if (mpi_errno != MPI_SUCCESS)
        goto fn_fail;

    mpi_errno = MPIDI_CH4R_get_symmetric_heap(size, comm, &baseP, *win_ptr);

    if (mpi_errno != MPI_SUCCESS)
        goto fn_fail;

    win = *win_ptr;
    win->base = baseP;

    *(void **) baseptr = (void *) win->base;
    mpi_errno = MPIR_Barrier_impl(comm, &errflag);

    if (mpi_errno != MPI_SUCCESS)
        goto fn_fail;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4R_MPI_WIN_ALLOCATE);
    return mpi_errno;
  fn_fail:
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
    MPIDI_CH4U_EPOCH_LOCK_CHECK(win, mpi_errno, return mpi_errno);
    if (rank == MPI_PROC_NULL)
        goto fn_exit;

    MPIDI_CH4U_EPOCH_PER_TARGET_LOCK_CHECK(win, rank, mpi_errno, goto fn_fail);
    do {
        MPIDI_CH4R_PROGRESS();
    } while (MPIR_cc_get(MPIDI_CH4U_WIN_TARGET(win, rank, remote_cmpl_cnts)) != 0);

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

    MPIDI_CH4U_EPOCH_LOCK_CHECK(win, mpi_errno, goto fn_fail);

    /* FIXME: now we simply set per-target counters for lockall in case
     * user flushes per target, but this should be optimized. */
    do {
        MPIDI_CH4R_PROGRESS();
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

    MPIDI_CH4U_EPOCH_ORIGIN_CHECK(win, MPIDI_CH4U_EPOTYPE_LOCK_ALL, mpi_errno, return mpi_errno);
    /* NOTE: lockall blocking waits till all locks granted */
    MPIR_Assert(MPIDI_CH4U_WIN(win, sync).lockall.allLocked == win->comm_ptr->local_size);

    /* FIXME: now we simply set per-target counters for lockall in case
     * user flushes per target, but this should be optimized. */
    do {
        MPIDI_CH4R_PROGRESS();
        MPIDI_win_check_all_targets_remote_completed(win, &all_remote_completed);
    } while (all_remote_completed != 1);

    for (i = 0; i < win->comm_ptr->local_size; i++) {

        MPIDI_CH4U_win_cntrl_msg_t msg;
        msg.win_id = MPIDI_CH4U_WIN(win, win_id);
        msg.origin_rank = win->comm_ptr->rank;

        mpi_errno = MPIDI_NM_am_send_hdr(i, win->comm_ptr,
                                         MPIDI_CH4U_WIN_UNLOCKALL, &msg, sizeof(msg));
        if (mpi_errno != MPI_SUCCESS)
            MPIR_ERR_SETANDSTMT(mpi_errno, MPI_ERR_RMA_SYNC, goto fn_fail, "**rmasync");
    }

    MPIDI_CH4R_PROGRESS_WHILE(MPIDI_CH4U_WIN(win, sync).lockall.allLocked);

    MPIDI_CH4U_WIN(win, sync).origin_epoch_type = MPIDI_CH4U_EPOTYPE_NONE;

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


    if (rc != MPI_SUCCESS)
        goto fn_fail;

    mpi_errno = MPIR_Barrier_impl(comm, &errflag);

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
    MPIDI_CH4U_EPOCH_LOCK_CHECK(win, mpi_errno, return mpi_errno);
    if (rank == MPI_PROC_NULL)
        goto fn_exit;

    MPIDI_CH4U_EPOCH_PER_TARGET_LOCK_CHECK(win, rank, mpi_errno, goto fn_fail);

    do {
        MPIDI_CH4R_PROGRESS();
    } while (MPIR_cc_get(MPIDI_CH4U_WIN_TARGET(win, rank, local_cmpl_cnts)) != 0);

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

    MPIDI_CH4U_EPOCH_LOCK_CHECK(win, mpi_errno, goto fn_fail);
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

    MPIDI_CH4U_EPOCH_LOCK_CHECK(win, mpi_errno, goto fn_fail);

    do {
        MPIDI_CH4R_PROGRESS();

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

    MPIDI_CH4U_EPOCH_CHECK_TYPE(win, mpi_errno, goto fn_fail);

    MPIR_Assert(MPIDI_CH4U_WIN(win, sync).lockall.allLocked == 0);

    int size;
    size = win->comm_ptr->local_size;

    int i;
    for (i = 0; i < size; i++) {
        MPIDI_CH4U_win_cntrl_msg_t msg;
        msg.win_id = MPIDI_CH4U_WIN(win, win_id);
        msg.origin_rank = win->comm_ptr->rank;
        msg.lock_type = MPI_LOCK_SHARED;

        mpi_errno = MPIDI_NM_am_send_hdr(i, win->comm_ptr,
                                         MPIDI_CH4U_WIN_LOCKALL, &msg, sizeof(msg));
        if (mpi_errno != MPI_SUCCESS)
            MPIR_ERR_SETANDSTMT(mpi_errno, MPI_ERR_RMA_SYNC, goto fn_fail, "**rmasync");
    }

    MPIDI_CH4R_PROGRESS_WHILE(size != (int) MPIDI_CH4U_WIN(win, sync).lockall.allLocked);
    MPIDI_CH4U_WIN(win, sync).origin_epoch_type = MPIDI_CH4U_EPOTYPE_LOCK_ALL;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4R_MPI_WIN_LOCK_ALL);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* CH4R_WIN_H_INCLUDED */

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

int MPIDI_CH4R_RMA_Init_sync_pvars(void);
int MPIDI_CH4R_mpi_win_free(MPIR_Win ** win_ptr);
int MPIDI_CH4R_mpi_win_create(void *base, MPI_Aint length, int disp_unit, MPIR_Info * info,
                              MPIR_Comm * comm_ptr, MPIR_Win ** win_ptr);
int MPIDI_CH4R_mpi_win_attach(MPIR_Win * win, void *base, MPI_Aint size);
int MPIDI_CH4R_mpi_win_allocate_shared(MPI_Aint size, int disp_unit, MPIR_Info * info_ptr,
                                       MPIR_Comm * comm_ptr, void **base_ptr, MPIR_Win ** win_ptr);
int MPIDI_CH4R_mpi_win_detach(MPIR_Win * win, const void *base);
int MPIDI_CH4R_mpi_win_shared_query(MPIR_Win * win, int rank, MPI_Aint * size, int *disp_unit,
                                    void *baseptr);
int MPIDI_CH4R_mpi_win_allocate(MPI_Aint size, int disp_unit, MPIR_Info * info, MPIR_Comm * comm,
                                void *baseptr, MPIR_Win ** win_ptr);
int MPIDI_CH4R_mpi_win_create_dynamic(MPIR_Info * info, MPIR_Comm * comm, MPIR_Win ** win_ptr);

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
      next:
        curr_ptr = curr_ptr->next;
    }

    mpi_errno = MPIR_Barrier(win->comm_ptr, &errflag);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4R_MPI_WIN_SET_INFO);
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
        MPIDI_CH4R_PROGRESS();
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
        MPIDI_CH4R_PROGRESS();
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

    if (win->create_flavor == MPI_WIN_FLAVOR_ALLOCATE && win->base) {
        if (MPIDI_CH4U_WIN(win, mmap_sz) > 0)
            MPL_munmap(MPIDI_CH4U_WIN(win, mmap_addr), MPIDI_CH4U_WIN(win, mmap_sz), MPL_MEM_RMA);
        else if (MPIDI_CH4U_WIN(win, mmap_sz) == -1)
            MPL_free(win->base);
    }

    if (win->create_flavor == MPI_WIN_FLAVOR_SHARED) {
        if (MPIDI_CH4U_WIN(win, mmap_sz) > 0) {
            /* destroy shared window memory */
            mpi_errno = MPIDI_CH4U_destroy_shm_segment(MPIDI_CH4U_WIN(win, mmap_sz),
                                                       &MPIDI_CH4U_WIN(win, shm_segment_handle),
                                                       &MPIDI_CH4U_WIN(win, mmap_addr));
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
        }


        MPL_free(MPIDI_CH4U_WIN(win, shared_table));
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
    mpi_errno = MPIR_Barrier(win->comm_ptr, &errflag);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4R_MPI_WIN_FENCE);
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
        MPIDI_CH4R_PROGRESS();
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
        MPIDI_CH4R_PROGRESS();
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
        MPIDI_CH4R_PROGRESS();
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

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

#include "mpidimpl.h"
/* #include "ch4_impl.h" */
/* #include "ch4i_util.h" */
/* #include <opa_primitives.h> */
/* #include "ch4r_symheap.h" */
/* #include "uthash.h" */
#ifdef HAVE_SYS_MMAN_H
/* #include <sys/mman.h> */
#endif /* HAVE_SYS_MMAN_H */

/* BEGIN internal function decls */
int win_init(MPI_Aint length, int disp_unit, MPIR_Win ** win_ptr, MPIR_Info * info,
             MPIR_Comm * comm_ptr, int create_flavor, int model);
int win_finalize(MPIR_Win ** win_ptr);
/* END   internal function decls */

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4_RMA_Init_sync_pvars
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
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

#undef FUNCNAME
#define FUNCNAME win_init
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int win_init(MPI_Aint length, int disp_unit, MPIR_Win ** win_ptr, MPIR_Info * info,
             MPIR_Comm * comm_ptr, int create_flavor, int model)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Win *win = (MPIR_Win *) MPIR_Handle_obj_alloc(&MPIR_Win_mem);
    MPIDIG_win_target_t *targets = NULL;
    MPIR_Comm *win_comm_ptr;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_WIN_INIT);
    MPIR_FUNC_VERBOSE_RMA_ENTER(MPID_STATE_MPIDIG_WIN_INIT);

    MPIR_ERR_CHKANDSTMT(win == NULL, mpi_errno, MPI_ERR_NO_MEM, goto fn_fail, "**nomem");
    *win_ptr = win;

    memset(&win->dev.ch4u, 0, sizeof(MPIDIG_win_t));

    /* Duplicate the original communicator here to avoid having collisions
     * between internal collectives */
    mpi_errno = MPIR_Comm_dup_impl(comm_ptr, &win_comm_ptr);
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

    /* Initialize the info (hint) flags per window */
    MPIDIG_WIN(win, info_args).no_locks = 0;
    MPIDIG_WIN(win, info_args).accumulate_ordering = (MPIDI_ACCU_ORDER_RAR |
                                                      MPIDI_ACCU_ORDER_RAW |
                                                      MPIDI_ACCU_ORDER_WAR |
                                                      MPIDI_ACCU_ORDER_WAW);
    MPIDIG_WIN(win, info_args).accumulate_ops = MPIDI_ACCU_SAME_OP_NO_OP;
    MPIDIG_WIN(win, info_args).same_size = 0;
    MPIDIG_WIN(win, info_args).same_disp_unit = 0;
    MPIDIG_WIN(win, info_args).alloc_shared_noncontig = 0;
    if (win->create_flavor == MPI_WIN_FLAVOR_ALLOCATE
        || win->create_flavor == MPI_WIN_FLAVOR_SHARED) {
        MPIDIG_WIN(win, info_args).alloc_shm = 1;
    } else {
        MPIDIG_WIN(win, info_args).alloc_shm = 0;
    }

    if ((info != NULL) && ((int *) info != (int *) MPI_INFO_NULL)) {
        mpi_errno = MPIDIG_mpi_win_set_info(win, info);
        if (MPI_SUCCESS != mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }


    MPIDIG_WIN(win, mmap_sz) = 0;
    MPIDIG_WIN(win, mmap_addr) = NULL;

    MPIR_cc_set(&MPIDIG_WIN(win, local_cmpl_cnts), 0);
    MPIR_cc_set(&MPIDIG_WIN(win, remote_cmpl_cnts), 0);
    MPIR_cc_set(&MPIDIG_WIN(win, remote_acc_cmpl_cnts), 0);

    MPIDIG_WIN(win, win_id) = MPIDIG_generate_win_id(comm_ptr);
    MPIDIU_map_set(MPIDI_CH4_Global.win_map, MPIDIG_WIN(win, win_id), win, MPL_MEM_RMA);

  fn_exit:
    MPIR_FUNC_VERBOSE_RMA_EXIT(MPID_STATE_MPIDIG_WIN_INIT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME win_finalize
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int win_finalize(MPIR_Win ** win_ptr)
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

        MPIDIG_PROGRESS();

        MPIDI_win_check_all_targets_local_completed(win, &all_local_completed);
        MPIDI_win_check_all_targets_remote_completed(win, &all_remote_completed);

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

    MPIDIG_win_target_cleanall(win);
    MPIDIG_win_hash_clear(win);

    if (win->create_flavor == MPI_WIN_FLAVOR_ALLOCATE && win->base) {
        if (MPIDIG_WIN(win, mmap_sz) > 0)
            MPL_munmap(MPIDIG_WIN(win, mmap_addr), MPIDIG_WIN(win, mmap_sz), MPL_MEM_RMA);
        else if (MPIDIG_WIN(win, mmap_sz) == -1)
            MPL_free(win->base);
    }

    if (win->create_flavor == MPI_WIN_FLAVOR_SHARED) {
        if (MPIDIG_WIN(win, mmap_sz) > 0) {
            /* destroy and detach shared window memory */
            mpi_errno = MPL_shm_seg_detach(MPIDIG_WIN(win, shm_segment_handle),
                                           (char **) &MPIDIG_WIN(win, mmap_addr),
                                           MPIDIG_WIN(win, mmap_sz));
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
            mpi_errno = MPL_shm_hnd_finalize(&MPIDIG_WIN(win, shm_segment_handle));
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
        }


        MPL_free(MPIDIG_WIN(win, shared_table));
    }

    MPIDIU_map_erase(MPIDI_CH4_Global.win_map, MPIDIG_WIN(win, win_id));

    MPIR_Comm_release(win->comm_ptr);
    MPIR_Handle_obj_free(&MPIR_Win_mem, win);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_WIN_FINALIZE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDIG_mpi_win_free
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
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

#undef FUNCNAME
#define FUNCNAME MPIDIG_mpi_win_create
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
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

    mpi_errno = MPIR_Barrier(win->comm_ptr, &errflag);

    if (mpi_errno != MPI_SUCCESS)
        goto fn_fail;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_MPI_WIN_CREATE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDIG_mpi_win_attach
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
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

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_MPI_WIN_ATTACH);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDIG_mpi_win_allocate_shared
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDIG_mpi_win_allocate_shared(MPI_Aint size, int disp_unit, MPIR_Info * info_ptr,
                                   MPIR_Comm * comm_ptr, void **base_ptr, MPIR_Win ** win_ptr)
{
    int i, mpi_errno = MPI_SUCCESS;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    MPIR_Win *win = NULL;
    ssize_t total_size = 0LL;
    MPIDIG_win_shared_info_t *shared_table = NULL;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_MPI_WIN_ALLOCATE_SHARED);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_MPI_WIN_ALLOCATE_SHARED);

    mpi_errno = win_init(size, disp_unit, win_ptr, info_ptr, comm_ptr, MPI_WIN_FLAVOR_SHARED,
                         MPI_WIN_UNIFIED);

    win = *win_ptr;
    MPIR_T_PVAR_TIMER_START(RMA, rma_wincreate_allgather);
    MPIDIG_WIN(win, shared_table) =
        (MPIDIG_win_shared_info_t *) MPL_malloc(sizeof(MPIDIG_win_shared_info_t) *
                                                comm_ptr->local_size, MPL_MEM_RMA);
    shared_table = MPIDIG_WIN(win, shared_table);
    shared_table[comm_ptr->rank].size = size;
    shared_table[comm_ptr->rank].disp_unit = disp_unit;
    shared_table[comm_ptr->rank].shm_base_addr = NULL;

    mpi_errno = MPIR_Allgather(MPI_IN_PLACE,
                               0,
                               MPI_DATATYPE_NULL,
                               shared_table,
                               sizeof(MPIDIG_win_shared_info_t), MPI_BYTE, comm_ptr, &errflag);
    MPIR_T_PVAR_TIMER_END(RMA, rma_wincreate_allgather);
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

    /* allocate symmetric shared window memory */
    size_t page_sz, mapsize;

    mapsize = MPIDIU_get_mapsize(total_size, &page_sz);
    MPIDIG_WIN(win, mmap_sz) = mapsize;

    mpi_errno = MPIDIU_allocate_shm_segment(comm_ptr, mapsize, 1 /* symmetric_flag */ ,
                                            &MPIDIG_WIN(win, shm_segment_handle),
                                            &MPIDIG_WIN(win, mmap_addr));
    if (mpi_errno != MPI_SUCCESS)
        goto fn_fail;

    /* compute the base addresses of each process within the shared memory segment */
    {
        char *cur_base = (char *) MPIDIG_WIN(win, mmap_addr);
        for (i = 0; i < comm_ptr->local_size; i++) {
            if (shared_table[i].size)
                shared_table[i].shm_base_addr = cur_base;
            else
                shared_table[i].shm_base_addr = NULL;

            cur_base += shared_table[i].size;
        }
    }

  fn_zero:
    win->base = shared_table[comm_ptr->rank].shm_base_addr;
    win->size = size;

    mpi_errno = MPIDI_NM_mpi_win_allocate_shared_hook(win);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);

    *(void **) base_ptr = (void *) win->base;
    mpi_errno = MPIR_Barrier(comm_ptr, &errflag);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_MPI_WIN_ALLOCATE_SHARED);
    return mpi_errno;
  fn_fail:
    if (win_ptr)
        win_finalize(win_ptr);
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDIG_mpi_win_detach
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
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

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_MPI_WIN_DETACH);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDIG_mpi_win_shared_query
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDIG_mpi_win_shared_query(MPIR_Win * win, int rank, MPI_Aint * size, int *disp_unit,
                                void *baseptr)
{
    int mpi_errno = MPI_SUCCESS;
    int offset = rank;
    MPIDIG_win_shared_info_t *shared_table = MPIDIG_WIN(win, shared_table);

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_MPI_WIN_SHARED_QUERY);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_MPI_WIN_SHARED_QUERY);

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

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_MPI_WIN_SHARED_QUERY);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDIG_mpi_win_allocate
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDIG_mpi_win_allocate(MPI_Aint size, int disp_unit, MPIR_Info * info, MPIR_Comm * comm,
                            void *baseptr, MPIR_Win ** win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    void *baseP;
    MPIR_Win *win;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_MPI_WIN_ALLOCATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_MPI_WIN_ALLOCATE);

    mpi_errno = win_init(size, disp_unit, win_ptr, info, comm, MPI_WIN_FLAVOR_ALLOCATE,
                         MPI_WIN_UNIFIED);

    if (mpi_errno != MPI_SUCCESS)
        goto fn_fail;

    mpi_errno = MPIDIU_get_symmetric_heap(size, comm, &baseP, *win_ptr);

    if (mpi_errno != MPI_SUCCESS)
        goto fn_fail;

    win = *win_ptr;
    win->base = baseP;

    mpi_errno = MPIDI_NM_mpi_win_allocate_hook(win);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);

    *(void **) baseptr = (void *) win->base;
    mpi_errno = MPIR_Barrier(comm, &errflag);

    if (mpi_errno != MPI_SUCCESS)
        goto fn_fail;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_MPI_WIN_ALLOCATE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDIG_mpi_win_create_dynamic
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
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

    mpi_errno = MPIR_Barrier(comm, &errflag);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_MPI_WIN_CREATE_DYNAMIC);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

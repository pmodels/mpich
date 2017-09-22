/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#include "mpir_info.h"
#include "mpidrma.h"

extern MPIR_T_pvar_timer_t PVAR_TIMER_rma_wincreate_allgather ATTRIBUTE((unused));

#undef FUNCNAME
#define FUNCNAME MPIDI_Win_fns_init
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_Win_fns_init(MPIDI_CH3U_Win_fns_t * win_fns)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_WIN_FNS_INIT);

    MPIR_FUNC_VERBOSE_RMA_ENTER(MPID_STATE_MPIDI_WIN_FNS_INIT);

    win_fns->create = MPIDI_CH3U_Win_create;
    win_fns->allocate = MPIDI_CH3U_Win_allocate;
    win_fns->allocate_shared = MPIDI_CH3U_Win_allocate;
    win_fns->create_dynamic = MPIDI_CH3U_Win_create_dynamic;
    win_fns->gather_info = MPIDI_CH3U_Win_gather_info;
    win_fns->shared_query = MPIDI_CH3U_Win_shared_query;

    MPIR_FUNC_VERBOSE_RMA_EXIT(MPID_STATE_MPIDI_WIN_FNS_INIT);

    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Win_gather_info
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3U_Win_gather_info(void *base, MPI_Aint size, int disp_unit,
                               MPIR_Info * info, MPIR_Comm * comm_ptr, MPIR_Win ** win_ptr)
{
    int mpi_errno = MPI_SUCCESS, i, k, comm_size, rank;
    MPI_Aint *tmp_buf;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    MPIR_CHKPMEM_DECL(1);
    MPIR_CHKLMEM_DECL(1);
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH3U_WIN_GATHER_INFO);

    MPIR_FUNC_VERBOSE_RMA_ENTER(MPID_STATE_MPIDI_CH3U_WIN_GATHER_INFO);

    comm_size = (*win_ptr)->comm_ptr->local_size;
    rank = (*win_ptr)->comm_ptr->rank;

    MPIR_T_PVAR_TIMER_START(RMA, rma_wincreate_allgather);
    /* allocate memory for the base addresses, disp_units, and
     * completion counters of all processes */
    MPIR_CHKPMEM_MALLOC((*win_ptr)->basic_info_table, MPIDI_Win_basic_info_t *,
                        comm_size * sizeof(MPIDI_Win_basic_info_t),
                        mpi_errno, "(*win_ptr)->basic_info_table");

    /* get the addresses of the windows, window objects, and completion
     * counters of all processes.  allocate temp. buffer for communication */
    MPIR_CHKLMEM_MALLOC(tmp_buf, MPI_Aint *, 4 * comm_size * sizeof(MPI_Aint),
                        mpi_errno, "tmp_buf");

    /* FIXME: This needs to be fixed for heterogeneous systems */
    /* FIXME: If we wanted to validate the transfer as within range at the
     * origin, we'd also need the window size. */
    tmp_buf[4 * rank] = MPIR_Ptr_to_aint(base);
    tmp_buf[4 * rank + 1] = size;
    tmp_buf[4 * rank + 2] = (MPI_Aint) disp_unit;
    tmp_buf[4 * rank + 3] = (MPI_Aint) (*win_ptr)->handle;

    mpi_errno = MPIR_Allgather_impl(MPI_IN_PLACE, 0, MPI_DATATYPE_NULL,
                                    tmp_buf, 4, MPI_AINT, (*win_ptr)->comm_ptr, &errflag);
    MPIR_T_PVAR_TIMER_END(RMA, rma_wincreate_allgather);
    if (mpi_errno) {
        MPIR_ERR_POP(mpi_errno);
    }
    MPIR_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");

    k = 0;
    for (i = 0; i < comm_size; i++) {
        (*win_ptr)->basic_info_table[i].base_addr = MPIR_Aint_to_ptr(tmp_buf[k++]);
        (*win_ptr)->basic_info_table[i].size = tmp_buf[k++];
        (*win_ptr)->basic_info_table[i].disp_unit = (int) tmp_buf[k++];
        (*win_ptr)->basic_info_table[i].win_handle = (MPI_Win) tmp_buf[k++];
    }

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    MPIR_FUNC_VERBOSE_RMA_EXIT(MPID_STATE_MPIDI_CH3U_WIN_GATHER_INFO);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    MPIR_CHKPMEM_REAP();
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Win_create
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3U_Win_create(void *base, MPI_Aint size, int disp_unit, MPIR_Info * info,
                          MPIR_Comm * comm_ptr, MPIR_Win ** win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH3U_WIN_CREATE);

    MPIR_FUNC_VERBOSE_RMA_ENTER(MPID_STATE_MPIDI_CH3U_WIN_CREATE);

    mpi_errno = MPIDI_CH3U_Win_fns.gather_info(base, size, disp_unit, info, comm_ptr, win_ptr);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);

    if ((*win_ptr)->info_args.alloc_shm == TRUE && MPIDI_CH3U_Win_fns.detect_shm != NULL) {
        /* Detect if shared buffers are specified for the processes in the
         * current node. If so, enable shm RMA.*/
        mpi_errno = MPIDI_CH3U_Win_fns.detect_shm(win_ptr);
        if (mpi_errno != MPI_SUCCESS)
            MPIR_ERR_POP(mpi_errno);
        goto fn_exit;
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_RMA_EXIT(MPID_STATE_MPIDI_CH3U_WIN_CREATE);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Win_create_dynamic
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3U_Win_create_dynamic(MPIR_Info * info, MPIR_Comm * comm_ptr, MPIR_Win ** win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH3U_WIN_CREATE_DYNAMIC);

    MPIR_FUNC_VERBOSE_RMA_ENTER(MPID_STATE_MPIDI_CH3U_WIN_CREATE_DYNAMIC);

    mpi_errno = MPIDI_CH3U_Win_fns.gather_info(MPI_BOTTOM, 0, 1, info, comm_ptr, win_ptr);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_RMA_EXIT(MPID_STATE_MPIDI_CH3U_WIN_CREATE_DYNAMIC);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

#undef FUNCNAME
#define FUNCNAME MPID_Win_attach
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_Win_attach(MPIR_Win * win, void *base, MPI_Aint size)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_WIN_ATTACH);
    MPIR_FUNC_VERBOSE_RMA_ENTER(MPID_STATE_MPID_WIN_ATTACH);

    /* no op, all of memory is exposed */

  fn_exit:
    MPIR_FUNC_VERBOSE_RMA_EXIT(MPID_STATE_MPID_WIN_ATTACH);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME MPID_Win_detach
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_Win_detach(MPIR_Win * win, const void *base)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_WIN_DETACH);
    MPIR_FUNC_VERBOSE_RMA_ENTER(MPID_STATE_MPID_WIN_DETACH);

    /* no op, all of memory is exposed */

  fn_exit:
    MPIR_FUNC_VERBOSE_RMA_EXIT(MPID_STATE_MPID_WIN_DETACH);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Win_allocate
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3U_Win_allocate(MPI_Aint size, int disp_unit, MPIR_Info * info,
                            MPIR_Comm * comm_ptr, void *baseptr, MPIR_Win ** win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH3U_WIN_ALLOCATE);

    MPIR_FUNC_VERBOSE_RMA_ENTER(MPID_STATE_MPIDI_CH3U_WIN_ALLOCATE);

    if ((*win_ptr)->info_args.alloc_shm == TRUE) {
        if (MPIDI_CH3U_Win_fns.allocate_shm != NULL) {
            mpi_errno =
                MPIDI_CH3U_Win_fns.allocate_shm(size, disp_unit, info, comm_ptr, baseptr, win_ptr);
            if (mpi_errno != MPI_SUCCESS)
                MPIR_ERR_POP(mpi_errno);
            goto fn_exit;
        }
    }

    mpi_errno = MPIDI_CH3U_Win_allocate_no_shm(size, disp_unit, info, comm_ptr, baseptr, win_ptr);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_RMA_EXIT(MPID_STATE_MPIDI_CH3U_WIN_ALLOCATE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Win_allocate_no_shm
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3U_Win_allocate_no_shm(MPI_Aint size, int disp_unit, MPIR_Info * info,
                                   MPIR_Comm * comm_ptr, void *baseptr, MPIR_Win ** win_ptr)
{
    void **base_pp = (void **) baseptr;
    int mpi_errno = MPI_SUCCESS;
    MPIR_CHKPMEM_DECL(1);

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH3U_WIN_ALLOCATE_NO_SHM);
    MPIR_FUNC_VERBOSE_RMA_ENTER(MPID_STATE_MPIDI_CH3U_WIN_ALLOCATE_NO_SHM);

    if (size > 0) {
        MPIR_CHKPMEM_MALLOC(*base_pp, void *, size, mpi_errno, "(*win_ptr)->base");
        MPL_VG_MEM_INIT(*base_pp, size);
    }
    else if (size == 0) {
        *base_pp = NULL;
    }
    else {
        MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_SIZE, "**rmasize");
    }

    (*win_ptr)->base = *base_pp;

    mpi_errno = MPIDI_CH3U_Win_fns.gather_info(*base_pp, size, disp_unit, info, comm_ptr, win_ptr);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_RMA_EXIT(MPID_STATE_MPIDI_CH3U_WIN_ALLOCATE_NO_SHM);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    MPIR_CHKPMEM_REAP();
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Win_shared_query
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3U_Win_shared_query(MPIR_Win * win_ptr, int target_rank, MPI_Aint * size,
                                int *disp_unit, void *baseptr)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH3U_WIN_SHARED_QUERY);
    MPIR_FUNC_VERBOSE_RMA_ENTER(MPID_STATE_MPIDI_CH3U_WIN_SHARED_QUERY);

    *(void **) baseptr = win_ptr->base;
    *size = win_ptr->size;
    *disp_unit = win_ptr->disp_unit;

  fn_exit:
    MPIR_FUNC_VERBOSE_RMA_EXIT(MPID_STATE_MPIDI_CH3U_WIN_SHARED_QUERY);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME MPID_Win_set_info
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_Win_set_info(MPIR_Win * win, MPIR_Info * info)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_WIN_SET_INFO);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_WIN_SET_INFO);

    if (info != NULL) {
        int info_flag = 0;
        char info_value[MPI_MAX_INFO_VAL + 1];

        /********************************************************/
        /************** check for info no_locks *****************/
        /********************************************************/
        MPIR_Info_get_impl(info, "no_locks", MPI_MAX_INFO_VAL, info_value, &info_flag);
        if (info_flag) {
            if (!strncmp(info_value, "true", strlen("true")))
                win->info_args.no_locks = 1;
            if (!strncmp(info_value, "false", strlen("false")))
                win->info_args.no_locks = 0;
        }

        /********************************************************/
        /*************** check for info alloc_shm ***************/
        /********************************************************/
        info_flag = 0;
        MPIR_Info_get_impl(info, "alloc_shm", MPI_MAX_INFO_VAL, info_value, &info_flag);
        if (info_flag) {
            if (!strncmp(info_value, "true", sizeof("true")))
                win->info_args.alloc_shm = TRUE;
            if (!strncmp(info_value, "false", sizeof("false")))
                win->info_args.alloc_shm = FALSE;
        }

        if (win->create_flavor == MPI_WIN_FLAVOR_DYNAMIC)
            win->info_args.alloc_shm = FALSE;

        /********************************************************/
        /******* check for info alloc_shared_noncontig **********/
        /********************************************************/
        info_flag = 0;
        MPIR_Info_get_impl(info, "alloc_shared_noncontig", MPI_MAX_INFO_VAL,
                           info_value, &info_flag);
        if (info_flag) {
            if (!strncmp(info_value, "true", strlen("true")))
                win->info_args.alloc_shared_noncontig = 1;
            if (!strncmp(info_value, "false", strlen("false")))
                win->info_args.alloc_shared_noncontig = 0;
        }

        /********************************************************/
        /******* check for info accumulate_ordering    **********/
        /********************************************************/
        info_flag = 0;
        MPIR_Info_get_impl(info, "accumulate_ordering", MPI_MAX_INFO_VAL, info_value, &info_flag);
        if (info_flag) {
            if (!strncmp(info_value, "none", strlen("none"))) {
                win->info_args.accumulate_ordering = 0;
            }
            else {
                char *token, *save_ptr;
                int new_ordering = 0;

                token = (char *) strtok_r(info_value, ",", &save_ptr);
                while (token) {
                    if (!memcmp(token, "rar", 3))
                        new_ordering |= MPIDI_ACC_ORDER_RAR;
                    else if (!memcmp(token, "raw", 3))
                        new_ordering |= MPIDI_ACC_ORDER_RAW;
                    else if (!memcmp(token, "war", 3))
                        new_ordering |= MPIDI_ACC_ORDER_WAR;
                    else if (!memcmp(token, "waw", 3))
                        new_ordering |= MPIDI_ACC_ORDER_WAW;
                    else
                        MPIR_ERR_SETANDSTMT(mpi_errno, MPI_ERR_ARG, goto fn_fail, "**info");

                    token = (char *) strtok_r(NULL, ",", &save_ptr);
                }

                win->info_args.accumulate_ordering = new_ordering;
            }
        }

        /********************************************************/
        /******* check for info accumulate_ops         **********/
        /********************************************************/
        info_flag = 0;
        MPIR_Info_get_impl(info, "accumulate_ops", MPI_MAX_INFO_VAL, info_value, &info_flag);
        if (info_flag) {
            if (!strncmp(info_value, "same_op", sizeof("same_op")))
                win->info_args.accumulate_ops = MPIDI_ACC_OPS_SAME_OP;
            if (!strncmp(info_value, "same_op_no_op", sizeof("same_op_no_op")))
                win->info_args.accumulate_ops = MPIDI_ACC_OPS_SAME_OP_NO_OP;
        }

        /********************************************************/
        /******* check for info same_size              **********/
        /********************************************************/
        info_flag = 0;
        MPIR_Info_get_impl(info, "same_size", MPI_MAX_INFO_VAL, info_value, &info_flag);
        if (info_flag) {
            if (!strncmp(info_value, "true", sizeof("true")))
                win->info_args.same_size = TRUE;
            if (!strncmp(info_value, "false", sizeof("false")))
                win->info_args.same_size = FALSE;
        }

        /********************************************************/
        /******* check for info same_disp_unit         **********/
        /********************************************************/
        info_flag = 0;
        MPIR_Info_get_impl(info, "same_disp_unit", MPI_MAX_INFO_VAL, info_value, &info_flag);
        if (info_flag) {
            if (!strncmp(info_value, "true", sizeof("true")))
                win->info_args.same_disp_unit = TRUE;
            if (!strncmp(info_value, "false", sizeof("false")))
                win->info_args.same_disp_unit = FALSE;
        }
    }


  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_WIN_SET_INFO);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPID_Win_get_info
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_Win_get_info(MPIR_Win * win, MPIR_Info ** info_used)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_WIN_GET_INFO);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_WIN_GET_INFO);

    /* Allocate an empty info object */
    mpi_errno = MPIR_Info_alloc(info_used);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }

    /* Populate the predefined info keys */
    if (win->info_args.no_locks)
        mpi_errno = MPIR_Info_set_impl(*info_used, "no_locks", "true");
    else
        mpi_errno = MPIR_Info_set_impl(*info_used, "no_locks", "false");

    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }

    {
#define BUFSIZE 32
        char buf[BUFSIZE];
        if (win->info_args.accumulate_ordering == 0) {
            strncpy(buf, "none", BUFSIZE);
        }
        else {
            int c = 0;
            if (win->info_args.accumulate_ordering & MPIDI_ACC_ORDER_RAR)
                c += snprintf(buf + c, BUFSIZE - c, "%srar", (c > 0) ? "," : "");
            if (win->info_args.accumulate_ordering & MPIDI_ACC_ORDER_RAW)
                c += snprintf(buf + c, BUFSIZE - c, "%sraw", (c > 0) ? "," : "");
            if (win->info_args.accumulate_ordering & MPIDI_ACC_ORDER_WAR)
                c += snprintf(buf + c, BUFSIZE - c, "%swar", (c > 0) ? "," : "");
            if (win->info_args.accumulate_ordering & MPIDI_ACC_ORDER_WAW)
                c += snprintf(buf + c, BUFSIZE - c, "%swaw", (c > 0) ? "," : "");
        }

        MPIR_Info_set_impl(*info_used, "accumulate_ordering", buf);
        if (mpi_errno != MPI_SUCCESS) {
            MPIR_ERR_POP(mpi_errno);
        }
#undef BUFSIZE
    }

    if (win->info_args.accumulate_ops == MPIDI_ACC_OPS_SAME_OP)
        mpi_errno = MPIR_Info_set_impl(*info_used, "accumulate_ops", "same_op");
    else
        mpi_errno = MPIR_Info_set_impl(*info_used, "accumulate_ops", "same_op_no_op");

    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }

    if (win->info_args.alloc_shm == TRUE)
        mpi_errno = MPIR_Info_set_impl(*info_used, "alloc_shm", "true");
    else
        mpi_errno = MPIR_Info_set_impl(*info_used, "alloc_shm", "false");

    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }

    if (win->info_args.alloc_shared_noncontig)
        mpi_errno = MPIR_Info_set_impl(*info_used, "alloc_shared_noncontig", "true");
    else
        mpi_errno = MPIR_Info_set_impl(*info_used, "alloc_shared_noncontig", "false");

    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }

    if (win->info_args.same_size)
        mpi_errno = MPIR_Info_set_impl(*info_used, "same_size", "true");
    else
        mpi_errno = MPIR_Info_set_impl(*info_used, "same_size", "false");

    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }

    if (win->info_args.same_disp_unit)
        mpi_errno = MPIR_Info_set_impl(*info_used, "same_disp_unit", "true");
    else
        mpi_errno = MPIR_Info_set_impl(*info_used, "same_disp_unit", "false");
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_WIN_GET_INFO);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

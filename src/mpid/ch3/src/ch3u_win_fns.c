/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#include "mpidrma.h"

#ifdef USE_MPIU_INSTR
MPIU_INSTR_DURATION_EXTERN_DECL(wincreate_allgather);
#endif

#undef FUNCNAME
#define FUNCNAME MPIDI_Win_fns_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_Win_fns_init(MPIDI_CH3U_Win_fns_t *win_fns)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_WIN_FNS_INIT);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_WIN_FNS_INIT);

    win_fns->create             = MPIDI_CH3U_Win_create;
    win_fns->allocate           = MPIDI_CH3U_Win_allocate;
    win_fns->allocate_shared    = MPIDI_CH3U_Win_allocate_shared;
    win_fns->create_dynamic     = MPIDI_CH3U_Win_create_dynamic;

    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_WIN_FNS_INIT);

    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Win_create_gather
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3U_Win_create_gather( void *base, MPI_Aint size, int disp_unit,
                                  MPID_Info *info, MPID_Comm *comm_ptr, MPID_Win **win_ptr )
{
    int mpi_errno=MPI_SUCCESS, i, k, comm_size, rank;
    MPI_Aint *tmp_buf;
    int errflag = FALSE;
    MPIU_CHKPMEM_DECL(5);
    MPIU_CHKLMEM_DECL(1);
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3U_WIN_CREATE_GATHER);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_CH3U_WIN_CREATE_GATHER);

    comm_size = (*win_ptr)->comm_ptr->local_size;
    rank      = (*win_ptr)->comm_ptr->rank;

    MPIU_INSTR_DURATION_START(wincreate_allgather);
    /* allocate memory for the base addresses, disp_units, and
       completion counters of all processes */
    MPIU_CHKPMEM_MALLOC((*win_ptr)->base_addrs, void **,
                        comm_size*sizeof(void *),
                        mpi_errno, "(*win_ptr)->base_addrs");

    MPIU_CHKPMEM_MALLOC((*win_ptr)->sizes, MPI_Aint *, comm_size*sizeof(MPI_Aint),
                        mpi_errno, "(*win_ptr)->sizes");

    MPIU_CHKPMEM_MALLOC((*win_ptr)->disp_units, int *, comm_size*sizeof(int),
                        mpi_errno, "(*win_ptr)->disp_units");

    MPIU_CHKPMEM_MALLOC((*win_ptr)->all_win_handles, MPI_Win *,
                        comm_size*sizeof(MPI_Win),
                        mpi_errno, "(*win_ptr)->all_win_handles");

    MPIU_CHKPMEM_MALLOC((*win_ptr)->pt_rma_puts_accs, int *,
                        comm_size*sizeof(int),
                        mpi_errno, "(*win_ptr)->pt_rma_puts_accs");
    for (i=0; i<comm_size; i++) (*win_ptr)->pt_rma_puts_accs[i] = 0;

    /* get the addresses of the windows, window objects, and completion
       counters of all processes.  allocate temp. buffer for communication */
    MPIU_CHKLMEM_MALLOC(tmp_buf, MPI_Aint *, 4*comm_size*sizeof(MPI_Aint),
                        mpi_errno, "tmp_buf");

    /* FIXME: This needs to be fixed for heterogeneous systems */
    /* FIXME: If we wanted to validate the transfer as within range at the
       origin, we'd also need the window size. */
    tmp_buf[4*rank]   = MPIU_PtrToAint(base);
    tmp_buf[4*rank+1] = size;
    tmp_buf[4*rank+2] = (MPI_Aint) disp_unit;
    tmp_buf[4*rank+3] = (MPI_Aint) (*win_ptr)->handle;

    mpi_errno = MPIR_Allgather_impl(MPI_IN_PLACE, 0, MPI_DATATYPE_NULL,
                                    tmp_buf, 4, MPI_AINT,
                                    (*win_ptr)->comm_ptr, &errflag);
    MPIU_INSTR_DURATION_END(wincreate_allgather);
    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
    MPIU_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");

    k = 0;
    for (i=0; i<comm_size; i++)
    {
        (*win_ptr)->base_addrs[i] = MPIU_AintToPtr(tmp_buf[k++]);
        (*win_ptr)->sizes[i] = tmp_buf[k++];
        (*win_ptr)->disp_units[i] = (int) tmp_buf[k++];
        (*win_ptr)->all_win_handles[i] = (MPI_Win) tmp_buf[k++];
    }

fn_exit:
    MPIU_CHKLMEM_FREEALL();
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_CH3U_WIN_CREATE_GATHER);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Win_create
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3U_Win_create(void *base, MPI_Aint size, int disp_unit, MPID_Info *info,
                         MPID_Comm *comm_ptr, MPID_Win **win_ptr )
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3U_WIN_CREATE);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_CH3U_WIN_CREATE);

    mpi_errno = MPIDI_CH3U_Win_create_gather(base, size, disp_unit, info, comm_ptr, win_ptr);
    if (mpi_errno != MPI_SUCCESS) { MPIU_ERR_POP(mpi_errno); }

fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_CH3U_WIN_CREATE);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Win_create_dynamic
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3U_Win_create_dynamic(MPID_Info *info, MPID_Comm *comm_ptr, MPID_Win **win_ptr )
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3U_WIN_CREATE_DYNAMIC);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_CH3U_WIN_CREATE_DYNAMIC);

    mpi_errno = MPIDI_CH3U_Win_create_gather(MPI_BOTTOM, 0, 1, info, comm_ptr, win_ptr);
    if (mpi_errno != MPI_SUCCESS) { MPIU_ERR_POP(mpi_errno); }

fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_CH3U_WIN_CREATE_DYNAMIC);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME MPIDI_Win_attach
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_Win_attach(MPID_Win *win, void *base, MPI_Aint size)
{
    int mpi_errno = MPI_SUCCESS;

    MPIDI_STATE_DECL(MPID_STATE_MPIDI_WIN_ATTACH);
    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_WIN_ATTACH);

    /* no op, all of memory is exposed */

 fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_WIN_ATTACH);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
 fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME MPIDI_Win_detach
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_Win_detach(MPID_Win *win, const void *base)
{
    int mpi_errno = MPI_SUCCESS;

    MPIDI_STATE_DECL(MPID_STATE_MPIDI_WIN_DETACH);
    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_WIN_DETACH);

    /* no op, all of memory is exposed */

 fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_WIN_DETACH);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
 fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Win_allocate_shared
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3U_Win_allocate_shared(MPI_Aint size, int disp_unit, MPID_Info *info, MPID_Comm *comm_ptr,
                                  void **base_ptr, MPID_Win **win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3U_WIN_ALLOCATE_SHARED);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_CH3U_WIN_ALLOCATE_SHARED);

    MPIU_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**notimpl");

fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_CH3U_WIN_ALLOCATE_SHARED);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Win_allocate
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3U_Win_allocate(MPI_Aint size, int disp_unit, MPID_Info *info,
                           MPID_Comm *comm_ptr, void *baseptr, MPID_Win **win_ptr )
{
    void **base_pp = (void **) baseptr;
    int mpi_errno = MPI_SUCCESS;
    MPIU_CHKPMEM_DECL(1);

    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3U_WIN_ALLOCATE);
    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_CH3U_WIN_ALLOCATE);

    if (size > 0) {
        MPIU_CHKPMEM_MALLOC(*base_pp, void *, size, mpi_errno, "(*win_ptr)->base");
    } else if (size == 0) {
        *base_pp = NULL;
    } else {
        MPIU_ERR_SETANDJUMP(mpi_errno, MPI_ERR_SIZE, "**rmasize");
    }

    (*win_ptr)->base = *base_pp;

    mpi_errno = MPIDI_CH3U_Win_create_gather(*base_pp, size, disp_unit, info, comm_ptr, win_ptr);
    if (mpi_errno != MPI_SUCCESS) { MPIU_ERR_POP(mpi_errno); }

fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_CH3U_WIN_ALLOCATE);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

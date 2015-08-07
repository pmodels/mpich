/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2011 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "mpl_utlist.h"

/* -- Begin Profiling Symbol Block for routine MPI_T_pvar_handle_alloc */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_T_pvar_handle_alloc = PMPI_T_pvar_handle_alloc
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_T_pvar_handle_alloc  MPI_T_pvar_handle_alloc
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_T_pvar_handle_alloc as PMPI_T_pvar_handle_alloc
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_T_pvar_handle_alloc(MPI_T_pvar_session session, int pvar_index, void *obj_handle,
                            MPI_T_pvar_handle *handle, int *count) __attribute__((weak,alias("PMPI_T_pvar_handle_alloc")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_T_pvar_handle_alloc
#define MPI_T_pvar_handle_alloc PMPI_T_pvar_handle_alloc

/* Define storage for the ALL_HANDLES constant */
MPIR_T_pvar_handle_t MPIR_T_pvar_all_handles_obj =
{
#ifdef HAVE_ERROR_CHECKING
MPIR_T_PVAR_HANDLE, /* pvar handle tag for error checking */
#endif
0
};
MPIR_T_pvar_handle_t * const MPI_T_PVAR_ALL_HANDLES = &MPIR_T_pvar_all_handles_obj;

/* any non-MPI functions go here, especially non-static ones */

#undef FUNCNAME
#define FUNCNAME MPIR_T_pvar_handle_alloc_impl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_T_pvar_handle_alloc_impl(MPI_T_pvar_session session, int pvar_index,
                                  void *obj_handle, MPI_T_pvar_handle *handle,int *count)
{
    int mpi_errno = MPI_SUCCESS;
    int cnt, extra, bytes;
    int is_sum, is_watermark;
    const pvar_table_entry_t *info;
    MPIR_T_pvar_handle_t *hnd;

    MPIU_CHKPMEM_DECL(1);

    info = (pvar_table_entry_t *) utarray_eltptr(pvar_table, pvar_index);

    if (info->get_count == NULL) {
        cnt = info->count;
    } else {
        info->get_count(info->addr, obj_handle, &cnt);
    }

    bytes = MPID_Datatype_get_basic_size(info->datatype);
    is_sum = FALSE;
    is_watermark = FALSE;
    extra = 0;

    if (info->varclass == MPI_T_PVAR_CLASS_COUNTER ||
        info->varclass == MPI_T_PVAR_CLASS_AGGREGATE ||
        info->varclass == MPI_T_PVAR_CLASS_TIMER)
    {
        /* Extra memory for accum, offset, current */
        is_sum = TRUE;
        extra = bytes * cnt * 3;
    } else if (info->varclass == MPI_T_PVAR_CLASS_HIGHWATERMARK ||
               info->varclass == MPI_T_PVAR_CLASS_LOWWATERMARK)
    {
        is_watermark = TRUE;
    }

    /* Allocate memory and bzero it */
    MPIU_CHKPMEM_CALLOC(hnd, MPIR_T_pvar_handle_t*, sizeof(*hnd) + extra,
                        mpi_errno, "performance variable handle");
#ifdef HAVE_ERROR_CHECKING
    hnd->kind = MPIR_T_PVAR_HANDLE;
#endif

    /* Setup the common fields */
    if (is_sum)
        hnd->flags |= MPIR_T_PVAR_FLAG_SUM;
    else if (is_watermark)
        hnd->flags |= MPIR_T_PVAR_FLAG_WATERMARK;

    hnd->addr = info->addr;
    hnd->datatype = info->datatype;
    hnd->count = cnt;
    hnd->varclass = info->varclass;
    hnd->flags = info->flags;
    hnd->session = session;
    hnd->info = info;
    hnd->obj_handle = obj_handle;
    hnd->get_value = info->get_value;
    hnd->bytes = bytes;
    hnd->count = cnt;

    /* Init pointers to cache buffers for a SUM */
    if (MPIR_T_pvar_is_sum(hnd)) {
        hnd->accum = (char*)(hnd) + sizeof(*hnd);
        hnd->offset = (char*)(hnd) + sizeof(*hnd) + bytes*cnt;
        hnd->current = (char*)(hnd) + sizeof(*hnd) + bytes*cnt*2;
    }

    if (MPIR_T_pvar_is_continuous(hnd))
        MPIR_T_pvar_set_started(hnd);

    /* Set starting value of a continuous SUM */
    if (MPIR_T_pvar_is_continuous(hnd) && MPIR_T_pvar_is_sum(hnd)) {
        /* Cache current value of a SUM in offset.
         * accum is zero since we called CALLOC before.
         */
        if (hnd->get_value == NULL)
            MPIU_Memcpy(hnd->offset, hnd->addr, bytes*cnt);
        else
            hnd->get_value(hnd->addr, hnd->obj_handle, hnd->count, hnd->offset);
    }

     /* Link a WATERMARK handle to its pvar & set starting value if continuous */
    if (MPIR_T_pvar_is_watermark(hnd)) {
        MPIR_T_pvar_watermark_t *mark = (MPIR_T_pvar_watermark_t *)hnd->addr;
        if (!mark->first_used) {
            /* Use the special handle slot for optimization if available */
            mark->first_used = TRUE;
            MPIR_T_pvar_set_first(hnd);

            /* Set starting value */
            if (MPIR_T_pvar_is_continuous(hnd)) {
                mark->first_started = TRUE;
                mark->watermark = mark->current;
            } else {
                mark->first_started = FALSE;
            }
        } else {
            /* If the special handle slot is unavailable, link it to hlist */
            if (mark->hlist == NULL) {
                hnd->prev2 = hnd;
                mark->hlist = hnd;
            } else {
                hnd->prev2 = hnd;
                hnd->next2 = mark->hlist;
                mark->hlist->prev2 = hnd;
                mark->hlist = hnd;
            }

            /* Set starting value */
            if (MPIR_T_pvar_is_continuous(hnd))
                hnd->watermark = mark->current;
        }
    }

    /* Link the handle in its session and return it */
    MPL_DL_APPEND(session->hlist, hnd);
    *handle = hnd;
    *count = cnt;

    MPIU_CHKPMEM_COMMIT();
fn_exit:
    return mpi_errno;
fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
}

#endif /* MPICH_MPI_FROM_PMPI */

#undef FUNCNAME
#define FUNCNAME MPI_T_pvar_handle_alloc
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
MPI_T_pvar_handle_alloc - Allocate a handle for a performance variable

Input Parameters:
+ session - identifier of performance experiment session (handle)
. pvar_index - index of performance variable for which handle is to be allocated (integer)
- obj_handle - reference to a handle of the MPI object to which this variable is supposed to be bound (pointer)

Output Parameters:
+ handle - allocated handle (handle)
- count - number of elements used to represent this variable (integer)

.N ThreadSafe

.N Errors
.N MPI_SUCCESS
.N MPI_T_ERR_NOT_INITIALIZED
.N MPI_T_ERR_INVALID_SESSION
.N MPI_T_ERR_INVALID_INDEX
.N MPI_T_ERR_OUT_OF_HANDLES
@*/
int MPI_T_pvar_handle_alloc(MPI_T_pvar_session session, int pvar_index,
                            void *obj_handle, MPI_T_pvar_handle *handle, int *count)
{
    int mpi_errno = MPI_SUCCESS;
    pvar_table_entry_t *entry;

    MPID_MPI_STATE_DECL(MPID_STATE_MPI_T_PVAR_HANDLE_ALLOC);
    MPIR_ERRTEST_MPIT_INITIALIZED(mpi_errno);
    MPIR_T_THREAD_CS_ENTER();
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_T_PVAR_HANDLE_ALLOC);

    /* Validate parameters  */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS
        {
            MPIR_ERRTEST_PVAR_SESSION(session, mpi_errno);
            MPIR_ERRTEST_PVAR_INDEX(pvar_index, mpi_errno);
            MPIR_ERRTEST_ARGNULL(count, "count", mpi_errno);
            MPIR_ERRTEST_ARGNULL(handle, "handle", mpi_errno);
            /* Do not test obj_handle since it may be NULL when no binding */
        }
        MPID_END_ERROR_CHECKS
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    entry = (pvar_table_entry_t *) utarray_eltptr(pvar_table, pvar_index);
    if (!entry->active) {
        mpi_errno = MPI_T_ERR_INVALID_INDEX;
        goto fn_fail;
    }

    mpi_errno = MPIR_T_pvar_handle_alloc_impl(session, pvar_index, obj_handle, handle, count);
    if (mpi_errno) goto fn_fail;

    /* ... end of body of routine ... */

fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_T_PVAR_HANDLE_ALLOC);
    MPIR_T_THREAD_CS_EXIT();
    return mpi_errno;

fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno = MPIR_Err_create_code(
            mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
            "**mpi_t_pvar_handle_alloc", "**mpi_t_pvar_handle_alloc %p %d %p %p %p", session, pvar_index, obj_handle, handle, count);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm(NULL, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

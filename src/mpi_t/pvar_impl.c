/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "utlist.h"

/* Define storage for the ALL_HANDLES constant */
MPIR_T_pvar_handle_t MPIR_T_pvar_all_handles_obj = { 0 };

MPIR_T_pvar_handle_t *const MPI_T_PVAR_ALL_HANDLES = &MPIR_T_pvar_all_handles_obj;

void MPIR_T_pvar_env_init(void)
{
    int i;
    static const UT_icd pvar_table_entry_icd = { sizeof(pvar_table_entry_t), NULL, NULL, NULL };

    utarray_new(pvar_table, &pvar_table_entry_icd, MPL_MEM_MPIT);
    for (i = 0; i < MPIR_T_PVAR_CLASS_NUMBER; i++) {
        pvar_hashs[i] = NULL;
    }

#ifdef HAVE_ERROR_CHECKING
    MPIR_T_pvar_all_handles_obj.kind = MPIR_T_PVAR_HANDLE;
#endif
}

int MPIR_T_pvar_handle_alloc_impl(MPI_T_pvar_session session, int pvar_index,
                                  void *obj_handle, MPI_T_pvar_handle * handle, int *count)
{
    int mpi_errno = MPI_SUCCESS;
    int cnt, extra, bytes;
    int is_sum, is_watermark;
    const pvar_table_entry_t *info;
    MPIR_T_pvar_handle_t *hnd;

    info = (pvar_table_entry_t *) utarray_eltptr(pvar_table, pvar_index);

    if (info->get_count == NULL) {
        cnt = info->count;
    } else {
        info->get_count(info->addr, obj_handle, &cnt);
    }

    bytes = MPIR_Datatype_get_basic_size(info->datatype);
    is_sum = FALSE;
    is_watermark = FALSE;
    extra = 0;

    if (info->varclass == MPI_T_PVAR_CLASS_COUNTER ||
        info->varclass == MPI_T_PVAR_CLASS_AGGREGATE || info->varclass == MPI_T_PVAR_CLASS_TIMER) {
        /* Extra memory for accum, offset, current */
        is_sum = TRUE;
        extra = bytes * cnt * 3;
    } else if (info->varclass == MPI_T_PVAR_CLASS_HIGHWATERMARK ||
               info->varclass == MPI_T_PVAR_CLASS_LOWWATERMARK) {
        is_watermark = TRUE;
    }

    /* Allocate memory and bzero it */
    hnd = MPL_malloc(sizeof(MPIR_T_pvar_handle_t) + extra, MPL_MEM_MPIT);
    if (!hnd) {
        *handle = MPI_T_PVAR_HANDLE_NULL;
        mpi_errno = MPI_T_ERR_OUT_OF_HANDLES;
        goto fn_fail;
    }
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
        hnd->accum = (char *) (hnd) + sizeof(*hnd);
        hnd->offset = (char *) (hnd) + sizeof(*hnd) + bytes * cnt;
        hnd->current = (char *) (hnd) + sizeof(*hnd) + bytes * cnt * 2;
    }

    if (MPIR_T_pvar_is_continuous(hnd))
        MPIR_T_pvar_set_started(hnd);

    /* Set starting value of a continuous SUM */
    if (MPIR_T_pvar_is_continuous(hnd) && MPIR_T_pvar_is_sum(hnd)) {
        /* Cache current value of a SUM in offset.
         * accum is zero since we called CALLOC before.
         */
        if (hnd->get_value == NULL)
            MPIR_Memcpy(hnd->offset, hnd->addr, bytes * cnt);
        else
            hnd->get_value(hnd->addr, hnd->obj_handle, hnd->count, hnd->offset);
    }

    /* Link a WATERMARK handle to its pvar & set starting value if continuous */
    if (MPIR_T_pvar_is_watermark(hnd)) {
        MPIR_T_pvar_watermark_t *mark = (MPIR_T_pvar_watermark_t *) hnd->addr;
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
    DL_APPEND(session->hlist, hnd);
    *handle = hnd;
    *count = cnt;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_T_pvar_handle_free_impl(MPI_T_pvar_session session, MPI_T_pvar_handle * handle)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_T_pvar_handle_t *hnd = *handle;

    DL_DELETE(session->hlist, hnd);

    /* Unlink handle from pvar if it is a watermark */
    if (MPIR_T_pvar_is_watermark(hnd)) {
        MPIR_T_pvar_watermark_t *mark = (MPIR_T_pvar_watermark_t *) hnd->addr;
        if (MPIR_T_pvar_is_first(hnd)) {
            mark->first_used = 0;
            mark->first_started = 0;
        } else {
            MPIR_Assert(mark->hlist);
            if (mark->hlist == hnd) {
                /* hnd happens to be the head */
                mark->hlist = hnd->next2;
                if (mark->hlist != NULL)
                    mark->hlist->prev2 = mark->hlist;
            } else {
                hnd->prev2->next2 = hnd->next2;
                if (hnd->next2 != NULL)
                    hnd->next2->prev2 = hnd->prev2;
            }
        }
    }

    MPL_free(hnd);
    *handle = MPI_T_PVAR_HANDLE_NULL;

    return mpi_errno;
}

int MPIR_T_pvar_read_impl(MPI_T_pvar_session session, MPI_T_pvar_handle handle, void *restrict buf)
{
    int i, mpi_errno = MPI_SUCCESS;

    /* Reading a never started pvar, or a stopped and then reset wartermark,
     * will run into this nasty situation. MPI-3.0 did not define what error
     * code should be returned. We returned a generic MPI error code. With MPI-3.1
     * approved, we changed it to MPI_T_ERR_INVALID.
     */
    if (!MPIR_T_pvar_is_oncestarted(handle)) {
        mpi_errno = MPI_T_ERR_INVALID;
        goto fn_fail;
    }

    /* For SUM pvars, return accum value + current value - offset value */
    if (MPIR_T_pvar_is_sum(handle) && MPIR_T_pvar_is_started(handle)) {
        if (handle->get_value == NULL) {
            /* A running SUM without callback. Read its current value directly */
            switch (handle->datatype) {
                case MPI_UNSIGNED_LONG_LONG:
                    /* Put long long and double at front, since they are common */
                    for (i = 0; i < handle->count; i++) {
                        ((unsigned long long *) buf)[i] = ((unsigned long long *) handle->accum)[i]
                            + ((unsigned long long *) handle->addr)[i]
                            - ((unsigned long long *) handle->offset)[i];
                    }
                    break;
                case MPI_DOUBLE:
                    for (i = 0; i < handle->count; i++) {
                        ((double *) buf)[i] = ((double *) handle->accum)[i]
                            + ((double *) handle->addr)[i]
                            - ((double *) handle->offset)[i];
                    }
                    break;
                case MPI_UNSIGNED:
                    for (i = 0; i < handle->count; i++) {
                        ((unsigned *) buf)[i] = ((unsigned *) handle->accum)[i]
                            + ((unsigned *) handle->addr)[i]
                            - ((unsigned *) handle->offset)[i];
                    }
                    break;
                case MPI_UNSIGNED_LONG:
                    for (i = 0; i < handle->count; i++) {
                        ((unsigned long *) buf)[i] = ((unsigned long *) handle->accum)[i]
                            + ((unsigned long *) handle->addr)[i]
                            - ((unsigned long *) handle->offset)[i];
                    }
                    break;
                default:
                    /* Code should never come here */
                    mpi_errno = MPI_ERR_INTERN;
                    goto fn_fail;
                    break;
            }
        } else {
            /* A running SUM with callback. Read its current value into handle */
            handle->get_value(handle->addr, handle->obj_handle, handle->count, handle->current);

            switch (handle->datatype) {
                case MPI_UNSIGNED_LONG_LONG:
                    for (i = 0; i < handle->count; i++) {
                        ((unsigned long long *) buf)[i] = ((unsigned long long *) handle->accum)[i]
                            + ((unsigned long *) handle->current)[i]
                            - ((unsigned long long *) handle->offset)[i];
                    }
                    break;
                case MPI_DOUBLE:
                    for (i = 0; i < handle->count; i++) {
                        ((double *) buf)[i] = ((double *) handle->accum)[i]
                            + ((double *) handle->current)[i]
                            - ((double *) handle->offset)[i];
                    }
                    break;
                case MPI_UNSIGNED:
                    for (i = 0; i < handle->count; i++) {
                        ((unsigned *) buf)[i] = ((unsigned *) handle->accum)[i]
                            + ((unsigned *) handle->current)[i]
                            - ((unsigned *) handle->offset)[i];
                    }
                    break;
                case MPI_UNSIGNED_LONG:
                    for (i = 0; i < handle->count; i++) {
                        ((unsigned long *) buf)[i] = ((unsigned long *) handle->accum)[i]
                            + ((unsigned long *) handle->current)[i]
                            - ((unsigned long *) handle->offset)[i];
                    }
                    break;
                default:
                    /* Code should never come here */
                    mpi_errno = MPI_ERR_INTERN;
                    goto fn_fail;
                    break;
            }
        }
    } else if (MPIR_T_pvar_is_sum(handle) && !MPIR_T_pvar_is_started(handle)) {
        /* A SUM is stopped. Return accum directly */
        MPIR_Memcpy(buf, handle->accum, handle->bytes * handle->count);
    } else if (MPIR_T_pvar_is_watermark(handle)) {
        /* Callback and array are not allowed for watermarks, since they
         * can not guarantee correct semantics of watermarks.
         */
        MPIR_Assert(handle->get_value == NULL && handle->count == 1);

        if (MPIR_T_pvar_is_first(handle)) {
            /* Current value of the first handle of a watermark is stored at
             * a special location nearby the watermark itself.
             */
            switch (handle->datatype) {
                case MPI_UNSIGNED_LONG_LONG:
                    *(unsigned long long *) buf =
                        ((MPIR_T_pvar_watermark_t *) handle->addr)->watermark.ull;
                    break;
                case MPI_DOUBLE:
                    *(double *) buf = ((MPIR_T_pvar_watermark_t *) handle->addr)->watermark.f;
                    break;
                case MPI_UNSIGNED:
                    *(unsigned *) buf = ((MPIR_T_pvar_watermark_t *) handle->addr)->watermark.u;
                    break;
                case MPI_UNSIGNED_LONG:
                    *(unsigned long *) buf =
                        ((MPIR_T_pvar_watermark_t *) handle->addr)->watermark.ul;
                    break;
                default:
                    /* Code should never come here */
                    mpi_errno = MPI_ERR_INTERN;
                    goto fn_fail;
                    break;
            }
        } else {
            /* For remaining handles, their current value are in the handle */
            switch (handle->datatype) {
                case MPI_UNSIGNED_LONG_LONG:
                    *(unsigned long long *) buf = handle->watermark.ull;
                    break;
                case MPI_DOUBLE:
                    *(double *) buf = handle->watermark.f;
                    break;
                case MPI_UNSIGNED:
                    *(unsigned *) buf = handle->watermark.u;
                    break;
                case MPI_UNSIGNED_LONG:
                    *(unsigned long *) buf = handle->watermark.ul;
                    break;
                default:
                    /* Code should never come here */
                    mpi_errno = MPI_ERR_INTERN;
                    goto fn_fail;
                    break;
            }
        }
    } else {
        /* For STATE, LEVEL, SIZE, PERCENTAGE, no caching is needed */
        if (handle->get_value == NULL)
            MPIR_Memcpy(buf, handle->addr, handle->bytes * handle->count);
        else
            handle->get_value(handle->addr, handle->obj_handle, handle->count, buf);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_T_pvar_readreset_impl(MPI_T_pvar_session session, MPI_T_pvar_handle handle, void *buf)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_T_pvar_read_impl(session, handle, buf);
    if (mpi_errno != MPI_SUCCESS)
        goto fn_fail;

    mpi_errno = MPIR_T_pvar_reset_impl(session, handle);
    if (mpi_errno != MPI_SUCCESS)
        goto fn_fail;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_T_pvar_reset_impl(MPI_T_pvar_session session, MPI_T_pvar_handle handle)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_T_pvar_watermark_t *mark;

    if (MPIR_T_pvar_is_sum(handle)) {
        /* Use zero as starting value */
        memset(handle->accum, 0, handle->bytes * handle->count);

        /* Record current value as offset when pvar is running (i.e., started) */
        if (MPIR_T_pvar_is_started(handle)) {
            if (handle->get_value == NULL) {
                MPIR_Memcpy(handle->offset, handle->addr, handle->bytes * handle->count);
            } else {
                handle->get_value(handle->addr, handle->obj_handle, handle->count, handle->offset);
            }
        }
    } else if (MPIR_T_pvar_is_watermark(handle)) {
        if (MPIR_T_pvar_is_started(handle)) {
            /* Use the current value as starting value when pvar is running */
            mark = (MPIR_T_pvar_watermark_t *) handle->addr;
            if (MPIR_T_pvar_is_first(handle)) {
                MPIR_Assert(mark->first_used);
                mark->watermark = mark->current;
            } else {
                handle->watermark = mark->current;
            }
        } else {
            /* If pvar is stopped, clear the oncestarted flag
             * so that when it is re-started, it looks new.
             */
            MPIR_T_pvar_unset_oncestarted(handle);
        }
    }

    return mpi_errno;
}

int MPIR_T_pvar_session_create_impl(MPI_T_pvar_session * session)
{
    int mpi_errno = MPI_SUCCESS;

    *session = MPL_malloc(sizeof(MPI_T_pvar_session), MPL_MEM_MPIT);
    if (!*session) {
        *session = MPI_T_PVAR_SESSION_NULL;
        mpi_errno = MPI_T_ERR_OUT_OF_SESSIONS;
        goto fn_fail;
    }

    /* essential for utlist to work */
    (*session)->hlist = NULL;

#ifdef HAVE_ERROR_CHECKING
    (*session)->kind = MPIR_T_PVAR_SESSION;
#endif

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_T_pvar_session_free_impl(MPI_T_pvar_session * session)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_T_pvar_handle_t *hnd, *tmp;

    /* The draft standard is a bit unclear about the behavior of this function
     * w.r.t. outstanding pvar handles.  A conservative (from the implementors
     * viewpoint) interpretation implies that this routine should free all
     * outstanding handles attached to this session.  A more relaxed
     * interpretation puts that burden on the user.  We choose the conservative
     * view.*/
    DL_FOREACH_SAFE((*session)->hlist, hnd, tmp) {
        DL_DELETE((*session)->hlist, hnd);
        MPL_free(hnd);
    }
    MPL_free(*session);
    *session = MPI_T_PVAR_SESSION_NULL;

    return mpi_errno;
}

int MPIR_T_pvar_start_impl(MPI_T_pvar_session session, MPI_T_pvar_handle handle)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_T_pvar_watermark_t *mark;

    if (MPIR_T_pvar_is_sum(handle)) {
        /* To start SUM, we only need to cache its current value into offset.
         * If it has ever been started, accum holds correct value. Otherwise,
         * accum is zero since handle allocation.
         */
        if (handle->get_value == NULL) {
            MPIR_Memcpy(handle->offset, handle->addr, handle->bytes * handle->count);
        } else {
            handle->get_value(handle->addr, handle->obj_handle, handle->count, handle->offset);
        }
    } else if (MPIR_T_pvar_is_watermark(handle)) {
        /* To start WATERMARK, if it has not ever been started,
         * we need to set its starting value properly.
         */
        mark = (MPIR_T_pvar_watermark_t *) handle->addr;

        if (MPIR_T_pvar_is_first(handle)) {
            MPIR_Assert(mark->first_used);
            mark->first_started = TRUE;
            if (!MPIR_T_pvar_is_oncestarted(handle))
                mark->watermark = mark->current;
        } else {
            if (!MPIR_T_pvar_is_oncestarted(handle))
                handle->watermark = mark->current;
        }
    }

    /* OK, let's get started */
    MPIR_T_pvar_set_started(handle);

    return mpi_errno;
}

int MPIR_T_pvar_stop_impl(MPI_T_pvar_session session, MPI_T_pvar_handle handle)
{
    int i, mpi_errno = MPI_SUCCESS;
    MPIR_T_pvar_watermark_t *mark;

    MPIR_T_pvar_unset_started(handle);

    /* Side-effect when pvar is SUM or WATERMARK */
    if (MPIR_T_pvar_is_sum(handle)) {
        /* Read the current value first */
        if (handle->get_value == NULL) {
            MPIR_Memcpy(handle->current, handle->addr, handle->bytes * handle->count);
        } else {
            handle->get_value(handle->addr, handle->obj_handle, handle->count, handle->current);
        }

        /* Subtract offset from current, and accumulate the result to accum */
        switch (handle->datatype) {
            case MPI_UNSIGNED_LONG_LONG:
                for (i = 0; i < handle->count; i++) {
                    ((unsigned long long *) handle->accum)[i] +=
                        ((unsigned long long *) handle->current)[i]
                        - ((unsigned long long *) handle->offset)[i];
                }
                break;
            case MPI_DOUBLE:
                for (i = 0; i < handle->count; i++) {
                    ((double *) handle->accum)[i] += ((double *) handle->current)[i]
                        - ((double *) handle->offset)[i];
                }
                break;
            case MPI_UNSIGNED:
                for (i = 0; i < handle->count; i++) {
                    ((unsigned *) handle->accum)[i] += ((unsigned *) handle->current)[i]
                        - ((unsigned *) handle->offset)[i];
                }
                break;
            case MPI_UNSIGNED_LONG:
                for (i = 0; i < handle->count; i++) {
                    ((unsigned long *) handle->accum)[i] += ((unsigned long *) handle->current)[i]
                        - ((unsigned long *) handle->offset)[i];
                }
                break;
            default:
                /* Code should never come here */
                mpi_errno = MPI_ERR_INTERN;
                goto fn_fail;
                break;
        }

    } else if (MPIR_T_pvar_is_watermark(handle)) {
        /* When handle is first, clear the flag in pvar too */
        if (MPIR_T_pvar_is_first(handle)) {
            mark = (MPIR_T_pvar_watermark_t *) handle->addr;
            MPIR_Assert(mark->first_used);
            mark->first_started = FALSE;
        }
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_T_pvar_write_impl(MPI_T_pvar_session session, MPI_T_pvar_handle handle, const void *buf)
{
    /* This function should never be called */
    return MPI_ERR_INTERN;
}

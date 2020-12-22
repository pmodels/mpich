/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

int MPIR_T_category_get_categories_impl(int cat_index, int len, int indices[])
{
    int mpi_errno = MPI_SUCCESS;
    cat_table_entry_t *cat;
    int i, num_subcats, count;

    cat = (cat_table_entry_t *) utarray_eltptr(cat_table, cat_index);
    num_subcats = utarray_len(cat->subcat_indices);
    count = len < num_subcats ? len : num_subcats;

    for (i = 0; i < count; i++) {
        indices[i] = *(int *) utarray_eltptr(cat->subcat_indices, i);
    }

    return mpi_errno;
}

int MPIR_T_category_get_cvars_impl(int cat_index, int len, int indices[])
{
    int mpi_errno = MPI_SUCCESS;
    cat_table_entry_t *cat;
    int i, num_cvars, count;

    cat = (cat_table_entry_t *) utarray_eltptr(cat_table, cat_index);
    num_cvars = utarray_len(cat->cvar_indices);
    count = len < num_cvars ? len : num_cvars;

    for (i = 0; i < count; i++) {
        indices[i] = *(int *) utarray_eltptr(cat->cvar_indices, i);
    }

    return mpi_errno;
}

int MPIR_T_category_get_pvars_impl(int cat_index, int len, int indices[])
{
    int mpi_errno = MPI_SUCCESS;
    cat_table_entry_t *cat;
    int i, num_pvars, count;

    cat = (cat_table_entry_t *) utarray_eltptr(cat_table, cat_index);
    num_pvars = utarray_len(cat->pvar_indices);
    count = len < num_pvars ? len : num_pvars;

    for (i = 0; i < count; i++) {
        indices[i] = *(int *) utarray_eltptr(cat->pvar_indices, i);
    }

    return mpi_errno;
}

int MPIR_T_cvar_handle_alloc_impl(int cvar_index, void *obj_handle, MPI_T_cvar_handle * handle,
                                  int *count)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_T_cvar_handle_t *hnd;

    cvar_table_entry_t *cvar = (cvar_table_entry_t *) utarray_eltptr(cvar_table, cvar_index);

    /* Allocate handle memory */
    hnd = MPL_malloc(sizeof(MPIR_T_cvar_handle_t), MPL_MEM_MPIT);
    if (!hnd) {
        *handle = MPI_T_CVAR_HANDLE_NULL;
        mpi_errno = MPI_T_ERR_OUT_OF_HANDLES;
        goto fn_fail;
    }
#ifdef HAVE_ERROR_CHECKING
    hnd->kind = MPIR_T_CVAR_HANDLE;
#endif

    /* It is time to fix addr and count if they are unknown */
    if (cvar->get_count != NULL)
        cvar->get_count(obj_handle, count);
    else
        *count = cvar->count;

    hnd->count = *count;

    if (cvar->get_addr != NULL)
        cvar->get_addr(obj_handle, &(hnd->addr));
    else
        hnd->addr = cvar->addr;

    /* Cache other fields */
    hnd->datatype = cvar->datatype;
    hnd->scope = cvar->scope;

    *handle = hnd;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_T_cvar_read_impl(MPI_T_cvar_handle handle, void *buf)
{
    int mpi_errno = MPI_SUCCESS;
    int i, count;
    void *addr;
    MPIR_T_cvar_handle_t *hnd = handle;

    count = hnd->count;
    addr = hnd->addr;
    MPIT_Assert(addr != NULL);

    switch (hnd->datatype) {
        case MPI_INT:
            for (i = 0; i < count; i++)
                ((int *) buf)[i] = ((int *) addr)[i];
            break;
        case MPI_UNSIGNED:
            for (i = 0; i < count; i++)
                ((unsigned *) buf)[i] = ((unsigned *) addr)[i];
            break;
        case MPI_UNSIGNED_LONG:
            for (i = 0; i < count; i++)
                ((unsigned long *) buf)[i] = ((unsigned long *) addr)[i];
            break;
        case MPI_UNSIGNED_LONG_LONG:
            for (i = 0; i < count; i++)
                ((unsigned long long *) buf)[i] = ((unsigned long long *) addr)[i];
            break;
        case MPI_DOUBLE:
            for (i = 0; i < count; i++)
                ((double *) buf)[i] = ((double *) addr)[i];
            break;
        case MPI_CHAR:
            MPL_strncpy(buf, addr, count);
            break;
        default:
            mpi_errno = MPI_T_ERR_INVALID;
            goto fn_fail;
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_T_cvar_write_impl(MPI_T_cvar_handle handle, const void *buf)
{
    int mpi_errno = MPI_SUCCESS;
    unsigned int i, count;
    void *addr;
    MPIR_T_cvar_handle_t *hnd = handle;

    count = hnd->count;
    addr = hnd->addr;
    MPIT_Assert(addr != NULL);

    switch (hnd->datatype) {
        case MPI_INT:
            for (i = 0; i < count; i++)
                ((int *) addr)[i] = ((int *) buf)[i];
            break;
        case MPI_UNSIGNED:
            for (i = 0; i < count; i++)
                ((unsigned *) addr)[i] = ((unsigned *) buf)[i];
            break;
        case MPI_UNSIGNED_LONG:
            for (i = 0; i < count; i++)
                ((unsigned long *) addr)[i] = ((unsigned long *) buf)[i];
            break;
        case MPI_UNSIGNED_LONG_LONG:
            for (i = 0; i < count; i++)
                ((unsigned long long *) addr)[i] = ((unsigned long long *) buf)[i];
            break;
        case MPI_DOUBLE:
            for (i = 0; i < count; i++)
                ((double *) addr)[i] = ((double *) buf)[i];
            break;
        case MPI_CHAR:
            MPIT_Assert(count > strlen(buf));   /* Make sure buf will not overflow this cvar */
            MPL_strncpy(addr, buf, count);
            break;
        default:
            mpi_errno = MPI_T_ERR_INVALID;
            goto fn_fail;
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

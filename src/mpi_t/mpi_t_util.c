/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2011 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* this file contains helper routines for the implementation of the MPI_T
 * interface */

#include "mpiimpl.h"
#include "mpiu_utarray.h"

/* A dynamic array of pointers to all known performance variables.  The array
 * will grow dynamically as variables are added by lower level initialization
 * code. */
static UT_array *all_pvars = NULL;

/* Called by lower-level initialization code to add pvars to the global list.
 * Will cause the value returned by MPIX_T_pvar_get_num to be incremented and
 * sets up that new index to work with get_info, handle_alloc, etc. */
int MPIR_T_pvar_add(const char *name,
                    enum MPIR_T_verbosity_t verbosity,
                    enum MPIR_T_pvar_class_t varclass,
                    MPI_Datatype dtype,
                    struct MPIR_T_enum *enumtype,
                    const char *desc,
                    enum MPIR_T_bind_t binding,
                    int readonly,
                    int continuous,
                    int atomic,
                    enum MPIR_T_pvar_impl_kind impl_kind,
                    void *var_state,
                    MPIR_T_pvar_handle_creator_fn *create_fn,
                    int *index_p)
{
    struct MPIR_T_pvar_info *info;

    if (!all_pvars) {
        utarray_new(all_pvars, &ut_ptr_icd);
    }

    info = MPIU_Malloc(sizeof(*info));
    MPIU_Assert(info != NULL);

    info->name = MPIU_Strdup((name ? name : ""));
    MPIU_Assert(info->name);
    info->verbosity = verbosity;
    info->varclass = varclass;
    info->dtype = dtype;
    info->etype = enumtype;
    info->desc = MPIU_Strdup((desc ? desc : ""));
    MPIU_Assert(info->desc);
    info->binding = binding;
    info->readonly = readonly;
    info->continuous = continuous;
    info->atomic = atomic;
    info->impl_kind = impl_kind;
    info->var_state = var_state;
    info->create_fn = create_fn;

    utarray_push_back(all_pvars, &info);
    *index_p = utarray_len(all_pvars);

    return MPI_SUCCESS;
}

int MPIR_T_get_num_pvars(int *num)
{
    MPIU_Assert(num != NULL);
    if (all_pvars) {
        *num = utarray_len(all_pvars);
    }
    else {
        *num = 0;
    }
    return MPI_SUCCESS;
}

int MPIR_T_get_pvar_info_by_idx(int idx, struct MPIR_T_pvar_info **info_p)
{
    MPIU_Assert(idx >= 0);
    MPIU_Assert(all_pvars != NULL);
    MPIU_Assert(idx < utarray_len(all_pvars));

    *info_p = *(struct MPIR_T_pvar_info **)utarray_eltptr(all_pvars, idx);
    return MPI_SUCCESS;
}

int MPIR_T_finalize_pvars(void)
{
    struct MPIR_T_pvar_info **info_p;
    if (all_pvars) {
        for (info_p = (struct MPIR_T_pvar_info **)utarray_front(all_pvars);
             info_p != NULL;
             info_p = (struct MPIR_T_pvar_info **)utarray_next(all_pvars, info_p))
        {
            MPIU_Free((*info_p)->name);
            MPIU_Free((*info_p)->desc);
            MPIU_Free((*info_p));
        }
        utarray_free(all_pvars);
        all_pvars = NULL;
    }

    return MPI_SUCCESS;
}

/* Implements an MPI_T-style strncpy.  Here is the description from the draft
 * standard:
 *
 *   Several MPI tool information interface functions return one or more
 *   strings. These functions have two arguments for each string to be returned:
 *   an OUT parameter that identifies a pointer to the buffer in which the
 *   string will be returned, and an IN/OUT parameter to pass the length of the
 *   buffer. The user is responsible for the memory allocation of the buffer and
 *   must pass the size of the buffer (n) as the length argument. Let n be the
 *   length value specified to the function. On return, the function writes at
 *   most n − 1 of the string’s characters into the buffer, followed by a null
 *   terminator. If the returned string’s length is greater than or equal to n,
 *   the string will be truncated to n − 1 characters. In this case, the length
 *   of the string plus one (for the terminating null character) is returned in
 *   the length argument. If the user passes the null pointer as the buffer
 *   argument or passes 0 as the length argument, the function does not return
 *   the string and only returns the length of the string plus one in the length
 *   argument. If the user passes the null pointer as the length argument, the
 *   buffer argument is ignored and nothing is returned.
 *
 * So this routine copies up to (*len)-1 characters from src to dst and then
 * sets *len to (strlen(dst)+1).  If dst==NULL, just return (strlen(src)+1) in
 * *len.
 *
 * This routine does not follow MPICH error handling conventions.
 */
void MPIU_Tool_strncpy(char *dst, const char *src, int *len)
{
    MPIU_Assert(len);
    MPIU_Assert(*len >= 0);

    if (!dst || !len) {
        /* just return the space needed to hold src, including the terminator */
        *len = strlen(src);
    }
    else {
        /* MPL_strncpy will always terminate the string */
        MPL_strncpy(dst, src, *len);
        *len = strlen(dst);
    }
}


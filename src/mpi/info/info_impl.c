/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* All the MPIR_Info routines may be called before initialization or after finalization of MPI. */
int MPIR_Info_delete_impl(MPIR_Info * info_ptr, const char *key)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Info *prev_ptr, *curr_ptr;

    prev_ptr = info_ptr;
    curr_ptr = info_ptr->next;

    while (curr_ptr) {
        if (!strncmp(curr_ptr->key, key, MPI_MAX_INFO_KEY)) {
            /* MPI_Info objects are allocated by MPL_direct_malloc(), so they need to be
             * freed by MPL_direct_free(), not MPL_free(). */
            MPL_direct_free(curr_ptr->key);
            MPL_direct_free(curr_ptr->value);
            prev_ptr->next = curr_ptr->next;
            MPIR_Info_handle_obj_free(&MPIR_Info_mem, curr_ptr);
            break;
        }
        prev_ptr = curr_ptr;
        curr_ptr = curr_ptr->next;
    }

    /* If curr_ptr is not defined, we never found the key */
    MPIR_ERR_CHKANDJUMP1((!curr_ptr), mpi_errno, MPI_ERR_INFO_NOKEY, "**infonokey",
                         "**infonokey %s", key);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Info_dup_impl(MPIR_Info * info_ptr, MPIR_Info ** new_info_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Info *curr_old, *curr_new;

    *new_info_ptr = NULL;
    if (!info_ptr)
        goto fn_exit;

    /* Note that this routine allocates info elements one at a time.
     * In the multithreaded case, each allocation may need to acquire
     * and release the allocation lock.  If that is ever a problem, we
     * may want to add an "allocate n elements" routine and execute this
     * it two steps: count and then allocate */
    /* FIXME : multithreaded */
    mpi_errno = MPIR_Info_alloc(&curr_new);
    MPIR_ERR_CHECK(mpi_errno);
    *new_info_ptr = curr_new;

    curr_old = info_ptr->next;
    while (curr_old) {
        mpi_errno = MPIR_Info_alloc(&curr_new->next);
        MPIR_ERR_CHECK(mpi_errno);

        curr_new = curr_new->next;
        /* MPI_Info objects may not be allocated by MPL_strdup() since MPL_strdup() may not be
         * called before MPI_Init() and the allocated memory must have been freed before
         * MPI_Finalize() while MPI-4 allows calling MPI_Info routines before MPI_Init() and
         * after MPI_Finalize(). */
        curr_new->key = MPL_direct_strdup(curr_old->key);
        curr_new->value = MPL_direct_strdup(curr_old->value);

        curr_old = curr_old->next;
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Info_get_impl(MPIR_Info * info_ptr, const char *key, int valuelen, char *value, int *flag)
{
    MPIR_Info *curr_ptr;
    int err = 0, mpi_errno = 0;

    curr_ptr = info_ptr->next;
    *flag = 0;

    while (curr_ptr) {
        if (!strncmp(curr_ptr->key, key, MPI_MAX_INFO_KEY)) {
            err = MPL_strncpy(value, curr_ptr->value, valuelen + 1);
            /* +1 because the MPI Standard says "In C, valuelen
             * (passed to MPI_Info_get) should be one less than the
             * amount of allocated space to allow for the null
             * terminator*/
            *flag = 1;
            break;
        }
        curr_ptr = curr_ptr->next;
    }

    /* --BEGIN ERROR HANDLING-- */
    if (err != 0) {
        mpi_errno =
            MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, __func__, __LINE__,
                                 MPI_ERR_INFO_VALUE, "**infovallong", NULL);
    }
    /* --END ERROR HANDLING-- */

    return mpi_errno;
}

int MPIR_Info_get_nkeys_impl(MPIR_Info * info_ptr, int *nkeys)
{
    int n;

    info_ptr = info_ptr->next;
    n = 0;

    while (info_ptr) {
        info_ptr = info_ptr->next;
        n++;
    }
    *nkeys = n;

    return MPI_SUCCESS;
}

int MPIR_Info_get_nthkey_impl(MPIR_Info * info_ptr, int n, char *key)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Info *curr_ptr;
    int nkeys;

    curr_ptr = info_ptr->next;
    nkeys = 0;
    while (curr_ptr && nkeys != n) {
        curr_ptr = curr_ptr->next;
        nkeys++;
    }

    /* verify that n is valid */
    MPIR_ERR_CHKANDJUMP2((!curr_ptr), mpi_errno, MPI_ERR_ARG, "**infonkey", "**infonkey %d %d", n,
                         nkeys);

    /* if key is MPI_MAX_INFO_KEY long, MPL_strncpy will null-terminate it for
     * us */
    MPL_strncpy(key, curr_ptr->key, MPI_MAX_INFO_KEY);
    /* Eventually, we could remember the location of this key in
     * the head using the key/value locations (and a union datatype?) */

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Info_get_valuelen_impl(MPIR_Info * info_ptr, const char *key, int *valuelen, int *flag)
{
    MPIR_Info *curr_ptr;

    curr_ptr = info_ptr->next;
    *flag = 0;

    while (curr_ptr) {
        if (!strncmp(curr_ptr->key, key, MPI_MAX_INFO_KEY)) {
            *valuelen = (int) strlen(curr_ptr->value);
            *flag = 1;
            break;
        }
        curr_ptr = curr_ptr->next;
    }

    return MPI_SUCCESS;
}

int MPIR_Info_set_impl(MPIR_Info * info_ptr, const char *key, const char *value)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Info *curr_ptr, *prev_ptr;

    MPIR_FUNC_ENTER;

    prev_ptr = info_ptr;
    curr_ptr = info_ptr->next;

    while (curr_ptr) {
        if (!strncmp(curr_ptr->key, key, MPI_MAX_INFO_KEY)) {
            /* Key already present; replace value */

            /* MPI_Info objects may not be allocated by MPL_strdup() since MPL_strdup() may not be
             * called before MPI_Init() and the allocated memory must have been freed before
             * MPI_Finalize() while MPI-4 allows calling MPI_Info routines before MPI_Init() and
             * after MPI_Finalize().  For the same reason, we need to use free, not MPL_free(). */
            MPL_direct_free(curr_ptr->value);
            curr_ptr->value = MPL_direct_strdup(value);
            break;
        }
        prev_ptr = curr_ptr;
        curr_ptr = curr_ptr->next;
    }

    if (!curr_ptr) {
        /* Key not present, insert value */
        mpi_errno = MPIR_Info_alloc(&curr_ptr);
        MPIR_ERR_CHECK(mpi_errno);

        /*printf("Inserting new elm %x at %x\n", curr_ptr->id, prev_ptr->id); */
        prev_ptr->next = curr_ptr;
        /* MPI_Info objects must be allocated by MPL_direct_strdup(), not MPL_strdup() because
         * this function may be called before MPI_Init() and after MPI_Finalize(). */
        curr_ptr->key = MPL_direct_strdup(key);
        curr_ptr->value = MPL_direct_strdup(value);
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

int MPIR_Info_get_string_impl(MPIR_Info * info_ptr, const char *key, int *buflen, char *value,
                              int *flag)
{
    MPIR_Info *curr_ptr;

    curr_ptr = info_ptr->next;
    *flag = 0;

    while (curr_ptr) {
        if (!strncmp(curr_ptr->key, key, MPI_MAX_INFO_KEY)) {
            int old_buflen = *buflen;
            /* It needs to include a terminator. */
            int new_buflen = (int) (strlen(curr_ptr->value) + 1);
            if (old_buflen > 0) {
                /* Copy the value. */
                MPL_strncpy(value, curr_ptr->value, old_buflen);
                /* No matter whether MPL_strncpy() returns an error or not
                 * (i.e., whether curr_ptr->value fits value or not),
                 * it is not an error. */
            }
            *buflen = new_buflen;
            *flag = 1;
            break;
        }
        curr_ptr = curr_ptr->next;
    }

    return MPI_SUCCESS;
}

int MPIR_Info_create_env_impl(int argc, char **argv, MPIR_Info ** new_info_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    /* Allocate an empty info object. */
    MPIR_Info *info_ptr = NULL;
    mpi_errno = MPIR_Info_alloc(&info_ptr);
    MPIR_ERR_CHECK(mpi_errno);
    /* Set up the info value. */
    MPIR_Info_setup_env(info_ptr);

    *new_info_ptr = info_ptr;

  fn_exit:
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

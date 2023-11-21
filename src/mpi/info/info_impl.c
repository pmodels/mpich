/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

static int info_find_key(MPIR_Info * info_ptr, const char *key)
{
    for (int i = 0; i < info_ptr->size; i++) {
        if (strncmp(info_ptr->entries[i].key, key, MPI_MAX_INFO_KEY) == 0) {
            return i;
        }
    }
    return -1;
}

const char *MPIR_Info_lookup(MPIR_Info * info_ptr, const char *key)
{
    if (!info_ptr) {
        return NULL;
    }

    for (int i = 0; i < info_ptr->size; i++) {
        if (strncmp(info_ptr->entries[i].key, key, MPI_MAX_INFO_KEY) == 0) {
            return info_ptr->entries[i].value;
        }
    }
    return NULL;
}

/* All the MPIR_Info routines may be called before initialization or after finalization of MPI. */
int MPIR_Info_delete_impl(MPIR_Info * info_ptr, const char *key)
{
    int mpi_errno = MPI_SUCCESS;

    int found_index = info_find_key(info_ptr, key);
    MPIR_ERR_CHKANDJUMP1((found_index < 0), mpi_errno, MPI_ERR_INFO_NOKEY, "**infonokey",
                         "**infonokey %s", key);

    /* MPI_Info objects are allocated by MPL_direct_malloc(), so they need to be
     * freed by MPL_direct_free(), not MPL_free(). */
    MPL_direct_free(info_ptr->entries[found_index].key);
    MPL_direct_free(info_ptr->entries[found_index].value);

    /* move up the later entries */
    for (int i = found_index + 1; i < info_ptr->size; i++) {
        info_ptr->entries[i - 1] = info_ptr->entries[i];
    }
    info_ptr->size--;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Info_dup_impl(MPIR_Info * info_ptr, MPIR_Info ** new_info_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    *new_info_ptr = NULL;
    if (!info_ptr)
        goto fn_exit;

    MPIR_Info *info_new;
    mpi_errno = MPIR_Info_alloc(&info_new);
    MPIR_ERR_CHECK(mpi_errno);
    *new_info_ptr = info_new;

    for (int i = 0; i < info_ptr->size; i++) {
        MPIR_Info_push(info_new, info_ptr->entries[i].key, info_ptr->entries[i].value);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Info_get_impl(MPIR_Info * info_ptr, const char *key, int valuelen, char *value, int *flag)
{
    int mpi_errno = MPI_SUCCESS;

    const char *v = MPIR_Info_lookup(info_ptr, key);
    if (!v) {
        *flag = 0;
    } else {
        *flag = 1;
        /* +1 because the MPI Standard says "In C, valuelen
         * (passed to MPI_Info_get) should be one less than the
         * amount of allocated space to allow for the null
         * terminator*/
        int err = MPL_strncpy(value, v, valuelen + 1);
        if (err != 0) {
            mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, __func__, __LINE__,
                                             MPI_ERR_INFO_VALUE, "**infovallong", NULL);
        }
    }

    return mpi_errno;
}

int MPIR_Info_get_nkeys_impl(MPIR_Info * info_ptr, int *nkeys)
{
    *nkeys = info_ptr->size;

    return MPI_SUCCESS;
}

int MPIR_Info_get_nthkey_impl(MPIR_Info * info_ptr, int n, char *key)
{
    int mpi_errno = MPI_SUCCESS;

    /* verify that n is valid */
    MPIR_ERR_CHKANDJUMP2((n >= info_ptr->size), mpi_errno, MPI_ERR_ARG, "**infonkey",
                         "**infonkey %d %d", n, info_ptr->size);

    /* if key is MPI_MAX_INFO_KEY long, MPL_strncpy will null-terminate it for
     * us */
    MPL_strncpy(key, info_ptr->entries[n].key, MPI_MAX_INFO_KEY);
    /* Eventually, we could remember the location of this key in
     * the head using the key/value locations (and a union datatype?) */

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Info_get_valuelen_impl(MPIR_Info * info_ptr, const char *key, int *valuelen, int *flag)
{
    const char *v = MPIR_Info_lookup(info_ptr, key);

    if (!v) {
        *flag = 0;
    } else {
        *valuelen = (int) strlen(v);
        *flag = 1;
    }

    return MPI_SUCCESS;
}

int MPIR_Info_set_impl(MPIR_Info * info_ptr, const char *key, const char *value)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    int found_index = info_find_key(info_ptr, key);
    if (found_index < 0) {
        /* Key not present, insert value */
        mpi_errno = MPIR_Info_push(info_ptr, key, value);
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        /* Key already present; replace value */
        MPL_direct_free(info_ptr->entries[found_index].value);
        info_ptr->entries[found_index].value = MPL_direct_strdup(value);
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
    const char *v = MPIR_Info_lookup(info_ptr, key);
    if (!v) {
        *flag = 0;
    } else {
        *flag = 1;

        int old_buflen = *buflen;
        /* It needs to include a terminator. */
        int new_buflen = (int) (strlen(v) + 1);
        if (old_buflen > 0) {
            /* Copy the value. */
            MPL_strncpy(value, v, old_buflen);
            /* No matter whether MPL_strncpy() returns an error or not
             * (i.e., whether value fits or not), it is not an error. */
        }
        *buflen = new_buflen;
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
    MPIR_Info_setup_env(info_ptr, argc, argv);

    *new_info_ptr = info_ptr;

  fn_exit:
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

int MPIR_Info_set_hex_impl(MPIR_Info * info_ptr, const char *key, const void *value, int value_size)
{
    int mpi_errno = MPI_SUCCESS;

    char value_buf[1024];

    int rc;
    int len_out;
    rc = MPL_hex_encode(value, value_size, value_buf, 1024, &len_out);
    MPIR_Assertp(rc == MPL_SUCCESS);

    mpi_errno = MPIR_Info_set_impl(info_ptr, key, value_buf);

    return mpi_errno;
}

int MPIR_Info_decode_hex(const char *str, void *buf, int len)
{
    int mpi_errno = MPI_SUCCESS;

    int len_out;
    int rc = MPL_hex_decode(str, buf, len, &len_out);
    MPIR_ERR_CHKANDJUMP(rc || len_out != len, mpi_errno, MPI_ERR_OTHER, "**infohexinvalid");

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

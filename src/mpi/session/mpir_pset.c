/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpir_pset.h"

/* Global array of known psets managed by the process manager */
UT_array *pm_pset_array;

/* Mutex to protect global pset array from concurrent accesses of
 * MPI process and PMIx event thread
 *
 * This is required independent of multi-threaded support
 * because the PMIx event thread and MPI worker process/thread
 * may access the global PM pset array concurrently -
 * MPID_THREAD_CS_ENTER and MPID_THREAD_CS_EXIT cannot be used here.
 */
MPID_Thread_mutex_t pm_pset_mutex;

/**
 * @brief Copy function for MPIR_Psets in UT_arrays
 */
static
void pset_copy(void *_dst, const void *_src)
{
    MPIR_Pset *dst = (MPIR_Pset *) _dst;
    MPIR_Pset *src = (MPIR_Pset *) _src;

    dst->is_valid = src->is_valid;
    dst->size = src->size;
    dst->uri = src->uri ? MPL_strdup(src->uri) : NULL;
    dst->members = MPL_malloc(sizeof(MPIR_Pset_member) * src->size, MPL_MEM_SESSION);
    memcpy(dst->members, src->members, sizeof(MPIR_Pset_member) * src->size);
}

/**
 * @brief Destructor for MPIR_Psets in UT_arrays.
 */
static
void pset_dtor(void *_elt)
{
    MPIR_Pset *elt = (MPIR_Pset *) _elt;
    if (elt->uri != NULL)
        MPL_free(elt->uri);

    if (elt->members != NULL)
        MPL_free(elt->members);
}

/**
 * @brief Structure used to work with MPIR_Psets in UT_arrays. Configures init, copy and destructor methods for MPIR_Psets.
 */
static const UT_icd pset_array_icd = { sizeof(MPIR_Pset), NULL, pset_copy, pset_dtor };

/**
 * @brief   Find pset by its name (not thread-safe).
 */
static
int pset_find_by_name(const char *pset_name, MPIR_Pset ** pset)
{
    int ret = MPI_ERR_OTHER;
    MPIR_Pset *p = NULL;
    *pset = NULL;
    for (unsigned i = 0; i < utarray_len(pm_pset_array); i++) {
        p = (MPIR_Pset *) utarray_eltptr(pm_pset_array, i);
        if (strncasecmp(pset_name, p->uri, MAX(strlen(pset_name), strlen(p->uri))) == 0) {
            ret = MPI_SUCCESS;
            *pset = p;
            break;
        }
    }
    return ret;
}

int MPIR_Pset_by_name(const char *pset_name, MPIR_Pset ** pset)
{
    int ret, thr_err;

    MPID_Thread_mutex_lock(&pm_pset_mutex, &thr_err);
    MPIR_Assert(thr_err == MPI_SUCCESS);

    if (pset_name != NULL) {
        ret = pset_find_by_name(pset_name, pset);
    } else {
        ret = MPI_ERR_OTHER;
    }

    MPID_Thread_mutex_unlock(&pm_pset_mutex, &thr_err);
    MPIR_Assert(thr_err == MPI_SUCCESS);

    return ret;
}

int MPIR_Pset_by_idx(int idx, MPIR_Pset ** pset)
{
    int ret = MPI_SUCCESS;
    int thr_err;

    MPID_Thread_mutex_lock(&pm_pset_mutex, &thr_err);
    MPIR_Assert(thr_err == MPI_SUCCESS);

    if (idx >= 0 && idx < MPIR_Pset_count()) {
        *pset = (MPIR_Pset *) utarray_eltptr(pm_pset_array, idx);
    } else {
        ret = MPI_ERR_OTHER;
    }

    MPID_Thread_mutex_unlock(&pm_pset_mutex, &thr_err);
    MPIR_Assert(thr_err == MPI_SUCCESS);

    return ret;
}

int MPIR_Pset_init(void)
{
    int mpi_errno;

    utarray_new(pm_pset_array, &pset_array_icd, MPL_MEM_SESSION);
    MPID_Thread_mutex_create(&pm_pset_mutex, &mpi_errno);

    return mpi_errno;
}

int MPIR_Pset_finalize(void)
{
    int mpi_errno = MPI_SUCCESS;

    utarray_free(pm_pset_array);
    MPID_Thread_mutex_destroy(&pm_pset_mutex, &mpi_errno);

    return mpi_errno;
}

int MPIR_Pset_count(void)
{
    unsigned count = utarray_len(pm_pset_array);

    /* Overflow check */
    MPIR_Assert(count < INT_MAX);

    return (int) count;
}

int MPIR_Pset_add(MPIR_Pset * pset)
{
    int ret, thr_err;
    MPIR_Pset *p;

    MPID_Thread_mutex_lock(&pm_pset_mutex, &thr_err);
    MPIR_Assert(thr_err == MPI_SUCCESS);

    if (pset_find_by_name(pset->uri, &p) == MPI_ERR_OTHER) {
        /* Pset with uri NOT found in the parray */
        utarray_push_back(pm_pset_array, pset, MPL_MEM_SESSION);
        ret = MPI_SUCCESS;
    } else {
        ret = MPI_ERR_OTHER;
    }

    MPID_Thread_mutex_unlock(&pm_pset_mutex, &thr_err);
    MPIR_Assert(thr_err == MPI_SUCCESS);

    return ret;
}

int MPIR_Pset_invalidate(char *pset_name)
{
    int ret, thr_err;
    MPIR_Pset *p = NULL;

    MPID_Thread_mutex_lock(&pm_pset_mutex, &thr_err);
    MPIR_Assert(thr_err == MPI_SUCCESS);

    ret = pset_find_by_name(pset_name, &p);
    if (ret == MPI_SUCCESS) {
        p->is_valid = false;    /* Set to invalid */
    }

    MPID_Thread_mutex_unlock(&pm_pset_mutex, &thr_err);
    MPIR_Assert(thr_err == MPI_SUCCESS);

    return ret;
}

int MPIR_Pset_member_compare(void const *a, void const *b)
{
    /* This function is used to compare to pset members on qsort;
     * a and b are of tpye (MPIR_Pset_member *) which is an (int *) for now.
     * Hence, we can use an arithmetic operation to compare */
    return *(const int *) a - *(const int *) b;
}

int MPIR_Pset_member_is_self(MPIR_Pset_member * member)
{
    return ((int) *member == MPIR_Process.rank);
}

/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpir_pset.h"

/* Global array of known "external" psets, e.g managed by the process manager
 * or other pset sources */
UT_array *pset_array;

/* Mutex to protect global pset array from concurrent accesses of
 * MPI process and event threads of other pset sources such as PMIx
 *
 * This is required independent of multi-threaded support
 * because an event thread and MPI worker process/thread
 * may access the global pset array concurrently -
 * MPID_THREAD_CS_ENTER and MPID_THREAD_CS_EXIT cannot be used here.
 */
MPID_Thread_mutex_t pset_mutex;

#define MPIR_PSET_NUM_DEFAULT_PSETS 2
#define MPIR_PSET_WORLD_NAME "mpi://WORLD"
#define MPIR_PSET_SELF_NAME "mpi://SELF"

/* Copy function for MPIR_Psets in UT_arrays */
static
void pset_copy(void *_dst, const void *_src)
{
    MPIR_Pset *dst = (MPIR_Pset *) _dst;
    MPIR_Pset *src = (MPIR_Pset *) _src;

    dst->name = src->name ? MPL_strdup(src->name) : NULL;
    MPIR_Group_dup(src->group, NULL, &(dst->group));
}

/* Destructor for MPIR_Psets in UT_arrays */
static
void pset_dtor(void *_elt)
{
    MPIR_Pset *elt = (MPIR_Pset *) _elt;
    if (elt->name != NULL)
        MPL_free(elt->name);

    if (elt->group != NULL)
        MPIR_Group_free_impl(elt->group);
}

static const UT_icd pset_array_icd = { sizeof(MPIR_Pset), NULL, pset_copy, pset_dtor };

/* Find pset by its name (not thread-safe)
 * Returns true if pset is found in the global list, false otherwise */
static
bool pset_find_by_name(const char *pset_name, MPIR_Pset ** pset)
{
    bool found = false;
    MPIR_Pset *p = NULL;
    *pset = NULL;
    for (unsigned i = 0; i < utarray_len(pset_array); i++) {
        p = (MPIR_Pset *) utarray_eltptr(pset_array, i);
        if (strncasecmp(pset_name, p->name, MAX(strlen(pset_name), strlen(p->name))) == 0) {
            found = true;
            *pset = p;
            break;
        }
    }

    return found;
}

/* Initialize global external pset facilities */
int MPIR_Pset_init(void)
{
    int mpi_errno = MPI_SUCCESS;

    utarray_new(pset_array, &pset_array_icd, MPL_MEM_GROUP);
    MPID_Thread_mutex_create(&pset_mutex, &mpi_errno);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIR_pmi_pset_event_init();
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* Finalize global external pset facilities */
int MPIR_Pset_finalize(void)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_pmi_pset_event_finalize();
    MPIR_ERR_CHECK(mpi_errno);

    utarray_free(pset_array);
    MPID_Thread_mutex_destroy(&pset_mutex, &mpi_errno);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* Add an external pset to the global list (thread-safe)
 * Returns true if pset was added and false if pset was not added because
 * a pset with this name is already in the list. */
bool MPIR_Pset_add(MPIR_Pset * pset)
{
    int thr_err;
    bool added = false;
    MPIR_Pset *p;

    MPID_Thread_mutex_lock(&pset_mutex, &thr_err);
    MPIR_Assert(thr_err == MPI_SUCCESS);

    if (!pset_find_by_name(pset->name, &p)) {
        /* Pset with uri NOT found in the parray */
        utarray_push_back(pset_array, pset, MPL_MEM_GROUP);
        added = true;
    }

    MPID_Thread_mutex_unlock(&pset_mutex, &thr_err);
    MPIR_Assert(thr_err == MPI_SUCCESS);

    return added;
}

/* Add all MPI default psets to the session */
int MPIR_Session_add_default_psets(MPIR_Session * session_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    session_ptr->num_psets = MPIR_PSET_NUM_DEFAULT_PSETS;
    session_ptr->psets = MPL_malloc(session_ptr->num_psets * sizeof(struct MPIR_Pset),
                                    MPL_MEM_GROUP);
    MPIR_ERR_CHKANDJUMP(!session_ptr->psets, mpi_errno, MPI_ERR_OTHER, "**nomem");

    session_ptr->psets[0].name = MPL_strdup(MPIR_PSET_WORLD_NAME);
    mpi_errno = MPIR_Group_dup(MPIR_GROUP_WORLD_PTR, session_ptr, &session_ptr->psets[0].group);
    MPIR_ERR_CHECK(mpi_errno);

    session_ptr->psets[1].name = MPL_strdup(MPIR_PSET_SELF_NAME);
    mpi_errno = MPIR_Group_dup(MPIR_GROUP_SELF_PTR, session_ptr, &session_ptr->psets[1].group);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* Update a session's list of psets with any changes in the global external pset list (thread-safe) */
int MPIR_Session_update_psets(MPIR_Session * session_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    int thr_err;
    MPIR_Pset *pset = NULL;

    MPID_Thread_mutex_lock(&pset_mutex, &thr_err);
    MPIR_Assert(thr_err == MPI_SUCCESS);

    /* Shortcut for no global external psets */
    if (utarray_len(pset_array) == 0) {
        goto fn_exit;
    }

    /* Iterate over global pset list and check for new psets */
    for (unsigned i = session_ptr->global_pset_idx; i < utarray_len(pset_array); i++) {
        pset = (MPIR_Pset *) utarray_eltptr(pset_array, i);
        MPIR_Assert(pset != NULL);

        /* Check if session knows this pset already */
        int found = 0;
        for (int k = MPIR_PSET_NUM_DEFAULT_PSETS; k < session_ptr->num_psets; k++) {
            if (strcmp(session_ptr->psets[k].name, pset->name) == 0) {
                found = 1;
                break;
            }
        }

        if (!found) {
            session_ptr->psets = (MPIR_Pset *) MPL_realloc(session_ptr->psets,
                                                           (session_ptr->num_psets +
                                                            1) * sizeof(MPIR_Pset), MPL_MEM_GROUP);
            MPIR_ERR_CHKANDJUMP(!session_ptr->psets, mpi_errno, MPI_ERR_OTHER, "**nomem");

            /* Create a copy of the pset in the session */
            pset_copy(&(session_ptr->psets[session_ptr->num_psets]), pset);
            MPIR_Group_set_session_ptr(session_ptr->psets[session_ptr->num_psets].group,
                                       session_ptr);
            session_ptr->num_psets++;

            /* Next update: Start iterating the global list at the next element */
            session_ptr->global_pset_idx = i + 1;
        }
    }

  fn_exit:
    MPID_Thread_mutex_unlock(&pset_mutex, &thr_err);
    MPIR_Assert(thr_err == MPI_SUCCESS);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

int pmi_init(void);
void pmi_finalize(void);

#if !defined USE_PMI1_API && !defined USE_PMI2_API && !defined USE_PMIX_API
#define USE_PMI1_API
#endif

#ifdef USE_PMI2_API
/* PMI does not specify a max size for jobid_size in PMI2_Job_GetId.
   CH3 uses jobid_size=MAX_JOBID_LEN=1024 when calling
   PMI2_Job_GetId. */
#define MPIDI_MAX_JOBID_LEN PMI2_MAX_VALLEN
#endif

int pmi_init(void)
{
    int mpi_errno = MPI_SUCCESS;
#if defined(USE_PMI1_API)
    int max_pmi_name_length;
    int pmi_errno = PMI_Init(&has_parent);

    if (pmi_errno != PMI_SUCCESS) {
        MPIR_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**pmi_init", "**pmi_init %d", pmi_errno);
    }

    pmi_errno = PMI_Get_rank(&rank);

    if (pmi_errno != PMI_SUCCESS) {
        MPIR_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**pmi_get_rank",
                             "**pmi_get_rank %d", pmi_errno);
    }

    pmi_errno = PMI_Get_size(&size);

    if (pmi_errno != 0) {
        MPIR_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**pmi_get_size",
                             "**pmi_get_size %d", pmi_errno);
    }

    pmi_errno = PMI_Get_appnum(&appnum);

    if (pmi_errno != PMI_SUCCESS) {
        MPIR_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**pmi_get_appnum",
                             "**pmi_get_appnum %d", pmi_errno);
    }

    pmi_errno = PMI_KVS_Get_name_length_max(&max_pmi_name_length);
    if (pmi_errno != PMI_SUCCESS) {
        MPIR_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**pmi_kvs_get_name_length_max",
                             "**pmi_kvs_get_name_length_max %d", pmi_errno);
    }

    MPIDI_global.jobid = (char *) MPL_malloc(max_pmi_name_length, MPL_MEM_OTHER);
    pmi_errno = PMI_KVS_Get_my_name(MPIDI_global.jobid, max_pmi_name_length);
    if (pmi_errno != PMI_SUCCESS) {
        MPIR_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**pmi_kvs_get_my_name",
                             "**pmi_kvs_get_my_name %d", pmi_errno);
    }
#elif defined(USE_PMI2_API)
    int pmi_errno = PMI2_Init(&has_parent, &size, &rank, &appnum);

    if (pmi_errno != PMI2_SUCCESS) {
        MPIR_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**pmi_init", "**pmi_init %d", pmi_errno);
    }

    MPIDI_global.jobid = (char *) MPL_malloc(MPIDI_MAX_JOBID_LEN, MPL_MEM_OTHER);
    pmi_errno = PMI2_Job_GetId(MPIDI_global.jobid, MPIDI_MAX_JOBID_LEN);
    if (pmi_errno != PMI2_SUCCESS) {
        MPIR_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**pmi_job_getid",
                             "**pmi_job_getid %d", pmi_errno);
    }
#elif defined(USE_PMIX_API)
    pmix_value_t *pvalue = NULL;

    int pmi_errno = PMIx_Init(&MPIR_Process.pmix_proc, NULL, 0);
    if (pmi_errno != PMIX_SUCCESS) {
        MPIR_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**pmix_init", "**pmix_init %d",
                                pmi_errno);
    }
    rank = MPIR_Process.pmix_proc.rank;

    PMIX_PROC_CONSTRUCT(&MPIR_Process.pmix_wcproc);
    MPL_strncpy(MPIR_Process.pmix_wcproc.nspace, MPIR_Process.pmix_proc.nspace, PMIX_MAX_NSLEN);
    MPIR_Process.pmix_wcproc.rank = PMIX_RANK_WILDCARD;

    pmi_errno = PMIx_Get(&MPIR_Process.pmix_wcproc, PMIX_JOB_SIZE, NULL, 0, &pvalue);
    if (pmi_errno != PMIX_SUCCESS) {
        MPIR_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**pmix_get", "**pmix_get %d",
                                pmi_errno);
    }
    size = pvalue->data.uint32;
    PMIX_VALUE_RELEASE(pvalue);

    /* appnum, has_parent is not set for now */
    appnum = 0;
    has_parent = 0;
#endif
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

void pmi_finalize(void)
{
#if defined USE_PMI1_API
    PMI_Finalize();
#elif defined USE_PMI2_API
    PMI2_Finalize();
#elif defined USE_PMIX_API
    PMIx_Finalize(NULL, 0);
#endif
}

/**** init-time PMI utilities *************/

/**** run-time PMI utilities *************/

int pmi_get_universe_size(int *universe_size)
{
    int mpi_errno = MPI_SUCCESS;
    int pmi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_PMI_GET_UNIVERSE_SIZE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_PMI_GET_UNIVERSE_SIZE);

#ifdef USE_PMIX_API
    {
        pmix_value_t *pvalue = NULL;

        pmi_errno = PMIx_Get(&MPIR_Process.pmix_wcproc, PMIX_UNIV_SIZE, NULL, 0, &pvalue);
        if (pmi_errno != PMIX_SUCCESS)
            MPIR_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER,
                                 "**pmix_get", "**pmix_get %d", pmi_errno);
        *universe_size = pvalue->data.uint32;
        PMIX_VALUE_RELEASE(pvalue);
    }
#elif defined(USE_PMI2_API)
    {
        char val[PMI2_MAX_VALLEN];
        int found = 0;
        char *endptr;

        pmi_errno = PMI2_Info_GetJobAttr("universeSize", val, sizeof(val), &found);
        if (pmi_errno)
            MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**pmi_getjobattr");

        if (!found) {
            *universe_size = MPIR_UNIVERSE_SIZE_NOT_AVAILABLE;
        } else {
            *universe_size = strtol(val, &endptr, 0);
            MPIR_ERR_CHKINTERNAL(endptr - val != strlen(val), mpi_errno,
                                 "can't parse universe size");
        }
    }
#else
    pmi_errno = PMI_Get_universe_size(universe_size);

    if (pmi_errno)
        MPIR_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER,
                             "**pmi_get_universe_size", "**pmi_get_universe_size %d", pmi_errno);
#endif

    if (*universe_size < 0)
        *universe_size = MPIR_UNIVERSE_SIZE_NOT_AVAILABLE;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_PMI_GET_UNIVERSE_SIZE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifdef ENABLE_PMIX

/* Assumption: We are using OpenPMIx if all of
 * - PMIX_VERSION_{MAJOR, MINOR, RELEASE}
 * - PMIX_NUMERIC_VERSION
 * are defined. These are not part of the PMIx standard but are
 * provided by OpenPMIx in pmix_version.h. Macros for checks about
 * specific versions only work for OpenPMIx. If we are not using
 * OpenPMIx: Enable features and rely on compiler and PMIx runtime
 * environment to detect unsupported features or APIs. */
#if defined (PMIX_VERSION_MAJOR) && defined (PMIX_VERSION_MINOR) && \
    defined (PMIX_VERSION_RELEASE) && defined (PMIX_NUMERIC_VERSION)
/* PMIx Process Sets are supported since OpenPMIx major version 4 */
#define HAVE_PMIX_MIN_VERSION_4 (PMIX_VERSION_MAJOR >= 4)
#else
#define HAVE_PMIX_MIN_VERSION_4 1
#endif

/* PMIx_Info_load is only in OpenPMIx >= 5.0. Previous versions provide a macro. */
#ifdef NEED_PMIX_INFO_LOAD
#define PMIx_Info_load PMIX_INFO_LOAD
#endif

/* some clients return this deprecated error code from PMIx_Spawn */
#ifndef PMIX_ERR_NOT_IMPLEMENTED
#define PMIX_ERR_NOT_IMPLEMENTED -48
#endif

/* These are const after init. FIXME: We need a way to guard changes outside init. */
static pmix_proc_t pmix_proc;
static pmix_proc_t pmix_wcproc;
static pmix_proc_t pmix_parent;

static void pmix_not_supported(const char *elem, char *error_str, int len);
static int pmix_add_to_info(MPIR_Info * info_ptr, const char *key, const char *pmix_key,
                            MPIR_Info * target_ptr, int *key_found, size_t * counter, char **value);
static int mpi_to_pmix_keyvals(MPIR_Info * info_ptr, int ninfo, pmix_info_t ** pmix_info);
static int pmix_build_job_info(MPIR_Info * info_ptr, pmix_info_t ** pmix_job_info,
                               size_t * njob_info, char **path);
static int pmix_build_app_info(MPIR_Info * info_ptr, pmix_info_t ** pmix_app_info,
                               size_t * napp_info);
static int pmix_build_app_env(MPIR_Info * info_ptr, char ***env);
static int pmix_build_app_cmd(char *path, char *command, char **app_cmd);
static int pmix_prep_spawn(int count, char *commands[], char **argvs[], const int maxprocs[],
                           MPIR_Info * info_ptrs[], pmix_app_t * apps, pmix_info_t ** job_info,
                           size_t * njob_info);
static int pmix_fence_nspace_proc(const char *nspace, const char *parent_nspace, int parent_rank);

/* Array of PMIx process IDs for implicitly defining their unique MPIR_Process.rank
 * via their position in the array (also across multiple namespaces) */
static pmix_data_array_t *pmix_procs = NULL;
static void pmix_id_to_mpi_rank(pmix_proc_t proc, int *rank, int *size);
static void mpi_rank_to_pmix_id(int rank, pmix_proc_t * proc);

static int pmix_pset_define_hdlr;
static int pmix_pset_delete_hdlr;
static int pmix_register_event_handlers(void);

static int pmix_init(int *has_parent, int *rank, int *size, int *appnum)
{
    int mpi_errno = MPI_SUCCESS;
    int pmi_errno;

    pmi_max_key_size = PMIX_MAX_KEYLEN;
    pmi_max_val_size = 1024;    /* this is what PMI2_MAX_VALLEN currently set to */

    pmix_value_t *pvalue = NULL;

    /* Since we only call PMIx_Finalize once at `atexit` handler, we need prevent
     * calling PMIx_Init multiple times. */
    static int pmix_init_count = 0;
    pmix_init_count++;
    if (pmix_init_count == 1) {
        pmi_errno = PMIx_Init(&pmix_proc, NULL, 0);
        if (pmi_errno == PMIX_ERR_UNREACH) {
            /* no pmi server, assume we are a singleton */
            goto singleton_out;
        }
        MPIR_ERR_CHKANDJUMP1(pmi_errno != PMIX_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                             "**pmix_init", "**pmix_init %d", pmi_errno);

        PMIX_PROC_CONSTRUCT(&pmix_wcproc);
        MPL_strncpy(pmix_wcproc.nspace, pmix_proc.nspace, PMIX_MAX_NSLEN);
        pmix_wcproc.rank = PMIX_RANK_WILDCARD;

        pmi_kvs_name = MPL_strdup(pmix_proc.nspace);
    }

    pmi_errno = PMIx_Get(&pmix_wcproc, PMIX_JOB_SIZE, NULL, 0, &pvalue);
    MPIR_ERR_CHKANDJUMP1(pmi_errno != PMIX_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                         "**pmix_get", "**pmix_get %d", pmi_errno);
    *size = pvalue->data.uint32;
    PMIX_VALUE_RELEASE(pvalue);

    /* Fill pmix_procs array with all processes
     * Currently this is trivial for only one namespace, but once multiple
     * namespaces and terminating processes are considered this will become
     * more complicated and needs additional mechanisms such as PMIx Process
     * Group and PMIx Connect to determine the size and the participating
     * processes. */
    if (pmix_procs != NULL) {
        PMIX_DATA_ARRAY_FREE(pmix_procs);
    }
    PMIX_DATA_ARRAY_CREATE(pmix_procs, *size, PMIX_PROC);
    pmix_proc_t *procs = (pmix_proc_t *) pmix_procs->array;
    for (int i = 0; i < *size; i++) {
        PMIX_LOAD_PROCID(&(procs[i]), pmix_proc.nspace, i);
    }

    /* Set the MPI rank based on the pmix_procs array
     * For now this is trivial, *rank = pmix_proc.rank would do the same */
    pmix_id_to_mpi_rank(pmix_proc, rank, NULL);

    /* sessions: pmi_kvs_name is freed during finalize, so we need to set it
     * again in the reinitialization case */
    if (pmi_kvs_name == NULL) {
        pmi_kvs_name = MPL_strdup(pmix_proc.nspace);
    }

    PMIX_PROC_CONSTRUCT(&pmix_parent);
    pmi_errno = PMIx_Get(&pmix_proc, PMIX_PARENT_ID, NULL, 0, &pvalue);
    if (pmi_errno == PMIX_ERR_NOT_FOUND) {
        *has_parent = 0;        /* process not spawned */
    } else if (pmi_errno == PMIX_SUCCESS) {
        *has_parent = 1;        /* spawned process */
        PMIX_PROC_LOAD(&pmix_parent, pvalue->data.proc->nspace, pvalue->data.proc->rank);
        PMIX_VALUE_RELEASE(pvalue);
    } else {
        MPIR_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**pmix_get", "**pmix_get %s",
                             PMIx_Error_string(pmi_errno));
    }

    /* Get the appnum */
    pmi_errno = PMIx_Get(&pmix_proc, PMIX_APPNUM, NULL, 0, &pvalue);
    if (pmi_errno != PMIX_ERR_NOT_FOUND) {
        MPIR_ERR_CHKANDJUMP1(pmi_errno != PMIX_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                             "**pmix_get", "**pmix_get %s", PMIx_Error_string(pmi_errno));
        MPIR_Assert(pvalue->data.uint32 <= INT_MAX);    /* overflow check */
        *appnum = (int) pvalue->data.uint32;
        PMIX_VALUE_RELEASE(pvalue);
    }

    pmix_value_t value;
    pmix_value_t *val = &value;

    for (int i = 0; i < *size; i++) {
        pmi_errno = PMIx_Get(&(procs[i]), PMIX_FABRIC_COORDINATES, NULL, 0, &val);
        if (pmi_errno != PMIX_SUCCESS) {
            MPIR_Process.coords = NULL;
            break;
        }
        MPIR_Assert(val->data.coord->dims <= INT_MAX);
        MPIR_Process.coords_dims = (int) val->data.coord->dims;
        MPIR_Assert(MPIR_Process.coords_dims == 3);

        if (i == 0) {
            MPIR_Process.coords =
                MPL_malloc(sizeof(int) * MPIR_Process.coords_dims * (*size), MPL_MEM_BUFFER);
            MPIR_ERR_CHKANDJUMP(!MPIR_Process.coords, mpi_errno, MPI_ERR_OTHER, "**nomem");
        }

        if (PMIX_COORD == val->type) {
            for (int j = 0; j < MPIR_Process.coords_dims; j++) {
                /* MPIR_Process.coords stores the coords in this order: port number, switch_id, group_id */
                MPIR_Process.coords[i * MPIR_Process.coords_dims + j] =
                    val->data.coord->coord[MPIR_Process.coords_dims - 1 - j];
            }
        }
    }

    pmix_register_event_handlers();

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
  singleton_out:
    *rank = 0;
    *size = 1;
    *has_parent = 0;
    *appnum = 0;
    pmi_kvs_name = MPL_strdup("0");
    goto fn_exit;
}

static void pmix_exit(void)
{
    if (pmix_procs != NULL) {
        PMIX_DATA_ARRAY_FREE(pmix_procs);
    }
    PMIx_Finalize(NULL, 0);
}

static void pmix_abort(int exit_code, const char *error_msg)
{
    PMIx_Abort(exit_code, error_msg, NULL, 0);
}

static int pmix_put(const char *key, const char *val)
{
    int mpi_errno = MPI_SUCCESS;
    int pmi_errno;

    pmix_value_t value;
    value.type = PMIX_STRING;
    value.data.string = (char *) val;
    pmi_errno = PMIx_Put(PMIX_GLOBAL, key, &value);
    MPIR_ERR_CHKANDJUMP1(pmi_errno != PMIX_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                         "**pmix_put", "**pmix_put %d", pmi_errno);
    pmi_errno = PMIx_Commit();
    MPIR_ERR_CHKANDJUMP1(pmi_errno != PMIX_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                         "**pmix_commit", "**pmix_commit %d", pmi_errno);
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int pmix_get(int src, const char *key, char *val, int val_size)
{
    int mpi_errno = MPI_SUCCESS;
    int pmi_errno;

    pmix_value_t *pvalue;
    if (src < 0) {
        /* TODO: We must not use pmix_wcproc here once we have cases where there can be
         * multiple namespaces in pmix_procs */
        pmi_errno = PMIx_Get(&pmix_wcproc, key, NULL, 0, &pvalue);
    } else {
        pmix_proc_t proc;
        PMIX_PROC_CONSTRUCT(&proc);
        mpi_rank_to_pmix_id(src, &proc);

        pmi_errno = PMIx_Get(&proc, key, NULL, 0, &pvalue);
    }
    MPIR_ERR_CHKANDJUMP1(pmi_errno != PMIX_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                         "**pmix_get", "**pmix_get %d", pmi_errno);
    MPL_strncpy(val, pvalue->data.string, val_size);
    PMIX_VALUE_RELEASE(pvalue);
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int pmix_get_parent(const char *key, char *val, int val_size)
{
    int mpi_errno = MPI_SUCCESS;
    int pmi_errno = PMIX_SUCCESS;
    pmix_value_t *pvalue;
    pmi_errno = PMIx_Get(&pmix_parent, key, NULL, 0, &pvalue);
    MPIR_ERR_CHKANDJUMP1(pmi_errno != PMIX_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**pmix_get",
                         "**pmix_get %s", PMIx_Error_string(pmi_errno));
    MPL_strncpy(val, pvalue->data.string, val_size);
    PMIX_VALUE_RELEASE(pvalue);

    /* Fence with all other spawned procs in namespace and parent proc to
     * sync with MPIR_pmi_spawn_multiple in parent process */
    mpi_errno = pmix_fence_nspace_proc(pmix_proc.nspace, pmix_parent.nspace, pmix_parent.rank);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static bool pmix_get_jobattr(const char *key, char *valbuf)
{
    bool found = false;
    pmix_value_t *pvalue;

    /* translate MPICH key to PMIx standard format */
    if (strcmp(key, "PMI_process_mapping") == 0) {
        key = PMIX_ANL_MAP;
    }

    /* if this is a non-reserved key, we want to make sure not to block
     * by using PMIX_IMMEDIATE */
    pmix_info_t *info;
    PMIX_INFO_CREATE(info, 1);
    int flag = 1;
    PMIx_Info_load(info, PMIX_IMMEDIATE, &flag, PMIX_BOOL);

    /* TODO: check if pmix_wcproc is sufficient here if there are multiple
     * namespaces in pmix_procs */
    int pmi_errno = PMIx_Get(&pmix_wcproc, key, info, 1, &pvalue);
    if (pmi_errno == PMIX_SUCCESS) {
        strncpy(valbuf, pvalue->data.string, pmi_max_val_size);
        PMIX_VALUE_RELEASE(pvalue);
        found = true;
    }
    PMIX_INFO_FREE(info, 1);

    return found;
}

static int pmix_barrier(void)
{
    int mpi_errno = MPI_SUCCESS;
    int pmi_errno;

    pmix_info_t *info;
    PMIX_INFO_CREATE(info, 1);
    int flag = 1;
    PMIx_Info_load(info, PMIX_COLLECT_DATA, &flag, PMIX_BOOL);

    pmi_errno = PMIx_Fence((pmix_proc_t *) pmix_procs->array, pmix_procs->size, info, 1);
    MPIR_ERR_CHKANDJUMP1(pmi_errno != PMIX_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                         "**pmix_fence", "**pmix_fence %d", pmi_errno);
    PMIX_INFO_FREE(info, 1);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int pmix_barrier_only(void)
{
    int mpi_errno = MPI_SUCCESS;
    int pmi_errno;

    pmi_errno = PMIx_Fence((pmix_proc_t *) pmix_procs->array, pmix_procs->size, NULL, 0);
    MPIR_ERR_CHKANDJUMP1(pmi_errno != PMIX_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                         "**pmix_fence", "**pmix_fence %d", pmi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int pmix_barrier_local(void)
{
    int mpi_errno = MPI_SUCCESS;
    int pmi_errno;

    int local_size = MPIR_Process.local_size;
    pmix_proc_t *procs = NULL;
    PMIX_PROC_CREATE(procs, local_size);
    for (int i = 0; i < local_size; i++) {
        mpi_rank_to_pmix_id(MPIR_Process.node_local_map[i], &(procs[i]));
    }

    pmix_info_t *info;
    int flag = 1;
    PMIX_INFO_CREATE(info, 1);
    PMIx_Info_load(info, PMIX_COLLECT_DATA, &flag, PMIX_BOOL);

    pmi_errno = PMIx_Fence(procs, local_size, info, 1);
    MPIR_ERR_CHKANDJUMP1(pmi_errno != PMIX_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**pmix_fence",
                         "**pmix_fence %d", pmi_errno);

    PMIX_INFO_FREE(info, 1);
    PMIX_PROC_FREE(procs, local_size);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int pmix_optimized_put(const char *key, const char *val, int is_local)
{
    int mpi_errno = MPI_SUCCESS;

    int pmi_errno;
    pmix_value_t value;
    value.type = PMIX_STRING;
    value.data.string = (char *) val;
    pmi_errno = PMIx_Put(is_local ? PMIX_LOCAL : PMIX_GLOBAL, key, &value);
    MPIR_ERR_CHKANDJUMP1(pmi_errno != PMIX_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                         "**pmix_put", "**pmix_put %d", pmi_errno);
    pmi_errno = PMIx_Commit();
    MPIR_ERR_CHKANDJUMP1(pmi_errno != PMIX_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                         "**pmix_commit", "**pmix_commit %d", pmi_errno);
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int pmix_optimized_get(int src, const char *key, char *val, int valsize, int is_local)
{
    return pmix_get(src, key, val, valsize);
}

static int pmix_put_binary(const char *key, const char *buf, int bufsize, int is_local)
{
    int mpi_errno = MPI_SUCCESS;
    int pmi_errno;

    pmix_value_t value;
    value.type = PMIX_BYTE_OBJECT;
    value.data.bo.bytes = (char *) buf;
    value.data.bo.size = bufsize;
    pmi_errno = PMIx_Put(is_local ? PMIX_LOCAL : PMIX_GLOBAL, key, &value);
    MPIR_ERR_CHKANDJUMP1(pmi_errno != PMIX_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                         "**pmix_put", "**pmix_put %d", pmi_errno);
    pmi_errno = PMIx_Commit();
    MPIR_ERR_CHKANDJUMP1(pmi_errno != PMIX_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                         "**pmix_commit", "**pmix_commit %d", pmi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int pmix_get_binary(int src, const char *key, char *buf, int *p_size, int is_local)
{
    int mpi_errno = MPI_SUCCESS;
    int pmi_errno;

    int bufsize ATTRIBUTE((unused)) = *p_size;
    pmix_value_t *pvalue;
    if (src < 0) {
        /* TODO: We must not use pmix_wcproc here once we have cases where there can be
         * multiple namespaces in pmix_procs */
        pmi_errno = PMIx_Get(&pmix_wcproc, key, NULL, 0, &pvalue);
    } else {
        pmix_proc_t proc;
        PMIX_PROC_CONSTRUCT(&proc);
        mpi_rank_to_pmix_id(src, &proc);

        pmi_errno = PMIx_Get(&proc, key, NULL, 0, &pvalue);
    }
    MPIR_ERR_CHKANDJUMP1(pmi_errno != PMIX_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                         "**pmix_get", "**pmix_get %d", pmi_errno);
    MPIR_Assert(pvalue->type == PMIX_BYTE_OBJECT);
    MPIR_Assert(pvalue->data.bo.size <= bufsize);

    memcpy(buf, pvalue->data.bo.bytes, pvalue->data.bo.size);
    *p_size = pvalue->data.bo.size;

    PMIX_VALUE_RELEASE(pvalue);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int pmix_optional_bcast_barrier(MPIR_PMI_DOMAIN domain)
{
    if (domain == MPIR_PMI_DOMAIN_LOCAL) {
        return pmix_barrier_local();
    } else {
        return pmix_barrier();
    }
}

static int pmix_get_universe_size(int *universe_size)
{
    int mpi_errno = MPI_SUCCESS;
    int pmi_errno;
    pmix_value_t *pvalue = NULL;

    /* TODO: Double-check if using pmix_wcproc here yields correct results for
     * the PMIX_UNIV_SIZE if there are multiple namespaces in pmix_procs.
     * Is this a case that must be supported here? */
    pmi_errno = PMIx_Get(&pmix_wcproc, PMIX_UNIV_SIZE, NULL, 0, &pvalue);
    if (pmi_errno == PMIX_ERR_NOT_FOUND) {
        *universe_size = MPIR_UNIVERSE_SIZE_NOT_AVAILABLE;
        goto fn_exit;
    }
    MPIR_ERR_CHKANDJUMP1(pmi_errno != PMIX_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                         "**pmix_get", "**pmix_get %d", pmi_errno);
    *universe_size = pvalue->data.uint32;
    PMIX_VALUE_RELEASE(pvalue);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int pmix_spawn(int count, char *commands[], char **argvs[], const int maxprocs[],
                      MPIR_Info * info_ptrs[], int num_preput_keyval,
                      struct MPIR_PMI_KEYVAL *preput_keyvals, int *pmi_errcodes)
{
    int mpi_errno = MPI_SUCCESS;
    int pmi_errno;
    pmix_app_t *apps = NULL;
    pmix_info_t *job_info = NULL;
    size_t njob_info = 0;
    char new_nspace[PMIX_MAX_NSLEN + 1];

    /* Create the PMIx apps structure */
    PMIX_APP_CREATE(apps, count);
    MPIR_ERR_CHKANDJUMP(!apps, mpi_errno, MPI_ERR_OTHER, "**nomem");

    mpi_errno = pmix_prep_spawn(count, commands, argvs, maxprocs, info_ptrs,
                                apps, &job_info, &njob_info);
    MPIR_ERR_CHECK(mpi_errno);

    /* PMIx_Put to the KVS what is required by the spawned processes */
    if (num_preput_keyval > 0) {
        for (int j = 0; j < num_preput_keyval; j++) {
            mpi_errno = MPIR_pmi_kvs_put(preput_keyvals[j].key, preput_keyvals[j].val);
            MPIR_ERR_CHECK(mpi_errno);
        }
    }

    pmi_errno = PMIx_Spawn(job_info, njob_info, apps, count, new_nspace);

    /* Set the pmi error code for all apps of the job to that of PMIx_Spawn */
    if (pmi_errcodes) {
        for (int i = 0; i < count; i++) {
            pmi_errcodes[i] = pmi_errno;
        }
    }

    if (pmi_errno == PMIX_ERR_NOT_SUPPORTED || pmi_errno == PMIX_ERR_NOT_IMPLEMENTED) {
        char error_str[MPI_MAX_ERROR_STRING];
        pmix_not_supported("PMIx_Spawn", error_str, MPI_MAX_ERROR_STRING);
        MPIR_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_SPAWN, "**pmix_spawn", "**pmix_spawn %s",
                             error_str);
    }
    MPIR_ERR_CHKANDJUMP1(pmi_errno != PMIX_SUCCESS, mpi_errno, MPI_ERR_SPAWN,
                         "**pmix_spawn", "**pmix_spawn %s", PMIx_Error_string(pmi_errno));

    /* Fence with spawned procs in new nspace (counter part in MPIR_pmi_kvs_parent_get)
     * Preput data in KVS must be read by child procs before is overwritten in next Spawn */
    mpi_errno = pmix_fence_nspace_proc(new_nspace, pmix_proc.nspace, pmix_proc.rank);
    MPIR_ERR_CHECK(mpi_errno);
  fn_exit:
    /* Free job info */
    PMIX_INFO_FREE(job_info, njob_info);

    /* Free app array */
    if (apps) {
        for (int i = 0; i < count; i++) {
            /* We have to free via MPL because allocation happened via MPL */
            if (apps[i].cmd) {
                MPL_free(apps[i].cmd);
                apps[i].cmd = NULL;
            }
        }
        PMIX_APP_FREE(apps, count);
    }

    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int pmix_publish(const char name[], const char port[])
{
    int mpi_errno = MPI_SUCCESS;
    int pmi_errno;
    pmix_info_t *info;
    PMIX_INFO_CREATE(info, 1);
    MPL_strncpy(info[0].key, name, PMIX_MAX_KEYLEN);
    info[0].value.type = PMIX_STRING;
    info[0].value.data.string = MPL_direct_strdup(port);
    pmi_errno = PMIx_Publish(info, 1);
    PMIX_INFO_FREE(info, 1);
    MPIR_ERR_CHKANDJUMP1(pmi_errno, mpi_errno, MPI_ERR_NAME, "**namepubnotpub",
                         "**namepubnotpub %s", name);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int pmix_lookup(const char name[], char port[], int port_len)
{
    int mpi_errno = MPI_SUCCESS;
    int pmi_errno;
    pmix_pdata_t *pdata;
    PMIX_PDATA_CREATE(pdata, 1);
    MPL_strncpy(pdata[0].key, name, PMIX_MAX_KEYLEN);
    pmi_errno = PMIx_Lookup(pdata, 1, NULL, 0);
    if (pmi_errno == PMIX_SUCCESS) {
        MPL_strncpy(port, pdata[0].value.data.string, port_len);
    }
    PMIX_PDATA_FREE(pdata, 1);
    MPIR_ERR_CHKANDJUMP1(pmi_errno, mpi_errno, MPI_ERR_NAME, "**namepubnotfound",
                         "**namepubnotfound %s", name);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int pmix_unpublish(const char name[])
{
    int mpi_errno = MPI_SUCCESS;
    int pmi_errno = PMIX_SUCCESS;
    pmix_pdata_t *pdata;
    char *keys[2] = { (char *) name, NULL };

    /* Lookup the key that shall be unpublished to make sure it exists */
    PMIX_PDATA_CREATE(pdata, 1);
    MPL_strncpy(pdata[0].key, name, PMIX_MAX_KEYLEN);
    pmi_errno = PMIx_Lookup(pdata, 1, NULL, 0);
    PMIX_PDATA_FREE(pdata, 1);
    MPIR_ERR_CHKANDJUMP1(pmi_errno, mpi_errno, MPI_ERR_SERVICE, "**namepubnotunpub",
                         "**namepubnotunpub %s", name);

    pmi_errno = PMIx_Unpublish(keys, NULL, 0);
    MPIR_ERR_CHKANDJUMP1(pmi_errno, mpi_errno, MPI_ERR_SERVICE, "**namepubnotunpub",
                         "**namepubnotunpub %s", name);
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static
void pmix_not_supported(const char *elem, char *error_str, int len)
{
    int pmi_errno = PMIX_SUCCESS;
    pmix_value_t *rm_name = NULL;
    pmix_value_t *rm_version = NULL;
    char *name = NULL;
    char *version = NULL;

    /* Try to get infos about PMIx Host (name and version) */
    pmi_errno = PMIx_Get(&pmix_wcproc, PMIX_RM_NAME, NULL, 0, &rm_name);
    if (pmi_errno == PMIX_SUCCESS) {
        name = MPL_strdup(rm_name->data.string);
        PMIX_VALUE_RELEASE(rm_name);
    }

    pmi_errno = PMIx_Get(&pmix_wcproc, PMIX_RM_VERSION, NULL, 0, &rm_version);
    if (pmi_errno == PMIX_SUCCESS) {
        version = MPL_strdup(rm_version->data.string);
        PMIX_VALUE_RELEASE(rm_version);
    }

    /* Create a comprehensible error message based on the infos that
     * could be obtained about the PMIx Host */
    if (name && version) {
        snprintf(error_str, len, "%s not supported by PMIx Host %s version %s",
                 elem, name, version);
    } else if (name) {
        snprintf(error_str, len, "%s not supported by PMIx Host %s", elem, name);
    } else {
        snprintf(error_str, len, "%s not supported by PMIx Host", elem);
    }

    if (name) {
        MPL_free(name);
    }
    if (version) {
        MPL_free(version);
    }
}

/* Add a specific key/value pair from an MPIR_Info object to a target MPIR_info object */
static
int pmix_add_to_info(MPIR_Info * info_ptr, const char *key, const char *pmix_key,
                     MPIR_Info * target_ptr, int *key_found, size_t * counter, char **value)
{
    int mpi_errno = MPI_SUCCESS;
    int flag;
    char val[MPI_MAX_INFO_VAL];

    mpi_errno = MPIR_Info_get_impl(info_ptr, key, MPI_MAX_INFO_VAL, val, &flag);
    MPIR_ERR_CHECK(mpi_errno);

    if (flag) {
        /* Add pmix_key/ value pair to target info */
        mpi_errno = MPIR_Info_set_impl(target_ptr, pmix_key, val);
        MPIR_ERR_CHECK(mpi_errno);
        if (key_found) {
            *key_found = 1;
        }
        if (value) {
            *value = MPL_malloc(MPI_MAX_INFO_VAL, MPL_MEM_OTHER);
            MPL_strncpy(*value, val, MPI_MAX_INFO_VAL);
        }
        (*counter)++;
    } else {
        if (key_found) {
            *key_found = 0;
        }
        if (value) {
            *value = NULL;
        }
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static
int mpi_to_pmix_keyvals(MPIR_Info * info_ptr, int ninfo, pmix_info_t ** pmix_info)
{
    int mpi_errno = MPI_SUCCESS;
    if (ninfo > 0) {
        PMIX_INFO_CREATE(*pmix_info, ninfo);
        MPIR_ERR_CHKANDJUMP(!(*pmix_info), mpi_errno, MPI_ERR_OTHER, "**nomem");
        for (int k = 0; k < ninfo; k++) {
            char key[MPI_MAX_INFO_KEY];
            char val[MPI_MAX_INFO_VAL];
            int flag;
            mpi_errno = MPIR_Info_get_nthkey_impl(info_ptr, k, key);
            MPIR_ERR_CHECK(mpi_errno);
            mpi_errno = MPIR_Info_get_impl(info_ptr, key, MPI_MAX_INFO_VAL, val, &flag);
            MPIR_ERR_CHECK(mpi_errno);
            PMIx_Info_load(&((*pmix_info)[k]), key, val, PMIX_STRING);
        }
    }
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static
int pmix_build_job_info(MPIR_Info * info_ptr, pmix_info_t ** pmix_job_info, size_t * njob_info,
                        char **path)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Info *mpi_job_info;
    size_t ninfo = 0;

    if (info_ptr == NULL) {
        goto fn_exit;
    }

    mpi_errno = MPIR_Info_alloc(&mpi_job_info);
    MPIR_ERR_CHECK(mpi_errno);

    /* path - standard key */
    mpi_errno = pmix_add_to_info(info_ptr, "path", PMIX_PREFIX, mpi_job_info, NULL, &ninfo, path);
    MPIR_ERR_CHECK(mpi_errno);

    /* FIXME: There is currently no mapping of the standard key `soft` to a
     * PMIx key supported by PMIx_Spawn. Once PMIx_Spawn supports soft spawning
     * we should add the key `soft` here. */

    if (ninfo > 0) {
        mpi_errno = mpi_to_pmix_keyvals(mpi_job_info, ninfo, pmix_job_info);
        MPIR_ERR_CHECK(mpi_errno);
        *njob_info = ninfo;
    } else {
        *njob_info = 0;
        *pmix_job_info = NULL;
    }

    mpi_errno = MPIR_Info_free_impl(mpi_job_info);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static
int pmix_build_app_info(MPIR_Info * info_ptr, pmix_info_t ** pmix_app_info, size_t * napp_info)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Info *mpi_app_info = NULL;
    size_t ninfo = 0;
    int have_wdir = 0;

    /* Create the PMIx app info structure */
    mpi_errno = MPIR_Info_alloc(&mpi_app_info);
    MPIR_ERR_CHECK(mpi_errno);

    if (info_ptr != NULL) {
        /* host - standard key */
        mpi_errno = pmix_add_to_info(info_ptr, "host", PMIX_HOST, mpi_app_info, NULL, &ninfo, NULL);
        MPIR_ERR_CHECK(mpi_errno);

        /* wdir - standard key */
        mpi_errno =
            pmix_add_to_info(info_ptr, "wdir", PMIX_WDIR, mpi_app_info, &have_wdir, &ninfo, NULL);
        MPIR_ERR_CHECK(mpi_errno);

        /* FIXME: mpi_initial_errhandler is currently not supported by MPICH.
         * Once the support is available, we should parse the key here and
         * treat it accordingly */

        /* FIXME: There is currently no mapping of the standard keys `arch`
         * and `file` to a PMIx key supported by PMIx_Spawn. Once this changes
         * we should add the keys here */
    }

    /* If no info provided where to look for executable, assume current working dir */
    if (!have_wdir) {
        char cwd[MAXPATHLEN];
        if (NULL == getcwd(cwd, MAXPATHLEN)) {
            MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**other");
        }
        mpi_errno = MPIR_Info_set_impl(mpi_app_info, PMIX_WDIR, cwd);
        MPIR_ERR_CHECK(mpi_errno);
        ninfo++;
    }

    if (ninfo > 0) {
        mpi_errno = mpi_to_pmix_keyvals(mpi_app_info, ninfo, pmix_app_info);
        *napp_info = ninfo;
    } else {
        *napp_info = 0;
        *pmix_app_info = NULL;
    }

    mpi_errno = MPIR_Info_free_impl(mpi_app_info);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static
int pmix_build_app_env(MPIR_Info * info_ptr, char ***env)
{
    int mpi_errno = MPI_SUCCESS;
    int key_found = 0;
    char vals[MPI_MAX_INFO_VAL];

    if (info_ptr == NULL) {
        goto fn_exit;
    }

    /* env - (non-standard key)
     * A list of environment variable settings split by `\n` (newline) to be provided
     * to the newly spawned processes, for example:
     * ENVVAR1=3
     * ENVVAR2=myvalue */
    mpi_errno = MPIR_Info_get_impl(info_ptr, "env", MPI_MAX_INFO_VAL, vals, &key_found);
    MPIR_ERR_CHECK(mpi_errno);

    /* copy environment variables */
    if (key_found) {
        char **env_vals = NULL;
        PMIX_ARGV_SPLIT(env_vals, vals, '\n');
        PMIX_ARGV_COPY(*env, env_vals);
        PMIX_ARGV_FREE(env_vals);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static
int pmix_build_app_cmd(char *path, char *command, char **app_cmd)
{
    /* Either: User specified a path where to find the executable,
     * we have to include this path in addition to command.
     *
     * Or: No path specified by user, use command */
    if (path) {
        int cmdlen = strlen(path) + strlen(command) + 1 + 1;    /* +1 for '/' and +1 for null terminator */
        *app_cmd = MPL_malloc(cmdlen, MPL_MEM_OTHER);
        snprintf(*app_cmd, cmdlen, "%s/%s", path, command);
    } else {
        *app_cmd = MPL_strdup(command);
    }
    return MPI_SUCCESS;
}

static
int pmix_prep_spawn(int count, char *commands[], char **argvs[], const int maxprocs[],
                    MPIR_Info * info_ptrs[], pmix_app_t * apps, pmix_info_t ** job_info,
                    size_t * njob_info)
{
    int mpi_errno = MPI_SUCCESS;
    char *path = NULL;

    for (int i = 0; i < count; i++) {
        apps[i].cmd = NULL;

        /* Save maxprocs */
        apps[i].maxprocs = maxprocs[i];

        /* Copy the argv */
        if ((argvs != MPI_ARGVS_NULL) && (argvs[i] != MPI_ARGV_NULL) && (*argvs[i] != NULL)) {
            PMIX_ARGV_COPY(apps[i].argv, argvs[i]);
        }

        if ((info_ptrs != NULL) && (info_ptrs[i] != NULL)) {
            /* Build the job info based on the info provided to the first app */
            if (i == 0) {
                mpi_errno = pmix_build_job_info(info_ptrs[i], job_info, njob_info, &path);
                MPIR_ERR_CHECK(mpi_errno);
            }

            /* Build app info */
            mpi_errno = pmix_build_app_info(info_ptrs[i], &(apps[i].info), &(apps[i].ninfo));
            MPIR_ERR_CHECK(mpi_errno);

            /* Build app env */
            mpi_errno = pmix_build_app_env(info_ptrs[i], &(apps[i].env));
            MPIR_ERR_CHECK(mpi_errno);
        }

        /* Build app cmd */
        mpi_errno = pmix_build_app_cmd(path, commands[i], &(apps[i].cmd));
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    if (path) {
        MPL_free(path);
    }
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static
int pmix_fence_nspace_proc(const char *nspace, const char *parent_nspace, int parent_rank)
{
    int mpi_errno = MPI_SUCCESS;
    int pmi_errno = PMIX_SUCCESS;
    int fence_collect_data = 1;
    size_t nprocs = 0;
    pmix_proc_t *procs = NULL;
    pmix_info_t *fence_info = NULL;
    pmix_value_t *pvalue = NULL;
    pmix_proc_t nspace_proc;

    if (nspace == NULL) {
        mpi_errno = MPI_ERR_OTHER;
        goto fn_fail;
    }

    /* Determine size of nspace */
    PMIX_LOAD_PROCID(&nspace_proc, nspace, PMIX_RANK_WILDCARD);
    pmi_errno = PMIx_Get(&nspace_proc, PMIX_JOB_SIZE, NULL, 0, &pvalue);
    MPIR_ERR_CHKANDJUMP1(pmi_errno != PMIX_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                         "**pmix_get", "**pmix_get %d", pmi_errno);
    nprocs = pvalue->data.uint32;
    PMIX_VALUE_RELEASE(pvalue);

    /* Create procs array for fence of procs in nspace and parent */
    PMIX_PROC_CREATE(procs, nprocs + 1);        /* +1 for parent */
    for (size_t i = 0; i < nprocs; i++) {
        PMIX_LOAD_PROCID(&(procs[i]), nspace, i);
    }
    PMIX_PROC_LOAD(&(procs[nprocs]), parent_nspace, parent_rank);

    /* Fence */
    PMIX_INFO_CREATE(fence_info, 1);
    PMIx_Info_load(fence_info, PMIX_COLLECT_DATA, &fence_collect_data, PMIX_BOOL);
    pmi_errno = PMIx_Fence(procs, nprocs + 1, fence_info, 1);
    MPIR_ERR_CHKANDJUMP1(pmi_errno != PMIX_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                         "**pmix_fence", "**pmix_fence %d", pmi_errno);

  fn_exit:
    if (fence_info) {
        PMIX_INFO_FREE(fence_info, 1);
    }
    if (procs) {
        PMIX_PROC_FREE(procs, nprocs + 1);
    }
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/**
 * @brief Callback for PMIx process set define event handler
 *        (for function prototype see PMIx 4 standard)
 */
static
void pmix_pset_define_callback(size_t refid, pmix_status_t status, const pmix_proc_t * source,
                               pmix_info_t * info, size_t ninfo, pmix_info_t * results,
                               size_t nresults, pmix_event_notification_cbfunc_fn_t cbfunc,
                               void *cbdata)
{
    pmix_status_t cb_status = PMIX_EVENT_ACTION_COMPLETE;
#if HAVE_PMIX_MIN_VERSION_4
    char *pset_name = NULL;
    pmix_proc_t *membership = NULL;
    MPIR_Pset_member *members = NULL;
    int size = 0;
    bool name_key_found = false;
    bool membership_key_found = false;
    bool process_in_pset = false;
    MPIR_Pset *new_pset = NULL;

    /* Extract pset name and membership from the info object */
    for (size_t i = 0; i < ninfo; i++) {
        if (PMIX_CHECK_KEY(&(info[i]), PMIX_PSET_NAME)) {
            name_key_found = true;
            pset_name = info[i].value.data.string;
        } else if (PMIX_CHECK_KEY(&(info[i]), PMIX_PSET_MEMBERS)) {
            membership_key_found = true;

            /* Overflow check */
            MPIR_Assert(info[i].value.data.darray->size <= INT_MAX);
            size = (int) info[i].value.data.darray->size;

            /* Create members array for new pset  */
            membership = (pmix_proc_t *) info[i].value.data.darray->array;
            members = MPL_malloc(size * sizeof(MPIR_Pset_member), MPL_MEM_OTHER);
            if (members == NULL) {
                cb_status = PMIX_EVENT_NO_ACTION_TAKEN;
                goto fn_exit;
            }
            for (int k = 0; k < size; k++) {
                pmix_proc_t proc = membership[k];

                /* Check overflow */
                MPIR_Assert(proc.rank <= INT_MAX);

                if (PMIX_CHECK_RANK(proc.rank, pmix_proc.rank) &&
                    PMIX_CHECK_NSPACE(proc.nspace, pmix_proc.nspace)) {
                    process_in_pset = true;
                }

                if (!PMIX_CHECK_NSPACE(proc.nspace, pmix_proc.nspace)) {
                    /* Currently PMIx process sets only work if all MPI processes
                     * are in one PMIx namespace due to a lack of consistent mapping
                     * from PMIx process ID to MPI rank across different namespaces.
                     * See comments in pmix_init.
                     *
                     * For now, ignore the pset if it includes a process from a
                     * different namespace.
                     *
                     * TODO: Remove this case here once multiple namespaces in
                     * pmix_procs are supported */
                    MPL_DBG_MSG_FMT(MPIR_DBG_INIT, TYPICAL, (MPL_DBG_FDEST,
                                                             "PMIx pset define callback: \
                                                             pset '%s' includes process from different namespace: %s. \
                                                             This is not supported yet and the pset is ignored.", pset_name, proc.nspace));
                    goto fn_exit;
                }

                pmix_id_to_mpi_rank(proc, (int *) &(members[k]), NULL);
            }
        }

        if (name_key_found && membership_key_found) {
            break;
        }
    }

    if (!name_key_found || !membership_key_found || pset_name == NULL || members == NULL) {
        cb_status = PMIX_EVENT_NO_ACTION_TAKEN;
        goto fn_exit;
    }

    if (size == 0) {
        /* Prevent adding empty psets (no error) */
        MPL_DBG_MSG_FMT(MPIR_DBG_INIT, TYPICAL, (MPL_DBG_FDEST,
                                                 "PMIx pset define callback: pset '%s' has size 0 and is not added.",
                                                 pset_name));
        goto fn_exit;
    }

    if (!process_in_pset) {
        /* Prevent adding a pset in which the process is not a member (no error) */
        MPL_DBG_MSG_FMT(MPIR_DBG_INIT, TYPICAL, (MPL_DBG_FDEST,
                                                 "PMIx pset define callback: not a member of pset '%s', pset not added.",
                                                 pset_name));
        goto fn_exit;
    }

    /* Sort members array, required for group creation from pset */
    qsort(members, (size_t) size, sizeof(MPIR_Pset_member), MPIR_Pset_member_compare);

    /* Create the new pset and add it to the global pmix pset array */
    new_pset = MPL_malloc(sizeof(MPIR_Pset), MPL_MEM_OTHER);
    new_pset->uri = pset_name;
    new_pset->size = size;
    new_pset->is_valid = true;
    new_pset->members = members;

    int add_status = MPIR_Pset_add(new_pset);
    if (add_status == MPI_ERR_OTHER) {
        cb_status = PMIX_EVENT_NO_ACTION_TAKEN;
        MPL_DBG_MSG_FMT(MPIR_DBG_INIT, TYPICAL, (MPL_DBG_FDEST,
                                                 "PMIx pset define callback: pset '%s' already exists, not added again.",
                                                 pset_name));
        goto fn_exit;
    }

  fn_exit:
    if (members != NULL)
        MPL_free(members);
    if (new_pset != NULL)
        MPL_free(new_pset);
#endif
    /* If a callback function is provided, we have to call it.
     * We cannot treat errors here, but we can give a hint via cbfunc status that something went wrong */
    if (NULL != cbfunc) {
        cbfunc(cb_status, NULL, 0, NULL, NULL, cbdata);
    }

    return;
}

/**
 * @brief Callback for PMIx process set delete event handler
 *        (for function prototype see PMIx 4 standard)
 */
static
void pmix_pset_delete_callback(size_t refid, pmix_status_t status, const pmix_proc_t * source,
                               pmix_info_t * info, size_t ninfo, pmix_info_t * results,
                               size_t nresults, pmix_event_notification_cbfunc_fn_t cbfunc,
                               void *cbdata)
{
    pmix_status_t cb_status = PMIX_EVENT_ACTION_COMPLETE;
#if HAVE_PMIX_MIN_VERSION_4
    char *pset_name = NULL;
    bool key_found = false;

    /* Extract pset name from the info object */
    for (size_t i = 0; i < ninfo; i++) {
        if (PMIX_CHECK_KEY(&(info[i]), PMIX_PSET_NAME)) {
            key_found = true;
            pset_name = info[i].value.data.string;
            break;
        }
    }

    if (!key_found) {
        cb_status = PMIX_EVENT_NO_ACTION_TAKEN;
        goto fn_exit;
    }

    /* Search in known PMIx psets for pset_name and invalidate the pset */
    int ret = MPIR_Pset_invalidate(pset_name);
    if (ret == MPI_ERR_OTHER) {
        /* Pset not found in this process (not an error!) */
        MPL_DBG_MSG_FMT(MPIR_DBG_INIT, TYPICAL, (MPL_DBG_FDEST,
                                                 "PMIx pset delete callback: pset '%s' not found.",
                                                 pset_name));
    }

  fn_exit:
#endif
    if (NULL != cbfunc) {
        /* If a callback function is provided, we have to call it
         * We cannot treat an error here, but we can give a hint via cbfunc status that something went wrong */
        cbfunc(cb_status, NULL, 0, NULL, NULL, cbdata);
    }
}

static
int pmix_register_event_handlers(void)
{
#if HAVE_PMIX_MIN_VERSION_4
    int mpi_errno = MPI_SUCCESS;
    /* Set the PMIX codes of the events for which handlers shall be registered */
    pmix_status_t code_pset_define[1] = { PMIX_PROCESS_SET_DEFINE };
    pmix_status_t code_pset_delete[1] = { PMIX_PROCESS_SET_DELETE };

    /* Give handlers a name so we can identify them more easily */
    pmix_info_t info_pset_define[1];
    pmix_info_t info_pset_delete[1];

    PMIX_INFO_LOAD(&info_pset_define[0], PMIX_EVENT_HDLR_NAME, "Process-Set-Define", PMIX_STRING);
    PMIX_INFO_LOAD(&info_pset_delete[0], PMIX_EVENT_HDLR_NAME, "Process-Set-Delete", PMIX_STRING);

    /* Register event handlers for PMIx process set define and delete events
     * in a blocking way (last two parameters are NULL) and treat errors */
    pmix_pset_define_hdlr =
        (int) PMIx_Register_event_handler(code_pset_define, 1, info_pset_define, 1,
                                          pmix_pset_define_callback, NULL, NULL);
    if (pmix_pset_define_hdlr < 0) {
        MPIR_ERR_CHKANDJUMP1(pmix_pset_define_hdlr != PMIX_SUCCESS, mpi_errno,
                             MPI_ERR_OTHER, "**pmix_register_event_handler",
                             "**pmix_register_event_handler %d", pmix_pset_define_hdlr);
    }
    pmix_pset_delete_hdlr =
        (int) PMIx_Register_event_handler(code_pset_delete, 1, info_pset_delete, 1,
                                          pmix_pset_delete_callback, NULL, NULL);
    if (pmix_pset_delete_hdlr < 0) {
        MPIR_ERR_CHKANDJUMP1(pmix_pset_delete_hdlr != PMIX_SUCCESS, mpi_errno,
                             MPI_ERR_OTHER, "**pmix_register_event_handler",
                             "**pmix_register_event_handler %d", pmix_pset_delete_hdlr);
    }

    /* Release PMIx info objects */
    PMIX_INFO_DESTRUCT(&info_pset_define[0]);
    PMIX_INFO_DESTRUCT(&info_pset_delete[0]);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
#else
    return MPI_SUCCESS;
#endif
}

static
int pmix_deregister_event_handlers(void)
{
#if HAVE_PMIX_MIN_VERSION_4
    int mpi_errno = MPI_SUCCESS;
    /* Deregister PMIx event handler for pset define and delete events
     * in a blocking way (last two parameters are NULL) and treat errors */
    pmix_status_t rc;
    rc = PMIx_Deregister_event_handler((size_t) pmix_pset_define_hdlr, NULL, NULL);
    MPIR_ERR_CHKANDJUMP1(rc != PMIX_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                         "**pmix_deregister_event_handler", "**pmix_deregister_event_handler %d",
                         rc);
    pmix_pset_define_hdlr = 0;

    rc = PMIx_Deregister_event_handler((size_t) pmix_pset_delete_hdlr, NULL, NULL);
    MPIR_ERR_CHKANDJUMP1(rc != PMIX_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                         "**pmix_deregister_event_handler", "**pmix_deregister_event_handler %d",
                         rc);
    pmix_pset_delete_hdlr = 0;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
#else
    return MPI_SUCCESS;
#endif
}

static
void pmix_id_to_mpi_rank(pmix_proc_t proc, int *rank, int *size)
{
    size_t n = pmix_procs->size;
    pmix_proc_t *procs = (pmix_proc_t *) pmix_procs->array;
    size_t i;

    /* Overflow check */
    MPIR_Assert(n <= INT_MAX);
    if (size) {
        *size = (int) n;
    }

    if (!rank) {
        return;
    }

    /* Find the proc in the array */
    for (i = 0; i < n; i++) {
        if (PMIX_CHECK_RANK(procs[i].rank, proc.rank) &&
            PMIX_CHECK_NSPACE(procs[i].nspace, proc.nspace)) {
            /* Overflow check */
            MPIR_Assert(i <= INT_MAX);
            *rank = (int) i;
            break;
        }
    }
}

static
void mpi_rank_to_pmix_id(int rank, pmix_proc_t * proc)
{
    /* Sanity check */
    MPIR_Assert(rank < pmix_procs->size);

    if (proc == NULL) {
        PMIX_PROC_CREATE(proc, 1);
    }

    /* MPI rank is index of proc in the array */
    pmix_proc_t *procs = (pmix_proc_t *) pmix_procs->array;
    PMIX_LOAD_PROCID(proc, procs[rank].nspace, procs[rank].rank);
}


#else /* ENABLE_PMIX */

static int pmix_init(int *has_parent, int *rank, int *size, int *appnum)
{
    return MPI_ERR_INTERN;
}

static void pmix_exit(void)
{
    MPIR_Assert(0);
}

static void pmix_abort(int exit_code, const char *error_msg)
{
    MPIR_Assert(0);
}

static int pmix_put(const char *key, const char *val)
{
    return MPI_ERR_INTERN;
}

static int pmix_get(int src, const char *key, char *val, int val_size)
{
    return MPI_ERR_INTERN;
}

static int pmix_get_parent(const char *key, char *val, int val_size)
{
    return MPI_ERR_INTERN;
}

static bool pmix_get_jobattr(const char *key, char *valbuf)
{
    MPIR_Assert(0);
    return false;
}

static int pmix_barrier(void)
{
    return MPI_ERR_INTERN;
}

static int pmix_barrier_only(void)
{
    return MPI_ERR_INTERN;
}

static int pmix_barrier_local(void)
{
    return MPI_ERR_INTERN;
}

static int pmix_optimized_put(const char *key, const char *val, int is_local)
{
    return MPI_ERR_INTERN;
}

static int pmix_optimized_get(int src, const char *key, char *val, int valsize, int is_local)
{
    return MPI_ERR_INTERN;
}

static int pmix_put_binary(const char *key, const char *buf, int bufsize, int is_local)
{
    return MPI_ERR_INTERN;
}

static int pmix_get_binary(int src, const char *key, char *buf, int *p_size, int is_local)
{
    return MPI_ERR_INTERN;
}

static int pmix_optional_bcast_barrier(MPIR_PMI_DOMAIN domain)
{
    return MPI_ERR_INTERN;
}

static int pmix_get_universe_size(int *universe_size)
{
    return MPI_ERR_INTERN;
}

static int pmix_spawn(int count, char *commands[], char **argvs[], const int maxprocs[],
                      MPIR_Info * info_ptrs[], int num_preput_keyval,
                      struct MPIR_PMI_KEYVAL *preput_keyvals, int *pmi_errcodes)
{
    return MPI_ERR_INTERN;
}

static int pmix_publish(const char name[], const char port[])
{
    return MPI_ERR_INTERN;
}

static int pmix_lookup(const char name[], char port[], int port_len)
{
    return MPI_ERR_INTERN;
}

static int pmix_unpublish(const char name[])
{
    return MPI_ERR_INTERN;
}

static int pmix_deregister_event_handlers(void)
{
    return MPI_ERR_INTERN;
}

#endif /* ENABLE_PMIX */

/* build nodemap using PMIx_Resolve_nodes */
static int pmix_build_nodemap(int *nodemap, int sz)
{
#ifndef ENABLE_PMIX
    return MPI_ERR_INTERN;
#else
    int mpi_errno = MPI_SUCCESS;
    int pmi_errno;
    UT_array *nspaces = NULL;
    UT_array *nodelist = NULL;
    pmix_proc_t *procs = (pmix_proc_t *) pmix_procs->array;

    /* STEP 1: Build list of namespaces
     * This is trivial for cases with only one namespace, i.e. nspaces,
     * will contain exactly one element in this case. */
    utarray_new(nspaces, &ut_str_icd, MPL_MEM_OTHER);
    for (size_t i = 0; i < pmix_procs->size; i++) {
        /* Add non-duplicate nspaces to list */
        bool is_duplicate = false;
        char *nspace = procs[i].nspace;
        for (unsigned u = 0; u < utarray_len(nspaces); u++) {
            char *n = *(char **) utarray_eltptr(nspaces, u);
            if (strncmp(nspace, n, MIN(strlen(nspace), strlen(n))) == 0) {
                is_duplicate = true;
                break;
            }
        }
        if (!is_duplicate) {
            utarray_push_back(nspaces, &nspace, MPL_MEM_OTHER);
        }
    }

    /* STEP 2: Build list of nodes, resolve nodes and processes for all namespaces */
    utarray_new(nodelist, &ut_str_icd, MPL_MEM_OTHER);

    for (unsigned i = 0; i < utarray_len(nspaces); i++) {
        char *nspace = *(char **) utarray_eltptr(nspaces, i);
        char *nodes = NULL, *node = NULL;

        pmi_errno = PMIx_Resolve_nodes(nspace, &nodes);
        MPIR_ERR_CHKANDJUMP1(pmi_errno != PMIX_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                             "**pmix_resolve_nodes", "**pmix_resolve_nodes %d", pmi_errno);
        MPIR_Assert(nodes);

        node = strtok(nodes, ",");
        while (node) {
            /* Add non-duplicate nodes to list */
            bool is_duplicate = false;
            unsigned node_id = 0;
            for (node_id = 0; node_id < utarray_len(nodelist); node_id++) {
                char *n = *(char **) utarray_eltptr(nodelist, node_id);
                if (strncmp(node, n, MIN(strlen(node), strlen(n))) == 0) {
                    is_duplicate = true;
                    break;
                }
            }
            if (!is_duplicate) {
                utarray_push_back(nodelist, &node, MPL_MEM_OTHER);
            }

            /* Resolve processes of this nspace on node */
            pmix_proc_t *node_procs = NULL;
            size_t nnode_procs = 0;
            pmi_errno = PMIx_Resolve_peers(node, nspace, &node_procs, &nnode_procs);
            MPIR_ERR_CHKANDJUMP1(pmi_errno != PMIX_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                                 "**pmix_resolve_peers", "**pmix_resolve_peers %d", pmi_errno);

            if (nnode_procs > 0) {
                /* Iterate over peers to set their node id (outer loop) */
                for (size_t j = 0; j < nnode_procs; j++) {
                    int rank = -1;
                    pmix_id_to_mpi_rank(node_procs[j], &rank, NULL);
                    MPIR_Assert(rank < MPIR_Process.size);
                    /* rank == -1 means that node_procs[j] is not participating in pmix_procs => terminated */
                    if (rank != -1) {
                        /* node_id is determined above where this node is compared to
                         * the content of the nodelist and eventually added to it */
                        nodemap[rank] = node_id;
                    }
                }
                PMIX_PROC_FREE(node_procs, nnode_procs);
            }
            node = strtok(NULL, ",");
        }
        /* PMIx latest adds pmix_free. We should switch to that at some point */
        MPL_external_free(nodes);
    }

  fn_exit:
    if (nodelist) {
        utarray_free(nodelist);
    }
    if (nspaces) {
        utarray_free(nspaces);
    }
    return mpi_errno;
  fn_fail:
    goto fn_exit;
#endif
}

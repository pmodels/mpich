/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifdef ENABLE_PMI2

static int pmi2_init(int *has_parent, int *rank, int *size, int *appnum)
{
    int mpi_errno = MPI_SUCCESS;
    int pmi_errno;

    pmi_max_key_size = PMI2_MAX_KEYLEN;
    pmi_max_val_size = PMI2_MAX_VALLEN;

    pmi_errno = PMI2_Init(has_parent, size, rank, appnum);
    MPIR_ERR_CHKANDJUMP1(pmi_errno != PMI2_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                         "**pmi_init", "**pmi_init %d", pmi_errno);

    pmi_kvs_name = (char *) MPL_malloc(PMI2_MAX_VALLEN, MPL_MEM_OTHER);
    pmi_errno = PMI2_Job_GetId(pmi_kvs_name, PMI2_MAX_VALLEN);
    MPIR_ERR_CHKANDJUMP1(pmi_errno != PMI2_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                         "**pmi_job_getid", "**pmi_job_getid %d", pmi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static void pmi2_exit(void)
{
    PMI2_Finalize();
}

static void pmi2_abort(int exit_code, const char *error_msg)
{
    PMI2_Abort(TRUE, error_msg);
}

static int pmi2_put(const char *key, const char *val)
{
    int mpi_errno = MPI_SUCCESS;
    int pmi_errno;

    pmi_errno = PMI2_KVS_Put(key, val);
    MPIR_ERR_CHKANDJUMP1(pmi_errno != PMI2_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                         "**pmi_kvsput", "**pmi_kvsput %d", pmi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int pmi2_get(int src, const char *key, char *val, int val_size)
{
    int mpi_errno = MPI_SUCCESS;
    int pmi_errno;

    if (src < 0)
        src = PMI2_ID_NULL;
    int out_len;
    pmi_errno = PMI2_KVS_Get(pmi_kvs_name, src, key, val, val_size, &out_len);
    MPIR_ERR_CHKANDJUMP1(pmi_errno != PMI2_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                         "**pmi_kvsget", "**pmi_kvsget %d", pmi_errno);
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static bool pmi2_get_jobattr(const char *key, char *valbuf)
{
    if (strcmp(key, "PMI_dead_processes") == 0) {
        int out_len;
        int pmi_errno = PMI2_KVS_Get(pmi_kvs_name, PMI2_ID_NULL, key, valbuf, pmi_max_val_size, &out_len);
        if (pmi_errno != PMI2_SUCCESS || out_len == 0) {
            return false;
        }
    } else {
        int found;
        int pmi_errno = PMI2_Info_GetJobAttr(key, valbuf, pmi_max_val_size, &found);
        if (pmi_errno != PMI2_SUCCESS || !found) {
            return false;
        }
    }

    return true;
}

static int pmi2_barrier(void)
{
    int mpi_errno = MPI_SUCCESS;
    int pmi_errno;

    pmi_errno = PMI2_KVS_Fence();
    MPIR_ERR_CHKANDJUMP1(pmi_errno != PMI2_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                         "**pmi_kvsfence", "**pmi_kvsfence %d", pmi_errno);
    /* Get a non-existent key, it only returns after every process called fence */
    int out_len;
    PMI2_KVS_Get(pmi_kvs_name, PMI2_ID_NULL, "-NONEXIST-KEY", NULL, 0, &out_len);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int pmi2_barrier_local(void)
{
    return pmi2_barrier();
}

static int pmi2_optimized_put(const char *key, const char *val, int is_local)
{
    int mpi_errno = MPI_SUCCESS;

    if (!is_local) {
        mpi_errno = pmi2_put(key, val);
    } else {
        int pmi_errno = PMI2_Info_PutNodeAttr(key, val);
        MPIR_ERR_CHKANDJUMP(pmi_errno != PMI2_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                            "**pmi_putnodeattr");
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int pmi2_optimized_get(int src, const char *key, char *val, int valsize, int is_local)
{
    if (is_local) {
        int mpi_errno = MPI_SUCCESS;
        int found;
        int pmi_errno = PMI2_Info_GetNodeAttr(key, val, valsize, &found, TRUE);
        if (pmi_errno != PMI2_SUCCESS) {
            MPIR_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**pmi_getnodeattr");
        } else if (!found) {
            MPIR_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**pmi_getnodeattr");
        }
        return mpi_errno;
    } else {
        return pmi2_get(src, key, val, valsize);
    }
}

static int pmi2_optional_bcast_barrier(MPIR_PMI_DOMAIN domain)
{
    if (domain == MPIR_PMI_DOMAIN_LOCAL) {
        /* PMI2 local uses Put/GetNodeAttr, no need for barrier */
        return MPI_SUCCESS;
    } else {
        return pmi2_barrier();
    }
}

static int pmi2_get_universe_size(int *universe_size)
{
    int mpi_errno = MPI_SUCCESS;
    int pmi_errno;
    char val[PMI2_MAX_VALLEN];
    int found = 0;
    char *endptr;

    pmi_errno = PMI2_Info_GetJobAttr("universeSize", val, sizeof(val), &found);
    MPIR_ERR_CHKANDJUMP1(pmi_errno != PMI2_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                         "**pmi_getjobattr", "**pmi_getjobattr %d", pmi_errno);
    if (!found) {
        *universe_size = MPIR_UNIVERSE_SIZE_NOT_AVAILABLE;
    } else {
        *universe_size = strtol(val, &endptr, 0);
        MPIR_ERR_CHKINTERNAL(endptr - val != strlen(val), mpi_errno, "can't parse universe size");
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int pmi2_spawn(int count, char *commands[], char **argvs[], const int maxprocs[], MPIR_Info * info_ptrs[], int num_preput_keyval, struct MPIR_PMI_KEYVAL *preput_keyvals, int *pmi_errcodes)
{
    int mpi_errno = MPI_SUCCESS;
    int pmi_errno;
    INFO_TYPE *preput_vector = NULL;

    int *info_keyval_sizes = NULL;
    INFO_TYPE **info_keyval_vectors = NULL;
    mpi_errno = get_info_kv_vectors(count, info_ptrs, &info_keyval_vectors, &info_keyval_sizes);
    MPIR_ERR_CHECK(mpi_errno);

    if (num_preput_keyval > 0) {
        preput_vector = MPL_malloc(num_preput_keyval * sizeof(INFO_TYPE), MPL_MEM_BUFFER);
        MPIR_ERR_CHKANDJUMP(!preput_vector, mpi_errno, MPI_ERR_OTHER, "**nomem");
        for (int i = 0; i < num_preput_keyval; i++) {
            INFO_TYPE_KEY(preput_vector[i]) = preput_keyvals[i].key;
            INFO_TYPE_VAL(preput_vector[i]) = preput_keyvals[i].val;
        }
    }

    int *argcs = MPL_malloc(count * sizeof(int), MPL_MEM_DYNAMIC);
    MPIR_Assert(argcs);

    /* compute argcs array */
    for (int i = 0; i < count; ++i) {
        argcs[i] = 0;
        if (argvs != NULL && argvs[i] != NULL) {
            while (argvs[i][argcs[i]]) {
                ++argcs[i];
            }
        }
    }

    pmi_errno = PMI2_Job_Spawn(count, (const char **) commands,
                               argcs, (const char ***) argvs, maxprocs,
                               info_keyval_sizes, (const PMI2_keyval_t **) info_keyval_vectors,
                               num_preput_keyval, (const PMI2_keyval_t *) preput_vector,
                               NULL, 0, pmi_errcodes);
    MPL_free(argcs);
    MPIR_ERR_CHKANDJUMP1(pmi_errno != PMI2_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                         "**pmi_spawn_multiple", "**pmi_spawn_multiple %d", pmi_errno);

  fn_exit:
    free_pmi_keyvals(info_keyval_vectors, count, info_keyval_sizes);
    if (num_preput_keyval > 0) {
        MPL_free(preput_vector);
    }
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int pmi2_publish(const char name[], const char port[])
{
    int mpi_errno = MPI_SUCCESS;
    int pmi_errno;
    /* release the global CS for PMI calls */
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    pmi_errno = PMI2_Nameserv_publish(name, NULL, port);
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_ERR_CHKANDJUMP1(pmi_errno, mpi_errno, MPI_ERR_NAME, "**namepubnotpub",
                         "**namepubnotpub %s", name);
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int pmi2_lookup(const char name[], char port[])
{
    int mpi_errno = MPI_SUCCESS;
    int pmi_errno;
    /* release the global CS for PMI calls */
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    pmi_errno = PMI2_Nameserv_lookup(name, NULL, port, MPI_MAX_PORT_NAME);
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_ERR_CHKANDJUMP1(pmi_errno, mpi_errno, MPI_ERR_NAME, "**namepubnotfound",
                         "**namepubnotfound %s", name);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int pmi2_unpublish(const char name[])
{
    int mpi_errno = MPI_SUCCESS;
    int pmi_errno;
    /* release the global CS for PMI calls */
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    pmi_errno = PMI2_Nameserv_unpublish(name, NULL);
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_ERR_CHKANDJUMP1(pmi_errno, mpi_errno, MPI_ERR_SERVICE, "**namepubnotunpub",
                         "**namepubnotunpub %s", name);
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#else  /* ENABLE_PMI2 */

static int pmi2_init(int *has_parent, int *rank, int *size, int *appnum)
{
    return MPI_ERR_INTERN;
}

static void pmi2_exit(void)
{
    MPIR_Assert(0);
}

static void pmi2_abort(int exit_code, const char *error_msg)
{
    MPIR_Assert(0);
}

static int pmi2_put(const char *key, const char *val)
{
    return MPI_ERR_INTERN;
}

static int pmi2_get(int src, const char *key, char *val, int val_size)
{
    return MPI_ERR_INTERN;
}

static bool pmi2_get_jobattr(const char *key, char *valbuf)
{
    MPIR_Assert(0);
    return false;
}

static int pmi2_barrier(void)
{
    return MPI_ERR_INTERN;
}

static int pmi2_barrier_local(void)
{
    return MPI_ERR_INTERN;
}

static int pmi2_optimized_put(const char *key, const char *val, int is_local)
{
    return MPI_ERR_INTERN;
}

static int pmi2_optimized_get(int src, const char *key, char *val, int valsize, int is_local)
{
    return MPI_ERR_INTERN;
}

static int pmi2_optional_bcast_barrier(MPIR_PMI_DOMAIN domain)
{
    return MPI_ERR_INTERN;
}

static int pmi2_get_universe_size(int *universe_size)
{
    return MPI_ERR_INTERN;
}

static int pmi2_spawn(int count, char *commands[], char **argvs[], const int maxprocs[], MPIR_Info * info_ptrs[], int num_preput_keyval, struct MPIR_PMI_KEYVAL *preput_keyvals, int *pmi_errcodes)
{
    return MPI_ERR_INTERN;
}

static int pmi2_publish(const char name[], const char port[])
{
    return MPI_ERR_INTERN;
}

static int pmi2_lookup(const char name[], char port[])
{
    return MPI_ERR_INTERN;
}

static int pmi2_unpublish(const char name[])
{
    return MPI_ERR_INTERN;
}

#endif /* ENABLE_PMI2 */

#ifdef USE_PMI2_SLURM
static int pmi2_spawn_slurm(int count, char *commands[], char **argvs[],
                            const int maxprocs[], MPIR_Info * info_ptrs[],
                            int num_preput_keyval, struct MPIR_PMI_KEYVAL *preput_keyvals,
                            int *pmi_errcodes)
{
    int mpi_errno = MPI_SUCCESS;
    int pmi_errno;

    int *info_keyval_sizes = NULL;
    INFO_TYPE **info_keyval_vectors = NULL;
    mpi_errno = get_info_kv_vectors(count, info_ptrs, &info_keyval_vectors, &info_keyval_sizes);
    MPIR_ERR_CHECK(mpi_errno);

    const INFO_TYPE **preput_vector = NULL;
    INFO_TYPE *preput_vector_array = NULL;

    if (num_preput_keyval > 0) {
        preput_vector = MPL_malloc(num_preput_keyval * sizeof(INFO_TYPE *), MPL_MEM_BUFFER);
        MPIR_ERR_CHKANDJUMP(!preput_vector, mpi_errno, MPI_ERR_OTHER, "**nomem");
        preput_vector_array = MPL_malloc(num_preput_keyval * sizeof(INFO_TYPE), MPL_MEM_BUFFER);
        MPIR_ERR_CHKANDJUMP(!preput_vector_array, mpi_errno, MPI_ERR_OTHER, "**nomem");
        for (int i = 0; i < num_preput_keyval; i++) {
            INFO_TYPE_KEY(preput_vector_array[i]) = (char *) preput_keyvals[i].key;
            INFO_TYPE_VAL(preput_vector_array[i]) = preput_keyvals[i].val;
            preput_vector[i] = &preput_vector_array[i];
        }
    }

    int *argcs = MPL_malloc(count * sizeof(int), MPL_MEM_DYNAMIC);
    MPIR_Assert(argcs);

    /* compute argcs array */
    for (int i = 0; i < count; ++i) {
        argcs[i] = 0;
        if (argvs != NULL && argvs[i] != NULL) {
            while (argvs[i][argcs[i]]) {
                ++argcs[i];
            }
        }
    }

    pmi_errno = PMI2_Job_Spawn(count, (const char **) commands,
                               argcs, (const char ***) argvs, maxprocs,
                               info_keyval_sizes, (const struct MPID_Info **) info_keyval_vectors,
                               num_preput_keyval, (const struct MPID_Info **) preput_vector,
                               NULL, 0, pmi_errcodes);
    MPL_free(argcs);
    MPIR_ERR_CHKANDJUMP1(pmi_errno != PMI2_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                         "**pmi_spawn_multiple", "**pmi_spawn_multiple %d", pmi_errno);

  fn_exit:
    free_pmi_keyvals(info_keyval_vectors, count, info_keyval_sizes);
    if (num_preput_keyval > 0) {
        MPL_free(preput_vector_array);
        MPL_free(preput_vector);
    }
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
#endif

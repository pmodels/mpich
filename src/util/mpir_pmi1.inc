/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifdef ENABLE_PMI1

static int pmi1_init(int *has_parent, int *rank, int *size, int *appnum)
{
    int mpi_errno = MPI_SUCCESS;
    int pmi_errno;

    pmi_errno = PMI_Init(has_parent);
    MPIR_ERR_CHKANDJUMP1(pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                         "**pmi_init", "**pmi_init %d", pmi_errno);
    pmi_errno = PMI_Get_rank(rank);
    MPIR_ERR_CHKANDJUMP1(pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                         "**pmi_get_rank", "**pmi_get_rank %d", pmi_errno);
    pmi_errno = PMI_Get_size(size);
    MPIR_ERR_CHKANDJUMP1(pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                         "**pmi_get_size", "**pmi_get_size %d", pmi_errno);
    pmi_errno = PMI_Get_appnum(appnum);
    MPIR_ERR_CHKANDJUMP1(pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                         "**pmi_get_appnum", "**pmi_get_appnum %d", pmi_errno);

    int pmi_max_kvs_name_length;
    pmi_errno = PMI_KVS_Get_name_length_max(&pmi_max_kvs_name_length);
    MPIR_ERR_CHKANDJUMP1(pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                         "**pmi_kvs_get_name_length_max",
                         "**pmi_kvs_get_name_length_max %d", pmi_errno);
    pmi_kvs_name = (char *) MPL_malloc(pmi_max_kvs_name_length, MPL_MEM_OTHER);
    pmi_errno = PMI_KVS_Get_my_name(pmi_kvs_name, pmi_max_kvs_name_length);
    MPIR_ERR_CHKANDJUMP1(pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                         "**pmi_kvs_get_my_name", "**pmi_kvs_get_my_name %d", pmi_errno);

    pmi_errno = PMI_KVS_Get_key_length_max(&pmi_max_key_size);
    MPIR_ERR_CHKANDJUMP1(pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                         "**pmi_kvs_get_key_length_max",
                         "**pmi_kvs_get_key_length_max %d", pmi_errno);
    pmi_errno = PMI_KVS_Get_value_length_max(&pmi_max_val_size);
    MPIR_ERR_CHKANDJUMP1(pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                         "**pmi_kvs_get_value_length_max",
                         "**pmi_kvs_get_value_length_max %d", pmi_errno);
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static void pmi1_exit(void)
{
    PMI_Finalize();
}

static void pmi1_abort(int exit_code, const char *error_msg)
{
    PMI_Abort(exit_code, error_msg);
}

static int pmi1_put(const char *key, const char *val)
{
    int mpi_errno = MPI_SUCCESS;
    int pmi_errno;

    pmi_errno = PMI_KVS_Put(pmi_kvs_name, key, val);
    MPIR_ERR_CHKANDJUMP1(pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                         "**pmi_kvs_put", "**pmi_kvs_put %d", pmi_errno);
    pmi_errno = PMI_KVS_Commit(pmi_kvs_name);
    MPIR_ERR_CHKANDJUMP1(pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                         "**pmi_kvs_commit", "**pmi_kvs_commit %d", pmi_errno);
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int pmi1_get(int src, const char *key, char *val, int val_size)
{
    int mpi_errno = MPI_SUCCESS;
    int pmi_errno;

    /* src is not used in PMI1 */
    pmi_errno = PMI_KVS_Get(pmi_kvs_name, key, val, val_size);
    MPIR_ERR_CHKANDJUMP1(pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                         "**pmi_kvs_get", "**pmi_kvs_get %d", pmi_errno);
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static bool pmi1_get_jobattr(const char *key, char *valbuf)
{
    int pmi_errno = PMI_KVS_Get(pmi_kvs_name, key, valbuf, pmi_max_val_size);
    if (pmi_errno != PMI_SUCCESS) {
        return false;
    }

    /* we either get "unavailable" or a valid filename */
    if (strcmp(valbuf, "unavailable") == 0) {
        return false;
    }

    return true;
}

static int pmi1_barrier(void)
{
    int mpi_errno = MPI_SUCCESS;
    int pmi_errno;

    pmi_errno = PMI_Barrier();
    MPIR_ERR_CHKANDJUMP1(pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                         "**pmi_barrier", "**pmi_barrier %d", pmi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int pmi1_barrier_local(void)
{
    return pmi1_barrier();
}

static int pmi1_optimized_put(const char *key, const char *val, int is_local)
{
    return pmi1_put(key, val);
}

static int pmi1_optimized_get(int src, const char *key, char *val, int valsize, int is_local)
{
    return pmi1_get(src, key, val, valsize);
}

static int pmi1_optional_bcast_barrier(MPIR_PMI_DOMAIN domain)
{
    return pmi1_barrier();
}

static int pmi1_get_universe_size(int *universe_size)
{
    int mpi_errno = MPI_SUCCESS;
    int pmi_errno;

    pmi_errno = PMI_Get_universe_size(universe_size);
    MPIR_ERR_CHKANDJUMP1(pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                         "**pmi_get_universe_size", "**pmi_get_universe_size %d", pmi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int pmi1_spawn(int count, char *commands[], char **argvs[], const int maxprocs[], MPIR_Info * info_ptrs[], int num_preput_keyval, struct MPIR_PMI_KEYVAL *preput_keyvals, int *pmi_errcodes)
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

    pmi_errno = PMI_Spawn_multiple(count, (const char **) commands, (const char ***) argvs,
                                   maxprocs,
                                   info_keyval_sizes, (const PMI_keyval_t **) info_keyval_vectors,
                                   num_preput_keyval, (const PMI_keyval_t *) preput_vector,
                                   pmi_errcodes);

    MPIR_ERR_CHKANDJUMP1(pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER,
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

static int pmi1_publish(const char name[], const char port[])
{
    int mpi_errno = MPI_SUCCESS;
    int pmi_errno;
    pmi_errno = PMI_Publish_name(name, port);
    MPIR_ERR_CHKANDJUMP1(pmi_errno, mpi_errno, MPI_ERR_NAME, "**namepubnotpub",
                         "**namepubnotpub %s", name);
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int pmi1_lookup(const char name[], char port[])
{
    int mpi_errno = MPI_SUCCESS;
    int pmi_errno;
    pmi_errno = PMI_Lookup_name(name, port);
    MPIR_ERR_CHKANDJUMP1(pmi_errno, mpi_errno, MPI_ERR_NAME, "**namepubnotfound",
                         "**namepubnotfound %s", name);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int pmi1_unpublish(const char name[])
{
    int mpi_errno = MPI_SUCCESS;
    int pmi_errno;
    pmi_errno = PMI_Unpublish_name(name);
    MPIR_ERR_CHKANDJUMP1(pmi_errno, mpi_errno, MPI_ERR_SERVICE, "**namepubnotunpub",
                         "**namepubnotunpub %s", name);
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#else  /* ENABLE_PMI1 */

static int pmi1_init(int *has_parent, int *rank, int *size, int *appnum)
{
    return MPI_ERR_INTERN;
}

static void pmi1_exit(void)
{
    MPIR_Assert(0);
}

static void pmi1_abort(int exit_code, const char *error_msg)
{
    MPIR_Assert(0);
}

static int pmi1_put(const char *key, const char *val)
{
    return MPI_ERR_INTERN;
}

static int pmi1_get(int src, const char *key, char *val, int val_size)
{
    return MPI_ERR_INTERN;
}

static bool pmi1_get_jobattr(const char *key, char *valbuf)
{
    MPIR_Assert(0);
    return false;
}

static int pmi1_barrier(void)
{
    return MPI_ERR_INTERN;
}

static int pmi1_barrier_local(void)
{
    return MPI_ERR_INTERN;
}

static int pmi1_optimized_put(const char *key, const char *val, int is_local)
{
    return MPI_ERR_INTERN;
}

static int pmi1_optimized_get(int src, const char *key, char *val, int valsize, int is_local)
{
    return MPI_ERR_INTERN;
}

static int pmi1_optional_bcast_barrier(MPIR_PMI_DOMAIN domain)
{
    return MPI_ERR_INTERN;
}

static int pmi1_get_universe_size(int *universe_size)
{
    return MPI_ERR_INTERN;
}

static int pmi1_spawn(int count, char *commands[], char **argvs[], const int maxprocs[], MPIR_Info * info_ptrs[], int num_preput_keyval, struct MPIR_PMI_KEYVAL *preput_keyvals, int *pmi_errcodes)
{
    return MPI_ERR_INTERN;
}

static int pmi1_publish(const char name[], const char port[])
{
    return MPI_ERR_INTERN;
}

static int pmi1_lookup(const char name[], char port[])
{
    return MPI_ERR_INTERN;
}

static int pmi1_unpublish(const char name[])
{
    return MPI_ERR_INTERN;
}

#endif /* ENABLE_PMI1 */

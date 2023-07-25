/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifdef ENABLE_PMIX

static pmix_proc_t pmix_proc;
static pmix_proc_t pmix_wcproc;

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
    }

    *rank = pmix_proc.rank;

    pmi_errno = PMIx_Get(&pmix_wcproc, PMIX_JOB_SIZE, NULL, 0, &pvalue);
    MPIR_ERR_CHKANDJUMP1(pmi_errno != PMIX_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                         "**pmix_get", "**pmix_get %d", pmi_errno);
    *size = pvalue->data.uint32;
    PMIX_VALUE_RELEASE(pvalue);

    /* PMIX_JOBID seems to be more supported than PMIX_NSPACE */
    /* TODO: fallback in case the key is not supported */
    pmi_errno = PMIx_Get(&pmix_wcproc, PMIX_JOBID, NULL, 0, &pvalue);
    MPIR_ERR_CHKANDJUMP1(pmi_errno != PMIX_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                         "**pmix_get", "**pmix_get %d", pmi_errno);
    pmi_kvs_name = MPL_strdup(pvalue->data.string);
    PMIX_VALUE_RELEASE(pvalue);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
  singleton_out:
    *rank = 0;
    *size = 1;
    *appnum = 0;
    *has_parent = 0;
    pmi_kvs_name = MPL_strdup("0");
    goto fn_exit;
}

static void pmix_exit(void)
{
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
        pmi_errno = PMIx_Get(NULL, key, NULL, 0, &pvalue);
    } else {
        pmix_proc_t proc;
        PMIX_PROC_CONSTRUCT(&proc);
        proc.rank = src;

        pmi_errno = PMIx_Get(&proc, key, NULL, 0, &pvalue);
    }
    MPIR_ERR_CHKANDJUMP1(pmi_errno != PMIX_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                         "**pmix_get", "**pmix_get %d", pmi_errno);
    strncpy(val, pvalue->data.string, val_size);
    PMIX_VALUE_RELEASE(pvalue);
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static bool pmix_get_jobattr(const char *key, char *valbuf)
{
    pmix_value_t *pvalue;
    int pmi_errno = PMIx_Get(NULL, key, NULL, 0, &pvalue);
    if (pmi_errno != PMIX_SUCCESS) {
        return false;
    }
    strncpy(valbuf, pvalue->data.string, pmi_max_val_size);
    PMIX_VALUE_RELEASE(pvalue);
    return true;
}

static int pmix_barrier(void)
{
    int mpi_errno = MPI_SUCCESS;
    int pmi_errno;

    pmix_info_t *info;
    PMIX_INFO_CREATE(info, 1);
    int flag = 1;
    PMIX_INFO_LOAD(info, PMIX_COLLECT_DATA, &flag, PMIX_BOOL);

    /* use global wildcard proc set */
    pmi_errno = PMIx_Fence(&pmix_wcproc, 1, info, 1);
    MPIR_ERR_CHKANDJUMP1(pmi_errno != PMIX_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                         "**pmix_fence", "**pmix_fence %d", pmi_errno);
    PMIX_INFO_FREE(info, 1);

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
    pmix_proc_t *procs = MPL_malloc(local_size * sizeof(pmix_proc_t), MPL_MEM_OTHER);
    for (int i = 0; i < local_size; i++) {
        PMIX_PROC_CONSTRUCT(&procs[i]);
        strncpy(procs[i].nspace, pmix_proc.nspace, PMIX_MAX_NSLEN);
        procs[i].rank = MPIR_Process.node_local_map[i];
    }

    pmix_info_t *info;
    int flag = 1;
    PMIX_INFO_CREATE(info, 1);
    PMIX_INFO_LOAD(info, PMIX_COLLECT_DATA, &flag, PMIX_BOOL);

    pmi_errno = PMIx_Fence(procs, local_size, info, 1);
    MPIR_ERR_CHKANDJUMP1(pmi_errno != PMIX_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**pmix_fence",
                         "**pmix_fence %d", pmi_errno);

    PMIX_INFO_FREE(info, 1);
    MPL_free(procs);

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

    int bufsize = *p_size;
    pmix_value_t *pvalue;
    if (src < 0) {
        pmi_errno = PMIx_Get(NULL, key, NULL, 0, &pvalue);
    } else {
        pmix_proc_t proc;
        PMIX_PROC_CONSTRUCT(&proc);
        proc.rank = src;

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
        pmix_barrier_local();
    } else {
        return pmix_barrier();
    }
}

static int pmix_get_universe_size(int *universe_size)
{
    int mpi_errno = MPI_SUCCESS;
    int pmi_errno;
    pmix_value_t *pvalue = NULL;

    pmi_errno = PMIx_Get(&pmix_wcproc, PMIX_UNIV_SIZE, NULL, 0, &pvalue);
    MPIR_ERR_CHKANDJUMP1(pmi_errno != PMIX_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                         "**pmix_get", "**pmix_get %d", pmi_errno);
    *universe_size = pvalue->data.uint32;
    PMIX_VALUE_RELEASE(pvalue);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int pmix_spawn(int count, char *commands[], char **argvs[], const int maxprocs[], MPIR_Info * info_ptrs[], int num_preput_keyval, struct MPIR_PMI_KEYVAL *preput_keyvals, int *pmi_errcodes)
{
    /* not supported yet */
    return MPI_ERR_INTERN;
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

static int pmix_lookup(const char name[], char port[])
{
    int mpi_errno = MPI_SUCCESS;
    int pmi_errno;
    pmix_pdata_t *pdata;
    PMIX_PDATA_CREATE(pdata, 1);
    MPL_strncpy(pdata[0].key, name, PMIX_MAX_KEYLEN);
    pmi_errno = PMIx_Lookup(pdata, 1, NULL, 0);
    if (pmi_errno == PMIX_SUCCESS) {
        MPL_strncpy(port, pdata[0].value.data.string, MPI_MAX_PORT_NAME);
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
    char *keys[2] = { (char *) name, NULL };
    PMIx_Unpublish(keys, NULL, 0);
    return MPI_SUCCESS;
}

#else  /* ENABLE_PMIX */

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

static bool pmix_get_jobattr(const char *key, char *valbuf)
{
    MPIR_Assert(0);
    return false;
}

static int pmix_barrier(void)
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

static int pmix_spawn(int count, char *commands[], char **argvs[], const int maxprocs[], MPIR_Info * info_ptrs[], int num_preput_keyval, struct MPIR_PMI_KEYVAL *preput_keyvals, int *pmi_errcodes)
{
    return MPI_ERR_INTERN;
}

static int pmix_publish(const char name[], const char port[])
{
    return MPI_ERR_INTERN;
}

static int pmix_lookup(const char name[], char port[])
{
    return MPI_ERR_INTERN;
}

static int pmix_unpublish(const char name[])
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
    char *nodelist = NULL, *node = NULL;
    pmix_proc_t *procs = NULL;
    size_t nprocs, node_id = 0;

    pmi_errno = PMIx_Resolve_nodes(pmix_proc.nspace, &nodelist);
    MPIR_ERR_CHKANDJUMP1(pmi_errno != PMIX_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                         "**pmix_resolve_nodes", "**pmix_resolve_nodes %d", pmi_errno);
    MPIR_Assert(nodelist);

    node = strtok(nodelist, ",");
    while (node) {
        pmi_errno = PMIx_Resolve_peers(node, pmix_proc.nspace, &procs, &nprocs);
        MPIR_ERR_CHKANDJUMP1(pmi_errno != PMIX_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                             "**pmix_resolve_peers", "**pmix_resolve_peers %d", pmi_errno);
        for (int i = 0; i < nprocs; i++) {
            nodemap[procs[i].rank] = node_id;
        }
        node_id++;
        node = strtok(NULL, ",");
    }
    /* PMIx latest adds pmix_free. We should switch to that at some point */
    MPL_external_free(nodelist);
    PMIX_PROC_FREE(procs, nprocs);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
#endif
}


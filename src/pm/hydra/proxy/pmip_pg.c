/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "pmip.h"
#include "utarray.h"

static UT_array *PMIP_pgs;

static void pg_destructor(void *_elt)
{
    struct pmip_pg *pg = _elt;
    MPL_free(pg->spawner_kvsname);
    MPL_free(pg->kvsname);
    MPL_free(pg->pmi_process_mapping);
    MPL_free(pg->downstreams);
    MPL_free(pg->iface_ip_env_name);
    MPL_free(pg->hostname);
    HYDU_free_exec_list(pg->exec_list);
    HYD_pmcd_free_pmi_kvs_list(pg->kvs);

    HASH_CLEAR(hh, pg->hash_get);
    for (int i = 0; i < pg->num_elems; i++) {
        MPL_free((pg->cache_get + i)->key);
        MPL_free((pg->cache_get + i)->val);
    }
    MPL_free(pg->cache_get);
}

#define FIND_DOWNSTREAM(fd, field) do { \
    struct pmip_downstream *arr = ut_type_array(PMIP_downstreams, struct pmip_downstream *); \
    for (int i = 0; i < utarray_len(PMIP_downstreams); i++) { \
        if (arr[i].field == fd) { \
            return &arr[i]; \
        } \
    } \
    return NULL; \
} while (0)

void PMIP_pg_init(void)
{
    static UT_icd pg_icd = { sizeof(struct pmip_pg), NULL, NULL, pg_destructor };

    utarray_new(PMIP_pgs, &pg_icd, MPL_MEM_OTHER);
}

void PMIP_pg_finalize(void)
{
    utarray_free(PMIP_pgs);
}

struct pmip_pg *PMIP_new_pg(int pgid, int proxy_id)
{
    int idx = utarray_len(PMIP_pgs);

    utarray_extend_back(PMIP_pgs, MPL_MEM_OTHER);
    struct pmip_pg *pg = (void *) utarray_back(PMIP_pgs);

    pg->idx = idx;
    pg->pgid = pgid;
    pg->proxy_id = proxy_id;

    if (HYD_pmcd_pmip.singleton_port > 0 && utarray_len(PMIP_pgs) == 1) {
        assert(pgid == 0 && proxy_id == 0);
        pg->is_singleton = true;
    }

    HYD_pmcd_pmi_allocate_kvs(&pg->kvs);
    /* the rest of the fields have been zero-filled */

    return pg;
}

struct pmip_pg *PMIP_pg_0(void)
{
    if (utarray_len(PMIP_pgs) > 0) {
        return (void *) utarray_front(PMIP_pgs);
    } else {
        return NULL;
    }
}

struct pmip_pg *PMIP_pg_from_downstream(struct pmip_downstream *downstream)
{
    return (void *) utarray_eltptr(PMIP_pgs, downstream->pg_idx);
}

/* iterate each pg */
HYD_status PMIP_foreach_pg_do(HYD_status(*callback) (struct pmip_pg * pg))
{
    HYD_status status = HYD_SUCCESS;

    int n = utarray_len(PMIP_pgs);
    struct pmip_pg *arr = ut_type_array(PMIP_pgs, struct pmip_pg *);
    for (int i = 0; i < n; i++) {
        status = callback(&arr[i]);
        HYDU_ERR_POP(status, "foreach_pg_do failed at i = %d / %d", i, n);
    }

  fn_exit:
    return status;
  fn_fail:
    goto fn_exit;
}

/* linear search.
 * Typical case is the first pg when spawn is not used. When spawn is used, we don't
 * expect a single proxy to host too many active spawns
 */
struct pmip_pg *PMIP_find_pg(int pgid, int proxy_id)
{
    int n = utarray_len(PMIP_pgs);
    struct pmip_pg *arr = ut_type_array(PMIP_pgs, struct pmip_pg *);
    for (int i = 0; i < n; i++) {
        if (arr[i].pgid == pgid && arr[i].proxy_id == proxy_id) {
            return &arr[i];
        }
    }
    return NULL;
}

HYD_status PMIP_pg_alloc_downstreams(struct pmip_pg * pg, int num_procs)
{
    HYD_status status = HYD_SUCCESS;

    pg->num_procs = num_procs;
    pg->downstreams = MPL_calloc(num_procs, sizeof(struct pmip_downstream), MPL_MEM_OTHER);
    HYDU_ASSERT(pg->downstreams, status);

    for (int i = 0; i < num_procs; i++) {
        pg->downstreams[i].pg_idx = pg->idx;
        pg->downstreams[i].pid = -1;
        pg->downstreams[i].exit_status = PMIP_EXIT_STATUS_UNSET;
        pg->downstreams[i].pmi_fd = HYD_FD_UNSET;
    }

  fn_exit:
    return status;
  fn_fail:
    goto fn_exit;
}

/* whole-pg downstreams query utilities */

#define GET_DOWNSTREAM_INTARR(pg, field) do { \
    int *p = MPL_malloc(pg->num_procs * sizeof(int), MPL_MEM_OTHER); \
    if (p) { \
        for (int i = 0; i < pg->num_procs; i++) { \
            p[i] = pg->downstreams[i].field; \
        } \
    } \
    return p; \
} while (0)

bool PMIP_pg_has_open_stdoe(struct pmip_pg *pg)
{
    for (int i = 0; i < pg->num_procs; i++) {
        if (pg->downstreams[i].out != HYD_FD_CLOSED || pg->downstreams[i].err != HYD_FD_CLOSED) {
            return true;
        }
    }
    return false;
}

int *PMIP_pg_get_pid_list(struct pmip_pg *pg)
{
    GET_DOWNSTREAM_INTARR(pg, pid);
}

int *PMIP_pg_get_stdout_list(struct pmip_pg *pg)
{
    GET_DOWNSTREAM_INTARR(pg, out);
}

int *PMIP_pg_get_stderr_list(struct pmip_pg *pg)
{
    GET_DOWNSTREAM_INTARR(pg, err);
}

int *PMIP_pg_get_exit_status_list(struct pmip_pg *pg)
{
    GET_DOWNSTREAM_INTARR(pg, exit_status);
}

#define FIND_DOWNSTREAM_BY(field) do { \
    int n = utarray_len(PMIP_pgs); \
    struct pmip_pg *arr = ut_type_array(PMIP_pgs, struct pmip_pg *); \
    for (int i = 0; i < n; i++) { \
        for (int j = 0; j < arr[i].num_procs; j++) { \
            if (arr[i].downstreams[j].field == field) { \
                return &arr[i].downstreams[j]; \
            } \
        } \
    } \
} while (0)


struct pmip_downstream *PMIP_find_downstream_by_fd(int pmi_fd)
{
    FIND_DOWNSTREAM_BY(pmi_fd);
    return NULL;
}

struct pmip_downstream *PMIP_find_downstream_by_pid(int pid)
{
    FIND_DOWNSTREAM_BY(pid);
    return NULL;
}

int PMIP_get_total_process_count(void)
{
    int count = 0;

    int n = utarray_len(PMIP_pgs);
    struct pmip_pg *arr = ut_type_array(PMIP_pgs, struct pmip_pg *);
    for (int i = 0; i < n; i++) {
        count += arr[i].num_procs;
    }

    return count;
}

bool PMIP_has_open_stdoe(void)
{
    int n = utarray_len(PMIP_pgs);
    struct pmip_pg *arr = ut_type_array(PMIP_pgs, struct pmip_pg *);
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < arr[i].num_procs; j++) {
            if (arr[i].downstreams[j].out != HYD_FD_CLOSED ||
                arr[i].downstreams[j].err != HYD_FD_CLOSED) {
                return true;
            }
        }
    }
    return false;
}

void PMIP_bcast_signal(int sig)
{
    int n = utarray_len(PMIP_pgs);
    struct pmip_pg *arr = ut_type_array(PMIP_pgs, struct pmip_pg *);
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < arr[i].num_procs; j++) {
            int pid = arr[i].downstreams[j].pid;
            if (pid != -1) {
#if defined(HAVE_GETPGID) && defined(HAVE_SETSID)
                /* If we are able to get the process group ID, and the
                 * child process has its own process group ID, send a
                 * signal to the entire process group */
                int pgid = getpgid(pid);
                killpg(pgid, sig);
#else
                kill(pid, sig);
#endif
            }
        }
    }
}

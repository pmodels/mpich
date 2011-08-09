/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "topo.h"

#if defined HAVE_PLPA
#include "plpa/topo_plpa.h"
#endif /* HAVE_PLPA */

#if defined HAVE_HWLOC
#include "hwloc/topo_hwloc.h"
#endif /* HAVE_HWLOC */

struct HYDT_topo_info HYDT_topo_info = { 0 };

static HYD_status init_topolib(void)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* Initialize the topology library requested by the user */
#if defined HAVE_PLPA
    if (!strcmp(HYDT_topo_info.topolib, "plpa")) {
        status = HYDT_topo_plpa_init(&HYDT_topo_info.support_level);
        HYDU_ERR_POP(status, "unable to initialize plpa\n");
    }
#endif /* HAVE_PLPA */

#if defined HAVE_HWLOC
    if (!strcmp(HYDT_topo_info.topolib, "hwloc")) {
        status = HYDT_topo_hwloc_init(&HYDT_topo_info.support_level);
        HYDU_ERR_POP(status, "unable to initialize hwloc\n");
    }
#endif /* HAVE_HWLOC */

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status handle_none_binding(void)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* If no binding is given, we just set all mappings to -1 */
    HYDU_MALLOC(HYDT_topo_info.bindmap, struct HYDT_topo_cpuset_t *,
                sizeof(struct HYDT_topo_cpuset_t), status);
    HYDT_topo_info.total_proc_units = 1;
    HYDT_topo_cpuset_zero(&HYDT_topo_info.bindmap[0]);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status handle_user_binding(const char *binding)
{
    int i, j;
    char *bindstr = HYDU_strdup(binding), *bindentry;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* Initialize all values to map to all CPUs */
    for (i = 0; i < HYDT_topo_info.total_proc_units; i++)
        for (j = 0; j < HYDT_topo_info.total_proc_units; j++)
            HYDT_topo_cpuset_set(j, &HYDT_topo_info.bindmap[i]);

    i = 0;
    bindentry = strtok(bindstr, ",");
    while (bindentry) {
        HYDT_topo_cpuset_zero(&HYDT_topo_info.bindmap[i]);
        HYDT_topo_cpuset_set(atoi(bindentry) % HYDT_topo_info.total_proc_units,
                             &HYDT_topo_info.bindmap[i]);
        i++;
        bindentry = strtok(NULL, ",");

        /* If the user provided more OS indices than the number of
         * processing units the system has, ignore the extra ones */
        if (i >= HYDT_topo_info.total_proc_units)
            break;
    }
    HYDU_FREE(bindstr);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static void search_leaf_pu(struct HYDT_topo_obj obj, int leaf, int *idx)
{
    int i;

    if (obj.type == leaf) {
        HYDT_topo_cpuset_dup(obj.cpuset, &HYDT_topo_info.bindmap[*idx]);
        (*idx)++;
    }
    else {
        for (i = 0; i < obj.num_children; i++)
            search_leaf_pu(obj.children[i], leaf, idx);
    }
}

static void assign_proc_units(int leaf)
{
    int idx, i;

    /* assign mappings for the first set of processing elements that
     * the user requested for and then duplicate this to the remaining
     * entries */
    idx = 0;
    search_leaf_pu(HYDT_topo_info.machine, leaf, &idx);
    for (i = idx; i < HYDT_topo_info.total_proc_units; i++)
        HYDT_topo_cpuset_dup(HYDT_topo_info.bindmap[i % idx], &HYDT_topo_info.bindmap[i]);
}

static HYD_status handle_cpu_binding(const char *binding)
{
    int leaf;
    char *bindstr = HYDU_strdup(binding), *bindentry, *elem;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* find the entry after "cpu:" */
    bindentry = strtok(bindstr, ":");
    bindentry = strtok(NULL, ":");

    if (bindentry == NULL) {
        /* No extension option specified; use all resources */
        leaf = HYDT_TOPO_OBJ_END - 1;
    }
    else {
        elem = strtok(bindentry, ",");
        do {
            if (!strcmp(elem, "socket") || !strcmp(elem, "sockets"))
                leaf = HYDT_TOPO_OBJ_SOCKET;
            else if (!strcmp(elem, "core") || !strcmp(elem, "cores"))
                leaf = HYDT_TOPO_OBJ_CORE;
            else if (!strcmp(elem, "thread") || !strcmp(elem, "threads"))
                leaf = HYDT_TOPO_OBJ_THREAD;
            else
                HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                                    "unrecognized binding option %s\n", binding);

            elem = strtok(NULL, ",");
        } while (elem);
    }
    HYDU_FREE(bindstr);

    assign_proc_units(leaf);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status handle_cache_binding(const char *binding)
{
    int i, leaf, cache_depth;
    char *bindstr = HYDU_strdup(binding), *bindentry, *elem;
    struct HYDT_topo_obj *obj;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* find the entry after "cache:" */
    bindentry = strtok(bindstr, ":");
    bindentry = strtok(NULL, ":");

    if (bindentry == NULL) {
        /* No extension option specified; use all resources */
        leaf = HYDT_TOPO_OBJ_END - 1;
    }
    else {
        elem = strtok(bindentry, ",");
        do {
            if (!strcmp(elem, "l3") || !strcmp(elem, "L3"))
                cache_depth = 3;
            else if (!strcmp(elem, "l2") || !strcmp(elem, "L2"))
                cache_depth = 2;
            else if (!strcmp(elem, "l1") || !strcmp(elem, "L1"))
                cache_depth = 1;
            else
                HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                                    "unrecognized binding option %s\n", binding);

            elem = strtok(NULL, ",");
        } while (elem);
    }
    HYDU_FREE(bindstr);

    for (obj = &HYDT_topo_info.machine;;) {
        /* see if the target cache depth is present in this object */
        for (i = 0; i < obj->mem.num_caches; i++)
            if (obj->mem.cache_depth[i] == cache_depth)
                leaf = obj->type;

        /* if we are not at the end yet, move one level deeper */
        if (obj->num_children)
            obj = &obj->children[0];
        else
            break;
    }

    assign_proc_units(leaf);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYDT_topo_init(char *user_binding, char *user_topolib)
{
    const char *binding = NULL, *topolib = NULL;
    int i;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (user_binding)
        binding = user_binding;
    else if (MPL_env2str("HYDRA_BINDING", &binding) == 0)
        binding = NULL;

    if (user_topolib)
        topolib = user_topolib;
    else if (MPL_env2str("HYDRA_TOPOLIB", &topolib) == 0)
        topolib = HYDRA_DEFAULT_TOPOLIB;

    HYDT_topo_info.support_level = HYDT_TOPO_SUPPORT_NONE;
    if (topolib)
        HYDT_topo_info.topolib = HYDU_strdup(topolib);
    else
        HYDT_topo_info.topolib = NULL;
    HYDT_topo_info.bindmap = NULL;


    HYDT_topo_init_obj(&HYDT_topo_info.machine);


    if (!binding || !strcmp(binding, "none")) {
        status = handle_none_binding();
        HYDU_ERR_POP(status, "error handling none binding\n");
        goto fn_exit;
    }

    status = init_topolib();
    HYDU_ERR_POP(status, "error initializing topolib\n");


    /* no binding support available */
    if (HYDT_topo_info.support_level == HYDT_TOPO_SUPPORT_NONE) {
        HYDU_MALLOC(HYDT_topo_info.bindmap, struct HYDT_topo_cpuset_t *,
                    sizeof(struct HYDT_topo_cpuset_t), status);
        HYDT_topo_info.total_proc_units = 1;
        HYDT_topo_cpuset_zero(&HYDT_topo_info.bindmap[0]);

        goto fn_exit;
    }


    /* at least basic support available */
    HYDU_MALLOC(HYDT_topo_info.bindmap, struct HYDT_topo_cpuset_t *,
                HYDT_topo_info.total_proc_units * sizeof(struct HYDT_topo_cpuset_t), status);

    /* Initialize all entries */
    for (i = 0; i < HYDT_topo_info.total_proc_units; i++)
        HYDT_topo_cpuset_zero(&HYDT_topo_info.bindmap[i]);


    /* handle basic support level */
    if (!strncmp(binding, "user:", strlen("user:"))) {
        status = handle_user_binding(binding + strlen("user:"));
        HYDU_ERR_POP(status, "error handling user binding\n");
        goto fn_exit;
    }

    if (!strcmp(binding, "rr")) {
        for (i = 0; i < HYDT_topo_info.total_proc_units; i++)
            HYDT_topo_cpuset_set(i, &HYDT_topo_info.bindmap[i]);
        goto fn_exit;
    }


    /* If we reached here, the user requested for CPU topology aware
     * binding. */
    if (HYDT_topo_info.support_level < HYDT_TOPO_SUPPORT_CPUTOPO)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                            "topology binding not supported on this platform\n");


    /* handle CPU topology aware binding */
    if (!strncmp(binding, "cpu", strlen("cpu"))) {
        status = handle_cpu_binding(binding);
        HYDU_ERR_POP(status, "error handling cpu binding\n");
        goto fn_exit;
    }

    /* handle memory topology aware binding */
    if (!strncmp(binding, "cache", strlen("cache"))) {
        status = handle_cache_binding(binding);
        HYDU_ERR_POP(status, "error handling cache binding\n");
        goto fn_exit;
    }

    HYDU_dump(stderr, "unrecognized binding mode \"%s\"; ignoring\n", binding);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static void cleanup_topo_level(struct HYDT_topo_obj level)
{
    int i;

    level.parent = NULL;

    if (level.mem.cache_size)
        HYDU_FREE(level.mem.cache_size);

    if (level.mem.cache_depth)
        HYDU_FREE(level.mem.cache_depth);

    if (level.children) {
        for (i = 0; i < level.num_children; i++)
            cleanup_topo_level(level.children[i]);
        HYDU_FREE(level.children);
    }
}

HYD_status HYDT_topo_bind(struct HYDT_topo_cpuset_t cpuset)
{
    int i;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

#if defined HAVE_PLPA
    if (!strcmp(HYDT_topo_info.topolib, "plpa")) {
        status = HYDT_topo_plpa_bind(cpuset);
        HYDU_ERR_POP(status, "PLPA failure binding process to core\n");
        goto fn_exit;
    }
#endif /* HAVE_PLPA */

#if defined HAVE_HWLOC
    if (!strcmp(HYDT_topo_info.topolib, "hwloc")) {
        status = HYDT_topo_hwloc_bind(cpuset);
        HYDU_ERR_POP(status, "HWLOC failure binding process to core\n");
        goto fn_exit;
    }
#endif /* HAVE_HWLOC */

    for (i = 0; i < HYDT_TOPO_MAX_CPU_COUNT / SIZEOF_UNSIGNED_LONG; i++) {
        if (cpuset.set[i]) {
            HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "no topology library available\n");
        }
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

void HYDT_topo_pid_to_cpuset(int process_id, struct HYDT_topo_cpuset_t *cpuset)
{
    HYDT_topo_cpuset_dup(HYDT_topo_info.bindmap[process_id % HYDT_topo_info.total_proc_units],
                         cpuset);
}

HYD_status HYDT_topo_finalize(void)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* Finalize the topology library requested by the user */
#if defined HAVE_PLPA
    if (!strcmp(HYDT_topo_info.topolib, "plpa")) {
        status = HYDT_topo_plpa_finalize();
        HYDU_ERR_POP(status, "unable to finalize plpa\n");
    }
#endif /* HAVE_PLPA */

#if defined HAVE_HWLOC
    if (!strcmp(HYDT_topo_info.topolib, "hwloc")) {
        status = HYDT_topo_hwloc_finalize();
        HYDU_ERR_POP(status, "unable to finalize hwloc\n");
    }
#endif /* HAVE_HWLOC */

    if (HYDT_topo_info.bindmap)
        HYDU_FREE(HYDT_topo_info.bindmap);

    if (HYDT_topo_info.topolib)
        HYDU_FREE(HYDT_topo_info.topolib);

    cleanup_topo_level(HYDT_topo_info.machine);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

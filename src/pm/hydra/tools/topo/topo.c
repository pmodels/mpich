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

HYD_status HYDT_topo_init(char *user_binding, char *user_topolib)
{
    char *bindstr, *bindentry, *elem;
    const char *binding = NULL, *topolib = NULL;
    int i, j, k, use_topo_obj[HYDT_TOPO_OBJ_END] = { 0 }, child_id;
    int use_cache_level = 0, break_out;
    HYDT_topo_obj_type_t topo_end = HYDT_TOPO_OBJ_END;
    struct HYDT_topo_obj *obj;
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

    /***************************** NONE *****************************/
    if (!binding || !strcmp(binding, "none")) {
        /* If no binding is given, we just set all mappings to -1 */
        HYDU_MALLOC(HYDT_topo_info.bindmap, struct HYDT_topo_cpuset_t *,
                    sizeof(struct HYDT_topo_cpuset_t), status);
        HYDT_topo_info.total_proc_units = 1;
        HYDT_topo_cpuset_zero(&HYDT_topo_info.bindmap[0]);

        goto fn_exit;
    }

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

    /* If we are not able to initialize the topology library, we set
     * all mappings to -1 */
    if (HYDT_topo_info.support_level == HYDT_TOPO_SUPPORT_NONE) {
        HYDU_MALLOC(HYDT_topo_info.bindmap, struct HYDT_topo_cpuset_t *,
                    sizeof(struct HYDT_topo_cpuset_t), status);
        HYDT_topo_info.total_proc_units = 1;
        HYDT_topo_cpuset_zero(&HYDT_topo_info.bindmap[0]);

        goto fn_exit;
    }

    /* We at least have basic support from the topology library, so the
     * total_proc_units field is valid */
    HYDU_MALLOC(HYDT_topo_info.bindmap, struct HYDT_topo_cpuset_t *,
                HYDT_topo_info.total_proc_units * sizeof(struct HYDT_topo_cpuset_t), status);

    /* Initialize all entries */
    for (i = 0; i < HYDT_topo_info.total_proc_units; i++)
        HYDT_topo_cpuset_zero(&HYDT_topo_info.bindmap[i]);


    /***************************** USER *****************************/
    if (!strncmp(binding, "user:", strlen("user:"))) {
        /* Initialize all values to map to all CPUs */
        for (i = 0; i < HYDT_topo_info.total_proc_units; i++)
            for (j = 0; j < HYDT_topo_info.total_proc_units; j++)
                HYDT_topo_cpuset_set(j, &HYDT_topo_info.bindmap[i]);

        i = 0;
        bindstr = HYDU_strdup(binding + strlen("user:"));
        bindentry = strtok(bindstr, ",");
        while (bindentry) {
            HYDT_topo_cpuset_zero(&HYDT_topo_info.bindmap[i]);
            HYDT_topo_cpuset_set(atoi(bindentry) % HYDT_topo_info.total_proc_units,
                                 &HYDT_topo_info.bindmap[i]);
            i++;
            bindentry = strtok(NULL, ",");

            /* If the user provided more OS indices than the number of
             * processing units the system has, ignore the extra
             * ones */
            if (i >= HYDT_topo_info.total_proc_units)
                break;
        }
        HYDU_FREE(bindstr);

        goto fn_exit;
    }

    /***************************** RR *****************************/
    if (!strcmp(binding, "rr")) {
        for (i = 0; i < HYDT_topo_info.total_proc_units; i++)
            HYDT_topo_cpuset_set(i, &HYDT_topo_info.bindmap[i]);

        goto fn_exit;
    }


    /***************************** CPU *****************************/
    if (!strncmp(binding, "cpu", strlen("cpu"))) {
        /* If we reached here, the user requested for CPU topology
         * aware binding. */
        if (HYDT_topo_info.support_level < HYDT_TOPO_SUPPORT_CPUTOPO)
            HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                                "topology binding not supported on this platform\n");

        bindstr = HYDU_strdup(binding);
        bindentry = strtok(bindstr, ":");
        bindentry = strtok(NULL, ":");

        if (bindentry == NULL) {
            /* No extension option specified; use all resources */
            for (i = HYDT_TOPO_OBJ_MACHINE; i < HYDT_TOPO_OBJ_END; i++)
                use_topo_obj[i] = 1;
        }
        else {
            elem = strtok(bindentry, ",");
            do {
                if (!strcmp(elem, "socket") || !strcmp(elem, "sockets"))
                    use_topo_obj[HYDT_TOPO_OBJ_SOCKET] = 1;
                else if (!strcmp(elem, "core") || !strcmp(elem, "cores"))
                    use_topo_obj[HYDT_TOPO_OBJ_CORE] = 1;
                else if (!strcmp(elem, "thread") || !strcmp(elem, "threads"))
                    use_topo_obj[HYDT_TOPO_OBJ_THREAD] = 1;
                else
                    HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                                        "unrecognized binding option %s\n", binding);

                elem = strtok(NULL, ",");
            } while (elem);
        }
        HYDU_FREE(bindstr);

        for (i = HYDT_TOPO_OBJ_END - 1; i > HYDT_TOPO_OBJ_MACHINE; i--) {
            /* If an object has to be used, its parent object is also
             * used. For example, you cannot use a core without using
             * a socket. */
            if (use_topo_obj[i])
                use_topo_obj[i - 1] = 1;
        }
        /* We should have at least one process on the node; otherwise,
         * the mapping makes no sense */
        for (i = HYDT_TOPO_OBJ_MACHINE; i < HYDT_TOPO_OBJ_NODE; i++)
            use_topo_obj[i] = 1;

        topo_end = HYDT_TOPO_OBJ_END;
        for (i = HYDT_TOPO_OBJ_MACHINE; i < HYDT_TOPO_OBJ_END; i++) {
            if (use_topo_obj[i] == 0) {
                topo_end = (HYDT_topo_obj_type_t) i;
                break;
            }
        }
    }


    /***************************** CACHE *****************************/
    if (!strncmp(binding, "cache", strlen("cache"))) {
        /* If we reached here, the user requested for memory topology
         * aware binding. */
        if (HYDT_topo_info.support_level < HYDT_TOPO_SUPPORT_MEMTOPO)
            HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                                "memory topology binding not supported on this platform\n");

        bindstr = HYDU_strdup(binding);
        bindentry = strtok(bindstr, ":");
        bindentry = strtok(NULL, ":");

        /* Don't share any cache to start with */
        use_cache_level = HYDT_INVALID_CACHE_DEPTH;
        if (bindentry == NULL) {
            /* No extension option specified; use all resources */
            use_cache_level = 1;
        }
        else {
            elem = strtok(bindentry, ",");
            do {
                if (!strcmp(elem, "l3") || !strcmp(elem, "L3")) {
                    if (use_cache_level > 3)
                        use_cache_level = 3;
                }
                else if (!strcmp(elem, "l2") || !strcmp(elem, "L2")) {
                    if (use_cache_level > 2)
                        use_cache_level = 2;
                }
                else if (!strcmp(elem, "l1") || !strcmp(elem, "L1")) {
                    if (use_cache_level > 1)
                        use_cache_level = 1;
                }
                else {
                    HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                                        "unrecognized binding option\n");
                }

                elem = strtok(NULL, ",");
            } while (elem);
        }
        HYDU_FREE(bindstr);

        topo_end = HYDT_TOPO_OBJ_END;
        obj = &HYDT_topo_info.machine;
        break_out = 0;
        for (i = HYDT_TOPO_OBJ_MACHINE; i < HYDT_TOPO_OBJ_END; i++) {
            for (j = 0; j < obj->mem.num_caches; j++) {
                /* If the cache level is lower than what I'm allowed
                 * to share, and there are more than one OS indices
                 * below this level (there is actually sharing) mark
                 * this as the lowest level I can get to */
                if (obj->mem.cache_depth[j] == use_cache_level) {
                    topo_end = (HYDT_topo_obj_type_t) (i + 1);
                    break_out = 1;
                    break;
                }
            }
            if (break_out)
                break;

            obj = obj->children;
        }
    }

    /* Common part for the CPU and Cache binding schemes */
    if (!strncmp(binding, "cpu", strlen("cpu")) || !strncmp(binding, "cache", strlen("cache"))) {
        i = 0;
        j = 0;
        obj = &HYDT_topo_info.machine;
        while (1) {
            /* go down the left most branch from where we are */
            for (; j < topo_end - 1; j++)
                obj = obj->children;

            HYDT_topo_cpuset_dup(obj->cpuset, &HYDT_topo_info.bindmap[i]);
            i++;

            child_id = HYDT_TOPO_OBJ_CHILD_ID(obj);
            if (child_id < obj->parent->num_children - 1) {
                /* Move to the next sibling */
                obj++;
                continue;
            }
            else {
                /* No more siblings; move to an ancestor who has a
                 * sibling */
                do {
                    obj = obj->parent;
                    j--;
                    if (obj == NULL || obj->parent == NULL)
                        break;

                    child_id = HYDT_TOPO_OBJ_CHILD_ID(obj);
                } while (child_id == obj->parent->num_children - 1);

                /* If we are out of ancestors; break out */
                if (obj == NULL || obj->parent == NULL)
                    break;

                /* Else, move to the ancestor's sibling */
                obj++;
            }
        }

        /* Store the number of values we got from the topology. Repeat
         * these values till we fill in all the processing elements */
        j = i;
        while (1) {
            for (k = 0; k < j; k++) {
                if (i == HYDT_topo_info.total_proc_units)
                    break;
                HYDT_topo_info.bindmap[i++] = HYDT_topo_info.bindmap[k];
            }
            if (i == HYDT_topo_info.total_proc_units)
                break;
        }

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

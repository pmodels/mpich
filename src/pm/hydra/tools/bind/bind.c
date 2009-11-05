/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_utils.h"
#include "bind.h"

#if defined HAVE_PLPA
#include "plpa/bind_plpa.h"
#endif /* HAVE_PLPA */

#if defined HAVE_HWLOC
#include "hwloc/bind_hwloc.h"
#endif /* HAVE_HWLOC */

struct HYDT_bind_info HYDT_bind_info;

HYD_status HYDT_bind_init(char *binding, char *bindlib)
{
    char *bindstr, *bindentry, *obj;
    int i, j, k, use_topo_obj[HYDT_TOPO_END] = { 0 }, child_id, found_obj;
    HYDT_topo_obj_type_t topo_end;
    struct HYDT_topo_obj *topo_obj[HYDT_TOPO_END];
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDT_bind_info.support_level = HYDT_BIND_NONE;
    HYDT_bind_info.bindlib = HYDU_strdup(bindlib);
    HYDT_bind_info.bindmap = NULL;

    /***************************** NONE *****************************/
    if (!binding || !strcmp(binding, "none")) {
        /* If no binding is given, we just set all mappings to -1 */
        HYDU_MALLOC(HYDT_bind_info.bindmap, int *, sizeof(int), status);
        HYDT_bind_info.total_proc_units = 1;
        HYDT_bind_info.bindmap[0] = -1;

        goto fn_exit;
    }

    /* Initialize the binding library requested by the user */
#if defined HAVE_PLPA
    if (!strcmp(HYDT_bind_info.bindlib, "plpa")) {
        status = HYDT_bind_plpa_init(&HYDT_bind_info.support_level);
        HYDU_ERR_POP(status, "unable to initialize plpa\n");
    }
#endif /* HAVE_PLPA */

#if defined HAVE_HWLOC
    if (!strcmp(HYDT_bind_info.bindlib, "hwloc")) {
        status = HYDT_bind_hwloc_init(&HYDT_bind_info.support_level);
        HYDU_ERR_POP(status, "unable to initialize hwloc\n");
    }
#endif /* HAVE_HWLOC */

    /* If we are not able to initialize the binding library, we set
     * all mappings to -1 */
    if (HYDT_bind_info.support_level == HYDT_BIND_NONE) {
        HYDU_MALLOC(HYDT_bind_info.bindmap, int *, sizeof(int), status);
        HYDT_bind_info.total_proc_units = 1;
        HYDT_bind_info.bindmap[0] = -1;

        goto fn_exit;
    }


    /***************************** USER *****************************/
    if (!strncmp(binding, "user:", strlen("user:"))) {
        /* Find the number of processing elements */
        bindstr = HYDU_strdup(binding + strlen("user:"));
        HYDT_bind_info.total_proc_units = 0;
        bindentry = strtok(bindstr, ",");
        while (bindentry) {
            HYDT_bind_info.total_proc_units++;
            bindentry = strtok(NULL, ",");
        }

        /* Find the actual processing elements */
        HYDU_MALLOC(HYDT_bind_info.bindmap, int *,
                    HYDT_bind_info.total_proc_units * sizeof(int), status);
        i = 0;
        bindstr = HYDU_strdup(binding + strlen("user:"));
        bindentry = strtok(bindstr, ",");
        while (bindentry) {
            HYDT_bind_info.bindmap[i++] = atoi(bindentry);
            bindentry = strtok(NULL, ",");
        }

        goto fn_exit;
    }


    /***************************** RR *****************************/
    HYDU_MALLOC(HYDT_bind_info.bindmap, int *,
                HYDT_bind_info.total_proc_units * sizeof(int), status);

    /* RR is supported at the basic binding level */
    if (!strcmp(binding, "rr")) {
        for (i = 0; i < HYDT_bind_info.total_proc_units; i++)
            HYDT_bind_info.bindmap[i] = i;

        goto fn_exit;
    }


    /* If we reached here, the user requested for topology aware
     * binding. */
    if (HYDT_bind_info.support_level != HYDT_BIND_TOPO)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                            "topology binding not supported on this platform\n");


    /***************************** TOPO *****************************/
    if (!strncmp(binding, "topo", strlen("topo"))) {
        bindstr = HYDU_strdup(binding);
        bindentry = strtok(bindstr, ":");
        bindentry = strtok(NULL, ":");

        if (bindentry == NULL) {
            /* No extension option specified; use all resources */
            for (i = HYDT_TOPO_MACHINE; i < HYDT_TOPO_END; i++)
                use_topo_obj[i] = 1;
        }
        else {
            obj = strtok(bindentry, ",");
            do {
                if (!strcmp(obj, "node"))
                    use_topo_obj[HYDT_TOPO_NODE] = 1;
                else if (!strcmp(obj, "socket") || !strcmp(obj, "sockets"))
                    use_topo_obj[HYDT_TOPO_SOCKET] = 1;
                else if (!strcmp(obj, "core") || !strcmp(obj, "cores"))
                    use_topo_obj[HYDT_TOPO_CORE] = 1;
                else if (!strcmp(obj, "thread") || !strcmp(obj, "threads"))
                    use_topo_obj[HYDT_TOPO_THREAD] = 1;
                else
                    HYDU_ERR_POP(status, "unrecognized binding option\n");

                obj = strtok(NULL, ",");
            } while (obj);
        }

        for (i = HYDT_TOPO_END - 1; i > HYDT_TOPO_MACHINE; i--) {
            if (use_topo_obj[i])
                use_topo_obj[i - 1] = 1;
        }

        topo_end = HYDT_TOPO_END;
        for (i = HYDT_TOPO_MACHINE; i < HYDT_TOPO_END; i++) {
            if (use_topo_obj[i] == 0) {
                topo_end = i;
                break;
            }
        }

        /* Initialize indices and topology objects */
        topo_obj[HYDT_TOPO_MACHINE] = &HYDT_bind_info.machine;
        for (j = HYDT_TOPO_MACHINE; j < HYDT_TOPO_END; j++) {
            if (j)
                topo_obj[j] = topo_obj[j - 1]->children;
        }

        for (i = 0; i < HYDT_bind_info.total_proc_units; i++) {
            HYDT_bind_info.bindmap[i] = topo_obj[HYDT_TOPO_END - 1]->os_index;

            /* If we are done, break out */
            if (i == HYDT_bind_info.total_proc_units - 1)
                break;

            /* If not, increment the object structure */
            found_obj = 0;

            for (j = HYDT_TOPO_END - 1; j > HYDT_TOPO_MACHINE; j--) {

                /* If our topology depth is greater than what the user
                 * requested, don't try to find any more siblings */
                if (j >= topo_end)
                    continue;

                child_id = HYDT_TOPO_CHILD_ID(topo_obj[j]);
                if (child_id < topo_obj[j]->parent->num_children - 1) {
                    /* This object is not the last of the siblings;
                     * move to the next sibling */
                    topo_obj[j] = &topo_obj[j]->parent->children[child_id + 1];
                    for (k = j + 1; k < HYDT_TOPO_END; k++)
                        topo_obj[k] = topo_obj[k - 1]->children;

                    found_obj = 1;
                    break;
                }
            }

            if (!found_obj) {
                /* Initialize indices and topology objects */
                topo_obj[HYDT_TOPO_MACHINE] = &HYDT_bind_info.machine;
                for (j = HYDT_TOPO_MACHINE; j < HYDT_TOPO_END; j++) {
                    if (j)
                        topo_obj[j] = topo_obj[j - 1]->children;
                }
            }
        }

        goto fn_exit;
    }


    /* If we reached here, the user requested for topology aware
     * binding. */
    if (HYDT_bind_info.support_level != HYDT_BIND_MEMTOPO)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                            "memory topology binding not supported on this platform\n");

    /***************************** TOPOMEM *****************************/
    if (!strncmp(binding, "topomem", strlen("topomem"))) {
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                            "memory-aware topology binding is not implemented yet\n");
    }


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

    if (level.shared_memory_depth)
        HYDU_FREE(level.shared_memory_depth);

    if (level.children)
        for (i = 0; i < level.num_children; i++)
            cleanup_topo_level(level.children[i]);
}

void HYDT_bind_finalize(void)
{
    if (HYDT_bind_info.bindmap)
        HYDU_FREE(HYDT_bind_info.bindmap);

    if (HYDT_bind_info.bindlib)
        HYDU_FREE(HYDT_bind_info.bindlib);

    cleanup_topo_level(HYDT_bind_info.machine);
}

HYD_status HYDT_bind_process(int os_index)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

#if defined HAVE_PLPA
    if (!strcmp(HYDT_bind_info.bindlib, "plpa")) {
        status = HYDT_bind_plpa_process(os_index);
        HYDU_ERR_POP(status, "PLPA failure binding process to core\n");
    }
#endif /* HAVE_PLPA */

#if defined HAVE_HWLOC
    if (!strcmp(HYDT_bind_info.bindlib, "hwloc")) {
        status = HYDT_bind_hwloc_process(os_index);
        HYDU_ERR_POP(status, "HWLOC failure binding process to core\n");
    }
#endif /* HAVE_HWLOC */

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

int HYDT_bind_get_os_index(int process_id)
{
    /* TODO: Allow the binding layer to export CPU sets instead of
     * single units */

    return HYDT_bind_info.bindmap[process_id % HYDT_bind_info.total_proc_units];
}

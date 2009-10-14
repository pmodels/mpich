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

struct HYDU_bind_info HYDU_bind_info;

HYD_Status HYDU_bind_init(char *binding, char *bindlib)
{
    char *bindstr, *bindentry;
    int sock, core, thread, i, j;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_bind_info.support_level = HYDU_BIND_NONE;
    HYDU_bind_info.topology = NULL;
    HYDU_bind_info.bindlib = HYDU_strdup(bindlib);

    if (!binding || !strcmp(binding, "none")) {
        /* If no binding is given, we just set all mappings to -1 */
        HYDU_MALLOC(HYDU_bind_info.bindmap, int *, sizeof(int), status);
        HYDU_bind_info.num_procs = 1;
        HYDU_bind_info.bindmap[0] = -1;

        goto fn_exit;
    }

    if (!strncmp(binding, "user:", strlen("user:"))) {
        /* If the user specified the binding, we don't need to
         * initialize anything */
        bindstr = HYDU_strdup(binding + strlen("user:"));

        /* Find the number of processing elements */
        HYDU_bind_info.num_procs = 0;
        bindentry = strtok(bindstr, ",");
        while (bindentry) {
            HYDU_bind_info.num_procs++;
            bindentry = strtok(NULL, ",");
        }

        /* Find the actual processing elements */
        HYDU_MALLOC(HYDU_bind_info.bindmap, int *, HYDU_bind_info.num_procs * sizeof(int),
                    status);
        i = 0;
        bindentry = strtok(bindstr, ",");
        while (bindentry) {
            HYDU_bind_info.bindmap[i++] = atoi(bindentry);
            bindentry = strtok(NULL, ",");
        }

        goto fn_exit;
    }

    /* If a real binding is required, we initialize the binding
     * library requested by the user */
#if defined HAVE_PLPA
    if (!strcmp(HYDU_bind_info.bindlib, "plpa")) {
        status = HYDU_bind_plpa_init(&HYDU_bind_info.support_level);
        HYDU_ERR_POP(status, "unable to initialize plpa\n");
    }
#endif /* HAVE_PLPA */

    if (HYDU_bind_info.support_level != HYDU_BIND_NONE) {
        HYDU_MALLOC(HYDU_bind_info.bindmap, int *, HYDU_bind_info.num_procs * sizeof(int),
                    status);

        for (i = 0; i < HYDU_bind_info.num_procs; i++) {

            /* RR is supported at the basic binding level */
            if (!strcmp(binding, "rr")) {
                HYDU_bind_info.bindmap[i] = i;
                continue;
            }

            /* If we reached here, the user requested for topology
             * aware binding. */
            if (HYDU_bind_info.support_level != HYDU_BIND_TOPO)
                HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                                    "topology binding not supported on this platform\n");

            if (!strcmp(binding, "buddy")) {
                thread = i / (HYDU_bind_info.num_sockets * HYDU_bind_info.num_cores);

                core = i % (HYDU_bind_info.num_sockets * HYDU_bind_info.num_cores);
                core /= HYDU_bind_info.num_sockets;

                sock = i % HYDU_bind_info.num_sockets;
            }
            else if (!strcmp(binding, "pack")) {
                sock = i / (HYDU_bind_info.num_cores * HYDU_bind_info.num_threads);

                core = i % (HYDU_bind_info.num_cores * HYDU_bind_info.num_threads);
                core /= HYDU_bind_info.num_threads;

                thread = i % HYDU_bind_info.num_threads;
            }
            else {
                HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "unknown binding option\n");
            }

            for (j = 0; j < HYDU_bind_info.num_procs; j++) {
                if (HYDU_bind_info.topology[j].socket_rank == sock &&
                    HYDU_bind_info.topology[j].core_rank == core &&
                    HYDU_bind_info.topology[j].thread_rank == thread) {
                    HYDU_bind_info.bindmap[i] = HYDU_bind_info.topology[j].processor_id;
                    break;
                }
            }
        }
    }
    else {
        /* If no binding is supported, we just set all mappings to -1 */
        HYDU_MALLOC(HYDU_bind_info.bindmap, int *, sizeof(int), status);
        HYDU_bind_info.num_procs = 1;
        HYDU_bind_info.bindmap[0] = -1;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

void HYDU_bind_finalize(void)
{
    if (HYDU_bind_info.bindmap)
        HYDU_FREE(HYDU_bind_info.bindmap);

    if (HYDU_bind_info.bindlib)
        HYDU_FREE(HYDU_bind_info.bindlib);

    if (HYDU_bind_info.topology)
        HYDU_FREE(HYDU_bind_info.topology);
}

HYD_Status HYDU_bind_process(int core)
{
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

#if defined HAVE_PLPA
    if (!strcmp(HYDU_bind_info.bindlib, "plpa")) {
        status = HYDU_bind_plpa_process(core);
        HYDU_ERR_POP(status, "PLPA failure binding process to core\n");
    }
#endif /* HAVE_PLPA */

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


int HYDU_bind_get_core_id(int id)
{
    return HYDU_bind_info.bindmap[id % HYDU_bind_info.num_procs];
}

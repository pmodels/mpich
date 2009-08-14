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
    char *user_bind_map;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_bind_info.support_level = HYDU_BIND_NONE;
    HYDU_bind_info.bind_map = NULL;

    if (binding)
        HYDU_bind_info.binding = HYDU_strdup(binding);
    else
        HYDU_bind_info.binding = HYDU_strdup("none");

    if (!strncmp(HYDU_bind_info.binding, "user:", strlen("user:"))) {
        user_bind_map = HYDU_bind_info.binding + strlen("user:");
        HYDU_bind_info.binding[strlen("user")] = '\0';
    }
    else
        user_bind_map = NULL;

    HYDU_bind_info.bindlib = bindlib;

#if defined HAVE_PLPA
    if (!strcmp(HYDU_bind_info.bindlib, "plpa")) {
        status = HYDU_bind_plpa_init(user_bind_map, &HYDU_bind_info.support_level);
        HYDU_ERR_POP(status, "unable to initialize plpa\n");
    }
#endif /* HAVE_PLPA */

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
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
    int socket, core, thread, i, realid;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (HYDU_bind_info.support_level != HYDU_BIND_NONE) {
        realid = (id % HYDU_bind_info.num_procs);

        /* NONE, RR and USER are supported at the basic binding
         * level */
        if (!strcmp(HYDU_bind_info.binding, "none")) {
            return -1;
        }
        else if (!strcmp(HYDU_bind_info.binding, "rr")) {
            return HYDU_bind_info.bind_map[realid].processor_id;
        }
        else if (!strcmp(HYDU_bind_info.binding, "user")) {
            if (!HYDU_bind_info.user_bind_valid)
                return -1;
            else
                return HYDU_bind_info.user_bind_map[realid];
        }

        /* If we reached here, the user requested for topology aware
         * binding. */
        if (HYDU_bind_info.support_level == HYDU_BIND_TOPO) {
            if (!strcmp(HYDU_bind_info.binding, "buddy")) {
                thread = realid / (HYDU_bind_info.num_sockets * HYDU_bind_info.num_cores);

                core = realid % (HYDU_bind_info.num_sockets * HYDU_bind_info.num_cores);
                core /= HYDU_bind_info.num_sockets;

                socket = realid % HYDU_bind_info.num_sockets;
            }
            else if (!strcmp(HYDU_bind_info.binding, "pack")) {
                socket = realid / (HYDU_bind_info.num_cores * HYDU_bind_info.num_threads);

                core = realid % (HYDU_bind_info.num_cores * HYDU_bind_info.num_threads);
                core /= HYDU_bind_info.num_threads;

                thread = realid % HYDU_bind_info.num_threads;
            }
            else {
                HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "unknown binding option\n");
            }

            for (i = 0; i < HYDU_bind_info.num_procs; i++) {
                if (HYDU_bind_info.bind_map[i].socket_rank == socket &&
                    HYDU_bind_info.bind_map[i].core_rank == core &&
                    HYDU_bind_info.bind_map[i].thread_rank == thread) {
                    return HYDU_bind_info.bind_map[i].processor_id;
                }
            }
        }
        else {
            HYDU_Error_printf("Topology-aware binding is not supported on this platform\n");
        }
    }

fn_exit:
    HYDU_FUNC_EXIT();
    return -1;

fn_fail:
    goto fn_exit;
}

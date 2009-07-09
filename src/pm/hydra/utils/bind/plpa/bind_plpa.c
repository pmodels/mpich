/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_utils.h"
#include "bind.h"

struct HYDU_bind_info HYDU_bind_info;

#include "plpa.h"
#include "plpa_internal.h"

HYD_Status HYDU_bind_plpa_init(char *user_bind_map, int *supported)
{
    PLPA_NAME(api_type_t) p;
    int ret, i, j;
    int num_procs, max_proc_id;
    int num_sockets = -1, max_socket_id;
    int num_cores = -1, max_core_id;
    int socket, core;
    char *str;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (!((PLPA_NAME(api_probe) (&p) == 0) && (p == PLPA_NAME_CAPS(PROBE_OK)))) {
        /* If this failed, we just return without binding */
        HYDU_Warn_printf("plpa api probe failed; not binding\n");
        goto fn_exit;
    }

    /* We need topology information too */
    ret = PLPA_NAME(have_topology_information) (supported);
    if ((ret == 0) && (*supported == 1)) {
        /* Find the maximum number of processing elements */
        ret = PLPA_NAME(get_processor_data) (PLPA_NAME_CAPS(COUNT_ALL), &num_procs,
                                             &max_proc_id);
        if (ret) {
            HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                                "plpa get processor data failed\n");
        }

        /* PLPA only gives information about sockets and cores */
        ret = PLPA_NAME(get_socket_info) (&num_sockets, &max_socket_id);
        if (ret) {
            HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                                "plpa get processor data failed\n");
        }

        ret = PLPA_NAME(get_core_info) (0, &num_cores, &max_core_id);
        if (ret) {
            HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                                "plpa get processor data failed\n");
        }

        HYDU_MALLOC(HYDU_bind_info.bind_map, int **, num_sockets * sizeof(int *), status);
        for (i = 0; i < num_sockets; i++) {
            HYDU_MALLOC(HYDU_bind_info.bind_map[i], int *, num_cores * sizeof(int), status);
            for (j = 0; j < num_cores; j++)
                HYDU_bind_info.bind_map[i][j] = -1;
        }

        for (i = 0; i < num_sockets * num_cores; i++) {
            ret = PLPA_NAME(map_to_socket_core) (i, &socket, &core);
            if (ret)
                HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                                    "plpa map_to_socket_core failed\n");
            HYDU_bind_info.bind_map[socket][core] = i;
        }
    }
    else {
        /* If this failed, we just return without binding */
        HYDU_Warn_printf("plpa unable to get topology information; not binding\n");
        goto fn_exit;
    }

    if (user_bind_map) {
        HYDU_MALLOC(HYDU_bind_info.user_bind_map, int *, num_cores * num_sockets * sizeof(int),
                    status);
        for (i = 0; i < num_sockets * num_cores; i++)
            HYDU_bind_info.user_bind_map[i] = -1;

        HYDU_bind_info.user_bind_valid = 1;
        i = 0;
        str = strtok(user_bind_map, ",");
        do {
            if (!str || i >= num_cores * num_sockets)
                break;
            HYDU_bind_info.user_bind_map[i++] = atoi(str);
            str = strtok(NULL, ",");
        } while (1);
    }

    HYDU_bind_info.num_procs = num_procs;
    HYDU_bind_info.num_sockets = num_sockets;
    HYDU_bind_info.num_cores = num_cores;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_Status HYDU_bind_plpa_process(int core)
{
    int ret;
    PLPA_NAME(cpu_set_t) cpuset;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    PLPA_NAME_CAPS(CPU_ZERO) (&cpuset);
    PLPA_NAME_CAPS(CPU_SET) (core % HYDU_bind_info.num_procs, &cpuset);
    ret = PLPA_NAME(sched_setaffinity) (0, 1, &cpuset);
    if (ret)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "plpa setaffinity failed\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

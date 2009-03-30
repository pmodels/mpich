/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_utils.h"

static struct bind_info {
    int supported;
    int num_procs;
    int num_sockets;
    int num_cores;

    int **bind_map;

    int user_bind_valid;
    int *user_bind_map;
} bind_info;

HYD_Status HYDU_bind_init(char *user_bind_map)
{
    PLPA_NAME(api_type_t) p;
    int ret, supported, i, j;
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
    ret = PLPA_NAME(have_topology_information) (&supported);
    if ((ret == 0) && (supported == 1)) {
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

        HYDU_MALLOC(bind_info.bind_map, int **, num_sockets * sizeof(int *), status);
        for (i = 0; i < num_sockets; i++) {
            HYDU_MALLOC(bind_info.bind_map[i], int *, num_cores * sizeof(int), status);
            for (j = 0; j < num_cores; j++)
                bind_info.bind_map[i][j] = -1;
        }

        for (i = 0; i < num_sockets * num_cores; i++) {
            ret = PLPA_NAME(map_to_socket_core) (i, &socket, &core);
            if (ret)
                HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                                    "plpa map_to_socket_core failed\n");
            bind_info.bind_map[socket][core] = i;
        }
    }
    else {
        /* If this failed, we just return without binding */
        HYDU_Warn_printf("plpa unable to get topology information; not binding\n");
        goto fn_exit;
    }

    if (user_bind_map) {
        HYDU_MALLOC(bind_info.user_bind_map, int *, num_cores * num_sockets * sizeof(int),
                    status);
        for (i = 0; i < num_sockets * num_cores; i++)
            bind_info.user_bind_map[i] = -1;

        bind_info.user_bind_valid = 1;
        i = 0;
        str = strtok(user_bind_map, ",");
        do {
            if (!str || i >= num_cores * num_sockets)
                break;
            bind_info.user_bind_map[i++] = atoi(str);
            fflush(stdout);
            str = strtok(NULL, ",");
        } while (1);
    }

    bind_info.supported = 1;
    bind_info.num_procs = num_procs;
    bind_info.num_sockets = num_sockets;
    bind_info.num_cores = num_cores;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

void HYDU_bind_process(int core)
{
    int ret;
    PLPA_NAME(cpu_set_t) cpuset;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (bind_info.supported) {
        PLPA_NAME_CAPS(CPU_ZERO) (&cpuset);
        PLPA_NAME_CAPS(CPU_SET) (core % bind_info.num_procs, &cpuset);
        ret = PLPA_NAME(sched_setaffinity) (0, 1, &cpuset);
        if (ret)
            HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "plpa setaffinity failed\n");
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return;

  fn_fail:
    goto fn_exit;
}


int HYDU_bind_get_core_id(int id, HYD_Binding binding)
{
    int socket = 0, core = 0, curid, realid;

    HYDU_FUNC_ENTER();

    if (bind_info.supported) {
        realid = (id % (bind_info.num_cores * bind_info.num_sockets));

        if (binding == HYD_BIND_RR) {
            return realid;
        }
        else if (binding == HYD_BIND_BUDDY) {
            /* If we reached the maximum, loop around */
            curid = 0;
            for (core = 0; core < bind_info.num_cores; core++) {
                for (socket = 0; socket < bind_info.num_sockets; socket++) {
                    if (curid == realid)
                        return bind_info.bind_map[socket][core];
                    curid++;
                }
            }
        }
        else if (binding == HYD_BIND_PACK) {
            curid = 0;
            for (socket = 0; socket < bind_info.num_sockets; socket++) {
                for (core = 0; core < bind_info.num_cores; core++) {
                    if (curid == realid)
                        return bind_info.bind_map[socket][core];
                    curid++;
                }
            }
        }
        else if (binding == HYD_BIND_NONE) {
            return -1;
        }
        else if (binding == HYD_BIND_USER) {
            if (!bind_info.user_bind_valid)
                return -1;
            else
                return bind_info.user_bind_map[realid];
        }
    }
    else {
        HYDU_Error_printf("Process-core binding is not supported on this platform\n");
    }

    HYDU_FUNC_EXIT();
    return -1;
}

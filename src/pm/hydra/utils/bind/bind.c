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
} bind_info = { 0, -1, -1, -1 , NULL };

HYD_Status HYDU_bind_init(void)
{
    PLPA_NAME(api_type_t) p;
    int ret, supported, i, j;
    int num_procs, max_proc_id;
    int num_sockets = -1, max_socket_id;
    int num_cores = -1, max_core_id;
    int socket, core;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (!((PLPA_NAME(api_probe)(&p) == 0) && (p == PLPA_NAME_CAPS(PROBE_OK)))) {
        /* If this failed, we just return without binding */
        HYDU_Warn_printf("plpa api probe failed; not binding\n");
        goto fn_exit;
    }

    /* We need topology information too */
    ret = PLPA_NAME(have_topology_information)(&supported);
    if ((ret == 0) && (supported == 1)) {
        /* Find the maximum number of processing elements */
        ret = PLPA_NAME(get_processor_data)(PLPA_NAME_CAPS(COUNT_ALL), &num_procs,
                                            &max_proc_id);
        if (ret) {
            HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                                "plpa get processor data failed\n");
        }

        /* PLPA only gives information about sockets and cores */
        ret = PLPA_NAME(get_socket_info)(&num_sockets, &max_socket_id);
        if (ret) {
            HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                                "plpa get processor data failed\n");
        }

        ret = PLPA_NAME(get_core_info)(0, &num_cores, &max_core_id);
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
            ret = PLPA_NAME(map_to_socket_core)(i, &socket, &core);
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
        PLPA_NAME_CAPS(CPU_ZERO)(&cpuset);
        PLPA_NAME_CAPS(CPU_SET)(core % bind_info.num_procs, &cpuset);
        ret = PLPA_NAME(sched_setaffinity)(0, 1, &cpuset);
        if (ret)
            HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "plpa setaffinity failed\n");
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return;

  fn_fail:
    goto fn_exit;
}


int HYDU_next_core(int old_core, HYD_Binding binding)
{
    int socket, core, proc;
    int ret;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (bind_info.supported) {
        if (binding == HYD_BIND_RR) {
            /* Round-robin is easy; just give the next core */
            return ((old_core + 1) % (bind_info.num_sockets * bind_info.num_cores));
        }
        else if (binding == HYD_BIND_BUDDY) {
            if (old_core == -1)
                return 0;

            /* First find the old core */
            for (core = 0; core < bind_info.num_cores; core++) {
                for (socket = 0; socket < bind_info.num_sockets; socket++) {
                    if (bind_info.bind_map[socket][core] == old_core)
                        break;
                }
                if (bind_info.bind_map[socket][core] == old_core)
                    break;
            }

            /* If there is another socket available after this, give
             * the same core ID on that socket */
            if (socket < bind_info.num_sockets - 1)
                return bind_info.bind_map[socket+1][core];
            /* If we are the last socket, and there is a core left
             * after ours, give that core on the first socket */
            else if (core < bind_info.num_cores - 1)
                return bind_info.bind_map[0][core+1];
            /* If we are the last socket and last core, loop back to
             * the start */
            else
                return bind_info.bind_map[0][0];
        }
        else if (binding == HYD_BIND_PACK) {
            if (old_core == -1)
                return 0;

            /* First find the old core */
            for (core = 0; core < bind_info.num_cores; core++) {
                for (socket = 0; socket < bind_info.num_sockets; socket++) {
                    if (bind_info.bind_map[socket][core] == old_core)
                        break;
                }
                if (bind_info.bind_map[socket][core] == old_core)
                    break;
            }

            /* If there is another core available after this, give
             * that core ID on the same socket */
            if (core < bind_info.num_cores - 1)
                return bind_info.bind_map[socket][core+1];
            /* If we are the last core, and there is a socket left
             * after ours, give the first core on that socket */
            else if (socket < bind_info.num_sockets - 1)
                return bind_info.bind_map[socket+1][0];
            /* If we are the last socket and last core, loop back to
             * the start */
            else
                return bind_info.bind_map[0][0];
        }
        else if (binding == HYD_BIND_NONE) {
            return -1;
        }
        else if (binding == HYD_BIND_USER) {
            HYDU_Error_printf("User-specified binding is not supported yet\n");
            return -1;
        }
    }
    else {
        HYDU_Error_printf("Process-core binding is not supported on this platform\n");
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return -1;

  fn_fail:
    goto fn_exit;
}

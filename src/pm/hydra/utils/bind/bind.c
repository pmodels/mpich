/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_utils.h"

struct HYDU_bind_info {
    int supported;
    int num_procs;
    int num_sockets;
    int num_cores;
};

static struct HYDU_bind_info HYDU_bind_info = { 0, -1, -1, -1 };

HYD_Status HYDU_bind_init(void)
{
    PLPA_NAME(api_type_t) p;
    int ret, supported;
    int num_procs, max_proc_id;
    int num_sockets = -1, max_socket_id;
    int num_cores = -1, max_core_id;
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
    }
    else {
        /* If this failed, we just return without binding */
        HYDU_Warn_printf("plpa unable to get topology information; not binding\n");
        goto fn_exit;
    }

    HYDU_bind_info.supported = 1;
    HYDU_bind_info.num_procs = num_procs;
    HYDU_bind_info.num_sockets = num_sockets;
    HYDU_bind_info.num_cores = num_cores;

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

    if (HYDU_bind_info.supported) {
        PLPA_NAME_CAPS(CPU_ZERO)(&cpuset);
        PLPA_NAME_CAPS(CPU_SET)(core % HYDU_bind_info.num_procs, &cpuset);
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
    int ret, new_core = -1, found;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* Round-robin is easy; just give the next core */
    if (HYDU_bind_info.supported) {
        if (binding == HYD_BIND_RR) {
            return (old_core + 1);
        }
        else if (binding == HYD_BIND_BUDDY) {
            found = 0;
            for (core = 0; core < HYDU_bind_info.num_cores; core++)
                for (socket = 0; socket < HYDU_bind_info.num_sockets; socket++) {
                    ret = PLPA_NAME(map_to_processor_id)(socket, core, &proc);
                    if (ret)
                        HYDU_ERR_SETANDJUMP2(status, HYD_INTERNAL_ERROR,
                                             "plpa map_to_proc_id failed (%d,%d)\n",
                                             socket, core);

                    if (found)
                        return proc;
                    else if (proc != core)
                        continue;
                    else
                        found = 1;
                }

            return -1;
        }
        else if (binding == HYD_BIND_PACK) {
            found = 0;
            for (socket = 0; socket < HYDU_bind_info.num_sockets; socket++) {
                for (core = 0; core < HYDU_bind_info.num_cores; core++)
                    ret = PLPA_NAME(map_to_processor_id)(socket, core, &proc);
                    if (ret)
                        HYDU_ERR_SETANDJUMP2(status, HYD_INTERNAL_ERROR,
                                             "plpa map_to_proc_id failed (%d,%d)\n",
                                             socket, core);

                    if (found)
                        return proc;
                    else if (proc != core)
                        continue;
                    else
                        found = 1;
                }

            return -1;
        }
        else
            return -1;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return new_core;

  fn_fail:
    new_core = -1;
    goto fn_exit;
}

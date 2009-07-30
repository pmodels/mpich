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

HYD_Status HYDU_bind_plpa_init(char *user_bind_map, HYDU_bind_support_level_t *support_level)
{
    PLPA_NAME(api_type_t) p;
    int ret, i, j, max, id;
    int processor, socket, core, thread;
    char *str;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (!((PLPA_NAME(api_probe) (&p) == 0) && (p == PLPA_NAME_CAPS(PROBE_OK)))) {
        /* If this failed, we just return without binding */
        HYDU_Warn_printf("plpa api runtime probe failed\n");
        goto fn_fail;
    }

    /* Find the maximum number of processing elements */
    ret = PLPA_NAME(get_processor_data) (PLPA_NAME_CAPS(COUNT_ONLINE),
                                         &HYDU_bind_info.num_procs, &max);
    if (ret) {
        /* Unable to get number of processors */
        HYDU_Warn_printf("plpa get processor data failed\n");
        goto fn_fail;
    }

    HYDU_MALLOC(HYDU_bind_info.bind_map, HYDU_bind_map_t *,
                HYDU_bind_info.num_procs * sizeof(HYDU_bind_map_t), status);
    for (i = 0; i < HYDU_bind_info.num_procs; i++) {
        HYDU_bind_info.bind_map[i].processor_id = -1;
        HYDU_bind_info.bind_map[i].socket_rank = -1;
        HYDU_bind_info.bind_map[i].socket_id = -1;
        HYDU_bind_info.bind_map[i].core_rank = -1;
        HYDU_bind_info.bind_map[i].core_id = -1;
        HYDU_bind_info.bind_map[i].thread_rank = -1;
        HYDU_bind_info.bind_map[i].thread_id = -1;
    }

    for (i = 0; i < HYDU_bind_info.num_procs; i++) {
        ret = PLPA_NAME(get_processor_id) (i, PLPA_NAME_CAPS(COUNT_ALL), &processor);
        if (ret) {
            /* Unable to get processor ID */
            HYDU_Warn_printf("plpa get processor id failed\n");
            if (HYDU_bind_info.bind_map)
                HYDU_FREE(HYDU_bind_info.bind_map);
            goto fn_fail;
        }
        HYDU_bind_info.bind_map[i].processor_id = processor;
    }

    /* We have qualified for basic binding support level */
    *support_level = HYDU_BIND_BASIC;

    /* If the user is specifying the binding, we just use it */
    if (user_bind_map) {
        HYDU_MALLOC(HYDU_bind_info.user_bind_map, int *,
                    HYDU_bind_info.num_procs * sizeof(int), status);
        for (i = 0; i < HYDU_bind_info.num_procs; i++)
            HYDU_bind_info.user_bind_map[i] = -1;

        HYDU_bind_info.user_bind_valid = 1;
        i = 0;
        str = strtok(user_bind_map, ",");
        do {
            if (!str || i >= HYDU_bind_info.num_procs)
                break;
            HYDU_bind_info.user_bind_map[i++] = atoi(str);
            str = strtok(NULL, ",");
        } while (1);

        /* For user binding, we don't need to get the topology
         * information */
        goto fn_exit;
    }

    /* PLPA only gives information about sockets and cores */
    ret = PLPA_NAME(get_socket_info) (&HYDU_bind_info.num_sockets, &max);
    if (ret) {
        /* Unable to get number of sockets */
        HYDU_Warn_printf("plpa get socket info failed\n");
        goto fn_fail;
    }

    ret = PLPA_NAME(get_core_info) (0, &HYDU_bind_info.num_cores, &max);
    if (ret) {
        /* Unable to get number of cores */
        HYDU_Warn_printf("plpa get core info failed\n");
        goto fn_fail;
    }

    HYDU_bind_info.num_threads = HYDU_bind_info.num_procs /
        (HYDU_bind_info.num_sockets * HYDU_bind_info.num_cores);

    /* Find the socket and core IDs for all processor IDs */
    for (i = 0; i < HYDU_bind_info.num_procs; i++) {
        ret = PLPA_NAME(map_to_socket_core) (HYDU_bind_info.bind_map[i].processor_id,
                                             &socket, &core);
        if (ret) {
            /* Unable to get number of cores */
            HYDU_Warn_printf("plpa unable to map socket to core\n");
            goto fn_fail;
        }

        HYDU_bind_info.bind_map[i].socket_id = socket;
        HYDU_bind_info.bind_map[i].core_id = core;

        thread = -1;
        for (j = 0; j < i; j++)
            if (HYDU_bind_info.bind_map[j].socket_id == socket &&
                HYDU_bind_info.bind_map[j].core_id == core)
                thread = HYDU_bind_info.bind_map[j].thread_id;
        thread++;

        HYDU_bind_info.bind_map[i].thread_id = thread;
        HYDU_bind_info.bind_map[i].thread_rank = thread;
    }

    /* Find the rank of each socket ID */
    for (i = 0; i < HYDU_bind_info.num_sockets; i++) {
        ret = PLPA_NAME(get_socket_id) (i, &id);
        if (ret) {
            /* Unable to get socket id */
            HYDU_Warn_printf("plpa unable to get socket id\n");
            goto fn_fail;
        }
        for (j = 0; j < HYDU_bind_info.num_procs; j++)
            if (HYDU_bind_info.bind_map[j].socket_id == id)
                HYDU_bind_info.bind_map[j].socket_rank = i;
    }

    /* Find the rank of each core ID */
    for (i = 0; i < HYDU_bind_info.num_cores; i++) {
        ret = PLPA_NAME(get_core_id) (HYDU_bind_info.bind_map[0].socket_id, i, &id);
        if (ret) {
            /* Unable to get socket id */
            HYDU_Warn_printf("plpa unable to get socket id\n");
            goto fn_fail;
        }
        for (j = 0; j < HYDU_bind_info.num_procs; j++)
            if (HYDU_bind_info.bind_map[j].core_id == id)
                HYDU_bind_info.bind_map[j].core_rank = i;
    }

    /* We have qualified for topology-aware binding support level */
    *support_level = HYDU_BIND_TOPO;

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

    /* If the specified core is negative, we just ignore it */
    if (core < 0)
        goto fn_exit;

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

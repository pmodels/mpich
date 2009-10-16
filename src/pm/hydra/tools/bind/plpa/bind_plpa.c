/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_utils.h"
#include "bind.h"
#include "bind_plpa.h"

struct HYDT_bind_info HYDT_bind_info;

#include "plpa.h"
#include "plpa_internal.h"

HYD_status HYDT_bind_plpa_init(HYDT_bind_support_level_t * support_level)
{
    PLPA_NAME(api_type_t) p;
    int ret, i, j, max, id;
    int processor, sock, core, thread;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (!((PLPA_NAME(api_probe) (&p) == 0) && (p == PLPA_NAME_CAPS(PROBE_OK)))) {
        /* If this failed, we just return without binding */
        HYDU_warn_printf("plpa api runtime probe failed\n");
        goto fn_fail;
    }

    /* Find the maximum number of processing elements */
    ret = PLPA_NAME(get_processor_data) (PLPA_NAME_CAPS(COUNT_ONLINE),
                                         &HYDT_bind_info.num_procs, &max);
    if (ret) {
        /* Unable to get number of processors */
        HYDU_warn_printf("plpa get processor data failed\n");
        goto fn_fail;
    }

    HYDU_MALLOC(HYDT_bind_info.topology, struct HYDT_topology *,
                HYDT_bind_info.num_procs * sizeof(struct HYDT_topology), status);
    for (i = 0; i < HYDT_bind_info.num_procs; i++) {
        HYDT_bind_info.topology[i].processor_id = -1;
        HYDT_bind_info.topology[i].socket_rank = -1;
        HYDT_bind_info.topology[i].socket_id = -1;
        HYDT_bind_info.topology[i].core_rank = -1;
        HYDT_bind_info.topology[i].core_id = -1;
        HYDT_bind_info.topology[i].thread_rank = -1;
        HYDT_bind_info.topology[i].thread_id = -1;
    }

    for (i = 0; i < HYDT_bind_info.num_procs; i++) {
        ret = PLPA_NAME(get_processor_id) (i, PLPA_NAME_CAPS(COUNT_ALL), &processor);
        if (ret) {
            /* Unable to get processor ID */
            HYDU_warn_printf("plpa get processor id failed\n");
            if (HYDT_bind_info.topology)
                HYDU_FREE(HYDT_bind_info.topology);
            goto fn_fail;
        }
        HYDT_bind_info.topology[i].processor_id = processor;
    }

    /* We have qualified for basic binding support level */
    *support_level = HYDT_BIND_BASIC;

    /* PLPA only gives information about sockets and cores */
    ret = PLPA_NAME(get_socket_info) (&HYDT_bind_info.num_sockets, &max);
    if (ret) {
        /* Unable to get number of sockets */
        HYDU_warn_printf("plpa get socket info failed\n");
        goto fn_fail;
    }

    ret = PLPA_NAME(get_core_info) (0, &HYDT_bind_info.num_cores, &max);
    if (ret) {
        /* Unable to get number of cores */
        HYDU_warn_printf("plpa get core info failed\n");
        goto fn_fail;
    }

    HYDT_bind_info.num_threads = HYDT_bind_info.num_procs /
        (HYDT_bind_info.num_sockets * HYDT_bind_info.num_cores);

    /* Find the socket and core IDs for all processor IDs */
    for (i = 0; i < HYDT_bind_info.num_procs; i++) {
        ret = PLPA_NAME(map_to_socket_core) (HYDT_bind_info.topology[i].processor_id,
                                             &sock, &core);
        if (ret) {
            /* Unable to get number of cores */
            HYDU_warn_printf("plpa unable to map socket to core\n");
            goto fn_fail;
        }

        HYDT_bind_info.topology[i].socket_id = sock;
        HYDT_bind_info.topology[i].core_id = core;

        thread = -1;
        for (j = 0; j < i; j++)
            if (HYDT_bind_info.topology[j].socket_id == sock &&
                HYDT_bind_info.topology[j].core_id == core)
                thread = HYDT_bind_info.topology[j].thread_id;
        thread++;

        HYDT_bind_info.topology[i].thread_id = thread;
        HYDT_bind_info.topology[i].thread_rank = thread;
    }

    /* Find the rank of each socket ID */
    for (i = 0; i < HYDT_bind_info.num_sockets; i++) {
        ret = PLPA_NAME(get_socket_id) (i, &id);
        if (ret) {
            /* Unable to get socket id */
            HYDU_warn_printf("plpa unable to get socket id\n");
            goto fn_fail;
        }
        for (j = 0; j < HYDT_bind_info.num_procs; j++)
            if (HYDT_bind_info.topology[j].socket_id == id)
                HYDT_bind_info.topology[j].socket_rank = i;
    }

    /* Find the rank of each core ID */
    for (i = 0; i < HYDT_bind_info.num_cores; i++) {
        ret = PLPA_NAME(get_core_id) (HYDT_bind_info.topology[0].socket_id, i, &id);
        if (ret) {
            /* Unable to get socket id */
            HYDU_warn_printf("plpa unable to get socket id\n");
            goto fn_fail;
        }
        for (j = 0; j < HYDT_bind_info.num_procs; j++)
            if (HYDT_bind_info.topology[j].core_id == id)
                HYDT_bind_info.topology[j].core_rank = i;
    }

    /* We have qualified for topology-aware binding support level */
    *support_level = HYDT_BIND_TOPO;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYDT_bind_plpa_process(int core)
{
    int ret;
    PLPA_NAME(cpu_set_t) cpuset;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* If the specified core is negative, we just ignore it */
    if (core < 0)
        goto fn_exit;

    PLPA_NAME_CAPS(CPU_ZERO) (&cpuset);
    PLPA_NAME_CAPS(CPU_SET) (core % HYDT_bind_info.num_procs, &cpuset);
    ret = PLPA_NAME(sched_setaffinity) (0, 1, &cpuset);
    if (ret)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "plpa setaffinity failed\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

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
    int ret, i, j, k, proc_id, socket_id, core_id, max, total_cores;
    struct HYDT_bind_obj *node, *sock, *core, *thread;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (!((PLPA_NAME(api_probe) (&p) == 0) && (p == PLPA_NAME_CAPS(PROBE_OK)))) {
        /* If this failed, we just return without binding */
        HYDU_warn_printf("plpa api runtime probe failed\n");
        goto fn_fail;
    }

    /* Find the maximum number of processing elements */
    ret = PLPA_NAME(get_processor_data) (PLPA_NAME_CAPS(COUNT_ONLINE),
                                         &HYDT_bind_info.total_proc_units, &max);
    if (ret) {
        /* Unable to get number of processors */
        HYDU_warn_printf("plpa get processor data failed\n");
        goto fn_fail;
    }

    /* We have qualified for basic binding support level */
    *support_level = HYDT_BIND_SUPPORT_BASIC;

    /* Setup the machine level */
    HYDT_bind_info.machine.type = HYDT_BIND_OBJ_MACHINE;
    HYDT_bind_info.machine.os_index = -1;       /* This is a set, not a single unit */
    HYDT_bind_info.machine.parent = NULL;
    HYDT_bind_info.machine.num_children = 1;
    HYDU_MALLOC(HYDT_bind_info.machine.children, struct HYDT_bind_obj *,
                sizeof(struct HYDT_bind_obj), status);

    /* Setup the node level */
    node = &HYDT_bind_info.machine.children[0];
    node->type = HYDT_BIND_OBJ_NODE;
    node->os_index = -1;
    node->parent = &HYDT_bind_info.machine;
    ret = PLPA_NAME(get_socket_info) (&node->num_children, &max);
    if (ret) {
        HYDU_warn_printf("plpa get socket info failed\n");
        goto fn_fail;
    }
    HYDU_MALLOC(node->children, struct HYDT_bind_obj *,
                sizeof(struct HYDT_bind_obj) * node->num_children, status);

    /* Setup the socket level */
    total_cores = 0;
    for (i = 0; i < node->num_children; i++) {
        sock = &node->children[i];
        sock->type = HYDT_BIND_OBJ_SOCKET;
        sock->os_index = -1;
        sock->parent = node;

        ret = PLPA_NAME(get_socket_id) (i, &socket_id);
        if (ret) {
            HYDU_warn_printf("plpa get socket id failed\n");
            goto fn_fail;
        }

        ret = PLPA_NAME(get_core_info) (socket_id, &sock->num_children, &max);
        if (ret) {
            HYDU_warn_printf("plpa get core info failed\n");
            goto fn_fail;
        }
        HYDU_MALLOC(sock->children, struct HYDT_bind_obj *,
                    sizeof(struct HYDT_bind_obj) * sock->num_children, status);

        total_cores += sock->num_children;
    }

    if (HYDT_bind_info.total_proc_units % total_cores) {
        HYDU_warn_printf("total processing units is not a multiple of total cores\n");
        goto fn_fail;
    }

    /* Core level objects */
    for (i = 0; i < node->num_children; i++) {
        sock = &node->children[i];

        for (j = 0; j < sock->num_children; j++) {
            core = &sock->children[j];
            core->type = HYDT_BIND_OBJ_CORE;
            core->os_index = -1;
            core->parent = sock;
            core->num_children = HYDT_bind_info.total_proc_units / total_cores;
            HYDU_MALLOC(core->children, struct HYDT_bind_obj *,
                        sizeof(struct HYDT_bind_obj) * core->num_children, status);

            for (k = 0; k < core->num_children; k++) {
                thread = &core->children[k];
                thread->type = HYDT_BIND_OBJ_THREAD;
                thread->os_index = -1;
                thread->parent = core;
                thread->num_children = 0;
                thread->children = NULL;
            }
        }
    }

    for (i = 0; i < HYDT_bind_info.total_proc_units; i++) {
        ret = PLPA_NAME(get_processor_id) (i, PLPA_NAME_CAPS(COUNT_ONLINE), &proc_id);
        if (ret) {
            HYDU_warn_printf("plpa unable to get processor id\n");
            goto fn_fail;
        }

        ret = PLPA_NAME(map_to_socket_core) (proc_id, &socket_id, &core_id);
        if (ret) {
            HYDU_warn_printf("plpa unable to map socket to core\n");
            goto fn_fail;
        }

        for (j = 0; j < HYDT_bind_info.total_proc_units / total_cores; j++) {
            thread = &node->children[socket_id].children[core_id].children[j];
            if (thread->os_index == -1) {
                thread->os_index = i;
                break;
            }
        }
    }

    /* We have qualified for CPU topology-aware binding support level */
    *support_level = HYDT_BIND_SUPPORT_CPUTOPO;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYDT_bind_plpa_process(int os_index)
{
    int ret;
    PLPA_NAME(cpu_set_t) cpuset;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* If the specified os_index is negative, we just ignore it */
    if (os_index < 0)
        goto fn_exit;

    PLPA_NAME_CAPS(CPU_ZERO) (&cpuset);
    PLPA_NAME_CAPS(CPU_SET) (os_index % HYDT_bind_info.total_proc_units, &cpuset);
    ret = PLPA_NAME(sched_setaffinity) (0, 1, &cpuset);
    if (ret)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "plpa setaffinity failed\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

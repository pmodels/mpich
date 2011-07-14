/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "topo.h"
#include "topo_plpa.h"

struct HYDT_topo_info HYDT_topo_info;

#include "plpa.h"

static void set_cpuset_idx(int idx, struct HYDT_topo_obj *obj)
{
    struct HYDT_topo_obj *tmp = obj;

    do {
        HYDT_topo_cpuset_set(idx, &tmp->cpuset);
        tmp = tmp->parent;
    } while (tmp);
}

HYD_status HYDT_topo_plpa_init(HYDT_topo_support_level_t * support_level)
{
    PLPA_NAME(api_type_t) p;
    int ret, i, j, k, proc_id, socket_id, core_id, max, total_cores;
    struct HYDT_topo_obj *node, *sock, *core, *thread;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (!((PLPA_NAME(api_probe) (&p) == 0) && (p == PLPA_NAME_CAPS(PROBE_OK)))) {
        /* If this failed, we just return without initializing the topology */
        HYDU_warn_printf("plpa api runtime probe failed\n");
        goto fn_fail;
    }

    /* Find the maximum number of processing elements */
    ret = PLPA_NAME(get_processor_data) (PLPA_NAME_CAPS(COUNT_ONLINE),
                                         &HYDT_topo_info.total_proc_units, &max);
    if (ret) {
        /* Unable to get number of processors */
        HYDU_warn_printf("plpa get processor data failed\n");
        goto fn_fail;
    }

    /* We have qualified for basic topology support level */
    *support_level = HYDT_TOPO_SUPPORT_BASIC;

    /* Setup the machine level */
    HYDT_topo_info.machine.type = HYDT_TOPO_OBJ_MACHINE;
    HYDT_topo_cpuset_zero(&HYDT_topo_info.machine.cpuset);
    HYDT_topo_info.machine.parent = NULL;
    HYDT_topo_info.machine.num_children = 1;
    status = HYDT_topo_alloc_objs(HYDT_topo_info.machine.num_children,
                                  &HYDT_topo_info.machine.children);
    HYDU_ERR_POP(status, "error allocating topo objects\n");

    /* Setup the node level */
    node = &HYDT_topo_info.machine.children[0];
    node->type = HYDT_TOPO_OBJ_NODE;
    HYDT_topo_cpuset_zero(&node->cpuset);
    node->parent = &HYDT_topo_info.machine;
    ret = PLPA_NAME(get_socket_info) (&node->num_children, &max);
    if (ret) {
        HYDU_warn_printf("plpa get socket info failed\n");
        goto fn_fail;
    }
    status = HYDT_topo_alloc_objs(node->num_children, &node->children);
    HYDU_ERR_POP(status, "error allocating topo objects\n");

    /* Setup the socket level */
    total_cores = 0;
    for (i = 0; i < node->num_children; i++) {
        sock = &node->children[i];
        sock->type = HYDT_TOPO_OBJ_SOCKET;
        HYDT_topo_cpuset_zero(&sock->cpuset);
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
        status = HYDT_topo_alloc_objs(sock->num_children, &sock->children);
        HYDU_ERR_POP(status, "error allocating topo objects\n");

        total_cores += sock->num_children;
    }

    if (HYDT_topo_info.total_proc_units % total_cores) {
        HYDU_warn_printf("total processing units is not a multiple of total cores\n");
        goto fn_fail;
    }

    /* Core level objects */
    for (i = 0; i < node->num_children; i++) {
        sock = &node->children[i];

        for (j = 0; j < sock->num_children; j++) {
            core = &sock->children[j];
            core->type = HYDT_TOPO_OBJ_CORE;
            HYDT_topo_cpuset_zero(&core->cpuset);
            core->parent = sock;
            core->num_children = HYDT_topo_info.total_proc_units / total_cores;
            status = HYDT_topo_alloc_objs(core->num_children, &core->children);
            HYDU_ERR_POP(status, "error allocating topo objects\n");

            for (k = 0; k < core->num_children; k++) {
                thread = &core->children[k];
                thread->type = HYDT_TOPO_OBJ_THREAD;
                HYDT_topo_cpuset_zero(&thread->cpuset);
                thread->parent = core;
                thread->num_children = 0;
                thread->children = NULL;
            }
        }
    }

    for (i = 0; i < HYDT_topo_info.total_proc_units; i++) {
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

        /* We can't distinguish between threads on the same core, so
         * we assign both */
        for (j = 0; j < HYDT_topo_info.total_proc_units / total_cores; j++) {
            thread = &node->children[socket_id].children[core_id].children[j];
            set_cpuset_idx(i, thread);
        }
    }

    /* We have qualified for CPU topology support level */
    *support_level = HYDT_TOPO_SUPPORT_CPUTOPO;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYDT_topo_plpa_bind(struct HYDT_topo_cpuset_t cpuset)
{
    int ret, i;
    int isset = 0;
    PLPA_NAME(cpu_set_t) plpa_cpuset;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    PLPA_NAME_CAPS(CPU_ZERO) (&plpa_cpuset);
    isset = 0;
    for (i = 0; i < HYDT_topo_info.total_proc_units; i++) {
        if (HYDT_topo_cpuset_isset(i, cpuset)) {
            PLPA_NAME_CAPS(CPU_SET) (i, &plpa_cpuset);
            isset = 1;
        }
    }

    if (isset) {
        ret = PLPA_NAME(sched_setaffinity) (0, HYDT_topo_info.total_proc_units, &plpa_cpuset);
        if (ret)
            HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "plpa setaffinity failed\n");
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYDT_topo_plpa_finalize(void)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* FIXME: We do not check for the return value of this function,
     * because it always seems to return an error. But not calling it
     * is causing some resource leaks. */
    PLPA_NAME(finalize) ();

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

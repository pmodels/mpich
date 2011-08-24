/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 * Copyright © 2006-2011 Guillaume Mercier, Institut Polytechnique de
 * Bordeaux. All rights reserved. Permission is hereby granted to use,
 * reproduce, prepare derivative works, and to redistribute to others.
 */

#include "hydra.h"
#include "topo.h"
#include "topo_hwloc.h"

static hwloc_topology_t topology;
static int hwloc_initialized = 0;

#define dprint(d, ...)                           \
    do {                                         \
        int _i;                                  \
        for (_i = 0; _i < d; _i++)               \
            fprintf(stderr, "    ");             \
        fprintf(stderr, __VA_ARGS__);            \
    } while (0)

/* Retained for debugging purposes */
static void print_obj_info(hwloc_obj_t obj) ATTRIBUTE((unused));
static void print_obj_info(hwloc_obj_t obj)
{
    int i;

    if (obj->type == HWLOC_OBJ_CACHE)
        dprint(obj->depth, "[%s] L%u cache size: %lu\n",
               hwloc_obj_type_string(obj->type), obj->attr->cache.depth,
               obj->attr->cache.size);
    else {
        if (obj->memory.total_memory || obj->memory.local_memory)
            dprint(obj->depth, "[%s:%u] total memory: %lu; local memory: %lu\n",
                   hwloc_obj_type_string(obj->type), obj->os_index, obj->memory.total_memory,
                   obj->memory.local_memory);
        else
            dprint(obj->depth, "[%s:%u]\n", hwloc_obj_type_string(obj->type), obj->os_index);
    }

    for (i = 0; i < obj->arity; i++)
        print_obj_info(obj->children[i]);
}

static int hwloc_to_hydra_cpuset_dup(hwloc_cpuset_t hwloc_cpuset,
                                     struct HYDT_topo_cpuset_t *hydra_cpuset)
{
    int i, count = 0;

    HYDT_topo_cpuset_zero(hydra_cpuset);
    for (i = 0; i < HYDT_topo_info.total_proc_units; i++) {
        if (hwloc_cpuset_isset(hwloc_cpuset, i)) {
            HYDT_topo_cpuset_set(i, hydra_cpuset);
            count++;
        }
    }

    return count;
}

static int hydra_to_hwloc_cpuset_dup(struct HYDT_topo_cpuset_t hydra_cpuset,
                                     hwloc_cpuset_t hwloc_cpuset)
{
    int i, count = 0;

    hwloc_cpuset_zero(hwloc_cpuset);
    for (i = 0; i < HYDT_topo_info.total_proc_units; i++) {
        if (HYDT_topo_cpuset_isset(i, hydra_cpuset)) {
            hwloc_cpuset_set(hwloc_cpuset, i);
            count++;
        }
    }

    return count;
}

static int get_cache_nbobjs(hwloc_obj_t hobj, hwloc_cpuset_t cpuset)
{
    int count = 0, i;

    /* count all cache objects which have the target cpuset map */
    if (hobj->type == HWLOC_OBJ_CACHE && !hwloc_cpuset_compare(hobj->cpuset, cpuset))
        count++;

    for (i = 0; i < hobj->arity; i++)
        count += get_cache_nbobjs(hobj->children[i], cpuset);

    return count;
}

static void load_cache_objs(hwloc_obj_t hobj, hwloc_cpuset_t cpuset,
                            struct HYDT_topo_obj *obj, int *idx)
{
    int i;

    if (hobj->type == HWLOC_OBJ_CACHE && !hwloc_cpuset_compare(hobj->cpuset, cpuset)) {
        obj->mem.cache_size[*idx] = hobj->attr->cache.size;
        obj->mem.cache_depth[*idx] = hobj->attr->cache.depth;
        (*idx)++;
    }

    for (i = 0; i < hobj->arity; i++)
        load_cache_objs(hobj->children[i], cpuset, obj, idx);
}

HYD_status HYDT_topo_hwloc_init(HYDT_topo_support_level_t * support_level)
{
    int node, sock, core, thread, idx;

    hwloc_obj_t obj_sys;
    hwloc_obj_t obj_node;
    hwloc_obj_t obj_sock;
    hwloc_obj_t obj_core;
    hwloc_obj_t obj_thread;

    struct HYDT_topo_obj *node_ptr, *sock_ptr, *core_ptr, *thread_ptr;

    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    hwloc_topology_init(&topology);
    hwloc_topology_load(topology);

    hwloc_initialized = 1;

    /* Get the max number of processing elements */
    HYDT_topo_info.total_proc_units = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_PU);

    /* We have qualified for basic topology support level */
    *support_level = HYDT_TOPO_SUPPORT_BASIC;

    /* Setup the machine level */
    obj_sys = hwloc_get_root_obj(topology);

    /* Retained for debugging purposes */
    /* print_obj_info(obj_sys); */

    /* init Hydra structure */
    HYDT_topo_info.machine.type = HYDT_TOPO_OBJ_MACHINE;
    HYDT_topo_cpuset_zero(&HYDT_topo_info.machine.cpuset);
    HYDT_topo_info.machine.parent = NULL;

    HYDT_topo_info.machine.num_children = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_NODE);
    /* If there is no real node, consider there is one */
    if (!HYDT_topo_info.machine.num_children)
        HYDT_topo_info.machine.num_children = 1;
    status = HYDT_topo_alloc_objs(HYDT_topo_info.machine.num_children,
                                  &HYDT_topo_info.machine.children);
    HYDU_ERR_POP(status, "error allocating topo objects\n");

    /* Setup the nodes levels */
    for (node = 0; node < HYDT_topo_info.machine.num_children; node++) {
        node_ptr = &HYDT_topo_info.machine.children[node];
        node_ptr->type = HYDT_TOPO_OBJ_NODE;
        node_ptr->parent = &HYDT_topo_info.machine;
        HYDT_topo_cpuset_zero(&node_ptr->cpuset);

        if (!(obj_node = hwloc_get_obj_inside_cpuset_by_type(topology, obj_sys->cpuset,
                                                             HWLOC_OBJ_NODE, node)))
            obj_node = obj_sys;

        /* copy the hwloc cpuset to hydra format */
        hwloc_to_hydra_cpuset_dup(obj_node->cpuset, &node_ptr->cpuset);

        /* memory information */
        node_ptr->mem.local_mem_size = obj_node->memory.local_memory;

        /* find the number of cache objects which match my cpuset */
        node_ptr->mem.num_caches = get_cache_nbobjs(obj_sys, obj_node->cpuset);

        /* add the actual cache objects that match my cpuset */
        if (node_ptr->mem.num_caches) {
            HYDU_MALLOC(node_ptr->mem.cache_size, size_t *,
                        node_ptr->mem.num_caches * sizeof(size_t), status);
            HYDU_MALLOC(node_ptr->mem.cache_depth, int *,
                        node_ptr->mem.num_caches * sizeof(int), status);
            idx = 0;
            load_cache_objs(obj_sys, obj_node->cpuset, node_ptr, &idx);
        }

        node_ptr->num_children =
            hwloc_get_nbobjs_inside_cpuset_by_type(topology, obj_node->cpuset,
                                                   HWLOC_OBJ_SOCKET);
        /* In case there is no socket */
        if (!node_ptr->num_children)
            node_ptr->num_children = 1;

        status = HYDT_topo_alloc_objs(node_ptr->num_children, &node_ptr->children);
        HYDU_ERR_POP(status, "error allocating topo objects\n");

        /* Setup the socket level */
        for (sock = 0; sock < node_ptr->num_children; sock++) {
            sock_ptr = &node_ptr->children[sock];
            sock_ptr->type = HYDT_TOPO_OBJ_SOCKET;
            sock_ptr->parent = node_ptr;
            HYDT_topo_cpuset_zero(&sock_ptr->cpuset);

            if (!(obj_sock = hwloc_get_obj_inside_cpuset_by_type(topology, obj_node->cpuset,
                                                                 HWLOC_OBJ_SOCKET, sock)))
                obj_sock = obj_node;

            /* copy the hwloc cpuset to hydra format */
            hwloc_to_hydra_cpuset_dup(obj_sock->cpuset, &sock_ptr->cpuset);

            /* memory information */
            sock_ptr->mem.local_mem_size = obj_sock->memory.local_memory;

            /* find the number of cache objects which match my cpuset */
            sock_ptr->mem.num_caches = get_cache_nbobjs(obj_sys, obj_sock->cpuset);

            /* add the actual cache objects that match my cpuset */
            if (sock_ptr->mem.num_caches) {
                HYDU_MALLOC(sock_ptr->mem.cache_size, size_t *,
                            sock_ptr->mem.num_caches * sizeof(size_t), status);
                HYDU_MALLOC(sock_ptr->mem.cache_depth, int *,
                            sock_ptr->mem.num_caches * sizeof(int), status);
                idx = 0;
                load_cache_objs(obj_sys, obj_sock->cpuset, sock_ptr, &idx);
            }

            sock_ptr->num_children =
                hwloc_get_nbobjs_inside_cpuset_by_type(topology, obj_sock->cpuset,
                                                       HWLOC_OBJ_CORE);

            /* In case there is no core */
            if (!sock_ptr->num_children)
                sock_ptr->num_children = 1;

            status = HYDT_topo_alloc_objs(sock_ptr->num_children, &sock_ptr->children);
            HYDU_ERR_POP(status, "error allocating topo objects\n");

            /* setup the core level */
            for (core = 0; core < sock_ptr->num_children; core++) {
                core_ptr = &sock_ptr->children[core];
                core_ptr->type = HYDT_TOPO_OBJ_CORE;
                core_ptr->parent = sock_ptr;
                HYDT_topo_cpuset_zero(&core_ptr->cpuset);

                if (!(obj_core = hwloc_get_obj_inside_cpuset_by_type(topology,
                                                                     obj_sock->cpuset,
                                                                     HWLOC_OBJ_CORE, core)))
                    obj_core = obj_sock;

                /* copy the hwloc cpuset to hydra format */
                hwloc_to_hydra_cpuset_dup(obj_core->cpuset, &core_ptr->cpuset);

                /* memory information */
                core_ptr->mem.local_mem_size = obj_core->memory.local_memory;

                /* find the number of cache objects which match my cpuset */
                core_ptr->mem.num_caches = get_cache_nbobjs(obj_sys, obj_core->cpuset);

                /* add the actual cache objects that match my cpuset */
                if (core_ptr->mem.num_caches) {
                    HYDU_MALLOC(core_ptr->mem.cache_size, size_t *,
                                core_ptr->mem.num_caches * sizeof(size_t), status);
                    HYDU_MALLOC(core_ptr->mem.cache_depth, int *,
                                core_ptr->mem.num_caches * sizeof(int), status);
                    idx = 0;
                    load_cache_objs(obj_sys, obj_core->cpuset, core_ptr, &idx);
                }

                core_ptr->num_children =
                    hwloc_get_nbobjs_inside_cpuset_by_type(topology, obj_core->cpuset,
                                                           HWLOC_OBJ_PU);

                /* In case there is no thread */
                if (!core_ptr->num_children)
                    core_ptr->num_children = 1;

                status = HYDT_topo_alloc_objs(core_ptr->num_children, &core_ptr->children);
                HYDU_ERR_POP(status, "error allocating topo objects\n");

                /* setup the thread level */
                for (thread = 0; thread < core_ptr->num_children; thread++) {
                    thread_ptr = &core_ptr->children[thread];
                    thread_ptr->type = HYDT_TOPO_OBJ_THREAD;
                    thread_ptr->parent = core_ptr;
                    thread_ptr->num_children = 0;
                    thread_ptr->children = NULL;
                    HYDT_topo_cpuset_zero(&thread_ptr->cpuset);

                    if (!(obj_thread =
                          hwloc_get_obj_inside_cpuset_by_type(topology, obj_core->cpuset,
                                                              HWLOC_OBJ_PU, thread)))
                        HYDU_ERR_POP(status, "unable to detect processing units\n");

                    /* copy the hwloc cpuset to hydra format */
                    hwloc_to_hydra_cpuset_dup(obj_thread->cpuset, &thread_ptr->cpuset);

                    /* memory information */
                    thread_ptr->mem.local_mem_size = obj_thread->memory.local_memory;

                    /* find the number of cache objects which match my cpuset */
                    thread_ptr->mem.num_caches = get_cache_nbobjs(obj_sys, obj_thread->cpuset);

                    /* add the actual cache objects that match my cpuset */
                    if (thread_ptr->mem.num_caches) {
                        HYDU_MALLOC(thread_ptr->mem.cache_size, size_t *,
                                    thread_ptr->mem.num_caches * sizeof(size_t), status);
                        HYDU_MALLOC(thread_ptr->mem.cache_depth, int *,
                                    thread_ptr->mem.num_caches * sizeof(int), status);
                        idx = 0;
                        load_cache_objs(obj_sys, obj_thread->cpuset, thread_ptr, &idx);
                    }
                }
            }
        }
    }

    /* We have qualified for memory topology support level */
    *support_level = HYDT_TOPO_SUPPORT_MEMTOPO;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYDT_topo_hwloc_bind(struct HYDT_topo_cpuset_t cpuset)
{
    hwloc_cpuset_t hwloc_cpuset = hwloc_cpuset_alloc();
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (hydra_to_hwloc_cpuset_dup(cpuset, hwloc_cpuset))
        hwloc_set_cpubind(topology, hwloc_cpuset, HWLOC_CPUBIND_THREAD);

    hwloc_cpuset_free(hwloc_cpuset);
    HYDU_FUNC_EXIT();
    return status;
}

HYD_status HYDT_topo_hwloc_finalize(void)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (hwloc_initialized)
        hwloc_topology_destroy(topology);

    HYDU_FUNC_EXIT();
    return status;
}

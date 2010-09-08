/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_utils.h"
#include "bind.h"
#include "bind_hwloc.h"

static hwloc_topology_t topology;

#define dprint(d, ...)                          \
    do {                                        \
        int _i;                                 \
        for (_i = 0; _i < d; _i++)              \
            printf("    ");                     \
        printf(__VA_ARGS__);                    \
    } while (0)

#define OBJ_TYPE(t)                             \
    (t == HWLOC_OBJ_SYSTEM ? "SYSTEM" :         \
     t == HWLOC_OBJ_MACHINE ? "MACHINE" :       \
     t == HWLOC_OBJ_NODE ? "NODE" :             \
     t == HWLOC_OBJ_SOCKET ? "SOCKET" :         \
     t == HWLOC_OBJ_CACHE ? "CACHE" :           \
     t == HWLOC_OBJ_CORE ? "CORE" :             \
     t == HWLOC_OBJ_PU ? "THREAD" : "UNKNOWN")

static void print_obj_info(hwloc_obj_t obj) ATTRIBUTE((unused));
static void print_obj_info(hwloc_obj_t obj)
{
    int i;

    if (obj->type == HWLOC_OBJ_CACHE)
        dprint(obj->depth, "[%s] L%u cache size: %lu\n",
               OBJ_TYPE(obj->type), obj->attr->cache.depth, obj->attr->cache.size);
    else {
        if (obj->memory.total_memory || obj->memory.local_memory)
            dprint(obj->depth, "[%s] total memory: %lu; local memory: %lu\n",
                   OBJ_TYPE(obj->type), obj->memory.total_memory, obj->memory.local_memory);
        else
            dprint(obj->depth, "[%s]\n", OBJ_TYPE(obj->type));
    }

    for (i = 0; i < obj->arity; i++)
        print_obj_info(obj->children[i]);
}

static inline void load_mem_cache_info(struct HYDT_topo_obj *obj, hwloc_obj_t hobj)
{
    static int listed_depth = 0;
    static int logical_index = -1;

    if (hobj)
        obj->local_mem_size = hobj->memory.local_memory;

    if (hobj && hobj->arity == 1 && hobj->children[0]->type == HWLOC_OBJ_CACHE) {
        obj->cache_size = hobj->children[0]->attr->cache.size;
        obj->cache_depth = hobj->children[0]->attr->cache.depth;
    }
    else if (hobj && hobj->parent && hobj->parent->arity == 1 &&
             hobj->parent->type == HWLOC_OBJ_CACHE &&
             (listed_depth == 0 || hobj->parent->attr->cache.depth < listed_depth) &&
             (logical_index == -1 || hobj->parent->logical_index != logical_index)) {
        obj->cache_size = hobj->parent->attr->cache.size;
        obj->cache_depth = hobj->parent->attr->cache.depth;
        listed_depth = obj->cache_depth;
        logical_index = hobj->parent->logical_index;
    }
    else {
        obj->cache_size = 0;
        obj->cache_depth = 0;
    }
}

HYD_status HYDT_bind_hwloc_init(HYDT_bind_support_level_t * support_level)
{
    int node, sock, core, thread;

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

    /* Get the max number of processing elements */
    HYDT_bind_info.total_proc_units = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_PU);

    /* We have qualified for basic binding support level */
    *support_level = HYDT_BIND_BASIC;

    /* Setup the machine level */
    obj_sys = hwloc_get_root_obj(topology);

    /* init Hydra structure */
    HYDT_bind_info.machine.type = HYDT_OBJ_MACHINE;
    HYDT_bind_info.machine.os_index = -1;       /* This is a set, not a single unit */
    HYDT_bind_info.machine.parent = NULL;
    load_mem_cache_info(&HYDT_bind_info.machine, obj_sys);

    /* There is no real node, consider there is one */
    HYDT_bind_info.machine.num_children = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_NODE);
    if (!HYDT_bind_info.machine.num_children)
        HYDT_bind_info.machine.num_children = 1;
    HYDU_MALLOC(HYDT_bind_info.machine.children, struct HYDT_topo_obj *,
                sizeof(struct HYDT_topo_obj) * HYDT_bind_info.machine.num_children, status);

    /* Setup the nodes levels */
    for (node = 0; node < HYDT_bind_info.machine.num_children; node++) {
        node_ptr = &HYDT_bind_info.machine.children[node];
        node_ptr->type = HYDT_OBJ_NODE;
        node_ptr->parent = &HYDT_bind_info.machine;

        obj_node = hwloc_get_obj_inside_cpuset_by_type(topology, obj_sys->cpuset,
                                                       HWLOC_OBJ_NODE, node);

        load_mem_cache_info(node_ptr, obj_node);

        if (!obj_node)
            obj_node = obj_sys;
        node_ptr->os_index = obj_node->os_index;
        node_ptr->num_children =
            hwloc_get_nbobjs_inside_cpuset_by_type(topology, obj_node->cpuset,
                                                   HWLOC_OBJ_SOCKET);
        /* In case there is no socket */
        if (!node_ptr->num_children)
            node_ptr->num_children = 1;

        HYDU_MALLOC(node_ptr->children, struct HYDT_topo_obj *,
                    sizeof(struct HYDT_topo_obj) * node_ptr->num_children, status);

        /* Setup the socket level */
        for (sock = 0; sock < node_ptr->num_children; sock++) {
            sock_ptr = &node_ptr->children[sock];
            sock_ptr->type = HYDT_OBJ_SOCKET;
            sock_ptr->parent = node_ptr;

            obj_sock = hwloc_get_obj_inside_cpuset_by_type(topology, obj_node->cpuset,
                                                           HWLOC_OBJ_SOCKET, sock);

            load_mem_cache_info(sock_ptr, obj_sock);

            if (!obj_sock)
                obj_sock = obj_node;

            sock_ptr->os_index = obj_sock->os_index;
            sock_ptr->num_children =
                hwloc_get_nbobjs_inside_cpuset_by_type(topology, obj_sock->cpuset,
                                                       HWLOC_OBJ_CORE);

            /* In case there is no core */
            if (!sock_ptr->num_children)
                sock_ptr->num_children = 1;

            HYDU_MALLOC(sock_ptr->children, struct HYDT_topo_obj *,
                        sizeof(struct HYDT_topo_obj) * sock_ptr->num_children, status);

            /* setup the core level */
            for (core = 0; core < sock_ptr->num_children; core++) {
                core_ptr = &sock_ptr->children[core];
                core_ptr->type = HYDT_OBJ_CORE;
                core_ptr->parent = sock_ptr;

                obj_core = hwloc_get_obj_inside_cpuset_by_type(topology,
                                                               obj_sock->cpuset,
                                                               HWLOC_OBJ_CORE, core);

                load_mem_cache_info(core_ptr, obj_core);

                if (!obj_core)
                    obj_core = obj_sock;

                core_ptr->os_index = obj_core->os_index;
                core_ptr->num_children =
                    hwloc_get_nbobjs_inside_cpuset_by_type(topology, obj_core->cpuset,
                                                           HWLOC_OBJ_PU);

                /* In case there is no thread */
                if (!core_ptr->num_children)
                    core_ptr->num_children = 1;

                HYDU_MALLOC(core_ptr->children, struct HYDT_topo_obj *,
                            sizeof(struct HYDT_topo_obj) * core_ptr->num_children, status);

                /* setup the thread level */
                for (thread = 0; thread < core_ptr->num_children; thread++) {
                    obj_thread = hwloc_get_obj_inside_cpuset_by_type(topology,
                                                                     obj_core->cpuset,
                                                                     HWLOC_OBJ_PU, thread);
                    thread_ptr = &core_ptr->children[thread];
                    thread_ptr->type = HYDT_OBJ_THREAD;
                    thread_ptr->os_index = obj_thread->os_index;
                    thread_ptr->parent = core_ptr;
                    thread_ptr->num_children = 0;
                    thread_ptr->children = NULL;

                    load_mem_cache_info(thread_ptr, obj_thread);
                }
            }
        }
    }

    /* We have qualified for memory topology-aware binding support level */
    *support_level = HYDT_BIND_MEM;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYDT_bind_hwloc_process(int os_index)
{
    hwloc_cpuset_t cpuset = hwloc_cpuset_alloc();

    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    hwloc_cpuset_set(cpuset, os_index);
    hwloc_set_cpubind(topology, cpuset, HWLOC_CPUBIND_THREAD);

  fn_exit:
    hwloc_cpuset_free(cpuset);
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

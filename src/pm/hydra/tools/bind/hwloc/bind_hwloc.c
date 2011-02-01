/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "bind.h"
#include "bind_hwloc.h"

static hwloc_topology_t topology;

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

static int count_attached_caches(hwloc_obj_t hobj, hwloc_cpuset_t cpuset)
{
    int count = 0, i;

    if (hobj->type == HWLOC_OBJ_CACHE && !hwloc_cpuset_compare(hobj->cpuset, cpuset))
        count++;

    for (i = 0; i < hobj->arity; i++)
        count += count_attached_caches(hobj->children[i], cpuset);

    return count;
}

static void gather_attached_caches(struct HYDT_bind_obj *obj, hwloc_obj_t hobj,
                                   hwloc_cpuset_t cpuset, int cindex)
{
    int i;
    int cidx = cindex;

    if (hobj->type == HWLOC_OBJ_CACHE && !hwloc_cpuset_compare(hobj->cpuset, cpuset)) {
        obj->mem.cache_size[cidx] = hobj->attr->cache.size;
        obj->mem.cache_depth[cidx] = hobj->attr->cache.depth;
        cidx++;
    }

    for (i = 0; i < hobj->arity; i++)
        gather_attached_caches(obj, hobj->children[i], cpuset, cidx);
}

static HYD_status load_mem_cache_info(struct HYDT_bind_obj *obj, hwloc_obj_t hobj)
{
    HYD_status status = HYD_SUCCESS;

    if (obj == NULL || hobj == NULL)
        goto fn_exit;

    obj->mem.local_mem_size = hobj->memory.local_memory;

    /* Check how many cache objects match out cpuset */
    obj->mem.num_caches = count_attached_caches(hobj, hobj->cpuset);

    if (obj->mem.num_caches) {
        HYDU_MALLOC(obj->mem.cache_size, size_t *, obj->mem.num_caches * sizeof(size_t),
                    status);
        memset(obj->mem.cache_size, 0, obj->mem.num_caches * sizeof(size_t));
        HYDU_MALLOC(obj->mem.cache_depth, int *, obj->mem.num_caches * sizeof(int), status);
        memset(obj->mem.cache_depth, 0, obj->mem.num_caches * sizeof(int));

        gather_attached_caches(obj, hobj, hobj->cpuset, 0);
    }

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static void set_cpuset_idx(int idx, struct HYDT_bind_obj *obj)
{
    struct HYDT_bind_obj *tmp = obj;

    do {
        HYDT_bind_cpuset_set(idx, &tmp->cpuset);
        tmp = tmp->parent;
    } while (tmp);
}

HYD_status HYDT_bind_hwloc_init(HYDT_bind_support_level_t * support_level)
{
    int node, sock, core, thread;

    hwloc_obj_t obj_sys;
    hwloc_obj_t obj_node;
    hwloc_obj_t obj_sock;
    hwloc_obj_t obj_core;
    hwloc_obj_t obj_thread;

    struct HYDT_bind_obj *node_ptr, *sock_ptr, *core_ptr, *thread_ptr;

    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    hwloc_topology_init(&topology);
    hwloc_topology_load(topology);

    /* Get the max number of processing elements */
    HYDT_bind_info.total_proc_units = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_PU);

    /* We have qualified for basic binding support level */
    *support_level = HYDT_BIND_SUPPORT_BASIC;

    /* Setup the machine level */
    obj_sys = hwloc_get_root_obj(topology);

    /* Retained for debugging purposes */
    /* print_obj_info(obj_sys); */

    /* init Hydra structure */
    HYDT_bind_info.machine.type = HYDT_BIND_OBJ_MACHINE;
    HYDT_bind_cpuset_zero(&HYDT_bind_info.machine.cpuset);
    HYDT_bind_info.machine.parent = NULL;

    status = load_mem_cache_info(&HYDT_bind_info.machine, obj_sys);
    HYDU_ERR_POP(status, "error loading memory/cache info\n");

    /* There is no real node, consider there is one */
    HYDT_bind_info.machine.num_children = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_NODE);
    if (!HYDT_bind_info.machine.num_children)
        HYDT_bind_info.machine.num_children = 1;
    HYDU_MALLOC(HYDT_bind_info.machine.children, struct HYDT_bind_obj *,
                sizeof(struct HYDT_bind_obj) * HYDT_bind_info.machine.num_children, status);

    /* Setup the nodes levels */
    for (node = 0; node < HYDT_bind_info.machine.num_children; node++) {
        node_ptr = &HYDT_bind_info.machine.children[node];
        node_ptr->type = HYDT_BIND_OBJ_NODE;
        node_ptr->parent = &HYDT_bind_info.machine;

        obj_node = hwloc_get_obj_inside_cpuset_by_type(topology, obj_sys->cpuset,
                                                       HWLOC_OBJ_NODE, node);

        status = load_mem_cache_info(node_ptr, obj_node);
        HYDU_ERR_POP(status, "error loading memory/cache info\n");

        if (!obj_node)
            obj_node = obj_sys;
        HYDT_bind_cpuset_zero(&node_ptr->cpuset);
        node_ptr->num_children =
            hwloc_get_nbobjs_inside_cpuset_by_type(topology, obj_node->cpuset,
                                                   HWLOC_OBJ_SOCKET);
        /* In case there is no socket */
        if (!node_ptr->num_children)
            node_ptr->num_children = 1;

        HYDU_MALLOC(node_ptr->children, struct HYDT_bind_obj *,
                    sizeof(struct HYDT_bind_obj) * node_ptr->num_children, status);

        /* Setup the socket level */
        for (sock = 0; sock < node_ptr->num_children; sock++) {
            sock_ptr = &node_ptr->children[sock];
            sock_ptr->type = HYDT_BIND_OBJ_SOCKET;
            sock_ptr->parent = node_ptr;

            obj_sock = hwloc_get_obj_inside_cpuset_by_type(topology, obj_node->cpuset,
                                                           HWLOC_OBJ_SOCKET, sock);

            status = load_mem_cache_info(sock_ptr, obj_sock);
            HYDU_ERR_POP(status, "error loading memory/cache info\n");

            if (!obj_sock)
                obj_sock = obj_node;

            HYDT_bind_cpuset_zero(&sock_ptr->cpuset);
            sock_ptr->num_children =
                hwloc_get_nbobjs_inside_cpuset_by_type(topology, obj_sock->cpuset,
                                                       HWLOC_OBJ_CORE);

            /* In case there is no core */
            if (!sock_ptr->num_children)
                sock_ptr->num_children = 1;

            HYDU_MALLOC(sock_ptr->children, struct HYDT_bind_obj *,
                        sizeof(struct HYDT_bind_obj) * sock_ptr->num_children, status);

            /* setup the core level */
            for (core = 0; core < sock_ptr->num_children; core++) {
                core_ptr = &sock_ptr->children[core];
                core_ptr->type = HYDT_BIND_OBJ_CORE;
                core_ptr->parent = sock_ptr;

                obj_core = hwloc_get_obj_inside_cpuset_by_type(topology,
                                                               obj_sock->cpuset,
                                                               HWLOC_OBJ_CORE, core);

                status = load_mem_cache_info(core_ptr, obj_core);
                HYDU_ERR_POP(status, "error loading memory/cache info\n");

                if (!obj_core)
                    obj_core = obj_sock;

                HYDT_bind_cpuset_zero(&core_ptr->cpuset);
                core_ptr->num_children =
                    hwloc_get_nbobjs_inside_cpuset_by_type(topology, obj_core->cpuset,
                                                           HWLOC_OBJ_PU);

                /* In case there is no thread */
                if (!core_ptr->num_children)
                    core_ptr->num_children = 1;

                HYDU_MALLOC(core_ptr->children, struct HYDT_bind_obj *,
                            sizeof(struct HYDT_bind_obj) * core_ptr->num_children, status);

                /* setup the thread level */
                for (thread = 0; thread < core_ptr->num_children; thread++) {
                    obj_thread = hwloc_get_obj_inside_cpuset_by_type(topology,
                                                                     obj_core->cpuset,
                                                                     HWLOC_OBJ_PU, thread);
                    thread_ptr = &core_ptr->children[thread];
                    thread_ptr->type = HYDT_BIND_OBJ_THREAD;
                    thread_ptr->parent = core_ptr;
                    thread_ptr->num_children = 0;
                    thread_ptr->children = NULL;

                    HYDT_bind_cpuset_zero(&thread_ptr->cpuset);
                    set_cpuset_idx(obj_thread->os_index, thread_ptr);

                    status = load_mem_cache_info(thread_ptr, obj_thread);
                    HYDU_ERR_POP(status, "error loading memory/cache info\n");
                }
            }
        }
    }

    /* We have qualified for memory topology-aware binding support level */
    *support_level = HYDT_BIND_SUPPORT_MEMTOPO;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYDT_bind_hwloc_process(struct HYDT_bind_cpuset_t cpuset)
{
    hwloc_cpuset_t hwloc_cpuset = hwloc_cpuset_alloc();
    int i, count = 0;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    hwloc_cpuset_zero(hwloc_cpuset);
    for (i = 0; i < HYDT_bind_info.total_proc_units; i++) {
        if (HYDT_bind_cpuset_isset(i, cpuset)) {
            hwloc_cpuset_set(hwloc_cpuset, i);
            count++;
        }
    }

    if (count)
        hwloc_set_cpubind(topology, hwloc_cpuset, HWLOC_CPUBIND_THREAD);

    hwloc_cpuset_free(hwloc_cpuset);
    HYDU_FUNC_EXIT();
    return status;
}

HYD_status HYDT_bind_hwloc_finalize(void)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* Nothing to finalize for now */

    HYDU_FUNC_EXIT();
    return status;
}

/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */

#include "hydra.h"
#include "topo.h"
#include "topo_hwloc.h"

struct HYDT_topo_hwloc_info HYDT_topo_hwloc_info = { 0 };

static hwloc_topology_t topology;
static int hwloc_initialized = 0;

static HYD_status handle_user_binding(const char *binding)
{
    int i;
    char *bindstr, *bindentry;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDT_topo_hwloc_info.num_bitmaps = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_PU);
    HYDU_MALLOC(HYDT_topo_hwloc_info.bitmap, hwloc_bitmap_t *,
                HYDT_topo_hwloc_info.num_bitmaps * sizeof(hwloc_bitmap_t), status);

    /* Initialize all values to map to all CPUs */
    for (i = 0; i < HYDT_topo_hwloc_info.num_bitmaps; i++) {
        HYDT_topo_hwloc_info.bitmap[i] = hwloc_bitmap_alloc();
        hwloc_bitmap_zero(HYDT_topo_hwloc_info.bitmap[i]);
    }

    i = 0;
    bindstr = HYDU_strdup(binding);
    bindentry = strtok(bindstr, ",");
    while (bindentry) {
        if (strcmp(bindentry, "-1"))
            hwloc_bitmap_set(HYDT_topo_hwloc_info.bitmap[i],
                             atoi(bindentry) % HYDT_topo_hwloc_info.num_bitmaps);
        i++;
        bindentry = strtok(NULL, ",");

        /* If the user provided more PU indices than the number of
         * processing units the system has, ignore the extra ones */
        if (i >= HYDT_topo_hwloc_info.num_bitmaps)
            break;
    }
    HYDU_FREE(bindstr);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status handle_rr_binding(void)
{
    int i;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDT_topo_hwloc_info.num_bitmaps = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_PU);
    HYDU_MALLOC(HYDT_topo_hwloc_info.bitmap, hwloc_bitmap_t *,
                HYDT_topo_hwloc_info.num_bitmaps * sizeof(hwloc_bitmap_t), status);

    for (i = 0; i < HYDT_topo_hwloc_info.num_bitmaps; i++) {
        HYDT_topo_hwloc_info.bitmap[i] = hwloc_bitmap_alloc();
        hwloc_bitmap_only(HYDT_topo_hwloc_info.bitmap[i], i);
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status handle_cpu_bitmap_binding(hwloc_obj_type_t obj_type)
{
    int i;
    hwloc_obj_t root, obj;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDT_topo_hwloc_info.num_bitmaps = hwloc_get_nbobjs_by_type(topology, obj_type);
    HYDU_MALLOC(HYDT_topo_hwloc_info.bitmap, hwloc_bitmap_t *,
                HYDT_topo_hwloc_info.num_bitmaps * sizeof(hwloc_bitmap_t), status);

    root = hwloc_get_root_obj(topology);
    for (i = 0; i < HYDT_topo_hwloc_info.num_bitmaps; i++) {
        obj = hwloc_get_obj_inside_cpuset_by_type(topology, root->cpuset, obj_type, i);
        HYDT_topo_hwloc_info.bitmap[i] = hwloc_bitmap_dup(obj->cpuset);
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status handle_rr_cpu_bitmap_binding(hwloc_obj_type_t obj_type)
{
    int i, j, k, p, q, num_objs;
    hwloc_obj_t root, obj;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* total number of bitmaps we'll create */
    HYDT_topo_hwloc_info.num_bitmaps = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_PU);
    HYDU_MALLOC(HYDT_topo_hwloc_info.bitmap, hwloc_bitmap_t *,
                HYDT_topo_hwloc_info.num_bitmaps * sizeof(hwloc_bitmap_t), status);

    /* number of bitmaps of the object provided by the user */
    num_objs = hwloc_get_nbobjs_by_type(topology, obj_type);

    root = hwloc_get_root_obj(topology);

    i = 0;
    j = 0;
    k = 0;
    while (1) {
        obj = hwloc_get_obj_inside_cpuset_by_type(topology, root->cpuset, obj_type, j);

        q = 0;
        for (p = 0; p < HYDT_topo_hwloc_info.num_bitmaps; p++) {
            if (hwloc_bitmap_isset(obj->cpuset, p))
                q++;
            if (q == k + 1)
                break;
        }

        if (q == k + 1) {       /* found the j'th PU index */
            HYDT_topo_hwloc_info.bitmap[i] = hwloc_bitmap_alloc();
            hwloc_bitmap_only(HYDT_topo_hwloc_info.bitmap[i], p);
        }

        i++;
        j++;

        if (i == HYDT_topo_hwloc_info.num_bitmaps)
            break;
        if (j == num_objs) {
            j = 0;
            k++;
        }
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static hwloc_obj_t find_first_cache_obj(int level)
{
    int depth = 1;
    hwloc_obj_t root, obj;

    HYDU_FUNC_ENTER();

    root = hwloc_get_root_obj(topology);
    obj = root;
    while (obj->arity)
        obj = obj->children[0];

    while (obj != root) {
        if (obj->type == HWLOC_OBJ_CACHE) {
            if (depth == level)
                break;
            else
                depth++;
        }
        obj = obj->parent;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return obj;

  fn_fail:
    goto fn_exit;
}

static HYD_status handle_cache_bitmap_binding(int level)
{
    int i;
    hwloc_obj_t obj, first;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    obj = find_first_cache_obj(level);

    if (obj->parent == NULL)
        HYDT_topo_hwloc_info.num_bitmaps = 1;
    else {
        first = obj;
        HYDT_topo_hwloc_info.num_bitmaps = 0;
        while (obj) {
            HYDT_topo_hwloc_info.num_bitmaps++;
            obj = obj->next_cousin;
        }
        obj = first;
    }

    HYDU_MALLOC(HYDT_topo_hwloc_info.bitmap, hwloc_bitmap_t *,
                HYDT_topo_hwloc_info.num_bitmaps * sizeof(hwloc_bitmap_t), status);

    for (i = 0; i < HYDT_topo_hwloc_info.num_bitmaps; i++) {
        HYDT_topo_hwloc_info.bitmap[i] = hwloc_bitmap_dup(obj->cpuset);
        obj = obj->next_cousin;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status handle_rr_cache_bitmap_binding(int level)
{
    int i, j, k, p, q;
    hwloc_obj_t obj, first;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* total number of bitmaps we'll create */
    HYDT_topo_hwloc_info.num_bitmaps = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_PU);
    HYDU_MALLOC(HYDT_topo_hwloc_info.bitmap, hwloc_bitmap_t *,
                HYDT_topo_hwloc_info.num_bitmaps * sizeof(hwloc_bitmap_t), status);

    i = 0;
    j = 0;
    k = 0;
    first = find_first_cache_obj(level);
    obj = first;
    while (1) {
        q = 0;
        for (p = 0; p < HYDT_topo_hwloc_info.num_bitmaps; p++) {
            if (hwloc_bitmap_isset(obj->cpuset, p))
                q++;
            if (q == k + 1)
                break;
        }

        if (q == k + 1) {       /* found the j'th PU index */
            HYDT_topo_hwloc_info.bitmap[i] = hwloc_bitmap_alloc();
            hwloc_bitmap_only(HYDT_topo_hwloc_info.bitmap[i], p);
        }

        i++;
        j++;
        obj = obj->next_cousin;

        if (i == HYDT_topo_hwloc_info.num_bitmaps)
            break;
        if (obj == NULL) {
            obj = first;
            j = 0;
            k++;
        }
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYDT_topo_hwloc_init(const char *binding)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    hwloc_topology_init(&topology);
    hwloc_topology_load(topology);

    hwloc_initialized = 1;

    HYDT_topo_hwloc_info.num_bitmaps = 0;
    HYDT_topo_hwloc_info.bitmap = NULL;

    /* user */
    if (!strncmp(binding, "user:", strlen("user:"))) {
        status = handle_user_binding(binding + strlen("user:"));
        HYDU_ERR_POP(status, "unable to handle user binding\n");
        goto fn_exit;
    }

    /* rr */
    if (!strcmp(binding, "rr")) {
        status = handle_rr_binding();
        HYDU_ERR_POP(status, "unable to handle rr binding\n");
        goto fn_exit;
    }

    /* socket */
    if (!strcmp(binding, "socket") || !strcmp(binding, "sockets")) {
        status = handle_cpu_bitmap_binding(HWLOC_OBJ_SOCKET);
        HYDU_ERR_POP(status, "unable to handle socket binding\n");
        goto fn_exit;
    }

    /* core */
    if (!strcmp(binding, "core") || !strcmp(binding, "cores")) {
        status = handle_cpu_bitmap_binding(HWLOC_OBJ_CORE);
        HYDU_ERR_POP(status, "unable to handle core binding\n");
        goto fn_exit;
    }

    /* cache */
    if (!strcmp(binding, "l1") || !strcmp(binding, "L1")) {
        status = handle_cache_bitmap_binding(1);
        HYDU_ERR_POP(status, "unable to handle cache binding\n");
        goto fn_exit;
    }
    if (!strcmp(binding, "l2") || !strcmp(binding, "L2")) {
        status = handle_cache_bitmap_binding(2);
        HYDU_ERR_POP(status, "unable to handle cache binding\n");
        goto fn_exit;
    }
    if (!strcmp(binding, "l3") || !strcmp(binding, "L3")) {
        status = handle_cache_bitmap_binding(3);
        HYDU_ERR_POP(status, "unable to handle cache binding\n");
        goto fn_exit;
    }

    /* rr by socket */
    if (!strcmp(binding, "rr:socket") || !strcmp(binding, "rr:sockets")) {
        status = handle_rr_cpu_bitmap_binding(HWLOC_OBJ_SOCKET);
        HYDU_ERR_POP(status, "unable to handle rr-socket binding\n");
        goto fn_exit;
    }

    /* rr by core */
    if (!strcmp(binding, "rr:core") || !strcmp(binding, "rr:cores")) {
        status = handle_rr_cpu_bitmap_binding(HWLOC_OBJ_CORE);
        HYDU_ERR_POP(status, "unable to handle rr-core binding\n");
        goto fn_exit;
    }

    /* rr by cache */
    if (!strcmp(binding, "rr:l1") || !strcmp(binding, "rr:L1")) {
        status = handle_rr_cache_bitmap_binding(1);
        HYDU_ERR_POP(status, "unable to handle rr:cache binding\n");
        goto fn_exit;
    }
    if (!strcmp(binding, "rr:l2") || !strcmp(binding, "rr:L2")) {
        status = handle_rr_cache_bitmap_binding(2);
        HYDU_ERR_POP(status, "unable to handle rr:cache binding\n");
        goto fn_exit;
    }
    if (!strcmp(binding, "rr:l3") || !strcmp(binding, "rr:L3")) {
        status = handle_rr_cache_bitmap_binding(3);
        HYDU_ERR_POP(status, "unable to handle rr:cache binding\n");
        goto fn_exit;
    }

    HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                        "unrecognized binding mode \"%s\"\n", binding);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYDT_topo_hwloc_bind(int idx)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    hwloc_set_cpubind(topology,
                      HYDT_topo_hwloc_info.bitmap[idx % HYDT_topo_hwloc_info.num_bitmaps], 0);

    HYDU_FUNC_EXIT();
    return status;
}

HYD_status HYDT_topo_hwloc_get_topomap(char **topomap)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    *topomap = NULL;

    HYDU_FUNC_EXIT();
    return status;
}

HYD_status HYDT_topo_hwloc_get_processmap(char **processmap)
{
    int i, j, k, add_comma;
    char *tmp[HYD_NUM_TMP_STRINGS];
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    i = 0;
    tmp[i++] = HYDU_strdup("(");

    for (j = 0; j < HYDT_topo_hwloc_info.num_bitmaps; j++) {
        add_comma = 0;
        for (k = 0; k < HYDT_topo_hwloc_info.num_bitmaps; k++) {
            if (hwloc_bitmap_isset(HYDT_topo_hwloc_info.bitmap[j], k)) {
                if (add_comma)
                    tmp[i++] = HYDU_strdup(",");
                tmp[i++] = HYDU_int_to_str(k);
                add_comma = 1;
            }
        }
        if (j < HYDT_topo_hwloc_info.num_bitmaps - 1)
            tmp[i++] = HYDU_strdup(";");
    }

    tmp[i++] = HYDU_strdup(")");
    tmp[i++] = NULL;

    status = HYDU_str_alloc_and_join(tmp, processmap);
    HYDU_ERR_POP(status, "unable to append string list\n");

    /* if it's a logically empty mapping, replace it with NULL */
    if (!strcmp(*processmap, "()")) {
        HYDU_FREE(*processmap);
        *processmap = NULL;
    }

    HYDU_free_strlist(tmp);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
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

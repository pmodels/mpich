/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */

#include "hydra.h"
#include "topo_hwloc.h"

#define MAP_LENGTH      (5)

struct HYDT_topo_hwloc_info HYDT_topo_hwloc_info = { 0 };

static hwloc_topology_t topology;
static int hwloc_initialized = 0;

static HYD_status handle_user_binding(const char *binding)
{
    int i, j, k, num_bind_entries, *bind_entry_lengths;
    char *bindstr, **bind_entries;
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

    num_bind_entries = 1;
    for (i = 0; binding[i]; i++)
        if (binding[i] == ',')
            num_bind_entries++;
    HYDU_MALLOC(bind_entries, char **, num_bind_entries * sizeof(char *), status);
    HYDU_MALLOC(bind_entry_lengths, int *, num_bind_entries * sizeof(int), status);

    for (i = 0; i < num_bind_entries; i++)
        bind_entry_lengths[i] = 0;

    j = 0;
    for (i = 0; binding[i]; i++) {
        if (binding[i] != ',')
            bind_entry_lengths[j]++;
        else
            j++;
    }

    for (i = 0; i < num_bind_entries; i++) {
        HYDU_MALLOC(bind_entries[i], char *, bind_entry_lengths[i] * sizeof(char), status);
    }

    j = 0;
    k = 0;
    for (i = 0; binding[i]; i++) {
        if (binding[i] != ',')
            bind_entries[j][k++] = binding[i];
        else {
            bind_entries[j][k] = 0;
            j++;
            k = 0;
        }
    }
    bind_entries[j][k++] = 0;


    for (i = 0; i < num_bind_entries; i++) {
        bindstr = strtok(bind_entries[i], "+");
        while (bindstr) {
            hwloc_bitmap_set(HYDT_topo_hwloc_info.bitmap[i],
                             atoi(bindstr) % HYDT_topo_hwloc_info.num_bitmaps);
            bindstr = strtok(NULL, "+");
        }
    }

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

static hwloc_obj_t find_obj_containing_pu(hwloc_obj_type_t type, int idx, int cache_depth)
{
    int i;
    hwloc_obj_t obj;

    obj = hwloc_get_root_obj(topology);
    if (!obj || !hwloc_bitmap_isset(obj->cpuset, idx))
        return NULL;

    while (obj) {
        if (obj->type == type)
            if (type != HWLOC_OBJ_CACHE || obj->attr->cache.depth == cache_depth)
                break;
        for (i = 0; i < obj->arity; i++) {
            if (hwloc_bitmap_isset(obj->children[i]->cpuset, idx)) {
                obj = obj->children[i];
                break;
            }
        }
    }

    return obj;
}

static HYD_status get_nbobjs_by_type(hwloc_obj_type_t type, int *nbobjs,
                                     int *nbobjs_per_parent)
{
    int x, nb;
    hwloc_obj_type_t parent, t;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    parent = HWLOC_OBJ_SYSTEM;

    if (type == HWLOC_OBJ_PU)
        parent = HWLOC_OBJ_CORE;
    else if (type == HWLOC_OBJ_CORE)
        parent = HWLOC_OBJ_SOCKET;
    else if (type == HWLOC_OBJ_SOCKET)
        parent = HWLOC_OBJ_NODE;
    else if (type == HWLOC_OBJ_NODE)
        parent = HWLOC_OBJ_MACHINE;
    else if (type == HWLOC_OBJ_MACHINE)
        parent = HWLOC_OBJ_MACHINE;

    HYDU_ASSERT(parent != HWLOC_OBJ_SYSTEM, status);

    nb = 0;
    t = type;
    while (1) {
        nb = hwloc_get_nbobjs_by_type(topology, t);
        if (nb)
            break;
        if (t == HWLOC_OBJ_SYSTEM)
            break;
        while (--t == HWLOC_OBJ_CACHE);
    }
    HYDU_ASSERT(nb, status);
    if (nbobjs)
        *nbobjs = nb;

    if (nbobjs_per_parent == NULL)
        goto fn_exit;

    x = 0;
    t = parent;
    while (1) {
        x = hwloc_get_nbobjs_by_type(topology, t);
        if (x)
            break;
        while (--t == HWLOC_OBJ_CACHE);
        if (t == HWLOC_OBJ_SYSTEM)
            break;
    }
    HYDU_ASSERT(x, status);
    HYDU_ASSERT(nb % x == 0, status);

    *nbobjs_per_parent = (nb / x);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status split_count_field(const char *str, char **split_str, int *count)
{
    char *full_str = HYDU_strdup(str), *count_str;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    *split_str = strtok(full_str, ":");
    count_str = strtok(NULL, ":");
    if (count_str)
        *count = atoi(count_str);
    else
        *count = 1;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static int parse_cache_string(const char *str)
{
    char *t1, *t2;

    if (str[0] != 'l')
        return 0;

    t1 = HYDU_strdup(str + 1);
    for (t2 = t1;; t2++) {
        if (*t2 == 'c') {
            *t2 = 0;
            break;
        }
        else if (*t2 < '0' || *t2 > '9')
            return 0;
    }

    return atoi(t1);
}

static HYD_status cache_to_cpu_type(int cache_depth, hwloc_obj_type_t * cpu_type)
{
    hwloc_obj_t cache_obj, cpu_obj;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    cache_obj = hwloc_get_root_obj(topology);
    HYDU_ASSERT(cache_obj, status);

    while (cache_obj && cache_obj->type != HWLOC_OBJ_CACHE &&
           cache_obj->attr->cache.depth != cache_depth)
        cache_obj = cache_obj->first_child;
    if (cache_obj == NULL) {
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "cache level %d not found\n",
                            cache_depth);
    }

    cpu_obj = hwloc_get_root_obj(topology);
    HYDU_ASSERT(cpu_obj, status);

    while (cpu_obj && cpu_obj->type == HWLOC_OBJ_CACHE &&
           !hwloc_bitmap_isequal(cpu_obj->cpuset, cache_obj->cpuset))
        cpu_obj = cpu_obj->first_child;
    if (cpu_obj == NULL) {
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                            "could not find cpu type that maps cache\n");
    }

    *cpu_type = cpu_obj->type;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status obj_type_to_map_str(hwloc_obj_type_t type, int cache_depth, char **map)
{
    hwloc_obj_type_t cpu_type;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (type == HWLOC_OBJ_MACHINE) {
        *map = HYDU_strdup("BTCSN");
        goto fn_exit;
    }
    else if (type == HWLOC_OBJ_NODE) {
        *map = HYDU_strdup("NTCSB");
        goto fn_exit;
    }
    else if (type == HWLOC_OBJ_SOCKET) {
        *map = HYDU_strdup("STCNB");
        goto fn_exit;
    }
    else if (type == HWLOC_OBJ_CORE) {
        *map = HYDU_strdup("CTSNB");
        goto fn_exit;
    }
    else if (type == HWLOC_OBJ_PU) {
        *map = HYDU_strdup("TCSNB");
        goto fn_exit;
    }

    HYDU_ASSERT(type == HWLOC_OBJ_CACHE, status);

    status = cache_to_cpu_type(cache_depth, &cpu_type);
    HYDU_ERR_POP(status, "error while mapping cache to cpu object\n");

    status = obj_type_to_map_str(cpu_type, cache_depth, map);
    HYDU_ERR_POP(status, "error while mapping object to map string\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static int balance_obj_idx(int *obj_idx, int *nbobjs_per_parent)
{
    int i, ret;

    ret = 0;
    for (i = 0; i < MAP_LENGTH - 1; i++) {
        while (obj_idx[i] >= nbobjs_per_parent[i]) {
            obj_idx[i] -= nbobjs_per_parent[i];
            obj_idx[i + 1]++;
        }
        while (obj_idx[i] < 0) {
            obj_idx[i] += nbobjs_per_parent[i];
            obj_idx[i + 1]--;
        }
    }
    while (obj_idx[MAP_LENGTH - 1] >= nbobjs_per_parent[MAP_LENGTH - 1]) {
        obj_idx[MAP_LENGTH - 1] -= nbobjs_per_parent[MAP_LENGTH - 1];
        ret = 1;
    }

    return ret;
}

static HYD_status handle_bitmap_binding(const char *binding, const char *mapping)
{
    int i, j, k, idx, bind_count, map_count, cache_depth = 0;
    hwloc_obj_t obj;
    hwloc_obj_type_t bind_obj_type;
    int total_nbobjs[MAP_LENGTH], obj_idx[MAP_LENGTH], nbpu_per_obj[MAP_LENGTH];
    int nbobjs_per_parent[MAP_LENGTH];
    char *bind_str, *map_str;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* split out the count fields */
    status = split_count_field(binding, &bind_str, &bind_count);
    HYDU_ERR_POP(status, "error splitting count field\n");

    status = split_count_field(mapping, &map_str, &map_count);
    HYDU_ERR_POP(status, "error splitting count field\n");


    /* get the binding object */
    if (!strcmp(bind_str, "board"))
        bind_obj_type = HWLOC_OBJ_MACHINE;
    else if (!strcmp(bind_str, "numa"))
        bind_obj_type = HWLOC_OBJ_NODE;
    else if (!strcmp(bind_str, "socket"))
        bind_obj_type = HWLOC_OBJ_SOCKET;
    else if (!strcmp(bind_str, "core"))
        bind_obj_type = HWLOC_OBJ_CORE;
    else if (!strcmp(bind_str, "hwthread"))
        bind_obj_type = HWLOC_OBJ_PU;
    else {
        /* check if it's in the l*cache format */
        cache_depth = parse_cache_string(bind_str);
        if (!cache_depth) {
            HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                                "unrecognized binding string \"%s\"\n", binding);
        }
        bind_obj_type = HWLOC_OBJ_CACHE;
    }


    /* get the mapping string */
    if (!strcmp(map_str, "board")) {
        HYDU_FREE(map_str);
        obj_type_to_map_str(HWLOC_OBJ_MACHINE, 0, &map_str);
    }
    else if (!strcmp(map_str, "numa")) {
        HYDU_FREE(map_str);
        obj_type_to_map_str(HWLOC_OBJ_NODE, 0, &map_str);
    }
    else if (!strcmp(map_str, "socket")) {
        HYDU_FREE(map_str);
        obj_type_to_map_str(HWLOC_OBJ_SOCKET, 0, &map_str);
    }
    else if (!strcmp(map_str, "core")) {
        HYDU_FREE(map_str);
        obj_type_to_map_str(HWLOC_OBJ_CORE, 0, &map_str);
    }
    else if (!strcmp(map_str, "hwthread")) {
        HYDU_FREE(map_str);
        obj_type_to_map_str(HWLOC_OBJ_PU, 0, &map_str);
    }
    else {
        cache_depth = parse_cache_string(map_str);
        if (cache_depth) {
            HYDU_FREE(map_str);
            obj_type_to_map_str(HWLOC_OBJ_CACHE, cache_depth, &map_str);
        }
        else {
            for (i = 0; i < strlen(map_str); i++) {
                if (map_str[i] >= 'a' && map_str[i] <= 'z')
                    map_str[i] += ('A' - 'a');

                /* If any of the characters are not in the form, we
                 * want, return an error */
                if (map_str[i] != 'T' && map_str[i] != 'C' && map_str[i] != 'S' &&
                    map_str[i] != 'N' && map_str[i] != 'B') {
                    HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                                        "unrecognized mapping string \"%s\"\n", mapping);
                }
            }
        }
    }


    for (i = 0; i < MAP_LENGTH; i++) {
        if (map_str[i] == 'T')
            status = get_nbobjs_by_type(HWLOC_OBJ_PU, &total_nbobjs[i], &nbobjs_per_parent[i]);
        else if (map_str[i] == 'C')
            status =
                get_nbobjs_by_type(HWLOC_OBJ_CORE, &total_nbobjs[i], &nbobjs_per_parent[i]);
        else if (map_str[i] == 'S')
            status =
                get_nbobjs_by_type(HWLOC_OBJ_SOCKET, &total_nbobjs[i], &nbobjs_per_parent[i]);
        else if (map_str[i] == 'N')
            status =
                get_nbobjs_by_type(HWLOC_OBJ_NODE, &total_nbobjs[i], &nbobjs_per_parent[i]);
        else if (map_str[i] == 'B')
            status =
                get_nbobjs_by_type(HWLOC_OBJ_MACHINE, &total_nbobjs[i], &nbobjs_per_parent[i]);
        HYDU_ERR_POP(status, "unable to get number of objects\n");

        nbpu_per_obj[i] = HYDT_topo_hwloc_info.num_bitmaps / total_nbobjs[i];
        obj_idx[i] = 0;
    }

    i = 0;
    while (i < HYDT_topo_hwloc_info.num_bitmaps) {
        for (j = 0; j < bind_count; j++) {
            for (idx = 0, k = 0; k < MAP_LENGTH; k++)
                idx += (obj_idx[k] * nbpu_per_obj[k]);

            obj = find_obj_containing_pu(bind_obj_type, idx++, cache_depth);
            if (obj == NULL)
                break;

            hwloc_bitmap_or(HYDT_topo_hwloc_info.bitmap[i], HYDT_topo_hwloc_info.bitmap[i],
                            obj->cpuset);

            obj_idx[0] += map_count;
            balance_obj_idx(obj_idx, nbobjs_per_parent);
        }
        i++;
    }

    /* reset the number of bitmaps available to what we actually set */
    HYDT_topo_hwloc_info.num_bitmaps = i;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYDT_topo_hwloc_init(const char *binding, const char *mapping, const char *membind)
{
    int i;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_ASSERT(binding, status);

    hwloc_topology_init(&topology);
    hwloc_topology_load(topology);

    hwloc_initialized = 1;


    /* initialize bitmaps */
    status = get_nbobjs_by_type(HWLOC_OBJ_PU, &HYDT_topo_hwloc_info.num_bitmaps, NULL);
    HYDU_ERR_POP(status, "unable to get number of PUs\n");

    HYDU_MALLOC(HYDT_topo_hwloc_info.bitmap, hwloc_bitmap_t *,
                HYDT_topo_hwloc_info.num_bitmaps * sizeof(hwloc_bitmap_t), status);

    for (i = 0; i < HYDT_topo_hwloc_info.num_bitmaps; i++) {
        HYDT_topo_hwloc_info.bitmap[i] = hwloc_bitmap_alloc();
        hwloc_bitmap_zero(HYDT_topo_hwloc_info.bitmap[i]);
    }


    /* bindings that don't require mapping */
    if (!strncmp(binding, "user:", strlen("user:"))) {
        status = handle_user_binding(binding + strlen("user:"));
        HYDU_ERR_POP(status, "error binding to %s\n", binding);
        goto fn_exit;
    }
    else if (!strcmp(binding, "rr")) {
        status = handle_rr_binding();
        HYDU_ERR_POP(status, "error binding to %s\n", binding);
        goto fn_exit;
    }

    status = handle_bitmap_binding(binding, mapping ? mapping : binding);
    HYDU_ERR_POP(status, "error binding with bind \"%s\" and map \"%s\"\n", binding, mapping);


    /* Memory binding options */
    if (membind == NULL)
        HYDT_topo_hwloc_info.membind = HWLOC_MEMBIND_DEFAULT;
    else if (!strcmp(membind, "firsttouch"))
        HYDT_topo_hwloc_info.membind = HWLOC_MEMBIND_FIRSTTOUCH;
    else if (!strcmp(membind, "nexttouch"))
        HYDT_topo_hwloc_info.membind = HWLOC_MEMBIND_NEXTTOUCH;
    else if (!strncmp(membind, "bind:", strlen("bind:"))) {
        HYDT_topo_hwloc_info.membind = HWLOC_MEMBIND_BIND;
    }
    else if (!strncmp(membind, "interleave:", strlen("interleave:"))) {
        HYDT_topo_hwloc_info.membind = HWLOC_MEMBIND_INTERLEAVE;
    }
    else if (!strncmp(membind, "replicate:", strlen("replicate:"))) {
        HYDT_topo_hwloc_info.membind = HWLOC_MEMBIND_REPLICATE;
    }
    else {
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                            "unrecognized membind policy \"%s\"\n", membind);
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYDT_topo_hwloc_bind(int idx)
{
    int id = idx % HYDT_topo_hwloc_info.num_bitmaps;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    hwloc_set_cpubind(topology, HYDT_topo_hwloc_info.bitmap[id], 0);
    hwloc_set_membind(topology, HYDT_topo_hwloc_info.bitmap[id],
                      HYDT_topo_hwloc_info.membind, 0);

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

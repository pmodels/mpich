/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2009 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */

#include "hydra_topo.h"
#include "hydra_topo_internal.h"
#include "hydra_topo_hwloc.h"
#include "hydra_err.h"
#include "hydra_mem.h"

#define MAP_LENGTH      (5)

struct HYDI_topo_hwloc_info HYDI_topo_hwloc_info = { 0 };

static hwloc_topology_t topology;
static int hwloc_initialized = 0;

static HYD_status handle_user_binding(const char *binding)
{
    int i, j, k, num_bind_entries, *bind_entry_lengths;
    char *bindstr, **bind_entries;
    HYD_status status = HYD_SUCCESS;

    HYD_FUNC_ENTER();

    HYD_ASSERT(hwloc_initialized, status);

    num_bind_entries = 1;
    for (i = 0; binding[i]; i++)
        if (binding[i] == ',')
            num_bind_entries++;
    HYD_MALLOC(bind_entries, char **, num_bind_entries * sizeof(char *), status);
    HYD_MALLOC(bind_entry_lengths, int *, num_bind_entries * sizeof(int), status);

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
        HYD_MALLOC(bind_entries[i], char *, bind_entry_lengths[i] * sizeof(char), status);
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

    /* initialize bitmaps */
    HYD_MALLOC(HYDI_topo_hwloc_info.bitmap, hwloc_bitmap_t *,
               num_bind_entries * sizeof(hwloc_bitmap_t), status);

    for (i = 0; i < num_bind_entries; i++) {
        HYDI_topo_hwloc_info.bitmap[i] = hwloc_bitmap_alloc();
        hwloc_bitmap_zero(HYDI_topo_hwloc_info.bitmap[i]);
        bindstr = strtok(bind_entries[i], "+");
        while (bindstr) {
            hwloc_bitmap_set(HYDI_topo_hwloc_info.bitmap[i], atoi(bindstr));
            bindstr = strtok(NULL, "+");
        }
    }

    HYDI_topo_hwloc_info.num_bitmaps = num_bind_entries;
    HYDI_topo_hwloc_info.user_binding = 1;

    /* free temporary memory */
    for (i = 0; i < num_bind_entries; i++) {
        MPL_free(bind_entries[i]);
    }
    MPL_free(bind_entries);
    MPL_free(bind_entry_lengths);

  fn_exit:
    HYD_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status handle_rr_binding(void)
{
    int i;
    HYD_status status = HYD_SUCCESS;

    HYD_FUNC_ENTER();

    HYD_ASSERT(hwloc_initialized, status);

    /* initialize bitmaps */
    HYDI_topo_hwloc_info.num_bitmaps = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_PU);

    HYD_MALLOC(HYDI_topo_hwloc_info.bitmap, hwloc_bitmap_t *,
               HYDI_topo_hwloc_info.num_bitmaps * sizeof(hwloc_bitmap_t), status);

    for (i = 0; i < HYDI_topo_hwloc_info.num_bitmaps; i++) {
        HYDI_topo_hwloc_info.bitmap[i] = hwloc_bitmap_alloc();
        hwloc_bitmap_only(HYDI_topo_hwloc_info.bitmap[i], i);
    }

  fn_exit:
    HYD_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status split_count_field(const char *str, char **split_str, int *count)
{
    char *full_str = MPL_strdup(str), *count_str;
    HYD_status status = HYD_SUCCESS;

    HYD_FUNC_ENTER();

    *split_str = strtok(full_str, ":");
    count_str = strtok(NULL, ":");
    if (count_str)
        *count = atoi(count_str);
    else
        *count = 1;

    HYD_FUNC_EXIT();
    return status;
}

static int parse_cache_string(const char *str)
{
    char *t1, *t2;

    if (str[0] != 'l')
        return 0;

    t1 = MPL_strdup(str + 1);
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

static HYD_status handle_bitmap_binding(const char *binding, const char *mapping)
{
    int i, j, k, bind_count, map_count, cache_depth = 0, bind_depth = 0, map_depth = 0;
    int total_map_objs, total_bind_objs, num_pus_in_map_domain, num_pus_in_bind_domain,
        total_map_domains;
    hwloc_obj_t map_obj, bind_obj, *start_pu;
    hwloc_cpuset_t *map_domains;
    char *bind_str, *map_str;
    HYD_status status = HYD_SUCCESS;

    HYD_FUNC_ENTER();

    /* split out the count fields */
    status = split_count_field(binding, &bind_str, &bind_count);
    HYD_ERR_POP(status, "error splitting count field\n");

    status = split_count_field(mapping, &map_str, &map_count);
    HYD_ERR_POP(status, "error splitting count field\n");


    /* get the binding object */
    if (!strcmp(bind_str, "board"))
        bind_depth = hwloc_get_type_or_above_depth(topology, HWLOC_OBJ_MACHINE);
    else if (!strcmp(bind_str, "numa"))
        bind_depth = hwloc_get_type_or_above_depth(topology, HWLOC_OBJ_NODE);
    else if (!strcmp(bind_str, "socket"))
        bind_depth = hwloc_get_type_or_above_depth(topology, HWLOC_OBJ_SOCKET);
    else if (!strcmp(bind_str, "core"))
        bind_depth = hwloc_get_type_or_above_depth(topology, HWLOC_OBJ_CORE);
    else if (!strcmp(bind_str, "hwthread"))
        bind_depth = hwloc_get_type_or_above_depth(topology, HWLOC_OBJ_PU);
    else {
        /* check if it's in the l*cache format */
        cache_depth = parse_cache_string(bind_str);
        if (!cache_depth) {
            HYD_ERR_SETANDJUMP(status, HYD_ERR_INTERNAL, "unrecognized binding string \"%s\"\n",
                               binding);
        }
        bind_depth = hwloc_get_cache_type_depth(topology, cache_depth, -1);
    }

    /* get the mapping */
    if (!strcmp(map_str, "board"))
        map_depth = hwloc_get_type_or_above_depth(topology, HWLOC_OBJ_MACHINE);
    else if (!strcmp(map_str, "numa"))
        map_depth = hwloc_get_type_or_above_depth(topology, HWLOC_OBJ_NODE);
    else if (!strcmp(map_str, "socket"))
        map_depth = hwloc_get_type_or_above_depth(topology, HWLOC_OBJ_SOCKET);
    else if (!strcmp(map_str, "core"))
        map_depth = hwloc_get_type_or_above_depth(topology, HWLOC_OBJ_CORE);
    else if (!strcmp(map_str, "hwthread"))
        map_depth = hwloc_get_type_or_above_depth(topology, HWLOC_OBJ_PU);
    else {
        cache_depth = parse_cache_string(map_str);
        if (!cache_depth) {
            HYD_ERR_SETANDJUMP(status, HYD_ERR_INTERNAL, "unrecognized mapping string \"%s\"\n",
                               mapping);
        }
        map_depth = hwloc_get_cache_type_depth(topology, cache_depth, -1);
    }

    /*
     * Process Affinity Algorithm:
     *
     * The code below works in 3 stages. The end result is an array of all the possible
     * binding bitmaps for a system, based on the options specified.
     *
     * 1. Define all possible mapping "domains" in a system. A mapping domain is a group
     *    of hardware elements found by traversing the topology. Each traversal skips the
     *    number of elements the user specified in the mapping string. The traversal ends
     *    when the next mapping domain == the first mapping domain. Note that if the
     *    mapping string defines a domain that is larger than the system size, we exit
     *    with an error.
     *
     * 2. Define the number of possible binding domains within a mapping domain. This
     *    process is similar to step 1, in that we traverse the mapping domain finding
     *    all possible bind combinations, stopping when a duplicate of the first binding
     *    is reached. If a binding is larger (in # of PUs) than the mapping domain,
     *    the number of possible bindings for that domain is 1. In this stage, we also
     *    locate the first PU in each mapping domain for use later during binding.
     *
     * 3. Create the binding bitmaps. We allocate an array of bitmaps and fill them in
     *    with all possible bindings. The starting PU in each mapping domain is advanced
     *    if and when we wrap around to the beginning of the mapping domains. This ensures
     *    that we do not repeat.
     *
     */

    /* calculate the number of map domains */
    total_map_objs = hwloc_get_nbobjs_by_depth(topology, map_depth);
    num_pus_in_map_domain = (HYDI_topo_hwloc_info.total_num_pus / total_map_objs) * map_count;
    HYD_ERR_CHKANDJUMP(status, num_pus_in_map_domain > HYDI_topo_hwloc_info.total_num_pus,
                       HYD_ERR_INTERNAL, "mapping option \"%s\" larger than total system size\n",
                       mapping);

    /* The number of total_map_domains should be large enough to
     * contain all contiguous map object collections of length
     * map_count.  For example, if the map object is "socket" and the
     * map_count is 3, on a system with 4 sockets, the following map
     * domains should be included: (0,1,2), (3,0,1), (2,3,0), (1,2,3).
     * We do this by finding how many times we need to replicate the
     * list of the map objects so that an integral number of map
     * domains can map to them.  In the above case, the list of map
     * objects is replicated 3 times. */
    for (i = 1; (i * total_map_objs) % map_count; i++);
    total_map_domains = (i * total_map_objs) / map_count;

    /* initialize the map domains */
    HYD_MALLOC(map_domains, hwloc_bitmap_t *, total_map_domains * sizeof(hwloc_bitmap_t), status);
    HYD_MALLOC(start_pu, hwloc_obj_t *, total_map_domains * sizeof(hwloc_obj_t), status);

    /* For each map domain, find the next map object (first map object
     * for the first map domain) and add the following "map_count"
     * number of contiguous map objects, wrapping to the first one if
     * needed, to the map domain.  Store the first PU in the first map
     * object of the map domain as "start_pu".  This is needed later
     * for the actual binding. */
    map_obj = NULL;
    for (i = 0; i < total_map_domains; i++) {
        map_domains[i] = hwloc_bitmap_alloc();
        hwloc_bitmap_zero(map_domains[i]);

        for (j = 0; j < map_count; j++) {
            map_obj = hwloc_get_next_obj_by_depth(topology, map_depth, map_obj);
            /* map_obj will be NULL if it reaches the end. call again to wrap around */
            if (!map_obj)
                map_obj = hwloc_get_next_obj_by_depth(topology, map_depth, map_obj);

            if (j == 0)
                start_pu[i] =
                    hwloc_get_obj_inside_cpuset_by_type(topology, map_obj->cpuset, HWLOC_OBJ_PU, 0);

            hwloc_bitmap_or(map_domains[i], map_domains[i], map_obj->cpuset);
        }
    }


    /* Find the possible binding domains is similar to that of map
     * domains.  But if a binding domain is larger (in # of PUs) than
     * the mapping domain, the number of possible bindings for that
     * domain is 1. */

    /* calculate the number of possible bindings and allocate bitmaps for them */
    total_bind_objs = hwloc_get_nbobjs_by_depth(topology, bind_depth);
    num_pus_in_bind_domain = (HYDI_topo_hwloc_info.total_num_pus / total_bind_objs) * bind_count;

    if (num_pus_in_bind_domain < num_pus_in_map_domain) {
        for (i = 1; (i * num_pus_in_map_domain) % num_pus_in_bind_domain; i++);
        HYDI_topo_hwloc_info.num_bitmaps =
            (i * num_pus_in_map_domain * total_map_domains) / num_pus_in_bind_domain;
    }
    else {
        HYDI_topo_hwloc_info.num_bitmaps = total_map_domains;
    }

    /* initialize bitmaps */
    HYD_MALLOC(HYDI_topo_hwloc_info.bitmap, hwloc_bitmap_t *,
               HYDI_topo_hwloc_info.num_bitmaps * sizeof(hwloc_bitmap_t), status);

    for (i = 0; i < HYDI_topo_hwloc_info.num_bitmaps; i++) {
        HYDI_topo_hwloc_info.bitmap[i] = hwloc_bitmap_alloc();
        hwloc_bitmap_zero(HYDI_topo_hwloc_info.bitmap[i]);
    }

    /* do bindings */
    i = 0;
    while (i < HYDI_topo_hwloc_info.num_bitmaps) {
        for (j = 0; j < total_map_domains; j++) {
            bind_obj = hwloc_get_ancestor_obj_by_depth(topology, bind_depth, start_pu[j]);

            for (k = 0; k < bind_count; k++) {
                hwloc_bitmap_or(HYDI_topo_hwloc_info.bitmap[i], HYDI_topo_hwloc_info.bitmap[i],
                                bind_obj->cpuset);

                /* if the binding is smaller than the mapping domain, wrap around inside that domain */
                if (num_pus_in_bind_domain < num_pus_in_map_domain) {
                    bind_obj =
                        hwloc_get_next_obj_inside_cpuset_by_depth(topology, map_domains[j],
                                                                  bind_depth, bind_obj);
                    if (!bind_obj)
                        bind_obj =
                            hwloc_get_next_obj_inside_cpuset_by_depth(topology, map_domains[j],
                                                                      bind_depth, bind_obj);
                }
                else {
                    bind_obj = hwloc_get_next_obj_by_depth(topology, bind_depth, bind_obj);
                    if (!bind_obj)
                        bind_obj = hwloc_get_next_obj_by_depth(topology, bind_depth, bind_obj);
                }

            }
            i++;

            /* advance the starting position for this map domain, if needed */
            if (num_pus_in_bind_domain < num_pus_in_map_domain) {
                for (k = 0; k < num_pus_in_bind_domain; k++) {
                    start_pu[j] =
                        hwloc_get_next_obj_inside_cpuset_by_type(topology, map_domains[j],
                                                                 HWLOC_OBJ_PU, start_pu[j]);
                    if (!start_pu[j])
                        start_pu[j] =
                            hwloc_get_next_obj_inside_cpuset_by_type(topology, map_domains[j],
                                                                     HWLOC_OBJ_PU, start_pu[j]);
                }
            }
        }
    }

    /* free temporary memory */
    MPL_free(map_domains);
    MPL_free(start_pu);

  fn_exit:
    HYD_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYDI_topo_hwloc_init(const char *binding, const char *mapping, const char *membind)
{
    HYD_status status = HYD_SUCCESS;

    HYD_FUNC_ENTER();

    HYD_ASSERT(binding, status);

    hwloc_topology_init(&topology);
    hwloc_topology_load(topology);

    HYDI_topo_hwloc_info.total_num_pus = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_PU);

    hwloc_initialized = 1;

    /* bindings that don't require mapping */
    if (!strncmp(binding, "user:", strlen("user:"))) {
        status = handle_user_binding(binding + strlen("user:"));
        HYD_ERR_POP(status, "error binding to %s\n", binding);
        goto fn_exit;
    }
    else if (!strcmp(binding, "rr")) {
        status = handle_rr_binding();
        HYD_ERR_POP(status, "error binding to %s\n", binding);
        goto fn_exit;
    }

    status = handle_bitmap_binding(binding, mapping ? mapping : binding);
    HYD_ERR_POP(status, "error binding with bind \"%s\" and map \"%s\"\n", binding, mapping);


    /* Memory binding options */
    if (membind == NULL)
        HYDI_topo_hwloc_info.membind = HWLOC_MEMBIND_DEFAULT;
    else if (!strcmp(membind, "firsttouch"))
        HYDI_topo_hwloc_info.membind = HWLOC_MEMBIND_FIRSTTOUCH;
    else if (!strcmp(membind, "nexttouch"))
        HYDI_topo_hwloc_info.membind = HWLOC_MEMBIND_NEXTTOUCH;
    else if (!strncmp(membind, "bind:", strlen("bind:"))) {
        HYDI_topo_hwloc_info.membind = HWLOC_MEMBIND_BIND;
    }
    else if (!strncmp(membind, "interleave:", strlen("interleave:"))) {
        HYDI_topo_hwloc_info.membind = HWLOC_MEMBIND_INTERLEAVE;
    }
    else if (!strncmp(membind, "replicate:", strlen("replicate:"))) {
        HYDI_topo_hwloc_info.membind = HWLOC_MEMBIND_REPLICATE;
    }
    else {
        HYD_ERR_SETANDJUMP(status, HYD_ERR_INTERNAL, "unrecognized membind policy \"%s\"\n",
                           membind);
    }

  fn_exit:
    HYD_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYDI_topo_hwloc_bind(int idx)
{
    int id;
    HYD_status status = HYD_SUCCESS;

    HYD_FUNC_ENTER();

    /* For processes where the user did not specify a binding unit, no binding is needed. */
    if (!HYDI_topo_hwloc_info.user_binding || (idx < HYDI_topo_hwloc_info.num_bitmaps)) {
        id = idx % HYDI_topo_hwloc_info.num_bitmaps;

        if (HYDI_topo_info.debug) {
            /* Print the binding bitmaps for debugging. */
            int i;
            char *binding;

            HYD_MALLOC(binding, char *, HYDI_topo_hwloc_info.total_num_pus + 1, status);
            memset(binding, '\0', HYDI_topo_hwloc_info.total_num_pus + 1);

            for (i = 0; i < HYDI_topo_hwloc_info.total_num_pus; i++) {
                if (hwloc_bitmap_isset(HYDI_topo_hwloc_info.bitmap[id], i))
                    *(binding + i) = '1';
                else
                    *(binding + i) = '0';
            }

            HYD_PRINT_NOPREFIX(stdout, "process %d binding: %s\n", idx, binding);
            MPL_free(binding);
        }
        hwloc_set_cpubind(topology, HYDI_topo_hwloc_info.bitmap[id], 0);
        hwloc_set_membind(topology, HYDI_topo_hwloc_info.bitmap[id], HYDI_topo_hwloc_info.membind,
                          0);
    }

  fn_exit:
    HYD_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYDI_topo_hwloc_finalize(void)
{
    HYD_status status = HYD_SUCCESS;

    HYD_FUNC_ENTER();

    if (hwloc_initialized)
        hwloc_topology_destroy(topology);

    HYD_FUNC_EXIT();
    return status;
}

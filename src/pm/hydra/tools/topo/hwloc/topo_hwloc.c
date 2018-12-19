/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2009 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */

#include "hydra.h"
#include "topo.h"
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

    HYDU_ASSERT(hwloc_initialized, status);

    num_bind_entries = 1;
    for (i = 0; binding[i]; i++)
        if (binding[i] == ',')
            num_bind_entries++;
    HYDU_MALLOC_OR_JUMP(bind_entries, char **, num_bind_entries * sizeof(char *), status);
    HYDU_MALLOC_OR_JUMP(bind_entry_lengths, int *, num_bind_entries * sizeof(int), status);

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
        HYDU_MALLOC_OR_JUMP(bind_entries[i], char *, bind_entry_lengths[i] * sizeof(char), status);
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
    HYDU_MALLOC_OR_JUMP(HYDT_topo_hwloc_info.bitmap, hwloc_bitmap_t *,
                        num_bind_entries * sizeof(hwloc_bitmap_t), status);

    for (i = 0; i < num_bind_entries; i++) {
        HYDT_topo_hwloc_info.bitmap[i] = hwloc_bitmap_alloc();
        hwloc_bitmap_zero(HYDT_topo_hwloc_info.bitmap[i]);
        bindstr = strtok(bind_entries[i], "+");
        while (bindstr) {
            hwloc_bitmap_set(HYDT_topo_hwloc_info.bitmap[i], atoi(bindstr));
            bindstr = strtok(NULL, "+");
        }
    }

    HYDT_topo_hwloc_info.num_bitmaps = num_bind_entries;
    HYDT_topo_hwloc_info.user_binding = 1;

    /* free temporary memory */
    for (i = 0; i < num_bind_entries; i++) {
        MPL_free(bind_entries[i]);
    }
    MPL_free(bind_entries);
    MPL_free(bind_entry_lengths);

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

    HYDU_ASSERT(hwloc_initialized, status);

    /* initialize bitmaps */
    HYDT_topo_hwloc_info.num_bitmaps = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_PU);

    HYDU_MALLOC_OR_JUMP(HYDT_topo_hwloc_info.bitmap, hwloc_bitmap_t *,
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

static HYD_status split_count_field(const char *str, char **split_str, int *count)
{
    char *full_str = MPL_strdup(str), *count_str;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    *split_str = strtok(full_str, ":");
    count_str = strtok(NULL, ":");
    if (count_str)
        *count = atoi(count_str);
    else
        *count = 1;

    HYDU_FUNC_EXIT();
    return status;
}


struct hwloc_obj_info_table {
    const char *val;
    hwloc_obj_type_t obj_type;
};

static struct hwloc_obj_info_table hwloc_obj_info[] = {
    {"machine", HWLOC_OBJ_MACHINE},
    {"socket", HWLOC_OBJ_PACKAGE},
    {"numa", HWLOC_OBJ_NUMANODE},
    {"core", HWLOC_OBJ_CORE},
    {"hwthread", HWLOC_OBJ_PU},
    {"l1dcache", HWLOC_OBJ_L1CACHE},
    {"l1ucache", HWLOC_OBJ_L1CACHE},
    {"l1icache", HWLOC_OBJ_L1ICACHE},
    {"l1cache", HWLOC_OBJ_L1CACHE},
    {"l2dcache", HWLOC_OBJ_L2CACHE},
    {"l2ucache", HWLOC_OBJ_L2CACHE},
    {"l2icache", HWLOC_OBJ_L2ICACHE},
    {"l2cache", HWLOC_OBJ_L2CACHE},
    {"l3dcache", HWLOC_OBJ_L3CACHE},
    {"l3ucache", HWLOC_OBJ_L3CACHE},
    {"l3icache", HWLOC_OBJ_L3ICACHE},
    {"l3cache", HWLOC_OBJ_L3CACHE},
    {"l4dcache", HWLOC_OBJ_L4CACHE},
    {"l4ucache", HWLOC_OBJ_L4CACHE},
    {"l4cache", HWLOC_OBJ_L4CACHE},
    {"l5dcache", HWLOC_OBJ_L5CACHE},
    {"l5ucache", HWLOC_OBJ_L5CACHE},
    {"l5cache", HWLOC_OBJ_L5CACHE},
    {NULL, HWLOC_OBJ_TYPE_MAX}
};

static int io_device_found(const char *resource, const char *devname, hwloc_obj_t io_device,
                           hwloc_obj_osdev_type_t obj_type)
{
    if (!strncmp(resource, devname, strlen(devname))) {
        /* device type does not match */
        if (io_device->attr->osdev.type != obj_type)
            return 0;

        /* device prefix does not match */
        if (strncmp(io_device->name, devname, strlen(devname)))
            return 0;

        /* specific device is supplied, but does not match */
        if (strlen(resource) != strlen(devname) && strcmp(io_device->name, resource))
            return 0;
    }

    return 1;
}

static HYD_status get_hw_obj_list(const char *resource, hwloc_obj_t ** resource_list,
                                  int *resource_list_length, int *resource_count, char **prefix,
                                  hwloc_obj_type_t * type)
{
    HYD_status status = HYD_SUCCESS;
    int i;
    char *resource_str;
    hwloc_obj_t *resource_obj = NULL;
    hwloc_obj_type_t obj_type = HWLOC_OBJ_TYPE_MAX;
    char *obj_type_prefix = NULL;
    int count = 0;

    HYDU_FUNC_ENTER();

    status = split_count_field(resource, &resource_str, resource_count);
    HYDU_ERR_POP(status, "error splitting count field\n");

    if (!strncmp(resource, "pci:", strlen("pci:"))) {
        hwloc_obj_t pci_obj = hwloc_get_pcidev_by_busidstring(topology,
                                                              resource + strlen("pci:"));

        if (pci_obj == NULL)
            HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                                "unrecognized pci device id string \"%s\"\n", resource);
        HYDU_MALLOC_OR_JUMP(resource_obj, hwloc_obj_t *, sizeof(hwloc_obj_t), status);
        resource_obj[0] = hwloc_get_non_io_ancestor_obj(topology, pci_obj);
        *resource_count = 1;
        *resource_list_length = 1;
        obj_type_prefix = MPL_strdup(resource);
        obj_type = HWLOC_OBJ_OS_DEVICE;
    } else if (!strncmp(resource, "gpu", strlen("gpu"))) {
        hwloc_obj_t io_device = NULL;

        obj_type_prefix = MPL_strdup("gpu");
        obj_type = HWLOC_OBJ_OS_DEVICE;

        while ((io_device = hwloc_get_next_osdev(topology, io_device))) {
            int gpuid = atoi(resource + strlen("gpu"));

            if (io_device->attr->osdev.type != HWLOC_OBJ_OSDEV_GPU)
                continue;

            if (*(resource + strlen("gpu")) == '\0' ||
                *(resource + strlen("gpu")) == ':' || gpuid == io_device->logical_index) {
                HYDU_REALLOC_OR_JUMP(resource_obj, hwloc_obj_t *,
                                     (count + 1) * sizeof(hwloc_obj_t), status);
                resource_obj[count++] = hwloc_get_non_io_ancestor_obj(topology, io_device);
            }
        }
        *resource_list_length = count;
    } else if (!strncmp(resource, "ib", strlen("ib"))
               || !strncmp(resource, "hfi", strlen("hfi"))
               || !strncmp(resource, "eth", strlen("eth"))
               || !strncmp(resource, "en", strlen("en"))) {
        hwloc_obj_t io_device = NULL;
        obj_type = HWLOC_OBJ_OS_DEVICE;

        if (!strncmp(resource, "ib", strlen("ib")))
            obj_type_prefix = MPL_strdup("ib");
        else if (!strncmp(resource, "hfi", strlen("hfi")))
            obj_type_prefix = MPL_strdup("hfi");
        else if (!strncmp(resource, "eth", strlen("eth")) || !strncmp(resource, "en", strlen("en")))
            obj_type_prefix = MPL_strdup("en");

        while ((io_device = hwloc_get_next_osdev(topology, io_device))) {
            if (!io_device_found(resource_str, "hfi", io_device, HWLOC_OBJ_OSDEV_OPENFABRICS))
                continue;
            if (!io_device_found(resource_str, "ib", io_device, HWLOC_OBJ_OSDEV_NETWORK))
                continue;
            if (!io_device_found(resource_str, "eth", io_device, HWLOC_OBJ_OSDEV_NETWORK) &&
                !io_device_found(resource_str, "en", io_device, HWLOC_OBJ_OSDEV_NETWORK))
                continue;
            HYDU_REALLOC_OR_JUMP(resource_obj, hwloc_obj_t *, (count + 1) * sizeof(hwloc_obj_t),
                                 status);
            resource_obj[count++] = hwloc_get_non_io_ancestor_obj(topology, io_device);
        }
        *resource_list_length = count;
    } else {
        for (i = 0; hwloc_obj_info[i].val; i++) {
            if (!strcmp(hwloc_obj_info[i].val, resource_str)) {
                obj_type = hwloc_obj_info[i].obj_type;
                obj_type_prefix = MPL_strdup(hwloc_obj_info[i].val);
                break;
            }
        }

        if (obj_type != HWLOC_OBJ_TYPE_MAX) {
            hwloc_obj_t obj = NULL;
            while ((obj = hwloc_get_next_obj_by_type(topology, obj_type, obj))) {
                int cpuset_covered = 0;
                /* MCDRAM and NUMA have the same cpuset, hence skipping objects
                 * with the same cpuset
                 */
                for (i = 0; i < count; i++) {
                    if ((resource_obj[i]->complete_cpuset && obj->complete_cpuset) &&
                        !hwloc_bitmap_compare(resource_obj[i]->complete_cpuset,
                                              obj->complete_cpuset)) {
                        cpuset_covered = 1;
                        break;
                    } else if (!hwloc_bitmap_compare(resource_obj[i]->cpuset, obj->cpuset)) {
                        cpuset_covered = 1;
                        break;
                    }
                }
                if (!cpuset_covered) {
                    HYDU_REALLOC_OR_JUMP(resource_obj, hwloc_obj_t *,
                                         (count + 1) * sizeof(hwloc_obj_t), status);
                    resource_obj[count++] = obj;
                }
            }
            *resource_list_length = count;
        }
    }

    if (obj_type == HWLOC_OBJ_TYPE_MAX)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "unrecognized directive \"%s\"\n",
                            resource);

    if (*resource_list_length == 0)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                            "hardware object not found in the target \"%s\"\n", resource);

  fn_exit:
    HYDU_FUNC_EXIT();
    *resource_list = resource_obj;
    if (obj_type_prefix != NULL)
        *prefix = obj_type_prefix;
    *type = obj_type;
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status handle_bitmap_binding(const char *binding, const char *mapping)
{
    int i, j, k, bind_count, map_count;
    int total_map_objs, total_bind_objs, num_pus_in_map_domain,
        num_pus_in_bind_domain, total_map_domains;
    hwloc_obj_t bind_obj = NULL, map_obj = NULL, *start_pu = NULL;
    char *bind_obj_prefix = NULL, *map_obj_prefix = NULL;
    hwloc_obj_t *bind_list, *map_list;
    hwloc_obj_type_t bind_type, map_type;
    hwloc_cpuset_t *map_domains;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status =
        get_hw_obj_list(binding, &bind_list, &total_bind_objs, &bind_count, &bind_obj_prefix,
                        &bind_type);
    HYDU_ERR_POP(status, "error finding bind objects\n");

    status =
        get_hw_obj_list(mapping, &map_list, &total_map_objs, &map_count, &map_obj_prefix,
                        &map_type);
    HYDU_ERR_POP(status, "error finding map objects\n");

    if ((bind_type != HWLOC_OBJ_OS_DEVICE && map_type == HWLOC_OBJ_OS_DEVICE) ||
        (bind_type == HWLOC_OBJ_OS_DEVICE && strcmp(bind_obj_prefix, map_obj_prefix)))
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                            "IO device binding policy \"%s\" does not support IO device mapping policy \"%s\"\n",
                            binding, mapping);

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
    num_pus_in_map_domain = (HYDT_topo_hwloc_info.total_num_pus / total_map_objs) * map_count;
    HYDU_ERR_CHKANDJUMP(status, num_pus_in_map_domain > HYDT_topo_hwloc_info.total_num_pus,
                        HYD_INTERNAL_ERROR, "mapping option \"%s\" larger than total system size\n",
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
    HYDU_MALLOC_OR_JUMP(map_domains, hwloc_bitmap_t *, total_map_domains * sizeof(hwloc_bitmap_t),
                        status);
    HYDU_MALLOC_OR_JUMP(start_pu, hwloc_obj_t *, total_map_domains * sizeof(hwloc_obj_t), status);

    /* For each map domain, find the next map object (first map object
     * for the first map domain) and add the following "map_count"
     * number of contiguous map objects, wrapping to the first one if
     * needed, to the map domain.  Store the first PU in the first map
     * object of the map domain as "start_pu".  This is needed later
     * for the actual binding. */
    for (i = 0; i < total_map_domains; i++) {
        map_domains[i] = hwloc_bitmap_alloc();
        hwloc_bitmap_zero(map_domains[i]);

        for (j = 0; j < map_count; j++) {
            map_obj = map_list[(i * map_count + j) % total_map_objs];
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
    num_pus_in_bind_domain = (HYDT_topo_hwloc_info.total_num_pus / total_bind_objs) * bind_count;

    if (num_pus_in_bind_domain < num_pus_in_map_domain) {
        for (i = 1; (i * num_pus_in_map_domain) % num_pus_in_bind_domain; i++);
        HYDT_topo_hwloc_info.num_bitmaps =
            (i * num_pus_in_map_domain * total_map_domains) / num_pus_in_bind_domain;
    } else {
        HYDT_topo_hwloc_info.num_bitmaps = total_map_domains;
    }

    /* initialize bitmaps */
    HYDU_MALLOC_OR_JUMP(HYDT_topo_hwloc_info.bitmap, hwloc_bitmap_t *,
                        HYDT_topo_hwloc_info.num_bitmaps * sizeof(hwloc_bitmap_t), status);

    for (i = 0; i < HYDT_topo_hwloc_info.num_bitmaps; i++) {
        HYDT_topo_hwloc_info.bitmap[i] = hwloc_bitmap_alloc();
        hwloc_bitmap_zero(HYDT_topo_hwloc_info.bitmap[i]);
    }

    /* do bindings */
    i = 0;
    while (i < HYDT_topo_hwloc_info.num_bitmaps) {
        for (j = 0; j < total_map_domains; j++) {
            int current_bind_index = 0;
            for (k = 0; k < total_bind_objs; k++) {
                if (hwloc_obj_is_in_subtree(topology, start_pu[j], bind_list[k])) {
                    bind_obj = bind_list[k];
                    current_bind_index = k;
                    break;
                }
            }

            for (k = 0; k < bind_count; k++) {
                hwloc_bitmap_or(HYDT_topo_hwloc_info.bitmap[i], HYDT_topo_hwloc_info.bitmap[i],
                                bind_obj->cpuset);

                /* if the binding is smaller than the mapping domain, wrap around inside that domain */
                if (num_pus_in_bind_domain < num_pus_in_map_domain) {
                    int count;
                    hwloc_obj_t obj_covering_domain =
                        hwloc_get_obj_covering_cpuset(topology, map_domains[j]);
                    for (count = (current_bind_index + 1) % total_bind_objs;
                         count != current_bind_index; count = (count + 1) % total_bind_objs) {
                        hwloc_obj_t temp_bind_obj = bind_list[count];
                        if (obj_covering_domain == temp_bind_obj ||
                            hwloc_obj_is_in_subtree(topology, temp_bind_obj, obj_covering_domain)) {
                            bind_obj = temp_bind_obj;
                            break;
                        }
                    }
                    current_bind_index = count;
                } else {
                    bind_obj = bind_list[(current_bind_index + 1) % total_bind_objs];
                    current_bind_index = (current_bind_index + 1) % total_bind_objs;
                }

            }
            i++;

            /* advance the starting position for this map domain, if needed */
            if (num_pus_in_bind_domain < num_pus_in_map_domain) {
                for (k = 0; k < num_pus_in_bind_domain; k++) {
                    start_pu[j] = hwloc_get_next_obj_inside_cpuset_by_type(topology, map_domains[j],
                                                                           HWLOC_OBJ_PU,
                                                                           start_pu[j]);
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
    MPL_free(map_list);
    MPL_free(bind_list);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYDT_topo_hwloc_init(const char *binding, const char *mapping, const char *membind)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_ASSERT(binding, status);

    hwloc_topology_init(&topology);
    hwloc_topology_set_io_types_filter(topology, HWLOC_TYPE_FILTER_KEEP_ALL);
    hwloc_topology_load(topology);

    HYDT_topo_hwloc_info.total_num_pus = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_PU);

    hwloc_initialized = 1;

    /* bindings that don't require mapping */
    if (!strncmp(binding, "user:", strlen("user:"))) {
        status = handle_user_binding(binding + strlen("user:"));
        HYDU_ERR_POP(status, "error binding to %s\n", binding);
        goto fn_exit;
    } else if (!strcmp(binding, "rr")) {
        status = handle_rr_binding();
        HYDU_ERR_POP(status, "error binding to %s\n", binding);
        goto fn_exit;
    }

    status = handle_bitmap_binding(binding, mapping ? mapping : binding);
    HYDU_ERR_POP(status, "error binding with bind \"%s\" and map \"%s\"\n", binding,
                 mapping ? mapping : "NULL");


    /* Memory binding options */
    if (membind == NULL)
        HYDT_topo_hwloc_info.membind = HWLOC_MEMBIND_DEFAULT;
    else if (!strcmp(membind, "firsttouch"))
        HYDT_topo_hwloc_info.membind = HWLOC_MEMBIND_FIRSTTOUCH;
    else if (!strcmp(membind, "nexttouch"))
        HYDT_topo_hwloc_info.membind = HWLOC_MEMBIND_NEXTTOUCH;
    else if (!strncmp(membind, "bind:", strlen("bind:"))) {
        HYDT_topo_hwloc_info.membind = HWLOC_MEMBIND_BIND;
    } else if (!strncmp(membind, "interleave:", strlen("interleave:"))) {
        HYDT_topo_hwloc_info.membind = HWLOC_MEMBIND_INTERLEAVE;
    } else {
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
    int id;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* For processes where the user did not specify a binding unit, no binding is needed. */
    if (!HYDT_topo_hwloc_info.user_binding || (idx < HYDT_topo_hwloc_info.num_bitmaps)) {
        id = idx % HYDT_topo_hwloc_info.num_bitmaps;

        if (HYDT_topo_info.debug) {
            /* Print the binding bitmaps for debugging. */
            int i;
            char *binding;

            HYDU_MALLOC_OR_JUMP(binding, char *, HYDT_topo_hwloc_info.total_num_pus + 1, status);
            memset(binding, '\0', HYDT_topo_hwloc_info.total_num_pus + 1);

            for (i = 0; i < HYDT_topo_hwloc_info.total_num_pus; i++) {
                if (hwloc_bitmap_isset(HYDT_topo_hwloc_info.bitmap[id], i))
                    *(binding + i) = '1';
                else
                    *(binding + i) = '0';
            }

            HYDU_dump_noprefix(stdout, "process %d binding: %s\n", idx, binding);
            MPL_free(binding);
        }
        hwloc_set_cpubind(topology, HYDT_topo_hwloc_info.bitmap[id], 0);
        hwloc_set_membind(topology, HYDT_topo_hwloc_info.bitmap[id],
                          HYDT_topo_hwloc_info.membind, 0);
    }

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

/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef TOPO_HWLOC_H_INCLUDED
#define TOPO_HWLOC_H_INCLUDED

#include <hwloc.h>
#include <assert.h>

struct HYDT_topo_hwloc_info {
    unsigned int num_bitmaps;
    hwloc_bitmap_t *bitmap;
    hwloc_membind_policy_t membind;
    int user_binding;
    int total_num_pus;
    char *xml_topology_file;
};
extern struct HYDT_topo_hwloc_info HYDT_topo_hwloc_info;

HYD_status HYDT_topo_hwloc_init(const char *binding, const char *mapping, const char *membind);
HYD_status HYDT_topo_hwloc_bind(int idx);
HYD_status HYDT_topo_hwloc_finalize(void);

#endif /* TOPO_HWLOC_H_INCLUDED */

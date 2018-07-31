/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2009 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 * Copyright © 2006-2011 Guillaume Mercier, Institut Polytechnique de
 * Bordeaux. All rights reserved. Permission is hereby granted to use,
 * reproduce, prepare derivative works, and to redistribute to others.
 */

#ifndef HYDRA_TOPO_HWLOC_H_INCLUDED
#define HYDRA_TOPO_HWLOC_H_INCLUDED

#include <hwloc.h>
#include <assert.h>

struct HYDI_topo_hwloc_info {
    int num_bitmaps;
    hwloc_bitmap_t *bitmap;
    hwloc_membind_policy_t membind;
    int user_binding;
    int total_num_pus;
};
extern struct HYDI_topo_hwloc_info HYDI_topo_hwloc_info;

HYD_status HYDI_topo_hwloc_init(const char *binding, const char *mapping, const char *membind);
HYD_status HYDI_topo_hwloc_bind(int idx);
HYD_status HYDI_topo_hwloc_finalize(void);

#endif /* HYDRA_TOPO_HWLOC_H_INCLUDED */

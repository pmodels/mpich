/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 * Copyright © 2006-2011 Guillaume Mercier, Institut Polytechnique de
 * Bordeaux. All rights reserved. Permission is hereby granted to use,
 * reproduce, prepare derivative works, and to redistribute to others.
 */

#ifndef TOPO_HWLOC_H_INCLUDED
#define TOPO_HWLOC_H_INCLUDED

#include <hwloc.h>
#include <assert.h>

HYD_status HYDT_topo_hwloc_init(HYDT_topo_support_level_t * support_level);
HYD_status HYDT_topo_hwloc_bind(struct HYDT_topo_cpuset_t cpuset);
HYD_status HYDT_topo_hwloc_finalize(void);

#endif /* TOPO_HWLOC_H_INCLUDED */

/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef BIND_HWLOC_H_INCLUDED
#define BIND_HWLOC_H_INCLUDED

#include <hwloc.h>
#include <assert.h>

HYD_status HYDT_bind_hwloc_init(HYDT_bind_support_level_t * support_level);
HYD_status HYDT_bind_hwloc_process(struct HYDT_bind_cpuset_t cpuset);

#endif /* BIND_HWLOC_H_INCLUDED */

/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */

#include "mpiimpl.h"

#ifdef HAVE_HWLOC
#include "hwloc.h"
#endif

int MPII_hwtopo_init(void)
{
}

int MPII_hwtopo_finalize(void)
{
}

bool MPIR_hwtopo_is_initialized(void)
{
}


MPIR_hwtopo_gid_t MPIR_hwtopo_get_leaf(void)
{
}

int MPIR_hwtopo_get_depth(MPIR_hwtopo_gid_t gid)
{
}

MPIR_hwtopo_gid_t MPIR_hwtopo_get_ancestor(MPIR_hwtopo_gid_t gid, int depth)
{
}


MPIR_hwtopo_type_e MPIR_hwtopo_get_type_id(const char *name)
{
}

MPIR_hwtopo_gid_t MPIR_hwtopo_get_obj_by_type(MPIR_hwtopo_type_e type)
{
}

MPIR_hwtopo_gid_t MPIR_hwtopo_get_obj_by_name(const char *name)
{
}

int MPIR_hwtopo_mem_bind(void *baseaddr, size_t len, MPIR_hwtopo_gid_t gid)
{
}


uint64_t MPIR_hwtopo_get_node_mem(void)
{
}

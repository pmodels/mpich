/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef BIND_H_INCLUDED
#define BIND_H_INCLUDED

#include "hydra_utils.h"

#define HYDT_TOPO_CHILD_ID(obj) \
    ((((char *) obj) - ((char *) obj->parent->children)) / sizeof(struct HYDT_topo_obj))

typedef enum {
    HYDT_BIND_NONE = 0,
    HYDT_BIND_BASIC,
    HYDT_BIND_TOPO,
    HYDT_BIND_MEMTOPO
} HYDT_bind_support_level_t;

typedef enum {
    HYDT_TOPO_MACHINE = 0, /* Cache-coherent set of processors */
    HYDT_TOPO_NODE, /* Sockets sharing memory dimms */
    HYDT_TOPO_SOCKET,
    HYDT_TOPO_CORE,
    HYDT_TOPO_THREAD,
    HYDT_TOPO_END /* The last element */
} HYDT_topo_obj_type_t;

struct HYDT_topo_obj {
    HYDT_topo_obj_type_t type;

    int os_index; /* OS index */

    struct HYDT_topo_obj *parent;

    int num_children;
    struct HYDT_topo_obj *children;

    /* Depth of the shared memory regions. This is a pointer to
     * accomodate multiple levels of memory shared by this set of
     * processing units. */
    int *shared_memory_depth;
};

struct HYDT_bind_info {
    HYDT_bind_support_level_t support_level;

    char *bindlib;
    int *bindmap;

    /* This is needed for all binding levels, except "NONE" */
    int total_proc_units;

    struct HYDT_topo_obj machine;
};

extern struct HYDT_bind_info HYDT_bind_info;

HYD_status HYDT_bind_init(char *binding, char *bindlib);
void HYDT_bind_finalize(void);
HYD_status HYDT_bind_process(int os_index);
int HYDT_bind_get_os_index(int process_id);

#endif /* BIND_H_INCLUDED */

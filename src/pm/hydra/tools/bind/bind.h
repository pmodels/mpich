/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef BIND_H_INCLUDED
#define BIND_H_INCLUDED

#include "hydra_utils.h"

typedef enum {
    HYDT_BIND_NONE = 0,
    HYDT_BIND_BASIC,
    HYDT_BIND_TOPO
} HYDT_bind_support_level_t;

struct HYDT_bind_info {
    HYDT_bind_support_level_t support_level;

    int num_procs;
    int num_sockets;
    int num_cores;
    int num_threads;

    int *bindmap;
    char *bindlib;

    struct HYDT_topology {
        int processor_id;

        int socket_rank;
        int socket_id;

        int core_rank;
        int core_id;

        int thread_rank;
        int thread_id;
    } *topology;
};

extern struct HYDT_bind_info HYDT_bind_info;

#endif /* BIND_H_INCLUDED */

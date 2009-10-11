/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef BIND_H_INCLUDED
#define BIND_H_INCLUDED

#include "hydra_utils.h"

typedef enum {
    HYDU_BIND_NONE = 0,
    HYDU_BIND_BASIC,
    HYDU_BIND_TOPO
} HYDU_bind_support_level_t;

struct HYDU_bind_info {
    HYDU_bind_support_level_t support_level;

    int num_procs;
    int num_sockets;
    int num_cores;
    int num_threads;

    int *bindmap;
    char *bindlib;

    struct HYDU_topology {
        int processor_id;

        int socket_rank;
        int socket_id;

        int core_rank;
        int core_id;

        int thread_rank;
        int thread_id;
    } *topology;
};

extern struct HYDU_bind_info HYDU_bind_info;

#endif /* BIND_H_INCLUDED */

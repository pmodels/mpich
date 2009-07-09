/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef BIND_H_INCLUDED
#define BIND_H_INCLUDED

#include "hydra_utils.h"

struct HYDU_bind_info {
    int supported;
    int num_procs;
    int num_sockets;
    int num_cores;

    int **bind_map;

    int user_bind_valid;
    int *user_bind_map;
};

extern struct HYDU_bind_info HYDU_bind_info;

#endif /* BIND_H_INCLUDED */

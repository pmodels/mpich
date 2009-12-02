/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef HYDRA_H_INCLUDED
#define HYDRA_H_INCLUDED

#include <stdio.h>
#include "hydra_base.h"
#include "hydra_utils.h"

struct HYD_handle {
    struct HYD_user_global user_global;

    char *base_path;

    char *css;
    char *rmk;

    int ckpoint_int;

    int print_rank_map;
    int print_all_exitcodes;

    int ranks_per_proc;

     HYD_status(*stdout_cb) (void *buf, int buflen);
     HYD_status(*stderr_cb) (void *buf, int buflen);

    /* Timeout (in seconds) is filled in by the UI to be passed to the
     * bootstrap server.
     *
     * FIXME: make this a function parameter.
     */
    int timeout;

    /* All of the available nodes */
    struct HYD_node *node_list;
    int global_core_count;

    /* Process groups */
    struct HYD_pg pg_list;

    /* Random parameters used for internal code */
    int func_depth;
};

extern struct HYD_handle HYD_handle;

#endif /* HYDRA_H_INCLUDED */

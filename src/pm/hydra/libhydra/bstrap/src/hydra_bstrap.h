/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef BSTRAP_SERVER_H_INCLUDED
#define BSTRAP_SERVER_H_INCLUDED

#include "hydra_base.h"
#include "hydra_node.h"
#include "hydra_hash.h"

int HYD_bstrap_query_avail(const char *bstrap);

HYD_status HYD_bstrap_setup(const char *path, const char *launcher, const char *launcher_exec,
                            int num_nodes, struct HYD_node *node_list, int my_proxy_id,
                            const char *port_range, char *const *const pmi_args, int pgid,
                            int *num_downstream, int *downstream_stdin,
                            struct HYD_int_hash **downstream_stdout_hash,
                            struct HYD_int_hash **downstream_stderr_hash,
                            struct HYD_int_hash **downstream_control_hash,
                            int **downstream_proxy_id, int **downstream_pid, int debug,
                            int tree_width);

HYD_status HYD_bstrap_finalize(const char *launcher);

#endif /* BSTRAP_SERVER_H_INCLUDED */

/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef SLURM_H_INCLUDED
#define SLURM_H_INCLUDED

#include "hydra.h"

HYD_status HYDT_bscd_slurm_launch_procs(int pgid, char **args, struct HYD_host *hosts,
                                        int num_hosts, int use_rmk, int k, int myid,
                                        int *control_fd);
HYD_status HYDT_bscd_slurm_query_proxy_id(int *proxy_id);
HYD_status HYDT_bscd_slurm_query_native_int(int *ret);
HYD_status HYDT_bscd_slurm_query_node_list(struct HYD_node **node_list);
HYD_status HYDT_bscd_slurm_query_env_inherit(const char *env_name, int *ret);

#endif /* SLURM_H_INCLUDED */

/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef SLURM_H_INCLUDED
#define SLURM_H_INCLUDED

#include "hydra.h"

HYD_status HYDT_bscd_slurm_launch_procs(char **args, struct HYD_proxy **proxy_list, int num_hosts,
                                        int use_rmk, int *control_fd);
HYD_status HYDT_bscd_slurm_query_proxy_id(int *proxy_id);
HYD_status HYDT_bscd_slurm_query_native_int(int *ret);
HYD_status HYDT_bscd_slurm_query_node_list(struct HYD_node **node_list);
HYD_status HYDT_bscd_slurm_query_env_inherit(const char *env_name, int *should_inherit);

#endif /* SLURM_H_INCLUDED */

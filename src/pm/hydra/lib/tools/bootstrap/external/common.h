/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef COMMON_H_INCLUDED
#define COMMON_H_INCLUDED

#include "ssh.h"
#include "rsh.h"
#include "slurm.h"
#include "lsf.h"
#include "ll.h"
#include "sge.h"
#include "pbs.h"
#include "cobalt.h"

int HYDTI_bscd_env_is_avail(const char *env_name);
int HYDTI_bscd_in_env_list(const char *env_name, const char *env_list[]);

HYD_status HYDT_bscd_common_launch_procs(int pgid, char **args, struct HYD_host *hosts,
                                         int num_hosts, int use_rmk, int k, int myid,
                                         int *control_fd);

#endif /* COMMON_H_INCLUDED */

/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef EXTERNAL_H_INCLUDED
#define EXTERNAL_H_INCLUDED

#include "ssh.h"
#include "slurm.h"
#include "lsf.h"
#include "ll.h"
#include "sge.h"
#include "pbs.h"

HYD_status HYDT_bscd_external_launch_procs(char **args, struct HYD_node *node_list,
                                           int *control_fd);
HYD_status HYDT_bscd_external_launcher_finalize(void);
HYD_status HYDT_bscd_external_query_env_inherit(const char *env_name, int *ret);
HYD_status HYDT_bscd_external_query_native_int(int *ret);
HYD_status HYDT_bscd_external_query_jobid(char **jobid);

#endif /* EXTERNAL_H_INCLUDED */

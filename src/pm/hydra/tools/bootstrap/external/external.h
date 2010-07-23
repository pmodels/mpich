/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef EXTERNAL_H_INCLUDED
#define EXTERNAL_H_INCLUDED

#include "ssh.h"
#include "slurm.h"

HYD_status HYDT_bscd_external_launch_procs(char **args, struct HYD_node *node_list,
                                           int *control_fd, int enable_stdin,
                                           HYD_status(*stdout_cb) (void *buf, int buflen),
                                           HYD_status(*stderr_cb) (void *buf, int buflen));
HYD_status HYDT_bscd_external_finalize(void);
HYD_status HYDT_bscd_external_query_env_inherit(const char *env_name, int *ret);

#endif /* EXTERNAL_H_INCLUDED */

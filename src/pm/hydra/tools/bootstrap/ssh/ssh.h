/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef SSH_H_INCLUDED
#define SSH_H_INCLUDED

#include "hydra_base.h"

HYD_status HYDT_bscd_ssh_launch_procs(char **args, struct HYD_node *node_list,
                                      HYD_status(*stdout_cb) (void *buf, int buflen),
                                      HYD_status(*stderr_cb) (void *buf, int buflen));

#endif /* SSH_H_INCLUDED */

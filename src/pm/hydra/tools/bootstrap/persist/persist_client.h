/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2009 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef PERSIST_CLIENT_H_INCLUDED
#define PERSIST_CLIENT_H_INCLUDED

#include "hydra.h"
#include "bscu.h"
#include "persist.h"

HYD_status HYDT_bscd_persist_launch_procs(char **args, struct HYD_proxy *proxy_list,
                                          int use_rmk, int *control_fd);
HYD_status HYDT_bscd_persist_wait_for_completion(int timeout);

extern int *HYDT_bscd_persist_control_fd;
extern int HYDT_bscd_persist_node_count;

#endif /* PERSIST_CLIENT_H_INCLUDED */

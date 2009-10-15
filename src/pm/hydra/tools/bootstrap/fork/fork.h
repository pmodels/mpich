/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef FORK_H_INCLUDED
#define FORK_H_INCLUDED

#include "hydra_base.h"

HYD_Status HYD_BSCD_fork_launch_procs(char **global_args, const char *proxy_id_str,
                                      struct HYD_Proxy *proxy_list);

#endif /* FORK_H_INCLUDED */

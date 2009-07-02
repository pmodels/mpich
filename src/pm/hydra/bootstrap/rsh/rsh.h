/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef RSH_H_INCLUDED
#define RSH_H_INCLUDED

#include "hydra_base.h"

HYD_Status HYD_BSCD_rsh_launch_procs(char **global_args, const char *partition_id_str,
                                     struct HYD_Partition *partition_list);

#endif /* RSH_H_INCLUDED */

/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef SLURM_H_INCLUDED
#define SLURM_H_INCLUDED

#include "hydra_base.h"

HYD_Status HYD_BSCD_slurm_launch_procs(char **global_args, char *partition_id_str);

#endif /* SLURM_H_INCLUDED */

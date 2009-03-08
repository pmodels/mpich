/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef HYDRA_LAUNCH_H_INCLUDED
#define HYDRA_LAUNCH_H_INCLUDED

#include "hydra.h"

HYD_Status HYDU_Append_env(HYD_Env_t * env_list, char **client_arg, int id);
HYD_Status HYDU_Append_exec(char **exec, char **client_arg);
HYD_Status HYDU_Append_wdir(char **client_arg);
HYD_Status HYDU_Allocate_Partition(struct HYD_Partition_list **partition);
HYD_Status HYDU_Create_process(char **client_arg, int *in, int *out, int *err, int *pid);

#endif /* HYDRA_LAUNCH_H_INCLUDED */

/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef HYDRA_TOOLS_H_INCLUDED
#define HYDRA_TOOLS_H_INCLUDED

#include "hydra_base.h"

/* bind */
HYD_Status HYDU_bind_init(char *binding, char *bindlib);
void HYDU_bind_finalize(void);
HYD_Status HYDU_bind_process(int core);
int HYDU_bind_get_core_id(int id);

/* checkpointing */
HYD_Status HYDU_ckpoint_init(char *ckpointlib, char *ckpoint_prefix);
HYD_Status HYDU_ckpoint_suspend(void);
HYD_Status HYDU_ckpoint_restart(HYD_Env_t * envlist, int num_ranks, int ranks[], int *in,
                                int *out, int *err);

#endif /* HYDRA_TOOLS_H_INCLUDED */

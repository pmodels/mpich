/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef HYDRA_TOOLS_H_INCLUDED
#define HYDRA_TOOLS_H_INCLUDED

#include "hydra_base.h"

/* bind */
HYD_status HYDT_bind_init(char *binding, char *bindlib);
void HYDT_bind_finalize(void);
HYD_status HYDT_bind_process(int os_index);
int HYDT_bind_get_os_index(int process_id);

/* checkpointing */
HYD_status HYDT_ckpoint_init(char *ckpointlib, char *ckpoint_prefix);
HYD_status HYDT_ckpoint_suspend(void);
HYD_status HYDT_ckpoint_restart(HYD_env_t * envlist, int num_ranks, int ranks[], int *in,
                                int *out, int *err);

#endif /* HYDRA_TOOLS_H_INCLUDED */

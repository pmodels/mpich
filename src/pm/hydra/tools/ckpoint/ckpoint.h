/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef CKPOINT_H_INCLUDED
#define CKPOINT_H_INCLUDED

#include "hydra_utils.h"

struct HYDT_ckpoint_info {
    char *ckpointlib;
    char *ckpoint_prefix;
};

extern struct HYDT_ckpoint_info HYDT_ckpoint_info;

HYD_status HYDT_ckpoint_init(char *ckpointlib, char *ckpoint_prefix);
HYD_status HYDT_ckpoint_suspend(void);
HYD_status HYDT_ckpoint_restart(HYD_env_t * envlist, int num_ranks, int ranks[], int *in,
                                int *out, int *err);

#endif /* CKPOINT_H_INCLUDED */

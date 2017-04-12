/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef HYDRA_SPAWN_H_INCLUDED
#define HYDRA_SPAWN_H_INCLUDED

#include "hydra_base.h"
#include "hydra_env.h"

HYD_status HYD_spawn(char **client_arg, int envcount, char *const *const env, int *in, int *out,
                     int *err, int *pid, int idx);

#endif /* HYDRA_SPAWN_H_INCLUDED */

/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef HYDRA_ENV_H_INCLUDED
#define HYDRA_ENV_H_INCLUDED

#include "hydra_base.h"

struct HYD_env {
    char *env_name;
    char *env_value;
    struct HYD_env *next;
};

HYD_status HYD_env_to_str(struct HYD_env *env, char **str);
HYD_status HYD_env_list_inherited(struct HYD_env **env_list);
HYD_status HYD_env_create(struct HYD_env **env, const char *env_name, const char *env_value);
HYD_status HYD_env_free(struct HYD_env *env);
HYD_status HYD_env_free_list(struct HYD_env *env);
HYD_status HYD_env_append_to_list(const char *env_name, const char *env_value,
                                  struct HYD_env **env_list);
HYD_status HYD_env_put(struct HYD_env *env);
HYD_status HYD_env_put_list(struct HYD_env *env_list);

#endif /* HYDRA_ENV_H_INCLUDED */

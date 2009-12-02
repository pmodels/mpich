/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef UIU_H_INCLUDED
#define UIU_H_INCLUDED

#include "hydra.h"

struct HYD_uiu_exec_info {
    int process_count;
    char *exec[HYD_NUM_TMP_STRINGS];

    /* Local environment */
    HYD_env_t *user_env;
    char *env_prop;

    struct HYD_uiu_exec_info *next;
};

extern struct HYD_uiu_exec_info *HYD_uiu_exec_info_list;

void HYD_uiu_init_params(void);
void HYD_uiu_free_params(void);
HYD_status HYD_uiu_create_proxy_list(void);
HYD_status HYD_uiu_get_current_exec_info(struct HYD_uiu_exec_info **info);
void HYD_uiu_print_params(void);
HYD_status HYD_uiu_alloc_exec_info(struct HYD_uiu_exec_info **exec_info);
void HYD_uiu_free_exec_info_list(struct HYD_uiu_exec_info *exec_info_list);

#endif /* UIU_H_INCLUDED */

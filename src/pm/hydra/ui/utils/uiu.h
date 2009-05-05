/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef UIU_H_INCLUDED
#define UIU_H_INCLUDED

#include "hydra.h"

void HYD_UIU_init_params(void);
void HYD_UIU_free_params(void);
HYD_Status HYD_UIU_merge_exec_info_to_partition(void);
HYD_Status HYD_UIU_create_env_list(void);
HYD_Status HYD_UIU_get_current_exec_info(struct HYD_Exec_info **info);
void HYD_UIU_print_params(void);

#endif /* UIU_H_INCLUDED */

/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef UIU_H_INCLUDED
#define UIU_H_INCLUDED

#include "hydra.h"

void HYD_uiu_init_params(void);
void HYD_uiu_free_params(void);
HYD_status HYD_uiu_create_proxy_list(void);
HYD_status HYD_uiu_get_current_exec_info(struct HYD_exec_info **info);
void HYD_uiu_print_params(void);

#endif /* UIU_H_INCLUDED */

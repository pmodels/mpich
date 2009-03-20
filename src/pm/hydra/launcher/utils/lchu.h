/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef LCHU_H_INCLUDED
#define LCHU_H_INCLUDED

#include "hydra.h"

void HYD_LCHU_init_params(void);
void HYD_LCHU_free_params(void);
void HYD_LCHU_free_proc_params(void);
HYD_Status HYD_LCHU_create_host_list(void);
HYD_Status HYD_LCHU_create_env_list(void);
void HYD_LCHU_print_params(void);
HYD_Status HYD_LCHU_allocate_proc_params(struct HYD_Proc_params **params);
HYD_Status HYD_LCHU_get_current_proc_params(struct HYD_Proc_params **params);

#endif /* LCHU_H_INCLUDED */

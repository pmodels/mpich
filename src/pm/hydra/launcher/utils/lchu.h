/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef LCHU_H_INCLUDED
#define LCHU_H_INCLUDED

#include "hydra.h"

void HYD_LCHU_Init_params(void);
void HYD_LCHU_Free_params(void);
void HYD_LCHU_Free_proc_params(void);
HYD_Status HYD_LCHU_Create_host_list(void);
HYD_Status HYD_LCHU_Create_env_list(void);

#endif /* LCHU_H_INCLUDED */

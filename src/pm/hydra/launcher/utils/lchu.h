/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef LCHU_H_INCLUDED
#define LCHU_H_INCLUDED

#include "hydra.h"

HYD_Status HYD_LCHU_Create_host_list(void);
HYD_Status HYD_LCHU_Free_host_list(void);
HYD_Status HYD_LCHU_Create_env_list(void);
HYD_Status HYD_LCHU_Free_env_list(void);
HYD_Status HYD_LCHU_Free_io(void);
HYD_Status HYD_LCHU_Free_exec(void);
HYD_Status HYD_LCHU_Free_proc_params(void);

#endif /* LCHU_H_INCLUDED */

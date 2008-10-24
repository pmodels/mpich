/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef CSIU_H_INCLUDED
#define CSIU_H_INCLUDED

#include "csi.h"

int HYD_CSU_Time_left(void);
int HYD_CSU_Free_env_list(HYD_CSI_Env_t * env_list);
int HYD_CSU_Free_exec_list(HYD_CSI_Exec_t * exec_list);

#endif /* CSIU_H_INCLUDED */

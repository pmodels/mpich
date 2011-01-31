/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef UIU_H_INCLUDED
#define UIU_H_INCLUDED

#include "hydra_server.h"

void HYD_uiu_init_params(void);
void HYD_uiu_free_params(void);
void HYD_uiu_print_params(void);
HYD_status HYD_uiu_stdout_cb(int pgid, int proxy_id, int rank, void *buf, int buflen);
HYD_status HYD_uiu_stderr_cb(int pgid, int proxy_id, int rank, void *buf, int buflen);

#endif /* UIU_H_INCLUDED */

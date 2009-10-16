/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPIEXEC_H_INCLUDED
#define MPIEXEC_H_INCLUDED

#include "hydra.h"

HYD_status HYD_uii_mpx_add_to_proxy_list(char *hostname, int num_procs);
HYD_status HYD_uii_mpx_get_parameters(char **t_argv);
HYD_status HYD_uii_mpx_stdout_cb(int fd, HYD_event_t events, void *userp);
HYD_status HYD_uii_mpx_stderr_cb(int fd, HYD_event_t events, void *userp);
HYD_status HYD_uii_mpx_stdin_cb(int fd, HYD_event_t events, void *userp);

#endif /* MPIEXEC_H_INCLUDED */

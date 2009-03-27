/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPIEXEC_H_INCLUDED
#define MPIEXEC_H_INCLUDED

#include "hydra.h"

HYD_Status HYD_LCHI_get_parameters(char **t_argv);
HYD_Status HYD_LCHI_stdout_cb(int fd, HYD_Event_t events, void *userp);
HYD_Status HYD_LCHI_stderr_cb(int fd, HYD_Event_t events, void *userp);
HYD_Status HYD_LCHI_stdin_cb(int fd, HYD_Event_t events, void *userp);

#endif /* MPIEXEC_H_INCLUDED */

/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPIEXEC_H_INCLUDED
#define MPIEXEC_H_INCLUDED

#include "hydra.h"
#include "csi.h"

HYD_Status HYD_LCHI_Get_parameters(int t_argc, char **t_argv);
HYD_Status HYD_LCHI_stdout_cb(int fd, HYD_CSI_Event_t events);
HYD_Status HYD_LCHI_stderr_cb(int fd, HYD_CSI_Event_t events);
HYD_Status HYD_LCHI_stdin_cb(int fd, HYD_CSI_Event_t events);

#endif /* MPIEXEC_H_INCLUDED */

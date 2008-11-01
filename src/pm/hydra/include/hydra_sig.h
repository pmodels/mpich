/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef HYDRA_SIG_H_INCLUDED
#define HYDRA_SIG_H_INCLUDED

#include "hydra.h"
#include <signal.h>
#include <sys/wait.h>

typedef void (*sighandler_t) (int);

HYD_Status HYDU_Set_signal(int signum, sighandler_t handler);

#endif /* HYDRA_SIG_H_INCLUDED */

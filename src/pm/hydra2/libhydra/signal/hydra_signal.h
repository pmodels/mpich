/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef HYDRA_SIGNAL_H_INCLUDED
#define HYDRA_SIGNAL_H_INCLUDED

#include "hydra_base.h"

HYD_status HYD_signal_set(int signum, void (*handler) (int));
HYD_status HYD_signal_set_common(void (*handler) (int));

#endif /* HYDRA_SIGNAL_H_INCLUDED */

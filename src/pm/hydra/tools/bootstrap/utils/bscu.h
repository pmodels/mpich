/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef BSCU_H_INCLUDED
#define BSCU_H_INCLUDED

#include "hydra.h"
#include "demux.h"

extern int *HYD_bscu_fd_list;
extern int HYD_bscu_fd_count;
extern int *HYD_bscu_pid_list;
extern int HYD_bscu_pid_count;

HYD_status HYDT_bscu_wait_for_completion(int timeout);
HYD_status HYDT_bscu_stdio_cb(int fd, HYD_event_t events, void *userp);

#endif /* BSCU_H_INCLUDED */

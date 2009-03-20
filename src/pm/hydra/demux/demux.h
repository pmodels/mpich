/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef DEMUX_H_INCLUDED
#define DEMUX_H_INCLUDED

#include "hydra.h"

HYD_Status HYD_DMX_register_fd(int num_fds, int *fd, HYD_Event_t events,
                               HYD_Status(*callback) (int fd, HYD_Event_t events));
HYD_Status HYD_DMX_deregister_fd(int fd);
HYD_Status HYD_DMX_wait_for_event(int time);
HYD_Status HYD_DMX_finalize(void);

#endif /* DEMUX_H_INCLUDED */

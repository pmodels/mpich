/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef DEMUX_H_INCLUDED
#define DEMUX_H_INCLUDED

#include "hydra.h"

HYD_status HYDT_dmx_register_fd(int num_fds, int *fd, HYD_event_t events, void *userp,
                                HYD_status(*callback) (int fd, HYD_event_t events,
                                                       void *userp));
HYD_status HYDT_dmx_deregister_fd(int fd);
HYD_status HYDT_dmx_wait_for_event(int time);
HYD_status HYDT_dmx_finalize(void);

#endif /* DEMUX_H_INCLUDED */

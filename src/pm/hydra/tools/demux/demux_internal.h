/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2009 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef DEMUX_INTERNAL_H_INCLUDED
#define DEMUX_INTERNAL_H_INCLUDED

#include "hydra.h"

struct HYDT_dmxu_callback {
    int num_fds;
    int *fd;
    HYD_event_t events;
    void *userp;
     HYD_status(*callback) (int fd, HYD_event_t events, void *userp);

    struct HYDT_dmxu_callback *next;
};

extern int HYDT_dmxu_num_cb_fds;
extern struct HYDT_dmxu_callback *HYDT_dmxu_cb_list;
extern int HYDT_dmxu_got_sigttin;

struct HYDT_dmxu_fns {
    HYD_status(*wait_for_event) (int wtime);
    HYD_status(*stdin_valid) (int *out);
};

HYD_status HYDT_dmxi_stdin_valid(int *out);

#if defined HAVE_POLL
HYD_status HYDT_dmxu_poll_wait_for_event(int wtime);
HYD_status HYDT_dmxu_poll_stdin_valid(int *out);
#endif /* HAVE_POLL */

#if defined HAVE_SELECT
HYD_status HYDT_dmxu_select_wait_for_event(int wtime);
HYD_status HYDT_dmxu_select_stdin_valid(int *out);
#endif /* HAVE_SELECT */

#endif /* DEMUX_INTERNAL_H_INCLUDED */

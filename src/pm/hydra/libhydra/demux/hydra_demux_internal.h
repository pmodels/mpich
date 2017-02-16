/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef DEMUX_INTERNAL_H_INCLUDED
#define DEMUX_INTERNAL_H_INCLUDED

#include "hydra_demux.h"
#include "mpl_uthash.h"

struct HYDI_dmx_callback {
    int fd;
    HYD_dmx_event_t events;
    void *userp;
     HYD_status(*callback) (int fd, HYD_dmx_event_t events, void *userp);
    MPL_UT_hash_handle hh;
};

extern int HYDI_dmx_num_cb_fds;
extern struct HYDI_dmx_callback *HYDI_dmx_cb_list;
extern int HYDI_dmx_got_sigttin;

HYD_status HYD_dmxi_stdin_valid(int *out);

#if defined HAVE_POLL
HYD_status HYDI_dmx_poll_wait_for_event(int wtime);
#endif /* HAVE_POLL */

#if defined HAVE_SELECT
HYD_status HYDI_dmx_select_wait_for_event(int wtime);
#endif /* HAVE_SELECT */

#endif /* DEMUX_INTERNAL_H_INCLUDED */

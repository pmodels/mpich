/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef CENTRAL_H_INCLUDED
#define CENTRAL_H_INCLUDED

/* Currently we only have one command */
enum HYD_Proxy_cmds {
    KILLALL_PROCS
};

extern int HYD_PMCD_Central_listenfd;

HYD_Status HYD_PMCD_Central_cb(int fd, HYD_Event_t events);

#endif /* CENTRAL_H_INCLUDED */

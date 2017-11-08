/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2009 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef SSH_H_INCLUDED
#define SSH_H_INCLUDED

#include "hydra.h"

/* Modern sshd servers don't like more than a certain number of ssh
 * connections from the same IP address per minute. If we exceed that,
 * the server assumes it's a hack-in attack, and does not accept any
 * more connections. So, we limit the number of ssh connections. */
#define HYDRA_LAUNCHER_SSH_DEFAULT_LIMIT 8
#define HYDRA_LAUNCHER_SSH_DEFAULT_LIMIT_TIME 15

#define older(a,b) \
    ((((a).tv_sec < (b).tv_sec) ||                                      \
      (((a).tv_sec == (b).tv_sec) && ((a).tv_usec < (b).tv_usec))) ? 1 : 0)

struct HYDT_bscd_ssh_time {
    char *hostname;
    struct timeval *init_time;
    struct HYDT_bscd_ssh_time *next;
};

extern int HYDT_bscd_ssh_limit;
extern int HYDT_bscd_ssh_limit_time;
extern int HYDT_bscd_ssh_warnings;
extern struct HYDT_bscd_ssh_time *HYDT_bscd_ssh_time;

HYD_status HYDTI_bscd_ssh_store_launch_time(char *hostname);

HYD_status HYDT_bscd_ssh_query_env_inherit(const char *env_name, int *ret);
HYD_status HYDT_bscd_ssh_launcher_finalize(void);

#endif /* SSH_H_INCLUDED */

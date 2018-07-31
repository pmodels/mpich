/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2009 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef SSH_INTERNAL_H_INCLUDED
#define SSH_INTERNAL_H_INCLUDED

#include "hydra_base.h"

/* Modern sshd servers don't like more than a certain number of ssh
 * connections from the same IP address per minute. If we exceed that,
 * the server assumes it's a hack-in attack, and does not accept any
 * more connections. So, we limit the number of ssh connections. */
#define HYDI_BSTRAP_SSH_DEFAULT_LIMIT 8
#define HYDI_BSTRAP_SSH_DEFAULT_LIMIT_TIME 15

#define older(a,b) \
    ((((a).tv_sec < (b).tv_sec) ||                                      \
      (((a).tv_sec == (b).tv_sec) && ((a).tv_usec < (b).tv_usec))) ? 1 : 0)

extern int HYDI_ssh_limit;
extern int HYDI_ssh_limit_time;
extern int HYDI_ssh_warnings;

HYD_status HYDI_ssh_store_launch_time(const char *hostname);
HYD_status HYDI_ssh_free_launch_elements(void);

#endif /* SSH_INTERNAL_H_INCLUDED */

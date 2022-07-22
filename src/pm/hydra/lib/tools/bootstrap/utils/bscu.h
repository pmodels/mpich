/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef BSCU_H_INCLUDED
#define BSCU_H_INCLUDED

#include "hydra.h"
#include "demux.h"

struct HYD_proxy_pid {
    struct HYD_proxy *proxy;
    int pid;
};

extern int *HYD_bscu_fd_list;
extern int HYD_bscu_fd_count;

HYD_status HYDT_bscu_pid_list_grow(int add_count);
void HYDT_bscu_pid_list_push(struct HYD_proxy *proxy, int pid);
struct HYD_proxy_pid *HYDT_bscu_pid_list_find(int pid);
HYD_status HYDT_bscu_wait_for_completion(int timeout);
HYD_status HYDT_bscu_stdio_cb(int fd, HYD_event_t events, void *userp);

#endif /* BSCU_H_INCLUDED */

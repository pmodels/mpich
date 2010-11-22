/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef BSCU_H_INCLUDED
#define BSCU_H_INCLUDED

#include "hydra_base.h"
#include "demux.h"

extern int *HYD_bscu_fd_list;
extern int HYD_bscu_fd_count;
extern int *HYD_bscu_pid_list;
extern int HYD_bscu_pid_count;

HYD_status HYDT_bscu_finalize(void);
HYD_status HYDT_bscu_query_node_list(struct HYD_node **node_list);
HYD_status HYDT_bscu_query_usize(int *size);
HYD_status HYDT_bscu_query_proxy_id(int *proxy_id);
HYD_status HYDT_bscu_wait_for_completion(int timeout);
HYD_status HYDT_bscu_query_env_inherit(const char *env_name, int *ret);
HYD_status HYDT_bscu_cleanup_procs(void);
HYD_status HYDT_bscu_stdio_cb(int fd, HYD_event_t events, void *userp);
HYD_status HYDT_bscu_query_native_int(int *ret);

#endif /* BSCU_H_INCLUDED */

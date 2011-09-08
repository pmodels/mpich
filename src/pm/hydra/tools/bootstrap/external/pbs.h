/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef PBS_H_INCLUDED
#define PBS_H_INCLUDED

#include "hydra.h"

#if defined(HAVE_TM_H)
#include "tm.h"

struct HYDT_bscd_pbs_sys {
    int spawn_count;
    tm_event_t *spawn_events;
    tm_task_id *task_id;        /* Array of TM task(process) IDs */
};

extern struct HYDT_bscd_pbs_sys *HYDT_bscd_pbs_sys;

HYD_status HYDT_bscd_pbs_launch_procs(char **args, struct HYD_proxy *proxy_list,
                                      int *control_fd);
HYD_status HYDT_bscd_pbs_query_env_inherit(const char *env_name, int *ret);
HYD_status HYDT_bscd_pbs_wait_for_completion(int timeout);
HYD_status HYDT_bscd_pbs_launcher_finalize(void);
#endif /* if defined(HAVE_TM_H) */

HYD_status HYDT_bscd_pbs_query_native_int(int *ret);
HYD_status HYDT_bscd_pbs_query_node_list(struct HYD_node **node_list);
HYD_status HYDT_bscd_pbs_query_jobid(char **jobid);

#endif /* PBS_H_INCLUDED */

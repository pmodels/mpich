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

#define HYDT_PBS_STRLEN 160
struct HYDT_bscd_pbs_node {
    tm_node_id    id;
    char          name[HYDT_PBS_STRLEN];
};

struct HYDT_bscd_pbs_sys {
    struct tm_roots            tm_root;
    int                        spawned_count;
    int                        size;
    tm_task_id                *taskIDs;     /* Array of TM task(process) IDs */
    tm_event_t                *events;      /* Array of TM event IDs */
    int                       *taskobits;   /* Array of TM task exit codes */
    int                        num_nodes;   /* Number of elements in nodes[] */
    struct HYDT_bscd_pbs_node *nodes;       /* Array of pbs_nodes[] */
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

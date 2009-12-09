/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef PM_UTILS_H_INCLUDED
#define PM_UTILS_H_INCLUDED

/* The set of commands supported */
enum HYD_pmu_cmd {
    PROC_INFO,
    KILL_JOB,
    PROXY_SHUTDOWN,
    CKPOINT
};

HYD_status HYD_pmu_send_exec_info(struct HYD_proxy *proxy);

#endif /* PM_UTILS_H_INCLUDED */

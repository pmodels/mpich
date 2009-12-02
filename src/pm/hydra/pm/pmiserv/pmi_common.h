/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef PMI_COMMON_H_INCLUDED
#define PMI_COMMON_H_INCLUDED

/* The set of commands supported */
enum HYD_pmcd_pmi_proxy_cmds {
    PROC_INFO,
    KILL_JOB,
    PROXY_SHUTDOWN,
    CKPOINT
};

/* FIXME: This structure only provides the PMI_ID, as we currently
 * only support single job environments with no dynamic
 * processes. When there are multiple jobs or dynamic processes, we
 * will need a process group ID as well. */
struct HYD_pmcd_pmi_header {
    int pmi_id;
};

#endif /* PMI_COMMON_H_INCLUDED */

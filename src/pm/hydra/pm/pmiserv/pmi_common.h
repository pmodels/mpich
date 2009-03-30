/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef PMI_COMMON_H_INCLUDED
#define PMI_COMMON_H_INCLUDED

/* The set of commands supported */
enum HYD_PMCD_pmi_proxy_cmds {
    PROC_INFO,
    KILL_JOB,
    PROXY_SHUTDOWN,
    USE_AS_STDOUT,
    USE_AS_STDERR,
    USE_AS_STDIN
};

#endif /* PMI_COMMON_H_INCLUDED */

/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef HYDRA_H_INCLUDED
#define HYDRA_H_INCLUDED

#include <stdio.h>
#include "hydra_base.h"
#include "hydra_utils.h"

struct HYD_Handle_ {
    char *base_path;
    int proxy_port;
    char *boot_server;

    int debug;
    int enablex;
    char *wdir;
    char *host_file;

    /* Global environment */
    HYD_Env_t *global_env;
    HYD_Env_t *system_env;
    HYD_Env_t *user_env;
    HYD_Env_prop_t prop;
    HYD_Env_t *prop_env;

    int in;
     HYD_Status(*stdin_cb) (int fd, HYD_Event_t events);

    /* Start time and timeout. These are filled in by the launcher,
     * but are utilized by the demux engine and the boot-strap server
     * to decide how long we need to wait for. */
    HYD_Time start;
    HYD_Time timeout;

    /* Each structure will contain all hosts/cores that use the same
     * executable and environment. */
    struct HYD_Proc_params {
        int exec_proc_count;
        char *exec[HYD_EXEC_ARGS];

        struct HYD_Partition_list *partition;

        /* Local environment */
        HYD_Env_t *user_env;
        HYD_Env_prop_t prop;
        HYD_Env_t *prop_env;

        /* Callback functions for the stdout/stderr events. These can
         * be the same. */
         HYD_Status(*stdout_cb) (int fd, HYD_Event_t events);
         HYD_Status(*stderr_cb) (int fd, HYD_Event_t events);

        struct HYD_Proc_params *next;
    } *proc_params;

    /* Random parameters used for internal code */
    int func_depth;
    char stdin_tmp_buf[HYD_TMPBUF_SIZE];
    int stdin_buf_offset;
    int stdin_buf_count;
};

typedef struct HYD_Handle_ HYD_Handle;

/* We'll use this as the central handle that has most of the
 * information needed by everyone. All data to be written has to be
 * done before the HYD_CSI_Wait_for_completion() function is called,
 * except for two exceptions:
 *
 * 1. The timeout value is initially added by the launcher before the
 * HYD_CSI_Wait_for_completion() function is called, but can be edited
 * by the control system within this call. There's no guarantee on
 * what value it will contain for the other layers.
 *
 * 2. There is no guarantee on what the exit status will contain till
 * the HYD_CSI_Wait_for_completion() function returns (where the
 * bootstrap server can fill out these values).
 */
extern HYD_Handle handle;

#endif /* HYDRA_H_INCLUDED */

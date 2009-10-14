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
    struct HYD_User_global user_global;

    char *base_path;
    int proxy_port;

    char *css;
    char *rmk;

    int ckpoint_int;

    int print_rank_map;
    int print_all_exitcodes;
    int pm_env;

    int ranks_per_proc;

     HYD_Status(*stdin_cb) (int fd, HYD_Event_t events, void *userp);
     HYD_Status(*stdout_cb) (int fd, HYD_Event_t events, void *userp);
     HYD_Status(*stderr_cb) (int fd, HYD_Event_t events, void *userp);

    /* Start time and timeout. These are filled in by the launcher,
     * but are utilized by the demux engine and the boot-strap server
     * to decide how long we need to wait for. */
    HYD_Time start;
    HYD_Time timeout;

    struct HYD_Proxy *proxy_list;
    int global_core_count;

    struct HYD_Exec_info *exec_info_list;
    int global_process_count;

    /* Random parameters used for internal code */
    int func_depth;
    char stdin_tmp_buf[HYD_TMPBUF_SIZE];
    int stdin_buf_offset;
    int stdin_buf_count;
};

typedef struct HYD_Handle_ HYD_Handle;

extern HYD_Handle HYD_handle;

#endif /* HYDRA_H_INCLUDED */

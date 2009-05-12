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

#define FORALL_ACTIVE_PARTITIONS(partition, partition_list)    \
    for ((partition) = (partition_list); (partition) && (partition)->base->active; \
         (partition) = (partition)->next)

#define FORALL_PARTITIONS(partition, partition_list)    \
    for ((partition) = (partition_list); (partition); (partition) = (partition)->next)

struct HYD_Handle_ {
    char *base_path;
    int proxy_port;
    HYD_Launch_mode_t launch_mode;

    char *bootstrap;
    char *css;
    char *rmk;
    HYD_Binding binding;
    char *user_bind_map;

    int debug;
    int print_rank_map;
    int print_all_exitcodes;
    int enablex;
    int pm_env;
    char *wdir;
    char *host_file;

    int ranks_per_proc;
    char *bootstrap_exec;

    /* Global environment */
    HYD_Env_t *global_env;
    HYD_Env_t *system_env;
    HYD_Env_t *user_env;
    HYD_Env_prop_t prop;
    HYD_Env_t *prop_env;

     HYD_Status(*stdin_cb) (int fd, HYD_Event_t events, void *userp);
     HYD_Status(*stdout_cb) (int fd, HYD_Event_t events, void *userp);
     HYD_Status(*stderr_cb) (int fd, HYD_Event_t events, void *userp);

    /* Start time and timeout. These are filled in by the launcher,
     * but are utilized by the demux engine and the boot-strap server
     * to decide how long we need to wait for. */
    HYD_Time start;
    HYD_Time timeout;

    int global_core_count;

    struct HYD_Exec_info *exec_info_list;
    struct HYD_Partition *partition_list;

    /* Random parameters used for internal code */
    int func_depth;
    char stdin_tmp_buf[HYD_TMPBUF_SIZE];
    int stdin_buf_offset;
    int stdin_buf_count;
};

typedef struct HYD_Handle_ HYD_Handle;

extern HYD_Handle handle;

#endif /* HYDRA_H_INCLUDED */

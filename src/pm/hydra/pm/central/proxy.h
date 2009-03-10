/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef PROXY_H_INCLUDED
#define PROXY_H_INCLUDED

#include "hydra.h"

struct HYD_Proxy_params {
    HYD_Env_t *global_env;
    HYD_Env_t *env_list;
    int proc_count;
    int pmi_id;
    char *args[HYD_EXEC_ARGS];
    struct HYD_Partition_list *partition;

    int *pid;
    int *out;
    int *err;
    int in;

    int stdin_buf_offset;
    int stdin_buf_count;
    char stdin_tmp_buf[HYD_TMPBUF_SIZE];
};

extern struct HYD_Proxy_params HYD_Proxy_params;
extern int HYD_Proxy_listenfd;

HYD_Status HYD_Proxy_get_params(int t_argc, char **t_argv);
HYD_Status HYD_Proxy_listen_cb(int fd, HYD_Event_t events);
HYD_Status HYD_Proxy_stdout_cb(int fd, HYD_Event_t events);
HYD_Status HYD_Proxy_stderr_cb(int fd, HYD_Event_t events);
HYD_Status HYD_Proxy_stdin_cb(int fd, HYD_Event_t events);

#endif /* PROXY_H_INCLUDED */

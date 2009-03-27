/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef PMI_PROXY_H_INCLUDED
#define PMI_PROXY_H_INCLUDED

#include "hydra_base.h"
#include "hydra_utils.h"

struct HYD_PMCD_pmi_proxy_params {
    int debug;

    int proxy_port;
    int is_persistent;
    char *wdir;
    HYD_Binding binding;
    char *user_bind_map;

    HYD_Env_t *global_env;

    int one_pass_count;
    int partition_proc_count;
    int exec_proc_count;

    /* Process segmentation information for this partition */
    struct HYD_Partition_segment *segment_list;
    struct HYD_Partition_exec *exec_list;

    int *pid;
    int *out;
    int *err;
    int *exit_status;
    int in;
    int rproxy_connfd;

    int stdin_buf_offset;
    int stdin_buf_count;
    char stdin_tmp_buf[HYD_TMPBUF_SIZE];
};

extern struct HYD_PMCD_pmi_proxy_params HYD_PMCD_pmi_proxy_params;
extern int HYD_PMCD_pmi_proxy_listenfd;

HYD_Status HYD_PMCD_pmi_proxy_init_params(struct HYD_PMCD_pmi_proxy_params *proxy_params);
HYD_Status HYD_PMCD_pmi_proxy_cleanup_params(struct HYD_PMCD_pmi_proxy_params *proxy_params);
HYD_Status HYD_PMCD_pmi_proxy_get_params(int t_argc, char **t_argv);
HYD_Status HYD_PMCD_pmi_proxy_get_next_keyvalp(char **bufp, int *buf_lenp, char **keyp,
                                    int *key_lenp, char **valp, int *val_lenp, char separator); 
HYD_Status HYD_PMCD_pmi_proxy_handle_cmd(int fd, char *cmd, int cmd_len);
HYD_Status HYD_PMCD_pmi_proxy_handle_launch_cmd(int job_connfd, char *launch_cmd, int cmd_len);
HYD_Status HYD_PMCD_pmi_proxy_listen_cb(int fd, HYD_Event_t events, void *userp);
HYD_Status HYD_PMCD_pmi_proxy_remote_cb(int fd, HYD_Event_t events, void *userp);
HYD_Status HYD_PMCD_pmi_proxy_rstdout_cb(int fd, HYD_Event_t events, void *userp);
HYD_Status HYD_PMCD_pmi_proxy_stdout_cb(int fd, HYD_Event_t events, void *userp);
HYD_Status HYD_PMCD_pmi_proxy_stderr_cb(int fd, HYD_Event_t events, void *userp);
HYD_Status HYD_PMCD_pmi_proxy_stdin_cb(int fd, HYD_Event_t events, void *userp);

#endif /* PMI_PROXY_H_INCLUDED */

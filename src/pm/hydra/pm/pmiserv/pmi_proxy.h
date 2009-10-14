/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef PMI_PROXY_H_INCLUDED
#define PMI_PROXY_H_INCLUDED

#include "hydra_base.h"
#include "hydra_utils.h"
#include "pmi_common.h"

struct HYD_PMCD_pmi_proxy_params {
    /* Proxy details */
    struct {
        char *server_name;
        int server_port;
        HYD_Launch_mode_t launch_mode;
        int proxy_id;
        char *bootstrap;
        char *bootstrap_exec;
        int enablex;
        int debug;
    } proxy;

    char *wdir;
    char *pmi_port_str;

    char *binding;
    char *bindlib;

    char *ckpointlib;
    char *ckpoint_prefix;
    int ckpoint_restart;

    struct HYD_Env_global global_env;

    int global_core_count;
    int proxy_core_count;
    int exec_proc_count;

    int procs_are_launched;

    /* Process segmentation information for this proxy */
    struct HYD_Proxy_segment *segment_list;
    struct HYD_Proxy_exec *exec_list;

    struct {
        int out;
        int err;
        int in;
        int control;
    } upstream;

    int *pid;
    int *out;
    int *err;
    int *exit_status;
    int in;

    int stdin_buf_offset;
    int stdin_buf_count;
    char stdin_tmp_buf[HYD_TMPBUF_SIZE];
};

extern struct HYD_PMCD_pmi_proxy_params HYD_PMCD_pmi_proxy_params;

/* utils */
HYD_Status HYD_PMCD_pmi_proxy_get_params(char **t_argv);
HYD_Status HYD_PMCD_pmi_proxy_cleanup_params(void);
HYD_Status HYD_PMCD_pmi_proxy_procinfo(int fd);
HYD_Status HYD_PMCD_pmi_proxy_launch_procs(void);
void HYD_PMCD_pmi_proxy_killjob(void);

/* callback */
HYD_Status HYD_PMCD_pmi_proxy_control_connect_cb(int fd, HYD_Event_t events, void *userp);
HYD_Status HYD_PMCD_pmi_proxy_control_cmd_cb(int fd, HYD_Event_t events, void *userp);
HYD_Status HYD_PMCD_pmi_proxy_stdout_cb(int fd, HYD_Event_t events, void *userp);
HYD_Status HYD_PMCD_pmi_proxy_stderr_cb(int fd, HYD_Event_t events, void *userp);
HYD_Status HYD_PMCD_pmi_proxy_stdin_cb(int fd, HYD_Event_t events, void *userp);

#endif /* PMI_PROXY_H_INCLUDED */

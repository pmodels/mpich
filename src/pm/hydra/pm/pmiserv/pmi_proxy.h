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

typedef enum {
    HYD_PMCD_PMI_PROXY_UNSET,
    HYD_PMCD_PMI_PROXY_RUNTIME,
    HYD_PMCD_PMI_PROXY_PERSISTENT
} HYD_PMCD_pmi_proxy_type;

struct HYD_PMCD_pmi_proxy_params {
    int debug;
    /* If enabled the proxy does not fork and exit */
    int proxy_defer_exit;

    int proxy_port;
    HYD_PMCD_pmi_proxy_type proxy_type;
    char *wdir;
    HYD_Binding binding;
    char *user_bind_map;

    HYD_Env_t *global_env;

    int one_pass_count;
    int partition_proc_count;
    int exec_proc_count;

    int procs_are_launched;

    /* Process segmentation information for this partition */
    struct HYD_Partition_segment *segment_list;
    struct HYD_Partition_exec *exec_list;

    int out_upstream_fd;
    int err_upstream_fd;
    int in_upstream_fd;
    int control_fd;

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
extern int HYD_PMCD_pmi_proxy_listenfd;

/* utils */
HYD_Status HYD_PMCD_pmi_proxy_get_params(char **t_argv);
HYD_Status HYD_PMCD_pmi_proxy_cleanup_params(void);
HYD_Status HYD_PMCD_pmi_proxy_procinfo(int fd);
HYD_Status HYD_PMCD_pmi_proxy_launch_procs(void);
void HYD_PMCD_pmi_proxy_killjob(void);

/* callback */
HYD_Status HYD_PMCD_pmi_proxy_listen_cb(int fd, HYD_Event_t events, void *userp);
HYD_Status HYD_PMCD_pmi_proxy_stdout_cb(int fd, HYD_Event_t events, void *userp);
HYD_Status HYD_PMCD_pmi_proxy_stderr_cb(int fd, HYD_Event_t events, void *userp);
HYD_Status HYD_PMCD_pmi_proxy_stdin_cb(int fd, HYD_Event_t events, void *userp);

#endif /* PMI_PROXY_H_INCLUDED */

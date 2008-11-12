/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef CSI_H_INCLUDED
#define CSI_H_INCLUDED

#include <sys/time.h>
#include "hydra.h"
#include "hydra_env.h"

#define HYD_CSI_OUT  (1)
#define HYD_CSI_IN   (2)

typedef u_int16_t HYD_CSI_Event_t;

#define HYD_CSI_TMPBUF_SIZE (64 * 1024)
#define HYD_CSI_EXEC_ARGS 200

typedef enum {
    HYD_CSI_PROP_NONE,
    HYD_CSI_PROP_ENVALL,
    HYD_CSI_PROP_ENVNONE,
    HYD_CSI_PROP_ENVLIST
} HYD_CSI_Prop_t;

struct HYD_CSI_Handle_ {
    int debug;
    int enablex;
    char *wdir;

    HYDU_Env_t *global_env;
    HYDU_Env_t *system_env;
    HYDU_Env_t *user_env;
    HYD_CSI_Prop_t prop;
    HYDU_Env_t *prop_env;

    int in;
    void *stdin_cb;
    char stdin_tmp_buf[HYD_CSI_TMPBUF_SIZE];
    int stdin_buf_offset;
    int stdin_buf_count;

    /* Start time and timeout. These are filled in by the launcher,
     * but are utilized by the demux engine and the boot-strap server
     * to decide how long we need to wait for. */
    struct timeval start;
    struct timeval timeout;

    /* Each structure will contain all hosts/cores that use the same
     * executable and environment. */
    struct HYD_CSI_Proc_params {
        int user_num_procs;
        int total_num_procs;
        char **total_proc_list;
        int *total_core_list;

        char *host_file;

        char *exec[HYD_CSI_EXEC_ARGS];
        HYDU_Env_t *user_env;
        HYD_CSI_Prop_t prop;
        HYDU_Env_t *prop_env;

        /* These output FDs are filled in by the lower layers */
        int *out;
        int *err;

        /* Callback functions for the stdout/stderr events. These can
         * be the same. */
        void *stdout_cb;
        void *stderr_cb;

        /* Status > 0 means that it is not set yet. Successful
         * completion of a process will set the status to 0. An error
         * will set this to a negative value corresponding to the
         * error. Depending on the bootstrap server, these values
         * might correspond to per-process status, or can be a common
         * value for all processes. */
        int *pid;
        int *exit_status;
        int *exit_status_valid;

        struct HYD_CSI_Proc_params *next;
    } *proc_params;
};

typedef struct HYD_CSI_Handle_ HYD_CSI_Handle;

/* We'll use this as the central handle that has most of the
 * information needed by everyone. All data to be written has to be
 * done before the HYD_CSI_Wait_for_completion() function is called,
 * except for two exceptions:
 *
 * 1. The timeout value is initially added by the launcher before the
 * HYD_CSI_Wait_for_completion() function is called, but can be edited
 * by the control system within this call. There's no guarantee on
 * what value it will contain for the other layers. (this should
 * probably not be a part of the CSI Handle at all?).
 *
 * 2. There is no guarantee on what the exit status will contain till
 * the HYD_CSI_Wait_for_completion() function returns (where the
 * bootstrap server can fill out these values).
 */
extern HYD_CSI_Handle csi_handle;

HYD_Status HYD_CSI_Launch_procs(void);
HYD_Status HYD_CSI_Wait_for_completion(void);
HYD_Status HYD_CSI_Close_fd(int fd);
HYD_Status HYD_CSI_Finalize(void);

#endif /* CSI_H_INCLUDED */

/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPX_H_INCLUDED
#define MPX_H_INCLUDED

#include "hydra.h"

#define PMI_MAXKEYLEN    (64)   /* max length of key in keyval space */
#define PMI_MAXVALLEN    (1024) /* max length of value in keyval space */
#define PMI_MAXKVSLEN    (256)  /* max length of various names */

enum MPX_cmd_type {
    /* upstream to downstream */
    MPX_CMD_TYPE__PRIMARY_ENV = 0,
    MPX_CMD_TYPE__SECONDARY_ENV,
    MPX_CMD_TYPE__CWD,
    MPX_CMD_TYPE__EXEC,
    MPX_CMD_TYPE__KVSNAME,
    MPX_CMD_TYPE__LAUNCH_PROCESSES,
    MPX_CMD_TYPE__PMI_BARRIER_OUT,
    MPX_CMD_TYPE__KVCACHE_OUT,
    MPX_CMD_TYPE__PMI_PROCESS_MAPPING,
    MPX_CMD_TYPE__SIGNAL,

    /* downstream to upstream */
    MPX_CMD_TYPE__PMI_BARRIER_IN,
    MPX_CMD_TYPE__STDOUT,
    MPX_CMD_TYPE__STDERR,
    MPX_CMD_TYPE__KVCACHE_IN,
    MPX_CMD_TYPE__PID,
};

struct MPX_cmd {
    enum MPX_cmd_type type;
    int data_len;

    union {
        struct {
            int block_start;
            int block_size;
            int block_stride;
        } process_id;

        struct {
            int exec_proc_count;
        } exec;

        struct {
            int pgid;
            int proxy_id;
            int pmi_id;
        } stdoe;

        struct {
            int pgid;
            int num_blocks;
        } kvcache;

        struct {
            int pgid;
        } barrier_in;

        struct {
            int signum;
        } signal;

        struct {
            int proxy_id;
            int pgid;
        } pids;
    } u;
};

HYD_status MPX_bcast(struct MPX_cmd cmd, int upstream_fd, struct HYD_int_hash *downstream_fd_hash,
                     void *buf);
HYD_status MPX_primary_env_bcast(struct MPX_cmd cmd, int upstream_fd,
                                 struct HYD_int_hash *downstream_fd_hash, int envcount,
                                 char **env);

#endif /* MPX_H_INCLUDED */

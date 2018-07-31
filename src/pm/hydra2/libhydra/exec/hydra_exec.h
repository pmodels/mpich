/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef HYDRA_EXEC_H_INCLUDED
#define HYDRA_EXEC_H_INCLUDED

#include "hydra_base.h"

struct HYD_exec {
    char *exec[HYD_NUM_TMP_STRINGS];
    int proc_count;
    struct HYD_exec *next;
};

HYD_status HYD_exec_alloc(struct HYD_exec **exec);
void HYD_exec_free_list(struct HYD_exec *exec_list);

#endif /* HYDRA_EXEC_H_INCLUDED */

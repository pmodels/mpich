/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef CKPOINT_BLCR_H_INCLUDED
#define CKPOINT_BLCR_H_INCLUDED

HYD_status HYDT_ckpoint_blcr_init(void);
HYD_status HYDT_ckpoint_blcr_checkpoint(const char *prefix, int pgid, int id, int ckpoint_num);
HYD_status HYDT_ckpoint_blcr_restart(const char *prefix, int pgid, int id, int ckpoint_num,
                                     struct HYD_env *envlist, int num_ranks, int ranks[],
                                     int *in, int *out, int *err, int *pid);

#endif /* CKPOINT_BLCR_H_INCLUDED */

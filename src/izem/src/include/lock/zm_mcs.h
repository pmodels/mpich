/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 * See COPYRIGHT in top-level directory.
 */

#ifndef _ZM_MCS_H
#define _ZM_MCS_H
#include "lock/zm_lock_types.h"

int zm_mcs_init(zm_mcs_t *);
int zm_mcs_acquire(zm_mcs_t *L, zm_mcs_qnode_t* I);
int zm_mcs_release(zm_mcs_t *L, zm_mcs_qnode_t *I);

#endif /* _ZM_MCS_H */

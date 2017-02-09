/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 * See COPYRIGHT in top-level directory.
 */

#ifndef _ZM_CSVMCS_H
#define _ZM_CSVMCS_H
#include "lock/zm_lock_types.h"

int zm_csvmcs_init(zm_csvmcs_t *);
int zm_csvmcs_acquire(zm_csvmcs_t *L, zm_mcs_qnode_t* I);
int zm_csvmcs_release(zm_csvmcs_t *L);

#endif /* _ZM_CSVMCS_H */

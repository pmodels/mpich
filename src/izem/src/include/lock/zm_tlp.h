/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 * See COPYRIGHT in top-level directory.
 */

#ifndef _ZM_TLP_H
#define _ZM_TLP_H
#include "lock/zm_lock_types.h"

#if (ZM_TLP_HIGH_P == ZM_TICKET) || (ZM_TLP_LOW_P  == ZM_TICKET)
#include "lock/zm_ticket.h"
#endif
#if (ZM_TLP_HIGH_P == ZM_MCS) || (ZM_TLP_LOW_P  == ZM_MCS)
#include "lock/zm_mcs.h"
#endif

int zm_tlp_init(zm_tlp_t *);
int zm_tlp_acquire(zm_tlp_t* lock);
int zm_tlp_acquire_low(zm_tlp_t* lock);
int zm_tlp_release(zm_tlp_t* lock);

#endif /* _ZM_TLP_H */

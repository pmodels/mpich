/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 * See COPYRIGHT in top-level directory.
 */

#ifndef _ZM_TICKET_H
#define _ZM_TICKET_H
#include "lock/zm_lock_types.h"

int zm_ticket_init(zm_ticket_t *);
int zm_ticket_acquire(zm_ticket_t* lock);
int zm_ticket_release(zm_ticket_t* lock);

#endif /* _ZM_TICKET_H */

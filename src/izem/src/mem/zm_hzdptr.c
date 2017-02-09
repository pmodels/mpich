/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 * See COPYRIGHT in top-level directory.
 */

#include "mem/zm_hzdptr.h"

zm_atomic_ptr_t zm_hzdptr_list; /* head of the list*/
zm_atomic_uint_t zm_hplist_length; /* N: correlates with the number
                                        of threads */

/* per-thread pointer to its own hazard pointer node */
zm_thread_local zm_hzdptr_lnode_t* zm_my_hplnode;



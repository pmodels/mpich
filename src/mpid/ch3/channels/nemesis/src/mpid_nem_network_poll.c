/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpid_nem_impl.h"
#include "mpid_nem_nets.h"

#undef FUNCNAME
#define FUNCNAME MPID_nem_network_poll
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_nem_network_poll(int in_blocking_progress)
{
    return MPID_nem_netmod_func->poll(in_blocking_progress);
}

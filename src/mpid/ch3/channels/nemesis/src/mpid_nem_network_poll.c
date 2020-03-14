/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpid_nem_impl.h"
#include "mpid_nem_nets.h"

int MPID_nem_network_poll(int in_blocking_progress)
{
    return MPID_nem_netmod_func->poll(in_blocking_progress);
}

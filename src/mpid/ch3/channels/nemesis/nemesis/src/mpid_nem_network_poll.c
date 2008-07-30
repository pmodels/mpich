/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpid_nem_impl.h"
#include "mpid_nem_nets.h"

#undef FUNCNAME
#define FUNCNAME MPID_nem_network_poll
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_network_poll (MPID_nem_poll_dir_t in_or_out)
{
    return MPID_nem_net_module_poll (in_or_out);
}

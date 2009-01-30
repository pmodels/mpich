/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "gm_impl.h"

int
MPID_nem_gm_test()
{
    return gm_receive_pending (MPID_nem_module_gm_port);
}

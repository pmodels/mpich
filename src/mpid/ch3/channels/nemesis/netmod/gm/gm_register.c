/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "gm_impl.h"

int
MPID_nem_gm_register_mem (void *p, int len)
{
    if (gm_register_memory (MPID_nem_module_gm_port, p, len) == GM_SUCCESS)
	return 0;
    else
	return -1;
}

int
MPID_nem_gm_deregister_mem (void *p, int len)
{
    if (gm_deregister_memory (MPID_nem_module_gm_port, p, len) == GM_SUCCESS)
	return 0;
    else
	return -1;
}



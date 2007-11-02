/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "dummy_module.h"

int
MPID_nem_dummy_module_finalize ( void )
{
  return MPID_nem_dummy_module_ckpt_shutdown ();    
}

int
MPID_nem_dummy_module_ckpt_shutdown ( void )
{
  return 0 ;
}


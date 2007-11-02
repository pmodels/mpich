/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"

/* This function initializes the table of routines used for 
   implementing the MPI Port-related functions.  This channel does not
   support those operations, so all functions are set to NULL */
int MPIDI_CH3_PortFnsInit( MPIDI_PortFns *portFns )
{
    portFns->OpenPort    = 0;
    portFns->ClosePort   = 0;
    portFns->CommAccept  = 0;
    portFns->CommConnect = 0;
    return MPI_SUCCESS;
}

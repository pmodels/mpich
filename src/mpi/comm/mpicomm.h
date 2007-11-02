/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* Function prototypes for communicator helper functions */
/* The MPIR_Get_contextid routine is in mpiimpl.h so that the device 
   may use it */
/* int MPIR_Get_contextid( MPID_Comm * ); */
int MPIR_Get_intercomm_contextid( MPID_Comm *, int *, int * );
void MPIR_Free_contextid( int );


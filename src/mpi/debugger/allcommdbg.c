/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2012 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>

/* style: allow:printf:6 sig:0 */

/* This definition is almost the same as the MPIR_Comm_list in dbginit, 
   except a void * is used instead of MPID_Comm * for head; for the use
   here, void * is all that is needed */
typedef struct MPIR_Comm_list {
    int sequence_number;   /* Used to detect changes in the list */
    void *head;       /* Head of the list */
} MPIR_Comm_list;

extern MPIR_Comm_list MPIR_All_communicators;

int main( int argc, char **argv )
{
  int errs = 0;
  MPI_Init( &argc, &argv );

  printf( "sequence number for communicators is %d\n", MPIR_All_communicators.sequence_number );
  printf( "head pointer is %p\n", MPIR_All_communicators.head );

  if (MPIR_All_communicators.head == (void *)0) {
    printf( "ERROR: The communicator list field has a null head pointer\n" );
    printf( "Either the debugger support is not enabled (--enable-debuginfo)\n\
or the necessary symbols, including the extern variable\n\
MPIR_All_communicators, are not properly exported to the main program\n" );
    errs++;
  }

  MPI_Finalize( );
  if (errs) {
    printf( " Found %d errors\n", errs );
  }
  else {
    printf( " No Errors\n" );
  }
  
  return 0;
}

/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* Definitions private to the datatype code */
extern int MPIR_Datatype_init( void );
extern int MPIR_Datatype_builtin_fillin( void );
extern int MPIR_Datatype_init_names( void );
extern void MPIR_Datatype_iscontig( MPI_Datatype , int * );

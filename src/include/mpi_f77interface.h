/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* These functions are provided by the MPICH2 code for the Fortran interface,
   and provide the interfaces needed to keep track of which MPI internal
   objects need to have Fortran or Fortran 90 characteristics */
void MPIR_Keyval_set_fortran( int );
void MPIR_Keyval_set_fortran90( int );
void MPIR_Grequest_set_lang_f77( MPI_Request greq );

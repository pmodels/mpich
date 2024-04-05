/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPII_F77INTERFACE_H_INCLUDED
#define MPII_F77INTERFACE_H_INCLUDED

/* These functions are provided by the MPICH code for the Fortran interface,
   and provide the interfaces needed to keep track of which MPI internal
   objects need to have Fortran or Fortran 90 characteristics */
MPICH_API_PUBLIC void MPII_Keyval_set_fortran(int);
MPICH_API_PUBLIC void MPII_Keyval_set_fortran90(int);
MPICH_API_PUBLIC void MPII_Grequest_set_lang_f77(MPI_Request greq);

#define MPII_ATTR_C_TO_FORTRAN(ATTR) ((ATTR)+1)

#endif /* MPII_F77INTERFACE_H_INCLUDED */

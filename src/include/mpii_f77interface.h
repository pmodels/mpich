/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPII_F77INTERFACE_H_INCLUDED
#define MPII_F77INTERFACE_H_INCLUDED

/* These functions are provided by the MPICH code for the Fortran interface,
   and provide the interfaces needed to keep track of which MPI internal
   objects need to have Fortran or Fortran 90 characteristics */
MPICH_API_PUBLIC void MPII_Keyval_set_fortran(int);
MPICH_API_PUBLIC void MPII_Keyval_set_fortran90(int);
MPICH_API_PUBLIC void MPII_Grequest_set_lang_f77(MPI_Request greq);
#if defined(HAVE_FORTRAN_BINDING) && !defined(HAVE_FINT_IS_INT)
MPICH_API_PUBLIC void MPII_Op_set_fc(MPI_Op);
typedef void (MPII_F77_User_function) (void *, void *, MPI_Fint *, MPI_Fint *);
MPICH_API_PUBLIC void MPII_Errhandler_set_fc(MPI_Errhandler);
#endif

#define MPII_ATTR_C_TO_FORTRAN(ATTR) ((ATTR)+1)

#endif /* MPII_F77INTERFACE_H_INCLUDED */

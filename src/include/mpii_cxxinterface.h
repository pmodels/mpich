/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */

#ifndef MPII_CXX_INTERFACE_H_INCLUDED
#define MPII_CXX_INTERFACE_H_INCLUDED

extern void MPII_Keyval_set_cxx( int, void (*)(void), void (*)(void) );
extern void MPII_Op_set_cxx( MPI_Op, void (*)(void) );
extern void MPII_Errhandler_set_cxx( MPI_Errhandler, void (*)(void) );

#endif /* MPII_CXX_INTERFACE_H_INCLUDED */

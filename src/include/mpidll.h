/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  
 *  (C) 2005 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef MPIDLL_H_INCLUDED
#define MPIDLL_H_INCLUDED

int MPIU_DLL_Open( const char libname[], void **handle );
int MPIU_DLL_FindSym( void *handle, const char symbol[], void **value );
int MPIU_DLL_Close( void *handle );

#endif

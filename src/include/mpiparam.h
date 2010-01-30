/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef MPIPARAM_H_INCLUDED
#define MPIPARAM_H_INCLUDED
/* ------------------------------------------------------------------------- */
/* mpiparam.h*/
/* ------------------------------------------------------------------------- */

/* Parameter handling.  These functions have not been implemented yet.
   See src/util/param.[ch] */
typedef enum MPIU_Param_result_t { 
    MPIU_PARAM_FOUND = 0, 
    MPIU_PARAM_OK = 1, 
    MPIU_PARAM_ERROR = 2 
} MPIU_Param_result_t;
int MPIU_Param_init( int *, char *[], const char [] );
int MPIU_Param_bcast( void );
int MPIU_Param_register( const char [], const char [], const char [] );
int MPIU_Param_get_int( const char [], int, int * );
int MPIU_Param_get_string( const char [], const char *, char ** );
int MPIU_Param_get_range( const char name[], int *lowPtr, int *highPtr );
void MPIU_Param_finalize( void );

/* See mpishared.h as well */
/* ------------------------------------------------------------------------- */
/* end of mpiparam.h*/
/* ------------------------------------------------------------------------- */

#endif /* MPIPARAM_H_INCLUDED */

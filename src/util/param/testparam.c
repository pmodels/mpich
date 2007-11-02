/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id: testparam.c,v 1.6 2005/08/11 20:59:49 gropp Exp $
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* For testing only */
/* style: allow:printf:1 sig:0 */   

#include "param.h"
#include <stdio.h>

int main( int argc, char *argv[] )
{
    int val;

    MPIU_Param_init( 0, 0, "sample.prm" );

    MPIU_Param_bcast( );
    
    MPIU_Param_get_int( "SOCKBUFSIZE", 65536, &val );

    printf( "Socket size is %d\n", val );

    MPIU_Param_finalize( );
}

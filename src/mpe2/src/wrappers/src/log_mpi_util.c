/*
   (C) 2001 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
#include "mpe_wrappers_conf.h"

#if defined( STDC_HEADERS ) || defined( HAVE_STDLIB_H )
#include <stdlib.h>
#endif
#if defined( STDC_HEADERS ) || defined( HAVE_STDIO_H )
#include <stdio.h>
#endif
#if defined( STDC_HEADERS ) || defined( HAVE_STRING_H )
#include <string.h>
#endif

/* Copy of CLOG_Util_getenvbool() */
int MPE_Util_getenvbool( char *env_var, int default_value );
int MPE_Util_getenvbool( char *env_var, int default_value )
{ 
    char *env_val;
  
    env_val = (char *) getenv( env_var );
    if ( env_val != NULL ) {
        if (    strcmp( env_val, "true" ) == 0
             || strcmp( env_val, "TRUE" ) == 0
             || strcmp( env_val, "yes" ) == 0 
             || strcmp( env_val, "YES" ) == 0 )
            return 1;
        else if (    strcmp( env_val, "false" ) == 0
                  || strcmp( env_val, "FALSE" ) == 0
                  || strcmp( env_val, "no" ) == 0
                  || strcmp( env_val, "NO" ) == 0 )
            return 0;
        else {
            fprintf( stderr, __FILE__":MPE_Util_getenvbool() - \n"
                             "\t""Environment variable %s has invalid boolean "
                             "value %s and will be set to %d.\n",
                             env_var, env_val, default_value );
            fflush( stderr );
        }
    }
    return default_value;
}

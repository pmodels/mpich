#include "cross_conf.h"

#ifdef F77_NAME_UPPER
#define flogical_ FLOGICAL
#elif defined(F77_NAME_LOWER) || defined(F77_NAME_MIXED)
#define flogical_ flogical
#endif

#if defined( STD_HEADERS) || defined( HAVE_STDIO_H )
#include <stdio.h>
#endif
#include "mpi.h"

int main( int argc, char *argv[] )
{
    FILE     *cross_file;
    MPI_Fint  itrue, ifalse;

    cross_file = fopen( CROSS_SPEC_FILE, "a" );
    if ( cross_file == NULL ) {
        fprintf( stderr, "Can't open %s for appending!\n", CROSS_SPEC_FILE );
        return 1;
    }

    /* Invoke the Fortran subroutine to get Fortran's TRUE/FALSE in C */
    fprintf( cross_file, "%s\n", "# Fortran to C runtime characteristics..." );
    fprintf( cross_file, "CROSS_MPI_STATUS_SIZE=%d\n",
                          sizeof(MPI_Status)/sizeof(MPI_Fint) );
    flogical_( &itrue, &ifalse );
    fprintf( cross_file, "CROSS_FORTRAN2C_TRUE=%d\n", itrue );
    fprintf( cross_file, "CROSS_FORTRAN2C_FALSE=%d\n", ifalse );

    fclose( cross_file );
    return 0;
}

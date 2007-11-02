/*
 * Globus device code:          Copyright 2005 Northern Illinois University
 * Borrowed MPICH-G2 code:      Copyright 2000 Argonne National Laboratory and Northern Illinois University
 * Borrowed MPICH2 device code: Copyright 2001 Argonne National Laboratory
 *
 * XXX: INSERT POINTER TO OFFICIAL COPYRIGHT TEXT
 */

#include <stdlib.h>
#include <stdio.h>
#include "mpidconf.h"
#include "mpig_vmpi.h"

int mpig_app_main(int argc, char ** argv);

/*
 * the MPI-2 standard does not require that the application pass the command line arguments to MPI_Init().  futhermore, the
 * application can expect the command line arguments passed to its main() routine to be only those destined for the application.
 *
 * on the other hand, a vendor MPI that is based on the MPI-1 standard may require that the command line arguments be passed to
 * MPI_Init().  in addition, it may add arguments as long as it removes them before MPI_Init() returns.
 *
 * in the case where MPIg is using a vendor MPI based on the MPI-1 standard, the command line arguments must be acquired and
 * processed by the vendor MPI_Init before the application's main() routine is called.  it is not sufficient to stash a copy of
 * the arguments and hand those to the vendor MPI_Init() when mpig_cm_vmpi_init() is called.  the vendor startup process may have
 * added arguments that must be removed before the application's main() routine is called, and the only way to guarantee the
 * removal of such arguments is to call the vendor MPI_Init() before calling the application main() routine.
 */
int main(int argc, char ** argv)
{
    int vrc;
    int app_rc;
    
    vrc = mpig_vmpi_init(&argc, &argv);
    if (vrc)
    {
	fprintf(stderr, "\nERROR: initialization of the vendor MPI failed\n\n");
	return 1;
    }

    app_rc = mpig_app_main(argc, argv);

    vrc = mpig_vmpi_finalize();
    if (vrc)
    {
	fprintf(stderr, "\nERROR: finalization of the vendor MPI failed\n\n");
	return 1;
    }
    
    return app_rc;
}

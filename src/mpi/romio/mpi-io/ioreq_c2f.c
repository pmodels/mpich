/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpioimpl.h"

#ifdef HAVE_WEAK_SYMBOLS

#if defined(HAVE_PRAGMA_WEAK_ALIAS)
#pragma weak MPIO_Request_c2f = PMPIO_Request_c2f
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPIO_Request_c2f MPIO_Request_c2f
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPIO_Request_c2f as PMPIO_Request_c2f
/* end of weak pragmas */
#endif

/* Include mapping from MPI->PMPI */
#define MPIO_BUILD_PROFILING
#include "mpioprof.h"
#endif
#include "adio_extern.h"

/*@
    MPIO_Request_c2f - Translates a C I/O-request handle to a
                       Fortran I/O-request handle

Input Parameters:
. request - C I/O-request handle (handle)

Return Value:
  Fortran I/O-request handle (integer)
@*/
MPI_Fint MPIO_Request_c2f(MPIO_Request request)
{
    return ((MPI_Fint) request);
}

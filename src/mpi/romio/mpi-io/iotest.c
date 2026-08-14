/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpioimpl.h"

#ifdef HAVE_WEAK_SYMBOLS

#if defined(HAVE_PRAGMA_WEAK_ALIAS)
#pragma weak MPIO_Test = PMPIO_Test
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPIO_Test MPIO_Test
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPIO_Test as PMPIO_Test
/* end of weak pragmas */
#endif

/* Include mapping from MPI->PMPI */
#define MPIO_BUILD_PROFILING
#include "mpioprof.h"
#endif

/* status object not filled currently */

/*@
    MPIO_Test - Test the completion of a nonblocking read or write

Input Parameters:
. request - request object (handle)

Output Parameters:
. flag - true if operation completed (logical)
. status - status object (Status)

.N fortran
@*/
int MPIO_Test(MPIO_Request * request, int *flag, MPI_Status * status)
{
    return (MPI_Test(request, flag, status));
}

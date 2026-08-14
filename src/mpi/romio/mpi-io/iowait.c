/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpioimpl.h"

#ifdef HAVE_WEAK_SYMBOLS

#if defined(HAVE_PRAGMA_WEAK_ALIAS)
#pragma weak MPIO_Wait = PMPIO_Wait
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPIO_Wait MPIO_Wait
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPIO_Wait as PMPIO_Wait
/* end of weak pragmas */
#endif

/* Include mapping from MPI->PMPI */
#define MPIO_BUILD_PROFILING
#include "mpioprof.h"
#endif

/* status object not filled currently */

/*@
    MPIO_Wait - Waits for the completion of a nonblocking read or write

Input Parameters:
. request - request object (handle)

Output Parameters:
. status - status object (Status)

.N fortran
@*/
int MPIO_Wait(MPIO_Request * request, MPI_Status * status)
{
    return (MPI_Wait(request, status));
}

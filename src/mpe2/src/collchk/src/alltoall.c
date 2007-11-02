/*
   (C) 2004 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
#include "collchk.h" 

#define BUFFS_NOT_SHARED 0

int MPI_Alltoall( void* sbuff, int scnt, MPI_Datatype stype,
                  void* rbuff, int rcnt, MPI_Datatype rtype,
                  MPI_Comm comm)
{
    int             g2g = 1;
    char            call[COLLCHK_SM_STRLEN];

    sprintf( call, "ALLTOALL" );

    /* Check if init has been called */
    g2g = CollChk_is_init();

    if( g2g ) {
        /* check call consistency */
        CollChk_same_call( comm, call );
        
        /* check datatype signature consistency */
        /* This is the same datatype signature test as MPI_Allgather */
        CollChk_dtype_allgather(comm, stype, scnt, rtype, rcnt,
                                BUFFS_NOT_SHARED, call);

        /* make the call */
        return (PMPI_Alltoall(sbuff, scnt, stype, rbuff, rcnt, rtype, comm));
    }
    else {
        /* init not called */
        return CollChk_err_han( "MPI_Init() has not been called!",
                                COLLCHK_ERR_NOT_INIT, call, comm );
    }
}

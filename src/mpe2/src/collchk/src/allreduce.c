/*
   (C) 2004 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
#include "collchk.h" 

int MPI_Allreduce( void* sbuff, void* rbuff, int cnt, 
                   MPI_Datatype dt, MPI_Op op, MPI_Comm comm)
{
    int g2g = 1;
    char call[COLLCHK_SM_STRLEN];

    sprintf( call, "ALLREDUCE" );

    /* Check if init has been called */
    g2g = CollChk_is_init();

    if( g2g ) {
        /* check for call consistency */
        CollChk_same_call( comm, call );
        /* check for MPI_IN_PLACE consistency if needed */
        CollChk_check_buff( comm, sbuff, call );
        /* check for same operations */
        CollChk_same_op( comm, op, call );
        
        /* check for same datatypes */
        CollChk_dtype_bcast( comm, dt, cnt, 0, call );
        
        /* make the call */
        return PMPI_Allreduce( sbuff, rbuff, cnt, dt, op, comm );
    }
    else {
        /* init not called */
        return CollChk_err_han( "MPI_Init() has not been called!",
                                COLLCHK_ERR_NOT_INIT, call, comm );
    }
}


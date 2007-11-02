/*
   (C) 2004 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
#include "collchk.h" 

int MPI_Allgatherv( void* sbuff, int scnt, MPI_Datatype stype,
                    void* rbuff, int *rcnts, int *displs, MPI_Datatype rtype,
                    MPI_Comm comm )
{
    char            call[COLLCHK_SM_STRLEN];
    int             g2g = 1, r;
    int             are2buffs;

    sprintf( call, "ALLGATHERV" );

    /* Check if init has been called */
    g2g = CollChk_is_init();

    if( g2g ) {
        MPI_Comm_rank(comm, &r);

        /* check for call consistency */
        CollChk_same_call( comm, call );
        /* check MPI_IN_PLACE consistency */
        CollChk_check_buff( comm, sbuff, call );

        /* check data signature consistency */
#if defined( HAVE_MPI_IN_PLACE )
        are2buffs = ( sbuff != MPI_IN_PLACE );
#else
        are2buffs = 1;
#endif
        CollChk_dtype_allgatherv(comm, stype, scnt, rtype, rcnts,
                                 are2buffs, call);

        /* make the call */
        return PMPI_Allgatherv( sbuff, scnt, stype,
                                rbuff, rcnts, displs, rtype,
                                comm );
    }
    else {
        /* init not called */
        return CollChk_err_han( "MPI_Init() has not been called!",
                                COLLCHK_ERR_NOT_INIT, call, comm );
    }
}

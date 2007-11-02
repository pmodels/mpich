/*
   (C) 2004 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
#include "collchk.h" 

int MPI_Allgather( void* sbuff, int scnt, MPI_Datatype stype,
                   void* rbuff, int rcnt, MPI_Datatype rtype,
                   MPI_Comm comm )
{
    int             g2g = 1;
    char            call[COLLCHK_SM_STRLEN];
    int             are2buffs;

    sprintf( call, "ALLGATHER" );

    /* Check if init has been called */
    g2g = CollChk_is_init();

    if( g2g ) {
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
        CollChk_dtype_allgather(comm, stype, scnt, rtype, rcnt,
                                are2buffs, call);

        /* make the call */
        return PMPI_Allgather( sbuff, scnt, stype, rbuff, rcnt, rtype, comm );
    }
    else {
        /* init not called */
        return CollChk_err_han( "MPI_Init() has not been called!",
                                COLLCHK_ERR_NOT_INIT, call, comm );
    }
}

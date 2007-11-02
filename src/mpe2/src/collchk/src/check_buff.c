/*
   (C) 2004 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
#include "collchk.h" 

int CollChk_check_buff(MPI_Comm comm, void *buff, char* call)
{
#if defined( HAVE_MPI_IN_PLACE )
    int   num_buffs_in_place;
    int   is_consistent;
    int   rank, size;
    char  err_str[COLLCHK_STD_STRLEN];

    /* get the rank and size */
    MPI_Comm_size(comm, &size);

    num_buffs_in_place = (buff == MPI_IN_PLACE);
    PMPI_Allreduce( MPI_IN_PLACE, &num_buffs_in_place, 1, MPI_INT,
                    MPI_SUM, comm );
    is_consistent = (num_buffs_in_place == 0 || num_buffs_in_place == size);
    if ( !is_consistent ) {
        MPI_Comm_rank(comm, &rank);
        sprintf(err_str,"Inconsistent use of MPI_IN_PLACE is detected "
                        "at rank %d.\n", rank);
    }
    else
        sprintf(err_str, COLLCHK_NO_ERROR_STR);

    if (!is_consistent) {
        return CollChk_err_han(err_str, COLLCHK_ERR_INPLACE, call, comm);
    }
#endif

    return MPI_SUCCESS;
}

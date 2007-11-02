/*
   (C) 2004 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
#include "collchk.h" 

int CollChk_same_high_low(MPI_Comm comm, int high_low, char* call)
{
    char err_str[COLLCHK_STD_STRLEN];

    if(    CollChk_same_int(comm, high_low, call, "High/Low", err_str)
        != MPI_SUCCESS ) {
        return CollChk_err_han(err_str, COLLCHK_ERR_HIGH_LOW, call, comm);
    }
    else {
        return MPI_SUCCESS;
    }

}

/*
   (C) 2004 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
#include "collchk.h" 

int CollChk_same_tag(MPI_Comm comm, int tag, char* call)
{
    char err_str[COLLCHK_STD_STRLEN];

    if (CollChk_same_int(comm, tag, call, "Tag", err_str) != MPI_SUCCESS) {
        return CollChk_err_han(err_str, COLLCHK_ERR_TAG, call, comm);
    }
    else {
        return MPI_SUCCESS;
    }

}

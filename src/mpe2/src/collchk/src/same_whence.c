/*
   (C) 2004 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
#include "collchk.h" 

int CollChk_same_whence(MPI_Comm comm, int whence, char* call)
{
    char err_str[COLLCHK_STD_STRLEN];

    if (    CollChk_same_int(comm, whence, call, "Whence", err_str)
         != MPI_SUCCESS) {
        return CollChk_err_han(err_str, COLLCHK_ERR_WHENCE, call, comm);
    }
    else {
        return MPI_SUCCESS;
    }

}

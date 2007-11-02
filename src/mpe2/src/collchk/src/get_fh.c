/*
   (C) 2004 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
#include "collchk.h" 

int CollChk_get_fh(MPI_File fh, MPI_Comm *comm)
{
    int cur_idx = 0;

    /* find the fh */
    while( cur_idx < CollChk_fh_cnt && CollChk_fh_list[cur_idx].fh != fh )
        cur_idx++;

    /* return the comm if found */
    if ( cur_idx < CollChk_fh_cnt && CollChk_fh_list[cur_idx].fh == fh ) {
        /* the comm was found */
        *comm = CollChk_fh_list[cur_idx].comm;
        return 1;
    }
    else {
        /* the comm was not found */
        comm = NULL;
        return 0;
    }
}

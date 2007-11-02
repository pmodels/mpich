/*
   (C) 2004 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
#include "collchk.h" 

void CollChk_add_fh( MPI_File fh, MPI_Comm comm )
{
    CollChk_fh_t *new_fh;

    /* increase the size */
    CollChk_fh_cnt++;

    /* get the memory */
    CollChk_fh_list = (CollChk_fh_t *) realloc( CollChk_fh_list,
                                                CollChk_fh_cnt
                                              * sizeof(CollChk_fh_t) );

    /*add the new_fh*/
    new_fh       = &(CollChk_fh_list[CollChk_fh_cnt-1]);
    new_fh->fh   = fh;
    new_fh->comm = comm;
}


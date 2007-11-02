/*
   (C) 2004 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
#include "collchk.h" 

void CollChk_add_win( MPI_Win win, MPI_Comm comm )
{
    CollChk_win_t new_win;

    /* get the memory */
    CollChk_win_list = (CollChk_win_t *) realloc( CollChk_win_list,
                                                  (CollChk_win_cnt+1)
                                                * sizeof(CollChk_win_t) );

    /* add the new_win */
    new_win.win = win;
    new_win.comm = comm;
    CollChk_win_list[CollChk_win_cnt] = new_win;

    /* increase the size; */
    CollChk_win_cnt++;
}

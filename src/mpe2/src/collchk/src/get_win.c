/*
   (C) 2004 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
#include "collchk.h" 

int CollChk_get_win(MPI_Win win, MPI_Comm *comm)
{
    int crr_win=0, found=0;

    /* find the win */
    while( (crr_win < CollChk_win_cnt) && !found ) {
        if(win == CollChk_win_list[crr_win].win) {
            found = 1;
        } 
        else {
            crr_win++;
        }
    }

    /* return the comm if found */
    if(found) {
        /* the comm was found */
        *comm = CollChk_win_list[crr_win].comm;
        return 1;
    }
    else {
        /* the comm was not found */
        comm = NULL;
        return 0;
    }
}

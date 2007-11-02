/*
   (C) 2004 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
#include "collchk.h" 

int CollChk_is_init(void)
{
    int fl;
    
    MPI_Initialized(&fl);
    
    if (fl)
        return 1;
    else
        return 0;
}
 

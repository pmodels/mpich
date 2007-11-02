/*
   (C) 2004 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
#include "collchk.h" 

void CollChk_set_begin(char* in)
{
    strcpy(CollChk_begin_str, in);
    COLLCHK_CALLED_BEGIN = 1;
}    

void CollChk_unset_begin(void)
{
    COLLCHK_CALLED_BEGIN = 0;
}

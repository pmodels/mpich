/*
   (C) 2004 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
#include "collchk.h" 

int MPI_File_open(MPI_Comm comm, char *filename, int amode,
                  MPI_Info info, MPI_File *fh)
{
    int g2g = 1,i;
    char call[COLLCHK_SM_STRLEN];

    sprintf(call, "FILE_OPEN");

    /* Check if init has been called */
    g2g = CollChk_is_init();

    if(g2g) {
        /* check for call consistancy */
        CollChk_same_call(comm, call);
        /* check for amode consistancy */
        CollChk_same_amode(comm, amode, call);

        /* make the call */
        i = PMPI_File_open(comm, filename, amode, info, fh);

        /* save the fh's communicator for future reference */
        CollChk_add_fh(*fh, comm);

        /* return to the user */
        return i;    
    }
    else {
        /* init not called */
        return CollChk_err_han("MPI_Init() has not been called!",
                               COLLCHK_ERR_NOT_INIT, call, comm);
    }
}

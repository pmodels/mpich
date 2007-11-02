/*
   (C) 2004 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
#include "collchk.h" 

int MPI_File_preallocate(MPI_File fh, MPI_Offset size)
{
    int g2g = 1;
    char call[COLLCHK_SM_STRLEN];
    MPI_Comm comm;

    sprintf(call, "FILE_PREALLOCATE");

    /* Check if init has been called */
    g2g = CollChk_is_init();

    if(g2g) {
        /* get the comm */
        if (!CollChk_get_fh(fh, &comm)) {
            return CollChk_err_han("File has not been Opened",
                                   COLLCHK_ERR_FILE_NOT_OPEN,
                                   call, MPI_COMM_WORLD);
        }

        /* check for call consistancy */
        CollChk_same_call(comm, call);

        /* make the call */
        return PMPI_File_preallocate(fh, size); 
    }
    else {
        /* init not called */
        return CollChk_err_han("MPI_Init() has not been called!",
                               COLLCHK_ERR_NOT_INIT, call, MPI_COMM_WORLD);
    }
}

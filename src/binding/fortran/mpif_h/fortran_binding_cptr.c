/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpi_fortimpl.h"

FORT_DLL_SPEC void FORT_CALL mpi_alloc_mem_cptr_(MPI_Aint * size, MPI_Fint * info, void **baseptr,
                                                 MPI_Fint * ierr)
{
    if (MPIR_F_NeedInit) {
        mpirinitf_();
        MPIR_F_NeedInit = 0;
    }

    void *baseptr_i;
    *ierr = MPI_Alloc_mem(*size, (MPI_Info) (*info), &baseptr_i);
    *baseptr = baseptr_i;
}

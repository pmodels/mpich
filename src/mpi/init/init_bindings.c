/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "mpi_init.h"

/* ** CXX binding **************/
void MPII_init_binding_cxx(void)
{
#ifdef HAVE_CXX_BINDING
    /* Set the functions used to call functions in the C++ binding
     * for reductions and attribute operations.  These are null
     * until a C++ operation is defined.  This allows the C code
     * that implements these operations to not invoke a C++ code
     * directly, which may force the inclusion of symbols known only
     * to the C++ compiler (e.g., under more non-GNU compilers, including
     * Solaris and IRIX). */
    MPIR_Process.cxx_call_op_fn = 0;
#endif
}

/* ** F08 binding **************/
#ifdef HAVE_F08_BINDING
int *MPIR_C_MPI_UNWEIGHTED MPICH_API_PUBLIC;
int *MPIR_C_MPI_WEIGHTS_EMPTY MPICH_API_PUBLIC;

MPI_F08_status MPIR_F08_MPI_STATUS_IGNORE_OBJ;
MPI_F08_status MPIR_F08_MPI_STATUSES_IGNORE_OBJ[1];
int MPIR_F08_MPI_IN_PLACE;
int MPIR_F08_MPI_BOTTOM;

/* Althought the two STATUS pointers are required but the MPI3.0,  they are not used in MPICH F08 binding */
MPI_F08_status *MPI_F08_STATUS_IGNORE = &MPIR_F08_MPI_STATUS_IGNORE_OBJ;
MPI_F08_status *MPI_F08_STATUSES_IGNORE = &MPIR_F08_MPI_STATUSES_IGNORE_OBJ[0];

void MPII_init_binding_f08(void)
{
    MPIR_C_MPI_UNWEIGHTED = MPI_UNWEIGHTED;
    MPIR_C_MPI_WEIGHTS_EMPTY = MPI_WEIGHTS_EMPTY;
}

#else
void MPII_init_binding_f08(void)
{
}
#endif

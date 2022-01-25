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

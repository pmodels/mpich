/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2014 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/*
  Input Parameters:
+ base - base address (integer)
- disp - displacement (integer)

  Return value:
  A new MPI_Aint value that is equivalent to the sum of the base and disp
  arguments, where base represents a base address returned by a call
  to MPI_GET_ADDRESS and disp represents a signed integer displacement.
*/
#undef FUNCNAME
#define FUNCNAME MPID_Aint_add
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPI_Aint MPID_Aint_add(MPI_Aint base, MPI_Aint disp)
{
    MPI_Aint result;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_AINT_ADD);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_AINT_ADD);

    result =  MPIR_VOID_PTR_CAST_TO_MPI_AINT ((char*)MPIR_AINT_CAST_TO_VOID_PTR(base) + disp);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_AINT_ADD);
    return result;
}

/*
  Input Parameters:
+ addr1 - minuend address (integer)
- addr2 - subtrahend address (integer)

  Return value:
  A new MPI_Aint value that is equivalent to the difference between addr1 and
  addr2 arguments, where addr1 and addr2 represent addresses returned by calls
  to MPI_GET_ADDRESS.
*/
#undef FUNCNAME
#define FUNCNAME MPID_Aint_diff
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPI_Aint MPID_Aint_diff(MPI_Aint addr1, MPI_Aint addr2)
{
    MPI_Aint result;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_AINT_DIFF);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_AINT_DIFF);

    result =  MPIR_PTR_DISP_CAST_TO_MPI_AINT ((char*)MPIR_AINT_CAST_TO_VOID_PTR(addr1) - (char*)MPIR_AINT_CAST_TO_VOID_PTR(addr2));

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_AINT_DIFF);
    return result;
}

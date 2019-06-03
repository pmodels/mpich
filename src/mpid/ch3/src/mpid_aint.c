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
  A new size_t value that is equivalent to the sum of the base and disp
  arguments, where base represents a base address returned by a call
  to MPI_GET_ADDRESS and disp represents a signed integer displacement.
*/
size_t MPID_Aint_add(size_t base, size_t disp)
{
    size_t result;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_AINT_ADD);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_AINT_ADD);

    result = (size_t) ((char *) base + disp);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_AINT_ADD);
    return result;
}

/*
  Input Parameters:
+ addr1 - minuend address (integer)
- addr2 - subtrahend address (integer)

  Return value:
  A new size_t value that is equivalent to the difference between addr1 and
  addr2 arguments, where addr1 and addr2 represent addresses returned by calls
  to MPI_GET_ADDRESS.
*/
size_t MPID_Aint_diff(size_t addr1, size_t addr2)
{
    size_t result;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_AINT_DIFF);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_AINT_DIFF);

    result = (size_t) ((char *) addr1 - (char *) addr2);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_AINT_DIFF);
    return result;
}

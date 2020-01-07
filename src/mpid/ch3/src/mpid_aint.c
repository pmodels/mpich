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
MPI_Aint MPID_Aint_add(MPI_Aint base, MPI_Aint disp)
{
    MPI_Aint result;




    result = (MPI_Aint) ((char *) base + disp);


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
MPI_Aint MPID_Aint_diff(MPI_Aint addr1, MPI_Aint addr2)
{
    MPI_Aint result;




    result = (MPI_Aint) ((char *) addr1 - (char *) addr2);


    return result;
}

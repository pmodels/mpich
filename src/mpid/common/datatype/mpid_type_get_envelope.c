/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <mpi.h>
#include <mpiimpl.h>
#include <mpid_datatype.h>
#include <mpid_dataloop.h>

/*@
  MPID_Type_get_envelope - get envelope information from datatype

Input Parameters:
. datatype - MPI datatype

Output Parameters:
+ num_integers - number of integers used to create datatype
. num_addresses - number of MPI_Aints used to create datatype
. num_datatypes - number of MPI_Datatypes used to create datatype
- combiner - function type used to create datatype
@*/

int MPID_Type_get_envelope(MPI_Datatype datatype,
			   int *num_integers,
			   int *num_addresses,
			   int *num_datatypes,
			   int *combiner)
{
    if (HANDLE_GET_KIND(datatype) == HANDLE_KIND_BUILTIN ||
	datatype == MPI_FLOAT_INT ||
	datatype == MPI_DOUBLE_INT ||
	datatype == MPI_LONG_INT ||
	datatype == MPI_SHORT_INT ||
	datatype == MPI_LONG_DOUBLE_INT)
    {
	*combiner      = MPI_COMBINER_NAMED;
	*num_integers  = 0;
	*num_addresses = 0;
	*num_datatypes = 0;
    }
    else {
	MPID_Datatype *dtp;

	MPID_Datatype_get_ptr(datatype, dtp);

	*combiner      = dtp->contents->combiner;
	*num_integers  = dtp->contents->nr_ints;
	*num_addresses = dtp->contents->nr_aints;
	*num_datatypes = dtp->contents->nr_types;
    }

    return MPI_SUCCESS;
}

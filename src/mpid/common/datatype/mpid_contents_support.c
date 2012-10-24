/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <mpi.h>
#include <mpid_datatype.h>

void MPID_Type_access_contents(MPI_Datatype type,
			       int **ints_p,
			       MPI_Aint **aints_p,
			       MPI_Datatype **types_p)
{
    int nr_ints, nr_aints, nr_types, combiner;
    int types_sz, struct_sz, ints_sz, epsilon, align_sz = 8;
    MPID_Datatype *dtp;
    MPID_Datatype_contents *cp;

    MPIR_Type_get_envelope_impl(type, &nr_ints, &nr_aints, &nr_types, &combiner);

    /* hardcoded handling of MPICH contents format... */
    MPID_Datatype_get_ptr(type, dtp);
    DLOOP_Assert(dtp != NULL);

    cp = dtp->contents;
    DLOOP_Assert(cp != NULL);

#ifdef HAVE_MAX_STRUCT_ALIGNMENT
    if (align_sz > HAVE_MAX_STRUCT_ALIGNMENT) {
	align_sz = HAVE_MAX_STRUCT_ALIGNMENT;
    }
#endif

    struct_sz = sizeof(MPID_Datatype_contents);
    types_sz  = nr_types * sizeof(MPI_Datatype);
    ints_sz   = nr_ints * sizeof(int);

    if ((epsilon = struct_sz % align_sz)) {
	struct_sz += align_sz - epsilon;
    }
    if ((epsilon = types_sz % align_sz)) {
	types_sz += align_sz - epsilon;
    }
    if ((epsilon = ints_sz % align_sz)) {
	ints_sz += align_sz - epsilon;
    }
    *types_p = (MPI_Datatype *) (((char *) cp) + struct_sz);
    *ints_p  = (int *) (((char *) (*types_p)) + types_sz);
    *aints_p = (MPI_Aint *) (((char *) (*ints_p)) + ints_sz);
    /* end of hardcoded handling of MPICH contents format */

    return;
}

/* FIXME: Is this routine complete?  Why is it needed? If it is needed, it 
   must have a comment that describes why it is needed and the arguments 
   must have ATTRIBUTE((unused)) */
void MPID_Type_release_contents(MPI_Datatype type,
				int **ints_p,
				MPI_Aint **aints_p,
				MPI_Datatype **types_p)
{
    return;
}
					   

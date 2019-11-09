/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *   Copyright (C) 1997 University of Chicago.
 *   See COPYRIGHT notice in top-level directory.
 */

#include "adio.h"

#if defined(ROMIO_INSIDE_MPICH) || defined(HAVE_MPIR_DATATYPE_ISCONTIG)

/* MPICH also provides this routine */
void MPIR_Datatype_iscontig(MPI_Datatype datatype, int *flag);

void ADIOI_Datatype_iscontig(MPI_Datatype datatype, int *flag)
{
    MPIR_Datatype_iscontig(datatype, flag);

    /* if the datatype is reported as contigous, check if the true_lb is
     * non-zero, and if so, mark the datatype as noncontiguous */
    if (*flag) {
        MPI_Aint true_extent, true_lb;

        MPI_Type_get_true_extent(datatype, &true_lb, &true_extent);

        if (true_lb > 0)
            *flag = 0;
    }
}

#elif (defined(MPIHP) && defined(HAVE_MPI_INFO))
/* i.e. HPMPI 1.4 only */

int hpmp_dtiscontig(MPI_Datatype datatype);

void ADIOI_Datatype_iscontig(MPI_Datatype datatype, int *flag)
{
    *flag = hpmp_dtiscontig(datatype);
}

#elif (defined(MPISGI) && !defined(NO_MPI_SGI_type_is_contig))

int MPI_SGI_type_is_contig(MPI_Datatype datatype);

void ADIOI_Datatype_iscontig(MPI_Datatype datatype, int *flag)
{
    MPI_Aint displacement;
    MPI_Type_lb(datatype, &distplacement);

    /* SGI's MPI_SGI_type_is_contig() returns true for indexed
     * datatypes with holes at the beginning, which causes
     * problems with ROMIO's use of this function.
     */
    *flag = MPI_SGI_type_is_contig(datatype) && (displacement == 0);
}

#elif defined(ROMIO_INSIDE_OMPI)

/* void ADIOI_Datatype_iscontig(MPI_Datatype datatype, int *flag) is defined
 * and implemented in OpenMPI itself */

#else

void ADIOI_Datatype_iscontig(MPI_Datatype datatype, int *flag)
{
    int i, nints, nadds, ntypes, combiner, *ints;
    MPI_Aint *adds;
    MPI_Datatype *types;

    MPI_Type_get_envelope(datatype, &nints, &nadds, &ntypes, &combiner);

    if (combiner == MPI_COMBINER_NAMED) {
        *flag = 1;
        return;
    }

    if (combiner == MPI_COMBINER_RESIZED) {
        /* because nints == 0, cannot check ints[0] */
        *flag = 0;
        return;
    }

    ints = (int *) ADIOI_Malloc((nints + 1) * sizeof(int));
    adds = (MPI_Aint *) ADIOI_Malloc((nadds + 1) * sizeof(MPI_Aint));
    types = (MPI_Datatype *) ADIOI_Malloc((ntypes + 1) * sizeof(MPI_Datatype));
    MPI_Type_get_contents(datatype, nints, nadds, ntypes, ints, adds, types);

    if (ints[0] == 0)   /* zero-size datatype */
        *flag = 1;
    else if (combiner == MPI_COMBINER_CONTIGUOUS)
        ADIOI_Datatype_iscontig(types[0], flag);
    else
        *flag = 0;

    for (i = 0; i < ntypes; i++) {
        int ni, na, nt, cb;
        MPI_Type_get_envelope(types[i], &ni, &na, &nt, &cb);
        if (cb != MPI_COMBINER_NAMED)
            MPI_Type_free(types + i);
    }

    ADIOI_Free(ints);
    ADIOI_Free(adds);
    ADIOI_Free(types);
}
#endif

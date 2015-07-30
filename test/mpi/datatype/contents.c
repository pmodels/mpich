/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include "mpitestconf.h"
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <stdio.h>
#include <stdlib.h>

static int verbose = 0;

/* tests */
int builtin_float_test(void);
int vector_of_vectors_test(void);
int optimizable_vector_of_basics_test(void);
int indexed_of_basics_test(void);
int indexed_of_vectors_test(void);
int struct_of_basics_test(void);

/* helper functions */
char *combiner_to_string(int combiner);
int parse_args(int argc, char **argv);

int main(int argc, char **argv)
{
    int err, errs = 0;

    MPI_Init(&argc, &argv);     /* MPI-1.2 doesn't allow for MPI_Init(0,0) */
    parse_args(argc, argv);

    /* To improve reporting of problems about operations, we
     * change the error handler to errors return */
    MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);

    /* perform some tests */
    err = builtin_float_test();
    errs += err;
    if (err) {
        fprintf(stderr, "Found %d errors in builtin float test.\n", err);
    }

    err = vector_of_vectors_test();
    errs += err;
    if (err) {
        fprintf(stderr, "Found %d errors in vector of vectors test.\n", err);
    }

    err = optimizable_vector_of_basics_test();
    errs += err;
    if (err) {
        fprintf(stderr, "Found %d errors in vector of basics test.\n", err);
    }

    err = indexed_of_basics_test();
    errs += err;
    if (err) {
        fprintf(stderr, "Found %d errors in indexed of basics test.\n", err);
    }

    err = indexed_of_vectors_test();
    errs += err;
    if (err) {
        fprintf(stderr, "Found %d errors in indexed of vectors test.\n", err);
    }

#ifdef HAVE_MPI_TYPE_CREATE_STRUCT
    err = struct_of_basics_test();
    errs += err;
#endif

    /* print message and exit */
    if (errs) {
        fprintf(stderr, "Found %d errors\n", errs);
    }
    else {
        printf(" No Errors\n");
    }
    MPI_Finalize();
    return 0;
}

/* builtin_float_test()
 *
 * Tests functionality of get_envelope() and get_contents() on a MPI_FLOAT.
 *
 * Returns the number of errors encountered.
 */
int builtin_float_test(void)
{
    int nints, nadds, ntypes, combiner;

    int err, errs = 0;

    err = MPI_Type_get_envelope(MPI_FLOAT, &nints, &nadds, &ntypes, &combiner);

    if (combiner != MPI_COMBINER_NAMED)
        errs++;
    if (verbose && combiner != MPI_COMBINER_NAMED)
        fprintf(stderr, "combiner = %s; should be named\n", combiner_to_string(combiner));

    /* Note: it is erroneous to call MPI_Type_get_contents() on a basic. */
    return errs;
}

/* vector_of_vectors_test()
 *
 * Builds a vector of a vector of ints.  Assuming an int array of size 9
 * integers, and treating the array as a 3x3 2D array, this will grab the
 * corners.
 *
 * Returns the number of errors encountered.
 */
int vector_of_vectors_test(void)
{
    MPI_Datatype inner_vector, inner_vector_copy;
    MPI_Datatype outer_vector;

    int nints, nadds, ntypes, combiner, *ints;
    MPI_Aint *adds = NULL;
    MPI_Datatype *types;

    int err, errs = 0;

    /* set up type */
    err = MPI_Type_vector(2, 1, 2, MPI_INT, &inner_vector);
    if (err != MPI_SUCCESS) {
        if (verbose)
            fprintf(stderr, "error in MPI call; aborting after %d errors\n", errs + 1);
        return errs + 1;
    }

    err = MPI_Type_vector(2, 1, 2, inner_vector, &outer_vector);
    if (err != MPI_SUCCESS) {
        if (verbose)
            fprintf(stderr, "error in MPI call; aborting after %d errors\n", errs + 1);
        return errs + 1;
    }

    /* decode outer vector (get envelope, then contents) */
    err = MPI_Type_get_envelope(outer_vector, &nints, &nadds, &ntypes, &combiner);
    if (err != MPI_SUCCESS) {
        if (verbose)
            fprintf(stderr, "error in MPI call; aborting after %d errors\n", errs + 1);
        return errs + 1;
    }

    if (nints != 3)
        errs++;
    if (nadds != 0)
        errs++;
    if (ntypes != 1)
        errs++;
    if (combiner != MPI_COMBINER_VECTOR)
        errs++;

    if (verbose) {
        if (nints != 3)
            fprintf(stderr, "outer vector nints = %d; should be 3\n", nints);
        if (nadds != 0)
            fprintf(stderr, "outer vector nadds = %d; should be 0\n", nadds);
        if (ntypes != 1)
            fprintf(stderr, "outer vector ntypes = %d; should be 1\n", ntypes);
        if (combiner != MPI_COMBINER_VECTOR)
            fprintf(stderr, "outer vector combiner = %s; should be vector\n",
                    combiner_to_string(combiner));
    }
    if (errs) {
        if (verbose)
            fprintf(stderr, "aborting after %d errors\n", errs);
        return errs;
    }

    ints = malloc(nints * sizeof(*ints));
    if (nadds)
        adds = malloc(nadds * sizeof(*adds));
    types = malloc(ntypes * sizeof(*types));

    /* get contents of outer vector */
    err = MPI_Type_get_contents(outer_vector, nints, nadds, ntypes, ints, adds, types);

    if (ints[0] != 2)
        errs++;
    if (ints[1] != 1)
        errs++;
    if (ints[2] != 2)
        errs++;

    if (verbose) {
        if (ints[0] != 2)
            fprintf(stderr, "outer vector count = %d; should be 2\n", ints[0]);
        if (ints[1] != 1)
            fprintf(stderr, "outer vector blocklength = %d; should be 1\n", ints[1]);
        if (ints[2] != 2)
            fprintf(stderr, "outer vector stride = %d; should be 2\n", ints[2]);
    }
    if (errs) {
        if (verbose)
            fprintf(stderr, "aborting after %d errors\n", errs);
        return errs;
    }

    inner_vector_copy = types[0];
    free(ints);
    if (nadds)
        free(adds);
    free(types);

    /* decode inner vector */
    err = MPI_Type_get_envelope(inner_vector_copy, &nints, &nadds, &ntypes, &combiner);
    if (err != MPI_SUCCESS) {
        if (verbose)
            fprintf(stderr, "error in MPI call; aborting after %d errors\n", errs + 1);
        return errs + 1;
    }

    if (nints != 3)
        errs++;
    if (nadds != 0)
        errs++;
    if (ntypes != 1)
        errs++;
    if (combiner != MPI_COMBINER_VECTOR)
        errs++;

    if (verbose) {
        if (nints != 3)
            fprintf(stderr, "inner vector nints = %d; should be 3\n", nints);
        if (nadds != 0)
            fprintf(stderr, "inner vector nadds = %d; should be 0\n", nadds);
        if (ntypes != 1)
            fprintf(stderr, "inner vector ntypes = %d; should be 1\n", ntypes);
        if (combiner != MPI_COMBINER_VECTOR)
            fprintf(stderr, "inner vector combiner = %s; should be vector\n",
                    combiner_to_string(combiner));
    }
    if (errs) {
        if (verbose)
            fprintf(stderr, "aborting after %d errors\n", errs);
        return errs;
    }

    ints = malloc(nints * sizeof(*ints));
    if (nadds)
        adds = malloc(nadds * sizeof(*adds));
    types = malloc(ntypes * sizeof(*types));

    err = MPI_Type_get_contents(inner_vector_copy, nints, nadds, ntypes, ints, adds, types);

    if (ints[0] != 2)
        errs++;
    if (ints[1] != 1)
        errs++;
    if (ints[2] != 2)
        errs++;

    if (verbose) {
        if (ints[0] != 2)
            fprintf(stderr, "inner vector count = %d; should be 2\n", ints[0]);
        if (ints[1] != 1)
            fprintf(stderr, "inner vector blocklength = %d; should be 1\n", ints[1]);
        if (ints[2] != 2)
            fprintf(stderr, "inner vector stride = %d; should be 2\n", ints[2]);
    }
    if (errs) {
        if (verbose)
            fprintf(stderr, "aborting after %d errors\n", errs);
        return errs;
    }

    free(ints);
    if (nadds)
        free(adds);
    free(types);

    MPI_Type_free(&inner_vector_copy);
    MPI_Type_free(&inner_vector);
    MPI_Type_free(&outer_vector);

    return 0;
}

/* optimizable_vector_of_basics_test()
 *
 * Builds a vector of ints.  Count is 10, blocksize is 2, stride is 2, so this
 * is equivalent to a contig of 20.  But remember...we should get back our
 * suboptimal values under MPI-2.
 *
 * Returns the number of errors encountered.
 */
int optimizable_vector_of_basics_test(void)
{
    MPI_Datatype parent_type;

    int nints, nadds, ntypes, combiner, *ints;
    MPI_Aint *adds = NULL;
    MPI_Datatype *types;

    int err, errs = 0;

    /* set up type */
    err = MPI_Type_vector(10, 2, 2, MPI_INT, &parent_type);

    /* decode */
    err = MPI_Type_get_envelope(parent_type, &nints, &nadds, &ntypes, &combiner);

    if (nints != 3)
        errs++;
    if (nadds != 0)
        errs++;
    if (ntypes != 1)
        errs++;
    if (combiner != MPI_COMBINER_VECTOR)
        errs++;

    if (verbose) {
        if (nints != 3)
            fprintf(stderr, "nints = %d; should be 3\n", nints);
        if (nadds != 0)
            fprintf(stderr, "nadds = %d; should be 0\n", nadds);
        if (ntypes != 1)
            fprintf(stderr, "ntypes = %d; should be 1\n", ntypes);
        if (combiner != MPI_COMBINER_VECTOR)
            fprintf(stderr, "combiner = %s; should be vector\n", combiner_to_string(combiner));
    }

    ints = malloc(nints * sizeof(*ints));
    if (nadds)
        adds = malloc(nadds * sizeof(*adds));
    types = malloc(ntypes * sizeof(*types));

    err = MPI_Type_get_contents(parent_type, nints, nadds, ntypes, ints, adds, types);

    if (ints[0] != 10)
        errs++;
    if (ints[1] != 2)
        errs++;
    if (ints[2] != 2)
        errs++;
    if (types[0] != MPI_INT)
        errs++;

    if (verbose) {
        if (ints[0] != 10)
            fprintf(stderr, "count = %d; should be 10\n", ints[0]);
        if (ints[1] != 2)
            fprintf(stderr, "blocklength = %d; should be 2\n", ints[1]);
        if (ints[2] != 2)
            fprintf(stderr, "stride = %d; should be 2\n", ints[2]);
        if (types[0] != MPI_INT)
            fprintf(stderr, "type is not MPI_INT\n");
    }

    free(ints);
    if (nadds)
        free(adds);
    free(types);

    MPI_Type_free(&parent_type);

    return errs;
}


/* indexed_of_basics_test(void)
 *
 * Simple indexed type.
 *
 * Returns number of errors encountered.
 */
int indexed_of_basics_test(void)
{
    MPI_Datatype parent_type;
    int s_count = 3, s_blocklengths[3] = { 3, 2, 1 };
    int s_displacements[3] = { 10, 20, 30 };

    int nints, nadds, ntypes, combiner, *ints;
    MPI_Aint *adds = NULL;
    MPI_Datatype *types;

    int err, errs = 0;

    /* set up type */
    err = MPI_Type_indexed(s_count, s_blocklengths, s_displacements, MPI_INT, &parent_type);

    /* decode */
    err = MPI_Type_get_envelope(parent_type, &nints, &nadds, &ntypes, &combiner);

    if (nints != 7)
        errs++;
    if (nadds != 0)
        errs++;
    if (ntypes != 1)
        errs++;
    if (combiner != MPI_COMBINER_INDEXED)
        errs++;

    if (verbose) {
        if (nints != 7)
            fprintf(stderr, "nints = %d; should be 7\n", nints);
        if (nadds != 0)
            fprintf(stderr, "nadds = %d; should be 0\n", nadds);
        if (ntypes != 1)
            fprintf(stderr, "ntypes = %d; should be 1\n", ntypes);
        if (combiner != MPI_COMBINER_INDEXED)
            fprintf(stderr, "combiner = %s; should be indexed\n", combiner_to_string(combiner));
    }

    ints = malloc(nints * sizeof(*ints));
    if (nadds)
        adds = malloc(nadds * sizeof(*adds));
    types = malloc(ntypes * sizeof(*types));

    err = MPI_Type_get_contents(parent_type, nints, nadds, ntypes, ints, adds, types);

    if (ints[0] != s_count)
        errs++;
    if (ints[1] != s_blocklengths[0])
        errs++;
    if (ints[2] != s_blocklengths[1])
        errs++;
    if (ints[3] != s_blocklengths[2])
        errs++;
    if (ints[4] != s_displacements[0])
        errs++;
    if (ints[5] != s_displacements[1])
        errs++;
    if (ints[6] != s_displacements[2])
        errs++;
    if (types[0] != MPI_INT)
        errs++;

    if (verbose) {
        if (ints[0] != s_count)
            fprintf(stderr, "count = %d; should be %d\n", ints[0], s_count);
        if (ints[1] != s_blocklengths[0])
            fprintf(stderr, "blocklength[0] = %d; should be %d\n", ints[1], s_blocklengths[0]);
        if (ints[2] != s_blocklengths[1])
            fprintf(stderr, "blocklength[1] = %d; should be %d\n", ints[2], s_blocklengths[1]);
        if (ints[3] != s_blocklengths[2])
            fprintf(stderr, "blocklength[2] = %d; should be %d\n", ints[3], s_blocklengths[2]);
        if (ints[4] != s_displacements[0])
            fprintf(stderr, "displacement[0] = %d; should be %d\n", ints[4], s_displacements[0]);
        if (ints[5] != s_displacements[1])
            fprintf(stderr, "displacement[1] = %d; should be %d\n", ints[5], s_displacements[1]);
        if (ints[6] != s_displacements[2])
            fprintf(stderr, "displacement[2] = %d; should be %d\n", ints[6], s_displacements[2]);
        if (types[0] != MPI_INT)
            fprintf(stderr, "type[0] does not match\n");
    }

    free(ints);
    if (nadds)
        free(adds);
    free(types);

    MPI_Type_free(&parent_type);
    return errs;
}

/* indexed_of_vectors_test()
 *
 * Builds an indexed type of vectors of ints.
 *
 * Returns the number of errors encountered.
 */
int indexed_of_vectors_test(void)
{
    MPI_Datatype inner_vector, inner_vector_copy;
    MPI_Datatype outer_indexed;

    int i_count = 3, i_blocklengths[3] = { 3, 2, 1 };
    int i_displacements[3] = { 10, 20, 30 };

    int nints, nadds, ntypes, combiner, *ints;
    MPI_Aint *adds = NULL;
    MPI_Datatype *types;

    int err, errs = 0;

    /* set up type */
    err = MPI_Type_vector(2, 1, 2, MPI_INT, &inner_vector);
    if (err != MPI_SUCCESS) {
        if (verbose)
            fprintf(stderr, "error in MPI call; aborting after %d errors\n", errs + 1);
        return errs + 1;
    }

    err = MPI_Type_indexed(i_count, i_blocklengths, i_displacements, inner_vector, &outer_indexed);
    if (err != MPI_SUCCESS) {
        if (verbose)
            fprintf(stderr, "error in MPI call; aborting after %d errors\n", errs + 1);
        return errs + 1;
    }

    /* decode outer vector (get envelope, then contents) */
    err = MPI_Type_get_envelope(outer_indexed, &nints, &nadds, &ntypes, &combiner);
    if (err != MPI_SUCCESS) {
        if (verbose)
            fprintf(stderr, "error in MPI call; aborting after %d errors\n", errs + 1);
        return errs + 1;
    }

    if (nints != 7)
        errs++;
    if (nadds != 0)
        errs++;
    if (ntypes != 1)
        errs++;
    if (combiner != MPI_COMBINER_INDEXED)
        errs++;

    if (verbose) {
        if (nints != 7)
            fprintf(stderr, "nints = %d; should be 7\n", nints);
        if (nadds != 0)
            fprintf(stderr, "nadds = %d; should be 0\n", nadds);
        if (ntypes != 1)
            fprintf(stderr, "ntypes = %d; should be 1\n", ntypes);
        if (combiner != MPI_COMBINER_INDEXED)
            fprintf(stderr, "combiner = %s; should be indexed\n", combiner_to_string(combiner));
    }

    if (errs) {
        if (verbose)
            fprintf(stderr, "aborting after %d errors\n", errs);
        return errs;
    }

    ints = malloc(nints * sizeof(*ints));
    if (nadds)
        adds = malloc(nadds * sizeof(*adds));
    types = malloc(ntypes * sizeof(*types));

    /* get contents of outer vector */
    err = MPI_Type_get_contents(outer_indexed, nints, nadds, ntypes, ints, adds, types);

    if (ints[0] != i_count)
        errs++;
    if (ints[1] != i_blocklengths[0])
        errs++;
    if (ints[2] != i_blocklengths[1])
        errs++;
    if (ints[3] != i_blocklengths[2])
        errs++;
    if (ints[4] != i_displacements[0])
        errs++;
    if (ints[5] != i_displacements[1])
        errs++;
    if (ints[6] != i_displacements[2])
        errs++;

    if (verbose) {
        if (ints[0] != i_count)
            fprintf(stderr, "count = %d; should be %d\n", ints[0], i_count);
        if (ints[1] != i_blocklengths[0])
            fprintf(stderr, "blocklength[0] = %d; should be %d\n", ints[1], i_blocklengths[0]);
        if (ints[2] != i_blocklengths[1])
            fprintf(stderr, "blocklength[1] = %d; should be %d\n", ints[2], i_blocklengths[1]);
        if (ints[3] != i_blocklengths[2])
            fprintf(stderr, "blocklength[2] = %d; should be %d\n", ints[3], i_blocklengths[2]);
        if (ints[4] != i_displacements[0])
            fprintf(stderr, "displacement[0] = %d; should be %d\n", ints[4], i_displacements[0]);
        if (ints[5] != i_displacements[1])
            fprintf(stderr, "displacement[1] = %d; should be %d\n", ints[5], i_displacements[1]);
        if (ints[6] != i_displacements[2])
            fprintf(stderr, "displacement[2] = %d; should be %d\n", ints[6], i_displacements[2]);
    }

    if (errs) {
        if (verbose)
            fprintf(stderr, "aborting after %d errors\n", errs);
        return errs;
    }

    inner_vector_copy = types[0];
    free(ints);
    if (nadds)
        free(adds);
    free(types);

    /* decode inner vector */
    err = MPI_Type_get_envelope(inner_vector_copy, &nints, &nadds, &ntypes, &combiner);
    if (err != MPI_SUCCESS) {
        if (verbose)
            fprintf(stderr, "error in MPI call; aborting after %d errors\n", errs + 1);
        return errs + 1;
    }

    if (nints != 3)
        errs++;
    if (nadds != 0)
        errs++;
    if (ntypes != 1)
        errs++;
    if (combiner != MPI_COMBINER_VECTOR)
        errs++;

    if (verbose) {
        if (nints != 3)
            fprintf(stderr, "inner vector nints = %d; should be 3\n", nints);
        if (nadds != 0)
            fprintf(stderr, "inner vector nadds = %d; should be 0\n", nadds);
        if (ntypes != 1)
            fprintf(stderr, "inner vector ntypes = %d; should be 1\n", ntypes);
        if (combiner != MPI_COMBINER_VECTOR)
            fprintf(stderr, "inner vector combiner = %s; should be vector\n",
                    combiner_to_string(combiner));
    }
    if (errs) {
        if (verbose)
            fprintf(stderr, "aborting after %d errors\n", errs);
        return errs;
    }

    ints = malloc(nints * sizeof(*ints));
    if (nadds)
        adds = malloc(nadds * sizeof(*adds));
    types = malloc(ntypes * sizeof(*types));

    err = MPI_Type_get_contents(inner_vector_copy, nints, nadds, ntypes, ints, adds, types);

    if (ints[0] != 2)
        errs++;
    if (ints[1] != 1)
        errs++;
    if (ints[2] != 2)
        errs++;

    if (verbose) {
        if (ints[0] != 2)
            fprintf(stderr, "inner vector count = %d; should be 2\n", ints[0]);
        if (ints[1] != 1)
            fprintf(stderr, "inner vector blocklength = %d; should be 1\n", ints[1]);
        if (ints[2] != 2)
            fprintf(stderr, "inner vector stride = %d; should be 2\n", ints[2]);
    }
    if (errs) {
        if (verbose)
            fprintf(stderr, "aborting after %d errors\n", errs);
        return errs;
    }

    free(ints);
    if (nadds)
        free(adds);
    free(types);

    MPI_Type_free(&inner_vector_copy);
    MPI_Type_free(&inner_vector);
    MPI_Type_free(&outer_indexed);

    return 0;
}


#ifdef HAVE_MPI_TYPE_CREATE_STRUCT
/* struct_of_basics_test(void)
 *
 * There's nothing simple about structs :).  Although this is an easy one.
 *
 * Returns number of errors encountered.
 *
 * NOT TESTED.
 */
int struct_of_basics_test(void)
{
    MPI_Datatype parent_type;
    int s_count = 3, s_blocklengths[3] = { 3, 2, 1 };
    MPI_Aint s_displacements[3] = { 10, 20, 30 };
    MPI_Datatype s_types[3] = { MPI_CHAR, MPI_INT, MPI_FLOAT };

    int nints, nadds, ntypes, combiner, *ints;
    MPI_Aint *adds = NULL;
    MPI_Datatype *types;

    int err, errs = 0;

    /* set up type */
    err = MPI_Type_create_struct(s_count, s_blocklengths, s_displacements, s_types, &parent_type);

    /* decode */
    err = MPI_Type_get_envelope(parent_type, &nints, &nadds, &ntypes, &combiner);

    if (nints != 4)
        errs++;
    if (nadds != 3)
        errs++;
    if (ntypes != 3)
        errs++;
    if (combiner != MPI_COMBINER_STRUCT)
        errs++;

    if (verbose) {
        if (nints != 4)
            fprintf(stderr, "nints = %d; should be 3\n", nints);
        if (nadds != 3)
            fprintf(stderr, "nadds = %d; should be 0\n", nadds);
        if (ntypes != 3)
            fprintf(stderr, "ntypes = %d; should be 3\n", ntypes);
        if (combiner != MPI_COMBINER_STRUCT)
            fprintf(stderr, "combiner = %s; should be struct\n", combiner_to_string(combiner));
    }

    ints = malloc(nints * sizeof(*ints));
    adds = malloc(nadds * sizeof(*adds));
    types = malloc(ntypes * sizeof(*types));

    err = MPI_Type_get_contents(parent_type, nints, nadds, ntypes, ints, adds, types);

    if (ints[0] != s_count)
        errs++;
    if (ints[1] != s_blocklengths[0])
        errs++;
    if (ints[2] != s_blocklengths[1])
        errs++;
    if (ints[3] != s_blocklengths[2])
        errs++;
    if (adds[0] != s_displacements[0])
        errs++;
    if (adds[1] != s_displacements[1])
        errs++;
    if (adds[2] != s_displacements[2])
        errs++;
    if (types[0] != s_types[0])
        errs++;
    if (types[1] != s_types[1])
        errs++;
    if (types[2] != s_types[2])
        errs++;

    if (verbose) {
        if (ints[0] != s_count)
            fprintf(stderr, "count = %d; should be %d\n", ints[0], s_count);
        if (ints[1] != s_blocklengths[0])
            fprintf(stderr, "blocklength[0] = %d; should be %d\n", ints[1], s_blocklengths[0]);
        if (ints[2] != s_blocklengths[1])
            fprintf(stderr, "blocklength[1] = %d; should be %d\n", ints[2], s_blocklengths[1]);
        if (ints[3] != s_blocklengths[2])
            fprintf(stderr, "blocklength[2] = %d; should be %d\n", ints[3], s_blocklengths[2]);
        if (adds[0] != s_displacements[0])
            fprintf(stderr, "displacement[0] = %d; should be %d\n", adds[0], s_displacements[0]);
        if (adds[1] != s_displacements[1])
            fprintf(stderr, "displacement[1] = %d; should be %d\n", adds[1], s_displacements[1]);
        if (adds[2] != s_displacements[2])
            fprintf(stderr, "displacement[2] = %d; should be %d\n", adds[2], s_displacements[2]);
        if (types[0] != s_types[0])
            fprintf(stderr, "type[0] does not match\n");
        if (types[1] != s_types[1])
            fprintf(stderr, "type[1] does not match\n");
        if (types[2] != s_types[2])
            fprintf(stderr, "type[2] does not match\n");
    }

    free(ints);
    free(adds);
    free(types);

    MPI_Type_free(&parent_type);

    return errs;
}
#endif

/* combiner_to_string(combiner)
 *
 * Converts a numeric combiner into a pointer to a string used for printing.
 */
char *combiner_to_string(int combiner)
{
    static char c_named[] = "named";
    static char c_contig[] = "contig";
    static char c_vector[] = "vector";
    static char c_hvector[] = "hvector";
    static char c_indexed[] = "indexed";
    static char c_hindexed[] = "hindexed";
    static char c_struct[] = "struct";
#ifdef HAVE_MPI2_COMBINERS
    static char c_dup[] = "dup";
    static char c_hvector_integer[] = "hvector_integer";
    static char c_hindexed_integer[] = "hindexed_integer";
    static char c_indexed_block[] = "indexed_block";
    static char c_struct_integer[] = "struct_integer";
    static char c_subarray[] = "subarray";
    static char c_darray[] = "darray";
    static char c_f90_real[] = "f90_real";
    static char c_f90_complex[] = "f90_complex";
    static char c_f90_integer[] = "f90_integer";
    static char c_resized[] = "resized";
#endif

    if (combiner == MPI_COMBINER_NAMED)
        return c_named;
    if (combiner == MPI_COMBINER_CONTIGUOUS)
        return c_contig;
    if (combiner == MPI_COMBINER_VECTOR)
        return c_vector;
    if (combiner == MPI_COMBINER_HVECTOR)
        return c_hvector;
    if (combiner == MPI_COMBINER_INDEXED)
        return c_indexed;
    if (combiner == MPI_COMBINER_HINDEXED)
        return c_hindexed;
    if (combiner == MPI_COMBINER_STRUCT)
        return c_struct;
#ifdef HAVE_MPI2_COMBINERS
    if (combiner == MPI_COMBINER_DUP)
        return c_dup;
    if (combiner == MPI_COMBINER_HVECTOR_INTEGER)
        return c_hvector_integer;
    if (combiner == MPI_COMBINER_HINDEXED_INTEGER)
        return c_hindexed_integer;
    if (combiner == MPI_COMBINER_INDEXED_BLOCK)
        return c_indexed_block;
    if (combiner == MPI_COMBINER_STRUCT_INTEGER)
        return c_struct_integer;
    if (combiner == MPI_COMBINER_SUBARRAY)
        return c_subarray;
    if (combiner == MPI_COMBINER_DARRAY)
        return c_darray;
    if (combiner == MPI_COMBINER_F90_REAL)
        return c_f90_real;
    if (combiner == MPI_COMBINER_F90_COMPLEX)
        return c_f90_complex;
    if (combiner == MPI_COMBINER_F90_INTEGER)
        return c_f90_integer;
    if (combiner == MPI_COMBINER_RESIZED)
        return c_resized;
#endif

    return NULL;
}

int parse_args(int argc, char **argv)
{
#ifdef HAVE_GET_OPT
    int ret;

    while ((ret = getopt(argc, argv, "v")) >= 0) {
        switch (ret) {
        case 'v':
            verbose = 1;
            break;
        }
    }
#else
#endif
    return 0;
}

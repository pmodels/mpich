/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
/*
 * This program was sent in as an example that did not perform as expected.
 * The program has a bug in that it is sending 3 characters but receiving
 * three integers, which is not a valid MPI program (the type signatures 
 * must match).  However, a good MPI implementation will handle this 
 * gracefully, which is why this test is included in the error directory
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mpi.h"
#include "mpitest.h"

#define BUFSIZE 128

#define FAIL_ERROR       123
#define RESFAIL_ERROR    123
#define INTERNAL_ERROR   123

void if_error(const char *function, const char *data, int ret);
void if_error(const char *function, const char *data, int ret)
{
    if (ret == 0)
        return;

    if (data)
        printf("%s for %s returned %d (%#x)\n", function, data, ret,ret);
    else
        printf("%s returned %d (%#x)\n", function, ret, ret);

    exit(INTERNAL_ERROR);
}

int main (int argc, char *argv[])
{
    int ret,errs = 0;
    char *src, *sendrec;
    int bufsize = BUFSIZE;

    int myrank, nprocs;
    int i;
    MPI_Status status;

    int small_non_contig_struct_count = 3;
    int small_non_contig_struct_blocklens[] = {1, 1, 1};
    MPI_Aint small_non_contig_struct_disps[] = {0, 2, 4};
    MPI_Datatype small_non_contig_struct_types[] = {MPI_CHAR, MPI_CHAR,MPI_CHAR};
    MPI_Datatype small_non_contig_struct_type;

    int contig_indexed_count = 3;
    int contig_indexed_blocklens[] = {1, 2, 1};
    int contig_indexed_indices[] = {4, 8, 16};
    int contig_indexed_inner_type = MPI_INT;
    int contig_indexed_type;

    MTest_Init( &argc, &argv );
    ret = MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
    ret = MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    if (nprocs < 2) {
        printf("Need at least 2 procs\n");
        exit(RESFAIL_ERROR);
    }

    ret = MPI_Type_struct(small_non_contig_struct_count,
			  small_non_contig_struct_blocklens, 
			  small_non_contig_struct_disps,
			  small_non_contig_struct_types, 
			  &small_non_contig_struct_type);
    if_error("MPI_Type_struct", "small_non_contig_struct_type", ret);

    ret = MPI_Type_commit(&small_non_contig_struct_type);
    if_error("MPI_Type_commit", "small_non_contig_struct_type", ret);

    ret = MPI_Type_indexed(contig_indexed_count,contig_indexed_blocklens, 
			   contig_indexed_indices,contig_indexed_inner_type, 
			   &contig_indexed_type);
    if_error("MPI_Type_indexed", "contig_indexed_type", ret);

    ret = MPI_Type_commit(&contig_indexed_type);
    if_error("MPI_Type_commit", "contig_indexed_type", ret);


    ret = MPI_Alloc_mem(bufsize, MPI_INFO_NULL, &src);

    if (ret != 0) {
        printf("MPI_Alloc_mem src = #%x\n", ret);
        exit(INTERNAL_ERROR);
    }

    ret = MPI_Alloc_mem(bufsize, MPI_INFO_NULL, &sendrec);

    if (ret != 0) {
        printf("MPI_Alloc_mem sendrec buf = #%x\n", ret);
        exit(INTERNAL_ERROR);
    }


    for (i=0; i<bufsize; i++) {
        src[i] = (char) i+1;
    }

    memset(sendrec, 0, bufsize);

    MPI_Barrier(MPI_COMM_WORLD);
    if (myrank == 1) {
        MPI_Send(src, 1, small_non_contig_struct_type, 0, 0xabc,MPI_COMM_WORLD);
    } else {
	MPI_Errhandler_set( MPI_COMM_WORLD, MPI_ERRORS_RETURN );
        ret = MPI_Recv(sendrec, 1, contig_indexed_type, 1, 0xabc,
		       MPI_COMM_WORLD, &status);
	if (ret == MPI_SUCCESS) {
	    printf( "MPI_Recv succeeded with non-matching datatype signature\n" );
	    errs++;
	}
    }

    MPI_Barrier(MPI_COMM_WORLD);

    MPI_Type_free( &small_non_contig_struct_type );
    MPI_Type_free( &contig_indexed_type );

    MPI_Free_mem(src);
    MPI_Free_mem(sendrec);

    MTest_Finalize( errs );
    MPI_Finalize( );

    return 0;
}


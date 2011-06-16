#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mpi.h"
#include "mpitest.h"

#define RING_NUM_NEIGHBORS   2

static int validate_dgraph(MPI_Comm dgraph_comm)
{
    int         comm_topo;
    int         src_sz, dest_sz;
    int         wgt_flag, ierr;
    int         srcs[RING_NUM_NEIGHBORS], dests[RING_NUM_NEIGHBORS];
    int        *src_wgts, *dest_wgts;

    int         world_rank, world_size;
    int         idx, nbr_sep;

    comm_topo = MPI_UNDEFINED;
    MPI_Topo_test(dgraph_comm, &comm_topo);
    switch (comm_topo) {
        case MPI_DIST_GRAPH :
            break;
        default:
            fprintf(stderr, "dgraph_comm is NOT of type MPI_DIST_GRAPH\n");
            return 0;
    }

    ierr = MPI_Dist_graph_neighbors_count(dgraph_comm,
                                          &src_sz, &dest_sz, &wgt_flag);
    if (ierr != MPI_SUCCESS) {
        fprintf(stderr, "MPI_Dist_graph_neighbors_count() fails!\n");
        return 0;
    }
/*
    else
        fprintf(stderr, "MPI_Dist_graph_neighbors_count() succeeds!\n");
*/

    if (wgt_flag) {
        fprintf(stderr, "dgraph_comm is NOT created with MPI_UNWEIGHTED\n");
        return 0;
    }
/*
    else
        fprintf(stderr, "dgraph_comm is created with MPI_UNWEIGHTED\n");
*/
    if (src_sz != RING_NUM_NEIGHBORS || dest_sz != RING_NUM_NEIGHBORS) {
        fprintf(stderr, "source or destination edge array is not of size %d.\n",
                         RING_NUM_NEIGHBORS);
        fprintf(stderr, "src_sz = %d, dest_sz = %d\n", src_sz, dest_sz);
        return 0;
    }

    /*
       src_wgts and dest_wgts could be anything, e.g. NULL, since
       MPI_Dist_graph_neighbors_count() returns MPI_UNWEIGHTED.
       Since this program has a Fortran77 version, and standard Fortran77
       has no pointer and NULL, so use MPI_UNWEIGHTED for the weighted arrays.
    */
    src_wgts  = MPI_UNWEIGHTED;
    dest_wgts = MPI_UNWEIGHTED;
    ierr = MPI_Dist_graph_neighbors(dgraph_comm,
                                    src_sz, srcs, src_wgts,
                                    dest_sz, dests, dest_wgts);
    if (ierr != MPI_SUCCESS) {
        fprintf(stderr, "MPI_Dist_graph_neighbors() fails!\n");
        return 0;
    }
/*
    else
        fprintf(stderr, "MPI_Dist_graph_neighbors() succeeds!\n");
*/

    /*
       Check if the neighbors returned from MPI are really
       the nearest neighbors within a ring.
    */
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
 
    for (idx=0; idx < src_sz; idx++) {
        nbr_sep = abs(srcs[idx] - world_rank);
        if ( nbr_sep != 1 && nbr_sep != (world_size-1) ) {
            fprintf(stderr, "srcs[%d]=%d is NOT a neighbor of my rank %d.\n",
                            idx, srcs[idx], world_rank);
            return 0;
        }  
    }
    for (idx=0; idx < dest_sz; idx++) {
        nbr_sep = abs(dests[idx] - world_rank);
        if ( nbr_sep != 1 && nbr_sep != (world_size-1) ) {
            fprintf(stderr, "dests[%d]=%d is NOT a neighbor of my rank %d.\n",
                            idx, dests[idx], world_rank);
            return 0;
        }  
    }

    /*
    fprintf(stderr, "dgraph_comm is of type MPI_DIST_GRAPH "
                    "of a bidirectional ring.\n");
    */
    return 1;
}

/*
   Specify a distributed graph of a bidirectional ring of the MPI_COMM_WORLD,
   i.e. everyone only talks to left and right neighbors. 
*/
int main(int argc, char *argv[])
{
    MPI_Comm    dgraph_comm;
    int         world_size, world_rank, ierr;
    int         errs = 0;

    int         src_sz, dest_sz;
    int         degs[1];
    int         srcs[RING_NUM_NEIGHBORS], dests[RING_NUM_NEIGHBORS];
    
    MTest_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    degs[0]  = 2;
    srcs[0]  = world_rank;
    dests[0] = world_rank-1 <  0          ? world_size-1 : world_rank-1 ;
    dests[1] = world_rank+1 >= world_size ?            0 : world_rank+1 ;
    ierr = MPI_Dist_graph_create(MPI_COMM_WORLD, 1, srcs, degs, dests,
                                 MPI_UNWEIGHTED, MPI_INFO_NULL, 1,
                                 &dgraph_comm);
    if ( ierr != MPI_SUCCESS )  {
        fprintf(stderr, "MPI_Dist_graph_create() fails!\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
        return 1;
    }
    if (!validate_dgraph(dgraph_comm)) {
        fprintf(stderr, "MPI_Dist_graph_create() does NOT create "
                        "a bidirectional ring graph!\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
        return 1;
    }
    MPI_Comm_free(&dgraph_comm);
    
    src_sz   = 2;
    srcs[0]  = world_rank-1 <  0          ? world_size-1 : world_rank-1 ;
    srcs[1]  = world_rank+1 >= world_size ?            0 : world_rank+1 ;
    dest_sz  = 2;
    dests[0] = world_rank-1 <  0          ? world_size-1 : world_rank-1 ;
    dests[1] = world_rank+1 >= world_size ?            0 : world_rank+1 ;
    ierr = MPI_Dist_graph_create_adjacent(MPI_COMM_WORLD,
                                          src_sz, srcs, MPI_UNWEIGHTED,
                                          dest_sz, dests, MPI_UNWEIGHTED,
                                          MPI_INFO_NULL, 1, &dgraph_comm);
    if ( ierr != MPI_SUCCESS ) {
        fprintf(stderr, "MPI_Dist_graph_create_adjacent() fails!\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
        return 1;
    }
    if (!validate_dgraph(dgraph_comm)) {
        fprintf(stderr, "MPI_Dist_graph_create_adjacent() does NOT create "
                        "a bidirectional ring graph!\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
        return 1;
    }
    MPI_Comm_free(&dgraph_comm);

    MTest_Finalize(errs);
    MPI_Finalize();
    return 0;
}

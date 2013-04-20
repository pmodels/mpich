/* -*- Mode: C++; c-basic-offset:4 ; -*- */
/*
 *  (C) 2013 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpi.h"
#include "mpitestconf.h"
#include "mpitestcxx.h"
#ifdef HAVE_IOSTREAM
// Not all C++ compilers have iostream instead of iostream.h
#include <iostream>
#ifdef HAVE_NAMESPACE_STD
// Those that do often need the std namespace; otherwise, a bare "cout"
// is likely to fail to compile
using namespace std;
#endif
#else
#include <iostream.h>
#endif

int main( int argc, char *argv[] )
{
    int errs = 0;
    int indegree, outdegree, sources[4], dests[4], sweights[4], dweights[4];
    int wrank, wsize;
    MPI::Distgraphcomm dcomm;

    MTest_Init();

    // Create a graph where each process sends to rank+1, rank+3 and
    // receives from rank - 2
    wrank = MPI::COMM_WORLD.Get_rank();
    wsize = MPI::COMM_WORLD.Get_size();
    indegree  = 0;
    outdegree = 0;
    if (wrank+1 < wsize) dests[outdegree++] = wrank+1;
    if (wrank+3 < wsize) dests[outdegree++] = wrank+3;
    if (wrank-2 >= 0) sources[indegree++] = wrank-2;

    // Create with no reordering to test final ranks
    dcomm = MPI::COMM_WORLD.Dist_graph_create_adjacent( indegree, sources,
				outdegree, dests, MPI::INFO_NULL, false );
    
    
    // Check that the created communicator has the properties specified
    if (dcomm.Get_topology() != MPI_DIST_GRAPH) {
	errs++;
	cout << "Incorrect topology for dist_graph: " << 
	    dcomm.Get_topology() << "\n";
    }
    else {
	int myindegree, myoutdegree;
	bool myweighted;
	dcomm.Get_dist_neighbors_count( myindegree, myoutdegree, myweighted );
	if (myindegree != indegree) {
	    errs++;
	    cout << "Indegree is " << myindegree << " should be " << indegree 
		 << "\n";
	}
	if (myoutdegree != outdegree) {
	    errs++;
	    cout << "Outdegree is " << myoutdegree << " should be " << 
		outdegree << "\n";
	}
	if (myweighted) {
	    errs++;
	    cout << "Weighted is true, should be false\n";
	}
    }
    if (!errs) {
	int mysources[4], mysweights[4], mydests[4], mydweights[4], i;
	dcomm.Get_dist_neighbors( 4, mysources, mysweights, 
				  4, mydests, mydweights );
	// May need to sort mysources and mydests first
	for (i=0; i<indegree; i++) {
	    if (mysources[i] != sources[i]) {
		errs++;
	    }
	}
	for (i=0; i<outdegree; i++) {
	    if (mydests[i] != dests[i]) {
		errs++;
	    }
	}
    }
    dcomm.Free();

    // ToDo: 
    // Test dup and clone.  
    // Test with weights.  
    // Test with reorder.
    // Test Dist_graph (not just adjacent) with awkward specification.
    // Note that there is no dist_graph_map function in MPI (!)
  
    MTest_Finalize( errs );
    MPI::Finalize();

    return 0;
}

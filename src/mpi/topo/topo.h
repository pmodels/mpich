/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

typedef struct MPIR_Graph_topology {
  int nnodes;
  int nedges;
  int *index;
  int *edges;
} MPIR_Graph_topology;

typedef struct MPIR_Cart_topology {
  int nnodes;     /* Product of dims[*], gives the size of the topology */
  int ndims;
  int *dims;
  int *periodic;
  int *position;
} MPIR_Cart_topology;

typedef struct MPIR_Dist_graph_topology {
    int indegree;
    int *in;
    int *in_weights;
    int outdegree;
    int *out;
    int *out_weights;
    int is_weighted;
} MPIR_Dist_graph_topology;

typedef struct MPIR_Topology { 
  MPIR_Topo_type kind;
  union topo { 
    MPIR_Graph_topology graph;
    MPIR_Cart_topology  cart;
    MPIR_Dist_graph_topology dist_graph;
  } topo;
} MPIR_Topology;

MPIR_Topology *MPIR_Topology_get( MPID_Comm * );
int MPIR_Topology_put( MPID_Comm *, MPIR_Topology * );
int MPIR_Cart_create( MPID_Comm *, int, const int [], 
		      const int [], int, MPI_Comm * );
int MPIR_Graph_create( MPID_Comm *, int, 
		       const int[], const int[], int, 
		       MPI_Comm *);
int MPIR_Dims_create( int, int, int * );
int MPIR_Graph_map( const MPID_Comm *, int, const int[], const int[], int* );
int MPIR_Cart_map( const MPID_Comm *, int, const int[],  const int[], int* );

/* Returns the canonicalized count of neighbors for the given topology as though
 * MPI_Dist_graph_neighbors_count were called with a distributed graph topology,
 * even if the given topology is actually Cartesian or Graph.  Useful for
 * implementing neighborhood collective operations. */
int MPIR_Topo_canon_nhb_count(MPID_Comm *comm_ptr, int *indegree, int *outdegree, int *weighted);

/* Returns the canonicalized list of neighbors for a given topology, separated
 * into inbound and outbound edges.  Equivalent to MPI_Dist_graph_neighbors but
 * works for any topology type by canonicalizing according to the rules in
 * Section 7.6 of the MPI-3.0 standard. */
int MPIR_Topo_canon_nhb(MPID_Comm *comm_ptr,
                        int indegree, int sources[], int inweights[],
                        int outdegree, int dests[], int outweights[]);

#define MAX_CART_DIM 16

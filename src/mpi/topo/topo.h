/* -*- Mode: C; c-basic-offset:4 ; -*- */
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

#define MAX_CART_DIM 16

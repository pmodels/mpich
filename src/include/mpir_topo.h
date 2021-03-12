/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIR_TOPO_H_INCLUDED
#define MPIR_TOPO_H_INCLUDED

/*
 * The following structure allows the device detailed control over the
 * functions that are used to implement the topology routines.  If either
 * the pointer to this structure is null or any individual entry is null,
 * the default function is used (this follows exactly the same rules as the
 * collective operations, provided in the MPIR_Collops structure).
 */

typedef struct MPII_Topo_ops {
    int (*cartCreate) (const MPIR_Comm *, int, const int[], const int[], int, MPI_Comm *);
    int (*cartMap) (const MPIR_Comm *, int, const int[], const int[], int *);
    int (*graphCreate) (const MPIR_Comm *, int, const int[], const int[], int, MPI_Comm *);
    int (*graphMap) (const MPIR_Comm *, int, const int[], const int[], int *);
} MPII_Topo_ops;


typedef struct MPII_Graph_topology {
    int nnodes;
    int nedges;
    int *index;
    int *edges;
} MPII_Graph_topology;

typedef struct MPII_Cart_topology {
    int nnodes;                 /* Product of dims[*], gives the size of the topology */
    int ndims;
    int *dims;
    int *periodic;
    int *position;
} MPII_Cart_topology;

typedef struct MPII_Dist_graph_topology {
    int indegree;
    int *in;
    int *in_weights;
    int outdegree;
    int *out;
    int *out_weights;
    int is_weighted;
} MPII_Dist_graph_topology;

struct MPIR_Topology {
    MPIR_Topo_type kind;
    union topo {
        MPII_Graph_topology graph;
        MPII_Cart_topology cart;
        MPII_Dist_graph_topology dist_graph;
    } topo;
};

int MPIR_Dims_create(int, int, int *);

MPIR_Topology *MPIR_Topology_get(MPIR_Comm *);
int MPIR_Topology_put(MPIR_Comm *, MPIR_Topology *);

/* Returns the canonicalized count of neighbors for the given topology as though
 * MPI_Dist_graph_neighbors_count were called with a distributed graph topology,
 * even if the given topology is actually Cartesian or Graph.  Useful for
 * implementing neighborhood collective operations. */
int MPIR_Topo_canon_nhb_count(MPIR_Comm * comm_ptr, int *indegree, int *outdegree, int *weighted);

/* Returns the canonicalized list of neighbors for a given topology, separated
 * into inbound and outbound edges.  Equivalent to MPI_Dist_graph_neighbors but
 * works for any topology type by canonicalizing according to the rules in
 * Section 7.6 of the MPI-3.0 standard. */
int MPIR_Topo_canon_nhb(MPIR_Comm * comm_ptr,
                        int indegree, int sources[], int inweights[],
                        int outdegree, int dests[], int outweights[]);

#define MAX_CART_DIM 16

#endif /* MPIR_TOPO_H_INCLUDED */

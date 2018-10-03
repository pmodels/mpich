/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */

/*---------------------------------------------------------------------*/
/*     (C) Copyright 2017 Parallel Processing Research Laboratory      */
/*                   Queen's University at Kingston                    */
/*                Neighborhood Collective Communication                */
/*                    Seyed Hessamedin Mirsadeghi                      */
/*---------------------------------------------------------------------*/
#ifdef SCHED_DEBUG
#define MAX_DEBUG_SCHED_REQS 500
#endif

#define MAX_COMB_DEGREE 50      /* Fix this later by finding the value for combining degree at runtime */
#define MAX_LOOP_COUNT 50
#define SCHED_MEM_TO_FREE_MAX_SIZE 3*MAX_COMB_DEGREE+1  /* +1 for incom_tmp_buf,
                                                         * one t for make_all_combined,
                                                         * one t for send_own_data,
                                                         * one t for get_friend_data */
#define COMB_LIST_START_IDX 3

typedef struct Scheduling_msg {
    int is_off;
    int *comb_list;
    int comb_list_size;
    int t;
} Scheduling_msg;

typedef struct Common_neighbor {
    int rank;
    int index;
} Common_neighbor;

typedef struct Sorted_cmn_nbrs {
    /* an array of this struct (one per t) plus
     * some other struct (for ordinary neighbors)
     * is all we need to save for building the
     * schedule later on. We need to clean things up!
     */
    Common_neighbor *cmn_nbrs;
    int num_cmn_nbrs;
    int offload_start;
    int offload_end;
    int onload_start;
    int onload_end;
} Sorted_cmn_nbrs;

typedef enum Operation {
    COMB_SEND,                  /* offloaded */
    COMB_RECV,                  /* onloaded */
    COMB_IDLE
} Comb_Operation;

typedef struct Comb_element {
    int paired_frnd;
    Comb_Operation opt;
} Comb_element;

typedef struct Common_nbrhood_matrix {
    int **matrix;
    int **outnbrs_innbrs_bitmap;        /* Not currently an actual bitmap */
    int *row_sizes;             /* indegree of each outgoing neighbor */
    int *ignore_row;            /* Determines whether the neighbor is considered in
                                 * the process of finding friends. */
    int *is_row_offloaded;      /* Determines whether a neighbor should be
                                 * communicated with at the end. I.e., whether
                                 * it has been offloaded or not. */
    int *my_innbrs_bitmap;      /* Not currently an actual bitmap */
    int indegree;
    int num_rows;               /* outdegree */
    int num_elements;           /* sum of all row_sizes */
    int t;
    int num_onloaded[MAX_COMB_DEGREE];
    int num_offloaded[MAX_COMB_DEGREE];
    Sorted_cmn_nbrs sorted_cmn_nbrs[MAX_COMB_DEGREE];
    Comb_element **comb_matrix; /* For each outgoing neighbor represents the
                                 * list of ranks with which this rank should
                                 * combine message before sending it out. */
    int *comb_matrix_num_entries_in_row;
} Common_nbrhood_matrix;

typedef struct nbh_coll_patt {
    Common_nbrhood_matrix *cmn_nbh_mat;
    int **incom_sched_mat;

} nbh_coll_patt;

int MPIR_Build_neighb_coll_patt(MPIR_Comm * comm_ptr);
int find_in_arr(const int *array, int size, int value);
int print_in_file(int rank, char *name);
int print_vec(int rank, int size, const int *vec, char *name);

#ifndef MPIR_TOPO_H_INCLUDED
#define MPIR_TOPO_H_INCLUDED

/*
 * The following struture allows the device detailed control over the
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
    nbh_coll_patt *nbh_coll_patt;
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

/* topology impl functions */
int MPIR_Cart_create(MPIR_Comm *, int, const int[], const int[], int, MPI_Comm *);
int MPIR_Cart_map(const MPIR_Comm *, int, const int[], const int[], int *);
int MPIR_Cart_shift_impl(MPIR_Comm * comm_ptr, int direction, int displ, int *source, int *dest);

void MPIR_Cart_rank_impl(struct MPIR_Topology *cart_ptr, const int *coords, int *rank);
int MPIR_Cart_create_impl(MPIR_Comm * comm_ptr, int ndims, const int dims[],
                          const int periods[], int reorder, MPI_Comm * comm_cart);
int MPIR_Cart_map_impl(const MPIR_Comm * comm_ptr, int ndims, const int dims[],
                       const int periodic[], int *newrank);

int MPIR_Graph_create(MPIR_Comm *, int, const int[], const int[], int, MPI_Comm *);
int MPIR_Graph_map(const MPIR_Comm *, int, const int[], const int[], int *);
int MPIR_Graph_neighbors_count_impl(MPIR_Comm * comm_ptr, int rank, int *nneighbors);
int MPIR_Graph_neighbors_impl(MPIR_Comm * comm_ptr, int rank, int maxneighbors, int *neighbors);
int MPIR_Graph_map_impl(const MPIR_Comm * comm_ptr, int nnodes,
                        const int indx[], const int edges[], int *newrank);

int MPIR_Dist_graph_neighbors_count_impl(MPIR_Comm * comm_ptr, int *indegree, int *outdegree,
                                         int *weighted);
int MPIR_Dist_graph_neighbors_impl(MPIR_Comm * comm_ptr, int maxindegree, int sources[],
                                   int sourceweights[], int maxoutdegree, int destinations[],
                                   int destweights[]);

#endif /* MPIR_TOPO_H_INCLUDED */

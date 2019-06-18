/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2018 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2018 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#ifndef CH4_COLL_SELECT_TREE_TYPES_H_INCLUDED
#define CH4_COLL_SELECT_TREE_TYPES_H_INCLUDED

#include "./json-c/json.h"
#include "./json-c/json_object_private.h"
#include "coll_algo_params.h"

#define MPIDU_SELECTION_OBJECT_TYPE         json_type_object
#define MPIDU_SELECTION_ARRAY_TYPE          json_type_array
#define MPIDU_SELECTION_INT_TYPE            json_type_int
#define MPIDU_SELECTION_DOUBLE_TYPE         json_type_double
#define MPIDU_SELECTION_STR_TYPE            json_type_string
#define MPIDU_SELECTION_BOOL_TYPE           json_type_boolean
#define MPIDU_SELECTION_NULL_TYPE           json_type_null


/* Maximum containers per leaf node. Each container will have a composition id/algorithm id and
 * we assume, at the time of writing, each collective operation can be broken down to 10 at max */
#define MPIDU_SELECTION_MAX_CNT (10)
/* Maximum character length of each entry in the JSON file */
#define MPIDU_SELECTION_BUFFER_MAX_SIZE (256)
/* Maximum storage size of the entire selection tree. MPI will abort if the selection tree size
 * goes beyond this. The memory limit can be expanded or allocated dynamically from the JSON file size */
#define MPIDU_SELECTION_STORAGE_SIZE (1024*1024)        /* 1MB */
#define MPIDU_SELECTION_DEFAULT_ALGO_ID (-1)
#define MPIDU_SELECTION_DEFAULT_KEY  (-1)
#define MPIDU_SELECTION_DELIMITER_VALUE "="
#define MPIDU_SELECTION_DELIMITER_SUFFIX "_"
#define MPIDU_SELECTION_COLL_KEY_TOKEN "collective="
#define MPIDU_SELECTION_COMMSIZE_KEY_TOKEN "comm_size="
#define MPIDU_SELECTION_MSGSIZE_KEY_TOKEN "msg_size="
#define MPIDU_SELECTION_COMPOSITION_TOKEN "composition"
#define MPIDU_SELECTION_NETMOD_TOKEN "netmod"
#define MPIDU_SELECTION_SHM_TOKEN "shm"

#define MPIDU_SELECTION_POW2_TOKEN "-2"
typedef struct {
    uint8_t *base_addr;
    ptrdiff_t current_offset;
} MPIDU_SELECTION_storage_handler;

typedef ptrdiff_t MPIDU_SELECTION_storage_entry;
typedef json_object *MPIDU_SELECTION_json_node_t;

#define MPIDU_SELECTION_HANDLER_TO_POINTER(_storage, _entry) ((struct MPIDU_SELECTION_tree_node *)(_storage->base_addr + _entry))
#define MPIDU_SELECTION_NODE_FIELD(_storage, _entry, _field)  (MPIDU_SELECTION_HANDLER_TO_POINTER(_storage, _entry)->_field)
#define MPIDU_SELECTION_NULL_STORAGE {NULL, 0}
#define MPIDU_SELECTION_NULL_ENTRY ((MPIDU_SELECTION_storage_entry)-1)

/*Define enums for different types of decision available in collective selection infrastructure */
typedef enum {
    MPIDU_SELECTION_DIRECTORY,  /* selection layer - CH4/netmod/shm */
    MPIDU_SELECTION_COMM_KIND,  /* kind of communicator - intra/inter */
    MPIDU_SELECTION_COMM_HIERARCHY,     /* commnunicator hierarchy - flat, parent, node comm, roots comm */
    MPIDU_SELECTION_COLLECTIVE, /* collective */
    MPIDU_SELECTION_COMMSIZE,   /* communicator size */
    MPIDU_SELECTION_MSGSIZE,    /* message size */
    MPIDU_SELECTION_CONTAINER,  /* algorithm compositions */
    MPIDU_SELECTION_TYPES_NUM,  /* enum for iterating over different selection types */
    MPIDU_SELECTION_DEFAULT_TERMINAL_NODE_TYPE = MPIDU_SELECTION_CONTAINER,
    MPIDU_SELECTION_DEFAULT_NODE_TYPE = -1
} MPIDU_SELECTION_node_type_t;

/* Define node structure of the selection tree */
typedef struct MPIDU_SELECTION_tree_node {
    MPIDU_SELECTION_storage_entry parent;       /* The parent node of the current node. */
    MPIDU_SELECTION_node_type_t type;   /* The type of this node (as described in the definition
                                         * of MPIDU_SELECITON_node_type_t. */
    MPIDU_SELECTION_node_type_t next_layer_type;        /* The type of the next layer of children below
                                                         * this node. */
    int key;                    /* Value associated with the node. This key is used in the traversal and usually consists of information from collective signature */
    int children_count;         /* The number of children that this node will have */
    int cur_child_idx;          /* The index of the previously added child. Used to keep track of
                                 * where to put the next child in the array of nodes. */
    union {
        MPIDU_SELECTION_storage_entry offset[0];        /* Array of entries to store children offset */
        MPIDIG_coll_algo_generic_container_t containers[0];     /* The array of containers that hold
                                                                 * the information about the
                                                                 * collecives to be chosen from by
                                                                 * this node. */
    };
} MPIDU_SELECTION_node_t;

/* Structure to construct the leaf node entries */
typedef struct MPIDU_SELECTION_coll_entry {
    int id;
    void (*create_container) (MPIDU_SELECTION_json_node_t value, int *cnt_num,
                              MPIDIG_coll_algo_generic_container_t * cnt, int coll_id);
} MPIDU_SELECTION_tree_entry_t;

/* The structure to describe all of the fields used to attempt a match when traversing the
 * collective selection tree. */
typedef struct MPIDU_SELECTION_match_pattern {
    MPIDU_SELECTION_node_type_t terminal_node_type;

    int directory;
    int comm_kind;
    int comm_hierarchy_kind;
    int coll_id;
    int comm_size;
    int msg_size;
} MPIDU_SELECTION_match_pattern_t;

/* Structure describing the collective being executed. */
typedef struct MPIDU_SELECTON_coll_signature {
    int coll_id;
    MPIR_Comm *comm;
    union {
        struct {
            const void *sendbuf;
            int sendcount;
            MPI_Datatype sendtype;
            void *recvbuf;
            int recvcount;
            MPI_Datatype recvtype;
            MPIR_Errflag_t *errflag;
        } allgather;
        struct {
            const void *sendbuf;
            int sendcount;
            MPI_Datatype sendtype;
            void *recvbuf;
            const int *recvcounts;
            const int *displs;
            MPI_Datatype recvtype;
            MPIR_Errflag_t *errflag;
        } allgatherv;
        struct {
            const void *sendbuf;
            void *recvbuf;
            int count;
            MPI_Datatype datatype;
            MPI_Op op;
            MPIR_Errflag_t *errflag;
        } allreduce;
        struct {
            const void *sendbuf;
            int sendcount;
            MPI_Datatype sendtype;
            void *recvbuf;
            int recvcount;
            MPI_Datatype recvtype;
            MPIR_Errflag_t *errflag;
        } alltoall;
        struct {
            const void *sendbuf;
            const int *sendcounts;
            const int *sdispls;
            MPI_Datatype sendtype;
            void *recvbuf;
            const int *recvcounts;
            const int *rdispls;
            MPI_Datatype recvtype;
            MPIR_Errflag_t *errflag;
        } alltoallv;
        struct {
            const void *sendbuf;
            const int *sendcounts;
            const int *sdispls;
            const MPI_Datatype *sendtypes;
            void *recvbuf;
            const int *recvcounts;
            const int *rdispls;
            const MPI_Datatype *recvtypes;
            MPIR_Errflag_t *errflag;
        } alltoallw;
        struct {
        } barrier;
        struct {
            void *buffer;
            int count;
            MPI_Datatype datatype;
            int root;
            MPIR_Errflag_t *errflag;
        } bcast;
        struct {
            const void *sendbuf;
            void *recvbuf;
            int count;
            MPI_Datatype datatype;
            MPI_Op op;
            MPIR_Errflag_t *errflag;
        } exscan;
        struct {
            const void *sendbuf;
            int sendcount;
            MPI_Datatype sendtype;
            void *recvbuf;
            int recvcount;
            MPI_Datatype recvtype;
            int root;
            MPIR_Errflag_t *errflag;
        } gather;
        struct {
            const void *sendbuf;
            int sendcount;
            MPI_Datatype sendtype;
            void *recvbuf;
            const int *recvcounts;
            const int *displs;
            MPI_Datatype recvtype;
            int root;
            MPIR_Errflag_t *errflag;
        } gatherv;
        struct {
            const void *sendbuf;
            void *recvbuf;
            const int *recvcounts;
            MPI_Datatype datatype;
            MPI_Op op;
            MPIR_Errflag_t *errflag;
        } reduce_scatter;
        struct {
            const void *sendbuf;
            void *recvbuf;
            int count;
            MPI_Datatype datatype;
            MPI_Op op;
            int root;
            MPIR_Errflag_t *errflag;
        } reduce;
        struct {
            const void *sendbuf;
            void *recvbuf;
            int recvcount;
            MPI_Datatype datatype;
            MPI_Op op;
            MPIR_Errflag_t *errflag;
        } reduce_scatter_block;
        struct {
            const void *sendbuf;
            void *recvbuf;
            int count;
            MPI_Datatype datatype;
            MPI_Op op;
            MPIR_Errflag_t *errflag;
        } scan;
        struct {
            const void *sendbuf;
            int sendcount;
            MPI_Datatype sendtype;
            void *recvbuf;
            int recvcount;
            MPI_Datatype recvtype;
            int root;
            MPIR_Errflag_t *errflag;
        } scatter;
        struct {
            const void *sendbuf;
            const int *sendcounts;
            const int *displs;
            MPI_Datatype sendtype;
            void *recvbuf;
            int recvcount;
            MPI_Datatype recvtype;
            int root;
            MPIR_Errflag_t *errflag;
        } scatterv;
    } coll;
} MPIDU_SELECTON_coll_signature_t;

/* Enums used in building and traversing the selection tree */
typedef enum {
    MPIDU_SELECTION_ALLGATHER = 0,
    MPIDU_SELECTION_ALLGATHERV,
    MPIDU_SELECTION_ALLREDUCE,
    MPIDU_SELECTION_ALLTOALL,
    MPIDU_SELECTION_ALLTOALLV,
    MPIDU_SELECTION_ALLTOALLW,
    MPIDU_SELECTION_BARRIER,
    MPIDU_SELECTION_BCAST,
    MPIDU_SELECTION_EXSCAN,
    MPIDU_SELECTION_GATHER,
    MPIDU_SELECTION_GATHERV,
    MPIDU_SELECTION_REDUCE_SCATTER,
    MPIDU_SELECTION_REDUCE,
    MPIDU_SELECTION_SCAN,
    MPIDU_SELECTION_SCATTER,
    MPIDU_SELECTION_SCATTERV,
    MPIDU_SELECTION_REDUCE_SCATTER_BLOCK,
    MPIDU_SELECTION_IALLGATHER,
    MPIDU_SELECTION_IALLGATHERV,
    MPIDU_SELECTION_IALLREDUCE,
    MPIDU_SELECTION_IALLTOALL,
    MPIDU_SELECTION_IALLTOALLV,
    MPIDU_SELECTION_IALLTOALLW,
    MPIDU_SELECTION_IBARRIER,
    MPIDU_SELECTION_IBCAST,
    MPIDU_SELECTION_IEXSCAN,
    MPIDU_SELECTION_IGATHER,
    MPIDU_SELECTION_IGATHERV,
    MPIDU_SELECTION_IREDUCE_SCATTER,
    MPIDU_SELECTION_IREDUCE,
    MPIDU_SELECTION_ISCAN,
    MPIDU_SELECTION_ISCATTER,
    MPIDU_SELECTION_ISCATTERV,
    MPIDU_SELECTION_IREDUCE_SCATTER_BLOCK,
    MPIDU_SELECTION_COLLECTIVES_MAX
} MPIDU_SELECTION_coll_id_t;

typedef enum {
    MPIDU_SELECTION_COMPOSITION,
    MPIDU_SELECTION_NETMOD,
    MPIDU_SELECTION_SHM,
    MPIDU_SELECTION_DIRECTORY_KIND_NUM
} MPIDU_SELECTION_directory_kind_t;

typedef enum {
    MPIDU_SELECTION_INTRA_COMM,
    MPIDU_SELECTION_INTER_COMM,
    MPIDU_SELECTION_COMM_KIND_NUM
} MPIDU_SELECTION_comm_kind_t;

typedef enum {
    MPIDU_SELECTION_FLAT_COMM,
    MPIDU_SELECTION_TOPO_COMM,
    MPIDU_SELECTION_COMM_HIERARCHY_NUM
} MPIDU_SELECTION_comm_hierarchy_kind_t;

#endif /* CH4_COLL_SELECT_TREE_TYPES_H_INCLUDED */

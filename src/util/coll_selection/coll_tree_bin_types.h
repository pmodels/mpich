#ifndef MPIU_COLL_SELECTION_TREE_TYPES_H_INCLUDED
#define MPIU_COLL_SELECTION_TREE_TYPES_H_INCLUDED

#include "coll_selection_tree_pre.h"

typedef enum {
    MPIU_COLL_SELECTION_ALLGATHER = 0,
    MPIU_COLL_SELECTION_ALLGATHERV,
    MPIU_COLL_SELECTION_ALLREDUCE,
    MPIU_COLL_SELECTION_ALLTOALL,
    MPIU_COLL_SELECTION_ALLTOALLV,
    MPIU_COLL_SELECTION_ALLTOALLW,
    MPIU_COLL_SELECTION_BARRIER,
    MPIU_COLL_SELECTION_BCAST,
    MPIU_COLL_SELECTION_EXSCAN,
    MPIU_COLL_SELECTION_GATHER,
    MPIU_COLL_SELECTION_GATHERV,
    MPIU_COLL_SELECTION_REDUCE_SCATTER,
    MPIU_COLL_SELECTION_REDUCE,
    MPIU_COLL_SELECTION_SCAN,
    MPIU_COLL_SELECTION_SCATTER,
    MPIU_COLL_SELECTION_SCATTERV,
    MPIU_COLL_SELECTION_REDUCE_SCATTER_BLOCK,
    MPIU_COLL_SELECTION_IALLGATHER,
    MPIU_COLL_SELECTION_IALLGATHERV,
    MPIU_COLL_SELECTION_IALLREDUCE,
    MPIU_COLL_SELECTION_IALLTOALL,
    MPIU_COLL_SELECTION_IALLTOALLV,
    MPIU_COLL_SELECTION_IALLTOALLW,
    MPIU_COLL_SELECTION_IBARRIER,
    MPIU_COLL_SELECTION_IBCAST,
    MPIU_COLL_SELECTION_IEXSCAN,
    MPIU_COLL_SELECTION_IGATHER,
    MPIU_COLL_SELECTION_IGATHERV,
    MPIU_COLL_SELECTION_IREDUCE_SCATTER,
    MPIU_COLL_SELECTION_IREDUCE,
    MPIU_COLL_SELECTION_ISCAN,
    MPIU_COLL_SELECTION_ISCATTER,
    MPIU_COLL_SELECTION_ISCATTERV,
    MPIU_COLL_SELECTION_IREDUCE_SCATTER_BLOCK,
    MPIU_COLL_SELECTION_COLLECTIVES_NUMBER
} MPIU_COLL_SELECTION_coll_id_t;

typedef enum {
    MPIU_COLL_SELECTION_INTRA_COMM,
    MPIU_COLL_SELECTION_INTER_COMM,
    MPIU_COLL_SELECTION_COMM_KIND_NUM
} MPIU_COLL_SELECTION_comm_kind_t;

typedef enum {
    MPIU_COLL_SELECTION_FLAT_COMM,
    MPIU_COLL_SELECTION_TOPO_COMM,
    MPIU_COLL_SELECTION_COMM_HIERARCHY_NUM
} MPIU_COLL_SELECTION_comm_hierarchy_kind_t;

typedef enum {
    MPIU_COLL_SELECTION_STORAGE,
    MPIU_COLL_SELECTION_COMM_KIND,
    MPIU_COLL_SELECTION_COMM_HIERARCHY,
    MPIU_COLL_SELECTION_COLLECTIVE,
    MPIU_COLL_SELECTION_COMMSIZE,
    MPIU_COLL_SELECTION_MSGSIZE,
    MPIU_COLL_SELECTION_CONTAINER,
    MPIU_COLL_SELECTION_TYPES_NUM,
    MPIU_COLL_SELECTION_DEFAULT_TERMINAL_NODE_TYPE = MPIU_COLL_SELECTION_CONTAINER,
    MPIU_COLL_SELECTION_DEFAULT_NODE_TYPE = -1
} MPIU_COLL_SELECTION_node_type_t;

typedef struct MPIU_COLL_SELECTION_tree_node {
    MPIU_COLL_SELECTION_storage_handler parent;
    MPIU_COLL_SELECTION_node_type_t type;
    MPIU_COLL_SELECTION_node_type_t next_layer_type;
    int key;
    int children_count;
    int cur_child_idx;
    union {
        MPIU_COLL_SELECTION_storage_handler offset[0];
        MPIDIG_coll_algo_generic_container_t containers[0];
    };
} MPIU_COLL_SELECTION_tree_node_t;

typedef struct MPIU_COLL_SELECTION_match_pattern {
    MPIU_COLL_SELECTION_node_type_t terminal_node_type;

    int storage;
    int comm_kind;
    int comm_hierarchy_kind;
    int coll_id;
    int comm_size;
    int msg_size;
} MPIU_COLL_SELECTION_match_pattern_t;

typedef struct MPIU_COLL_SELECTON_coll_signature {
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
} MPIU_COLL_SELECTON_coll_signature_t;

extern MPIU_COLL_SELECTON_coll_signature_t MPIU_COLL_SELECTON_coll_sig;

MPIU_COLL_SELECTION_storage_handler
MPIU_COLL_SELECTION_create_node(MPIU_COLL_SELECTION_storage_handler parent,
                                MPIU_COLL_SELECTION_node_type_t node_type,
                                MPIU_COLL_SELECTION_node_type_t next_layer_type,
                                int node_key, int children_count);

MPIU_COLL_SELECTION_storage_handler
MPIU_COLL_SELECTION_create_leaf(MPIU_COLL_SELECTION_storage_handler parent,
                                int node_type, int containers_count, void *containers);

void *MPIU_COLL_SELECTION_get_container(MPIU_COLL_SELECTION_storage_handler node);

extern MPIU_COLL_SELECTION_storage_handler MPIU_COLL_SELECTION_tree_load(char *filename);

extern int MPIU_COLL_SELECTION_init(void);

extern int MPIU_COLL_SELECTION_dump(void);

void
MPIU_COLL_SELECTION_build_bin_tree_generic_part(MPIU_COLL_SELECTION_storage_handler *
                                                root,
                                                MPIU_COLL_SELECTION_storage_handler *
                                                inter_comm_subtree,
                                                MPIU_COLL_SELECTION_storage_handler *
                                                topo_aware_comm_subtree,
                                                MPIU_COLL_SELECTION_storage_handler *
                                                flat_comm_subtree);

void MPIU_COLL_SELECTION_build_bin_tree_default_inter(MPIU_COLL_SELECTION_storage_handler
                                                      inter_comm_subtree);

void
MPIU_COLL_SELECTION_build_bin_tree_default_topo_aware(MPIU_COLL_SELECTION_storage_handler
                                                      topo_aware_comm_subtree);

void MPIU_COLL_SELECTION_build_bin_tree_default_flat(MPIU_COLL_SELECTION_storage_handler
                                                     flat_comm_subtree);
void MPIU_COLL_SELECTION_init_match_pattern(MPIU_COLL_SELECTION_match_pattern_t * match_pattern);

MPIU_COLL_SELECTION_storage_handler
MPIU_COLL_SELECTION_find_entry(MPIU_COLL_SELECTION_storage_handler entry,
                               MPIU_COLL_SELECTION_match_pattern_t * match_pattern);

void
MPIU_COLL_SELECTION_init_comm_match_pattern(MPIR_Comm * comm,
                                            MPIU_COLL_SELECTION_match_pattern_t *
                                            match_pattern,
                                            MPIU_COLL_SELECTION_node_type_t terminal_layer_type);

void
MPIU_COLL_SELECTION_init_coll_match_pattern(MPIU_COLL_SELECTON_coll_signature_t * coll_sig,
                                            MPIU_COLL_SELECTION_match_pattern_t *
                                            match_pattern,
                                            MPIU_COLL_SELECTION_node_type_t terminal_layer_type);
void
MPIU_COLL_SELECTION_set_match_pattern_key(MPIU_COLL_SELECTION_match_pattern_t *
                                          match_pattern,
                                          MPIU_COLL_SELECTION_node_type_t layer_type, int key);
int
MPIU_COLL_SELECTION_get_match_pattern_key(MPIU_COLL_SELECTION_match_pattern_t *
                                          match_pattern,
                                          MPIU_COLL_SELECTION_node_type_t layer_type);
#endif /* MPIU_COLL_SELECTION_TREE_TYPES_H_INCLUDED */

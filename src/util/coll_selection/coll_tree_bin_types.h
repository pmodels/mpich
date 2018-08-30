#ifndef MPIU_SELECTION_TYPES_H_INCLUDED
#define MPIU_SELECTION_TYPES_H_INCLUDED

#include "coll_tree_json.h"

typedef enum {
    MPIU_SELECTION_INTRA_COMM,
    MPIU_SELECTION_INTER_COMM,
    MPIU_SELECTION_COMM_KIND_NUM
} MPIU_SELECTION_comm_kind_t;

typedef enum {
    MPIU_SELECTION_FLAT_COMM,
    MPIU_SELECTION_TOPO_COMM,
    MPIU_SELECTION_COMM_HIERARCHY_NUM
} MPIU_SELECTION_comm_hierarchy_kind_t;

typedef enum {
    MPIU_SELECTION_DIRECTORY,
    MPIU_SELECTION_COMM_KIND,
    MPIU_SELECTION_COMM_HIERARCHY,
    MPIU_SELECTION_COLLECTIVE,
    MPIU_SELECTION_COMMSIZE,
    MPIU_SELECTION_MSGSIZE,
    MPIU_SELECTION_CONTAINER,
    MPIU_SELECTION_TYPES_NUM,
    MPIU_SELECTION_DEFAULT_TERMINAL_NODE_TYPE = MPIU_SELECTION_CONTAINER,
    MPIU_SELECTION_DEFAULT_NODE_TYPE = -1
} MPIU_SELECTION_node_type_t;

typedef struct MPIU_SELECTION_tree_node {
    MPIU_SELECTION_storage_entry parent;
    MPIU_SELECTION_node_type_t type;
    MPIU_SELECTION_node_type_t next_layer_type;
    int key;
    int children_count;
    int cur_child_idx;
    union {
        MPIU_SELECTION_storage_entry offset[0];
        MPIDIG_coll_algo_generic_container_t containers[0];
    };
} MPIU_SELECTION_node_t;

typedef struct MPIU_SELECTION_match_pattern {
    MPIU_SELECTION_node_type_t terminal_node_type;

    int directory;
    int comm_kind;
    int comm_hierarchy_kind;
    int coll_id;
    int comm_size;
    int msg_size;
} MPIU_SELECTION_match_pattern_t;

typedef struct MPIU_SELECTON_coll_signature {
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
} MPIU_SELECTON_coll_signature_t;

extern MPIU_SELECTON_coll_signature_t MPIU_COLL_SELECTON_coll_sig;

MPIU_SELECTION_storage_entry
MPIU_SELECTION_create_node(MPIU_SELECTION_storage_handler * storage,
                           MPIU_SELECTION_storage_entry parent,
                           MPIU_SELECTION_node_type_t node_type,
                           MPIU_SELECTION_node_type_t next_layer_type,
                           int node_key, int children_count);

MPIU_SELECTION_storage_entry
MPIU_SELECTION_create_leaf(MPIU_SELECTION_storage_handler * storage,
                           MPIU_SELECTION_storage_entry parent,
                           int node_type, int containers_count, void *containers);

void *MPIU_SELECTION_get_container(MPIU_SELECTION_storage_handler * storage,
                                   MPIU_SELECTION_storage_entry node);

MPIU_SELECTION_storage_entry MPIU_SELECTION_get_node_parent(MPIU_SELECTION_storage_handler *
                                                            storage,
                                                            MPIU_SELECTION_storage_entry node);

extern MPIU_SELECTION_storage_entry MPIU_SELECTION_tree_load(MPIU_SELECTION_storage_handler *
                                                             storage, char *filename);

extern int MPIU_SELECTION_init(void);

extern int MPIU_SELECTION_dump(MPIU_SELECTION_storage_handler * storage);

void
MPIU_SELECTION_build_bin_tree_generic_part(MPIU_SELECTION_storage_handler * storage,
                                           MPIU_SELECTION_storage_entry * root,
                                           MPIU_SELECTION_storage_entry * inter_comm_subtree,
                                           MPIU_SELECTION_storage_entry * topo_aware_comm_subtree,
                                           MPIU_SELECTION_storage_entry * flat_comm_subtree);

void MPIU_SELECTION_build_bin_tree_default_inter(MPIU_SELECTION_storage_handler * storage,
                                                 MPIU_SELECTION_storage_entry inter_comm_subtree);

void
MPIU_SELECTION_build_bin_tree_default_topo_aware(MPIU_SELECTION_storage_handler * storage,
                                                 MPIU_SELECTION_storage_entry
                                                 topo_aware_comm_subtree);

void MPIU_SELECTION_build_bin_tree_default_flat(MPIU_SELECTION_storage_handler * storage,
                                                MPIU_SELECTION_storage_entry flat_comm_subtree);
void MPIU_SELECTION_init_match_pattern(MPIU_SELECTION_match_pattern_t * match_pattern);

MPIU_SELECTION_storage_entry
MPIU_SELECTION_find_entry(MPIU_SELECTION_storage_handler * storage,
                          MPIU_SELECTION_storage_entry entry,
                          MPIU_SELECTION_match_pattern_t * match_pattern);

void
MPIU_SELECTION_init_comm_match_pattern(MPIR_Comm * comm,
                                       MPIU_SELECTION_match_pattern_t *
                                       match_pattern,
                                       MPIU_SELECTION_node_type_t terminal_layer_type);

void
MPIU_SELECTION_init_coll_match_pattern(MPIU_SELECTON_coll_signature_t * coll_sig,
                                       MPIU_SELECTION_match_pattern_t *
                                       match_pattern,
                                       MPIU_SELECTION_node_type_t terminal_layer_type);
void
MPIU_SELECTION_set_match_pattern_key(MPIU_SELECTION_match_pattern_t *
                                     match_pattern, MPIU_SELECTION_node_type_t layer_type, int key);
int
MPIU_SELECTION_get_match_pattern_key(MPIU_SELECTION_match_pattern_t *
                                     match_pattern, MPIU_SELECTION_node_type_t layer_type);
#if defined (MPL_USE_DBG_LOGGING)
void
MPIU_SELECTION_match_layer_and_key_to_str(char *layer, char *key_str,
                                          MPIU_SELECTION_storage_entry match_node);
#endif /* MPL_USE_DBG_LOGGING */

#endif /* MPIU_SELECTION_TYPES_H_INCLUDED */

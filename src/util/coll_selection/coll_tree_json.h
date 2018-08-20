#ifndef MPIU_COLL_SELECTION_TREE_TOOLS_H_INCLUDED
#define MPIU_COLL_SELECTION_TREE_TOOLS_H_INCLUDED

#include "coll_algo_params.h"
#include "coll_tree_bin_types.h"
#include "coll_tree_bin_pre.h"
#include "./json-c/json.h"
#include "./json-c/json_object_private.h"

#define MPIU_COLL_SELECTION_TREE_NODE json_object *

#define MPIU_COLL_SELECTION_TREE_OBJECT_TYPE         json_type_object
#define MPIU_COLL_SELECTION_TREE_ARRAY_TYPE          json_type_array
#define MPIU_COLL_SELECTION_TREE_INT_TYPE            json_type_int
#define MPIU_COLL_SELECTION_TREE_DOUBLE_TYPE         json_type_double
#define MPIU_COLL_SELECTION_TREE_STR_TYPE            json_type_string
#define MPIU_COLL_SELECTION_TREE_BOOL_TYPE           json_type_boolean
#define MPIU_COLL_SELECTION_TREE_NULL_TYPE           json_type_null

#define MPIU_COLL_SELECTION_TREE_MAX_CNT (10)
#define MPIU_COLL_SELECTION_TREE_MAX_TUNE_ENTRY (100)
#define MPIU_COLL_SELECTION_TREE_DEFAULT_ALGO_ID (-1)
#define MPIU_COLL_SELECTION_TREE_DELIMITER_VALUE "="
#define MPIU_COLL_SELECTION_TREE_DELIMITER_SUFFIX "_"
#define MPIU_COLL_SELECTION_TREE_BUFFER_MAX_SIZE (256)
#define MPIU_COLL_SELECTION_TREE_LAYERS_NUM (3)
#define MPIU_COLL_SELECTION_TREE_COLL_KEY_TOKEN "collective="
#define MPIU_COLL_SELECTION_TREE_COMMSIZE_KEY_TOKEN "comm_size="
#define MPIU_COLL_SELECTION_TREE_MSGSIZE_KEY_TOKEN "msg_size="
#define MPIU_COLL_SELECTION_TREE_COMPOSITION_TOKEN "composition"
#define MPIU_COLL_SELECTION_TREE_NETMOD_TOKEN "netmod"
#define MPIU_COLL_SELECTION_TREE_SHM_TOKEN "shm"

#define MPIU_COLL_SELECTION_TREE_POW2_TOKEN "-2"

typedef json_type MPIU_COLL_SELECTION_TREE_NODE_TYPE;

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
typedef struct MPIU_COLL_SELECTION_TREE_coll_entry {
    int id;
    void (*create_container) (MPIU_COLL_SELECTION_TREE_NODE value, int *cnt_num,
                              MPIDIG_coll_algo_generic_container_t * cnt, int coll_id);
} MPIU_COLL_SELECTION_TREE_entry_t;

typedef enum {
    MPIU_COLL_SELECTION_TREE_COMPOSITION = 0,
    MPIU_COLL_SELECTION_TREE_NETMOD,
    MPIU_COLL_SELECTION_TREE_SHM,
    MPIU_COLL_SELECTION_TREE_ALGORITHM_LAYERS_NUM
} MPIiU_COLL_SELECTION_TREE_algorithm_type_t;

static inline MPIU_COLL_SELECTION_TREE_NODE MPIU_COLL_SELECTION_TREE_text_to_json(char *contents);
static inline char *MPIU_COLL_SELECTION_TREE_get_node_json_name(MPIU_COLL_SELECTION_TREE_NODE
                                                                json_node, int index);

static inline MPIU_COLL_SELECTION_TREE_NODE
    * MPIU_COLL_SELECTION_TREE_get_node_json_value(MPIU_COLL_SELECTION_TREE_NODE json_node,
                                                   int index);
static inline MPIU_COLL_SELECTION_TREE_NODE
MPIU_COLL_SELECTION_TREE_get_node_json_next(MPIU_COLL_SELECTION_TREE_NODE json_node, int index);
static inline int MPIU_COLL_SELECTION_TREE_get_node_json_type(MPIU_COLL_SELECTION_TREE_NODE
                                                              json_node);
static inline int MPIU_COLL_SELECTION_TREE_get_node_json_peer_count(MPIU_COLL_SELECTION_TREE_NODE
                                                                    json_node);
MPIU_COLL_SELECTION_TREE_NODE MPIU_COLL_SELECTION_TREE_read(char *file);
void MPIU_COLL_SELECTION_TREE_json_to_bin(MPIU_COLL_SELECTION_TREE_NODE json_node,
                                          MPIU_COLL_SELECTION_storage_handler bin_node);
int MPIU_COLL_SELECTION_TREE_get_node_type(MPIU_COLL_SELECTION_TREE_NODE json_node, int ind);
void MPIU_COLL_SELECTION_TREE_get_node_key(MPIU_COLL_SELECTION_TREE_NODE json_node,
                                           int ind, char *key);
void MPIU_COLL_SELECTION_TREE_handle_object(MPIU_COLL_SELECTION_TREE_NODE json_node,
                                            MPIU_COLL_SELECTION_storage_handler node);
void MPIU_COLL_SELECTION_TREE_create_containers(MPIU_COLL_SELECTION_TREE_NODE
                                                json_node, int *cnt_num,
                                                MPIDIG_coll_algo_generic_container_t *
                                                cnt, int coll_id);
int MPIU_COLL_SELECTION_TREE_get_algorithm_type(char *str);
int MPIU_COLL_SELECTION_TREE_convert_key_str(char *key_str);
void MPIU_COLL_SELECTION_TREE_init_json_node(MPIU_COLL_SELECTION_TREE_NODE * json_node);

int MPIU_COLL_SELECTION_TREE_free_json_node(MPIU_COLL_SELECTION_TREE_NODE json_node);

int MPIU_COLL_SELECTION_TREE_fill_json_node(MPIU_COLL_SELECTION_TREE_NODE json_node,
                                            char *key, MPIU_COLL_SELECTION_TREE_NODE value);
int MPIU_COLL_SELECTION_TREE_is_coll_in_json(MPIU_COLL_SELECTION_TREE_NODE
                                             json_node_in,
                                             MPIU_COLL_SELECTION_TREE_NODE *
                                             json_node_out, int coll_id);
extern char *MPIU_COLL_SELECTION_TREE_coll_list[];
extern int MPIU_COLL_SELECTION_TREE_current_collective_id;
extern char ***MPIU_COLL_SELECTION_TREE_ALGORITHMS[];
extern MPIU_COLL_SELECTION_TREE_entry_t *MPIU_COLL_SELECTION_TREE_parse[];
extern int MPIU_COLL_SELECTION_TREE_MAX_INDICES[MPIU_COLL_SELECTION_COLLECTIVES_NUMBER]
    [MPIU_COLL_SELECTION_TREE_ALGORITHM_LAYERS_NUM];
#endif /* MPIU_COLL_SELECTION_TREE_TOOLS_H_INCLUDED */

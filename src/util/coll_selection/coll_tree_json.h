#ifndef MPIU_SELECTION_TREE_TOOLS_H_INCLUDED
#define MPIU_SELECTION_TREE_TOOLS_H_INCLUDED

#include "coll_algo_params.h"
#include "coll_tree_bin_types.h"
#include "coll_tree_bin_pre.h"
#include "./json-c/json.h"
#include "./json-c/json_object_private.h"

#define MPIU_SELECTION_NODE json_object *

#define MPIU_SELECTION_OBJECT_TYPE         json_type_object
#define MPIU_SELECTION_ARRAY_TYPE          json_type_array
#define MPIU_SELECTION_INT_TYPE            json_type_int
#define MPIU_SELECTION_DOUBLE_TYPE         json_type_double
#define MPIU_SELECTION_STR_TYPE            json_type_string
#define MPIU_SELECTION_BOOL_TYPE           json_type_boolean
#define MPIU_SELECTION_NULL_TYPE           json_type_null

#define MPIU_SELECTION_MAX_CNT (10)
#define MPIU_SELECTION_MAX_TUNE_ENTRY (100)
#define MPIU_SELECTION_DEFAULT_ALGO_ID (-1)
#define MPIU_SELECTION_DELIMITER_VALUE "="
#define MPIU_SELECTION_DELIMITER_SUFFIX "_"
#define MPIU_SELECTION_BUFFER_MAX_SIZE (256)
#define MPIU_SELECTION_LAYERS_NUM (3)
#define MPIU_SELECTION_COLL_KEY_TOKEN "collective="
#define MPIU_SELECTION_COMMSIZE_KEY_TOKEN "comm_size="
#define MPIU_SELECTION_MSGSIZE_KEY_TOKEN "msg_size="
#define MPIU_SELECTION_COMPOSITION_TOKEN "composition"
#define MPIU_SELECTION_NETMOD_TOKEN "netmod"
#define MPIU_SELECTION_SHM_TOKEN "shm"

#define MPIU_SELECTION_POW2_TOKEN "-2"

typedef json_type MPIU_SELECTION_NODE_TYPE;

typedef enum {
    MPIU_SELECTION_ALLGATHER = 0,
    MPIU_SELECTION_ALLGATHERV,
    MPIU_SELECTION_ALLREDUCE,
    PIU_SELECTION_ALLTOALL,
    MPIU_SELECTION_ALLTOALLV,
    MPIU_SELECTION_ALLTOALLW,
    MPIU_SELECTION_BARRIER,
    MPIU_SELECTION_BCAST,
    MPIU_SELECTION_EXSCAN,
    MPIU_SELECTION_GATHER,
    MPIU_SELECTION_GATHERV,
    MPIU_SELECTION_REDUCE_SCATTER,
    MPIU_SELECTION_REDUCE,
    MPIU_SELECTION_SCAN,
    MPIU_SELECTION_SCATTER,
    MPIU_SELECTION_SCATTERV,
    MPIU_SELECTION_REDUCE_SCATTER_BLOCK,
    MPIU_SELECTION_IALLGATHER,
    MPIU_SELECTION_IALLGATHERV,
    MPIU_SELECTION_IALLREDUCE,
    MPIU_SELECTION_IALLTOALL,
    MPIU_SELECTION_IALLTOALLV,
    MPIU_SELECTION_IALLTOALLW,
    MPIU_SELECTION_IBARRIER,
    MPIU_SELECTION_IBCAST,
    MPIU_SELECTION_IEXSCAN,
    MPIU_SELECTION_IGATHER,
    MPIU_SELECTION_IGATHERV,
    MPIU_SELECTION_IREDUCE_SCATTER,
    MPIU_SELECTION_IREDUCE,
    MPIU_SELECTION_ISCAN,
    MPIU_SELECTION_ISCATTER,
    MPIU_SELECTION_ISCATTERV,
    MPIU_SELECTION_IREDUCE_SCATTER_BLOCK,
    MPIU_SELECTION_COLLECTIVES_NUMBER
} MPIU_SELECTION_coll_id_t;
typedef struct MPIU_SELECTION_TREE_coll_entry {
    int id;
    void (*create_container) (MPIU_SELECTION_NODE value, int *cnt_num,
                              MPIDIG_coll_algo_generic_container_t * cnt, int coll_id);
} MPIU_SELECTION_tree_entry_t;

typedef enum {
    MPIU_SELECTION_COMPOSITION = 0,
    MPIU_SELECTION_NETMOD,
    MPIU_SELECTION_SHM,
    MPIU_SELECTION_ALGORITHM_LAYERS_NUM
} MPIU_SELECTION_tree_algorithm_type_t;

static inline MPIU_SELECTION_NODE MPIU_SELECTION_TREE_text_to_json(char *contents);
static inline char *MPIU_SELECTION_tree_get_node_json_name(MPIU_SELECTION_NODE
                                                           json_node, int index);

static inline MPIU_SELECTION_NODE
MPIU_SELECTION_tree_get_node_json_value(MPIU_SELECTION_NODE json_node, int index);
static inline MPIU_SELECTION_NODE
MPIU_SELECTION_tree_get_node_json_next(MPIU_SELECTION_NODE json_node, int index);
static inline int MPIU_SELECTION_tree_get_node_json_type(MPIU_SELECTION_NODE json_node);
static inline int MPIU_SELECTION_tree_get_node_json_peer_count(MPIU_SELECTION_NODE json_node);
MPIU_SELECTION_NODE MPIU_SELECTION_tree_read(char *file);
void MPIU_SELECTION_tree_json_to_bin(MPIU_SELECTION_NODE json_node,
                                     MPIU_SELECTION_storage_handler bin_node);
int MPIU_SELECTION_tree_get_node_type(MPIU_SELECTION_NODE json_node, int ind);
void MPIU_SELECTION_tree_get_node_key(MPIU_SELECTION_NODE json_node, int ind, char *key);
void MPIU_SELECTION_tree_handle_object(MPIU_SELECTION_NODE json_node,
                                       MPIU_SELECTION_storage_handler node);
void MPIU_SELECTION_tree_create_containers(MPIU_SELECTION_NODE
                                           json_node, int *cnt_num,
                                           MPIDIG_coll_algo_generic_container_t * cnt, int coll_id);
int MPIU_SELECTION_tree_get_algorithm_type(char *str);
void MPIU_SELECTION_tree_convert_key_int(int key_int, char *key_str, int *is_pow2);
int MPIU_SELECTION_tree_convert_key_str(char *key_str);
void MPIU_SELECTION_tree_init_json_node(MPIU_SELECTION_NODE * json_node);

int MPIU_SELECTION_tree_free_json_node(MPIU_SELECTION_NODE json_node);

int MPIU_SELECTION_tree_fill_json_node(MPIU_SELECTION_NODE json_node,
                                       char *key, MPIU_SELECTION_NODE value);
int MPIU_SELECTION_tree_is_coll_in_json(MPIU_SELECTION_NODE
                                        json_node_in,
                                        MPIU_SELECTION_NODE * json_node_out, int coll_id);
extern char *MPIU_SELECTION_tree_coll_list[];
extern int MPIU_SELECTION_tree_current_collective_id;
extern char ***MPIU_SELECTION_ALGORITHMS[];
extern MPIU_SELECTION_tree_entry_t *MPIU_SELECTION_parse[];
extern int MPIU_SELECTION_MAX_INDICES[MPIU_SELECTION_COLLECTIVES_NUMBER]
    [MPIU_SELECTION_ALGORITHM_LAYERS_NUM];
#endif /* MPIU_SELECTION_TREE_TOOLS_H_INCLUDED */

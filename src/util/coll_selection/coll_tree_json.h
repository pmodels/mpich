#ifndef MPIU_COLL_SELECTION_TREE_TOOLS_H_INCLUDED
#define MPIU_COLL_SELECTION_TREE_TOOLS_H_INCLUDED

#include "coll_algo_params.h"
#include "coll_tree_bin_types.h"
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
#define MPIU_COLL_SELECTION_TREE_COLL_KEY_TOKEN "collective="
#define MPIU_COLL_SELECTION_TREE_COMMSIZE_KEY_TOKEN "comm_size="
#define MPIU_COLL_SELECTION_TREE_MSGSIZE_KEY_TOKEN "msg_size="

typedef json_type MPIU_COLL_SELECTION_TREE_NODE_TYPE;

static inline MPIU_COLL_SELECTION_TREE_NODE MPIU_COLL_SELECTION_TREE_text_to_json(char *contents);
static inline char *MPIU_COLL_SELECTION_TREE_get_node_json_name(MPIU_COLL_SELECTION_TREE_NODE
                                                                json_node, int index);
static inline MPIU_COLL_SELECTION_TREE_NODE
MPIU_COLL_SELECTION_TREE_get_node_json_next(MPIU_COLL_SELECTION_TREE_NODE json_node, int index);
static inline int MPIU_COLL_SELECTION_TREE_get_node_json_type(MPIU_COLL_SELECTION_TREE_NODE
                                                              json_node,
                                                              MPIU_COLL_SELECTION_TREE_NODE_TYPE
                                                              type);
static inline int MPIU_COLL_SELECTION_TREE_get_node_json_peer_count(MPIU_COLL_SELECTION_TREE_NODE
                                                                    json_node);
MPIU_COLL_SELECTION_TREE_NODE MPIU_COLL_SELECTION_TREE_read(char *file);
#endif /* MPIU_COLL_SELECTION_TREE_TOOLS_H_INCLUDED */

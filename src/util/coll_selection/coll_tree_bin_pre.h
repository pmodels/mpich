/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_FAILURE_ON_MATCH_FALLBACK
      category    : COLLECTIVE
      type        : boolean
      default     : 0
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Abort with error message instead of silent fallback
        to default tree node in case of condition mismatch

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

#ifndef MPIU_COLLi_SELECTION_TREE_PRE_H_INCLUDED
#define MPIU_COLL_SELECTION_TREE_PRE_H_INCLUDED

#include <stddef.h>

struct MPIU_COLL_SELECTION_tree_node;

#define MPIU_COLL_SELECTION_storage_handler ptrdiff_t
#define MPIU_COLL_SELECTION_HANDLER_TO_POINTER(_handler) ((struct MPIU_COLL_SELECTION_tree_node *)(MPIU_COLL_SELECTION_offset_tree + _handler))
#define MPIU_COLL_SELECTION_NODE_FIELD(_node, _field)  (MPIU_COLL_SELECTION_HANDLER_TO_POINTER(_node)->_field)

#define MPIU_COLL_SELECTION_NULL_ENTRY ((MPIU_COLL_SELECTION_storage_handler)-1)
#define MPIU_COLL_SELECTION_STORAGE_SIZE (1024*1024)
#define MPIU_COLL_SELECTION_COMPOSITION (0)

/* *INDENT-OFF* */
#define MPIU_COLL_SELECTION_level_match_condition(root_entry_, match_entry_, match_condition_) \
{                                                                                              \
    int i = 0;                                                                                 \
    int level_width = 0;                                                                       \
    match_entry_ = MPIU_COLL_SELECTION_NULL_ENTRY;                                             \
    level_width = MPIU_COLL_SELECTION_NODE_FIELD(root_entry_, children_count);                 \
    for (i = 0; i < level_width; i++) {                                                        \
        match_entry_ = MPIU_COLL_SELECTION_NODE_FIELD(root_entry_, offset[i]);                 \
        if (match_condition_) {                                                                \
            break;                                                                             \
        }                                                                                      \
    }                                                                                          \
    if (i == level_width) {                                                                    \
        if (MPIR_CVAR_FAILURE_ON_MATCH_FALLBACK) {                                             \
            MPIR_Assert_fail_fmt("FAILURE_ON_MATCH_FALLBACK", __FILE__,                        \
                                 __LINE__, "Key was not matched on layer type: %d",            \
                                 MPIU_COLL_SELECTION_NODE_FIELD(match_entry_, type));          \
        }                                                                                      \
        match_entry_ = MPIU_COLL_SELECTION_NODE_FIELD(root_entry_, offset[i - 1]);             \
    }                                                                                          \
}

#define MPIU_COLL_SELECTION_level_match_id(root_entry_, match_entry_, match_id_, level_width_)  \
{                                                                                               \
    int id = 0;                                                                                 \
    int level_width = 0;                                                                        \
    int match_entry_tmp_ = MPIU_COLL_SELECTION_NULL_ENTRY;                                      \
    match_entry_ = MPIU_COLL_SELECTION_NULL_ENTRY;                                              \
    level_width = MPIU_COLL_SELECTION_NODE_FIELD(root_entry_, children_count);                  \
    if (level_width == level_width_) {                                                          \
        if (match_id_ < level_width && match_id_ >= 0) {                                        \
            match_entry_tmp_ = MPIU_COLL_SELECTION_NODE_FIELD(root_entry_, offset[match_id_]);  \
            if (MPIU_COLL_SELECTION_NODE_FIELD(match_entry_tmp_, key) == match_id_) {           \
                match_entry_ = match_entry_tmp_;                                                \
            }                                                                                   \
        }                                                                                       \
    } else {                                                                                    \
        for (id = 0; id < level_width; id++) {                                                  \
            match_entry_tmp_ = MPIU_COLL_SELECTION_NODE_FIELD(root_entry_, offset[id]);         \
            if (MPIU_COLL_SELECTION_NODE_FIELD(match_entry_tmp_, key) == match_id_) {           \
                match_entry_ = match_entry_tmp_;                                                \
                break;                                                                          \
            }                                                                                   \
        }                                                                                       \
    }                                                                                           \
    if (match_entry_ == MPIU_COLL_SELECTION_NULL_ENTRY) {                                       \
        if (MPIR_CVAR_FAILURE_ON_MATCH_FALLBACK) {                                              \
            MPIR_Assert_fail_fmt("FAILURE_ON_MATCH_FALLBACK", __FILE__,                         \
                                 __LINE__, "Key was not matched on layer type: %d",             \
                                 MPIU_COLL_SELECTION_NODE_FIELD(root_entry_, next_layer_type)); \
        }                                                                                       \
    }                                                                                           \
}

#define MPIU_COLL_SELECTION_level_match_default(root_entry_, match_entry_)               \
{                                                                                        \
    int level_width = 0;                                                                 \
    level_width = MPIU_COLL_SELECTION_NODE_FIELD(root_entry_, children_count);           \
    match_entry_ = MPIU_COLL_SELECTION_NODE_FIELD(root_entry_, offset[level_width - 1]); \
}
/* *INDENT-ON* */

extern MPIU_COLL_SELECTION_storage_handler MPIU_COLL_SELECTION_tree_current_offset;
extern char MPIU_COLL_SELECTION_offset_tree[];

typedef MPIU_COLL_SELECTION_storage_handler(*MPIU_COLL_SELECTION_create_coll_tree_cb)
 (MPIU_COLL_SELECTION_storage_handler parent, int coll_id);

extern MPIU_COLL_SELECTION_create_coll_tree_cb coll_topo_aware_compositions[];
extern MPIU_COLL_SELECTION_create_coll_tree_cb coll_flat_compositions[];

#endif /* MPIU_COLL_SELECTION_PRE_TYPES_H_INCLUDED */

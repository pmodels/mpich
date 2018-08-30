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

#ifndef MPIU_SELECTION_TREE_PRE_H_INCLUDED
#define MPIU_SELECTION_TREE_PRE_H_INCLUDED

#include <stddef.h>

struct MPIU_SELECTION_tree_node;

typedef struct {
    uint8_t *base_addr;
    ptrdiff_t current_offset;
} MPIU_SELECTION_storage_handler;

#define MPIU_SELECTION_storage_entry ptrdiff_t
#define MPIU_SELECTION_HANDLER_TO_POINTER(_storage, _entry) ((struct MPIU_SELECTION_tree_node *)(_storage->base_addr + _entry))
#define MPIU_SELECTION_NODE_FIELD(_storage, _entry, _field)  (MPIU_SELECTION_HANDLER_TO_POINTER(_storage, _entry)->_field)

#define MPIU_SELECTION_NULL_STORAGE {NULL, 0}
#define MPIU_SELECTION_NULL_ENTRY ((MPIU_SELECTION_storage_entry)-1)
#define MPIU_SELECTION_STORAGE_SIZE (1024*1024)
#define MPIU_SELECTION_NAME_LENGTH  (256)
#define MPIU_SELECTION_DEFAULT_KEY (-1)

/* *INDENT-OFF* */
#define MPIU_SELECTION_level_match_condition(storage_, root_entry_, match_entry_, match_condition_)      \
{                                                                                                        \
    int i = 0;                                                                                           \
    int level_width = 0;                                                                                 \
    match_entry_ = MPIU_SELECTION_NULL_ENTRY;                                                            \
    level_width = MPIU_SELECTION_NODE_FIELD(storage_, root_entry_, children_count);                      \
    for (i = 0; i < level_width; i++) {                                                                  \
        match_entry_ = MPIU_SELECTION_NODE_FIELD(storage_, root_entry_, offset[i]);                      \
        if (match_condition_) {                                                                          \
            break;                                                                                       \
        }                                                                                                \
    }                                                                                                    \
    if (i == level_width) {                                                                              \
        if (MPIR_CVAR_FAILURE_ON_MATCH_FALLBACK) {                                                       \
            MPIR_Assert_fail_fmt("FAILURE_ON_MATCH_FALLBACK", __FILE__,                                  \
                                 __LINE__, "Key was not matched on layer type: %d",                      \
                                 MPIU_SELECTION_NODE_FIELD(storage_, match_entry_, type));               \
        }                                                                                                \
        match_entry_ = MPIU_SELECTION_NODE_FIELD(storage_, root_entry_, offset[i - 1]);                  \
    }                                                                                                    \
}

#define MPIU_SELECTION_level_match_complex_condition(storage_, root_entry_, match_entry_, ref_val_)                                          \
{                                                                                                                                            \
    int i = 0;                                                                                                                               \
    int level_width = 0;                                                                                                                     \
    int value_ = 0;                                                                                                                          \
    int is_pow2_ = 0;                                                                                                                        \
    char key_str_[MPIU_SELECTION_BUFFER_MAX_SIZE];                                                                                           \
    int key_int_ = 0;                                                                                                                        \
    int ceil_pof2 = 0;                                                                                                                       \
    match_entry_ = MPIU_SELECTION_NULL_ENTRY;                                                                                                \
    level_width = MPIU_SELECTION_NODE_FIELD(storage_, root_entry_, children_count);                                                          \
                                                                                                                                             \
    for (i = 0; i < level_width; i++) {                                                                                                      \
        match_entry_ = MPIU_SELECTION_NODE_FIELD(storage_, root_entry_, offset[i]);                                                          \
        value_ = MPIU_SELECTION_NODE_FIELD(storage_, match_entry_, key);                                                                     \
        MPIU_SELECTION_tree_convert_key_int(value_, key_str_, &is_pow2_);                                                                    \
        key_int_ = atoi(key_str_);                                                                                                           \
        if ((is_pow2_ && MPL_is_pof2(ref_val_, &ceil_pof2) && (((key_int_ != 0)  && (ref_val_ < key_int_)) || (key_int_ == 0))) ||           \
            (!is_pow2_ && (key_int_ != 0) && (ref_val_ < key_int_))) {                                                                       \
            break;                                                                                                                           \
        }                                                                                                                                    \
    }                                                                                                                                        \
    if (i == level_width) {                                                                                                                  \
        if (MPIR_CVAR_FAILURE_ON_MATCH_FALLBACK) {                                                                                           \
            MPIR_Assert_fail_fmt("FAILURE_ON_MATCH_FALLBACK", __FILE__,                                                                      \
                                 __LINE__, "Key was not matched on layer type: %d",                                                          \
                                 MPIU_SELECTION_NODE_FIELD(storage_, match_entry_, type));                                                   \
        }                                                                                                                                    \
        match_entry_ = MPIU_SELECTION_NODE_FIELD(storage_, root_entry_, offset[i - 1]);                                                      \
    }                                                                                                                                        \
}

#define MPIU_SELECTION_level_match_id(storage_, root_entry_, match_entry_, match_id_, level_width_)       \
{                                                                                                         \
    int id = 0;                                                                                           \
    int level_width = 0;                                                                                  \
    int match_entry_tmp_ = MPIU_SELECTION_NULL_ENTRY;                                                     \
    match_entry_ = MPIU_SELECTION_NULL_ENTRY;                                                             \
    level_width = MPIU_SELECTION_NODE_FIELD(storage_, root_entry_, children_count);                       \
    if (level_width == level_width_) {                                                                    \
        if (match_id_ < level_width && match_id_ >= 0) {                                                  \
            match_entry_tmp_ = MPIU_SELECTION_NODE_FIELD(storage_, root_entry_, offset[match_id_]);       \
            if (MPIU_SELECTION_NODE_FIELD(storage_, match_entry_tmp_, key) == match_id_) {                \
                match_entry_ = match_entry_tmp_;                                                          \
            }                                                                                             \
        }                                                                                                 \
    } else {                                                                                              \
        for (id = 0; id < level_width; id++) {                                                            \
            match_entry_tmp_ = MPIU_SELECTION_NODE_FIELD(storage_, root_entry_, offset[id]);              \
            if (MPIU_SELECTION_NODE_FIELD(storage_, match_entry_tmp_, key) == match_id_) {                \
                match_entry_ = match_entry_tmp_;                                                          \
                break;                                                                                    \
            }                                                                                             \
        }                                                                                                 \
    }                                                                                                     \
    if (match_entry_ == MPIU_SELECTION_NULL_ENTRY) {                                                      \
        if (MPIR_CVAR_FAILURE_ON_MATCH_FALLBACK) {                                                        \
            MPIR_Assert_fail_fmt("FAILURE_ON_MATCH_FALLBACK", __FILE__,                                   \
                                 __LINE__, "Key was not matched on layer type: %d",                       \
                                 MPIU_SELECTION_NODE_FIELD(storage_, root_entry_, next_layer_type));      \
        }                                                                                                 \
    }                                                                                                     \
}

#define MPIU_SELECTION_level_match_default(storage_, root_entry_, match_entry_)                \
{                                                                                              \
    int level_width = 0;                                                                       \
    level_width = MPIU_SELECTION_NODE_FIELD(storage_, root_entry_, children_count);            \
    match_entry_ = MPIU_SELECTION_NODE_FIELD(storage_, root_entry_, offset[level_width - 1]);  \
}
/* *INDENT-ON* */

extern MPIU_SELECTION_storage_handler MPIU_SELECTION_tree_global_storage;

typedef MPIU_SELECTION_storage_entry(*MPIU_SELECTION_create_coll_tree_cb)
 (MPIU_SELECTION_storage_handler * storage, MPIU_SELECTION_storage_entry parent, int coll_id);

extern MPIU_SELECTION_create_coll_tree_cb coll_topo_aware_compositions[];
extern MPIU_SELECTION_create_coll_tree_cb coll_flat_compositions[];

extern char *MPIU_SELECTION_coll_names[];
extern char *MPIU_SELECTION_comm_kind_names[];
extern char *MPIU_SELECTION_comm_hierarchy_names[];

#endif /* MPIU_SELECTION_PRE_H_INCLUDED */

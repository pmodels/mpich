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

#include "mpiimpl.h"
#include <stdio.h>
#include "coll_algo_params.h"
#include "ch4_coll_select_tree_types.h"
#include "ch4_coll_select_tree_build.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_FAIL_ON_MATCH_FALLBACK
      category    : COLLECTIVE
      type        : boolean
      default     : 0
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        If input is not as expected,
        abort with error message instead of silent fallback
        to default tree node

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

/* Match a specified condition across an entire level of the selection tree. A "level" is defined as
 * a group of sibling nodes that all have the same type, not necessarily all nodes at the same level
 * across the entire tree. */
#define MPIDU_SELECTION_level_match_condition(storage_, root_entry_, match_entry_, match_condition_)      \
{                                                                                                        \
    int i = 0;                                                                                           \
    int level_width = 0;                                                                                 \
                                                                                               \
    match_entry_ = MPIDU_SELECTION_NULL_ENTRY;                                                            \
    level_width = MPIDU_SELECTION_NODE_FIELD(storage_, root_entry_, children_count);                      \
                                                                                               \
    /* Check if the given condition is matched by any of the nodes in the given level */       \
    for (i = 0; i < level_width; i++) {                                                                  \
        match_entry_ = MPIDU_SELECTION_NODE_FIELD(storage_, root_entry_, offset[i]);                      \
        if (match_condition_) {                                                                          \
            break;                                                                                       \
        }                                                                                                \
    }                                                                                                    \
                                                                                               \
    /* If we didn't match anything, check to see whether we should return an error or */       \
    /* Generate a default selection. */                                                        \
    if (i == level_width) {                                                                              \
        if (MPIR_CVAR_FAIL_ON_MATCH_FALLBACK) {                                                       \
            MPIR_Assert_fail_fmt("FAILURE_ON_MATCH_FALLBACK", __FILE__,                                  \
                                 __LINE__, "Key was not matched on layer type: %d",                      \
                                 MPIDU_SELECTION_NODE_FIELD(storage_, match_entry_, type));               \
        }                                                                                                \
        match_entry_ = MPIDU_SELECTION_NODE_FIELD(storage_, root_entry_, offset[i - 1]);                  \
    }                                                                                                    \
}

/* If the match condition is a complex condition specified using a special keyword (eg; pow2),
 * we parse the information using special checks */
#define MPIDU_SELECTION_level_match_complex_condition(storage_, root_entry_, match_entry_, ref_val_)                                          \
{                                                                                                                                            \
    int i = 0;                                                                                                                               \
    int level_width = 0;                                                                                                                     \
    int value_ = 0;                                                                                                                          \
    int is_pow2_ = 0;                                                                                                                        \
    char key_str_[MPIDU_SELECTION_BUFFER_MAX_SIZE];                                                                                           \
    int key_int_ = 0;                                                                                                                        \
    int ceil_pof2 = 0;                                                                                                                       \
    match_entry_ = MPIDU_SELECTION_NULL_ENTRY;                                                                                                \
    level_width = MPIDU_SELECTION_NODE_FIELD(storage_, root_entry_, children_count);                                                          \
                                                                                                                                             \
    for (i = 0; i < level_width; i++) {                                                                                                      \
        match_entry_ = MPIDU_SELECTION_NODE_FIELD(storage_, root_entry_, offset[i]);                                                          \
        value_ = MPIDU_SELECTION_NODE_FIELD(storage_, match_entry_, key);                                                                     \
        MPIDU_SELECTION_tree_convert_key_int(value_, key_str_, &is_pow2_);                                                                    \
        key_int_ = atoi(key_str_);                                                                                                           \
        if ((is_pow2_ && MPL_is_pof2(ref_val_, &ceil_pof2) && (((key_int_ != 0)  && (ref_val_ < key_int_)) || (key_int_ == 0))) ||           \
            (!is_pow2_ && (key_int_ != 0) && (ref_val_ < key_int_))) {                                                                       \
            break;                                                                                                                           \
        }                                                                                                                                    \
    }                                                                                                                                        \
    if (i == level_width) {                                                                                                                  \
        if (MPIR_CVAR_FAIL_ON_MATCH_FALLBACK) {                                                                                           \
            MPIR_Assert_fail_fmt("FAILURE_ON_MATCH_FALLBACK", __FILE__,                                                                      \
                                 __LINE__, "Key was not matched on layer type: %d",                                                          \
                                 MPIDU_SELECTION_NODE_FIELD(storage_, match_entry_, type));                                                   \
        }                                                                                                                                    \
        match_entry_ = MPIDU_SELECTION_NODE_FIELD(storage_, root_entry_, offset[i - 1]);                                                      \
    }                                                                                                                                        \
}

#define MPIDU_SELECTION_level_match_id(storage_, root_entry_, match_entry_, match_id_, level_width_)       \
{                                                                                                         \
    int id = 0;                                                                                           \
    int level_width = 0;                                                                                  \
    int match_entry_tmp_ = MPIDU_SELECTION_NULL_ENTRY;                                                     \
                                                                                                          \
    match_entry_ = MPIDU_SELECTION_NULL_ENTRY;                                                             \
    level_width = MPIDU_SELECTION_NODE_FIELD(storage_, root_entry_, children_count);                       \
                                                                                                          \
    /* Check if the current level all possible children. If so, we can matched entry by simply specifying the
 * offset of match_id. Else we need to iterate through all the children in the level to find the matching entry */                                                                        \
    if (level_width == level_width_) {                                                                    \
        if (match_id_ < level_width && match_id_ >= 0) {                                                  \
            match_entry_tmp_ = MPIDU_SELECTION_NODE_FIELD(storage_, root_entry_, offset[match_id_]);       \
            if (MPIDU_SELECTION_NODE_FIELD(storage_, match_entry_tmp_, key) == match_id_) {                \
                match_entry_ = match_entry_tmp_;                                                          \
            }                                                                                             \
        }                                                                                                 \
    } else {                                                                                              \
        for (id = 0; id < level_width; id++) {                                                            \
            match_entry_tmp_ = MPIDU_SELECTION_NODE_FIELD(storage_, root_entry_, offset[id]);              \
            if (MPIDU_SELECTION_NODE_FIELD(storage_, match_entry_tmp_, key) == match_id_) {                \
                match_entry_ = match_entry_tmp_;                                                          \
                break;                                                                                    \
            }                                                                                             \
        }                                                                                                 \
    }                                                                                                     \
                                                                                                          \
    /* If we didn't match anything, check to see whether we should return an error or */                  \
    /* Generate a default selection. */                                                                   \
    if (match_entry_ == MPIDU_SELECTION_NULL_ENTRY) {                                                      \
        if (MPIR_CVAR_FAIL_ON_MATCH_FALLBACK) {                                                        \
            MPIR_Assert_fail_fmt("FAILURE_ON_MATCH_FALLBACK", __FILE__,                                   \
                                 __LINE__, "Key was not matched on layer type: %d",                       \
                                 MPIDU_SELECTION_NODE_FIELD(storage_, root_entry_, next_layer_type));      \
        }                                                                                                 \
    }                                                                                                     \
}

/* Pick the default entry for a particular level, which is always the last entry. */
#define MPIDU_SELECTION_level_match_default(storage_, root_entry_, match_entry_)                \
{                                                                                              \
    int level_width = 0;                                                                       \
    level_width = MPIDU_SELECTION_NODE_FIELD(storage_, root_entry_, children_count);            \
    match_entry_ = MPIDU_SELECTION_NODE_FIELD(storage_, root_entry_, offset[level_width - 1]);  \
}

/* Get the collective container for a particular node. The node must be a leaf node or this function
 * will return NULL to indicate that it does not have a container associated with it. */
void *MPIDU_SELECTION_get_container(MPIDU_SELECTION_storage_handler * storage,
                                    MPIDU_SELECTION_storage_entry node)
{
    if ((MPIDU_SELECTION_NODE_FIELD(storage, node, type) == MPIDU_SELECTION_CONTAINER) &&
        MPIDU_SELECTION_NODE_FIELD(storage, node, containers) != NULL) {
        return (void *) MPIDU_SELECTION_NODE_FIELD(storage, node, containers);
    } else {
        return NULL;
    }
}

/* Construct the match pattern object.
 * match_pattern - The object which will contain the information about how to perform a match at a
 *                 particular node of the selection tree.
 * layer_type - The type of node where the match pattern is being used which determines the kind of
 *              information stored in the match pattern object.
 * key - The key that will be used to match against.
 */
void MPIDU_SELECTION_set_match_pattern_key(MPIDU_SELECTION_match_pattern_t * match_pattern,
                                           MPIDU_SELECTION_node_type_t layer_type, int key)
{
    switch (layer_type) {
        case MPIDU_SELECTION_DIRECTORY:
            match_pattern->directory = key;
            break;
        case MPIDU_SELECTION_COMM_KIND:
            match_pattern->comm_kind = key;
            break;
        case MPIDU_SELECTION_COMM_HIERARCHY:
            match_pattern->comm_hierarchy_kind = key;
            break;
        case MPIDU_SELECTION_COLLECTIVE:
            match_pattern->coll_id = key;
            break;
        case MPIDU_SELECTION_COMMSIZE:
            match_pattern->comm_size = key;
            break;
        case MPIDU_SELECTION_MSGSIZE:
            match_pattern->msg_size = key;
            break;
        default:
            break;
    }
}

/* Retrieve the key used by the match pattern object. The key is the return value.
 * match_pattern - The object which will contain the information about how to perform a match at a
 *                 particular node of the selection tree.
 * layer_type - The type of node where the match pattern is being used which determines the kind of
 *              information stored in the match pattern object.
 */
int MPIDU_SELECTION_get_match_pattern_key(MPIDU_SELECTION_match_pattern_t * match_pattern,
                                          MPIDU_SELECTION_node_type_t layer_type)
{
    switch (layer_type) {
        case MPIDU_SELECTION_DIRECTORY:
            return match_pattern->directory;
        case MPIDU_SELECTION_COMM_KIND:
            return match_pattern->comm_kind;
        case MPIDU_SELECTION_COMM_HIERARCHY:
            return match_pattern->comm_hierarchy_kind;
        case MPIDU_SELECTION_COLLECTIVE:
            return match_pattern->coll_id;
        case MPIDU_SELECTION_COMMSIZE:
            return match_pattern->comm_size;
        case MPIDU_SELECTION_MSGSIZE:
            return match_pattern->msg_size;
        default:
            break;
    }
    return MPIDU_SELECTION_DEFAULT_KEY;
}

/* Initialize the match pattern object by setting all of the variables to dummy values. */
void MPIDU_SELECTION_init_match_pattern(MPIDU_SELECTION_match_pattern_t * match_pattern)
{
    MPIR_Assert(match_pattern != NULL);
    match_pattern->terminal_node_type = MPIDU_SELECTION_DEFAULT_TERMINAL_NODE_TYPE;

    match_pattern->directory = -1;
    match_pattern->comm_kind = -1;
    match_pattern->comm_hierarchy_kind = -1;
    match_pattern->coll_id = -1;
    match_pattern->comm_size = -1;
    match_pattern->msg_size = -1;
}

/* Set up a match pattern for a particular communicator, using its size, kind, and hierarchy. */
void MPIDU_SELECTION_init_comm_match_pattern(MPIR_Comm * comm,
                                             MPIDU_SELECTION_match_pattern_t *
                                             match_pattern, MPIDU_SELECTION_node_type_t
                                             terminal_node_type)
{
    MPIDU_SELECTION_init_match_pattern(match_pattern);

    match_pattern->terminal_node_type = terminal_node_type;
    MPIR_Assert(comm != NULL);
    MPIDU_SELECTION_set_match_pattern_key(match_pattern, MPIDU_SELECTION_COMMSIZE,
                                          comm->local_size);
    MPIDU_SELECTION_set_match_pattern_key(match_pattern, MPIDU_SELECTION_COMM_KIND,
                                          comm->comm_kind);
    switch (comm->hierarchy_kind) {
        case MPIR_COMM_HIERARCHY_KIND__FLAT:
            MPL_FALLTHROUGH;
        case MPIR_COMM_HIERARCHY_KIND__PARENT:
            MPIDU_SELECTION_set_match_pattern_key(match_pattern,
                                                  MPIDU_SELECTION_COMM_HIERARCHY,
                                                  comm->hierarchy_kind);
            break;
        case MPIR_COMM_HIERARCHY_KIND__NODE_ROOTS:
            MPL_FALLTHROUGH;
        case MPIR_COMM_HIERARCHY_KIND__NODE:
            MPIDU_SELECTION_set_match_pattern_key(match_pattern,
                                                  MPIDU_SELECTION_COMM_HIERARCHY,
                                                  MPIR_COMM_HIERARCHY_KIND__FLAT);
            break;
        default:
            MPIR_Assert(0);
            break;
    }
}

/* Initialize a match pattern to select a particular collective using its signature. It will make
 * sure to match the collective type as well as other specifics for a particular collective, if
 * applicable. */
void MPIDU_SELECTION_init_coll_match_pattern(MPIDU_SELECTON_coll_signature_t * coll_sig,
                                             MPIDU_SELECTION_match_pattern_t *
                                             match_pattern, MPIDU_SELECTION_node_type_t
                                             terminal_node_type)
{
    MPI_Aint type_size = 0;

    MPIR_Assert(coll_sig != NULL);

    match_pattern->terminal_node_type = terminal_node_type;

    MPIDU_SELECTION_set_match_pattern_key(match_pattern,
                                          MPIDU_SELECTION_COLLECTIVE, coll_sig->coll_id);

    switch (coll_sig->coll_id) {
        case MPIDU_SELECTION_BCAST:
            MPIR_Datatype_get_size_macro(coll_sig->coll.bcast.datatype, type_size);
            MPIDU_SELECTION_set_match_pattern_key(match_pattern,
                                                  MPIDU_SELECTION_MSGSIZE,
                                                  (type_size * coll_sig->coll.bcast.count));
            break;
        case MPIDU_SELECTION_BARRIER:
            break;
        default:
            break;
    }
}

MPIDU_SELECTION_storage_entry
MPIDU_SELECTION_find_entry(MPIDU_SELECTION_storage_handler * storage,
                           MPIDU_SELECTION_storage_entry root_node,
                           MPIDU_SELECTION_match_pattern_t * match_pattern)
{
    MPIDU_SELECTION_node_type_t root_type = MPIDU_SELECTION_DEFAULT_TERMINAL_NODE_TYPE;
    MPIDU_SELECTION_node_type_t next_layer_type = MPIDU_SELECTION_DEFAULT_TERMINAL_NODE_TYPE;
    MPIDU_SELECTION_storage_entry match_node = MPIDU_SELECTION_NULL_ENTRY;
    int match_pattern_key = 0;

    if (root_node != MPIDU_SELECTION_NULL_ENTRY) {

        root_type = MPIDU_SELECTION_NODE_FIELD(storage, root_node, type);

        /* Keep searching until we find the node we're looking for or we find an empty node. */
        while ((root_type != match_pattern->terminal_node_type) &&
               (root_type != MPIDU_SELECTION_DEFAULT_TERMINAL_NODE_TYPE)) {

            next_layer_type = MPIDU_SELECTION_NODE_FIELD(storage, root_node, next_layer_type);
            match_pattern_key =
                MPIDU_SELECTION_get_match_pattern_key(match_pattern, next_layer_type);

            switch (next_layer_type) {
                case MPIDU_SELECTION_DIRECTORY:
                    MPL_FALLTHROUGH;
                case MPIDU_SELECTION_COMM_KIND:
                    MPL_FALLTHROUGH;
                case MPIDU_SELECTION_COMM_HIERARCHY:
                    MPIDU_SELECTION_level_match_condition(storage, root_node, match_node,
                                                          (match_pattern_key ==
                                                           MPIDU_SELECTION_NODE_FIELD
                                                           (storage, match_node, key)));
                    break;
                case MPIDU_SELECTION_COLLECTIVE:
                    MPIDU_SELECTION_level_match_id(storage, root_node, match_node,
                                                   match_pattern_key,
                                                   MPIDU_SELECTION_COLLECTIVES_MAX);
                    break;
                case MPIDU_SELECTION_COMMSIZE:
                    MPIDU_SELECTION_level_match_complex_condition(storage, root_node, match_node,
                                                                  match_pattern_key);
                    break;
                case MPIDU_SELECTION_MSGSIZE:
                    MPIDU_SELECTION_level_match_condition(storage, root_node, match_node,
                                                          (match_pattern_key <=
                                                           MPIDU_SELECTION_NODE_FIELD
                                                           (storage, match_node, key)));
                    break;
                default:
                    /* Move to the next layer from the last element on current layer */
                    MPIDU_SELECTION_level_match_default(storage, root_node, match_node);
                    break;
            }

            /* Stop searching if a layer with a given key has not been found */
            if (match_node == MPIDU_SELECTION_NULL_ENTRY) {
                return MPIDU_SELECTION_NULL_ENTRY;
            }
            root_node = match_node;
            root_type = MPIDU_SELECTION_NODE_FIELD(storage, root_node, type);
        }
        return match_node;
    }

    return MPIDU_SELECTION_NULL_ENTRY;
}

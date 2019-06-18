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
#include <sys/stat.h>
#include "ch4_coll_select_tree_types.h"
#include "ch4_coll_select_tree_build.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_COLL_SELECTION_TUNING_FILE
      category    : COLLECTIVE
      type        : string
      default     : "../src/mpid/ch4/src/tuning/legacy_logic.json"
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Defines the name of tuning file

    - name        : MPIR_CVAR_COLL_NETMOD_SELECTION_TUNING_FILE
      category    : COLLECTIVE
      type        : string
      default     : "../src/mpid/ch4/src/tuning/netmod_logic.json"
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Defines the name of tuning file

    - name        : MPIR_CVAR_COLL_SHM_SELECTION_TUNING_FILE
      category    : COLLECTIVE
      type        : string
      default     : "../src/mpid/ch4/src/tuning/shm_logic.json"
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Defines the name of tuning file

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

static char offset_tree[MPIDU_SELECTION_STORAGE_SIZE];
MPIDU_SELECTION_storage_handler MPIDU_SELECTION_tree_global_storage = {
    .base_addr = offset_tree,
    .current_offset = 0
};

int MPIDU_SELECTION_tree_current_collective_id = 0;
/*This array consists of all the possible collectives. Default compositions for
 * each collective should be specified in the same order for correct access */
const char *MPIDU_SELECTION_coll_names[] = {
    "ALLGATHER",
    "ALLGATHERV",
    "ALLREDUCE",
    "ALLTOALL",
    "ALLTOALLV",
    "ALLTOALLW",
    "BARRIER",
    "BCAST",
    "EXSCAN",
    "GATHER",
    "GATHERV",
    "REDUCE_SCATTER",
    "REDUCE",
    "SCAN",
    "SCATTER",
    "SCATTERV",
    "REDUCE_SCATTER_BLOCK",
    "IALLGATHER",
    "IALLGATHERV",
    "IALLREDUCE",
    "IALLTOALL",
    "IALLTOALLV",
    "IALLTOALLW",
    "IBARRIER",
    "IBCAST",
    "IEXSCAN",
    "IGATHER",
    "IGATHERV",
    "IREDUCE_SCATTER",
    "IREDUCE",
    "ISCAN",
    "ISCATTER",
    "ISCATTERV",
    "IREDUCE_SCATTER_BLOCK"
};

const char *MPIDU_SELECTION_comm_kind_names[] = {
    "INTRA_COMM",
    "INTER_COMM"
};

const char *MPIDU_SELECTION_comm_hierarchy_names[] = {
    "FLAT_COMM",
    "TOPO_COMM"
};

/* Function to create a non-leaf node in the selection tree data structure. The node will have a
 * parent (which could be NULL to indicate that this is the root of the tree), and some children
 * (which should not be NULL or this would be a leaf node).
 * parent          - A pointer to the parent node of the new node. Could be
 *                   MPIDU_SELECTION_NULL_ENTRY if this is a root node.
 * node_type       - The type of node to construct. Types are listed in the definition of
 *                   MPIDU_SELECTION_node_types_t.
 * next_layer_type - The type of the next node down in the tree. Types are listed in the definition
 *                   of MPIDU_SELECTION_node_types_t.
 * node_key        - The Key to identify the node. Decisions of tree traversal are based on the node_key
 * children_count  - The number of children that this node will have.
 */
MPIDU_SELECTION_storage_entry
MPIDU_SELECTION_create_node(MPIDU_SELECTION_storage_handler * storage,
                            MPIDU_SELECTION_storage_entry parent,
                            MPIDU_SELECTION_node_type_t node_type,
                            MPIDU_SELECTION_node_type_t next_layer_type,
                            int node_key, int children_count)
{
    int node_offset = 0;
    int child_index = 0;
    /* Get information about where to create the node in the storage space. Storage space is linear. */
    MPIDU_SELECTION_storage_entry tmp = storage->current_offset;

    /*Calculate node_offset which tells you where to put the next peer because all my children sit next to me */
    node_offset =
        sizeof(MPIDU_SELECTION_node_t) + sizeof(MPIDU_SELECTION_storage_entry) * children_count;

    if (parent != MPIDU_SELECTION_NULL_ENTRY) {
        /*Get information about how many children of my parents are already created */
        child_index = MPIDU_SELECTION_NODE_FIELD(storage, parent, cur_child_idx);
        /* Add information about my location to my parent's node info */
        MPIDU_SELECTION_NODE_FIELD(storage, parent, offset[child_index]) = tmp;
        MPIDU_SELECTION_NODE_FIELD(storage, parent, cur_child_idx)++;
    }

    storage->current_offset += node_offset;
    MPIR_Assertp(storage->current_offset <= MPIDU_SELECTION_STORAGE_SIZE);

    MPIDU_SELECTION_NODE_FIELD(storage, tmp, parent) = parent;
    MPIDU_SELECTION_NODE_FIELD(storage, tmp, cur_child_idx) = 0;
    MPIDU_SELECTION_NODE_FIELD(storage, tmp, type) = node_type;
    MPIDU_SELECTION_NODE_FIELD(storage, tmp, next_layer_type) = next_layer_type;
    MPIDU_SELECTION_NODE_FIELD(storage, tmp, key) = node_key;
    MPIDU_SELECTION_NODE_FIELD(storage, tmp, children_count) = children_count;

    return tmp;
}

/* Function to create a leaf node in the selection tree data structure.
 * parent           - A pointer to the parent node of the new node. Could be
 *                    MPIDU_SELECTION_NULL_ENTRY if this is a root node.
 * node_type        - The type of node to construct. Types are listed in the definition of
 *                    MPIDU_SELECTION_node_types_t.
 * containers_count - Total of number of containers associated with the leaf node
 * containers       - List of containers. If a collective is performed using a combination of collectives and/or
 *                    algorithms at different levels (netmod/shm), each of those will have a container associated with it
 */
void
MPIDU_SELECTION_create_leaf(MPIDU_SELECTION_storage_handler * storage,
                            MPIDU_SELECTION_storage_entry parent, int node_type,
                            int containers_count, void *containers)
{
    int node_offset = 0;
    MPIDU_SELECTION_storage_entry tmp = storage->current_offset;

    node_offset =
        sizeof(MPIDU_SELECTION_node_t) +
        sizeof(MPIDIG_coll_algo_generic_container_t) * containers_count;

    if (parent != MPIDU_SELECTION_NULL_ENTRY)
        MPIDU_SELECTION_NODE_FIELD(storage, parent, offset[0]) = tmp;

    memset(MPIDU_SELECTION_HANDLER_TO_POINTER(storage, tmp), 0, node_offset);
    storage->current_offset += node_offset;

    MPIDU_SELECTION_NODE_FIELD(storage, tmp, parent) = parent;
    MPIDU_SELECTION_NODE_FIELD(storage, tmp, type) = node_type;
    MPIDU_SELECTION_NODE_FIELD(storage, tmp, children_count) = containers_count;
    if (containers != NULL) {
        MPIR_Memcpy((void *) MPIDU_SELECTION_NODE_FIELD(storage, tmp, containers), containers,
                    sizeof(MPIDIG_coll_algo_generic_container_t) * containers_count);
    }
}

/* Load the collective selection tree. If the user provided a tuning file, use that to construct the
 * selection tree, otherwise load the defaults. */
int MPIDU_SELECTION_init()
{
    int mpi_errno = MPI_SUCCESS;

    MPIDI_global.coll_selection = MPIDU_SELECTION_tree_load(&MPIDU_SELECTION_tree_global_storage);

    if (MPIDI_global.coll_selection == MPIDU_SELECTION_NULL_ENTRY) {
        mpi_errno = MPI_ERR_OTHER;
    }

    return mpi_errno;
}

/* Build the generic part of the selection tree.
 * root - The root of the tree being created
 * inter_comm_subtree - The first node of the subtree used for inter-communicators
 * topo_aware_comm_subtree - The first node of the subtree used for topology aware communicators
 * flat_comm_subtree - The first node of the subtree used for topology unaware communicators
 */
void MPIDU_SELECTION_build_selection_tree_generic_part(MPIDU_SELECTION_storage_handler * storage,
                                                       MPIDU_SELECTION_storage_entry * root,
                                                       MPIDU_SELECTION_storage_entry * composition,
                                                       MPIDU_SELECTION_storage_entry *
                                                       netmod_subtree,
                                                       MPIDU_SELECTION_storage_entry * shm_subtree,
                                                       MPIDU_SELECTION_storage_entry *
                                                       inter_comm_subtree,
                                                       MPIDU_SELECTION_storage_entry *
                                                       topo_aware_comm_subtree,
                                                       MPIDU_SELECTION_storage_entry *
                                                       flat_comm_subtree)
{
    MPIDU_SELECTION_storage_entry intra_comm_subtree = MPIDU_SELECTION_NULL_ENTRY;

    *root =
        MPIDU_SELECTION_create_node(storage, *root,
                                    MPIDU_SELECTION_DEFAULT_NODE_TYPE,
                                    MPIDU_SELECTION_DIRECTORY, -1,
                                    MPIDU_SELECTION_DIRECTORY_KIND_NUM);
    /*Ch4 level selection tree */
    *composition =
        MPIDU_SELECTION_create_node(storage, *root, MPIDU_SELECTION_DIRECTORY,
                                    MPIDU_SELECTION_COMM_KIND,
                                    MPIDU_SELECTION_COMPOSITION, MPIDU_SELECTION_COMM_KIND_NUM);

    *netmod_subtree =
        MPIDU_SELECTION_create_node(storage, *root, MPIDU_SELECTION_DIRECTORY,
                                    MPIDU_SELECTION_COLLECTIVE,
                                    MPIDU_SELECTION_NETMOD, MPIDU_SELECTION_COLLECTIVES_MAX);

    *shm_subtree =
        MPIDU_SELECTION_create_node(storage, *root, MPIDU_SELECTION_DIRECTORY,
                                    MPIDU_SELECTION_COLLECTIVE,
                                    MPIDU_SELECTION_SHM, MPIDU_SELECTION_COLLECTIVES_MAX);
    /* The nodes after inter-communicators are used to determine the kind of collective being
     * performed. */
    *inter_comm_subtree =
        MPIDU_SELECTION_create_node(storage, *composition, MPIDU_SELECTION_COMM_KIND,
                                    MPIDU_SELECTION_COLLECTIVE,
                                    MPIDU_SELECTION_INTER_COMM, MPIDU_SELECTION_COLLECTIVES_MAX);

    /* The nodes after intra-communicators are used to determine the topology awareness of the
     * communicator. */
    intra_comm_subtree =
        MPIDU_SELECTION_create_node(storage, *composition, MPIDU_SELECTION_COMM_KIND,
                                    MPIDU_SELECTION_COMM_HIERARCHY,
                                    MPIDU_SELECTION_INTRA_COMM, MPIDU_SELECTION_COMM_HIERARCHY_NUM);

    /* The nodes immediately below the topology awareness of the communicator are used to determine
     * the kind of collective being performed. */
    *topo_aware_comm_subtree =
        MPIDU_SELECTION_create_node(storage, intra_comm_subtree, MPIDU_SELECTION_COMM_HIERARCHY,
                                    MPIDU_SELECTION_COLLECTIVE, MPIDU_SELECTION_TOPO_COMM,
                                    MPIDU_SELECTION_COLLECTIVES_MAX);
    *flat_comm_subtree =
        MPIDU_SELECTION_create_node(storage, intra_comm_subtree, MPIDU_SELECTION_COMM_HIERARCHY,
                                    MPIDU_SELECTION_COLLECTIVE, MPIDU_SELECTION_FLAT_COMM,
                                    MPIDU_SELECTION_COLLECTIVES_MAX);
}

/* Functions to create default collective selection trees for a particular collective. */

void
MPIDU_SELECTION_create_coll_tree_inter_compositions_default(MPIDU_SELECTION_storage_handler *
                                                            storage,
                                                            MPIDU_SELECTION_storage_entry parent,
                                                            int coll_id)
{
    MPIDU_SELECTION_storage_entry coll = MPIDU_SELECTION_NULL_ENTRY;

    coll = MPIDU_SELECTION_create_node(storage, parent, MPIDU_SELECTION_COLLECTIVE,
                                       MPIDU_SELECTION_CONTAINER, coll_id, 1);

    MPIDU_SELECTION_create_leaf(storage, coll, MPIDU_SELECTION_CONTAINER, 1, (void *)
                                MPIDI_Coll_inter_composition_default_container[coll_id]);
}

#ifndef MPIDI_CH4_DIRECT_NETMOD
void
MPIDU_SELECTION_create_coll_tree_shm_algorithms_default(MPIDU_SELECTION_storage_handler *
                                                        storage,
                                                        MPIDU_SELECTION_storage_entry parent,
                                                        int coll_id)
{
    MPIDU_SELECTION_storage_entry coll = MPIDU_SELECTION_NULL_ENTRY;

    coll = MPIDU_SELECTION_create_node(storage, parent, MPIDU_SELECTION_COLLECTIVE,
                                       MPIDU_SELECTION_CONTAINER, coll_id, 1);

    MPIDU_SELECTION_create_leaf(storage, coll, MPIDU_SELECTION_CONTAINER, 1, (void *)
                                MPIDI_SHM_get_default_container(coll_id));
}
#endif

void
MPIDU_SELECTION_create_coll_tree_netmod_algorithms_default(MPIDU_SELECTION_storage_handler *
                                                           storage,
                                                           MPIDU_SELECTION_storage_entry parent,
                                                           int coll_id)
{
    MPIDU_SELECTION_storage_entry coll = MPIDU_SELECTION_NULL_ENTRY;

    coll = MPIDU_SELECTION_create_node(storage, parent, MPIDU_SELECTION_COLLECTIVE,
                                       MPIDU_SELECTION_CONTAINER, coll_id, 1);

    MPIDU_SELECTION_create_leaf(storage, coll, MPIDU_SELECTION_CONTAINER, 1, (void *)
                                MPIDI_NM_get_default_container(coll_id));
}

void
MPIDU_SELECTION_create_coll_tree_topo_aware_compositions_default(MPIDU_SELECTION_storage_handler *
                                                                 storage,
                                                                 MPIDU_SELECTION_storage_entry
                                                                 parent, int coll_id)
{
    MPIDU_SELECTION_storage_entry coll = MPIDU_SELECTION_NULL_ENTRY;

    coll = MPIDU_SELECTION_create_node(storage, parent, MPIDU_SELECTION_COLLECTIVE,
                                       MPIDU_SELECTION_CONTAINER, coll_id, 1);

    MPIDU_SELECTION_create_leaf(storage, coll, MPIDU_SELECTION_CONTAINER, 1, (void *)
                                MPIDI_Coll_intra_composition_default_container[coll_id]);
}

void
MPIDU_SELECTION_create_coll_tree_flat_compositions_default(MPIDU_SELECTION_storage_handler *
                                                           storage,
                                                           MPIDU_SELECTION_storage_entry parent,
                                                           int coll_id)
{
    MPIDU_SELECTION_storage_entry coll = MPIDU_SELECTION_NULL_ENTRY;

    coll = MPIDU_SELECTION_create_node(storage, parent, MPIDU_SELECTION_COLLECTIVE,
                                       MPIDU_SELECTION_CONTAINER, coll_id, 1);

    MPIDU_SELECTION_create_leaf(storage, coll, MPIDU_SELECTION_CONTAINER, 1, (void *)
                                MPIDI_Coll_intra_composition_default_container[coll_id]);
}

/* Set up the default compositions for inter-communicators */
void MPIDU_SELECTION_build_selection_tree_default_inter(MPIDU_SELECTION_storage_handler * storage,
                                                        MPIDU_SELECTION_storage_entry
                                                        inter_comm_subtree)
{
    int coll_id = 0;

    for (coll_id = 0; coll_id < MPIDU_SELECTION_COLLECTIVES_MAX; coll_id++) {
        MPIDU_SELECTION_create_coll_tree_inter_compositions_default(storage, inter_comm_subtree,
                                                                    coll_id);
    }
}

/* Set up the default compositions for topology aware communicators */
void MPIDU_SELECTION_build_selection_tree_default_topo_aware(MPIDU_SELECTION_storage_handler *
                                                             storage,
                                                             MPIDU_SELECTION_storage_entry
                                                             topo_aware_comm_subtree)
{
    int coll_id = 0;

    for (coll_id = 0; coll_id < MPIDU_SELECTION_COLLECTIVES_MAX; coll_id++) {
        MPIDU_SELECTION_create_coll_tree_topo_aware_compositions_default(storage,
                                                                         topo_aware_comm_subtree,
                                                                         coll_id);
    }
}

/* Set up the default compositions for topology unaware communicators */
void MPIDU_SELECTION_build_selection_tree_default_flat(MPIDU_SELECTION_storage_handler * storage,
                                                       MPIDU_SELECTION_storage_entry
                                                       flat_comm_subtree)
{
    int coll_id = 0;

    for (coll_id = 0; coll_id < MPIDU_SELECTION_COLLECTIVES_MAX; coll_id++) {
        MPIDU_SELECTION_create_coll_tree_flat_compositions_default(storage, flat_comm_subtree,
                                                                   coll_id);
    }
}

/* Load the selection tree from a file (if provided) or construct a generic tree (if filename is
 * NULL). */
MPIDU_SELECTION_storage_entry MPIDU_SELECTION_tree_load(MPIDU_SELECTION_storage_handler * storage)
{
    MPIDU_SELECTION_storage_entry root = MPIDU_SELECTION_NULL_ENTRY;

    /* construct a generic selection tree. */
    MPIDU_SELECTION_storage_entry inter_comm_subtree = MPIDU_SELECTION_NULL_ENTRY;
    MPIDU_SELECTION_storage_entry composition = MPIDU_SELECTION_NULL_ENTRY;
    MPIDU_SELECTION_storage_entry netmod_subtree = MPIDU_SELECTION_NULL_ENTRY;
    MPIDU_SELECTION_storage_entry shm_subtree = MPIDU_SELECTION_NULL_ENTRY;
    MPIDU_SELECTION_storage_entry topo_aware_comm_subtree = MPIDU_SELECTION_NULL_ENTRY;
    MPIDU_SELECTION_storage_entry flat_comm_subtree = MPIDU_SELECTION_NULL_ENTRY;
    MPIDU_SELECTION_json_node_t custom_subtree = MPIDU_SELECTION_NULL_TYPE;
    int coll_id = 0;

    /* Build generic part of the selection tree */
    MPIDU_SELECTION_build_selection_tree_generic_part(storage,
                                                      &root, &composition, &netmod_subtree,
                                                      &shm_subtree,
                                                      &inter_comm_subtree,
                                                      &topo_aware_comm_subtree, &flat_comm_subtree);

    /***************************************************************************/
    /*      Build selection trees from JSON node for topo aware comm           */
    /***************************************************************************/
    custom_subtree = MPIDU_SELECTION_tree_read(MPIR_CVAR_COLL_SELECTION_TUNING_FILE);
    for (coll_id = 0; coll_id < MPIDU_SELECTION_COLLECTIVES_MAX; coll_id++) {
        MPIDU_SELECTION_json_node_t coll_json = MPIDU_SELECTION_NULL_TYPE;
        int is_found = 0;

        MPIDU_SELECTION_tree_init_json_node(&coll_json);
        if (coll_json != MPIDU_SELECTION_NULL_TYPE) {
            is_found = MPIDU_SELECTION_tree_is_coll_in_json(custom_subtree, &coll_json, coll_id);
            if (is_found) {
                MPIDU_SELECTION_json_to_selection_tree(storage, coll_json, topo_aware_comm_subtree);
            } else {
                MPIDU_SELECTION_create_coll_tree_topo_aware_compositions_default(storage,
                                                                                 topo_aware_comm_subtree,
                                                                                 coll_id);
            }
            MPIDU_SELECTION_tree_free_json_node(coll_json);
        } else {
            MPIDU_SELECTION_create_coll_tree_topo_aware_compositions_default(storage,
                                                                             topo_aware_comm_subtree,
                                                                             coll_id);
        }
    }

    custom_subtree = MPIDU_SELECTION_tree_read(MPIR_CVAR_COLL_NETMOD_SELECTION_TUNING_FILE);
    for (coll_id = 0; coll_id < MPIDU_SELECTION_COLLECTIVES_MAX; coll_id++) {
        MPIDU_SELECTION_json_node_t coll_json = MPIDU_SELECTION_NULL_TYPE;
        int is_found = 0;

        MPIDU_SELECTION_tree_init_json_node(&coll_json);
        if (coll_json != MPIDU_SELECTION_NULL_TYPE) {
            is_found = MPIDU_SELECTION_tree_is_coll_in_json(custom_subtree, &coll_json, coll_id);
            if (is_found) {
                MPIDU_SELECTION_json_to_selection_tree(storage, coll_json, netmod_subtree);
            } else {
                MPIDU_SELECTION_create_coll_tree_netmod_algorithms_default(storage, netmod_subtree,
                                                                           coll_id);
            }
            MPIDU_SELECTION_tree_free_json_node(coll_json);
        } else {
            MPIDU_SELECTION_create_coll_tree_netmod_algorithms_default(storage, netmod_subtree,
                                                                       coll_id);
        }
    }
#ifndef MPIDI_CH4_DIRECT_NETMOD
    /*SHM layer tree */
    custom_subtree = MPIDU_SELECTION_tree_read(MPIR_CVAR_COLL_SHM_SELECTION_TUNING_FILE);
    for (coll_id = 0; coll_id < MPIDU_SELECTION_COLLECTIVES_MAX; coll_id++) {
        MPIDU_SELECTION_json_node_t coll_json = MPIDU_SELECTION_NULL_TYPE;
        int is_found = 0;

        MPIDU_SELECTION_tree_init_json_node(&coll_json);
        if (coll_json != MPIDU_SELECTION_NULL_TYPE) {
            is_found = MPIDU_SELECTION_tree_is_coll_in_json(custom_subtree, &coll_json, coll_id);
            if (is_found) {
                MPIDU_SELECTION_json_to_selection_tree(storage, coll_json, shm_subtree);
            } else {
                MPIDU_SELECTION_create_coll_tree_shm_algorithms_default(storage, shm_subtree,
                                                                        coll_id);
            }
            MPIDU_SELECTION_tree_free_json_node(coll_json);
        } else {
            MPIDU_SELECTION_create_coll_tree_shm_algorithms_default(storage, shm_subtree, coll_id);
        }
    }
#endif
    /* Build selection trees for flat and inter comms from JSON nodes */
    MPIDU_SELECTION_build_selection_tree_default_inter(storage, inter_comm_subtree);
    MPIDU_SELECTION_build_selection_tree_default_flat(storage, flat_comm_subtree);

    return root;
}

/* Search for a collective with 'coll_id' in 'json_node_in' subtree and
 * create separate 'json_node_out' json subtree for that collective */
int MPIDU_SELECTION_tree_is_coll_in_json(MPIDU_SELECTION_json_node_t
                                         json_node_in,
                                         MPIDU_SELECTION_json_node_t * json_node_out, int coll_id)
{
    int length = 0;
    int index = 0;
    char key_str[MPIDU_SELECTION_BUFFER_MAX_SIZE];

    if (json_node_in == MPIDU_SELECTION_NULL_TYPE) {
        return 0;
    }

    length = MPIDU_SELECTION_tree_get_node_json_peer_count(json_node_in);
    for (index = 0; index < length; index++) {
        MPIR_Assert(MPIDU_SELECTION_tree_get_node_type(json_node_in, 0) ==
                    MPIDU_SELECTION_tree_get_node_type(json_node_in, index));

        MPIDU_SELECTION_tree_get_node_key(json_node_in, index, key_str);

        if (MPIDU_SELECTION_tree_get_node_type(json_node_in, 0) == MPIDU_SELECTION_COLLECTIVE) {
            if (strcasecmp(MPIDU_SELECTION_coll_names[coll_id], key_str) == 0) {
                char *key = NULL;
                MPIDU_SELECTION_json_node_t value = MPIDU_SELECTION_NULL_TYPE;

                key = MPIDU_SELECTION_tree_get_node_json_name(json_node_in, index);
                value = MPIDU_SELECTION_tree_get_node_json_value(json_node_in, index);
                MPIDU_SELECTION_tree_fill_json_node(*json_node_out, key, value);
                return 1;
            }
        }
    }
    return 0;
}

void MPIDU_SELECTION_tree_init_json_node(MPIDU_SELECTION_json_node_t * json_node)
{
    *json_node = json_object_new_object();
}

int MPIDU_SELECTION_tree_free_json_node(MPIDU_SELECTION_json_node_t json_node)
{
    int res = 0;
    res = json_object_put(json_node);
    return res;
}

int MPIDU_SELECTION_tree_fill_json_node(MPIDU_SELECTION_json_node_t json_node,
                                        char *key, MPIDU_SELECTION_json_node_t value)
{
    int res = 0;
    res = json_object_object_add(json_node, key, value);
    return res;
}

/* Takes a character string and provides it to the json object parser to be parsed and used by
 * subsequent json calls. */
MPIDU_SELECTION_json_node_t MPIDU_SELECTION_tree_text_to_json(char *contents)
{
    MPIDU_SELECTION_json_node_t root = MPIDU_SELECTION_NULL_TYPE;
    root = json_tokener_parse(contents);
    return root;
}

/* Get the name of the json node specified by the index number. */
char *MPIDU_SELECTION_tree_get_node_json_name(MPIDU_SELECTION_json_node_t json_node, int index)
{
    struct json_object_iter iter;
    int count = 0;

    json_object_object_foreachC(json_node, iter) {
        if (count == index)
            return (char *) lh_entry_k(iter.entry);
        count++;
    }

    return NULL;
}

/* Get the object stored in the json node specified by the index. */
MPIDU_SELECTION_json_node_t
MPIDU_SELECTION_tree_get_node_json_value(MPIDU_SELECTION_json_node_t json_node, int index)
{
    struct json_object_iter iter;
    int count = 0;

    json_object_object_foreachC(json_node, iter) {
        if (count == index)
            return (struct json_object *) lh_entry_v(iter.entry);
        count++;
    }

    return NULL;
}

/* Get the object stored in the json node specified by the index. */
MPIDU_SELECTION_json_node_t MPIDU_SELECTION_tree_get_node_json_next(MPIDU_SELECTION_json_node_t
                                                                    json_node, int index)
{
    struct json_object_iter iter;
    int count = 0;

    json_object_object_foreachC(json_node, iter) {
        if (count == index)
            return (struct json_object *) lh_entry_v(iter.entry);
        count++;
    }

    return MPIDU_SELECTION_NULL_TYPE;
}

/* Get the type of the node specified by json_node. */
int MPIDU_SELECTION_tree_get_node_json_type(MPIDU_SELECTION_json_node_t json_node)
{
    return json_node->o_type;
}

/* Get the number of peers available from the specified json_node. */
int MPIDU_SELECTION_tree_get_node_json_peer_count(MPIDU_SELECTION_json_node_t json_node)
{
    if (json_node == MPIDU_SELECTION_NULL_TYPE) {
        return 0;
    } else {
        return json_node->o.c_object->count;
    }
}

/* Read the json objects specified by the filename provided and parse them into the collective
 * selection tree format. */
MPIDU_SELECTION_json_node_t MPIDU_SELECTION_tree_read(const char *file)
{
    MPIDU_SELECTION_json_node_t root = MPIDU_SELECTION_NULL_TYPE;
    FILE *fp = NULL;
    struct stat filestat;
    int size = 0;
    char *contents = NULL;

    if (file == NULL) {
        goto fn_exit;
    }

    if (stat(file, &filestat) != 0) {
        MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE, (MPL_DBG_FDEST, "File %s not found\n", file));
        goto fn_exit;
    }

    size = filestat.st_size;
    contents = (char *) MPL_malloc(filestat.st_size, MPL_MEM_OTHER);
    if (contents == NULL) {
        MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                        (MPL_DBG_FDEST, "Unable to allocate %d bytes\n", size));
        goto fn_exit;
    }

    fp = fopen(file, "rt");
    if (fp == NULL) {
        MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE, (MPL_DBG_FDEST, "Unable to open %s\n", file));
        goto fn_exit;
    }

    size_t left_to_read = size;
    size_t completed = 0;

    while (left_to_read) {
        completed = fread((char *) contents + (size - left_to_read), 1, left_to_read, fp);

        if (completed == 0) {
            break;
        }

        left_to_read -= completed;
    }

    if (left_to_read != 0) {
        MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                        (MPL_DBG_FDEST, "Unable to read content of %s\n", file));
        fclose(fp);
        goto fn_exit;
    }
    fclose(fp);

    root = MPIDU_SELECTION_tree_text_to_json(contents);
    if (root == MPIDU_SELECTION_NULL_TYPE) {
        MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                        (MPL_DBG_FDEST, "Unable to load tuning settings\n"));
        goto fn_exit;
    }

  fn_exit:
    if (contents)
        MPL_free(contents);

    return root;
}

/* Construct internal representation of the collective selection tree from the json format. */
void MPIDU_SELECTION_json_to_selection_tree(MPIDU_SELECTION_storage_handler * storage,
                                            MPIDU_SELECTION_json_node_t json_node,
                                            MPIDU_SELECTION_storage_entry node)
{
    if (json_node == MPIDU_SELECTION_NULL_TYPE) {
        return;
    }
    switch (MPIDU_SELECTION_tree_get_node_json_type(json_node)) {
        case MPIDU_SELECTION_OBJECT_TYPE:
            MPIDU_SELECTION_tree_handle_object(storage, json_node, node);
            break;
        case MPIDU_SELECTION_NULL_TYPE:
            MPL_FALLTHROUGH;
        case MPIDU_SELECTION_ARRAY_TYPE:
            MPL_FALLTHROUGH;
        case MPIDU_SELECTION_INT_TYPE:
            MPL_FALLTHROUGH;
        case MPIDU_SELECTION_DOUBLE_TYPE:
            MPL_FALLTHROUGH;
        case MPIDU_SELECTION_STR_TYPE:
            MPL_FALLTHROUGH;
        case MPIDU_SELECTION_BOOL_TYPE:
            MPL_FALLTHROUGH;
        default:
            MPIR_Assert(0);
            break;
    }
}

/* Compare the type of algorithm from the input string and return the internal value to represent
 * it. */
int MPIDU_SELECTION_tree_get_algorithm_type(char *str)
{
    if (strstr(str, MPIDU_SELECTION_COMPOSITION_TOKEN)) {
        return MPIDU_SELECTION_COMPOSITION;
    } else if (strstr(str, MPIDU_SELECTION_NETMOD_TOKEN)) {
        return MPIDU_SELECTION_NETMOD;
    } else if (strstr(str, MPIDU_SELECTION_SHM_TOKEN)) {
        return MPIDU_SELECTION_SHM;
    } else {
        return -1;
    }
}

/* Compare the type of json node from the input object and return the internal value to represent
 * it. */
int MPIDU_SELECTION_tree_get_node_type(MPIDU_SELECTION_json_node_t json_node, int ind)
{
    if (strstr
        (MPIDU_SELECTION_tree_get_node_json_name(json_node, ind), MPIDU_SELECTION_COLL_KEY_TOKEN)) {
        return MPIDU_SELECTION_COLLECTIVE;
    } else
        if (strstr
            (MPIDU_SELECTION_tree_get_node_json_name(json_node, ind),
             MPIDU_SELECTION_COMMSIZE_KEY_TOKEN)) {
        return MPIDU_SELECTION_COMMSIZE;
    } else
        if (strstr
            (MPIDU_SELECTION_tree_get_node_json_name(json_node, ind),
             MPIDU_SELECTION_MSGSIZE_KEY_TOKEN)) {
        return MPIDU_SELECTION_MSGSIZE;
    } else {
        return MPIDU_SELECTION_CONTAINER;
    }
}

/* Compare the name of json node from the input object and return the internal value to represent
 * it. */
void MPIDU_SELECTION_tree_get_node_key(MPIDU_SELECTION_json_node_t json_node, int ind, char *key)
{
    if (strstr
        (MPIDU_SELECTION_tree_get_node_json_name(json_node, ind), MPIDU_SELECTION_COLL_KEY_TOKEN)) {
        MPL_strncpy(key,
                    MPIDU_SELECTION_tree_get_node_json_name(json_node,
                                                            ind) +
                    strlen(MPIDU_SELECTION_COLL_KEY_TOKEN), MPIDU_SELECTION_BUFFER_MAX_SIZE);
    } else
        if (strstr
            (MPIDU_SELECTION_tree_get_node_json_name(json_node, ind),
             MPIDU_SELECTION_COMMSIZE_KEY_TOKEN)) {
        MPL_strncpy(key,
                    MPIDU_SELECTION_tree_get_node_json_name(json_node,
                                                            ind) +
                    strlen(MPIDU_SELECTION_COMMSIZE_KEY_TOKEN), MPIDU_SELECTION_BUFFER_MAX_SIZE);
    } else
        if (strstr
            (MPIDU_SELECTION_tree_get_node_json_name(json_node, ind),
             MPIDU_SELECTION_MSGSIZE_KEY_TOKEN)) {
        MPL_strncpy(key,
                    MPIDU_SELECTION_tree_get_node_json_name(json_node,
                                                            ind) +
                    strlen(MPIDU_SELECTION_MSGSIZE_KEY_TOKEN), MPIDU_SELECTION_BUFFER_MAX_SIZE);
    } else {
        return;
    }
}

/* Convert the input string to a numerical value that can be used for comparison. */
int MPIDU_SELECTION_tree_convert_key_str(char *key_str)
{
    if (strncmp(key_str, "pow2_", strlen("pow2_")) == 0) {
        char buff[MPIDU_SELECTION_BUFFER_MAX_SIZE] = { 0 };
        char *head = NULL;
        char *value = NULL;

        MPL_strncpy(buff, key_str, MPIDU_SELECTION_BUFFER_MAX_SIZE);

        head = strtok(buff, MPIDU_SELECTION_DELIMITER_SUFFIX);
        if (head != NULL)
            value = strtok(NULL, MPIDU_SELECTION_DELIMITER_SUFFIX);

        MPL_snprintf(key_str, MPIDU_SELECTION_BUFFER_MAX_SIZE, "%s%s",
                     MPIDU_SELECTION_POW2_TOKEN, value);
        return atoi(key_str);
    } else if (strncmp(key_str, "pow2", strlen("pow2")) == 0) {
        MPL_snprintf(key_str, MPIDU_SELECTION_BUFFER_MAX_SIZE, MPIDU_SELECTION_POW2_TOKEN);
        return atoi(key_str);
    } else {
        return atoi(key_str);
    }
}

/* Convert the integer value to a string. */
void MPIDU_SELECTION_tree_convert_key_int(int key_int, char *key_str, int *is_pow2)
{
    MPL_snprintf(key_str, MPIDU_SELECTION_BUFFER_MAX_SIZE, "%d", key_int);

    if (strncmp(key_str, MPIDU_SELECTION_POW2_TOKEN, strlen(MPIDU_SELECTION_POW2_TOKEN)) == 0) {
        *is_pow2 = 1;

        MPL_snprintf(key_str, MPIDU_SELECTION_BUFFER_MAX_SIZE, "%s",
                     key_str + strlen(MPIDU_SELECTION_POW2_TOKEN));
    }

}

/* Create internal representation of 'json_node' with parent 'node' to hierarchically organized
 * 'storage' trees and recursively invoke MPIDU_SELECTION_json_to_selection_tree to handle the rest
 * of json tree. In case when the current 'json_object' stands for terminal leaf
 * it adds all the dependent containers to the 'storage' leaf node */
void MPIDU_SELECTION_tree_handle_object(MPIDU_SELECTION_storage_handler * storage,
                                        MPIDU_SELECTION_json_node_t json_node,
                                        MPIDU_SELECTION_storage_entry node)
{
    int length = 0;
    int i = 0;
    char key_str[MPIDU_SELECTION_BUFFER_MAX_SIZE];
    MPIDU_SELECTION_storage_entry new_node = MPIDU_SELECTION_NULL_ENTRY;

    if (json_node == MPIDU_SELECTION_NULL_TYPE) {
        return;
    }

    length = MPIDU_SELECTION_tree_get_node_json_peer_count(json_node);
    for (i = 0; i < length; i++) {
        /* Node type should be one of the selection types */
        MPIR_Assert(MPIDU_SELECTION_tree_get_node_type(json_node, 0) ==
                    MPIDU_SELECTION_tree_get_node_type(json_node, i));
        /* Node should be of type collective, comm_size or mesg size */
        if (MPIDU_SELECTION_tree_get_node_type(json_node, 0) != MPIDU_SELECTION_CONTAINER) {
            MPIDU_SELECTION_tree_get_node_key(json_node, i, key_str);
            if (MPIDU_SELECTION_tree_get_node_type(json_node, 0) == MPIDU_SELECTION_COLLECTIVE) {
                int coll_id = 0;
                for (coll_id = 0; coll_id < MPIDU_SELECTION_COLLECTIVES_MAX; coll_id++) {
                    if (strcasecmp(MPIDU_SELECTION_coll_names[coll_id], key_str) == 0) {
                        MPIDU_SELECTION_tree_current_collective_id = coll_id;
                        break;
                    }
                }
                MPL_snprintf(key_str, MPIDU_SELECTION_BUFFER_MAX_SIZE, "%d", coll_id);
            }

            MPIR_Assert(MPIDU_SELECTION_tree_get_node_type
                        (MPIDU_SELECTION_tree_get_node_json_next(json_node, 0),
                         0) ==
                        MPIDU_SELECTION_tree_get_node_type
                        (MPIDU_SELECTION_tree_get_node_json_next(json_node, i), 0));

            /* Create a new node in selection tree for current JSON object. We pass in information
             * about parent, parent type, current node, current node type, next layer type and
             * number of children of each node */
            new_node =
                MPIDU_SELECTION_create_node(storage, node,
                                            MPIDU_SELECTION_tree_get_node_type
                                            (json_node, 0),
                                            MPIDU_SELECTION_tree_get_node_type
                                            (MPIDU_SELECTION_tree_get_node_json_next
                                             (json_node, 0), 0),
                                            MPIDU_SELECTION_tree_convert_key_str(key_str),
                                            MPIDU_SELECTION_tree_get_node_json_peer_count
                                            (MPIDU_SELECTION_tree_get_node_json_next
                                             (json_node, i)));

            /* Repeat the above steps for next JSON level */
            MPIDU_SELECTION_json_to_selection_tree(storage, MPIDU_SELECTION_tree_get_node_json_next
                                                   (json_node, i), new_node);

        } else {
            /* If the JSON object is of the last level, construct a leaf node which will have a
             * set of containers representing the composition.
             * MPIDU_SELECTION_MAX_CNT represents the maximum containers a leaf node can have.
             * For example, an alpha composition for a collective will have netmod level algorithms and
             * multiple shm level algorithms composed of different collectives. Each level of decision is
             * represented by a composition.  Fill the containers with any given composition from the JSON file.
             * Choose the default values for the rest of the containers.
             * MPIDU_SELECTION_MAX_CNT represents the maximum containers a leaf node can have */
            MPIDIG_coll_algo_generic_container_t cnt[MPIDU_SELECTION_MAX_CNT];
            int cnt_num = 0;

            MPIDU_SELECTION_tree_create_containers(json_node, &cnt_num, cnt,
                                                   MPIDU_SELECTION_tree_current_collective_id);
            MPIDU_SELECTION_create_leaf(storage, node, MPIDU_SELECTION_CONTAINER, cnt_num, cnt);
        }
    }
}

/* Parse json object 'json_node' which reflect a composition algorithm id
 * (like "composition=alpha") and calls correspondent callback 'create_containers'
 * to fill the array of dependent containers 'cnt' */
void MPIDU_SELECTION_tree_create_containers(MPIDU_SELECTION_json_node_t
                                            json_node, int *cnt_num,
                                            MPIDIG_coll_algo_generic_container_t * cnt, int coll_id)
{
    char *leaf_name = NULL;
    char *key = NULL;
    char *value = NULL;
    int algorithm_type = -1, i = 0;

    if (json_node == MPIDU_SELECTION_NULL_TYPE) {
        return;
    }

    leaf_name = MPL_strdup(MPIDU_SELECTION_tree_get_node_json_name(json_node, 0));

    char buff[MPIDU_SELECTION_BUFFER_MAX_SIZE] = { 0 };

    if (leaf_name) {

        MPL_strncpy(buff, leaf_name, MPIDU_SELECTION_BUFFER_MAX_SIZE);

        key = strtok(buff, MPIDU_SELECTION_DELIMITER_VALUE);
        value = strtok(NULL, MPIDU_SELECTION_DELIMITER_VALUE);
        algorithm_type = MPIDU_SELECTION_tree_get_algorithm_type(key);
        if ((coll_id < MPIDU_SELECTION_COLLECTIVES_MAX) &&
            (algorithm_type == MPIDU_SELECTION_COMPOSITION)) {
            MPIDI_CH4_container_parser(json_node, cnt_num, cnt, coll_id, value);
        } else if ((coll_id < MPIDU_SELECTION_COLLECTIVES_MAX) &&
                   (algorithm_type == MPIDU_SELECTION_NETMOD)) {
            MPIDI_NM_algorithm_parser(coll_id, cnt_num, cnt, value);
#ifndef MPIDI_CH4_DIRECT_NETMOD
        } else if ((coll_id < MPIDU_SELECTION_COLLECTIVES_MAX) &&
                   (algorithm_type == MPIDU_SELECTION_SHM)) {
            MPIDI_SHM_algorithm_parser(coll_id, cnt_num, cnt, value);
#endif
        }
        MPL_free(leaf_name);
    }
    for (i = *cnt_num; i < MPIDU_SELECTION_MAX_CNT; i++)
        cnt[i].id = -1;
    *cnt_num = MPIDU_SELECTION_MAX_CNT;
}

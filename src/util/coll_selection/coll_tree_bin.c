#ifndef MPIU_COLL_SELECTION_TREE_H_INCLUDED
#define MPIU_COLL_SELECTION_TREE_H_INCLUDED

#include <sys/stat.h>
#include "mpiimpl.h"
#include "coll_tree_bin_types.h"
#include "coll_tree_bin_from_cvars.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_TUNING_FILE
      category    : COLLECTIVE
      type        : string
      default     : NULL
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Defines the name of tuning file

    - name        : MPIR_CVAR_TUNING_JSON_FILE_DUMP
      category    : COLLECTIVE
      type        : string
      default     : NULL
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Defines the name of tuning file to dump

    - name        : MPIR_CVAR_TUNING_BIN_FILE_DUMP
      category    : COLLECTIVE
      type        : string
      default     : NULL
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Defines the name of binary tuning file to dump

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

/* *INDENT-OFF* */
MPIU_COLL_SELECTION_create_coll_tree_cb
    coll_inter_compositions[MPIU_COLL_SELECTION_COLLECTIVES_NUMBER] = {
        MPIU_COLL_SELECTION_create_coll_tree_inter_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_inter_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_inter_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_inter_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_inter_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_inter_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_inter_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_inter_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_inter_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_inter_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_inter_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_inter_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_inter_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_inter_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_inter_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_inter_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_inter_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_inter_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_inter_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_inter_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_inter_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_inter_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_inter_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_inter_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_inter_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_inter_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_inter_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_inter_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_inter_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_inter_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_inter_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_inter_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_inter_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_inter_compositions_default};

MPIU_COLL_SELECTION_create_coll_tree_cb
    coll_topo_aware_compositions[MPIU_COLL_SELECTION_COLLECTIVES_NUMBER] = {
        MPIU_COLL_SELECTION_create_coll_tree_topo_aware_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_topo_aware_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_topo_aware_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_topo_aware_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_topo_aware_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_topo_aware_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_topo_aware_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_topo_aware_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_topo_aware_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_topo_aware_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_topo_aware_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_topo_aware_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_topo_aware_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_topo_aware_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_topo_aware_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_topo_aware_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_topo_aware_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_topo_aware_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_topo_aware_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_topo_aware_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_topo_aware_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_topo_aware_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_topo_aware_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_topo_aware_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_topo_aware_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_topo_aware_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_topo_aware_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_topo_aware_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_topo_aware_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_topo_aware_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_topo_aware_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_topo_aware_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_topo_aware_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_topo_aware_compositions_default};

MPIU_COLL_SELECTION_create_coll_tree_cb
    coll_flat_compositions[MPIU_COLL_SELECTION_COLLECTIVES_NUMBER] = {
        MPIU_COLL_SELECTION_create_coll_tree_flat_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_flat_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_flat_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_flat_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_flat_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_flat_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_flat_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_flat_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_flat_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_flat_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_flat_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_flat_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_flat_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_flat_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_flat_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_flat_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_flat_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_flat_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_flat_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_flat_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_flat_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_flat_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_flat_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_flat_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_flat_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_flat_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_flat_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_flat_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_flat_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_flat_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_flat_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_flat_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_flat_compositions_default,
        MPIU_COLL_SELECTION_create_coll_tree_flat_compositions_default};

/* *INDENT-ON* */

MPIU_COLL_SELECTION_storage_handler MPIU_COLL_SELECTION_tree_current_offset = 0;
char MPIU_COLL_SELECTION_offset_tree[MPIU_COLL_SELECTION_STORAGE_SIZE];

MPIU_COLL_SELECTION_storage_handler
MPIU_COLL_SELECTION_create_node(MPIU_COLL_SELECTION_storage_handler parent,
                                MPIU_COLL_SELECTION_node_type_t node_type,
                                MPIU_COLL_SELECTION_node_type_t next_layer_type,
                                int node_key, int children_count)
{
    int node_offset = 0;
    int child_index = 0;
    MPIU_COLL_SELECTION_storage_handler tmp = MPIU_COLL_SELECTION_tree_current_offset;

    node_offset =
        sizeof(MPIU_COLL_SELECTION_tree_node_t) +
        sizeof(MPIU_COLL_SELECTION_storage_handler) * children_count;
    if (parent != MPIU_COLL_SELECTION_NULL_ENTRY) {
        child_index = MPIU_COLL_SELECTION_NODE_FIELD(parent, cur_child_idx);
        MPIU_COLL_SELECTION_NODE_FIELD(parent, offset[child_index]) = tmp;
        MPIU_COLL_SELECTION_NODE_FIELD(parent, cur_child_idx)++;
    }

    MPIU_COLL_SELECTION_tree_current_offset += node_offset;
    MPIR_Assertp(MPIU_COLL_SELECTION_tree_current_offset <= MPIU_COLL_SELECTION_STORAGE_SIZE);

    MPIU_COLL_SELECTION_NODE_FIELD(tmp, parent) = parent;
    MPIU_COLL_SELECTION_NODE_FIELD(tmp, cur_child_idx) = 0;
    MPIU_COLL_SELECTION_NODE_FIELD(tmp, type) = node_type;
    MPIU_COLL_SELECTION_NODE_FIELD(tmp, next_layer_type) = next_layer_type;
    MPIU_COLL_SELECTION_NODE_FIELD(tmp, key) = node_key;
    MPIU_COLL_SELECTION_NODE_FIELD(tmp, children_count) = children_count;

    return tmp;
}

MPIU_COLL_SELECTION_storage_handler
MPIU_COLL_SELECTION_create_leaf(MPIU_COLL_SELECTION_storage_handler parent, int node_type,
                                int containers_count, void *containers)
{
    int node_offset = 0;
    MPIU_COLL_SELECTION_storage_handler tmp = MPIU_COLL_SELECTION_tree_current_offset;

    node_offset =
        sizeof(MPIU_COLL_SELECTION_tree_node_t) +
        sizeof(MPIDIG_coll_algo_generic_container_t) * containers_count;
    if (parent != MPIU_COLL_SELECTION_NULL_ENTRY)
        MPIU_COLL_SELECTION_NODE_FIELD(parent, offset[0]) = tmp;
    memset(MPIU_COLL_SELECTION_HANDLER_TO_POINTER(tmp), 0, node_offset);
    MPIU_COLL_SELECTION_tree_current_offset += node_offset;

    MPIU_COLL_SELECTION_NODE_FIELD(tmp, parent) = parent;
    MPIU_COLL_SELECTION_NODE_FIELD(tmp, type) = node_type;
    MPIU_COLL_SELECTION_NODE_FIELD(tmp, children_count) = containers_count;
    if (containers != NULL) {
        MPIR_Memcpy((void *) MPIU_COLL_SELECTION_NODE_FIELD(tmp, containers), containers,
                    sizeof(MPIDIG_coll_algo_generic_container_t) * containers_count);
    }

    return tmp;
}

void *MPIU_COLL_SELECTION_get_container(MPIU_COLL_SELECTION_storage_handler node)
{
    if ((MPIU_COLL_SELECTION_NODE_FIELD(node, type) == MPIU_COLL_SELECTION_CONTAINER) &&
        MPIU_COLL_SELECTION_NODE_FIELD(node, containers) != NULL) {
        return (void *) MPIU_COLL_SELECTION_NODE_FIELD(node, containers);
    } else {
        return NULL;
    }
}

MPIU_COLL_SELECTION_storage_handler
MPIU_COLL_SELECTION_get_node_parent(MPIU_COLL_SELECTION_storage_handler node)
{
    return MPIU_COLL_SELECTION_NODE_FIELD(node, parent);
}

int MPIU_COLL_SELECTION_dump()
{
    int mpi_errno = MPI_SUCCESS;
    FILE *fp = NULL;

    if (MPIR_CVAR_TUNING_BIN_FILE_DUMP) {
        if (MPIR_Process.comm_world->rank == 0) {
            if ((fp = fopen(MPIR_CVAR_TUNING_BIN_FILE_DUMP, "wb")) == NULL) {
                MPL_DBG_MSG_FMT(MPIDI_DBG_COLL, VERBOSE,
                                (MPL_DBG_FDEST, "Unable to open %s\n",
                                 MPIR_CVAR_TUNING_BIN_FILE_DUMP));
                mpi_errno = MPI_ERR_OTHER;
            } else {
                size_t size = MPIU_COLL_SELECTION_tree_current_offset;
                size_t left_to_write = size;
                size_t completed = 0;

                while (left_to_write) {
                    completed =
                        fwrite((char *) MPIU_COLL_SELECTION_offset_tree + (size - left_to_write), 1,
                               left_to_write, fp);

                    if (completed == 0) {
                        break;
                    }

                    left_to_write -= completed;
                }

                if (left_to_write != 0) {
                    MPL_DBG_MSG_FMT(MPIDI_DBG_COLL, VERBOSE,
                                    (MPL_DBG_FDEST, "Unable to write %s\n",
                                     MPIR_CVAR_TUNING_BIN_FILE_DUMP));
                }

                fclose(fp);
            }
        }
    }
    return mpi_errno;
}

int MPIU_COLL_SELECTION_init()
{
    int mpi_errno = MPI_SUCCESS;

    MPIDI_CH4_Global.coll_selection = MPIU_COLL_SELECTION_tree_load(MPIR_CVAR_TUNING_FILE);

    if (MPIDI_CH4_Global.coll_selection == MPIU_COLL_SELECTION_NULL_ENTRY) {
        mpi_errno = MPI_ERR_OTHER;
    }

    return mpi_errno;
}

void MPIU_COLL_SELECTION_build_bin_tree_generic_part(MPIU_COLL_SELECTION_storage_handler *
                                                     root,
                                                     MPIU_COLL_SELECTION_storage_handler *
                                                     inter_comm_subtree,
                                                     MPIU_COLL_SELECTION_storage_handler *
                                                     topo_aware_comm_subtree,
                                                     MPIU_COLL_SELECTION_storage_handler *
                                                     flat_comm_subtree)
{
    MPIU_COLL_SELECTION_storage_handler intra_comm_subtree = MPIU_COLL_SELECTION_NULL_ENTRY;
    MPIU_COLL_SELECTION_storage_handler composition = MPIU_COLL_SELECTION_NULL_ENTRY;

    *root =
        MPIU_COLL_SELECTION_create_node(*root,
                                        MPIU_COLL_SELECTION_DEFAULT_NODE_TYPE,
                                        MPIU_COLL_SELECTION_STORAGE, -1, 1);

    composition =
        MPIU_COLL_SELECTION_create_node(*root,
                                        MPIU_COLL_SELECTION_STORAGE,
                                        MPIU_COLL_SELECTION_COMM_KIND,
                                        MPIU_COLL_SELECTION_COMPOSITION,
                                        MPIU_COLL_SELECTION_COMM_KIND_NUM);
    *inter_comm_subtree =
        MPIU_COLL_SELECTION_create_node(composition, MPIU_COLL_SELECTION_COMM_KIND,
                                        MPIU_COLL_SELECTION_COLLECTIVE,
                                        MPIU_COLL_SELECTION_INTER_COMM,
                                        MPIU_COLL_SELECTION_COLLECTIVES_NUMBER);
    intra_comm_subtree =
        MPIU_COLL_SELECTION_create_node(composition, MPIU_COLL_SELECTION_COMM_KIND,
                                        MPIU_COLL_SELECTION_COMM_HIERARCHY,
                                        MPIU_COLL_SELECTION_INTRA_COMM,
                                        MPIU_COLL_SELECTION_COMM_HIERARCHY_NUM);
    *topo_aware_comm_subtree =
        MPIU_COLL_SELECTION_create_node(intra_comm_subtree, MPIU_COLL_SELECTION_COMM_HIERARCHY,
                                        MPIU_COLL_SELECTION_COLLECTIVE,
                                        MPIU_COLL_SELECTION_TOPO_COMM,
                                        MPIU_COLL_SELECTION_COLLECTIVES_NUMBER);
    *flat_comm_subtree =
        MPIU_COLL_SELECTION_create_node(intra_comm_subtree, MPIU_COLL_SELECTION_COMM_HIERARCHY,
                                        MPIU_COLL_SELECTION_COLLECTIVE,
                                        MPIU_COLL_SELECTION_FLAT_COMM,
                                        MPIU_COLL_SELECTION_COLLECTIVES_NUMBER);
}

void MPIU_COLL_SELECTION_build_bin_tree_default_inter(MPIU_COLL_SELECTION_storage_handler
                                                      inter_comm_subtree)
{
    int coll_id = 0;

    for (coll_id = 0; coll_id < MPIU_COLL_SELECTION_COLLECTIVES_NUMBER; coll_id++) {
        coll_inter_compositions[coll_id] (inter_comm_subtree, coll_id);
    }
}

void MPIU_COLL_SELECTION_build_bin_tree_default_topo_aware(MPIU_COLL_SELECTION_storage_handler
                                                           topo_aware_comm_subtree)
{
    int coll_id = 0;

    for (coll_id = 0; coll_id < MPIU_COLL_SELECTION_COLLECTIVES_NUMBER; coll_id++) {
        coll_topo_aware_compositions[coll_id] (topo_aware_comm_subtree, coll_id);
    }
}

void MPIU_COLL_SELECTION_build_bin_tree_default_flat(MPIU_COLL_SELECTION_storage_handler
                                                     flat_comm_subtree)
{
    int coll_id = 0;

    for (coll_id = 0; coll_id < MPIU_COLL_SELECTION_COLLECTIVES_NUMBER; coll_id++) {
        coll_flat_compositions[coll_id] (flat_comm_subtree, coll_id);
    }
}

MPIU_COLL_SELECTION_storage_handler MPIU_COLL_SELECTION_tree_load(char *filename)
{
    MPIU_COLL_SELECTION_storage_handler root = MPIU_COLL_SELECTION_NULL_ENTRY;

    if (filename) {
        FILE *fp = NULL;
        struct stat filestat;
        int size = 0;
        if (stat(filename, &filestat) == 0) {
            size = filestat.st_size;

            if ((fp = fopen(filename, "rb")) != NULL) {
                size_t left_to_read = size;
                size_t completed = 0;

                while (left_to_read) {
                    completed =
                        fread((char *) MPIU_COLL_SELECTION_offset_tree + (size - left_to_read), 1,
                              left_to_read, fp);

                    if (completed == 0) {
                        break;
                    }

                    left_to_read -= completed;
                }

                if (left_to_read == 0) {
                    root = (MPIU_COLL_SELECTION_storage_handler) 0;
                } else {
                    MPL_DBG_MSG_FMT(MPIDI_DBG_COLL, VERBOSE,
                                    (MPL_DBG_FDEST, "Unable to read %s\n", filename));
                }

                fclose(fp);
            } else {
                MPL_DBG_MSG_FMT(MPIDI_DBG_COLL, VERBOSE,
                                (MPL_DBG_FDEST, "Unable to open %s\n", filename));
            }
        } else {
            MPL_DBG_MSG_FMT(MPIDI_DBG_COLL, VERBOSE,
                            (MPL_DBG_FDEST, "File %s not found\n", filename));
        }
    }
    if (root == MPIU_COLL_SELECTION_NULL_ENTRY) {
        MPIU_COLL_SELECTION_storage_handler inter_comm_subtree = MPIU_COLL_SELECTION_NULL_ENTRY;
        MPIU_COLL_SELECTION_storage_handler intra_comm_subtree = MPIU_COLL_SELECTION_NULL_ENTRY;
        MPIU_COLL_SELECTION_storage_handler topo_aware_comm_subtree =
            MPIU_COLL_SELECTION_NULL_ENTRY;
        MPIU_COLL_SELECTION_storage_handler flat_comm_subtree = MPIU_COLL_SELECTION_NULL_ENTRY;

        /***************************************************************************/
        /*               Build generic part of binary selection tree               */
        /***************************************************************************/
        MPIU_COLL_SELECTION_build_bin_tree_generic_part(&root,
                                                        &inter_comm_subtree,
                                                        &topo_aware_comm_subtree,
                                                        &flat_comm_subtree);

        /***************************************************************************/
        /*               Build json to binary trees for topo aware comm            */
        /***************************************************************************/
        MPIU_COLL_SELECTION_build_bin_tree_default_topo_aware(topo_aware_comm_subtree);

        /***************************************************************************/
        /*               Build json to binary trees for flat and inter comms       */
        /***************************************************************************/
        MPIU_COLL_SELECTION_build_bin_tree_default_inter(inter_comm_subtree);
        MPIU_COLL_SELECTION_build_bin_tree_default_flat(flat_comm_subtree);
    }

    return root;
}
#endif /* MPIU_COLL_SELECTION_TREE_H_INCLUDED */

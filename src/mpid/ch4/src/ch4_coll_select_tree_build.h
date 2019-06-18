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

#ifndef CH4_COLL_SELECT_TREE_BUILD_H_INCLUDED
#define CH4_COLL_SELECT_TREE_BUILD_H_INCLUDED

#include "ch4_coll_select_tree_types.h"

/* Fix this */
extern MPIDU_SELECTION_storage_handler MPIDU_SELECTION_tree_global_storage;
extern char MPIDU_SELECTION_offset_tree[];

extern void MPIDI_CH4_container_parser(MPIDU_SELECTION_json_node_t json_node, int *cnt_num,
                                       MPIDIG_coll_algo_generic_container_t * cnt, int coll_id,
                                       char *value);

MPIDU_SELECTION_storage_entry MPIDU_SELECTION_tree_load(MPIDU_SELECTION_storage_handler * storage);

int MPIDU_SELECTION_init(void);

void MPIDU_SELECTION_build_selection_tree_generic_part(MPIDU_SELECTION_storage_handler * storage,
                                                       MPIDU_SELECTION_storage_entry * root,
                                                       MPIDU_SELECTION_storage_entry *
                                                       generic_subtree,
                                                       MPIDU_SELECTION_storage_entry *
                                                       netmod_subtree,
                                                       MPIDU_SELECTION_storage_entry * shm_subtree,
                                                       MPIDU_SELECTION_storage_entry *
                                                       inter_comm_subtree,
                                                       MPIDU_SELECTION_storage_entry *
                                                       topo_aware_comm_subtree,
                                                       MPIDU_SELECTION_storage_entry *
                                                       flat_comm_subtree);

void MPIDU_SELECTION_build_selection_tree_default_inter(MPIDU_SELECTION_storage_handler * storage,
                                                        MPIDU_SELECTION_storage_entry
                                                        inter_comm_subtree);

void
MPIDU_SELECTION_create_coll_tree_inter_compositions_default(MPIDU_SELECTION_storage_handler *
                                                            storage,
                                                            MPIDU_SELECTION_storage_entry parent,
                                                            int coll_id);

void
MPIDU_SELECTION_build_selection_tree_default_topo_aware(MPIDU_SELECTION_storage_handler * storage,
                                                        MPIDU_SELECTION_storage_entry
                                                        topo_aware_comm_subtree);

void
MPIDU_SELECTION_create_coll_tree_topo_aware_compositions_default(MPIDU_SELECTION_storage_handler *
                                                                 storage,
                                                                 MPIDU_SELECTION_storage_entry
                                                                 parent, int coll_id);

void MPIDU_SELECTION_build_selection_tree_default_flat(MPIDU_SELECTION_storage_handler * storage,
                                                       MPIDU_SELECTION_storage_entry
                                                       flat_comm_subtree);
void
MPIDU_SELECTION_create_coll_tree_flat_compositions_default(MPIDU_SELECTION_storage_handler *
                                                           storage,
                                                           MPIDU_SELECTION_storage_entry parent,
                                                           int coll_id);

void
MPIDU_SELECTION_create_coll_tree_netmod_algorithms_default(MPIDU_SELECTION_storage_handler *
                                                           storage,
                                                           MPIDU_SELECTION_storage_entry parent,
                                                           int coll_id);
#ifndef MPIDI_CH4_DIRECT_NETMOD
void MPIDU_SELECTION_create_coll_tree_shm_algorithms_default(MPIDU_SELECTION_storage_handler *
                                                             storage,
                                                             MPIDU_SELECTION_storage_entry parent,
                                                             int coll_id);
#endif

void MPIDU_SELECTION_build_bin_tree_default_flat(MPIDU_SELECTION_storage_handler * storage,
                                                 MPIDU_SELECTION_storage_entry flat_comm_subtree);
MPIDU_SELECTION_json_node_t MPIDU_SELECTION_tree_text_to_json(char *contents);
char *MPIDU_SELECTION_tree_get_node_json_name(MPIDU_SELECTION_json_node_t json_node, int index);

MPIDU_SELECTION_json_node_t
MPIDU_SELECTION_tree_get_node_json_value(MPIDU_SELECTION_json_node_t json_node, int index);
MPIDU_SELECTION_json_node_t
MPIDU_SELECTION_tree_get_node_json_next(MPIDU_SELECTION_json_node_t json_node, int index);
int MPIDU_SELECTION_tree_get_node_json_type(MPIDU_SELECTION_json_node_t json_node);
int MPIDU_SELECTION_tree_get_node_json_peer_count(MPIDU_SELECTION_json_node_t json_node);
MPIDU_SELECTION_json_node_t MPIDU_SELECTION_tree_read(const char *file);
MPIDU_SELECTION_storage_entry
MPIDU_SELECTION_create_node(MPIDU_SELECTION_storage_handler * storage,
                            MPIDU_SELECTION_storage_entry parent,
                            MPIDU_SELECTION_node_type_t node_type,
                            MPIDU_SELECTION_node_type_t next_layer_type,
                            int node_key, int children_count);

void MPIDU_SELECTION_create_leaf(MPIDU_SELECTION_storage_handler * storage,
                                 MPIDU_SELECTION_storage_entry parent,
                                 int node_type, int containers_count, void *containers);
void MPIDU_SELECTION_json_to_selection_tree(MPIDU_SELECTION_storage_handler * storage,
                                            MPIDU_SELECTION_json_node_t json_node,
                                            MPIDU_SELECTION_storage_entry bin_node);
int MPIDU_SELECTION_tree_get_node_type(MPIDU_SELECTION_json_node_t json_node, int ind);
void MPIDU_SELECTION_tree_get_node_key(MPIDU_SELECTION_json_node_t json_node, int ind, char *key);
void MPIDU_SELECTION_tree_handle_object(MPIDU_SELECTION_storage_handler * storage,
                                        MPIDU_SELECTION_json_node_t json_node,
                                        MPIDU_SELECTION_storage_entry node);
void MPIDU_SELECTION_tree_create_containers(MPIDU_SELECTION_json_node_t json_node, int *cnt_num,
                                            MPIDIG_coll_algo_generic_container_t * cnt,
                                            int coll_id);
int MPIDU_SELECTION_tree_get_algorithm_type(char *str);
void MPIDU_SELECTION_tree_convert_key_int(int key_int, char *key_str, int *is_pow2);
int MPIDU_SELECTION_tree_convert_key_str(char *key_str);
void MPIDU_SELECTION_tree_init_json_node(MPIDU_SELECTION_json_node_t * json_node);

int MPIDU_SELECTION_tree_free_json_node(MPIDU_SELECTION_json_node_t json_node);

int MPIDU_SELECTION_tree_fill_json_node(MPIDU_SELECTION_json_node_t json_node,
                                        char *key, MPIDU_SELECTION_json_node_t value);
int MPIDU_SELECTION_tree_is_coll_in_json(MPIDU_SELECTION_json_node_t
                                         json_node_in,
                                         MPIDU_SELECTION_json_node_t * json_node_out, int coll_id);
#endif /* CH4_COLL_SELECT_TREE_BUILD_H_INCLUDED */

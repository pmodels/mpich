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

#ifndef CH4_COLL_SELECT_TREE_TRAVERSAL_H_INCLUDED
#define CH4_COLL_SELECT_TREE_TRAVERSAL_H_INCLUDED


void *MPIDU_SELECTION_get_container(MPIDU_SELECTION_storage_handler * storage,
                                    MPIDU_SELECTION_storage_entry node);


void MPIDU_SELECTION_init_match_pattern(MPIDU_SELECTION_match_pattern_t * match_pattern);

MPIDU_SELECTION_storage_entry
MPIDU_SELECTION_find_entry(MPIDU_SELECTION_storage_handler * storage,
                           MPIDU_SELECTION_storage_entry entry,
                           MPIDU_SELECTION_match_pattern_t * match_pattern);

void
MPIDU_SELECTION_init_comm_match_pattern(MPIR_Comm * comm,
                                        MPIDU_SELECTION_match_pattern_t *
                                        match_pattern,
                                        MPIDU_SELECTION_node_type_t terminal_layer_type);

void
MPIDU_SELECTION_init_coll_match_pattern(MPIDU_SELECTON_coll_signature_t * coll_sig,
                                        MPIDU_SELECTION_match_pattern_t *
                                        match_pattern,
                                        MPIDU_SELECTION_node_type_t terminal_layer_type);
void
MPIDU_SELECTION_set_match_pattern_key(MPIDU_SELECTION_match_pattern_t *
                                      match_pattern, MPIDU_SELECTION_node_type_t layer_type,
                                      int key);
int MPIDU_SELECTION_get_match_pattern_key(MPIDU_SELECTION_match_pattern_t * match_pattern,
                                          MPIDU_SELECTION_node_type_t layer_type);

void
MPIDU_SELECTION_match_layer_and_key_to_str(char *layer_str, char *key_str,
                                           MPIDU_SELECTION_storage_handler * storage,
                                           MPIDU_SELECTION_storage_entry match_node);

#endif /* CH4_COLL_SELECT_TREE_TRAVERSAL_H_INCLUDED */

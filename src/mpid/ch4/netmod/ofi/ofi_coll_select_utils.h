/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2018 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef OFI_COLL_SELECT_UTILS_H_INCLUDED
#define OFI_COLL_SELECT_UTILS_H_INCLUDED

#include "ch4_coll_select_tree_types.h"
#include "ch4_coll_select_tree_traversal.h"
#include "ch4_coll_select_utils.h"

/* Get the default container for a particular collective signature. */
MPL_STATIC_INLINE_PREFIX const void *MPIDI_NM_get_default_container(MPIDU_SELECTION_coll_id_t
                                                                    coll_id)
{
    return MPIDI_OFI_coll_default_container[coll_id];
}

/* Check to see if the input container says to pick the algorithm automatically. If not, use the
 * provided container. Otherwise, find the default collective container and use that instead. */
MPL_STATIC_INLINE_PREFIX const MPIDI_OFI_coll_algo_container_t
    * MPIDI_OFI_coll_select(MPIDU_SELECTON_coll_signature_t * coll_sig,
                            const void *ch4_algo_parameters_container_in)
{
    const MPIDI_OFI_coll_algo_container_t *container = NULL;
    MPIDU_SELECTION_storage_handler *storage = NULL;
    MPIDU_SELECTION_storage_entry entry = MPIDU_SELECTION_NULL_ENTRY;
    MPIDU_SELECTION_match_pattern_t match_pattern;

    storage = MPIDI_get_container_storage(coll_sig);

    bool is_valid = false;

    if ((ch4_algo_parameters_container_in != NULL) &&
        ((const MPIDI_OFI_coll_algo_container_t *) ch4_algo_parameters_container_in)->id !=
        MPIDI_COLL_AUTO_SELECT) {
        container = (const MPIDI_OFI_coll_algo_container_t *) ch4_algo_parameters_container_in;
        is_valid = (container != NULL);
        if (!is_valid) {
            container = (const MPIDI_OFI_coll_algo_container_t *)
                MPIDI_NM_get_default_container(coll_sig->coll_id);
        }
    } else if (storage != NULL) {
        /* Initizlaize match pattern fields to default values and then set fileds like coll name and msg size, etc., and whatever applicable from coll_sig
         *          This function bascially initializes the match key from coll_sig*/
        MPIDU_SELECTION_init_match_pattern(&match_pattern);
        MPIDU_SELECTION_init_coll_match_pattern(coll_sig, &match_pattern,
                                                MPIDU_SELECTION_CONTAINER);
        MPIDU_SELECTION_set_match_pattern_key(&match_pattern, MPIDU_SELECTION_DIRECTORY,
                                              MPIDU_SELECTION_NETMOD);
        MPIDU_SELECTION_set_match_pattern_key(&match_pattern, MPIDU_SELECTION_COMMSIZE,
                                              MPIR_Comm_size(coll_sig->comm));
        entry = MPIDU_SELECTION_find_entry(storage, MPIDI_global.coll_selection, &match_pattern);

        if (entry != MPIDU_SELECTION_NULL_ENTRY) {
            container = MPIDU_SELECTION_get_container(storage, entry);
            is_valid = (container != NULL);
        }
    }

    if (!is_valid) {
        container = (const MPIDI_OFI_coll_algo_container_t *)
            MPIDI_NM_get_default_container(coll_sig->coll_id);
    }

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                    (MPL_DBG_FDEST, "OFI container_id: %d, is_default: %s\n",
                     container->id, (!is_valid) ? "TRUE" : "FALSE"));

    return container;
}

#endif /* OFI_COLL_SELECT_UTILS_H_INCLUDED */

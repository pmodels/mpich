/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2018 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef CH4_COLL_SELECT_UTILS_H_INCLUDED
#define CH4_COLL_SELECT_UTILS_H_INCLUDED

#include "ch4_coll_select_tree_build.h"
#include "ch4_coll_select_tree_traversal.h"


/* Get the node in the collective selection tree to start traversing for the given collective. */
MPL_STATIC_INLINE_PREFIX MPIDU_SELECTION_storage_handler
    * MPIDI_get_container_storage(MPIDU_SELECTON_coll_signature_t * coll_sig ATTRIBUTE((unused)))
{
    return &MPIDU_SELECTION_tree_global_storage;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_check_storage(MPIDU_SELECTION_storage_handler storage)
{
    if (storage.base_addr != NULL) {
        return 1;
    }
    return 0;
}

/* Get the default collective composiontion for the given collective. */
MPL_STATIC_INLINE_PREFIX const MPIDIG_coll_algo_generic_container_t
    * MPIDI_get_container_default_value(MPIDU_SELECTON_coll_signature_t * coll_sig)
{
    return (coll_sig->comm->comm_kind == MPIR_COMM_KIND__INTERCOMM) ?
        MPIDI_Coll_inter_composition_default_container[coll_sig->coll_id] :
        MPIDI_Coll_intra_composition_default_container[coll_sig->coll_id];
}

/* Traverse the collective selection tree and find the best algorithm. */
MPL_STATIC_INLINE_PREFIX const MPIDIG_coll_algo_generic_container_t
    * MPIDI_coll_select(MPIDU_SELECTON_coll_signature_t * coll_sig)
{
    const MPIDIG_coll_algo_generic_container_t *container = NULL;
    bool is_valid = false;
    MPIDU_SELECTION_storage_handler *storage = NULL;
    MPIDU_SELECTION_storage_entry entry = MPIDU_SELECTION_NULL_ENTRY;
    MPIDU_SELECTION_match_pattern_t match_pattern;

    storage = MPIDI_get_container_storage(coll_sig);

    if (storage != NULL) {
        /* Initizlaize match pattern fields to default values and then set fileds like coll name and msg size, etc., and whatever applicable from coll_sig
         *          This function bascially initializes the match key from coll_sig*/
        MPIDU_SELECTION_init_comm_match_pattern(coll_sig->comm, &match_pattern,
                                                MPIDU_SELECTION_COMMSIZE);
        MPIDU_SELECTION_set_match_pattern_key(&match_pattern, MPIDU_SELECTION_DIRECTORY,
                                              MPIDU_SELECTION_COMPOSITION);
        MPIDU_SELECTION_init_coll_match_pattern(coll_sig, &match_pattern,
                                                MPIDU_SELECTION_CONTAINER);

        entry = MPIDU_SELECTION_find_entry(storage, MPIDI_global.coll_selection, &match_pattern);

        if (entry != MPIDU_SELECTION_NULL_ENTRY) {
            container = MPIDU_SELECTION_get_container(storage, entry);
            is_valid = (container != NULL);
        }
    }

    if (!is_valid) {
        container = MPIDI_get_container_default_value(coll_sig);
    }

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                    (MPL_DBG_FDEST, "Composition container_id: %d, is_default: %s\n",
                     container->id, (!is_valid) ? "TRUE" : "FALSE"));

    return container;
}

#endif /* CH4_COLL_SELECT_UTILS_H_INCLUDED */

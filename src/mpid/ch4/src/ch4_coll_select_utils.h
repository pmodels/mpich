#ifndef CH4_COLL_SELECT_UTILS_H_INCLUDED
#define CH4_COLL_SELECT_UTILS_H_INCLUDED

#include "coll_tree_bin_types.h"

MPL_STATIC_INLINE_PREFIX MPIU_SELECTION_storage_handler
MPIDI_get_container_storage(MPIU_SELECTON_coll_signature_t * coll_sig)
{
    return (MPIDI_CH4_COMM(coll_sig->comm).coll_tuning)[coll_sig->coll_id];
}

MPL_STATIC_INLINE_PREFIX int
MPIDI_check_container(MPIU_SELECTON_coll_signature_t * coll_sig, void *container)
{
    if (container != NULL) {
        return 1;
    }
    return 0;
}

MPL_STATIC_INLINE_PREFIX const MPIDI_coll_algo_container_t
    * MPIDI_get_container_default_value(MPIU_SELECTON_coll_signature_t * coll_sig)
{
    return (coll_sig->comm->comm_kind == MPIR_COMM_KIND__INTERCOMM) ?
        MPIDI_Coll_inter_composition_default_cnt[coll_sig->coll_id] :
        MPIDI_Coll_intra_composition_default_cnt[coll_sig->coll_id];
}

MPL_STATIC_INLINE_PREFIX const MPIDI_coll_algo_container_t
    * MPIDI_coll_select(MPIU_SELECTON_coll_signature_t * coll_sig)
{
    void *container = NULL;
    int is_valid = 0;
    MPIU_SELECTION_storage_handler storage = MPIU_SELECTION_NULL_ENTRY;
    MPIU_SELECTION_storage_handler entry = MPIU_SELECTION_NULL_ENTRY;
    MPIU_SELECTION_match_pattern_t match_pattern;

    storage = MPIDI_get_container_storage(coll_sig);

    if (storage != MPIU_SELECTION_NULL_ENTRY) {
        MPIU_SELECTION_init_coll_match_pattern(coll_sig, &match_pattern, MPIU_SELECTION_CONTAINER);

        entry = MPIU_SELECTION_find_entry(storage, &match_pattern);

        if (entry != MPIU_SELECTION_NULL_ENTRY) {
            container = MPIU_SELECTION_get_container(entry);

            if (container != NULL) {
                is_valid = MPIDI_check_container(coll_sig, container);
            }
        }
    }

    if (!is_valid) {
        container = (void *) MPIDI_get_container_default_value(coll_sig);
    }

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL_SELECTION, VERBOSE,
                    (MPL_DBG_FDEST, "Composition container_id: %d, is_default: %s\n",
                     ((const MPIDI_coll_algo_container_t *) container)->id,
                     (!is_valid) ? "TRUE" : "FALSE"));

    return (const MPIDI_coll_algo_container_t *) container;
}

#endif /* CH4_COLL_SELECT_UTILS_H_INCLUDED */

#ifndef OFI_COLL_SELECT_UTILS_H_INCLUDED
#define OFI_COLL_SELECT_UTILS_H_INCLUDED

#include "coll_tree_bin_types.h"

MPL_STATIC_INLINE_PREFIX int
MPIDI_OFI_check_container(MPIU_COLL_SELECTON_coll_signature_t * coll_sig, const void *container)
{
    if (container != NULL) {
        return 1;
    }
    return 0;
}

MPL_STATIC_INLINE_PREFIX const MPIDI_OFI_coll_algo_container_t
    * MPIDI_OFI_get_container_default_value(MPIU_COLL_SELECTON_coll_signature_t * coll_sig)
{
    return MPIDI_OFI_Coll_default_cnt[coll_sig->coll_id];
}

MPL_STATIC_INLINE_PREFIX const MPIDI_OFI_coll_algo_container_t
    * MPIDI_OFI_coll_select(MPIU_COLL_SELECTON_coll_signature_t *
                            coll_sig, const void *ch4_algo_parameters_container_in)
{
    const void *container = NULL;
    int is_valid = 0;

    if ((ch4_algo_parameters_container_in != NULL) &&
        ((const MPIDI_OFI_coll_algo_container_t *) ch4_algo_parameters_container_in)->id !=
        MPIDI_COLL_AUTO_SELECT) {
        container = ch4_algo_parameters_container_in;
        is_valid = MPIDI_OFI_check_container(coll_sig, container);
        if (!is_valid) {
            container = (void *) MPIDI_OFI_get_container_default_value(coll_sig);
        }
    } else {
        container = (void *) MPIDI_OFI_get_container_default_value(coll_sig);
    }

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL_SELECTION, VERBOSE,
                    (MPL_DBG_FDEST, "OFI container_id: %d, is_default: %s\n",
                     ((const MPIDI_OFI_coll_algo_container_t *) container)->id,
                     (!is_valid) ? "TRUE" : "FALSE"));

    return (const MPIDI_OFI_coll_algo_container_t *) container;
}

#endif /* OFI_COLL_SELECT_UTILS_H_INCLUDED */

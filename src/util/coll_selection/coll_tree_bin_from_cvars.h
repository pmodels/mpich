#ifndef MPIU_SELECTION_TREE_FROM_CVARS_H_INCLUDED
#define MPIU_SELECTION_TREE_FROM_CVARS_H_INCLUDED

#include "coll_tree_bin_types.h"
#include "ch4_coll_containers.h"
#include "../../mpid/ch4/netmod/ofi/ofi_coll_containers.h"
#include "../../mpid/ch4/shm/posix/posix_coll_containers.h"

static MPIU_SELECTION_storage_entry
MPIU_SELECTION_create_coll_tree_inter_compositions_default(MPIU_SELECTION_storage_handler * storage,
                                                           MPIU_SELECTION_storage_entry parent,
                                                           int coll_id)
{
    MPIU_SELECTION_storage_entry coll = MPIU_SELECTION_NULL_ENTRY;
    MPIU_SELECTION_storage_entry algo = MPIU_SELECTION_NULL_ENTRY;

    coll = MPIU_SELECTION_create_node(storage, parent, MPIU_SELECTION_COLLECTIVE,
                                      MPIU_SELECTION_CONTAINER, coll_id, 1);

    algo = MPIU_SELECTION_create_leaf(storage, coll, MPIU_SELECTION_CONTAINER, 1, (void *)
                                      MPIDI_Coll_inter_composition_default_cnt[coll_id]);
    return coll;
}

static MPIU_SELECTION_storage_entry
    MPIU_SELECTION_create_coll_tree_topo_aware_compositions_default
    (MPIU_SELECTION_storage_handler * storage, MPIU_SELECTION_storage_entry parent, int coll_id) {
    MPIU_SELECTION_storage_entry coll = MPIU_SELECTION_NULL_ENTRY;
    MPIU_SELECTION_storage_entry algo = MPIU_SELECTION_NULL_ENTRY;

    coll = MPIU_SELECTION_create_node(storage, parent, MPIU_SELECTION_COLLECTIVE,
                                      MPIU_SELECTION_CONTAINER, coll_id, 1);

    algo = MPIU_SELECTION_create_leaf(storage, coll, MPIU_SELECTION_CONTAINER, 1, (void *)
                                      MPIDI_Coll_intra_composition_default_cnt[coll_id]);
    return coll;
}

static MPIU_SELECTION_storage_entry
MPIU_SELECTION_create_coll_tree_flat_compositions_default(MPIU_SELECTION_storage_handler * storage,
                                                          MPIU_SELECTION_storage_entry parent,
                                                          int coll_id)
{
    MPIU_SELECTION_storage_entry coll = MPIU_SELECTION_NULL_ENTRY;
    MPIU_SELECTION_storage_entry algo = MPIU_SELECTION_NULL_ENTRY;

    coll = MPIU_SELECTION_create_node(storage, parent, MPIU_SELECTION_COLLECTIVE,
                                      MPIU_SELECTION_CONTAINER, coll_id, 1);

    algo = MPIU_SELECTION_create_leaf(storage, coll, MPIU_SELECTION_CONTAINER, 1, (void *)
                                      MPIDI_Coll_intra_composition_default_cnt[coll_id]);
    return coll;
}

#endif /* MPIU_SELECTION_TREE_FROM_CVARS_H_INCLUDED */

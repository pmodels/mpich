#ifndef CH4_COLL_SELECT_INIT_H_INCLUDED
#define CH4_COLL_SELECT_INIT_H_INCLUDED

#include "ch4_coll_select_utils.h"
#include "coll_tree_bin_types.h"

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4_mpi_comm_collective_selection_init
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_CH4_mpi_comm_collective_selection_init(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;
    int coll_id = 0;
    MPIU_COLL_SELECTION_match_pattern_t match_pattern;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4_COMM_COLLECTIVE_SELECTION_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_COMM_CH4_COLLECTIVE_SELECTION_INIT);

    MPIDI_CH4_COMM(comm).coll_tuning =
        (MPIU_COLL_SELECTION_storage_handler *) MPL_malloc(MPIU_COLL_SELECTION_COLLECTIVES_NUMBER *
                                                           sizeof
                                                           (MPIU_COLL_SELECTION_storage_handler),
                                                           MPL_MEM_COMM);

    MPIU_COLL_SELECTION_init_comm_match_pattern(comm, &match_pattern, MPIU_COLL_SELECTION_COMMSIZE);
    MPIU_COLL_SELECTION_set_match_pattern_key(&match_pattern, MPIU_COLL_SELECTION_STORAGE,
                                              MPIU_COLL_SELECTION_COMPOSITION);

    for (coll_id = 0; coll_id < MPIU_COLL_SELECTION_COLLECTIVES_NUMBER; coll_id++) {
        MPIU_COLL_SELECTION_set_match_pattern_key(&match_pattern, MPIU_COLL_SELECTION_COLLECTIVE,
                                                  coll_id);
        MPIDI_CH4_COMM(comm).coll_tuning[coll_id] =
            MPIU_COLL_SELECTION_find_entry(MPIDI_CH4_Global.coll_selection, &match_pattern);
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_COMM_CH4_COLLECTIVE_SELECTION_INIT);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4_mpi_comm_collective_selection_finalize
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_CH4_mpi_comm_collective_selection_finalize(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4_COMM_COLLECTIVE_SELECTION_FINALIZE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_COMM_CH4_COLLECTIVE_SELECTION_FINALIZE);

    if (MPIDI_CH4_COMM(comm).coll_tuning) {
        MPL_free(MPIDI_CH4_COMM(comm).coll_tuning);
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_COMM_CH4_COLLECTIVE_SELECTION_FINALIZE);
    return mpi_errno;
}

#endif /* CH4_COLL_SELECT_INIT_H_INCLUDED */

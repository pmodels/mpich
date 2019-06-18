
/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */
#include "mpidimpl.h"
#include "shm_noinline.h"
#include "../posix/posix_noinline.h"

void MPIDI_SHMI_algorithm_parser(MPIDU_SELECTION_coll_id_t coll_id, int *cnt_num,
                                 MPIDIG_coll_algo_generic_container_t * cnt, char *value)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_ALGORITHM_PARSER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_ALGORITHM_PARSER);

    MPIDI_POSIX_algorithm_parser(coll_id, cnt_num, cnt, value);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_ALGORITHM_PARSER);
}

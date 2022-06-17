/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef CH4_COMM_H_INCLUDED
#define CH4_COMM_H_INCLUDED

#include "ch4_impl.h"

int MPIDI_Comm_split_type(MPIR_Comm * user_comm_ptr, int split_type, int key, MPIR_Info * info_ptr,
                          MPIR_Comm ** newcomm_ptr);

MPL_STATIC_INLINE_PREFIX int MPIDI_set_comm_hint_sender_vci(MPIR_Comm * comm, int type, int value);
MPL_STATIC_INLINE_PREFIX int MPIDI_set_comm_hint_receiver_vci(MPIR_Comm * comm, int type,
                                                              int value);
MPL_STATIC_INLINE_PREFIX int MPIDI_set_comm_hint_vci(MPIR_Comm * comm, int type, int value);
int MPIDI_Comm_create_multi_leaders(MPIR_Comm * comm);
int MPIDI_Comm_create_multi_leader_subcomms(MPIR_Comm * comm, int num_leads);

MPL_STATIC_INLINE_PREFIX int MPID_Comm_AS_enabled(MPIR_Comm * comm)
{
    MPIR_FUNC_ENTER;

    MPIR_Assert(0);

    MPIR_FUNC_EXIT;
    return MPI_SUCCESS;
}

#endif /* CH4_COMM_H_INCLUDED */

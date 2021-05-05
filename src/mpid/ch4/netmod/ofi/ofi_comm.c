/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "ofi_impl.h"
#include "mpidu_bc.h"
#include "ofi_noinline.h"

static int update_multi_nic_hints(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;

    if (comm) {
        /* If the user set a multi-nic hint, but num_nics = 1, disable multi-nic optimizations. */
        if (MPIDI_OFI_global.num_nics > 1) {
            int was_enabled_striping = MPIDI_OFI_COMM(comm).enable_striping;

            /* Check if we should use striping */
            if (comm->hints[MPIR_COMM_HINT_ENABLE_STRIPING] != -1)
                MPIDI_OFI_COMM(comm).enable_striping = comm->hints[MPIR_COMM_HINT_ENABLE_STRIPING];
            else
                MPIDI_OFI_COMM(comm).enable_striping = MPIR_CVAR_CH4_OFI_ENABLE_STRIPING;

            /* If striping was on and we disabled it here, decrement the global counter. */
            if (was_enabled_striping > 0 && MPIDI_OFI_COMM(comm).enable_striping == 0)
                MPIDI_OFI_global.num_comms_enabled_striping--;
            /* If striping was off and we enabled it here, increment the global counter. */
            else if (was_enabled_striping <= 0 && MPIDI_OFI_COMM(comm).enable_striping != 0)
                MPIDI_OFI_global.num_comms_enabled_striping++;

            if (MPIDI_OFI_COMM(comm).enable_striping) {
                MPIDI_OFI_global.stripe_threshold = MPIR_CVAR_CH4_OFI_STRIPING_THRESHOLD;
            }

            int was_enabled_hashing = MPIDI_OFI_COMM(comm).enable_hashing;

            /* Check if we should use hashing */
            if (comm->hints[MPIR_COMM_HINT_ENABLE_MULTI_NIC_HASHING] != -1)
                MPIDI_OFI_COMM(comm).enable_hashing =
                    comm->hints[MPIR_COMM_HINT_ENABLE_MULTI_NIC_HASHING];
            else        /* If this value is -1, that means the user hasn't set a value. */
                MPIDI_OFI_COMM(comm).enable_hashing = MPIR_CVAR_CH4_OFI_ENABLE_MULTI_NIC_HASHING;

            /* If the user set the hashing hint, but not no_any_tag/source, disable
             * hashing because it would introduce a major performance penalty to have to post a
             * request on each NIC and then cancel it later. */
            if (MPIDI_OFI_COMM(comm).enable_hashing &&
                (!comm->hints[MPIR_COMM_HINT_NO_ANY_TAG] ||
                 !comm->hints[MPIR_COMM_HINT_NO_ANY_SOURCE])) {
                MPIDI_OFI_COMM(comm).enable_hashing = 0;
            }

            /* If hashing was on and we disabled it here, decrement the global counter. */
            if (was_enabled_hashing > 0 && MPIDI_OFI_COMM(comm).enable_hashing == 0)
                MPIDI_OFI_global.num_comms_enabled_hashing--;
            /* If hashing was off and we enabled it here, increment the global counter. */
            else if (was_enabled_hashing <= 0 && MPIDI_OFI_COMM(comm).enable_hashing != 0)
                MPIDI_OFI_global.num_comms_enabled_hashing++;
        }
    }

    return mpi_errno;
}

int MPIDI_OFI_mpi_comm_commit_pre_hook(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_MPI_COMM_COMMIT_PRE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_MPI_COMM_COMMIT_PRE_HOOK);

    MPIDIU_map_create(&MPIDI_OFI_COMM(comm).huge_send_counters, MPL_MEM_COMM);
    MPIDIU_map_create(&MPIDI_OFI_COMM(comm).huge_recv_counters, MPL_MEM_COMM);

    /* no connection for non-dynamic or non-root-rank of intercomm */
    MPIDI_OFI_COMM(comm).conn_id = -1;

    /* Initialize the multi-nic optimization values */
    MPIDI_OFI_global.num_comms_enabled_striping = 0;
    MPIDI_OFI_global.num_comms_enabled_hashing = 0;
    MPIDI_OFI_COMM(comm).enable_striping = 0;
    MPIDI_OFI_COMM(comm).enable_hashing = 0;

    /* eagain defaults to off */
    if (comm->hints[MPIR_COMM_HINT_EAGAIN] == 0) {
        comm->hints[MPIR_COMM_HINT_EAGAIN] = FALSE;
    }

    update_multi_nic_hints(comm);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_MPI_COMM_COMMIT_PRE_HOOK);
    return mpi_errno;
}

int MPIDI_OFI_mpi_comm_commit_post_hook(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_MPI_COMM_COMMIT_POST_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_MPI_COMM_COMMIT_POST_HOOK);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_MPI_COMM_COMMIT_POST_HOOK);
    return mpi_errno;
}

int MPIDI_OFI_mpi_comm_free_hook(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_MPI_COMM_FREE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_MPI_COMM_FREE_HOOK);

    /* If we enabled striping or hashing, decrement the counter. */
    MPIDI_OFI_global.num_comms_enabled_striping -=
        (MPIDI_OFI_COMM(comm).enable_striping != 0 ? 1 : 0);
    MPIDI_OFI_global.num_comms_enabled_hashing -=
        (MPIDI_OFI_COMM(comm).enable_hashing != 0 ? 1 : 0);

    MPIDIU_map_destroy(MPIDI_OFI_COMM(comm).huge_send_counters);
    MPIDIU_map_destroy(MPIDI_OFI_COMM(comm).huge_recv_counters);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_MPI_COMM_FREE_HOOK);
    return mpi_errno;
}

int MPIDI_OFI_comm_set_hints(MPIR_Comm * comm, MPIR_Info * info)
{
    int mpi_errno = MPI_SUCCESS;

    update_multi_nic_hints(comm);

    return mpi_errno;
}

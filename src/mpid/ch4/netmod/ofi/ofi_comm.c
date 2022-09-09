/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "ofi_impl.h"
#include "mpidu_bc.h"
#include "ofi_noinline.h"

#define HAS_PREF_NIC(comm) comm->hints[MPIR_COMM_HINT_MULTI_NIC_PREF_NIC] != -1

static int update_multi_nic_hints(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;

    if (comm) {
        /* If the user set a multi-nic hint, but num_nics = 1, disable multi-nic optimizations. */
        if (MPIDI_OFI_global.num_nics > 1) {
            int was_enabled_striping = MPIDI_OFI_COMM(comm).enable_striping;

            /* Check if we should use striping */
            if (HAS_PREF_NIC(comm)) {
                /* If the user specified a particular NIC, don't use striping. */
                MPIDI_OFI_COMM(comm).enable_striping = 0;
            } else if (comm->hints[MPIR_COMM_HINT_ENABLE_MULTI_NIC_STRIPING] != -1)
                MPIDI_OFI_COMM(comm).enable_striping =
                    comm->hints[MPIR_COMM_HINT_ENABLE_MULTI_NIC_STRIPING];
            else
                MPIDI_OFI_COMM(comm).enable_striping = MPIR_CVAR_CH4_OFI_ENABLE_MULTI_NIC_STRIPING;

            /* If striping was on and we disabled it here, decrement the global counter. */
            if (was_enabled_striping > 0 && MPIDI_OFI_COMM(comm).enable_striping == 0)
                MPIDI_OFI_global.num_comms_enabled_striping--;
            /* If striping was off and we enabled it here, increment the global counter. */
            else if (was_enabled_striping <= 0 && MPIDI_OFI_COMM(comm).enable_striping != 0)
                MPIDI_OFI_global.num_comms_enabled_striping++;

            if (MPIDI_OFI_COMM(comm).enable_striping) {
                MPIDI_OFI_global.stripe_threshold = MPIR_CVAR_CH4_OFI_MULTI_NIC_STRIPING_THRESHOLD;
            }

            int was_enabled_hashing = MPIDI_OFI_COMM(comm).enable_hashing;

            /* Check if we should use hashing */
            if (HAS_PREF_NIC(comm)) {
                /* If the user specified a particular NIC, don't use hashing.  */
                MPIDI_OFI_COMM(comm).enable_hashing = 0;
            } else if (comm->hints[MPIR_COMM_HINT_ENABLE_MULTI_NIC_HASHING] != -1)
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

static int update_nic_preferences(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;

    if (comm) {
        /* If the user has set a preferred NIC, we need to exchange it with all processes and store
         * it in the communciator object. If this happens while traffic is outstanding, it will
         * cause problems so the user needs to quiesce traffic first. */
        if (comm->hints[MPIR_COMM_HINT_MULTI_NIC_PREF_NIC] != -1) {
            /* comm is used to exchange its pref_nic hints. So, to avoid its behavior change while it is
             * under use, exchange pref_nic using a local copy first, then copy it to comm.pref_nic */
            int *pref_nic_copy = MPL_malloc(sizeof(int) * comm->remote_size, MPL_MEM_ADDRESS);
            MPIR_ERR_CHKANDJUMP(pref_nic_copy == NULL, mpi_errno, MPI_ERR_NO_MEM, "**nomem");

            /* Make sure the NIC id is in a valid range. If the user went over the number of NICs,
             * loop back around to the first NIC. */
            comm->hints[MPIR_COMM_HINT_MULTI_NIC_PREF_NIC] %= MPIDI_OFI_global.num_nics;

            /* The user will be providing a value that is consistent across all process on the same
             * node, but internally, the NICs are reordered so that index 0 is the NIC that will be
             * used by default. Compare the user's preference to the original IDs stored when
             * setting up the NICs during initialization. */
            for (int i = 0; i < MPIDI_OFI_global.num_nics; ++i) {
                if (MPIDI_OFI_global.nic_info[i].id ==
                    comm->hints[MPIR_COMM_HINT_MULTI_NIC_PREF_NIC]) {
                    pref_nic_copy[comm->rank] = i;
                    break;
                }
            }

            MPIR_Errflag_t errflag = MPIR_ERR_NONE;
            /* Collect the NIC IDs set for the other ranks. We always expect to receive a single
             * NIC id from each rank, i.e., one MPI_INT. */
            mpi_errno = MPIR_Allgather_allcomm_auto(MPI_IN_PLACE, 0, MPI_INT,
                                                    pref_nic_copy, 1, MPI_INT, comm, &errflag);
            MPIR_ERR_CHECK(mpi_errno);

            if (MPIDI_OFI_COMM(comm).pref_nic == NULL) {
                MPIDI_OFI_COMM(comm).pref_nic = MPL_malloc(sizeof(int) * comm->remote_size,
                                                           MPL_MEM_ADDRESS);
                MPIR_ERR_CHKANDJUMP(MPIDI_OFI_COMM(comm).pref_nic == NULL, mpi_errno,
                                    MPI_ERR_NO_MEM, "**nomem");
            }

            /* Update comm.pref_nic */
            for (int i = 0; i < comm->remote_size; ++i) {
                MPIDI_OFI_COMM(comm).pref_nic[i] = pref_nic_copy[i];
            }
            MPL_free(pref_nic_copy);
        }
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_OFI_mpi_comm_commit_pre_hook(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    /* no connection for non-dynamic or non-root-rank of intercomm */
    MPIDI_OFI_COMM(comm).conn_id = -1;

    /* Initialize the multi-nic optimization values */
    MPIDI_OFI_global.num_comms_enabled_striping = 0;
    MPIDI_OFI_global.num_comms_enabled_hashing = 0;
    MPIDI_OFI_COMM(comm).enable_striping = 0;
    MPIDI_OFI_COMM(comm).enable_hashing = 0;
    MPIDI_OFI_COMM(comm).pref_nic = NULL;

    /* eagain defaults to off */
    if (comm->hints[MPIR_COMM_HINT_EAGAIN] == 0) {
        comm->hints[MPIR_COMM_HINT_EAGAIN] = FALSE;
    }

    if (comm->hints[MPIR_COMM_HINT_ENABLE_MULTI_NIC_STRIPING] == -1) {
        comm->hints[MPIR_COMM_HINT_ENABLE_MULTI_NIC_STRIPING] =
            MPIR_CVAR_CH4_OFI_ENABLE_MULTI_NIC_STRIPING;
    }

    if (comm->hints[MPIR_COMM_HINT_ENABLE_MULTI_NIC_HASHING] == -1) {
        comm->hints[MPIR_COMM_HINT_ENABLE_MULTI_NIC_HASHING] =
            MPIR_CVAR_CH4_OFI_ENABLE_MULTI_NIC_HASHING;
    }

    mpi_errno = update_multi_nic_hints(comm);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_OFI_mpi_comm_commit_post_hook(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    /* When setting up built in communicators, there won't be any way to do collectives yet. We also
     * won't have any info hints to propagate so there won't be any preferences that need to be
     * communicated. */
    if (comm != MPIR_Process.comm_world) {
        mpi_errno = update_nic_preferences(comm);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_OFI_mpi_comm_free_hook(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    /* If we enabled striping or hashing, decrement the counter. */
    MPIDI_OFI_global.num_comms_enabled_striping -=
        (MPIDI_OFI_COMM(comm).enable_striping != 0 ? 1 : 0);
    MPIDI_OFI_global.num_comms_enabled_hashing -=
        (MPIDI_OFI_COMM(comm).enable_hashing != 0 ? 1 : 0);

    MPL_free(MPIDI_OFI_COMM(comm).pref_nic);

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

int MPIDI_OFI_comm_set_hints(MPIR_Comm * comm, MPIR_Info * info)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = update_multi_nic_hints(comm);
    MPIR_ERR_CHECK(mpi_errno);
    mpi_errno = update_nic_preferences(comm);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

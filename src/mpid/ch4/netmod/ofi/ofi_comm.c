/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "ofi_impl.h"
#include "mpidu_bc.h"
#include "ofi_noinline.h"
#include "coll/ofi_triggered.h"

static int update_multi_nic_hints(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;

    if (comm) {
        /* If the user set a multi-nic hint, but num_nics = 1, disable multi-nic optimizations. */
        if (MPIDI_OFI_global.num_nics > 1) {
            int was_enabled_striping = MPIDI_OFI_COMM(comm).enable_striping;

            /* Check if we should use striping */
            if (comm->hints[MPIR_COMM_HINT_MULTI_NIC_PREF_NIC] == 1) {
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
            if (comm->hints[MPIR_COMM_HINT_MULTI_NIC_PREF_NIC] == 1) {
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
            if (MPIDI_OFI_COMM(comm).pref_nic == NULL) {
                MPIDI_OFI_COMM(comm).pref_nic = MPL_malloc(sizeof(int) * comm->remote_size,
                                                           MPL_MEM_ADDRESS);
            }

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
                    MPIDI_OFI_COMM(comm).pref_nic[comm->rank] = i;
                    break;
                }
            }

            MPIR_Errflag_t errflag = MPIR_ERR_NONE;
            mpi_errno = MPIR_Allgather_allcomm_auto(MPI_IN_PLACE, 0, MPI_INT,
                                                    MPIDI_OFI_COMM(comm).pref_nic,
                                                    comm->remote_size, MPI_INT, comm, &errflag);
            MPIR_ERR_POP(mpi_errno);
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
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_MPI_COMM_COMMIT_PRE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_MPI_COMM_COMMIT_PRE_HOOK);

    /* no connection for non-dynamic or non-root-rank of intercomm */
    MPIDI_OFI_COMM(comm).conn_id = -1;

    /* for triggered op with blocking small message */
    MPIDI_OFI_COMM(comm).blk_sml_bcast = NULL;

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
    /* When setting up built in communicators, there won't be any way to do collectives yet. We also
     * won't have any info hints to propagate so there won't be any preferences that need to be
     * communicated. */
    if (comm != MPIR_Process.comm_world) {
        mpi_errno = update_nic_preferences(comm);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_MPI_COMM_COMMIT_PRE_HOOK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_OFI_mpi_comm_commit_post_hook(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_MPI_COMM_COMMIT_POST_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_MPI_COMM_COMMIT_POST_HOOK);

    MPL_free(MPIDI_OFI_COMM(comm).pref_nic);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_MPI_COMM_COMMIT_POST_HOOK);
    return mpi_errno;
}

/* For blocking small message collectives, resources are allocated per communicator
 * while schedule for the next iteration is pre-posted in an overlapped manner as the
 * collective is being executed. This function frees these resources when the communicators is freed. */
MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_trig_blocking_small_msg_destroy(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    MPIDI_OFI_trig_bcast_blocking_small_msg *blk_sml_bcast;

    blk_sml_bcast = MPIDI_OFI_COMM(comm).blk_sml_bcast;
    if (blk_sml_bcast) {
        MPIDI_OFI_free_deferred_works(blk_sml_bcast->works, blk_sml_bcast->num_works, true);
        MPL_free(blk_sml_bcast->works);

        if (blk_sml_bcast->size > 0) {
            for (i = 0; i < blk_sml_bcast->size; i++) {
                fi_close(&blk_sml_bcast->recv_cntr[i]->fid);
                fi_close(&blk_sml_bcast->rcv_mr[i]->fid);
                MPL_free(blk_sml_bcast->recv_buf[i]);
            }
            MPL_free(blk_sml_bcast->recv_buf);
            MPL_free(blk_sml_bcast->recv_cntr);
            MPL_free(blk_sml_bcast->rcv_mr);
        }

        fi_close(&blk_sml_bcast->send_cntr->fid);
        MPL_free(blk_sml_bcast);
        MPIDI_OFI_COMM(comm).blk_sml_bcast = NULL;
    }

    return mpi_errno;
}

int MPIDI_OFI_mpi_comm_free_hook(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_MPI_COMM_FREE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_MPI_COMM_FREE_HOOK);

    MPIDI_OFI_trig_blocking_small_msg_destroy(comm);
    /* If we enabled striping or hashing, decrement the counter. */
    MPIDI_OFI_global.num_comms_enabled_striping -=
        (MPIDI_OFI_COMM(comm).enable_striping != 0 ? 1 : 0);
    MPIDI_OFI_global.num_comms_enabled_hashing -=
        (MPIDI_OFI_COMM(comm).enable_hashing != 0 ? 1 : 0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_MPI_COMM_FREE_HOOK);
    return mpi_errno;
}

int MPIDI_OFI_comm_set_hints(MPIR_Comm * comm, MPIR_Info * info)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = update_multi_nic_hints(comm);
    MPIR_ERR_POP(mpi_errno);
    mpi_errno = update_nic_preferences(comm);
    MPIR_ERR_POP(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

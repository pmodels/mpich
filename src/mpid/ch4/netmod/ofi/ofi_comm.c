/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "ofi_impl.h"
#include "mpidu_bc.h"
#include "ofi_noinline.h"
#include "coll/ofi_triggered.h"

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

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_switch_coll_init(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;
    struct fi_av_set_attr av_set_attr;
    struct fid_av_set *av_set = NULL;
    fi_addr_t comm_addr;
    struct fid_mc *coll_mc;
    MPIR_Request *req_ptr;

    MPIR_Assert(comm != NULL);

    MPIDI_OFI_COMM(comm).offload_coll.coll_mc = NULL;
    MPIDI_OFI_COMM(comm).offload_coll.av_set = NULL;
    MPIDI_OFI_COMM(comm).offload_coll.req = NULL;

    if (comm->local_size == 1)
        goto fn_exit;

    /* does not apply to intercommunicator and its local comm */
    if (comm->hierarchy_kind == MPIR_COMM_HIERARCHY_KIND__NODE ||
        comm->comm_kind == MPIR_COMM_KIND__INTERCOMM ||
        MPIR_CONTEXT_READ_FIELD(IS_LOCALCOMM, comm->context_id))
        goto fn_exit;

    if (comm == MPIR_Process.comm_world) {
        av_set_attr.count = comm->local_size;
        av_set_attr.start_addr = 0;
        av_set_attr.end_addr = comm->local_size * MPIDI_OFI_global.num_nics - 1;
        av_set_attr.stride = MPIDI_OFI_global.num_nics;
        MPIDI_OFI_CALL(fi_av_set(MPIDI_OFI_global.ctx[0].av, &av_set_attr, &av_set, NULL),
                       fi_av_set);
        av_set_attr.comm_key = NULL;
        av_set_attr.comm_key_size = 0;
    } else {
        /* create an empty av_set first */
        av_set_attr.count = 0;
        av_set_attr.start_addr = FI_ADDR_NOTAVAIL;
        av_set_attr.end_addr = FI_ADDR_NOTAVAIL;
        av_set_attr.stride = 1;
        av_set_attr.comm_key = (void *) &comm->context_id;
        av_set_attr.comm_key_size = sizeof(comm->context_id);
        MPIDI_OFI_CALL(fi_av_set(MPIDI_OFI_global.ctx[0].av, &av_set_attr, &av_set, NULL),
                       fi_av_set);
        /* add group members */
        for (int i = 0; i < comm->local_size; i++) {
            fi_addr_t addr = MPIDI_OFI_comm_to_phys(comm, i, 0, 0, 0);
            fi_av_set_insert(av_set, addr);
        }
    }

    /* return avset address for communication in the next step */
    MPIDI_OFI_CALL(fi_av_set_addr(av_set, &comm_addr), fi_av_set_addr);

    /* setup multicast group, calling allreduce internally */
    req_ptr = MPIR_Request_create(MPIR_REQUEST_KIND__COLL);
    if (!req_ptr)
        MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**nomem");
    MPIR_Request_add_ref(req_ptr);

    MPIDI_OFI_CALL(fi_join_collective(MPIDI_OFI_global.ctx[0].ep, comm_addr,
                                      av_set, 0, &coll_mc,
                                      (void *) &(MPIDI_OFI_REQUEST(req_ptr, context))),
                   fi_join_collective);

    /* for comm_world, wait for this setup to finish now because we
     * can't call OFI in comm free hook later */
    if (comm == MPIR_Process.comm_world) {
        while (MPIR_cc_get(*req_ptr->cc_ptr) != 0) {
            MPID_Progress_test(NULL);
        }
        MPIR_Request_free(req_ptr);
    } else
        MPIDI_OFI_COMM(comm).offload_coll.req = req_ptr;

    MPIDI_OFI_COMM(comm).offload_coll.coll_mc = coll_mc;
    MPIDI_OFI_COMM(comm).offload_coll.av_set = av_set;
    MPIDI_OFI_COMM(comm).offload_coll.coll_addr = fi_mc_addr(coll_mc);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_switch_coll_free(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;

    if (comm->local_size == 1)
        goto fn_exit;

    if (MPIDI_OFI_COMM(comm).offload_coll.req) {
        /* wait for ofi collective group setup to finish */
        while (MPIR_cc_get(*MPIDI_OFI_COMM(comm).offload_coll.req->cc_ptr) != 0) {
            MPID_Progress_test(NULL);
        }
        MPIR_Request_free(MPIDI_OFI_COMM(comm).offload_coll.req);
        MPIDI_OFI_COMM(comm).offload_coll.req = NULL;
    }
    if (MPIDI_OFI_COMM(comm).offload_coll.coll_mc) {
        MPIDI_OFI_CALL(fi_close(&(MPIDI_OFI_COMM(comm).offload_coll.coll_mc->fid)), fi_close);
        MPIDI_OFI_COMM(comm).offload_coll.coll_mc = NULL;
    }
    if (MPIDI_OFI_COMM(comm).offload_coll.av_set) {
        MPIDI_OFI_CALL(fi_close(&(MPIDI_OFI_COMM(comm).offload_coll.av_set->fid)), fi_close);
        MPIDI_OFI_COMM(comm).offload_coll.av_set = NULL;
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

    /* for triggered op with blocking small message */
    MPIDI_OFI_COMM(comm).blk_sml_bcast = NULL;
    MPIDI_OFI_COMM(comm).blk_sml_allred = NULL;

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

    if (MPIDI_OFI_ENABLE_OFI_COLLECTIVE) {
        mpi_errno = MPIDI_OFI_switch_coll_init(comm);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }

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

    mpi_errno =
        MPIR_Csel_prune(MPIDI_global.nm.ofi.csel_root, comm, &MPIDI_OFI_COMM(comm).csel_comm);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
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

/* For blocking small message collectives, resources are allocated per communicator
 * while schedule for the next iteration is pre-posted in an overlapped manner as the
 * collective is being executed. This function frees these resources when the communicators is freed. */
MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_trig_blocking_small_msg_destroy(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    MPIDI_OFI_trig_bcast_blocking_small_msg *blk_sml_bcast;
    MPIDI_OFI_trig_allred_blocking_small_msg *blk_sml_allred;

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

    blk_sml_allred = MPIDI_OFI_COMM(comm).blk_sml_allred;
    if (blk_sml_allred) {
        MPIDI_OFI_free_deferred_works(blk_sml_allred->works, blk_sml_allred->num_works, true);
        MPL_free(blk_sml_allred->works);

        if (blk_sml_allred->size > 0) {
            for (i = 0; i < blk_sml_allred->size; i++) {
                MPL_free(blk_sml_allred->recv_buf[i]);
                fi_close(&blk_sml_allred->recv_cntr[i]->fid);
                fi_close(&blk_sml_allred->rcv_mr[i]->fid);
            }
            MPL_free(blk_sml_allred->recv_buf);
            MPL_free(blk_sml_allred->recv_cntr);
            MPL_free(blk_sml_allred->rcv_mr);
        }
        fi_close(&blk_sml_allred->send_cntr->fid);
        fi_close(&blk_sml_allred->atomic_cntr->fid);
        MPL_free(blk_sml_allred);
        MPIDI_OFI_COMM(comm).blk_sml_allred = NULL;
    }

    return mpi_errno;
}

/* Do not call OFI functions in this free hook, this is because
 * in case the comm is comm_world, the OFI library has been finalized
 * before comm_world, etc built-in communicators are freed
 */
int MPIDI_OFI_mpi_comm_free_hook(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    if (MPIDI_OFI_COMM(comm).csel_comm) {
        mpi_errno = MPIR_Csel_free(MPIDI_OFI_COMM(comm).csel_comm);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
        MPIDI_OFI_COMM(comm).csel_comm = NULL;
    }

    MPIDI_OFI_trig_blocking_small_msg_destroy(comm);
    /* If we enabled striping or hashing, decrement the counter. */
    MPIDI_OFI_global.num_comms_enabled_striping -=
        (MPIDI_OFI_COMM(comm).enable_striping != 0 ? 1 : 0);
    MPIDI_OFI_global.num_comms_enabled_hashing -=
        (MPIDI_OFI_COMM(comm).enable_hashing != 0 ? 1 : 0);

    if (MPIDI_OFI_ENABLE_OFI_COLLECTIVE)
        MPIDI_OFI_switch_coll_free(comm);

    MPL_free(MPIDI_OFI_COMM(comm).pref_nic);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
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

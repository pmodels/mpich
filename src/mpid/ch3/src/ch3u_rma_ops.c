/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidrma.h"

MPIR_T_PVAR_DOUBLE_TIMER_DECL_EXTERN(RMA, rma_rmaqueue_set);

#define MPIDI_PASSIVE_TARGET_DONE_TAG  348297
#define MPIDI_PASSIVE_TARGET_RMA_TAG 563924

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_CH3_RMA_OP_POKING_PROGRESS
      category    : CH3
      type        : int
      default     : 100
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
          Specify the threshold of number of posted operations
          when starting poking progress in operation routines.
          When the value is negative, runtime never pokes progress
          engine in operation routines; when the value is zero,
          runtime always pokes progress engine in operation
          routines; when the value is larger than zero, runtime
          starts to poke progress engine when number of posted
          operations reaches that value.

    - name        : MPIR_CVAR_CH3_RMA_OP_PIGGYBACK_LOCK_DATA_SIZE
      category    : CH3
      type        : int
      default     : 65536
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
          Specify the threshold of data size of a RMA operation
          which can be piggybacked with a LOCK message. It is
          always a positive value and should not be smaller
          than MPIDI_RMA_IMMED_BYTES.
          If user sets it as a small value, for middle and large
          data size, we will lose performance because of always
          waiting for round-trip of LOCK synchronization; if
          user sets it as a large value, we need to consume
          more memory on target side to buffer this lock request
          when lock is not satisfied.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Put
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_Put(const void *origin_addr, int origin_count, MPI_Datatype
                   origin_datatype, int target_rank, MPI_Aint target_disp,
                   int target_count, MPI_Datatype target_datatype, MPID_Win * win_ptr,
                   MPID_Request * ureq)
{
    int mpi_errno = MPI_SUCCESS;
    int dt_contig ATTRIBUTE((unused)), rank;
    MPID_Datatype *dtp;
    MPI_Aint dt_true_lb ATTRIBUTE((unused));
    MPIDI_msg_sz_t data_sz;
    MPIDI_VC_t *orig_vc = NULL, *target_vc = NULL;
    int made_progress = 0;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_PUT);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_PUT);

    MPIU_ERR_CHKANDJUMP(win_ptr->states.access_state == MPIDI_RMA_NONE,
                        mpi_errno, MPI_ERR_RMA_SYNC, "**rmasync");

    if (target_rank == MPI_PROC_NULL) {
        goto fn_exit;
    }

    MPIDI_Datatype_get_info(origin_count, origin_datatype, dt_contig, data_sz, dtp, dt_true_lb);

    if (data_sz == 0) {
        goto fn_exit;
    }

    rank = win_ptr->comm_ptr->rank;

    if (win_ptr->shm_allocated == TRUE && target_rank != rank &&
        win_ptr->create_flavor != MPI_WIN_FLAVOR_SHARED) {
        /* check if target is local and shared memory is allocated on window,
         * if so, we directly perform this operation on shared memory region. */

        /* FIXME: Here we decide whether to perform SHM operations by checking if origin and target are on
         * the same node. However, in ch3:sock, even if origin and target are on the same node, they do
         * not within the same SHM region. Here we filter out ch3:sock by checking shm_allocated flag first,
         * which is only set to TRUE when SHM region is allocated in nemesis.
         * In future we need to figure out a way to check if origin and target are in the same "SHM comm".
         */
        MPIDI_Comm_get_vc(win_ptr->comm_ptr, rank, &orig_vc);
        MPIDI_Comm_get_vc(win_ptr->comm_ptr, target_rank, &target_vc);
    }

    /* If the put is a local operation, do it here */
    if (target_rank == rank || win_ptr->create_flavor == MPI_WIN_FLAVOR_SHARED ||
        (win_ptr->shm_allocated == TRUE && orig_vc->node_id == target_vc->node_id)) {
        mpi_errno = MPIDI_CH3I_Shm_put_op(origin_addr, origin_count, origin_datatype, target_rank,
                                          target_disp, target_count, target_datatype, win_ptr);
        if (mpi_errno)
            MPIU_ERR_POP(mpi_errno);

        if (ureq) {
            /* Complete user request and release the ch3 ref */
            MPID_Request_set_completed(ureq);
            MPID_Request_release(ureq);
        }
    }
    else {
        MPIDI_RMA_Op_t *new_ptr = NULL;
        MPIDI_CH3_Pkt_put_t *put_pkt = NULL;
        MPI_Aint origin_type_size;
        size_t immed_len, len;
        int use_immed_pkt = FALSE;
        int is_origin_contig, is_target_contig;

        /* queue it up */
        mpi_errno = MPIDI_CH3I_Win_get_op(win_ptr, &new_ptr);
        if (mpi_errno != MPI_SUCCESS)
            MPIU_ERR_POP(mpi_errno);

        MPIR_T_PVAR_TIMER_START(RMA, rma_rmaqueue_set);

        /******************** Setting operation struct areas ***********************/

        /* FIXME: For contig and very short operations, use a streamlined op */
        new_ptr->origin_addr = (void *) origin_addr;
        new_ptr->origin_count = origin_count;
        new_ptr->origin_datatype = origin_datatype;
        new_ptr->target_rank = target_rank;

        /* Remember user request */
        if (ureq) {
            new_ptr->ureq = ureq;
        }

        /* if source or target datatypes are derived, increment their
         * reference counts */
        if (!MPIR_DATATYPE_IS_PREDEFINED(origin_datatype)) {
            MPID_Datatype_get_ptr(origin_datatype, dtp);
            MPID_Datatype_add_ref(dtp);
            new_ptr->is_dt = 1;
        }
        if (!MPIR_DATATYPE_IS_PREDEFINED(target_datatype)) {
            MPID_Datatype_get_ptr(target_datatype, dtp);
            MPID_Datatype_add_ref(dtp);
            new_ptr->is_dt = 1;
        }

        MPID_Datatype_is_contig(origin_datatype, &is_origin_contig);
        MPID_Datatype_is_contig(target_datatype, &is_target_contig);

        MPID_Datatype_get_size_macro(origin_datatype, origin_type_size);
        MPIU_Assign_trunc(len, origin_count * origin_type_size, size_t);

        /* Judge if we can use IMMED data packet */
        if (!new_ptr->is_dt && is_origin_contig && is_target_contig) {
            MPIU_Assign_trunc(immed_len,
                              (MPIDI_RMA_IMMED_BYTES / origin_type_size) * origin_type_size,
                              size_t);
            if (len <= immed_len)
                use_immed_pkt = TRUE;
        }

        /* Judge if this operation is an piggyback candidate */
        if (!new_ptr->is_dt) {
            /* FIXME: currently we only piggyback LOCK flag with op using predefined datatypes
             * for both origin and target data. We should extend this optimization to derived
             * datatypes as well. */
            if (len <= MPIR_CVAR_CH3_RMA_OP_PIGGYBACK_LOCK_DATA_SIZE)
                new_ptr->piggyback_lock_candidate = 1;
        }

        /************** Setting packet struct areas in operation ****************/

        put_pkt = &(new_ptr->pkt.put);

        if (use_immed_pkt) {
            MPIDI_Pkt_init(put_pkt, MPIDI_CH3_PKT_PUT_IMMED);
        }
        else {
            MPIDI_Pkt_init(put_pkt, MPIDI_CH3_PKT_PUT);
        }

        put_pkt->addr = (char *) win_ptr->basic_info_table[target_rank].base_addr +
            win_ptr->basic_info_table[target_rank].disp_unit * target_disp;
        put_pkt->count = target_count;
        put_pkt->datatype = target_datatype;
        put_pkt->info.dataloop_size = 0;
        put_pkt->target_win_handle = win_ptr->basic_info_table[target_rank].win_handle;
        put_pkt->source_win_handle = win_ptr->handle;
        put_pkt->flags = MPIDI_CH3_PKT_FLAG_NONE;
        if (use_immed_pkt) {
            void *src = (void *) origin_addr, *dest = (void *) (put_pkt->info.data);
            mpi_errno = immed_copy(src, dest, len);
            if (mpi_errno != MPI_SUCCESS)
                MPIU_ERR_POP(mpi_errno);
        }

        MPIR_T_PVAR_TIMER_END(RMA, rma_rmaqueue_set);

        mpi_errno = MPIDI_CH3I_Win_enqueue_op(win_ptr, new_ptr);
        if (mpi_errno)
            MPIU_ERR_POP(mpi_errno);

        mpi_errno = MPIDI_CH3I_RMA_Make_progress_target(win_ptr, target_rank, &made_progress);
        if (mpi_errno != MPI_SUCCESS)
            MPIU_ERR_POP(mpi_errno);

        if (MPIR_CVAR_CH3_RMA_OP_POKING_PROGRESS >= 0 &&
            win_ptr->accumulated_ops_cnt >= MPIR_CVAR_CH3_RMA_OP_POKING_PROGRESS) {
            mpi_errno = poke_progress_engine();
            if (mpi_errno != MPI_SUCCESS)
                MPIU_ERR_POP(mpi_errno);
        }

        if (MPIR_CVAR_CH3_RMA_ACTIVE_REQ_THRESHOLD >= 0 &&
            win_ptr->active_req_cnt >= MPIR_CVAR_CH3_RMA_ACTIVE_REQ_THRESHOLD) {
            while (win_ptr->active_req_cnt >= MPIR_CVAR_CH3_RMA_ACTIVE_REQ_THRESHOLD) {
                int local_completed = 0, remote_completed = 0;
                mpi_errno = MPIDI_CH3I_RMA_Cleanup_ops_win(win_ptr, &local_completed,
                                                           &remote_completed);
                if (mpi_errno != MPI_SUCCESS)
                    MPIU_ERR_POP(mpi_errno);
                mpi_errno = poke_progress_engine();
                if (mpi_errno != MPI_SUCCESS)
                    MPIU_ERR_POP(mpi_errno);
            }
        }
    }

  fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_PUT);
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Get
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_Get(void *origin_addr, int origin_count, MPI_Datatype
                   origin_datatype, int target_rank, MPI_Aint target_disp,
                   int target_count, MPI_Datatype target_datatype, MPID_Win * win_ptr,
                   MPID_Request * ureq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_msg_sz_t data_sz;
    int dt_contig ATTRIBUTE((unused)), rank;
    MPI_Aint dt_true_lb ATTRIBUTE((unused));
    MPID_Datatype *dtp;
    MPIDI_VC_t *orig_vc = NULL, *target_vc = NULL;
    int made_progress = 0;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_GET);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_GET);

    MPIU_ERR_CHKANDJUMP(win_ptr->states.access_state == MPIDI_RMA_NONE,
                        mpi_errno, MPI_ERR_RMA_SYNC, "**rmasync");

    if (target_rank == MPI_PROC_NULL) {
        goto fn_exit;
    }

    MPIDI_Datatype_get_info(origin_count, origin_datatype, dt_contig, data_sz, dtp, dt_true_lb);

    if (data_sz == 0) {
        goto fn_exit;
    }

    rank = win_ptr->comm_ptr->rank;

    if (win_ptr->shm_allocated == TRUE && target_rank != rank &&
        win_ptr->create_flavor != MPI_WIN_FLAVOR_SHARED) {
        /* check if target is local and shared memory is allocated on window,
         * if so, we directly perform this operation on shared memory region. */

        /* FIXME: Here we decide whether to perform SHM operations by checking if origin and target are on
         * the same node. However, in ch3:sock, even if origin and target are on the same node, they do
         * not within the same SHM region. Here we filter out ch3:sock by checking shm_allocated flag first,
         * which is only set to TRUE when SHM region is allocated in nemesis.
         * In future we need to figure out a way to check if origin and target are in the same "SHM comm".
         */
        MPIDI_Comm_get_vc(win_ptr->comm_ptr, rank, &orig_vc);
        MPIDI_Comm_get_vc(win_ptr->comm_ptr, target_rank, &target_vc);
    }

    /* If the get is a local operation, do it here */
    if (target_rank == rank || win_ptr->create_flavor == MPI_WIN_FLAVOR_SHARED ||
        (win_ptr->shm_allocated == TRUE && orig_vc->node_id == target_vc->node_id)) {
        mpi_errno = MPIDI_CH3I_Shm_get_op(origin_addr, origin_count, origin_datatype, target_rank,
                                          target_disp, target_count, target_datatype, win_ptr);
        if (mpi_errno)
            MPIU_ERR_POP(mpi_errno);

        if (ureq) {
            /* Complete user request and release the ch3 ref */
            MPID_Request_set_completed(ureq);
            MPID_Request_release(ureq);
        }
    }
    else {
        MPIDI_RMA_Op_t *new_ptr = NULL;
        MPIDI_CH3_Pkt_get_t *get_pkt = NULL;
        MPI_Aint target_type_size;
        size_t immed_len, len;
        int use_immed_resp_pkt = FALSE;
        int is_origin_contig, is_target_contig;

        /* queue it up */
        mpi_errno = MPIDI_CH3I_Win_get_op(win_ptr, &new_ptr);
        if (mpi_errno != MPI_SUCCESS)
            MPIU_ERR_POP(mpi_errno);

        MPIR_T_PVAR_TIMER_START(RMA, rma_rmaqueue_set);

        /******************** Setting operation struct areas ***********************/

        /* FIXME: For contig and very short operations, use a streamlined op */
        new_ptr->origin_addr = origin_addr;
        new_ptr->origin_count = origin_count;
        new_ptr->origin_datatype = origin_datatype;
        new_ptr->target_rank = target_rank;

        /* Remember user request */
        if (ureq) {
            new_ptr->ureq = ureq;
        }

        /* if source or target datatypes are derived, increment their
         * reference counts */
        if (!MPIR_DATATYPE_IS_PREDEFINED(origin_datatype)) {
            MPID_Datatype_get_ptr(origin_datatype, dtp);
            MPID_Datatype_add_ref(dtp);
            new_ptr->is_dt = 1;
        }
        if (!MPIR_DATATYPE_IS_PREDEFINED(target_datatype)) {
            MPID_Datatype_get_ptr(target_datatype, dtp);
            MPID_Datatype_add_ref(dtp);
            new_ptr->is_dt = 1;
        }

        MPID_Datatype_is_contig(origin_datatype, &is_origin_contig);
        MPID_Datatype_is_contig(target_datatype, &is_target_contig);

        MPID_Datatype_get_size_macro(target_datatype, target_type_size);
        MPIU_Assign_trunc(len, target_count * target_type_size, size_t);

        /* Judge if we can use IMMED data response packet */
        if (!new_ptr->is_dt && is_origin_contig && is_target_contig) {
            MPIU_Assign_trunc(immed_len,
                              (MPIDI_RMA_IMMED_BYTES / target_type_size) * target_type_size,
                              size_t);
            if (len <= immed_len)
                use_immed_resp_pkt = TRUE;
        }

        /* Judge if this operation is an piggyback candidate. */
        if (!new_ptr->is_dt) {
            /* FIXME: currently we only piggyback LOCK flag with op using predefined datatypes
             * for both origin and target data. We should extend this optimization to derived
             * datatypes as well. */
            new_ptr->piggyback_lock_candidate = 1;
        }

        /************** Setting packet struct areas in operation ****************/

        get_pkt = &(new_ptr->pkt.get);
        MPIDI_Pkt_init(get_pkt, MPIDI_CH3_PKT_GET);
        get_pkt->addr = (char *) win_ptr->basic_info_table[target_rank].base_addr +
            win_ptr->basic_info_table[target_rank].disp_unit * target_disp;
        get_pkt->count = target_count;
        get_pkt->datatype = target_datatype;
        get_pkt->info.dataloop_size = 0;
        get_pkt->target_win_handle = win_ptr->basic_info_table[target_rank].win_handle;
        get_pkt->flags = MPIDI_CH3_PKT_FLAG_NONE;
        if (use_immed_resp_pkt)
            get_pkt->flags |= MPIDI_CH3_PKT_FLAG_RMA_IMMED_RESP;

        MPIR_T_PVAR_TIMER_END(RMA, rma_rmaqueue_set);

        mpi_errno = MPIDI_CH3I_Win_enqueue_op(win_ptr, new_ptr);
        if (mpi_errno)
            MPIU_ERR_POP(mpi_errno);

        mpi_errno = MPIDI_CH3I_RMA_Make_progress_target(win_ptr, target_rank, &made_progress);
        if (mpi_errno != MPI_SUCCESS)
            MPIU_ERR_POP(mpi_errno);

        if (MPIR_CVAR_CH3_RMA_OP_POKING_PROGRESS >= 0 &&
            win_ptr->accumulated_ops_cnt >= MPIR_CVAR_CH3_RMA_OP_POKING_PROGRESS) {
            mpi_errno = poke_progress_engine();
            if (mpi_errno != MPI_SUCCESS)
                MPIU_ERR_POP(mpi_errno);
        }

        if (MPIR_CVAR_CH3_RMA_ACTIVE_REQ_THRESHOLD >= 0 &&
            win_ptr->active_req_cnt >= MPIR_CVAR_CH3_RMA_ACTIVE_REQ_THRESHOLD) {
            while (win_ptr->active_req_cnt >= MPIR_CVAR_CH3_RMA_ACTIVE_REQ_THRESHOLD) {
                int local_completed = 0, remote_completed = 0;
                mpi_errno = MPIDI_CH3I_RMA_Cleanup_ops_win(win_ptr, &local_completed,
                                                           &remote_completed);
                if (mpi_errno != MPI_SUCCESS)
                    MPIU_ERR_POP(mpi_errno);
                mpi_errno = poke_progress_engine();
                if (mpi_errno != MPI_SUCCESS)
                    MPIU_ERR_POP(mpi_errno);
            }
        }
    }

  fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_GET);
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Accumulate
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_Accumulate(const void *origin_addr, int origin_count, MPI_Datatype
                          origin_datatype, int target_rank, MPI_Aint target_disp,
                          int target_count, MPI_Datatype target_datatype, MPI_Op op,
                          MPID_Win * win_ptr, MPID_Request * ureq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_msg_sz_t data_sz;
    int dt_contig ATTRIBUTE((unused)), rank;
    MPI_Aint dt_true_lb ATTRIBUTE((unused));
    MPID_Datatype *dtp;
    MPIDI_VC_t *orig_vc = NULL, *target_vc = NULL;
    int made_progress = 0;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_ACCUMULATE);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_ACCUMULATE);

    MPIU_ERR_CHKANDJUMP(win_ptr->states.access_state == MPIDI_RMA_NONE,
                        mpi_errno, MPI_ERR_RMA_SYNC, "**rmasync");

    if (target_rank == MPI_PROC_NULL) {
        goto fn_exit;
    }

    MPIDI_Datatype_get_info(origin_count, origin_datatype, dt_contig, data_sz, dtp, dt_true_lb);

    if (data_sz == 0) {
        goto fn_exit;
    }

    rank = win_ptr->comm_ptr->rank;

    if (win_ptr->shm_allocated == TRUE && target_rank != rank &&
        win_ptr->create_flavor != MPI_WIN_FLAVOR_SHARED) {
        /* check if target is local and shared memory is allocated on window,
         * if so, we directly perform this operation on shared memory region. */

        /* FIXME: Here we decide whether to perform SHM operations by checking if origin and target are on
         * the same node. However, in ch3:sock, even if origin and target are on the same node, they do
         * not within the same SHM region. Here we filter out ch3:sock by checking shm_allocated flag first,
         * which is only set to TRUE when SHM region is allocated in nemesis.
         * In future we need to figure out a way to check if origin and target are in the same "SHM comm".
         */
        MPIDI_Comm_get_vc(win_ptr->comm_ptr, rank, &orig_vc);
        MPIDI_Comm_get_vc(win_ptr->comm_ptr, target_rank, &target_vc);
    }

    /* Do =! rank first (most likely branch?) */
    if (target_rank == rank || win_ptr->create_flavor == MPI_WIN_FLAVOR_SHARED ||
        (win_ptr->shm_allocated == TRUE && orig_vc->node_id == target_vc->node_id)) {
        mpi_errno = MPIDI_CH3I_Shm_acc_op(origin_addr, origin_count, origin_datatype,
                                          target_rank, target_disp, target_count, target_datatype,
                                          op, win_ptr);
        if (mpi_errno)
            MPIU_ERR_POP(mpi_errno);

        if (ureq) {
            /* Complete user request and release the ch3 ref */
            MPID_Request_set_completed(ureq);
            MPID_Request_release(ureq);
        }
    }
    else {
        MPIDI_RMA_Op_t *new_ptr = NULL;
        MPIDI_CH3_Pkt_accum_t *accum_pkt = NULL;
        MPI_Aint origin_type_size;
        size_t immed_len, len;
        int use_immed_pkt = FALSE;
        int is_origin_contig, is_target_contig;
        MPI_Aint stream_elem_count, stream_unit_count;
        MPI_Aint predefined_dtp_size, predefined_dtp_count, predefined_dtp_extent;
        MPID_Datatype *origin_dtp = NULL, *target_dtp = NULL;
        int i;

        /* queue it up */
        mpi_errno = MPIDI_CH3I_Win_get_op(win_ptr, &new_ptr);
        if (mpi_errno != MPI_SUCCESS)
            MPIU_ERR_POP(mpi_errno);

        MPIR_T_PVAR_TIMER_START(RMA, rma_rmaqueue_set);

        /******************** Setting operation struct areas ***********************/

        new_ptr->origin_addr = (void *) origin_addr;
        new_ptr->origin_count = origin_count;
        new_ptr->origin_datatype = origin_datatype;
        new_ptr->target_rank = target_rank;

        /* Remember user request */
        if (ureq) {
            new_ptr->ureq = ureq;
        }

        /* if source or target datatypes are derived, increment their
         * reference counts */
        if (!MPIR_DATATYPE_IS_PREDEFINED(origin_datatype)) {
            MPID_Datatype_get_ptr(origin_datatype, origin_dtp);
            new_ptr->is_dt = 1;
        }
        if (!MPIR_DATATYPE_IS_PREDEFINED(target_datatype)) {
            MPID_Datatype_get_ptr(target_datatype, target_dtp);
            new_ptr->is_dt = 1;
        }

        MPID_Datatype_get_size_macro(origin_datatype, origin_type_size);
        MPIU_Assign_trunc(len, origin_count * origin_type_size, size_t);

        /* Get size and count for predefined datatype elements */
        if (MPIR_DATATYPE_IS_PREDEFINED(origin_datatype)) {
            predefined_dtp_size = origin_type_size;
            predefined_dtp_count = origin_count;
            MPID_Datatype_get_extent_macro(origin_datatype, predefined_dtp_extent);
        }
        else {
            MPIU_Assert(origin_dtp->basic_type != MPI_DATATYPE_NULL);
            MPID_Datatype_get_size_macro(origin_dtp->basic_type, predefined_dtp_size);
            predefined_dtp_count = len / predefined_dtp_size;
            MPID_Datatype_get_extent_macro(origin_dtp->basic_type, predefined_dtp_extent);
        }
        MPIU_Assert(predefined_dtp_count > 0 && predefined_dtp_size > 0 &&
                    predefined_dtp_extent > 0);

        /* Calculate number of predefined elements in each stream unit, and
         * total number of stream units. */
        stream_elem_count = MPIDI_CH3U_Acc_stream_size / predefined_dtp_extent;
        stream_unit_count = (predefined_dtp_count - 1) / stream_elem_count + 1;
        MPIU_Assert(stream_elem_count > 0 && stream_unit_count > 0);

        for (i = 0; i < stream_unit_count; i++) {
            if (origin_dtp != NULL) {
                MPID_Datatype_add_ref(origin_dtp);
            }
            if (target_dtp != NULL) {
                MPID_Datatype_add_ref(target_dtp);
            }
        }

        MPID_Datatype_is_contig(origin_datatype, &is_origin_contig);
        MPID_Datatype_is_contig(target_datatype, &is_target_contig);

        /* Judge if we can use IMMED data packet */
        if (!new_ptr->is_dt && is_origin_contig && is_target_contig) {
            MPIU_Assign_trunc(immed_len,
                              (MPIDI_RMA_IMMED_BYTES / origin_type_size) * origin_type_size,
                              size_t);
            if (len <= immed_len)
                use_immed_pkt = TRUE;
        }

        /* Judge if this operation is an piggyback candidate. */
        if (!new_ptr->is_dt) {
            /* FIXME: currently we only piggyback LOCK flag with op using predefined datatypes
             * for both origin and target data. We should extend this optimization to derived
             * datatypes as well. */
            if (len <= MPIR_CVAR_CH3_RMA_OP_PIGGYBACK_LOCK_DATA_SIZE)
                new_ptr->piggyback_lock_candidate = 1;
        }

        /************** Setting packet struct areas in operation ****************/

        accum_pkt = &(new_ptr->pkt.accum);

        if (use_immed_pkt) {
            MPIDI_Pkt_init(accum_pkt, MPIDI_CH3_PKT_ACCUMULATE_IMMED);
        }
        else {
            MPIDI_Pkt_init(accum_pkt, MPIDI_CH3_PKT_ACCUMULATE);
        }

        accum_pkt->addr = (char *) win_ptr->basic_info_table[target_rank].base_addr +
            win_ptr->basic_info_table[target_rank].disp_unit * target_disp;
        accum_pkt->count = target_count;
        accum_pkt->datatype = target_datatype;
        accum_pkt->info.dataloop_size = 0;
        accum_pkt->op = op;
        accum_pkt->target_win_handle = win_ptr->basic_info_table[target_rank].win_handle;
        accum_pkt->source_win_handle = win_ptr->handle;
        accum_pkt->flags = MPIDI_CH3_PKT_FLAG_NONE;
        if (use_immed_pkt) {
            void *src = (void *) origin_addr, *dest = (void *) (accum_pkt->info.data);
            mpi_errno = immed_copy(src, dest, len);
            if (mpi_errno != MPI_SUCCESS)
                MPIU_ERR_POP(mpi_errno);
        }

        /* NOTE: here we backup the original starting address for the entire operation
           on target window in 'original_target_addr', because when actually issuing
           this operation, we may stream this operation and overwrite 'addr' with the
           starting address for the streaming unit. */
        new_ptr->original_target_addr = accum_pkt->addr;

        MPIR_T_PVAR_TIMER_END(RMA, rma_rmaqueue_set);

        mpi_errno = MPIDI_CH3I_Win_enqueue_op(win_ptr, new_ptr);
        if (mpi_errno)
            MPIU_ERR_POP(mpi_errno);

        mpi_errno = MPIDI_CH3I_RMA_Make_progress_target(win_ptr, target_rank, &made_progress);
        if (mpi_errno != MPI_SUCCESS)
            MPIU_ERR_POP(mpi_errno);

        if (MPIR_CVAR_CH3_RMA_OP_POKING_PROGRESS >= 0 &&
            win_ptr->accumulated_ops_cnt >= MPIR_CVAR_CH3_RMA_OP_POKING_PROGRESS) {
            mpi_errno = poke_progress_engine();
            if (mpi_errno != MPI_SUCCESS)
                MPIU_ERR_POP(mpi_errno);
        }

        if (MPIR_CVAR_CH3_RMA_ACTIVE_REQ_THRESHOLD >= 0 &&
            win_ptr->active_req_cnt >= MPIR_CVAR_CH3_RMA_ACTIVE_REQ_THRESHOLD) {
            while (win_ptr->active_req_cnt >= MPIR_CVAR_CH3_RMA_ACTIVE_REQ_THRESHOLD) {
                int local_completed = 0, remote_completed = 0;
                mpi_errno = MPIDI_CH3I_RMA_Cleanup_ops_win(win_ptr, &local_completed,
                                                           &remote_completed);
                if (mpi_errno != MPI_SUCCESS)
                    MPIU_ERR_POP(mpi_errno);
                mpi_errno = poke_progress_engine();
                if (mpi_errno != MPI_SUCCESS)
                    MPIU_ERR_POP(mpi_errno);
            }
        }
    }

  fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_ACCUMULATE);
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Get_accumulate
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_Get_accumulate(const void *origin_addr, int origin_count,
                              MPI_Datatype origin_datatype, void *result_addr, int result_count,
                              MPI_Datatype result_datatype, int target_rank, MPI_Aint target_disp,
                              int target_count, MPI_Datatype target_datatype, MPI_Op op,
                              MPID_Win * win_ptr, MPID_Request * ureq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_msg_sz_t data_sz;
    int rank;
    int dt_contig ATTRIBUTE((unused));
    MPI_Aint dt_true_lb ATTRIBUTE((unused));
    MPID_Datatype *dtp;
    MPIDI_VC_t *orig_vc = NULL, *target_vc = NULL;
    int made_progress = 0;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_GET_ACCUMULATE);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_GET_ACCUMULATE);

    MPIU_ERR_CHKANDJUMP(win_ptr->states.access_state == MPIDI_RMA_NONE,
                        mpi_errno, MPI_ERR_RMA_SYNC, "**rmasync");

    if (target_rank == MPI_PROC_NULL) {
        goto fn_exit;
    }

    MPIDI_Datatype_get_info(target_count, target_datatype, dt_contig, data_sz, dtp, dt_true_lb);

    if (data_sz == 0) {
        goto fn_exit;
    }

    rank = win_ptr->comm_ptr->rank;

    if (win_ptr->shm_allocated == TRUE && target_rank != rank &&
        win_ptr->create_flavor != MPI_WIN_FLAVOR_SHARED) {
        /* check if target is local and shared memory is allocated on window,
         * if so, we directly perform this operation on shared memory region. */

        /* FIXME: Here we decide whether to perform SHM operations by checking if origin and target are on
         * the same node. However, in ch3:sock, even if origin and target are on the same node, they do
         * not within the same SHM region. Here we filter out ch3:sock by checking shm_allocated flag first,
         * which is only set to TRUE when SHM region is allocated in nemesis.
         * In future we need to figure out a way to check if origin and target are in the same "SHM comm".
         */
        MPIDI_Comm_get_vc(win_ptr->comm_ptr, rank, &orig_vc);
        MPIDI_Comm_get_vc(win_ptr->comm_ptr, target_rank, &target_vc);
    }

    /* Do =! rank first (most likely branch?) */
    if (target_rank == rank || win_ptr->create_flavor == MPI_WIN_FLAVOR_SHARED ||
        (win_ptr->shm_allocated == TRUE && orig_vc->node_id == target_vc->node_id)) {
        mpi_errno = MPIDI_CH3I_Shm_get_acc_op(origin_addr, origin_count, origin_datatype,
                                              result_addr, result_count, result_datatype,
                                              target_rank, target_disp, target_count,
                                              target_datatype, op, win_ptr);
        if (mpi_errno)
            MPIU_ERR_POP(mpi_errno);

        if (ureq) {
            /* Complete user request and release the ch3 ref */
            MPID_Request_set_completed(ureq);
            MPID_Request_release(ureq);
        }
    }
    else {
        MPIDI_RMA_Op_t *new_ptr = NULL;

        /* Append the operation to the window's RMA ops queue */
        mpi_errno = MPIDI_CH3I_Win_get_op(win_ptr, &new_ptr);
        if (mpi_errno != MPI_SUCCESS)
            MPIU_ERR_POP(mpi_errno);

        /* TODO: Can we use the MPIDI_RMA_ACC_CONTIG optimization? */

        MPIR_T_PVAR_TIMER_START(RMA, rma_rmaqueue_set);

        if (op == MPI_NO_OP) {
            /* Convert GAcc to a Get */
            MPIDI_CH3_Pkt_get_t *get_pkt;
            MPI_Aint target_type_size;
            size_t len, immed_len;
            int use_immed_resp_pkt = FALSE;
            int is_result_contig, is_target_contig;

            /******************** Setting operation struct areas ***********************/

            new_ptr->origin_addr = result_addr;
            new_ptr->origin_count = result_count;
            new_ptr->origin_datatype = result_datatype;
            new_ptr->target_rank = target_rank;

            /* Remember user request */
            if (ureq) {
                new_ptr->ureq = ureq;
            }

            if (!MPIR_DATATYPE_IS_PREDEFINED(result_datatype)) {
                MPID_Datatype_get_ptr(result_datatype, dtp);
                MPID_Datatype_add_ref(dtp);
                new_ptr->is_dt = 1;
            }
            if (!MPIR_DATATYPE_IS_PREDEFINED(target_datatype)) {
                MPID_Datatype_get_ptr(target_datatype, dtp);
                MPID_Datatype_add_ref(dtp);
                new_ptr->is_dt = 1;
            }

            MPID_Datatype_get_size_macro(target_datatype, target_type_size);
            MPIU_Assign_trunc(len, target_count * target_type_size, size_t);

            MPID_Datatype_is_contig(result_datatype, &is_result_contig);
            MPID_Datatype_is_contig(target_datatype, &is_target_contig);

            /* Judge if we can use IMMED data response packet */
            if (!new_ptr->is_dt && is_result_contig && is_target_contig) {
                MPIU_Assign_trunc(immed_len,
                                  (MPIDI_RMA_IMMED_BYTES / target_type_size) * target_type_size,
                                  size_t);
                if (len <= immed_len)
                    use_immed_resp_pkt = TRUE;
            }

            /* Judge if this operation is a piggyback candidate */
            if (!new_ptr->is_dt) {
                /* FIXME: currently we only piggyback LOCK flag with op using predefined datatypes
                 * for both origin and target data. We should extend this optimization to derived
                 * datatypes as well. */
                new_ptr->piggyback_lock_candidate = 1;
            }

            /************** Setting packet struct areas in operation ****************/

            get_pkt = &(new_ptr->pkt.get);
            MPIDI_Pkt_init(get_pkt, MPIDI_CH3_PKT_GET);
            get_pkt->addr = (char *) win_ptr->basic_info_table[target_rank].base_addr +
                win_ptr->basic_info_table[target_rank].disp_unit * target_disp;
            get_pkt->count = target_count;
            get_pkt->datatype = target_datatype;
            get_pkt->info.dataloop_size = 0;
            get_pkt->target_win_handle = win_ptr->basic_info_table[target_rank].win_handle;
            get_pkt->flags = MPIDI_CH3_PKT_FLAG_NONE;
            if (use_immed_resp_pkt == TRUE)
                get_pkt->flags |= MPIDI_CH3_PKT_FLAG_RMA_IMMED_RESP;
        }

        else {
            MPIDI_CH3_Pkt_get_accum_t *get_accum_pkt;
            MPI_Aint origin_type_size;
            size_t immed_len, orig_len;
            int use_immed_pkt = FALSE;
            int is_origin_contig, is_target_contig, is_result_contig;
            MPI_Aint stream_elem_count, stream_unit_count;
            MPI_Aint predefined_dtp_size, predefined_dtp_count, predefined_dtp_extent;
            MPID_Datatype *origin_dtp = NULL, *target_dtp = NULL, *result_dtp = NULL;
            int i;

            /******************** Setting operation struct areas ***********************/

            new_ptr->origin_addr = (void *) origin_addr;
            new_ptr->origin_count = origin_count;
            new_ptr->origin_datatype = origin_datatype;
            new_ptr->result_addr = result_addr;
            new_ptr->result_count = result_count;
            new_ptr->result_datatype = result_datatype;
            new_ptr->target_rank = target_rank;

            /* Remember user request */
            if (ureq) {
                new_ptr->ureq = ureq;
            }

            /* if source or target datatypes are derived, increment their
             * reference counts */
            if (!MPIR_DATATYPE_IS_PREDEFINED(origin_datatype)) {
                MPID_Datatype_get_ptr(origin_datatype, origin_dtp);
                new_ptr->is_dt = 1;
            }
            if (!MPIR_DATATYPE_IS_PREDEFINED(result_datatype)) {
                MPID_Datatype_get_ptr(result_datatype, target_dtp);
                new_ptr->is_dt = 1;
            }
            if (!MPIR_DATATYPE_IS_PREDEFINED(target_datatype)) {
                MPID_Datatype_get_ptr(target_datatype, result_dtp);
                new_ptr->is_dt = 1;
            }

            MPID_Datatype_get_size_macro(origin_datatype, origin_type_size);
            MPIU_Assign_trunc(orig_len, origin_count * origin_type_size, size_t);

            /* Get size and count for predefined datatype elements */
            if (MPIR_DATATYPE_IS_PREDEFINED(origin_datatype)) {
                predefined_dtp_size = origin_type_size;
                predefined_dtp_count = origin_count;
                MPID_Datatype_get_extent_macro(origin_datatype, predefined_dtp_extent);
            }
            else {
                MPIU_Assert(origin_dtp->basic_type != MPI_DATATYPE_NULL);
                MPID_Datatype_get_size_macro(origin_dtp->basic_type, predefined_dtp_size);
                predefined_dtp_count = orig_len / predefined_dtp_size;
                MPID_Datatype_get_extent_macro(origin_dtp->basic_type, predefined_dtp_extent);
            }
            MPIU_Assert(predefined_dtp_count > 0 && predefined_dtp_size > 0 &&
                        predefined_dtp_extent > 0);

            /* Calculate number of predefined elements in each stream unit, and
             * total number of stream units. */
            stream_elem_count = MPIDI_CH3U_Acc_stream_size / predefined_dtp_extent;
            stream_unit_count = (predefined_dtp_count - 1) / stream_elem_count + 1;
            MPIU_Assert(stream_elem_count > 0 && stream_unit_count > 0);

            for (i = 0; i < stream_unit_count; i++) {
                if (origin_dtp != NULL) {
                    MPID_Datatype_add_ref(origin_dtp);
                }
                if (target_dtp != NULL) {
                    MPID_Datatype_add_ref(target_dtp);
                }
                if (result_dtp != NULL) {
                    MPID_Datatype_add_ref(result_dtp);
                }
            }

            MPID_Datatype_is_contig(origin_datatype, &is_origin_contig);
            MPID_Datatype_is_contig(target_datatype, &is_target_contig);
            MPID_Datatype_is_contig(result_datatype, &is_result_contig);

            /* Judge if we can use IMMED data packet */
            if (!new_ptr->is_dt && is_origin_contig && is_target_contig && is_result_contig) {
                MPIU_Assign_trunc(immed_len,
                                  (MPIDI_RMA_IMMED_BYTES / origin_type_size) * origin_type_size,
                                  size_t);
                if (orig_len <= immed_len)
                    use_immed_pkt = TRUE;
            }

            /* Judge if this operation is a piggyback candidate */
            if (!new_ptr->is_dt) {
                /* FIXME: currently we only piggyback LOCK flag with op using predefined datatypes
                 * for origin, target and result data. We should extend this optimization to derived
                 * datatypes as well. */
                if (orig_len <= MPIR_CVAR_CH3_RMA_OP_PIGGYBACK_LOCK_DATA_SIZE)
                    new_ptr->piggyback_lock_candidate = 1;
            }

            /************** Setting packet struct areas in operation ****************/

            get_accum_pkt = &(new_ptr->pkt.get_accum);

            if (use_immed_pkt) {
                MPIDI_Pkt_init(get_accum_pkt, MPIDI_CH3_PKT_GET_ACCUM_IMMED);
            }
            else {
                MPIDI_Pkt_init(get_accum_pkt, MPIDI_CH3_PKT_GET_ACCUM);
            }

            get_accum_pkt->addr = (char *) win_ptr->basic_info_table[target_rank].base_addr +
                win_ptr->basic_info_table[target_rank].disp_unit * target_disp;
            get_accum_pkt->count = target_count;
            get_accum_pkt->datatype = target_datatype;
            get_accum_pkt->info.dataloop_size = 0;
            get_accum_pkt->op = op;
            get_accum_pkt->target_win_handle = win_ptr->basic_info_table[target_rank].win_handle;
            get_accum_pkt->flags = MPIDI_CH3_PKT_FLAG_NONE;
            if (use_immed_pkt) {
                void *src = (void *) origin_addr, *dest = (void *) (get_accum_pkt->info.data);
                mpi_errno = immed_copy(src, dest, orig_len);
                if (mpi_errno != MPI_SUCCESS)
                    MPIU_ERR_POP(mpi_errno);
            }

            /* NOTE: here we backup the original starting address for the entire operation
               on target window in 'original_target_addr', because when actually issuing
               this operation, we may stream this operation and overwrite 'addr' with the
               starting address for the streaming unit. */
            new_ptr->original_target_addr = get_accum_pkt->addr;
        }

        MPIR_T_PVAR_TIMER_END(RMA, rma_rmaqueue_set);

        mpi_errno = MPIDI_CH3I_Win_enqueue_op(win_ptr, new_ptr);
        if (mpi_errno)
            MPIU_ERR_POP(mpi_errno);

        mpi_errno = MPIDI_CH3I_RMA_Make_progress_target(win_ptr, target_rank, &made_progress);
        if (mpi_errno != MPI_SUCCESS)
            MPIU_ERR_POP(mpi_errno);

        if (MPIR_CVAR_CH3_RMA_OP_POKING_PROGRESS >= 0 &&
            win_ptr->accumulated_ops_cnt >= MPIR_CVAR_CH3_RMA_OP_POKING_PROGRESS) {
            mpi_errno = poke_progress_engine();
            if (mpi_errno != MPI_SUCCESS)
                MPIU_ERR_POP(mpi_errno);
        }

        if (MPIR_CVAR_CH3_RMA_ACTIVE_REQ_THRESHOLD >= 0 &&
            win_ptr->active_req_cnt >= MPIR_CVAR_CH3_RMA_ACTIVE_REQ_THRESHOLD) {
            while (win_ptr->active_req_cnt >= MPIR_CVAR_CH3_RMA_ACTIVE_REQ_THRESHOLD) {
                int local_completed = 0, remote_completed = 0;
                mpi_errno = MPIDI_CH3I_RMA_Cleanup_ops_win(win_ptr, &local_completed,
                                                           &remote_completed);
                if (mpi_errno != MPI_SUCCESS)
                    MPIU_ERR_POP(mpi_errno);
                mpi_errno = poke_progress_engine();
                if (mpi_errno != MPI_SUCCESS)
                    MPIU_ERR_POP(mpi_errno);
            }
        }
    }

  fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_GET_ACCUMULATE);
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME MPIDI_Put
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_Put(const void *origin_addr, int origin_count, MPI_Datatype
              origin_datatype, int target_rank, MPI_Aint target_disp,
              int target_count, MPI_Datatype target_datatype, MPID_Win * win_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    MPIDI_STATE_DECL(MPID_STATE_MPIDI_PUT);
    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_PUT);

    mpi_errno = MPIDI_CH3I_Put(origin_addr, origin_count, origin_datatype,
                               target_rank, target_disp, target_count, target_datatype,
                               win_ptr, NULL);

  fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_PUT);
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

#undef FUNCNAME
#define FUNCNAME MPIDI_Get
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_Get(void *origin_addr, int origin_count, MPI_Datatype
              origin_datatype, int target_rank, MPI_Aint target_disp,
              int target_count, MPI_Datatype target_datatype, MPID_Win * win_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    MPIDI_STATE_DECL(MPID_STATE_MPIDI_GET);
    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_GET);

    mpi_errno = MPIDI_CH3I_Get(origin_addr, origin_count, origin_datatype,
                               target_rank, target_disp, target_count, target_datatype,
                               win_ptr, NULL);

  fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_GET);
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

#undef FUNCNAME
#define FUNCNAME MPIDI_Accumulate
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_Accumulate(const void *origin_addr, int origin_count, MPI_Datatype
                     origin_datatype, int target_rank, MPI_Aint target_disp,
                     int target_count, MPI_Datatype target_datatype, MPI_Op op, MPID_Win * win_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    MPIDI_STATE_DECL(MPID_STATE_MPIDI_ACCUMULATE);
    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_ACCUMULATE);

    mpi_errno = MPIDI_CH3I_Accumulate(origin_addr, origin_count, origin_datatype,
                                      target_rank, target_disp, target_count, target_datatype,
                                      op, win_ptr, NULL);

  fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_ACCUMULATE);
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

#undef FUNCNAME
#define FUNCNAME MPIDI_Get_accumulate
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_Get_accumulate(const void *origin_addr, int origin_count,
                         MPI_Datatype origin_datatype, void *result_addr, int result_count,
                         MPI_Datatype result_datatype, int target_rank, MPI_Aint target_disp,
                         int target_count, MPI_Datatype target_datatype, MPI_Op op,
                         MPID_Win * win_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    MPIDI_STATE_DECL(MPID_STATE_MPIDI_GET_ACCUMULATE);
    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_GET_ACCUMULATE);

    mpi_errno = MPIDI_CH3I_Get_accumulate(origin_addr, origin_count, origin_datatype,
                                          result_addr, result_count, result_datatype,
                                          target_rank, target_disp, target_count,
                                          target_datatype, op, win_ptr, NULL);

  fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_GET_ACCUMULATE);
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME MPIDI_Compare_and_swap
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_Compare_and_swap(const void *origin_addr, const void *compare_addr,
                           void *result_addr, MPI_Datatype datatype, int target_rank,
                           MPI_Aint target_disp, MPID_Win * win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    int rank;
    MPIDI_VC_t *orig_vc = NULL, *target_vc = NULL;
    int made_progress = 0;

    MPIDI_STATE_DECL(MPID_STATE_MPIDI_COMPARE_AND_SWAP);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_COMPARE_AND_SWAP);

    MPIU_ERR_CHKANDJUMP(win_ptr->states.access_state == MPIDI_RMA_NONE,
                        mpi_errno, MPI_ERR_RMA_SYNC, "**rmasync");

    if (target_rank == MPI_PROC_NULL) {
        goto fn_exit;
    }

    rank = win_ptr->comm_ptr->rank;

    if (win_ptr->shm_allocated == TRUE && target_rank != rank &&
        win_ptr->create_flavor != MPI_WIN_FLAVOR_SHARED) {
        /* check if target is local and shared memory is allocated on window,
         * if so, we directly perform this operation on shared memory region. */

        /* FIXME: Here we decide whether to perform SHM operations by checking if origin and target are on
         * the same node. However, in ch3:sock, even if origin and target are on the same node, they do
         * not within the same SHM region. Here we filter out ch3:sock by checking shm_allocated flag first,
         * which is only set to TRUE when SHM region is allocated in nemesis.
         * In future we need to figure out a way to check if origin and target are in the same "SHM comm".
         */
        MPIDI_Comm_get_vc(win_ptr->comm_ptr, rank, &orig_vc);
        MPIDI_Comm_get_vc(win_ptr->comm_ptr, target_rank, &target_vc);
    }

    /* The datatype must be predefined, and one of: C integer, Fortran integer,
     * Logical, Multi-language types, or Byte.  This is checked above the ADI,
     * so there's no need to check it again here. */

    /* FIXME: For shared memory windows, we should provide an implementation
     * that uses a processor atomic operation. */
    if (target_rank == rank || win_ptr->create_flavor == MPI_WIN_FLAVOR_SHARED ||
        (win_ptr->shm_allocated == TRUE && orig_vc->node_id == target_vc->node_id)) {
        mpi_errno = MPIDI_CH3I_Shm_cas_op(origin_addr, compare_addr, result_addr,
                                          datatype, target_rank, target_disp, win_ptr);
        if (mpi_errno)
            MPIU_ERR_POP(mpi_errno);
    }
    else {
        MPIDI_RMA_Op_t *new_ptr = NULL;
        MPIDI_CH3_Pkt_cas_t *cas_pkt = NULL;
        MPI_Aint type_size;
        void *src = NULL, *dest = NULL;

        /* Append this operation to the RMA ops queue */
        mpi_errno = MPIDI_CH3I_Win_get_op(win_ptr, &new_ptr);
        if (mpi_errno != MPI_SUCCESS)
            MPIU_ERR_POP(mpi_errno);

        MPIR_T_PVAR_TIMER_START(RMA, rma_rmaqueue_set);

        /******************** Setting operation struct areas ***********************/

        new_ptr->origin_addr = (void *) origin_addr;
        new_ptr->origin_count = 1;
        new_ptr->origin_datatype = datatype;
        new_ptr->result_addr = result_addr;
        new_ptr->result_datatype = datatype;
        new_ptr->compare_addr = (void *) compare_addr;
        new_ptr->compare_datatype = datatype;
        new_ptr->target_rank = target_rank;
        new_ptr->piggyback_lock_candidate = 1;  /* CAS is always able to piggyback LOCK */

        /************** Setting packet struct areas in operation ****************/

        cas_pkt = &(new_ptr->pkt.cas);
        MPIDI_Pkt_init(cas_pkt, MPIDI_CH3_PKT_CAS_IMMED);
        cas_pkt->addr = (char *) win_ptr->basic_info_table[target_rank].base_addr +
            win_ptr->basic_info_table[target_rank].disp_unit * target_disp;
        cas_pkt->datatype = datatype;
        cas_pkt->target_win_handle = win_ptr->basic_info_table[target_rank].win_handle;
        cas_pkt->flags = MPIDI_CH3_PKT_FLAG_NONE;

        /* REQUIRE: All datatype arguments must be of the same, builtin
         * type and counts must be 1. */
        MPID_Datatype_get_size_macro(datatype, type_size);
        MPIU_Assert(type_size <= sizeof(MPIDI_CH3_CAS_Immed_u));

        src = (void *) origin_addr, dest = (void *) (&(cas_pkt->origin_data));
        mpi_errno = immed_copy(src, dest, type_size);
        if (mpi_errno != MPI_SUCCESS)
            MPIU_ERR_POP(mpi_errno);

        src = (void *) compare_addr, dest = (void *) (&(cas_pkt->compare_data));
        mpi_errno = immed_copy(src, dest, type_size);
        if (mpi_errno != MPI_SUCCESS)
            MPIU_ERR_POP(mpi_errno);

        MPIR_T_PVAR_TIMER_END(RMA, rma_rmaqueue_set);

        mpi_errno = MPIDI_CH3I_Win_enqueue_op(win_ptr, new_ptr);
        if (mpi_errno)
            MPIU_ERR_POP(mpi_errno);

        mpi_errno = MPIDI_CH3I_RMA_Make_progress_target(win_ptr, target_rank, &made_progress);
        if (mpi_errno != MPI_SUCCESS)
            MPIU_ERR_POP(mpi_errno);

        if (MPIR_CVAR_CH3_RMA_OP_POKING_PROGRESS >= 0 &&
            win_ptr->accumulated_ops_cnt >= MPIR_CVAR_CH3_RMA_OP_POKING_PROGRESS) {
            mpi_errno = poke_progress_engine();
            if (mpi_errno != MPI_SUCCESS)
                MPIU_ERR_POP(mpi_errno);
        }

        if (MPIR_CVAR_CH3_RMA_ACTIVE_REQ_THRESHOLD >= 0 &&
            win_ptr->active_req_cnt >= MPIR_CVAR_CH3_RMA_ACTIVE_REQ_THRESHOLD) {
            while (win_ptr->active_req_cnt >= MPIR_CVAR_CH3_RMA_ACTIVE_REQ_THRESHOLD) {
                int local_completed = 0, remote_completed = 0;
                mpi_errno = MPIDI_CH3I_RMA_Cleanup_ops_win(win_ptr, &local_completed,
                                                           &remote_completed);
                if (mpi_errno != MPI_SUCCESS)
                    MPIU_ERR_POP(mpi_errno);
                mpi_errno = poke_progress_engine();
                if (mpi_errno != MPI_SUCCESS)
                    MPIU_ERR_POP(mpi_errno);
            }
        }
    }

  fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_COMPARE_AND_SWAP);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME MPIDI_Fetch_and_op
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_Fetch_and_op(const void *origin_addr, void *result_addr,
                       MPI_Datatype datatype, int target_rank,
                       MPI_Aint target_disp, MPI_Op op, MPID_Win * win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    int rank;
    MPIDI_VC_t *orig_vc = NULL, *target_vc = NULL;
    int made_progress = 0;

    MPIDI_STATE_DECL(MPID_STATE_MPIDI_FETCH_AND_OP);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_FETCH_AND_OP);

    MPIU_ERR_CHKANDJUMP(win_ptr->states.access_state == MPIDI_RMA_NONE,
                        mpi_errno, MPI_ERR_RMA_SYNC, "**rmasync");

    if (target_rank == MPI_PROC_NULL) {
        goto fn_exit;
    }

    rank = win_ptr->comm_ptr->rank;

    if (win_ptr->shm_allocated == TRUE && target_rank != rank &&
        win_ptr->create_flavor != MPI_WIN_FLAVOR_SHARED) {
        /* check if target is local and shared memory is allocated on window,
         * if so, we directly perform this operation on shared memory region. */

        /* FIXME: Here we decide whether to perform SHM operations by checking if origin and target are on
         * the same node. However, in ch3:sock, even if origin and target are on the same node, they do
         * not within the same SHM region. Here we filter out ch3:sock by checking shm_allocated flag first,
         * which is only set to TRUE when SHM region is allocated in nemesis.
         * In future we need to figure out a way to check if origin and target are in the same "SHM comm".
         */
        MPIDI_Comm_get_vc(win_ptr->comm_ptr, rank, &orig_vc);
        MPIDI_Comm_get_vc(win_ptr->comm_ptr, target_rank, &target_vc);
    }

    /* The datatype and op must be predefined.  This is checked above the ADI,
     * so there's no need to check it again here. */

    /* FIXME: For shared memory windows, we should provide an implementation
     * that uses a processor atomic operation. */
    if (target_rank == rank || win_ptr->create_flavor == MPI_WIN_FLAVOR_SHARED ||
        (win_ptr->shm_allocated == TRUE && orig_vc->node_id == target_vc->node_id)) {
        mpi_errno = MPIDI_CH3I_Shm_fop_op(origin_addr, result_addr, datatype,
                                          target_rank, target_disp, op, win_ptr);
        if (mpi_errno)
            MPIU_ERR_POP(mpi_errno);
    }
    else {
        MPIDI_RMA_Op_t *new_ptr = NULL;

        /* Append this operation to the RMA ops queue */
        mpi_errno = MPIDI_CH3I_Win_get_op(win_ptr, &new_ptr);
        if (mpi_errno != MPI_SUCCESS)
            MPIU_ERR_POP(mpi_errno);

        MPIR_T_PVAR_TIMER_START(RMA, rma_rmaqueue_set);

        if (op == MPI_NO_OP) {
            /* Convert FOP to a Get */
            MPIDI_CH3_Pkt_get_t *get_pkt;
            MPI_Aint target_type_size;
            size_t immed_len;
            int use_immed_resp_pkt = FALSE;
            int is_contig;

            /******************** Setting operation struct areas ***********************/

            new_ptr->origin_addr = result_addr;
            new_ptr->origin_count = 1;
            new_ptr->origin_datatype = datatype;
            new_ptr->target_rank = target_rank;
            new_ptr->piggyback_lock_candidate = 1;

            /************** Setting packet struct areas in operation ****************/

            MPID_Datatype_get_size_macro(datatype, target_type_size);
            MPIU_Assert(target_type_size <= sizeof(MPIDI_CH3_FOP_Immed_u));

            MPID_Datatype_is_contig(datatype, &is_contig);

            if (is_contig) {
                /* Judege if we can use IMMED data for response packet */
                MPIU_Assign_trunc(immed_len,
                                  (MPIDI_RMA_IMMED_BYTES / target_type_size) * target_type_size,
                                  size_t);
                if (target_type_size <= immed_len)
                    use_immed_resp_pkt = TRUE;
            }

            get_pkt = &(new_ptr->pkt.get);
            MPIDI_Pkt_init(get_pkt, MPIDI_CH3_PKT_GET);
            get_pkt->addr = (char *) win_ptr->basic_info_table[target_rank].base_addr +
                win_ptr->basic_info_table[target_rank].disp_unit * target_disp;
            get_pkt->count = 1;
            get_pkt->datatype = datatype;
            get_pkt->info.dataloop_size = 0;
            get_pkt->target_win_handle = win_ptr->basic_info_table[target_rank].win_handle;
            get_pkt->flags = MPIDI_CH3_PKT_FLAG_NONE;
            if (use_immed_resp_pkt == TRUE)
                get_pkt->flags |= MPIDI_CH3_PKT_FLAG_RMA_IMMED_RESP;
        }
        else {
            MPIDI_CH3_Pkt_fop_t *fop_pkt;
            MPI_Aint type_size;
            size_t immed_len;
            int use_immed_pkt = FALSE;
            int is_contig;

            /******************** Setting operation struct areas ***********************/

            new_ptr->origin_addr = (void *) origin_addr;
            new_ptr->origin_count = 1;
            new_ptr->origin_datatype = datatype;
            new_ptr->result_addr = result_addr;
            new_ptr->result_datatype = datatype;
            new_ptr->target_rank = target_rank;
            new_ptr->piggyback_lock_candidate = 1;

            /************** Setting packet struct areas in operation ****************/

            MPID_Datatype_get_size_macro(datatype, type_size);
            MPIU_Assert(type_size <= sizeof(MPIDI_CH3_FOP_Immed_u));

            MPID_Datatype_is_contig(datatype, &is_contig);

            if (is_contig) {
                /* Judge if we can use IMMED data packet */
                MPIU_Assign_trunc(immed_len,
                                  (MPIDI_RMA_IMMED_BYTES / type_size) * type_size, size_t);
                if (type_size <= immed_len) {
                    use_immed_pkt = TRUE;
                }
            }

            fop_pkt = &(new_ptr->pkt.fop);

            if (use_immed_pkt) {
                MPIDI_Pkt_init(fop_pkt, MPIDI_CH3_PKT_FOP_IMMED);
            }
            else {
                MPIDI_Pkt_init(fop_pkt, MPIDI_CH3_PKT_FOP);
            }
            fop_pkt->addr = (char *) win_ptr->basic_info_table[target_rank].base_addr +
                win_ptr->basic_info_table[target_rank].disp_unit * target_disp;
            fop_pkt->datatype = datatype;
            fop_pkt->op = op;
            fop_pkt->target_win_handle = win_ptr->basic_info_table[target_rank].win_handle;
            fop_pkt->flags = MPIDI_CH3_PKT_FLAG_NONE;
            if (use_immed_pkt) {
                void *src = (void *) origin_addr, *dest = (void *) (fop_pkt->info.data);
                mpi_errno = immed_copy(src, dest, type_size);
                if (mpi_errno != MPI_SUCCESS)
                    MPIU_ERR_POP(mpi_errno);
            }
        }

        MPIR_T_PVAR_TIMER_END(RMA, rma_rmaqueue_set);

        mpi_errno = MPIDI_CH3I_Win_enqueue_op(win_ptr, new_ptr);
        if (mpi_errno)
            MPIU_ERR_POP(mpi_errno);

        mpi_errno = MPIDI_CH3I_RMA_Make_progress_target(win_ptr, target_rank, &made_progress);
        if (mpi_errno != MPI_SUCCESS)
            MPIU_ERR_POP(mpi_errno);

        if (MPIR_CVAR_CH3_RMA_OP_POKING_PROGRESS >= 0 &&
            win_ptr->accumulated_ops_cnt >= MPIR_CVAR_CH3_RMA_OP_POKING_PROGRESS) {
            mpi_errno = poke_progress_engine();
            if (mpi_errno != MPI_SUCCESS)
                MPIU_ERR_POP(mpi_errno);
        }

        if (MPIR_CVAR_CH3_RMA_ACTIVE_REQ_THRESHOLD >= 0 &&
            win_ptr->active_req_cnt >= MPIR_CVAR_CH3_RMA_ACTIVE_REQ_THRESHOLD) {
            while (win_ptr->active_req_cnt >= MPIR_CVAR_CH3_RMA_ACTIVE_REQ_THRESHOLD) {
                int local_completed = 0, remote_completed = 0;
                mpi_errno = MPIDI_CH3I_RMA_Cleanup_ops_win(win_ptr, &local_completed,
                                                           &remote_completed);
                if (mpi_errno != MPI_SUCCESS)
                    MPIU_ERR_POP(mpi_errno);
                mpi_errno = poke_progress_engine();
                if (mpi_errno != MPI_SUCCESS)
                    MPIU_ERR_POP(mpi_errno);
            }
        }
    }

  fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_FETCH_AND_OP);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

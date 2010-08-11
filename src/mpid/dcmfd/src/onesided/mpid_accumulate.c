/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file src/onesided/mpid_accumulate.c
 * \brief MPI-DCMF MPI_Accumulate functionality
 */

#include "mpid_onesided.h"

#define DATATYPE_ADDITIONAL(dt)	\
	(dt == MPI_FLOAT_INT ||	\
	dt == MPI_DOUBLE_INT ||	\
	dt == MPI_LONG_INT ||	\
	dt == MPI_SHORT_INT ||	\
	dt == MPI_LONG_DOUBLE_INT)

#define DATATYPE_PREDEFINED(dt)	\
        (HANDLE_GET_KIND(dt) == HANDLE_KIND_BUILTIN ||	\
				DATATYPE_ADDITIONAL(dt))

/**
 * \brief Utility routine to provide accumulate function locally.
 *
 * Utility routine to provide accumulate function locally.
 *
 * Called from \e target_accumulate(), or \e MPID_Accumulate() in the
 * case of local operation (to self).
 *
 * Non-contiguous datatypes are handled by \e MPID_Accumulate(),
 * which splits the data into contiguous regions. Note that MPICH2
 * states that datatypes used with MPI_Accumulate must be "monotonic",
 * they must be of only one underlying type.
 *
 * We have to cast the 'src' param to '(void *)' in our calls to the
 * operand function because MPI_User_function is declared without the 'const'
 * even though MPID_User_function union elements have the 'const'.
 * Also, cxx_call_op_fn is missing the 'const'.  Even with that, we still
 * have to cast when assigning to uop from op_ptr, because of the way
 * f77_function and c_function are declared.
 *
 * \param[in] win	Pointer to MPID_Win object
 * \param[in] dst	Pointer to destination buffer
 * \param[in] src	Pointer to source buffer
 * \param[in] lpid	lpid of origin
 * \param[in] dt	Local datatype
 * \param[in] op	Operand
 * \param[in] num	number of dt elements
 * \return nothing
 *
 */
static void local_accumulate(MPID_Win *win, char *dst, const char *src,
                int lpid, MPI_Datatype dt, MPI_Op op, int num) {
        MPI_User_function *uop;
        int is_cxx = 0;

        if (HANDLE_GET_KIND(op) == HANDLE_KIND_BUILTIN) {
                uop = MPIR_Op_table[op%16 - 1];
        } else {
                MPID_Op *op_ptr;

                /*
                 * This case can only happen if accumulate originated locally.
                 */
                MPID_Op_get_ptr(op, op_ptr);
                if (op_ptr->language == MPID_LANG_C) {
                        uop = (MPI_User_function *)op_ptr->function.c_function;
#ifdef HAVE_CXX_BINDING
                } else if (op_ptr->language == MPID_LANG_CXX) {
                        uop = (MPI_User_function *)op_ptr->function.c_function;
                        ++is_cxx;
#endif /* HAVE_CXX_BINDING */
                } else {
                        uop = (MPI_User_function *)op_ptr->function.f77_function;
                }
        }
        /* MPI_REPLACE was filtered-out in MPID_Accumulate() */
        /* Also, MPID_Accumulate() only passes builtin types to us */
        /* builtin implies contiguous */
#ifdef HAVE_CXX_BINDING
        if (is_cxx)
                (*MPIR_Process.cxx_call_op_fn)((void *)src, dst, num, dt, uop);
        else
#endif /* HAVE_CXX_BINDING */
                (*uop)((void *)src, dst, &num, &dt);
}

/**
 * \brief Utility routine to provide accumulate function on target.
 *
 * Utility routine to provide accumulate function on target.
 * Updates RMA count.
 *
 * w0 = MPID_MSGTYPE_ACC (not used)
 * w1 = Window handle
 * w2 = Rank of origin
 * w3 = Destination buffer address
 * w4 = "eltype" datatype handle (must be builtin)
 * w5 = Operand
 * w6 = number of datatype elements
 * w7 = (not used)
 *
 * Called from "long message" ACCUMULATE completion callback
 * or "short message" ACCUMULATE receive callback.
 *
 * \param[in] mi	MPIDU_Onesided_info_t for accumulate, as described above
 * \param[in] src	Pointer to source buffer
 * \param[in] lpid	lpid of origin
 * \return nothing
 */
void target_accumulate(MPIDU_Onesided_info_t *mi,
                                        const char *src, int lpid) {
        MPID_Win *win = NULL;

        MPID_Win_get_ptr((MPI_Win)mi->mpid_info_w1, win);
        MPID_assert_debug(win != NULL);
        local_accumulate(win, (char *)mi->mpid_info_w3, src,
                mi->mpid_info_w2, mi->mpid_info_w4, mi->mpid_info_w5, mi->mpid_info_w6);
        rma_recvs_cb(win, mi->mpid_info_w2, lpid, 1);
}

/**
 * \brief Callback for Accumulate recv completion
 *
 * "Message receive completion" callback used for MPID_MSGTYPE_ACC
 * to implement the accumulate function. Decodes data from request
 * cache object, frees request, does accumulate, and updates RMA count.
 *
 * Used for "long message" ACCUMULATE.
 *
 * To use this callback, the "xtra" info (DCQuad) must
 * be filled as follows:
 *
 * - \e w0 - ignored
 * - \e w1 - ignored
 * - \e w2 - (int *)multi-struct buffer (int *, DCQuad[], data)
 * - \e w3 - origin lpid
 *
 * \param[in] v	Pointer to DCMF request object
 * \return nothing
 *
 * \ref msginfo_usage
 */
void accum_cb(void *v, DCMF_Error_t *e) {
        MPIDU_Onesided_info_t *info;
        MPIDU_Onesided_xtra_t xtra;
        char *buf;

        MPIDU_free_req((DCMF_Request_t *)v, &xtra);
        info = (MPIDU_Onesided_info_t *)xtra.mpid_xtra_w2;
        buf = (char *)(info + 1);

        target_accumulate(info, buf, (int)xtra.mpid_xtra_w3);
        MPIDU_FREE(info, e, "accum_cb");
}

/**
 * \page accum_design MPID_Accumulate Design
 *
 * A MPID_Accumulate sequence is as follows:
 *
 * <B>Origin node calls MPI_Accumulate</B>
 *
 * - A sanity-check is done to
 * ensure that the window is in a valid state to initiate
 * an accumulate RMA operation.
 * These checks include testing that
 * the epoch currently in affect is not \e NONE or \e POST.
 * Additionally, the target node is checked to ensure it is
 * currently a legitimate target of an RMA operation.
 * - If the target node is the origin node, and the epoch type
 * is \e LOCK, then require that the local lock be acquired.
 * - If the operand is MPI_REPLACE, then perform a Put instead.
 * - Require that the oprand be a built-in operand.
 * - If the origin datatype is non-contiguous, allocate a buffer
 * and pack the origin data into a contiguous buffer.
 * - If the target datatype is not built-in, send the target
 * datatype to the target node for caching there (sending of
 * a particular datatype to a particular target is done only
 * once per job).
 *  - Send a message of type MPID_MSGTYPE_DT_MAP which contains
 * the datatype map structure as created by make_dt_map_vec().
 *  - Send a message of type MPID_MSGTYPE_DT_IOV which contains
 * the datatype iov structure, also created by make_dt_map_vec().
 * - Create the msg info for the accumulate operation.
 * - Call DCMF_Send to send the data and associated parameters
 * to the target node.
 * - Increment the RMA operations count for target node.
 * - Wait for all sends to complete, calling advance in the loop.
 *
 * <B>Target node invokes the receive callback</B>
 *
 * If the datatype was sent, cases \e MPID_MSGTYPE_DT_MAP and
 * \e MPID_MSGTYPE_DT_IOV will be invoked, in sequence.
 *
 * Case: MPID_MSGTYPE_DT_MAP
 *
 * - Sanity-check the message by testing for msginfo count of 1.
 * - Allocate space for the map and store its pointer in the datatype
 * cache element.
 * - Receive the remote map contents.
 *
 * Case: MPID_MSGTYPE_DT_IOV
 *
 * - Sanity-check the message by testing for msginfo count of 1.
 * - Allocate a iov buffer and store its pointer in the datatype
 * cache element's dataloop field.
 * - Receive the remote datatype iov contents.
 *
 * Case: MPID_MSGTYPE_ACC
 *
 * - Sanity-check message...
 * - Setup operand function pointer.
 * - Invoke operand from \e MPIR_Op_table on received data and
 * specified target window buffer.
 * - Increment counter of received RMA operations.
 */
/// \cond NOT_REAL_CODE
#undef FUNCNAME
#define FUNCNAME MPID_Accumulate
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
/// \endcond
/**
 * \brief MPI-DCMF glue for MPI_ACCUMULATE function
 *
 * Perform DEST = DEST (op) SOURCE for \e origin_count number of
 * \e origin_datatype at \e origin_addr
 * to node \e target_rank into \e target_count number of \e target_datatype
 * into window location \e target_disp offset (window displacement units)
 *
 * According to the MPI Specification:
 *
 *	Each datatype argument must be a predefined datatype or
 *	a derived datatype, where all basic components are of the
 *	same predefined datatype. Both datatype arguments must be
 *	constructed from the same predefined datatype.
 *
 * \param[in] origin_addr	Source buffer
 * \param[in] origin_count	Number of datatype elements
 * \param[in] origin_datatype	Source datatype
 * \param[in] target_rank	Destination rank (target)
 * \param[in] target_disp	Displacement factor in target buffer
 * \param[in] target_count	Number of target datatype elements
 * \param[in] target_datatype	Destination datatype
 * \param[in] op		Operand to perform
 * \param[in] win_ptr		Window
 * \return MPI_SUCCESS, MPI_ERR_RMA_SYNC, MPI_ERR_OP,
 *	or error returned from MPIR_Localcopy, MPID_Segment_init,
 *	mpid_queue_datatype, or DCMF_Send.
 *
 * \ref msginfo_usage\n
 * \ref accum_design
 */
int MPID_Accumulate(void *origin_addr, int origin_count,
        MPI_Datatype origin_datatype, int target_rank,
        MPI_Aint target_disp, int target_count,
        MPI_Datatype target_datatype, MPI_Op op, MPID_Win *win_ptr)
{
        int mpi_errno = MPI_SUCCESS;
        int dt_contig, rank;
        MPID_Datatype *dtp;
        MPI_Aint dt_true_lb;
        MPIDI_msg_sz_t data_sz;
        int lpid;
        mpid_dt_info dti = {0};
        int i, j;
        char *buf;
        int sent = 0;
        MPIDU_Onesided_xtra_t xtra = {0};
        int *refp = NULL;
        DCMF_Callback_t cb_send;
        char *s, *dd;
        DCMF_Request_t *reqp;
        MPIDU_Onesided_info_t *info;
        DCMF_Consistency consistency = win_ptr->_dev.my_cstcy;
        MPID_MPI_STATE_DECL(MPID_STATE_MPID_WIN_ACCUMULATE);

        MPID_MPI_FUNC_ENTER(MPID_STATE_MPID_WIN_ACCUMULATE);

        if (win_ptr->_dev.epoch_type == MPID_EPOTYPE_REFENCE) {
		win_ptr->_dev.epoch_type = MPID_EPOTYPE_FENCE;
	}
        if (win_ptr->_dev.epoch_type == MPID_EPOTYPE_NONE ||
                        win_ptr->_dev.epoch_type == MPID_EPOTYPE_POST ||
                        !MPIDU_VALID_RMA_TARGET(win_ptr, target_rank)) {
                /* --BEGIN ERROR HANDLING-- */
                MPIU_ERR_SETANDSTMT(mpi_errno, MPI_ERR_RMA_SYNC,
                                        goto fn_fail, "**rmasync");
                /* --END ERROR HANDLING-- */
        }
        if (op == MPI_REPLACE) {
                /* Just do a PUT instead... */
                mpi_errno = MPID_Put(origin_addr, origin_count, origin_datatype,
                        target_rank, target_disp, target_count, target_datatype, win_ptr);
                if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
                goto fn_exit;
        }

        MPIDI_Datatype_get_info(origin_count, origin_datatype,
                dt_contig, data_sz, dtp, dt_true_lb);
        if ((data_sz == 0) || (target_rank == MPI_PROC_NULL)) {
                goto fn_exit;
        }
        if (DATATYPE_ADDITIONAL(origin_datatype)) {
		dt_contig = 1;	// treat MINLOC/MAXLOC types as contig
		data_sz = origin_count * dtp->extent;
	}
        rank = win_ptr->_dev.comm_ptr->rank;
        lpid = MPIDU_world_rank(win_ptr, target_rank);

        if (!DATATYPE_PREDEFINED(target_datatype)) {
                /* force map to get built but don't assume it was sent (use our lpid) */
                (void)MPIDU_check_dt(mpid_my_lpid, target_datatype, &dti);
                MPID_assert(dti.map != NULL);
                MPID_assert(dti.map[0].dt != 0);
        }
        if (target_rank == rank) {
                /*
                 * We still must have acquired the lock, unless
                 * we specified NOCHECK.
                 */
                if (win_ptr->_dev.epoch_type == MPID_EPOTYPE_LOCK &&
                    !(win_ptr->_dev.epoch_assert & MPI_MODE_NOCHECK) &&
                    MPIDU_is_lock_free(win_ptr)) {
                        /* --BEGIN ERROR HANDLING-- */
                        MPIU_ERR_SETANDSTMT(mpi_errno, MPI_ERR_RMA_SYNC,
                                        goto fn_fail, "**rmasync");
                        /* --END ERROR HANDLING-- */
                }
        } else {
                MPIU_ERR_CHKANDJUMP1(
                        (HANDLE_GET_KIND(op) != HANDLE_KIND_BUILTIN),
                        mpi_errno, MPI_ERR_OP, "**opnotpredefined",
                        "**opnotpredefined %d", op );
        }

        if (dt_contig) { /* all builtin datatypes are also contig */
                buf = origin_addr;
                cb_send.function = done_rqc_cb;
        } else {
                MPID_Segment segment;
                DLOOP_Offset last = data_sz;
		struct mpid_put_cb_data *put;

                MPIDU_MALLOC(buf, char, data_sz + sizeof(struct mpid_put_cb_data), mpi_errno, "MPID_Accumulate");
                if (buf == NULL) {
                        MPID_Abort(NULL, MPI_ERR_NO_SPACE, -1,
                                "Unable to allocate non-"
                                "contiguous buffer");
                }
                put = (struct mpid_put_cb_data *)buf;
                buf += sizeof(struct mpid_put_cb_data);
                refp = &put->ref;
                xtra.mpid_xtra_w1 = (size_t)put; // 'put' struct
                xtra.mpid_xtra_w2 = (size_t)put; // generic buf to free
		put->flag = 0; // no memory region
                cb_send.function = done_reffree_rqc_cb;
                *refp = 0;
                mpi_errno = MPID_Segment_init(origin_addr, origin_count,
                        origin_datatype, &segment, 0);
                if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
                MPID_Segment_pack(&segment, 0, &last, buf);
                MPID_assert_debug(last == data_sz);
        }
        dd = win_ptr->_dev.coll_info[target_rank].base_addr +
                win_ptr->_dev.coll_info[target_rank].disp_unit * target_disp;
	size_t mrcfg_bytes;
	void * mrcfg_base;
	DCMF_Memregion_query(&win_ptr->_dev.coll_info[target_rank].mem_region,
			&mrcfg_bytes, &mrcfg_base);
	dd += (size_t)mrcfg_base;
        if (DATATYPE_PREDEFINED(target_datatype)) {
                if (target_rank == rank) { /* local accumulate */
                        local_accumulate(win_ptr, dd, buf, lpid,
                                target_datatype, op, target_count);
                } else { /* ! local accumulate */
			if (DATATYPE_ADDITIONAL(target_datatype)) {
				
				MPID_Datatype_get_ptr(target_datatype, dtp);
				data_sz = dtp->extent;
			} else {
				data_sz = MPID_Datatype_get_basic_size(target_datatype);
			}
                        xtra.mpid_xtra_w0 = (size_t)&win_ptr->_dev.my_rma_pends;
                        reqp = MPIDU_get_req(&xtra, &info);
                        info->mpid_info_w0 = MPID_MSGTYPE_ACC;
                        info->mpid_info_w1 = win_ptr->_dev.coll_info[target_rank].win_handle;
                        info->mpid_info_w2 = rank;
                        info->mpid_info_w3 = (size_t)dd;
                        info->mpid_info_w4 = target_datatype;
                        info->mpid_info_w5 = op;
                        info->mpid_info_w6 = target_count;
                        info->mpid_info_w7 = 0;
                        ++win_ptr->_dev.my_rma_pends;
                        if (refp) { ++*refp; }
                        ++sent;
                        cb_send.clientdata = reqp;
                        mpi_errno = DCMF_Send(&bg1s_sn_proto, reqp, cb_send,
                                consistency, lpid,
                                target_count * data_sz,
                                buf, info->info, MPIDU_1SINFO_NQUADS);
                        if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
                        ++win_ptr->_dev.coll_info[target_rank].rma_sends;
                } /* ! local accumulate */
        } else {
                s = buf;
		if (refp) *refp = target_count * dti.map_len;
                for (j = 0; j < target_count; ++j) {
                        for (i = 0; i < dti.map_len; ++i) {
                                if (target_rank == rank) { /* local accumulate */
                                        local_accumulate(win_ptr, dd + dti.map[i].off,
                                                s, lpid,
                                                dti.map[i].dt, op, dti.map[i].num);
                                } else { /* ! local accumulate */

					MPIDU_Progress_spin(win_ptr->_dev.my_rma_pends >
						MPIDI_Process.rma_pending);
                                        xtra.mpid_xtra_w0 = (size_t)&win_ptr->_dev.my_rma_pends;
                                        reqp = MPIDU_get_req(&xtra, &info);
                                        info->mpid_info_w0 = MPID_MSGTYPE_ACC;
                                        info->mpid_info_w1 = win_ptr->_dev.coll_info[target_rank].win_handle;
                                        info->mpid_info_w2 = rank;
                                        info->mpid_info_w3 = (size_t)dd + dti.map[i].off;
                                        info->mpid_info_w4 = dti.map[i].dt;
                                        info->mpid_info_w5 = op;
                                        info->mpid_info_w6 = dti.map[i].num;
                                        info->mpid_info_w7 = 0;
                                        ++win_ptr->_dev.my_rma_pends;
                                        ++sent;
                                        cb_send.clientdata = reqp;
                                        mpi_errno = DCMF_Send(&bg1s_sn_proto, reqp, cb_send,
                                                consistency, lpid, dti.map[i].len, s, info->info, MPIDU_1SINFO_NQUADS);
                                        if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
                                        ++win_ptr->_dev.coll_info[target_rank].rma_sends;
                                } /* ! local accumulate */
                                s += dti.map[i].len;
                        } /* for map_len */
                        dd += dti.dtp->extent;
                } /* for target_count */
        }
        /*
         * TBD: Could return without waiting for sends...
         */
        MPIDU_Progress_spin(win_ptr->_dev.my_rma_pends > 0);
        if (sent == 0 && xtra.mpid_xtra_w2) {
                MPIDU_FREE(xtra.mpid_xtra_w2, mpi_errno, "MPID_Accumulate");
        }

fn_exit:
        MPID_MPI_FUNC_EXIT(MPID_STATE_MPID_WIN_ACCUMULATE);
        return mpi_errno;
        /* --BEGIN ERROR HANDLING-- */
fn_fail:
        goto fn_exit;
        /* --END ERROR HANDLING-- */
}

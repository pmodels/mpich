/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file src/onesided/mpid_put.c
 * \brief MPI-DCMF MPI_Put functionality
 */

#include "mpid_onesided.h"

/**
 * \page put_design MPID_Put Design
 *
 * <B>Origin node calls MPI_Put</B>
 *
 * - A sanity-check is done to
 * ensure that the window is in a valid state to initiate
 * a put RMA operation.
 * These checks include testing that
 * the epoch currently in affect is not \e NONE or \e POST.
 * - If target rank is origin rank, call MPIR_Localcopy.
 * - If origin datatype is not contiguous, allocate a buffer
 * for contiguous data and pack the local data into it.
 *  - This allocation includes a reference count which is used
 * in the send callbacks to determine when the buffer can be freed.
 * - If target datatype is contiguous, put data from local buffer
 * into target window buffer.
 * - If target datatype is non-contiguous:
 *  - Create IO Vector from target datatype.
 *  - Perform multiple put's from local buffer into target window
 * buffer segments.
 * - Wait for all sends to go.
 * - If no sends were initiated, and origin datatype non-contiguous,
 * free the buffer.
 */
/// \cond NOT_REAL_CODE
#undef FUNCNAME
#define FUNCNAME MPID_Put
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
/// \endcond
/**
 * \brief MPI-DCMF glue for MPI_PUT function
 *
 * Put \e origin_count number of \e origin_datatype from \e origin_addr
 * to node \e target_rank into \e target_count number of \e target_datatype
 * into window location \e target_disp offset (window displacement units)
 *
 * \param[in] origin_addr	Source buffer
 * \param[in] origin_count	Number of datatype elements
 * \param[in] origin_datatype	Source datatype
 * \param[in] target_rank	Destination rank (target)
 * \param[in] target_disp	Displacement factor in target buffer
 * \param[in] target_count	Number of target datatype elements
 * \param[in] target_datatype	Destination datatype
 * \param[in] win_ptr		Window
 * \return MPI_SUCCESS, MPI_ERR_RMA_SYNC, or error returned from
 *	MPIR_Localcopy, MPID_Segment_init, or DCMF_Send.
 *
 * \ref msginfo_usage\n
 * \ref put_design
 */
int MPID_Put(void *origin_addr, int origin_count,
        MPI_Datatype origin_datatype, int target_rank,
        MPI_Aint target_disp, int target_count,
        MPI_Datatype target_datatype, MPID_Win *win_ptr)
{
        int mpi_errno = MPI_SUCCESS;
        int dt_contig, rank;
        MPID_Datatype *dtp;
        MPI_Aint dt_true_lb;
        MPIDI_msg_sz_t data_sz;
        MPID_MPI_STATE_DECL(MPID_STATE_MPID_PUT);

        MPID_MPI_FUNC_ENTER(MPID_STATE_MPID_PUT);

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

        MPIDI_Datatype_get_info(origin_count, origin_datatype,
                dt_contig, data_sz, dtp, dt_true_lb);
        if ((data_sz == 0) || (target_rank == MPI_PROC_NULL)) {
                goto fn_exit;
        }
        rank = win_ptr->_dev.comm_ptr->rank;

        /* If the put is a local operation, do it here */
        if (target_rank == rank) {
                if (win_ptr->_dev.epoch_type == MPID_EPOTYPE_LOCK &&
                                MPIDU_is_lock_free(win_ptr)) {
                        /* --BEGIN ERROR HANDLING-- */
                        MPIU_ERR_SETANDSTMT(mpi_errno, MPI_ERR_RMA_SYNC,
                                        goto fn_fail, "**rmasync");
                        /* --END ERROR HANDLING-- */
                }
                mpi_errno = MPIR_Localcopy(origin_addr, origin_count,
                                origin_datatype,
                                (char *)win_ptr->base +
                                        win_ptr->disp_unit *
                                        target_disp,
                                target_count, target_datatype);
                if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
        } else {
                /* queue it up */
                DCMF_Callback_t cb_send;
                int *refp = NULL;
                DCMF_Request_t *reqp;
                int t_dt_contig;
                MPID_Datatype *t_dtp;
                MPI_Aint t_dt_true_lb;
                MPIDI_msg_sz_t t_data_sz;
                MPID_Segment segment;
                mpid_dt_info dti;
                int i, j, sent = 0;
		DLOOP_Offset last;
                char *b, *s, *buf;
                MPIDU_Onesided_info_t *info;
                int lpid;
                MPIDU_Onesided_xtra_t xtra = {0};
		struct mpid_put_cb_data *put;
		DCMF_Memregion_t *bufmr;

                lpid = MPIDU_world_rank(win_ptr, target_rank);
                MPIDI_Datatype_get_info(target_count, target_datatype,
                        t_dt_contig, t_data_sz, t_dtp, t_dt_true_lb);
                /* NOTE! t_data_sz already is adjusted for target_count */

                if (dt_contig && origin_addr >= win_ptr->base &&
				origin_addr < win_ptr->base + win_ptr->size) {
			// put from inside origin window, re-use origin window memregion.
                        buf = origin_addr;
                       	cb_send.function = done_rqc_cb;
			bufmr = &win_ptr->_dev.coll_info[rank].mem_region;
			s = (char *)((char *)buf -
				(char *)win_ptr->base +
				(char *)win_ptr->_dev.coll_info[rank].base_addr);
                } else {
			// need memregion, also may need to pack
			if (dt_contig) {
                        	buf = origin_addr;
                        	MPIDU_MALLOC(put, struct mpid_put_cb_data,
					sizeof(struct mpid_put_cb_data),
					mpi_errno, "MPID_Put");
                        	if (put == NULL) {
                                	MPID_Abort(NULL, MPI_ERR_NO_SPACE, -1,
                                        	"Unable to allocate "
                                        	"put buffer");
                        	}
			} else {
                        	MPIDU_MALLOC(buf, char, data_sz +
					sizeof(struct mpid_put_cb_data),
					mpi_errno, "MPID_Put");
                        	if (buf == NULL) {
                                	MPID_Abort(NULL, MPI_ERR_NO_SPACE, -1,
                                        	"Unable to allocate "
                                        	"non-contiguous buffer");
                        	}
                        	put = (struct mpid_put_cb_data *)buf;
                        	buf += sizeof(struct mpid_put_cb_data);
                        	mpi_errno = MPID_Segment_init(origin_addr,
                                        	origin_count,
                                        	origin_datatype, &segment, 0);
                        	if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
                        	last = data_sz;
                        	MPID_Segment_pack(&segment, 0, &last, buf);
                        	MPID_assert_debug(last == data_sz);
			}
                        refp = &put->ref;
                        xtra.mpid_xtra_w1 = (size_t)put; // 'put' struct
                        xtra.mpid_xtra_w2 = (size_t)put; // generic buf to free
                        cb_send.function = done_reffree_rqc_cb;
			void * mrcfg_base  = buf;
			size_t mrcfg_bytes = data_sz;
			(void)DCMF_Memregion_create(&put->memreg, &mrcfg_bytes, mrcfg_bytes, mrcfg_base, 0);
			// check errors?
			bufmr = &put->memreg;
			s = (char *)(buf - (char *)mrcfg_base);
			put->flag = 1; // we have a memregion
                        *refp = 0;
                }
#ifdef USE_DCMF_PUT
#else /* ! USE_DCMF_PUT */
		size_t mrcfg_bytes;
		void * mrcfg_base;
		DCMF_Memregion_query(&win_ptr->_dev.coll_info[target_rank].mem_region, &mrcfg_bytes, &mrcfg_base);
#endif /* ! USE_DCMF_PUT */
                xtra.mpid_xtra_w0 = (size_t)&win_ptr->_dev.my_rma_pends;
                if (t_dt_contig) {
                        b = win_ptr->_dev.coll_info[target_rank].base_addr +
                                win_ptr->_dev.coll_info[target_rank].disp_unit * target_disp;
#ifdef USE_DCMF_PUT
                        reqp = MPIDU_get_req(&xtra, NULL);
#else /* ! USE_DCMF_PUT */
                        reqp = MPIDU_get_req(&xtra, &info);
                	info->mpid_info_w0 = MPID_MSGTYPE_PUT;
                	info->mpid_info_w1 = win_ptr->_dev.coll_info[target_rank].win_handle;
                	info->mpid_info_w2 = rank;
                        info->mpid_info_w3 = (size_t)((size_t)mrcfg_base + b);
#endif /* ! USE_DCMF_PUT */
                        cb_send.clientdata = reqp;
                        ++win_ptr->_dev.my_rma_pends;
                        if (refp) { ++*refp; }
                        ++sent;
#ifdef USE_DCMF_PUT
			mpi_errno = DCMF_Put(&bg1s_pt_proto, reqp,
				cb_send, win_ptr->_dev.my_cstcy, lpid,
				t_data_sz,
				bufmr,
				&win_ptr->_dev.coll_info[target_rank].mem_region,
				(size_t)s,
				(size_t)b,
				(DCMF_Callback_t){NULL, NULL});
#else /* ! USE_DCMF_PUT */
                        mpi_errno = DCMF_Send(&bg1s_sn_proto, reqp,
                                cb_send, win_ptr->_dev.my_cstcy, lpid,
                                t_data_sz, buf,
                                info->info, MPIDU_1SINFO_NQUADS);
#endif /* ! USE_DCMF_PUT */
                        if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
                        ++win_ptr->_dev.coll_info[target_rank].rma_sends;
                } else {
                        /* force map to get built but don't assume
                         * it was sent (use our lpid) */
                        (void)MPIDU_check_dt(mpid_my_lpid, target_datatype, &dti);
                        MPID_assert(dti.map != NULL);
                        xtra.mpid_xtra_w1 = (size_t)&win_ptr->_dev.my_rma_pends;
                        b = win_ptr->_dev.coll_info[target_rank].base_addr +
                                win_ptr->_dev.coll_info[target_rank].disp_unit *
                                target_disp;
#ifdef USE_DCMF_PUT
			// 's' already set
#else /* ! USE_DCMF_PUT */
			s = buf;
			b += (size_t)mrcfg_base;
#endif /* ! USE_DCMF_PUT */
                        if (refp) *refp = target_count * dti.map_len;
                        for (j = 0; j < target_count; ++j) {
                                for (i = 0; i < dti.map_len; i++) {
					MPIDU_Progress_spin(win_ptr->_dev.my_rma_pends >
						MPIDI_Process.rma_pending);
#ifdef USE_DCMF_PUT
                                        reqp = MPIDU_get_req(&xtra, NULL);
                                        cb_send.clientdata = reqp;
                                        ++win_ptr->_dev.my_rma_pends;
                                        ++sent;
					mpi_errno = DCMF_Put(&bg1s_pt_proto, reqp,
						cb_send, win_ptr->_dev.my_cstcy, lpid,
						dti.map[i].len,
						bufmr,
						&win_ptr->_dev.coll_info[target_rank].mem_region,
						(size_t)s,
						(size_t)b + dti.map[i].off,
						(DCMF_Callback_t){NULL, NULL});
#else /* ! USE_DCMF_PUT */
                                        reqp = MPIDU_get_req(&xtra, &info);
                			info->mpid_info_w0 = MPID_MSGTYPE_PUT;
                			info->mpid_info_w1 = win_ptr->_dev.coll_info[target_rank].win_handle;
                			info->mpid_info_w2 = rank;
                                        info->mpid_info_w3 = (size_t)b +
						dti.map[i].off;
                                        cb_send.clientdata = reqp;
                                        ++win_ptr->_dev.my_rma_pends;
                                        ++sent;
                                        mpi_errno = DCMF_Send(&bg1s_sn_proto,
                                                reqp, cb_send,
                                                win_ptr->_dev.my_cstcy, lpid,
                                                dti.map[i].len,
						s,
                                                info->info, MPIDU_1SINFO_NQUADS);
#endif /* ! USE_DCMF_PUT */
                                        if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
                                        ++win_ptr->_dev.coll_info[target_rank].rma_sends;
                                        s += dti.map[i].len;
                                }
                                b += dti.dtp->extent;
                        }
                }
#ifdef USE_DCMF_PUT
		if (sent) {
			/*
			 * inform target how many messages we sent.
			 * this serves as a completion notification
			 * since the consistency is MATCH.
			 */
                        xtra.mpid_xtra_w1 = 0;
                        xtra.mpid_xtra_w2 = 0;
			reqp = MPIDU_get_req(&xtra, &info);
                	info->mpid_info_w0 = MPID_MSGTYPE_PUT;
                	info->mpid_info_w1 = win_ptr->_dev.coll_info[target_rank].win_handle;
                	info->mpid_info_w2 = rank;
                	info->mpid_info_w3 = sent;
			cb_send.function = done_rqc_cb;
			cb_send.clientdata = reqp;
			++win_ptr->_dev.my_rma_pends;
                        mpi_errno = DCMF_Send(&bg1s_sn_proto, reqp,
                                cb_send, win_ptr->_dev.my_cstcy, lpid,
                                0, NULL,
                                info->info, MPIDU_1SINFO_NQUADS);
		}
#endif /* USE_DCMF_PUT */
                /* TBD: someday this will be done elsewhere */
                MPIDU_Progress_spin(win_ptr->_dev.my_rma_pends > 0);
                if (sent == 0 && xtra.mpid_xtra_w2) {
                        MPIDU_FREE(xtra.mpid_xtra_w2, mpi_errno, "MPID_Put");
                }
        }

fn_exit:
        MPID_MPI_FUNC_EXIT(MPID_STATE_MPID_PUT);
        return mpi_errno;

        /* --BEGIN ERROR HANDLING-- */
fn_fail:
        goto fn_exit;
        /* --END ERROR HANDLING-- */
}

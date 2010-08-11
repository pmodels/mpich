/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file src/onesided/mpid_get.c
 * \brief MPI-DCMF MPI_Get functionality
 */

#include "mpid_onesided.h"

/**
 * \page get_design MPID_Get Design
 *
 * A MPID_Get sequence is as follows:
 *
 * <B>Origin node calls MPI_Get</B>
 *
 * - A sanity-check is done to
 * ensure that the window is in a valid state to initiate
 * a get RMA operation.
 * These checks include testing that
 * the epoch currently in affect is not \e NONE or \e POST.
 * - If target rank is origin rank, call MPIR_Localcopy.
 * - If origin datatype is not contiguous, allocate a buffer
 * for contiguous data.
 * - If target datatype is contiguous, get data from target node
 * into local buffer.
 * - If target datatype is non-contiguous:
 *  - Create IO Vector from target datatype.
 *  - Perform multiple get's from target into local buffer.
 * - If origin datatype is not contiguous, unpack data from buffer
 * into local window. (free buffer)
 */
/// \cond NOT_REAL_CODE
#undef FUNCNAME
#define FUNCNAME MPID_Get
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
/// \endcond
/**
 * \brief MPI-DCMF glue for MPI_GET function
 *
 * Get \e target_count number of \e target_datatype from \e target_rank
 * from window location \e target_disp offset (window displacement units)
 * into \e origin_count number of \e origin_datatype at \e origin_addr
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
 *	MPIR_Localcopy, MPID_Segment_init, or DCMF_Get.
 *
 * \ref msginfo_usage\n
 * \ref get_design
 */
int MPID_Get(void *origin_addr, int origin_count,
        MPI_Datatype origin_datatype, int target_rank,
        MPI_Aint target_disp, int target_count,
        MPI_Datatype target_datatype, MPID_Win *win_ptr)
{
        int mpi_errno = MPI_SUCCESS;
        int dt_contig, rank;
        MPID_Datatype *dtp;
        MPI_Aint dt_true_lb;
        MPIDI_msg_sz_t data_sz;
        MPID_MPI_STATE_DECL(MPID_STATE_MPID_GET);

        MPID_MPI_FUNC_ENTER(MPID_STATE_MPID_GET);

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

        /* If the get is a local operation, do it here */
        if (target_rank == rank) {
                mpi_errno = MPIR_Localcopy(
                        (char *)win_ptr->base +
                                win_ptr->disp_unit * target_disp,
                        target_count, target_datatype,
                        origin_addr, origin_count, origin_datatype);
                if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
        } else {
                /* queue it up */
                /*
                 * Since GET can never be left pending, we keep
                 * GETs separate from other RMA ops which might not
                 * complete until later. We also do not increment
                 * [target]rma_sends here.
                 */
                DCMF_Callback_t cb_send;
                DCMF_Request_t *reqp;
                int t_dt_contig;
                MPID_Datatype *t_dtp;
                MPI_Aint t_dt_true_lb;
                MPIDI_msg_sz_t t_data_sz;
                mpid_dt_info dti;
                int i, j, get_len;
		int *refp = NULL;
                char *b, *s, *t, *buf;
		DCMF_Memregion_t *bufmr;
		size_t mrcfg_bytes;
		void * mrcfg_base;
                int lpid;
                MPIDU_Onesided_xtra_t xtra = {0};

                lpid = MPIDU_world_rank(win_ptr, target_rank);
                MPIDI_Datatype_get_info(target_count, target_datatype,
                        t_dt_contig, t_data_sz, t_dtp, t_dt_true_lb);
                /* NOTE! t_data_sz already is adjusted for target_count */

                get_len = (data_sz < t_data_sz ? data_sz : t_data_sz);

                xtra.mpid_xtra_w0 = (size_t)&win_ptr->_dev.my_get_pends;
                if (dt_contig && origin_addr >= win_ptr->base &&
				origin_addr < win_ptr->base + win_ptr->size) {
			// get to inside origin window, re-use origin window memregion.
                        buf = origin_addr;
                        cb_send.function = done_rqc_cb;
			bufmr = &win_ptr->_dev.coll_info[rank].mem_region;
			s = (char *)((char *)buf -
				(char *)win_ptr->base +
				win_ptr->_dev.coll_info[rank].base_addr);
                } else {
			// need memregion, also may need to unpack
			struct mpid_get_cb_data *get;
			if (dt_contig) {
				buf = origin_addr;
                        	MPIDU_MALLOC(get, struct mpid_get_cb_data,
					sizeof(struct mpid_get_cb_data),
					mpi_errno, "MPID_Get");
                        	if (get == NULL) {
                                	MPID_Abort(NULL, MPI_ERR_NO_SPACE, -1,
                                        	"Unable to allocate "
                                        	"get buffer");
                        	}
				get->dtp = NULL; // do not unpack
			} else {
                        	MPIDU_MALLOC(buf, char, get_len +
					sizeof(struct mpid_get_cb_data),
					mpi_errno, "MPID_Get");
                        	if (buf == NULL) {
                                	MPID_Abort(NULL, MPI_ERR_NO_SPACE, -1,
                                        	"Unable to allocate non-"
                                        	"contiguous buffer");
                        	}
				MPID_Datatype_add_ref(dtp);
				get = (struct mpid_get_cb_data *)buf;
				buf += sizeof(struct mpid_get_cb_data);
				get->dtp = dtp;
				get->addr = origin_addr;
				get->count = origin_count;
				get->len = get_len;
				get->buf = buf;
                        }
			mrcfg_base = buf;
			mrcfg_bytes = get_len;
			(void)DCMF_Memregion_create(&get->memreg, &mrcfg_bytes, mrcfg_bytes, mrcfg_base, 0);
			// check errors?
			bufmr = &get->memreg;
			s = (char *)(buf - (size_t)mrcfg_base);
			get->ref = 0;
			refp = &get->ref;
			xtra.mpid_xtra_w1 = (size_t)get; // 'get' struct
			xtra.mpid_xtra_w2 = (size_t)get; // generic buf to free
                        cb_send.function = done_getfree_rqc_cb;
                }
                if (t_dt_contig) {
                        reqp = MPIDU_get_req(&xtra, NULL);
                        cb_send.clientdata = reqp;
                        ++win_ptr->_dev.my_get_pends;
			if (refp) ++(*refp);
                        t = win_ptr->_dev.coll_info[target_rank].base_addr +
                                win_ptr->_dev.coll_info[target_rank].disp_unit * target_disp;
                        mpi_errno = DCMF_Get(&bg1s_gt_proto, reqp,
                                cb_send, win_ptr->_dev.my_cstcy, lpid,
                                t_data_sz,
				&win_ptr->_dev.coll_info[target_rank].mem_region,
				bufmr,
				(size_t)t,
				(size_t)s);
                        if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
                } else {
                        /* force map to get built but don't assume
                         * it was sent (use our lpid) */
                        (void)MPIDU_check_dt(mpid_my_lpid, target_datatype, &dti);
                        MPID_assert(dti.map != NULL);
                        b = win_ptr->_dev.coll_info[target_rank].base_addr +
                                win_ptr->_dev.coll_info[target_rank].disp_unit *
                                target_disp;
			// 's' already set
			if (refp) *refp = target_count * dti.map_len;
                        for (j = 0; j < target_count; ++j) {
                                for (i = 0; i < dti.map_len; i++) {
					MPIDU_Progress_spin(win_ptr->_dev.my_get_pends >
						MPIDI_Process.rma_pending);
                                        t = b + dti.map[i].off;
                                        reqp = MPIDU_get_req(&xtra, NULL);
                                        cb_send.clientdata = reqp;
                                        ++win_ptr->_dev.my_get_pends;
                                        mpi_errno = DCMF_Get(&bg1s_gt_proto,
                                                reqp, cb_send,
                                                win_ptr->_dev.my_cstcy, lpid,
                                                dti.map[i].len,
						&win_ptr->_dev.coll_info[target_rank].mem_region,
						bufmr,
						(size_t)t, 
						(size_t)s);
                                        if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
                                        s += dti.map[i].len;
                                }
                                b += dti.dtp->extent;
                        }
                }
		/**
		 * \todo we don't know when the "request to get" messages have been sent,
		 * but we should wait for that.
		 */
        }

fn_exit:
        MPID_MPI_FUNC_EXIT(MPID_STATE_MPID_GET);
        return mpi_errno;

        /* --BEGIN ERROR HANDLING-- */
fn_fail:
        goto fn_exit;
        /* --END ERROR HANDLING-- */
}

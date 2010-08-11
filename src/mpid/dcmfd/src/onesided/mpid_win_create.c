/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file src/onesided/mpid_win_create.c
 * \brief MPI-DCMF MPI_Win_create/free functionality
 *
 * Includes general onesided initialization (via first call to MPID_Win_create).
 */

#include "mpid_onesided.h"

/**
 * \brief One-time initialization of control messages protocol
 *
 * \return nothing
 */
static void mpid_ctl_init(void) {
        DCMF_Control_Configuration_t ctl_cfg =
                { 
		  DCMF_DEFAULT_CONTROL_PROTOCOL, 
		  DCMF_DEFAULT_NETWORK,
		  recv_ctl_cb, NULL
		};
        DCMF_Control_register(&bg1s_ct_proto, &ctl_cfg);
}

/**
 * \brief One-time initialization of locks
 *
 * \return nothing
 */
static void mpid_lock_init(void) {
}

/**
 * \brief One-time initialization of sends
 *
 * \return nothing
 */
static void mpid_send_init(void) {
        DCMF_Send_Configuration_t send_cfg =
                { DCMF_DEFAULT_SEND_PROTOCOL, 
		  DCMF_DEFAULT_NETWORK,
		  recv_sm_cb, NULL, recv_cb, NULL };

        DCMF_Send_register(&bg1s_sn_proto, &send_cfg);
}

/**
 * \brief One-time initialization of puts
 *
 * \return nothing
 */
static void mpid_put_init(void) {
        DCMF_Put_Configuration_t put_cfg =
                { DCMF_DEFAULT_PUT_PROTOCOL, DCMF_DEFAULT_NETWORK };

        DCMF_Put_register(&bg1s_pt_proto, &put_cfg);
}

/**
 * \brief One-time initialization of gets
 *
 * \return nothing
 */
static void mpid_get_init(void) {
        DCMF_Get_Configuration_t get_cfg =
                { DCMF_DEFAULT_GET_PROTOCOL, DCMF_DEFAULT_NETWORK };

        DCMF_Get_register(&bg1s_gt_proto, &get_cfg);
}

/**
 * \brief User defined function to handle summing of rma_sends
 *
 * \param[in] v1	Source data
 * \param[in] v2	Destination data
 * \param[in] i1	number of elements
 * \param[in] d1	datatype
 * \return	nothing
 */
static void sum_coll_info(void *v1, void *v2, int *i1, MPI_Datatype *d1) {
        struct MPID_Win_coll_info *in = v1, *out = v2;
        int len = *i1;
        int x;

        MPID_assert_debug(*d1 == Coll_info_rma_dt);
        for (x = 0; x < len; ++x) {
                out->rma_sends += in->rma_sends;
                ++out;
                ++in;
        }
}

/**
 * \brief One-time MPID one-sided initialization
 *
 * \return nothing
 */
static void mpid_init(void) {
        mpid_lock_init();
        mpid_send_init();
        mpid_ctl_init();
        mpid_get_init();
        mpid_put_init();
        /*
         * need typemap { (int,12), (int,28), ... }
         *
         * i.e. [0] => &(*win_ptr)->coll_info[0].rma_sends,
         *	[1] => &(*win_ptr)->coll_info[1].rma_sends,
         *	[2] => &(*win_ptr)->coll_info[2].rma_sends,
         *	...
         */
        PMPI_Type_contiguous(sizeof(struct MPID_Win_coll_info) / sizeof(int),
				MPI_INT, &Coll_info_rma_dt);
        PMPI_Type_commit(&Coll_info_rma_dt);
        PMPI_Op_create(sum_coll_info, 0, &Coll_info_rma_op);
}

/// \cond NOT_REAL_CODE
#undef FUNCNAME
#define FUNCNAME MPID_Win_create
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
/// \endcond
/**
 * \brief MPI-DCMF glue for MPI_WIN_CREATE function
 *
 * Create a window object. Allocates a MPID_Win object and initializes it,
 * then allocates the collective info array, initalizes our entry, and
 * performs an Allgather to distribute/collect the rest of the array entries.
 *
 * ON first call, initializes (registers) protocol objects for locking,
 * get, and send operations to message layer. Also creates datatype to
 * represent the rma_sends element of the collective info array,
 * used later to synchronize epoch end events.
 *
 * \param[in] base	Local window buffer
 * \param[in] size	Local window size
 * \param[in] disp_unit	Displacement unit size
 * \param[in] info	Window hints (not used)
 * \param[in] comm_ptr	Communicator
 * \param[out] win_ptr	Window
 * \return MPI_SUCCESS, MPI_ERR_OTHER, or error returned from
 *	NMPI_Comm_dup or MPIR_Allgather_impl.
 */
int MPID_Win_create(void *base, MPI_Aint size, int disp_unit,
                MPID_Info *info, MPID_Comm *comm_ptr,
                MPID_Win **win_ptr)
{
        static int initial = 0;
        MPID_Win *win;
        int mpi_errno=MPI_SUCCESS, comm_size, rank;

        MPID_MPI_STATE_DECL(MPID_STATE_MPID_WIN_CREATE);

        MPID_MPI_FUNC_ENTER(MPID_STATE_MPID_WIN_CREATE);

        MPIU_UNREFERENCED_ARG(info);

        comm_size = MPIDU_comm_size_c(comm_ptr);
        rank = comm_ptr->rank;

        if (!initial++) {
                MPID_assert_debug(sizeof(((MPIDU_Onesided_ctl_t *)0)->_c_u) <=
                                sizeof(((MPIDU_Onesided_ctl_t *)0)->ctl));
                mpid_init();
                mpid_my_lpid = MPIDU_world_rank_c(comm_ptr, rank);
                //initial = 0;
        }

        win = (MPID_Win *)MPIU_Handle_obj_alloc(&MPID_Win_mem);
        MPIU_ERR_CHKANDJUMP(!win,mpi_errno,MPI_ERR_OTHER,"**nomem");
        memset((char *)win + sizeof(MPIU_Handle_head), 0,
                        sizeof(*win) - sizeof(MPIU_Handle_head));

        win->base = base;
        win->size = size;
        win->disp_unit = disp_unit;
/* MPID_DEV_WIN_DECL ... */
        mpidu_init_lock(win);
        win->_dev.epoch_type = MPID_EPOTYPE_NONE;
        win->_dev.my_cstcy = DCMF_MATCH_CONSISTENCY;

        mpi_errno = MPI_Comm_dup_impl(comm_ptr, &win->_dev.comm_ptr);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        win->comm = win->_dev.comm_ptr->handle;
        
        MPID_assert_debug(win->comm != MPI_COMM_NULL);
        MPID_assert_debug(win->_dev.comm_ptr != NULL);

        /* allocate memory for the base addresses, disp_units, and
                completion counters of all processes */

        MPIDU_MALLOC(win->_dev.coll_info, struct MPID_Win_coll_info,
                comm_size * sizeof(struct MPID_Win_coll_info),
                mpi_errno, "win->_dev.coll_info");
        /* FIXME: This needs to be fixed for heterogeneous systems */
        win->_dev.coll_info[rank].disp_unit = disp_unit;
        win->_dev.coll_info[rank].win_handle = win->handle;
        win->_dev.coll_info[rank].rma_sends = 0; /* Allgather zeros these */
	// DCMF_assert_debug(sizeof(DCMF_Memregion_t) == sizeof(void *));
	size_t mem_cfg_bytes = size;
	void * mem_cfg_base = base;
	DCMF_Result err = DCMF_Memregion_create(
			&win->_dev.coll_info[rank].mem_region,
			&mem_cfg_bytes, mem_cfg_bytes, mem_cfg_base, 0);
        if (err) { MPIU_ERR_POP(err); }
	// 'base_addr' is now an offset for the buf in memregion
	win->_dev.coll_info[rank].base_addr = (char *)((char *)base - (char *)mem_cfg_base);

        mpi_errno = MPIR_Allgather_impl(MPI_IN_PLACE, 0, MPI_DATATYPE_NULL,
                                        win->_dev.coll_info,
                                        sizeof(struct MPID_Win_coll_info),
                                        MPI_BYTE, comm_ptr);
        if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
        /* try to avoid a race where one node sends us a lock request
         * before we're ready */
        mpi_errno = MPIR_Barrier_impl(comm_ptr);
        if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
        *win_ptr = win;

fn_exit:
        MPID_MPI_FUNC_EXIT(MPID_STATE_MPID_WIN_CREATE);
        return mpi_errno;
        /* --BEGIN ERROR HANDLING-- */
fn_fail:
        goto fn_exit;
        /* --END ERROR HANDLING-- */
}

/// \cond NOT_REAL_CODE
#undef FUNCNAME
#define FUNCNAME MPID_Win_free
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
/// \endcond
/**
 * \brief MPI-DCMF glue for MPI_WIN_FREE function
 *
 * Release all references and free memory associated with window.
 *
 * \param[in,out] win_ptr	Window
 * \return MPI_SUCCESS or error returned from MPIR_Barrier_impl.
 */
int MPID_Win_free(MPID_Win **win_ptr)
{
        int mpi_errno=MPI_SUCCESS;
        MPID_Win *win = *win_ptr;
        MPID_MPI_STATE_DECL(MPID_STATE_MPID_WIN_FREE);

        MPID_MPI_FUNC_ENTER(MPID_STATE_MPID_WIN_FREE);

        MPID_assert(win->_dev.epoch_type == MPID_EPOTYPE_NONE ||
        		win->_dev.epoch_type == MPID_EPOTYPE_REFENCE);

        mpi_errno = MPIR_Barrier_impl(win->_dev.comm_ptr);
        if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
        /*
         * previous while loop  and barrier will not exit until all waiters have
         * been granted the lock.
         */
        int rank = win->_dev.comm_ptr->rank;
	(void) DCMF_Memregion_destroy(
			&win->_dev.coll_info[rank].mem_region);
        mpi_errno = MPIR_Comm_free_impl(win->_dev.comm_ptr);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        MPIDU_FREE(win->_dev.coll_info, mpi_errno, "win->_dev.coll_info");
        mpidu_free_lock(win);
        /** \todo check whether refcount needs to be decremented
         * here as in group_free */
        MPIU_Handle_obj_free(&MPID_Win_mem, win);
        *win_ptr = NULL;
fn_exit:
        MPID_MPI_FUNC_EXIT(MPID_STATE_MPID_WIN_FREE);
        return mpi_errno;
        /* --BEGIN ERROR HANDLING-- */
fn_fail:
        goto fn_exit;
        /* --END ERROR HANDLING-- */
}

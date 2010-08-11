/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file src/onesided/mpid_win_pscw.c
 * \brief MPI-DCMF MPI_Win_post/start/complete/wait(test) functionality
 */

#include "mpid_onesided.h"

/**
 * \brief Test if MPI_Win_post exposure epoch has ended
 *
 * Must call advance at least once per call. Tests if all
 * MPID_MSGTYPE_COMPLETE messages have been received, and whether
 * all RMA ops that were sent have been received. If epoch does end,
 * must cleanup all structures, counters, flags, etc.
 *
 * Assumes that MPID_Progress_start() and MPID_Progress_end() bracket
 * this call, in some meaningful fashion.
 *
 * \param[in] win	Window
 * \return	TRUE if epoch has ended
 */
static int mpid_check_post_done(MPID_Win *win) {
        int rank = win->_dev.comm_ptr->rank;

        (void)MPID_Progress_test();
        if (!(win->_dev.epoch_assert & MPI_MODE_NOCHECK) &&
            (win->_dev.my_sync_done < win->_dev.epoch_size ||
                        win->_dev.my_rma_recvs < win->_dev.coll_info[rank].rma_sends)) {
                return 0;
        }
        win->_dev.epoch_size = 0;
        win->_dev.epoch_assert = 0;
	epoch_clear(win);
        /*
         * Any RMA ops we initiated would be handled in a
         * Win_start/Win_complete epoch and that would have
         * zeroed our target rma_sends.
         */
        epoch_end_cb(win);
        return 1;
}

/// \cond NOT_REAL_CODE
#undef FUNCNAME
#define FUNCNAME MPID_Win_complete
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
/// \endcond
/**
 * \brief MPI-DCMF glue for MPI_WIN_COMPLETE function
 *
 * End the access epoch began by MPID_Win_start.
 * Sends a MPID_MSGTYPE_COMPLETE message, containing our count of
 * RMA operations sent to that node, to all nodes in the group.
 *
 * \param[in] win_ptr		Window
 * \return MPI_SUCCESS, MPI_ERR_RMA_SYNC, or error returned from
 *	MPIDU_proto_send.
 *
 * \ref post_design
 */
int MPID_Win_complete(MPID_Win *win_ptr)
{
        unsigned pending;
        int mpi_errno = MPI_SUCCESS;
        MPID_MPI_STATE_DECL(MPID_STATE_MPID_WIN_COMPLETE);

        MPID_MPI_FUNC_ENTER(MPID_STATE_MPID_WIN_COMPLETE);

        if (win_ptr->_dev.epoch_type != MPID_EPOTYPE_START &&
                        win_ptr->_dev.epoch_type != MPID_EPOTYPE_POSTSTART) {
                /* --BEGIN ERROR HANDLING-- */
                MPIU_ERR_SETANDSTMT(mpi_errno, MPI_ERR_RMA_SYNC,
                                        goto fn_fail, "**rmasync");
                /* --END ERROR HANDLING-- */
        }

        if (!(win_ptr->_dev.epoch_assert & MPI_MODE_NOCHECK)) {
                /* This zeroes the respective rma_sends counts...  */
                mpi_errno = MPIDU_proto_send(win_ptr, win_ptr->start_group_ptr,
                        MPID_MSGTYPE_COMPLETE);
                if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
        }
        /*
         * MPICH2 says that we cannot return until all RMA ops
         * have completed at the origin (i.e. been sent).
	 *
	 * Since MPIDU_proto_send() uses DCMF_Control(), there
	 * are no pending sends to wait for.
         */
        MPIDU_Progress_spin(win_ptr->_dev.my_rma_pends > 0 ||
			win_ptr->_dev.my_get_pends > 0);
        win_ptr->start_assert = 0;
        MPIU_Object_release_ref(win_ptr->start_group_ptr, &pending);
        win_ptr->start_group_ptr = NULL;
        if (win_ptr->_dev.epoch_type == MPID_EPOTYPE_POSTSTART) {
                win_ptr->_dev.epoch_type = MPID_EPOTYPE_POST;
        } else {
                win_ptr->_dev.epoch_size = 0;
                epoch_clear(win_ptr);
                epoch_end_cb(win_ptr);
        }

fn_exit:
        MPID_MPI_FUNC_EXIT(MPID_STATE_MPID_WIN_COMPLETE);
        return mpi_errno;

        /* --BEGIN ERROR HANDLING-- */
fn_fail:
        goto fn_exit;
        /* --END ERROR HANDLING-- */
}

/**
 * \page post_design MPID_Win_post-start / complete-wait Design
 *
 * MPID_Win_post and MPID_Win_start take a group object. In
 * each case this group object excludes the calling node.
 * Each node calling MPID_Win_post or MPID_Win_start may
 * specify a different group, however all nodes listed in a
 * MPID_Win_start group must call MPID_Win_post, and vice versa.
 * A node may call both MPID_Win_post and MPID_Win_start, but
 * only in that order. Likewise, MPID_Win_complete and MPID_Win_wait
 * (or MPID_Win_test) must be called in that order.
 *
 * A post-start / RMA / complete-wait sequence is as follows:
 *
 * The following assumes that all nodes are in-sync with respect to
 * synchronization primitives. If not, nodes will fail their
 * synchronization calls as appropriate.
 *
 * <B>All nodes permitting RMA access call MPI_Win_post</B>
 *
 * - A sanity-check is done to
 * ensure that the window is in a valid state to enter a
 * \e POST access epoch. These checks include testing that
 * no other epoch is currently in affect.
 * - If the local window is currenty locked then wait for the
 * lock to be released (calling advance in the loop).
 * - Set epoch type to MPID_EPOTYPE_POST, epoch size to group size,
 * RMA OK flag to TRUE, and save MPI_MODE_* assertions.
 * - If MPI_MODE_NOCHECK is set, return MPI_SUCCESS.
 * - Send MPID_MSGTYPE_POST protocol messages to all nodes in group.
 * - Wait for all sends to complete (not for receives to complete).
 *
 * <B>All nodes intending to do RMA access call MPI_Win_start</B>
 *
 * - A sanity-check is done to
 * ensure that the window is in a valid state to enter a
 * \e START access epoch. These checks include testing that
 * either a \e POST epoch or no epoch is currently in affect.
 * - If the current epoch is MPID_EPOTYPE_NONE and the
 * local window is currenty locked then wait for the
 * lock to be released (calling advance in the loop).
 * - Set epoch type to MPID_EPOTYPE_START or MPID_EPOTYPE_POSTSTART
 * and save MPI_MODE_* \e start assertions.
 * - Take a reference on the group and save the \e start group (pointer).
 * - If MPI_MODE_NOCHECK is set then return MPI_SUCCESS now.
 * - Wait for MPID_MSGTYPE_POST messages to be received from
 * all nodes in group.
 *
 * <B>One or more nodes invoke RMA operation(s)</B>
 *
 * - See respective RMA operation calls for details
 *
 * <B>All nodes that intended to do RMA access call MPI_Win_complete</B>
 *
 * - A sanity-check is done to
 * ensure that the window is in a valid state to end a
 * \e START access epoch. These checks include testing that
 * either a \e START epoch or \e POSTSTART epoch is currently in affect.
 * - Send MPID_MSGTYPE_COMPLETE messages to all nodes in the group.
 * These messages include the count of RMA operations that were sent.
 * - Wait for all messages to send, including RMA operations that
 * had not previously gone out (MPICH2 requirement).
 * Call advance in the loop.
 * - Reset epoch info as appropriate, to state prior to MPID_Win_start
 * call (i.e. epoch type either MPID_EPOTYPE_POST or MPID_EPOTYPE_NONE).
 * - Release reference on group.
 *
 * <B>All nodes that allowed RMA access call MPI_Win_wait</B>
 *
 * MPID_Win_wait and MPID_Win_test are interchangeable, for the sake
 * of this discussion.
 *
 * - A sanity-check is done to
 * ensure that the window is in a valid state to end a
 * \e POST access epoch. These checks include testing that
 * a \e POST epoch is currently in affect.
 * - Call advance.
 * - Test if MPID_MSGTYPE_COMPLETE messages have been received from
 * all nodes in group, and that all RMA operations sent to us have been
 * received by us. MPID_Win_test will return FALSE under this condition,
 * while MPID_Win_wait will loop back to the "Call advance" step.
 * - Reset epoch info to indicate the \e POST epoch has ended.
 * - Return TRUE (MPI_SUCCESS).
 */
/// \cond NOT_REAL_CODE
#undef FUNCNAME
#define FUNCNAME MPID_Win_start
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
/// \endcond
/**
 * \brief MPI-DCMF glue for MPI_WIN_START function
 *
 * Begin an access epoch for nodes in group.
 * Waits for all nodes in group to send us a MPID_MSGTYPE_POST message.
 *
 * \param[in] group_ptr		Group
 * \param[in] assert		Synchronization hints
 * \param[in] win_ptr		Window
 * \return MPI_SUCCESS or MPI_ERR_RMA_SYNC.
 *
 * \todo In the NOCHECK case, do we still need to Barrier?
 *
 * \ref post_design
 */
int MPID_Win_start(MPID_Group *group_ptr, int assert, MPID_Win *win_ptr)
{
        int mpi_errno = MPI_SUCCESS;
        MPID_MPI_STATE_DECL(MPID_STATE_MPID_WIN_START);

        MPID_MPI_FUNC_ENTER(MPID_STATE_MPID_WIN_START);
        if (win_ptr->_dev.epoch_type == MPID_EPOTYPE_NONE ||
			win_ptr->_dev.epoch_type == MPID_EPOTYPE_REFENCE) {
                MPIDU_Spin_lock_free(win_ptr);
                win_ptr->_dev.epoch_type = MPID_EPOTYPE_START;
                win_ptr->_dev.epoch_size = group_ptr->size;
        } else if (win_ptr->_dev.epoch_type == MPID_EPOTYPE_POST) {
                win_ptr->_dev.epoch_type = MPID_EPOTYPE_POSTSTART;
        } else {
                /* --BEGIN ERROR HANDLING-- */
                MPIU_ERR_SETANDSTMT(mpi_errno, MPI_ERR_RMA_SYNC,
                                        goto fn_fail, "**rmasync");
                /* --END ERROR HANDLING-- */
        }
        win_ptr->start_assert = assert;
        MPIU_Object_add_ref(group_ptr);
        win_ptr->start_group_ptr = group_ptr;
        /**
         * \todo MPI_MODE_NOCHECK might still include POST messages,
         * so the my_sync_begin counter could be incremented. Need to
         * ensure it gets zeroed (appropriately) later...  This is an
         * erroneous condition and needs to be detected and result in
         * reasonable failure.
         */
        if (!(assert & MPI_MODE_NOCHECK)) {
                MPIDU_Progress_spin(win_ptr->_dev.my_sync_begin < group_ptr->size);
                win_ptr->_dev.my_sync_begin = 0;
        }

fn_exit:
        MPID_MPI_FUNC_EXIT(MPID_STATE_MPID_WIN_START);
        return mpi_errno;

        /* --BEGIN ERROR HANDLING-- */
fn_fail:
        goto fn_exit;
        /* --END ERROR HANDLING-- */
}

/// \cond NOT_REAL_CODE
#undef FUNCNAME
#define FUNCNAME MPID_Win_post
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
/// \endcond
/**
 * \brief MPI-DCMF glue for MPI_WIN_POST function
 *
 * Begin an exposure epoch on nodes in group. Sends MPID_MSGTYPE_POST
 * message to all nodes in group.
 *
 * \param[in] group_ptr	Group
 * \param[in] assert	Synchronization hints
 * \param[in] win_ptr	Window
 * \return MPI_SUCCESS, MPI_ERR_RMA_SYNC, or error returned from
 *	MPIDU_proto_send.
 *
 * \ref post_design
 */
int MPID_Win_post(MPID_Group *group_ptr, int assert, MPID_Win *win_ptr)
{
        int mpi_errno = MPI_SUCCESS;
        volatile unsigned pending = 0;
        MPID_MPI_STATE_DECL(MPID_STATE_MPID_WIN_POST);

        MPID_MPI_FUNC_ENTER(MPID_STATE_MPID_WIN_POST);
        if (win_ptr->_dev.epoch_type != MPID_EPOTYPE_NONE &&
			win_ptr->_dev.epoch_type != MPID_EPOTYPE_REFENCE) {
                /* --BEGIN ERROR HANDLING-- */
                MPIU_ERR_SETANDSTMT(mpi_errno, MPI_ERR_RMA_SYNC,
                                        goto fn_fail, "**rmasync");
                /* --END ERROR HANDLING-- */
        }
        MPIDU_Spin_lock_free(win_ptr);
        MPID_assert_debug(win_ptr->_dev.my_rma_pends == 0 &&
			win_ptr->_dev.my_get_pends == 0);
        win_ptr->_dev.epoch_size = group_ptr->size;
        win_ptr->_dev.epoch_type = MPID_EPOTYPE_POST;
        win_ptr->_dev.epoch_assert = assert;
        win_ptr->_dev.epoch_rma_ok = 1;
        if (assert & MPI_MODE_NOSTORE) {
                /* TBD: anything to optimize? */
        }
        if (assert & MPI_MODE_NOPUT) {
                /* handled later */
        }
        /**
         * \todo In the NOCHECK case, do we still need to Barrier?
         * How do we detect a mismatch of MPI_MODE_NOCHECK in
         * Win_post/Win_start? If the _post has NOCHECK but the _start
         * did not, the _start will wait forever for the POST messages.
         * One option is to still send POST messages in the NOCHECK
         * case, and just not wait in the _start. The POST message
         * could then send the assert value and allow verification
         * when _start nodes call RMA ops.
         */
        if (!(assert & MPI_MODE_NOCHECK)) {
                mpi_errno = MPIDU_proto_send(win_ptr, group_ptr,
                                MPID_MSGTYPE_POST);
                if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
                /**
                 * \todo In theory, we could just return now without
                 * advance/wait.
                 * MPICH2 says this call "does not block", but is
                 * waiting for messages to send considered blocking in
                 * that context? The receiving nodes (in Win_start)
                 * will not procede with RMA ops until they get this
                 * message, so we need to ensure reasonable progress
                 * between the time we call Win_post and Win_wait.
                 * Also, Win_test is not supposed to block in any
                 * fashion, so it should not wait for the sends to
                 * complete either. It seems that the idea is to have
                 * RMA ops going on while this node is executing code
                 * between Win_post and Win_wait, but that won't
                 * happen unless enough calls are made to advance
                 * during that time...
                 * "need input"...
                 */
                MPIDU_Progress_spin(pending > 0);
        }

fn_exit:
        MPID_MPI_FUNC_EXIT(MPID_STATE_MPID_WIN_POST);
        return mpi_errno;
        /* --BEGIN ERROR HANDLING-- */
fn_fail:
        goto fn_exit;
        /* --END ERROR HANDLING-- */
}

/// \cond NOT_REAL_CODE
#undef FUNCNAME
#define FUNCNAME MPID_Win_test
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
/// \endcond
/**
 * \brief MPI-DCMF glue for MPI_WIN_TEST function
 *
 * Test whether the exposure epoch started by MPID_Win_post has ended.
 * If it has ended, clean up and reset window. This routine must call
 * advance at least once, for any code path.
 *
 * \param[in] win_ptr		Window
 * \param[out] flag		Status of synchronization (TRUE = complete)
 * \return MPI_SUCCESS or MPI_ERR_RMA_SYNC.
 *
 * \see mpid_check_post_done
 *
 * \ref post_design
 */
int MPID_Win_test (MPID_Win *win_ptr, int *flag)
{
        int mpi_errno = MPI_SUCCESS;
        MPID_MPI_STATE_DECL(MPID_STATE_MPID_WIN_TEST);

        MPID_MPI_FUNC_ENTER(MPID_STATE_MPID_WIN_TEST);

        if (win_ptr->_dev.epoch_type != MPID_EPOTYPE_POST) {
                /* --BEGIN ERROR HANDLING-- */
                MPIU_ERR_SETANDSTMT(mpi_errno, MPI_ERR_RMA_SYNC,
                                        goto fn_fail, "**rmasync");
                /* --END ERROR HANDLING-- */
        }
        if ((win_ptr->_dev.epoch_assert & MPI_MODE_NOPUT) &&
                                win_ptr->_dev.my_rma_recvs > 0) {
                /* TBD: handled earlier? */
        }
        *flag = (mpid_check_post_done(win_ptr) != 0);

fn_exit:
        MPID_MPI_FUNC_EXIT(MPID_STATE_MPID_WIN_TEST);
        return mpi_errno;
        /* --BEGIN ERROR HANDLING-- */
fn_fail:
        goto fn_exit;
        /* --END ERROR HANDLING-- */
}

/// \cond NOT_REAL_CODE
#undef FUNCNAME
#define FUNCNAME MPID_Win_wait
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
/// \endcond
/**
 * \brief MPI-DCMF glue for MPI_WIN_WAIT function
 *
 * Wait for exposure epoch started by MPID_Win_post to end.
 *
 * \param[in] win_ptr		Window
 * \return MPI_SUCCESS or MPI_ERR_RMA_SYNC.
 *
 * \see mpid_check_post_done
 *
 * \ref post_design
 */
int MPID_Win_wait(MPID_Win *win_ptr)
{
        int mpi_errno = MPI_SUCCESS;
        MPID_MPI_STATE_DECL(MPID_STATE_MPID_WIN_WAIT);

        MPID_MPI_FUNC_ENTER(MPID_STATE_MPID_WIN_WAIT);

        if (win_ptr->_dev.epoch_type != MPID_EPOTYPE_POST) {
                /* --BEGIN ERROR HANDLING-- */
                MPIU_ERR_SETANDSTMT(mpi_errno, MPI_ERR_RMA_SYNC,
                                        goto fn_fail, "**rmasync");
                /* --END ERROR HANDLING-- */
        }
        if ((win_ptr->_dev.epoch_assert & MPI_MODE_NOPUT) &&
                                win_ptr->_dev.my_rma_recvs > 0) {
                /* TBD: handled earlier? */
        }
        while (!mpid_check_post_done(win_ptr)) {
                DCMF_CriticalSection_cycle(0);
        }

fn_exit:
        MPID_MPI_FUNC_EXIT(MPID_STATE_MPID_WIN_WAIT);
        return mpi_errno;
        /* --BEGIN ERROR HANDLING-- */
fn_fail:
        goto fn_exit;
        /* --END ERROR HANDLING-- */
}

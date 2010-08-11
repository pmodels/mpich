/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file src/onesided/mpid_win_lock.c
 * \brief MPI-DCMF MPI_Win_lock/unlock functionality
 */

#include "mpid_onesided.h"

/**
 * \brief Progress (advance) spin to acquire lock locally
 *
 * Adds a local waiter to the lock wait queue, to ensure that
 * we will eventually get a chance. This special waiter (ack[0].w0 == 0)
 * will result in the \e my_sync_done flag getting set, breaking us
 * out of the loop. At this point, we will have acquired the lock
 * (possibly shared with others).
 *
 * Called from MPID_Win_lock when the \e dest rank is ourself (local).
 *
 * \param[in] win	Pointer to MPID_Win object
 * \param[in] rank	Our rank (convenience)
 * \param[in] type	Lock type
 * \return nothing
 */
#define MPIDU_Spin_lock_acquire(win, rank, type) {		\
        if (local_lock(win, rank, type) == 0) {			\
                (win)->_dev.my_sync_done = 0;			\
                MPIDU_add_waiter(win, rank, type, NULL);	\
                MPIDU_Progress_spin((win)->_dev.my_sync_done == 0);\
        }							\
}

/*
 * * * * * Win Locks and Lock wait queue * * * * *
 */

/**
 * \page lock_wait_design Lock Wait Queue Design
 *
 * When a lock cannot be immediately granted, the caller
 * specifics (rank, lock request type, DCMF ack info) is added
 * to the bottom of the lock wait queue. A lock may not be
 * granted for the following reasons:
 *
 * - Window is involved in some other type of epoch.
 * - Lock is in an incompatible state with request type.
 * - Lock and request are shared, but the lock wait queue
 * is not empty. This request must be queued to avoid
 * starvation of the waiter(s).
 *
 * When ever a lock is released, the top of the lock wait queue
 * is examined to see if the waiter is requesting a lock type
 * that is compatible with the current lock status.
 *
 * - Lock is free: any waiter can be granted
 * - Lock is shared: only a share waiter can be granted
 * - Lock is exclusive: no waiter can be granted
 * - Waiter is "dummy" requesting lock type 0, in which case
 * subsequent waiters are left waiting so that another type
 * of epoch can begin.
 *
 * The third case cannot happen since an unlock cannot result
 * in the lock (still) being locked exclusive.
 *
 * The second case would probably only find a waiter of type
 * exclusive, since any shared requests that came along would
 * have been granted.
 *
 * \ref unlk_wait_design
 */

/**
 * \brief Shortcut for accessing the lock waiter queue
 * object in the window structure.
 */
#define MPIDU_WIN_LOCK_QUEUE(w) ((struct mpid_qhead *)(w)->_dev._lock_queue)

/** \brief Test whether lock queue has no waiters */
#define MPIDU_LOCK_QUEUE_EMPTY(w) \
        (MPIDU_WIN_LOCK_QUEUE(w)->blocks == NULL ||	\
        MPIDU_WIN_LOCK_QUEUE(w)->blocks->next_used == NULL)

/** \brief Test if lock is unlocked */
#define MPID_LOCK_IS_FREE(w)	((w)->_dev.lock_granted == 0)
/** \brief Test if lock is locked exclusive */
#define MPID_LOCK_IS_EXCL(w)	((w)->_dev.lock_granted & INT_MSB)
/** \brief Test if lock is locked shared */
#define MPID_LOCK_IS_SHARE(w)	((w)->_dev.lock_granted &&	\
                                !((w)->_dev.lock_granted & INT_MSB))
/** \brief Test if lock is locked exclusive by rank 'r' */
#define MPID_LOCK_ISMY_EXCL(w, r)	\
                                ((w)->_dev.lock_granted == ((r) | INT_MSB))
/** \brief Test if lock is locked shared by rank 'r' \note Can't tell who's locked shared */
#define MPID_LOCK_ISMY_SHARE(w, r)	\
                                ((w)->_dev.lock_granted &&	\
                                !((w)->_dev.lock_granted & INT_MSB))

/** \brief Test if lock is OK to lock exclusive */
#define MPID_LOCK_OK_EXCL(w)	((w)->_dev.lock_granted == 0)
/** \brief Test if lock is OK to lock shared */
#define MPID_LOCK_OK_SHARE(w)	(!((w)->_dev.lock_granted & INT_MSB))

/** \brief Lock the lock in exclusive mode */
#define MPID_LOCK_EXCL(w, r)	((w)->_dev.lock_granted = (r) | INT_MSB)
/** \brief Unlock the lock (from exclusive mode) */
#define MPID_UNLOCK_EXCL(w, r)	((w)->_dev.lock_granted = 0)
/** \brief Lock the lock in shared mode */
#define MPID_LOCK_SHARE(w, r)	(++(w)->_dev.lock_granted)
/** \brief Unlock the lock (from shared mode) */
#define MPID_UNLOCK_SHARE(w, r)	(--(w)->_dev.lock_granted)

/**
 * \brief Examine local lock and return current lock type.
 *
 * \param[in] win	Window object containing lock in question
 * \return Lock type (status):
 *	- \e MPI_LOCK_EXCLUSIVE - locked in exclusive mode.
 *	- \e MPI_LOCK_SHARED - locked by one or more nodes in shared mode.
 *	- \e 0 - not locked.
 *
 * \ref rsrc_design\n
 * \ref lock_wait_design
 */
static int local_lock_type(MPID_Win *win) {
        MPID_assert_debug(MPI_LOCK_EXCLUSIVE != 0 &&
                        MPI_LOCK_SHARED != 0);
        if (MPID_LOCK_IS_FREE(win)) {
                return 0;
        } else if (MPID_LOCK_IS_EXCL(win)) {
                return MPI_LOCK_EXCLUSIVE;
        } else /* MPID_LOCK_IS_SHARE(win) */ {
                return MPI_LOCK_SHARED;
        }
}

/**
 * \brief Local lock routine.
 *
 * Called from lock receive callback.
 * Also sets epoch_rma_ok if lock is acquired.
 *
 * \param[in] win	Pointer to MPID_Win structure
 * \param[in] orig	Rank of origin (locker)
 * \param[in] type	Type of lock being requested
 * \return 1 if the lock was granted, or
 *	0 if the lock was refused (caller must wait).
 *
 * \ref lock_design\n
 * \ref rsrc_design\n
 * \ref lock_wait_design
 */
static unsigned local_lock(MPID_Win *win, int orig, int type) {
        if (type == MPI_LOCK_EXCLUSIVE) {
                if (MPID_LOCK_OK_EXCL(win)) {
                        MPID_LOCK_EXCL(win, orig);
                        return 1;
                }
        } else /* type == MPI_LOCK_SHARED */ {
                if (MPID_LOCK_OK_SHARE(win)) {
                        MPID_LOCK_SHARE(win, orig);
                        MPID_assert_debug(MPID_LOCK_OK_SHARE(win));
                        return 1;
                }
        }
        return 0;
}

/**
 * \brief Local unlock routine.
 *
 * Called from unlock receive callback.
 * Gets origin (unlocker) rank, (expected) lock type, and origin RMA
 * ops count (number of RMA ops that node originated to this node).
 * Returns -1 if the lock is not in appropriate state to be unlocked
 * (by the calling node and expected lock type).
 * Clears epoch_rma_ok if lock is released (completely - i.e. last
 * shared lock release).
 *
 * \param[in] win	Pointer to MPID_Win structure
 * \param[in] orig	Rank of origin (locker)
 * \return 1 if the lock was released, or
 *	0 if the lock was "busy".
 *
 * \ref lock_design\n
 * \ref rsrc_design\n
 * \ref lock_wait_design
 */
static unsigned local_unlock(MPID_Win *win, int orig) {
        MPID_assert_debug(!MPID_LOCK_IS_FREE(win));
        if (MPID_LOCK_IS_EXCL(win)) {
                MPID_UNLOCK_EXCL(win, orig);
        } else if (MPID_LOCK_IS_SHARE(win)) {
                MPID_UNLOCK_SHARE(win, orig);
                MPID_assert_debug(MPID_LOCK_OK_SHARE(win));
        }
        return 1;
}

/** \brief Number of Lock Wait Queue elements per allocation block */
#define MPIDU_NUM_ALLOC_WAITERS	7
/**
 * \brief Lock Wait Queue Element
 *
 * \ref rsrc_design\n
 * \ref lock_wait_design
 */
struct mpid_lock_waiter {
        struct mpid_lock_waiter *next;	/**< next used or next free */
        int waiter_rank;	/**< rank requesting lock */
        int lock_type;		/**< lock type requested */
        int *_pad;		/**< pad to even fraction of cacheline */
        MPIDU_Onesided_ctl_t ackinfo;	/**< dcmf context (opaque) info.
                                 * Not directly used in communications. */
};
/** \brief Padding for  Lock Wait Queue Element resource block header */
#define MPIDU_PAD_ALLOC_WAITERS	\
(sizeof(struct mpid_lock_waiter) - sizeof(struct mpid_resource))

/**
 * \brief Setup a lock wait queue entry.
 *
 * Adds a new waiter to the (end of the) wait queue.
 * Saves rank, type, and ackinfo in the wait object.
 * Typically called from lock receive callback.
 *
 * A lock type of 0 (none) is used as a "break-point" to
 * ensure non-lock epoch starts will eventually succeed.
 *
 * \param[in] win	Pointer to MPID_Win structure
 * \param[in] rank	Rank of origin (locker)
 * \param[in] type	Type of lock being requested
 * \param[in] ackinfo	Additional info to save on queue (may be NULL)
 * \return nothing
 *
 * \ref rsrc_design\n
 * \ref lock_wait_design
 */
static void MPIDU_add_waiter(MPID_Win *win, int rank, int type,
                                        MPIDU_Onesided_ctl_t *ackinfo) {
        struct mpid_lock_waiter wp;

        wp.waiter_rank = rank;
        wp.lock_type = type;
        if (ackinfo) {
                wp.ackinfo = *ackinfo;
        } else {
                memset(&wp.ackinfo, 0, sizeof(wp.ackinfo));
        }
        (void)MPIDU_add_element(MPIDU_WIN_LOCK_QUEUE(win), &wp);
}

/**
 * \brief Conditionally pops next waiter off lock wait queue.
 *
 * Returns the next waiter on the window lock, provided its desired
 * lock type is compatible with the current lock status.
 * Fills in rank, type, and ackinfo with pertinent data,
 * on success. Called by unlock (recv callback). If lock status
 * (after the unlock) is (still) shared, then the next waiter must
 * want shared (probably not a likely scenario since it should have
 * been granted the lock when requested). However, this check ensures
 * that an exclusive lock waiter cannot grab a lock that is still in
 * shared mode. Typically called from unlock receive callback.
 *
 * \param[in] win	Pointer to MPID_Win structure
 * \param[out] rank	Rank of origin (locker)
 * \param[out] type	Type of lock being requested
 * \param[out] ackinfo	Additional info from original lock call (may be NULL)
 * \return 0 if lock waiter was popped, or
 *	1 if the queue was empty or next waiter incompatible
 *
 * \ref lock_design\n
 * \ref rsrc_design\n
 * \ref lock_wait_design
 */
static int MPIDU_pop_waiter(MPID_Win *win, int *rank, int *type,
                                        MPIDU_Onesided_ctl_t *ackinfo) {
        struct mpid_lock_waiter wp;
        struct mpid_resource *lq = MPIDU_WIN_LOCK_QUEUE(win)->blocks;
        int lt;

        if (lq == NULL || lq->next_used == NULL) {
                return 1; /* no one waiting */
        }
        if (MPIDU_peek_element(MPIDU_WIN_LOCK_QUEUE(win), &wp)) {
                return 1; /* no one waiting */
        }
        if (!rank && !type && wp.lock_type != 0) {
                return 1;	/* not our marker... */
        }
        lt = local_lock_type(win);
        if (lt == MPI_LOCK_EXCLUSIVE ||
                        (lt != 0 && lt != wp.lock_type)) {
                return 1; /* not a compatible waiter */
        }
        (void)MPIDU_pop_element(MPIDU_WIN_LOCK_QUEUE(win), NULL);
        if (rank) {
                *rank = wp.waiter_rank;
        }
        if (type) {
                *type = wp.lock_type;
        }
        if (ackinfo) {
                *ackinfo = wp.ackinfo;
        }
        return 0;
}

/**
 * \brief Progress (advance) wait for window lock to be released
 *
 * Adds a dummy waiter to the lock wait queue, so ensure that
 * unlock will eventually give us a chance.
 *
 * Called from various epoch-start code to ensure no other node is
 * accessing our window while we are in another epoch.
 *
 * \todo Probably sohuld assert that the popped waiter,
 * if any, was our NULL one.
 *
 * \param[in] win       Pointer to MPID_Win object
 * \return nothing
 */
void MPIDU_Spin_lock_free(MPID_Win *win) {
        MPIDU_add_waiter(win, 0, 0, NULL);
        MPIDU_Progress_spin(!MPID_LOCK_IS_FREE(win));
        MPIDU_pop_waiter(win, NULL, NULL, NULL);
}

int MPIDU_is_lock_free(MPID_Win *win) {
        return MPID_LOCK_IS_FREE(win);
}

/*
 * * * * * Unlock wait queue * * * * *
 */

/**
 * \page unlk_wait_design Unlock Wait Queue Design
 *
 * The Unlock Wait Queue is used to delay unlocking of a
 * window until all outstanding RMA operations have completed.
 *
 * Each unlock request includes the number of RMA operations
 * that were initiated by that origin. When the unlock is attempted,
 * this number is compared to the count of RMA ops received from that
 * origin and if the numbers do not match the unlock request is queued.
 *
 * Whenever an RMA operation is processed, the routine \e rma_recvs_cb()
 * is called and the unlock wait queue is checked for an entry from that
 * origin node, and that the counts now match. If so, the unlock is dequeued
 * and the lock released (acknowledge message sent to origin).
 *
 * Note that this unlock may include granting of the lock to other
 * nodes (i.e. processing of the lock wait queue).
 *
 * \ref lock_wait_design
 */

/**
 * \brief Shortcut for accessing the lock waiter queue
 * object in the window structure.
 */
#define MPIDU_WIN_UNLK_QUEUE(w) ((struct mpid_qhead *)(w)->_dev._unlk_queue)

/** \brief Number of Unlock Wait Queue elements per allocation block */
#define MPIDU_NUM_UNLK_ENTRIES	7
/**
 * \brief Unlock Wait Queue Element
 */
struct mpid_unlk_entry {
        struct mpid_unlk_entry *next;	/**< next used or next free */
        int rank;		/**< origin rank (unlocker) */
        int rmas;		/**< number of rmas sent by origin */
        int _pad;		/**< pad to power of 2 size */
};
/** \brief Padding for Datatype Cache Element resource block header */
#define MPIDU_PAD_UNLK_ENTRIES	0

void mpidu_init_lock(MPID_Win *win) {
        MPIDU_INIT_QHEAD(MPIDU_WIN_LOCK_QUEUE(win),
                MPIDU_NUM_ALLOC_WAITERS,
                sizeof(struct mpid_lock_waiter),
                MPIDU_PAD_ALLOC_WAITERS);
        MPIDU_INIT_QHEAD(MPIDU_WIN_UNLK_QUEUE(win),
                MPIDU_NUM_UNLK_ENTRIES,
                sizeof(struct mpid_unlk_entry),
                MPIDU_PAD_UNLK_ENTRIES);
}

void mpidu_free_lock(MPID_Win *win) {
        MPID_assert_debug(MPIDU_WIN_LOCK_QUEUE(win)->blocks == NULL ||
                MPIDU_WIN_LOCK_QUEUE(win)->blocks->next_used == NULL);
        MPIDU_free_resource(MPIDU_WIN_LOCK_QUEUE(win));
        MPIDU_free_resource(MPIDU_WIN_UNLK_QUEUE(win));
}

/**
 * \brief Callback function to match unlock wait queue entry
 *
 * 'v1' is a struct mpid_unlk_entry with rank and rmas filled in with
 *      desired origin rank and current RMA ops count from origin.
 * 'v2' is the (currrent) struct mpid_unlk_entry being examined as
 *      a potential match.
 *
 * Returns success (match) if an unlock element exists with origin rank
 * and the expected RMA ops count from that rank has been reached.
 *
 * \param[in] v1	Desired unlock queue pseudo-element
 * \param[in] v2	Unlock queue element to compare with 'v1'
 * \param[in] v3	not used
 * \return boolean indicating if 'v2' matches 'v1'.
 *
 * \ref unlk_wait_design
 */
static int mpid_match_unlk(void *v1, void *v2, void *v3) {
        struct mpid_unlk_entry *w1 = (struct mpid_unlk_entry *)v1;
        struct mpid_unlk_entry *w2 = (struct mpid_unlk_entry *)v2;

        return (w1->rank != w2->rank || w1->rmas < w2->rmas);
}

/**
 * \brief Locate the desired unlocker if it is waiting
 *
 * Does not return success unless the RMA ops counters match.
 *
 * \param[in] win	Pointer to window
 * \param[in] rank	Origin rank (unlocker)
 * \param[out] ctl	Reconstructed UNLOCK message, if unlocker found
 *
 * \ref unlk_wait_design
 * \ref msginfo_usage
 */
static struct mpid_unlk_entry *MPIDU_locate_unlk(MPID_Win *win, int rank, MPIDU_Onesided_ctl_t *ctl) {
        struct mpid_unlk_entry el, *ep;
        struct mpid_element *pp = NULL;

        el.rank = rank;
        el.rmas = win->_dev.coll_info[rank].rma_sends;
        ep = MPIDU_find_element(MPIDU_WIN_UNLK_QUEUE(win), mpid_match_unlk, NULL, &el, &pp);
        if (ep) {
                if (ctl) {
                        ctl->mpid_ctl_w0 = MPID_MSGTYPE_UNLOCK;
                        ctl->mpid_ctl_w1 = win->handle;
                        ctl->mpid_ctl_w2 = ep->rank;
                        ctl->mpid_ctl_w3 = ep->rmas;
                }
                MPIDU_free_element(MPIDU_WIN_UNLK_QUEUE(win), ep, pp);
        }
        return ep;
}

/**
 * \brief Add an (unsuccessful) unlocker to the wait queue
 *
 * Decomposes the UNLOCK message to save needed data.
 *
 * \param[in] win	Pointer to window
 * \param[in] rank	Origin rank (unlocker)
 * \param[in] ctl	UNLOCK message
 *
 * \ref unlk_wait_design
 * \ref msginfo_usage
 */
static void MPIDU_add_unlk(MPID_Win *win, int rank, const MPIDU_Onesided_ctl_t *ctl) {
        struct mpid_unlk_entry wp;

        MPID_assert_debug(ctl != NULL);
        MPID_assert_debug(rank == ctl->mpid_ctl_w2);
        wp.rank = rank;
        wp.rmas = ctl->mpid_ctl_w3;
        (void)MPIDU_add_element(MPIDU_WIN_UNLK_QUEUE(win), &wp);
}

/**
 * \brief Callback invoked to count an RMA operation received
 *
 * Increments window's \e my_rma_recvs counter.
 * If window lock is held, then also increment RMA counter
 * for specific origin node, and check whether this RMA op
 * completes the epoch and an unlock is waiting to be processed.
 *
 * We use \e rma_sends to count received RMA ops because we
 * know we won't be using that to count sent RMA ops since
 * we cannot be in an access epoch while in a LOCK exposure epoch.
 *
 * Called from both the "long message" completion callbacks and
 * the "short message" receive callback, in case of PUT or
 * ACCUMULATE only.
 *
 * \param[in] win	Pointer to MPID_Win object
 * \param[in] orig	Rank of originator of RMA operation
 * \param[in] lpid	lpid of originator of RMA operation
 * \return nothing
 */
void rma_recvs_cb(MPID_Win *win, int orig, int lpid, int count) {
        win->_dev.my_rma_recvs += count;
        if (!MPID_LOCK_IS_FREE(win)) {
                struct mpid_unlk_entry *ep;
                MPIDU_Onesided_ctl_t ctl;

                win->_dev.coll_info[orig].rma_sends += count;
                ep = MPIDU_locate_unlk(win, orig, &ctl);
                if (ep) {
                        unlk_cb(&ctl, lpid);
                }
        }
}

/**
 * \brief Lock receive callback.
 *
 * Attempts to acquire the lock.
 * On success, sends ACK to origin.
 * On failure to acquire lock,
 * adds caller to lock wait queue.
 *
 * Does not attempt to acquire lock (counted as failure)
 * if window is currently in some other epoch.
 *
 * \param[in] info	Pointer to msginfo from origin (locker)
 * \param[in] lpid	lpid of origin node (locker)
 * \return nothing
 *
 * \ref msginfo_usage\n
 * \ref lock_design
 */
void lock_cb(const MPIDU_Onesided_ctl_t *info, int lpid)
{
        MPID_Win *win;
        int ret;
        int orig, type;
        MPIDU_Onesided_ctl_t ack;

        MPID_assert_debug(info->mpid_ctl_w0 == MPID_MSGTYPE_LOCK);
        MPID_Win_get_ptr((MPI_Win)info->mpid_ctl_w1, win);
        MPID_assert_debug(win != NULL);
        orig = info->mpid_ctl_w2;
        type = info->mpid_ctl_w3;
        ack.mpid_ctl_w0 = MPID_MSGTYPE_LOCKACK;
        ack.mpid_ctl_w1 = win->_dev.coll_info[orig].win_handle;
        ack.mpid_ctl_w2 = lpid;
        ack.mpid_ctl_w3 = 0;
        ret = ((win->_dev.epoch_type == MPID_EPOTYPE_NONE ||
		win->_dev.epoch_type == MPID_EPOTYPE_REFENCE) &&
                                        local_lock(win, orig, type));
        if (!ret) {
                MPIDU_add_waiter(win, orig, type, &ack);
        } else {
                win->_dev.epoch_rma_ok = 1;
                (void) DCMF_Control(&bg1s_ct_proto, win->_dev.my_cstcy, lpid, &ack.ctl);
        }
}

/**
 * \brief Epoch End callback.
 *
 * Called whenever epoch_type is set to MPID_EPOTYPE_NONE, i.e. an
 * access/exposure epoch ends. Also called when the window lock is
 * released (by the origin node).
 *
 * This is used to prevent locks from being acquired while some other
 * access/exposure epoch is active on a window, and queues the lock
 * attempt until such time as the epoch has ended.
 *
 * \param[in] win	Pointer to MPID_Win whose epoch has ended
 */
void epoch_end_cb(MPID_Win *win) {
        int rank, type, lpid;
        MPIDU_Onesided_ctl_t info;
        int ret;

        /*
         * Wake up any waiting lockers.
         *
         * This works in the case of a shared-lock release when not all
         * lockers have released (no compatible waiter will be found).
         *
         * This also works in the case of non-lock epochs ending.
         *
         * An epoch-start call will spin waiting for lock to be released.
         * Before spinning, it will queue a waiter with lock type 0 (none),
         * so that this loop will not block progress indefinitely.
         */
        while (MPIDU_pop_waiter(win, &rank, &type, &info) == 0 &&
                                                        type != 0) {
                /* compatible waiter found */
                ret = local_lock(win, rank, type);
                MPID_assert_debug(ret != 0);
                if (info.mpid_ctl_w0 == 0) {	/* local request */
                        ++win->_dev.my_sync_done;
                } else {
                        win->_dev.epoch_rma_ok = 1;
                        lpid = info.mpid_ctl_w2;
                        (void) DCMF_Control(&bg1s_ct_proto, win->_dev.my_cstcy, lpid, &info.ctl);
                }
        }
}

/**
 * \brief Unlock receive callback.
 *
 * Attempts to release the lock.
 * If the lock cannot be released (due to outstanding RMA ops not
 * yet received) then the unlocker is placed on a queue where its
 * request will be re-evaluated when RMA ops are received.
 * If lock can be released, any lock waiters are woken up in
 * \e epoch_end_cb() and an MPID_MSGTYPE_UNLOCKACK is sent to the unlocker.
 *
 * \param[in] info	Pointer to msginfo from origin (unlocker)
 * \param[in] lpid	lpid of origin node (unlocker)
 * \return nothing
 *
 * \ref msginfo_usage\n
 * \ref lock_design
 */
void unlk_cb(const MPIDU_Onesided_ctl_t *info, int lpid) {
        MPID_Win *win;
        unsigned ret;
        int orig, rmas;

        MPID_assert_debug(info->mpid_ctl_w0 == MPID_MSGTYPE_UNLOCK);
        MPID_Win_get_ptr((MPI_Win)info->mpid_ctl_w1, win);
        MPID_assert_debug(win != NULL);
        orig = info->mpid_ctl_w2;
        rmas = info->mpid_ctl_w3;
        ret = ((rmas && win->_dev.coll_info[orig].rma_sends < rmas) ||
                local_unlock(win, orig));
        if (ret) {	/* lock was released */
                MPIDU_Onesided_ctl_t ack;
                if (MPID_LOCK_IS_FREE(win)) {
			epoch_clear(win);
                }
                epoch_end_cb(win);
                ack.mpid_ctl_w0 = MPID_MSGTYPE_UNLOCKACK;
                ack.mpid_ctl_w1 = win->_dev.coll_info[orig].win_handle;
                ack.mpid_ctl_w2 = mpid_my_lpid;
                ack.mpid_ctl_w3 = 0;
                (void) DCMF_Control(&bg1s_ct_proto, win->_dev.my_cstcy, lpid, &ack.ctl);
        } else {
                MPIDU_add_unlk(win, orig, info);
        }
}

/**
 * \page lock_design MPID_Win_lock / unlock Design
 *
 * MPID_Win_lock/unlock use DCMF_Control() to send messages (both requests and
 * acknowledgements). All aspects of locking, including queueing waiters for
 * lock and unlock, is done in this layer, in this source file.
 *
 * A lock request / RMA / unlock sequence is as follows:
 *
 * <B>MPI_Win_lock Called</B>
 *
 * - A sanity-check is done to
 * ensure that the window is in a valid state to enter a
 * \e LOCK access epoch. These checks include testing that
 * no other epoch is currently in affect.
 * - If the local window is currenty locked then wait for the
 * lock to be released (calling advance in the loop).
 * This is made deterministic by inserting a "dummy" waiter on
 * the lock wait queue which will cause an unlock to stop
 * trying to grant lock waiters.
 * - If MPI_MODE_NOCHECK was specified, then return success now.
 * - Setup a msginfo structure with the msg type, target window
 * handle, our rank, and lock type, and call DCMF_Control to start
 * the message on its way to the target.
 * Spin waiting for both the message to send and the my_sync_done flag
 * to get set (by receive callback of MPID_MSGTYPE_LOCKACK message) indicating the lock
 * has been granted.
 *
 * <B>On the target node the MPID_MSGTYPE_LOCK callback is invoked</B>
 *
 * - If the lock cannot be granted, either because the target node
 * is currently involved in some other access/exposure epoch or the lock
 * is currently granted in an incompatible mode:
 *  - An entry is added to the end of the lock wait queue,
 * containing the rank, lock mode, and ack info,
 * and the callback returns to the message layer without sending
 * MPID_MSGTYPE_LOCKACK,
 * which causes the origin node to wait.
 *  - At some point in the future a node unlocks the window, or the
 * current epoch ends, at
 * which time this entry is removed from the lock wait queue and
 * progress continues with the lock granted.
 * - If (when) the lock can be granted, by a call to epoch_end_cb() either from
 * a specific MPID_* epoch-ending synchronization or target processing of MPI_Win_unlock():
 *  - As long as compatible waiters are found at the head of the lock wait queue,
 * an MPID_MSGTYPE_LOCKACK message is created from the waiter info and sent to the
 * (each) origin node, causing the origin's my_sync_done flag to get set, waking it up.
 *
 * <B>Origin wakes up after lock completion</B>
 *
 * - Set epoch type, target node, and MPI_MODE_* flags in window.
 * This effectively creates the epoch.
 *
 * <B>Origin invokes RMA operation(s)</B>
 *
 * - See respective RMA operation calls for details
 *
 * <B>Origin calls MPI_Win_unlock</B>
 *
 * - Basic sanity-checks are done, including testing that the
 * window is actually in a \e LOCK access epoch and that the target
 * node specified is the same as the target node of the MPI_Win_lock.
 * - If any RMA operations (sends) are pending, wait for them to be
 * sent (calling advance in the loop).
 * - If MPI_MODE_NOCHECK was not asserted in the original lock call:
 *  - setup message with the msg type MPID_MSGTYPE_UNLOCK, target window handle,
 * our rank, and the number of RMA operations that were initiated
 * to this target.
 *  - Call DCMF_Control, to send the message (unlock request).
 *  - Spin waiting for message to send and my_sync_done to get set.
 *
 * <B>On the target node the unlock callback is invoked</B>
 *
 *  - Sanity-check the unlock to ensure it matches the original lock.
 *  - If the number of RMA operations sent to us by the origin exceeds the number
 * of operations received from the origin:
 *   - add the unlock request to the unlock wait queue.
 * Receive callbacks for RMA operations
 * will update the RMA ops counter(s) and process any unlock waiters who's
 * counts now match.
 *  - Otherwise, Release the lock:
 *   - Call epoch_end_cb() which will
 * generate MPID_MSGTYPE_LOCKACK messages to all compatible lock waiters.
 *   - Send an MPID_MSGTYPE_UNLOCKACK message to the origin.  This message
 * causes the origin's my_sync_done flag to get set, waking it up.
 *
 * <B>Origin wakes up after unlock completion</B>
 *
 *  - Reset epoch info in window to indicate the epoch has ended.
 */
/// \cond NOT_REAL_CODE
#undef FUNCNAME
#define FUNCNAME MPID_Win_lock
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
/// \endcond
/**
 * \brief MPI-DCMF glue for MPI_WIN_LOCK function
 *
 * Begin an access epoch to node \e dest.
 * Does not return until target has locked window.
 *
 * epoch_size is overloaded here, since the assumed
 * epoch size for MPID_EPOTYPE_LOCK is 1. We use this
 * field to save the target (locked) rank.  This can
 * be used later to validate the target of an RMA operation
 * or to sanity-check the unlock.
 *
 * \param[in] lock_type	Lock type (exclusive or shared)
 * \param[in] dest	Destination rank (target)
 * \param[in] assert	Synchronization hints
 * \param[in] win_ptr	Window
 * \return MPI_SUCCESS, MPI_ERR_RMA_SYNC, or error returned from
 *	DCMF_Lock.
 *
 * \ref msginfo_usage\n
 * \ref lock_design
 */
int MPID_Win_lock(int lock_type, int dest, int assert,
                                        MPID_Win *win_ptr)
{
        int mpi_errno = MPI_SUCCESS;
        MPIDU_Onesided_ctl_t info;
        int lpid;
        MPID_MPI_STATE_DECL(MPID_STATE_MPID_WIN_LOCK);

        MPID_MPI_FUNC_ENTER(MPID_STATE_MPID_WIN_LOCK);

        MPIU_UNREFERENCED_ARG(assert);

        if (dest == MPI_PROC_NULL) goto fn_exit;

        if (win_ptr->_dev.epoch_type != MPID_EPOTYPE_NONE &&
			win_ptr->_dev.epoch_type != MPID_EPOTYPE_REFENCE) {
                /* --BEGIN ERROR HANDLING-- */
                MPIU_ERR_SETANDSTMT(mpi_errno, MPI_ERR_RMA_SYNC,
                                        goto fn_fail, "**rmasync");
                /* --END ERROR HANDLING-- */
        }
        /**
         * \todo Should we pass NOCHECK along with RMA ops,
         * so that target can confirm?
`	 */
        if (!(win_ptr->_dev.epoch_assert & MPI_MODE_NOCHECK)) {
                if (dest == win_ptr->_dev.comm_ptr->rank) {
                        MPIDU_Spin_lock_acquire(win_ptr, dest, lock_type);
                } else {
                        info.mpid_ctl_w0 = MPID_MSGTYPE_LOCK;
                        info.mpid_ctl_w1 = win_ptr->_dev.coll_info[dest].win_handle;
                        info.mpid_ctl_w2 = win_ptr->_dev.comm_ptr->rank;
                        info.mpid_ctl_w3 = lock_type;
                        lpid = MPIDU_world_rank(win_ptr, dest);
                        win_ptr->_dev.my_sync_done = 0;
                        mpi_errno = DCMF_Control(&bg1s_ct_proto, win_ptr->_dev.my_cstcy, lpid, &info.ctl);
                        if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
                        MPIDU_Progress_spin(win_ptr->_dev.my_sync_done == 0);
                }
        }

        win_ptr->_dev.epoch_type = MPID_EPOTYPE_LOCK;
        win_ptr->_dev.epoch_size = dest;
        win_ptr->_dev.epoch_assert = assert;

fn_exit:
        MPID_MPI_FUNC_EXIT(MPID_STATE_MPID_WIN_LOCK);
        return mpi_errno;
        /* --BEGIN ERROR HANDLING-- */
fn_fail:
        goto fn_exit;
        /* --END ERROR HANDLING-- */
}

/// \cond NOT_REAL_CODE
#undef FUNCNAME
#define FUNCNAME MPID_Win_unlock
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
/// \endcond
/**
 * \brief MPI-DCMF glue for MPI_WIN_UNLOCK function
 *
 * End access epoch started by MPID_Win_lock.
 * Sends to target the number of RMA ops we performed.
 * Target node will not unlock until it has received all RMA ops we sent.
 * While unlock failed call advance.
 *
 * \param[in] dest	Destination rank (target)
 * \param[in] win_ptr	Window
 * \return MPI_SUCCESS, MPI_ERR_RMA_SYNC, or error returned from
 *	DCMF_Unlock.
 *
 * \ref msginfo_usage\n
 * \ref lock_design
 */
int MPID_Win_unlock(int dest, MPID_Win *win_ptr)
{
        int mpi_errno = MPI_SUCCESS;
        int lpid;
        MPIDU_Onesided_ctl_t info;
        MPID_MPI_STATE_DECL(MPID_STATE_MPID_WIN_UNLOCK);

        MPID_MPI_FUNC_ENTER(MPID_STATE_MPID_WIN_UNLOCK);

        if (dest == MPI_PROC_NULL) goto fn_exit;

        if (win_ptr->_dev.epoch_type != MPID_EPOTYPE_LOCK) {
                /* --BEGIN ERROR HANDLING-- */
                MPIU_ERR_SETANDSTMT(mpi_errno, MPI_ERR_RMA_SYNC,
                                        goto fn_fail, "**rmasync");
                /* --END ERROR HANDLING-- */
        }
        MPID_assert(dest == win_ptr->_dev.epoch_size);

        /*
         * We wait for all RMA sends to drain here, just for neatness.
         * TBD: It may be possible to do this only in the advance loop
         * after the unlock request.
         */
        MPIDU_Progress_spin(win_ptr->_dev.my_rma_pends > 0 ||
				win_ptr->_dev.my_get_pends > 0);

        if (!(win_ptr->_dev.epoch_assert & MPI_MODE_NOCHECK)) {
                if (dest == win_ptr->_dev.comm_ptr->rank) {
                        (void)local_unlock(win_ptr, dest);
                        /* our (subsequent) call to epoch_end_cb() will
                         * handle any lock waiters... */
                } else {
                        info.mpid_ctl_w0 = MPID_MSGTYPE_UNLOCK;
                        info.mpid_ctl_w1 = win_ptr->_dev.coll_info[dest].win_handle;
                        info.mpid_ctl_w2 = win_ptr->_dev.comm_ptr->rank;
                        info.mpid_ctl_w3 = win_ptr->_dev.coll_info[dest].rma_sends;
                        /*
                         * Win_unlock should not return until all RMA ops are
                         * complete at the target. So, we loop here until the
                         * target tells us all RMA ops are finished. We also
                         * zero the rma_sends param in the loop, so that the
                         * target can just always += the number and not get an
                         * unreasonable number of pending ops, plus should we
                         * ever decide to do other RMA ops between attempts to
                         * unlock, we can pass that number to the target and it
                         * will update its counter.
                         */
                        lpid = MPIDU_world_rank(win_ptr, dest);
                        win_ptr->_dev.my_sync_done = 0;
                        mpi_errno = DCMF_Control(&bg1s_ct_proto, win_ptr->_dev.my_cstcy, lpid, &info.ctl);
                        if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
                        MPIDU_Progress_spin(win_ptr->_dev.my_rma_pends > 0 ||
					win_ptr->_dev.my_get_pends > 0 ||
                                        win_ptr->_dev.my_sync_done == 0);
                }
        }
        win_ptr->_dev.epoch_size = 0;
        win_ptr->_dev.epoch_assert = 0;
	epoch_clear(win_ptr);
        epoch_end_cb(win_ptr);

fn_exit:
        MPID_MPI_FUNC_EXIT(MPID_STATE_MPID_WIN_UNLOCK);
        return mpi_errno;
        /* --BEGIN ERROR HANDLING-- */
fn_fail:
        goto fn_exit;
        /* --END ERROR HANDLING-- */
}

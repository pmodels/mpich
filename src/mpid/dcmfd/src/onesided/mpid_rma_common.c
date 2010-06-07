/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file src/onesided/mpid_rma_common.c
 * \brief MPI-DCMF Code and data common to RMA modules
 */

#include "mpid_onesided.h"

#if 0
char *msgtypes[] = {
[MPID_MSGTYPE_NONE] = "MPID_MSGTYPE_NONE",
[MPID_MSGTYPE_LOCK] = "MPID_MSGTYPE_LOCK",
[MPID_MSGTYPE_UNLOCK] = "MPID_MSGTYPE_UNLOCK",
[MPID_MSGTYPE_POST] = "MPID_MSGTYPE_POST",
[MPID_MSGTYPE_START] = "MPID_MSGTYPE_START",
[MPID_MSGTYPE_COMPLETE] = "MPID_MSGTYPE_COMPLETE",
[MPID_MSGTYPE_WAIT] = "MPID_MSGTYPE_WAIT",
[MPID_MSGTYPE_FENCE] = "MPID_MSGTYPE_FENCE",
[MPID_MSGTYPE_UNFENCE] = "MPID_MSGTYPE_UNFENCE",
[MPID_MSGTYPE_PUT] = "MPID_MSGTYPE_PUT",
[MPID_MSGTYPE_GET] = "MPID_MSGTYPE_GET",
[MPID_MSGTYPE_ACC] = "MPID_MSGTYPE_ACC",
[MPID_MSGTYPE_DT_MAP] = "MPID_MSGTYPE_DT_MAP",
[MPID_MSGTYPE_DT_IOV] = "MPID_MSGTYPE_DT_IOV",
[MPID_MSGTYPE_LOCKACK] = "MPID_MSGTYPE_LOCKACK",
[MPID_MSGTYPE_UNLOCKACK] = "MPID_MSGTYPE_UNLOCKACK",
};
#endif

/** \brief DCMF Protocol object for DCMF_Send() calls */
DCMF_Protocol_t bg1s_sn_proto;
/** \brief DCMF Protocol object for DCMF_Put() calls */
DCMF_Protocol_t bg1s_pt_proto;
/** \brief DCMF Protocol object for DCMF_Get() calls */
DCMF_Protocol_t bg1s_gt_proto;
/** \brief DCMF Protocol object for DCMF_Control() calls */
DCMF_Protocol_t bg1s_ct_proto;

/**
 * \page msginfo_usage Message info (DCQuad) usage conventions
 *
 * First (or only) quad:
 *	- \e w0 = Message type (MPID_MSGTYPE_*)
 *	- \e w1 = (target) window handle
 *		(except Datatype map/iov messages use num elements)
 *	- \e w2 = (originating) rank
 *	- \e w3 = Depends on message type:
 *<TABLE BORDER=0 CELLPADDING=0>
 *<TR><TH ALIGN="RIGHT">Epoch End:</TH>
 *	<TD>RMA send count</TD></TR>
 *<TR><TH ALIGN="RIGHT">Put/Accumulate:</TH>
 *	<TD>target memory address</TD></TR>
 *<TR><TH ALIGN="RIGHT">Lock:</TH>
 *	<TD>lock type</TD></TR>
 *<TR><TH ALIGN="RIGHT">Datatype/loop:</TH>
 *	<TD>datatype (handle on origin)</TD></TR>
 *</TABLE>
 *
 * Additional quads are message-specific
 *
 * MPID_MSGTYPE_ACC:
 *	- \e w0 = target datatype (handle on origin)
 *	- \e w1 = operand handle (must be builtin)
 *	- \e w2 = length (number of datatype instances)
 *	- \e w3 = 0
 *
 * \note Epoch End includes MPID_MSGTYPE_UNLOCK and MPID_MSGTYPE_COMPLETE
 * (MPID_Win_fence() uses MPIR_Allreduce_impl()).
 */

/** \brief global for our lpid */
unsigned mpid_my_lpid = -1;

/**
 * \brief Build datatype map and iovec
 *
 * \param[in] dt	Datatype to build map/iov for
 * \param[out] dti	Pointer to datatype info struct
 */
void make_dt_map_vec(MPI_Datatype dt, mpid_dt_info *dti) {
        int nb;
	DLOOP_Offset last;
        MPID_Segment seg;
        MPID_Type_map *mv;
        DLOOP_VECTOR *iv;
        int i;
        MPI_Datatype eltype;
        unsigned size;
        MPID_Datatype *dtp;
        /* NOTE: we know "dt" is not builtin, else why do this? */

        /* Use existing routines to get IOV */

        MPID_Datatype_get_ptr(dt, dtp);
        nb = dtp->max_contig_blocks + 1;

        MPIDU_MALLOC(mv, MPID_Type_map, nb * sizeof(*mv), last, "MPID_Type_map");
        MPID_assert(mv != NULL);
        iv = (DLOOP_VECTOR *)mv;
        MPID_Segment_init(NULL, 1, dt, &seg, 0);
        last = dtp->size;
        MPID_Segment_pack_vector(&seg, 0, &last, iv, &nb);
        if (HANDLE_GET_KIND(dtp->eltype) == HANDLE_KIND_BUILTIN) {
                eltype = dtp->eltype;
                size = MPID_Datatype_get_basic_size(eltype);
        } else {
                eltype = 0;
                size = 0; /* don't care */
        }
        /* This works because we go backwards, and DLOOP_VECTOR << MPID_Type_map */
        for (i = nb; i > 0; ) {
                --i;
                mv[i].off = (size_t)iv[i].DLOOP_VECTOR_BUF;
                mv[i].len = iv[i].DLOOP_VECTOR_LEN;
                mv[i].num = (eltype ? mv[i].len / size : 0);
                mv[i].dt = eltype;
        }
        dti->map_len = nb;
        dti->map = mv;
        dti->dtp = dtp;
}

/**
 * \brief Datatype created to represent the rma_sends element
 * of the coll_info array of the window structure
 */
MPI_Datatype Coll_info_rma_dt;
/**
 * \brief User defined function created to process the rma_sends
 * elements of the coll_info array of the window structure
 */
MPI_Op Coll_info_rma_op;

/*
 * * * * * Generic resource pool management * * * * *
 */

/**
 * \page rsrc_design Basic resource element design
 *
 * Generic resources are managed through a \e mpid_qhead structure
 * which defines the basic geometry of the allocation blocks and
 * references the first allocated block.
 *
 * Generic resources are allocated in blocks. Each block begins
 * with a header of \e mpid_resource and is followed by a number
 * of elements whose size and count are defined for each resource
 * type. Also, the header size is defined, which may be larger
 * than the natural size of \e mpid_resource in order to optimize
 * memory layout (i.e. cache usage).
 *
 * Resource blocks are never freed (exception: lock wait queue
 * resources are freed when the window is freed).
 *
 * Specific resources are defined by their elements. Every resource
 * element must have as its first field the \e next pointer, as
 * defined by \e mpid_element. Following this is
 * defined by the needs of the specific resource.
 *
 * A newly allocated block is initialized with all elements \e next
 * chained together and the block's \e next_free pointer set to the
 * first (free) element. Except for the first allocated block
 * (the one directly referenced by \e mpid_qhead), the \e next_free
 * is not used again.  This also applies to the \e next_used and
 * \e last_used pointers.
 *
 * A newly allocated secondary block is linked into the \e mpid_qhead
 * \e next_block chain and \e next_free chain.
 *
 * When an element is taken from the free list, it is always taken
 * from the top of the list, i.e. directly from \e next_free.
 * When an element is returned to the free list, it is placed
 * (pushed) on the top. So an element to be allocated is always the
 * one most-recently freed, i.e. a LIFO queue.
 *
 * When an element is added to the used list, it is always added
 * to the end of the list, i.e. using the \e last_used pointer.
 * If an arbitrary element is taken from the used list, it is
 * taken from the top of the list, i.e. using \e next_used.  This
 * effectively forms a FIFO queue.
 *
 * Other routines exist that permit the used list to be searched,
 * and the found element may be removed from the used list
 * "out of turn". A specific resource implementation decides
 * how it will use this list.
 */

/**
 * \brief Allocate a new block of elements.
 *
 * Unconditionally allocates a block of resources as described by
 * 'qhead' and link the block into 'qhead'. The new elements
 * are added to the 'qhead' free list. The new elements are
 * uninitialized except for the mpid_element field(s).
 *
 * \param[in] qhead	Queue Head
 * \return nothing
 *
 * \ref rsrc_design
 */
void MPIDU_alloc_resource(struct mpid_qhead *qhead) {
        struct mpid_resource *nq;
        struct mpid_element *ne, *nl;
        struct mpid_resource *lq;
        int x, z;

        z = qhead->hdl + (qhead->num * qhead->len);
        MPIDU_MALLOC(nq, struct mpid_resource, z, x, "MPIDU_alloc_resource");
        MPID_assert(nq != NULL);
        nq->next_used = NULL;
        nq->last_used = NULL;
        nq->next_block = NULL;
        /* point to last element in block */
        ne = (struct mpid_element *)((char *)nq + z - qhead->len);
        nl = NULL;
        /* stitch together elements, starting at end */
        for (x = 0; x < qhead->num; ++x) {
                ne->next = nl;
                nl = ne;
                ne = (struct mpid_element *)((char *)ne - qhead->len);
        }
        nq->next_free = nl;
        /* Determine how to append new block to block list */
        if (qhead->blocks == NULL) {
                qhead->blocks = nq;
        } else {
                /* locate end of block list */
		lq = qhead->lastblock;
                lq->next_block = nq;
                lq = qhead->blocks; /* reset/rewind */
                /* determine how to append new elements to free list */
                if (lq->next_free == NULL) {
                        lq->next_free = nq->next_free;
                } else {
                        /* locate end of free list */
                        for (ne = lq->next_free; ne->next;
                                                ne = ne->next);
                        ne->next = nq->next_free;
                }
        }
	qhead->lastblock = nq;
}

/**
 * \brief Unconditionally free all resource blocks
 * referenced by 'qhead'.
 *
 * NOTE: elements such as datatype cache require addition freeing
 * and so won't work with this. We could add a "free func ptr" to
 * qhead and call it here - so each element type can free any other
 * buffers it may have allocated.
 *
 * Right now, this is only called by Win_free() on the lock and unlock
 * wait queues, which do no additional allocation.
 *
 * \param[in] qhead	Queue Head
 * \return nothing
 *
 * \ref rsrc_design
 */
void MPIDU_free_resource(struct mpid_qhead *qhead) {
        struct mpid_resource *qp, *np;

        for (qp = qhead->blocks; qp != NULL; qp = np) {
                np = qp->next_block;
                MPIDU_FREE(qp, e, "MPIDU_free_resource");
        }
        qhead->blocks = NULL;
}

/**
 * \brief Get a new (unused) resource element.
 *
 * Take a resource element off the free list and put it on the
 * end of used list (bottom of queue). Element is uninitialized
 * except for mpid_element structure fields.
 *
 * \param[in] qhead	Queue Head
 * \return pointer to element.
 *
 * \ref rsrc_design
 */
void *MPIDU_get_element(struct mpid_qhead *qhead) {
        struct mpid_resource *lq = qhead->blocks;
        struct mpid_element *wp;

        if (lq == NULL || lq->next_free == NULL) {
                MPIDU_alloc_resource(qhead);
                lq = qhead->blocks;
                MPID_assert_debug(lq != NULL && lq->next_free != NULL);
        }
        wp = lq->next_free;
        lq->next_free = wp->next;
        if (lq->last_used != NULL) {
                lq->last_used->next = wp;
        }
        wp->next = NULL;
        lq->last_used = wp;
        if (lq->next_used == NULL) {
                lq->next_used = wp;
        }
        return wp;
}

/**
 * \brief Initialize a new (unused) element.
 *
 * Get a new element from free list and initialize its contents
 * from 'el'. Element is placed at bottom of queue.
 * 'el' is assumed to be of qhead->len size.
 *
 * \param[in] qhead	Queue Head
 * \param[in] el	Pointer to new element data
 * \return pointer to element.
 *
 * \ref rsrc_design
 */
void *MPIDU_add_element(struct mpid_qhead *qhead, void *el) {
        struct mpid_element *wp;

        wp = MPIDU_get_element(qhead);
        MPID_assert_debug(wp != NULL);
        memcpy(	(char *)wp + sizeof(struct mpid_element),
                (char *)el + sizeof(struct mpid_element),
                qhead->len - sizeof(struct mpid_element));
        return wp;
}

/**
 * \brief Peek at top element in queue (used list).
 *
 * Copy contents of first element in used list (top of queue)
 * into 'el'. Does not alter qhead (used or free lists).
 * 'el' is assumed to be of qhead->len size.
 *
 * \param[in] qhead	Queue Head
 * \param[out] el	Pointer to destination for element data
 * \return 1 if no elements on queue or
 *	0 on success with 'el' filled-in.
 *
 * \ref rsrc_design
 */
int MPIDU_peek_element(struct mpid_qhead *qhead, void *el) {
        struct mpid_resource *lq = qhead->blocks;

        if (lq == NULL || lq->next_used == NULL) {
                return 1;
        }
        if (el != NULL) {
                memcpy(el, lq->next_used, qhead->len);
        }
        return 0;
}

/**
 * \brief Free an element.
 *
 * Remove element 'el' (parent element 'pe') from used list
 * and place on free list. Typically, this is only called
 * after calling MPIDU_find_element() to obtain 'el' and 'pe'.
 *
 * (See MPIDU_free_resource()) This does not take into account
 * any additional allocations done by the element type. Whether
 * any such buffers need to be freed depends on how the element-
 * type re-uses elements (when taken off the free list).
 *
 * \param[in] qhead	Queue Head
 * \param[in] el	Element object
 * \param[in] pe	Parent element object, or NULL if 'el'
 *		is at top of queue.
 * \return nothing
 *
 * \ref rsrc_design
 */
void MPIDU_free_element(struct mpid_qhead *qhead,
                                        void *el, void *pe) {
        struct mpid_resource *lq = qhead->blocks;
        struct mpid_element *wp = el;
        struct mpid_element *pp = pe;

        MPID_assert_debug(lq != NULL && wp != NULL);
        /*
         * sanity check - 'pp' must be parent of 'wp'
         * or 'wp' must be at qhead->next_used.
         */
        MPID_assert_debug(pp == NULL || pp->next == wp);
        MPID_assert_debug(pp != NULL || lq->next_used == wp);
        if (lq->last_used == wp) {
                lq->last_used = pp;
        }
        if (pp) {
                pp->next = wp->next;
        } else {
                lq->next_used = wp->next;
        }
        wp->next = lq->next_free;
        lq->next_free = wp;
}

/**
 * \brief Pop first element off used list (top of queue).
 *
 * Element contents is copied into 'el', if not NULL.
 * Popped element is placed on free list.
 * Returns 0 (success) if element was popped, or 1 if list empty.
 *
 * \param[in] qhead	Queue Head
 * \param[out] el	Element contents buffer
 * \return 1 if no elements on queue or
 *	0 on success with 'el' filled-in.
 *
 * \ref rsrc_design
 */
int MPIDU_pop_element(struct mpid_qhead *qhead, void *el) {
        struct mpid_element *wp;
        struct mpid_resource *lq = qhead->blocks;

        if (lq == NULL || lq->next_used == NULL) {
                return 1;
        }
        wp = lq->next_used;
        if (el != NULL) {
                memcpy(el, wp, qhead->len);
        }
        /* we know there was no parent... */
        MPIDU_free_element(qhead, wp, NULL);
        return 0;
}

/**
 * \brief Find specific element in queue.
 *
 * Find element in used list that "matches" according to
 * 'func'('el', ...). 'func' is called with arbitrary parameter 'el'
 * and pointer to element under test. Only one element is found,
 * always the first "match". 'func' returns 0 for match (success).
 *
 * Returns NULL if no match found.
 * If 'parent' is not NULL, returns pointer to parent element there.
 * Note, '*parent' == NULL means element is first in list.
 *
 * \param[in] qhead	Queue Head
 * \param[in] func	Function to use to test for desired element
 * \param[in] v3	void arg passed to \e func in 3rd arg
 * \param[in] el	Static first parameter for 'func'
 * \param[in,out] parent	Pointer to parent element to start search from;
 *		Pointer to parent element of match found,
 *		or NULL if 'el' is at top of queue.
 * \return Pointer to element found with 'parent' set,
 *	or NULL if not found.
 *
 * \ref rsrc_design
 */
void *MPIDU_find_element(struct mpid_qhead *qhead,
                int (*func)(void *, void *, void *), void *v3, void *el,
                struct mpid_element **parent) {
        struct mpid_element *wp, *pp = NULL;

        if (qhead->blocks == NULL) {
                return NULL;
        }
        wp = (parent && *parent ? (*parent)->next : qhead->blocks->next_used);
        if (wp) {
                if (!func(el, wp, v3)) {
                        // we don't remove it here...
                        //qhead->blocks->next_used = wp->next;
                } else {
                        for (pp = wp; pp->next && func(el, pp->next, v3);
                                                pp = pp->next);
                        wp = pp->next;
                }
        }
        if (parent) {
                *parent = pp;
        }
        return wp;
}

/*
 * * * * * Remote (origin, foreign) Datatype cache * * * * *
 */

/**
 * \page dtcache_design Datatype Cache Design
 *
 * The datatype cache element stores the rank, datatype handle
 * and the localized datatype object (map and iovec). Builtin
 * datatypes are not cached (and not sent).
 *
 * This cache is used in a split fashion, where "cloned"
 * cache entries exist on the origin side to tell the origin
 * when it can skip (re-)sending the datatype.  On the target
 * side the datatype will be fully allocated for each origin.
 * Because a node may be both an origin at one time and
 * a target at another, cache entries must be separated since
 * the handles in the two cases might match but do not indicate
 * the same datatype. Entries that are origin side dataypes have
 * the (target) rank with the high bit set.  This prevents a
 * collision between local datatypes we send to that target
 * and foreign datatypes sent to us from that target.
 *
 * Datatype transfers are done in two sends.
 *
 * - The first send
 * consists of the \e MPID_Type_map structure, as generated on
 * the origin node.
 * - The second send is the datatype's \e DLOOP_VECTOR, which
 * defines the contiguous, type-less, regions.
 *
 * The actual (original) map and iovec are created/stored in a cache entry
 * under the origin node. Since the origin node never talks to itself,
 * this cache entry will never conflict with any remote datatype caching.
 *
 * Before any sends are done on the origin node, an attempt is made
 * to create a new cache entry for this datatype/target rank pair.
 * If this succeeds, then the datatype has not been sent to the
 * target before and so will be sent now.  Otherwise the entire
 * transfer of the datatype will be skipped.
 *
 * When the target node receives the first send, the callback
 * attempts to create a datatype cache entry for the datatype/origin
 * pair. Then a handle-object is created and a receive is setup
 * into the handle-object map buffer.
 *
 * When the target node receives the second send, the callback
 * allocates a buffer for the iovec.  It then sets up to
 * receive into the dataloop buffer.
 *
 * In order to facilitate/optimize cache flushing, a remote (target)
 * node always receives a datatype that is sent, even if it already
 * has a cache entry (i.e. it overwrites any existing cache data).
 * This means that the origin node must only flush its own, local, cache
 * when a datatype goes away, and if/when a new datatype uses the
 * same handle then the target side will get a new copy and replace
 * the old one.
 */

/** \brief Number of Datatype Cache elements per allocation block */
#define MPIDU_NUM_DTC_ENTRIES	7
/**
 * \brief Datatype Cache Element
 */
struct mpid_dtc_entry {
        struct mpid_dtc_entry *next;	/**< next used or next free */
        int lpid;		/**< origin lpid, or target lpid | MSB */
        MPI_Datatype dt;	/**< datatype handle on origin */
        int _pad;		/**< pad to power of two size */
        mpid_dt_info dti;	/**< extracted info from datatype */
};
/** \brief Padding for Datatype Cache Element resource block header */
#define MPIDU_PAD_DTC_ENTRIES	0

/** \brief Queue Head for Datatype Cache */
static struct mpid_qhead dtc =
        MPIDU_INIT_QHEAD_DECL(MPIDU_NUM_DTC_ENTRIES,
                sizeof(struct mpid_dtc_entry), MPIDU_PAD_DTC_ENTRIES);

/*
 * The following are used on the ranks passed to MPIDU_locate_dt()
 * (et al.), specifically in the rank embedded in the element used to
 * create, and search for, elements in the datatype cache.
 */
/** \brief OR'ed with rank in datatype cache in DT receives */
#define MPIDU_ORIGIN_FLAG	0
/** \brief OR'ed with rank in datatype cache in DT sends */
#define MPIDU_TARGET_FLAG	INT_MSB

/** \brief test whether a datatype cache rank is target (origin-side entry) */
#define MPIDU_IS_TARGET(r)	(((r) & MPIDU_TARGET_FLAG) == MPIDU_TARGET_FLAG)

/** \brief extract a datatype cache rank realm (TARGET or ORIGIN) */
#define MPIDU_DT_REALM(r)	((r) & INT_MSB)

/** \brief extract a datatype cache rank */
#define MPIDU_DT_LPID(r)	((r) & ~INT_MSB)

/**
 * \brief Callback function to match datatype cache entry
 *
 * 'v1' is a struct mpid_dtc_entry with lpid and dt filled in with
 *      desired origin lpid and foreign datatype handle.
 * 'v2' is the (currrent) struct mpid_dtc_entry being examined as
 *      a potential match.
 * 'v3' optional pointer to element pointer, which will be filled
 *	with the element that contains the already-built datatype
 *	map and iovec, if it exists.  This element is the one that
 *	has the local node's lpid.
 *
 * \param[in] v1	Desired datatype cache pseudo-element
 * \param[in] v2	Datatype cache element to compare with 'v1'
 * \param[in] v3	Pointer to Datatype cache element pointer
 *			where same datatype but different target
 *			will be saved, if v3 not NULL
 * \return boolean indicating if 'v2' does not matche 'v1'.
 *
 * \ref dtcache_design
 */
static int mpid_match_dt(void *v1, void *v2, void *v3) {
        struct mpid_dtc_entry *w1 = (struct mpid_dtc_entry *)v1;
        struct mpid_dtc_entry *w2 = (struct mpid_dtc_entry *)v2;

        if (w1->dt != w2->dt) {
                /* couldn't possibly match */
                return 1;
        }
        if (w1->lpid == w2->lpid) {
                /* exact match */
                return 0;
        }
        if (v3 && MPIDU_DT_LPID(w2->lpid) == mpid_my_lpid) {
                *((struct mpid_dtc_entry **)v3) = w2;
        }
        return 1;
}

/**
 * \brief Locate a cached foreign datatype.
 *
 * Internal use only - within datatype cache routines.
 * Locate a foreign (remote, origin) datatype cache object in
 * local cache. Returns pointer to datatype cache object.
 * Uses origin lpid and (foreign) datatype to match.
 * Flag/pointer 'new' indicates whether the object must not already exist.
 * If 'new' is not NULL and object exists, sets *new to "0"; or if does
 *	not exist then create new object and set *new to "1".
 * If 'new' is NULL and object does not exist, returns NULL.
 *
 * \param[in] lpid	Rank of origin (locker)
 * \param[in] dt	Datatype handle to search for
 * \param[in] new	Pointer to boolean for flag indicating
 *			new element was created.  If this is not NULL,
 *			then a new element will be created if none exists.
 * \param[in] src	Pointer to datatype cache element pointer
 *			used to save "closest match" element.
 * \return If 'new' is false, returns pointer to
 *	datatype cache element found, or NULL if none found.
 *	In the case of 'new' being true, returns NULL if
 *	datatype already exists, or a pointer to a newly-created
 *	cache element otherwise.
 *
 * \ref dtcache_design
 */
static struct mpid_dtc_entry *MPIDU_locate_dt(int lpid,
                                MPI_Datatype dt, int *new,
                                struct mpid_dtc_entry **src) {
        struct mpid_dtc_entry el, *ep;

        el.lpid = lpid;
        el.dt = dt;
        ep = MPIDU_find_element(&dtc, mpid_match_dt, src, &el, NULL);
        if (new) {
                if (ep == NULL) {
                        /* el was untouched by (failed) MPIDU_find_element() */
                        memset(&el.dti, 0, sizeof(el.dti));
                        ep = MPIDU_add_element(&dtc, &el);
                        *new = 1;
                } else {
                        *new = 0;
                }
        }
        return ep;
}

/**
 * \brief Callback function to match datatype cache entry for all lpids
 *
 * 'v1' is a struct mpid_dtc_entry with dt filled in with
 *      desired origin foreign datatype handle.
 * 'v2' is the (currrent) struct mpid_dtc_entry being examined as
 *      a potential match.
 *
 * \param[in] v1	Desired datatype cache pseudo-element
 * \param[in] v2	Datatype cache element to compare with 'v1'
 * \param[in] v3	Not used.
 * \return boolean indicating if 'v2' does not match 'v1', match
 *			on origin-side cache entry with same handle.
 *
 * \ref dtcache_design
 */
static int mpid_flush_dt(void *v1, void *v2, void *v3) {
        struct mpid_dtc_entry *w1 = (struct mpid_dtc_entry *)v1;
        struct mpid_dtc_entry *w2 = (struct mpid_dtc_entry *)v2;

        return (!MPIDU_IS_TARGET(w2->lpid) || w1->dt != w2->dt);
}

/**
 * \brief Function to remove all datatype cache entries for specific datatype
 *
 * Should be called whenever a datatype is freed/destroyed. Alternatively,
 * could be called whenever a datatype is detected as having changed
 * (i.e. handle gets re-used).
 *
 * \param[in] dtp	MPID_Datatype object to be flushed
 * \return	number of entries flushed
 */
static int MPIDU_flush_dt(MPID_Datatype *dtp) {
        struct mpid_dtc_entry el, *ep;
        struct mpid_element *pp = NULL;
        int n = 0;

        el.dt = dtp->handle;
        while ((ep = MPIDU_find_element(&dtc, mpid_flush_dt, NULL, &el, &pp)) != NULL) {
                if (MPIDU_DT_LPID(ep->lpid) == mpid_my_lpid) {
                        if (ep->dti.map)
                                MPIDU_FREE(ep->dti.map, mpi_errno, "MPIDU_flush_dt");
                        if (ep->dti.dtp && !ep->dti.dtp->handle)
                                MPIDU_FREE(ep->dti.dtp, mpi_errno, "MPIDU_flush_dt");
                }
                MPIDU_free_element(&dtc, ep, pp);
                ++n;
        }
        return n;
}

void MPIDU_dtc_free(MPID_Datatype *dtp) {
        (void)MPIDU_flush_dt(dtp);
}

#ifdef NOT_USED
/**
 * \brief Get Datatype info for a foreign datatype
 *
 * Lookup a foreign (remote, origin) datatype in local cache.
 * Uses origin lpid and (foreign) datatype.
 *
 * \param[in] lpid	Rank of origin
 * \param[in] fdt	Foreign (origin) datatype handle to search for
 * \param[out] dti	Pointer to datatype info struct
 * \return 0 if locally cached datatype found,
 *	or 1 if not found.
 *
 * \ref dtcache_design
 */
static int MPIDU_lookup_dt(int lpid, MPI_Datatype fdt, mpid_dt_info *dti) {
        struct mpid_dtc_entry *dtc;

        if (lpid == mpid_my_lpid) { /* origin == target, was cached as "target" */
                lpid |= MPIDU_TARGET_FLAG;
        } else {
                lpid |= MPIDU_ORIGIN_FLAG;
        }
        dtc = MPIDU_locate_dt(lpid, fdt, NULL, NULL);
        if (dtc != NULL && dtc->dti.map != NULL) {
                if (dti) {
                        *dti = dtc->dti;
                }
                return 0;
        } else {
                return 1;
        }
}
#endif /* NOT_USED */

/**
 * \brief Prepare to receive a foreign datatype (step 1 - map).
 *
 * Called when MPID_MSGTYPE_DT_MAP (first datatype packet) received.
 * Returns NULL if this datatype is already in the cache.
 * Since the origin should be mirroring our cache status,
 * we would expect to never see this case here.
 * Must be the first of sequence:
 *	- MPID_MSGTYPE_DT_MAP
 *	- MPID_MSGTYPE_DT_IOV
 *	- MPID_MSGTYPE_ACC (_PUT, _GET)
 * Although, the cache operation is not dependant on any subsequent
 * RMA operations - i.e. the caching may be done for its own sake.
 *
 * Allocates storage for the map and updates cache element.
 *
 *	mpid_info_w0 = MPID_MSGTYPE_MAP
 *	mpid_info_w1 = map size, bytes
 *	mpid_info_w2 = origin lpid
 *	mpid_info_w3 = foreign datatype handle
 *	mpid_info_w4 = datatype extent
 *	mpid_info_w5 = datatype element type
 *	mpid_info_w6 = datatype element size
 *	mpid_info_w7 = (not used)
 *
 * \param[in] mi	MPIDU_Onesided_info_t containing data
 * \return pointer to buffer to receive foreign datatype map
 *	structure, or NULL if datatype is already cached.
 *
 * \ref dtcache_design
 */
char *MPID_Prepare_rem_dt(MPIDU_Onesided_info_t *mi) {
        struct mpid_dtc_entry *dtc;
        int new = 0;

        dtc = MPIDU_locate_dt(mi->mpid_info_w2 | MPIDU_ORIGIN_FLAG,
                        (MPI_Datatype)mi->mpid_info_w3, &new, NULL);
        if (!new) {
                /* if origin is re-sending, they must know what they're doing. */
                if (dtc->dti.map) MPIDU_FREE(dtc->dti.map, mpi_errno, "MPID_Prepare_rem_dt");
        }
        if (!dtc->dti.dtp) {
                dtc->dti.dtp = MPIDU_MALLOC(dtc->dti.dtp, MPID_Datatype, sizeof(MPID_Datatype), mpi_errno, "MPID_Prepare_rem_dt");
                MPID_assert(dtc->dti.dtp != NULL);
        }
        /* caution! not a real datatype object! */
        dtc->dti.dtp->handle = 0;
        dtc->dti.dtp->extent = mi->mpid_info_w4;
        dtc->dti.dtp->eltype = mi->mpid_info_w5;
        dtc->dti.dtp->element_size = mi->mpid_info_w6;
        dtc->dti.map_len = 0;
        MPIDU_MALLOC(dtc->dti.map, MPID_Type_map, mi->mpid_info_w1 * sizeof(*dtc->dti.map), mpi_errno, "MPID_Prepare_rem_dt");
        MPID_assert(dtc->dti.map != NULL);
#ifdef NOT_USED
        dtc->dti.iov = NULL;
#endif /* NOT_USED */
        return (char *)dtc->dti.map;
}

#ifdef NOT_USED
/**
 * \brief Prepare to update foreign datatype (step 2 - iov).
 *
 * Called when MPID_MSGTYPE_DT_IOV (second datatype packet) received.
 * Returns NULL if this datatype is already in the cache.
 * Must be the second of sequence:
 *	- MPID_MSGTYPE_DT_MAP
 *	- MPID_MSGTYPE_DT_IOV
 *	- MPID_MSGTYPE_ACC (_PUT, _GET)
 *
 * Allocates storage for the iov and updates cache element.
 *
 * \param[in] lpid	Rank of origin
 * \param[in] fdt	Foreign (origin) datatype handle to search for
 * \param[in] dlz	iov size (number of elements)
 * \return pointer to buffer to receive foreign datatype iov
 *	structure, or NULL if datatype is already cached.
 *
 * \ref dtcache_design
 */
static char *mpid_update_rem_dt(int lpid, MPI_Datatype fdt, int dlz) {
        struct mpid_dtc_entry *dtc;

        dtc = MPIDU_locate_dt(lpid | MPIDU_ORIGIN_FLAG, fdt, NULL, NULL);
        MPID_assert_debug(dtc != NULL);
        MPID_assert_debug(dtc->dti.iov == NULL); /* must follow MPID_MSGTYPE_DT_MAP */
        dtc->dti.iov_len = 0;
        MPIDU_MALLOC(dtc->dti.iov, DLOOP_VECTOR, dlz * sizeof(*dtc->dti.iov), lpid, "mpid_update_rem_dt");
        MPID_assert(dtc->dti.iov != NULL);
        return (char *)dtc->dti.iov;
}
#endif /* NOT_USED */

/**
 *  \brief completion for datatype cache messages (map and iov)
 *
 * To use this callback, the msginfo (DCQuad) must
 * be filled as follows:
 *
 * - \e w0 - extent size
 * - \e w1 - number of elements in map or iov
 * - \e w2 - origin rank
 * - \e w3 - datatype handle on origin
 *
 * \param[in] xtra	Pointer to xtra msginfo saved from original message
 * \return	nothing
 *
 */
void MPID_Recvdone1_rem_dt(MPIDU_Onesided_xtra_t *xtra) {
        struct mpid_dtc_entry *dtc;

        dtc = MPIDU_locate_dt(xtra->mpid_xtra_w2 | MPIDU_ORIGIN_FLAG, xtra->mpid_xtra_w3, NULL, NULL);
        MPID_assert_debug(dtc != NULL);
        dtc->dti.map_len = xtra->mpid_xtra_w1;
}

#ifdef NOT_USED
/**
 *  \brief completion for datatype cache messages (map and iov)
 *
 * To use this callback, the msginfo (DCQuad) must
 * be filled as follows:
 *
 * - \e w0 - MPID_MSGTYPE_DT_IOV
 * - \e w1 - number of elements in map or iov
 * - \e w2 - origin rank
 * - \e w3 - datatype handle on origin
 *
 * \param[in] xtra	Pointer to xtra msginfo saved from original message
 * \return	nothing
 *
 */
static void mpid_recvdone2_rem_dt(MPIDU_Onesided_xtra_t *xtra) {
        struct mpid_dtc_entry *dtc;

        dtc = MPIDU_locate_dt(xtra->mpid_xtra_w2 | MPIDU_ORIGIN_FLAG, xtra->mpid_xtra_w3, NULL, NULL);
        MPID_assert_debug(dtc != NULL);
        dtc->dti.iov_len = xtra->mpid_xtra_w1;
}
#endif /* NOT_USED */

/**
 * \brief Checks whether a local datatype has already been cached
 * at the target node.
 *
 * Determine whether a local datatype has already been sent to
 * this target (and thus is cached over there).
 * Returns bool TRUE if datatype is (should be) in lpid's cache.
 *
 * Should only be called on the origin.
 *
 * \param[in] lpid	lpid of target
 * \param[in] dt	Local datatype handle to search for
 * \param[out] dti	Pointer to datatype info struct
 * \return Boolean TRUE if the datatype has already been cached.
 *
 * \ref dtcache_design
 */
int MPIDU_check_dt(int lpid, MPI_Datatype dt, mpid_dt_info *dti) {
        struct mpid_dtc_entry *dtc, *cln = NULL;
        int new = 0;

        dtc = MPIDU_locate_dt(lpid | MPIDU_TARGET_FLAG, dt, &new, &cln);
        if (new) {
                if (cln) {
                        /*
                         * same local datatype, different target,
                         * copy what we already have cached.
                         */
                        dtc->dti = cln->dti;
                } else {
                        /*
                         * first time using this datatype - create map/iov.
                         */
                        make_dt_map_vec(dt, &dtc->dti);
#ifdef NOT_USED
                        MPID_assert_debug(dtc->dti.iov != NULL);
#endif /* NOT_USED */
                        MPID_assert_debug(dtc->dti.map != NULL);
                }
        }
        if (dti) {
                *dti = dtc->dti;
        }
/* we never send datatype */
return 1;
        return (!new);
}

/*
 * * * * * Request object (DCMF_Request_t) cache * * * * *
 *
 * because the request object is larger than a cache line,
 * no attempt is made to keep objects cache-aligned, for example
 * by padding the header to be the same size as the element or
 * padding the element to a cache-line size.
 *
 * The "piggy-back" data is declared as DCQuad for no special
 * reason - it was simply a convenient type that contained
 * adequate space.  This component is not used directly as
 * msginfo in any message layer calls.
 *
 */

/**
 * \page rqcache_design Request Object Cache Design
 *
 * The request cache element consists of a \e DCMF_Request_t
 * and a single \e DCQuad that may be used to save context
 * between the routine that allocated the request object and the
 * callback that frees it.
 *
 * When a request is allocated, the only value returned is
 * a pointer to the \e DCMF_Request_t field of the cache element.
 * When a request is freed, the cache must be searched for
 * a matching element, which is then moved to the free list.
 * Before the element is moved to the free list, the \e DCQuad
 * must be copied into a caller-supplied buffer or it will be lost.
 *
 * Callbacks that involve a request cache element will call
 * \e MPIDU_free_req with a \e DCQuad buffer to receive the context
 * info, if used. Then the context info is examined and action
 * taken accordingly.  Common use for the contaxt info is to
 * free a buffer involved in a send operation and/or decrement
 * a counter to indicate completion.
 */

/** \brief Number of Request Cache elements per allocation block */
#define MPIDU_NUM_RQC_ENTRIES	8
/**
 * \brief DCMF Request Cache Element
 *
 * \ref rqcache_design
 */
struct mpid_rqc_entry {
        struct mpid_rqc_entry *next;	/**< next used or next free */
        int _pad[3];	/**< must 16-byte align DCMF_Request_t */
        MPIDU_Onesided_xtra_t xtra;	/**< generic piggy-back.
                         * Not directly used in communications. */
	MPIDU_Onesided_info_t info;	/**< MPID1S info */
        DCMF_Request_t req;	/**< DCMF Request object */
};
/** \brief Padding for Request Cache Element resource block header */
#define MPIDU_PAD_RQC_ENTRIES	0

/** \brief Queue Head for DCMF Request Object Cache */
static struct mpid_qhead rqc =
        MPIDU_INIT_QHEAD_DECL(MPIDU_NUM_RQC_ENTRIES,
                sizeof(struct mpid_rqc_entry), MPIDU_PAD_RQC_ENTRIES);

/**
 * \brief Test if a request object is represented by the given element.
 *
 * \param[in] v1	Pointer to DCMF request object in question
 * \param[in] v2	Pointer to request cache element to test
 * \param[in] v3	not used
 * \return	1 if NOT a matching request
 *
 * \ref rqcache_design
 */
static int mpid_match_rq(void *v1, void *v2, void *v3) {
        DCMF_Request_t *w1 = (DCMF_Request_t *)v1;
        struct mpid_rqc_entry *w2 = (struct mpid_rqc_entry *)v2;

        return (w1 != &w2->req);
}

/**
 * \brief Get a new request object from the resource queue.
 *
 * If 'xtra' is not NULL, copy data into request cache element,
 * otherwise zero the field.
 * Returns pointer to the request component of the cache element.
 *
 * \param[in] xtra	Optional pointer to additional info to save
 * \param[out] info	Optional pointer to private msg info to use
 * \return Pointer to DCMF request object
 *
 * \ref rqcache_design
 */
DCMF_Request_t *MPIDU_get_req(MPIDU_Onesided_xtra_t *xtra,
			MPIDU_Onesided_info_t **info) {
        struct mpid_rqc_entry *rqe;

        rqe = MPIDU_get_element(&rqc);
        MPID_assert_debug(rqe != NULL);
	// This assert is not relavent for non-BG and not needed for BG.
        // MPID_assert_debug((((unsigned)&rqe->req) & 0x0f) == 0);
        if (xtra) {
                rqe->xtra = *xtra;
        } else {
                memset(&rqe->xtra, 0, sizeof(rqe->xtra));
        }
	if (info) {
		*info = &rqe->info;
	}
        return &rqe->req;
}

/**
 * \brief Release a DCMF request object and retrieve info
 *
 * Locate the request object in the request cache and free it.
 * If 'xtra' is not NULL, copy piggy-back data into 'bgp'.
 * Assumes request object was returned by a call to MPIDU_get_req().
 *
 * \param[in] req	Pointer to DCMF request object being released
 * \param[out] xtra	Optional pointer to receive saved additional info
 * \return nothing
 *
 * \ref rqcache_design
 */
void MPIDU_free_req(DCMF_Request_t *req, MPIDU_Onesided_xtra_t *xtra) {
        struct mpid_rqc_entry *rqe;
        struct mpid_element *pp = NULL;

        rqe = (struct mpid_rqc_entry *)MPIDU_find_element(&rqc,
                                        mpid_match_rq, NULL, req, &pp);
        MPID_assert_debug(rqe != NULL);
        if (xtra) {
                *xtra = rqe->xtra;
        }
        MPIDU_free_element(&rqc, rqe, pp);
}

/*
 * * * * * * Callbacks used on request cache objects * * * * *
 */

/**
 * \brief Generic request cache done callback with counter decr
 *
 * Callback for decrementing a "done" or pending count.
 *
 * To use this callback, the "xtra" info (DCQuad) must
 * be filled as follows:
 *
 * - \e w0 - (int *) pending counter
 * - \e w1 - ignored
 * - \e w2 - ignored
 * - \e w3 - ignored
 *
 * \param[in] v	Pointer to DCMF request object
 * \return nothing
 *
 * \ref rqcache_design
 */
void done_rqc_cb(void *v, DCMF_Error_t *e) {
        volatile unsigned *pending;
        MPIDU_Onesided_xtra_t xtra;

        MPIDU_free_req((DCMF_Request_t *)v, &xtra);
        pending = (volatile unsigned *)xtra.mpid_xtra_w0;
        if (pending) {
                --(*pending);
        }
}

#ifdef NOT_USED
/**
 * \brief Generic request cache done callback with counter decr
 * and 2-buffer freeing.
 *
 * Callback for decrementing a "done" or pending count and
 * freeing malloc() memory, up to two pointers.
 *
 * To use this callback, the "xtra" info (DCQuad) must
 * be filled as follows:
 *
 * - \e w0 - (int *) pending counter
 * - \e w1 - ignored
 * - \e w2 - (void *) allocated memory if not NULL
 * - \e w3 - (void *) allocated memory if not NULL
 *
 * \param[in] v	Pointer to DCMF request object
 * \return nothing
 *
 * \ref rqcache_design
 */
static void done_free_rqc_cb(void *v, DCMF_Error_t *e) {
        volatile unsigned *pending;
        MPIDU_Onesided_xtra_t xtra;

        MPIDU_free_req((DCMF_Request_t *)v, &xtra);
        pending = (volatile unsigned *)xtra.mpid_xtra_w0;
        if (pending) {
                --(*pending);
        }
        if (xtra.mpid_xtra_w2) { MPIDU_FREE(xtra.mpid_xtra_w2, e, "xtra.mpid_xtra_w2"); }
        if (xtra.mpid_xtra_w3) { MPIDU_FREE(xtra.mpid_xtra_w3, e, "xtra.mpid_xtra_w3"); }
}
#endif /* NOT_USED */

/**
 * \brief request cache done callback for Get, with counter decr,
 * ref count, buffer freeing and dt release when ref count reaches zero.
 * Also uses dt to unpack results into application buffer.
 *
 * Callback for decrementing a "done" or pending count and
 * freeing malloc() memory, up to two pointers, when ref count goes 0.
 *
 * To use this callback, the "xtra" info (DCQuad) must
 * be filled as follows:
 *
 * - \e w0 - (int *) pending counter
 * - \e w1 - (int *) get struct
 * - \e w2 - (void *) allocated memory if not NULL
 * - \e w3 - (void *) allocated memory if not NULL
 *
 * \param[in] v	Pointer to DCMF request object
 * \return nothing
 *
 * \ref rqcache_design
 */
void done_getfree_rqc_cb(void *v, DCMF_Error_t *e) {
        volatile unsigned *pending;
	volatile struct mpid_get_cb_data *get;
        MPIDU_Onesided_xtra_t xtra;

        MPIDU_free_req((DCMF_Request_t *)v, &xtra);
        pending = (volatile unsigned *)xtra.mpid_xtra_w0;
        get = (volatile struct mpid_get_cb_data *)xtra.mpid_xtra_w1;
        if (pending) {
                --(*pending);
        }
	MPID_assert_debug(get != NULL);
        if (--get->ref == 0) {
		if (get->dtp) {
			MPID_Segment segment;
			DLOOP_Offset last;

			int mpi_errno = MPID_Segment_init(get->addr,
					get->count,
					get->dtp->handle, &segment, 0);
			MPID_assert_debug(mpi_errno == MPI_SUCCESS);
			last = get->len;
			MPID_Segment_unpack(&segment, 0, &last, get->buf);
			MPID_assert_debug(last == get->len);
			MPID_Datatype_release(get->dtp);
		}
		DCMF_Memregion_destroy((DCMF_Memregion_t *)&get->memreg);
                if (xtra.mpid_xtra_w2) { MPIDU_FREE(xtra.mpid_xtra_w2, e, "xtra.mpid_xtra_w2"); }
                if (xtra.mpid_xtra_w3) { MPIDU_FREE(xtra.mpid_xtra_w3, e, "xtra.mpid_xtra_w3"); }
        }
}


/**
 * \brief Generic request cache done callback with counter decr,
 * ref count, and 2-buffer freeing when ref count reaches zero.
 *
 * Callback for decrementing a "done" or pending count and
 * freeing malloc() memory, up to two pointers, when ref count goes 0.
 *
 * To use this callback, the "xtra" info (DCQuad) must
 * be filled as follows:
 *
 * - \e w0 - (int *) pending counter
 * - \e w1 - (int *) reference counter
 * - \e w2 - (void *) allocated memory if not NULL
 * - \e w3 - (void *) datatype to release if not NULL
 *
 * \param[in] v	Pointer to DCMF request object
 * \return nothing
 *
 * \ref rqcache_design
 */
void done_reffree_rqc_cb(void *v, DCMF_Error_t *e) {
        volatile unsigned *pending;
	volatile struct mpid_put_cb_data *put;
        MPIDU_Onesided_xtra_t xtra;

        MPIDU_free_req((DCMF_Request_t *)v, &xtra);
        pending = (volatile unsigned *)xtra.mpid_xtra_w0;
        put = (volatile struct mpid_put_cb_data *)xtra.mpid_xtra_w1;
        if (pending) {
                --(*pending);
        }
	MPID_assert_debug(put != NULL);
        if (--put->ref == 0) {
		if (put->flag) DCMF_Memregion_destroy((DCMF_Memregion_t *)&put->memreg);
                if (xtra.mpid_xtra_w2) { MPIDU_FREE(xtra.mpid_xtra_w2, e, "xtra.mpid_xtra_w2"); }
                if (xtra.mpid_xtra_w3) { MPIDU_FREE(xtra.mpid_xtra_w3, e, "xtra.mpid_xtra_w3"); }
        }
}

#ifdef NOT_USED
/**
 * \brief Callback for freeing malloc() memory, up to two pointers.
 *
 * To use this callback, the "xtra" info (DCQuad) must
 * be filled as follows:
 *
 * - \e w0 - (void *) allocated memory if not NULL
 * - \e w1 - (void *) allocated memory if not NULL
 * - \e w2 - ignored
 * - \e w3 - ignored
 *
 * \param[in] v	Pointer to DCMF request object
 * \return nothing
 *
 * \ref rqcache_design
 */
static void free_rqc_cb(void *v, DCMF_Error_t *e) {
        MPIDU_Onesided_xtra_t xtra;

        MPIDU_free_req((DCMF_Request_t *)v, &xtra);
        if (xtra.mpid_xtra_w0) { MPIDU_FREE(xtra.mpid_xtra_w0, e, "xtra.mpid_xtra_w0"); }
        if (xtra.mpid_xtra_w1) { MPIDU_FREE(xtra.mpid_xtra_w1, e, "xtra.mpid_xtra_w1"); }
}
#endif /* NOT_USED */

/**
 * \brief Generic request cache callback for RMA op completion
 *
 * Callback for incrementing window RMA recvs count.
 * Used only by Put and Accumulate (not used by Get).
 *
 * Only used for a "long message" - i.e. multi-packet - PUT.
 *
 * To use this callback, the "xtra" info (DCQuad) must
 * be filled as follows:
 *
 * - \e w0 - ignored
 * - \e w1 - window handle
 * - \e w2 - origin rank
 * - \e w3 - origin lpid
 *
 * \param[in] v	Pointer to DCMF request object
 * \return nothing
 *
 * \ref rqcache_design
 */
void rma_rqc_cb(void *v, DCMF_Error_t *e) {
        MPID_Win *win;
        MPIDU_Onesided_xtra_t xtra;

        MPIDU_free_req((DCMF_Request_t *)v, &xtra);
        MPID_Win_get_ptr((MPI_Win)xtra.mpid_xtra_w1, win);
        MPID_assert_debug(win != NULL);
        rma_recvs_cb(win, xtra.mpid_xtra_w2, xtra.mpid_xtra_w3, 1);
}

/**
 * \brief Generic callback for request cache
 *
 * Callback for simply (only) freeing the request cache object.
 *
 * To use this callback, the "xtra" info (DCQuad) must
 * be filled as follows:
 *
 * - \e w0 - ignored
 * - \e w1 - ignored
 * - \e w2 - ignored
 * - \e w3 - ignored
 *
 * \param[in] v	Pointer to DCMF request object
 * \return nothing
 *
 * \ref rqcache_design
 */
void none_rqc_cb(void *v, DCMF_Error_t *e) {
        MPIDU_free_req((DCMF_Request_t *)v, NULL);
}

#ifdef NOT_USED
/**
 * \brief Generic send done callback
 *
 * Local send callback.
 *
 * Simple "done" callback, currently used only by lock/unlock.
 * Assumes param is an int * and decrements it.
 *
 * \param[in] v	Pointer to integer counter to decrement
 * \return nothing
 */
static void done_cb(void *v, DCMF_Error_t *e) {
        int *cp = (int *)v;
        --(*cp);
}
#endif /* NOT_USED */

/**
 *  \brief receive callback for datatype cache messages (map and iov)
 *
 * \param[in] v	Pointer to request object used for transfer
 * \return	nothing
 */
void dtc1_rqc_cb(void *v, DCMF_Error_t *e) {
        MPIDU_Onesided_xtra_t xtra;

        MPIDU_free_req((DCMF_Request_t *)v, &xtra);
        MPID_Recvdone1_rem_dt(&xtra);
}

#ifdef NOT_USED
/**
 *  \brief receive callback for datatype cache messages (map and iov)
 *
 * \param[in] v	Pointer to request object used for transfer
 * \return	nothing
 */
static void dtc2_rqc_cb(void *v, DCMF_Error_t *e) {
        MPIDU_Onesided_xtra_t xtra;

        MPIDU_free_req((DCMF_Request_t *)v, &xtra);
        mpid_recvdone2_rem_dt(&xtra);
}
#endif /* NOT_USED */

/*
 * * * * * * * * * * * * * * * * * * * * * *
 */

/**
 * \brief Send (spray) a protocol message to a group of nodes.
 *
 * Send a protocol message to all members of a group (or the
 * window-comm if no group).
 *
 * Currently, this routine will only be called once per group
 * (i.e. once during an exposure or access epoch). If it ends
 * up being called more than once, it might make sense to build
 * a translation table between the group rank and the window
 * communicator rank.  Or if we can determine that the same
 * group is being used in multiple, successive, epochs. In practice,
 * it takes more work to build a translation table than to lookup
 * ranks ad-hoc.
 *
 * \param[in] win	Pointer to MPID_Win object
 * \param[in] grp	Optional pointer to MPID_Group object
 * \param[in] type	Type of message (MPID_MSGTYPE_*)
 * \return MPI_SUCCESS or error returned from DCMF_Send.
 *
 * \ref msginfo_usage
 */
int MPIDU_proto_send(MPID_Win *win, MPID_Group *grp, int type) {
        int lpid, x;
        MPIDU_Onesided_ctl_t ctl;
        int size, comm_size = 0, comm_rank;
        int mpi_errno = MPI_SUCCESS;
        MPID_VCR *vc;
        DCMF_Consistency consistency = win->_dev.my_cstcy;

        /*
         * \todo Confirm this:
         * For inter-comms, we only talk to the remote nodes. For
         * intra-comms there are no remote or local nodes.
         * So, we always use win->_dev.comm_ptr->vcr (?)
         * However, we have to choose remote_size, in the case of
         * inter-comms, vs. local_size. This decision also
         * affects MPIDU_world_rank_c().
         */
        size = MPIDU_comm_size(win);
        vc = MPIDU_world_vcr(win);
        MPID_assert_debug(vc != NULL && size > 0);
        if (grp) {
                comm_size = size;
                size = grp->size;
        }
        /** \todo is it OK to lower consistency here? */
        consistency = DCMF_RELAXED_CONSISTENCY;

        ctl.mpid_ctl_w0 = type;
        ctl.mpid_ctl_w2 = win->_dev.comm_ptr->rank;
        for (x = 0; x < size; ++x) {
                if (grp) {
                        int z;

                        lpid = grp->lrank_to_lpid[x].lpid;
                        /* convert group rank to comm rank */
                        for (z = 0; z < comm_size &&
                                lpid != vc[z]; ++z);
                        MPID_assert_debug(z < comm_size);
                        comm_rank = z;
                } else {
                        lpid = vc[x];
                        comm_rank = x;
                }
                ctl.mpid_ctl_w1 = win->_dev.coll_info[comm_rank].win_handle;
                if (type == MPID_MSGTYPE_COMPLETE) {
                        ctl.mpid_ctl_w3 = win->_dev.coll_info[comm_rank].rma_sends;
                        win->_dev.coll_info[comm_rank].rma_sends = 0;
                }
		if (lpid == mpid_my_lpid) {
			if (type == MPID_MSGTYPE_POST) {
				++win->_dev.my_sync_begin;
			} else if (type == MPID_MSGTYPE_COMPLETE) {
				++win->_dev.my_sync_done;
			}
		} else {
                	mpi_errno = DCMF_Control(&bg1s_ct_proto, consistency, lpid, &ctl.ctl);
                	if (mpi_errno) { break; }
		}
        }
        return mpi_errno;
}

/*
 * * * * * * * * * * * * * * * * * * * * * *
 */

/**
 * \brief validate whether a lpid is in a given group
 *
 * Searches the group lpid list for a match.
 *
 * \param[in] lpid	World rank of the node in question
 * \param[in] grp	Group to validate against
 * \return TRUE is lpid is in group
 */
int MPIDU_valid_group_rank(int lpid, MPID_Group *grp) {
        int size = grp->size;
        int z;

        for (z = 0; z < size &&
                lpid != grp->lrank_to_lpid[z].lpid; ++z);
        return (z < size);
}

/*
 * Remote (receiver) Callbacks.
 */

/**
 * \brief Receive callback for RMA protocol and operations messages
 *
 * "Small" message callback - the entire message is already here.
 * Process it now and return.
 *
 * \param[in] _mi	Pointer to msginfo
 * \param[in] ct	Number of DCQuad's in msginfo
 * \param[in] or	Rank of origin
 * \param[in] sb	Pointer to send buffer (data received)
 * \param[in] sl	Length (bytes) of data
 * \return nothing
 *
 * \ref msginfo_usage
 */
void recv_sm_cb(void *cd, const DCQuad *_mi, unsigned ct, size_t or,
                        const char *sb, const size_t sl) {
        MPID_Win *win;
        char *rb;
        MPIDU_Onesided_ctl_t *mc = (MPIDU_Onesided_ctl_t *)_mi;
        MPIDU_Onesided_info_t *mi = (MPIDU_Onesided_info_t *)_mi;

	// assert(mc->mpid_ctl_w0 == mi->mpid_info_w0);
        switch (mc->mpid_ctl_w0) {
        /* The following all use msginfo as DCMF_Control_t (DCQuad[1]) */
        case MPID_MSGTYPE_COMPLETE:
                MPID_assert_debug(ct == MPIDU_1SCTL_NQUADS);
                MPID_assert_debug(sl == 0);
                MPID_Win_get_ptr((MPI_Win)mc->mpid_ctl_w1, win);
                MPID_assert_debug(win != NULL);
                win->_dev.coll_info[win->_dev.comm_ptr->rank].rma_sends += mc->mpid_ctl_w3;
                ++win->_dev.my_sync_done;
                break;
        case MPID_MSGTYPE_POST:
                MPID_assert_debug(ct == MPIDU_1SCTL_NQUADS);
                MPID_assert_debug(sl == 0);
                MPID_Win_get_ptr((MPI_Win)mc->mpid_ctl_w1, win);
                MPID_assert_debug(win != NULL);
                ++win->_dev.my_sync_begin;
                break;
        case MPID_MSGTYPE_LOCK:
                MPID_assert_debug(ct == MPIDU_1SCTL_NQUADS);
                lock_cb(mc, or);
                break;
        case MPID_MSGTYPE_UNLOCK:
                MPID_assert_debug(ct == MPIDU_1SCTL_NQUADS);
                unlk_cb(mc, or);
                break;
        case MPID_MSGTYPE_LOCKACK:
        case MPID_MSGTYPE_UNLOCKACK:
                MPID_assert_debug(ct == MPIDU_1SCTL_NQUADS);
                MPID_Win_get_ptr((MPI_Win)mc->mpid_ctl_w1, win);
                MPID_assert_debug(win != NULL);
                ++win->_dev.my_sync_done;
                break;

        /* The following all use msginfo as DCQuad[2] */
        case MPID_MSGTYPE_PUT:
                MPID_assert_debug(ct == MPIDU_1SINFO_NQUADS);
                MPID_Win_get_ptr((MPI_Win)mi->mpid_info_w1, win);
                MPID_assert_debug(win != NULL);
                MPIDU_assert_PUTOK(win);
                if (win->_dev.epoch_assert & MPI_MODE_NOPUT) {
                        /** \todo exact error handling */
                }
#ifdef USE_DCMF_PUT
		/* In this case, the message is a completion notification */
                rma_recvs_cb(win, mi->mpid_info_w2, or, mi->mpid_info_w3);
#else /* ! USE_DCMF_PUT */
                MPID_assert_debug(sl != 0);
                memcpy((char *)mi->mpid_info_w3, sb, sl);
                rma_recvs_cb(win, mi->mpid_info_w2, or, 1);
#endif /* ! USE_DCMF_PUT */
                break;
        case MPID_MSGTYPE_DT_MAP:
                MPID_assert_debug(ct == MPIDU_1SINFO_NQUADS);
                rb = MPID_Prepare_rem_dt(mi);
                if (rb) {
                        MPIDU_Onesided_xtra_t xtra;

                        xtra.mpid_xtra_w0 = mi->mpid_info_w0;
                        xtra.mpid_xtra_w1 = mi->mpid_info_w1;
                        xtra.mpid_xtra_w2 = mi->mpid_info_w2;
                        xtra.mpid_xtra_w3 = mi->mpid_info_w3;
                        memcpy(rb, sb, sl);
                        MPID_Recvdone1_rem_dt(&xtra);
                }
                break;
        case MPID_MSGTYPE_DT_IOV:
#ifdef NOT_USED
                MPID_assert_debug(ct == MPIDU_1SINFO_NQUADS);
                rb = mpid_update_rem_dt(mi->mpid_info_w2, mi->mpid_info_w3, mi->mpid_info_w1);
                if (rb) {
                        memcpy(rb, sb, sl);
                        mpid_recvdone2_rem_dt(_mi);
                }
                break;
#endif /* NOT_USED */
                MPID_abort();
        case MPID_MSGTYPE_ACC:
                MPID_assert_debug(ct == MPIDU_1SINFO_NQUADS);
                MPID_Win_get_ptr((MPI_Win)mi->mpid_info_w1, win);
                MPID_assert_debug(win != NULL);
                MPIDU_assert_PUTOK(win);
                if (win->_dev.epoch_assert & MPI_MODE_NOPUT) {
                        /** \todo exact error handling */
                }
                target_accumulate(mi, sb, or);
                break;

        /* Not supported message types */
        case MPID_MSGTYPE_GET: /* GET can't generate these */
                MPID_abort();
        default:
                /*
                 * Don't know what to do with this... we have some data
                 * (possibly) but don't have a target address to copy it
                 * to (or know what else to do with it).
                 */
                break;
        }
}

/**
 * \brief Callback for DCMF_Control() messages
 *
 * Simple pass-through to recv_sm_cb() with zero-length data.
 *
 * \param[in] ctl	Control message (one quad)
 * \param[in] or	Origin node lpid
 * \return	nothing
 */
void recv_ctl_cb(void *cd, const DCMF_Control_t *ctl, size_t or) {
        recv_sm_cb(cd, (const DCQuad *)ctl, MPIDU_1SCTL_NQUADS, or, NULL, 0);
}

/**
 * \brief Receive callback for RMA operations messages
 *
 * "Message receive initiated" callback.
 * This one should never get called for protocol messages.
 * Setup buffers, get a request object, and return so receive can begin.
 * In some cases (e.g. MPID_MSGTYPE_ACC) the processing is done in the
 * receive completion callback, otherwise that callback just frees
 * the request and cleans up (updates counters).
 *
 * \param[in] _mi	Pointer to msginfo
 * \param[in] ct	Number of DCQuad's in msginfo
 * \param[in] or	Rank of origin
 * \param[in] sl	Length (bytes) of sent data
 * \param[out] rl	Length (bytes) of data to receive
 * \param[out] rb	receive buffer
 * \param[out] cb	callback to invoke after receive
 * \return Pointer to DCMF request object to use for receive,
 *	or NULL to discard received data
 *
 * \ref msginfo_usage
 */
DCMF_Request_t *recv_cb(void *cd, const DCQuad *_mi, unsigned ct,
                        size_t or, const size_t sl, size_t *rl,
                        char **rb, DCMF_Callback_t *cb)
{
        DCMF_Request_t *req;
        MPID_Win *win;
        MPIDU_Onesided_info_t *mi = (MPIDU_Onesided_info_t *)_mi;

        switch (mi->mpid_info_w0) {
        /* The following all use msginfo as DCQuad[2] */
        case MPID_MSGTYPE_PUT:
#ifdef USE_DCMF_PUT
		MPID_abort();
#else /* ! USE_DCMF_PUT */
                MPID_assert_debug(ct == MPIDU_1SINFO_NQUADS);
                MPID_Win_get_ptr((MPI_Win)mi->mpid_info_w1, win);
                MPID_assert_debug(win != NULL);
                MPIDU_assert_PUTOK(win);
                if (win->_dev.epoch_assert & MPI_MODE_NOPUT) {
                        /** \todo exact error handling */
                }
                MPID_assert_debug(mi->mpid_info_w3 >=
                                        (size_t)win->base &&
                        mi->mpid_info_w3 + sl <=
                                        (size_t)win->base + win->size);
                *rl = sl;
                *rb = (char *)mi->mpid_info_w3;
                {	MPIDU_Onesided_xtra_t xtra;
                        xtra.mpid_xtra_w0 = mi->mpid_info_w3;
                        xtra.mpid_xtra_w1 = mi->mpid_info_w1;
                        xtra.mpid_xtra_w2 = mi->mpid_info_w2;
                        xtra.mpid_xtra_w3 = or;
                        req = MPIDU_get_req(&xtra, NULL);
                }
                cb->clientdata = req;
                cb->function = rma_rqc_cb;
                return req;
#endif /* ! USE_DCMF_PUT */
        case MPID_MSGTYPE_DT_MAP:
                MPID_assert_debug(ct == MPIDU_1SINFO_NQUADS);
                *rb = MPID_Prepare_rem_dt(mi);
                if (!*rb) {
                        return NULL;
                }
                *rl = sl;
                {	MPIDU_Onesided_xtra_t xtra;
                        xtra.mpid_xtra_w0 = mi->mpid_info_w0;
                        xtra.mpid_xtra_w1 = mi->mpid_info_w1;
                        xtra.mpid_xtra_w2 = mi->mpid_info_w2;
                        xtra.mpid_xtra_w3 = mi->mpid_info_w3;
                        req = MPIDU_get_req(&xtra, NULL);
                }
                cb->clientdata = req;
                cb->function = dtc1_rqc_cb;
                return req;
        case MPID_MSGTYPE_DT_IOV:
#ifdef NOT_USED
                MPID_assert_debug(ct == MPIDU_1SINFO_NQUADS);
                *rb = mpid_update_rem_dt(mi->mpid_info_w2, mi->mpid_info_w3, mi->mpid_info_w1);
                if (!*rb) {
                        return NULL;
                }
                *rl = sl;
                {	MPIDU_Onesided_xtra_t xtra;
                        xtra.mpid_xtra_w0 = mi->mpid_info_w0;
                        xtra.mpid_xtra_w1 = mi->mpid_info_w1;
                        xtra.mpid_xtra_w2 = mi->mpid_info_w2;
                        xtra.mpid_xtra_w3 = mi->mpid_info_w3;
                        req = MPIDU_get_req(&xtra, NULL);
                }
                cb->clientdata = req;
                cb->function = dtc2_rqc_cb;
                return req;
#endif /* NOT_USED */
                MPID_abort();
        case MPID_MSGTYPE_ACC:
                MPID_assert_debug(ct == MPIDU_1SINFO_NQUADS);
        { /* block */
                MPIDU_Onesided_info_t *info;
                MPIDU_Onesided_xtra_t xtra = {0};

                MPID_Win_get_ptr((MPI_Win)mi->mpid_info_w1, win);
                MPID_assert_debug(win != NULL);
                MPIDU_assert_PUTOK(win);
                if (win->_dev.epoch_assert & MPI_MODE_NOPUT) {
                        /** \todo exact error handling */
                }
                /** \note These embedded DCQuads are not directly
                 * used in any communications. */
                MPIDU_MALLOC(info, MPIDU_Onesided_info_t,
                        sizeof(MPIDU_Onesided_info_t) + sl, e, "MPID_MSGTYPE_ACC");
                MPID_assert_debug(info != NULL);
                *rb = (char *)(info + 1);
                *rl = sl;
                memcpy(info, mi, sizeof(MPIDU_Onesided_info_t));
                xtra.mpid_xtra_w2 = (size_t)info;
                xtra.mpid_xtra_w3 = or;
                req = MPIDU_get_req(&xtra, NULL);
                cb->clientdata = req;
                cb->function = accum_cb;
                return req;
        } /* block */

        /* The following all use msginfo as DCMF_Control_t (DCQuad[1]) */
        case MPID_MSGTYPE_POST:
        case MPID_MSGTYPE_COMPLETE:
                /* Win_post/Win_complete messages are always small. */
        case MPID_MSGTYPE_LOCK:
        case MPID_MSGTYPE_UNLOCK:
        case MPID_MSGTYPE_LOCKACK:
        case MPID_MSGTYPE_UNLOCKACK:
                MPID_abort();
        case MPID_MSGTYPE_GET: /* GET can't generate these */
                MPID_abort();
        default:
                break;
        }
        return NULL;
}

/*
 * End of remote callbacks.
 */

/**
 * \brief Reset all counters and indicators related to active RMA epochs
 *
 * Assumes all synchronization and wait-for-completion have been done.
 * Sets epoch type to "NONE".
 *
 * \param[in] win	Window whose epoch is finished
 */
void epoch_clear(MPID_Win *win) {
	int x;
	int size = MPIDU_comm_size(win);
	win->_dev.epoch_type = MPID_EPOTYPE_NONE;
	win->_dev.epoch_rma_ok = 0;
	win->_dev.my_rma_recvs = 0;
	win->_dev.my_sync_done = 0;
	win->_dev.my_sync_begin = 0;
	for (x = 0; x < size; ++x) {
		win->_dev.coll_info[x].rma_sends = 0;
	}
}

#ifdef NOT_USED
/**
 * \brief Send local datatype to target node
 *
 * Routine to send target datatype to target node.
 * These sends are handled by recv callbacks above...
 *
 * \param[in] dt	datatype handle to send
 * \param[in] o_lpid	Origin lpid
 * \param[in] t_lpid	Target lpid
 * \param[out] pending	Pointer to send done counter
 * \param[in,out] consistency	Pointer for consistency used for sends (out)
 * \return MPI_SUCCESS, or error returned by DCMF_Send.
 *
 * \ref msginfo_usage\n
 * \ref dtcache_design
 */
int mpid_queue_datatype(MPI_Datatype dt,
                        int o_lpid, int t_lpid, volatile unsigned *pending,
                        DCMF_Consistency *consistency) {
        MPIDU_Onesided_info_t *info;
        MPIDU_Onesided_xtra_t xtra = {0};
        DCMF_Callback_t cb_send;
        DCMF_Request_t *reqp;
        int mpi_errno = MPI_SUCCESS;
        mpid_dt_info dti;

        if (MPIDU_check_dt(t_lpid, dt, &dti)) {
                /* we've previously sent this datatype to that target */
                return mpi_errno;
        }
        /** \todo need to ensure we don't LOWER consistency... */
        *consistency = DCMF_WEAK_CONSISTENCY;
        xtra.mpid_xtra_w0 = (size_t)pending;

        reqp = MPIDU_get_req(&xtra, &info);
        info->mpid_info_w0 = MPID_MSGTYPE_DT_MAP;
        info->mpid_info_w1 = dti.map_len;
        info->mpid_info_w2 = o_lpid;
        info->mpid_info_w3 = dt;
        info->mpid_info_w4 = dti.dtp->extent;
        info->mpid_info_w5 = dti.dtp->eltype;
        info->mpid_info_w6 = dti.dtp->element_size;
        ++(*pending);
        cb_send.function = done_rqc_cb;
        cb_send.clientdata = reqp;
        mpi_errno = DCMF_Send(&bg1s_sn_proto, reqp, cb_send,
                        *consistency, t_lpid,
                        dti.map_len * sizeof(*dti.map), (char *)dti.map,
                        info->info, MPIDU_1SINFO_NQUADS);
        if (mpi_errno) { return(mpi_errno); }
        reqp = MPIDU_get_req(&xtra, &info);
        info->mpid_info_w0 = MPID_MSGTYPE_DT_IOV;
        info->mpid_info_w1 = dti.iov_len;
        info->mpid_info_w2 = o_lpid;
        info->mpid_info_w3 = dt;
        info->mpid_info_w4 = dti.dtp->extent;
        info->mpid_info_w5 = dti.dtp->eltype;
        info->mpid_info_w6 = dti.dtp->element_size;
        ++(*pending);
        cb_send.function = done_rqc_cb;
        cb_send.clientdata = reqp;
        mpi_errno = DCMF_Send(&bg1s_sn_proto, reqp, cb_send,
                        *consistency, t_lpid,
                        dti.iov_len * sizeof(*dti.iov), (char *)dti.iov,
                        info->info, MPIDU_1SINFO_NQUADS);
        return mpi_errno;
}
#endif /* NOT_USED */

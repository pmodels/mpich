/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file src/onesided/mpid_onesided.h
 * \brief MPI-DCMF Common declarations and definitions for RMA
 */
#define USE_DCMF_PUT

#include "mpidimpl.h"

/* redefine these for debugging purposes */
/**
 * \brief Macro for allocating memory
 *
 * \param[out] p variable to receive new pointer
 * \param[in] t type of "p", excluding "*"
 * \param[in] z size in bytes to allocate
 * \param[in] e variable to receive error code
 * \param[in] n string to identify where/what allocation is
 * \return nothing, per ce. fills in "p" with address or NULL
 */
#define MPIDU_MALLOC(p, t, z, e, n)	p = (t *)MPIU_Malloc(z)

/**
 * \brief Macro for allocating memory
 *
 * \param[in] p variable containing address to be freed
 * \param[in] e variable to receive error code
 * \param[in] n string to identify where/what allocation/free is
 */
#define MPIDU_FREE(p, e, n)		MPIU_Free((void *)p)

/**
 * \brief structure of DCMF_Control_t as used by RMA ops
 *
 * DCMF_Control_t is assumed to be (the equivalent of)
 * four size_t "words" on all platforms.
 */
#define MPIDU_1SCTL_NQUADS	(sizeof(DCMF_Control_t) / sizeof(DCQuad))
typedef union {
        DCMF_Control_t ctl;	/**< access to underlying type */
        struct {
                size_t _0w0;	/**< word 0 */
                size_t _0w1;	/**< word 1 */
                size_t _0w2;	/**< word 2 */
                size_t _0w3;	/**< word 3 */
        } _c_u;	/**< overlay of DCMF_Control_t */
} MPIDU_Onesided_ctl_t __attribute__ ((__aligned__ (16)));
/* assert(sizeof(MPIDU_Onesided_ctl_t._c_u) <= sizeof(MPIDU_Onesided_ctl_t.ctl)); */

#define mpid_ctl_w0	_c_u._0w0	/**< ctl word 0 */
#define mpid_ctl_w1	_c_u._0w1	/**< ctl word 1 */
#define mpid_ctl_w2	_c_u._0w2	/**< ctl word 2 */
#define mpid_ctl_w3	_c_u._0w3	/**< ctl word 3 */

/**
 * \brief structure of "msginfo" as used by RMA ops
 *
 * We always use 2 quads, just for consistency.
 */
#define MPIDU_1SINFO_NQUADS	((sizeof(struct _i_u_s) + sizeof(DCQuad) - 1) / sizeof(DCQuad))
typedef union {
        struct _i_u_s {
                size_t _0w0;	/**< word 0 */
                size_t _0w1;	/**< word 1 */
                size_t _0w2;	/**< word 2 */
                size_t _0w3;	/**< word 3 */
                size_t _1w0;	/**< word 4 */
                size_t _1w1;	/**< word 5 */
                size_t _1w2;	/**< word 6 */
                size_t _1w3;	/**< word 7 */
        } _i_u;	/**< overlay of DCQuads */
        DCQuad info[MPIDU_1SINFO_NQUADS]; /**< access to underlying type */
} MPIDU_Onesided_info_t __attribute__ ((__aligned__ (16)));

#define mpid_info_w0	_i_u._0w0	/**< info word 0 */
#define mpid_info_w1	_i_u._0w1	/**< info word 1 */
#define mpid_info_w2	_i_u._0w2	/**< info word 2 */
#define mpid_info_w3	_i_u._0w3	/**< info word 3 */
#define mpid_info_w4	_i_u._1w0	/**< info word 4 */
#define mpid_info_w5	_i_u._1w1	/**< info word 5 */
#define mpid_info_w6	_i_u._1w2	/**< info word 6 */
#define mpid_info_w7	_i_u._1w3	/**< info word 7 */

/**
 * \brief structure of "extra data" as used in request pool
 */
typedef struct {
        size_t mpid_xtra_w0;	/**< word 0 */
        size_t mpid_xtra_w1;	/**< word 1 */
        size_t mpid_xtra_w2;	/**< word 2 */
        size_t mpid_xtra_w3;	/**< word 3 */
} MPIDU_Onesided_xtra_t;

#if 0
/**
 * \brief Translate message type into string
 */
extern char *msgtypes[];
#endif

/** \brief DCMF Protocol object for DCMF_Send() calls */
extern DCMF_Protocol_t bg1s_sn_proto;
/** \brief DCMF Protocol object for DCMF_Put() calls */
extern DCMF_Protocol_t bg1s_pt_proto;
/** \brief DCMF Protocol object for DCMF_Get() calls */
extern DCMF_Protocol_t bg1s_gt_proto;
/** \brief DCMF Protocol object for DCMF_Control() calls */
extern DCMF_Protocol_t bg1s_ct_proto;

/** \brief global for our lpid */
extern unsigned mpid_my_lpid;

/**
 * \brief struct used by MPID_Get to delay unpacking of datatype
 *
 * Active only when the origin datatype is non-contiguous.
 * Reference count is used to determine when the last chunk of
 * data is received, rest of struct is used to unpack data.
 */
struct mpid_get_cb_data {
	int ref;		/**< reference counter */
        MPID_Datatype *dtp;	/**< datatype object */
	void *addr;		/**< origin address of get */
	int count;		/**< origin count */
	int len;		/**< computed length of get */
	char *buf;		/**< temp buffer of packed data */
	int _pad[2];		/**< preserve alignment */
	DCMF_Memregion_t memreg;/**< memory region for temp buffer */
};

/**
 * \brief struct used by MPID_Put to delay freeing of resources
 *
 * Active only when the origin datatype is non-contiguous.
 * Reference count is used to determine when the last chunk of
 * data is completed, rest of struct is used to free.
 */
struct mpid_put_cb_data {
	int ref;		/**< reference counter */
	int flag;		/**< flags - curr only memregion */
	int _pad[2];		/**< preserve alignment */
	DCMF_Memregion_t memreg;/**< memory region for temp buffer */
};

/** \brief datatype map entry - adjacent fields with same datatype are combined
 *
 * map[x].len = map[x].num * MPID_Datatype_get_basic_size(map[x].dt);
 * map[x+1].off = map[x].off + map[x].len + <stride>;
 *
 */
typedef struct MPID_Type_map {
        unsigned off;		/**< offset in datatype */
        unsigned len;		/**< length of map segment */
        unsigned num;		/**< number of fields */
        MPI_Datatype dt;	/**< datatype of fields */
} MPID_Type_map;

/*******************************************************/

/**
 * \brief Structure to house info on cached datatypes
 *
 * Used to store info in the actual cache, and also to pass
 * info back and forth.
 *
 * Only derived datatypes ever use this. eltype and elsize are
 * zero if the dtp->eltype is not builtin. MPID_Accumulate()
 * uses this for error checking.
 */
typedef struct {
        MPID_Datatype *dtp;	/**< datatype object */
        int _pad;		/**< padding to power of 2 length */
        MPID_Type_map *map;	/**< datatype map */
        int map_len;		/**< datatype map length */
} mpid_dt_info;

/**
 * \brief Build datatype map and iovec
 *
 * \param[in] dt	Datatype to build map/iov for
 * \param[out] dti	Pointer to datatype info struct
 */
void make_dt_map_vec(MPI_Datatype dt, mpid_dt_info *dti);


/** \brief Hi-order bit of integer type \note should be taken from standard include */
#define INT_MSB	(1UL << (sizeof(int) * 8 - 1))

/**
 * \brief Datatype created to represent the rma_sends element
 * of the coll_info array of the window structure
 */
extern MPI_Datatype Coll_info_rma_dt;
/**
 * \brief User defined function created to process the rma_sends
 * elements of the coll_info array of the window structure
 */
extern MPI_Op Coll_info_rma_op;

/**
 * \brief Progress (advance) wait - how to spin and make progress
 *
 * 'expr' is true if must wait, i.e.
 *	while(expr) { make_progress; }
 *
 * Guaranteed to call DCMF_Messager_advance() _at least_ once
 * (via MPID_Progress_wait())
 *
 * In DCMF, MPID_Progress_wait() never returns an error...
 * Also, MPID_Progress_state is never used.
 *
 * \param[in] expr	Conditional expression to be evaluated on each loop,
 *			FALSE will terminate loop.
 * \return nothing
 */
#define MPIDU_Progress_spin(expr) {             \
        do {                                    \
                (void)MPID_Progress_test();     \
                MPID_CS_EXIT();                 \
                MPID_CS_ENTER();                \
        } while (expr);                         \
}

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
 * \brief Basic resource element structure.
 *
 * This implements the basic linked-list pointer(s)
 * needed to manage generic elements.
 * An actual element implementation will have additional
 * fields.
 *
 * \ref rsrc_design
 */
struct mpid_element {
        struct mpid_element *next;	/**< next used or next free */
};

/**
 * \brief Resource block header structure.
 *
 * This structure exists at the beginning of each
 * allocated block of elements.
 *
 * \ref rsrc_design
 */
struct mpid_resource {
        struct mpid_element *next_used;	/**< Pointer to top of queue */
        struct mpid_element *last_used;	/**< Pointer to bottom of queue */
        struct mpid_element *next_free;	/**< Pointer to top of free list */
        struct mpid_resource *next_block;	/**< Pointer to next allocated block */
};

/**
 * \brief Resource "queue head" structure.
 *
 * This structure is required to exist and be initialized
 * before any resource or element management functions are call.
 *
 * \ref rsrc_design
 */
struct mpid_qhead {
        int num;	/**< number of elements per block */
        short len;	/**< length of each element */
        short hdl;	/**< length of header in block */
        struct mpid_resource *blocks; /**< Allocated block(s) chain */
        struct mpid_resource *lastblock; /**< last block in chain */
};

/**
 * \brief Queue-head static initializer.
 *
 * This is used to initialize a mpid_qhead structure at
 * compile-time.
 *
 * \param[in] n	number of elements per block
 * \param[in] l	length (bytes) of each element
 * \param[in] p	padding for header: added to sizeof(mpid_resource)
 * \return static initializer
 *
 * \ref rsrc_design
 */
#define MPIDU_INIT_QHEAD_DECL(n, l, p) \
        { (n), (l), sizeof(struct mpid_resource) + p, NULL }

/**
 * \brief Queue-head dynamic initializer.
 *
 * This is used to initialize a mpid_qhead structure at
 * runtime.
 *
 * \param[in] qp	pointer to mpid_qhead structure
 * \param[in] n	number of elements per block
 * \param[in] l	length (bytes) of each element
 * \param[in] p	padding for header: added to sizeof(mpid_resource)
 * \return n/a
 *
 * \ref rsrc_design
 */
#define MPIDU_INIT_QHEAD(qp, n, l, p) {	\
        (qp)->num = n;			\
        (qp)->len = l;			\
        (qp)->hdl = sizeof(struct mpid_resource) + p;	\
        (qp)->blocks = NULL;		\
}

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
void MPIDU_alloc_resource(struct mpid_qhead *qhead);

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
void MPIDU_free_resource(struct mpid_qhead *qhead);

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
void *MPIDU_get_element(struct mpid_qhead *qhead);

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
void *MPIDU_add_element(struct mpid_qhead *qhead, void *el);

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
int MPIDU_peek_element(struct mpid_qhead *qhead, void *el);

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
void MPIDU_free_element(struct mpid_qhead *qhead, void *el, void *pe);

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
int MPIDU_pop_element(struct mpid_qhead *qhead, void *el);

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
                struct mpid_element **parent);

/*
 * * * * * Win Locks and Lock wait queue * * * * *
 */

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
void MPIDU_Spin_lock_free(MPID_Win *win);

/**
 * \brief Test whether window lock is free
 *
 * \param[in] win       Pointer to MPID_Win object
 * \return	Boolean TRUE if lock is free
 */
int MPIDU_is_lock_free(MPID_Win *win);

/*
 * * * * * Unlock wait queue * * * * *
 */

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

/**
 * \brief Remove a datatype cache entry
 *
 * \param[in] dtp	Pointer to MPID_Datatype object to un-cache
 * \return	nothing
 */
void MPIDU_dtc_free(MPID_Datatype *dtp);

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
int MPIDU_lookup_dt(int lpid, MPI_Datatype fdt, mpid_dt_info *dti);
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
char *MPID_Prepare_rem_dt(MPIDU_Onesided_info_t *mi);

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
char *mpid_update_rem_dt(int lpid, MPI_Datatype fdt, int dlz);
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
void MPID_Recvdone1_rem_dt(MPIDU_Onesided_xtra_t *xtra);

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
void mpid_recvdone2_rem_dt(MPIDU_Onesided_xtra_t *xtra);
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
int MPIDU_check_dt(int lpid, MPI_Datatype dt, mpid_dt_info *dti);

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

/**
 * \brief Get a new request object from the resource queue.
 *
 * If 'xtra' is not NULL, copy data into request cache element,
 * otherwise zero the field.
 * Returns pointer to the request component of the cache element.
 *
 * \param[in] xtra	Optional pointer to additional info to save
 * \param[out] info	Optional pointer to private info to use
 * \return Pointer to DCMF request object
 *
 * \ref rqcache_design
 */
DCMF_Request_t *MPIDU_get_req(MPIDU_Onesided_xtra_t *xtra,
			MPIDU_Onesided_info_t **info);

/**
 * \brief Release a DCMF request object and retrieve info
 *
 * Locate the request object in the request cache and free it.
 * If 'xtra' is not NULL, copy piggy-back data into 'xtra'.
 * Assumes request object was returned by a call to MPIDU_get_req().
 *
 * \param[in] req	Pointer to DCMF request object being released
 * \param[out] xtra	Optional pointer to receive saved additional info
 * \return nothing
 *
 * \ref rqcache_design
 */
void MPIDU_free_req(DCMF_Request_t *req, MPIDU_Onesided_xtra_t *xtra);

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
void done_rqc_cb(void *v, DCMF_Error_t *);

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
void done_free_rqc_cb(void *v, DCMF_Error_t *);
#endif /* NOT_USED */

/**
 * \brief request cache done callback for Get, with counter decr,
 * ref count, buffer freeing and dt release when ref count reaches zero.
 * Also uses dt to unpack results into application buffer.
 *
 * Callback for decrementing a "done" or pending count and
 * freeing malloc() memory, up to two pointers, when ref count goes 0.
 *
 * To use this callback, the "xtra" info must
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
void done_getfree_rqc_cb(void *v, DCMF_Error_t *);

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
 * - \e w3 - (void *) allocated memory if not NULL
 *
 * \param[in] v	Pointer to DCMF request object
 * \return nothing
 *
 * \ref rqcache_design
 */
void done_reffree_rqc_cb(void *v, DCMF_Error_t *);

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
void free_rqc_cb(void *v, DCMF_Error_t *);
#endif /* NOT_USED */

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
void rma_recvs_cb(MPID_Win *win, int orig, int lpid, int count);

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
void rma_rqc_cb(void *v, DCMF_Error_t *);

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
void none_rqc_cb(void *v, DCMF_Error_t *);

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
void done_cb(void *v, DCMF_Error_t *);
#endif /* NOT_USED */

/**
 *  \brief receive callback for datatype cache messages (map and iov)
 *
 * \param[in] v	Pointer to request object used for transfer
 * \return	nothing
 */
void dtc1_rqc_cb(void *v, DCMF_Error_t *);

#ifdef NOT_USED
/**
 *  \brief receive callback for datatype cache messages (map and iov)
 *
 * \param[in] v	Pointer to request object used for transfer
 * \return	nothing
 */
void dtc2_rqc_cb(void *v, DCMF_Error_t *);
#endif /* NOT_USED */

/*
 * * * * * * * * * * * * * * * * * * * * * *
 */

/**
 * \brief Return communicator's VCR table to use for translations
 *
 * \param[in] c	Pointer to communicator object
 * \return VCR table to use for rank translations
 *
 * \todo Confirm this: we only ever talk to remote nodes,
 * so always use (c)->vcr (?).
 */
static inline MPID_VCR *MPIDU_world_vcr_c(MPID_Comm *c) {
        return (c)->vcr;
}

/**
 * \brief Convert comm rank to world rank
 *
 * \param[in] c	Pointer to communicator object
 * \param[in] r	Rank of node in window
 * \return World rank (lpid) of node
 *
 * \todo Confirm this: we only ever talk to remote nodes,
 * so always use (c)->vcr (?).
 */
static inline int MPIDU_world_rank_c(MPID_Comm *c, int r) {
        MPID_VCR *vc;
        vc = MPIDU_world_vcr_c(c);
        return vc[r];
}

/**
 * \brief Return the active size of a communicator
 *
 * For inter-comms, we will only talk to the remote side so
 * return that size.  Otherwise, use the local size.
 *
 * \param[in] c	Pointer to communicator object
 * \return Size of communicator that we will talk to
 */
static inline int MPIDU_comm_size_c(MPID_Comm *c) {
        return ((c)->comm_kind == MPID_INTERCOMM ?
                (c)->remote_size :
                (c)->local_size);
}

/**
 * \brief Convert window rank to world rank
 *
 * Used in MPID_Win_{lock|put|get|accumulate} to get the
 * COMM_WORLD rank of a window-comm rank in order to get
 * the destination parameter of a DCMF_ operation.
 *
 * Assumes MPID_Win_* might be called with an intercomm.
 *
 * Note, MPIDU_proto_send() does not call this since it
 * ends up getting the COMM_WORLD rank as a result of the
 * group-to-window-comm rank translation.
 *
 * \param[in] w	Pointer to MPID_Win object
 * \param[in] r	Rank of node in window
 * \return World rank (lpid) of node
 */
#define MPIDU_world_rank(w, r)	MPIDU_world_rank_c((w)->_dev.comm_ptr, r)

/**
 * \brief Return the VCR of a window's communicator
 *
 * \param[in] w	Pointer to MPID_Win object
 * \return VCR table to use for rank translations
 */
#define MPIDU_world_vcr(w)	MPIDU_world_vcr_c((w)->_dev.comm_ptr)

/**
 * \brief Return the active size of a window's communicator
 *
 * \param[in] w	Pointer to MPID_Win object
 * \return Size of communicator
 */
#define MPIDU_comm_size(w)	MPIDU_comm_size_c((w)->_dev.comm_ptr)

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
int MPIDU_proto_send(MPID_Win *win, MPID_Group *grp, int type);

/**
 * \brief Utility routine to provide accumulate function on target.
 *
 * Utility routine to provide accumulate function on target.
 *
 * Called from "long message" ACCUMULATE completion callback
 * or "short message" ACCUMULATE receive callback.
 *
 * \param[in] win	Pointer to MPID_Win object
 * \param[in] dst	Pointer to destination buffer
 * \param[in] src	Pointer to source buffer
 * \param[in] rank	Rank of origin
 * \param[in] fdt	Foreign datatype
 * \param[in] op	Operand
 * \param[in] num	number of Foreign datatype elements
 * \return nothing
 */
void target_accumulate(MPIDU_Onesided_info_t *mi,
                                const char *src, int lpid);

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
int MPIDU_valid_group_rank(int lpid, MPID_Group *grp);

/**
 * \brief Test whether a window is in an RMA access epoch
 *
 * Assert that the local window is in the proper mode to receive
 * RMA operations.
 *
 * \note One cause of this to fail is improper use of MPI_MODE_NOPUT.
 *
 * \param[in] w	Pointer to window object
 * \return	TRUE if RMA ops are allowed
 */
#define MPIDU_assert_RMAOK(w)	MPID_assert((w)->_dev.epoch_rma_ok)
/**
 * \brief Test whether a window is in an RMA access epoch
 *
 * Assert that the local window is in the proper mode to receive
 * RMA operations.
 *
 * \note One cause of this to fail is improper use of MPI_MODE_NOPUT.
 * Another is erroneous use of MPI_MODE_NOCHECK.
 *
 * \param[in] w	Pointer to window object
 * \return	TRUE if PUT/ACCUMULATE ops are allowed
 */
#define MPIDU_assert_PUTOK(w)	MPID_assert((w)->_dev.epoch_rma_ok && \
                                !((w)->_dev.epoch_assert & MPI_MODE_NOPUT))

/**
 * \brief validate that an RMA target is legitimate for the epoch type
 *
 * For MPID_EPOTYPE_LOCK requires target to be the same as that
 * used in the MPID_Win_lock call.
 *
 * For MPID_EPOTYPE_FENCE allows any target,
 * assuming that the target was validated against comm_ptr
 * by the MPI layer.
 *
 * For MPID_EPOTYPE_*START valids the rank against group_ptr.
 *
 * \todo Is this check too expensive to be done on every RMA?
 *
 * \param[in] w	Window
 * \param[in] r	Rank
 * \return TRUE if rank is valid for current epoch
 */
#define MPIDU_VALID_RMA_TARGET(w, r)	\
        (((w)->_dev.epoch_type == MPID_EPOTYPE_LOCK && \
                (w)->_dev.epoch_size == (r)) || \
        (w)->_dev.epoch_type == MPID_EPOTYPE_FENCE || \
        (((w)->_dev.epoch_type == MPID_EPOTYPE_START || \
          (w)->_dev.epoch_type == MPID_EPOTYPE_POSTSTART) && \
                MPIDU_valid_group_rank(MPIDU_world_rank(w, r), \
                                        (w)->start_group_ptr)))

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
                        const char *sb, const size_t sl);

/**
 * \brief Callback for DCMF_Control() messages
 *
 * Simple pass-through to recv_sm_cb() with zero-length data.
 *
 * \param[in] ctl	Control message (one quad)
 * \param[in] or	Origin node lpid
 * \return	nothing
 */
void recv_ctl_cb(void *cd, const DCMF_Control_t *ctl, size_t or);

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
 * \param[in] v Pointer to DCMF request object
 * \return nothing
 *
 * \ref msginfo_usage
 */
void accum_cb(void *v, DCMF_Error_t *);

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
                        char **rb, DCMF_Callback_t *cb);

void mpidu_init_lock(MPID_Win *win);
void mpidu_free_lock(MPID_Win *win);

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
void lock_cb(const MPIDU_Onesided_ctl_t *info, int lpid);

/**
 * \brief Reset all counters and indicators related to active RMA epochs
 *
 * Assumes all synchronization and wait-for-completion have been done.
 * Sets epoch type to "NONE". 
 *
 * \param[in] win       Window whose epoch is finished
 */
void epoch_clear(MPID_Win *win);

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
void epoch_end_cb(MPID_Win *win);

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
void unlk_cb(const MPIDU_Onesided_ctl_t *info, int lpid);

/*
 * End of remote callbacks.
 */

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
                        DCMF_Consistency *consistency);
#endif /* NOT_USED */

/*
 * End of utility routines
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 */

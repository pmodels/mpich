/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef XPMEM_SEG_H_INCLUDED
#define XPMEM_SEG_H_INCLUDED

#include "xpmem_pre.h"
#include "xpmem_impl.h"

/****************************************/
/* Segment cache internal routines      */
/****************************************/

enum {
    AVL_RIGHT,
    AVL_LEFT
};

/* AVL tree is balanced tree, so 64 stack size should be enough to go
 * through all the nodes in the tree in practice. In case it exceeds,
 * we throw an assertion error. This problem can be fixed by using
 * heap memory, but it adds additional overhead. */
#define MPIDI_XPMEM_AVL_STACK_SIZE 64
#define MPIDI_XPMEM_AVL_DECLARE_STACK(stack, type, size) \
        const size_t stack##_size = size; \
        type stack[stack##_size]; \
        int stack##_sp = 0
#define MPIDI_XPMEM_AVL_STACK_PUSH(stack, value) do {           \
        MPIR_Assert(stack##_sp < MPIDI_XPMEM_AVL_STACK_SIZE);   \
        stack[stack##_sp++] = value;                            \
} while (0)
#define MPIDI_XPMEM_AVL_STACK_POP(stack, value) do {   \
        MPIR_Assert(stack##_sp > 0);                   \
        value = stack[--stack##_sp];                   \
} while (0)
#define MPIDI_XPMEM_AVL_STACK_EMPTY(stack) (!stack##_sp)

/* Creates a new segment and attaches into local virtual address space. */
MPL_STATIC_INLINE_PREFIX int MPIDI_XPMEM_seg_do_create(MPIDI_XPMEM_seg_t ** seg_ptr, uint64_t low,
                                                       uint64_t high, xpmem_apid_t apid)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_XPMEM_seg_t *seg = NULL;
    struct xpmem_addr xpmem_addr;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_XPMEM_SEG_DO_CREATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_XPMEM_SEG_DO_CREATE);

    *seg_ptr = MPIR_Handle_obj_alloc(&MPIDI_XPMEM_seg_mem);
    MPIR_ERR_CHKANDJUMP1(!(*seg_ptr), mpi_errno, MPI_ERR_OTHER, "**nomem",
                         "**nomem %s", "MPIDI_XPMEM_seg_t");
    seg = *seg_ptr;
    seg->low = low;
    seg->high = high;
    MPIR_Object_set_ref(seg, 0);

    xpmem_addr.apid = apid;
    xpmem_addr.offset = seg->low;
    seg->vaddr = xpmem_attach(xpmem_addr, high - low, NULL);
    /* virtual address or failure(-1) */
    MPIR_ERR_CHKANDJUMP(seg->vaddr == (void *) -1, mpi_errno, MPI_ERR_OTHER, "**xpmem_attach");

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_XPMEM_SEG_DO_CREATE);
    return mpi_errno;
  fn_fail:
    if (seg)    /* in case xpmem_attach fails */
        MPIR_Handle_obj_free(&MPIDI_XPMEM_seg_mem, seg);
    goto fn_exit;
}

/* Detaches and releases a segment. */
MPL_STATIC_INLINE_PREFIX int MPIDI_XPMEM_seg_do_release(MPIDI_XPMEM_seg_t * seg)
{
    int ret;
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_XPMEM_SEG_DO_RELEASE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_XPMEM_SEG_DO_RELEASE);

    MPIR_Assert(MPIR_Object_get_ref(seg) == 0);
    ret = xpmem_detach((void *) seg->vaddr);
    MPIR_ERR_CHKANDJUMP(ret == -1, mpi_errno, MPI_ERR_OTHER, "**xpmem_detach");
    MPIR_Handle_obj_free(&MPIDI_XPMEM_seg_mem, seg);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_XPMEM_SEG_DO_RELEASE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_XPMEM_avl_do_update_node_info(MPIDI_XPMEM_seg_t * node)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_XPMEM_AVL_DO_UPDATE_NODE_INFO);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_XPMEM_AVL_DO_UPDATE_NODE_INFO);

    int lheight = node->left == NULL ? 0 : node->left->height;
    int rheight = node->right == NULL ? 0 : node->right->height;
    node->height = (lheight < rheight ? rheight : lheight) + 1;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_XPMEM_AVL_DO_UPDATE_NODE_INFO);
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_XPMEM_avl_do_right_rotation(MPIDI_XPMEM_seg_t * parent,
                                                               MPIDI_XPMEM_seg_t * left_child)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_XPMEM_AVL_DO_RIGHT_ROTATION);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_XPMEM_AVL_DO_RIGHT_ROTATION);

    parent->left = left_child->right;
    left_child->right = parent;

    left_child->parent = parent->parent;
    if (left_child->parent != NULL) {
        if (left_child->parent->left == parent)
            left_child->parent->left = left_child;
        else
            left_child->parent->right = left_child;
    }

    parent->parent = left_child;
    if (parent->left != NULL)
        parent->left->parent = parent;

    mpi_errno = MPIDI_XPMEM_avl_do_update_node_info(parent);
    MPIR_ERR_CHECK(mpi_errno);
    mpi_errno = MPIDI_XPMEM_avl_do_update_node_info(left_child);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_XPMEM_AVL_DO_RIGHT_ROTATION);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_XPMEM_avl_do_left_rotation(MPIDI_XPMEM_seg_t * parent,
                                                              MPIDI_XPMEM_seg_t * right_child)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_XPMEM_AVL_DO_LEFT_ROTATION);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_XPMEM_AVL_DO_LEFT_ROTATION);

    parent->right = right_child->left;
    right_child->left = parent;

    right_child->parent = parent->parent;
    if (right_child->parent != NULL) {
        if (right_child->parent->left == parent)
            right_child->parent->left = right_child;
        else
            right_child->parent->right = right_child;
    }

    parent->parent = right_child;
    if (parent->right != NULL)
        parent->right->parent = parent;

    mpi_errno = MPIDI_XPMEM_avl_do_update_node_info(parent);
    MPIR_ERR_CHECK(mpi_errno);
    mpi_errno = MPIDI_XPMEM_avl_do_update_node_info(right_child);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_XPMEM_AVL_DO_LEFT_ROTATION);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_XPMEM_avl_do_left_right_rotation(MPIDI_XPMEM_seg_t * parent,
                                                                    MPIDI_XPMEM_seg_t * left_child)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_XPMEM_AVL_DO_LEFT_RIGHT_ROTATION);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_XPMEM_AVL_DO_LEFT_RIGHT_ROTATION);

    MPIDI_XPMEM_seg_t *lr_child = left_child->right;
    /* Left rotate */
    mpi_errno = MPIDI_XPMEM_avl_do_left_rotation(left_child, lr_child);
    MPIR_ERR_CHECK(mpi_errno);
    /* Right rotate */
    mpi_errno = MPIDI_XPMEM_avl_do_right_rotation(parent, lr_child);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_XPMEM_AVL_DO_LEFT_RIGHT_ROTATION);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_XPMEM_avl_do_right_left_rotation(MPIDI_XPMEM_seg_t * parent,
                                                                    MPIDI_XPMEM_seg_t * right_child)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_XPMEM_AVL_DO_RIGHT_LEFT_ROTATION);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_XPMEM_AVL_DO_RIGHT_LEFT_ROTATION);

    MPIDI_XPMEM_seg_t *rl_child = right_child->left;
    mpi_errno = MPIDI_XPMEM_avl_do_right_rotation(right_child, rl_child);
    MPIR_ERR_CHECK(mpi_errno);
    mpi_errno = MPIDI_XPMEM_avl_do_left_rotation(parent, rl_child);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_XPMEM_AVL_DO_RIGHT_LEFT_ROTATION);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* Create a new segment and initialize tree node attributes */
MPL_STATIC_INLINE_PREFIX int MPIDI_XPMEM_segtree_do_create_node(uint64_t low,
                                                                uint64_t high,
                                                                xpmem_apid_t apid,
                                                                MPIDI_XPMEM_seg_t ** seg_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_XPMEM_seg_t *seg = NULL;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_XPMEM_SEGTREE_DO_CREATE_NODE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_XPMEM_SEGTREE_DO_CREATE_NODE);

    mpi_errno = MPIDI_XPMEM_seg_do_create(seg_ptr, low, high, apid);
    MPIR_ERR_CHECK(mpi_errno);

    seg = *seg_ptr;
    seg->height = 1;
    seg->parent = NULL;
    seg->left = NULL;
    seg->right = NULL;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_XPMEM_SEGTREE_DO_CREATE_NODE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/*
* Threads safe search and insert routine for a specified address range.
* It first performs binary search to find a matched segment; if it does
* not find one, then it inserts a new segment into the AVL tree. By combining
* search and insert into one routine, we need only one top-down traversal.
* The time complexity is O(logN).
*
* Input parameters:
*  - tree: the segment cache tree.
*  - low:  low address of searched address range.
*  - high: high address of searched address range.
*  - apid: apid of remote process. Used when inserting a new segment.
*
* Output parameters:
*  - seg_ptr: matched or newly inserted segment.
*  - voffset: offset between low address of the cached segment and the
*             inputed low address.
*
* Search procedure: find a matched segment which includes the entire range
*  specified by the input low address and high address. If input low address
*  is smaller than recorded segment, we then search left branch; otherwise,
*  we search right branch.
*
* Insert procedure: create and attach a new segment for the input address range.
*  We insert it into the tree based on the low address. If low address of the new
*  segment is smaller than the current segment node in AVL tree, it will be
*  inserted into left branch; otherwise, it will be inserted into the right branch.
*  After insertion, we check the height of each sub tree to make sure all of subtrees
*  still obey the AVL tree requirement (left branch and right branch height difference
*  cannot exceed 1). If it exceeds the limit, we then adjust the height of tree to
*  make it balanced again. */
MPL_STATIC_INLINE_PREFIX int MPIDI_XPMEM_segtree_do_search_and_insert_safe(MPIDI_XPMEM_segtree_t
                                                                           * tree, uint64_t low,
                                                                           uint64_t high,
                                                                           xpmem_apid_t apid,
                                                                           MPIDI_XPMEM_seg_t **
                                                                           seg_ptr, off_t * voffset)
{
    int direction;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_XPMEM_seg_t *sub_root = NULL;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_XPMEM_SEGTREE_DO_SEARCH_AND_INSERT_SAFE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_XPMEM_SEGTREE_DO_SEARCH_AND_INSERT_SAFE);

    MPID_THREAD_CS_ENTER(VCI, tree->lock);

    if (tree->root == NULL) {
        mpi_errno = MPIDI_XPMEM_segtree_do_create_node(low, high, apid, seg_ptr);
        MPIR_ERR_CHECK(mpi_errno);

        *voffset = 0;
        tree->root = *seg_ptr;
        tree->tree_size = 1;
    } else {
        sub_root = tree->root;

        MPIDI_XPMEM_AVL_DECLARE_STACK(node_stack, MPIDI_XPMEM_seg_t *, MPIDI_XPMEM_AVL_STACK_SIZE);

        do {
            if (low < sub_root->low) {
                /* Insert into left branch */
                if (sub_root->left != NULL) {
                    MPIDI_XPMEM_AVL_STACK_PUSH(node_stack, sub_root);
                    sub_root = sub_root->left;
                    continue;
                } else {
                    direction = AVL_LEFT;
                }
            } else if (high <= sub_root->high) {
                /* Hit the cache */
                *seg_ptr = sub_root;
                /* voffset is the offset between registered virtual address in
                 * cached segment and the virtual address we actually need. */
                *voffset = low - sub_root->low;
                goto fn_exit;
            } else {
                /* Insert into right branch */
                if (sub_root->right != NULL) {
                    MPIDI_XPMEM_AVL_STACK_PUSH(node_stack, sub_root);
                    sub_root = sub_root->right;
                    continue;
                } else {
                    direction = AVL_RIGHT;
                }
            }

            mpi_errno = MPIDI_XPMEM_segtree_do_create_node(low, high, apid, seg_ptr);
            MPIR_ERR_CHECK(mpi_errno);

            if (direction == AVL_LEFT)
                sub_root->left = *seg_ptr;
            else
                sub_root->right = *seg_ptr;

            (*seg_ptr)->parent = sub_root;
            *voffset = 0;
            tree->tree_size++;

          stack_recovery:
            mpi_errno = MPIDI_XPMEM_avl_do_update_node_info(sub_root);
            MPIR_ERR_CHECK(mpi_errno);

            int lheight = sub_root->left == NULL ? 0 : sub_root->left->height;
            int rheight = sub_root->right == NULL ? 0 : sub_root->right->height;
            if (lheight - rheight > 1) {
                /* left unbalanced */
                MPIDI_XPMEM_seg_t *ct_node = sub_root->left;
                int lct_height = ct_node->left == NULL ? 0 : ct_node->left->height;
                if (lct_height + 1 == lheight) {
                    /* Insert into left child of left child */
                    /* Do right rotation */
                    mpi_errno = MPIDI_XPMEM_avl_do_right_rotation(sub_root, ct_node);
                    MPIR_ERR_CHECK(mpi_errno);
                } else {
                    /* Insert into right child of left child */
                    /* Do left-right rotation */
                    mpi_errno = MPIDI_XPMEM_avl_do_left_right_rotation(sub_root, ct_node);
                    MPIR_ERR_CHECK(mpi_errno);
                }
            } else if (rheight - lheight > 1) {
                /* right unbalanced */
                MPIDI_XPMEM_seg_t *ct_node = sub_root->right;
                int lct_height = ct_node->left == NULL ? 0 : ct_node->left->height;
                if (lct_height + 1 == rheight) {
                    /* Insert into left child of right child */
                    /* Do right-left rotation */
                    mpi_errno = MPIDI_XPMEM_avl_do_right_left_rotation(sub_root, ct_node);
                    MPIR_ERR_CHECK(mpi_errno);
                } else {
                    /* Insert into right child of right child */
                    /* Do left rotation */
                    mpi_errno = MPIDI_XPMEM_avl_do_left_rotation(sub_root, ct_node);
                    MPIR_ERR_CHECK(mpi_errno);
                }
            }

            if (!MPIDI_XPMEM_AVL_STACK_EMPTY(node_stack)) {
                MPIDI_XPMEM_AVL_STACK_POP(node_stack, sub_root);
                goto stack_recovery;
            } else
                break;

            /* This infinite loop will break in either two cases:
             * (1) when we search AVL tree and hit the segment cache, it means
             * no insertion is needed, and we can jump to fn_exit and return cached
             * segment right away.
             * (2) when we cannot find cached segment, we need to attach and insert
             * a new segment into AVL tree; in this case, loop will exit when we
             * finish insertion and AVL tree height adjustment.  */
        } while (1);

        while (tree->root->parent != NULL)
            tree->root = tree->root->parent;
    }

  fn_exit:
    MPID_THREAD_CS_EXIT(VCI, tree->lock);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_XPMEM_SEGTREE_DO_SEARCH_AND_INSERT_SAFE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/****************************************/
/* Segment cache public routines        */
/****************************************/

/* Initialize an empty tree for segment cache.
 * It should be called only once for a AVL tree at MPI init.*/
MPL_STATIC_INLINE_PREFIX int MPIDI_XPMEM_segtree_init(MPIDI_XPMEM_segtree_t * tree)
{
    int mpi_errno = MPI_SUCCESS, ret;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_XPMEM_SEGTREE_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_XPMEM_SEGTREE_INIT);

    tree->root = NULL;
    tree->tree_size = 0;
    MPID_Thread_mutex_create(&tree->lock, &ret);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_XPMEM_SEGTREE_INIT);
    return mpi_errno;
}

/* Deletes all registered segments in the segment cache.
 * It detaches and frees each segment. */
MPL_STATIC_INLINE_PREFIX int MPIDI_XPMEM_segtree_delete_all(MPIDI_XPMEM_segtree_t * tree)
{
    uint8_t direction;
    int mpi_errno = MPI_SUCCESS, ret;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_XPMEM_SEGTREE_DELETE_ALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_XPMEM_SEGTREE_DELETE_ALL);

    MPIDI_XPMEM_AVL_DECLARE_STACK(node_stack, MPIDI_XPMEM_seg_t *, MPIDI_XPMEM_AVL_STACK_SIZE);
    MPIDI_XPMEM_AVL_DECLARE_STACK(direction_stack, uint8_t, MPIDI_XPMEM_AVL_STACK_SIZE);

    if (tree->tree_size) {
        MPIDI_XPMEM_seg_t *node = tree->root;
        while (1) {
            /* this delete function will traverse all the segments stored in AVL
             * tree, and this infinite loop is used to avoid recursive call of
             * function. It exits when all segments have been deleted (also means
             * stack is empty). */

            if (node->left != NULL) {
                MPIDI_XPMEM_AVL_STACK_PUSH(node_stack, node);
                /* AVL_LEFT stands for recovering from LEFT. */
                MPIDI_XPMEM_AVL_STACK_PUSH(direction_stack, AVL_LEFT);
                node = node->left;
                continue;
            }
          left_recovery:
            if (node->right != NULL) {
                MPIDI_XPMEM_AVL_STACK_PUSH(node_stack, node);
                /* AVL_RIGHT stands for recovering from RIGHT. */
                MPIDI_XPMEM_AVL_STACK_PUSH(direction_stack, AVL_RIGHT);
                node = node->right;
                continue;
            }
          right_recovery:
            mpi_errno = MPIDI_XPMEM_seg_do_release(node);
            MPIR_ERR_CHECK(mpi_errno);
            tree->tree_size--;
            if (!MPIDI_XPMEM_AVL_STACK_EMPTY(node_stack)) {
                MPIDI_XPMEM_AVL_STACK_POP(node_stack, node);
                MPIDI_XPMEM_AVL_STACK_POP(direction_stack, direction);
                if (direction == AVL_RIGHT)
                    goto right_recovery;
                else
                    goto left_recovery;
            } else
                break;
        }
    }

    MPID_Thread_mutex_destroy(&tree->lock, &ret);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_XPMEM_SEGTREE_DELETE_ALL);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* Registers a segment into cache for the specified remote buffer.
 * It internally rounds down the low address and rounds up the size to
 * ensure the cached segment is page aligned. Specific tree is given to
 * differentiate different cache tree (e.g. user buffer tree used to cache
 * user buffer, and XPMEM cooperative counter tree used to cache counter
 * obj)
 *
 * Input parameters:
 * - node_rank:    rank of remote process on local node.
 * - size:         size in bytes of the remote buffer.
 * - remote_vaddr: start virtual address of the remote buffer
 * - segcache: specific tree we want to insert segment into
 * Output parameters:
 * - seg_ptr: registered segment. It can be a matched existing segment
 *            or a newly created one.
 * - vaddr:   corresponding start address of the remote buffer in local
 *            virtual address space. */
MPL_STATIC_INLINE_PREFIX int MPIDI_XPMEM_seg_regist(int node_rank, size_t size,
                                                    void *remote_vaddr,
                                                    MPIDI_XPMEM_seg_t ** seg_ptr, void **vaddr,
                                                    MPIDI_XPMEM_segtree_t * segcache)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_XPMEM_segmap_t *segmap = &MPIDI_XPMEM_global.segmaps[node_rank];
    MPIDI_XPMEM_seg_t *seg = NULL;
    off_t offset_diff = 0, voffset = 0;
    uint64_t seg_low, seg_size, seg_high;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_XPMEM_SEG_REGIST);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_XPMEM_SEG_REGIST);
    /* Get apid if it is the first time registered on the local process. */
    if (segmap->apid == -1) {
        segmap->apid = xpmem_get(segmap->remote_segid, XPMEM_RDWR, XPMEM_PERMIT_MODE,
                                 MPIDI_XPMEM_PERMIT_VALUE);
        /* 64-bit access permit ID or failure(-1) */
        MPIR_ERR_CHKANDJUMP(segmap->apid == -1, mpi_errno, MPI_ERR_OTHER, "**xpmem_get");
        XPMEM_TRACE("seg: register apid 0x%lx for node_rank %d, segid 0x%lx\n",
                    (uint64_t) segmap->apid, node_rank, (uint64_t) segmap->remote_segid);
    }

    /* Search a cached segment or create a new one. Both low and size must be page aligned. */
    seg_low = MPL_ROUND_DOWN_ALIGN((uint64_t) remote_vaddr,
                                   (uint64_t) MPIDI_XPMEM_global.sys_page_sz);
    offset_diff = (off_t) remote_vaddr - seg_low;
    seg_size = MPL_ROUND_UP_ALIGN(size + (size_t) offset_diff, MPIDI_XPMEM_global.sys_page_sz);
    seg_high = seg_low + seg_size;
    mpi_errno =
        MPIDI_XPMEM_segtree_do_search_and_insert_safe(segcache, seg_low, seg_high,
                                                      segmap->apid, &seg, &voffset);
    MPIR_ERR_CHECK(mpi_errno);
    MPIR_Object_add_ref(seg);
    /* return mapped vaddr without round down */
    *vaddr = (void *) ((off_t) seg->vaddr + offset_diff + voffset);
    *seg_ptr = seg;
    XPMEM_TRACE("seg: register segment %p(refcount %d) for node_rank %d, apid 0x%lx, "
                "size 0x%lx->0x%lx, seg->low %p->0x%lx, attached_vaddr %p, vaddr %p\n", seg,
                MPIR_Object_get_ref(seg), node_rank, (uint64_t) segmap->apid, size, seg_size,
                remote_vaddr, seg->low, seg->vaddr, *vaddr);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_XPMEM_SEG_REGIST);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* Deregister a segment from cache.
 * It only decreases the segment's reference count. */
MPL_STATIC_INLINE_PREFIX int MPIDI_XPMEM_seg_deregist(MPIDI_XPMEM_seg_t * seg)
{
    int mpi_errno = MPI_SUCCESS, c = 0;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_XPMEM_SEG_DEREGIST);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_XPMEM_SEG_DEREGIST);
    MPIR_Object_release_ref(seg, &c);
    XPMEM_TRACE("seg: deregister segment %p(refcount %d) vaddr=%p\n", seg,
                MPIR_Object_get_ref(seg), seg->vaddr);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_XPMEM_SEG_DEREGIST);
    return mpi_errno;
}

#endif /* XPMEM_SEG_H_INCLUDED */

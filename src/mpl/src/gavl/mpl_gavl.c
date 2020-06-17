/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpl.h"
#include <assert.h>

#define GAVL_LEFT 0
#define GAVL_RIGHT 1
/*
 * We assume AVL tree height will not exceed 64. AVL tree with 64 height in worst case
 * can contain 27777890035287 nodes which is far enough for current applications.
 * The idea to compute worse case nodes is as follows:
 * In worse case, AVL tree with height h_p should have h_p - 1 height left child and
 * h_p - 2 height right child; therefore, the worse case nodes N(h_p) = N(h_p - 1) + N(h_p - 2) + 1.
 * Since we know N(1) = 1 and N(2) = 2, we can use iteration to compute N(64) = 27777890035287.
 */
#define MAX_STACK_SIZE 64

typedef struct gavl_tree_node {
    struct gavl_tree_node *parent;
    struct gavl_tree_node *left;
    struct gavl_tree_node *right;
    long height;
    long addr;
    uintptr_t len;
    const void *val;
} gavl_tree_node_s;

typedef struct gavl_tree {
    gavl_tree_node_s *root;
    uintptr_t valsize;
    int need_thread_safety;
    MPL_thread_mutex_t lock;
} gavl_tree_s;

#define DECLARE_STACK(type, stack) \
    type stack[MAX_STACK_SIZE];    \
    int stack##_sp = 0

#define STACK_PUSH(stack, value)             \
    do {                                     \
        assert(stack##_sp < MAX_STACK_SIZE); \
        stack[stack##_sp++] = value;         \
    } while (0)

#define STACK_POP(stack, value)      \
    do {                             \
        assert(stack##_sp > 0);      \
        value = stack[--stack##_sp]; \
    } while (0)

#define STACK_EMPTY(stack) (!stack##_sp)

static void gavl_update_node_info(gavl_tree_node_s * node_iptr)
{
    int lheight = node_iptr->left == NULL ? 0 : node_iptr->left->height;
    int rheight = node_iptr->right == NULL ? 0 : node_iptr->right->height;
    node_iptr->height = (lheight < rheight ? rheight : lheight) + 1;
    return;
}

static void gavl_right_rotation(gavl_tree_node_s * parent_ptr, gavl_tree_node_s * lchild)
{
    parent_ptr->left = lchild->right;
    lchild->right = parent_ptr;
    lchild->parent = parent_ptr->parent;
    if (lchild->parent != NULL) {
        if (lchild->parent->left == parent_ptr)
            lchild->parent->left = lchild;
        else
            lchild->parent->right = lchild;
    }

    parent_ptr->parent = lchild;
    if (parent_ptr->left != NULL)
        parent_ptr->left->parent = parent_ptr;

    gavl_update_node_info(parent_ptr);
    gavl_update_node_info(lchild);
    return;
}

static void gavl_left_rotation(gavl_tree_node_s * parent_ptr, gavl_tree_node_s * rchild)
{
    parent_ptr->right = rchild->left;
    rchild->left = parent_ptr;
    rchild->parent = parent_ptr->parent;
    if (rchild->parent != NULL) {
        if (rchild->parent->left == parent_ptr)
            rchild->parent->left = rchild;
        else
            rchild->parent->right = rchild;
    }

    parent_ptr->parent = rchild;
    if (parent_ptr->right != NULL)
        parent_ptr->right->parent = parent_ptr;

    gavl_update_node_info(parent_ptr);
    gavl_update_node_info(rchild);
    return;
}

static void gavl_left_right_rotation(gavl_tree_node_s * parent_ptr, gavl_tree_node_s * lchild)
{
    gavl_tree_node_s *rlchild = lchild->right;
    gavl_left_rotation(lchild, rlchild);
    gavl_right_rotation(parent_ptr, rlchild);
    return;
}

static void gavl_right_left_rotation(gavl_tree_node_s * parent_ptr, gavl_tree_node_s * rchild)
{
    gavl_tree_node_s *lrchild = rchild->left;
    gavl_right_rotation(rchild, lrchild);
    gavl_left_rotation(parent_ptr, lrchild);
    return;
}

MPL_STATIC_INLINE_PREFIX int gavl_cmp_func(long ustart, uintptr_t len, gavl_tree_node_s * tnode)
{
    long uend = ustart + len;
    long tstart = tnode->addr;
    long tend = tnode->addr + tnode->len;

    if (ustart < tstart)
        return -1;
    else if (uend <= tend)
        return 0;
    else
        return 1;
}

int MPL_gavl_tree_create(int need_thread_safety, MPL_gavl_tree_t * gavl_tree)
{
    int mpl_err;
    gavl_tree_s *gavl_tree_iptr;

    gavl_tree_iptr = (gavl_tree_s *) MPL_malloc(sizeof(gavl_tree_s), MPL_MEM_OTHER);
    if (gavl_tree_iptr == NULL)
        return MPL_ERR_SHM_NOMEM;

    if (need_thread_safety) {
        assert(MPL_THREAD_PACKAGE_NAME != MPL_THREAD_PACKAGE_NONE);
        MPL_thread_mutex_create(&gavl_tree_iptr->lock, &mpl_err);
        if (mpl_err != MPL_SUCCESS) {
            MPL_free(gavl_tree_iptr);
            return mpl_err;
        }
    }
    gavl_tree_iptr->need_thread_safety = need_thread_safety;

    gavl_tree_iptr->root = NULL;
    *gavl_tree = (MPL_gavl_tree_t) gavl_tree_iptr;
    return MPL_SUCCESS;
}

int MPL_gavl_tree_insert(MPL_gavl_tree_t gavl_tree, const void *addr, uintptr_t len,
                         const void *val)
{
    int mpl_err = MPL_SUCCESS;
    gavl_tree_node_s *node_ptr;
    gavl_tree_s *gavl_tree_iptr = (gavl_tree_s *) gavl_tree;

    if (gavl_tree_iptr->need_thread_safety) {
        MPL_thread_mutex_lock(&gavl_tree_iptr->lock, &mpl_err, MPL_THREAD_PRIO_HIGH);
        if (mpl_err != MPL_SUCCESS)
            return mpl_err;
    }

    node_ptr = (gavl_tree_node_s *) MPL_malloc(sizeof(gavl_tree_node_s), MPL_MEM_OTHER);
    node_ptr->parent = NULL;
    node_ptr->left = NULL;
    node_ptr->right = NULL;
    node_ptr->height = 1;
    node_ptr->addr = (long) addr;
    node_ptr->len = len;
    node_ptr->val = val;

    if (gavl_tree_iptr->root == NULL)
        gavl_tree_iptr->root = node_ptr;
    else {
        DECLARE_STACK(gavl_tree_node_s *, node_stack);
        gavl_tree_node_s *cur_node = gavl_tree_iptr->root;
        int direction;
        do {
            int cmp_ret = gavl_cmp_func((long) node_ptr->addr, node_ptr->len, cur_node);
            if (cmp_ret < 0) {
                if (cur_node->left != NULL) {
                    STACK_PUSH(node_stack, cur_node);
                    cur_node = cur_node->left;
                    continue;
                } else
                    direction = GAVL_LEFT;
            } else {
                if (cur_node->right != NULL) {
                    STACK_PUSH(node_stack, cur_node);
                    cur_node = cur_node->right;
                    continue;
                } else
                    direction = GAVL_RIGHT;
            }

            if (direction == GAVL_LEFT)
                cur_node->left = node_ptr;
            else
                cur_node->right = node_ptr;
            node_ptr->parent = cur_node;

          stack_recovery:
            gavl_update_node_info(cur_node);

            int lheight = cur_node->left == NULL ? 0 : cur_node->left->height;
            int rheight = cur_node->right == NULL ? 0 : cur_node->right->height;
            if (lheight - rheight > 1) {
                gavl_tree_node_s *lnode = cur_node->left;
                int llheight = lnode->left == NULL ? 0 : lnode->left->height;
                if (llheight + 1 == lheight)
                    gavl_right_rotation(cur_node, lnode);
                else
                    gavl_left_right_rotation(cur_node, lnode);
            } else if (rheight - lheight > 1) {
                gavl_tree_node_s *rnode = cur_node->right;
                int rlheight = rnode->left == NULL ? 0 : rnode->left->height;
                if (rlheight + 1 == rheight)
                    gavl_right_left_rotation(cur_node, rnode);
                else
                    gavl_left_rotation(cur_node, rnode);
            }

            if (!STACK_EMPTY(node_stack)) {
                STACK_POP(node_stack, cur_node);
                goto stack_recovery;
            } else
                break;
        } while (1);

        while (gavl_tree_iptr->root->parent != NULL)
            gavl_tree_iptr->root = gavl_tree_iptr->root->parent;
    }
    if (gavl_tree_iptr->need_thread_safety)
        MPL_thread_mutex_unlock(&gavl_tree_iptr->lock, &mpl_err);

    return mpl_err;
}

int MPL_gavl_tree_search(MPL_gavl_tree_t gavl_tree, const void *addr, uintptr_t len, void **val)
{
    int mpl_err = MPL_SUCCESS;
    gavl_tree_node_s *cur_node;
    gavl_tree_s *gavl_tree_iptr = (gavl_tree_s *) gavl_tree;

    if (gavl_tree_iptr->need_thread_safety) {
        MPL_thread_mutex_lock(&gavl_tree_iptr->lock, &mpl_err, MPL_THREAD_PRIO_HIGH);
        if (mpl_err != MPL_SUCCESS)
            return mpl_err;
    }

    *val = NULL;
    cur_node = gavl_tree_iptr->root;
    while (cur_node) {
        int cmp_ret = gavl_cmp_func((long) addr, len, cur_node);
        if (cmp_ret == 0) {
            *val = (void *) cur_node->val;
            break;
        } else if (cmp_ret < 0)
            cur_node = cur_node->left;
        else
            cur_node = cur_node->right;
    }

    if (gavl_tree_iptr->need_thread_safety)
        MPL_thread_mutex_unlock(&gavl_tree_iptr->lock, &mpl_err);
    return mpl_err;
}

int MPL_gavl_tree_free(MPL_gavl_tree_t gavl_tree, void (*free_fn) (void *))
{
    int mpl_err = MPL_SUCCESS;
    gavl_tree_s *gavl_tree_iptr = (gavl_tree_s *) gavl_tree;
    gavl_tree_node_s *cur_node = gavl_tree_iptr->root;
    gavl_tree_node_s *dnode = NULL;
    while (cur_node) {
        if (cur_node->left)
            cur_node = cur_node->left;
        else if (cur_node->right)
            cur_node = cur_node->right;
        else {
            dnode = cur_node;
            cur_node = cur_node->parent;
            if (cur_node) {
                if (cur_node->left == dnode)
                    cur_node->left = NULL;
                else
                    cur_node->right = NULL;
            }
            if (free_fn)
                free_fn((void *) dnode->val);
            MPL_free(dnode);
        }
    }
    if (gavl_tree_iptr->need_thread_safety)
        MPL_thread_mutex_destroy(&gavl_tree_iptr->lock, &mpl_err);
    MPL_free(gavl_tree_iptr);
    return mpl_err;
}

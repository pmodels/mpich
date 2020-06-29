/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpl.h"
#include <assert.h>

/*
 * We assume AVL tree height will not exceed 64. AVL tree with 64 height in worst case
 * can contain 27777890035287 nodes which is far enough for current applications.
 * The idea to compute worse case nodes is as follows:
 * In worse case, AVL tree with height h_p should have h_p - 1 height left child and
 * h_p - 2 height right child; therefore, the worse case nodes N(h_p) = N(h_p - 1) + N(h_p - 2) + 1.
 * Since we know N(1) = 1 and N(2) = 2, we can use iteration to compute N(64) = 27777890035287.
 */
#define MAX_STACK_SIZE 64

enum {
    SEARCH_LEFT,
    SEARCH_RIGHT,
    BUFFER_MATCH,
    NO_BUFFER_MATCH
};

typedef struct gavl_tree_node {
    struct gavl_tree_node *parent;
    struct gavl_tree_node *left;
    struct gavl_tree_node *right;
    uintptr_t height;
    uintptr_t addr;
    uintptr_t len;
    const void *val;
} gavl_tree_node_s;

typedef struct gavl_tree {
    gavl_tree_node_s *root;
    void (*gavl_free_fn) (void *);
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

#define CLEAR_STACK(stack)           \
    do {                             \
        stack##_sp = 0;              \
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

static int gavl_subset_cmp_func(uintptr_t ustart, uintptr_t len, gavl_tree_node_s * tnode)
{
    int cmp_ret;
    uintptr_t uend = ustart + len;
    uintptr_t tstart = tnode->addr;
    uintptr_t tend = tnode->addr + tnode->len;

    if (tstart <= ustart && uend <= tend)
        cmp_ret = BUFFER_MATCH;
    else if (ustart < tstart)
        cmp_ret = SEARCH_LEFT;
    else
        cmp_ret = SEARCH_RIGHT;

    return cmp_ret;
}

static int gavl_intersect_cmp_func(uintptr_t ustart, uintptr_t len, gavl_tree_node_s * tnode)
{
    int cmp_ret;
    uintptr_t uend = ustart + len;
    uintptr_t tstart = tnode->addr;
    uintptr_t tend = tnode->addr + tnode->len;

    if (uend <= tstart)
        cmp_ret = SEARCH_LEFT;
    else if (tend <= ustart)
        cmp_ret = SEARCH_RIGHT;
    else
        cmp_ret = BUFFER_MATCH;

    return cmp_ret;
}

int MPL_gavl_tree_create(void (*free_fn) (void *), MPL_gavl_tree_t * gavl_tree)
{
    int mpl_err = MPL_SUCCESS;
    gavl_tree_s *gavl_tree_iptr;

    gavl_tree_iptr = (gavl_tree_s *) MPL_malloc(sizeof(gavl_tree_s), MPL_MEM_OTHER);
    if (gavl_tree_iptr == NULL) {
        mpl_err = MPL_ERR_SHM_NOMEM;
        goto fn_fail;
    }

    gavl_tree_iptr->root = NULL;
    gavl_tree_iptr->gavl_free_fn = free_fn;
    *gavl_tree = (MPL_gavl_tree_t) gavl_tree_iptr;

  fn_exit:
    return mpl_err;
  fn_fail:
    goto fn_exit;
}

int MPL_gavl_tree_insert(MPL_gavl_tree_t gavl_tree, const void *addr, uintptr_t len,
                         const void *val)
{
    int mpl_err = MPL_SUCCESS;
    gavl_tree_node_s *node_ptr;
    gavl_tree_s *gavl_tree_iptr = (gavl_tree_s *) gavl_tree;

    node_ptr = (gavl_tree_node_s *) MPL_malloc(sizeof(gavl_tree_node_s), MPL_MEM_OTHER);
    if (node_ptr == NULL) {
        mpl_err = MPL_ERR_SHM_NOMEM;
        goto fn_fail;
    }
    node_ptr->parent = NULL;
    node_ptr->left = NULL;
    node_ptr->right = NULL;
    node_ptr->height = 1;
    node_ptr->addr = (uintptr_t) addr;
    node_ptr->len = len;
    node_ptr->val = val;

    if (gavl_tree_iptr->root == NULL) {
        gavl_tree_iptr->root = node_ptr;
    } else {
        DECLARE_STACK(gavl_tree_node_s *, node_stack);
        gavl_tree_node_s *cur_node = gavl_tree_iptr->root;
        int direction;
        do {
            int cmp_ret = gavl_subset_cmp_func((uintptr_t) node_ptr->addr, node_ptr->len, cur_node);
            if (cmp_ret == SEARCH_LEFT) {
                if (cur_node->left != NULL) {
                    STACK_PUSH(node_stack, cur_node);
                    cur_node = cur_node->left;
                    continue;
                } else {
                    direction = SEARCH_LEFT;
                }
            } else if (cmp_ret == SEARCH_RIGHT) {
                if (cur_node->right != NULL) {
                    STACK_PUSH(node_stack, cur_node);
                    cur_node = cur_node->right;
                    continue;
                } else {
                    direction = SEARCH_RIGHT;
                }
            } else {
                /* we cannot insert the duplicate buffer */
                gavl_tree_iptr->gavl_free_fn((void *) node_ptr->val);
                MPL_free(node_ptr);
                goto fn_exit;
            }

            if (direction == SEARCH_LEFT)
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
            } else {
                break;
            }
        } while (1);

        while (gavl_tree_iptr->root->parent != NULL)
            gavl_tree_iptr->root = gavl_tree_iptr->root->parent;
    }

  fn_exit:
    return mpl_err;
  fn_fail:
    goto fn_exit;
}

int MPL_gavl_tree_search(MPL_gavl_tree_t gavl_tree, const void *addr, uintptr_t len, void **val)
{
    int mpl_err = MPL_SUCCESS;
    gavl_tree_node_s *cur_node;
    gavl_tree_s *gavl_tree_iptr = (gavl_tree_s *) gavl_tree;

    *val = NULL;
    cur_node = gavl_tree_iptr->root;
    while (cur_node) {
        int cmp_ret = gavl_subset_cmp_func((uintptr_t) addr, len, cur_node);
        if (cmp_ret == BUFFER_MATCH) {
            *val = (void *) cur_node->val;
            break;
        } else if (cmp_ret == SEARCH_LEFT) {
            cur_node = cur_node->left;
        } else {
            cur_node = cur_node->right;
        }
    }

    return mpl_err;
}

int MPL_gavl_tree_free(MPL_gavl_tree_t gavl_tree)
{
    int mpl_err = MPL_SUCCESS;
    gavl_tree_s *gavl_tree_iptr = (gavl_tree_s *) gavl_tree;
    gavl_tree_node_s *cur_node = gavl_tree_iptr->root;
    gavl_tree_node_s *dnode = NULL;
    while (cur_node) {
        if (cur_node->left) {
            cur_node = cur_node->left;
        } else if (cur_node->right) {
            cur_node = cur_node->right;
        } else {
            dnode = cur_node;
            cur_node = cur_node->parent;
            if (cur_node) {
                if (cur_node->left == dnode)
                    cur_node->left = NULL;
                else
                    cur_node->right = NULL;
            }
            if (gavl_tree_iptr->gavl_free_fn)
                gavl_tree_iptr->gavl_free_fn((void *) dnode->val);
            MPL_free(dnode);
        }
    }
    MPL_free(gavl_tree_iptr);
    return mpl_err;
}

int MPL_gavl_tree_delete(MPL_gavl_tree_t gavl_tree, const void *addr, uintptr_t len)
{
    int mpl_err = MPL_SUCCESS;
    gavl_tree_node_s *cur_node;
    gavl_tree_node_s *inorder_node;
    gavl_tree_s *gavl_tree_iptr = (gavl_tree_s *) gavl_tree;

    DECLARE_STACK(gavl_tree_node_s *, node_stack);

    void *val;
    do {
        CLEAR_STACK(node_stack);
        cur_node = gavl_tree_iptr->root;
        val = NULL;
        if (cur_node == NULL) {
            goto fn_exit;
        } else {
            do {
                int cmp_ret = gavl_intersect_cmp_func((uintptr_t) addr, len, cur_node);
                if (cmp_ret == SEARCH_LEFT) {
                    if (cur_node->left != NULL) {
                        STACK_PUSH(node_stack, cur_node);
                        cur_node = cur_node->left;
                        continue;
                    } else {
                        break;
                    }
                } else if (cmp_ret == SEARCH_RIGHT) {
                    if (cur_node->right != NULL) {
                        STACK_PUSH(node_stack, cur_node);
                        cur_node = cur_node->right;
                        continue;
                    } else {
                        break;
                    }
                }

                if (cur_node->right == NULL) {
                    if (cur_node->parent == NULL) {
                        /* delete root node */
                        if (cur_node->left) {
                            gavl_tree_iptr->root = cur_node->left;
                            gavl_tree_iptr->root->parent = NULL;
                        } else {
                            gavl_tree_iptr->root = NULL;
                        }
                        val = (void *) cur_node->val;
                        MPL_free(cur_node);
                        break;
                    } else {
                        inorder_node = cur_node->parent;
                        if (inorder_node->left == cur_node)
                            inorder_node->left = cur_node->left;
                        else
                            inorder_node->right = cur_node->left;
                        if (cur_node->left)
                            cur_node->left->parent = inorder_node;

                        val = (void *) cur_node->val;
                        MPL_free(cur_node);
                        STACK_PUSH(node_stack, inorder_node);
                    }
                } else {
                    inorder_node = cur_node->right;
                    STACK_PUSH(node_stack, cur_node);
                    while (inorder_node->left) {
                        STACK_PUSH(node_stack, inorder_node);
                        inorder_node = inorder_node->left;
                    }

                    if (inorder_node->parent != cur_node) {
                        if (inorder_node->right)
                            inorder_node->right->parent = inorder_node->parent;
                        inorder_node->parent->left = inorder_node->right;
                    } else {
                        /* only right child of deleted node */
                        cur_node->right = NULL;
                    }
                    val = (void *) cur_node->val;
                    cur_node->addr = inorder_node->addr;
                    cur_node->len = inorder_node->len;
                    cur_node->val = inorder_node->val;
                    MPL_free(inorder_node);
                }

                STACK_POP(node_stack, cur_node);

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
                } else {
                    break;
                }
            } while (1);

            while (gavl_tree_iptr->root && gavl_tree_iptr->root->parent != NULL)
                gavl_tree_iptr->root = gavl_tree_iptr->root->parent;

            if (val && gavl_tree_iptr->gavl_free_fn)
                gavl_tree_iptr->gavl_free_fn(val);
        }
    } while (val);

  fn_exit:
    return mpl_err;
  fn_fail:
    goto fn_exit;
}

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

enum {
    SUBSET_SEARCH,
    INTERSECTION_SEARCH
};

#ifdef __SUNPRO_C
#pragma error_messages (off, E_ANONYMOUS_STRUCT_DECL)
#endif

typedef struct gavl_tree_node {
    union {
        struct {
            struct gavl_tree_node *parent;
            struct gavl_tree_node *left;
            struct gavl_tree_node *right;
        };
        struct gavl_tree_node *next;
    };
    uintptr_t height;
    uintptr_t addr;
    uintptr_t len;
    const void *val;
} gavl_tree_node_s;

typedef struct gavl_tree {
    gavl_tree_node_s *root;
    void (*gavl_free_fn) (void *);
    /* internal stack structure. used to track the traverse trace for
     * tree rebalance at node insertion or deletion */
    gavl_tree_node_s *stack[MAX_STACK_SIZE];
    int stack_sp;
    /* cur_node points to the starting node of tree rebalance */
    gavl_tree_node_s *cur_node;
    /* store nodes that are removed from tree but haven't been freed */
    gavl_tree_node_s *remove_list;
} gavl_tree_s;

#define GAVL_TREE_NODE_INIT(node_ptr, addr, len, val)   \
    do {                                                \
        (node_ptr)->height = 1;                         \
        (node_ptr)->addr = (uintptr_t) addr;            \
        (node_ptr)->len = len;                          \
        (node_ptr)->val = val;                          \
    } while (0)

#define GAVL_TREE_NODE_CMP(node_ptr, addr, len, mode, ret)      \
    do {                                                        \
        if (mode == SUBSET_SEARCH) {                            \
            ret = gavl_subset_cmp_func(node_ptr, addr, len);    \
        }                                                       \
        else if (mode == INTERSECTION_SEARCH) {                 \
            ret = gavl_intersect_cmp_func(node_ptr, addr, len); \
        }                                                       \
    } while (0)

/* STACK is needed to rebalance the tree */
#define TREE_STACK_PUSH(tree_ptr, value)               \
    do {                                               \
        assert(tree_ptr->stack_sp < MAX_STACK_SIZE);   \
        tree_ptr->stack[tree_ptr->stack_sp++] = value; \
    } while (0)

#define TREE_STACK_POP(tree_ptr, value)                \
    do {                                               \
        assert(tree_ptr->stack_sp > 0);                \
        value = tree_ptr->stack[--tree_ptr->stack_sp]; \
    } while (0)

#define TREE_STACK_START(tree_ptr) (tree_ptr)->stack_sp = 0
#define TREE_STACK_IS_EMPTY(tree_ptr) (!(tree_ptr)->stack_sp)

static void gavl_tree_remove_nodes(gavl_tree_s * tree_ptr, uintptr_t addr, uintptr_t len, int mode);
static void gavl_tree_delete_removed_nodes(gavl_tree_s * tree_ptr, uintptr_t addr, uintptr_t len);

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

MPL_STATIC_INLINE_PREFIX int gavl_subset_cmp_func(gavl_tree_node_s * tnode, uintptr_t ustart,
                                                  uintptr_t len)
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

MPL_STATIC_INLINE_PREFIX int gavl_intersect_cmp_func(gavl_tree_node_s * tnode, uintptr_t ustart,
                                                     uintptr_t len)
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

/*
 * MPL_gavl_tree_create
 * Description: create a gavl tree
 * Parameters:
 * free_fn        - (IN) user free function to free buffer object
 * gavl_tree      - (OUT) created gavl tree
 */
int MPL_gavl_tree_create(void (*free_fn) (void *), MPL_gavl_tree_t * gavl_tree)
{
    int mpl_err = MPL_SUCCESS;
    gavl_tree_s *tree_ptr;

    tree_ptr = (gavl_tree_s *) MPL_calloc(1, sizeof(gavl_tree_s), MPL_MEM_OTHER);
    if (tree_ptr == NULL) {
        mpl_err = MPL_ERR_NOMEM;
        goto fn_fail;
    }

    tree_ptr->gavl_free_fn = free_fn;
    *gavl_tree = (MPL_gavl_tree_t) tree_ptr;

  fn_exit:
    return mpl_err;
  fn_fail:
    goto fn_exit;
}

static gavl_tree_node_s *gavl_tree_search_internal(gavl_tree_s * tree_ptr, uintptr_t addr,
                                                   uintptr_t len, int mode, int *cmp_ret_ptr)
{
    /* this function assumes there is at least one node in the tree */
    int cmp_ret = NO_BUFFER_MATCH;
    gavl_tree_node_s *cur_node = tree_ptr->root;

    TREE_STACK_START(tree_ptr);
    do {
        GAVL_TREE_NODE_CMP(cur_node, addr, len, mode, cmp_ret);
        if (cmp_ret == SEARCH_LEFT) {
            if (cur_node->left != NULL) {
                TREE_STACK_PUSH(tree_ptr, cur_node);
                cur_node = cur_node->left;
                continue;
            } else {
                break;
            }
        } else if (cmp_ret == SEARCH_RIGHT) {
            if (cur_node->right != NULL) {
                TREE_STACK_PUSH(tree_ptr, cur_node);
                cur_node = cur_node->right;
                continue;
            } else {
                break;
            }
        } else {
            /* node match */
            break;
        }
    } while (1);

    *cmp_ret_ptr = cmp_ret;
    tree_ptr->cur_node = cur_node;
    return cur_node;
}

/* if avl tree is possibly unbalanced, gavl_tree_rebalance should be called to rebalance
 * it. In unbalanced avl tree, the height difference between left and right child is at
 * most 2; gavl_tree_rebalance takes it as a premise in order to rebalance tree correcly */
static void gavl_tree_rebalance(gavl_tree_s * tree_ptr)
{
    gavl_tree_node_s *cur_node = tree_ptr->cur_node;

    if (cur_node) {
        do {
            gavl_update_node_info(cur_node);

            int lheight = cur_node->left == NULL ? 0 : cur_node->left->height;
            int rheight = cur_node->right == NULL ? 0 : cur_node->right->height;
            if (lheight - rheight > 1) {
                /* find imbalance: left child is 2 level higher than right child */
                gavl_tree_node_s *lnode = cur_node->left;
                int llheight = lnode->left == NULL ? 0 : lnode->left->height;
                /* if left child's (lnode's) left child causes this imbalance, we need to perform right
                 * rotation to reduce the height of lnode's left child;
                 * right rotation sets left child (lnode) as central node, moves lnode to parent (cur_node)
                 * position, assigns cur_node to right child of lnode and then lnode's right child to left
                 * child of cur_node.
                 * else we need to perform left-right rotation for rebalance; left-right rotation first
                 * moves right child (rlnode) of left child (lnode) to lnode position and assigns left
                 * child of rlnode to right child of lnode; then set rlnode as central node and perform
                 * right rotation as mentioned above */
                if (llheight + 1 == lheight)
                    gavl_right_rotation(cur_node, lnode);
                else
                    gavl_left_right_rotation(cur_node, lnode);
            } else if (rheight - lheight > 1) {
                /* find imbalance: right child is 2 level higher than left child */
                gavl_tree_node_s *rnode = cur_node->right;
                int rlheight = rnode->left == NULL ? 0 : rnode->left->height;
                /* the purpose of gavl_right_left_rotation and gavl_left_rotation is similar to
                 * gavl_right_rotation and gavl_left_right_rotation mention above; the difference
                 * is just doing rotation for right child here*/
                if (rlheight + 1 == rheight)
                    gavl_right_left_rotation(cur_node, rnode);
                else
                    gavl_left_rotation(cur_node, rnode);
            }

            /* rebalance the previous nodes in traverse trace */
            if (!TREE_STACK_IS_EMPTY(tree_ptr)) {
                TREE_STACK_POP(tree_ptr, cur_node);
                continue;
            } else {
                break;
            }
        } while (1);

        /* after rebalance, we need to update root because it might be changed after rebalance */
        while (tree_ptr->root && tree_ptr->root->parent != NULL)
            tree_ptr->root = tree_ptr->root->parent;
    }

    return;
}

/*
 * MPL_gavl_tree_insert
 * Description: insert a node with key (addr, len) into gavl tree. If new node is duplicate,
 *              we should not insert it and need to free the node and return. This function
 *              is not thread-safe.
 * Parameters:
 * gavl_tree        - (IN) gavl tree object
 * addr             - (IN) input buffer starting addr
 * len              - (IN) input buffer length
 * val              - (IN) buffer object
 */
int MPL_gavl_tree_insert(MPL_gavl_tree_t gavl_tree, const void *addr, uintptr_t len,
                         const void *val)
{
    int mpl_err = MPL_SUCCESS;
    gavl_tree_node_s *node_ptr;
    gavl_tree_s *tree_ptr = (gavl_tree_s *) gavl_tree;

    /* we remove all nodes that are subset of input key (addr, len) from the tree and add them
     * into tree remove_list */
    gavl_tree_remove_nodes(tree_ptr, (uintptr_t) addr, len, SUBSET_SEARCH);

    node_ptr = (gavl_tree_node_s *) MPL_calloc(1, sizeof(gavl_tree_node_s), MPL_MEM_OTHER);
    if (node_ptr == NULL) {
        mpl_err = MPL_ERR_NOMEM;
        goto fn_fail;
    }

    GAVL_TREE_NODE_INIT(node_ptr, addr, len, val);

    if (tree_ptr->root == NULL) {
        tree_ptr->root = node_ptr;
    } else {
        gavl_tree_node_s *pnode;
        int cmp_ret;

        /* search the node which will become the parent of new node */
        pnode = gavl_tree_search_internal(tree_ptr, (uintptr_t) node_ptr->addr, node_ptr->len,
                                          SUBSET_SEARCH, &cmp_ret);

        /* find which side the new node should be inserted */
        if (cmp_ret == BUFFER_MATCH) {
            /* new node is duplicate, we need to delete new node and exit */
            tree_ptr->gavl_free_fn((void *) node_ptr->val);
            MPL_free(node_ptr);
            goto fn_exit;
        }

        /* insert new node into pnode */
        if (cmp_ret == SEARCH_LEFT)
            pnode->left = node_ptr;
        else
            pnode->right = node_ptr;
        node_ptr->parent = pnode;

        /* after insertion, the tree could be imbalanced, so rebalance is required here */
        gavl_tree_rebalance(tree_ptr);
    }

  fn_exit:
    return mpl_err;
  fn_fail:
    goto fn_exit;
}

/*
 * MPL_gavl_tree_search
 * Description: search a node that matches input key (addr, len) and return corresponding
 *              buffer object. This function is not thread-safe.
 * Parameters:
 * gavl_tree        - (IN) gavl tree object
 * addr             - (IN) input buffer starting addr
 * len              - (IN) input buffer length
 * val              - (OUT) matched buffer object
 */
int MPL_gavl_tree_search(MPL_gavl_tree_t gavl_tree, const void *addr, uintptr_t len, void **val)
{
    int mpl_err = MPL_SUCCESS;
    gavl_tree_node_s *cur_node;
    gavl_tree_s *tree_ptr = (gavl_tree_s *) gavl_tree;

    *val = NULL;
    cur_node = tree_ptr->root;
    while (cur_node) {
        int cmp_ret = gavl_subset_cmp_func(cur_node, (uintptr_t) addr, len);
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

/*
 * MPL_gavl_tree_destory
 * Description: free all nodes and buffer objects in the tree and tree itself.
 * Parameters:
 * gavl_tree        - (IN)  gavl tree object
 */
int MPL_gavl_tree_destory(MPL_gavl_tree_t gavl_tree)
{
    int mpl_err = MPL_SUCCESS;
    gavl_tree_s *tree_ptr = (gavl_tree_s *) gavl_tree;
    gavl_tree_node_s *cur_node = tree_ptr->root;
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
            if (tree_ptr->gavl_free_fn)
                tree_ptr->gavl_free_fn((void *) dnode->val);
            MPL_free(dnode);
        }
    }
    MPL_free(tree_ptr);
    return mpl_err;
}

static void gavl_tree_remove_node_internal(gavl_tree_s * tree_ptr, gavl_tree_node_s * dnode)
{
    gavl_tree_node_s *inorder_node;

    if (dnode->right == NULL) {
        /* no right child, next inorder node is parent */
        if (dnode->parent == NULL) {
            /* delete root node; if it has left child, set left child as root node;
             * if not, set tree root as NULL */
            if (dnode->left) {
                tree_ptr->root = dnode->left;
                tree_ptr->root->parent = NULL;
            } else {
                tree_ptr->root = NULL;
            }
        } else {
            /* assign deleted node's left child to its parent */
            inorder_node = dnode->parent;
            if (inorder_node->left == dnode)
                inorder_node->left = dnode->left;
            else
                inorder_node->right = dnode->left;

            if (dnode->left)
                dnode->left->parent = inorder_node;

            TREE_STACK_PUSH(tree_ptr, inorder_node);
        }
    } else {
        const void *tmp_val;
        uintptr_t tmp_addr, tmp_len;

        /* find the next inorder node and move its buffer objects to dnode;
         * the original buffer object in dnode is freed */
        inorder_node = dnode->right;
        TREE_STACK_PUSH(tree_ptr, dnode);

        /* search left most node of right child of dnode for next inorder node */
        while (inorder_node->left) {
            TREE_STACK_PUSH(tree_ptr, inorder_node);
            inorder_node = inorder_node->left;
        }

        /* remove inorder_node from the tree. */
        if (inorder_node->parent != dnode) {
            if (inorder_node->right)
                inorder_node->right->parent = inorder_node->parent;
            inorder_node->parent->left = inorder_node->right;
        } else {
            dnode->right = NULL;
        }

        /* exchange inorder_node with dnode and then add dnode into remove_list */
        tmp_val = dnode->val;
        tmp_addr = dnode->addr;
        tmp_len = dnode->len;
        dnode->addr = inorder_node->addr;
        dnode->len = inorder_node->len;
        dnode->val = inorder_node->val;
        inorder_node->addr = tmp_addr;
        inorder_node->len = tmp_len;
        inorder_node->val = tmp_val;
        dnode = inorder_node;
    }

    /* add removed node into remove list */
    dnode->next = tree_ptr->remove_list;
    tree_ptr->remove_list = dnode;

    /* update stack for the consequent rebalance which will start from the top of
     * the stack (i.e., tree_ptr->cur_node). */
    if (TREE_STACK_IS_EMPTY(tree_ptr)) {
        tree_ptr->cur_node = NULL;
    } else {
        TREE_STACK_POP(tree_ptr, tree_ptr->cur_node);
    }
    return;
}

/*
 * MPL_gavl_tree_delete
 * Description: delete all intersecting nodes with input buffer in gavl tree and free
 *              corresponding buffer objects using user-provided free function. This
 *              function is not thread-safe.
 * Parameters:
 * gavl_tree        - (IN) gavl tree object
 * addr             - (IN) input buffer starting addr
 * len              - (IN) input buffer length
 */
int MPL_gavl_tree_delete(MPL_gavl_tree_t gavl_tree, const void *addr, uintptr_t len)
{
    int mpl_err = MPL_SUCCESS;
    gavl_tree_s *tree_ptr = (gavl_tree_s *) gavl_tree;

    /* move all nodes intersecting input buffer (addr, len) to remove_list */
    gavl_tree_remove_nodes(tree_ptr, (uintptr_t) addr, len, INTERSECTION_SEARCH);

    /* free nodes and buffer objects from remove list */
    gavl_tree_delete_removed_nodes(tree_ptr, (uintptr_t) addr, len);

  fn_exit:
    return mpl_err;
  fn_fail:
    goto fn_exit;
}

static void gavl_tree_remove_nodes(gavl_tree_s * tree_ptr, uintptr_t addr, uintptr_t len, int mode)
{
    int cmp_ret;
    gavl_tree_node_s *dnode;

    while (tree_ptr->root) {
        /* search and return the node to be deleted */
        dnode = gavl_tree_search_internal(tree_ptr, (uintptr_t) addr, len, mode, &cmp_ret);

        /* check whether dnode matches (addr, len) */
        if (cmp_ret != BUFFER_MATCH) {
            /* we didn't find deleted node and exit */
            goto fn_exit;
        }

        /* detach the matched node from tree and add removed node into remove list */
        gavl_tree_remove_node_internal(tree_ptr, dnode);

        /* we perform rebalance after every internal deletion in order to ensure
         * lightweight rebalance that rotates left and right childs with at most
         * 2 height difference. */
        gavl_tree_rebalance(tree_ptr);
    };

  fn_exit:
    return;
}

/* gavl_tree_delete_removed_nodes delete all nodes that intersect input key (addr, len) in remove_list */
static void gavl_tree_delete_removed_nodes(gavl_tree_s * tree_ptr, uintptr_t addr, uintptr_t len)
{
    int cmp_ret;
    gavl_tree_node_s *prev, *cur, *dnode;

    cur = tree_ptr->remove_list;
    prev = NULL;
    while (cur) {
        cmp_ret = gavl_intersect_cmp_func(cur, addr, len);
        if (cmp_ret == BUFFER_MATCH) {
            if (prev)
                prev->next = cur->next;
            else
                tree_ptr->remove_list = cur->next;

            dnode = cur;
            cur = cur->next;
            if (tree_ptr->gavl_free_fn)
                tree_ptr->gavl_free_fn((void *) dnode->val);
            MPL_free(dnode);
        } else {
            prev = cur;
            cur = cur->next;
        }
    }
}

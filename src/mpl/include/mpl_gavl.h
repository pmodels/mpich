#ifndef GAVL_H_INCLUDED
#define GAVL_H_INCLUDED

#include <stdlib.h>
#include <string.h>
#include "mpidimpl.h"
typedef void *MPL_gavl_tree_hd_t;

#define MPL_GAVL_TREE_HEADER \
    void *parent;            \
    void *left;              \
    void *right;             \
    long height

#define MPL_GAVL_LEFT 0
#define MPL_GAVL_RIGHT 1

#define MPL_MAX_STACK_SIZE 64

#define MPL_DECLARE_STACK(type, stack) \
    type stack[MPL_MAX_STACK_SIZE];    \
    int stack##_sp = 0

#define MPL_STACK_PUSH(stack, value)                  \
    do                                                \
    {                                                 \
        MPIR_Assert(stack##_sp < MPL_MAX_STACK_SIZE); \
        stack[stack##_sp++] = value;                  \
    } while (0)

#define MPL_STACK_POP(stack, value)  \
    do                               \
    {                                \
        MPIR_Assert(stack##_sp > 0); \
        value = stack[--stack##_sp]; \
    } while (0)

#define MPL_STACK_EMPTY(stack) (!stack##_sp)

/*
 *  For user-defined compare function, first parameter unode should be a 
 *  user-provided node, and second parameter tnode is a node inside tree
 *   
 *  Meaning of the return value of compare function:
 *  negative number => unode < tnode
 *  0 => unode == tnode
 *  positive number => unode > tnode
 *   
 *  For user-defined free function, when user passes NULL, there will be
 *  no action in finalizing tree. The free function should follow function
 *  interface "void (*)(void *node)"
 */

#define MPL_define_gavl_tree_type(NodeType)                      \
    typedef struct MPL_gavl_tree_internal                        \
    {                                                            \
        NodeType *root;                                          \
        MPID_Thread_mutex_t tlock;                               \
        int (*gavl_cmp_func)(NodeType * unode, NodeType *tnode); \
        void (*gavl_free_func)(void *node);                      \
    } MPL_gavl_tree_internal_t

#define MPL_gavl_tree_create(NodeType, gavl_cmp_func_ptr, gavl_free_func_ptr, gavl_tree_hd, mpl_err)                  \
    do                                                                                                                \
    {                                                                                                                 \
        MPL_define_gavl_tree_type(NodeType);                                                                          \
        int mpl_gavl_tree_ret;                                                                                        \
        MPL_gavl_tree_internal_t *mpl_gavl_tree_iptr;                                                                 \
        mpl_err = 0;                                                                                                  \
        mpl_gavl_tree_iptr = (MPL_gavl_tree_internal_t *)MPL_malloc(sizeof(MPL_gavl_tree_internal_t), MPL_MEM_OTHER); \
        if (mpl_gavl_tree_iptr == NULL)                                                                               \
        {                                                                                                             \
            mpl_err = -1;                                                                                             \
            break;                                                                                                    \
        }                                                                                                             \
        mpl_gavl_tree_iptr->root = NULL;                                                                              \
        MPID_Thread_mutex_create(&mpl_gavl_tree_iptr->tlock, &mpl_err);                                               \
        if (mpl_err != 0)                                                                                             \
            break;                                                                                                    \
        mpl_gavl_tree_iptr->gavl_cmp_func = gavl_cmp_func_ptr;                                                        \
        mpl_gavl_tree_iptr->gavl_free_func = gavl_free_func_ptr;                                                      \
        gavl_tree_hd = (MPL_gavl_tree_hd_t)mpl_gavl_tree_iptr;                                                        \
    } while (0)

#define MPL_gavl_update_node_info(NodeType, node_iptr)                                       \
    do                                                                                       \
    {                                                                                        \
        int lheight = node_iptr->left == NULL ? 0 : ((NodeType *)node_iptr->left)->height;   \
        int rheight = node_iptr->right == NULL ? 0 : ((NodeType *)node_iptr->right)->height; \
        node_iptr->height = (lheight < rheight ? rheight : lheight) + 1;                     \
    } while (0)

#define MPL_gavl_right_rotation(NodeType, parent_ptr, lchild)     \
    do                                                            \
    {                                                             \
        parent_ptr->left = lchild->right;                         \
        lchild->right = (void *)parent_ptr;                       \
        lchild->parent = parent_ptr->parent;                      \
        if (lchild->parent != NULL)                               \
        {                                                         \
            if (((NodeType *)lchild->parent)->left == parent_ptr) \
                ((NodeType *)lchild->parent)->left = lchild;      \
            else                                                  \
                ((NodeType *)lchild->parent)->right = lchild;     \
        }                                                         \
                                                                  \
        parent_ptr->parent = (void *)lchild;                      \
        if (parent_ptr->left != NULL)                             \
            ((NodeType *)parent_ptr->left)->parent = parent_ptr;  \
                                                                  \
        MPL_gavl_update_node_info(NodeType, parent_ptr);          \
        MPL_gavl_update_node_info(NodeType, lchild);              \
                                                                  \
    } while (0)

#define MPL_gavl_left_rotation(NodeType, parent_ptr, rchild)              \
    do                                                                    \
    {                                                                     \
        parent_ptr->right = rchild->left;                                 \
        rchild->left = (void *)parent_ptr;                                \
        rchild->parent = parent_ptr->parent;                              \
        if (rchild->parent != NULL)                                       \
        {                                                                 \
            if (((NodeType *)rchild->parent)->left == (void *)parent_ptr) \
                ((NodeType *)rchild->parent)->left = (void *)rchild;      \
            else                                                          \
                ((NodeType *)rchild->parent)->right = (void *)rchild;     \
        }                                                                 \
                                                                          \
        parent_ptr->parent = (void *)rchild;                              \
        if (parent_ptr->right != NULL)                                    \
            ((NodeType *)parent_ptr->right)->parent = (void *)parent_ptr; \
                                                                          \
        MPL_gavl_update_node_info(NodeType, parent_ptr);                  \
        MPL_gavl_update_node_info(NodeType, rchild);                      \
    } while (0)

#define MPL_gavl_left_right_rotation(NodeType, parent_ptr, lchild) \
    do                                                             \
    {                                                              \
        NodeType *rlchild = (NodeType *)lchild->right;             \
        MPL_gavl_left_rotation(NodeType, lchild, rlchild);         \
        MPL_gavl_right_rotation(NodeType, parent_ptr, rlchild);    \
    } while (0)

#define MPL_gavl_right_left_rotation(NodeType, parent_ptr, rchild) \
    do                                                             \
    {                                                              \
        NodeType *lrchild = (NodeType *)rchild->left;              \
        MPL_gavl_right_rotation(NodeType, rchild, lrchild);        \
        MPL_gavl_left_rotation(NodeType, parent_ptr, lrchild);     \
    } while (0)

#define MPL_gavl_tree_insert(NodeType, gavl_tree_hd, node_ptr, mpl_err)                                \
    do                                                                                                 \
    {                                                                                                  \
        int MPL_gavl_tree_insert_func(MPL_gavl_tree_hd_t gavl_tree_ihd, NodeType *node_iptr)           \
        {                                                                                              \
            MPL_define_gavl_tree_type(NodeType);                                                       \
            MPL_gavl_tree_internal_t *mpl_gavl_tree_iptr = (MPL_gavl_tree_internal_t *)gavl_tree_ihd;  \
            if (node_iptr == NULL || mpl_gavl_tree_iptr == NULL)                                       \
                return -1;                                                                             \
            node_iptr->parent = NULL;                                                                  \
            node_iptr->left = NULL;                                                                    \
            node_iptr->right = NULL;                                                                   \
            node_iptr->height = 1;                                                                     \
            MPID_THREAD_CS_ENTER(VCI, mpl_gavl_tree_iptr->tlock);                                      \
            if (mpl_gavl_tree_iptr->root == NULL)                                                      \
            {                                                                                          \
                mpl_gavl_tree_iptr->root = node_iptr;                                                  \
            }                                                                                          \
            else                                                                                       \
            {                                                                                          \
                MPL_DECLARE_STACK(NodeType *, node_stack);                                             \
                NodeType *cur_node = mpl_gavl_tree_iptr->root;                                         \
                int direction;                                                                         \
                do                                                                                     \
                {                                                                                      \
                    int func_ret = mpl_gavl_tree_iptr->gavl_cmp_func(node_iptr, cur_node);             \
                    if (func_ret < 0)                                                                  \
                    {                                                                                  \
                        if (cur_node->left != NULL)                                                    \
                        {                                                                              \
                            MPL_STACK_PUSH(node_stack, cur_node);                                      \
                            cur_node = (NodeType *)cur_node->left;                                     \
                            continue;                                                                  \
                        }                                                                              \
                        else                                                                           \
                        {                                                                              \
                            direction = MPL_GAVL_LEFT;                                                 \
                        }                                                                              \
                    }                                                                                  \
                    else if (func_ret > 0)                                                             \
                    {                                                                                  \
                        if (cur_node->right != NULL)                                                   \
                        {                                                                              \
                            MPL_STACK_PUSH(node_stack, cur_node);                                      \
                            cur_node = (NodeType *)cur_node->right;                                    \
                            continue;                                                                  \
                        }                                                                              \
                        else                                                                           \
                        {                                                                              \
                            direction = MPL_GAVL_RIGHT;                                                \
                        }                                                                              \
                    }                                                                                  \
                    else                                                                               \
                    {                                                                                  \
                        break;                                                                         \
                    }                                                                                  \
                                                                                                       \
                    if (direction == MPL_GAVL_LEFT)                                                    \
                        cur_node->left = (void *)node_iptr;                                            \
                    else                                                                               \
                        cur_node->right = (void *)node_iptr;                                           \
                    node_iptr->parent = cur_node;                                                      \
                                                                                                       \
                stack_recovery:                                                                        \
                    MPL_gavl_update_node_info(NodeType, cur_node);                                     \
                                                                                                       \
                    int lheight = cur_node->left == NULL ? 0 : ((NodeType *)cur_node->left)->height;   \
                    int rheight = cur_node->right == NULL ? 0 : ((NodeType *)cur_node->right)->height; \
                    if (lheight - rheight > 1)                                                         \
                    {                                                                                  \
                        NodeType *lnode = (NodeType *)cur_node->left;                                  \
                        int llheight = lnode->left == NULL ? 0 : ((NodeType *)lnode->left)->height;    \
                        if (llheight + 1 == lheight)                                                   \
                        {                                                                              \
                            MPL_gavl_right_rotation(NodeType, cur_node, lnode);                        \
                        }                                                                              \
                        else                                                                           \
                        {                                                                              \
                            MPL_gavl_left_right_rotation(NodeType, cur_node, lnode);                   \
                        }                                                                              \
                    }                                                                                  \
                    else if (rheight - lheight > 1)                                                    \
                    {                                                                                  \
                        NodeType *rnode = (NodeType *)cur_node->right;                                 \
                        int rlheight = rnode->left == NULL ? 0 : ((NodeType *)rnode->left)->height;    \
                        if (rlheight + 1 == rheight)                                                   \
                        {                                                                              \
                            MPL_gavl_right_left_rotation(NodeType, cur_node, rnode);                   \
                        }                                                                              \
                        else                                                                           \
                        {                                                                              \
                            MPL_gavl_left_rotation(NodeType, cur_node, rnode);                         \
                        }                                                                              \
                    }                                                                                  \
                                                                                                       \
                    if (!MPL_STACK_EMPTY(node_stack))                                                  \
                    {                                                                                  \
                        MPL_STACK_POP(node_stack, cur_node);                                           \
                        goto stack_recovery;                                                           \
                    }                                                                                  \
                    else                                                                               \
                        break;                                                                         \
                } while (1);                                                                           \
                                                                                                       \
                while (mpl_gavl_tree_iptr->root->parent != NULL)                                       \
                    mpl_gavl_tree_iptr->root = (NodeType *)mpl_gavl_tree_iptr->root->parent;           \
            }                                                                                          \
            MPID_THREAD_CS_EXIT(VCI, mpl_gavl_tree_iptr->tlock);                                       \
            return 0;                                                                                  \
        }                                                                                              \
        mpl_err = MPL_gavl_tree_insert_func(gavl_tree_hd, node_ptr);                                   \
    } while (0)

#define MPL_gavl_tree_search(NodeType, gavl_tree_hd, node_ptr, out_ptr, mpl_err)                 \
    do                                                                                           \
    {                                                                                            \
        MPL_define_gavl_tree_type(NodeType);                                                     \
        MPL_gavl_tree_internal_t *mpl_gavl_tree_iptr = (MPL_gavl_tree_internal_t *)gavl_tree_hd; \
        NodeType *cur_node = mpl_gavl_tree_iptr->root;                                           \
        NodeType *node_iptr = node_ptr;                                                          \
        mpl_err = 0;                                                                             \
        out_ptr = NULL;                                                                          \
        if (node_iptr == NULL)                                                                   \
        {                                                                                        \
            mpl_err = -1;                                                                        \
            break;                                                                               \
        }                                                                                        \
        while (cur_node)                                                                         \
        {                                                                                        \
            if (mpl_gavl_tree_iptr->gavl_cmp_func(node_iptr, cur_node) == 0)                     \
            {                                                                                    \
                out_ptr = cur_node;                                                              \
                break;                                                                           \
            }                                                                                    \
            else if (mpl_gavl_tree_iptr->gavl_cmp_func(node_iptr, cur_node) < 0)                 \
                cur_node = (NodeType *)cur_node->left;                                           \
            else                                                                                 \
                cur_node = (NodeType *)cur_node->right;                                          \
        }                                                                                        \
    } while (0)

#define MPL_gavl_tree_finalize(NodeType, gavl_tree_hd, mpl_err)                                  \
    do                                                                                           \
    {                                                                                            \
        MPL_define_gavl_tree_type(NodeType);                                                     \
        MPL_gavl_tree_internal_t *mpl_gavl_tree_iptr = (MPL_gavl_tree_internal_t *)gavl_tree_hd; \
        NodeType *cur_node;                                                                      \
        void *dnode = NULL;                                                                      \
        mpl_err = 0;                                                                             \
        if (mpl_gavl_tree_iptr == NULL)                                                          \
        {                                                                                        \
            mpl_err = -1;                                                                        \
            break;                                                                               \
        }                                                                                        \
        cur_node = mpl_gavl_tree_iptr->root;                                                     \
        while (cur_node)                                                                         \
        {                                                                                        \
            if (cur_node->left)                                                                  \
                cur_node = (NodeType *)cur_node->left;                                           \
            else if (cur_node->right)                                                            \
                cur_node = (NodeType *)cur_node->right;                                          \
            else                                                                                 \
            {                                                                                    \
                dnode = (void *)cur_node;                                                        \
                cur_node = (NodeType *)cur_node->parent;                                         \
                if (cur_node)                                                                    \
                {                                                                                \
                    if (cur_node->left == dnode)                                                 \
                        cur_node->left = NULL;                                                   \
                    else                                                                         \
                        cur_node->right = NULL;                                                  \
                }                                                                                \
                if (mpl_gavl_tree_iptr->gavl_free_func)                                          \
                    mpl_gavl_tree_iptr->gavl_free_func(dnode);                                   \
            }                                                                                    \
        }                                                                                        \
        MPL_free(mpl_gavl_tree_iptr);                                                            \
        gavl_tree_hd = (MPL_gavl_tree_hd_t)0;                                                    \
    } while (0)

#endif /* GAVL_H_INCLUDED */

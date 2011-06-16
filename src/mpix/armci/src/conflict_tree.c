/** Copyright (C) 2010. See COPYRIGHT in top-level directory.
  *
  * Conflict Tree -- James Dinan <dinan@mcs.anl.gov>
  *
  * Conflict trees are used by ARMCI-MPI to detect conflicting accesses due to
  * overlapping memory regions.
  *
  * This implementation uses an AVL tree which is a self-balancing binary tree.
  * In contrast with interval and segment trees which can store mutiple
  * overlapping regions and support stabbing queries, the conflict tree does
  * not allow any overlap among elements in the tree.  If an overlapping insert
  * is performed, it will fail.  Thus, objects in the tree are totally ordered
  * and a standard binary tree (in this case, the AVL tree) is sufficient for
  * detecting conflicts.
  */

#include <stdio.h>
#include <stdlib.h>

#include <debug.h>
#include <conflict_tree.h>

#define MAX(A,B) (((A) > (B)) ? A : B)

static ctree_t ctree_balance(ctree_t node);
static void ctree_destroy_rec(ctree_t root);
static inline int ctree_node_height(ctree_t node);
static inline void ctree_rotate_left(ctree_t node);
static inline void ctree_rotate_right(ctree_t node);

const ctree_t CTREE_EMPTY = NULL;


/** Locate the node that conflicts with the given address range.
  *
  * @param[in] root Root of the ctree.
  * @param[in] lo   Low end of the range.
  * @param[in] hi   High end of the range.
  * @return         Pointer to the ctree node or NULL if not found.
  */
ctree_t ctree_locate(ctree_t root, uint8_t *lo, uint8_t *hi) {
  ctree_t cur = root;

  while (cur != NULL) {
    if (   (lo >= cur->lo && lo <= cur->hi)
        || (hi >= cur->lo && hi <= cur->hi)
        || (lo <  cur->lo && hi >  cur->hi))
      break;

    else if (lo < cur->lo)
      cur = cur->left;
   
    else /* lo > cur->hi */
      cur = cur->right;
  }

  return cur;
}


/** Insert an address range into the conflict detection tree.
  *
  * @param[inout] root The root of the tree
  * @param[in]    lo   Lower bound of the address range
  * @param[in]    hi   Upper bound of the address range
  * @return            Zero on success, nonzero when a conflict is detected.
  *                    When a conflict exists, the range is not added.
  */
int ctree_insert(ctree_t *root, uint8_t *lo, uint8_t *hi) {
  ctree_t cur;
  ctree_t new_node = (ctree_t) malloc(sizeof(struct ctree_node_s));

  new_node->lo     = lo;
  new_node->hi     = hi;
  new_node->height = 1;
  new_node->parent = NULL;
  new_node->left   = NULL;
  new_node->right  = NULL;

  cur = *root;

  // CASE: Empty tree
  if (cur == NULL) {
    *root = new_node;
    return 0;
  }

  for (;;) {

    // Check for conflicts as we go
    if (   (lo >= cur->lo && lo <= cur->hi)
        || (hi >= cur->lo && hi <= cur->hi)
        || (lo <  cur->lo && hi >  cur->hi)) {
      ARMCII_Dbg_print(DEBUG_CAT_CTREE, "Conflict inserting [%p, %p] with [%p, %p]\n", lo, hi, cur->lo, cur->hi);
      free(new_node);
      return 1;
    }

    // Place in left subtree
    else if (lo < cur->lo) {
      if (cur->left == NULL) {
        new_node->parent = cur;
        cur->left = new_node;
        *root = ctree_balance(cur);
        return 0;
      } else {
        cur = cur->left;
      }
    }
   
    // Place in right subtree
    else /* lo > cur->hi */ {
      if (cur->right == NULL) {
        new_node->parent = cur;
        cur->right = new_node;
        *root = ctree_balance(cur);
        return 0;
      } else {
        cur = cur->right;
      }
    }
  }
}


/** Rotate the given pivot node to the left.
  *
  * @param[in] node This is the pivot node, it will be the new subtree root
  *                 after the rotation is performed.
  */
static inline void ctree_rotate_left(ctree_t node) {
  ctree_t old_root = node->parent;

  //ARMCII_Dbg_print(DEBUG_CAT_CTREE, "[%10p, %10p] l=%d r=%d\n", node->lo, node->hi, 
  //    ctree_node_height(node->left), ctree_node_height(node->right));

  ARMCII_Assert(old_root->right == node);

  // Set the parent pointer
  node->parent     = old_root->parent;

  // Set the parent's child pointer
  if (node->parent != NULL) {
    if (node->parent->left == old_root)
      node->parent->left = node;
    else
      node->parent->right = node;
  }

  // Set child pointers and their parents
  old_root->right         = node->left;
  if (old_root->right != NULL)
    old_root->right->parent = old_root;

  node->left              = old_root;
  node->left->parent      = node;

  old_root->height = MAX(ctree_node_height(old_root->left), ctree_node_height(old_root->right)) + 1;
  node->height     = MAX(ctree_node_height(node->left), ctree_node_height(node->right)) + 1;
}


/** Rotate the given pivot node to the right.
  *
  * @param[in] node This is the pivot node, it will be the new subtree root
  *                 after the rotation is performed.
  */
static inline void ctree_rotate_right(ctree_t node) {
  ctree_t old_root = node->parent;

  //ARMCII_Dbg_print(DEBUG_CAT_CTREE, "[%10p, %10p] l=%d r=%d\n", node->lo, node->hi, 
  //    ctree_node_height(node->left), ctree_node_height(node->right));

  ARMCII_Assert(old_root->left == node);

  // Set the parent pointer
  node->parent     = old_root->parent;

  // Set the parent's child pointer
  if (node->parent != NULL) {
    if (node->parent->left == old_root)
      node->parent->left = node;
    else
      node->parent->right = node;
  }

  // Set child pointers and their parents
  old_root->left  = node->right;
  if (old_root->left != NULL)
    old_root->left->parent = old_root;

  node->right         = old_root;
  node->right->parent = node;

  old_root->height = MAX(ctree_node_height(old_root->left), ctree_node_height(old_root->right)) + 1;
  node->height     = MAX(ctree_node_height(node->left), ctree_node_height(node->right)) + 1;
}


/** Fetch the height of a given node.
  */
static inline int ctree_node_height(ctree_t node) {
  if (node == NULL)
    return 0;
  else
    return node->height;
}


/** Rebalance the tree starting with the current node and proceeding
  * upwards towards the root.
  */
static ctree_t ctree_balance(ctree_t node) {
  ctree_t root = node;

  while (node != NULL) {
    int height_l = ctree_node_height(node->left);
    int height_r = ctree_node_height(node->right);

    node->height = MAX(ctree_node_height(node->left), ctree_node_height(node->right)) + 1;

    // Rebalance to preserve the property that right and left heights
    // differ by at most 1.
    if (abs(height_l - height_r) >= 2) {

      // CASE: Right is heavy
      if (height_l - height_r == -2) {
        int height_r_l = ctree_node_height(node->right->left);
        int height_r_r = ctree_node_height(node->right->right);

        // CASE: Right, right
        if (height_r_l - height_r_r <= 0) {
          node = node->right;
          ctree_rotate_left(node);
        }
        // CASE: Right, left
        else {
          ctree_rotate_right(node->right->left);
          node = node->right;
          ctree_rotate_left(node);
        }

      // CASE: Left is heavy
      } else if (height_l - height_r == 2) {
        int height_l_l = ctree_node_height(node->left->left);
        int height_l_r = ctree_node_height(node->left->right);

        // CASE: Left, left
        if (height_l_l - height_l_r >= 0) {
          node = node->left;
          ctree_rotate_right(node);
        }
        // CASE: Left, right
        else {
          ctree_rotate_left(node->left->right);
          node = node->left;
          ctree_rotate_right(node);
        }

      } else {
        ARMCII_Error("CTree invariant violated, height difference of %d is too large", height_l - height_r);
      }
    }

    root = node;
    node = node->parent;
  }

  return root;
}


/** Recursive function to traverse a tree and destroy all its nodes.
  */
static void ctree_destroy_rec(ctree_t root) {
  if (root == NULL) return;
  
  ctree_destroy_rec(root->left);
  ctree_destroy_rec(root->right);

  free(root);
}


/** Destroy a conflict tree.
  */
void ctree_destroy(ctree_t *root) {
  ctree_destroy_rec(*root);

  *root = NULL;
}


/** Print out the contents of the conflict detection tree.
  */
void ctree_print(ctree_t root) {
  if (root != NULL) {
    int  i,idx;
    char s[32] = "";

    ctree_print(root->left);

    for (i = 1, idx = 0; i < 32-1 && i < root->height; i++)
      idx += sprintf(s+idx, "\t");
    
    printf("%10p:%s[%p, %p] p=%p h=%d\n", (void*)root, s, root->lo, root->hi, (void*)(root->parent), root->height);

    ctree_print(root->right);
  }
}

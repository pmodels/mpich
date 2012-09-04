/*
 * Copyright (C) 2010. See COPYRIGHT in top-level directory.
 */

#ifndef _CONFLICT_TREE_H
#define _CONFLICT_TREE_H

#include <armci_internals.h>

struct ctree_node_s {
  uint8_t *lo;
  uint8_t *hi;

  int height;

  struct ctree_node_s *parent;
  struct ctree_node_s *left;
  struct ctree_node_s *right;
};

typedef struct ctree_node_s * ctree_t;

extern const ctree_t CTREE_EMPTY;

int     ctree_insert(ctree_t *root, uint8_t *lo, uint8_t *hi);
ctree_t ctree_locate(ctree_t root, uint8_t *lo, uint8_t *hi);
void    ctree_destroy(ctree_t *root);
void    ctree_print(ctree_t root);


#endif /* _CONFLICT_TREE_H */

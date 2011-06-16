/*
 * Copyright (C) 2010. See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <conflict_tree.h>

#define MIN(X,Y) ((X) < (Y) ? X : Y)
#define MAX(X,Y) ((X) > (Y) ? X : Y)

#define MAX_INTVL 1000
#define NELT      1000

uint8_t *data[NELT][2];

int main(int argc, char **argv) {
  int i, next, upper_bound;
  ctree_t ctree = CTREE_EMPTY;

  srand(time(NULL));

  // Generate random intervals that fully cover the space from [0,next)
  for (i = next = 0; i < NELT; i++, next++) {
    data[i][0]  = ((uint8_t*) NULL) + next;
    next        = next + rand()%MAX_INTVL;
    data[i][1]  = ((uint8_t*) NULL) + next;
    upper_bound = next;
  }

  // Perform NELT random swaps so elements are inserted in random order
  for (i = 0; i < NELT; i++) {
    int j = rand() % NELT;
    int k = rand() % NELT;
    uint8_t *tmp[2];

    tmp[0]  = data[j][0];
    tmp[1]  = data[j][1];

    data[j][0] = data[k][0];
    data[j][1] = data[k][1];

    data[k][0] = tmp[0];
    data[k][1] = tmp[1];
  }

  // Build the conflict tree
  for (i = 0; i < NELT; i++) {
    printf(" + Inserting [%p, %p]\n", data[i][0], data[i][1]);
    int conflict = ctree_insert(&ctree, data[i][0], data[i][1]);

    if (conflict) {
      printf("*** Error, conflict inserting [%p, %p]\n", data[i][0], data[i][1]);
      ctree_print(ctree);
      exit(1);
    }
  }

  printf("\n");
  ctree_print(ctree);
  printf("\n");

  // Generate random test samples
  for (i = 0; i < NELT; i++) {
    int x = rand() % upper_bound;
    int y = rand() % upper_bound;

    data[i][0]  = ((uint8_t*) NULL) + MIN(x,y);
    data[i][1]  = ((uint8_t*) NULL) + MAX(x,y);
  }

  for (i = 0; i < NELT; i++) {
    printf(" + Checking [%p, %p]\n", data[i][0], data[i][1]);
    int conflict = ctree_insert(&ctree, data[i][0], data[i][1]);

    if (!conflict) {
      printf("*** Error, no conflict inserting [%p, %p]\n", data[i][0], data[i][1]);
      ctree_print(ctree);
      exit(1);
    }
  }

  ctree_destroy(&ctree);

  return 0;
}

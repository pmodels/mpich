/*
 * Copyright (C) 2010. See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <conflict_tree.h>

#define NELT 1000

uint8_t *data[NELT];

int main(int argc, char **argv) {
  int i;
  ctree_t ctree = CTREE_EMPTY;

  srand(time(NULL));

  for (i = 0; i < NELT; i++)
    data[i] = ((uint8_t*) NULL) + i;

  // Perform NELT random swaps
  for (i = 0; i < NELT; i++) {
    int j = rand() % NELT;
    int k = rand() % NELT;
    uint8_t *tmp = data[j];
    data[j] = data[k];
    data[k] = tmp;
  }

  for (i = 0; i < NELT; i++) {
    printf(" + Inserting [%p, %p]\n", data[i], data[i]);
    int conflict = ctree_insert(&ctree, data[i], data[i]);

    if (conflict) {
      printf("*** Error, conflict inserting %p\n", data[i]);
      ctree_print(ctree);
      exit(1);
    }
  }

  printf("\n");
  ctree_print(ctree);
  printf("\n");

  for (i = 0; i < NELT; i++) {
    printf(" + Checking [%p, %p]\n", data[i], data[i]);
    int conflict = ctree_insert(&ctree, data[i], data[i]);

    if (!conflict) {
      printf("*** Error, no conflict inserting %p\n", data[i]);
      ctree_print(ctree);
      exit(1);
    }
  }

  ctree_destroy(&ctree);

  return 0;
}

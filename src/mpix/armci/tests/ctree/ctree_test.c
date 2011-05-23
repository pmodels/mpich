/*
 * Copyright (C) 2010. See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <conflict_tree.h>

#define MAX 20
#define INC 4

int main(int argc, char **argv) {
  uint8_t *i;
  ctree_t ctree = CTREE_EMPTY;

  printf("========== FORWARD INSERT CHECK ==========\n");

  for (i = 0; i <= (uint8_t*) MAX; i+=INC) {
    printf("----- Inserting [%10p, %10p] -----\n", i, i+INC-1);
    int conflict = ctree_insert(&ctree, i, i+INC-1);
    ctree_print(ctree);

    if (conflict)
      printf("Error, conflict inserting %p\n", i);
  }

  for (i = 0; i <= (uint8_t*) MAX; i+=INC) {
    printf("----- Checking [%10p, %10p] -----\n", i, i+INC-1);
    int conflict = ctree_insert(&ctree, i, i+INC-1);

    if (!conflict)
      printf("Error, no conflict inserting %p\n", i);
  }

  ctree_destroy(&ctree);

  printf("========== REVERSE INSERT CHECK ==========\n");

  for (i = (uint8_t*) MAX+INC; i-INC <= (uint8_t*) MAX+INC; i-=INC) {
    printf("----- Inserting [%10p, %10p] -----\n", i-INC, i-1);
    int conflict = ctree_insert(&ctree, i-INC, i-1);
    ctree_print(ctree);

    if (conflict)
      printf("Error, conflict inserting %p\n", i);
  }

  for (i = 0; i <= (uint8_t*) MAX; i+=INC) {
    printf("----- Checking [%10p, %10p] -----\n", i, i+INC-1);
    int conflict = ctree_insert(&ctree, i, i+INC-1);

    if (!conflict)
      printf("Error, no conflict inserting %p\n", i);
  }

  ctree_destroy(&ctree);

  return 0;
}

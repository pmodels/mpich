/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#define ROWS (8)
#define COLS (ROWS)
#define SIZE (ROWS * ROWS)

void print_matrix(int *matrix, int rows, int cols, const char *desc);
void init_matrix(int *matrix, int rows, int cols);
void set_matrix(int *matrix, int rows, int cols, int val);

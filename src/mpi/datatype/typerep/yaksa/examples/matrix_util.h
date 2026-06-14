/*
 * Copyright 2026 Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
 */

#define ROWS (8)
#define COLS (ROWS)
#define SIZE (ROWS * ROWS)

void print_matrix(int *matrix, int rows, int cols, const char *desc);
void init_matrix(int *matrix, int rows, int cols);
void set_matrix(int *matrix, int rows, int cols, int val);

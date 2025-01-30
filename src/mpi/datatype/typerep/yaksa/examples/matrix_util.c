/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <stdio.h>
#include "matrix_util.h"

void print_matrix(int *matrix, int rows, int cols, const char *desc)
{
    int digits = 0;
    int max = (rows * cols) - 1;

    while (max > 0) {
        max /= 10;
        digits++;
    }

    fprintf(stdout, "%s\n", desc);
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            fprintf(stdout, "%.*d ", digits, matrix[(i * cols) + j]);
        }
        fprintf(stdout, "\n");
    }
    fprintf(stdout, "\n");
}

void init_matrix(int *matrix, int rows, int cols)
{
    for (int i = 0; i < rows; i++)
        for (int j = 0; j < cols; j++)
            matrix[(i * cols) + j] = (int) (i * cols) + j;
}

void set_matrix(int *matrix, int rows, int cols, int val)
{
    for (int i = 0; i < rows; i++)
        for (int j = 0; j < cols; j++)
            matrix[(i * cols) + j] = (int) val;
}

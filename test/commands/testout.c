/*
 * Copyright 2026 Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

int main()
{
    setvbuf(stdout, NULL, _IOLBF, 0);
    printf("first line\n");
    sleep(1);
    printf("second line\n");
    sleep(1);
    printf("last line\n");
    fflush(stdout);
    return 0;
}

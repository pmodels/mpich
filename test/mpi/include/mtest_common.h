/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MTEST_COMMON_H_INCLUDED
#define MTEST_COMMON_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

MPI_Aint MTestDefaultMaxBufferSize();

typedef void MTestArgList;
MTestArgList *MTestArgListCreate(int argc, char *argv[]);
char *MTestArgListGetString(MTestArgList * head, const char *arg);
int MTestArgListGetInt(MTestArgList * head, const char *arg);
long MTestArgListGetLong(MTestArgList * head, const char *arg);
void MTestArgListDestroy(MTestArgList * head);

#endif

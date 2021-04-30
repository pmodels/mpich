/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MTEST_COMMON_H_INCLUDED
#define MTEST_COMMON_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef enum {
    MTEST_MEM_TYPE__UNSET,
    MTEST_MEM_TYPE__UNREGISTERED_HOST,
    MTEST_MEM_TYPE__REGISTERED_HOST,
    MTEST_MEM_TYPE__DEVICE,
    MTEST_MEM_TYPE__SHARED,
} mtest_mem_type_e;

MPI_Aint MTestDefaultMaxBufferSize();

typedef void MTestArgList;
MTestArgList *MTestArgListCreate(int argc, char *argv[]);
char *MTestArgListGetString(MTestArgList * head, const char *arg);
int MTestArgListGetInt(MTestArgList * head, const char *arg);
long MTestArgListGetLong(MTestArgList * head, const char *arg);
const char *MTestArgListGetString_with_default(MTestArgList * head, const char *arg,
                                               const char *default_str);
int MTestArgListGetInt_with_default(MTestArgList * head, const char *arg, int default_val);
long MTestArgListGetLong_with_default(MTestArgList * head, const char *arg, long default_val);
mtest_mem_type_e MTestArgListGetMemType(MTestArgList * head, const char *arg);
void MTestArgListDestroy(MTestArgList * head);

void MTestAlloc(size_t size, mtest_mem_type_e type, void **hostbuf, void **devicebuf,
                bool is_calloc);
void MTestFree(mtest_mem_type_e type, void *hostbuf, void *devicebuf);
void MTestCopyContent(const void *sbuf, void *dbuf, size_t size, mtest_mem_type_e type);
void MTest_finalize_gpu();
#endif

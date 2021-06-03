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
    MTEST_MEM_TYPE__RANDOM,
    MTEST_MEM_TYPE__ALL,
} mtest_mem_type_e;

MPI_Aint MTestDefaultMaxBufferSize(void);

typedef void MTestArgList;
MTestArgList *MTestArgListCreate(int argc, char *argv[]);
char *MTestArgListSearch(MTestArgList * head, const char *arg);
char *MTestArgListGetString(MTestArgList * head, const char *arg);
int MTestArgListGetInt(MTestArgList * head, const char *arg);
long MTestArgListGetLong(MTestArgList * head, const char *arg);
const char *MTestArgListGetString_with_default(MTestArgList * head, const char *arg,
                                               const char *default_str);
int MTestArgListGetInt_with_default(MTestArgList * head, const char *arg, int default_val);
long MTestArgListGetLong_with_default(MTestArgList * head, const char *arg, long default_val);
mtest_mem_type_e MTestArgListGetMemType(MTestArgList * head, const char *arg);
void MTestArgListDestroy(MTestArgList * head);

mtest_mem_type_e MTest_memtype_random(void);
const char *MTest_memtype_name(mtest_mem_type_e memtype);
int *MTestParseIntList(const char *str, int *num);
char **MTestParseStringList(const char *str, int *num);
void MTestFreeStringList(char **str, int num);

#define MTEST_CREATE_AND_FREE_DTP_OBJS(dtp_, testsize_) ({              \
    int errs_ = 0;                                                      \
    MPI_Aint maxbufsize = MTestDefaultMaxBufferSize();                  \
    do {                                                                \
        int i_, err_;                                                   \
        DTP_obj_s obj_;                                                 \
        for(i_ = 0; i_ < testsize_; i_++) {                             \
            err_ = DTP_obj_create(dtp_, &obj_, maxbufsize);             \
            if (err_ != DTP_SUCCESS) {                                  \
                errs_++;                                                \
                break;                                                  \
            }                                                           \
            DTP_obj_free(obj_);                                         \
        }                                                               \
    } while (0);                                                        \
    errs_; })

#define MTestMalloc(size, type, hostbuf, devicebuf, device) MTestAlloc(size, type, hostbuf, devicebuf, 0, device)
#define MTestCalloc(size, type, hostbuf, devicebuf, device) MTestAlloc(size, type, hostbuf, devicebuf, 1, device)
void MTestAlloc(size_t size, mtest_mem_type_e type, void **hostbuf, void **devicebuf,
                bool is_calloc, int device);
void MTestFree(mtest_mem_type_e type, void *hostbuf, void *devicebuf);
void MTestCopyContent(const void *sbuf, void *dbuf, size_t size, mtest_mem_type_e type);
void MTest_finalize_gpu(void);
#endif

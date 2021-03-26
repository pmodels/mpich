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

MPI_Aint MTestDefaultMaxBufferSize();

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

#define MTEST_DTP_DECLARE(name) \
    mtest_mem_type_e name ## mem; \
    void *name ## buf; \
    void *name ## buf_h; \
    DTP_obj_s name ## _obj

#define MTest_dtp_malloc_max(name, device_id) \
    do { \
        MTestMalloc(maxbufsize, name ## mem, &name ## buf_h, &name ## buf, device_id); \
        assert(name ## buf && name ## buf_h); \
    } while (0)

#define MTest_dtp_malloc_obj(name, device_id) \
    do { \
        MTestMalloc(name ## _obj.DTP_bufsize, name ## mem, &name ## buf_h, &name ## buf, device_id); \
        assert(name ## buf && name ## buf_h); \
    } while (0)

#define MTest_dtp_free(name) \
    do { \
        MTestFree(name ## mem, name ## buf_h, name ## buf); \
    } while (0)

#define MTest_dtp_init(name, start, stride, count) \
    do { \
        err = DTP_obj_buf_init(name ## _obj, name ## buf_h, start, stride, count); \
        if (err != DTP_SUCCESS) { \
            printf("DTP_obj_buf_init " #name " failed.\n"); \
            errs++; \
        } \
        MTestCopyContent(name ## buf_h, name ## buf, name ## _obj.DTP_bufsize, name ## mem); \
    } while (0)

#define MTest_dtp_check(name, start, stride, count) \
    do { \
        MTestCopyContent(name ## buf, name ## buf_h, name ## _obj.DTP_bufsize, name ## mem); \
        err = DTP_obj_buf_check(name ## _obj, name ## buf_h, start, stride, count); \
        if (err != DTP_SUCCESS) { \
            printf("DTP_obj_buf_check " #name " failed.\n"); \
            errs++; \
        } \
    } while (0)

#define MTestMalloc(size, type, hostbuf, devicebuf, device) MTestAlloc(size, type, hostbuf, devicebuf, 0, device)
#define MTestCalloc(size, type, hostbuf, devicebuf, device) MTestAlloc(size, type, hostbuf, devicebuf, 1, device)
void MTestAlloc(size_t size, mtest_mem_type_e type, void **hostbuf, void **devicebuf,
                bool is_calloc, int device);
void MTestFree(mtest_mem_type_e type, void *hostbuf, void *devicebuf);
void MTestCopyContent(const void *sbuf, void *dbuf, size_t size, mtest_mem_type_e type);
void MTest_finalize_gpu();
#endif

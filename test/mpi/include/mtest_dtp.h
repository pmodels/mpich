/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MTEST_DTP_H_INCLUDED
#define MTEST_DTP_H_INCLUDED

#include "dtpools.h"
#include "mtest_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

typedef enum {
    MTEST_DTP_ONCE,
    MTEST_DTP_PT2PT,
    MTEST_DTP_COLL,
    MTEST_DTP_RMA,
    MTEST_DTP_GACC,
    MTEST_COLL_NOCOUNT,
    MTEST_COLL_COUNT,
} mtest_dtp_arg_type_e;

struct dtp_args {
    mtest_dtp_arg_type_e dtp_arg_type;
    char *basic_type;
    int count;
    int seed;
    int testsize;
    union {
        struct {
            int recvcnt;
            mtest_mem_type_e sendmem;
            mtest_mem_type_e recvmem;
        } pt2pt;
        struct {
            mtest_mem_type_e oddmem;
            mtest_mem_type_e evenmem;
        } coll;
        struct {
            mtest_mem_type_e origmem;
            mtest_mem_type_e targetmem;
            mtest_mem_type_e resultmem;
        } rma;
    } u;

    /* argument used to generate multiple test instances */
    mtest_mem_type_e memtype;
    int min_testsize;
    int max_testsize;
    int *counts;
    int num_counts;
    int num_total_tests;
    char **typelist;
    int num_types;
    int idx;
    /* For each configuration, repeat num_repeats times (for more random instances) */
    int num_repeats;
};

/* internal static */

static mtest_mem_type_e allmemtypes[] = {
    MTEST_MEM_TYPE__UNREGISTERED_HOST,
    MTEST_MEM_TYPE__REGISTERED_HOST,
    MTEST_MEM_TYPE__DEVICE
};

static int num_allmemtypes = 3;

static void split_testsizes(const char *str, int *min_size, int *max_size)
{
    const char *s = str;
    *min_size = atoi(s);
    while (*s && *s != ',')
        s++;
    if (*s == ',') {
        *max_size = atoi(s + 1);
    } else {
        *max_size = *min_size;
    }
}

static void set_testsize(struct dtp_args *dtp_args)
{
    dtp_args->testsize = 64000 / dtp_args->count + 1;
    if (dtp_args->testsize > dtp_args->max_testsize) {
        dtp_args->testsize = dtp_args->max_testsize;
    }
    if (dtp_args->testsize < dtp_args->min_testsize) {
        dtp_args->testsize = dtp_args->min_testsize;
    }
}

static bool set_count_and_type(struct dtp_args *dtp_args)
{
    int idx = dtp_args->idx / dtp_args->num_repeats;
    int idx_extra = 0;
    if (dtp_args->dtp_arg_type == MTEST_DTP_PT2PT) {
        idx_extra = idx % 2;
        idx /= 2;
    }

    dtp_args->count = dtp_args->counts[idx % dtp_args->num_counts];
    if (dtp_args->dtp_arg_type == MTEST_DTP_PT2PT) {
        dtp_args->u.pt2pt.recvcnt = dtp_args->count;
        if (idx_extra) {
            dtp_args->u.pt2pt.recvcnt *= 2;
        }
    }
    idx /= dtp_args->num_counts;

    if (idx < dtp_args->num_types) {
        dtp_args->basic_type = dtp_args->typelist[idx];
        return true;
    } else {
        return false;
    }
}

static void set_memtypes(struct dtp_args *dtp_args)
{
    if (dtp_args->memtype == MTEST_MEM_TYPE__RANDOM) {
        if (dtp_args->dtp_arg_type == MTEST_DTP_PT2PT) {
            while (1) {
                dtp_args->u.pt2pt.sendmem = MTest_memtype_random();
                dtp_args->u.pt2pt.recvmem = MTest_memtype_random();
                if (dtp_args->u.pt2pt.sendmem == MTEST_MEM_TYPE__DEVICE ||
                    dtp_args->u.pt2pt.recvmem == MTEST_MEM_TYPE__DEVICE) {
                    break;
                }
            }
        } else if (dtp_args->dtp_arg_type == MTEST_DTP_COLL) {
            while (1) {
                dtp_args->u.coll.oddmem = MTest_memtype_random();
                dtp_args->u.coll.evenmem = MTest_memtype_random();
                if (dtp_args->u.coll.oddmem == MTEST_MEM_TYPE__DEVICE ||
                    dtp_args->u.coll.evenmem == MTEST_MEM_TYPE__DEVICE) {
                    break;
                }
            }
        } else if (dtp_args->dtp_arg_type == MTEST_DTP_RMA) {
            while (1) {
                dtp_args->u.rma.origmem = MTest_memtype_random();
                dtp_args->u.rma.targetmem = MTest_memtype_random();
                if (dtp_args->u.rma.origmem == MTEST_MEM_TYPE__DEVICE ||
                    dtp_args->u.rma.targetmem == MTEST_MEM_TYPE__DEVICE) {
                    break;
                }
            }
        } else if (dtp_args->dtp_arg_type == MTEST_DTP_GACC) {
            while (1) {
                dtp_args->u.rma.origmem = MTest_memtype_random();
                dtp_args->u.rma.targetmem = MTest_memtype_random();
                dtp_args->u.rma.resultmem = MTest_memtype_random();
                if (dtp_args->u.rma.origmem == MTEST_MEM_TYPE__DEVICE ||
                    dtp_args->u.rma.targetmem == MTEST_MEM_TYPE__DEVICE ||
                    dtp_args->u.rma.resultmem == MTEST_MEM_TYPE__DEVICE) {
                    break;
                }
            }
        }
    }
}

/* external static */

static void dtp_args_init(struct dtp_args *dtp_args, mtest_dtp_arg_type_e dtp_arg_type,
                          int argc, char *argv[])
{
    MTestArgList *head = MTestArgListCreate(argc, argv);

    dtp_args->dtp_arg_type = dtp_arg_type;
    dtp_args->idx = 0;
    dtp_args->num_counts = 0;
    dtp_args->counts = NULL;
    dtp_args->num_types = 0;
    dtp_args->typelist = NULL;
    dtp_args->basic_type = NULL;

    char *counts_str = MTestArgListSearch(head, "counts");
    dtp_args->memtype = MTestArgListGetMemType(head, "memtype");
    if (dtp_arg_type == MTEST_COLL_COUNT || dtp_arg_type == MTEST_COLL_NOCOUNT) {
        if (dtp_args->memtype != MTEST_MEM_TYPE__UNREGISTERED_HOST &&
            dtp_args->memtype != MTEST_MEM_TYPE__ALL) {
            fprintf(stderr, "Error: argument -memtype= has to be host or all\n");
            exit(-1);
        }
        dtp_args->u.coll.oddmem = MTestArgListGetMemType(head, "oddmemtype");
        dtp_args->u.coll.evenmem = MTestArgListGetMemType(head, "evenmemtype");
        if (counts_str || dtp_args->memtype == MTEST_MEM_TYPE__ALL) {
            dtp_args->num_total_tests = 1;
            if (counts_str) {
                dtp_args->counts = MTestParseIntList(counts_str, &dtp_args->num_counts);
                dtp_args->num_total_tests *= dtp_args->num_counts;
            }
            if (dtp_args->memtype == MTEST_MEM_TYPE__ALL) {
                dtp_args->num_total_tests *= num_allmemtypes;
            }
        } else {
            dtp_args->dtp_arg_type = MTEST_DTP_ONCE;
            dtp_args->num_total_tests = 1;
            if (dtp_arg_type == MTEST_COLL_COUNT) {
                dtp_args->count = MTestArgListGetLong(head, "count");
            }
        }
    } else if (counts_str) {
        const char *s_tmp;

        if (dtp_args->memtype != MTEST_MEM_TYPE__UNREGISTERED_HOST &&
            dtp_args->memtype != MTEST_MEM_TYPE__RANDOM) {
            fprintf(stderr, "Error: argument -memtype= has to be host or random\n");
            exit(-1);
        }

        dtp_args->seed = MTestArgListGetInt(head, "seed");

        s_tmp = MTestArgListGetString(head, "testsizes");
        split_testsizes(s_tmp, &dtp_args->min_testsize, &dtp_args->max_testsize);

        dtp_args->counts = MTestParseIntList(counts_str, &dtp_args->num_counts);

        s_tmp = MTestArgListGetString(head, "typelist");
        dtp_args->typelist = MTestParseStringList(s_tmp, &dtp_args->num_types);

        /* allow direct memtypes when "memtype" is "host" */
        if (dtp_arg_type == MTEST_DTP_PT2PT) {
            dtp_args->u.pt2pt.sendmem = MTestArgListGetMemType(head, "sendmem");
            dtp_args->u.pt2pt.recvmem = MTestArgListGetMemType(head, "recvmem");
            dtp_args->num_total_tests = dtp_args->num_types * dtp_args->num_counts * 2;
        } else if (dtp_arg_type == MTEST_DTP_COLL) {
            dtp_args->u.coll.oddmem = MTestArgListGetMemType(head, "oddmemtype");
            dtp_args->u.coll.evenmem = MTestArgListGetMemType(head, "evenmemtype");
            dtp_args->num_total_tests = dtp_args->num_types * dtp_args->num_counts;
        } else if (dtp_arg_type == MTEST_DTP_RMA) {
            dtp_args->u.rma.origmem = MTestArgListGetMemType(head, "origmem");
            dtp_args->u.rma.targetmem = MTestArgListGetMemType(head, "targetmem");
            dtp_args->num_total_tests = dtp_args->num_types * dtp_args->num_counts;
        } else if (dtp_arg_type == MTEST_DTP_GACC) {
            dtp_args->u.rma.origmem = MTestArgListGetMemType(head, "origmem");
            dtp_args->u.rma.targetmem = MTestArgListGetMemType(head, "targetmem");
            dtp_args->u.rma.resultmem = MTestArgListGetMemType(head, "resultmem");
            dtp_args->num_total_tests = dtp_args->num_types * dtp_args->num_counts;
        }
    } else {
        dtp_args->seed = MTestArgListGetInt(head, "seed");
        dtp_args->testsize = MTestArgListGetInt(head, "testsize");
        dtp_args->basic_type = strdup(MTestArgListGetString(head, "type"));
        if (dtp_arg_type == MTEST_DTP_PT2PT) {
            dtp_args->count = MTestArgListGetLong(head, "sendcnt");
            dtp_args->u.pt2pt.recvcnt = MTestArgListGetLong(head, "recvcnt");
            dtp_args->u.pt2pt.sendmem = MTestArgListGetMemType(head, "sendmem");
            dtp_args->u.pt2pt.recvmem = MTestArgListGetMemType(head, "recvmem");
        } else if (dtp_arg_type == MTEST_DTP_COLL) {
            dtp_args->count = MTestArgListGetLong(head, "count");
            dtp_args->u.coll.oddmem = MTestArgListGetMemType(head, "oddmemtype");
            dtp_args->u.coll.evenmem = MTestArgListGetMemType(head, "evenmemtype");
        } else if (dtp_arg_type == MTEST_DTP_RMA) {
            dtp_args->count = MTestArgListGetLong(head, "count");
            dtp_args->u.rma.origmem = MTestArgListGetMemType(head, "origmem");
            dtp_args->u.rma.targetmem = MTestArgListGetMemType(head, "targetmem");
        } else if (dtp_arg_type == MTEST_DTP_GACC) {
            dtp_args->count = MTestArgListGetLong(head, "count");
            dtp_args->u.rma.origmem = MTestArgListGetMemType(head, "origmem");
            dtp_args->u.rma.targetmem = MTestArgListGetMemType(head, "targetmem");
            dtp_args->u.rma.resultmem = MTestArgListGetMemType(head, "resultmem");
        }
        dtp_args->dtp_arg_type = MTEST_DTP_ONCE;
        dtp_args->num_total_tests = 1;
    }
    dtp_args->num_repeats = MTestArgListGetInt_with_default(head, "repeat", 1);
    dtp_args->num_total_tests *= dtp_args->num_repeats;

    MTestArgListDestroy(head);
}

static void dtp_args_finalize(struct dtp_args *dtp_args)
{
    if (dtp_args->dtp_arg_type == MTEST_DTP_ONCE) {
        if (dtp_args->basic_type) {
            free(dtp_args->basic_type);
        }
    } else {
        if (dtp_args->counts) {
            free(dtp_args->counts);
        }

        if (dtp_args->typelist) {
            MTestFreeStringList(dtp_args->typelist, dtp_args->num_types);
        }
    }
}

static bool dtp_args_get_next(struct dtp_args *dtp_args)
{
    if (dtp_args->idx >= dtp_args->num_total_tests) {
        return false;
    }

    if (dtp_args->dtp_arg_type == MTEST_DTP_ONCE) {
        dtp_args->idx++;
    } else if (dtp_args->dtp_arg_type == MTEST_COLL_NOCOUNT) {
        dtp_args->u.coll.evenmem = allmemtypes[dtp_args->idx];
        dtp_args->u.coll.oddmem = MTEST_MEM_TYPE__DEVICE;
        dtp_args->idx++;
    } else if (dtp_args->dtp_arg_type == MTEST_COLL_COUNT) {
        int idx = dtp_args->idx;
        dtp_args->count = dtp_args->counts[idx / num_allmemtypes];
        dtp_args->u.coll.evenmem = allmemtypes[idx % num_allmemtypes];
        dtp_args->u.coll.oddmem = MTEST_MEM_TYPE__DEVICE;
        dtp_args->idx++;
    } else {
        set_count_and_type(dtp_args);
        set_testsize(dtp_args);
        set_memtypes(dtp_args);
        dtp_args->seed++;
        dtp_args->idx++;
    }
    return true;
}

/* Wrap dtp object functions for isolating memtype support */

typedef enum {
    MTEST_DTP_BUF_UNALLOCATED,
    MTEST_DTP_BUF_MAX,          /* buffer need be separately allocated and freed */
    MTEST_DTP_BUF_OBJ,          /* buffer and dtp_obj created and destroyed together */
} mtest_dtp_buf_mode_e;

struct mtest_obj {
    mtest_mem_type_e memtype;
    void *buf;
    void *buf_h;
    DTP_obj_s dtp_obj;
    const char *name;
    DTP_pool_s dtp;
    MPI_Aint maxbufsize;
    mtest_dtp_buf_mode_e buf_mode;
    int device_id;
};

/* MTest_dtp_obj_start/finish once for each mtest_obj */
static inline void MTest_dtp_obj_start(struct mtest_obj *obj, const char *name,
                                       DTP_pool_s dtp, mtest_mem_type_e memtype,
                                       int device_id, bool use_max_buffer)
{
    obj->name = name;
    obj->dtp = dtp;
    obj->memtype = memtype;
    obj->device_id = device_id;
    obj->maxbufsize = MTestDefaultMaxBufferSize();
    obj->buf = NULL;
    if (use_max_buffer) {
        MTestMalloc(obj->maxbufsize, obj->memtype, &obj->buf_h, &obj->buf, device_id);
        assert(obj->buf && obj->buf_h);
        obj->buf_mode = MTEST_DTP_BUF_MAX;
    } else {
        obj->buf_mode = MTEST_DTP_BUF_UNALLOCATED;
    }
}

static inline void MTest_dtp_obj_finish(struct mtest_obj *obj)
{
    if (obj->buf_mode == MTEST_DTP_BUF_MAX) {
        MTestFree(obj->memtype, obj->buf_h, obj->buf);
        obj->buf_mode = MTEST_DTP_BUF_UNALLOCATED;
    }
}

/* MTest_dtp_create/destroy for each instance of dtp type */
static inline int MTest_dtp_create(struct mtest_obj *obj, bool alloc)
{
    int err = DTP_obj_create(obj->dtp, &obj->dtp_obj, obj->maxbufsize);
    if (alloc && obj->buf_mode != MTEST_DTP_BUF_MAX) {
        MTestMalloc(obj->dtp_obj.DTP_bufsize, obj->memtype, &obj->buf_h, &obj->buf, obj->device_id);
        assert(obj->buf && obj->buf_h);
        obj->buf_mode = MTEST_DTP_BUF_OBJ;
    }
    return err;
}

static inline int MTest_dtp_create_custom(struct mtest_obj *obj, bool alloc, const char *desc)
{
    int err = DTP_obj_create_custom(obj->dtp, &obj->dtp_obj, desc);
    if (alloc && obj->buf_mode != MTEST_DTP_BUF_MAX) {
        MTestMalloc(obj->dtp_obj.DTP_bufsize, obj->memtype, &obj->buf_h, &obj->buf, obj->device_id);
        assert(obj->buf && obj->buf_h);
        obj->buf_mode = MTEST_DTP_BUF_OBJ;
    }
    return err;
}

static inline int MTest_dtp_destroy(struct mtest_obj *obj)
{
    int err = DTP_obj_free(obj->dtp_obj);
    if (obj->buf_mode == MTEST_DTP_BUF_OBJ) {
        MTestFree(obj->memtype, obj->buf_h, obj->buf);
        obj->buf = NULL;
        obj->buf_mode = MTEST_DTP_BUF_UNALLOCATED;
    }
    return err;
}

/* utilitis for each instance of dtp obj */
static inline void MTest_dtp_print_desc(struct mtest_obj *obj)
{
    printf("%s [%s]\n", obj->name, DTP_obj_get_description(obj->dtp_obj));
}

static inline int MTest_dtp_init(struct mtest_obj *obj, int start, int stride, int count)
{
    int err;
    err = DTP_obj_buf_init(obj->dtp_obj, obj->buf_h, start, stride, count);
    if (err != DTP_SUCCESS) {
        printf("DTP_obj_buf_init failed.\n");
        MTest_dtp_print_desc(obj);
        return 1;
    }
    MTestCopyContent(obj->buf_h, obj->buf, obj->dtp_obj.DTP_bufsize, obj->memtype);
    return 0;
}

static inline int MTest_dtp_check(struct mtest_obj *obj, int start, int stride, int count,
                                  struct mtest_obj *obj2, int verbose)
{
    int err;
    MTestCopyContent(obj->buf, obj->buf_h, obj->dtp_obj.DTP_bufsize, obj->memtype);
    err = DTP_obj_buf_check(obj->dtp_obj, obj->buf_h, start, stride, count);
    if (err != DTP_SUCCESS) {
        if (verbose) {
            printf("DTP_obj_buf_check failed.\n");
            MTest_dtp_print_desc(obj);
            if (obj2) {
                MTest_dtp_print_desc(obj2);
            }
        }
        return 1;
    }
    return 0;
}

#endif /* MTEST_DTP_H_INCLUDED */

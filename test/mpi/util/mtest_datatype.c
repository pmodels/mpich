/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2014 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mtest_datatype.h"
#if defined(HAVE_STDIO_H) || defined(STDC_HEADERS)
#include <stdio.h>
#endif
#if defined(HAVE_STDLIB_H) || defined(STDC_HEADERS)
#include <stdlib.h>
#endif
#if defined(HAVE_STRING_H) || defined(STDC_HEADERS)
#include <string.h>
#endif
#ifdef HAVE_STDARG_H
#include <stdarg.h>
#endif
/* The following two includes permit the collection of resource usage
   data in the tests
 */
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif
#include <errno.h>


/* ------------------------------------------------------------------------ */
/* Internal functions                                                       */
/* ------------------------------------------------------------------------ */

/**
 * Decode a datatype object, return its extent, total space, and total size.
 */
static inline void datatype_decode(MTestDatatype * mtype, MPI_Aint * extent,
                                   MPI_Aint * totspace, MPI_Aint * totsize)
{
    int dt_size;
    MPI_Aint lb;
    int merr = 0;

    merr = MPI_Type_get_extent(mtype->datatype, &lb, extent);
    if (merr)
        MTestPrintError(merr);

    merr = MPI_Type_size(mtype->datatype, &dt_size);
    if (merr)
        MTestPrintError(merr);

    (*totspace) = lb + mtype->count * (*extent);
    (*totsize) = mtype->count * dt_size;
}

/**
 * Initialize parameters to generate a dataset.
 *
 * It returns data generating parameters to the upper function to generate value
 * for each element. The current version generates an arithmetic sequence with
 * following two parameters, this might be changed in future.
 * - init_term, the value of the first element.
 * - common_diff, the multiply for the next element.
 *
 * Note that we generate totally different sequence for each dataset, thus the
 * test can specify different datasets for multiple buffers, and check the
 * correctness of every byte in the receiving buffer. For example, in get_acc
 * test, the origin buffer, result buffer, and window buffer can have different
 * value, thus both result buffer and window buffer can be checked in single
 * execution.
 */
static void init_dataset_params(MTestDataset dataset, MPI_Aint set_size,
                                MPI_Aint * init_term, MPI_Aint * common_diff)
{
    switch (dataset) {
    case MTEST_DATA_SET1:
        (*init_term) = 0;
        (*common_diff) = 1;
        break;
    case MTEST_DATA_SET2:
        (*init_term) = set_size;
        (*common_diff) = 1;
        break;
    case MTEST_DATA_EMPTY:
    default:
        (*init_term) = 0;
        (*common_diff) = 0;
        break;
    }
}

/* ------------------------------------------------------------------------ */
/* General datatype routines                                                */
/* ------------------------------------------------------------------------ */

static void *MTestTypeFree(MTestDatatype * mtype)
{
    if (mtype->buf)
        free(mtype->buf);

    /* Only free internal structures for created types. */
    if (!mtype->isBasic && !mtype->isDuped) {
        if (mtype->displs)
            free(mtype->displs);
        if (mtype->displ_in_bytes)
            free(mtype->displ_in_bytes);
        if (mtype->index)
            free(mtype->index);
        if (mtype->old_datatypes)
            free(mtype->old_datatypes);
    }

    mtype->buf = NULL;
    mtype->displs = NULL;
    mtype->displ_in_bytes = NULL;
    mtype->index = NULL;
    mtype->old_datatypes = NULL;

    return 0;
}

static inline void MTestTypeReset(MTestDatatype * mtype)
{
    mtype->isBasic = 0;
    mtype->isDuped = 0;
    mtype->printErrors = 0;
    mtype->buf = NULL;

    mtype->datatype = MPI_DATATYPE_NULL;
    mtype->count = 0;
    mtype->nblock = 0;
    mtype->index = NULL;

    mtype->stride = 0;
    mtype->blksize = 0;
    mtype->displ_in_bytes = NULL;

    mtype->displs = NULL;
    mtype->basesize = 0;

    mtype->old_datatypes = NULL;

    mtype->arr_sizes[0] = 0;
    mtype->arr_sizes[1] = 0;
    mtype->arr_subsizes[0] = 0;
    mtype->arr_subsizes[1] = 0;
    mtype->arr_starts[0] = 0;
    mtype->arr_starts[1] = 0;
    mtype->order = 0;

    mtype->InitBuf = NULL;
    mtype->FreeBuf = NULL;
    mtype->CheckBuf = NULL;
    mtype->Dup = NULL;
}

/*
 * Duplicate a MTest datatype object.
 *
 * Note that we only set all the structure pointers to the existing objects,
 * only the buffer is allocated separately.
 * */
static int MTestTypeDup(MTestDatatype old_mtype, MTestDatatype * new_mtype)
{
    int merr = 0;

    MTestTypeReset(new_mtype);

    new_mtype->isDuped = 1;

    /* Copy all internal structures from old type. */
    new_mtype->datatype = old_mtype.datatype;
    new_mtype->count = old_mtype.count;
    new_mtype->nblock = old_mtype.nblock;
    new_mtype->index = old_mtype.index;

    new_mtype->stride = old_mtype.stride;
    new_mtype->blksize = old_mtype.blksize;
    new_mtype->displ_in_bytes = old_mtype.displ_in_bytes;

    new_mtype->displs = old_mtype.displs;
    new_mtype->basesize = old_mtype.basesize;

    new_mtype->old_datatypes = old_mtype.old_datatypes;

    new_mtype->arr_sizes[0] = old_mtype.arr_sizes[0];
    new_mtype->arr_sizes[1] = old_mtype.arr_sizes[1];
    new_mtype->arr_subsizes[0] = old_mtype.arr_subsizes[0];
    new_mtype->arr_subsizes[1] = old_mtype.arr_subsizes[1];
    new_mtype->arr_starts[0] = old_mtype.arr_starts[0];
    new_mtype->arr_starts[1] = old_mtype.arr_starts[1];
    new_mtype->order = old_mtype.order;

    new_mtype->InitBuf = old_mtype.InitBuf;
    new_mtype->FreeBuf = old_mtype.FreeBuf;
    new_mtype->CheckBuf = old_mtype.CheckBuf;
    new_mtype->Dup = old_mtype.Dup;

    return merr;
}

/* ------------------------------------------------------------------------ */
/* Datatype routines for contiguous datatypes                               */
/* ------------------------------------------------------------------------ */
/*
 * Initialize buffer of basic datatype
 */
static void *MTestTypeContigInit(MTestDatatype * mtype, MTestDataset dataset)
{
    MPI_Aint totspace = 0, extent = 0;
    MPI_Aint init_term = 0, common_diff = 0, data = 0, totsize = 0;

    if (mtype->count > 0) {
        unsigned char *p;
        MPI_Aint i;

        /* Get datatype information and dataset */
        datatype_decode(mtype, &extent, &totspace, &totsize);
        init_dataset_params(dataset, totsize, &init_term, &common_diff);

        /* Allocate buffer */
        if (!mtype->buf) {
            mtype->buf = (void *) malloc(totspace);
        }
        p = (unsigned char *) (mtype->buf);
        if (!p) {
            char errmsg[128] = { 0 };
            sprintf(errmsg, "Out of memory in %s", __FUNCTION__);
            MTestError(errmsg);
        }

        /* Now, set value into actual elements. */
        data = init_term;
        for (i = 0; i < totspace; i++) {
            p[i] = (unsigned char) (0xff ^ ((data++ * common_diff) & 0xff));
        }
    }
    else {
        if (mtype->buf) {
            free(mtype->buf);
        }
        mtype->buf = 0;
    }
    return mtype->buf;
}

/*
 * Check value of received basic datatype buffer.
 */
static int MTestTypeContigCheckbuf(MTestDatatype * mtype, MTestDataset dataset)
{
    unsigned char *p;
    unsigned char expected;
    int err = 0;
    MPI_Aint i, totspace = 0, extent = 0;
    MPI_Aint init_term = 0, common_diff = 0, data = 0, totsize = 0;

    p = (unsigned char *) mtype->buf;
    if (p) {
        /* Get datatype information and dataset */
        datatype_decode(mtype, &extent, &totspace, &totsize);
        init_dataset_params(dataset, totsize, &init_term, &common_diff);

        data = init_term;
        for (i = 0; i < totspace; i++) {
            expected = (unsigned char) (0xff ^ ((data++ * common_diff) & 0xff));

            if (p[i] != expected) {
                err++;
                if (mtype->printErrors && err < 10) {
                    printf("Data expected = %x but got p[%ld] = %x\n", expected, i, p[i]);
                    fflush(stdout);
                }
            }
        }
    }
    return err;
}


/* ------------------------------------------------------------------------ */
/* Datatype routines for vector datatypes                                   */
/* ------------------------------------------------------------------------ */

/*
 * Initialize buffer of vector datatype
 */
static void *MTestTypeVectorInit(MTestDatatype * mtype, MTestDataset dataset)
{
    MPI_Aint totspace = 0, extent = 0, dt_offset, byte_offset;
    MPI_Aint init_term = 0, common_diff = 0, data = 0, totsize = 0;

    if (mtype->count > 0) {
        unsigned char *p;
        MPI_Aint k, j;
        int i;

        /* Get datatype information and dataset */
        datatype_decode(mtype, &extent, &totspace, &totsize);
        init_dataset_params(dataset, totsize, &init_term, &common_diff);

        /* Allocate buffer */
        if (!mtype->buf) {
            mtype->buf = (void *) malloc(totspace);
        }
        p = (unsigned char *) (mtype->buf);
        if (!p) {
            char errmsg[128] = { 0 };
            sprintf(errmsg, "Out of memory in %s", __FUNCTION__);
            MTestError(errmsg);
        }

        /* First, set to -1 */
        for (k = 0; k < totspace; k++)
            p[k] = 0xff;

        /* For empty initialization, it is done. */
        if (dataset == MTEST_DATA_EMPTY)
            return mtype->buf;

        /* Now, set value into actual elements.
         * We require that the base type is a contiguous type */
        data = init_term;
        dt_offset = 0;
        /* For each datatype */
        for (k = 0; k < mtype->count; k++) {
            /* For each block */
            for (i = 0; i < mtype->nblock; i++) {
                byte_offset = dt_offset + i * mtype->stride;
                /* For each byte */
                for (j = 0; j < mtype->blksize; j++) {
                    p[byte_offset + j] = (unsigned char) (0xff ^ ((data++ * common_diff) & 0xff));
                }
            }
            dt_offset += extent;
        }
    }
    else {
        mtype->buf = 0;
    }
    return mtype->buf;
}

/*
 * Check value of received vector datatype buffer
 */
static int MTestTypeVectorCheckbuf(MTestDatatype * mtype, MTestDataset dataset)
{
    unsigned char *p;
    unsigned char expected;
    int i, err = 0;
    MPI_Aint totspace = 0, extent = 0, byte_offset, dt_offset;
    MPI_Aint init_term = 0, common_diff = 0, data = 0, totsize = 0;

    p = (unsigned char *) mtype->buf;
    if (p) {
        MPI_Aint k, j;

        /* Get datatype information and dataset */
        datatype_decode(mtype, &extent, &totspace, &totsize);
        init_dataset_params(dataset, totsize, &init_term, &common_diff);

        dt_offset = 0;
        data = init_term;
        /* For each datatype */
        for (k = 0; k < mtype->count; k++) {
            /* For each block */
            for (i = 0; i < mtype->nblock; i++) {
                byte_offset = dt_offset + i * mtype->stride;
                /* For each byte */
                for (j = 0; j < mtype->blksize; j++) {
                    expected = (unsigned char) (0xff ^ ((data++ * common_diff) & 0xff));
                    if (p[byte_offset + j] != expected) {
                        err++;
                        if (mtype->printErrors && err < 10) {
                            printf("Data expected = %x but got p[%d,%ld] = %x\n", expected, i, j,
                                   p[byte_offset + j]);
                            fflush(stdout);
                        }
                    }
                }
            }
            dt_offset += extent;
        }
    }
    return err;
}


/* ------------------------------------------------------------------------ */
/* Datatype routines for indexed datatypes                            */
/* ------------------------------------------------------------------------ */

/*
 * Initialize buffer of indexed datatype
 */
static void *MTestTypeIndexedInit(MTestDatatype * mtype, MTestDataset dataset)
{
    MPI_Aint totspace = 0, extent = 0, dt_offset, offset;
    MPI_Aint init_term = 0, common_diff = 0, data = 0, totsize = 0;

    if (mtype->count > 0) {
        unsigned char *p;
        MPI_Aint k, b;
        int i, j;

        /* Get datatype information and dataset */
        datatype_decode(mtype, &extent, &totspace, &totsize);
        init_dataset_params(dataset, totsize, &init_term, &common_diff);

        /* Allocate buffer */
        if (!mtype->buf) {
            mtype->buf = (void *) malloc(totspace);
        }
        p = (unsigned char *) (mtype->buf);
        if (!p) {
            char errmsg[128] = { 0 };
            sprintf(errmsg, "Out of memory in %s", __FUNCTION__);
            MTestError(errmsg);
        }

        /* First, set to -1 */
        for (k = 0; k < totspace; k++)
            p[k] = 0xff;

        /* For empty initialization, it is done. */
        if (dataset == MTEST_DATA_EMPTY)
            return mtype->buf;

        /* Now, set value into actual elements.
         * We require that the base type is a contiguous type */
        data = init_term;
        dt_offset = 0;
        /* For each datatype */
        for (k = 0; k < mtype->count; k++) {
            /* For each block */
            for (i = 0; i < mtype->nblock; i++) {
                /* For each element in the block */
                for (j = 0; j < mtype->index[i]; j++) {
                    offset = dt_offset + mtype->displ_in_bytes[i]
                        + j * mtype->basesize;
                    /* For each byte in the element */
                    for (b = 0; b < mtype->basesize; b++) {
                        p[offset + b] = (unsigned char) (0xff ^ ((data++ * common_diff) & 0xff));
                    }
                }
            }
            dt_offset += extent;
        }
    }
    else {
        /* count == 0 */
        if (mtype->buf) {
            free(mtype->buf);
        }
        mtype->buf = 0;
    }
    return mtype->buf;
}

/*
 * Check value of received indexed datatype buffer
 */
static int MTestTypeIndexedCheckbuf(MTestDatatype * mtype, MTestDataset dataset)
{
    unsigned char *p;
    unsigned char expected;
    int err = 0;
    MPI_Aint totspace = 0, extent = 0, offset, dt_offset;
    MPI_Aint init_term = 0, common_diff = 0, data = 0, totsize = 0;

    p = (unsigned char *) mtype->buf;
    if (p) {
        MPI_Aint k, b;
        int i, j;

        /* Get datatype information and dataset */
        datatype_decode(mtype, &extent, &totspace, &totsize);
        init_dataset_params(dataset, totsize, &init_term, &common_diff);

        dt_offset = 0;
        data = init_term;
        /* For each datatype */
        for (k = 0; k < mtype->count; k++) {
            /* For each block */
            for (i = 0; i < mtype->nblock; i++) {
                /* For each element in the block */
                for (j = 0; j < mtype->index[i]; j++) {
                    offset = dt_offset + mtype->displ_in_bytes[i]
                        + j * mtype->basesize;
                    /* For each byte in the element */
                    for (b = 0; b < mtype->basesize; b++) {
                        expected = (unsigned char) (0xff ^ ((data++ * common_diff) & 0xff));
                        if (p[offset + b] != expected) {
                            err++;
                            if (mtype->printErrors && err < 10) {
                                printf("Data expected = %x but got p[%d,%d] = %x\n",
                                       expected, i, j, p[offset + b]);
                                fflush(stdout);
                            }
                        }
                    }
                }
            }
            dt_offset += extent;
        }
    }
    return err;
}

/* ------------------------------------------------------------------------ */
/* Datatype routines for indexed-block datatypes                            */
/* ------------------------------------------------------------------------ */

/*
 * Initialize buffer of indexed-block datatype
 */
static void *MTestTypeIndexedBlockInit(MTestDatatype * mtype, MTestDataset dataset)
{
    MPI_Aint totspace = 0, extent = 0, offset, dt_offset;
    MPI_Aint init_term = 0, common_diff = 0, data = 0, totsize = 0;

    if (mtype->count > 0) {
        unsigned char *p;
        MPI_Aint k, j;
        int i;

        /* Get datatype information and dataset */
        datatype_decode(mtype, &extent, &totspace, &totsize);
        init_dataset_params(dataset, totsize, &init_term, &common_diff);

        /* Allocate buffer */
        if (!mtype->buf) {
            mtype->buf = (void *) malloc(totspace);
        }
        p = (unsigned char *) (mtype->buf);
        if (!p) {
            char errmsg[128] = { 0 };
            sprintf(errmsg, "Out of memory in %s", __FUNCTION__);
            MTestError(errmsg);
        }

        /* First, set to -1 */
        for (k = 0; k < totspace; k++)
            p[k] = 0xff;

        /* For empty initialization, it is done. */
        if (dataset == MTEST_DATA_EMPTY)
            return mtype->buf;

        /* Now, set value into actual elements.
         * We require that the base type is a contiguous type */
        data = init_term;
        dt_offset = 0;
        /* For each datatype */
        for (k = 0; k < mtype->count; k++) {
            /* For each block */
            for (i = 0; i < mtype->nblock; i++) {
                offset = dt_offset + mtype->displ_in_bytes[i];
                /* For each byte in the block */
                for (j = 0; j < mtype->blksize; j++) {
                    p[offset + j] = (unsigned char) (0xff ^ ((data++ * common_diff) & 0xff));
                }
            }
            dt_offset += extent;
        }
    }
    else {
        /* count == 0 */
        if (mtype->buf) {
            free(mtype->buf);
        }
        mtype->buf = 0;
    }
    return mtype->buf;
}

/*
 * Check value of received indexed-block datatype buffer
 */
static int MTestTypeIndexedBlockCheckbuf(MTestDatatype * mtype, MTestDataset dataset)
{
    unsigned char *p;
    unsigned char expected;
    MPI_Aint dt_offset, offset;
    MPI_Aint totspace = 0, extent = 0;
    MPI_Aint init_term = 0, common_diff = 0, data = 0, totsize = 0;
    int err = 0;

    p = (unsigned char *) mtype->buf;
    if (p) {
        MPI_Aint j, k;
        int i;

        /* Get datatype information and dataset */
        datatype_decode(mtype, &extent, &totspace, &totsize);
        init_dataset_params(dataset, totsize, &init_term, &common_diff);

        dt_offset = 0;
        data = init_term;
        /* For each datatype */
        for (k = 0; k < mtype->count; k++) {
            /* For each block */
            for (i = 0; i < mtype->nblock; i++) {
                offset = dt_offset + mtype->displ_in_bytes[i];
                /* For each byte in the block */
                for (j = 0; j < mtype->blksize; j++) {
                    expected = (unsigned char) (0xff ^ ((data++ * common_diff) & 0xff));
                    if (p[offset + j] != expected) {
                        err++;
                        if (mtype->printErrors && err < 10) {
                            printf("Data expected = %x but got p[%d,%ld] = %x\n",
                                   expected, i, j, p[offset + j]);
                            fflush(stdout);
                        }
                    }
                }
            }
            dt_offset += extent;
        }
    }
    return err;
}

/* ------------------------------------------------------------------------ */
/* Datatype routines for subarray datatypes with order Fortran              */
/* ------------------------------------------------------------------------ */

/*
 * Initialize buffer of subarray datatype.
 */
static void *MTestTypeSubarrayInit(MTestDatatype * mtype, MTestDataset dataset)
{
    MPI_Aint totspace = 0, extent = 0, offset, dt_offset, byte_offset;
    MPI_Aint init_term = 0, common_diff = 0, data = 0, totsize = 0;

    if (mtype->count > 0) {
        unsigned char *p;
        MPI_Aint k;
        int j, b, i;

        /* Get datatype information and dataset */
        datatype_decode(mtype, &extent, &totspace, &totsize);
        init_dataset_params(dataset, totsize, &init_term, &common_diff);

        /* Allocate buffer */
        if (!mtype->buf) {
            mtype->buf = (void *) malloc(totspace);
        }
        p = (unsigned char *) (mtype->buf);
        if (!p) {
            char errmsg[128] = { 0 };
            sprintf(errmsg, "Out of memory in %s", __FUNCTION__);
            MTestError(errmsg);
        }

        /* First, set to -1 */
        for (k = 0; k < totspace; k++)
            p[k] = 0xff;

        /* For empty initialization, it is done. */
        if (dataset == MTEST_DATA_EMPTY)
            return mtype->buf;

        /* Now, set value into actual elements.
         * We require that the base type is a contiguous type. */
        int ncol, sub_ncol, sub_nrow, sub_col_start, sub_row_start;
        ncol = mtype->arr_sizes[1];
        sub_nrow = mtype->arr_subsizes[0];
        sub_ncol = mtype->arr_subsizes[1];
        sub_row_start = mtype->arr_starts[0];
        sub_col_start = mtype->arr_starts[1];

        data = init_term;
        dt_offset = 0;
        /* For each datatype */
        for (k = 0; k < mtype->count; k++) {
            /* For each row */
            for (i = 0; i < sub_nrow; i++) {
                offset = (sub_row_start + i) * ncol + sub_col_start;
                /* For each element in row */
                for (j = 0; j < sub_ncol; j++) {
                    byte_offset = dt_offset + (offset + j) * mtype->basesize;
                    /* For each byte in element */
                    for (b = 0; b < mtype->basesize; b++)
                        p[byte_offset + b] =
                            (unsigned char) (0xff ^ ((data++ * common_diff) & 0xff));
                }
            }
            dt_offset += extent;
        }
    }
    else {
        /* count == 0 */
        if (mtype->buf) {
            free(mtype->buf);
        }
        mtype->buf = 0;
    }
    return mtype->buf;
}

/*
 * Check value of received subarray datatype buffer
 */
static int MTestTypeSubarrayCheckbuf(MTestDatatype * mtype, MTestDataset dataset)
{
    unsigned char *p;
    unsigned char expected;
    int err = 0;
    MPI_Aint totspace = 0, extent = 0, offset, dt_offset, byte_offset;
    MPI_Aint init_term = 0, common_diff = 0, data = 0, totsize = 0;

    p = (unsigned char *) mtype->buf;
    if (p) {
        MPI_Aint k;
        int j, b, i;

        /* Get datatype information and dataset */
        datatype_decode(mtype, &extent, &totspace, &totsize);
        init_dataset_params(dataset, totsize, &init_term, &common_diff);

        int ncol, sub_ncol, sub_nrow, sub_col_start, sub_row_start;
        ncol = mtype->arr_sizes[1];
        sub_nrow = mtype->arr_subsizes[0];
        sub_ncol = mtype->arr_subsizes[1];
        sub_row_start = mtype->arr_starts[0];
        sub_col_start = mtype->arr_starts[1];

        data = init_term;
        dt_offset = 0;
        /* For each datatype */
        for (k = 0; k < mtype->count; k++) {
            /* For each row */
            for (i = 0; i < sub_nrow; i++) {
                offset = (sub_row_start + i) * ncol + sub_col_start;
                /* For each element in row */
                for (j = 0; j < sub_ncol; j++) {
                    byte_offset = dt_offset + (offset + j) * mtype->basesize;
                    /* For each byte in element */
                    for (b = 0; b < mtype->basesize; b++) {
                        expected = (unsigned char) (0xff ^ ((data++ * common_diff) & 0xff));
                        if (p[byte_offset + b] != expected) {
                            err++;
                            if (mtype->printErrors && err < 10) {
                                printf("Data expected = %x but got p[%d,%d,%d] = %x\n",
                                       expected, i, j, b, p[byte_offset + b]);
                                fflush(stdout);
                            }
                        }
                    }
                }
            }
            dt_offset += extent;
        }
    }
    if (err)
        printf("%s error\n", __FUNCTION__);
    return err;
}

/* ------------------------------------------------------------------------ */
/* Datatype creators                                                        */
/* ------------------------------------------------------------------------ */

/*
 * Setup contiguous type info and handlers.
 *
 * A contiguous datatype is created by using following parameters (stride is unused).
 * nblock:   Number of blocks.
 * blocklen: Number of elements in each block. The total number of elements in
 *           this datatype is set as (nblock * blocklen).
 * lb:       Lower bound of the new datatype (ignored).
 * oldtype:  Datatype of element.
 */
static int MTestTypeContiguousCreate(int nblock, int blocklen, int stride, int lb,
                                     MPI_Datatype oldtype, const char *typename_prefix,
                                     MTestDatatype * mtype)
{
    int merr = 0;
    char type_name[128];

    MTestTypeReset(mtype);

    merr = MPI_Type_size(oldtype, &mtype->basesize);
    if (merr)
        MTestPrintError(merr);

    mtype->nblock = nblock;
    mtype->blksize = blocklen * mtype->basesize;

    merr = MPI_Type_contiguous(nblock * blocklen, oldtype, &mtype->datatype);
    if (merr)
        MTestPrintError(merr);
    merr = MPI_Type_commit(&mtype->datatype);
    if (merr)
        MTestPrintError(merr);

    memset(type_name, 0, sizeof(type_name));
    sprintf(type_name, "%s %s (%d count)", typename_prefix, "contiguous", nblock * blocklen);
    merr = MPI_Type_set_name(mtype->datatype, (char *) type_name);
    if (merr)
        MTestPrintError(merr);

    mtype->InitBuf = MTestTypeContigInit;
    mtype->FreeBuf = MTestTypeFree;
    mtype->CheckBuf = MTestTypeContigCheckbuf;
    mtype->Dup = MTestTypeDup;
    return merr;
}

/*
 * Setup vector type info and handlers.
 *
 * A vector datatype is created by using following parameters.
 * nblock:   Number of blocks.
 * blocklen: Number of elements in each block.
 * stride:   Strided number of elements between blocks.
 * lb:       Lower bound of the new datatype (ignored).
 * oldtype:  Datatype of element.
 */
static int MTestTypeVectorCreate(int nblock, int blocklen, int stride, int lb,
                                 MPI_Datatype oldtype, const char *typename_prefix,
                                 MTestDatatype * mtype)
{
    int merr = 0;
    char type_name[128];

    MTestTypeReset(mtype);

    merr = MPI_Type_size(oldtype, &mtype->basesize);
    if (merr)
        MTestPrintError(merr);

    /* These sizes are in bytes (see the VectorInit code) */
    mtype->stride = stride * mtype->basesize;
    mtype->blksize = blocklen * mtype->basesize;
    mtype->nblock = nblock;

    /* Vector uses stride in oldtypes */
    merr = MPI_Type_vector(nblock, blocklen, stride, oldtype, &mtype->datatype);
    if (merr)
        MTestPrintError(merr);
    merr = MPI_Type_commit(&mtype->datatype);
    if (merr)
        MTestPrintError(merr);

    memset(type_name, 0, sizeof(type_name));
    sprintf(type_name, "%s %s (%d nblock %d blocklen %d stride)", typename_prefix, "vector", nblock,
            blocklen, stride);
    merr = MPI_Type_set_name(mtype->datatype, (char *) type_name);
    if (merr)
        MTestPrintError(merr);

    mtype->InitBuf = MTestTypeVectorInit;
    mtype->FreeBuf = MTestTypeFree;
    mtype->CheckBuf = MTestTypeVectorCheckbuf;
    mtype->Dup = MTestTypeDup;
    return merr;
}

/*
 * Setup hvector type info and handlers.
 *
 * A hvector datatype is created by using following parameters.
 * nblock:   Number of blocks.
 * blocklen: Number of elements in each block.
 * stride:   Strided number of elements between blocks.
 * lb:       Lower bound of the new datatype (ignored).
 * oldtype:  Datatype of element.
 */
static int MTestTypeHvectorCreate(int nblock, int blocklen, int stride, int lb,
                                  MPI_Datatype oldtype, const char *typename_prefix,
                                  MTestDatatype * mtype)
{
    int merr;
    char type_name[128];

    MTestTypeReset(mtype);

    merr = MPI_Type_size(oldtype, &mtype->basesize);
    if (merr)
        MTestPrintError(merr);

    /* These sizes are in bytes (see the VectorInit code) */
    mtype->stride = stride * mtype->basesize;
    mtype->blksize = blocklen * mtype->basesize;
    mtype->nblock = nblock;

    /* Hvector uses stride in bytes */
    merr = MPI_Type_create_hvector(nblock, blocklen, mtype->stride, oldtype, &mtype->datatype);
    if (merr)
        MTestPrintError(merr);
    merr = MPI_Type_commit(&mtype->datatype);
    if (merr)
        MTestPrintError(merr);

    memset(type_name, 0, sizeof(type_name));
    sprintf(type_name, "%s %s (%d nblock %d blocklen %d stride)", typename_prefix, "hvector",
            nblock, blocklen, stride);
    merr = MPI_Type_set_name(mtype->datatype, (char *) type_name);
    if (merr)
        MTestPrintError(merr);

    /* User the same functions as vector, because mtype->stride is in bytes */
    mtype->InitBuf = MTestTypeVectorInit;
    mtype->FreeBuf = MTestTypeFree;
    mtype->CheckBuf = MTestTypeVectorCheckbuf;
    mtype->Dup = MTestTypeDup;

    return merr;
}

/*
 * Setup indexed type info and handlers.
 *
 * A indexed datatype is created by using following parameters.
 * nblock:   Number of blocks.
 * blocklen: Number of elements in each block. Each block has the same length.
 * stride:   Strided number of elements between two adjacent blocks. The
 *           displacement of each block is set as (index of current block * stride).
 * lb:       Lower bound of the new datatype.
 * oldtype:  Datatype of element.
 */
static int MTestTypeIndexedCreate(int nblock, int blocklen, int stride, int lb,
                                  MPI_Datatype oldtype, const char *typename_prefix,
                                  MTestDatatype * mtype)
{
    int merr = 0;
    char type_name[128];
    int i;

    MTestTypeReset(mtype);

    merr = MPI_Type_size(oldtype, &mtype->basesize);
    if (merr)
        MTestPrintError(merr);

    mtype->displs = (int *) malloc(nblock * sizeof(int));
    mtype->displ_in_bytes = (MPI_Aint *) malloc(nblock * sizeof(MPI_Aint));
    mtype->index = (int *) malloc(nblock * sizeof(int));
    if (!mtype->displs || !mtype->displ_in_bytes || !mtype->index) {
        char errmsg[128] = { 0 };
        sprintf(errmsg, "Out of memory in %s", __FUNCTION__);
        MTestError(errmsg);
    }

    mtype->nblock = nblock;
    for (i = 0; i < nblock; i++) {
        mtype->index[i] = blocklen;
        mtype->displs[i] = lb + stride * i;     /*stride between the start of two blocks */
        mtype->displ_in_bytes[i] = (lb + stride * i) * mtype->basesize;
    }

    /* Indexed uses displacement in oldtypes */
    merr = MPI_Type_indexed(nblock, mtype->index, mtype->displs, oldtype, &mtype->datatype);
    if (merr)
        MTestPrintError(merr);
    merr = MPI_Type_commit(&mtype->datatype);
    if (merr)
        MTestPrintError(merr);

    memset(type_name, 0, sizeof(type_name));
    sprintf(type_name, "%s %s (%d nblock %d blocklen %d stride %d lb)", typename_prefix,
            "index", nblock, blocklen, stride, lb);
    merr = MPI_Type_set_name(mtype->datatype, (char *) type_name);
    if (merr)
        MTestPrintError(merr);

    mtype->InitBuf = MTestTypeIndexedInit;
    mtype->FreeBuf = MTestTypeFree;
    mtype->CheckBuf = MTestTypeIndexedCheckbuf;
    mtype->Dup = MTestTypeDup;

    return merr;
}

/*
 * Setup hindexed type info and handlers.
 *
 * A hindexed datatype is created by using following parameters.
 * nblock:   Number of blocks.
 * blocklen: Number of elements in each block. Each block has the same length.
 * stride:   Strided number of elements between two adjacent blocks. The byte
 *           displacement of each block is set as (index of current block * stride * size of oldtype).
 * lb:       Lower bound of the new datatype.
 * oldtype:  Datatype of element.
 */
static inline int MTestTypeHindexedCreate(int nblock, int blocklen, int stride, int lb,
                                          MPI_Datatype oldtype, const char *typename_prefix,
                                          MTestDatatype * mtype)
{
    int merr;
    char type_name[128];
    int i;

    MTestTypeReset(mtype);

    merr = MPI_Type_size(oldtype, &mtype->basesize);
    if (merr)
        MTestPrintError(merr);

    mtype->index = (int *) malloc(nblock * sizeof(int));
    mtype->displ_in_bytes = (MPI_Aint *) malloc(nblock * sizeof(MPI_Aint));
    if (!mtype->displ_in_bytes || !mtype->index) {
        char errmsg[128] = { 0 };
        sprintf(errmsg, "Out of memory in %s", __FUNCTION__);
        MTestError(errmsg);
    }

    mtype->nblock = nblock;
    for (i = 0; i < nblock; i++) {
        mtype->index[i] = blocklen;
        mtype->displ_in_bytes[i] = (lb + stride * i) * mtype->basesize;
    }

    /* Hindexed uses displacement in bytes */
    merr = MPI_Type_create_hindexed(nblock, mtype->index, mtype->displ_in_bytes,
                                    oldtype, &mtype->datatype);
    if (merr)
        MTestPrintError(merr);
    merr = MPI_Type_commit(&mtype->datatype);
    if (merr)
        MTestPrintError(merr);

    memset(type_name, 0, sizeof(type_name));
    sprintf(type_name, "%s %s (%d nblock %d blocklen %d stride %d lb)", typename_prefix,
            "hindex", nblock, blocklen, stride, lb);
    merr = MPI_Type_set_name(mtype->datatype, (char *) type_name);
    if (merr)
        MTestPrintError(merr);

    /* Reuse indexed functions, because all of them only use displ_in_bytes */
    mtype->InitBuf = MTestTypeIndexedInit;
    mtype->FreeBuf = MTestTypeFree;
    mtype->CheckBuf = MTestTypeIndexedCheckbuf;
    mtype->Dup = MTestTypeDup;

    return merr;
}


/*
 * Setup indexed-block type info and handlers.
 *
 * A indexed-block datatype is created by using following parameters.
 * nblock:   Number of blocks.
 * blocklen: Number of elements in each block.
 * stride:   Strided number of elements between two adjacent blocks. The
 *           displacement of each block is set as (index of current block * stride).
 * lb:       Lower bound of the new datatype.
 * oldtype:  Datatype of element.
 */
static int MTestTypeIndexedBlockCreate(int nblock, int blocklen, int stride, int lb,
                                       MPI_Datatype oldtype, const char *typename_prefix,
                                       MTestDatatype * mtype)
{
    int merr;
    char type_name[128];
    int i;

    MTestTypeReset(mtype);

    merr = MPI_Type_size(oldtype, &mtype->basesize);
    if (merr)
        MTestPrintError(merr);

    mtype->displs = (int *) malloc(nblock * sizeof(int));
    mtype->displ_in_bytes = (MPI_Aint *) malloc(nblock * sizeof(MPI_Aint));
    if (!mtype->displs || !mtype->displ_in_bytes) {
        char errmsg[128] = { 0 };
        sprintf(errmsg, "Out of memory in %s", __FUNCTION__);
        MTestError(errmsg);
    }

    mtype->nblock = nblock;
    mtype->blksize = blocklen * mtype->basesize;
    for (i = 0; i < nblock; i++) {
        mtype->displs[i] = lb + stride * i;
        mtype->displ_in_bytes[i] = (lb + stride * i) * mtype->basesize;
    }

    /* Indexed-block uses displacement in oldtypes */
    merr = MPI_Type_create_indexed_block(nblock, blocklen, mtype->displs,
                                         oldtype, &mtype->datatype);
    if (merr)
        MTestPrintError(merr);
    merr = MPI_Type_commit(&mtype->datatype);
    if (merr)
        MTestPrintError(merr);

    memset(type_name, 0, sizeof(type_name));
    sprintf(type_name, "%s %s (%d nblock %d blocklen %d stride %d lb)", typename_prefix,
            "index_block", nblock, blocklen, stride, lb);
    merr = MPI_Type_set_name(mtype->datatype, (char *) type_name);
    if (merr)
        MTestPrintError(merr);

    mtype->InitBuf = MTestTypeIndexedBlockInit;
    mtype->FreeBuf = MTestTypeFree;
    mtype->CheckBuf = MTestTypeIndexedBlockCheckbuf;
    mtype->Dup = MTestTypeDup;

    return merr;
}

/*
 * Setup hindexed-block type info and handlers.
 *
 * A hindexed-block datatype is created by using following parameters.
 * nblock:   Number of blocks.
 * blocklen: Number of elements in each block.
 * stride:   Strided number of elements between two adjacent blocks. The byte
 *           displacement of each block is set as (index of current block * stride * size of oldtype).
 * lb:       Lower bound of the new datatype.
 * oldtype:  Datatype of element.
 */
static int MTestTypeHindexedBlockCreate(int nblock, int blocklen, int stride, int lb,
                                        MPI_Datatype oldtype, const char *typename_prefix,
                                        MTestDatatype * mtype)
{
    int merr;
    char type_name[128];
    int i;

    MTestTypeReset(mtype);

    merr = MPI_Type_size(oldtype, &mtype->basesize);
    if (merr)
        MTestPrintError(merr);

    mtype->displ_in_bytes = (MPI_Aint *) malloc(nblock * sizeof(MPI_Aint));
    if (!mtype->displ_in_bytes) {
        char errmsg[128] = { 0 };
        sprintf(errmsg, "Out of memory in %s", __FUNCTION__);
        MTestError(errmsg);
    }

    mtype->nblock = nblock;
    mtype->blksize = blocklen * mtype->basesize;
    for (i = 0; i < nblock; i++) {
        mtype->displ_in_bytes[i] = (lb + stride * i) * mtype->basesize;
    }

    /* Hindexed-block uses displacement in bytes */
    merr = MPI_Type_create_hindexed_block(nblock, blocklen, mtype->displ_in_bytes,
                                          oldtype, &mtype->datatype);
    if (merr)
        MTestPrintError(merr);
    merr = MPI_Type_commit(&mtype->datatype);
    if (merr)
        MTestPrintError(merr);

    memset(type_name, 0, sizeof(type_name));
    sprintf(type_name, "%s %s (%d nblock %d blocklen %d stride %d lb)", typename_prefix,
            "hindex_block", nblock, blocklen, stride, lb);
    merr = MPI_Type_set_name(mtype->datatype, (char *) type_name);
    if (merr)
        MTestPrintError(merr);

    /* Reuse indexed-block functions, because all of them only use displ_in_bytes */
    mtype->InitBuf = MTestTypeIndexedBlockInit;
    mtype->FreeBuf = MTestTypeFree;
    mtype->CheckBuf = MTestTypeIndexedBlockCheckbuf;
    mtype->Dup = MTestTypeDup;

    return merr;
}

/*
 * Setup struct type info and handlers.
 *
 * A struct datatype is created by using following parameters.
 * nblock:   Number of blocks.
 * blocklen: Number of elements in each block. Each block has the same length.
 * stride:   Strided number of elements between two adjacent blocks. The byte
 *           displacement of each block is set as (index of current block * stride * size of oldtype).
 * lb:       Lower bound of the new datatype.
 * oldtype:  Datatype of element. Each block has the same oldtype.
 */
static int MTestTypeStructCreate(int nblock, int blocklen, int stride, int lb,
                                 MPI_Datatype oldtype, const char *typename_prefix,
                                 MTestDatatype * mtype)
{
    int merr;
    char type_name[128];
    int i;

    MTestTypeReset(mtype);

    merr = MPI_Type_size(oldtype, &mtype->basesize);
    if (merr)
        MTestPrintError(merr);

    mtype->old_datatypes = (MPI_Datatype *) malloc(nblock * sizeof(MPI_Datatype));
    mtype->displ_in_bytes = (MPI_Aint *) malloc(nblock * sizeof(MPI_Aint));
    mtype->index = (int *) malloc(nblock * sizeof(int));
    if (!mtype->displ_in_bytes || !mtype->old_datatypes) {
        char errmsg[128] = { 0 };
        sprintf(errmsg, "Out of memory in %s", __FUNCTION__);
        MTestError(errmsg);
    }

    mtype->nblock = nblock;
    mtype->blksize = blocklen * mtype->basesize;
    for (i = 0; i < nblock; i++) {
        mtype->displ_in_bytes[i] = (lb + stride * i) * mtype->basesize;
        mtype->old_datatypes[i] = oldtype;
        mtype->index[i] = blocklen;
    }

    /* Struct uses displacement in bytes */
    merr = MPI_Type_create_struct(nblock, mtype->index, mtype->displ_in_bytes,
                                  mtype->old_datatypes, &mtype->datatype);
    if (merr)
        MTestPrintError(merr);
    merr = MPI_Type_commit(&mtype->datatype);
    if (merr)
        MTestPrintError(merr);

    memset(type_name, 0, sizeof(type_name));
    sprintf(type_name, "%s %s (%d nblock %d blocklen %d stride %d lb)", typename_prefix,
            "struct", nblock, blocklen, stride, lb);
    merr = MPI_Type_set_name(mtype->datatype, (char *) type_name);
    if (merr)
        MTestPrintError(merr);

    /* Reuse indexed functions, because they use the same displ_in_bytes and index */
    mtype->InitBuf = MTestTypeIndexedInit;
    mtype->FreeBuf = MTestTypeFree;
    mtype->CheckBuf = MTestTypeIndexedCheckbuf;
    mtype->Dup = MTestTypeDup;

    return merr;
}

/*
 * Setup order-C subarray type info and handlers.
 *
 * A 2D-subarray datatype specified with order C and located in the right-bottom
 * of the full array is created by using input parameters.
 * Number of elements in the dimensions of the full array: {nblock + lb, stride}
 * Number of elements in the dimensions of the subarray: {nblock, blocklen}
 * Starting of the subarray in each dimension: {1, stride - blocklen}
 * order: MPI_ORDER_C
 * oldtype: oldtype
 */
static int MTestTypeSubArrayOrderCCreate(int nblock, int blocklen, int stride, int lb,
                                         MPI_Datatype oldtype, const char *typename_prefix,
                                         MTestDatatype * mtype)
{
    int merr;
    char type_name[128];

    MTestTypeReset(mtype);

    merr = MPI_Type_size(oldtype, &mtype->basesize);
    if (merr)
        MTestPrintError(merr);

    mtype->arr_sizes[0] = nblock + lb;  /* {row, col} */
    mtype->arr_sizes[1] = stride;
    mtype->arr_subsizes[0] = nblock;    /* {row, col} */
    mtype->arr_subsizes[1] = blocklen;
    mtype->arr_starts[0] = lb;  /* {row, col} */
    mtype->arr_starts[1] = stride - blocklen;
    mtype->order = MPI_ORDER_C;

    merr = MPI_Type_create_subarray(2, mtype->arr_sizes, mtype->arr_subsizes, mtype->arr_starts,
                                    mtype->order, oldtype, &mtype->datatype);
    if (merr)
        MTestPrintError(merr);
    merr = MPI_Type_commit(&mtype->datatype);
    if (merr)
        MTestPrintError(merr);

    memset(type_name, 0, sizeof(type_name));
    sprintf(type_name, "%s %s (full{%d,%d}, sub{%d,%d},start{%d,%d})",
            typename_prefix, "subarray-c", mtype->arr_sizes[0], mtype->arr_sizes[1],
            mtype->arr_subsizes[0], mtype->arr_subsizes[1], mtype->arr_starts[0],
            mtype->arr_starts[1]);
    merr = MPI_Type_set_name(mtype->datatype, (char *) type_name);
    if (merr)
        MTestPrintError(merr);

    mtype->InitBuf = MTestTypeSubarrayInit;
    mtype->FreeBuf = MTestTypeFree;
    mtype->CheckBuf = MTestTypeSubarrayCheckbuf;
    mtype->Dup = MTestTypeDup;

    return merr;
}


/*
 * Setup order-Fortran subarray type info and handlers.
 *
 * A 2D-subarray datatype specified with order Fortran and located in the right
 * bottom of the full array is created by using input parameters.
 * Number of elements in the dimensions of the full array: {stride, nblock + lb}
 * Number of elements in the dimensions of the subarray: {blocklen, nblock}
 * Starting of the subarray in each dimension: {stride - blocklen, lb}
 * order: MPI_ORDER_FORTRAN
 * oldtype: oldtype
 */
static int MTestTypeSubArrayOrderFortranCreate(int nblock, int blocklen, int stride, int lb,
                                               MPI_Datatype oldtype, const char *typename_prefix,
                                               MTestDatatype * mtype)
{
    int merr;
    char type_name[128];

    MTestTypeReset(mtype);

    merr = MPI_Type_size(oldtype, &mtype->basesize);
    if (merr)
        MTestPrintError(merr);

    /* use the same row and col as that of order-c subarray for buffer
     * initialization and check because we access buffer in order-c */
    mtype->arr_sizes[0] = nblock + lb;  /* {row, col} */
    mtype->arr_sizes[1] = stride;
    mtype->arr_subsizes[0] = nblock;    /* {row, col} */
    mtype->arr_subsizes[1] = blocklen;
    mtype->arr_starts[0] = lb;  /* {row, col} */
    mtype->arr_starts[1] = stride - blocklen;
    mtype->order = MPI_ORDER_FORTRAN;

    /* reverse row and col when create datatype so that we can get the same
     * packed data on the other side in order to reuse the contig check function */
    int arr_sizes[2] = { mtype->arr_sizes[1], mtype->arr_sizes[0] };
    int arr_subsizes[2] = { mtype->arr_subsizes[1], mtype->arr_subsizes[0] };
    int arr_starts[2] = { mtype->arr_starts[1], mtype->arr_starts[0] };

    merr = MPI_Type_create_subarray(2, arr_sizes, arr_subsizes, arr_starts,
                                    mtype->order, oldtype, &mtype->datatype);
    if (merr)
        MTestPrintError(merr);
    merr = MPI_Type_commit(&mtype->datatype);
    if (merr)
        MTestPrintError(merr);

    memset(type_name, 0, sizeof(type_name));
    sprintf(type_name, "%s %s (full{%d,%d}, sub{%d,%d},start{%d,%d})",
            typename_prefix, "subarray-f", arr_sizes[0], arr_sizes[1],
            arr_subsizes[0], arr_subsizes[1], arr_starts[0], arr_starts[1]);
    merr = MPI_Type_set_name(mtype->datatype, (char *) type_name);
    if (merr)
        MTestPrintError(merr);

    mtype->InitBuf = MTestTypeSubarrayInit;
    mtype->FreeBuf = MTestTypeFree;
    mtype->CheckBuf = MTestTypeSubarrayCheckbuf;
    mtype->Dup = MTestTypeDup;

    return merr;
}

/* ------------------------------------------------------------------------ */
/* Datatype routines exposed to test generator                             */
/* ------------------------------------------------------------------------ */

/*
 * Setup basic type info and handlers.
 */
int MTestTypeBasicCreate(MPI_Datatype oldtype, MTestDatatype * mtype)
{
    int merr = 0;

    MTestTypeReset(mtype);

    merr = MPI_Type_size(oldtype, &mtype->basesize);
    if (merr)
        MTestPrintError(merr);

    mtype->datatype = oldtype;
    mtype->isBasic = 1;
    mtype->InitBuf = MTestTypeContigInit;
    mtype->FreeBuf = MTestTypeFree;
    mtype->CheckBuf = MTestTypeContigCheckbuf;
    mtype->Dup = MTestTypeDup;

    return merr;
}

/*
 * Setup dup type info and handlers.
 *
 * A dup datatype is created by using following parameters.
 * oldtype:  Datatype of element.
 */
int MTestTypeDupCreate(MPI_Datatype oldtype, MTestDatatype * mtype)
{
    int merr = 0;

    MTestTypeReset(mtype);

    merr = MPI_Type_size(oldtype, &mtype->basesize);
    if (merr)
        MTestPrintError(merr);

    merr = MPI_Type_dup(oldtype, &mtype->datatype);
    if (merr)
        MTestPrintError(merr);

    /* dup'ed types are already committed if the original type
     * was committed (MPI-2, section 8.8) */

    mtype->InitBuf = MTestTypeContigInit;
    mtype->FreeBuf = MTestTypeFree;
    mtype->CheckBuf = MTestTypeContigCheckbuf;
    mtype->Dup = MTestTypeDup;

    return merr;
}

void MTestTypeCreatorInit(MTestDdtCreator * creators)
{
    memset(creators, 0, sizeof(MTestDdtCreator) * MTEST_DDT_MAX);
    creators[MTEST_DDT_CONTIGUOUS] = MTestTypeContiguousCreate;
    creators[MTEST_DDT_VECTOR] = MTestTypeVectorCreate;
    creators[MTEST_DDT_HVECTOR] = MTestTypeHvectorCreate;
    creators[MTEST_DDT_INDEXED] = MTestTypeIndexedCreate;
    creators[MTEST_DDT_HINDEXED] = MTestTypeHindexedCreate;
    creators[MTEST_DDT_INDEXED_BLOCK] = MTestTypeIndexedBlockCreate;
    creators[MTEST_DDT_HINDEXED_BLOCK] = MTestTypeHindexedBlockCreate;
    creators[MTEST_DDT_STRUCT] = MTestTypeStructCreate;
    creators[MTEST_DDT_SUBARRAY_ORDER_C] = MTestTypeSubArrayOrderCCreate;
    creators[MTEST_DDT_SUBARRAY_ORDER_FORTRAN] = MTestTypeSubArrayOrderFortranCreate;
}

void MTestTypeMinCreatorInit(MTestDdtCreator * creators)
{
    memset(creators, 0, sizeof(MTestDdtCreator) * MTEST_DDT_MAX);
    creators[MTEST_MIN_DDT_VECTOR] = MTestTypeVectorCreate;
    creators[MTEST_MIN_DDT_INDEXED] = MTestTypeIndexedCreate;
}

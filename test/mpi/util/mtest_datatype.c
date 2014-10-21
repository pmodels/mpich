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
/* Datatype routines for contiguous datatypes                               */
/* ------------------------------------------------------------------------ */
/*
 * Initialize buffer of basic datatype
 */
static void *MTestTypeContigInit(MTestDatatype * mtype)
{
    MPI_Aint size;
    int merr;

    if (mtype->count > 0) {
        unsigned char *p;
        int i, totsize;
        merr = MPI_Type_extent(mtype->datatype, &size);
        if (merr)
            MTestPrintError(merr);
        totsize = size * mtype->count;
        if (!mtype->buf) {
            mtype->buf = (void *) malloc(totsize);
        }
        p = (unsigned char *) (mtype->buf);
        if (!p) {
            char errmsg[128] = { 0 };
            sprintf(errmsg, "Out of memory in %s", __FUNCTION__);
            MTestError(errmsg);
        }
        for (i = 0; i < totsize; i++) {
            p[i] = (unsigned char) (0xff ^ (i & 0xff));
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
 * Free buffer of basic datatype
 */
static void *MTestTypeContigFree(MTestDatatype * mtype)
{
    if (mtype->buf) {
        free(mtype->buf);
        mtype->buf = 0;
    }
    return 0;
}

/*
 * Check value of received basic datatype buffer.
 */
static int MTestTypeContigCheckbuf(MTestDatatype * mtype)
{
    unsigned char *p;
    unsigned char expected;
    int i, totsize, err = 0, merr;
    MPI_Aint size;

    p = (unsigned char *) mtype->buf;
    if (p) {
        merr = MPI_Type_extent(mtype->datatype, &size);
        if (merr)
            MTestPrintError(merr);
        totsize = size * mtype->count;
        for (i = 0; i < totsize; i++) {
            expected = (unsigned char) (0xff ^ (i & 0xff));
            if (p[i] != expected) {
                err++;
                if (mtype->printErrors && err < 10) {
                    printf("Data expected = %x but got p[%d] = %x\n", expected, i, p[i]);
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
static void *MTestTypeVectorInit(MTestDatatype * mtype)
{
    MPI_Aint size, totsize, dt_offset, byte_offset;
    int merr;

    if (mtype->count > 0) {
        unsigned char *p;
        int i, j, k, nc;

        merr = MPI_Type_extent(mtype->datatype, &size);
        if (merr)
            MTestPrintError(merr);
        totsize = mtype->count * size;
        if (!mtype->buf) {
            mtype->buf = (void *) malloc(totsize);
        }
        p = (unsigned char *) (mtype->buf);
        if (!p) {
            char errmsg[128] = { 0 };
            sprintf(errmsg, "Out of memory in %s", __FUNCTION__);
            MTestError(errmsg);
        }

        /* First, set to -1 */
        for (i = 0; i < totsize; i++)
            p[i] = 0xff;

        /* Now, set the actual elements to the successive values.
         * We require that the base type is a contiguous type */
        nc = 0;
        dt_offset = 0;
        /* For each datatype */
        for (k = 0; k < mtype->count; k++) {
            /* For each block */
            for (i = 0; i < mtype->nblock; i++) {
                byte_offset = dt_offset + i * mtype->stride;
                /* For each byte */
                for (j = 0; j < mtype->blksize; j++) {
                    p[byte_offset + j] = (unsigned char) (0xff ^ (nc & 0xff));
                    nc++;
                }
            }
            dt_offset += size;
        }
    }
    else {
        mtype->buf = 0;
    }
    return mtype->buf;
}

/*
 * Free buffer of vector datatype
 */
static void *MTestTypeVectorFree(MTestDatatype * mtype)
{
    if (mtype->buf) {
        free(mtype->buf);
        mtype->buf = 0;
    }
    return 0;
}

/*
 * Check value of received vector datatype buffer
 */
static int MTestTypeVectorCheckbuf(MTestDatatype * mtype)
{
    unsigned char *p;
    unsigned char expected;
    int i, err = 0, merr;
    MPI_Aint size = 0, byte_offset, dt_offset;

    p = (unsigned char *) mtype->buf;
    if (p) {
        int j, k, nc;
        merr = MPI_Type_extent(mtype->datatype, &size);
        if (merr)
            MTestPrintError(merr);

        nc = 0;
        dt_offset = 0;
        /* For each datatype */
        for (k = 0; k < mtype->count; k++) {
            /* For each block */
            for (i = 0; i < mtype->nblock; i++) {
                byte_offset = dt_offset + i * mtype->stride;
                /* For each byte */
                for (j = 0; j < mtype->blksize; j++) {
                    expected = (unsigned char) (0xff ^ (nc & 0xff));
                    if (p[byte_offset + j] != expected) {
                        err++;
                        if (mtype->printErrors && err < 10) {
                            printf("Data expected = %x but got p[%d,%d] = %x\n", expected, i, j,
                                   p[byte_offset + j]);
                            fflush(stdout);
                        }
                    }
                    nc++;
                }
            }
            dt_offset += size;
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
static void *MTestTypeIndexedInit(MTestDatatype * mtype)
{
    MPI_Aint size = 0, totsize;
    int merr;

    if (mtype->count > 0) {
        unsigned char *p;
        int i, j, k, b, nc, offset, dt_offset;

        /* Allocate buffer */
        merr = MPI_Type_extent(mtype->datatype, &size);
        if (merr)
            MTestPrintError(merr);
        totsize = size * mtype->count;

        if (!mtype->buf) {
            mtype->buf = (void *) malloc(totsize);
        }
        p = (unsigned char *) (mtype->buf);
        if (!p) {
            char errmsg[128] = { 0 };
            sprintf(errmsg, "Out of memory in %s", __FUNCTION__);
            MTestError(errmsg);
        }

        /* First, set to -1 */
        for (i = 0; i < totsize; i++)
            p[i] = 0xff;

        /* Now, set the actual elements to the successive values.
         * We require that the base type is a contiguous type */
        nc = 0;
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
                        p[offset + b] = (unsigned char) (0xff ^ (nc++ & 0xff));
                    }
                }
            }
            dt_offset += size;
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
 * Free buffer of indexed datatype
 */
static void *MTestTypeIndexedFree(MTestDatatype * mtype)
{
    if (mtype->buf) {
        free(mtype->buf);
        free(mtype->displs);
        free(mtype->displ_in_bytes);
        free(mtype->index);
        mtype->buf = 0;
        mtype->displs = 0;
        mtype->displ_in_bytes = 0;
        mtype->index = 0;
    }
    return 0;
}

/*
 * Check value of received indexed datatype buffer
 */
static int MTestTypeIndexedCheckbuf(MTestDatatype * mtype)
{
    unsigned char *p;
    unsigned char expected;
    int err = 0, merr;
    MPI_Aint size = 0;

    p = (unsigned char *) mtype->buf;
    if (p) {
        int i, j, k, b, nc, offset, dt_offset;
        merr = MPI_Type_extent(mtype->datatype, &size);
        if (merr)
            MTestPrintError(merr);

        nc = 0;
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
                        expected = (unsigned char) (0xff ^ (nc++ & 0xff));
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
            dt_offset += size;
        }
    }
    return err;
}


/* ------------------------------------------------------------------------ */
/* Datatype generators                                                      */
/* ------------------------------------------------------------------------ */

/*
 * Setup contiguous type info and handlers.
 *
 * A contiguous datatype is created by using following parameters (stride is unused).
 * nblock:   Number of blocks.
 * blocklen: Number of elements in each block. The total number of elements in
 *           this datatype is set as (nblock * blocklen).
 * oldtype:  Datatype of element.
 */
static int MTestTypeContiguousCreate(int nblock, int blocklen, int stride,
                                     MPI_Datatype oldtype, const char *typename_prefix,
                                     MTestDatatype * mtype)
{
    int merr = 0;
    char type_name[128];

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
    mtype->FreeBuf = MTestTypeContigFree;
    mtype->CheckBuf = MTestTypeContigCheckbuf;
    return merr;
}

/*
 * Setup vector type info and handlers.
 *
 * A vector datatype is created by using following parameters.
 * nblock:   Number of blocks.
 * blocklen: Number of elements in each block.
 * stride:   Strided number of elements between blocks.
 * oldtype:  Datatype of element.
 */
static int MTestTypeVectorCreate(int nblock, int blocklen, int stride,
                                 MPI_Datatype oldtype, const char *typename_prefix,
                                 MTestDatatype * mtype)
{
    int merr = 0;
    char type_name[128];

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
    mtype->FreeBuf = MTestTypeVectorFree;
    mtype->CheckBuf = MTestTypeVectorCheckbuf;
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
 * oldtype:  Datatype of element.
 */
static int MTestTypeIndexedCreate(int nblock, int blocklen, int stride,
                                  MPI_Datatype oldtype, const char *typename_prefix,
                                  MTestDatatype * mtype)
{
    int merr = 0;
    char type_name[128];
    int i;

    merr = MPI_Type_size(oldtype, &mtype->basesize);
    if (merr)
        MTestPrintError(merr);

    mtype->displs = (int *) malloc(nblock * sizeof(int));
    mtype->displ_in_bytes = (int *) malloc(nblock * sizeof(int));
    mtype->index = (int *) malloc(nblock * sizeof(int));
    if (!mtype->displs || !mtype->displ_in_bytes || !mtype->index) {
        char errmsg[128] = { 0 };
        sprintf(errmsg, "Out of memory in %s", __FUNCTION__);
        MTestError(errmsg);
    }

    mtype->nblock = nblock;
    for (i = 0; i < nblock; i++) {
        mtype->index[i] = blocklen;
        mtype->displs[i] = stride * i;  /*stride between the start of two blocks */
        mtype->displ_in_bytes[i] = stride * i * mtype->basesize;
    }

    /* Indexed uses displacement in oldtypes */
    merr = MPI_Type_indexed(nblock, mtype->index, mtype->displs, oldtype, &mtype->datatype);
    if (merr)
        MTestPrintError(merr);
    merr = MPI_Type_commit(&mtype->datatype);
    if (merr)
        MTestPrintError(merr);

    memset(type_name, 0, sizeof(type_name));
    sprintf(type_name, "%s %s (%d nblock %d blocklen %d stride)", typename_prefix, "index", nblock,
            blocklen, stride);
    merr = MPI_Type_set_name(mtype->datatype, (char *) type_name);
    if (merr)
        MTestPrintError(merr);

    mtype->InitBuf = MTestTypeIndexedInit;
    mtype->FreeBuf = MTestTypeIndexedFree;
    mtype->CheckBuf = MTestTypeIndexedCheckbuf;

    return merr;
}

/*
 * Setup basic type info and handlers.
 */
int MTestTypeBasicCreate(MPI_Datatype oldtype, MTestDatatype * mtype)
{
    int merr = 0;

    merr = MPI_Type_size(oldtype, &mtype->basesize);
    if (merr)
        MTestPrintError(merr);

    mtype->datatype = oldtype;
    mtype->isBasic = 1;
    mtype->InitBuf = MTestTypeContigInit;
    mtype->FreeBuf = MTestTypeContigFree;
    mtype->CheckBuf = MTestTypeContigCheckbuf;

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

    merr = MPI_Type_size(oldtype, &mtype->basesize);
    if (merr)
        MTestPrintError(merr);

    merr = MPI_Type_dup(oldtype, &mtype->datatype);
    if (merr)
        MTestPrintError(merr);

    /* dup'ed types are already committed if the original type
     * was committed (MPI-2, section 8.8) */

    mtype->InitBuf = MTestTypeContigInit;
    mtype->FreeBuf = MTestTypeContigFree;
    mtype->CheckBuf = MTestTypeContigCheckbuf;

    return merr;
}


/*
 * General initialization for receive buffer.
 * Allocate buffer and initialize for reception (e.g., set initial data to detect failure)
 * Both basic and derived datatype can be handled by using extent as buffer size.
 */
void *MTestTypeInitRecv(MTestDatatype * mtype)
{
    MPI_Aint size;
    int merr;

    if (mtype->count > 0) {
        signed char *p;
        int i, totsize;
        merr = MPI_Type_extent(mtype->datatype, &size);
        if (merr)
            MTestPrintError(merr);
        totsize = size * mtype->count;
        if (!mtype->buf) {
            mtype->buf = (void *) malloc(totsize);
        }
        p = (signed char *) (mtype->buf);
        if (!p) {
            char errmsg[128] = { 0 };
            sprintf(errmsg, "Out of memory in %s", __FUNCTION__);
            MTestError(errmsg);
        }
        for (i = 0; i < totsize; i++) {
            p[i] = 0xff;
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

void MTestTypeCreatorInit(MTestDdtCreator * creators)
{
    memset(creators, 0, sizeof(MTestDdtCreator) * MTEST_DDT_MAX);
    creators[MTEST_DDT_CONTIGUOUS] = MTestTypeContiguousCreate;
    creators[MTEST_DDT_VECTOR] = MTestTypeVectorCreate;
    creators[MTEST_DDT_INDEXED] = MTestTypeIndexedCreate;
}

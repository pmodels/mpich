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

static int dbgflag = 0;         /* Flag used for debugging */
static int wrank = -1;          /* World rank */
static int verbose = 0;         /* Message level (0 is none) */

/*
 * Utility routines for writing MPI datatype communication tests.
 *
 * Both basic and derived datatype are included.
 * For basic datatypes, every type has a test case that both the send and
 * receive buffer use the same datatype and count.
 *
 *  For derived datatypes:
 *    All the test cases are defined in this file, and the datatype definitions
 *    are in file mtest_datatype.c. Each test case will be automatically called
 *    by every datatype.
 *
 *  Test case generation:
 *    Every datatype tests derived datatype send buffer and
 *    derived datatype receive buffer separately. Each test contains various sub
 *    tests for different structures (i.e., different value of count or block
 *    length). The following four structures are defined:
 *      L count & S block length & S stride
 *      S count & L block length & S stride
 *      L count & S block length & L stride
 *      S count & L block length & L stride
 *      S count & L block length & S stride & S lower-bound
 *      contiguous (stride = block length)
 *      contiguous (stride = block length) & S lower-bound
 *
 *  How to add a new structure for each datatype:
 *    1. Add structure definition in function MTestDdtStructDefine.
 *    2. Increase MTEST_DDT_NUM_SUBTESTS
 *
 *  Datatype definition:
 *    Every type is initialized by the creation function stored in
 *    mtestDdtCreators variable, all of their create/init/check functions are
 *    defined in file mtest_datatype.c.
 *
 *  How to add a new derived datatype:
 *    1. Add the new datatype in enum MTEST_DERIVED_DT.
 *    2. Add its create/init/check functions in file mtest_datatype.c
 *    3. Add its creator function to mtestDdtCreators variable
 *
 *  Following three test levels of datatype are defined.
 *    1. Basic
 *      All basic datatypes
 *    2. Minimum
 *      All basic datatypes | Vector | Indexed
 *    3. Full
 *      All basic datatypes | Vector | Hvector | Indexed | Hindexed |
 *      Indexed-block | Hindexed-block | Subarray with order-C | Subarray with order-Fortran
 *
 *  There are two ways to specify the test level of datatype. The second way has
 *  higher priority (means the value specified by the first way will be overwritten
 *  by that in the second way).
 *  1. Specify global test level by setting the MPITEST_DATATYPE_TEST_LEVEL
 *     environment variable before execution (basic,min,full|full by default).
 *  2. Initialize a special level for a datatype loop by calling the corresponding
 *     initialization function before that loop, otherwise the default value specified
 *     in the first way is used.
 *    Basic     : MTestInitBasicDatatypes
 *    Minimum   : MTestInitMinDatatypes
 *    Full      : MTestInitFullDatatypes
 */

static int datatype_index = 0;

/* ------------------------------------------------------------------------ */
/* Routine and internal parameters to define the range of datatype tests */
/* ------------------------------------------------------------------------ */

#define MTEST_DDT_NUM_SUBTESTS 7        /* 7 kinds of derived datatype structure */
static MTestDdtCreator mtestDdtCreators[MTEST_DDT_MAX];

static int MTEST_BDT_START_IDX = -1;
static int MTEST_BDT_NUM_TESTS = 0;
static int MTEST_BDT_RANGE = 0;

static int MTEST_DDT_NUM_TYPES = 0;
static int MTEST_SEND_DDT_START_IDX = 0;
static int MTEST_SEND_DDT_NUM_TESTS = 0;
static int MTEST_SEND_DDT_RANGE = 0;

static int MTEST_RECV_DDT_START_IDX = 0;
static int MTEST_RECV_DDT_NUM_TESTS = 0;
static int MTEST_RECV_DDT_RANGE = 0;

enum {
    MTEST_DATATYPE_TEST_LEVEL_FULL,
    MTEST_DATATYPE_TEST_LEVEL_MIN,
    MTEST_DATATYPE_TEST_LEVEL_BASIC
};

/* current datatype test level */
static int MTEST_DATATYPE_TEST_LEVEL = MTEST_DATATYPE_TEST_LEVEL_FULL;
/* default datatype test level specified by environment variable */
static int MTEST_DATATYPE_TEST_LEVEL_ENV = -1;
/* default datatype initialization function */
static void (*MTestInitDefaultTestFunc) (void) = NULL;

static void MTestInitDatatypeGen(int basic_dt_num, int derived_dt_num)
{
    MTEST_BDT_START_IDX = 0;
    MTEST_BDT_NUM_TESTS = basic_dt_num;
    MTEST_BDT_RANGE = MTEST_BDT_START_IDX + MTEST_BDT_NUM_TESTS;
    MTEST_DDT_NUM_TYPES = derived_dt_num;
    MTEST_SEND_DDT_START_IDX = MTEST_BDT_NUM_TESTS;
    MTEST_SEND_DDT_NUM_TESTS = MTEST_DDT_NUM_TYPES * MTEST_DDT_NUM_SUBTESTS;
    MTEST_SEND_DDT_RANGE = MTEST_SEND_DDT_START_IDX + MTEST_SEND_DDT_NUM_TESTS;
    MTEST_RECV_DDT_START_IDX = MTEST_SEND_DDT_START_IDX + MTEST_SEND_DDT_NUM_TESTS;
    MTEST_RECV_DDT_NUM_TESTS = MTEST_DDT_NUM_TYPES * MTEST_DDT_NUM_SUBTESTS;
    MTEST_RECV_DDT_RANGE = MTEST_RECV_DDT_START_IDX + MTEST_RECV_DDT_NUM_TESTS;
}

static int MTestIsDatatypeGenInited()
{
    return (MTEST_BDT_START_IDX < 0) ? 0 : 1;
}

static void MTestPrintDatatypeGen()
{
    MTestPrintfMsg(1, "MTest datatype test level : %s. %d basic datatype tests, "
                   "%d derived datatype tests will be generated\n",
                   (MTEST_DATATYPE_TEST_LEVEL == MTEST_DATATYPE_TEST_LEVEL_FULL) ? "FULL" : "MIN",
                   MTEST_BDT_NUM_TESTS, MTEST_SEND_DDT_NUM_TESTS + MTEST_RECV_DDT_NUM_TESTS);
}

static void MTestResetDatatypeGen()
{
    MTEST_BDT_START_IDX = -1;
}

void MTestInitFullDatatypes(void)
{
    /* Do not allow to change datatype test level during loop.
     * Otherwise indexes will be wrong.
     * Test must explicitly call reset or wait for current datatype loop being
     * done before changing to another test level. */
    if (!MTestIsDatatypeGenInited()) {
        MTEST_DATATYPE_TEST_LEVEL = MTEST_DATATYPE_TEST_LEVEL_FULL;
        MTestTypeCreatorInit((MTestDdtCreator *) mtestDdtCreators);
        MTestInitDatatypeGen(MTEST_BDT_MAX, MTEST_DDT_MAX);
    }
    else {
        printf("Warning: trying to reinitialize mtest datatype during " "datatype iteration!");
    }
}

void MTestInitMinDatatypes(void)
{
    /* Do not allow to change datatype test level during loop.
     * Otherwise indexes will be wrong.
     * Test must explicitly call reset or wait for current datatype loop being
     * done before changing to another test level. */
    if (!MTestIsDatatypeGenInited()) {
        MTEST_DATATYPE_TEST_LEVEL = MTEST_DATATYPE_TEST_LEVEL_MIN;
        MTestTypeMinCreatorInit((MTestDdtCreator *) mtestDdtCreators);
        MTestInitDatatypeGen(MTEST_BDT_MAX, MTEST_MIN_DDT_MAX);
    }
    else {
        printf("Warning: trying to reinitialize mtest datatype during " "datatype iteration!");
    }
}

void MTestInitBasicDatatypes(void)
{
    /* Do not allow to change datatype test level during loop.
     * Otherwise indexes will be wrong.
     * Test must explicitly call reset or wait for current datatype loop being
     * done before changing to another test level. */
    if (!MTestIsDatatypeGenInited()) {
        MTEST_DATATYPE_TEST_LEVEL = MTEST_DATATYPE_TEST_LEVEL_BASIC;
        MTestInitDatatypeGen(MTEST_BDT_MAX, 0);
    }
    else {
        printf("Warning: trying to reinitialize mtest datatype during " "datatype iteration!");
    }
}

static inline void MTestInitDatatypeEnv()
{
    char *envval = 0;

    /* Read global test level specified by user environment variable.
     * Only initialize once at the first time that test calls datatype routine. */
    if (MTEST_DATATYPE_TEST_LEVEL_ENV > -1)
        return;

    /* default full */
    MTEST_DATATYPE_TEST_LEVEL_ENV = MTEST_DATATYPE_TEST_LEVEL_FULL;
    MTestInitDefaultTestFunc = MTestInitFullDatatypes;

    envval = getenv("MPITEST_DATATYPE_TEST_LEVEL");
    if (envval && strlen(envval)) {
        if (!strncmp(envval, "min", strlen("min"))) {
            MTEST_DATATYPE_TEST_LEVEL_ENV = MTEST_DATATYPE_TEST_LEVEL_MIN;
            MTestInitDefaultTestFunc = MTestInitMinDatatypes;
        }
        else if (!strncmp(envval, "basic", strlen("basic"))) {
            MTEST_DATATYPE_TEST_LEVEL_ENV = MTEST_DATATYPE_TEST_LEVEL_BASIC;
            MTestInitDefaultTestFunc = MTestInitBasicDatatypes;
        }
        else if (strncmp(envval, "full", strlen("full"))) {
            fprintf(stderr, "Unknown MPITEST_DATATYPE_TEST_LEVEL %s\n", envval);
        }
    }
}

/* -------------------------------------------------------------------------------*/
/* Routine to define various sets of blocklen/count/stride for derived datatypes. */
/* ------------------------------------------------------------------------------ */

static inline int MTestDdtStructDefine(int ddt_index, MPI_Aint tot_count, MPI_Aint * count,
                                       MPI_Aint * blen, MPI_Aint * stride,
                                       MPI_Aint * align_tot_count, MPI_Aint * lb)
{
    int merr = 0;
    int ddt_c_st;
    MPI_Aint _short = 0, _align_tot_count = 0, _count = 0, _blen = 0, _stride = 0;
    MPI_Aint _lb = 0;

    ddt_c_st = ddt_index % MTEST_DDT_NUM_SUBTESTS;

    /* Get short value according to user specified tot_count.
     * It is used as count for large-block-length structure, or block length
     * for large-count structure. */
    if (tot_count < 2) {
        _short = 1;
    }
    else if (tot_count < 64) {
        _short = 2;
    }
    else {
        _short = 64;
    }
    _align_tot_count = (tot_count + _short - 1) & ~(_short - 1);

    switch (ddt_c_st) {
    case 0:
        /* Large block length. */
        _count = _short;
        _blen = _align_tot_count / _short;
        _stride = _blen * 2;
        break;
    case 1:
        /* Large count */
        _count = _align_tot_count / _short;
        _blen = _short;
        _stride = _blen * 2;
        break;
    case 2:
        /* Large block length and large stride */
        _count = _short;
        _blen = _align_tot_count / _short;
        _stride = _blen * 10;
        break;
    case 3:
        /* Large count and large stride */
        _count = _align_tot_count / _short;
        _blen = _short;
        _stride = _blen * 10;
        break;
    case 4:
        /* Large block length with lb */
        _count = _short;
        _blen = _align_tot_count / _short;
        _stride = _blen * 2;
        _lb = _short / 2;       /* make sure lb < blen */
        break;
    case 5:
        /* Contig ddt (stride = block length) without lb */
        _count = _align_tot_count / _short;
        _blen = _short;
        _stride = _blen;
        break;
    case 6:
        /* Contig ddt (stride = block length) with lb */
        _count = _short;
        _blen = _align_tot_count / _short;
        _stride = _blen;
        _lb = _short / 2;       /* make sure lb < blen */
        break;
    default:
        /* Undefined index */
        merr = 1;
        break;
    }

    *align_tot_count = _align_tot_count;
    *count = _count;
    *blen = _blen;
    *stride = _stride;
    *lb = _lb;

    return merr;
}

/* ------------------------------------------------------------------------ */
/* Routine to generate basic datatypes                                       */
/* ------------------------------------------------------------------------ */

static inline int MTestGetBasicDatatypes(MTestDatatype * sendtype,
                                         MTestDatatype * recvtype, MPI_Aint tot_count)
{
    int merr = 0;
    int bdt_index = datatype_index - MTEST_BDT_START_IDX;
    if (bdt_index >= MTEST_BDT_MAX) {
        printf("Wrong index:  global %d, bst %d in %s\n", datatype_index, bdt_index, __FUNCTION__);
        merr++;
        return merr;
    }

    switch (bdt_index) {
    case MTEST_BDT_INT:
        merr = MTestTypeBasicCreate(MPI_INT, sendtype);
        merr = MTestTypeBasicCreate(MPI_INT, recvtype);
        break;
    case MTEST_BDT_DOUBLE:
        merr = MTestTypeBasicCreate(MPI_DOUBLE, sendtype);
        merr = MTestTypeBasicCreate(MPI_DOUBLE, recvtype);
        break;
    case MTEST_BDT_FLOAT_INT:
        merr = MTestTypeBasicCreate(MPI_FLOAT_INT, sendtype);
        merr = MTestTypeBasicCreate(MPI_FLOAT_INT, recvtype);
        break;
    case MTEST_BDT_SHORT:
        merr = MTestTypeBasicCreate(MPI_SHORT, sendtype);
        merr = MTestTypeBasicCreate(MPI_SHORT, recvtype);
        break;
    case MTEST_BDT_LONG:
        merr = MTestTypeBasicCreate(MPI_LONG, sendtype);
        merr = MTestTypeBasicCreate(MPI_LONG, recvtype);
        break;
    case MTEST_BDT_CHAR:
        merr = MTestTypeBasicCreate(MPI_CHAR, sendtype);
        merr = MTestTypeBasicCreate(MPI_CHAR, recvtype);
        break;
    case MTEST_BDT_UINT64_T:
        merr = MTestTypeBasicCreate(MPI_UINT64_T, sendtype);
        merr = MTestTypeBasicCreate(MPI_UINT64_T, recvtype);
        break;
    case MTEST_BDT_FLOAT:
        merr = MTestTypeBasicCreate(MPI_FLOAT, sendtype);
        merr = MTestTypeBasicCreate(MPI_FLOAT, recvtype);
        break;
    case MTEST_BDT_BYTE:
        merr = MTestTypeBasicCreate(MPI_BYTE, sendtype);
        merr = MTestTypeBasicCreate(MPI_BYTE, recvtype);
        break;
    }
    sendtype->count = tot_count;
    recvtype->count = tot_count;

    return merr;
}

/* ------------------------------------------------------------------------ */
/* Routine to generate send/receive derived datatypes                     */
/* ------------------------------------------------------------------------ */

static inline int MTestGetSendDerivedDatatypes(MTestDatatype * sendtype,
                                               MTestDatatype * recvtype, MPI_Aint tot_count)
{
    int merr = 0;
    int ddt_datatype_index, ddt_c_dt;
    MPI_Aint blen, stride, count, align_tot_count, lb;
    MPI_Datatype old_type = MPI_DOUBLE;

    /* Check index */
    ddt_datatype_index = datatype_index - MTEST_SEND_DDT_START_IDX;
    ddt_c_dt = ddt_datatype_index / MTEST_DDT_NUM_SUBTESTS;
    if (ddt_c_dt >= MTEST_DDT_MAX || !mtestDdtCreators[ddt_c_dt]) {
        printf("Wrong index:  global %d, send %d send-ddt %d, or undefined creator in %s\n",
               datatype_index, ddt_datatype_index, ddt_c_dt, __FUNCTION__);
        merr++;
        return merr;
    }

    /* Set datatype structure */
    merr = MTestDdtStructDefine(ddt_datatype_index, tot_count, &count, &blen,
                                &stride, &align_tot_count, &lb);
    if (merr) {
        printf("Wrong index:  global %d, send %d send-ddt %d, or undefined ddt structure in %s\n",
               datatype_index, ddt_datatype_index, ddt_c_dt, __FUNCTION__);
        merr++;
        return merr;
    }

    /* Create send datatype */
    merr = mtestDdtCreators[ddt_c_dt] (count, blen, stride, lb, old_type, "send", sendtype);
    if (merr)
        return merr;

    sendtype->count = 1;

    /* Create receive datatype */
    merr = MTestTypeBasicCreate(old_type, recvtype);
    if (merr)
        return merr;

    recvtype->count = sendtype->count * align_tot_count;

    return merr;
}

static inline int MTestGetRecvDerivedDatatypes(MTestDatatype * sendtype,
                                               MTestDatatype * recvtype, MPI_Aint tot_count)
{
    int merr = 0;
    int ddt_datatype_index, ddt_c_dt;
    MPI_Aint blen, stride, count, align_tot_count, lb;
    MPI_Datatype old_type = MPI_DOUBLE;

    /* Check index */
    ddt_datatype_index = datatype_index - MTEST_RECV_DDT_START_IDX;
    ddt_c_dt = ddt_datatype_index / MTEST_DDT_NUM_SUBTESTS;
    if (ddt_c_dt >= MTEST_DDT_MAX || !mtestDdtCreators[ddt_c_dt]) {
        printf("Wrong index:  global %d, recv %d recv-ddt %d, or undefined creator in %s\n",
               datatype_index, ddt_datatype_index, ddt_c_dt, __FUNCTION__);
        merr++;
        return merr;
    }

    /* Set datatype structure */
    merr = MTestDdtStructDefine(ddt_datatype_index, tot_count, &count, &blen,
                                &stride, &align_tot_count, &lb);
    if (merr) {
        printf("Wrong index:  global %d, recv %d recv-ddt %d, or undefined ddt structure in %s\n",
               datatype_index, ddt_datatype_index, ddt_c_dt, __FUNCTION__);
        return merr;
    }

    /* Create receive datatype */
    merr = mtestDdtCreators[ddt_c_dt] (count, blen, stride, lb, old_type, "recv", recvtype);
    if (merr)
        return merr;

    recvtype->count = 1;

    /* Create send datatype */
    merr = MTestTypeBasicCreate(old_type, sendtype);
    if (merr)
        return merr;

    sendtype->count = recvtype->count * align_tot_count;

    return merr;
}

/* ------------------------------------------------------------------------ */
/* Exposed routine to external tests                                         */
/* ------------------------------------------------------------------------ */
int MTestGetDatatypes(MTestDatatype * sendtype, MTestDatatype * recvtype, MPI_Aint tot_count)
{
    int merr = 0;

    MTestGetDbgInfo(&dbgflag, &verbose);
    MTestInitDatatypeEnv();
    MPI_Comm_rank(MPI_COMM_WORLD, &wrank);

    /* Initialize the default test level if test does not specify. */
    if (!MTestIsDatatypeGenInited()) {
        MTestInitDefaultTestFunc();
    }

    if (datatype_index == 0) {
        MTestPrintDatatypeGen();
    }

    /* Start generating tests */
    if (datatype_index < MTEST_BDT_RANGE) {
        merr = MTestGetBasicDatatypes(sendtype, recvtype, tot_count);

    }
    else if (datatype_index < MTEST_SEND_DDT_RANGE) {
        merr = MTestGetSendDerivedDatatypes(sendtype, recvtype, tot_count);

    }
    else if (datatype_index < MTEST_RECV_DDT_RANGE) {
        merr = MTestGetRecvDerivedDatatypes(sendtype, recvtype, tot_count);

    }
    else {
        /* out of range */
        datatype_index = -1;
        MTestResetDatatypeGen();
    }

    /* stop if error reported */
    if (merr) {
        datatype_index = -1;
    }

    if (datatype_index > 0) {
        /* general initialization for receive buffer. */
        recvtype->InitBuf = MTestTypeInitRecv;
    }

    datatype_index++;

    if (verbose >= 2 && datatype_index > 0) {
        MPI_Count ssize, rsize;
        MPI_Aint slb, rlb, sextent, rextent;
        const char *sendtype_nm = MTestGetDatatypeName(sendtype);
        const char *recvtype_nm = MTestGetDatatypeName(recvtype);
        MPI_Type_size_x(sendtype->datatype, &ssize);
        MPI_Type_size_x(recvtype->datatype, &rsize);

        MPI_Type_get_extent(sendtype->datatype, &slb, &sextent);
        MPI_Type_get_extent(recvtype->datatype, &rlb, &rextent);

        MTestPrintfMsg(2, "Get datatypes: send = %s(size %d ext %ld lb %ld count %d basesize %d), "
                       "recv = %s(size %d ext %ld lb %ld count %d basesize %d), tot_count=%d\n",
                       sendtype_nm, ssize, sextent, slb, sendtype->count, sendtype->basesize,
                       recvtype_nm, rsize, rextent, rlb, recvtype->count, recvtype->basesize,
                       tot_count);
        fflush(stdout);
    }

    return datatype_index;
}

/* Reset the datatype index (start from the initial data type.
   Note: This routine is rarely needed; MTestGetDatatypes automatically
   starts over after the last available datatype is used.
*/
void MTestResetDatatypes(void)
{
    datatype_index = 0;
    MTestResetDatatypeGen();
}

/* Return the index of the current datatype.  This is rarely needed and
   is provided mostly to enable debugging of the MTest package itself */
int MTestGetDatatypeIndex(void)
{
    return datatype_index;
}

/* Free the storage associated with a datatype */
void MTestFreeDatatype(MTestDatatype * mtype)
{
    int merr;
    /* Invoke a datatype-specific free function to handle
     * both the datatype and the send/receive buffers */
    if (mtype->FreeBuf) {
        (mtype->FreeBuf) (mtype);
    }
    /* Free the datatype itself if it was created */
    if (!mtype->isBasic) {
        merr = MPI_Type_free(&mtype->datatype);
        if (merr)
            MTestPrintError(merr);
    }
}

/* Check that a message was received correctly.  Returns the number of
   errors detected.  Status may be NULL or MPI_STATUS_IGNORE */
int MTestCheckRecv(MPI_Status * status, MTestDatatype * recvtype)
{
    int count;
    int errs = 0, merr;

    if (status && status != MPI_STATUS_IGNORE) {
        merr = MPI_Get_count(status, recvtype->datatype, &count);
        if (merr)
            MTestPrintError(merr);

        /* Check count against expected count */
        if (count != recvtype->count) {
            errs++;
        }
    }

    /* Check received data */
    if (!errs && recvtype->CheckBuf(recvtype)) {
        errs++;
    }
    return errs;
}

/* This next routine uses a circular buffer of static name arrays just to
   simplify the use of the routine */
const char *MTestGetDatatypeName(MTestDatatype * dtype)
{
    static char name[4][MPI_MAX_OBJECT_NAME];
    static int sp = 0;
    int rlen, merr;

    if (sp >= 4)
        sp = 0;
    merr = MPI_Type_get_name(dtype->datatype, name[sp], &rlen);
    if (merr)
        MTestPrintError(merr);
    return (const char *) name[sp++];
}

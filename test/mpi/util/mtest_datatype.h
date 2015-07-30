/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2014 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MTEST_DATATYPE_H_
#define MTEST_DATATYPE_H_

#include "mpi.h"
#include "mpitestconf.h"
#include "mpitest.h"

/* Provide backward portability to MPI 1 */
#ifndef MPI_VERSION
#define MPI_VERSION 1
#endif

enum MTEST_BASIC_DT {
    MTEST_BDT_INT,
    MTEST_BDT_DOUBLE,
    MTEST_BDT_FLOAT_INT,
    MTEST_BDT_SHORT,
    MTEST_BDT_LONG,
    MTEST_BDT_CHAR,
    MTEST_BDT_UINT64_T,
    MTEST_BDT_FLOAT,
    MTEST_BDT_BYTE,
    MTEST_BDT_MAX
};

enum MTEST_DERIVED_DT {
    MTEST_DDT_CONTIGUOUS,
    MTEST_DDT_VECTOR,
    MTEST_DDT_HVECTOR,
    MTEST_DDT_INDEXED,
    MTEST_DDT_HINDEXED,
    MTEST_DDT_INDEXED_BLOCK,
    MTEST_DDT_HINDEXED_BLOCK,
    MTEST_DDT_STRUCT,
    MTEST_DDT_SUBARRAY_ORDER_C,
    MTEST_DDT_SUBARRAY_ORDER_FORTRAN,
    MTEST_DDT_MAX
};

enum MTEST_MIN_DERIVED_DT {
    MTEST_MIN_DDT_VECTOR,
    MTEST_MIN_DDT_INDEXED,
    MTEST_MIN_DDT_MAX
};

typedef int (*MTestDdtCreator) (int, int, int, int, MPI_Datatype, const char *, MTestDatatype *);

extern void MTestTypeCreatorInit(MTestDdtCreator * creators);
extern void MTestTypeMinCreatorInit(MTestDdtCreator * creators);
extern void *MTestTypeInitRecv(MTestDatatype * mtype);

extern int MTestTypeBasicCreate(MPI_Datatype oldtype, MTestDatatype * mtype);
extern int MTestTypeDupCreate(MPI_Datatype oldtype, MTestDatatype * mtype);

#endif /* MTEST_DATATYPE_H_ */

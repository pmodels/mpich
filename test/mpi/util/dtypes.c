/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2014 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include "mpitestconf.h"
#include "mpitest.h"
#include "dtypes.h"
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

/* This file contains code to generate a variety of MPI datatypes for testing
   the various MPI routines.

   To simplify the test code, this generates an array of datatypes, buffers with
   data and buffers with no data (0 bits) for use in send and receive
   routines of various types.

   In addition, this doesn't even test all of the possibilities.  For example,
   there is currently no test of sending more than one item defined with
   MPI_Type_contiguous .

   Note also that this test assumes that the sending and receive types are
   the same.  MPI requires only that the type signatures match, which is
   a weaker requirement.

   This code was drawn from the MPICH-1 test suite and modified to fit the
   new MPICH test suite.  It provides an alternative set of datatype tests
   to the ones in mtest.c.

 */

/* Change this to test only the basic, predefined types */
static int basic_only = 0;

/*
   Arrays types, inbufs, outbufs, and counts are allocated by the
   CALLER.  n on input is the maximum number; on output, it is the
   number defined.

   See MTestDatatype2Allocate below for a routine to allocate these arrays.

   We may want to add a routine to call to check that the proper data
   has been received.
 */

/*
   Add a predefined MPI type to the tests.  _count instances of the
   type will be sent.
*/
#define SETUPBASICTYPE(_mpitype,_ctype,_count) { \
  int i; _ctype *a;	\
  if (cnt > *n) {*n = cnt; return; }			\
  types[cnt] = _mpitype; \
  inbufs[cnt] = (void *)calloc(_count,sizeof(_ctype)); \
  outbufs[cnt] = (void *)malloc(sizeof(_ctype) * (_count));	\
  a = (_ctype *)inbufs[cnt]; for (i=0; i<(_count); i++) a[i] = i;	\
  a = (_ctype *)outbufs[cnt]; for (i=0; i<(_count); i++) a[i] = 0;	\
  counts[cnt]  = _count; bytesize[cnt] = sizeof(_ctype) * (_count); cnt++; }

/*
   Add a contiguous version of a predefined type.  Send one instance of
   the type which contains _count copies of the predefined type.
 */
#define SETUPCONTIGTYPE(_mpitype,_ctype,_count) { \
  int i; _ctype *a; char*myname; \
  char _basename[MPI_MAX_OBJECT_NAME]; int _basenamelen;\
  if (cnt > *n) {*n = cnt; return; }\
  MPI_Type_contiguous(_count, _mpitype, types + cnt);\
  MPI_Type_commit(types + cnt);\
  inbufs[cnt] = (void *)calloc(_count, sizeof(_ctype)); \
  outbufs[cnt] = (void *)malloc(sizeof(_ctype) * (_count));	\
  a = (_ctype *)inbufs[cnt]; for (i=0; i<(_count); i++) a[i] = i;	\
  a = (_ctype *)outbufs[cnt]; for (i=0; i<(_count); i++) a[i] = 0;	\
  myname = (char *)malloc(100);\
  MPI_Type_get_name(_mpitype, _basename, &_basenamelen); \
  snprintf(myname, 100, "Contig type %s", _basename);	\
  MPI_Type_set_name(types[cnt], myname); \
  free(myname); \
  counts[cnt]  = 1;  bytesize[cnt] = sizeof(_ctype) * (_count); cnt++; }

/*
  Create a vector with _count elements, separated by stride _stride,
  of _mpitype.  Each block has a single element.
 */
#define SETUPVECTORTYPE(_mpitype,_ctype,_count,_stride,_name) { \
  int i; _ctype *a; char *myname;				\
  char _basename[MPI_MAX_OBJECT_NAME]; int _basenamelen;\
  if (cnt > *n) {*n = cnt; return; }\
  MPI_Type_vector(_count, 1, _stride, _mpitype, types + cnt);	\
  MPI_Type_commit(types + cnt);\
  inbufs[cnt] = (void *)calloc(sizeof(_ctype) * (_count) * (_stride),1); \
  outbufs[cnt] = (void *)calloc(sizeof(_ctype) * (_count) * (_stride),1); \
  a = (_ctype *)inbufs[cnt]; for (i=0; i<(_count); i++) a[i*(_stride)] = i; \
  a = (_ctype *)outbufs[cnt]; for (i=0; i<(_count); i++) a[i*(_stride)] = 0; \
  myname = (char *)malloc(100);\
  MPI_Type_get_name(_mpitype, _basename, &_basenamelen); \
  snprintf(myname, 100, "Vector type %s", _basename);		\
  MPI_Type_set_name(types[cnt], myname); \
  free(myname); \
  counts[cnt]  = 1; bytesize[cnt] = sizeof(_ctype) * (_count) * (_stride) ;\
  cnt++; }

/* This indexed type is setup like a contiguous type .
   Note that systems may try to convert this to contiguous, so we'll
   eventually need a test that has holes in it */
#define SETUPINDEXTYPE(_mpitype,_ctype,_count,_name) { \
  int i; int *lens, *disp; _ctype *a; char *myname;	\
  char _basename[MPI_MAX_OBJECT_NAME]; int _basenamelen;\
  if (cnt > *n) {*n = cnt; return; }\
  lens = (int *)malloc((_count) * sizeof(int)); \
  disp = (int *)malloc((_count) * sizeof(int)); \
  for (i=0; i<(_count); i++) { lens[i] = 1; disp[i] = i; } \
  MPI_Type_indexed((_count), lens, disp, _mpitype, types + cnt);\
  free(lens); free(disp); \
  MPI_Type_commit(types + cnt);\
  inbufs[cnt] = (void *)calloc((_count), sizeof(_ctype)); \
  outbufs[cnt] = (void *)malloc(sizeof(_ctype) * (_count)); \
  a = (_ctype *)inbufs[cnt]; for (i=0; i<(_count); i++) a[i] = i; \
  a = (_ctype *)outbufs[cnt]; for (i=0; i<(_count); i++) a[i] = 0; \
  myname = (char *)malloc(100);\
  MPI_Type_get_name(_mpitype, _basename, &_basenamelen); \
  snprintf(myname, 100, "Index type %s", _basename);		\
  MPI_Type_set_name(types[cnt], myname); \
  free(myname); \
  counts[cnt]  = 1;  bytesize[cnt] = sizeof(_ctype) * (_count); cnt++; }

/* This defines a structure of two basic members; by chosing things like
   (char, double), various packing and alignment tests can be made */
#define SETUPSTRUCT2TYPE(_mpitype1,_ctype1,_mpitype2,_ctype2,_count,_tname) { \
  int i; char *myname;						\
  MPI_Datatype b[3]; int cnts[3]; \
  struct name { _ctype1 a1; _ctype2 a2; } *a, samp;	\
  MPI_Aint disp[3];				\
  if (cnt > *n) {*n = cnt; return; }					\
  b[0] = _mpitype1; b[1] = _mpitype2; b[2] = MPI_UB;	\
  cnts[0] = 1; cnts[1] = 1; cnts[2] = 1;	\
  MPI_Get_address(&(samp.a2), &disp[1]);		\
  MPI_Get_address(&(samp.a1), &disp[0]);		\
  MPI_Get_address(&(samp) + 1, &disp[2]);	        \
  disp[1] = disp[1] - disp[0]; disp[2] = disp[2] - disp[0]; disp[0] = 0; \
  MPI_Type_create_struct(3, cnts, disp, b, types + cnt);		\
  MPI_Type_commit(types + cnt);					\
  inbufs[cnt] = (void *)calloc(sizeof(struct name) * (_count),1);	\
  outbufs[cnt] = (void *)calloc(sizeof(struct name) * (_count),1);	\
  a = (struct name *)inbufs[cnt]; for (i=0; i<(_count); i++) { a[i].a1 = i; \
      a[i].a2 = i; }							\
  a = (struct name *)outbufs[cnt]; for (i=0; i<(_count); i++) { a[i].a1 = 0; \
      a[i].a2 = 0; }							\
  myname = (char *)malloc(100);					\
  snprintf(myname, 100, "Struct type %s", _tname);		\
  MPI_Type_set_name(types[cnt], myname); \
  free(myname); \
  counts[cnt]  = (_count);  bytesize[cnt] = sizeof(struct name) * (_count);cnt++; }

/* This accomplished the same effect as VECTOR, but allow a count of > 1 */
#define SETUPSTRUCTTYPEUB(_mpitype,_ctype,_count,_stride) {	\
  int i; _ctype *a; char *myname;					\
  int blens[2];  MPI_Aint disps[2]; MPI_Datatype mtypes[2];	\
  char _basename[MPI_MAX_OBJECT_NAME]; int _basenamelen;\
  if (cnt > *n) {*n = cnt; return; }					\
  blens[0] = 1; blens[1] = 1; disps[0] = 0; \
  disps[1] = (_stride) * sizeof(_ctype); \
  mtypes[0] = _mpitype; mtypes[1] = MPI_UB;				\
  MPI_Type_create_struct(2, blens, disps, mtypes, types + cnt);	\
  MPI_Type_commit(types + cnt);					\
  inbufs[cnt] = (void *)calloc(sizeof(_ctype) * (_count) * (_stride),1);\
  outbufs[cnt] = (void *)calloc(sizeof(_ctype) * (_count) * (_stride),1);\
  a = (_ctype *)inbufs[cnt]; for (i=0; i<(_count); i++) a[i*(_stride)] = i;  \
  a = (_ctype *)outbufs[cnt]; for (i=0; i<(_count); i++) a[i*(_stride)] = 0; \
  myname = (char *)malloc(100);					\
  MPI_Type_get_name(_mpitype, _basename, &_basenamelen); \
  snprintf(myname, 100, "Struct (MPI_UB) type %s", _basename);	\
  MPI_Type_set_name(types[cnt], myname); \
  free(myname); \
  counts[cnt]  = (_count);  \
  bytesize[cnt] = sizeof(_ctype) * (_count) * (_stride);\
  cnt++; }

/*
 * Set whether only the basic types should be generated
 */
void MTestDatatype2BasicOnly(void)
{
    basic_only = 1;
}

static int nbasic_types = 0;
/* On input, n is the size of the various buffers.  On output,
   it is the number available types
 */
void MTestDatatype2Generate(MPI_Datatype * types, void **inbufs, void **outbufs,
                            int *counts, int *bytesize, int *n)
{
    int cnt = 0;                /* Number of defined types */
    int typecnt = 10;           /* Number of instances to send in most cases */
    int stride = 9;             /* Number of elements in vector to stride */

    /* First, generate an element of each basic type */
    SETUPBASICTYPE(MPI_CHAR, char, typecnt);
    SETUPBASICTYPE(MPI_SHORT, short, typecnt);
    SETUPBASICTYPE(MPI_INT, int, typecnt);
    SETUPBASICTYPE(MPI_LONG, long, typecnt);
    SETUPBASICTYPE(MPI_UNSIGNED_CHAR, unsigned char, typecnt);
    SETUPBASICTYPE(MPI_UNSIGNED_SHORT, unsigned short, typecnt);
    SETUPBASICTYPE(MPI_UNSIGNED, unsigned, typecnt);
    SETUPBASICTYPE(MPI_UNSIGNED_LONG, unsigned long, typecnt);
    SETUPBASICTYPE(MPI_FLOAT, float, typecnt);
    SETUPBASICTYPE(MPI_DOUBLE, double, typecnt);
    SETUPBASICTYPE(MPI_BYTE, char, typecnt);
#ifdef HAVE_LONG_LONG_INT
    SETUPBASICTYPE(MPI_LONG_LONG_INT, long long, typecnt);
#endif
#ifdef HAVE_LONG_DOUBLE
    SETUPBASICTYPE(MPI_LONG_DOUBLE, long double, typecnt);
#endif
    nbasic_types = cnt;

    if (basic_only) {
        *n = cnt;
        return;
    }
    /* Generate contiguous data items */
    SETUPCONTIGTYPE(MPI_CHAR, char, typecnt);
    SETUPCONTIGTYPE(MPI_SHORT, short, typecnt);
    SETUPCONTIGTYPE(MPI_INT, int, typecnt);
    SETUPCONTIGTYPE(MPI_LONG, long, typecnt);
    SETUPCONTIGTYPE(MPI_UNSIGNED_CHAR, unsigned char, typecnt);
    SETUPCONTIGTYPE(MPI_UNSIGNED_SHORT, unsigned short, typecnt);
    SETUPCONTIGTYPE(MPI_UNSIGNED, unsigned, typecnt);
    SETUPCONTIGTYPE(MPI_UNSIGNED_LONG, unsigned long, typecnt);
    SETUPCONTIGTYPE(MPI_FLOAT, float, typecnt);
    SETUPCONTIGTYPE(MPI_DOUBLE, double, typecnt);
    SETUPCONTIGTYPE(MPI_BYTE, char, typecnt);
#ifdef HAVE_LONG_LONG_INT
    SETUPCONTIGTYPE(MPI_LONG_LONG_INT, long long, typecnt);
#endif
#ifdef HAVE_LONG_DOUBLE
    SETUPCONTIGTYPE(MPI_LONG_DOUBLE, long double, typecnt);
#endif

    /* Generate vector items */
    SETUPVECTORTYPE(MPI_CHAR, char, typecnt, stride, "MPI_CHAR");
    SETUPVECTORTYPE(MPI_SHORT, short, typecnt, stride, "MPI_SHORT");
    SETUPVECTORTYPE(MPI_INT, int, typecnt, stride, "MPI_INT");
    SETUPVECTORTYPE(MPI_LONG, long, typecnt, stride, "MPI_LONG");
    SETUPVECTORTYPE(MPI_UNSIGNED_CHAR, unsigned char, typecnt, stride, "MPI_UNSIGNED_CHAR");
    SETUPVECTORTYPE(MPI_UNSIGNED_SHORT, unsigned short, typecnt, stride, "MPI_UNSIGNED_SHORT");
    SETUPVECTORTYPE(MPI_UNSIGNED, unsigned, typecnt, stride, "MPI_UNSIGNED");
    SETUPVECTORTYPE(MPI_UNSIGNED_LONG, unsigned long, typecnt, stride, "MPI_UNSIGNED_LONG");
    SETUPVECTORTYPE(MPI_FLOAT, float, typecnt, stride, "MPI_FLOAT");
    SETUPVECTORTYPE(MPI_DOUBLE, double, typecnt, stride, "MPI_DOUBLE");
    SETUPVECTORTYPE(MPI_BYTE, char, typecnt, stride, "MPI_BYTE");
#ifdef HAVE_LONG_LONG_INT
    SETUPVECTORTYPE(MPI_LONG_LONG_INT, long long, typecnt, stride, "MPI_LONG_LONG_INT");
#endif
#ifdef HAVE_LONG_DOUBLE
    SETUPVECTORTYPE(MPI_LONG_DOUBLE, long double, typecnt, stride, "MPI_LONG_DOUBLE");
#endif

    /* Generate indexed items */
    SETUPINDEXTYPE(MPI_CHAR, char, typecnt, "MPI_CHAR");
    SETUPINDEXTYPE(MPI_SHORT, short, typecnt, "MPI_SHORT");
    SETUPINDEXTYPE(MPI_INT, int, typecnt, "MPI_INT");
    SETUPINDEXTYPE(MPI_LONG, long, typecnt, "MPI_LONG");
    SETUPINDEXTYPE(MPI_UNSIGNED_CHAR, unsigned char, typecnt, "MPI_UNSIGNED_CHAR");
    SETUPINDEXTYPE(MPI_UNSIGNED_SHORT, unsigned short, typecnt, "MPI_UNSIGNED_SHORT");
    SETUPINDEXTYPE(MPI_UNSIGNED, unsigned, typecnt, "MPI_UNSIGNED");
    SETUPINDEXTYPE(MPI_UNSIGNED_LONG, unsigned long, typecnt, "MPI_UNSIGNED_LONG");
    SETUPINDEXTYPE(MPI_FLOAT, float, typecnt, "MPI_FLOAT");
    SETUPINDEXTYPE(MPI_DOUBLE, double, typecnt, "MPI_DOUBLE");
    SETUPINDEXTYPE(MPI_BYTE, char, typecnt, "MPI_BYTE");
#ifdef HAVE_LONG_LONG_INT
    SETUPINDEXTYPE(MPI_LONG_LONG_INT, long long, typecnt, "MPI_LONG_LONG_INT");
#endif
#ifdef HAVE_LONG_DOUBLE
    SETUPINDEXTYPE(MPI_LONG_DOUBLE, long double, typecnt, "MPI_LONG_DOUBLE");
#endif

    /* Generate struct items */
    SETUPSTRUCT2TYPE(MPI_CHAR, char, MPI_DOUBLE, double, typecnt, "char-double");
    SETUPSTRUCT2TYPE(MPI_DOUBLE, double, MPI_CHAR, char, typecnt, "double-char");
    SETUPSTRUCT2TYPE(MPI_UNSIGNED, unsigned, MPI_DOUBLE, double, typecnt, "unsigned-double");
    SETUPSTRUCT2TYPE(MPI_FLOAT, float, MPI_LONG, long, typecnt, "float-long");
    SETUPSTRUCT2TYPE(MPI_UNSIGNED_CHAR, unsigned char, MPI_CHAR, char, typecnt,
                     "unsigned char-char");
    SETUPSTRUCT2TYPE(MPI_UNSIGNED_SHORT, unsigned short, MPI_DOUBLE, double,
                     typecnt, "unsigned short-double");

    /* Generate struct using MPI_UB */
    SETUPSTRUCTTYPEUB(MPI_CHAR, char, typecnt, stride);
    SETUPSTRUCTTYPEUB(MPI_SHORT, short, typecnt, stride);
    SETUPSTRUCTTYPEUB(MPI_INT, int, typecnt, stride);
    SETUPSTRUCTTYPEUB(MPI_LONG, long, typecnt, stride);
    SETUPSTRUCTTYPEUB(MPI_UNSIGNED_CHAR, unsigned char, typecnt, stride);
    SETUPSTRUCTTYPEUB(MPI_UNSIGNED_SHORT, unsigned short, typecnt, stride);
    SETUPSTRUCTTYPEUB(MPI_UNSIGNED, unsigned, typecnt, stride);
    SETUPSTRUCTTYPEUB(MPI_UNSIGNED_LONG, unsigned long, typecnt, stride);
    SETUPSTRUCTTYPEUB(MPI_FLOAT, float, typecnt, stride);
    SETUPSTRUCTTYPEUB(MPI_DOUBLE, double, typecnt, stride);
    SETUPSTRUCTTYPEUB(MPI_BYTE, char, typecnt, stride);

    /* 60 different entries to this point + 4 for long long and
     * 4 for long double */
    *n = cnt;
}

/*
   MAX_TEST should be 1 + actual max (allows us to check that it was,
   indeed, large enough)
 */
#define MAX_TEST 70
void MTestDatatype2Allocate(MPI_Datatype ** types, void ***inbufs,
                            void ***outbufs, int **counts, int **bytesize, int *n)
{
    *types = (MPI_Datatype *) malloc(MAX_TEST * sizeof(MPI_Datatype));
    *inbufs = (void **) malloc(MAX_TEST * sizeof(void *));
    *outbufs = (void **) malloc(MAX_TEST * sizeof(void *));
    *counts = (int *) malloc(MAX_TEST * sizeof(int));
    *bytesize = (int *) malloc(MAX_TEST * sizeof(int));
    *n = MAX_TEST;
}

int MTestDatatype2Check(void *inbuf, void *outbuf, int size_bytes)
{
    char *in = (char *) inbuf, *out = (char *) outbuf;
    int i;
    for (i = 0; i < size_bytes; i++) {
        if (in[i] != out[i]) {
            return i + 1;
        }
    }
    return 0;
}

/*
 * This is a version of CheckData that prints error messages
 */
int MtestDatatype2CheckAndPrint(void *inbuf, void *outbuf, int size_bytes,
                                char *typename, int typenum)
{
    int errloc, world_rank;

    if ((errloc = MTestDatatype2Check(inbuf, outbuf, size_bytes))) {
        char *p1, *p2;
        MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
        fprintf(stderr,
                "Error in data with type %s (type %d on %d) at byte %d of %d\n",
                typename, typenum, world_rank, errloc - 1, size_bytes);
        p1 = (char *) inbuf;
        p2 = (char *) outbuf;
        fprintf(stderr, "Got %x expected %x\n", p2[errloc - 1], p1[errloc - 1]);
    }
    return errloc;
}

void MTestDatatype2Free(MPI_Datatype * types, void **inbufs, void **outbufs,
                        int *counts, int *bytesize, int n)
{
    int i;
    for (i = 0; i < n; i++) {
        if (inbufs[i])
            free(inbufs[i]);
        if (outbufs[i])
            free(outbufs[i]);
        /* Only if not basic ... */
        if (i >= nbasic_types)
            MPI_Type_free(types + i);
    }
    free(inbufs);
    free(outbufs);
    free(counts);
    free(bytesize);
}

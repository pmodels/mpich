/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include <math.h>
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>

/* Run this test on 2, 4, 6, 8, or 10 processes only */

int count, errcnt = 0, gerr = 0, size, rank;
MPI_Comm comm;

void a1(void),  b1(void),  c1(void),  d1(void),  e1(void),  f1(void),  g1(void),  h1(void),  a2(void),  b2(void),  c2(void),  d2(void),  e2(void),  f2(void),  g2(void),  h2(void),  a3(void),  b3(void),  c3(void),  d3(void),  e3(void),  f3(void),  g3(void),  h3(void),  a4(void),  b4(void),  c4(void),  d4(void),  e4(void),  f4(void),  g4(void),  h4(void),  a5(void),  b5(void),  c5(void),  d5(void),  e5(void),  f5(void),  a6(void),  b6(void),  c6(void),  d6(void),  e6(void),  f6(void),  a7(void),  b7(void),  c7(void),  d7(void),  e7(void),  f7(void),  a8(void),  b8(void),  c8(void),  d8(void),  e8(void),  f8(void),  a9(void),  b9(void),  c9(void),  d9(void),  e9(void),  f9(void),  a10(void),  b10(void),  c10(void),  d10(void),  e10(void),  f10(void),  a11(void),  b11(void),  c11(void),  d11(void),  e11(void),  f11(void),  a12(void),  b12(void),  c12(void),  d12(void),  e12(void),  f12(void),  g12(void),  a13(void),  b13(void),  c13(void),  d13(void),  e13(void),  f13(void),  g13(void),  a14(void),  b14(void),  c14(void),  d14(void),  e14(void),  f14(void),  a15(void),  b15(void),  c15(void),  d15(void),  e15(void),  f15(void),  a16(void),  b16(void),  c16(void),  d16(void),  e16(void),  f16(void),  a17(void),  b17(void),  c17(void),  d17(void),  e17(void),  f17(void),  a18(void),  b18(void),  c18(void),  d18(void),  e18(void),  a19(void),  b19(void),  c19(void),  d19(void),  e19(void);


void a1(void)
{
    int *in, *out, *sol;
    int  i, fnderr=0;
    in = (int *)malloc( count * sizeof(int) );
    out = (int *)malloc( count * sizeof(int) );
    sol = (int *)malloc( count * sizeof(int) );
    for (i=0; i<count; i++) { *(in + i) = i; *(sol + i) = i*size; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_INT, MPI_SUM, MPI_COMM_WORLD );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_INT and op MPI_SUM\n", rank );
    free( in );
    free( out );
    free( sol );
}


void b1(void)
{
    long *in, *out, *sol;
    int  i, fnderr=0;
    in = (long *)malloc( count * sizeof(long) );
    out = (long *)malloc( count * sizeof(long) );
    sol = (long *)malloc( count * sizeof(long) );
    for (i=0; i<count; i++) { *(in + i) = i; *(sol + i) = i*size; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_LONG, MPI_SUM, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_LONG and op MPI_SUM\n", rank );
    free( in );
    free( out );
    free( sol );
}


void c1(void)
{
    short *in, *out, *sol;
    int  i, fnderr=0;
    in = (short *)malloc( count * sizeof(short) );
    out = (short *)malloc( count * sizeof(short) );
    sol = (short *)malloc( count * sizeof(short) );
    for (i=0; i<count; i++) { *(in + i) = i; *(sol + i) = i*size; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_SHORT, MPI_SUM, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_SHORT and op MPI_SUM\n", rank );
    free( in );
    free( out );
    free( sol );
}


void d1(void)
{
    unsigned short *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned short *)malloc( count * sizeof(unsigned short) );
    out = (unsigned short *)malloc( count * sizeof(unsigned short) );
    sol = (unsigned short *)malloc( count * sizeof(unsigned short) );
    for (i=0; i<count; i++) { *(in + i) = i; *(sol + i) = i*size; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED_SHORT, MPI_SUM, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED_SHORT and op MPI_SUM\n", rank );
    free( in );
    free( out );
    free( sol );
}


void e1(void)
{
    unsigned *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned *)malloc( count * sizeof(unsigned) );
    out = (unsigned *)malloc( count * sizeof(unsigned) );
    sol = (unsigned *)malloc( count * sizeof(unsigned) );
    for (i=0; i<count; i++) { *(in + i) = i; *(sol + i) = i*size; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED, MPI_SUM, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED and op MPI_SUM\n", rank );
    free( in );
    free( out );
    free( sol );
}


void f1(void)
{
    unsigned long *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned long *)malloc( count * sizeof(unsigned long) );
    out = (unsigned long *)malloc( count * sizeof(unsigned long) );
    sol = (unsigned long *)malloc( count * sizeof(unsigned long) );
    for (i=0; i<count; i++) { *(in + i) = i; *(sol + i) = i*size; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED_LONG, MPI_SUM, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED_LONG and op MPI_SUM\n", rank );
    free( in );
    free( out );
    free( sol );
}


void g1(void)
{
    float *in, *out, *sol;
    int  i, fnderr=0;
    in = (float *)malloc( count * sizeof(float) );
    out = (float *)malloc( count * sizeof(float) );
    sol = (float *)malloc( count * sizeof(float) );
    for (i=0; i<count; i++) { *(in + i) = i; *(sol + i) = i*size; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_FLOAT, MPI_SUM, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_FLOAT and op MPI_SUM\n", rank );
    free( in );
    free( out );
    free( sol );
}


void h1(void)
{
    double *in, *out, *sol;
    int  i, fnderr=0;
    in = (double *)malloc( count * sizeof(double) );
    out = (double *)malloc( count * sizeof(double) );
    sol = (double *)malloc( count * sizeof(double) );
    for (i=0; i<count; i++) { *(in + i) = i; *(sol + i) = i*size; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_DOUBLE, MPI_SUM, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_DOUBLE and op MPI_SUM\n", rank );
    free( in );
    free( out );
    free( sol );
}


void a2(void)
{
    int *in, *out, *sol;
    int  i, fnderr=0;
    in = (int *)malloc( count * sizeof(int) );
    out = (int *)malloc( count * sizeof(int) );
    sol = (int *)malloc( count * sizeof(int) );
    for (i=0; i<count; i++) { *(in + i) = i; *(sol + i) = (i > 0) ? (int)(pow((double)(i),(double)size)+0.1) : 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_INT, MPI_PROD, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_INT and op MPI_PROD\n", rank );
    free( in );
    free( out );
    free( sol );
}


void b2(void)
{
    long *in, *out, *sol;
    int  i, fnderr=0;
    in = (long *)malloc( count * sizeof(long) );
    out = (long *)malloc( count * sizeof(long) );
    sol = (long *)malloc( count * sizeof(long) );
    for (i=0; i<count; i++) { *(in + i) = i; *(sol + i) = (i > 0) ? (int)(pow((double)(i),(double)size)+0.1) : 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_LONG, MPI_PROD, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_LONG and op MPI_PROD\n", rank );
    free( in );
    free( out );
    free( sol );
}


void c2(void)
{
    short *in, *out, *sol;
    int  i, fnderr=0;
    in = (short *)malloc( count * sizeof(short) );
    out = (short *)malloc( count * sizeof(short) );
    sol = (short *)malloc( count * sizeof(short) );
    for (i=0; i<count; i++) { *(in + i) = i; *(sol + i) = (i > 0) ? (int)(pow((double)(i),(double)size)+0.1) : 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_SHORT, MPI_PROD, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_SHORT and op MPI_PROD\n", rank );
    free( in );
    free( out );
    free( sol );
}


void d2(void)
{
    unsigned short *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned short *)malloc( count * sizeof(unsigned short) );
    out = (unsigned short *)malloc( count * sizeof(unsigned short) );
    sol = (unsigned short *)malloc( count * sizeof(unsigned short) );
    for (i=0; i<count; i++) { *(in + i) = i; *(sol + i) = (i > 0) ? (int)(pow((double)(i),(double)size)+0.1) : 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED_SHORT, MPI_PROD, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED_SHORT and op MPI_PROD\n", rank );
    free( in );
    free( out );
    free( sol );
}


void e2(void)
{
    unsigned *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned *)malloc( count * sizeof(unsigned) );
    out = (unsigned *)malloc( count * sizeof(unsigned) );
    sol = (unsigned *)malloc( count * sizeof(unsigned) );
    for (i=0; i<count; i++) { *(in + i) = i; *(sol + i) = (i > 0) ? (int)(pow((double)(i),(double)size)+0.1) : 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED, MPI_PROD, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED and op MPI_PROD\n", rank );
    free( in );
    free( out );
    free( sol );
}


void f2(void)
{
    unsigned long *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned long *)malloc( count * sizeof(unsigned long) );
    out = (unsigned long *)malloc( count * sizeof(unsigned long) );
    sol = (unsigned long *)malloc( count * sizeof(unsigned long) );
    for (i=0; i<count; i++) { *(in + i) = i; *(sol + i) = (i > 0) ? (int)(pow((double)(i),(double)size)+0.1) : 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED_LONG, MPI_PROD, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED_LONG and op MPI_PROD\n", rank );
    free( in );
    free( out );
    free( sol );
}


void g2(void)
{
    float *in, *out, *sol;
    int  i, fnderr=0;
    in = (float *)malloc( count * sizeof(float) );
    out = (float *)malloc( count * sizeof(float) );
    sol = (float *)malloc( count * sizeof(float) );
    for (i=0; i<count; i++) { *(in + i) = i; *(sol + i) = (i > 0) ? (int)(pow((double)(i),(double)size)+0.1) : 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_FLOAT, MPI_PROD, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_FLOAT and op MPI_PROD\n", rank );
    free( in );
    free( out );
    free( sol );
}


void h2(void)
{
    double *in, *out, *sol;
    int  i, fnderr=0;
    in = (double *)malloc( count * sizeof(double) );
    out = (double *)malloc( count * sizeof(double) );
    sol = (double *)malloc( count * sizeof(double) );
    for (i=0; i<count; i++) { *(in + i) = i; *(sol + i) = (i > 0) ? (int)(pow((double)(i),(double)size)+0.1) : 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_DOUBLE, MPI_PROD, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_DOUBLE and op MPI_PROD\n", rank );
    free( in );
    free( out );
    free( sol );
}



void a3(void)
{
    int *in, *out, *sol;
    int  i, fnderr=0;
    in = (int *)malloc( count * sizeof(int) );
    out = (int *)malloc( count * sizeof(int) );
    sol = (int *)malloc( count * sizeof(int) );
    for (i=0; i<count; i++) { *(in + i) = (rank + i); *(sol + i) = (size - 1 + i); 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_INT, MPI_MAX, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_INT and op MPI_MAX\n", rank );
    free( in );
    free( out );
    free( sol );
}


void b3(void)
{
    long *in, *out, *sol;
    int  i, fnderr=0;
    in = (long *)malloc( count * sizeof(long) );
    out = (long *)malloc( count * sizeof(long) );
    sol = (long *)malloc( count * sizeof(long) );
    for (i=0; i<count; i++) { *(in + i) = (rank + i); *(sol + i) = (size - 1 + i); 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_LONG, MPI_MAX, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_LONG and op MPI_MAX\n", rank );
    free( in );
    free( out );
    free( sol );
}


void c3(void)
{
    short *in, *out, *sol;
    int  i, fnderr=0;
    in = (short *)malloc( count * sizeof(short) );
    out = (short *)malloc( count * sizeof(short) );
    sol = (short *)malloc( count * sizeof(short) );
    for (i=0; i<count; i++) { *(in + i) = (rank + i); *(sol + i) = (size - 1 + i); 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_SHORT, MPI_MAX, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_SHORT and op MPI_MAX\n", rank );
    free( in );
    free( out );
    free( sol );
}


void d3(void)
{
    unsigned short *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned short *)malloc( count * sizeof(unsigned short) );
    out = (unsigned short *)malloc( count * sizeof(unsigned short) );
    sol = (unsigned short *)malloc( count * sizeof(unsigned short) );
    for (i=0; i<count; i++) { *(in + i) = (rank + i); *(sol + i) = (size - 1 + i); 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED_SHORT, MPI_MAX, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED_SHORT and op MPI_MAX\n", rank );
    free( in );
    free( out );
    free( sol );
}


void e3(void)
{
    unsigned *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned *)malloc( count * sizeof(unsigned) );
    out = (unsigned *)malloc( count * sizeof(unsigned) );
    sol = (unsigned *)malloc( count * sizeof(unsigned) );
    for (i=0; i<count; i++) { *(in + i) = (rank + i); *(sol + i) = (size - 1 + i); 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED, MPI_MAX, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED and op MPI_MAX\n", rank );
    free( in );
    free( out );
    free( sol );
}


void f3(void)
{
    unsigned long *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned long *)malloc( count * sizeof(unsigned long) );
    out = (unsigned long *)malloc( count * sizeof(unsigned long) );
    sol = (unsigned long *)malloc( count * sizeof(unsigned long) );
    for (i=0; i<count; i++) { *(in + i) = (rank + i); *(sol + i) = (size - 1 + i); 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED_LONG, MPI_MAX, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED_LONG and op MPI_MAX\n", rank );
    free( in );
    free( out );
    free( sol );
}


void g3(void)
{
    float *in, *out, *sol;
    int  i, fnderr=0;
    in = (float *)malloc( count * sizeof(float) );
    out = (float *)malloc( count * sizeof(float) );
    sol = (float *)malloc( count * sizeof(float) );
    for (i=0; i<count; i++) { *(in + i) = (rank + i); *(sol + i) = (size - 1 + i); 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_FLOAT, MPI_MAX, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_FLOAT and op MPI_MAX\n", rank );
    free( in );
    free( out );
    free( sol );
}


void h3(void)
{
    double *in, *out, *sol;
    int  i, fnderr=0;
    in = (double *)malloc( count * sizeof(double) );
    out = (double *)malloc( count * sizeof(double) );
    sol = (double *)malloc( count * sizeof(double) );
    for (i=0; i<count; i++) { *(in + i) = (rank + i); *(sol + i) = (size - 1 + i); 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_DOUBLE, MPI_MAX, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_DOUBLE and op MPI_MAX\n", rank );
    free( in );
    free( out );
    free( sol );
}



void a4(void)
{
    int *in, *out, *sol;
    int  i, fnderr=0;
    in = (int *)malloc( count * sizeof(int) );
    out = (int *)malloc( count * sizeof(int) );
    sol = (int *)malloc( count * sizeof(int) );
    for (i=0; i<count; i++) { *(in + i) = (rank + i); *(sol + i) = i; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_INT, MPI_MIN, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_INT and op MPI_MIN\n", rank );
    free( in );
    free( out );
    free( sol );
}


void b4(void)
{
    long *in, *out, *sol;
    int  i, fnderr=0;
    in = (long *)malloc( count * sizeof(long) );
    out = (long *)malloc( count * sizeof(long) );
    sol = (long *)malloc( count * sizeof(long) );
    for (i=0; i<count; i++) { *(in + i) = (rank + i); *(sol + i) = i; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_LONG, MPI_MIN, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_LONG and op MPI_MIN\n", rank );
    free( in );
    free( out );
    free( sol );
}


void c4(void)
{
    short *in, *out, *sol;
    int  i, fnderr=0;
    in = (short *)malloc( count * sizeof(short) );
    out = (short *)malloc( count * sizeof(short) );
    sol = (short *)malloc( count * sizeof(short) );
    for (i=0; i<count; i++) { *(in + i) = (rank + i); *(sol + i) = i; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_SHORT, MPI_MIN, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_SHORT and op MPI_MIN\n", rank );
    free( in );
    free( out );
    free( sol );
}


void d4(void)
{
    unsigned short *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned short *)malloc( count * sizeof(unsigned short) );
    out = (unsigned short *)malloc( count * sizeof(unsigned short) );
    sol = (unsigned short *)malloc( count * sizeof(unsigned short) );
    for (i=0; i<count; i++) { *(in + i) = (rank + i); *(sol + i) = i; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED_SHORT, MPI_MIN, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED_SHORT and op MPI_MIN\n", rank );
    free( in );
    free( out );
    free( sol );
}


void e4(void)
{
    unsigned *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned *)malloc( count * sizeof(unsigned) );
    out = (unsigned *)malloc( count * sizeof(unsigned) );
    sol = (unsigned *)malloc( count * sizeof(unsigned) );
    for (i=0; i<count; i++) { *(in + i) = (rank + i); *(sol + i) = i; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED, MPI_MIN, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED and op MPI_MIN\n", rank );
    free( in );
    free( out );
    free( sol );
}


void f4(void)
{
    unsigned long *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned long *)malloc( count * sizeof(unsigned long) );
    out = (unsigned long *)malloc( count * sizeof(unsigned long) );
    sol = (unsigned long *)malloc( count * sizeof(unsigned long) );
    for (i=0; i<count; i++) { *(in + i) = (rank + i); *(sol + i) = i; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED_LONG, MPI_MIN, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED_LONG and op MPI_MIN\n", rank );
    free( in );
    free( out );
    free( sol );
}


void g4(void)
{
    float *in, *out, *sol;
    int  i, fnderr=0;
    in = (float *)malloc( count * sizeof(float) );
    out = (float *)malloc( count * sizeof(float) );
    sol = (float *)malloc( count * sizeof(float) );
    for (i=0; i<count; i++) { *(in + i) = (rank + i); *(sol + i) = i; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_FLOAT, MPI_MIN, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_FLOAT and op MPI_MIN\n", rank );
    free( in );
    free( out );
    free( sol );
}


void h4(void)
{
    double *in, *out, *sol;
    int  i, fnderr=0;
    in = (double *)malloc( count * sizeof(double) );
    out = (double *)malloc( count * sizeof(double) );
    sol = (double *)malloc( count * sizeof(double) );
    for (i=0; i<count; i++) { *(in + i) = (rank + i); *(sol + i) = i; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_DOUBLE, MPI_MIN, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_DOUBLE and op MPI_MIN\n", rank );
    free( in );
    free( out );
    free( sol );
}



void a5(void)
{
    int *in, *out, *sol;
    int  i, fnderr=0;
    in = (int *)malloc( count * sizeof(int) );
    out = (int *)malloc( count * sizeof(int) );
    sol = (int *)malloc( count * sizeof(int) );
    for (i=0; i<count; i++) { *(in + i) = (rank & 0x1); *(sol + i) = (size > 1); 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_INT, MPI_LOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_INT and op MPI_LOR\n", rank );
    free( in );
    free( out );
    free( sol );
}


void b5(void)
{
    long *in, *out, *sol;
    int  i, fnderr=0;
    in = (long *)malloc( count * sizeof(long) );
    out = (long *)malloc( count * sizeof(long) );
    sol = (long *)malloc( count * sizeof(long) );
    for (i=0; i<count; i++) { *(in + i) = (rank & 0x1); *(sol + i) = (size > 1); 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_LONG, MPI_LOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_LONG and op MPI_LOR\n", rank );
    free( in );
    free( out );
    free( sol );
}


void c5(void)
{
    short *in, *out, *sol;
    int  i, fnderr=0;
    in = (short *)malloc( count * sizeof(short) );
    out = (short *)malloc( count * sizeof(short) );
    sol = (short *)malloc( count * sizeof(short) );
    for (i=0; i<count; i++) { *(in + i) = (rank & 0x1); *(sol + i) = (size > 1); 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_SHORT, MPI_LOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_SHORT and op MPI_LOR\n", rank );
    free( in );
    free( out );
    free( sol );
}


void d5(void)
{
    unsigned short *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned short *)malloc( count * sizeof(unsigned short) );
    out = (unsigned short *)malloc( count * sizeof(unsigned short) );
    sol = (unsigned short *)malloc( count * sizeof(unsigned short) );
    for (i=0; i<count; i++) { *(in + i) = (rank & 0x1); *(sol + i) = (size > 1); 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED_SHORT, MPI_LOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED_SHORT and op MPI_LOR\n", rank );
    free( in );
    free( out );
    free( sol );
}


void e5(void)
{
    unsigned *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned *)malloc( count * sizeof(unsigned) );
    out = (unsigned *)malloc( count * sizeof(unsigned) );
    sol = (unsigned *)malloc( count * sizeof(unsigned) );
    for (i=0; i<count; i++) { *(in + i) = (rank & 0x1); *(sol + i) = (size > 1); 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED, MPI_LOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED and op MPI_LOR\n", rank );
    free( in );
    free( out );
    free( sol );
}


void f5(void)
{
    unsigned long *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned long *)malloc( count * sizeof(unsigned long) );
    out = (unsigned long *)malloc( count * sizeof(unsigned long) );
    sol = (unsigned long *)malloc( count * sizeof(unsigned long) );
    for (i=0; i<count; i++) { *(in + i) = (rank & 0x1); *(sol + i) = (size > 1); 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED_LONG, MPI_LOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED_LONG and op MPI_LOR\n", rank );
    free( in );
    free( out );
    free( sol );
}



void a6(void)
{
    int *in, *out, *sol;
    int  i, fnderr=0;
    in = (int *)malloc( count * sizeof(int) );
    out = (int *)malloc( count * sizeof(int) );
    sol = (int *)malloc( count * sizeof(int) );
    for (i=0; i<count; i++) { *(in + i) = 0; *(sol + i) = 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_INT, MPI_LOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_INT and op MPI_LOR\n", rank );
    free( in );
    free( out );
    free( sol );
}


void b6(void)
{
    long *in, *out, *sol;
    int  i, fnderr=0;
    in = (long *)malloc( count * sizeof(long) );
    out = (long *)malloc( count * sizeof(long) );
    sol = (long *)malloc( count * sizeof(long) );
    for (i=0; i<count; i++) { *(in + i) = 0; *(sol + i) = 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_LONG, MPI_LOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_LONG and op MPI_LOR\n", rank );
    free( in );
    free( out );
    free( sol );
}


void c6(void)
{
    short *in, *out, *sol;
    int  i, fnderr=0;
    in = (short *)malloc( count * sizeof(short) );
    out = (short *)malloc( count * sizeof(short) );
    sol = (short *)malloc( count * sizeof(short) );
    for (i=0; i<count; i++) { *(in + i) = 0; *(sol + i) = 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_SHORT, MPI_LOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_SHORT and op MPI_LOR\n", rank );
    free( in );
    free( out );
    free( sol );
}


void d6(void)
{
    unsigned short *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned short *)malloc( count * sizeof(unsigned short) );
    out = (unsigned short *)malloc( count * sizeof(unsigned short) );
    sol = (unsigned short *)malloc( count * sizeof(unsigned short) );
    for (i=0; i<count; i++) { *(in + i) = 0; *(sol + i) = 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED_SHORT, MPI_LOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED_SHORT and op MPI_LOR\n", rank );
    free( in );
    free( out );
    free( sol );
}


void e6(void)
{
    unsigned *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned *)malloc( count * sizeof(unsigned) );
    out = (unsigned *)malloc( count * sizeof(unsigned) );
    sol = (unsigned *)malloc( count * sizeof(unsigned) );
    for (i=0; i<count; i++) { *(in + i) = 0; *(sol + i) = 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED, MPI_LOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED and op MPI_LOR\n", rank );
    free( in );
    free( out );
    free( sol );
}


void f6(void)
{
    unsigned long *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned long *)malloc( count * sizeof(unsigned long) );
    out = (unsigned long *)malloc( count * sizeof(unsigned long) );
    sol = (unsigned long *)malloc( count * sizeof(unsigned long) );
    for (i=0; i<count; i++) { *(in + i) = 0; *(sol + i) = 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED_LONG, MPI_LOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED_LONG and op MPI_LOR\n", rank );
    free( in );
    free( out );
    free( sol );
}



void a7(void)
{
    int *in, *out, *sol;
    int  i, fnderr=0;
    in = (int *)malloc( count * sizeof(int) );
    out = (int *)malloc( count * sizeof(int) );
    sol = (int *)malloc( count * sizeof(int) );
    for (i=0; i<count; i++) { *(in + i) = (rank == 1); *(sol + i) = (size > 1); 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_INT, MPI_LXOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_INT and op MPI_LXOR\n", rank);
    free( in );
    free( out );
    free( sol );
}


void b7(void)
{
    long *in, *out, *sol;
    int  i, fnderr=0;
    in = (long *)malloc( count * sizeof(long) );
    out = (long *)malloc( count * sizeof(long) );
    sol = (long *)malloc( count * sizeof(long) );
    for (i=0; i<count; i++) { *(in + i) = (rank == 1); *(sol + i) = (size > 1); 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_LONG, MPI_LXOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_LONG and op MPI_LXOR\n", rank );
    free( in );
    free( out );
    free( sol );
}


void c7(void)
{
    short *in, *out, *sol;
    int  i, fnderr=0;
    in = (short *)malloc( count * sizeof(short) );
    out = (short *)malloc( count * sizeof(short) );
    sol = (short *)malloc( count * sizeof(short) );
    for (i=0; i<count; i++) { *(in + i) = (rank == 1); *(sol + i) = (size > 1); 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_SHORT, MPI_LXOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_SHORT and op MPI_LXOR\n", rank );
    free( in );
    free( out );
    free( sol );
}


void d7(void)
{
    unsigned short *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned short *)malloc( count * sizeof(unsigned short) );
    out = (unsigned short *)malloc( count * sizeof(unsigned short) );
    sol = (unsigned short *)malloc( count * sizeof(unsigned short) );
    for (i=0; i<count; i++) { *(in + i) = (rank == 1); *(sol + i) = (size > 1); 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED_SHORT, MPI_LXOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED_SHORT and op MPI_LXOR\n", rank );
    free( in );
    free( out );
    free( sol );
}


void e7(void)
{
    unsigned *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned *)malloc( count * sizeof(unsigned) );
    out = (unsigned *)malloc( count * sizeof(unsigned) );
    sol = (unsigned *)malloc( count * sizeof(unsigned) );
    for (i=0; i<count; i++) { *(in + i) = (rank == 1); *(sol + i) = (size > 1); 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED, MPI_LXOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED and op MPI_LXOR\n", rank );
    free( in );
    free( out );
    free( sol );
}


void f7(void)
{
    unsigned long *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned long *)malloc( count * sizeof(unsigned long) );
    out = (unsigned long *)malloc( count * sizeof(unsigned long) );
    sol = (unsigned long *)malloc( count * sizeof(unsigned long) );
    for (i=0; i<count; i++) { *(in + i) = (rank == 1); *(sol + i) = (size > 1); 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED_LONG, MPI_LXOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED_LONG and op MPI_LXOR\n", rank );
    free( in );
    free( out );
    free( sol );
}



void a8(void)
{
    int *in, *out, *sol;
    int  i, fnderr=0;
    in = (int *)malloc( count * sizeof(int) );
    out = (int *)malloc( count * sizeof(int) );
    sol = (int *)malloc( count * sizeof(int) );
    for (i=0; i<count; i++) { *(in + i) = 0; *(sol + i) = 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_INT, MPI_LXOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_INT and op MPI_LXOR\n", rank );
    free( in );
    free( out );
    free( sol );
}


void b8(void)
{
    long *in, *out, *sol;
    int  i, fnderr=0;
    in = (long *)malloc( count * sizeof(long) );
    out = (long *)malloc( count * sizeof(long) );
    sol = (long *)malloc( count * sizeof(long) );
    for (i=0; i<count; i++) { *(in + i) = 0; *(sol + i) = 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_LONG, MPI_LXOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_LONG and op MPI_LXOR\n", rank );
    free( in );
    free( out );
    free( sol );
}


void c8(void)
{
    short *in, *out, *sol;
    int  i, fnderr=0;
    in = (short *)malloc( count * sizeof(short) );
    out = (short *)malloc( count * sizeof(short) );
    sol = (short *)malloc( count * sizeof(short) );
    for (i=0; i<count; i++) { *(in + i) = 0; *(sol + i) = 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_SHORT, MPI_LXOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_SHORT and op MPI_LXOR\n", rank );
    free( in );
    free( out );
    free( sol );
}


void d8(void)
{
    unsigned short *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned short *)malloc( count * sizeof(unsigned short) );
    out = (unsigned short *)malloc( count * sizeof(unsigned short) );
    sol = (unsigned short *)malloc( count * sizeof(unsigned short) );
    for (i=0; i<count; i++) { *(in + i) = 0; *(sol + i) = 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED_SHORT, MPI_LXOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED_SHORT and op MPI_LXOR\n", rank );
    free( in );
    free( out );
    free( sol );
}


void e8(void)
{
    unsigned *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned *)malloc( count * sizeof(unsigned) );
    out = (unsigned *)malloc( count * sizeof(unsigned) );
    sol = (unsigned *)malloc( count * sizeof(unsigned) );
    for (i=0; i<count; i++) { *(in + i) = 0; *(sol + i) = 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED, MPI_LXOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED and op MPI_LXOR\n", rank );
    free( in );
    free( out );
    free( sol );
}


void f8(void)
{
    unsigned long *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned long *)malloc( count * sizeof(unsigned long) );
    out = (unsigned long *)malloc( count * sizeof(unsigned long) );
    sol = (unsigned long *)malloc( count * sizeof(unsigned long) );
    for (i=0; i<count; i++) { *(in + i) = 0; *(sol + i) = 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED_LONG, MPI_LXOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED_LONG and op MPI_LXOR\n", rank );
    free( in );
    free( out );
    free( sol );
}



void a9(void)
{
    int *in, *out, *sol;
    int  i, fnderr=0;
    in = (int *)malloc( count * sizeof(int) );
    out = (int *)malloc( count * sizeof(int) );
    sol = (int *)malloc( count * sizeof(int) );
    for (i=0; i<count; i++) { *(in + i) = 1; *(sol + i) = 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_INT, MPI_LXOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_INT and op MPI_LXOR\n", rank );
    free( in );
    free( out );
    free( sol );
}

void b9(void)
{
    long *in, *out, *sol;
    int  i, fnderr=0;
    in = (long *)malloc( count * sizeof(long) );
    out = (long *)malloc( count * sizeof(long) );
    sol = (long *)malloc( count * sizeof(long) );
    for (i=0; i<count; i++) { *(in + i) = 1; *(sol + i) = 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_LONG, MPI_LXOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_LONG and op MPI_LXOR\n", rank );
    free( in );
    free( out );
    free( sol );
}


void c9(void)
{
    short *in, *out, *sol;
    int  i, fnderr=0;
    in = (short *)malloc( count * sizeof(short) );
    out = (short *)malloc( count * sizeof(short) );
    sol = (short *)malloc( count * sizeof(short) );
    for (i=0; i<count; i++) { *(in + i) = 1; *(sol + i) = 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_SHORT, MPI_LXOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_SHORT and op MPI_LXOR\n", rank );
    free( in );
    free( out );
    free( sol );
}


void d9(void)
{
    unsigned short *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned short *)malloc( count * sizeof(unsigned short) );
    out = (unsigned short *)malloc( count * sizeof(unsigned short) );
    sol = (unsigned short *)malloc( count * sizeof(unsigned short) );
    for (i=0; i<count; i++) { *(in + i) = 1; *(sol + i) = 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED_SHORT, MPI_LXOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED_SHORT and op MPI_LXOR\n", rank );
    free( in );
    free( out );
    free( sol );
}


void e9(void)
{
    unsigned *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned *)malloc( count * sizeof(unsigned) );
    out = (unsigned *)malloc( count * sizeof(unsigned) );
    sol = (unsigned *)malloc( count * sizeof(unsigned) );
    for (i=0; i<count; i++) { *(in + i) = 1; *(sol + i) = 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED, MPI_LXOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED and op MPI_LXOR\n", rank );
    free( in );
    free( out );
    free( sol );
}


void f9(void)
{
    unsigned long *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned long *)malloc( count * sizeof(unsigned long) );
    out = (unsigned long *)malloc( count * sizeof(unsigned long) );
    sol = (unsigned long *)malloc( count * sizeof(unsigned long) );
    for (i=0; i<count; i++) { *(in + i) = 1; *(sol + i) = 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED_LONG, MPI_LXOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED_LONG and op MPI_LXOR\n", rank );
    free( in );
    free( out );
    free( sol );
}



void a10(void)
{
    int *in, *out, *sol;
    int  i, fnderr=0;
    in = (int *)malloc( count * sizeof(int) );
    out = (int *)malloc( count * sizeof(int) );
    sol = (int *)malloc( count * sizeof(int) );
    for (i=0; i<count; i++) { *(in + i) = (rank & 0x1); *(sol + i) = 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_INT, MPI_LAND, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_INT and op MPI_LAND\n", rank );
    free( in );
    free( out );
    free( sol );
}


void b10(void)
{
    long *in, *out, *sol;
    int  i, fnderr=0;
    in = (long *)malloc( count * sizeof(long) );
    out = (long *)malloc( count * sizeof(long) );
    sol = (long *)malloc( count * sizeof(long) );
    for (i=0; i<count; i++) { *(in + i) = (rank & 0x1); *(sol + i) = 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_LONG, MPI_LAND, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_LONG and op MPI_LAND\n", rank );
    free( in );
    free( out );
    free( sol );
}


void c10(void)
{
    short *in, *out, *sol;
    int  i, fnderr=0;
    in = (short *)malloc( count * sizeof(short) );
    out = (short *)malloc( count * sizeof(short) );
    sol = (short *)malloc( count * sizeof(short) );
    for (i=0; i<count; i++) { *(in + i) = (rank & 0x1); *(sol + i) = 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_SHORT, MPI_LAND, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_SHORT and op MPI_LAND\n", rank );
    free( in );
    free( out );
    free( sol );
}


void d10(void)
{
    unsigned short *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned short *)malloc( count * sizeof(unsigned short) );
    out = (unsigned short *)malloc( count * sizeof(unsigned short) );
    sol = (unsigned short *)malloc( count * sizeof(unsigned short) );
    for (i=0; i<count; i++) { *(in + i) = (rank & 0x1); *(sol + i) = 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED_SHORT, MPI_LAND, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED_SHORT and op MPI_LAND\n", rank );
    free( in );
    free( out );
    free( sol );
}


void e10(void)
{
    unsigned *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned *)malloc( count * sizeof(unsigned) );
    out = (unsigned *)malloc( count * sizeof(unsigned) );
    sol = (unsigned *)malloc( count * sizeof(unsigned) );
    for (i=0; i<count; i++) { *(in + i) = (rank & 0x1); *(sol + i) = 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED, MPI_LAND, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED and op MPI_LAND\n", rank );
    free( in );
    free( out );
    free( sol );
}


void f10(void)
{
    unsigned long *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned long *)malloc( count * sizeof(unsigned long) );
    out = (unsigned long *)malloc( count * sizeof(unsigned long) );
    sol = (unsigned long *)malloc( count * sizeof(unsigned long) );
    for (i=0; i<count; i++) { *(in + i) = (rank & 0x1); *(sol + i) = 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED_LONG, MPI_LAND, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED_LONG and op MPI_LAND\n", rank );
    free( in );
    free( out );
    free( sol );
}



void a11(void)
{
    int *in, *out, *sol;
    int  i, fnderr=0;
    in = (int *)malloc( count * sizeof(int) );
    out = (int *)malloc( count * sizeof(int) );
    sol = (int *)malloc( count * sizeof(int) );
    for (i=0; i<count; i++) { *(in + i) = 1; *(sol + i) = 1; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_INT, MPI_LAND, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_INT and op MPI_LAND\n", rank );
    free( in );
    free( out );
    free( sol );
}


void b11(void)
{
    long *in, *out, *sol;
    int  i, fnderr=0;
    in = (long *)malloc( count * sizeof(long) );
    out = (long *)malloc( count * sizeof(long) );
    sol = (long *)malloc( count * sizeof(long) );
    for (i=0; i<count; i++) { *(in + i) = 1; *(sol + i) = 1; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_LONG, MPI_LAND, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_LONG and op MPI_LAND\n", rank );
    free( in );
    free( out );
    free( sol );
}


void c11(void)
{
    short *in, *out, *sol;
    int  i, fnderr=0;
    in = (short *)malloc( count * sizeof(short) );
    out = (short *)malloc( count * sizeof(short) );
    sol = (short *)malloc( count * sizeof(short) );
    for (i=0; i<count; i++) { *(in + i) = 1; *(sol + i) = 1; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_SHORT, MPI_LAND, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_SHORT and op MPI_LAND\n", rank );
    free( in );
    free( out );
    free( sol );
}


void d11(void)
{
    unsigned short *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned short *)malloc( count * sizeof(unsigned short) );
    out = (unsigned short *)malloc( count * sizeof(unsigned short) );
    sol = (unsigned short *)malloc( count * sizeof(unsigned short) );
    for (i=0; i<count; i++) { *(in + i) = 1; *(sol + i) = 1; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED_SHORT, MPI_LAND, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED_SHORT and op MPI_LAND\n", rank );
    free( in );
    free( out );
    free( sol );
}


void e11(void)
{
    unsigned *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned *)malloc( count * sizeof(unsigned) );
    out = (unsigned *)malloc( count * sizeof(unsigned) );
    sol = (unsigned *)malloc( count * sizeof(unsigned) );
    for (i=0; i<count; i++) { *(in + i) = 1; *(sol + i) = 1; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED, MPI_LAND, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED and op MPI_LAND\n", rank );
    free( in );
    free( out );
    free( sol );
}


void f11(void)
{
    unsigned long *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned long *)malloc( count * sizeof(unsigned long) );
    out = (unsigned long *)malloc( count * sizeof(unsigned long) );
    sol = (unsigned long *)malloc( count * sizeof(unsigned long) );
    for (i=0; i<count; i++) { *(in + i) = 1; *(sol + i) = 1; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED_LONG, MPI_LAND, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED_LONG and op MPI_LAND\n", rank );
    free( in );
    free( out );
    free( sol );
}


void a12(void)
{
    int *in, *out, *sol;
    int  i, fnderr=0;
    in = (int *)malloc( count * sizeof(int) );
    out = (int *)malloc( count * sizeof(int) );
    sol = (int *)malloc( count * sizeof(int) );
    for (i=0; i<count; i++) { *(in + i) = rank & 0x3; *(sol + i) = (size < 3) ? size - 1 : 0x3; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_INT, MPI_BOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_INT and op MPI_BOR\n", rank );
    free( in );
    free( out );
    free( sol );
}


void b12(void)
{
    long *in, *out, *sol;
    int  i, fnderr=0;
    in = (long *)malloc( count * sizeof(long) );
    out = (long *)malloc( count * sizeof(long) );
    sol = (long *)malloc( count * sizeof(long) );
    for (i=0; i<count; i++) { *(in + i) = rank & 0x3; *(sol + i) = (size < 3) ? size - 1 : 0x3; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_LONG, MPI_BOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_LONG and op MPI_BOR\n", rank );
    free( in );
    free( out );
    free( sol );
}


void c12(void)
{
    short *in, *out, *sol;
    int  i, fnderr=0;
    in = (short *)malloc( count * sizeof(short) );
    out = (short *)malloc( count * sizeof(short) );
    sol = (short *)malloc( count * sizeof(short) );
    for (i=0; i<count; i++) { *(in + i) = rank & 0x3; *(sol + i) = (size < 3) ? size - 1 : 0x3; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_SHORT, MPI_BOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_SHORT and op MPI_BOR\n", rank );
    free( in );
    free( out );
    free( sol );
}


void d12(void)
{
    unsigned short *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned short *)malloc( count * sizeof(unsigned short) );
    out = (unsigned short *)malloc( count * sizeof(unsigned short) );
    sol = (unsigned short *)malloc( count * sizeof(unsigned short) );
    for (i=0; i<count; i++) { *(in + i) = rank & 0x3; *(sol + i) = (size < 3) ? size - 1 : 0x3; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED_SHORT, MPI_BOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED_SHORT and op MPI_BOR\n", rank );
    free( in );
    free( out );
    free( sol );
}


void e12(void)
{
    unsigned *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned *)malloc( count * sizeof(unsigned) );
    out = (unsigned *)malloc( count * sizeof(unsigned) );
    sol = (unsigned *)malloc( count * sizeof(unsigned) );
    for (i=0; i<count; i++) { *(in + i) = rank & 0x3; *(sol + i) = (size < 3) ? size - 1 : 0x3; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED, MPI_BOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED and op MPI_BOR\n", rank );
    free( in );
    free( out );
    free( sol );
}


void f12(void)
{
    unsigned long *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned long *)malloc( count * sizeof(unsigned long) );
    out = (unsigned long *)malloc( count * sizeof(unsigned long) );
    sol = (unsigned long *)malloc( count * sizeof(unsigned long) );
    for (i=0; i<count; i++) { *(in + i) = rank & 0x3; *(sol + i) = (size < 3) ? size - 1 : 0x3; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED_LONG, MPI_BOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED_LONG and op MPI_BOR\n", rank );
    free( in );
    free( out );
    free( sol );
}


void g12(void)
{
    unsigned char *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned char *)malloc( count * sizeof(unsigned char) );
    out = (unsigned char *)malloc( count * sizeof(unsigned char) );
    sol = (unsigned char *)malloc( count * sizeof(unsigned char) );
    for (i=0; i<count; i++) { *(in + i) = rank & 0x3; *(sol + i) = (size < 3) ? size - 1 : 0x3; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_BYTE, MPI_BOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_BYTE and op MPI_BOR\n", rank );
    free( in );
    free( out );
    free( sol );
}


void a13(void)
{
    int *in, *out, *sol;
    int  i, fnderr=0;
    in = (int *)malloc( count * sizeof(int) );
    out = (int *)malloc( count * sizeof(int) );
    sol = (int *)malloc( count * sizeof(int) );
    for (i=0; i<count; i++) { *(in + i) = (rank == size-1 ? i : ~0); *(sol + i) = i; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_INT, MPI_BAND, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_INT and op MPI_BAND\n", rank );
    free( in );
    free( out );
    free( sol );
}


void b13(void)
{
    long *in, *out, *sol;
    int  i, fnderr=0;
    in = (long *)malloc( count * sizeof(long) );
    out = (long *)malloc( count * sizeof(long) );
    sol = (long *)malloc( count * sizeof(long) );
    for (i=0; i<count; i++) { *(in + i) = (rank == size-1 ? i : ~0); *(sol + i) = i; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_LONG, MPI_BAND, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_LONG and op MPI_BAND\n", rank );
    free( in );
    free( out );
    free( sol );
}


void c13(void)
{
    short *in, *out, *sol;
    int  i, fnderr=0;
    in = (short *)malloc( count * sizeof(short) );
    out = (short *)malloc( count * sizeof(short) );
    sol = (short *)malloc( count * sizeof(short) );
    for (i=0; i<count; i++) { *(in + i) = (rank == size-1 ? i : ~0); *(sol + i) = i; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_SHORT, MPI_BAND, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_SHORT and op MPI_BAND\n", rank );
    free( in );
    free( out );
    free( sol );
}


void d13(void)
{
    unsigned short *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned short *)malloc( count * sizeof(unsigned short) );
    out = (unsigned short *)malloc( count * sizeof(unsigned short) );
    sol = (unsigned short *)malloc( count * sizeof(unsigned short) );
    for (i=0; i<count; i++) { *(in + i) = (rank == size-1 ? i : ~0); *(sol + i) = i; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED_SHORT, MPI_BAND, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED_SHORT and op MPI_BAND\n", rank );
    free( in );
    free( out );
    free( sol );
}


void e13(void)
{
    unsigned *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned *)malloc( count * sizeof(unsigned) );
    out = (unsigned *)malloc( count * sizeof(unsigned) );
    sol = (unsigned *)malloc( count * sizeof(unsigned) );
    for (i=0; i<count; i++) { *(in + i) = (rank == size-1 ? i : ~0); *(sol + i) = i; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED, MPI_BAND, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED and op MPI_BAND\n", rank );
    free( in );
    free( out );
    free( sol );
}


void f13(void)
{
    unsigned long *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned long *)malloc( count * sizeof(unsigned long) );
    out = (unsigned long *)malloc( count * sizeof(unsigned long) );
    sol = (unsigned long *)malloc( count * sizeof(unsigned long) );
    for (i=0; i<count; i++) { *(in + i) = (rank == size-1 ? i : ~0); *(sol + i) = i; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED_LONG, MPI_BAND, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED_LONG and op MPI_BAND\n", rank );
    free( in );
    free( out );
    free( sol );
}


void g13(void)
{
    unsigned char *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned char *)malloc( count * sizeof(unsigned char) );
    out = (unsigned char *)malloc( count * sizeof(unsigned char) );
    sol = (unsigned char *)malloc( count * sizeof(unsigned char) );
    for (i=0; i<count; i++) { *(in + i) = (rank == size-1 ? i : ~0); *(sol + i) = i; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_BYTE, MPI_BAND, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_BYTE and op MPI_BAND\n", rank );
    free( in );
    free( out );
    free( sol );
}


void a14(void)
{
    int *in, *out, *sol;
    int  i, fnderr=0;
    in = (int *)malloc( count * sizeof(int) );
    out = (int *)malloc( count * sizeof(int) );
    sol = (int *)malloc( count * sizeof(int) );
    for (i=0; i<count; i++) { *(in + i) = (rank == size-1 ? i : 0); *(sol + i) = 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_INT, MPI_BAND, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_INT and op MPI_BAND\n", rank );
    free( in );
    free( out );
    free( sol );
}


void b14(void)
{
    long *in, *out, *sol;
    int  i, fnderr=0;
    in = (long *)malloc( count * sizeof(long) );
    out = (long *)malloc( count * sizeof(long) );
    sol = (long *)malloc( count * sizeof(long) );
    for (i=0; i<count; i++) { *(in + i) = (rank == size-1 ? i : 0); *(sol + i) = 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_LONG, MPI_BAND, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_LONG and op MPI_BAND\n", rank );
    free( in );
    free( out );
    free( sol );
}


void c14(void)
{
    short *in, *out, *sol;
    int  i, fnderr=0;
    in = (short *)malloc( count * sizeof(short) );
    out = (short *)malloc( count * sizeof(short) );
    sol = (short *)malloc( count * sizeof(short) );
    for (i=0; i<count; i++) { *(in + i) = (rank == size-1 ? i : 0); *(sol + i) = 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_SHORT, MPI_BAND, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_SHORT and op MPI_BAND\n", rank );
    free( in );
    free( out );
    free( sol );
}


void d14(void)
{
    unsigned short *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned short *)malloc( count * sizeof(unsigned short) );
    out = (unsigned short *)malloc( count * sizeof(unsigned short) );
    sol = (unsigned short *)malloc( count * sizeof(unsigned short) );
    for (i=0; i<count; i++) { *(in + i) = (rank == size-1 ? i : 0); *(sol + i) = 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED_SHORT, MPI_BAND, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED_SHORT and op MPI_BAND\n", rank );
    free( in );
    free( out );
    free( sol );
}


void e14(void)
{
    unsigned *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned *)malloc( count * sizeof(unsigned) );
    out = (unsigned *)malloc( count * sizeof(unsigned) );
    sol = (unsigned *)malloc( count * sizeof(unsigned) );
    for (i=0; i<count; i++) { *(in + i) = (rank == size-1 ? i : 0); *(sol + i) = 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED, MPI_BAND, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED and op MPI_BAND\n", rank );
    free( in );
    free( out );
    free( sol );
}


void f14(void)
{
    unsigned long *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned long *)malloc( count * sizeof(unsigned long) );
    out = (unsigned long *)malloc( count * sizeof(unsigned long) );
    sol = (unsigned long *)malloc( count * sizeof(unsigned long) );
    for (i=0; i<count; i++) { *(in + i) = (rank == size-1 ? i : 0); *(sol + i) = 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED_LONG, MPI_BAND, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED_LONG and op MPI_BAND\n", rank );
    free( in );
    free( out );
    free( sol );
}



void a15(void)
{
    int *in, *out, *sol;
    int  i, fnderr=0;
    in = (int *)malloc( count * sizeof(int) );
    out = (int *)malloc( count * sizeof(int) );
    sol = (int *)malloc( count * sizeof(int) );
    for (i=0; i<count; i++) { *(in + i) = (rank == 1)*0xf0 ; *(sol + i) = (size > 1)*0xf0 ; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_INT, MPI_BXOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_INT and op MPI_BXOR\n", rank );
    free( in );
    free( out );
    free( sol );
}


void b15(void)
{
    long *in, *out, *sol;
    int  i, fnderr=0;
    in = (long *)malloc( count * sizeof(long) );
    out = (long *)malloc( count * sizeof(long) );
    sol = (long *)malloc( count * sizeof(long) );
    for (i=0; i<count; i++) { *(in + i) = (rank == 1)*0xf0 ; *(sol + i) = (size > 1)*0xf0 ; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_LONG, MPI_BXOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_LONG and op MPI_BXOR\n", rank );
    free( in );
    free( out );
    free( sol );
}


void c15(void)
{
    short *in, *out, *sol;
    int  i, fnderr=0;
    in = (short *)malloc( count * sizeof(short) );
    out = (short *)malloc( count * sizeof(short) );
    sol = (short *)malloc( count * sizeof(short) );
    for (i=0; i<count; i++) { *(in + i) = (rank == 1)*0xf0 ; *(sol + i) = (size > 1)*0xf0 ; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_SHORT, MPI_BXOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_SHORT and op MPI_BXOR\n", rank );
    free( in );
    free( out );
    free( sol );
}


void d15(void)
{
    unsigned short *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned short *)malloc( count * sizeof(unsigned short) );
    out = (unsigned short *)malloc( count * sizeof(unsigned short) );
    sol = (unsigned short *)malloc( count * sizeof(unsigned short) );
    for (i=0; i<count; i++) { *(in + i) = (rank == 1)*0xf0 ; *(sol + i) = (size > 1)*0xf0 ; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED_SHORT, MPI_BXOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED_SHORT and op MPI_BXOR\n", rank );
    free( in );
    free( out );
    free( sol );
}


void e15(void)
{
    unsigned *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned *)malloc( count * sizeof(unsigned) );
    out = (unsigned *)malloc( count * sizeof(unsigned) );
    sol = (unsigned *)malloc( count * sizeof(unsigned) );
    for (i=0; i<count; i++) { *(in + i) = (rank == 1)*0xf0 ; *(sol + i) = (size > 1)*0xf0 ; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED, MPI_BXOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED and op MPI_BXOR\n", rank );
    free( in );
    free( out );
    free( sol );
}


void f15(void)
{
    unsigned long *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned long *)malloc( count * sizeof(unsigned long) );
    out = (unsigned long *)malloc( count * sizeof(unsigned long) );
    sol = (unsigned long *)malloc( count * sizeof(unsigned long) );
    for (i=0; i<count; i++) { *(in + i) = (rank == 1)*0xf0 ; *(sol + i) = (size > 1)*0xf0 ; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED_LONG, MPI_BXOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED_LONG and op MPI_BXOR\n", rank );
    free( in );
    free( out );
    free( sol );
}



void a16(void)
{
    int *in, *out, *sol;
    int  i, fnderr=0;
    in = (int *)malloc( count * sizeof(int) );
    out = (int *)malloc( count * sizeof(int) );
    sol = (int *)malloc( count * sizeof(int) );
    for (i=0; i<count; i++) { *(in + i) = 0; *(sol + i) = 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_INT, MPI_BXOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_INT and op MPI_BXOR\n", rank );
    free( in );
    free( out );
    free( sol );
}


void b16(void)
{
    long *in, *out, *sol;
    int  i, fnderr=0;
    in = (long *)malloc( count * sizeof(long) );
    out = (long *)malloc( count * sizeof(long) );
    sol = (long *)malloc( count * sizeof(long) );
    for (i=0; i<count; i++) { *(in + i) = 0; *(sol + i) = 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_LONG, MPI_BXOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_LONG and op MPI_BXOR\n", rank );
    free( in );
    free( out );
    free( sol );
}


void c16(void)
{
    short *in, *out, *sol;
    int  i, fnderr=0;
    in = (short *)malloc( count * sizeof(short) );
    out = (short *)malloc( count * sizeof(short) );
    sol = (short *)malloc( count * sizeof(short) );
    for (i=0; i<count; i++) { *(in + i) = 0; *(sol + i) = 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_SHORT, MPI_BXOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_SHORT and op MPI_BXOR\n", rank );
    free( in );
    free( out );
    free( sol );
}


void d16(void)
{
    unsigned short *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned short *)malloc( count * sizeof(unsigned short) );
    out = (unsigned short *)malloc( count * sizeof(unsigned short) );
    sol = (unsigned short *)malloc( count * sizeof(unsigned short) );
    for (i=0; i<count; i++) { *(in + i) = 0; *(sol + i) = 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED_SHORT, MPI_BXOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED_SHORT and op MPI_BXOR\n", rank );
    free( in );
    free( out );
    free( sol );
}


void e16(void)
{
    unsigned *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned *)malloc( count * sizeof(unsigned) );
    out = (unsigned *)malloc( count * sizeof(unsigned) );
    sol = (unsigned *)malloc( count * sizeof(unsigned) );
    for (i=0; i<count; i++) { *(in + i) = 0; *(sol + i) = 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED, MPI_BXOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED and op MPI_BXOR\n", rank );
    free( in );
    free( out );
    free( sol );
}


void f16(void)
{
    unsigned long *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned long *)malloc( count * sizeof(unsigned long) );
    out = (unsigned long *)malloc( count * sizeof(unsigned long) );
    sol = (unsigned long *)malloc( count * sizeof(unsigned long) );
    for (i=0; i<count; i++) { *(in + i) = 0; *(sol + i) = 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED_LONG, MPI_BXOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED_LONG and op MPI_BXOR\n", rank );
    free( in );
    free( out );
    free( sol );
}


void a17(void)
{
    int *in, *out, *sol;
    int  i, fnderr=0;
    in = (int *)malloc( count * sizeof(int) );
    out = (int *)malloc( count * sizeof(int) );
    sol = (int *)malloc( count * sizeof(int) );
    for (i=0; i<count; i++) { *(in + i) = ~0; *(sol + i) = 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_INT, MPI_BXOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_INT and op MPI_BXOR\n", rank );
    free( in );
    free( out );
    free( sol );
}


void b17(void)
{
    long *in, *out, *sol;
    int  i, fnderr=0;
    in = (long *)malloc( count * sizeof(long) );
    out = (long *)malloc( count * sizeof(long) );
    sol = (long *)malloc( count * sizeof(long) );
    for (i=0; i<count; i++) { *(in + i) = ~0; *(sol + i) = 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_LONG, MPI_BXOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_LONG and op MPI_BXOR\n", rank );
    free( in );
    free( out );
    free( sol );
}


void c17(void)
{
    short *in, *out, *sol;
    int  i, fnderr=0;
    in = (short *)malloc( count * sizeof(short) );
    out = (short *)malloc( count * sizeof(short) );
    sol = (short *)malloc( count * sizeof(short) );
    for (i=0; i<count; i++) { *(in + i) = ~0; *(sol + i) = 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_SHORT, MPI_BXOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_SHORT and op MPI_BXOR\n", rank );
    free( in );
    free( out );
    free( sol );
}


void d17(void)
{
    unsigned short *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned short *)malloc( count * sizeof(unsigned short) );
    out = (unsigned short *)malloc( count * sizeof(unsigned short) );
    sol = (unsigned short *)malloc( count * sizeof(unsigned short) );
    for (i=0; i<count; i++) { *(in + i) = ~0; *(sol + i) = 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED_SHORT, MPI_BXOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED_SHORT and op MPI_BXOR\n", rank );
    free( in );
    free( out );
    free( sol );
}


void e17(void)
{
    unsigned *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned *)malloc( count * sizeof(unsigned) );
    out = (unsigned *)malloc( count * sizeof(unsigned) );
    sol = (unsigned *)malloc( count * sizeof(unsigned) );
    for (i=0; i<count; i++) { *(in + i) = ~0; *(sol + i) = 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED, MPI_BXOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED and op MPI_BXOR\n", rank );
    free( in );
    free( out );
    free( sol );
}


void f17(void)
{
    unsigned long *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned long *)malloc( count * sizeof(unsigned long) );
    out = (unsigned long *)malloc( count * sizeof(unsigned long) );
    sol = (unsigned long *)malloc( count * sizeof(unsigned long) );
    for (i=0; i<count; i++) { *(in + i) = ~0; *(sol + i) = 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED_LONG, MPI_BXOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED_LONG and op MPI_BXOR\n", rank );
    free( in );
    free( out );
    free( sol );
}



void a18(void)
{
    struct int_test { int a; int b; } *in, *out, *sol;
    int  i,fnderr=0;
    in = (struct int_test *)malloc( count * sizeof(struct int_test) );
    out = (struct int_test *)malloc( count * sizeof(struct int_test) );
    sol = (struct int_test *)malloc( count * sizeof(struct int_test) );
    for (i=0; i<count; i++) { (in + i)->a = (rank + i); (in + i)->b = rank;
    (sol + i)->a = (size - 1 + i); (sol + i)->b = (size-1);
    (out + i)->a = 0; (out + i)->b = -1; }
    MPI_Allreduce( in, out, count, MPI_2INT, MPI_MAXLOC, comm );
    for (i=0; i<count; i++) { if ((out + i)->a != (sol + i)->a ||
	(out + i)->b != (sol + i)->b) {
	    errcnt++; fnderr++; 
	    fprintf( stderr, "(%d) Expected (%d,%d) got (%d,%d)\n", rank,
		(int)((sol + i)->a),
		(sol+i)->b, (int)((out+i)->a), (out+i)->b );
	}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_2INT and op MPI_MAXLOC (%d of %d wrong)\n",
	rank, fnderr, count );
    free( in );
    free( out );
    free( sol );
}


void b18(void)
{
    struct long_test { long a; int b; } *in, *out, *sol;
    int  i,fnderr=0;
    in = (struct long_test *)malloc( count * sizeof(struct long_test) );
    out = (struct long_test *)malloc( count * sizeof(struct long_test) );
    sol = (struct long_test *)malloc( count * sizeof(struct long_test) );
    for (i=0; i<count; i++) { (in + i)->a = (rank + i); (in + i)->b = rank;
    (sol + i)->a = (size - 1 + i); (sol + i)->b = (size-1);
    (out + i)->a = 0; (out + i)->b = -1; }
    MPI_Allreduce( in, out, count, MPI_LONG_INT, MPI_MAXLOC, comm );
    for (i=0; i<count; i++) { if ((out + i)->a != (sol + i)->a ||
	(out + i)->b != (sol + i)->b) {
	    errcnt++; fnderr++; 
	    fprintf( stderr, "(%d) Expected (%d,%d) got (%d,%d)\n", rank,
		(int)((sol + i)->a),
		(sol+i)->b, (int)((out+i)->a), (out+i)->b );
	}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_LONG_INT and op MPI_MAXLOC (%d of %d wrong)\n",
	rank, fnderr, count );
    free( in );
    free( out );
    free( sol );
}


void c18(void)
{
    struct short_test { short a; int b; } *in, *out, *sol;
    int  i,fnderr=0;
    in = (struct short_test *)malloc( count * sizeof(struct short_test) );
    out = (struct short_test *)malloc( count * sizeof(struct short_test) );
    sol = (struct short_test *)malloc( count * sizeof(struct short_test) );
    for (i=0; i<count; i++) { (in + i)->a = (rank + i); (in + i)->b = rank;
    (sol + i)->a = (size - 1 + i); (sol + i)->b = (size-1);
    (out + i)->a = 0; (out + i)->b = -1; }
    MPI_Allreduce( in, out, count, MPI_SHORT_INT, MPI_MAXLOC, comm );
    for (i=0; i<count; i++) { if ((out + i)->a != (sol + i)->a ||
	(out + i)->b != (sol + i)->b) {
	    errcnt++; fnderr++; 
	    fprintf( stderr, "(%d) Expected (%d,%d) got (%d,%d)\n", rank,
		(int)((sol + i)->a),
		(sol+i)->b, (int)((out+i)->a), (out+i)->b );
	}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_SHORT_INT and op MPI_MAXLOC (%d of %d wrong)\n",
	rank, fnderr, count );
    free( in );
    free( out );
    free( sol );
}


void d18(void)
{
    struct float_test { float a; int b; } *in, *out, *sol;
    int  i,fnderr=0;
    in = (struct float_test *)malloc( count * sizeof(struct float_test) );
    out = (struct float_test *)malloc( count * sizeof(struct float_test) );
    sol = (struct float_test *)malloc( count * sizeof(struct float_test) );
    for (i=0; i<count; i++) { (in + i)->a = (rank + i); (in + i)->b = rank;
    (sol + i)->a = (size - 1 + i); (sol + i)->b = (size-1);
    (out + i)->a = 0; (out + i)->b = -1; }
    MPI_Allreduce( in, out, count, MPI_FLOAT_INT, MPI_MAXLOC, comm );
    for (i=0; i<count; i++) { if ((out + i)->a != (sol + i)->a ||
	(out + i)->b != (sol + i)->b) {
	    errcnt++; fnderr++; 
	    fprintf( stderr, "(%d) Expected (%d,%d) got (%d,%d)\n", rank,
		(int)((sol + i)->a),
		(sol+i)->b, (int)((out+i)->a), (out+i)->b );
	}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_FLOAT_INT and op MPI_MAXLOC (%d of %d wrong)\n",
	rank, fnderr, count );
    free( in );
    free( out );
    free( sol );
}


void e18(void)
{
    struct double_test { double a; int b; } *in, *out, *sol;
    int  i,fnderr=0;
    in = (struct double_test *)malloc( count * sizeof(struct double_test) );
    out = (struct double_test *)malloc( count * sizeof(struct double_test) );
    sol = (struct double_test *)malloc( count * sizeof(struct double_test) );
    for (i=0; i<count; i++) { (in + i)->a = (rank + i); (in + i)->b = rank;
    (sol + i)->a = (size - 1 + i); (sol + i)->b = (size-1);
    (out + i)->a = 0; (out + i)->b = -1; }
    MPI_Allreduce( in, out, count, MPI_DOUBLE_INT, MPI_MAXLOC, comm );
    for (i=0; i<count; i++) { if ((out + i)->a != (sol + i)->a ||
	(out + i)->b != (sol + i)->b) {
	    errcnt++; fnderr++; 
	    fprintf( stderr, "(%d) Expected (%d,%d) got (%d,%d)\n", rank,
		(int)((sol + i)->a),
		(sol+i)->b, (int)((out+i)->a), (out+i)->b );
	}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_DOUBLE_INT and op MPI_MAXLOC (%d of %d wrong)\n",
	rank, fnderr, count );
    free( in );
    free( out );
    free( sol );
}



void a19(void)
{
    struct int_test { int a; int b; } *in, *out, *sol;
    int  i,fnderr=0;
    in = (struct int_test *)malloc( count * sizeof(struct int_test) );
    out = (struct int_test *)malloc( count * sizeof(struct int_test) );
    sol = (struct int_test *)malloc( count * sizeof(struct int_test) );
    for (i=0; i<count; i++) { (in + i)->a = (rank + i); (in + i)->b = rank;
    (sol + i)->a = i; (sol + i)->b = 0;
    (out + i)->a = 0; (out + i)->b = -1; }
    MPI_Allreduce( in, out, count, MPI_2INT, MPI_MINLOC, comm );
    for (i=0; i<count; i++) { if ((out + i)->a != (sol + i)->a ||
	(out + i)->b != (sol + i)->b) {
	    errcnt++; fnderr++; 
	    fprintf( stderr, "(%d) Expected (%d,%d) got (%d,%d)\n", rank,
		(int)((sol + i)->a),
		(sol+i)->b, (int)((out+i)->a), (out+i)->b );
	}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_2INT and op MPI_MINLOC (%d of %d wrong)\n",
	rank, fnderr, count );
    free( in );
    free( out );
    free( sol );
}


void b19(void)
{
    struct long_test { long a; int b; } *in, *out, *sol;
    int  i,fnderr=0;
    in = (struct long_test *)malloc( count * sizeof(struct long_test) );
    out = (struct long_test *)malloc( count * sizeof(struct long_test) );
    sol = (struct long_test *)malloc( count * sizeof(struct long_test) );
    for (i=0; i<count; i++) { (in + i)->a = (rank + i); (in + i)->b = rank;
    (sol + i)->a = i; (sol + i)->b = 0;
    (out + i)->a = 0; (out + i)->b = -1; }
    MPI_Allreduce( in, out, count, MPI_LONG_INT, MPI_MINLOC, comm );
    for (i=0; i<count; i++) { if ((out + i)->a != (sol + i)->a ||
	(out + i)->b != (sol + i)->b) {
	    errcnt++; fnderr++; 
	    fprintf( stderr, "(%d) Expected (%d,%d) got (%d,%d)\n", rank,
		(int)((sol + i)->a),
		(sol+i)->b, (int)((out+i)->a), (out+i)->b );
	}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_LONG_INT and op MPI_MINLOC (%d of %d wrong)\n",
	rank, fnderr, count );
    free( in );
    free( out );
    free( sol );
}


void c19(void)
{
    struct short_test { short a; int b; } *in, *out, *sol;
    int  i,fnderr=0;
    in = (struct short_test *)malloc( count * sizeof(struct short_test) );
    out = (struct short_test *)malloc( count * sizeof(struct short_test) );
    sol = (struct short_test *)malloc( count * sizeof(struct short_test) );
    for (i=0; i<count; i++) { (in + i)->a = (rank + i); (in + i)->b = rank;
    (sol + i)->a = i; (sol + i)->b = 0;
    (out + i)->a = 0; (out + i)->b = -1; }
    MPI_Allreduce( in, out, count, MPI_SHORT_INT, MPI_MINLOC, comm );
    for (i=0; i<count; i++) { if ((out + i)->a != (sol + i)->a ||
	(out + i)->b != (sol + i)->b) {
	    errcnt++; fnderr++; 
	    fprintf( stderr, "(%d) Expected (%d,%d) got (%d,%d)\n", rank,
		(int)((sol + i)->a),
		(sol+i)->b, (int)((out+i)->a), (out+i)->b );
	}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_SHORT_INT and op MPI_MINLOC (%d of %d wrong)\n",
	rank, fnderr, count );
    free( in );
    free( out );
    free( sol );
}


void d19(void)
{
    struct float_test { float a; int b; } *in, *out, *sol;
    int  i,fnderr=0;
    in = (struct float_test *)malloc( count * sizeof(struct float_test) );
    out = (struct float_test *)malloc( count * sizeof(struct float_test) );
    sol = (struct float_test *)malloc( count * sizeof(struct float_test) );
    for (i=0; i<count; i++) { (in + i)->a = (rank + i); (in + i)->b = rank;
    (sol + i)->a = i; (sol + i)->b = 0;
    (out + i)->a = 0; (out + i)->b = -1; }
    MPI_Allreduce( in, out, count, MPI_FLOAT_INT, MPI_MINLOC, comm );
    for (i=0; i<count; i++) { if ((out + i)->a != (sol + i)->a ||
	(out + i)->b != (sol + i)->b) {
	    errcnt++; fnderr++; 
	    fprintf( stderr, "(%d) Expected (%d,%d) got (%d,%d)\n", rank,
		(int)((sol + i)->a),
		(sol+i)->b, (int)((out+i)->a), (out+i)->b );
	}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_FLOAT_INT and op MPI_MINLOC (%d of %d wrong)\n",
	rank, fnderr, count );
    free( in );
    free( out );
    free( sol );
}


void e19(void)
{
    struct double_test { double a; int b; } *in, *out, *sol;
    int  i,fnderr=0;
    in = (struct double_test *)malloc( count * sizeof(struct double_test) );
    out = (struct double_test *)malloc( count * sizeof(struct double_test) );
    sol = (struct double_test *)malloc( count * sizeof(struct double_test) );
    for (i=0; i<count; i++) { (in + i)->a = (rank + i); (in + i)->b = rank;
    (sol + i)->a = i; (sol + i)->b = 0;
    (out + i)->a = 0; (out + i)->b = -1; }
    MPI_Allreduce( in, out, count, MPI_DOUBLE_INT, MPI_MINLOC, comm );
    for (i=0; i<count; i++) { if ((out + i)->a != (sol + i)->a ||
	(out + i)->b != (sol + i)->b) {
	    errcnt++; fnderr++; 
	    fprintf( stderr, "(%d) Expected (%d,%d) got (%d,%d)\n", rank,
		(int)((sol + i)->a),
		(sol+i)->b, (int)((out+i)->a), (out+i)->b );
	}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_DOUBLE_INT and op MPI_MINLOC (%d of %d wrong)\n",
	rank, fnderr, count );
    free( in );
    free( out );
    free( sol );
}


int main( int argc, char **argv )
{
    MPI_Init( &argc, &argv );

    MPI_Comm_size( MPI_COMM_WORLD, &size );
    MPI_Comm_rank( MPI_COMM_WORLD, &rank );
    count = 10;
    comm = MPI_COMM_WORLD;

    /* Test sum */
#ifdef DEBUG
    if (rank == 0) printf( "Testing MPI_SUM...\n" );
#endif

    a1();
    b1();
    c1();
    d1();
    e1();
    f1();
    g1();
    h1();

    gerr += errcnt;
    if (errcnt > 0)
	printf( "Found %d errors on %d for MPI_SUM\n", errcnt, rank );
    errcnt = 0;


    /* Test product */
#ifdef DEBUG
    if (rank == 0) printf( "Testing MPI_PROD...\n" );
#endif

    a2();
    b2();
    c2();
    d2();
    e2();
    f2();
    g2();
    h2();

    gerr += errcnt;
    if (errcnt > 0)
	printf( "Found %d errors on %d for MPI_PROD\n", errcnt, rank );
    errcnt = 0;


    /* Test max */
#ifdef DEBUG
    if (rank == 0) printf( "Testing MPI_MAX...\n" );
#endif

    a3();
    b3();
    c3();
    d3();
    e3();
    f3();
    g3();
    h3();

    gerr += errcnt;
    if (errcnt > 0)
	printf( "Found %d errors on %d for MPI_MAX\n", errcnt, rank );
    errcnt = 0;

    /* Test min */
#ifdef DEBUG
    if (rank == 0) printf( "Testing MPI_MIN...\n" );
#endif

    a4();
    b4();
    c4();
    d4();
    e4();
    f4();
    g4();
    h4();

    gerr += errcnt;
    if (errcnt > 0)
	printf( "Found %d errors on %d for MPI_MIN\n", errcnt, rank );
    errcnt = 0;


    /* Test LOR */
#ifdef DEBUG
    if (rank == 0) printf( "Testing MPI_LOR...\n" );
#endif

    a5();
    b5();
    c5();
    d5();
    e5();
    f5();

    gerr += errcnt;
    if (errcnt > 0)
	printf( "Found %d errors on %d for MPI_LOR(1)\n", errcnt, rank );
    errcnt = 0;


    a6();
    b6();
    c6();
    d6();
    e6();
    f6();

    gerr += errcnt;
    if (errcnt > 0)
	printf( "Found %d errors on %d for MPI_LOR(0)\n", errcnt, rank );
    errcnt = 0;

    /* Test LXOR */
#ifdef DEBUG
    if (rank == 0) printf( "Testing MPI_LXOR...\n" );
#endif

    a7();
    b7();
    c7();
    d7();
    e7();
    f7();

    gerr += errcnt;
    if (errcnt > 0)
	printf( "Found %d errors on %d for MPI_LXOR(1)\n", errcnt, rank );
    errcnt = 0;


    a8();
    b8();
    c8();
    d8();
    e8();
    f8();

    gerr += errcnt;
    if (errcnt > 0)
	printf( "Found %d errors on %d for MPI_LXOR(0)\n", errcnt, rank );
    errcnt = 0;


    a9();
    b9();
    c9();
    d9();
    e9();
    f9();

    gerr += errcnt;
    if (errcnt > 0)
	printf( "Found %d errors on %d for MPI_LXOR(1-0)\n", errcnt, rank );
    errcnt = 0;

    /* Test LAND */
#ifdef DEBUG
    if (rank == 0) printf( "Testing MPI_LAND...\n" );
#endif

    a10();
    b10();
    c10();
    d10();
    e10();
    f10();

    gerr += errcnt;
    if (errcnt > 0)
	printf( "Found %d errors on %d for MPI_LAND(0)\n", errcnt, rank );
    errcnt = 0;


    a11();
    b11();
    c11();
    d11();
    e11();
    f11();

    gerr += errcnt;
    if (errcnt > 0)
	printf( "Found %d errors on %d for MPI_LAND(1)\n", errcnt, rank );
    errcnt = 0;

    /* Test BOR */
#ifdef DEBUG
    if (rank == 0) printf( "Testing MPI_BOR...\n" );
#endif

    a12();
    b12();
    c12();
    d12();
    e12();
    f12();
    g12();

    gerr += errcnt;
    if (errcnt > 0)
	printf( "Found %d errors on %d for MPI_BOR(1)\n", errcnt, rank );
    errcnt = 0;

    /* Test BAND */
#ifdef DEBUG
    if (rank == 0) printf( "Testing MPI_BAND...\n" );
#endif

    a13();
    b13();
    c13();
    d13();
    e13();
    f13();
    g13();

    gerr += errcnt;
    if (errcnt > 0)
	printf( "Found %d errors on %d for MPI_BAND(1)\n", errcnt, rank );
    errcnt = 0;


    a14();
    b14();
    c14();
    d14();
    e14();
    f14();

    gerr += errcnt;
    if (errcnt > 0)
	printf( "Found %d errors on %d for MPI_BAND(0)\n", errcnt, rank );
    errcnt = 0;

    /* Test BXOR */
#ifdef DEBUG
    if (rank == 0) printf( "Testing MPI_BXOR...\n" );
#endif

    a15();
    b15();
    c15();
    d15();
    e15();
    f15();

    gerr += errcnt;
    if (errcnt > 0)
	printf( "Found %d errors on %d for MPI_BXOR(1)\n", errcnt, rank );
    errcnt = 0;


    a16();
    b16();
    c16();
    d16();
    e16();
    f16();

    gerr += errcnt;
    if (errcnt > 0)
	printf( "Found %d errors on %d for MPI_BXOR(0)\n", errcnt, rank );
    errcnt = 0;


    a17();
    b17();
    c17();
    d17();
    e17();
    f17();

    gerr += errcnt;
    if (errcnt > 0)
	printf( "Found %d errors on %d for MPI_BXOR(1-0)\n", errcnt, rank );
    errcnt = 0;


    /* Test Maxloc */
#ifdef DEBUG
    if (rank == 0) printf( "Testing MPI_MAXLOC...\n" );
#endif

    a18();
    b18();
    c18();
    d18();
    e18();

    gerr += errcnt;
    if (errcnt > 0)
	printf( "Found %d errors on %d for MPI_MAXLOC\n", errcnt, rank );
    errcnt = 0;


    /* Test minloc */
#ifdef DEBUG
    if (rank == 0) printf( "Testing MPI_MINLOC...\n" );
#endif


    a19();
    b19();
    c19();
    d19();
    e19();

    gerr += errcnt;
    if (errcnt > 0)
	printf( "Found %d errors on %d for MPI_MINLOC\n", errcnt, rank );
    errcnt = 0;

    if (gerr > 0) {
	MPI_Comm_rank( MPI_COMM_WORLD, &rank );
	printf( "Found %d errors overall on %d\n", gerr, rank );
    }

    {
	int toterrs;
	MPI_Allreduce( &gerr, &toterrs, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD );
	if (rank == 0) {
	    if (toterrs) {
		printf( " Found %d errors\n", toterrs );
	    }
	    else {
		printf( " No Errors\n" );
	    }
	    fflush( stdout );
	}
    }

    MPI_Finalize( );
    return 0;
}

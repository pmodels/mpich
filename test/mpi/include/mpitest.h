/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef MPITEST_H_INCLUDED
#define MPITEST_H_INCLUDED

#include "mpitestconf.h"

/*
 * Init and finalize test 
 */
void MTest_Init( int *, char *** );
void MTest_Finalize( int );
void MTestPrintError( int );
void MTestPrintErrorMsg( const char [], int );
void MTestPrintfMsg( int, const char [], ... );
void MTestError( const char [] );

/*
 * Utilities
 */
void MTestSleep( int );

/*
 * This structure contains the information used to test datatypes
 * buf is set to null when an MTestDatatype is created; the
 * InitBuf routine will allocate (if necessary) and initialize
 * the data.  InitBuf may be called multiple times (this is particularly
 * important for recv bufs), in which case the buffer will only 
 * be allocated if it has not already been created.
 */
typedef struct _MTestDatatype {
    MPI_Datatype datatype;
    void *buf;              /* buffer to use in communication */
    int  count;             /* count to use for this datatype */
    int  isBasic;           /* true if the type is predefined */
    int  printErrors;       /* true if errors should be printed
			       (used by the CheckBuf routines) */
    /* The following is optional data that is used by some of
       the derived datatypes */
    int  stride, nelm, blksize, *index;
    /* stride, nelm, and blksize are in bytes */
    int *displs, basesize;
    /* displacements are in multiples of base type; basesize is the
       size of that type*/
    void *(*InitBuf)( struct _MTestDatatype * );
    void *(*FreeBuf)( struct _MTestDatatype * );
    int   (*CheckBuf)( struct _MTestDatatype * );
} MTestDatatype;

int MTestCheckRecv( MPI_Status *, MTestDatatype * );
int MTestGetDatatypes( MTestDatatype *, MTestDatatype *, int );
void MTestResetDatatypes( void );
void MTestFreeDatatype( MTestDatatype * );
const char *MTestGetDatatypeName( MTestDatatype * );
int MTestGetDatatypeIndex( void );

int MTestGetIntracomm( MPI_Comm *, int );
int MTestGetIntracommGeneral( MPI_Comm *, int, int );
int MTestGetIntercomm( MPI_Comm *, int *, int );
int MTestGetComm( MPI_Comm *, int );
const char *MTestGetIntracommName( void );
const char *MTestGetIntercommName( void );
void MTestFreeComm( MPI_Comm * );

#ifdef HAVE_MPI_WIN_CREATE
int MTestGetWin( MPI_Win *, int );
const char *MTestGetWinName( void );
void MTestFreeWin( MPI_Win * );
#endif

#endif

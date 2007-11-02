/* -*- Mode: C++; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef MTEST_INCLUDED
#define MTEST_INCLUDED
/*
 * Init and finalize test 
 */
void MTest_Init( void );
void MTest_Finalize( int );
void MTestPrintError( int );
void MTestPrintErrorMsg( const char [], int );
void MTestPrintfMsg( int, const char [], ... );
void MTestError( const char [] );

/*
 * This structure contains the information used to test datatypes
 */
typedef struct _MTestDatatype {
    MPI::Datatype datatype;
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

int MTestCheckRecv( MPI::Status &, MTestDatatype * );
int MTestGetDatatypes( MTestDatatype *, MTestDatatype *, int );
void MTestResetDatatypes( void );
void MTestFreeDatatype( MTestDatatype * );
const char *MTestGetDatatypeName( MTestDatatype * );

int MTestGetIntracomm( MPI::Intracomm &, int );
int MTestGetIntracommGeneral( MPI::Intracomm &, int, bool );
int MTestGetIntercomm( MPI::Intercomm &, int &, int );
int MTestGetComm( MPI::Comm **, int );
const char *MTestGetIntracommName( void );
const char *MTestGetIntercommName( void );
void MTestFreeComm( MPI::Comm &comm );

#ifdef HAVE_MPI_WIN_CREATE
int MTestGetWin( MPI::Win &, bool );
const char *MTestGetWinName( void );
void MTestFreeWin( MPI::Win & );
#endif

#endif

/* -*- Mode: C++; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPITESTCXX_H_INCLUDED
#define MPITESTCXX_H_INCLUDED

#ifndef MPITESTCONF_H_INCLUDED
#error Required mpitestconf.h file not included first!
#endif

#include <string.h>
/*
 * Init and finalize test
 */
void MTest_Init(void);
void MTest_Finalize(int);
void MTestPrintError(int);
void MTestPrintErrorMsg(const char[], int);
void MTestPrintfMsg(int, const char[], ...);
void MTestError(const char[]);

int MTestInitBasicSignatureX(int, char **, int *, MPI::Datatype *);

int MTestGetIntracomm(MPI::Intracomm &, int);
int MTestGetIntracommGeneral(MPI::Intracomm &, int, bool);
int MTestGetIntercomm(MPI::Intercomm &, int &, int);
int MTestGetComm(MPI::Comm **, int);
const char *MTestGetIntracommName(void);
const char *MTestGetIntercommName(void);
void MTestFreeComm(MPI::Comm & comm);

int MTestSpawnPossible(int *);

#ifdef HAVE_MPI_WIN_CREATE
int MTestGetWin(MPI::Win &, bool);
const char *MTestGetWinName(void);
void MTestFreeWin(MPI::Win &);
#endif

/* useful for avoid valgrind warnings about padding bytes */
#define MTEST_VG_MEM_INIT(addr_, size_) \
do {                                    \
    memset(addr_, 0, size_);            \
} while (0)

#endif /* MPITESTCXX_H_INCLUDED */

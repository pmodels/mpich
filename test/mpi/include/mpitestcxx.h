/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPITESTCXX_H_INCLUDED
#define MPITESTCXX_H_INCLUDED

#include "mpi.h"
#include "mpitestconf.h"

/* *INDENT-OFF* */
extern "C" {
#include "mtest_common.h"
}
/* *INDENT-ON* */

#ifdef HAVE_IOSTREAM
#include <iostream>
#ifdef HAVE_NAMESPACE_STD
using namespace std;
#endif
#else
#include <iostream.h>
#endif
#include <string.h>

/* Init and finalize test */
void MTest_Init(void);
void MTest_Finalize(int);
void MTestPrintError(int);
void MTestPrintErrorMsg(const char[], int);
void MTestPrintfMsg(int, const char[], ...);
void MTestError(const char[]);

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

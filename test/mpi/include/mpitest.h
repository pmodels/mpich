/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPITEST_H_INCLUDED
#define MPITEST_H_INCLUDED

#include <string.h>
#include <mpi.h>
#include "mpitestconf.h"
#include "mpithreadtest.h"
#include "mtest_common.h"

/*
 * Init and finalize test
 */
void MTest_Init(int *, char ***);
void MTest_Init_thread(int *, char ***, int, int *);
void MTest_Finalize(int);
void MTestPrintError(int);
void MTestPrintErrorMsg(const char[], int);
void MTestPrintfMsg(int, const char[], ...);
void MTestError(const char[]);
int MTestReturnValue(int);

/*
 * Utilities
 */
void MTestSleep(int);
void MTestGetDbgInfo(int *dbgflag, int *verbose);

int MTestGetIntracomm(MPI_Comm *, int);
int MTestGetIntracommGeneral(MPI_Comm *, int, int);
int MTestGetIntercomm(MPI_Comm *, int *, int);
int MTestGetComm(MPI_Comm *, int);
int MTestTestIntercomm(MPI_Comm intercomm);
int MTestTestIntracomm(MPI_Comm intracomm);
int MTestTestComm(MPI_Comm comm);
const char *MTestGetIntracommName(void);
const char *MTestGetIntercommName(void);
void MTestFreeComm(MPI_Comm *);
void MTestCommRandomize(void);

int MTestSpawnPossible(int *);

#ifdef HAVE_MPI_WIN_CREATE
int MTestGetWin(MPI_Win *, int);
const char *MTestGetWinName(void);
void MTestFreeWin(MPI_Win *);
#endif

int MTestIsBasicDtype(MPI_Datatype type);

/* These macros permit overrides via:
 *     make CPPFLAGS='-DMTEST_MPI_VERSION=X -DMTEST_MPI_SUBVERSION=Y'
 * where X and Y are the major and minor versions of the MPI standard that is
 * being tested.  The override should work similarly if added to the CPPFLAGS at
 * configure time. */
#ifndef MTEST_MPI_VERSION
#define MTEST_MPI_VERSION MPI_VERSION
#endif
#ifndef MTEST_MPI_SUBVERSION
#define MTEST_MPI_SUBVERSION MPI_SUBVERSION
#endif

/* Makes it easier to conditionally compile test code for a particular minimum
 * version of the MPI Standard.  Right now there is only a MIN flavor but it
 * would be easy to add MAX or EXACT flavors if they become necessary at some
 * point.  Example usage:
 ------8<-------
#if MTEST_HAVE_MIN_MPI_VERSION(2,2)
... test for some feature that is only available in MPI-2.2 or later ...
#endif
 ------8<-------
 */
#define MTEST_HAVE_MIN_MPI_VERSION(major_,minor_) \
    ((MTEST_MPI_VERSION == (major_) && MTEST_MPI_SUBVERSION >= (minor_)) ||   \
    (MTEST_MPI_VERSION > (major_)))

/* useful for avoid valgrind warnings about padding bytes */
#define MTEST_VG_MEM_INIT(addr_, size_) \
do {                                    \
    memset(addr_, 0, size_);            \
} while (0)

#define MTEST_CREATE_AND_FREE_DTP_OBJS(dtp_, maxbufsize_, testsize_) ({ \
    int errs_ = 0;                                                      \
    do {                                                                \
        int i_, err_;                                                   \
        DTP_obj_s obj_;                                                 \
        for(i_ = 0; i_ < testsize_; i_++) {                             \
            err_ = DTP_obj_create(dtp_, &obj_, maxbufsize_);            \
            if (err_ != DTP_SUCCESS) {                                  \
                errs_++;                                                \
                break;                                                  \
            }                                                           \
            DTP_obj_free(obj_);                                         \
        }                                                               \
    } while (0);                                                        \
    errs_; })

#endif /* MPITEST_H_INCLUDED */

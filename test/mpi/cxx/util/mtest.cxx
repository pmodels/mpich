/* -*- Mode: C++; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include "mpitestconf.h"
#ifdef HAVE_IOSTREAM
// Not all C++ compilers have iostream instead of iostream.h
#include <iostream>
#ifdef HAVE_NAMESPACE_STD
// Those that do often need the std namespace; otherwise, a bare "cout"
// is likely to fail to compile
using namespace std;
#endif
#else
#include <iostream.h>
#endif
#include "mpitestcxx.h"
#include <stdlib.h>
#include <string.h>

static int dbgflag = 0;         /* Flag used for debugging */
static int wrank = -1;          /* World rank */
static int verbose = 0;         /* Message level (0 is none) */

static void MTestRMACleanup(void);

/*
 * Initialize and Finalize MTest
 */

/*
   Initialize MTest, initializing MPI if necessary.

 Environment Variables:
+ MPITEST_DEBUG - If set (to any value), turns on debugging output
- MPITEST_VERBOSE - If set to a numeric value, turns on that level of
  verbose output.

 */
void MTest_Init(void)
{
    bool flag;
    const char *envval = 0;
    int threadLevel, provided;

    threadLevel = MPI::THREAD_SINGLE;
    envval = getenv("MTEST_THREADLEVEL_DEFAULT");
    if (envval && *envval) {
        if (strcmp(envval, "MULTIPLE") == 0 || strcmp(envval, "multiple") == 0) {
            threadLevel = MPI::THREAD_MULTIPLE;
        } else if (strcmp(envval, "SERIALIZED") == 0 || strcmp(envval, "serialized") == 0) {
            threadLevel = MPI::THREAD_SERIALIZED;
        } else if (strcmp(envval, "FUNNELED") == 0 || strcmp(envval, "funneled") == 0) {
            threadLevel = MPI::THREAD_FUNNELED;
        } else if (strcmp(envval, "SINGLE") == 0 || strcmp(envval, "single") == 0) {
            threadLevel = MPI::THREAD_SINGLE;
        } else {
            cerr << "Unrecognized thread level " << envval << "\n";
            cerr.flush();
            /* Use exit since MPI_Init/Init_thread has not been called. */
            exit(1);
        }
    }

    flag = MPI::Is_initialized();
    if (!flag) {
        provided = MPI::Init_thread(threadLevel);
    }
#if defined(HAVE_MPI_IO)
    MPI::FILE_NULL.Set_errhandler(MPI::ERRORS_THROW_EXCEPTIONS);
#endif

    /* Check for debugging control */
    if (getenv("MPITEST_DEBUG")) {
        dbgflag = 1;
        wrank = MPI::COMM_WORLD.Get_rank();
    }

    /* Check for verbose control */
    envval = getenv("MPITEST_VERBOSE");
    if (envval) {
        char *s;
        long val = strtol(envval, &s, 0);
        if (s == envval) {
            /* This is the error case for strtol */
            cerr << "Warning: " << envval << " not valid for MPITEST_VERBOSE\n";
            cerr.flush();
        } else {
            if (val >= 0) {
                verbose = val;
            } else {
                cerr << "Warning: " << envval << " not valid for MPITEST_VERBOSE\n";
                cerr.flush();
            }
        }
    }
}

/*
  Finalize MTest.  errs is the number of errors on the calling process;
  this routine will write the total number of errors over all of MPI_COMM_WORLD
  to the process with rank zero, or " No Errors".
 */
void MTest_Finalize(int errs)
{
    int rank, toterrs;

    rank = MPI::COMM_WORLD.Get_rank();

    MPI::COMM_WORLD.Allreduce(&errs, &toterrs, 1, MPI::INT, MPI::SUM);
    if (rank == 0) {
        if (toterrs) {
            cout << " Found " << toterrs << " errors\n";
        } else {
            cout << " No Errors\n";
        }
        cout.flush();
    }
    // Clean up any persistent objects that we allocated
    MTestRMACleanup();

    MPI::Finalize();
}

/* ----------------------------------------------------------------------- */

/*
 * Create communicators.  Use separate routines for inter and intra
 * communicators (there is a routine to give both)
 * Note that the routines may return MPI::COMM_NULL, so code should test for
 * that return value as well.
 *
 */
static int interCommIdx = 0;
static int intraCommIdx = 0;
static const char *intraCommName = 0;
static const char *interCommName = 0;

/*
 * Get an intracommunicator with at least min_size members.  If "allowSmaller"
 * is true, allow the communicator to be smaller than MPI::COMM_WORLD and
 * for this routine to return MPI::COMM_NULL for some values.  Returns 0 if
 * no more communicators are available.
 */
int MTestGetIntracommGeneral(MPI::Intracomm & comm, int min_size, bool allowSmaller)
{
    int size, rank;
    bool done = false;
    bool isBasic = false;

    /* The while loop allows us to skip communicators that are too small.
     * MPI::COMM_NULL is always considered large enough */
    while (!done) {
        switch (intraCommIdx) {
            case 0:
                comm = MPI::COMM_WORLD;
                isBasic = true;
                intraCommName = "MPI::COMM_WORLD";
                break;
            case 1:
                /* dup of world */
                comm = MPI::COMM_WORLD.Dup();
                intraCommName = "Dup of MPI::COMM_WORLD";
                break;
            case 2:
                /* reverse ranks */
                size = MPI::COMM_WORLD.Get_size();
                rank = MPI::COMM_WORLD.Get_rank();
                comm = MPI::COMM_WORLD.Split(0, size - rank);
                intraCommName = "Rank reverse of MPI::COMM_WORLD";
                break;
            case 3:
                /* subset of world, with reversed ranks */
                size = MPI::COMM_WORLD.Get_size();
                rank = MPI::COMM_WORLD.Get_rank();
                comm = MPI::COMM_WORLD.Split((rank < size / 2), size - rank);
                intraCommName = "Rank reverse of half of MPI::COMM_WORLD";
                break;
            case 4:
                comm = MPI::COMM_SELF;
                isBasic = true;
                intraCommName = "MPI::COMM_SELF";
                break;

                /* These next cases are communicators that include some
                 * but not all of the processes */
            case 5:
            case 6:
            case 7:
            case 8:
                {
                    int newsize;
                    size = MPI::COMM_WORLD.Get_size();
                    newsize = size - (intraCommIdx - 4);

                    if (allowSmaller && newsize >= min_size) {
                        rank = MPI::COMM_WORLD.Get_rank();
                        comm = MPI::COMM_WORLD.Split(rank < newsize, rank);
                        if (rank >= newsize) {
                            comm.Free();
                            comm = MPI::COMM_NULL;
                        }
                    } else {
                        /* Act like default */
                        comm = MPI::COMM_NULL;
                        isBasic = true;
                        intraCommName = "MPI::COMM_NULL";
                        intraCommIdx = -1;
                    }
                }
                break;

                /* Other ideas: dup of self, cart comm, graph comm */
            default:
                comm = MPI::COMM_NULL;
                isBasic = true;
                intraCommName = "MPI::COMM_NULL";
                intraCommIdx = -1;
                break;
        }

        if (comm != MPI::COMM_NULL) {
            size = comm.Get_size();
            if (size >= min_size)
                done = true;
            else {
                /* Try again */
                if (!isBasic)
                    comm.Free();
                intraCommIdx++;
            }
        } else
            done = true;
    }

    intraCommIdx++;
    return intraCommIdx;
}

/*
 * Get an intracommunicator with at least min_size members.
 */
int MTestGetIntracomm(MPI::Intracomm & comm, int min_size)
{
    return MTestGetIntracommGeneral(comm, min_size, false);
}

/* Return the name of an intra communicator */
const char *MTestGetIntracommName(void)
{
    return intraCommName;
}

/*
 * Return an intercomm; set isLeftGroup to 1 if the calling process is
 * a member of the "left" group.
 */
int MTestGetIntercomm(MPI::Intercomm & comm, int &isLeftGroup, int min_size)
{
    int size, rank, remsize;
    bool done = false;
    MPI::Intracomm mcomm;
    int rleader;

    /* The while loop allows us to skip communicators that are too small.
     * MPI::COMM_NULL is always considered large enough.  The size is
     * the sum of the sizes of the local and remote groups */
    while (!done) {
        comm = MPI::COMM_NULL;
        isLeftGroup = 0;
        interCommName = "MPI_COMM_NULL";

        switch (interCommIdx) {
            case 0:
                /* Split comm world in half */
                rank = MPI::COMM_WORLD.Get_rank();
                size = MPI::COMM_WORLD.Get_size();
                if (size > 1) {
                    mcomm = MPI::COMM_WORLD.Split((rank < size / 2), rank);
                    if (rank == 0) {
                        rleader = size / 2;
                    } else if (rank == size / 2) {
                        rleader = 0;
                    } else {
                        /* Remote leader is signficant only for the processes
                         * designated local leaders */
                        rleader = -1;
                    }
                    isLeftGroup = rank < size / 2;
                    comm = mcomm.Create_intercomm(0, MPI::COMM_WORLD, rleader, 12345);
                    mcomm.Free();
                    interCommName = "Intercomm by splitting MPI::COMM_WORLD";
                } else {
                    comm = MPI::COMM_NULL;
                }
                break;
            case 1:
                /* Split comm world in to 1 and the rest */
                rank = MPI::COMM_WORLD.Get_rank();
                size = MPI::COMM_WORLD.Get_size();
                if (size > 1) {
                    mcomm = MPI::COMM_WORLD.Split(rank == 0, rank);
                    if (rank == 0) {
                        rleader = 1;
                    } else if (rank == 1) {
                        rleader = 0;
                    } else {
                        /* Remote leader is signficant only for the processes
                         * designated local leaders */
                        rleader = -1;
                    }
                    isLeftGroup = rank == 0;
                    comm = mcomm.Create_intercomm(0, MPI::COMM_WORLD, rleader, 12346);
                    mcomm.Free();
                    interCommName = "Intercomm by splitting MPI::COMM_WORLD into 1, rest";
                } else {
                    comm = MPI::COMM_NULL;
                }
                break;

            case 2:
                /* Split comm world in to 2 and the rest */
                rank = MPI::COMM_WORLD.Get_rank();
                size = MPI::COMM_WORLD.Get_size();
                if (size > 3) {
                    mcomm = MPI::COMM_WORLD.Split(rank < 2, rank);
                    if (rank == 0) {
                        rleader = 2;
                    } else if (rank == 2) {
                        rleader = 0;
                    } else {
                        /* Remote leader is signficant only for the processes
                         * designated local leaders */
                        rleader = -1;
                    }
                    isLeftGroup = rank < 2;
                    comm = mcomm.Create_intercomm(0, MPI::COMM_WORLD, rleader, 12347);
                    mcomm.Free();
                    interCommName = "Intercomm by splitting MPI::COMM_WORLD into 2, rest";
                } else {
                    comm = MPI::COMM_NULL;
                }
                break;

            default:
                comm = MPI::COMM_NULL;
                interCommName = "MPI::COMM_NULL";
                interCommIdx = -1;
                break;
        }
        if (comm != MPI::COMM_NULL) {
            size = comm.Get_size();
            remsize = comm.Get_remote_size();
            if (size + remsize >= min_size)
                done = true;
        } else
            done = true;

        /* we are only done if all processes are done */
        MPI::COMM_WORLD.Allreduce(MPI_IN_PLACE, &done, 1, MPI::BOOL, MPI::LAND);

        /* Advance the comm index whether we are done or not, otherwise we could
         * spin forever trying to allocate a too-small communicator over and
         * over again. */
        interCommIdx++;

        if (!done && comm != MPI::COMM_NULL) {
            comm.Free();
        }
    }

    return interCommIdx;
}

/* Return the name of an intercommunicator */
const char *MTestGetIntercommName(void)
{
    return interCommName;
}

/* Get a communicator of a given minimum size.  Both intra and inter
   communicators are provided
   Because Comm is an abstract base class, you can only have references
   to a Comm.*/
int MTestGetComm(MPI::Comm ** comm, int min_size)
{
    int idx;
    static int getinter = 0;

    if (!getinter) {
        MPI::Intracomm rcomm;
        idx = MTestGetIntracomm(rcomm, min_size);
        if (idx == 0) {
            getinter = 1;
        } else {
            MPI::Intracomm * ncomm = new MPI::Intracomm(rcomm);
            *comm = ncomm;
        }
    }
    if (getinter) {
        MPI::Intercomm icomm;
        int isLeft;
        idx = MTestGetIntercomm(icomm, isLeft, min_size);
        if (idx == 0) {
            getinter = 0;
        } else {
            MPI::Intercomm * ncomm = new MPI::Intercomm(icomm);
            *comm = ncomm;
        }
    }

    return idx;
}

/* Free a communicator.  It may be called with a predefined communicator
 or MPI_COMM_NULL */
void MTestFreeComm(MPI::Comm & comm)
{
    if (comm != MPI::COMM_WORLD && comm != MPI::COMM_SELF && comm != MPI::COMM_NULL) {
        comm.Free();
    }
}

/* ------------------------------------------------------------------------ */
void MTestPrintError(int errcode)
{
    int errclass, slen;
    char string[MPI_MAX_ERROR_STRING];

    errclass = MPI::Get_error_class(errcode);
    MPI::Get_error_string(errcode, string, slen);
    cout << "Error class " << errclass << "(" << string << ")\n";
    cout.flush();
}

void MTestPrintErrorMsg(const char msg[], int errcode)
{
    int errclass, slen;
    char string[MPI_MAX_ERROR_STRING];

    errclass = MPI::Get_error_class(errcode);
    MPI::Get_error_string(errcode, string, slen);
    cout << msg << ": Error class " << errclass << " (" << string << ")\n";
    cout.flush();
}

/* ------------------------------------------------------------------------ */
/* Fatal error.  Report and exit */
void MTestError(const char *msg)
{
    cerr << msg << "\n";
    cerr.flush();
    MPI::COMM_WORLD.Abort(1);
}

#ifdef HAVE_MPI_WIN_CREATE
/*
 * Create MPI Windows
 */
static int win_index = 0;
static const char *winName;
/* Use an attribute to remember the type of memory allocation (static,
   malloc, or MPI_Alloc_mem) */
static int mem_keyval = MPI::KEYVAL_INVALID;
int MTestGetWin(MPI::Win & win, bool mustBePassive)
{
    static char actbuf[1024];
    static char *pasbuf;
    char *buf;
    int n, rank;
    MPI::Info info;

    if (mem_keyval == MPI::KEYVAL_INVALID) {
        /* Create the keyval */
        mem_keyval = MPI::Win::Create_keyval(MPI::Win::NULL_COPY_FN, MPI::Win::NULL_DELETE_FN, 0);
    }

    switch (win_index) {
        case 0:
            /* Active target window */
            win = MPI::Win::Create(actbuf, 1024, 1, MPI::INFO_NULL, MPI::COMM_WORLD);
            winName = "active-window";
            win.Set_attr(mem_keyval, (void *) 0);
            break;
        case 1:
            /* Passive target window */
            pasbuf = (char *) MPI::Alloc_mem(1024, MPI::INFO_NULL);
            win = MPI::Win::Create(pasbuf, 1024, 1, MPI::INFO_NULL, MPI::COMM_WORLD);
            winName = "passive-window";
            win.Set_attr(mem_keyval, (void *) 2);
            break;
        case 2:
            /* Active target; all windows different sizes */
            rank = MPI::COMM_WORLD.Get_rank();
            n = rank * 64;
            if (n)
                buf = (char *) malloc(n);
            else
                buf = 0;
            win = MPI::Win::Create(buf, n, 1, MPI::INFO_NULL, MPI::COMM_WORLD);
            winName = "active-all-different-win";
            win.Set_attr(mem_keyval, (void *) 1);
            break;
        case 3:
            /* Active target, no locks set */
            rank = MPI::COMM_WORLD.Get_rank();
            n = rank * 64;
            if (n)
                buf = (char *) malloc(n);
            else
                buf = 0;
            info = MPI::Info::Create();
            info.Set("nolocks", "true");
            win = MPI::Win::Create(buf, n, 1, info, MPI::COMM_WORLD);
            info.Free();
            winName = "active-nolocks-all-different-win";
            win.Set_attr(mem_keyval, (void *) 1);
            break;
        default:
            win_index = -1;
    }
    win_index++;
    return win_index;
}

/* Return a pointer to the name associated with a window object */
const char *MTestGetWinName(void)
{

    return winName;
}

/* Free the storage associated with a window object */
void MTestFreeWin(MPI::Win & win)
{
    void *addr;
    bool flag;

    flag = win.Get_attr(MPI_WIN_BASE, &addr);
    if (!flag) {
        MTestError("Could not get WIN_BASE from window");
    }
    if (addr) {
        void *val;
        flag = win.Get_attr(mem_keyval, &val);
        if (flag) {
            if (val == (void *) 1) {
                free(addr);
            } else if (val == (void *) 2) {
                MPI::Free_mem(addr);
            }
            /* if val == (void *)0, then static data that must not be freed */
        }
    }
    win.Free();
}

static void MTestRMACleanup(void)
{
    if (mem_keyval != MPI::KEYVAL_INVALID) {
        MPI::Win::Free_keyval(mem_keyval);
    }
}
#else
static void MTestRMACleanup(void)
{
}
#endif

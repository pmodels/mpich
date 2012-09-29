/*===========================================================================
 *
 * Name:	testmpio.c
 *
 * Functions:
 *	dotests
 *	main
 *	test_collective
 *	test_datareps
 *	test_errhandlers
 *	test_filecontrol
 *	test_localpointer
 *	test_manycomms
 *	test_manyopens
 *	test_openclose
 *	test_openmodes
 *	test_nb_collective
 *	test_nb_localpointer
 *	test_nb_rdwr
 *	test_nb_readwrite
 *	test_nb_sharedpointer
 *	test_rdwr
 *	test_readwrite
 *	test_sharedpointer
 *
 *	some_handler
 *	user_handler
 *	user_native_extent_fn
 *	user16_extent_fn
 *	user16_read_fn
 *	user16_write_fn
 *	write_then_read
 *
 * Description:
 *	Tests MPI-IO interface functions.  Includes functions
 *	for testing collective open and close of files, independent
 *	read and write with using file and buffer types, file open
 *	modes, multiple opens of the same file, and access by
 *	multiple communicators, file control, error handling,
 *	and datarep and data conversion facilities.
 *
 * Traceability:
 *	Version		Date		Description
 *	-------		----		-----------
 *	3.0		01/09/98	initial HPSS version
 *	3.1		04/29/98	added test_datareps; mods for
 *					public release versions
 *	3.2		06/10/98	completed error checking
 *	3.3		06/22/98	fixed Testany/Waitany bug
 *	3.4P		06/30/98	error handling mods
 *	3.5P		08/18/98	added barriers in test_filecontrol
 *
 * Notes:
 *
 *	See the accompanying file LEGAL-NOTICE for disclaimers and
 *	information on commercial use.
 *
 *-------------------------------------------------------------------------*/

#include "mpi.h"

/* Temporary for MPICH2 testing */
#ifdef EARLY_MPICH2
#define MPI_Wait MPIO_Wait
#define MPI_Request MPIO_Request
#define MPI_Waitall MPIO_Waitall
#define MPI_Waitsome MPIO_Waitsome
#define MPI_Waitany MPIO_Waitany
#define MPI_Test MPIO_Test
#define MPI_Testall MPIO_Testall
#define MPI_Testsome MPIO_Testsome
#define MPI_Testany MPIO_Testany
#endif
/* End of temporary MPICH2 */

#ifdef MPICH2
/* testmpio requries this definition to use the varargs form
   of the user-defined error handlers */
#define USE_STDARG
#endif

/* define this if the MPI implementation sets the status value
   when an error occurs.  This is not required by the MPI
   standard, and user programs should not use status after an
   error return.
*/
/* #define STATUS_VALID_AFTER_ERROR */

/* Define the following to suppress non-local error tests */
#define NO_NON_LOCAL_ERRTESTS

/* Define VERBOSE to get more information */
/* #define VERBOSE */

/* Define DETAIL to be
      DETAIL_NONE (error messages only)
      DETAIL_BASE (messages about tests from process zero only)
      DETAIL_ALL  (all messages)
*/
#define DETAIL_NONE 0
#define DETAIL_BASE 1
#define DETAIL_ALL 2
/* You can define DETAIL when compiling to change the level of detail */
/* TODO: This should be runtime, rather than compile time, as the overhead
   is modest */
#ifndef DETAIL
/*#define DETAIL DETAIL_BASE*/
#define DETAIL DETAIL_NONE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>     /* For getpid */
#include <string.h>     /* For memset */
#include <time.h>

#define BUFLEN 80

#ifndef MAX_PATH_LEN
#define MAX_PATH_LEN 1024
#endif

#ifndef TRUE
#define TRUE 1
#endif /* TRUE */

#ifndef FALSE
#define FALSE 0
#endif /* FALSE */

/* Detail levels: 0=none, 1=BASIC, 2=ALL */
static int verbose = 0;
/* the "do...while" makes these look like a regular statement, avoiding
   bugs if these are used with sloppy coding */
#define ENTERING(fun)                                                   \
    do {                                                                \
        if (verbose && wrank == 0)                                      \
            fprintf(stderr, "(%d)FLOWTRACE: Entering %s\n", myid, fun); \
    } while (0)
#define LEAVING(fun)                                                    \
    do {                                                                \
        if (verbose && wrank == 0)                                      \
            fprintf(stderr, "(%d)FLOWTRACE: Leaving %s\n", myid, fun);  \
    } while (0)
#define DEBUG_TRACE(str)                                \
    do {                                                \
        if (verbose && wrank == 0)                      \
            fprintf(stderr, "(%d)%s\n", myid, str);     \
    } while (0)

/* We could use snprintf, but this is a workable solution for these*/
#define MAKEFILENAME(name, namelen, prefix, path, id)                   \
    do {                                                                \
        if (strlen(prefix) + strlen(path) + 11 > namelen) {             \
            fprintf(stderr, "name (length %lu) too short for file name at line %d\n", \
                    namelen, __LINE__);                                 \
            MPI_Abort(MPI_COMM_WORLD, 1);                               \
        }                                                               \
        else {                                                          \
            sprintf(name, "%s/%s%d", path, prefix, id);                 \
        }                                                               \
    } while (0)

#define MAKEFILENAME_I(name, namelen, prefix, path, id, i)              \
    do {                                                                \
        if (strlen(prefix) + strlen(path) + 22 > namelen) {             \
            fprintf(stderr, "name (length %lu) too short for file name at line %d\n", \
                    namelen, __LINE__);                                 \
            MPI_Abort(MPI_COMM_WORLD, 1);                               \
        }                                                               \
        else {                                                          \
            sprintf(name, "%s/%s%d_%d", path, prefix, i, id);           \
        }                                                               \
    } while (0)

#define MAKEFILENAME_S(name, namelen, prefix, path, suffix, id)         \
    do {                                                                \
        if (strlen(prefix) + strlen(path) + strlen(suffix)+ 12 > namelen) { \
            fprintf(stderr, "name (length %lu) too short for file name at line %d\n", \
                    namelen, __LINE__);                                 \
            MPI_Abort(MPI_COMM_WORLD, 1);                               \
        }                                                               \
        else {                                                          \
            sprintf(name, "%s/%s%d_%s", path, prefix, id, suffix);      \
        }                                                               \
    } while (0)

#define CHECK(a)                                                        \
    {                                                                   \
	if (!(a)) {                                                     \
            fprintf(stderr,                                             \
                    "(%d)Error at node %d, line %d of test program\n",  \
                    myid, myid, __LINE__);                              \
	}                                                               \
    }

#define CHECKINTS(a, b)                                                 \
    {                                                                   \
	CHECK((int)(a) == (int)(b));                                    \
	if ((int)(a) != (int)(b)) {                                     \
            fprintf(stderr, "(%d)got: %x expected: %x, line %d of test program\n", \
                    myid, (int)(a), (int)(b), __LINE__);		\
	}                                                               \
    }

/* The original code had CHECK((int)(a) ==(int)(b)) as the first executable
   line.  However, this is not correct for tests like

   CHECKERRS(mpio_result, MPI_ERR_FILE)

   since the error *code* in mpio_result may not be MPI_ERR_FILE, even
   though the error *class* is.  To fix this, the CHECK call is moved
   into the final test on the error class
   WDG - 9/4/03
*/
#define CHECKERRS(a, b)                                                 \
    {                                                                   \
	char got_string[MPI_MAX_ERROR_STRING];                          \
	char expected_string[MPI_MAX_ERROR_STRING];                     \
	int  string_len;                                                \
	int  ca, cb;                                                    \
	if ((int)(a) != (int)(b)) {                                     \
            MPI_Error_class(a, &ca);                                    \
            MPI_Error_class(b, &cb);                                    \
            CHECKINTS(ca, cb);                                          \
            if (ca != cb) {                                             \
                CHECK((int)(a) == (int)(b));                            \
                MPI_Error_string(a, got_string, &string_len);           \
                MPI_Error_string(b, expected_string, &string_len);      \
                fprintf(stderr, "(%d)got %d in class %d:  %s\n",        \
                        myid, (int)(a), (int)ca, got_string);           \
                fprintf(stderr, "(%d)expected %d in class %d:  %s\n",   \
                        myid, (int)(b), (int)cb, expected_string);      \
            }                                                           \
	}                                                               \
    }
/* WDG, 3/27/12.
   Check that the error (a) is of either the error class in b or c.
   If the test fails, report using error class b (c is not the preferred class)
 */
#define CHECKERRS2(a, b, c)                                             \
    {                                                                   \
	char got_string[MPI_MAX_ERROR_STRING];                          \
	char expected_string[MPI_MAX_ERROR_STRING];                     \
	int  string_len;                                                \
	int  ca;                                                        \
	if ((int)(a) != (int)(b)) {                                     \
	    MPI_Error_class(a, &ca);                                    \
	    if (ca != b && ca != c) {                                   \
		MPI_Error_string(a, got_string, &string_len);           \
		MPI_Error_string(b, expected_string, &string_len);      \
		CHECKINTS(ca, b);                                       \
		if (ca != b) {                                          \
                    CHECK((int)(a) == (int)(b));                        \
                    fprintf(stderr, "(%d)got %d in class %d:  %s\n",    \
                            myid, (int)(a), (int)ca, got_string);       \
                    fprintf(stderr, "(%d)expected %d in class %d:  %s\n", \
                            myid, (int)(b), (int)b, expected_string);   \
		}                                                       \
	    }                                                           \
	}                                                               \
    }

#define RETURN                                  \
    goto endtest;

#define RETURNERRS(a, b)                        \
    CHECKERRS(a, b);                            \
    if ((int)(a) != (int)(b))                   \
        RETURN;

#define EXITERR(a)                                              \
    if ((a) != MPI_SUCCESS) {                                   \
        fprintf(stderr,"Failed to initialize MPI: %d\n", a);    \
        exit(-1);                                               \
    }

#define CHECKSTRINGS(a, b, count)                                       \
    {                                                                   \
        int check;                                                      \
	check = memcmp(a, b, count);                                    \
	CHECK(check == 0);                                              \
	if (check != 0) {                                               \
            {   int i;                                                  \
                for (i=0; i<count; i++)                                 \
                    if (a[i] != b[i]) {                                 \
                        fprintf(stderr,                                 \
				"(%d)Error:  sent[%d]: %c (%d) "        \
				"received[%d]: %c (%d)\n",              \
				myid, i, a[i], a[i], i, b[i], b[i]);    \
                        break;                                          \
                    }                                                   \
            }                                                           \
	}                                                               \
    }

void dotests(void);
void test_collective(int numprocs, int myid);
#ifdef HAVE_MPI_REGISTER_DATAREP
void test_datareps(int numprocs, int myid);
#endif
void test_errhandlers(int numprocs, int myid);
void test_filecontrol(int numprocs, int myid);
void test_localpointer(int numprocs, int myid);
void test_manyopens(int numprocs, int myid);
void test_manycomms(int numprocs, int myid);
void test_openclose(int numprocs, int myid);
void test_openmodes(int numprocs, int myid);
void test_nb_collective(int numprocs, int myid);
void test_nb_localpointer(int numprocs, int myid);
void test_nb_rdwr(int numprocs, int myid);
void test_nb_readwrite(int numprocs, int myid);
void test_nb_sharedpointer(int numprocs, int myid);
void test_rdwr(int numprocs, int myid);
void test_readwrite(int numprocs, int myid);
void test_sharedpointer(int numprocs, int myid);

static int basepid;
static char userpath[MAX_PATH_LEN];
static char default_path[] = "./";
static int wrank = 0;           /* Rank in MPI_COMM_WORLD */

MPI_Info hints_to_test = MPI_INFO_NULL;

/*===========================================================================
 *
 * Function:	main
 *
 * Synopsis:
 *	int main(
 *		int	argc,		** IN
 *		char **	argv		** IN
 *	)
 *
 * Parameters:
 *	argc		number of command line args
 *	argv		command line argument strings
 *
 * Description:
 *	Initializes MPI-IO and performs the mpio tests (dotests).  Tests
 *	will be executed repeatedly, according to a command line
 *	argument.  Simple timing results are reported.
 *
 * Other Inputs:
 *	Command line arguments:
 *		<#iterations>	number of iterations of tests to perform
 *
 * Outputs:
 *	Prints out various test results.
 *
 * Interfaces:
 *	dotests
 *	MPI_Bcast
 *	MPI_Comm_rank
 *	MPI_Err_handler_set
 *	MPI_Finalize
 *	MPI_Get_version
 *	MPI_Info_create
 *	MPI_Info_free
 *	MPI_Info_set
 *	MPI_Init
 *
 *-------------------------------------------------------------------------*/

int main(int argc, char **argv)
{
    int mpio_result;
    int myid;
    int i;
    int count;
    time_t test_start_time, test_stop_time, elapsed_time;
    time_t total_time = 0;
    char *path;
    int mpi_version;
    int mpi_subversion;

    /* Set up mpi environment */

    mpio_result = MPI_Init(&argc, &argv);
    EXITERR(mpio_result);

    /* Try to keep going after all errors */

    mpio_result = MPI_Errhandler_set(MPI_COMM_WORLD, MPI_ERRORS_RETURN);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_Comm_rank(MPI_COMM_WORLD, &myid);
    CHECKERRS(mpio_result, MPI_SUCCESS);
    wrank = myid;       /* Save a global wrank since myid is passed to routines
                         * and could someday be different from the rank in
                         * COMM_WORLD */

    DEBUG_TRACE("MPI_Init complete");

    mpio_result = MPI_Get_version(&mpi_version, &mpi_subversion);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    if (myid == 0 && verbose) {
        fprintf(stderr, "***Current MPI version is %d.%d\n", mpi_version, mpi_subversion);
    }

    if (argc >= 2)
        count = atol(argv[1]);
    else
        count = 5;

    path = getenv("MPIO_USER_PATH");
    if (path == NULL) {
        strncpy(userpath, default_path, MAX_PATH_LEN - 1);
    }
    else {
        strncpy(userpath, path, MAX_PATH_LEN - 1);
        userpath[MAX_PATH_LEN - 1] = '\0';
    }

    basepid = getpid();

#ifdef VERBOSE
    fprintf(stderr, "(%d)pid = %d, userpath = %s\n", myid, basepid, userpath);
#endif

    mpio_result = MPI_Info_create(&hints_to_test);
    RETURNERRS(mpio_result, MPI_SUCCESS);

    /*
     *  Share node 0's pid among all nodes so they all use the same;
     *  this lets us create per-run filenames so simultaneous tests
     *  don't interfere with each other.
     */

    for (i = 0; i < count; i++) {
        mpio_result = MPI_Bcast(&basepid, 1, MPI_INT, 0, MPI_COMM_WORLD);
        RETURNERRS(mpio_result, MPI_SUCCESS);

        dotests();
        basepid++;
    }

  endtest:

    if (hints_to_test != MPI_INFO_NULL) {
        mpio_result = MPI_Info_free(&hints_to_test);
        CHECKERRS(mpio_result, MPI_SUCCESS);
    }

    mpio_result = MPI_Finalize();
    CHECKERRS(mpio_result, MPI_SUCCESS);

    DEBUG_TRACE("MPI_Finalize complete");

    LEAVING("testmpio main");

    if (myid == 0)
        printf(" No Errors\n");

    return 0;

}       /* main */


/*===========================================================================
 *
 * Function:	dotests
 *
 * Synopsis:
 *	void dotests(
 *		void
 *	)
 *
 * Parameters:
 *	None.
 *
 * Description:
 *	Determines the number of processes and the rank of the current
 *	process, and runs all the MPI-IO tests.
 *
 * Outputs:
 *	None.
 *
 * Interfaces:
 *	MPI_Comm_rank
 *	MPI_Comm_size
 *	test_collective
 *	test_datareps
 *	test_errhandlers
 *	test_filecontrol
 *	test_localpointer
 *	test_manycomms
 *	test_manyopens
 *	test_nb_readwrite
 *	test_nb_rdwr
 *	test_nb_localpointer
 *	test_nb_sharedpointer
 *	test_nb_collective
 *	test_openclose
 *	test_openmodes
 *	test_readwrite
 *	test_rdwr
 *	test_sharedpointer
 *
 *-------------------------------------------------------------------------*/

void dotests(void)
{
    int numprocs, myid;
    int mpio_result;

    mpio_result = MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
    RETURNERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_Comm_rank(MPI_COMM_WORLD, &myid);
    RETURNERRS(mpio_result, MPI_SUCCESS);

    ENTERING("dotests");

    test_manycomms(numprocs, myid);
    test_manyopens(numprocs, myid);
    test_openclose(numprocs, myid);
    test_openmodes(numprocs, myid);
    test_filecontrol(numprocs, myid);
    test_readwrite(numprocs, myid);
    test_rdwr(numprocs, myid);
    test_localpointer(numprocs, myid);
    test_sharedpointer(numprocs, myid);
    test_collective(numprocs, myid);
    test_nb_readwrite(numprocs, myid);
    test_nb_rdwr(numprocs, myid);
    test_nb_localpointer(numprocs, myid);
    test_nb_sharedpointer(numprocs, myid);
    test_nb_collective(numprocs, myid);
    test_errhandlers(numprocs, myid);
#ifdef HAVE_MPI_REGISTER_DATAREP
    test_datareps(numprocs, myid);
#endif

  endtest:

    LEAVING("dotests");

}       /* dotests */


/*===========================================================================
 *
 * Function:	test_openclose
 *
 * Synopsis:
 *	void test_openclose(
 *		int	numprocs,	** IN
 *		int	myid		** IN
 *	)
 *
 * Parameters:
 *	numprocs	how many processors are running test
 *	myid		node number of this processor
 *
 * Description:
 *	Opens and closes an MPI file collectively, without accessing
 *	its contents.
 *
 * Outputs:
 *	Logging and error messages are printed.
 *
 * Interfaces:
 *	MPI_File_close
 *	MPI_File_open
 *
 *-------------------------------------------------------------------------*/

void test_openclose(int numprocs, int myid)
{
    MPI_File mpio_fh;
    int mpio_result;
    char name[MAX_PATH_LEN];
    int amode;

    ENTERING("test_openclose");

    DEBUG_TRACE("Checking open and close file handles");

    MAKEFILENAME(name, sizeof(name), "mpio_openclose", userpath, basepid);

    amode = MPI_MODE_CREATE | MPI_MODE_RDWR | MPI_MODE_DELETE_ON_CLOSE;

    mpio_result = MPI_File_open(MPI_COMM_WORLD, name, amode, hints_to_test, &mpio_fh);
    RETURNERRS(mpio_result, MPI_SUCCESS);

    CHECK(mpio_fh != MPI_FILE_NULL);

    mpio_result = MPI_File_close(&mpio_fh);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    CHECK(mpio_fh == MPI_FILE_NULL);

    /* Try to close a file that isn't open; should fail but not crash */

    DEBUG_TRACE("Checking close of file that isn't open");

    mpio_result = MPI_File_close(&mpio_fh);
    CHECKERRS2(mpio_result, MPI_ERR_FILE, MPI_ERR_ARG);

    CHECK(mpio_fh == MPI_FILE_NULL);

#ifndef NO_NON_LOCAL_ERRTESTS
    if (numprocs > 1) {

        DEBUG_TRACE("Checking file open argument consistency");

        /*
         *  Check error if amodes not the same
         */

        amode = (myid % 2) ? MPI_MODE_CREATE : MPI_MODE_DELETE_ON_CLOSE;
        amode |= MPI_MODE_RDWR;

        mpio_result = MPI_File_open(MPI_COMM_WORLD, name, amode, hints_to_test, &mpio_fh);
        RETURNERRS(mpio_result, MPI_ERR_NOT_SAME);

        CHECK(mpio_fh == MPI_FILE_NULL);

        mpio_result = MPI_File_close(&mpio_fh);
        CHECKERRS2(mpio_result, MPI_ERR_FILE, MPI_ERR_ARG);

        CHECK(mpio_fh == MPI_FILE_NULL);

    }
#endif

  endtest:

    LEAVING("test_openclose");

}       /* test_openclose */


/*===========================================================================
 *
 * Function:	test_readwrite
 *
 * Synopsis:
 *	void test_readwrite(
 *		int	numprocs,	** IN
 *		int	myid		** IN
 *	)
 *
 * Parameters:
 *	numprocs	how many processors are running test
 *	myid		node number of this processor
 *
 * Description:
 *	Tests independent I/O calls, using MPI_BYTE buffer and file types.
 *	Each node writes some bytes to a separate part of the same file
 *	and then reads them back and checks to see that they match.
 *
 * Outputs:
 *	Logging and error messages are printed.
 *
 * Interfaces:
 *	MPI_File_close
 *	MPI_File_open
 *	MPI_File_read_at
 *	MPI_File_sync
 *	MPI_File_write_at
 *	MPI_Get_count
 *
 *-------------------------------------------------------------------------*/

void test_readwrite(int numprocs, int myid)
{
    MPI_File mpio_fh;
    MPI_Status mpi_status;
    int mpio_result;
    int count;
    char outbuf[BUFLEN];
    char inbuf[BUFLEN];
    MPI_Offset myoffset = myid * BUFLEN;
    char name[MAX_PATH_LEN];
    int amode;

    ENTERING("test_readwrite");

    MAKEFILENAME(name, sizeof(name), "mpio_readwrite", userpath, basepid);

    amode = MPI_MODE_CREATE | MPI_MODE_RDWR | MPI_MODE_DELETE_ON_CLOSE;

    mpio_result = MPI_File_open(MPI_COMM_WORLD, name, amode, hints_to_test, &mpio_fh);
    RETURNERRS(mpio_result, MPI_SUCCESS);

    DEBUG_TRACE("Checking independent I/O, default filetypes");

    /* Buffer contains A's for node 0, B's for node 1, etc. */

    memset(outbuf, 'A' + myid, BUFLEN);

    /* Test writing 0 bytes */

    mpio_result = MPI_File_write_at(mpio_fh, myoffset, outbuf, 0, MPI_BYTE, &mpi_status);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    if (mpio_result == MPI_SUCCESS) {
        /* Wrote correct number of bytes? */
        mpio_result = MPI_Get_count(&mpi_status, MPI_BYTE, &count);
        CHECKERRS(mpio_result, MPI_SUCCESS);
        CHECKINTS(count, 0);
    }

    mpio_result = MPI_File_write_at(mpio_fh, myoffset, outbuf, BUFLEN, MPI_BYTE, &mpi_status);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    if (mpio_result == MPI_SUCCESS) {
        /* Wrote correct number of bytes? */
        mpio_result = MPI_Get_count(&mpi_status, MPI_BYTE, &count);
        CHECKERRS(mpio_result, MPI_SUCCESS);
        CHECKINTS(count, BUFLEN);

        /* Clear buffer before reading back into it */
        memset(inbuf, 0, BUFLEN);

        /* Test reading 0 bytes */
        mpio_result = MPI_File_read_at(mpio_fh, myoffset, inbuf, 0, MPI_BYTE, &mpi_status);
        CHECKERRS(mpio_result, MPI_SUCCESS);

        if (mpio_result == MPI_SUCCESS) {
            /* Read correct number of bytes? */
            mpio_result = MPI_Get_count(&mpi_status, MPI_BYTE, &count);
            CHECKERRS(mpio_result, MPI_SUCCESS);
            CHECKINTS(count, 0);
        }

        mpio_result = MPI_File_sync(mpio_fh);
        CHECKERRS(mpio_result, MPI_SUCCESS);

        mpio_result = MPI_File_read_at(mpio_fh,
                                       myoffset, inbuf, BUFLEN, MPI_BYTE, &mpi_status);
        CHECKERRS(mpio_result, MPI_SUCCESS);

        if (mpio_result == MPI_SUCCESS) {
            /* Read correct number of bytes? */
            mpio_result = MPI_Get_count(&mpi_status, MPI_BYTE, &count);
            CHECKERRS(mpio_result, MPI_SUCCESS);
            CHECKINTS(count, BUFLEN);
            CHECKSTRINGS(outbuf, inbuf, BUFLEN);
        }
    }

    mpio_result = MPI_File_close(&mpio_fh);
    CHECKERRS(mpio_result, MPI_SUCCESS);

  endtest:

    LEAVING("test_readwrite");

}       /* test_readwrite */


/*===========================================================================
 *
 * Function:	test_openmodes
 *
 * Synopsis:
 *	void test_openmodes(
 *		int	numprocs,	** IN
 *		int	myid		** IN
 *	)
 *
 * Parameters:
 *	numprocs	how many processors are running test
 *	myid		node number of this processor
 *
 * Description:
 *	Open a file using each of the modes and see if the file modes
 *	work as they should.
 *
 * Outputs:
 *	Logging and error messages are printed.
 *
 * Interfaces:
 *	MPI_File_close
 *	MPI_File_open
 *	MPI_File_read_at
 *	MPI_File_write_at
 *	MPI_Get_count
 *
 *-------------------------------------------------------------------------*/

void test_openmodes(int numprocs, int myid)
{
    MPI_File mpio_fh;
    MPI_Status mpi_status;
    int mpio_result;
    int count;
    char outbuf[BUFLEN];
    char inbuf[BUFLEN];
    MPI_Offset myoffset = myid * BUFLEN;
    char name[MAX_PATH_LEN];
    int amode;

    ENTERING("test_openmodes");

    /* Test WRITE ONLY and CREATE */

    MAKEFILENAME(name, sizeof(name), "mpio_openmodes", userpath, basepid);

    DEBUG_TRACE("Checking create and wronly modes");

    amode = MPI_MODE_CREATE | MPI_MODE_WRONLY;

    mpio_result = MPI_File_open(MPI_COMM_WORLD, name, amode, hints_to_test, &mpio_fh);
    RETURNERRS(mpio_result, MPI_SUCCESS);

    /* Try to write it */

    memset(outbuf, 'A' + myid, BUFLEN);

    mpio_result = MPI_File_write_at(mpio_fh, myoffset, outbuf, BUFLEN, MPI_BYTE, &mpi_status);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    if (mpio_result == MPI_SUCCESS) {
        mpio_result = MPI_Get_count(&mpi_status, MPI_BYTE, &count);
        CHECKERRS(mpio_result, MPI_SUCCESS);
        CHECKINTS(count, BUFLEN);
    }

    /* Try to read it (this should fail) */

    mpio_result = MPI_File_read_at(mpio_fh, myoffset, inbuf, BUFLEN, MPI_BYTE, &mpi_status);
    /* WDG - FIXME: MPI_ERR_UNSUPPORTED_OPERATION is another
     * valid class for this error */
    CHECKERRS2(mpio_result, MPI_ERR_ACCESS, MPI_ERR_UNSUPPORTED_OPERATION);

#ifdef STATUS_VALID_AFTER_ERROR
    mpio_result = MPI_Get_count(&mpi_status, MPI_BYTE, &count);
    CHECKERRS(mpio_result, MPI_SUCCESS);
    CHECKINTS(count, 0);
#endif

    mpio_result = MPI_File_close(&mpio_fh);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    /* Test RDONLY and DELETE_ON_CLOSE */

    DEBUG_TRACE("Checking rdonly and delete_on_close modes ");

    amode = MPI_MODE_RDONLY | MPI_MODE_DELETE_ON_CLOSE;

    mpio_result = MPI_File_open(MPI_COMM_WORLD, name, amode, hints_to_test, &mpio_fh);
    RETURNERRS(mpio_result, MPI_SUCCESS);

    /* Try to read it */

    /* Clear buffer before reading into it */

    memset(inbuf, 0, BUFLEN);

    mpio_result = MPI_File_read_at(mpio_fh, myoffset, inbuf, BUFLEN, MPI_BYTE, &mpi_status);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    if (mpio_result == MPI_SUCCESS) {
        mpio_result = MPI_Get_count(&mpi_status, MPI_BYTE, &count);
        CHECKERRS(mpio_result, MPI_SUCCESS);
        CHECKINTS(count, BUFLEN);
        CHECKSTRINGS(outbuf, inbuf, BUFLEN);
    }

    /* Try to write it (should fail) */

    mpio_result = MPI_File_write_at(mpio_fh, myoffset, outbuf, BUFLEN, MPI_BYTE, &mpi_status);
    /* WDG - FIXME: MPI_ERR_READ_ONLY also valid, and in fact,
     * should be prefered */
    /* CHECKERRS(mpio_result, MPI_ERR_ACCESS); */
    CHECKERRS2(mpio_result, MPI_ERR_READ_ONLY, MPI_ERR_ACCESS);

#ifdef STATUS_VALID_AFTER_ERROR
    mpio_result = MPI_Get_count(&mpi_status, MPI_BYTE, &count);
    CHECKERRS(mpio_result, MPI_SUCCESS);
    CHECKINTS(count, 0);
#endif

    mpio_result = MPI_File_close(&mpio_fh);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    /*
     *  Test open without CREATE to see if DELETE_ON_CLOSE worked.
     *  This should fail if file was deleted correctly.
     */

    amode = MPI_MODE_RDONLY;

    mpio_result = MPI_File_open(MPI_COMM_WORLD, name, amode, hints_to_test, &mpio_fh);

    CHECKERRS(mpio_result, MPI_ERR_NO_SUCH_FILE);

    /* If we (erroneously) succeeded in opening the file, close it */

    if (mpio_result == MPI_SUCCESS) {
        mpio_result = MPI_File_close(&mpio_fh);
        CHECKERRS(mpio_result, MPI_SUCCESS);
    }

  endtest:

    LEAVING("test_openmodes");

}       /* test_openmodes */



/*===========================================================================
 *
 * Function:	test_manyopens
 *
 * Synopsis:
 *	void test_manyopens(
 *		int	numprocs,	** IN
 *		int	myid		** IN
 *	)
 *
 * Parameters:
 *	numprocs	how many processors are running test
 *	myid		node number of this processor
 *
 * Description:
 *	Create several communicators and open the same file several
 *	times with each of them.  This should tell us if MPI-IO
 *	can correctly distinguish simultaneous similar open requests.
 *
 * Outputs:
 *	Logging and error messages are printed.
 *
 * Interfaces:
 *	MPI_Barrier
 *	MPI_File_close
 *	MPI_File_delete
 *	MPI_File_open
 *	MPI_File_read_at
 *	MPI_File_write_at
 *	MPI_Get_count
 *
 * Notes:
 *	FILECOUNT constant (number of open files per node)
 *	must be at least 2.
 *
 *-------------------------------------------------------------------------*/

#define FILECOUNT 16

void test_manyopens(int numprocs, int myid)
{
    int i;
    MPI_File mpio_fh[FILECOUNT];
    MPI_Status mpi_status;
    int mpio_result;
    char outbuf[FILECOUNT][BUFLEN];
    char inbuf[FILECOUNT][BUFLEN];
    char name[MAX_PATH_LEN];
    MPI_Offset myoffset = myid * BUFLEN;
    int count;
    int amode;

    ENTERING("test_manyopens");

    /*
     *  Open a bunch of files.  Originally we used an order that varied
     *  by node number, but chose array indices so that mpio_fh[i]
     *  pointed to the same file on all nodes for a given i.  However,
     *  that relied on asynchronous open calls, which we have now
     *  removed, so we need to open the files in the same order on
     *  each node.
     */

    amode = MPI_MODE_CREATE | MPI_MODE_RDWR | MPI_MODE_DELETE_ON_CLOSE;

    DEBUG_TRACE("Checking multiple opens of different files");

    for (i = 0; i < FILECOUNT; i++) {
        MAKEFILENAME_I(name, sizeof(name), "mpio_manyopens", userpath, i, basepid);

        mpio_result = MPI_File_open(MPI_COMM_WORLD, name, amode, hints_to_test, &mpio_fh[i]);
        CHECKERRS(mpio_result, MPI_SUCCESS);
    }


    DEBUG_TRACE("Checking read/write to multiple files, different orders");

    /* Write to the separate files */

    for (i = 0; i < FILECOUNT; i++) {

        /* Only try more operations on the file if open succeeded */

        if (mpio_fh[i] == MPI_FILE_NULL)
            continue;

        /* Try to write different things to different files */

        memset(outbuf[i], ('A' + myid + (i << 5)) % 128, BUFLEN);

        mpio_result = MPI_File_write_at(mpio_fh[i],
                                        myoffset, outbuf[i], BUFLEN, MPI_BYTE, &mpi_status);
        CHECKERRS(mpio_result, MPI_SUCCESS);

        if (mpio_result == MPI_SUCCESS) {
            mpio_result = MPI_Get_count(&mpi_status, MPI_BYTE, &count);
            CHECKERRS(mpio_result, MPI_SUCCESS);
            CHECKINTS(count, BUFLEN);
        }
    }

    /* Read back from files in a different order */

    for (i = 1; i <= FILECOUNT; i++) {
        int offset_i = i % FILECOUNT;

        if (mpio_fh[offset_i] == MPI_FILE_NULL)
            continue;

        memset(inbuf[offset_i], 0, BUFLEN);

        mpio_result = MPI_File_read_at(mpio_fh[offset_i],
                                       myoffset, inbuf[offset_i],
                                       BUFLEN, MPI_BYTE, &mpi_status);
        CHECKERRS(mpio_result, MPI_SUCCESS);

        if (mpio_result == MPI_SUCCESS) {
            mpio_result = MPI_Get_count(&mpi_status, MPI_BYTE, &count);
            CHECKERRS(mpio_result, MPI_SUCCESS);
            CHECKINTS(count, BUFLEN);

            if (count == BUFLEN) {
                CHECKSTRINGS(outbuf[offset_i], inbuf[offset_i], BUFLEN);
            }
        }
    }

    /* Close/delete the files only after all the writes/reads are done */

    for (i = 0; i < FILECOUNT; i++) {
        if (mpio_fh[i] == MPI_FILE_NULL)
            continue;

        mpio_result = MPI_File_close(&mpio_fh[i]);
        CHECKERRS(mpio_result, MPI_SUCCESS);
    }

    /*
     *  This time, open the same file several times with different
     *  access modes.  This should work fine and create separate
     *  file table entries for each open.
     */

    DEBUG_TRACE("Checking multiple opens of same file, different modes");

    /* First open WRONLY */

    MAKEFILENAME_S(name, sizeof(name), "mpio_manyopens", userpath, "modes", basepid);

    amode = MPI_MODE_CREATE | MPI_MODE_WRONLY;

    mpio_result = MPI_File_open(MPI_COMM_WORLD, name, amode, hints_to_test, &mpio_fh[0]);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    /* Now open RDONLY */

    amode = MPI_MODE_RDONLY;

    mpio_result = MPI_File_open(MPI_COMM_WORLD, name, amode, hints_to_test, &mpio_fh[1]);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    if (mpio_fh[0] != MPI_FILE_NULL && mpio_fh[1] != MPI_FILE_NULL) {

        /* Should have created different file table entries */

        CHECK(mpio_fh[0] != mpio_fh[1]);

        /* Write to one file handle... */

        memset(outbuf[0], 'A' + myid, BUFLEN);

        mpio_result = MPI_File_write_at(mpio_fh[0],
                                        myoffset, outbuf[0], BUFLEN, MPI_BYTE, &mpi_status);
        CHECKERRS(mpio_result, MPI_SUCCESS);

        if (mpio_result == MPI_SUCCESS) {
            mpio_result = MPI_Get_count(&mpi_status, MPI_BYTE, &count);
            CHECKERRS(mpio_result, MPI_SUCCESS);
            CHECKINTS(count, BUFLEN);
        }

        /*
         *  Since we have (potentially) multiple threads (on HPSS
         *  server side) doing I/O for one file with two handles,
         *  it's possible that the write thread doesn't complete
         *  before the read thread from the hpss_Read below.  To
         *  prevent this race, force the file handle open for write
         *  to be closed before trying the read.
         */

        mpio_result = MPI_File_close(&mpio_fh[0]);
        CHECKERRS(mpio_result, MPI_SUCCESS);

        /* Read from the other */

        memset(inbuf[0], 0, BUFLEN);
        mpio_result = MPI_File_read_at(mpio_fh[1],
                                       myoffset, inbuf[0], BUFLEN, MPI_BYTE, &mpi_status);
        CHECKERRS(mpio_result, MPI_SUCCESS);

        if (mpio_result == MPI_SUCCESS) {
            mpio_result = MPI_Get_count(&mpi_status, MPI_BYTE, &count);
            CHECKERRS(mpio_result, MPI_SUCCESS);
            CHECKINTS(count, BUFLEN);
            if (count == BUFLEN)
                CHECKSTRINGS(outbuf[0], inbuf[0], BUFLEN);
        }

        mpio_result = MPI_File_close(&mpio_fh[1]);
        CHECKERRS(mpio_result, MPI_SUCCESS);

        /* Wait for all nodes to close the file */

        mpio_result = MPI_Barrier(MPI_COMM_WORLD);
        CHECKERRS(mpio_result, MPI_SUCCESS);

        /* Delete the file when done, but only from one node.  */

        if (myid == 0) {
            mpio_result = MPI_File_delete(name, MPI_INFO_NULL);
            CHECKERRS(mpio_result, MPI_SUCCESS);
        }
    }
    else {      /* Just close the files, ignore errors */
        if (mpio_fh[0] != MPI_FILE_NULL) {
            mpio_result = MPI_File_close(&mpio_fh[0]);
            CHECKERRS(mpio_result, MPI_SUCCESS);
        }
        if (mpio_fh[1] != MPI_FILE_NULL) {
            mpio_result = MPI_File_close(&mpio_fh[1]);
            CHECKERRS(mpio_result, MPI_SUCCESS);
        }
        if (myid == 0) {
            mpio_result = MPI_File_delete(name, MPI_INFO_NULL);
            CHECKERRS(mpio_result, MPI_SUCCESS);
        }
    }
/* endtest is used by the RETURN macro.  This test never bails early,
   so the endtest label is commented out to eliminate a compiler warning */
/* endtest: */

    LEAVING("test_manyopens");

}       /* test_manyopens */


/*===========================================================================
 *
 * Function:	test_manycomms
 *
 * Synopsis:
 *	void test_manycomms(
 *		int	numprocs,	** IN
 *		int	myid		** IN
 *	)
 *
 * Parameters:
 *	numprocs	how many processors are running test
 *	myid		node number of this processor
 *
 * Description:
 *	Open the same file with a bunch of different communicators.
 *	The communicators will overlap in the following pattern:
 *	   node 01234567  set
 * 		AAAAAAAA  0
 *		BBBBCCCC  1
 *		DEEEEEEF  2
 *	We'll try to make this pattern work for any number of nodes,
 *	but the test makes most sense with at least 4.  D and F will
 *	always be singletons at the end.
 *
 * Outputs:
 *	Logging and error messages are printed.
 *
 * Interfaces:
 *	MPI_Barrier
 *	MPI_Comm_free
 *	MPI_Comm_split
 *	MPI_File_close
 *	MPI_File_delete
 *	MPI_File_open
 *	MPI_File_read_at
 *	MPI_File_set_atomicity
 *	MPI_File_sync
 *	MPI_File_write_at
 *	MPI_Get_count
 *
 *-------------------------------------------------------------------------*/

void test_manycomms(int numprocs, int myid)
{
    int i;
    int offset_i;
    MPI_Offset cur_offset;
    MPI_File mpio_fh[3];
    MPI_Status mpi_status;
    int mpio_result;
    char outbuf[3][BUFLEN];
    char inbuf[3][BUFLEN];
    MPI_Comm comm[3];
    int color;
    int midpoint = numprocs / 2;
    MPI_Offset myoffset = myid * BUFLEN;
    char name[MAX_PATH_LEN];
    int count;
    int amode;

    comm[0] = MPI_COMM_WORLD;
    comm[1] = MPI_COMM_NULL;
    comm[2] = MPI_COMM_NULL;

    ENTERING("test_manycomms");

    /*  Split world into 2 communicators. */

    /*
     *  Create a pair of communicators with lower numbered
     *  nodes in one and the higher numbered nodes in the other.
     */

    mpio_result = MPI_Comm_split(comm[0], (myid < midpoint), 0, &comm[1]);
    RETURNERRS(mpio_result, MPI_SUCCESS);

    /*
     *  Now create a set of communicators that include the middle
     *  nodes or one of the end nodes.
     */

    if (myid == 0)
        color = 0;
    else if (myid == numprocs - 1)
        color = 1;
    else
        color = 2;

    mpio_result = MPI_Comm_split(comm[0], color, 0, &comm[2]);
    RETURNERRS(mpio_result, MPI_SUCCESS);

    /*
     *  We've got all our communicators (a total of 6 among all the
     *  nodes); now open some files and do some writing and reading.
     */

    DEBUG_TRACE("Checking multiple opens with different communicators");

    amode = MPI_MODE_CREATE | MPI_MODE_RDWR;

    for (i = 0; i < 3; i++) {
        MAKEFILENAME_I(name, sizeof(name), "mpio_manycomms", userpath, i, basepid);

        mpio_result = MPI_File_open(comm[i], name, amode, hints_to_test, &mpio_fh[i]);
        CHECKERRS(mpio_result, MPI_SUCCESS);

        /* Only try more operations on the file if open succeeded */

        if (mpio_fh[i] == MPI_FILE_NULL)
            continue;

        mpio_result = MPI_File_set_atomicity(mpio_fh[i], TRUE);
        CHECKERRS(mpio_result, MPI_SUCCESS);
    }

    /* Write to the file */

    for (i = 0; i < 3; i++) {
        /*
         *  Writing to the same file in different ways, so avoid
         *  stepping on previous data.
         */

        MPI_Offset cur_offset = myoffset + (i * numprocs * BUFLEN);

        /* Only try more operations on the file if open succeeded */

        if (mpio_fh[i] == MPI_FILE_NULL)
            continue;

        /* Try to write different things to different files */

        memset(outbuf[i], ('A' + myid + (i << 5)) % 128, BUFLEN);

        mpio_result = MPI_File_write_at(mpio_fh[i],
                                        cur_offset, outbuf[i], BUFLEN, MPI_BYTE, &mpi_status);
        CHECKERRS(mpio_result, MPI_SUCCESS);

        if (mpio_result == MPI_SUCCESS) {
            mpio_result = MPI_Get_count(&mpi_status, MPI_BYTE, &count);
            CHECKERRS(mpio_result, MPI_SUCCESS);
            CHECKINTS(count, BUFLEN);
        }
    }

    /* Read back from file in a different order */

    for (i = 1; i <= 3; i++) {
        offset_i = i % 3;
        cur_offset = myoffset + offset_i * numprocs * BUFLEN;

        if (mpio_fh[offset_i] == MPI_FILE_NULL)
            continue;

        memset(inbuf[offset_i], 0, BUFLEN);

        mpio_result = MPI_File_read_at(mpio_fh[offset_i],
                                       cur_offset, inbuf[offset_i],
                                       BUFLEN, MPI_BYTE, &mpi_status);
        CHECKERRS(mpio_result, MPI_SUCCESS);

        if (mpio_result == MPI_SUCCESS) {
            mpio_result = MPI_Get_count(&mpi_status, MPI_BYTE, &count);
            CHECKERRS(mpio_result, MPI_SUCCESS);
            CHECKINTS(count, BUFLEN);
            CHECKSTRINGS(outbuf[offset_i], inbuf[offset_i], BUFLEN);
        }

        mpio_result = MPI_File_close(&mpio_fh[offset_i]);
        CHECKERRS(mpio_result, MPI_SUCCESS);
    }

    /*
     *  Wait 'till everyone's done; since the nodes opened the
     *  files with different communicators, they won't synchronize
     *  when the file is closed.
     */

    mpio_result = MPI_Barrier(MPI_COMM_WORLD);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    for (i = 0; i < 3; i++) {
        MAKEFILENAME_I(name, sizeof(name), "mpio_manycomms", userpath, i, basepid);

        if (myid == 0) {
            mpio_result = MPI_File_delete(name, MPI_INFO_NULL);
            CHECKERRS(mpio_result, MPI_SUCCESS);
        }
    }

  endtest:

    if (comm[1] != MPI_COMM_NULL) {
        mpio_result = MPI_Comm_free(&comm[1]);
        CHECKERRS(mpio_result, MPI_SUCCESS);
    }
    if (comm[2] != MPI_COMM_NULL) {
        mpio_result = MPI_Comm_free(&comm[2]);
        CHECKERRS(mpio_result, MPI_SUCCESS);
    }

    LEAVING("test_manycomms");

}       /* test_manycomms */


/*===========================================================================
 *
 * Function:	test_rdwr
 *
 * Synopsis:
 *	void test_rdwr(
 *		int	numprocs,	** IN
 *		int	myid		** IN
 *	)
 *
 * Parameters:
 *	numprocs	how many processors are running test
 *	myid		node number of this processor
 *
 * Description:
 *	Tests collective I/O using derived file type and buffer type.
 *
 * Outputs:
 *	Logging and error messages are printed.
 *
 * Interfaces:
 *	MPI_File_close
 *	MPI_File_open
 *	MPI_File_read_at_all
 *	MPI_File_set_view
 *	MPI_File_write_at_all
 *	MPI_Get_count
 *	MPI_Type_commit
 *	MPI_Type_extent
 *	MPI_Type_free
 *	MPI_Type_struct
 *
 *-------------------------------------------------------------------------*/

void test_rdwr(int numprocs, int myid)
{
    MPI_File mpio_fh;
    MPI_Datatype file_type = MPI_DATATYPE_NULL;
    MPI_Datatype buf_type = MPI_DATATYPE_NULL;
    MPI_Status mpi_status;
    char *buf = NULL;
    char *rbuf = NULL;
    int bufcount;
    MPI_Offset offset;
    int i;
    char *p;
    int buf_blocklen;
    int mpio_result;
    int blens[3];
    MPI_Aint disps[3];
    MPI_Datatype types[3];
    MPI_Aint buf_extent;
    char name[MAX_PATH_LEN];
    int count;
    int size;
    int amode;

    ENTERING("test_rdwr");

    /* Set up buftype */

    buf_blocklen = 2048;
    blens[0] = 1;
    blens[1] = buf_blocklen;
    blens[2] = 1;
    disps[0] = 0;
    disps[1] = blens[1] * myid;
    disps[2] = blens[1] * numprocs;
    types[0] = MPI_LB;
    types[1] = MPI_CHAR;
    types[2] = MPI_UB;

    mpio_result = MPI_Type_struct(3, blens, disps, types, &buf_type);
    RETURNERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_Type_commit(&buf_type);
    RETURNERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_Type_extent(buf_type, &buf_extent);
    RETURNERRS(mpio_result, MPI_SUCCESS);

    /* Set up filetype */

    blens[0] = 1;
    blens[1] = 4096;
    blens[2] = 1;
    disps[0] = 0;
    disps[1] = blens[1] * myid;
    disps[2] = blens[1] * numprocs;
    types[0] = MPI_LB;
    types[1] = MPI_CHAR;
    types[2] = MPI_UB;

    mpio_result = MPI_Type_struct(3, blens, disps, types, &file_type);
    RETURNERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_Type_commit(&file_type);
    RETURNERRS(mpio_result, MPI_SUCCESS);

    /* Set up the buffers */

    offset = 0;
    bufcount = 8;
    size = bufcount * buf_extent;

    buf = malloc(size);
    if (buf == NULL) {
        DEBUG_TRACE("Couldn't allocate space for buf");
        RETURN;
    }

    rbuf = malloc(size);
    if (rbuf == NULL) {
        DEBUG_TRACE("Couldn't allocate space for rbuf");
        RETURN;
    }

    memset(buf, 0, size);
    memset(rbuf, 0, size);

    p = buf;
    for (i = 0; i < bufcount; i++) {
        memset((void *) (p + myid * buf_blocklen), '0' + myid, buf_blocklen);
        p += buf_extent;
    }

    MAKEFILENAME(name, sizeof(name), "mpio_rdwr", userpath, basepid);

    DEBUG_TRACE("Checking I/O with derived buftype and filetype");

    amode = MPI_MODE_CREATE | MPI_MODE_RDWR | MPI_MODE_DELETE_ON_CLOSE;

    mpio_result = MPI_File_open(MPI_COMM_WORLD, name, amode, hints_to_test, &mpio_fh);
    RETURNERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_File_set_view(mpio_fh, 0, MPI_CHAR, file_type, "native", MPI_INFO_NULL);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_File_write_at_all(mpio_fh, offset, buf, bufcount, buf_type, &mpi_status);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    if (mpio_result == MPI_SUCCESS) {
        mpio_result = MPI_Get_count(&mpi_status, MPI_BYTE, &count);
        CHECKERRS(mpio_result, MPI_SUCCESS);
        CHECKINTS(count, buf_blocklen * bufcount);

        mpio_result = MPI_File_read_at_all(mpio_fh,
                                           offset, rbuf, bufcount, buf_type, &mpi_status);
        CHECKERRS(mpio_result, MPI_SUCCESS);

        if (mpio_result == MPI_SUCCESS) {
            mpio_result = MPI_Get_count(&mpi_status, MPI_BYTE, &count);
            CHECKERRS(mpio_result, MPI_SUCCESS);
            CHECKINTS(count, buf_blocklen * bufcount);
            CHECKSTRINGS(buf, rbuf, size);
        }
    }

    mpio_result = MPI_File_close(&mpio_fh);
    CHECKERRS(mpio_result, MPI_SUCCESS);

  endtest:

    if (buf != NULL)
        free(buf);
    if (rbuf != NULL)
        free(rbuf);

    if (file_type != MPI_DATATYPE_NULL) {
        mpio_result = MPI_Type_free(&file_type);
        CHECKERRS(mpio_result, MPI_SUCCESS);
    }

    if (buf_type != MPI_DATATYPE_NULL) {
        mpio_result = MPI_Type_free(&buf_type);
        CHECKERRS(mpio_result, MPI_SUCCESS);
    }

    LEAVING("test_rdwr");

}       /* test_rdwr */


/*===========================================================================
 *
 * Function:	test_filecontrol
 *
 * Synopsis:
 *	void test_filecontrol(
 *		int	numprocs,	** IN
 *		int	myid		** IN
 *	)
 *
 * Parameters:
 *	numprocs	how many processors are running test
 *	myid		node number of this processor
 *
 * Description:
 *	Tests the file control commands.
 *
 * Outputs:
 *	Logging and error messages are printed.
 *
 * Interfaces:
 *	MPI_Barrier
 *	MPI_Comm_free
 *	MPI_Comm_group
 *	MPI_Comm_split
 *	MPI_File_close
 *	MPI_File_open
 *	MPI_File_get_amode
 *	MPI_File_get_atomicity
 *	MPI_File_get_byte_offset
 *	MPI_File_get_group
 *	MPI_File_get_info
 *	MPI_File_get_position
 *	MPI_File_get_position_shared
 *	MPI_File_get_size
 *	MPI_File_get_view
 *	MPI_File_preallocate
 *	MPI_File_set_atomicity
 *	MPI_File_set_view
 *	MPI_File_set_size
 *	MPI_File_write
 *	MPI_Group_compare
 *	MPI_Info_free
 *	MPI_Info_get
 *	MPI_Type_commit
 *	MPI_Type_contiguous
 *	MPI_Type_free
 *	MPI_Type_struct
 *
 *-------------------------------------------------------------------------*/

void test_filecontrol(int numprocs, int myid)
{

    MPI_File mpio_fh;
    MPI_Status mpi_status;
    char name[MAX_PATH_LEN];
    char split_name[MAX_PATH_LEN];
    int mpio_result;
    MPI_Datatype byteset = MPI_DATATYPE_NULL;
    MPI_Datatype floatset = MPI_DATATYPE_NULL;
    MPI_Datatype holetype = MPI_DATATYPE_NULL;
    MPI_Datatype typelist[3] = { MPI_LB, MPI_FLOAT, MPI_UB };
    int blocklens[3] = { 1, 1, 1 };
    MPI_Aint indices[3] = { 0, 0, 3 * sizeof(float) };
    char outbuf[BUFLEN];
    float floatbuf[10] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    MPI_Comm split_comm = MPI_COMM_NULL;
    int midpoint = numprocs / 2;
    int base;
    MPI_Group file_group, world_group;
    int amode_in, amode_out;
    MPI_Offset disp;
    MPI_Offset offset, byte_offset;
    MPI_Offset filesize;
    MPI_Datatype etype, filetype;
    int flag;
    MPI_Info hints;
    char filename[MAX_PATH_LEN];
    char datarep[MPI_MAX_DATAREP_STRING];
    int amode;

    ENTERING("test_filecontrol");

    memset(outbuf, 'A' + myid, BUFLEN);

    mpio_result = MPI_Type_contiguous(37, MPI_BYTE, &byteset);
    /* size is prime */
    RETURNERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_Type_commit(&byteset);
    RETURNERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_Type_contiguous(50, MPI_FLOAT, &floatset);
    RETURNERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_Type_commit(&floatset);
    RETURNERRS(mpio_result, MPI_SUCCESS);

    /*
     *  creating a struct with  a float followed by a 2-float hole.
     */

    mpio_result = MPI_Type_struct(3, blocklens, indices, typelist, &holetype);
    RETURNERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_Type_commit(&holetype);
    RETURNERRS(mpio_result, MPI_SUCCESS);

    MAKEFILENAME(name, sizeof(name), "mpio_testcontrol", userpath, basepid);

    amode_in = MPI_MODE_CREATE | MPI_MODE_RDWR | MPI_MODE_DELETE_ON_CLOSE;

    mpio_result = MPI_File_open(MPI_COMM_WORLD, name, amode_in, hints_to_test, &mpio_fh);
    RETURNERRS(mpio_result, MPI_SUCCESS);

    DEBUG_TRACE("Checking MPI_File_get_amode");

    mpio_result = MPI_File_get_amode(mpio_fh, &amode_out);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    if (mpio_result == MPI_SUCCESS) {
        CHECKINTS(amode_out, amode_in);
    }

    DEBUG_TRACE("Checking MPI_File_get_atomicity");

    mpio_result = MPI_File_get_atomicity(mpio_fh, &flag);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    if (mpio_result == MPI_SUCCESS) {
        /* flag == FALSE => nonatomic mode enabled, default */
        CHECKINTS(flag, FALSE);
    }

    DEBUG_TRACE("Checking MPI_File_get_group");

    mpio_result = MPI_File_get_group(mpio_fh, &file_group);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    if (mpio_result == MPI_SUCCESS) {
        mpio_result = MPI_Comm_group(MPI_COMM_WORLD, &world_group);
        CHECKERRS(mpio_result, MPI_SUCCESS);
        mpio_result = MPI_Group_compare(file_group, world_group, &flag);
        CHECKERRS(mpio_result, MPI_SUCCESS);
        CHECKINTS(flag, MPI_IDENT);
        MPI_Group_free(&world_group);
    }
    MPI_Group_free(&file_group);

    /* Test return from MPI_File_get_info:  name, hints */

    DEBUG_TRACE("Checking MPI_File_get_info");

    mpio_result = MPI_File_get_info(mpio_fh, &hints);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    if (mpio_result == MPI_SUCCESS) {
        DEBUG_TRACE("Checking filename in info");

        mpio_result = MPI_Info_get(hints, "filename", sizeof(filename), filename, &flag);

        if (mpio_result == MPI_SUCCESS) {
            if (flag == TRUE) {
                CHECK(strcmp(filename, name) == 0);
            }
            else {
                DEBUG_TRACE("Filename not in info");
            }
        }

        mpio_result = MPI_Info_free(&hints);
        CHECKERRS(mpio_result, MPI_SUCCESS);
    }

    /* Test MPI_File_get_view */

    /*
     *  NOTE:
     *  The MPI2 standard requires that derived datatypes be dup-ed
     *  by MPI_File_get_view, and so must be freed.  The standard is
     *  unclear (to us) about whether basic datatypes are dup-ed,
     *  so we assume they are not.
     */

    DEBUG_TRACE("Checking MPI_File_get_view, defaults");

    mpio_result = MPI_File_get_view(mpio_fh, &disp, &etype, &filetype, datarep);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    if (mpio_result == MPI_SUCCESS) {
        CHECKINTS(disp, 0);
        CHECK(strcmp(datarep, "native") == 0);

        if (etype != MPI_BYTE) {
            mpio_result = MPI_Type_free(&etype);
            CHECKERRS(mpio_result, MPI_SUCCESS);
        }
        if (filetype != MPI_BYTE) {
            mpio_result = MPI_Type_free(&filetype);
            CHECKERRS(mpio_result, MPI_SUCCESS);
        }
    }

    DEBUG_TRACE("Checking MPI_File_set_view, byteset");

    mpio_result = MPI_File_set_view(mpio_fh, 0, MPI_BYTE, byteset, "native", MPI_INFO_NULL);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    if (mpio_result == MPI_SUCCESS) {
        DEBUG_TRACE("Checking MPI_File_get_view, after set");

        mpio_result = MPI_File_get_view(mpio_fh, &disp, &etype, &filetype, datarep);
        CHECKERRS(mpio_result, MPI_SUCCESS);

        if (mpio_result == MPI_SUCCESS) {
            CHECKINTS(disp, 0);
            CHECK(strcmp(datarep, "native") == 0);

            if (etype != MPI_BYTE) {
                mpio_result = MPI_Type_free(&etype);
                CHECKERRS(mpio_result, MPI_SUCCESS);
            }
            mpio_result = MPI_Type_free(&filetype);
            CHECKERRS(mpio_result, MPI_SUCCESS);
        }
    }

    DEBUG_TRACE("Checking MPI_File_get_position");

    mpio_result = MPI_File_get_position(mpio_fh, &offset);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    if (mpio_result == MPI_SUCCESS) {
        CHECKINTS(offset, 0);

        mpio_result = MPI_File_get_byte_offset(mpio_fh, offset, &byte_offset);
        CHECKERRS(mpio_result, MPI_SUCCESS);

        if (mpio_result == MPI_SUCCESS) {
            CHECKINTS(byte_offset, 0);
        }
    }

    /* Do a write to see if pointer gets updated */
    mpio_result = MPI_File_write(mpio_fh, outbuf, BUFLEN, MPI_BYTE, &mpi_status);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    if (mpio_result == MPI_SUCCESS) {
        mpio_result = MPI_File_get_position(mpio_fh, &offset);
        CHECKERRS(mpio_result, MPI_SUCCESS);

        if (mpio_result == MPI_SUCCESS) {
            CHECKINTS(offset, BUFLEN);

            mpio_result = MPI_File_get_byte_offset(mpio_fh, offset, &byte_offset);
            CHECKERRS(mpio_result, MPI_SUCCESS);

            if (mpio_result == MPI_SUCCESS) {
                CHECKINTS(byte_offset, BUFLEN);
            }
        }

        DEBUG_TRACE("Checking MPI_File_get_size, after write");

        mpio_result = MPI_File_get_size(mpio_fh, &filesize);
        CHECKERRS(mpio_result, MPI_SUCCESS);

        if (mpio_result == MPI_SUCCESS) {
            CHECKINTS(filesize, BUFLEN);
        }
    }

    DEBUG_TRACE("Checking MPI_File_get_position_shared");

    mpio_result = MPI_File_get_position_shared(mpio_fh, &offset);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    if (mpio_result == MPI_SUCCESS) {
        CHECKINTS(offset, 0);

        mpio_result = MPI_File_get_byte_offset(mpio_fh, offset, &byte_offset);
        CHECKERRS(mpio_result, MPI_SUCCESS);

        if (mpio_result == MPI_SUCCESS) {
            CHECKINTS(byte_offset, 0);
        }
    }

    DEBUG_TRACE("Checking MPI_File_set_view, disp");

    mpio_result = MPI_File_set_view(mpio_fh, 1024, MPI_BYTE, byteset, "native", MPI_INFO_NULL);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    if (mpio_result == MPI_SUCCESS) {
        /*
         *  We could also test with some complicated series of
         *  write and reads.
         */
        mpio_result = MPI_File_get_view(mpio_fh, &disp, &etype, &filetype, datarep);
        CHECKERRS(mpio_result, MPI_SUCCESS);

        if (mpio_result == MPI_SUCCESS) {
            CHECKINTS(disp, 1024);

            if (etype != MPI_BYTE) {
                mpio_result = MPI_Type_free(&etype);
                CHECKERRS(mpio_result, MPI_SUCCESS);
            }
            mpio_result = MPI_Type_free(&filetype);
            CHECKERRS(mpio_result, MPI_SUCCESS);
        }
    }

    DEBUG_TRACE("Checking MPI_File_set_view, bogus");

    /* First try a bogus combination; should not be allowed! */

    mpio_result = MPI_File_set_view(mpio_fh, 0, MPI_FLOAT, byteset, "native", MPI_INFO_NULL);
    CHECKERRS(mpio_result, MPI_ERR_ARG);

    /* Now try a legitimate pair */

    mpio_result = MPI_File_set_view(mpio_fh, 0, MPI_FLOAT, floatset, "native", MPI_INFO_NULL);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    if (mpio_result == MPI_SUCCESS) {
        mpio_result = MPI_File_get_view(mpio_fh, &disp, &etype, &filetype, datarep);
        CHECKERRS(mpio_result, MPI_SUCCESS);

        if (mpio_result == MPI_SUCCESS) {

            if (etype != MPI_FLOAT) {
                mpio_result = MPI_Type_free(&etype);
                CHECKERRS(mpio_result, MPI_SUCCESS);
            }
            mpio_result = MPI_Type_free(&filetype);
            CHECKERRS(mpio_result, MPI_SUCCESS);
        }
    }

    DEBUG_TRACE("Checking MPI_File_set_atomicity");

    mpio_result = MPI_File_set_atomicity(mpio_fh, TRUE);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    if (mpio_result == MPI_SUCCESS) {
        mpio_result = MPI_File_get_atomicity(mpio_fh, &flag);
        CHECKERRS(mpio_result, MPI_SUCCESS);

        if (mpio_result == MPI_SUCCESS)
            CHECKINTS(flag, TRUE);
    }

    /* Wait until previous tests are completed on all nodes */

    mpio_result = MPI_Barrier(MPI_COMM_WORLD);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    /*
     *  Do a more demanding test by using a filetype with holes.
     */

    DEBUG_TRACE("Checking filetype with holes");

    mpio_result = MPI_File_set_view(mpio_fh, 37, MPI_FLOAT, holetype, "native", MPI_INFO_NULL);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_File_write(mpio_fh, floatbuf,
                                 sizeof(floatbuf) / sizeof(float), MPI_FLOAT, &mpi_status);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    if (mpio_result == MPI_SUCCESS) {
        mpio_result = MPI_File_get_position(mpio_fh, &offset);
        CHECKERRS(mpio_result, MPI_SUCCESS);

        mpio_result = MPI_File_get_byte_offset(mpio_fh, offset, &byte_offset);
        CHECKERRS(mpio_result, MPI_SUCCESS);

        CHECKINTS(byte_offset, 37 + 3 * sizeof(floatbuf));
    }

    DEBUG_TRACE("Checking MPI_File_set_size and MPI_File_preallocate");

    mpio_result = MPI_Barrier(MPI_COMM_WORLD);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_File_set_size(mpio_fh, 0);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_File_get_size(mpio_fh, &filesize);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    if (mpio_result == MPI_SUCCESS) {
        CHECKINTS(filesize, 0);
    }

    mpio_result = MPI_Barrier(MPI_COMM_WORLD);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_File_preallocate(mpio_fh, 1000);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_File_get_size(mpio_fh, &filesize);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    if (mpio_result == MPI_SUCCESS) {
        CHECKINTS(filesize, 1000);
    }

    /* Test that preallocate of smaller size leaves as is */

    mpio_result = MPI_File_preallocate(mpio_fh, 100);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_File_get_size(mpio_fh, &filesize);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    if (mpio_result == MPI_SUCCESS) {
        CHECKINTS(filesize, 1000);
    }

    /* Done with the first test file. */

    mpio_result = MPI_File_close(&mpio_fh);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    /*
     *  Check files opened with communicators that are a subset of
     *  all the nodes see that operations synchronize correctly.
     */

    /* Split world into 2 communicators */

    /*
     *  First define which nodes go in the new communicator;
     *  the lower half and upper half of the nodes will each
     *  form their own communicator here.
     */

    DEBUG_TRACE("Checking subset communicators");

    base = (myid < midpoint) ? 0 : midpoint;

    if (midpoint == 0)
        midpoint = 1;   /* avoid errors when n == 1 */

    mpio_result = MPI_Comm_split(MPI_COMM_WORLD, (myid < midpoint), 0, &split_comm);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    /* Different names for the two files */

    MAKEFILENAME_I(split_name, sizeof(split_name),
                   "mpio_split_control", userpath, basepid, base);

    amode = MPI_MODE_CREATE | MPI_MODE_RDWR | MPI_MODE_DELETE_ON_CLOSE;

    mpio_result = MPI_File_open(split_comm, split_name, amode, hints_to_test, &mpio_fh);
    RETURNERRS(mpio_result, MPI_SUCCESS);

    if (mpio_result == MPI_SUCCESS) {
        mpio_result = MPI_File_get_size(mpio_fh, &filesize);
        CHECKERRS(mpio_result, MPI_SUCCESS);

        if (mpio_result == MPI_SUCCESS) {
            CHECKINTS(filesize, 0);
        }

        mpio_result = MPI_File_close(&mpio_fh);
        CHECKERRS(mpio_result, MPI_SUCCESS);
    }

    /* Finally, try an operation on a closed file */

    DEBUG_TRACE("Checking operation on closed file");

    mpio_result = MPI_File_get_amode(mpio_fh, &amode);
    CHECKERRS2(mpio_result, MPI_ERR_FILE, MPI_ERR_ARG);

  endtest:

    if (split_comm != MPI_COMM_NULL) {
        mpio_result = MPI_Comm_free(&split_comm);
        CHECKERRS(mpio_result, MPI_SUCCESS);
    }
    if (floatset != MPI_DATATYPE_NULL) {
        mpio_result = MPI_Type_free(&floatset);
        CHECKERRS(mpio_result, MPI_SUCCESS);
    }
    if (byteset != MPI_DATATYPE_NULL) {
        mpio_result = MPI_Type_free(&byteset);
        CHECKERRS(mpio_result, MPI_SUCCESS);
    }
    if (holetype != MPI_DATATYPE_NULL) {
        mpio_result = MPI_Type_free(&holetype);
        CHECKERRS(mpio_result, MPI_SUCCESS);
    }

    LEAVING("test_filecontrol");

}       /* test_filecontrol */


/*===========================================================================
 *
 * Function:	test_localpointer
 *
 * Synopsis:
 *	void test_localpointer(
 *		int	numprocs,	** IN
 *		int	myid		** IN
 *	)
 *
 * Parameters:
 *	numprocs	how many processors are running test
 *	myid		node number of this processor
 *
 * Description:
 *	Tests the local file pointer commands, including seeks.
 *
 * Outputs:
 *	Logging and error messages are printed.
 *
 * Interfaces:
 *	MPI_File_close
 *	MPI_File_get_position
 *	MPI_File_open
 *	MPI_File_read
 *	MPI_File_seek
 *	MPI_File_set_view
 *	MPI_File_sync
 *	MPI_File_write
 *	MPI_Get_count
 *	MPI_Type_commit
 *	MPI_Type_contiguous
 *	MPI_Type_create_darray
 *	MPI_Type_free
 *
 *-------------------------------------------------------------------------*/

void test_localpointer(int numprocs, int myid)
{
    MPI_File mpio_fh;
    MPI_Status mpi_status;
    int count;
    char name1[MAX_PATH_LEN];
    char name2[MAX_PATH_LEN];
    int mpio_result;
    int i;
    MPI_Offset disp = BUFLEN * myid * 2;
    MPI_Datatype scatter = MPI_DATATYPE_NULL;
    MPI_Datatype floatarray = MPI_DATATYPE_NULL;
    char outbuf[BUFLEN];
    char inbuf[BUFLEN];
    char nullbuf[BUFLEN];
    float floatvec[100];
    float invec[100];
    MPI_Offset posn;
    int amode;
    int distrib;
    int darg;

    ENTERING("test_localpointer");

    memset(outbuf, 'A' + myid, BUFLEN);
    memset(nullbuf, 0, BUFLEN);

    MAKEFILENAME(name1, sizeof(name1), "mpio_testlocalpointer", userpath, basepid);
    MAKEFILENAME(name2, sizeof(name2), "mpio_testfloatvec", userpath, basepid);

    /*
     *  First we'll try a simple file where all the nodes
     *  have contiguous access to whole file, except that
     *  each starts with a different displacement.  The
     *  displacement is BUFLEN * myid * 2, to keep the nodes
     *  out of each others' way, but the pointers should
     *  have the same value everywhere.
     */

    DEBUG_TRACE("Checking local pointer with different displacements");

    amode = MPI_MODE_CREATE | MPI_MODE_RDWR | MPI_MODE_DELETE_ON_CLOSE;

    mpio_result = MPI_File_open(MPI_COMM_WORLD, name1, amode, hints_to_test, &mpio_fh);
    RETURNERRS(mpio_result, MPI_SUCCESS);

    /*
     *  First, have to set the view for this process so that
     *  disp is correct.
     */

    mpio_result = MPI_File_set_view(mpio_fh, disp, MPI_BYTE, MPI_BYTE,
                                    "native", MPI_INFO_NULL);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_File_get_position(mpio_fh, &posn);
    CHECKERRS(mpio_result, MPI_SUCCESS);
    CHECKINTS(posn, 0);

    /* Seek past the first BUFLEN bytes to create a hole */

    DEBUG_TRACE("Checking seek, creating a hole");

    mpio_result = MPI_File_seek(mpio_fh, (MPI_Offset) BUFLEN, MPI_SEEK_SET);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_File_get_position(mpio_fh, &posn);
    CHECKERRS(mpio_result, MPI_SUCCESS);
    CHECKINTS(posn, BUFLEN);

    /* Write some data after the hole */

    DEBUG_TRACE("Checking write and position after writing");

    mpio_result = MPI_File_write(mpio_fh, outbuf, BUFLEN, MPI_BYTE, &mpi_status);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_File_get_position(mpio_fh, &posn);
    CHECKERRS(mpio_result, MPI_SUCCESS);
    CHECKINTS(posn, 2 * BUFLEN);

    /*
     *  Seek to the start of (this node's piece of) the
     *  file and read back the hole and the data.
     */

    DEBUG_TRACE("Checking seek and read back data written");

    mpio_result = MPI_File_seek(mpio_fh, (MPI_Offset) 0, MPI_SEEK_SET);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_File_get_position(mpio_fh, &posn);
    CHECKERRS(mpio_result, MPI_SUCCESS);
    CHECKINTS(posn, 0);

    /* Just read partway through; should read hole (all 0's) */

    DEBUG_TRACE("Checking reading in hole");

    memset(inbuf, -1, BUFLEN / 2);
    mpio_result = MPI_File_read(mpio_fh, inbuf, BUFLEN / 2, MPI_BYTE, &mpi_status);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_File_get_position(mpio_fh, &posn);
    CHECKERRS(mpio_result, MPI_SUCCESS);
    CHECKINTS(posn, BUFLEN / 2);

    mpio_result = MPI_Get_count(&mpi_status, MPI_BYTE, &count);
    CHECKERRS(mpio_result, MPI_SUCCESS);
    CHECKINTS(count, BUFLEN / 2);
    CHECKSTRINGS(nullbuf, inbuf, BUFLEN / 2);

    /* Seek to the end of the hole */

    DEBUG_TRACE("Checking seek forward and read");

    mpio_result = MPI_File_seek(mpio_fh, (MPI_Offset) (BUFLEN / 2), MPI_SEEK_CUR);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_File_get_position(mpio_fh, &posn);
    CHECKERRS(mpio_result, MPI_SUCCESS);
    CHECKINTS(posn, BUFLEN);

    mpio_result = MPI_File_read(mpio_fh, inbuf, BUFLEN, MPI_BYTE, &mpi_status);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_File_get_position(mpio_fh, &posn);
    CHECKERRS(mpio_result, MPI_SUCCESS);
    CHECKINTS(posn, 2 * BUFLEN);

    mpio_result = MPI_Get_count(&mpi_status, MPI_BYTE, &count);
    CHECKERRS(mpio_result, MPI_SUCCESS);
    CHECKINTS(count, BUFLEN);
    CHECKSTRINGS(outbuf, inbuf, BUFLEN);

    /*
     *  Now seek backward from the end of this block and read
     *  these bytes again
     */

    DEBUG_TRACE("Checking seek backward and read");

    mpio_result = MPI_File_seek(mpio_fh, (MPI_Offset) (-BUFLEN / 2), MPI_SEEK_CUR);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_File_get_position(mpio_fh, &posn);
    CHECKERRS(mpio_result, MPI_SUCCESS);
    CHECKINTS(posn, BUFLEN + BUFLEN / 2);

    mpio_result = MPI_File_read(mpio_fh, inbuf, BUFLEN / 2, MPI_BYTE, &mpi_status);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_File_get_position(mpio_fh, &posn);
    CHECKERRS(mpio_result, MPI_SUCCESS);
    CHECKINTS(posn, 2 * BUFLEN);

    mpio_result = MPI_Get_count(&mpi_status, MPI_BYTE, &count);
    CHECKERRS(mpio_result, MPI_SUCCESS);
    CHECKINTS(count, BUFLEN / 2);
    CHECKSTRINGS(outbuf, inbuf, BUFLEN / 2);

    /* Try an illegal seek base */

    DEBUG_TRACE("Checking illegal seek");

    mpio_result = MPI_File_seek(mpio_fh, (MPI_Offset) 0, -1);
    CHECKERRS(mpio_result, MPI_ERR_ARG);

    /* Done with that file */

    mpio_result = MPI_File_close(&mpio_fh);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    /* Try seeking on a closed file */

    mpio_result = MPI_File_seek(mpio_fh, (MPI_Offset) 0, MPI_SEEK_SET);
    CHECKERRS2(mpio_result, MPI_ERR_FILE, MPI_ERR_ARG);

    /*
     *  Now we'll play with a file that has a more complicated
     *  pattern, with interleaved file types.
     */

    /* Nodes get objects in round-robin order */

    distrib = MPI_DISTRIBUTE_BLOCK;
    darg = MPI_DISTRIBUTE_DFLT_DARG;
    mpio_result = MPI_Type_create_darray(numprocs, myid, 1,
                                         &numprocs, &distrib, &darg, &numprocs,
                                         MPI_ORDER_C, MPI_FLOAT, &scatter);
    RETURNERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_Type_commit(&scatter);
    RETURNERRS(mpio_result, MPI_SUCCESS);

    /* Create a "short" vector type */

    mpio_result = MPI_Type_contiguous(10, MPI_FLOAT, &floatarray);
    RETURNERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_Type_commit(&floatarray);
    RETURNERRS(mpio_result, MPI_SUCCESS);

    /* Fill in the array of floats */
    for (i = 0; i < 100; i++) {
        floatvec[i] = i * (myid + 1);
    }

    DEBUG_TRACE("Checking darray filetype, contig/vector buftype");

    amode = MPI_MODE_CREATE | MPI_MODE_RDWR | MPI_MODE_DELETE_ON_CLOSE;

    mpio_result = MPI_File_open(MPI_COMM_WORLD, name2, amode, hints_to_test, &mpio_fh);
    RETURNERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_File_set_view(mpio_fh, 0, MPI_FLOAT, scatter, "native", MPI_INFO_NULL);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    /* Write out the array */

    mpio_result = MPI_File_write(mpio_fh, floatvec, 100, MPI_FLOAT, &mpi_status);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    if (mpio_result == MPI_SUCCESS) {
        mpio_result = MPI_File_get_position(mpio_fh, &posn);
        CHECKERRS(mpio_result, MPI_SUCCESS);
        CHECKINTS(posn, 100);
    }

    mpio_result = MPI_File_sync(mpio_fh);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    /*
     *  Read it back in pieces to see how well file pointers deal
     *  with buftype and filetypes.
     */

    /* Start reading after 20th element */

    mpio_result = MPI_File_seek(mpio_fh, (MPI_Offset) 20, MPI_SEEK_SET);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    if (mpio_result == MPI_SUCCESS) {
        mpio_result = MPI_File_get_position(mpio_fh, &posn);
        CHECKERRS(mpio_result, MPI_SUCCESS);
        CHECKINTS(posn, 20);
    }

    /* Now read back the last 80 items in pieces */

    for (i = 0; i < 4; i++) {
        /* Read two pieces at a time of 10 floats each */
        mpio_result = MPI_File_read(mpio_fh, invec + (i * 20), 2, floatarray, &mpi_status);
        CHECKERRS(mpio_result, MPI_SUCCESS);

        if (mpio_result == MPI_SUCCESS) {
            mpio_result = MPI_Get_count(&mpi_status, MPI_BYTE, &count);
            CHECKERRS(mpio_result, MPI_SUCCESS);
            CHECKINTS(count, 20 * sizeof(float));

            mpio_result = MPI_File_get_position(mpio_fh, &posn);
            CHECKERRS(mpio_result, MPI_SUCCESS);

            if (mpio_result == MPI_SUCCESS) {
                CHECKINTS(posn, 20 + ((i + 1) * 20));
            }
        }
    }

    /* Finally, see if what we read matches what we wrote */

    CHECK(memcmp(invec, floatvec + 20, 80 * sizeof(float)) == 0);

    mpio_result = MPI_File_close(&mpio_fh);
    CHECKERRS(mpio_result, MPI_SUCCESS);

  endtest:

    if (floatarray != MPI_DATATYPE_NULL) {
        mpio_result = MPI_Type_free(&floatarray);
        CHECKERRS(mpio_result, MPI_SUCCESS);
    }
    if (scatter != MPI_DATATYPE_NULL) {
        mpio_result = MPI_Type_free(&scatter);
        CHECKERRS(mpio_result, MPI_SUCCESS);
    }

    LEAVING("test_localpointer");

}       /* test_localpointer */


/*===========================================================================
 *
 * Function:	test_nb_localpointer
 *
 * Synopsis:
 *	void test_nb_localpointer(
 *		int	numprocs,	** IN
 *		int	myid		** IN
 *	)
 *
 * Parameters:
 *	numprocs	how many processors are running test
 *	myid		node number of this processor
 *
 * Description:
 *	Nonblocking version of the local pointer test.
 *
 * Outputs:
 *	Logging and error messages are printed.
 *
 * Interfaces:
 *	MPI_File_close
 *	MPI_File_get_position
 *	MPI_File_open
 *	MPI_File_iread
 *	MPI_File_seek
 *	MPI_File_set_view
 *	MPI_File_sync
 *	MPI_File_iwrite
 *	MPI_Get_count
 *	MPI_Type_commit
 *	MPI_Type_contiguous
 *	MPI_Type_create_darray
 *	MPI_Type_free
 *	MPI_Wait
 *
 *-------------------------------------------------------------------------*/

void test_nb_localpointer(int numprocs, int myid)
{
    MPI_File mpio_fh;
    MPI_Status mpi_status;
    MPI_Request mpi_request;
    int count;
    char name1[MAX_PATH_LEN];
    char name2[MAX_PATH_LEN];
    int mpio_result;
    int i;
    MPI_Offset disp = BUFLEN * myid * 2;
    MPI_Datatype scatter;
    MPI_Datatype floatarray;
    char outbuf[BUFLEN];
    char inbuf[BUFLEN];
    char nullbuf[BUFLEN];
    float floatvec[100];
    float invec[100];
    MPI_Offset posn;
    int amode;
    int distrib;
    int darg;

    ENTERING("test_nb_localpointer");

    memset(outbuf, 'A' + myid, BUFLEN);
    memset(nullbuf, 0, BUFLEN);

    MAKEFILENAME(name1, sizeof(name1), "mpio_nb_testlocalpointer", userpath, basepid);
    MAKEFILENAME(name2, sizeof(name2), "mpio_nb_testfloatvec", userpath, basepid);

    /*
     *  First we'll try a simple file where all the nodes
     *  have contiguous access to whole file, except that
     *  each starts with a different displacement.  The
     *  displacement is BUFLEN * myid * 2, to keep the nodes
     *  out of each others' way, but the pointers should
     *  have the same value everywhere.
     */

    DEBUG_TRACE("Checking local pointer with different displacements");

    amode = MPI_MODE_CREATE | MPI_MODE_RDWR | MPI_MODE_DELETE_ON_CLOSE;

    mpio_result = MPI_File_open(MPI_COMM_WORLD, name1, amode, hints_to_test, &mpio_fh);
    RETURNERRS(mpio_result, MPI_SUCCESS);

    /*
     *  First, have to set the view for this process so that
     *  disp is correct.
     */

    mpio_result = MPI_File_set_view(mpio_fh, disp, MPI_BYTE, MPI_BYTE,
                                    "native", MPI_INFO_NULL);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_File_get_position(mpio_fh, &posn);
    CHECKERRS(mpio_result, MPI_SUCCESS);
    CHECKINTS(posn, 0);

    /* Seek past the first BUFLEN bytes to create a hole */

    DEBUG_TRACE("Checking seek, creating a hole");

    mpio_result = MPI_File_seek(mpio_fh, (MPI_Offset) BUFLEN, MPI_SEEK_SET);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_File_get_position(mpio_fh, &posn);
    CHECKERRS(mpio_result, MPI_SUCCESS);
    CHECKINTS(posn, BUFLEN);

    /* Write some data after the hole */

    DEBUG_TRACE("Checking write and position after writing");

    mpio_result = MPI_File_iwrite(mpio_fh, outbuf, BUFLEN, MPI_BYTE, &mpi_request);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_Wait(&mpi_request, &mpi_status);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_File_get_position(mpio_fh, &posn);
    CHECKERRS(mpio_result, MPI_SUCCESS);
    CHECKINTS(posn, 2 * BUFLEN);

    /*
     *  Seek to the start of (this node's piece of) the
     *  file and read back the hole and the data.
     */

    DEBUG_TRACE("Checking seek and read back data written");

    mpio_result = MPI_File_seek(mpio_fh, (MPI_Offset) 0, MPI_SEEK_SET);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_File_get_position(mpio_fh, &posn);
    CHECKERRS(mpio_result, MPI_SUCCESS);
    CHECKINTS(posn, 0);

    /* Just read partway through; should read hole (all 0's) */

    DEBUG_TRACE("Checking reading in hole");

    memset(inbuf, -1, BUFLEN / 2);
    mpio_result = MPI_File_iread(mpio_fh, inbuf, BUFLEN / 2, MPI_BYTE, &mpi_request);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_Wait(&mpi_request, &mpi_status);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_File_get_position(mpio_fh, &posn);
    CHECKERRS(mpio_result, MPI_SUCCESS);
    CHECKINTS(posn, BUFLEN / 2);

    mpio_result = MPI_Get_count(&mpi_status, MPI_BYTE, &count);
    CHECKERRS(mpio_result, MPI_SUCCESS);
    CHECKINTS(count, BUFLEN / 2);

    CHECKSTRINGS(nullbuf, inbuf, BUFLEN / 2);

    /* Seek to the end of the hole */

    DEBUG_TRACE("Checking seek forward and read");

    mpio_result = MPI_File_seek(mpio_fh, (MPI_Offset) (BUFLEN / 2), MPI_SEEK_CUR);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_File_get_position(mpio_fh, &posn);
    CHECKERRS(mpio_result, MPI_SUCCESS);
    CHECKINTS(posn, BUFLEN);

    mpio_result = MPI_File_iread(mpio_fh, inbuf, BUFLEN, MPI_BYTE, &mpi_request);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_Wait(&mpi_request, &mpi_status);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_File_get_position(mpio_fh, &posn);
    CHECKERRS(mpio_result, MPI_SUCCESS);
    CHECKINTS(posn, 2 * BUFLEN);

    mpio_result = MPI_Get_count(&mpi_status, MPI_BYTE, &count);
    CHECKERRS(mpio_result, MPI_SUCCESS);
    CHECKINTS(count, BUFLEN);
    CHECKSTRINGS(outbuf, inbuf, BUFLEN);

    /*
     *  Now seek backward from the end of this block and read
     *  these bytes again
     */

    DEBUG_TRACE("Checking seek backward and read");

    mpio_result = MPI_File_seek(mpio_fh, (MPI_Offset) (-BUFLEN / 2), MPI_SEEK_CUR);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_File_get_position(mpio_fh, &posn);
    CHECKERRS(mpio_result, MPI_SUCCESS);
    CHECKINTS(posn, BUFLEN + BUFLEN / 2);

    mpio_result = MPI_File_iread(mpio_fh, inbuf, BUFLEN / 2, MPI_BYTE, &mpi_request);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_Wait(&mpi_request, &mpi_status);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_File_get_position(mpio_fh, &posn);
    CHECKERRS(mpio_result, MPI_SUCCESS);
    CHECKINTS(posn, 2 * BUFLEN);

    mpio_result = MPI_Get_count(&mpi_status, MPI_BYTE, &count);
    CHECKERRS(mpio_result, MPI_SUCCESS);
    CHECKINTS(count, BUFLEN / 2);
    CHECKSTRINGS(outbuf, inbuf, BUFLEN / 2);

    /* Try an illegal seek base */

    DEBUG_TRACE("Checking illegal seek");

    mpio_result = MPI_File_seek(mpio_fh, (MPI_Offset) 0, -1);
    CHECKERRS(mpio_result, MPI_ERR_ARG);

    /* Done with that file */

    mpio_result = MPI_File_close(&mpio_fh);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    /* Try seeking on a closed file */

    mpio_result = MPI_File_seek(mpio_fh, (MPI_Offset) 0, MPI_SEEK_SET);
    CHECKERRS2(mpio_result, MPI_ERR_FILE, MPI_ERR_ARG);

    /*
     *  Now we'll play with a file that has a more complicated
     *  pattern, with interleaved file types.
     */

    /* Nodes get objects in round-robin order */

    distrib = MPI_DISTRIBUTE_BLOCK;
    darg = MPI_DISTRIBUTE_DFLT_DARG;
    mpio_result = MPI_Type_create_darray(numprocs, myid, 1,
                                         &numprocs, &distrib, &darg, &numprocs,
                                         MPI_ORDER_C, MPI_FLOAT, &scatter);
    RETURNERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_Type_commit(&scatter);
    RETURNERRS(mpio_result, MPI_SUCCESS);

    /* Create a "short" vector type */

    mpio_result = MPI_Type_contiguous(10, MPI_FLOAT, &floatarray);
    RETURNERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_Type_commit(&floatarray);
    RETURNERRS(mpio_result, MPI_SUCCESS);

    /* Fill in the array of floats */

    for (i = 0; i < 100; i++) {
        floatvec[i] = i * (myid + 1);
    }

    DEBUG_TRACE("Checking darray filetype, contig/vector buftype");

    amode = MPI_MODE_CREATE | MPI_MODE_RDWR | MPI_MODE_DELETE_ON_CLOSE;

    mpio_result = MPI_File_open(MPI_COMM_WORLD, name2, amode, hints_to_test, &mpio_fh);

    CHECKERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_File_set_view(mpio_fh, 0, MPI_FLOAT, scatter, "native", MPI_INFO_NULL);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    /* Write out the array */

    mpio_result = MPI_File_iwrite(mpio_fh, floatvec, 100, MPI_FLOAT, &mpi_request);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_Wait(&mpi_request, &mpi_status);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_File_get_position(mpio_fh, &posn);
    CHECKERRS(mpio_result, MPI_SUCCESS);
    CHECKINTS(posn, 100);

    mpio_result = MPI_File_sync(mpio_fh);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    /*
     *  Read it back in pieces to see how well file pointers deal
     *  with buftype and filetypes.
     */


    /* Start reading after 20th element */

    mpio_result = MPI_File_seek(mpio_fh, (MPI_Offset) 20, MPI_SEEK_SET);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_File_get_position(mpio_fh, &posn);
    CHECKERRS(mpio_result, MPI_SUCCESS);
    CHECKINTS(posn, 20);

    /* Now read back the last 80 items in pieces */

    for (i = 0; i < 4; i++) {
        /* Read two pieces at a time of 10 floats each */
        mpio_result = MPI_File_iread(mpio_fh, invec + (i * 20), 2, floatarray, &mpi_request);
        CHECKERRS(mpio_result, MPI_SUCCESS);

        mpio_result = MPI_Wait(&mpi_request, &mpi_status);
        CHECKERRS(mpio_result, MPI_SUCCESS);

        mpio_result = MPI_File_get_position(mpio_fh, &posn);
        CHECKERRS(mpio_result, MPI_SUCCESS);

        CHECKINTS(posn, 20 + ((i + 1) * 20));
        mpio_result = MPI_Get_count(&mpi_status, MPI_BYTE, &count);
        CHECKERRS(mpio_result, MPI_SUCCESS);
        CHECKINTS(count, 20 * sizeof(float));
    }

    /* Finally, see if what we read matches what we wrote */

    CHECK(memcmp(invec, floatvec + 20, 80 * sizeof(float)) == 0);

    mpio_result = MPI_File_close(&mpio_fh);
    CHECKERRS(mpio_result, MPI_SUCCESS);

  endtest:

    if (floatarray != MPI_DATATYPE_NULL) {
        mpio_result = MPI_Type_free(&floatarray);
        CHECKERRS(mpio_result, MPI_SUCCESS);
    }
    if (scatter != MPI_DATATYPE_NULL) {
        mpio_result = MPI_Type_free(&scatter);
        CHECKERRS(mpio_result, MPI_SUCCESS);
    }

    LEAVING("test_nb_localpointer");

}       /* test_nb_localpointer */


/*===========================================================================
 *
 * Function:	test_collective
 *
 * Synopsis:
 *	void test_collective(
 *		int	numprocs,	** IN
 *		int	myid		** IN
 *	)
 *
 * Parameters:
 *	numprocs	how many processors are running test
 *	myid		node number of this processor
 *
 * Description:
 *	Tests the collective I/O operations.
 *
 * Outputs:
 *	Logging and error messages are printed.
 *
 * Interfaces:
 *	MPI_File_close
 *	MPI_File_open
 *	MPI_File_read_at_all
 *	MPI_File_set_view
 *	MPI_File_sync
 *	MPI_File_write_at_all
 *	MPI_Get_count
 *	MPI_Info_dup
 *	MPI_Info_free
 *	MPI_Info_set
 *	MPI_Type_create_darray
 *	MPI_Type_contiguous
 *	MPI_Type_commit
 *	MPI_Type_free
 *	MPI_Type_size
 *
 *-------------------------------------------------------------------------*/

#define BIG_BUFSIZE (1<<10)
#define SLEEP_TIME (3)  /* seconds */

void test_collective(int numprocs, int myid)
{
    char *outbuf = NULL;
    char *inbuf = NULL;
    MPI_File mpio_fh;
    MPI_Status mpi_status;
    int mpio_result;
    int count;
    int num_write;
    MPI_Offset disp = BIG_BUFSIZE * myid;
    MPI_Datatype scatter = MPI_DATATYPE_NULL;
    MPI_Datatype block_type = MPI_DATATYPE_NULL;
    MPI_Datatype big_buf_type = MPI_DATATYPE_NULL;
    MPI_Datatype my_type;
    char name[MAX_PATH_LEN];
    int amode;
    int distrib;
    int darg;
    MPI_Info collective_hints;

    ENTERING("test_collective");

    outbuf = malloc(BIG_BUFSIZE);
    inbuf = malloc(BIG_BUFSIZE);

    if (outbuf == NULL || inbuf == NULL) {
        DEBUG_TRACE("Couldn't allocate buffers for collective tests");
        RETURN;
    }

    mpio_result = MPI_Type_contiguous(BIG_BUFSIZE, MPI_BYTE, &big_buf_type);
    RETURNERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_Type_commit(&big_buf_type);
    RETURNERRS(mpio_result, MPI_SUCCESS);

    /* Test read/write with explicit interleaving */

    MAKEFILENAME(name, sizeof(name), "mpio_collective1", userpath, basepid);

    mpio_result = MPI_Info_dup(hints_to_test, &collective_hints);
    RETURNERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_Info_set(collective_hints, "access_style", "collective_only");
    RETURNERRS(mpio_result, MPI_SUCCESS);

    amode = MPI_MODE_CREATE | MPI_MODE_RDWR | MPI_MODE_DELETE_ON_CLOSE;

    mpio_result = MPI_File_open(MPI_COMM_WORLD, name, amode, collective_hints, &mpio_fh);
    RETURNERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_Info_free(&collective_hints);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    DEBUG_TRACE("Checking collective with explicit interleaved offsets");

    memset(outbuf, 'A' + myid, BIG_BUFSIZE);
    memset(inbuf, 0, BIG_BUFSIZE);
    mpio_result = MPI_File_write_at_all(mpio_fh, disp, outbuf, 1, big_buf_type, &mpi_status);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    if (mpio_result == MPI_SUCCESS) {
        mpio_result = MPI_Get_count(&mpi_status, MPI_BYTE, &count);
        CHECKERRS(mpio_result, MPI_SUCCESS);
        CHECKINTS(count, BIG_BUFSIZE);

        mpio_result = MPI_File_sync(mpio_fh);
        CHECKERRS(mpio_result, MPI_SUCCESS);

        mpio_result = MPI_File_read_at_all(mpio_fh, disp, inbuf, 1, big_buf_type, &mpi_status);
        CHECKERRS(mpio_result, MPI_SUCCESS);

        if (mpio_result == MPI_SUCCESS) {
            mpio_result = MPI_Get_count(&mpi_status, MPI_BYTE, &count);
            CHECKERRS(mpio_result, MPI_SUCCESS);
            CHECKINTS(count, BIG_BUFSIZE);
            CHECKSTRINGS(outbuf, inbuf, BIG_BUFSIZE);
        }
    }

    mpio_result = MPI_File_close(&mpio_fh);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    /* Test read/write with interleaved data */

    /*
     *  First create a file type that consists of blocks to be
     *  written in round-robin order.
     */

    mpio_result = MPI_Type_contiguous(64, MPI_BYTE, &block_type);
    RETURNERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_Type_commit(&block_type);
    RETURNERRS(mpio_result, MPI_SUCCESS);

    /* Nodes get objects in round-robin order */
    distrib = MPI_DISTRIBUTE_BLOCK;
    darg = MPI_DISTRIBUTE_DFLT_DARG;
    mpio_result = MPI_Type_create_darray(numprocs, myid, 1,
                                         &numprocs, &distrib, &darg, &numprocs,
                                         MPI_ORDER_C, block_type, &scatter);
    RETURNERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_Type_commit(&scatter);
    RETURNERRS(mpio_result, MPI_SUCCESS);

    /* Now open the file for interleaved data */

    MAKEFILENAME(name, sizeof(name), "mpio_collective2", userpath, basepid);

    DEBUG_TRACE("Checking collective with interleaved filetype");

    amode = MPI_MODE_CREATE | MPI_MODE_RDWR | MPI_MODE_DELETE_ON_CLOSE;

    mpio_result = MPI_File_open(MPI_COMM_WORLD, name, amode, hints_to_test, &mpio_fh);
    RETURNERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_File_set_view(mpio_fh, 0, MPI_BYTE, scatter, "native", MPI_INFO_NULL);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    memset(outbuf, 'A' + myid, BIG_BUFSIZE);
    memset(inbuf, 0, BIG_BUFSIZE);
    mpio_result = MPI_File_write_at_all(mpio_fh, 0, outbuf, 1, big_buf_type, &mpi_status);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    if (mpio_result == MPI_SUCCESS) {
        mpio_result = MPI_Get_count(&mpi_status, MPI_BYTE, &count);
        CHECKERRS(mpio_result, MPI_SUCCESS);
        CHECKINTS(count, BIG_BUFSIZE);

        mpio_result = MPI_File_sync(mpio_fh);
        CHECKERRS(mpio_result, MPI_SUCCESS);

        mpio_result = MPI_File_read_at_all(mpio_fh, 0, inbuf, 1, big_buf_type, &mpi_status);
        CHECKERRS(mpio_result, MPI_SUCCESS);

        if (mpio_result == MPI_SUCCESS) {
            mpio_result = MPI_Get_count(&mpi_status, MPI_BYTE, &count);
            CHECKERRS(mpio_result, MPI_SUCCESS);
            CHECKINTS(count, BIG_BUFSIZE);
            CHECKSTRINGS(outbuf, inbuf, BIG_BUFSIZE);
        }
    }

    mpio_result = MPI_File_close(&mpio_fh);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    /* Test read/write with different data sizes for each node */

    MAKEFILENAME(name, sizeof(name), "mpio_collective3", userpath, basepid);

    amode = MPI_MODE_CREATE | MPI_MODE_RDWR | MPI_MODE_DELETE_ON_CLOSE;

    mpio_result = MPI_File_open(MPI_COMM_WORLD, name, amode, hints_to_test, &mpio_fh);
    RETURNERRS(mpio_result, MPI_SUCCESS);

    DEBUG_TRACE("Checking collective with nonuniform sizes");

    memset(outbuf, 'A' + myid, BIG_BUFSIZE);
    memset(inbuf, 0, BIG_BUFSIZE);

    num_write = (myid % 2) ? BIG_BUFSIZE : 64;
    my_type = (myid % 2) ? big_buf_type : block_type;

    mpio_result = MPI_File_write_at_all(mpio_fh, disp, outbuf, 1, my_type, &mpi_status);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    if (mpio_result == MPI_SUCCESS) {
        mpio_result = MPI_Type_size(my_type, &num_write);
        CHECKERRS(mpio_result, MPI_SUCCESS);

        mpio_result = MPI_Get_count(&mpi_status, MPI_BYTE, &count);
        CHECKERRS(mpio_result, MPI_SUCCESS);

        CHECKERRS(mpio_result, MPI_SUCCESS);
        CHECKINTS(count, num_write);

        mpio_result = MPI_File_sync(mpio_fh);
        CHECKERRS(mpio_result, MPI_SUCCESS);

        mpio_result = MPI_File_read_at_all(mpio_fh, disp, inbuf, 1, my_type, &mpi_status);
        CHECKERRS(mpio_result, MPI_SUCCESS);

        if (mpio_result == MPI_SUCCESS) {
            mpio_result = MPI_Get_count(&mpi_status, MPI_BYTE, &count);
            CHECKERRS(mpio_result, MPI_SUCCESS);
            CHECKINTS(count, num_write);
            CHECKSTRINGS(outbuf, inbuf, count);
        }
    }

    mpio_result = MPI_File_close(&mpio_fh);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    /* Test time-delayed access by one node */

    MAKEFILENAME(name, sizeof(name), "mpio_collective4", userpath, basepid);

    amode = MPI_MODE_CREATE | MPI_MODE_RDWR | MPI_MODE_DELETE_ON_CLOSE;

    mpio_result = MPI_File_open(MPI_COMM_WORLD, name, amode, hints_to_test, &mpio_fh);
    RETURNERRS(mpio_result, MPI_SUCCESS);

    DEBUG_TRACE("Checking collective with time delays");

    num_write = 1024;   /* modest amount, worth aggregating */
    memset(outbuf, 'A' + myid, BIG_BUFSIZE);
    memset(inbuf, 0, BIG_BUFSIZE);

    /* Delay one node to see how the collective system responds */

    if (myid == 0)
        sleep(SLEEP_TIME);

    mpio_result = MPI_File_write_at_all(mpio_fh,
                                        disp, outbuf, num_write, MPI_BYTE, &mpi_status);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    if (mpio_result == MPI_SUCCESS) {
        mpio_result = MPI_Get_count(&mpi_status, MPI_BYTE, &count);
        CHECKERRS(mpio_result, MPI_SUCCESS);
        CHECKINTS(count, num_write);

        mpio_result = MPI_File_sync(mpio_fh);
        CHECKERRS(mpio_result, MPI_SUCCESS);

        mpio_result = MPI_File_read_at_all(mpio_fh,
                                           disp, inbuf, num_write, MPI_BYTE, &mpi_status);
        CHECKERRS(mpio_result, MPI_SUCCESS);

        if (mpio_result == MPI_SUCCESS) {
            mpio_result = MPI_Get_count(&mpi_status, MPI_BYTE, &count);
            CHECKERRS(mpio_result, MPI_SUCCESS);
            CHECKINTS(count, num_write);
            CHECKSTRINGS(outbuf, inbuf, count);
        }
    }

    mpio_result = MPI_File_close(&mpio_fh);
    CHECKERRS(mpio_result, MPI_SUCCESS);

  endtest:

    if (outbuf != NULL)
        free(outbuf);
    if (inbuf != NULL)
        free(inbuf);

    if (scatter != MPI_DATATYPE_NULL) {
        mpio_result = MPI_Type_free(&scatter);
        CHECKERRS(mpio_result, MPI_SUCCESS);
    }
    if (block_type != MPI_DATATYPE_NULL) {
        mpio_result = MPI_Type_free(&block_type);
        CHECKERRS(mpio_result, MPI_SUCCESS);
    }
    if (big_buf_type != MPI_DATATYPE_NULL) {
        mpio_result = MPI_Type_free(&big_buf_type);
        CHECKERRS(mpio_result, MPI_SUCCESS);
    }

    LEAVING("test_collective");

}       /* test_collective */


/*===========================================================================
 *
 * Function:	test_nb_rdwr
 *
 * Synopsis:
 *	void test_nb_rdwr(
 *		int	numprocs,	** IN
 *		int	myid		** IN
 *	)
 *
 * Parameters:
 *	numprocs	how many processors are running test
 *	myid		node number of this processor
 *
 * Description:
 *	Tests nonblocking, local pointer I/O using derived file type
 *	and buffer type.
 *
 * Outputs:
 *	Logging and error messages are printed.
 *
 * Interfaces:
 *	MPI_File_open
 *	MPI_File_close
 *	MPI_File_set_view
 *	MPI_File_iread_at
 *	MPI_File_iwrite_at
 *	MPI_File_set_atomicity
 *	MPI_Get_count
 *	MPI_Test
 *	MPI_Type_commit
 *	MPI_Type_extent
 *	MPI_Type_free
 *	MPI_Type_struct
 *	MPI_Wait
 *
 *-------------------------------------------------------------------------*/

void test_nb_rdwr(int numprocs, int myid)
{
    MPI_File mpio_fh;
    MPI_Datatype file_type = MPI_DATATYPE_NULL;
    MPI_Datatype buf_type = MPI_DATATYPE_NULL;
    MPI_Status mpi_status;
    MPI_Request mpi_request;
    char *buf = NULL;
    char *rbuf = NULL;
    char *p;
    int bufcount;
    MPI_Offset offset;
    int i;
    MPI_Aint buf_extent;
    int buf_blocklen;
    int mpio_result;
    int blens[3];
    MPI_Aint disps[3];
    MPI_Datatype types[3];
    char name[MAX_PATH_LEN];
    int count;
    int size;
    int amode;
    int flag;

    ENTERING("test_nb_rdwr");

    /* Set up buftype */

    buf_blocklen = 2048;
    blens[0] = 1;
    blens[1] = buf_blocklen;
    blens[2] = 1;
    disps[0] = 0;
    disps[1] = blens[1] * myid;
    disps[2] = blens[1] * numprocs;
    types[0] = MPI_LB;
    types[1] = MPI_CHAR;
    types[2] = MPI_UB;

    mpio_result = MPI_Type_struct(3, blens, disps, types, &buf_type);
    RETURNERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_Type_commit(&buf_type);
    RETURNERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_Type_extent(buf_type, &buf_extent);
    RETURNERRS(mpio_result, MPI_SUCCESS);

    /* Set up filetype */

    blens[0] = 1;
    blens[1] = 4096;
    blens[2] = 1;
    disps[0] = 0;
    disps[1] = blens[1] * myid;
    disps[2] = blens[1] * numprocs;
    types[0] = MPI_LB;
    types[1] = MPI_CHAR;
    types[2] = MPI_UB;

    mpio_result = MPI_Type_struct(3, blens, disps, types, &file_type);
    RETURNERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_Type_commit(&file_type);
    RETURNERRS(mpio_result, MPI_SUCCESS);

    /* Set up buffers */

    offset = 0;
    bufcount = 8;
    size = bufcount * buf_extent;
    buf = malloc(size);
    if (buf == NULL) {
        DEBUG_TRACE("Couldn't allocate space for buf");
        RETURN;
    }
    rbuf = malloc(size);
    if (rbuf == NULL) {
        DEBUG_TRACE("Couldn't allocate space for rbuf");
        RETURN;
    }
    memset(buf, 0, size);
    memset(rbuf, 0, size);
    p = buf;
    for (i = 0; i < bufcount; i++) {
        memset((void *) (p + myid * buf_blocklen), '0' + myid, buf_blocklen);
        p += buf_extent;
    }

    MAKEFILENAME(name, sizeof(name), "mpio_nb_rdwr", userpath, basepid);

    DEBUG_TRACE("Checking nonblocking, local pointer I/O");

    amode = MPI_MODE_CREATE | MPI_MODE_RDWR | MPI_MODE_DELETE_ON_CLOSE;

    mpio_result = MPI_File_open(MPI_COMM_WORLD, name, amode, hints_to_test, &mpio_fh);
    RETURNERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_File_set_view(mpio_fh, 0, MPI_CHAR, file_type, "native", MPI_INFO_NULL);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_File_set_atomicity(mpio_fh, TRUE);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_File_iwrite_at(mpio_fh, offset, buf, bufcount, buf_type, &mpi_request);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_Test(&mpi_request, &flag, &mpi_status);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    while (!flag && mpio_result == MPI_SUCCESS) {
        sleep(1);
        mpio_result = MPI_Test(&mpi_request, &flag, &mpi_status);
        CHECKERRS(mpio_result, MPI_SUCCESS);
    }

    if (mpio_result == MPI_SUCCESS) {
        mpio_result = MPI_Get_count(&mpi_status, MPI_BYTE, &count);
        CHECKERRS(mpio_result, MPI_SUCCESS);
        CHECKINTS(count, buf_blocklen * bufcount);

        mpio_result = MPI_File_iread_at(mpio_fh,
                                        offset, rbuf, bufcount, buf_type, &mpi_request);
        CHECKERRS(mpio_result, MPI_SUCCESS);

        mpio_result = MPI_Wait(&mpi_request, &mpi_status);
        CHECKERRS(mpio_result, MPI_SUCCESS);

        mpio_result = MPI_Get_count(&mpi_status, MPI_BYTE, &count);
        CHECKERRS(mpio_result, MPI_SUCCESS);
        CHECKINTS(count, buf_blocklen * bufcount);
        CHECKSTRINGS(buf, rbuf, size);
    }

    mpio_result = MPI_File_close(&mpio_fh);
    CHECKERRS(mpio_result, MPI_SUCCESS);

  endtest:

    if (buf != NULL)
        free(buf);
    if (rbuf != NULL)
        free(rbuf);

    if (file_type != MPI_DATATYPE_NULL) {
        mpio_result = MPI_Type_free(&file_type);
        CHECKERRS(mpio_result, MPI_SUCCESS);
    }
    if (buf_type != MPI_DATATYPE_NULL) {
        mpio_result = MPI_Type_free(&buf_type);
        CHECKERRS(mpio_result, MPI_SUCCESS);
    }

    LEAVING("test_nb_rdwr");

}       /* test_nb_rdwr */


/*===========================================================================
 *
 * Function:	test_nb_readwrite
 *
 * Synopsis:
 *	void test_nb_readwrite(
 *		int	numprocs,	** IN
 *		int	myid		** IN
 *	)
 *
 * Parameters:
 *	numprocs	how many processors are running test
 *	myid		node number of this processor
 *
 * Description:
 *	Tests independent, nonblocking I/O calls with flavors of Test/Wait.
 *	Each node writes some bytes to a separate part of the same file
 *	and then reads them back and checks to see that they match.
 *
 * Outputs:
 *	Logging and error messages are printed.
 *
 * Interfaces:
 *	MPI_File_close
 *	MPI_File_open
 *	MPI_File_iread_at
 *	MPI_File_iwrite_at
 *	MPI_File_sync
 *	MPI_Get_count
 *	MPI_Testall
 *	MPI_Testany
 *	MPI_Testsome
 *	MPI_Waitall
 *	MPI_Waitany
 *	MPI_Waitsome
 *
 *-------------------------------------------------------------------------*/

void test_nb_readwrite(int numprocs, int myid)
{
    MPI_File mpio_fh;
    MPI_Status mpi_status[3];
    MPI_Request mpi_request[3];
    int indices[3];
    int mpio_result;
    int count;
    char outbuf[BUFLEN];
    char inbuf[BUFLEN];
    MPI_Offset myoffset = myid * BUFLEN;
    char name[MAX_PATH_LEN];
    int amode;
    int i;
    int index;
    int previous;
    int flag;

    ENTERING("test_nb_readwrite");

    MAKEFILENAME(name, sizeof(name), "mpio_nb_readwrite", userpath, basepid);

    DEBUG_TRACE("Checking nonblocking, independent I/O");

    amode = MPI_MODE_CREATE | MPI_MODE_RDWR | MPI_MODE_DELETE_ON_CLOSE;

    mpio_result = MPI_File_open(MPI_COMM_WORLD, name, amode, hints_to_test, &mpio_fh);
    RETURNERRS(mpio_result, MPI_SUCCESS);

    for (i = 0; i < 3; i++)
        mpi_request[i] = MPI_REQUEST_NULL;

    /* Buffer contains A's for node 0, B's for node 1, etc. */
    memset(outbuf, 'A' + myid, BUFLEN);

    mpio_result = MPI_File_iwrite_at(mpio_fh, myoffset, outbuf, 0, MPI_BYTE, &mpi_request[0]);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_File_iwrite_at(mpio_fh,
                                     myoffset, outbuf, BUFLEN, MPI_BYTE, &mpi_request[2]);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    DEBUG_TRACE("Checking Waitall, then Testall");

    mpio_result = MPI_Waitall(3, mpi_request, mpi_status);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    /* Wrote correct number of bytes? */

    mpio_result = MPI_Get_count(&mpi_status[0], MPI_BYTE, &count);
    CHECKERRS(mpio_result, MPI_SUCCESS);
    CHECKINTS(count, 0);

    mpio_result = MPI_Get_count(&mpi_status[2], MPI_BYTE, &count);
    CHECKERRS(mpio_result, MPI_SUCCESS);
    CHECKINTS(count, BUFLEN);

    CHECK(mpi_status[1].MPI_TAG == MPI_ANY_TAG);
    CHECK(mpi_status[1].MPI_SOURCE == MPI_ANY_SOURCE);

    mpio_result = MPI_Get_count(&mpi_status[1], MPI_BYTE, &count);
    CHECKERRS(mpio_result, MPI_SUCCESS);
    CHECKINTS(count, 0);

    /* Now check Testall */

    memset(mpi_status, 0, sizeof(mpi_status));
    mpio_result = MPI_Testall(3, mpi_request, &flag, mpi_status);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    /* Since we previously did a Waitall, all should be inactive! */

    CHECKINTS(flag, 1);
    for (i = 0; i < count; i++) {
        CHECK(mpi_status[i].MPI_TAG == MPI_ANY_TAG);
        CHECK(mpi_status[i].MPI_SOURCE == MPI_ANY_SOURCE);

        mpio_result = MPI_Get_count(&mpi_status[i], MPI_BYTE, &count);
        CHECKERRS(mpio_result, MPI_SUCCESS);
        CHECKINTS(count, 0);
    }

    /* Now reissue Waitall */

    DEBUG_TRACE("Checking Waitall again");

    memset(mpi_status, 0, sizeof(mpi_status));
    mpio_result = MPI_Waitall(3, mpi_request, mpi_status);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    /* all should still be inactive! */

    for (i = 0; i < count; i++) {
        CHECK(mpi_status[i].MPI_TAG == MPI_ANY_TAG);
        CHECK(mpi_status[i].MPI_SOURCE == MPI_ANY_SOURCE);

        mpio_result = MPI_Get_count(&mpi_status[i], MPI_BYTE, &count);
        CHECKERRS(mpio_result, MPI_SUCCESS);
        CHECKINTS(count, 0);
    }

    mpio_result = MPI_File_sync(mpio_fh);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    /* Clear buffer before reading back into it */

    memset(inbuf, 0, BUFLEN);

    /* Clear request array */

    for (i = 0; i < 3; i++)
        mpi_request[i] = MPI_REQUEST_NULL;

    mpio_result = MPI_File_iread_at(mpio_fh, myoffset, inbuf, 0, MPI_BYTE, &mpi_request[2]);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_File_iread_at(mpio_fh,
                                    myoffset, inbuf, BUFLEN, MPI_BYTE, &mpi_request[1]);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    /* Do Testany, Testsome before Waitany */

    DEBUG_TRACE("Checking Testany/Testsome before Waitany/Waitsome");

    do {
        mpio_result = MPI_Testany(3, mpi_request, &index, &flag, mpi_status);
        CHECKERRS(mpio_result, MPI_SUCCESS);
    } while (!flag || index == MPI_UNDEFINED);

    /* Should have found at least one complete, but not 0 */

    CHECK(index != 0);
    previous = index;

    /* Read correct number of bytes? */

    mpio_result = MPI_Get_count(mpi_status, MPI_BYTE, &count);
    CHECKERRS(mpio_result, MPI_SUCCESS);
    if (index == 2) {
        CHECKINTS(count, 0);
    }
    else {
        CHECKINTS(count, BUFLEN);
    }

    /* Testsome should match remaining active request */

    do {
        mpio_result = MPI_Testsome(3, mpi_request, &count, indices, mpi_status);
        CHECKERRS(mpio_result, MPI_SUCCESS);
    } while (count == 0);

    CHECKINTS(count, 1);
    index = indices[0];
    CHECK(previous != index);
    CHECK(index != 0);

    /* Read correct number of bytes? */

    mpio_result = MPI_Get_count(&mpi_status[0], MPI_BYTE, &count);
    CHECKERRS(mpio_result, MPI_SUCCESS);
    if (index == 2) {
        CHECKINTS(count, 0);
    }
    else {
        CHECKINTS(count, BUFLEN);
    }

    mpio_result = MPI_Waitany(3, mpi_request, &index, mpi_status);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    /* All requests should be inactive now */

    CHECKINTS(index, MPI_UNDEFINED);

    mpio_result = MPI_Waitsome(3, mpi_request, &count, indices, mpi_status);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    CHECKINTS(count, MPI_UNDEFINED);

    CHECKSTRINGS(outbuf, inbuf, BUFLEN);

    /* Clear buffer before reading again */

    memset(inbuf, 0, BUFLEN);

    /* Clear request array */

    for (i = 0; i < 3; i++)
        mpi_request[i] = MPI_REQUEST_NULL;

    mpio_result = MPI_File_iread_at(mpio_fh, myoffset, inbuf, 0, MPI_BYTE, &mpi_request[0]);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_File_iread_at(mpio_fh,
                                    myoffset, inbuf, BUFLEN, MPI_BYTE, &mpi_request[1]);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    /* This time use Waitany and Waitsome */

    DEBUG_TRACE("Checking Waitany/Waitsome before Testany/Testsome");

    mpio_result = MPI_Waitany(3, mpi_request, &index, mpi_status);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    /* Should have found at least one complete, but not 2 */

    CHECK(index != MPI_UNDEFINED);
    CHECK(index != 2);
    previous = index;

    /* Read correct number of bytes? */

    mpio_result = MPI_Get_count(mpi_status, MPI_BYTE, &count);
    CHECKERRS(mpio_result, MPI_SUCCESS);
    if (index == 0) {
        CHECKINTS(count, 0);
    }
    else {
        CHECKINTS(count, BUFLEN);
    }

    /* Waitsome should match remaining active request */

    DEBUG_TRACE("Checking Waitsome, not all inactive");

    mpio_result = MPI_Waitsome(3, mpi_request, &count, indices, mpi_status);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    CHECKINTS(count, 1);
    index = indices[0];
    CHECK(previous != index);
    CHECK(index != 2);

    /* Read correct number of bytes? */

    mpio_result = MPI_Get_count(&mpi_status[0], MPI_BYTE, &count);
    CHECKERRS(mpio_result, MPI_SUCCESS);
    if (index == 0) {
        CHECKINTS(count, 0);
    }
    else {
        CHECKINTS(count, BUFLEN);
    }

    mpio_result = MPI_Testany(3, mpi_request, &index, &flag, mpi_status);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    /* All requests should be inactive now */

    CHECKINTS(flag, 1);
    CHECKINTS(index, MPI_UNDEFINED);

    mpio_result = MPI_Testsome(3, mpi_request, &count, indices, mpi_status);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    CHECKINTS(count, MPI_UNDEFINED);

    CHECKSTRINGS(outbuf, inbuf, BUFLEN);

    mpio_result = MPI_File_close(&mpio_fh);
    CHECKERRS(mpio_result, MPI_SUCCESS);

  endtest:

    LEAVING("test_nb_readwrite");

}       /* test_nb_readwrite */


/*===========================================================================
 *
 * Function:	test_nb_collective
 *
 * Synopsis:
 *	void test_nb_collective(
 *		int	numprocs,	** IN
 *		int	myid		** IN
 *	)
 *
 * Parameters:
 *	numprocs	how many processors are running test
 *	myid		node number of this processor
 *
 * Description:
 *	Nonblocking version of the collective tests.
 *
 * Outputs:
 *	Logging and error messages are printed.
 *
 * Interfaces:
 *	MPI_File_close
 *	MPI_File_open
 *	MPI_File_read_at_all_begin
 *	MPI_File_read_at_all_end
 *	MPI_File_set_atomicity
 *	MPI_File_set_view
 *	MPI_File_write_at_all_begin
 *	MPI_File_write_at_all_end
 *	MPI_Get_count
 *	MPI_Type_contiguous
 *	MPI_Type_commit
 *	MPI_Type_create_darray
 *	MPI_Type_free
 *	MPI_Type_size
 *
 *-------------------------------------------------------------------------*/

#define BIG_BUFSIZE (1<<10)
#define SLEEP_TIME (3)  /* seconds */

void test_nb_collective(int numprocs, int myid)
{
    char outbuf[BIG_BUFSIZE];
    char inbuf[BIG_BUFSIZE];
    MPI_File mpio_fh;
    MPI_Status mpi_status;
    int mpio_result;
    int count;
    int num_write;
    MPI_Offset disp = BIG_BUFSIZE * myid;
    MPI_Datatype scatter = MPI_DATATYPE_NULL;
    MPI_Datatype block_type = MPI_DATATYPE_NULL;
    MPI_Datatype big_buf_type = MPI_DATATYPE_NULL;
    MPI_Datatype my_type;
    char name[MAX_PATH_LEN];
    int amode;
    int distrib;
    int darg;

    ENTERING("test_nb_collective");

    mpio_result = MPI_Type_contiguous(BIG_BUFSIZE, MPI_BYTE, &big_buf_type);
    RETURNERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_Type_commit(&big_buf_type);
    RETURNERRS(mpio_result, MPI_SUCCESS);

    /* Test read/write with no interleaving */

    MAKEFILENAME(name, sizeof(name), "mpio_nb_collective1", userpath, basepid);

    DEBUG_TRACE("Checking nb_collective with explicit interleaved offsets");

    amode = MPI_MODE_CREATE | MPI_MODE_RDWR | MPI_MODE_DELETE_ON_CLOSE;

    mpio_result = MPI_File_open(MPI_COMM_WORLD, name, amode, hints_to_test, &mpio_fh);
    RETURNERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_File_set_atomicity(mpio_fh, TRUE);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    memset(outbuf, 'A' + myid, BIG_BUFSIZE);
    memset(inbuf, 0, BIG_BUFSIZE);

    mpio_result = MPI_File_write_at_all_begin(mpio_fh, disp, outbuf, 1, big_buf_type);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_File_write_at_all_end(mpio_fh, outbuf, &mpi_status);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_Get_count(&mpi_status, MPI_BYTE, &count);
    CHECKERRS(mpio_result, MPI_SUCCESS);
    CHECKINTS(count, BIG_BUFSIZE);

    mpio_result = MPI_File_read_at_all_begin(mpio_fh, disp, inbuf, 1, big_buf_type);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_File_read_at_all_end(mpio_fh, inbuf, &mpi_status);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_Get_count(&mpi_status, MPI_BYTE, &count);
    CHECKERRS(mpio_result, MPI_SUCCESS);
    CHECKINTS(count, BIG_BUFSIZE);
    CHECKSTRINGS(outbuf, inbuf, BIG_BUFSIZE);

    mpio_result = MPI_File_close(&mpio_fh);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    /* Test read/write with interleaved data */

    /*
     *  First create a file type that consists of blocks to be
     *  written in round-robin order.
     */

    mpio_result = MPI_Type_contiguous(64, MPI_BYTE, &block_type);
    RETURNERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_Type_commit(&block_type);
    RETURNERRS(mpio_result, MPI_SUCCESS);

    /* Nodes get objects in round-robin order */

    distrib = MPI_DISTRIBUTE_BLOCK;
    darg = MPI_DISTRIBUTE_DFLT_DARG;
    mpio_result = MPI_Type_create_darray(numprocs, myid, 1,
                                         &numprocs, &distrib, &darg, &numprocs,
                                         MPI_ORDER_C, block_type, &scatter);
    RETURNERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_Type_commit(&scatter);
    RETURNERRS(mpio_result, MPI_SUCCESS);

    /* Now open the file for interleaved data */

    MAKEFILENAME(name, sizeof(name), "mpio_nb_collective2", userpath, basepid);

    DEBUG_TRACE("Checking nb_collective with interleaved filetype");

    amode = MPI_MODE_CREATE | MPI_MODE_RDWR | MPI_MODE_DELETE_ON_CLOSE;

    mpio_result = MPI_File_open(MPI_COMM_WORLD, name, amode, hints_to_test, &mpio_fh);
    RETURNERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_File_set_view(mpio_fh, 0, MPI_BYTE, scatter, "native", MPI_INFO_NULL);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    memset(outbuf, 'A' + myid, BIG_BUFSIZE);
    memset(inbuf, 0, BIG_BUFSIZE);

    mpio_result = MPI_File_write_at_all_begin(mpio_fh, 0, outbuf, 1, big_buf_type);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_File_write_at_all_end(mpio_fh, outbuf, &mpi_status);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_Get_count(&mpi_status, MPI_BYTE, &count);
    CHECKERRS(mpio_result, MPI_SUCCESS);
    CHECKINTS(count, BIG_BUFSIZE);

    mpio_result = MPI_File_read_at_all_begin(mpio_fh, 0, inbuf, 1, big_buf_type);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_File_read_at_all_end(mpio_fh, inbuf, &mpi_status);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_Get_count(&mpi_status, MPI_BYTE, &count);
    CHECKERRS(mpio_result, MPI_SUCCESS);
    CHECKINTS(count, BIG_BUFSIZE);
    CHECKSTRINGS(outbuf, inbuf, BIG_BUFSIZE);

    mpio_result = MPI_File_close(&mpio_fh);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    /* Test read/write with different data sizes for each node */
    MAKEFILENAME(name, sizeof(name), "mpio_nb_collective3", userpath, basepid);

    DEBUG_TRACE("Checking nb_collective with nonuniform sizes");

    amode = MPI_MODE_CREATE | MPI_MODE_RDWR | MPI_MODE_DELETE_ON_CLOSE;

    mpio_result = MPI_File_open(MPI_COMM_WORLD, name, amode, hints_to_test, &mpio_fh);
    RETURNERRS(mpio_result, MPI_SUCCESS);

    memset(outbuf, 'A' + myid, BIG_BUFSIZE);
    memset(inbuf, 0, BIG_BUFSIZE);

    num_write = (myid % 2) ? BIG_BUFSIZE : 64;
    my_type = (myid % 2) ? big_buf_type : block_type;

    mpio_result = MPI_File_write_at_all_begin(mpio_fh, disp, outbuf, 1, my_type);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_File_write_at_all_end(mpio_fh, outbuf, &mpi_status);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_Type_size(my_type, &num_write);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_Get_count(&mpi_status, MPI_BYTE, &count);
    CHECKERRS(mpio_result, MPI_SUCCESS);
    CHECKINTS(count, num_write);

    mpio_result = MPI_File_read_at_all_begin(mpio_fh, disp, inbuf, 1, my_type);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_File_read_at_all_end(mpio_fh, inbuf, &mpi_status);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_Get_count(&mpi_status, MPI_BYTE, &count);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    CHECKINTS(count, num_write);
    CHECKSTRINGS(outbuf, inbuf, count);

    mpio_result = MPI_File_close(&mpio_fh);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    /* Test time-delayed access by one node */

    MAKEFILENAME(name, sizeof(name), "mpio_nb_collective4", userpath, basepid);

    DEBUG_TRACE("Checking nb_collective with time delays");

    amode = MPI_MODE_CREATE | MPI_MODE_RDWR | MPI_MODE_DELETE_ON_CLOSE;

    mpio_result = MPI_File_open(MPI_COMM_WORLD, name, amode, hints_to_test, &mpio_fh);
    RETURNERRS(mpio_result, MPI_SUCCESS);

    num_write = 1024;   /* modest amount, worth aggregating */
    memset(outbuf, 'A' + myid, BIG_BUFSIZE);
    memset(inbuf, 0, BIG_BUFSIZE);

    /* Delay one node to see how the collective system responds */

    if (myid == 0)
        sleep(SLEEP_TIME);

    mpio_result = MPI_File_write_at_all_begin(mpio_fh, disp, outbuf, num_write, MPI_BYTE);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_File_write_at_all_end(mpio_fh, outbuf, &mpi_status);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_Get_count(&mpi_status, MPI_BYTE, &count);
    CHECKERRS(mpio_result, MPI_SUCCESS);
    CHECKINTS(count, num_write);

    mpio_result = MPI_File_read_at_all_begin(mpio_fh, disp, inbuf, num_write, MPI_BYTE);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_File_read_at_all_end(mpio_fh, inbuf, &mpi_status);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_Get_count(&mpi_status, MPI_BYTE, &count);
    CHECKERRS(mpio_result, MPI_SUCCESS);
    CHECKINTS(count, num_write);
    CHECKSTRINGS(outbuf, inbuf, count);

    mpio_result = MPI_File_close(&mpio_fh);
    CHECKERRS(mpio_result, MPI_SUCCESS);

  endtest:

    if (scatter != MPI_DATATYPE_NULL) {
        mpio_result = MPI_Type_free(&scatter);
        CHECKERRS(mpio_result, MPI_SUCCESS);
    }
    if (block_type != MPI_DATATYPE_NULL) {
        mpio_result = MPI_Type_free(&block_type);
        CHECKERRS(mpio_result, MPI_SUCCESS);
    }
    if (big_buf_type != MPI_DATATYPE_NULL) {
        mpio_result = MPI_Type_free(&big_buf_type);
        CHECKERRS(mpio_result, MPI_SUCCESS);
    }

    LEAVING("test_nb_collective");

}       /* test_nb_collective */


/*===========================================================================
 *
 * Function:    test_sharedpointer
 *
 * Synopsis:
 *	void test_sharedpointer(
 *		int	numprocs,	** IN
 *		int	myid		** IN
 *	)
 *
 * Parameters:
 *	numprocs	how many processors are running test
 *	myid		node number of this processor
 *
 * Description:
 *	Tests the shared pointer I/O operations.
 *
 * Outputs:
 *	Logging and error messages are printed.
 *
 * Interfaces:
 *	MPI_File_close
 *	MPI_File_get_position_shared
 *	MPI_File_get_size
 *	MPI_File_open
 *	MPI_File_read_ordered
 *	MPI_File_read_shared
 *	MPI_File_seek_shared
 *	MPI_File_sync
 *	MPI_File_write_ordered
 *	MPI_File_write_shared
 *	MPI_Get_count
 *
 *-------------------------------------------------------------------------*/

void test_sharedpointer(int numprocs, int myid)
{
    MPI_File mpio_fh;
    MPI_Status mpi_status;
    int count;
    char name[MAX_PATH_LEN];
    int mpio_result;
    MPI_Offset offset, filesize;
    char outbuf[BUFLEN];
    char inbuf[BUFLEN];
    int amode_in;

    ENTERING("test_sharedpointer");

    memset(outbuf, 'A' + myid, BUFLEN);

    MAKEFILENAME(name, sizeof(name), "mpio_testshared", userpath, basepid);

    amode_in = MPI_MODE_CREATE | MPI_MODE_RDWR | MPI_MODE_DELETE_ON_CLOSE;

    mpio_result = MPI_File_open(MPI_COMM_WORLD, name, amode_in, hints_to_test, &mpio_fh);
    RETURNERRS(mpio_result, MPI_SUCCESS);

    DEBUG_TRACE("Checking MPI_File_write_shared");

    mpio_result = MPI_File_write_shared(mpio_fh, outbuf, BUFLEN, MPI_BYTE, &mpi_status);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    /* File sync is collective but need not be synchronizing */
    mpio_result = MPI_File_sync(mpio_fh);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_Get_count(&mpi_status, MPI_BYTE, &count);
    CHECKERRS(mpio_result, MPI_SUCCESS);
    CHECKINTS(count, BUFLEN);

    MPI_Barrier(MPI_COMM_WORLD);
    mpio_result = MPI_File_get_position_shared(mpio_fh, &offset);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    /*
     * printf("Offset is %x (%d)\n", (int)offset, (int)offset);
     * printf("BUFLEN = %x (%d)\n", BUFLEN, BUFLEN);
     */

    /*
     *  Can't check that offset is exactly BUFLEN since each
     *  node writes BUFLEN bytes.  But the offset should be
     *  some multiple (>1) times BUFLEN.
     *
     * This is because the write shared operations are independent,
     * so the offset may be any multiple of BUFLEN, upto the
     * size of the communicator.
     */

    CHECKINTS(offset % BUFLEN, 0);
    CHECK((offset / BUFLEN) >= 1);

    /*  Seek to end of file - should be numprocs * BUFLEN */

    DEBUG_TRACE("Checking MPI_File_seek_shared (end)");

    mpio_result = MPI_File_seek_shared(mpio_fh, (MPI_Offset) 0, MPI_SEEK_END);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_File_get_position_shared(mpio_fh, &offset);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    if (mpio_result == MPI_SUCCESS) {
        CHECKINTS(offset, numprocs * BUFLEN);
    }

    /* Now seek back to start of file */
    /* WDG - 9/4/03
     * Note that MPI_File_get_position_shared is *not* collective,
     * so the above call may be executed on some processes after other
     * processes have begun the MPI_File_seek_shared below.  To
     * avoid this, we insert an MPI_Barrier to ensure that
     * all processes complete the get_position call before
     * beginning the read */
    MPI_Barrier(MPI_COMM_WORLD);

    DEBUG_TRACE("Checking MPI_File_seek_shared (0)");

    mpio_result = MPI_File_seek_shared(mpio_fh, (MPI_Offset) 0, MPI_SEEK_SET);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_File_get_position_shared(mpio_fh, &offset);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    CHECKINTS(offset, 0);

    /*
     *  Prevent subsequent read from proceeding until everyone
     *  has checked seek position.
     */

    /*MPI_File_sync(mpio_fh); */
    /* WDG - 9/8/03
     * File_sync does not ensure that the subsequent read doesn't
     * proceed until all processes have checked the position.
     * For this, MPI_Barrier is necessary */
    MPI_Barrier(MPI_COMM_WORLD);

    DEBUG_TRACE("Checking MPI_File_read_shared");

    mpio_result = MPI_File_read_shared(mpio_fh, inbuf, BUFLEN, MPI_BYTE, &mpi_status);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    /*
     *  No guarantee that we read what we wrote, so only check
     *  that the count is correct!
     */

    mpio_result = MPI_Get_count(&mpi_status, MPI_BYTE, &count);
    CHECKERRS(mpio_result, MPI_SUCCESS);
    CHECKINTS(count, BUFLEN);

    mpio_result = MPI_File_close(&mpio_fh);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    MAKEFILENAME(name, sizeof(name), "mpio_testordered", userpath, basepid);

    amode_in = MPI_MODE_CREATE | MPI_MODE_RDWR | MPI_MODE_DELETE_ON_CLOSE;

    mpio_result = MPI_File_open(MPI_COMM_WORLD, name, amode_in, hints_to_test, &mpio_fh);
    RETURNERRS(mpio_result, MPI_SUCCESS);

    DEBUG_TRACE("Checking MPI_File_write_ordered");

    mpio_result = MPI_File_write_ordered(mpio_fh, outbuf, BUFLEN, MPI_BYTE, &mpi_status);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_Get_count(&mpi_status, MPI_BYTE, &count);
    CHECKERRS(mpio_result, MPI_SUCCESS);
    CHECKINTS(count, BUFLEN);

    mpio_result = MPI_File_sync(mpio_fh);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_File_get_size(mpio_fh, &filesize);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    if (mpio_result == MPI_SUCCESS) {
        CHECKINTS(filesize, numprocs * BUFLEN);
    }

    mpio_result = MPI_File_seek_shared(mpio_fh, (MPI_Offset) 0, MPI_SEEK_SET);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    /* Check that we are about to read at position 0 */

    mpio_result = MPI_File_get_position_shared(mpio_fh, &offset);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    if (mpio_result == MPI_SUCCESS) {
        CHECKINTS(offset, 0);
    }

    /* WDG - 9/4/03
     * Note that MPI_File_get_position_shared is *not* collective,
     * so the above call may be executed on some processes after other
     * processes have begun the MPI_File_read_ordered below.  To
     * avoid this, we insert an MPI_Barrier to ensure that
     * all processes complete the get_position call before
     * beginning the read */
    MPI_Barrier(MPI_COMM_WORLD);

    DEBUG_TRACE("Checking MPI_File_read_ordered");

    mpio_result = MPI_File_read_ordered(mpio_fh, inbuf, BUFLEN, MPI_BYTE, &mpi_status);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_Get_count(&mpi_status, MPI_BYTE, &count);
    CHECKERRS(mpio_result, MPI_SUCCESS);
    CHECKINTS(count, BUFLEN);

    /*
     *  Now we should get back exactly what we wrote!
     */

    CHECKSTRINGS(outbuf, inbuf, count);

    mpio_result = MPI_File_close(&mpio_fh);
    CHECKERRS(mpio_result, MPI_SUCCESS);

  endtest:

    LEAVING("test_sharedpointer");

}       /* test_sharedpointer */


/*===========================================================================
 *
 * Function:    test_nb_sharedpointer
 *
 * Synopsis:
 *	void test_nb_sharedpointer(
 *		int	numprocs,	** IN
 *		int	myid		** IN
 *	)
 *
 * Parameters:
 *	numprocs	how many processors are running test
 *	myid		node number of this processor
 *
 * Description:
 *	Tests the shared pointer, nonblocking I/O operations.
 *
 * Outputs:
 *	Logging and error messages are printed.
 *
 * Interfaces:
 *	MPI_File_close
 *	MPI_File_get_position_shared
 *	MPI_File_get_size
 *	MPI_File_open
 *	MPI_File_read_ordered_begin
 *	MPI_File_read_ordered_end
 *	MPI_File_iread_shared
 *	MPI_File_seek_shared
 *	MPI_File_sync
 *	MPI_File_write_ordered_begin
 *	MPI_File_write_ordered_end
 *	MPI_File_iwrite_shared
 *	MPI_Get_count
 *	MPI_Wait
 *
 *-------------------------------------------------------------------------*/

void test_nb_sharedpointer(int numprocs, int myid)
{
    MPI_File mpio_fh;
    MPI_Status mpi_status;
    MPI_Request mpi_request;
    int count;
    char name[MAX_PATH_LEN];
    int mpio_result;
    MPI_Offset offset;
    char outbuf[BUFLEN];
    char inbuf[BUFLEN];
    int amode_in;

    ENTERING("test_nb_sharedpointer");

    memset(outbuf, 'A' + myid, BUFLEN);

    MAKEFILENAME(name, sizeof(name), "mpio_nb_testshared", userpath, basepid);

    amode_in = MPI_MODE_CREATE | MPI_MODE_RDWR | MPI_MODE_DELETE_ON_CLOSE;

    mpio_result = MPI_File_open(MPI_COMM_WORLD, name, amode_in, hints_to_test, &mpio_fh);
    RETURNERRS(mpio_result, MPI_SUCCESS);

    DEBUG_TRACE("Checking MPI_File_iwrite_shared");

    mpio_result = MPI_File_iwrite_shared(mpio_fh, outbuf, BUFLEN, MPI_BYTE, &mpi_request);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_Wait(&mpi_request, &mpi_status);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_Get_count(&mpi_status, MPI_BYTE, &count);
    CHECKERRS(mpio_result, MPI_SUCCESS);
    CHECKINTS(count, BUFLEN);

    mpio_result = MPI_File_sync(mpio_fh);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_File_get_position_shared(mpio_fh, &offset);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    /*
     *  Can't check that offset is exactly BUFLEN since each
     *  node writes BUFLEN bytes.  But the offset should be
     *  some multiple (>1) times BUFLEN.
     */

    CHECKINTS(offset % BUFLEN, 0);
    CHECK((offset / BUFLEN) >= 1);

    /*  Seek back to beginning of file */
    /* Make sure that the get_position calls have ALL completed
     * before changing the position */
    MPI_Barrier(MPI_COMM_WORLD);

    mpio_result = MPI_File_seek_shared(mpio_fh, (MPI_Offset) 0, MPI_SEEK_SET);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    DEBUG_TRACE("Checking MPI_File_iread_shared");

    mpio_result = MPI_File_iread_shared(mpio_fh, inbuf, BUFLEN, MPI_BYTE, &mpi_request);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_Wait(&mpi_request, &mpi_status);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    /*
     *  No guarantee that we read what we wrote, so only check
     *  that the count is correct.
     */

    mpio_result = MPI_Get_count(&mpi_status, MPI_BYTE, &count);
    CHECKERRS(mpio_result, MPI_SUCCESS);
    CHECKINTS(count, BUFLEN);

    mpio_result = MPI_File_close(&mpio_fh);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    MAKEFILENAME(name, sizeof(name), "mpio_nb_testordered", userpath, basepid);

    mpio_result = MPI_File_open(MPI_COMM_WORLD, name, amode_in, hints_to_test, &mpio_fh);
    RETURNERRS(mpio_result, MPI_SUCCESS);

    DEBUG_TRACE("Checking MPI_File_write_ordered_begin");

    mpio_result = MPI_File_write_ordered_begin(mpio_fh, outbuf, BUFLEN, MPI_BYTE);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_File_write_ordered_end(mpio_fh, outbuf, &mpi_status);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_Get_count(&mpi_status, MPI_BYTE, &count);
    CHECKERRS(mpio_result, MPI_SUCCESS);
    CHECKINTS(count, BUFLEN);

    /*  Seek back to beginning of file */

    mpio_result = MPI_File_seek_shared(mpio_fh, (MPI_Offset) 0, MPI_SEEK_SET);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    DEBUG_TRACE("Checking MPI_File_read_ordered_begin");

    mpio_result = MPI_File_sync(mpio_fh);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_File_read_ordered_begin(mpio_fh, inbuf, BUFLEN, MPI_BYTE);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_File_read_ordered_end(mpio_fh, inbuf, &mpi_status);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_Get_count(&mpi_status, MPI_BYTE, &count);
    CHECKERRS(mpio_result, MPI_SUCCESS);
    CHECKINTS(count, BUFLEN);

    /*
     *  Now we should get back exactly what we wrote!
     */

    CHECKSTRINGS(outbuf, inbuf, count);

    mpio_result = MPI_File_close(&mpio_fh);
    CHECKERRS(mpio_result, MPI_SUCCESS);

  endtest:

    LEAVING("test_nb_sharedpointer");

}       /* test_nb_sharedpointer */


/*===========================================================================
 *
 * Function:	test_errhandlers
 *
 * Synopsis:
 *	void test_errhandlers(
 *		int	numprocs,	** IN
 *		int	myid		** IN
 *	)
 *
 * Parameters:
 *	numprocs	how many processors are running test
 *	myid		node number of this processor
 *
 * Description:
 *	Tests error handlers for files, individually and globally.
 *	Opens and closes an MPI-IO file, creates errhandlers, sets the
 *	file errhandler to be a user-defined errhandler.  Then it causes
 *	a few errors to occur so that the error handler will be exercised.
 *
 * Outputs:
 *	Logging and error messages are printed.
 *
 * Interfaces:
 *	some_handler
 *	user_handler
 *	MPI_Errhandler_create
 *	MPI_Errhandler_free
 *	MPI_Error_string
 *	MPI_File_close
 *	MPI_File_create_errhandler
 *	MPI_File_get_errhandler
 *	MPI_File_open
 *	MPI_File_read_at
 *	MPI_File_set_errhandler
 *	MPI_File_set_view
 *
 *-------------------------------------------------------------------------*/

static void user_handler(MPI_File * fh, int *errorcode,
/* Some versions of MPI don't support variable arguments */
#ifdef USE_STDARG
                         ...
#else                           /* USE_STDARG */
                         char *a, int *b, int *c
#endif                          /* USE_STDARG */
)
{
    char errstring[MPI_MAX_ERROR_STRING];
    int string_len;

    if (*errorcode != MPI_SUCCESS && verbose) {
        MPI_Error_string(*errorcode, errstring, &string_len);
        fprintf(stderr, "***In user_handler:  %s\n", errstring);
    }

    return;
}

static void some_handler(MPI_Comm * mpi_comm, int *errorcode,
/* Some versions of MPI don't support variable arguments */
#ifdef USE_STDARG
                         ...
#else                           /* USE_STDARG */
                         char *a, int *b, int *c
#endif                          /* USE_STDARG */
)
{
    fprintf(stderr, "In some_handler:  should never get here!");
    return;
}

void test_errhandlers(int numprocs, int myid)
{
    MPI_File mpio_fh;
    int mpio_result;
    char name[MAX_PATH_LEN];
    int amode;
    MPI_Errhandler handler;
    MPI_Errhandler current;
    MPI_Status mpi_status;
    char inbuf[80];

    ENTERING("test_errhandlers");

    MAKEFILENAME(name, sizeof(name), "mpio_errhandler", userpath, basepid);

    amode = MPI_MODE_CREATE | MPI_MODE_WRONLY | MPI_MODE_DELETE_ON_CLOSE;

    mpio_result = MPI_File_open(MPI_COMM_WORLD, name, amode, hints_to_test, &mpio_fh);
    RETURNERRS(mpio_result, MPI_SUCCESS);

    /* Check the default error handler for a file */

    DEBUG_TRACE("Checking default error handlers");

    mpio_result = MPI_File_get_errhandler(mpio_fh, &current);
    CHECKERRS(mpio_result, MPI_SUCCESS);
    CHECK(current == MPI_ERRORS_RETURN);

    /* Try setting error handler; should fail, not a file error handler */

    DEBUG_TRACE("Checking erroneous MPI_File_set_errhandler");

    mpio_result = MPI_Errhandler_create(some_handler, &handler);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_File_set_errhandler(mpio_fh, handler);
    CHECKERRS(mpio_result, MPI_ERR_ARG);

    MPI_Errhandler_free(&handler);

    /* Now create and set a correct file error handler */

    DEBUG_TRACE("Checking correct MPI_File_set_errhandler");

    mpio_result = MPI_File_create_errhandler(user_handler, &handler);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_File_set_errhandler(mpio_fh, handler);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_File_get_errhandler(mpio_fh, &current);
    CHECKERRS(mpio_result, MPI_SUCCESS);
    CHECK(current == handler);

    MPI_Errhandler_free(&current);

    /* Try to read; this should fail and invoke error handler */

    /*DEBUG_TRACE("Checking error handling through user-defined handler"); */
    /* WDG - since the user_handler prints output, */
    if (verbose) {
        fprintf(stderr, "*** Checking error handler\n");
        fprintf(stderr, "*** Manually check for reasonable messages\n");
    }

    mpio_result = MPI_File_read_at(mpio_fh, 0, inbuf, 80, MPI_BYTE, &mpi_status);
    /* WDG - FIXME: MPI_ERR_UNSUPPORTED_OPERATION is another
     * valid class for this error */
    CHECKERRS2(mpio_result, MPI_ERR_ACCESS, MPI_ERR_UNSUPPORTED_OPERATION);

    /* Try to set view with bogus datarep; this should fail */

    mpio_result = MPI_File_set_view(mpio_fh, 0, MPI_INT, MPI_INT, "unknown", MPI_INFO_NULL);
    CHECKERRS(mpio_result, MPI_ERR_UNSUPPORTED_DATAREP);

    mpio_result = MPI_File_close(&mpio_fh);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    /* Now try setting the global/default error handler */

    DEBUG_TRACE("Checking global error handler");

    mpio_result = MPI_File_set_errhandler(MPI_FILE_NULL, handler);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    /* This should fail */

    mpio_result = MPI_File_close(&mpio_fh);
    CHECKERRS2(mpio_result, MPI_ERR_FILE, MPI_ERR_ARG);

    /* So should this; file doesn't exist */

    amode = MPI_MODE_RDONLY;

    mpio_result = MPI_File_open(MPI_COMM_WORLD, name, amode, hints_to_test, &mpio_fh);

    CHECKERRS(mpio_result, MPI_ERR_NO_SUCH_FILE);

    mpio_result = MPI_File_set_errhandler(MPI_FILE_NULL, MPI_ERRORS_RETURN);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    MPI_Errhandler_free(&handler);

  endtest:

    LEAVING("test_errhandlers");

}       /* test_errhandlers */


/*===========================================================================
 *
 * Function:   test_datareps
 *
 * Synopsis:
 *	void test_datareps(
 *		int	numprocs,	** IN
 *		int	myid		** IN
 *	)
 *
 * Parameters:
 *	numprocs	how many processors are running test
 *	myid		node number of this processor
 *	
 * Description:
 *	Tests features of predefined and user-defined datarep use.
 *
 * Outputs:
 *	Logging and error messages are printed.
 *
 * Interfaces:
 *	MPI_File_close
 *	MPI_File_open
 *	MPI_File_get_size
 *	MPI_File_get_type_extent
 *	MPI_File_get_view
 *	MPI_File_set_view
 *	MPI_File_write
 *	MPI_File_write_at
 *	MPI_File_read_at
 *	MPI_Get_count
 *	MPI_Register_datarep
 *	MPI_Type_commit
 *	MPI_Type_contiguous
 *	MPI_Type_extent
 *	MPI_Type_free
 *	MPI_Type_size
 *	MPI_Type_struct
 *	user_native_extent_fn
 *	user16_extent_fn
 *	user16_read_fn
 *	user16_write_fn
 *	write_then_read
 *
 * Notes:
 *	Since dotests may be repeated, and datareps cannot be unregistered,
 *	we only register the user-defined reps on the first time through
 *	this function.
 *-------------------------------------------------------------------------*/

#ifdef HAVE_MPI_REGISTER_DATAREP
static int user_native_extent_fn(MPI_Datatype dtype, MPI_Aint * file_extent, void *extra_state)
{
    /* Just return normal type extent */

    return MPI_Type_extent(dtype, file_extent);

}       /* user_native_extent_fn */

static int user16_read_fn(void *userbuf,
                          MPI_Datatype buftype,
                          int count, void *filebuf, MPI_Offset position, void *extra_state)
{
    int i;
    char *ubuf = userbuf;
    char *fbuf = filebuf;

    if (buftype != MPI_BYTE)
        return MPI_ERR_ARG;

    for (i = 0; i < count; i++) {
        ubuf[i] = fbuf[16 * i];
    }

    return MPI_SUCCESS;

}       /* user16_read_fn */

static int user16_write_fn(void *userbuf,
                           MPI_Datatype buftype,
                           int count, void *filebuf, MPI_Offset position, void *extra_state)
{
    int i;
    char *ubuf = userbuf;
    char *fbuf = filebuf;

    if (buftype != MPI_BYTE)
        return MPI_ERR_ARG;

    memset(filebuf, 0, count * 16);

    for (i = 0; i < count; i++) {
        fbuf[16 * i] = ubuf[i];
    }

    return MPI_SUCCESS;

}       /* user16_write_fn */

static int user16_extent_fn(MPI_Datatype dtype, MPI_Aint * file_extent, void *extra_state)
{
    if (dtype != MPI_BYTE)
        return MPI_ERR_ARG;

    *file_extent = 16;

    return MPI_SUCCESS;

}       /* user16_extent_fn */

static void write_then_read(int myid, MPI_File mpio_fh)
{
    MPI_Offset myoffset = myid * BUFLEN;
    char outbuf[BUFLEN];
    char inbuf[BUFLEN];
    int mpio_result;
    MPI_Status mpi_status;
    int count;

    /* Buffer contains A's for node 0, B's for node 1, etc. */
    memset(outbuf, 'A' + myid, BUFLEN);

    mpio_result = MPI_File_write_at(mpio_fh, myoffset, outbuf, BUFLEN, MPI_BYTE, &mpi_status);
    RETURNERRS(mpio_result, MPI_SUCCESS);

    /* Wrote correct number of bytes? */
    mpio_result = MPI_Get_count(&mpi_status, MPI_BYTE, &count);
    CHECKERRS(mpio_result, MPI_SUCCESS);
    CHECKINTS(count, BUFLEN);

    /* Reset buffer before reading back into it */
    memset(inbuf, 0, BUFLEN);

    mpio_result = MPI_File_read_at(mpio_fh, myoffset, inbuf, BUFLEN, MPI_BYTE, &mpi_status);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_Get_count(&mpi_status, MPI_BYTE, &count);
    CHECKERRS(mpio_result, MPI_SUCCESS);
    CHECKINTS(count, BUFLEN);

    CHECKSTRINGS(outbuf, inbuf, BUFLEN);

  endtest:

    return;

}       /* write_then_read */
#endif

#ifdef HAVE_MPI_REGISTER_DATAREP
void test_datareps(int numprocs, int myid)
{
    MPI_Offset myoffset = myid * BUFLEN;
    MPI_File mpio_fh;
    int mpio_result;
    char name[MAX_PATH_LEN];
    char datarep[MPI_MAX_DATAREP_STRING];
    int amode;
    MPI_Offset disp;
    MPI_Datatype etype;
    MPI_Datatype filetype;
    MPI_Offset filesize;
    int blens[5];
    MPI_Aint disps[5];
    MPI_Datatype dtypes[5];
    MPI_Datatype struct_type = MPI_DATATYPE_NULL;
    MPI_Datatype contig_type = MPI_DATATYPE_NULL;
    MPI_Aint native_extent;
    MPI_Aint external32_extent;
    int mpi_size;
    MPI_Offset native_size;
    MPI_Offset external32_size;
    struct memtype {
        int x;
        char y;
        long double z;
    };
    struct memtype outbuffer[5];
    struct memtype inbuffer[5];
    int count;
    MPI_Status mpi_status;
    static int first_time = 1;
    int i;

    ENTERING("test_datareps");

    MAKEFILENAME(name, sizeof(name), "mpio_datarep", userpath, basepid);

    amode = MPI_MODE_CREATE | MPI_MODE_RDWR | MPI_MODE_DELETE_ON_CLOSE;

    mpio_result = MPI_File_open(MPI_COMM_WORLD, name, amode, hints_to_test, &mpio_fh);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    /* In default view, datarep should be "native" */

    DEBUG_TRACE("Checking default datarep");

    mpio_result = MPI_File_get_view(mpio_fh, &disp, &etype, &filetype, datarep);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    CHECK(strcmp(datarep, "native") == 0);

    if (etype != MPI_BYTE) {
        mpio_result = MPI_Type_free(&etype);
        CHECKERRS(mpio_result, MPI_SUCCESS);
    }
    if (filetype != MPI_BYTE) {
        mpio_result = MPI_Type_free(&filetype);
        CHECKERRS(mpio_result, MPI_SUCCESS);
    }

    DEBUG_TRACE("Checking all predefined datareps");

    /* Check setting view to external32 */

    mpio_result = MPI_File_set_view(mpio_fh, disp, MPI_BYTE, MPI_BYTE,
                                    "external32", MPI_INFO_NULL);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_File_get_view(mpio_fh, &disp, &etype, &filetype, datarep);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    DEBUG_TRACE("Checking returned values from get_view with external32");

    CHECK(strcmp(datarep, "external32") == 0);

    /* For external32, should 'dup' etype and filetype! */

    CHECK(etype != MPI_BYTE);
    CHECK(filetype != MPI_BYTE);

    if (etype != MPI_BYTE) {
        mpio_result = MPI_Type_free(&etype);
        CHECKERRS(mpio_result, MPI_SUCCESS);
    }
    if (filetype != MPI_BYTE) {
        mpio_result = MPI_Type_free(&filetype);
        CHECKERRS(mpio_result, MPI_SUCCESS);
    }

    /* Check setting view to internal */

    mpio_result = MPI_File_set_view(mpio_fh, disp, MPI_BYTE, MPI_BYTE,
                                    "internal", MPI_INFO_NULL);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_File_get_view(mpio_fh, &disp, &etype, &filetype, datarep);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    CHECK(strcmp(datarep, "internal") == 0);

    if (etype != MPI_BYTE) {
        mpio_result = MPI_Type_free(&etype);
        CHECKERRS(mpio_result, MPI_SUCCESS);
    }
    if (filetype != MPI_BYTE) {
        mpio_result = MPI_Type_free(&filetype);
        CHECKERRS(mpio_result, MPI_SUCCESS);
    }

    /* Check resetting view to native */

    mpio_result = MPI_File_set_view(mpio_fh, disp, MPI_BYTE, MPI_BYTE,
                                    "native", MPI_INFO_NULL);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_File_get_view(mpio_fh, &disp, &etype, &filetype, datarep);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    CHECK(strcmp(datarep, "native") == 0);

    if (etype != MPI_BYTE) {
        mpio_result = MPI_Type_free(&etype);
        CHECKERRS(mpio_result, MPI_SUCCESS);
    }
    if (filetype != MPI_BYTE) {
        mpio_result = MPI_Type_free(&filetype);
        CHECKERRS(mpio_result, MPI_SUCCESS);
    }

    /* Try using a datarep that hasn't been registered */

    DEBUG_TRACE("Checking an unregistered datarep");

    mpio_result = MPI_File_set_view(mpio_fh, disp, MPI_BYTE, MPI_BYTE,
                                    "unknown", MPI_INFO_NULL);
    CHECKERRS(mpio_result, MPI_ERR_UNSUPPORTED_DATAREP);

    DEBUG_TRACE("Checking registering a duplicate of a predefined datarep");

    mpio_result =
        MPI_Register_datarep("native",
                             MPI_CONVERSION_FN_NULL, MPI_CONVERSION_FN_NULL,
                             user_native_extent_fn, NULL);
    CHECKERRS(mpio_result, MPI_ERR_DUP_DATAREP);

    /* datarep should still be native */

    mpio_result = MPI_File_get_view(mpio_fh, &disp, &etype, &filetype, datarep);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    CHECK(strcmp(datarep, "native") == 0);

    if (etype != MPI_BYTE) {
        mpio_result = MPI_Type_free(&etype);
        CHECKERRS(mpio_result, MPI_SUCCESS);
    }
    if (filetype != MPI_BYTE) {
        mpio_result = MPI_Type_free(&filetype);
        CHECKERRS(mpio_result, MPI_SUCCESS);
    }

    /* Now register a user-defined datarep and use this in set view */

    if (first_time) {
        DEBUG_TRACE("Checking registering a user-defined datarep");

        mpio_result =
            MPI_Register_datarep("user-native",
                                 MPI_CONVERSION_FN_NULL,
                                 MPI_CONVERSION_FN_NULL, user_native_extent_fn, NULL);
        CHECKERRS(mpio_result, MPI_SUCCESS);
    }

    mpio_result = MPI_File_set_view(mpio_fh, disp, MPI_BYTE, MPI_BYTE,
                                    "user-native", MPI_INFO_NULL);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_File_get_view(mpio_fh, &disp, &etype, &filetype, datarep);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    CHECK(strcmp(datarep, "user-native") == 0);

    /* For user-defined type, should dup types */

    CHECK(etype != MPI_BYTE);
    CHECK(filetype != MPI_BYTE);

    if (etype != MPI_BYTE) {
        mpio_result = MPI_Type_free(&etype);
        CHECKERRS(mpio_result, MPI_SUCCESS);
    }
    if (filetype != MPI_BYTE) {
        mpio_result = MPI_Type_free(&filetype);
        CHECKERRS(mpio_result, MPI_SUCCESS);
    }

    /* Use the "user-native" datarep.  Should behave just like native. */

    write_then_read(myid, mpio_fh);

    /* Try registering another "user-native" datarep, "dup" error */

    DEBUG_TRACE("Checking registering a duplicate user-defined datarep");

    mpio_result = MPI_Register_datarep("user-native",
                                       MPI_CONVERSION_FN_NULL, MPI_CONVERSION_FN_NULL,
                                       user_native_extent_fn, NULL);
    CHECKERRS(mpio_result, MPI_ERR_DUP_DATAREP);

    /* Now try a user-defined datarep that is different from native */

    if (first_time) {
        DEBUG_TRACE("Checking registering another user-defined datarep");

        mpio_result = MPI_Register_datarep("user-16",
                                           user16_read_fn,
                                           user16_write_fn, user16_extent_fn, NULL);
        CHECKERRS(mpio_result, MPI_SUCCESS);
    }

    mpio_result = MPI_File_set_view(mpio_fh, disp, MPI_BYTE, MPI_BYTE,
                                    "user-16", MPI_INFO_NULL);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_File_get_view(mpio_fh, &disp, &etype, &filetype, datarep);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    CHECK(strcmp(datarep, "user-16") == 0);

    /* For user-defined type, should dup types */

    CHECK(etype != MPI_BYTE);
    CHECK(filetype != MPI_BYTE);

    if (etype != MPI_BYTE) {
        mpio_result = MPI_Type_free(&etype);
        CHECKERRS(mpio_result, MPI_SUCCESS);
    }
    if (filetype != MPI_BYTE) {
        mpio_result = MPI_Type_free(&filetype);
        CHECKERRS(mpio_result, MPI_SUCCESS);
    }

    DEBUG_TRACE("Checking I/O with user-defined datarep");

    write_then_read(myid, mpio_fh);

    /* Check that the file size was scaled accordingly */

    DEBUG_TRACE("Checking file size after user-defined read/write");

    mpio_result = MPI_File_get_size(mpio_fh, &filesize);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    CHECK(filesize >= (BUFLEN * 16 + myoffset * 16));

    mpio_result = MPI_File_close(&mpio_fh);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    /*
     *  Try using external32 for a datatype with alignment holes.
     *  Should get compact results using byte alignment; file sizes
     *  should differ, but read/write should work correctly to
     *  smooth differences from native in memory and external32 in
     *  the file.
     */

    DEBUG_TRACE("Checking external32 datarep");

    blens[0] = 1;
    blens[1] = 1;
    blens[2] = 1;
    disps[0] = 0;
    disps[1] = sizeof(int);
    disps[2] = sizeof(long double);
    dtypes[0] = MPI_INT;
    dtypes[1] = MPI_BYTE;
    dtypes[2] = MPI_LONG_DOUBLE;

    mpio_result = MPI_Type_struct(3, blens, disps, dtypes, &struct_type);
    RETURNERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_Type_commit(&struct_type);
    RETURNERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_Type_size(struct_type, &mpi_size);
    RETURNERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_Type_contiguous(5, struct_type, &contig_type);
    RETURNERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_Type_commit(&contig_type);
    RETURNERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_File_open(MPI_COMM_WORLD, name, amode, hints_to_test, &mpio_fh);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_File_set_view(mpio_fh, 0LL, struct_type, contig_type,
                                    "native", MPI_INFO_NULL);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    for (i = 0; i < 5; i++) {
        outbuffer[i].x = i + 1;
        outbuffer[i].y = i + 2;
        outbuffer[i].z = i + 3;
    }

    mpio_result = MPI_File_write(mpio_fh, outbuffer, 5, struct_type, &mpi_status);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_Get_count(&mpi_status, MPI_BYTE, &count);
    CHECKERRS(mpio_result, MPI_SUCCESS);
    CHECKINTS(count, mpi_size * 5);

    mpio_result = MPI_File_get_size(mpio_fh, &native_size);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_File_close(&mpio_fh);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_File_open(MPI_COMM_WORLD, name, amode, hints_to_test, &mpio_fh);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    mpio_result = MPI_File_set_view(mpio_fh, 0LL, struct_type, contig_type,
                                    "external32", MPI_INFO_NULL);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    DEBUG_TRACE("Checking extent of struct{INT,BYTE,LONGDOUBLE} in memory");

    mpio_result = MPI_Type_extent(struct_type, &native_extent);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    DEBUG_TRACE("Checking extent of struct{INT,BYTE,LONGDOUBLE} in file");

    mpio_result = MPI_File_get_type_extent(mpio_fh, struct_type, &external32_extent);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    CHECK(native_extent != external32_extent);
    CHECK(external32_extent == native_extent + 8);

    mpio_result = MPI_File_write(mpio_fh, outbuffer, 5, struct_type, &mpi_status);
    CHECKERRS(mpio_result, MPI_SUCCESS);

    if (mpio_result == MPI_SUCCESS) {
        DEBUG_TRACE("Checking size of file for external32 " "!= native size");

        mpio_result = MPI_File_get_size(mpio_fh, &external32_size);
        CHECKERRS(mpio_result, MPI_SUCCESS);

        CHECK(native_size != external32_size);
        CHECK(native_size == 5 * native_extent);
        CHECK(external32_size == 5 * external32_extent);

        DEBUG_TRACE("Checking data written is read back correctly");

        memset(inbuffer, 0, sizeof(inbuffer));

        mpio_result = MPI_File_read_at(mpio_fh, 0, inbuffer, 5, struct_type, &mpi_status);
        CHECKERRS(mpio_result, MPI_SUCCESS);

        for (i = 0; i < 5; i++) {
            CHECK(inbuffer[i].x == i + 1);
            CHECK(inbuffer[i].y == i + 2);
            CHECK(inbuffer[i].z == i + 3);
        }

    }

    mpio_result = MPI_File_close(&mpio_fh);
    CHECKERRS(mpio_result, MPI_SUCCESS);

  endtest:

    if (contig_type != MPI_DATATYPE_NULL) {
        mpio_result = MPI_Type_free(&contig_type);
        CHECKERRS(mpio_result, MPI_SUCCESS);
    }
    if (struct_type != MPI_DATATYPE_NULL) {
        mpio_result = MPI_Type_free(&struct_type);
        CHECKERRS(mpio_result, MPI_SUCCESS);
    }

    first_time = 0;

    LEAVING("test_datareps");

}       /* test_datareps */
#endif

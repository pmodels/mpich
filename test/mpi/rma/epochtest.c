/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2009 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/*
 * This test looks at the behavior of MPI_Win_fence and epochs.  Each 
 * MPI_Win_fence may both begin and end both the exposure and access epochs.
 * Thus, it is not necessary to use MPI_Win_fence in pairs.
 *
 * The tests have this form:
 *    Process A             Process B
 *     fence                 fence
 *      put,put
 *     fence                 fence
 *                            put,put
 *     fence                 fence
 *      put,put               put,put
 *     fence                 fence
 */

#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include "mpitest.h"

/*
static char MTEST_Descrip[] = "Put with Fences used to separate epochs";
*/

#define MAX_PERR 10

int PrintRecvedError( const char *, MTestDatatype *, MTestDatatype * );

int main( int argc, char **argv )
{
    int errs = 0, err;
    int rank, size, source, dest;
    int minsize = 2, count; 
    MPI_Comm      comm;
    MPI_Win       win;
    MPI_Aint      extent;
    MTestDatatype sendtype, recvtype;
    int           onlyInt = 0;

    MTest_Init( &argc, &argv );
    /* Check for a simple choice of communicator and datatypes */
    if (getenv( "MTEST_SIMPLE" )) onlyInt = 1;

    while (MTestGetIntracommGeneral( &comm, minsize, 1 )) {
	if (comm == MPI_COMM_NULL) continue;
	/* Determine the sender and receiver */
	MPI_Comm_rank( comm, &rank );
	MPI_Comm_size( comm, &size );
	source = 0;
	dest   = size - 1;
	
	for (count = 1; count < 65000; count = count * 2) {
	    while (MTestGetDatatypes( &sendtype, &recvtype, count )) {

		MTestPrintfMsg( 1, 
		       "Putting count = %d of sendtype %s receive type %s\n", 
				count, MTestGetDatatypeName( &sendtype ),
				MTestGetDatatypeName( &recvtype ) );

		/* Make sure that everyone has a recv buffer */
		recvtype.InitBuf( &recvtype );

		MPI_Type_extent( recvtype.datatype, &extent );
		MPI_Win_create( recvtype.buf, recvtype.count * extent, 
				extent, MPI_INFO_NULL, comm, &win );
		/* To improve reporting of problems about operations, we
		   change the error handler to errors return */
		MPI_Win_set_errhandler( win, MPI_ERRORS_RETURN );

		/* At this point, we have all of the elements that we 
		   need to begin the multiple fence and put tests */
		/* Fence 1 */
		err = MPI_Win_fence( MPI_MODE_NOPRECEDE, win ); 
		if (err) { if (errs++ < MAX_PERR) MTestPrintError(err); }
		/* Source puts */
		if (rank == source) {
		    sendtype.InitBuf( &sendtype );
		    
		    err = MPI_Put( sendtype.buf, sendtype.count, 
				   sendtype.datatype, dest, 0, 
				   recvtype.count, recvtype.datatype, win );
		    if (err) { if (errs++ < MAX_PERR) MTestPrintError(err); }
		}

		/* Fence 2 */
		err = MPI_Win_fence( 0, win );
		if (err) { if (errs++ < MAX_PERR) MTestPrintError(err); }
		/* dest checks data, then Dest puts */
		if (rank == dest) {
		    err = MTestCheckRecv( 0, &recvtype );
		    if (err) { if (errs++ < MAX_PERR) { 
			    PrintRecvedError( "fence 2", &sendtype, &recvtype );
			}
		    }
		    sendtype.InitBuf( &sendtype );
		    
		    err = MPI_Put( sendtype.buf, sendtype.count, 
				   sendtype.datatype, source, 0, 
				   recvtype.count, recvtype.datatype, win );
		    if (err) { if (errs++ < MAX_PERR) MTestPrintError(err); }
		}

		/* Fence 3 */
		err = MPI_Win_fence( 0, win );
		if (err) { if (errs++ < MAX_PERR) MTestPrintError(err); }
		/* src checks data, then Src and dest puts*/
		if (rank == source) {
		    err = MTestCheckRecv( 0, &recvtype );
		    if (err) { if (errs++ < MAX_PERR) { 
			    PrintRecvedError( "fence 3", &sendtype, &recvtype );
			}
		    }
		    sendtype.InitBuf( &sendtype );
		    
		    err = MPI_Put( sendtype.buf, sendtype.count, 
				   sendtype.datatype, dest, 0, 
				   recvtype.count, recvtype.datatype, win );
		    if (err) { if (errs++ < MAX_PERR) MTestPrintError(err); }
		}
		if (rank == dest) {
		    sendtype.InitBuf( &sendtype );
		    
		    err = MPI_Put( sendtype.buf, sendtype.count, 
				   sendtype.datatype, source, 0, 
				   recvtype.count, recvtype.datatype, win );
		    if (err) { if (errs++ < MAX_PERR) MTestPrintError(err); }
		}

		/* Fence 4 */
		err = MPI_Win_fence( MPI_MODE_NOSUCCEED, win );
		if (err) { if (errs++ < MAX_PERR) MTestPrintError(err); }
		/* src and dest checks data */
		if (rank == source) {
		    err = MTestCheckRecv( 0, &recvtype );
		    if (err) { if (errs++ < MAX_PERR) { 
			    PrintRecvedError( "src fence4", &sendtype, &recvtype );
			}
		    }
		}
		if (rank == dest) {
		    err = MTestCheckRecv( 0, &recvtype );
		    if (err) { if (errs++ < MAX_PERR) { 
			    PrintRecvedError( "dest fence4", &sendtype, &recvtype );
			}
		    }
		}

		MPI_Win_free( &win );
		MTestFreeDatatype( &sendtype );
		MTestFreeDatatype( &recvtype );

		/* Only do one datatype in the simple case */
		if (onlyInt) break;
	    }
	    /* Only do one count in the simple case */
	    if (onlyInt) break;
	}
        MTestFreeComm(&comm);
	/* Only do one communicator in the simple case */
	if (onlyInt) break;
    }

    MTest_Finalize( errs );

    
    
    MPI_Finalize();
    return 0;
}


int PrintRecvedError( const char *msg, 
		      MTestDatatype *sendtypePtr, MTestDatatype *recvtypePtr )
{
    printf( "At step %s, Data in target buffer did not match for destination datatype %s (put with source datatype %s)\n", 
	    msg, 
	    MTestGetDatatypeName( recvtypePtr ),
	    MTestGetDatatypeName( sendtypePtr ) );
    /* Redo the test, with the errors printed */
    recvtypePtr->printErrors = 1;
    (void)MTestCheckRecv( 0, recvtypePtr );
    return 0;
}

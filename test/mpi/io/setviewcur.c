/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include "mpitest.h"

/*
static char MTEST_Descrip[] = "Test set_view with DISPLACEMENT_CURRENT";
*/

int main( int argc, char *argv[] )
{
    int errs = 0, err;
    int size, rank, *buf;
    MPI_Offset offset;
    MPI_File fh;
    MPI_Comm comm;
    MPI_Status status;

    MTest_Init( &argc, &argv );

    /* This test reads a header then sets the view to every "size" int,
       using set view and current displacement.  The file is first written
       using a combination of collective and ordered writes */
       
    comm = MPI_COMM_WORLD;
    err = MPI_File_open( comm, (char*)"test.ord", 
		   MPI_MODE_WRONLY | MPI_MODE_CREATE, MPI_INFO_NULL, &fh );
    if (err) { errs++; MTestPrintErrorMsg( "Open(1)", err ); }
    MPI_Comm_size( comm, &size );
    MPI_Comm_rank( comm, &rank );
    buf = (int *)malloc( size * sizeof(int) );
    buf[0] = size;
    err = MPI_File_write_all( fh, buf, 1, MPI_INT, &status );
    if (err) { errs++; MTestPrintErrorMsg( "Write_all", err ); }
    err = MPI_File_get_position( fh, &offset );
    if (err) { errs++; MTestPrintErrorMsg( "Get_position", err ); }
    err = MPI_File_seek_shared( fh, offset, MPI_SEEK_SET );
    if (err) { errs++; MTestPrintErrorMsg( "Seek_shared", err ); }
    buf[0] = rank;
    err = MPI_File_write_ordered( fh, buf, 1, MPI_INT, &status );
    if (err) { errs++; MTestPrintErrorMsg( "Write_ordered", err ); }
    err = MPI_File_close( &fh );
    if (err) { errs++; MTestPrintErrorMsg( "Close(1)", err ); }

    /* Reopen the file as sequential */
    err = MPI_File_open( comm, (char*)"test.ord", 
		   MPI_MODE_RDONLY | MPI_MODE_SEQUENTIAL | 
			 MPI_MODE_DELETE_ON_CLOSE, MPI_INFO_NULL, &fh );
    if (err) { errs++; MTestPrintErrorMsg( "Open(Read)", err ); }
    
    if (rank == 0) {
	err = MPI_File_read_shared( fh, buf, 1, MPI_INT, &status );
	if (err) { errs++; MTestPrintErrorMsg( "Read_all", err ); }
	if (buf[0] != size) { 
	    errs++;
	    fprintf(stderr, "Unexpected value for the header = %d, should be %d\n",
		    buf[0], size ); fflush(stderr);
	}
    }
    MPI_Barrier( comm );
    /* All processes must provide the same file view for MODE_SEQUENTIAL */
    /* See MPI 2.1, 13.3 - DISPLACEMENT_CURRENT is *required* for 
       MODE_SEQUENTIAL files */
    err = MPI_File_set_view( fh, MPI_DISPLACEMENT_CURRENT, MPI_INT, 
		       MPI_INT, (char*)"native", MPI_INFO_NULL );
    if (err) { 
	errs++; 
	MTestPrintErrorMsg( "Set_view (DISPLACEMENT_CURRENT)", err );
    }
    buf[0] = -1;
    err = MPI_File_read_ordered( fh, buf, 1, MPI_INT, &status );
    if (err) { errs++; MTestPrintErrorMsg( "Read_all", err ); }
    if (buf[0] != rank) {
	errs++;
	fprintf( stderr, "%d: buf[0] = %d\n", rank, buf[0] ); fflush(stderr);
    }

    free( buf );
    err = MPI_File_close( &fh );
    if (err) { errs++; MTestPrintErrorMsg( "Close(2)", err ); }

    MTest_Finalize( errs );
    MPI_Finalize();
    return 0;
}

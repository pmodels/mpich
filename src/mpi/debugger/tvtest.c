/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*  
 *  (C) 2005 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/*
 * This file provides a simple test of the Totalview debugger interface
 * It also illustrates some (but not all) of the operations that the 
 * debugger performs using the routines defined in mpi_interface.h .
 */

/* You can build this with -DNOT_STANDALONE and then run it with a debugger;
   by default, it builds routines to simulate some actions that a 
   debugger might take when running it with the message queue interface */

#include <stdio.h>
#include "mpi.h"
#include <stdlib.h>
#include <string.h>

#include "mpi_interface.h"

/* style: allow:fprintf:8 sig:0 */
/* style: allow:printf:15 sig:0 */

int showQueues( int, int );
int init_dbr(void);

int main( int argc, char *argv[] )
{
    MPI_Request rreq, sreq, rreq2;
    int wrank, wsize;
    int buf = -1, sbuf = 2, rbuf=-1;;
    int vbuf[10];
    MPI_Comm dupworld;

    MPI_Init( &argc, &argv );

    /* Perform the initialization steps for accessing the message queues */
    init_dbr();

    /* Create some pending receives and unexpected messages */
    MPI_Comm_dup( MPI_COMM_WORLD, &dupworld );
    MPI_Comm_set_name( dupworld, "Dup of comm world" );
    MPI_Comm_rank( MPI_COMM_WORLD, &wrank );
    MPI_Comm_size( MPI_COMM_WORLD, &wsize );
    MPI_Irecv( &buf, 1, MPI_INT, (wrank + 1) % wsize, 17, dupworld, &rreq );
    MPI_Irecv( vbuf, 10, MPI_INT, (wrank + 1) % wsize, 19, dupworld, &rreq2 );
    MPI_Isend( &sbuf, 1, MPI_INT, (wrank + wsize - 1) % wsize, 18, 
	       MPI_COMM_WORLD, &sreq );
    /* This relies on buffering short eager messages */
    /*    MPI_Send( &ssbuf, 1, MPI_INT, (wrank + 2) %wsize, 18, dupworld );*/

    /* Access the queues */
    printf( "Should see pending recv with tag 17, 19 on dupworld and send with tag 18 on world\n" );
    showQueues( 3, 3 );
    MPI_Barrier( MPI_COMM_WORLD );

    /* Match up some of the messages */
    MPI_Send( &sbuf, 1, MPI_INT, (wrank + wsize - 1) % wsize, 17, dupworld );
    MPI_Recv( &rbuf, 1, MPI_INT, (wrank + 1) % wsize, 18, MPI_COMM_WORLD, 
    	      MPI_STATUS_IGNORE );
    MPI_Wait( &rreq, MPI_STATUS_IGNORE );
    MPI_Wait( &sreq, MPI_STATUS_IGNORE );

    /* Access the queues again */
    printf( "\nAfter a few send/receives\n" );
    printf( "Should see recv with tag 19 on dupworld\n" );
    showQueues( 3, 1 );

    MPI_Barrier( MPI_COMM_WORLD );
    MPI_Send( &sbuf, 1, MPI_INT, (wrank + wsize - 1) % wsize, 19, dupworld );

    /* Access the queues again */
    printf( "\nAfter a few send/receives (all now matched)\n" );
    showQueues( 3, 0 );

    /* Shut down */
    MPI_Cancel(&rreq2);
    MPI_Wait(&rreq2, MPI_STATUS_IGNORE);
    MPI_Comm_free(&dupworld);

    MPI_Finalize();
    
    return 0;
}

#if !defined(NOT_STANDALONE)
/* ------------------------------------------------------------------------- */
/* Forward references for the functions that simulate debugger operations */
static void dbgrI_put_image_info( mqs_image *image, mqs_image_info *info );
static mqs_image_info *dbgrI_get_image_info( mqs_image *image );
static int dbgrI_find_function( mqs_image *image, char *name, 
				mqs_lang_code lang, mqs_taddr_t *loc );
int dbgrI_find_symbol( mqs_image *image, char *name, mqs_taddr_t * loc );
static void dbgrI_put_process_info( mqs_process *process, 
				    mqs_process_info *info );
static mqs_process_info *dbgrI_get_process_info( mqs_process *process );
static void dbgrI_get_type_sizes( mqs_process *process, 
				  mqs_target_type_sizes *ts );
static int dbgrI_fetch_data( mqs_process *proc, mqs_taddr_t addr, int asize, 
			     void *data );
static void dbgrI_target_to_host (mqs_process *proc, const void *in_data, 
				  void *out_data, int asize );
static mqs_image *dbgrI_get_image( mqs_process *process );
mqs_type * dbgrI_find_type(mqs_image *image, char *name, 
			   mqs_lang_code lang);
int dbgrI_field_offset(mqs_type *type, char *name);
static int dbgrI_get_global_rank( mqs_process *process );

/* The global variables describing this process for the debugger */
static mqs_basic_callbacks   cb;
static mqs_image_callbacks   icb;
static mqs_process_callbacks pcb;
static mqs_image image;
static mqs_process process;

struct mqs_process_ {
    mqs_process_info *p_info;
};
struct mqs_image_ {
    mqs_image_info *i_info;
};

int init_dbr( void )
{
    int hasQ = 0;
    char *version = mqs_version_string();
    char *msg;
    
    if (mqs_version_compatibility() != MQS_INTERFACE_COMPATIBILITY) {
	fprintf( stderr, "Unexpected value of version\n" );
    }

    fprintf( stderr, "Version string: %s\n", version );

    if (mqs_dll_taddr_width() != sizeof(void *)) {
	fprintf( stderr, "Unexpected table width returned\n" ); 
	return 1;
    }

    /* Do the initialization. */
    memset( &cb, 0, sizeof(cb) );
    cb.mqs_malloc_fp           = malloc;
    cb.mqs_free_fp             = free;
    cb.mqs_dprints_fp          = 0;
    cb.mqs_put_image_info_fp   = dbgrI_put_image_info;
    cb.mqs_get_image_info_fp   = dbgrI_get_image_info;
    cb.mqs_put_process_info_fp = dbgrI_put_process_info;
    cb.mqs_get_process_info_fp = dbgrI_get_process_info;
    
    mqs_setup_basic_callbacks( &cb );

    icb.mqs_get_type_sizes_fp = dbgrI_get_type_sizes;
    icb.mqs_find_function_fp  = dbgrI_find_function;
    icb.mqs_find_symbol_fp    = dbgrI_find_symbol;
    icb.mqs_find_type_fp      = dbgrI_find_type;
    icb.mqs_field_offset_fp   = dbgrI_field_offset;
    icb.mqs_sizeof_fp         = 0;
    mqs_setup_image( &image, &icb );

    hasQ = mqs_image_has_queues( &image, &msg );
    if (hasQ == mqs_ok) {
	pcb.mqs_get_global_rank_fp = dbgrI_get_global_rank;
	pcb.mqs_get_image_fp       = dbgrI_get_image;
	pcb.mqs_fetch_data_fp      = dbgrI_fetch_data;
	pcb.mqs_target_to_host_fp  = dbgrI_target_to_host;
	mqs_setup_process( &process, &pcb );
	hasQ = mqs_process_has_queues( &process, &msg );
	if (hasQ != mqs_ok) {
	    fprintf( stderr, "Failed to get queues from process: %s\n", msg );
	}
    }
    else {
	fprintf( stderr, "Failed to get queues from image: %s\n", msg );
    }
    if (hasQ != mqs_ok) return 1;

    return 0;
}

int showQueues( int nComm, int expected )
{
    mqs_communicator comm;
    mqs_pending_operation op;
    int rc;
    int nFound = 0, nCommFound = 0;

    /* Get a stable copy of the active communicators */
    mqs_update_communicator_list( &process );

    mqs_setup_communicator_iterator( &process );
    while (mqs_get_communicator( &process, &comm ) == mqs_ok) {
	printf( "Communicator %s\n", comm.name );
	nCommFound++;
	rc = mqs_setup_operation_iterator( &process, mqs_pending_receives );
	if (rc == mqs_ok) {
	    printf( "Pending receives for communicator %s\n", comm.name );
	    while ((rc = mqs_next_operation( &process, &op )) == mqs_ok) {
		printf( "tag = %d, rank = %d, length = %d\n",
			(int)op.desired_tag, (int)op.desired_local_rank, 
			(int)op.desired_length );
		nFound++;
	    }
	}
	else if (rc == mqs_end_of_list) {
	    /* No operations */
	    printf( "No pending receives for communicator %s\n", comm.name );
	}
	else if (rc == mqs_no_information) {
	    printf( "No information available for communicator %s\n", 
		    comm.name );
	}
	else {
	    fprintf( stderr, 
		"Unknown return %d from mqs_setup_operation_iterator\n", rc );
	}

	rc = mqs_setup_operation_iterator( &process, mqs_unexpected_messages );
	if (rc == mqs_ok) {
	    printf( "Unexpected messages for communicator %s\n", comm.name );
	    while ((rc = mqs_next_operation( &process, &op )) == mqs_ok) {
		printf( "tag = %d, rank = %d, length = %d\n",
			(int)op.desired_tag, (int)op.desired_local_rank, 
			(int)op.desired_length );
		nFound++;
	    }
	}
	else if (rc == mqs_end_of_list) {
	    /* No operations */
	    printf( "No unexpected receives for communicator %s\n", comm.name );
	}
	else if (rc == mqs_no_information) {
	    printf( "No unexpected receive information available for communicator %s\n", 
		    comm.name );
	}
	else {
	    fprintf( stderr, 
		"Unknown return %d from mqs_setup_operation_iterator\n", rc );
	}

	rc = mqs_setup_operation_iterator( &process, mqs_pending_sends );
	if (rc == mqs_ok) {
	    printf( "Pending sends for communicator %s\n", comm.name );
	    while ((rc = mqs_next_operation( &process, &op )) == mqs_ok) {
		printf( "tag = %d, rank = %d, length = %d\n",
			(int)op.desired_tag, (int)op.desired_local_rank, 
			(int)op.desired_length );
		nFound++;
	    }
	}
	else if (rc == mqs_end_of_list) {
	    /* No operations */
	    printf( "No pending sends for communicator %s\n", comm.name );
	}
	else if (rc == mqs_no_information) {
	    printf( "No pending send information available for communicator %s\n", 
		    comm.name );
	}
	else {
	    fprintf( stderr, 
		"Unknown return %d from mqs_setup_operation_iterator\n", rc );
	}

	mqs_next_communicator( &process );
    }

    if (nFound < expected) {
	fprintf( stderr, "Error: expected to find %d queue entries but only saw %d\n", expected, nFound );
    }
    if (nCommFound < nComm) {
	fprintf( stderr, "Error: expected to find %d comms but only saw %d\n",
		 nComm, nCommFound );
    }
	
    fflush(stdout); fflush(stderr);
    return 0;
}



/* ----------------------------------------------------------------------- */
/* Example service routines */
/* FIXME: Move these into dbgstub.c */
/* ----------------------------------------------------------------------- */
static void dbgrI_put_image_info( mqs_image *image, mqs_image_info *info )
{
    image->i_info = info;
}

static mqs_image_info *dbgrI_get_image_info( mqs_image *image )
{
    return image->i_info;
}
static void dbgrI_put_process_info( mqs_process *process, 
				    mqs_process_info *info )
{
    process->p_info = info;
}

static mqs_process_info *dbgrI_get_process_info( mqs_process *process )
{
    return process->p_info;
}

static void dbgrI_get_type_sizes( mqs_process *process, 
				  mqs_target_type_sizes *ts )
{
    ts->short_size     = sizeof(short);
    ts->int_size       = sizeof(int);
    ts->long_size      = sizeof(long);
    ts->long_long_size = sizeof(long long);
    ts->pointer_size   = sizeof(void *);
}
/* This is a hack that knows exactly the names used in dll_mpich.c */
/* Note that if loc is null, don't save the address */
static int dbgrI_find_function( mqs_image *image, char *name, 
				 mqs_lang_code lang, mqs_taddr_t *loc )
{
    if (strcmp( name, "MPIR_Breakpoint" ) == 0) {
#if 0
	if (loc)
	    *loc = (mqs_taddr_t)MPIR_Breakpoint;
#endif
    }
    else {
	if (loc) 
	    *loc = 0;
    }
    return mqs_ok;
}


#define MPIU_Memcpy memcpy
/* Simulate requesting the debugger to fetch data from within this process */
static int dbgrI_fetch_data( mqs_process *proc, mqs_taddr_t addr, int asize, 
			     void *data )
{
    MPIU_Memcpy( data, (void *)addr, (size_t) asize );
    return mqs_ok;
}
/* Simulate converting data to debuggers byte ordering */
static void dbgrI_target_to_host (mqs_process *proc, const void *in_data, 
				  void *out_data, int asize )
{
    MPIU_Memcpy( out_data, in_data, asize );
}

/* Return the "debuggers" image structure (statically allocated above) */
static mqs_image *dbgrI_get_image( mqs_process *process )
{
    return &image;
}

static int dbgrI_get_global_rank( mqs_process *process )
{
    static int wrank = -1;
    if (wrank < 0) {
	MPI_Comm_rank( MPI_COMM_WORLD, &wrank );
    }

    return wrank;
}
#else
int init_dbr(void) { return 0;}
int showQueues(void) { return 0; }
#endif

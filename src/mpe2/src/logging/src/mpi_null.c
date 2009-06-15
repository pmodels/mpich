/*
   (C) 2001 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
#include "mpe_logging_conf.h"

#if defined( HAVE_STDIO_H ) || defined( STDC_HEADERS )
#include <stdio.h>
#endif
#if defined( STDC_HEADERS ) || defined( HAVE_STRING_H )
#include <string.h>
#endif
#if defined( STDC_HEADERS ) || defined( HAVE_STDLIB_H )
#include <stdlib.h>
#endif
#if defined( HAVE_UNISTD_H )
#include <unistd.h>
#endif

#include "clog_mem.h"
#include "clog_const.h"
#include "mpe_callstack.h"
#include "mpi_null.h"

#if defined( NEEDS_GETHOSTNAME_DECL )
int gethostname(char *name, size_t len);
#endif

#define ZMPI_PRINTSTACK() \
        do { \
            MPE_CallStack_t  cstk; \
            MPE_CallStack_init( &cstk ); \
            MPE_CallStack_fancyprint( &cstk, 2, \
                                      "\t", 1, MPE_CALLSTACK_UNLIMITED ); \
        } while (0)

static const int ZMPI_BOOL_FALSE = CLOG_BOOL_FALSE;
static const int ZMPI_BOOL_TRUE  = CLOG_BOOL_TRUE;

#define MPE_COMM_MAX        5
#define MPE_COMM_KEY_MAX   10

typedef struct {
    int   size;
    int   rank;
    void *attrs[MPE_COMM_KEY_MAX];
    int   is_attr_used[MPE_COMM_KEY_MAX];
} ZMPI_Comm_t;

/*
    In this serial MPI implementation: MPI_Comm which is defined as int is
    the index to the ZMPI_COMMSET->commptrs[] arrary.
*/
typedef struct {
    ZMPI_Comm_t  *commptrs[ MPE_COMM_MAX ]; 
    char          name[ MPI_MAX_PROCESSOR_NAME ];
    int           namelen;
    int           is_key_used[ MPE_COMM_KEY_MAX ];
} ZMPI_CommSet_t;

static ZMPI_CommSet_t*  ZMPI_COMMSET = NULL;

int MPI_WTIME_IS_GLOBAL;

int MPI_WTIME_IS_GLOBAL_VALUE;

static ZMPI_Comm_t*  ZMPI_Comm_create( void )
{
    ZMPI_Comm_t  *commptr;
    int           idx;

    commptr = (ZMPI_Comm_t *) MALLOC( sizeof(ZMPI_Comm_t) );
    if ( commptr == NULL ) {
        fprintf( stderr, __FILE__":ZMPI_Comm_create() -- MALLOC fails.\n" );
        fflush( stderr );
        return NULL;
    }
    commptr->size = 1;
    commptr->rank = 0;

    for ( idx = 0; idx < MPE_COMM_KEY_MAX; idx++ ) {
        commptr->attrs[idx]  = NULL;
        commptr->is_attr_used[idx] = ZMPI_BOOL_FALSE;
    }

    return commptr;
}

static void ZMPI_Comm_free( ZMPI_Comm_t **comm_handle )
{
    ZMPI_Comm_t  *commptr;

    commptr = *comm_handle;
    if ( commptr != NULL )
        FREE( commptr );
    *comm_handle = NULL;
}

int PMPI_Init( int *argc, char ***argv )
{
    int  *extra_state = NULL; /* unused variable */
    int   idx;

    ZMPI_COMMSET = (ZMPI_CommSet_t *) MALLOC( sizeof(ZMPI_CommSet_t) );
    if ( ZMPI_COMMSET == NULL ) {
        fprintf( stderr, __FILE__":PMPI_Init() -- MALLOC fails.\n" );
        fflush( stderr );
        return MPI_ERR_NO_MEM;
    }

    if ( gethostname( ZMPI_COMMSET->name, MPI_MAX_PROCESSOR_NAME ) != 0 ) {
        /*
           Since gethostname() fails, write a NULL character at the end
           of the name[] array to make sure the subsequent strlen() succeed.
        */
        ZMPI_COMMSET->name[MPI_MAX_PROCESSOR_NAME-1] = '\0';
    }
    ZMPI_COMMSET->namelen = strlen( ZMPI_COMMSET->name );

    ZMPI_COMMSET->commptrs[MPI_COMM_WORLD] = ZMPI_Comm_create();
    if ( ZMPI_COMMSET->commptrs[MPI_COMM_WORLD] == NULL ) {
        fprintf( stderr, __FILE__":PMPI_Init() -- "
                         "ZMPI_Comm_create() for MPI_COMM_WORLD fails.\n" );
        fflush( stderr );
        PMPI_Abort( MPI_COMM_WORLD, 1 );
        return MPI_ERR_NO_MEM;
    }
    ZMPI_COMMSET->commptrs[MPI_COMM_SELF]  = ZMPI_Comm_create();
    if ( ZMPI_COMMSET->commptrs[MPI_COMM_SELF] == NULL ) {
        fprintf( stderr, __FILE__":PMPI_Init() -- "
                         "ZMPI_Comm_create() for MPI_COMM_SELF fails.\n" );
        fflush( stderr );
        PMPI_Abort( MPI_COMM_WORLD, 1 );
        return MPI_ERR_NO_MEM;
    }

    for ( idx = MPI_COMM_SELF+1; idx < MPE_COMM_MAX; idx++ )
        ZMPI_COMMSET->commptrs[idx] = NULL;

    for ( idx = 0; idx < MPE_COMM_MAX; idx++ )
        ZMPI_COMMSET->is_key_used[idx] = ZMPI_BOOL_FALSE;

    /*
       Create a key for MPI_WTIME_IS_GLOBAL and set it to true
       since there is only 1 process in this MPI implementation.
       This is done for CLOG_Util_is_MPIWtime_synchronized()
       which should return TRUE.
    */
    PMPI_Comm_create_keyval( MPI_COMM_NULL_COPY_FN,
                             MPI_COMM_NULL_DELETE_FN,
                             &MPI_WTIME_IS_GLOBAL, extra_state );
    MPI_WTIME_IS_GLOBAL_VALUE = CLOG_BOOL_TRUE;
    PMPI_Comm_set_attr( MPI_COMM_WORLD, MPI_WTIME_IS_GLOBAL,
                        &MPI_WTIME_IS_GLOBAL_VALUE );
    PMPI_Comm_set_attr( MPI_COMM_SELF, MPI_WTIME_IS_GLOBAL,
                        &MPI_WTIME_IS_GLOBAL_VALUE );

    return MPI_SUCCESS;
}

int PMPI_Finalize( void )
{
    int idx;

    if ( ZMPI_COMMSET != NULL ) {
        for ( idx = 0; idx < MPE_COMM_MAX; idx++ )
            ZMPI_Comm_free( &(ZMPI_COMMSET->commptrs[idx]) );
        FREE( ZMPI_COMMSET );
        ZMPI_COMMSET = NULL;
    }

    return MPI_SUCCESS;
}

int PMPI_Abort( MPI_Comm comm, int errorcode )
{
    fprintf( stdout, __FILE__":PMPI_Abort( MPI_Comm=%d, errorcode=%d) -- "
                     "Aborting...\n", comm, errorcode );
    ZMPI_PRINTSTACK();
    PMPI_Finalize();
    exit( 1 );
    return MPI_SUCCESS;
}

int PMPI_Initialized( int *flag )
{
    *flag = ( ZMPI_COMMSET != NULL );
    return MPI_SUCCESS;
}

int PMPI_Get_processor_name( char *name, int *resultlen )
{
    strncpy( name, ZMPI_COMMSET->name, MPI_MAX_PROCESSOR_NAME );
    *resultlen = strlen( name );
    return MPI_SUCCESS;
}

int PMPI_Comm_size( MPI_Comm comm, int *size )
{
    *size = ZMPI_COMMSET->commptrs[comm]->size;
    return MPI_SUCCESS;
}

int PMPI_Comm_rank( MPI_Comm comm, int *rank )
{
    *rank = ZMPI_COMMSET->commptrs[comm]->rank;
    return MPI_SUCCESS;
}

/*
    Assume gettimeofday() exists, maybe configure needs to check this function.
*/
#if defined( HAVE_SYS_TIME_H )
#include <sys/time.h>
#endif
double PMPI_Wtime( void )
{
    struct timeval  tval;
    gettimeofday( &tval, NULL );
    return ( (double) tval.tv_sec + (double) tval.tv_usec * 0.000001 );
}

int PMPI_Comm_create_keyval( MPI_Comm_copy_attr_function *comm_copy_attr_fn,
                             MPI_Comm_delete_attr_function *comm_delete_attr_fn,
                             int *comm_keyval, void *extra_state )
{
    int  avail_key;

    for ( avail_key = 0; avail_key < MPE_COMM_KEY_MAX; avail_key++ ) {
        if ( ZMPI_COMMSET->is_key_used[avail_key] == ZMPI_BOOL_FALSE )
            break;
    }

    if ( avail_key < MPE_COMM_KEY_MAX ) {
        ZMPI_COMMSET->is_key_used[avail_key] = ZMPI_BOOL_TRUE;
        *comm_keyval = avail_key;
        return MPI_SUCCESS;
    }
    else {
        fprintf( stderr, __FILE__":PMPI_Comm_create_keyval() -- "
                         "Exceeding internal keyval[] size.\n" );
        fflush( stderr );
        PMPI_Abort( MPI_COMM_WORLD, 1 );
        return MPI_ERR_KEYVAL;
    }
}

int PMPI_Comm_free_keyval( int *comm_keyval )
{
    if ( *comm_keyval >= 0 && *comm_keyval < MPE_COMM_KEY_MAX ) {
        ZMPI_COMMSET->is_key_used[ *comm_keyval ] = ZMPI_BOOL_FALSE;
        *comm_keyval = MPI_KEYVAL_INVALID;
        return MPI_SUCCESS;
    }
    else {
        fprintf( stderr, __FILE__":PMPI_Comm_free_keyval() -- "
                         "Invalid comm_keyval, %d.\n", *comm_keyval );
        fflush( stderr );
        PMPI_Abort( MPI_COMM_WORLD, 1 );
        return MPI_ERR_KEYVAL;
    }
}

int PMPI_Comm_set_attr( MPI_Comm comm, int comm_keyval,
                        void *attribute_val )
{
    if ( comm_keyval >= 0 && comm_keyval < MPE_COMM_KEY_MAX ) {
        ZMPI_COMMSET->commptrs[comm]->attrs[comm_keyval] = attribute_val;
        return MPI_SUCCESS;
    }
    else {
        fprintf( stderr, __FILE__":PMPI_Comm_set_attr(MPI_Comm=%d) -- "
                         "Invalid comm_keyval, %d.\n", comm, comm_keyval );
        fflush( stderr );
        PMPI_Abort( comm, 1 );
        return MPI_ERR_KEYVAL;
    }
}

int PMPI_Comm_get_attr( MPI_Comm comm, int comm_keyval,
                        void *attribute_val, int *flag )
{
    if ( comm_keyval >= 0 && comm_keyval < MPE_COMM_KEY_MAX ) {
        (*(void **)attribute_val)
        = ZMPI_COMMSET->commptrs[comm]->attrs[comm_keyval];
        *flag = ZMPI_BOOL_TRUE;
    }
    else {
        *flag = ZMPI_BOOL_FALSE;
        fprintf( stderr, __FILE__":PMPI_Comm_get_attr(MPI_Comm=%d) -- "
                         "Invalid comm_keyval, %d.\n", comm, comm_keyval );
        fflush( stderr );
    }
    return MPI_SUCCESS;
}

int PMPI_Comm_test_inter( MPI_Comm comm, int *flag )
{
    *flag = ZMPI_BOOL_FALSE;
    return MPI_SUCCESS;
}

int PMPI_Ssend( void *buf, int count, MPI_Datatype datatype, int dest,
                int tag, MPI_Comm comm )
{
    fprintf( stderr, __FILE__":PMPI_Ssend() should not be invoked!" );
    ZMPI_PRINTSTACK();
    return MPI_SUCCESS;
}

int PMPI_Send( void *buf, int count, MPI_Datatype datatype, int dest,
               int tag, MPI_Comm comm )
{
    fprintf( stderr, __FILE__":PMPI_Send() should not be invoked!" );
    ZMPI_PRINTSTACK();
    return MPI_SUCCESS;
}

int PMPI_Recv( void *buf, int count, MPI_Datatype datatype, int source,
               int tag, MPI_Comm comm, MPI_Status *status )
{
    fprintf( stderr, __FILE__":PMPI_Recv() should not be invoked!" );
    ZMPI_PRINTSTACK();
    return MPI_SUCCESS;
}

int PMPI_Irecv( void *buf, int count, MPI_Datatype datatype, int source,
                int tag, MPI_Comm comm, MPI_Request *request )
{
    fprintf( stderr, __FILE__":PMPI_Irecv() should not be invoked!" );
    ZMPI_PRINTSTACK();
    return MPI_SUCCESS;
}

int PMPI_Wait( MPI_Request *request, MPI_Status *status )
{
    fprintf( stderr, __FILE__":PMPI_Wait() should not be invoked!" );
    ZMPI_PRINTSTACK();
    return MPI_SUCCESS;
}

int PMPI_Get_count( MPI_Status *status,  MPI_Datatype datatype, int *count )
{
    if ( status != NULL ) {
        if ((status->count % datatype) != 0)
            (*count) = MPI_UNDEFINED;
        else
            (*count) = status->count / datatype;
    }
    else {
        *count = MPI_UNDEFINED;
    }
    return MPI_SUCCESS;
}

int PMPI_Barrier( MPI_Comm comm )
{
    return MPI_SUCCESS;
}

int PMPI_Bcast( void *buffer, int count, MPI_Datatype datatype,
                int root, MPI_Comm comm )
{
    return MPI_SUCCESS;
}

int PMPI_Scatter( void *sendbuf, int sendcnt, MPI_Datatype sendtype,
                  void *recvbuf, int recvcnt, MPI_Datatype recvtype,
                  int root, MPI_Comm comm )
{
    if ( sendbuf != recvbuf )
        memcpy( recvbuf, sendbuf, sendcnt*sendtype );
    return MPI_SUCCESS;
}

int PMPI_Gather( void *sendbuf, int sendcnt, MPI_Datatype sendtype,
                 void *recvbuf, int recvcnt, MPI_Datatype recvtype,
                 int root, MPI_Comm comm )
{
    if ( sendbuf != recvbuf )
        memcpy( recvbuf, sendbuf, sendcnt*sendtype );
    return MPI_SUCCESS;
}

int PMPI_Scan( void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
               MPI_Op op, MPI_Comm comm )
{
    if ( sendbuf != recvbuf )
        memcpy( recvbuf, sendbuf, count*datatype );
    return MPI_SUCCESS;
}

int PMPI_Allreduce( void *sendbuf, void *recvbuf, int count,
                    MPI_Datatype datatype, MPI_Op op, MPI_Comm comm )
{
    if ( sendbuf != recvbuf )
        memcpy( recvbuf, sendbuf, count*datatype );
    return MPI_SUCCESS;
}

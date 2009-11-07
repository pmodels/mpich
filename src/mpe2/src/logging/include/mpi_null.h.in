/*
   (C) 2007 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
#if !defined( _MPI_NULL )
#define _MPI_NULL
/*
   A Serial MPI implementation for non-MPI program.
*/
/* Keep C++ compilers from getting confused */
#if defined(__cplusplus)
extern "C" {
#endif

#if !defined( _MPI_NULL_MPI_COMM )
#define _MPI_NULL_MPI_COMM
typedef int  MPI_Comm;
#endif
#define MPI_COMM_NULL  ((MPI_Comm)-1)
#define MPI_COMM_WORLD ((MPI_Comm)0)
#define MPI_COMM_SELF  ((MPI_Comm)1)

typedef int (MPI_Comm_copy_attr_function)(MPI_Comm, int, void *, void *,
                                          void *, int *);
#define MPI_COMM_NULL_COPY_FN ((MPI_Comm_copy_attr_function*)0)

typedef int (MPI_Comm_delete_attr_function)(MPI_Comm, int, void *, void *);
#define MPI_COMM_NULL_DELETE_FN ((MPI_Comm_delete_attr_function*)0)

/* A convenient trick to account for total message size in send/recv */
typedef int MPI_Datatype;
#define MPI_CHAR           ((MPI_Datatype)sizeof(char))
#define MPI_BYTE           ((MPI_Datatype)sizeof(char))
#define MPI_INT            ((MPI_Datatype)sizeof(int))
#define MPI_DOUBLE         ((MPI_Datatype)sizeof(double))

typedef struct MPI_Status {
    int count;
    int cancelled;
    int MPI_SOURCE;
    int MPI_TAG;
    int MPI_ERROR;
} MPI_Status;

typedef int MPI_Request;

typedef int MPI_Op;
#define MPI_MAX     (MPI_Op)(0x58000001)
#define MPI_MIN     (MPI_Op)(0x58000002)
#define MPI_SUM     (MPI_Op)(0x58000003)

extern int MPI_WTIME_IS_GLOBAL;
/* Pre-defined constants */
#define MPI_UNDEFINED      (-32766)
#define MPI_KEYVAL_INVALID 0x24000000
#define MPI_MAX_PROCESSOR_NAME 128

#define MPI_PROC_NULL   (-1)
#define MPI_ANY_SOURCE  (-2)
#define MPI_ROOT        (-3)
#define MPI_ANY_TAG     (-1)

/* MPI's error classes */
#define MPI_SUCCESS          0      /* Successful return code */
#define MPI_ERR_COMM         5      /* Invalid communicator */
#define MPI_ERR_INTERN      16      /* Internal error code    */
#define MPI_ERR_NO_MEM      34
#define MPI_ERR_KEYVAL      48      /* Erroneous attribute key */

int PMPI_Init( int *argc, char ***argv );

int PMPI_Finalize( void );

int PMPI_Abort( MPI_Comm comm, int errorcode );

int PMPI_Initialized( int *flag );

int PMPI_Get_processor_name( char *name, int *resultlen );

int PMPI_Comm_size( MPI_Comm comm, int *size );

int PMPI_Comm_rank( MPI_Comm comm, int *rank );

double PMPI_Wtime( void );

int PMPI_Comm_create_keyval( MPI_Comm_copy_attr_function *comm_copy_attr_fn,
                             MPI_Comm_delete_attr_function *comm_delete_attr_fn,
                             int *comm_keyval, void *extra_state );

int PMPI_Comm_free_keyval( int *comm_keyval );

int PMPI_Comm_set_attr( MPI_Comm comm, int comm_keyval,
                        void *attribute_val );

int PMPI_Comm_get_attr( MPI_Comm comm, int comm_keyval,
                        void *attribute_val, int *flag );

int PMPI_Comm_test_inter( MPI_Comm comm, int *flag );

int PMPI_Ssend( void *buf, int count, MPI_Datatype datatype, int dest,
                int tag, MPI_Comm comm );

int PMPI_Send( void *buf, int count, MPI_Datatype datatype, int dest,
               int tag, MPI_Comm comm );

int PMPI_Recv( void *buf, int count, MPI_Datatype datatype, int source,
               int tag, MPI_Comm comm, MPI_Status *status );

int PMPI_Irecv( void *buf, int count, MPI_Datatype datatype, int source,
                int tag, MPI_Comm comm, MPI_Request *request );

int PMPI_Wait( MPI_Request *request, MPI_Status *status );

int PMPI_Get_count( MPI_Status *status,  MPI_Datatype datatype, int *count );

int PMPI_Barrier( MPI_Comm comm );

int PMPI_Bcast( void *buffer, int count, MPI_Datatype datatype,
                int root, MPI_Comm comm );

int PMPI_Scan( void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
               MPI_Op op, MPI_Comm comm );

int PMPI_Scatter( void *sendbuf, int sendcnt, MPI_Datatype sendtype,
                  void *recvbuf, int recvcnt, MPI_Datatype recvtype,
                  int root, MPI_Comm comm );

int PMPI_Gather( void *sendbuf, int sendcnt, MPI_Datatype sendtype,
                 void *recvbuf, int recvcnt, MPI_Datatype recvtype,
                 int root, MPI_Comm comm );

int PMPI_Allreduce( void *sendbuf, void *recvbuf, int count,
                    MPI_Datatype datatype, MPI_Op op, MPI_Comm comm );

#if defined(__cplusplus)
}
#endif

#endif /* of _MPI_NULL */

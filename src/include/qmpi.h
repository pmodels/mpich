/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef QMPI_H
#define QMPI_H

enum QMPI_Functions_enum {
    MPI_ABORT_T,
    MPI_ACCUMULATE_T,
    MPI_ADD_ERROR_CLASS_T,
    MPI_ADD_ERROR_CODE_T,
    MPI_ADD_ERROR_STRING_T,
    MPI_AINT_ADD_T,
    MPI_AINT_DIFF_T,
    MPI_ALLGATHER_T,
    MPI_ALLGATHERV_T,
    MPI_ALLOC_MEM_T,
    MPI_ALLREDUCE_T,
    MPI_ALLTOALL_T,
    MPI_ALLTOALLV_T,
    MPI_ALLTOALLW_T,
    MPI_ATTR_DELETE_T,
    MPI_ATTR_GET_T,
    MPI_ATTR_PUT_T,
    MPI_BARRIER_T,
    MPI_BCAST_T,
    MPI_BSEND_T,
    MPI_BSEND_INIT_T,
    MPI_BUFFER_ATTACH_T,
    MPI_BUFFER_DETACH_T,
    MPI_CANCEL_T,
    MPI_CART_COORDS_T,
    MPI_CART_CREATE_T,
    MPI_CART_GET_T,
    MPI_CART_MAP_T,
    MPI_CART_RANK_T,
    MPI_CART_SHIFT_T,
    MPI_CART_SUB_T,
    MPI_CARTDIM_GET_T,
    MPI_CLOSE_PORT_T,
    MPI_COMM_ACCEPT_T,
    MPI_COMM_CALL_ERRHANDLER_T,
    MPI_COMM_COMPARE_T,
    MPI_COMM_CONNECT_T,
    MPI_COMM_CREATE_T,
    MPI_COMM_CREATE_ERRHANDLER_T,
    MPI_COMM_CREATE_GROUP_T,
    MPI_COMM_CREATE_KEYVAL_T,
    MPI_COMM_DELETE_ATTR_T,
    MPI_COMM_DISCONNECT_T,
    MPI_COMM_DUP_T,
    MPI_COMM_DUP_WITH_INFO_T,
    MPI_COMM_FREE_T,
    MPI_COMM_FREE_KEYVAL_T,
    MPI_COMM_GET_ATTR_T,
    MPI_COMM_GET_ERRHANDLER_T,
    MPI_COMM_GET_INFO_T,
    MPI_COMM_GET_NAME_T,
    MPI_COMM_GET_PARENT_T,
    MPI_COMM_GROUP_T,
    MPI_COMM_IDUP_T,
    MPI_COMM_JOIN_T,
    MPI_COMM_RANK_T,
    MPI_COMM_REMOTE_GROUP_T,
    MPI_COMM_REMOTE_SIZE_T,
    MPI_COMM_SET_ATTR_T,
    MPI_COMM_SET_ERRHANDLER_T,
    MPI_COMM_SET_INFO_T,
    MPI_COMM_SET_NAME_T,
    MPI_COMM_SIZE_T,
    MPI_COMM_SPAWN_T,
    MPI_COMM_SPAWN_MULTIPLE_T,
    MPI_COMM_SPLIT_T,
    MPI_COMM_SPLIT_TYPE_T,
    MPI_COMM_TEST_INTER_T,
    MPI_COMPARE_AND_SWAP_T,
    MPI_DIMS_CREATE_T,
    MPI_DIST_GRAPH_CREATE_T,
    MPI_DIST_GRAPH_CREATE_ADJACENT_T,
    MPI_DIST_GRAPH_NEIGHBORS_T,
    MPI_DIST_GRAPH_NEIGHBORS_COUNT_T,
    MPI_ERRHANDLER_FREE_T,
    MPI_ERROR_CLASS_T,
    MPI_ERROR_STRING_T,
    MPI_EXSCAN_T,
    MPI_FETCH_AND_OP_T,
    MPI_FILE_C2F_T,
    MPI_FILE_CALL_ERRHANDLER_T,
    MPI_FILE_CLOSE_T,
    MPI_FILE_CREATE_ERRHANDLER_T,
    MPI_FILE_DELETE_T,
    MPI_FILE_F2C_T,
    MPI_FILE_GET_AMODE_T,
    MPI_FILE_GET_ATOMICITY_T,
    MPI_FILE_GET_BYTE_OFFSET_T,
    MPI_FILE_GET_ERRHANDLER_T,
    MPI_FILE_GET_GROUP_T,
    MPI_FILE_GET_INFO_T,
    MPI_FILE_GET_POSITION_T,
    MPI_FILE_GET_POSITION_SHARED_T,
    MPI_FILE_GET_SIZE_T,
    MPI_FILE_GET_TYPE_EXTENT_T,
    MPI_FILE_GET_VIEW_T,
    MPI_FILE_IREAD_T,
    MPI_FILE_IREAD_ALL_T,
    MPI_FILE_IREAD_AT_T,
    MPI_FILE_IREAD_AT_ALL_T,
    MPI_FILE_IREAD_SHARED_T,
    MPI_FILE_IWRITE_T,
    MPI_FILE_IWRITE_ALL_T,
    MPI_FILE_IWRITE_AT_T,
    MPI_FILE_IWRITE_AT_ALL_T,
    MPI_FILE_IWRITE_SHARED_T,
    MPI_FILE_OPEN_T,
    MPI_FILE_PREALLOCATE_T,
    MPI_FILE_READ_T,
    MPI_FILE_READ_ALL_T,
    MPI_FILE_READ_ALL_BEGIN_T,
    MPI_FILE_READ_ALL_END_T,
    MPI_FILE_READ_AT_T,
    MPI_FILE_READ_AT_ALL_T,
    MPI_FILE_READ_AT_ALL_BEGIN_T,
    MPI_FILE_READ_AT_ALL_END_T,
    MPI_FILE_READ_ORDERED_T,
    MPI_FILE_READ_ORDERED_BEGIN_T,
    MPI_FILE_READ_ORDERED_END_T,
    MPI_FILE_READ_SHARED_T,
    MPI_FILE_SEEK_T,
    MPI_FILE_SEEK_SHARED_T,
    MPI_FILE_SET_ATOMICITY_T,
    MPI_FILE_SET_ERRHANDLER_T,
    MPI_FILE_SET_INFO_T,
    MPI_FILE_SET_SIZE_T,
    MPI_FILE_SET_VIEW_T,
    MPI_FILE_SYNC_T,
    MPI_FILE_WRITE_T,
    MPI_FILE_WRITE_ALL_T,
    MPI_FILE_WRITE_ALL_BEGIN_T,
    MPI_FILE_WRITE_ALL_END_T,
    MPI_FILE_WRITE_AT_T,
    MPI_FILE_WRITE_AT_ALL_T,
    MPI_FILE_WRITE_AT_ALL_BEGIN_T,
    MPI_FILE_WRITE_AT_ALL_END_T,
    MPI_FILE_WRITE_ORDERED_T,
    MPI_FILE_WRITE_ORDERED_BEGIN_T,
    MPI_FILE_WRITE_ORDERED_END_T,
    MPI_FILE_WRITE_SHARED_T,
    MPI_FINALIZE_T,
    MPI_FINALIZED_T,
    MPI_FREE_MEM_T,
    MPI_GATHER_T,
    MPI_GATHERV_T,
    MPI_GET_T,
    MPI_GET_ACCUMULATE_T,
    MPI_GET_ADDRESS_T,
    MPI_GET_COUNT_T,
    MPI_GET_ELEMENTS_T,
    MPI_GET_ELEMENTS_X_T,
    MPI_GET_LIBRARY_VERSION_T,
    MPI_GET_PROCESSOR_NAME_T,
    MPI_GET_VERSION_T,
    MPI_GRAPH_CREATE_T,
    MPI_GRAPH_GET_T,
    MPI_GRAPH_MAP_T,
    MPI_GRAPH_NEIGHBORS_T,
    MPI_GRAPH_NEIGHBORS_COUNT_T,
    MPI_GRAPHDIMS_GET_T,
    MPI_GREQUEST_COMPLETE_T,
    MPI_GREQUEST_START_T,
    MPI_GROUP_COMPARE_T,
    MPI_GROUP_DIFFERENCE_T,
    MPI_GROUP_EXCL_T,
    MPI_GROUP_FREE_T,
    MPI_GROUP_INCL_T,
    MPI_GROUP_INTERSECTION_T,
    MPI_GROUP_RANGE_EXCL_T,
    MPI_GROUP_RANGE_INCL_T,
    MPI_GROUP_RANK_T,
    MPI_GROUP_SIZE_T,
    MPI_GROUP_TRANSLATE_RANKS_T,
    MPI_GROUP_UNION_T,
    MPI_IALLGATHER_T,
    MPI_IALLGATHERV_T,
    MPI_IALLREDUCE_T,
    MPI_IALLTOALL_T,
    MPI_IALLTOALLV_T,
    MPI_IALLTOALLW_T,
    MPI_IBARRIER_T,
    MPI_IBCAST_T,
    MPI_IBSEND_T,
    MPI_IEXSCAN_T,
    MPI_IGATHER_T,
    MPI_IGATHERV_T,
    MPI_IMPROBE_T,
    MPI_IMRECV_T,
    MPI_INEIGHBOR_ALLGATHER_T,
    MPI_INEIGHBOR_ALLGATHERV_T,
    MPI_INEIGHBOR_ALLTOALL_T,
    MPI_INEIGHBOR_ALLTOALLV_T,
    MPI_INEIGHBOR_ALLTOALLW_T,
    MPI_INFO_CREATE_T,
    MPI_INFO_DELETE_T,
    MPI_INFO_DUP_T,
    MPI_INFO_FREE_T,
    MPI_INFO_GET_T,
    MPI_INFO_GET_NKEYS_T,
    MPI_INFO_GET_NTHKEY_T,
    MPI_INFO_GET_VALUELEN_T,
    MPI_INFO_SET_T,
    MPI_INIT_T,
    MPI_INIT_THREAD_T,
    MPI_INITIALIZED_T,
    MPI_INTERCOMM_CREATE_T,
    MPI_INTERCOMM_MERGE_T,
    MPI_IPROBE_T,
    MPI_IRECV_T,
    MPI_IREDUCE_T,
    MPI_IREDUCE_SCATTER_T,
    MPI_IREDUCE_SCATTER_BLOCK_T,
    MPI_IRSEND_T,
    MPI_IS_THREAD_MAIN_T,
    MPI_ISCAN_T,
    MPI_ISCATTER_T,
    MPI_ISCATTERV_T,
    MPI_ISEND_T,
    MPI_ISSEND_T,
    MPI_KEYVAL_CREATE_T,
    MPI_KEYVAL_FREE_T,
    MPI_LOOKUP_NAME_T,
    MPI_MPROBE_T,
    MPI_MRECV_T,
    MPI_NEIGHBOR_ALLGATHER_T,
    MPI_NEIGHBOR_ALLGATHERV_T,
    MPI_NEIGHBOR_ALLTOALL_T,
    MPI_NEIGHBOR_ALLTOALLV_T,
    MPI_NEIGHBOR_ALLTOALLW_T,
    MPI_OP_COMMUTATIVE_T,
    MPI_OP_CREATE_T,
    MPI_OP_FREE_T,
    MPI_OPEN_PORT_T,
    MPI_PACK_T,
    MPI_PACK_EXTERNAL_T,
    MPI_PACK_EXTERNAL_SIZE_T,
    MPI_PACK_SIZE_T,
    MPI_PCONTROL_T,
    MPI_PROBE_T,
    MPI_PUBLISH_NAME_T,
    MPI_PUT_T,
    MPI_QUERY_THREAD_T,
    MPI_RACCUMULATE_T,
    MPI_RECV_T,
    MPI_RECV_INIT_T,
    MPI_REDUCE_T,
    MPI_REDUCE_LOCAL_T,
    MPI_REDUCE_SCATTER_T,
    MPI_REDUCE_SCATTER_BLOCK_T,
    MPI_REQUEST_FREE_T,
    MPI_REQUEST_GET_STATUS_T,
    MPI_RGET_T,
    MPI_RGET_ACCUMULATE_T,
    MPI_RPUT_T,
    MPI_RSEND_T,
    MPI_RSEND_INIT_T,
    MPI_SCAN_T,
    MPI_SCATTER_T,
    MPI_SCATTERV_T,
    MPI_SEND_T,
    MPI_SEND_INIT_T,
    MPI_SENDRECV_T,
    MPI_SENDRECV_REPLACE_T,
    MPI_SSEND_T,
    MPI_SSEND_INIT_T,
    MPI_START_T,
    MPI_STARTALL_T,
    MPI_STATUS_SET_CANCELLED_T,
    MPI_STATUS_SET_ELEMENTS_T,
    MPI_STATUS_SET_ELEMENTS_X_T,
    MPI_T_CATEGORY_CHANGED_T,
    MPI_T_CATEGORY_GET_CATEGORIES_T,
    MPI_T_CATEGORY_GET_CVARS_T,
    MPI_T_CATEGORY_GET_INDEX_T,
    MPI_T_CATEGORY_GET_INFO_T,
    MPI_T_CATEGORY_GET_NUM_T,
    MPI_T_CATEGORY_GET_PVARS_T,
    MPI_T_CVAR_GET_INDEX_T,
    MPI_T_CVAR_GET_INFO_T,
    MPI_T_CVAR_GET_NUM_T,
    MPI_T_CVAR_HANDLE_ALLOC_T,
    MPI_T_CVAR_HANDLE_FREE_T,
    MPI_T_CVAR_READ_T,
    MPI_T_CVAR_WRITE_T,
    MPI_T_ENUM_GET_INFO_T,
    MPI_T_ENUM_GET_ITEM_T,
    MPI_T_FINALIZE_T,
    MPI_T_INIT_THREAD_T,
    MPI_T_PVAR_GET_INDEX_T,
    MPI_T_PVAR_GET_INFO_T,
    MPI_T_PVAR_GET_NUM_T,
    MPI_T_PVAR_HANDLE_ALLOC_T,
    MPI_T_PVAR_HANDLE_FREE_T,
    MPI_T_PVAR_READ_T,
    MPI_T_PVAR_READRESET_T,
    MPI_T_PVAR_RESET_T,
    MPI_T_PVAR_SESSION_CREATE_T,
    MPI_T_PVAR_SESSION_FREE_T,
    MPI_T_PVAR_START_T,
    MPI_T_PVAR_STOP_T,
    MPI_T_PVAR_WRITE_T,
    MPI_TEST_T,
    MPI_TEST_CANCELLED_T,
    MPI_TESTALL_T,
    MPI_TESTANY_T,
    MPI_TESTSOME_T,
    MPI_TOPO_TEST_T,
    MPI_TYPE_COMMIT_T,
    MPI_TYPE_CONTIGUOUS_T,
    MPI_TYPE_CREATE_DARRAY_T,
    MPI_TYPE_CREATE_F90_COMPLEX_T,
    MPI_TYPE_CREATE_F90_INTEGER_T,
    MPI_TYPE_CREATE_F90_REAL_T,
    MPI_TYPE_CREATE_HINDEXED_T,
    MPI_TYPE_CREATE_HINDEXED_BLOCK_T,
    MPI_TYPE_CREATE_HVECTOR_T,
    MPI_TYPE_CREATE_INDEXED_BLOCK_T,
    MPI_TYPE_CREATE_KEYVAL_T,
    MPI_TYPE_CREATE_RESIZED_T,
    MPI_TYPE_CREATE_STRUCT_T,
    MPI_TYPE_CREATE_SUBARRAY_T,
    MPI_TYPE_DELETE_ATTR_T,
    MPI_TYPE_DUP_T,
    MPI_TYPE_FREE_T,
    MPI_TYPE_FREE_KEYVAL_T,
    MPI_TYPE_GET_ATTR_T,
    MPI_TYPE_GET_CONTENTS_T,
    MPI_TYPE_GET_ENVELOPE_T,
    MPI_TYPE_GET_EXTENT_T,
    MPI_TYPE_GET_EXTENT_X_T,
    MPI_TYPE_GET_NAME_T,
    MPI_TYPE_GET_TRUE_EXTENT_T,
    MPI_TYPE_GET_TRUE_EXTENT_X_T,
    MPI_TYPE_INDEXED_T,
    MPI_TYPE_MATCH_SIZE_T,
    MPI_TYPE_SET_ATTR_T,
    MPI_TYPE_SET_NAME_T,
    MPI_TYPE_SIZE_T,
    MPI_TYPE_SIZE_X_T,
    MPI_TYPE_VECTOR_T,
    MPI_UNPACK_T,
    MPI_UNPACK_EXTERNAL_T,
    MPI_UNPUBLISH_NAME_T,
    MPI_WAIT_T,
    MPI_WAITALL_T,
    MPI_WAITANY_T,
    MPI_WAITSOME_T,
    MPI_WIN_ALLOCATE_T,
    MPI_WIN_ALLOCATE_SHARED_T,
    MPI_WIN_ATTACH_T,
    MPI_WIN_CALL_ERRHANDLER_T,
    MPI_WIN_COMPLETE_T,
    MPI_WIN_CREATE_T,
    MPI_WIN_CREATE_DYNAMIC_T,
    MPI_WIN_CREATE_ERRHANDLER_T,
    MPI_WIN_CREATE_KEYVAL_T,
    MPI_WIN_DELETE_ATTR_T,
    MPI_WIN_DETACH_T,
    MPI_WIN_FENCE_T,
    MPI_WIN_FLUSH_T,
    MPI_WIN_FLUSH_ALL_T,
    MPI_WIN_FLUSH_LOCAL_T,
    MPI_WIN_FLUSH_LOCAL_ALL_T,
    MPI_WIN_FREE_T,
    MPI_WIN_FREE_KEYVAL_T,
    MPI_WIN_GET_ATTR_T,
    MPI_WIN_GET_ERRHANDLER_T,
    MPI_WIN_GET_GROUP_T,
    MPI_WIN_GET_INFO_T,
    MPI_WIN_GET_NAME_T,
    MPI_WIN_LOCK_T,
    MPI_WIN_LOCK_ALL_T,
    MPI_WIN_POST_T,
    MPI_WIN_SET_ATTR_T,
    MPI_WIN_SET_ERRHANDLER_T,
    MPI_WIN_SET_INFO_T,
    MPI_WIN_SET_NAME_T,
    MPI_WIN_SHARED_QUERY_T,
    MPI_WIN_START_T,
    MPI_WIN_SYNC_T,
    MPI_WIN_TEST_T,
    MPI_WIN_UNLOCK_T,
    MPI_WIN_UNLOCK_ALL_T,
    MPI_WIN_WAIT_T,
    MPI_WTICK_T,
    MPI_WTIME_T,
    MPI_LAST_FUNC_T
};

int QMPI_Abort(void *context, MPI_Comm comm, int errorcode) MPICH_API_PUBLIC;
typedef int (QMPI_Abort_t) (void *context, MPI_Comm comm, int errorcode);

int QMPI_Accumulate(void *context, const void *origin_addr, int origin_count,
                    MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp,
                    int target_count, MPI_Datatype target_datatype, MPI_Op op,
                    MPI_Win win) MPICH_API_PUBLIC;
typedef int (QMPI_Accumulate_t) (void *context, const void *origin_addr, int origin_count,
                                 MPI_Datatype origin_datatype, int target_rank,
                                 MPI_Aint target_disp, int target_count,
                                 MPI_Datatype target_datatype, MPI_Op op, MPI_Win win);

int QMPI_Add_error_class(void *context, int *errorclass) MPICH_API_PUBLIC;
typedef int (QMPI_Add_error_class_t) (void *context, int *errorclass);

int QMPI_Add_error_code(void *context, int errorclass, int *errorcode) MPICH_API_PUBLIC;
typedef int (QMPI_Add_error_code_t) (void *context, int errorclass, int *errorcode);

int QMPI_Add_error_string(void *context, int errorcode, const char *string) MPICH_API_PUBLIC;
typedef int (QMPI_Add_error_string_t) (void *context, int errorcode, const char *string);

MPI_Aint QMPI_Aint_add(void *context, MPI_Aint base, MPI_Aint disp) MPICH_API_PUBLIC;
typedef MPI_Aint(QMPI_Aint_add_t) (void *context, MPI_Aint base, MPI_Aint disp);

MPI_Aint QMPI_Aint_diff(void *context, MPI_Aint addr1, MPI_Aint addr2) MPICH_API_PUBLIC;
typedef MPI_Aint(QMPI_Aint_diff_t) (void *context, MPI_Aint addr1, MPI_Aint addr2);

int QMPI_Allgather(void *context, const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                   void *recvbuf, int recvcount, MPI_Datatype recvtype,
                   MPI_Comm comm) MPICH_API_PUBLIC;
typedef int (QMPI_Allgather_t) (void *context, const void *sendbuf, int sendcount,
                                MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                MPI_Datatype recvtype, MPI_Comm comm);

int QMPI_Allgatherv(void *context, const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                    void *recvbuf, const int recvcounts[], const int displs[],
                    MPI_Datatype recvtype, MPI_Comm comm) MPICH_API_PUBLIC;
typedef int (QMPI_Allgatherv_t) (void *context, const void *sendbuf, int sendcount,
                                 MPI_Datatype sendtype, void *recvbuf, const int recvcounts[],
                                 const int displs[], MPI_Datatype recvtype, MPI_Comm comm);

int QMPI_Alloc_mem(void *context, MPI_Aint size, MPI_Info info, void *baseptr) MPICH_API_PUBLIC;
typedef int (QMPI_Alloc_mem_t) (void *context, MPI_Aint size, MPI_Info info, void *baseptr);

int QMPI_Allreduce(void *context, const void *sendbuf, void *recvbuf, int count,
                   MPI_Datatype datatype, MPI_Op op, MPI_Comm comm) MPICH_API_PUBLIC;
typedef int (QMPI_Allreduce_t) (void *context, const void *sendbuf, void *recvbuf, int count,
                                MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);

int QMPI_Alltoall(void *context, const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                  void *recvbuf, int recvcount, MPI_Datatype recvtype,
                  MPI_Comm comm) MPICH_API_PUBLIC;
typedef int (QMPI_Alltoall_t) (void *context, const void *sendbuf, int sendcount,
                               MPI_Datatype sendtype, void *recvbuf, int recvcount,
                               MPI_Datatype recvtype, MPI_Comm comm);

int QMPI_Alltoallv(void *context, const void *sendbuf, const int sendcounts[], const int sdispls[],
                   MPI_Datatype sendtype, void *recvbuf, const int recvcounts[],
                   const int rdispls[], MPI_Datatype recvtype, MPI_Comm comm) MPICH_API_PUBLIC;
typedef int (QMPI_Alltoallv_t) (void *context, const void *sendbuf, const int sendcounts[],
                                const int sdispls[], MPI_Datatype sendtype, void *recvbuf,
                                const int recvcounts[], const int rdispls[], MPI_Datatype recvtype,
                                MPI_Comm comm);

int QMPI_Alltoallw(void *context, const void *sendbuf, const int sendcounts[], const int sdispls[],
                   const MPI_Datatype sendtypes[], void *recvbuf, const int recvcounts[],
                   const int rdispls[], const MPI_Datatype recvtypes[],
                   MPI_Comm comm) MPICH_API_PUBLIC;
typedef int (QMPI_Alltoallw_t) (void *context, const void *sendbuf, const int sendcounts[],
                                const int sdispls[], const MPI_Datatype sendtypes[], void *recvbuf,
                                const int recvcounts[], const int rdispls[],
                                const MPI_Datatype recvtypes[], MPI_Comm comm);

int QMPI_Attr_delete(void *context, MPI_Comm comm, int keyval) MPICH_API_PUBLIC;
typedef int (QMPI_Attr_delete_t) (void *context, MPI_Comm comm, int keyval);

int QMPI_Attr_get(void *context, MPI_Comm comm, int keyval, void *attribute_val,
                  int *flag) MPICH_API_PUBLIC;
typedef int (QMPI_Attr_get_t) (void *context, MPI_Comm comm, int keyval, void *attribute_val,
                               int *flag);

int QMPI_Attr_put(void *context, MPI_Comm comm, int keyval, void *attribute_val) MPICH_API_PUBLIC;
typedef int (QMPI_Attr_put_t) (void *context, MPI_Comm comm, int keyval, void *attribute_val);

int QMPI_Barrier(void *context, MPI_Comm comm) MPICH_API_PUBLIC;
typedef int (QMPI_Barrier_t) (void *context, MPI_Comm comm);

int QMPI_Bcast(void *context, void *buffer, int count, MPI_Datatype datatype, int root,
               MPI_Comm comm) MPICH_API_PUBLIC;
typedef int (QMPI_Bcast_t) (void *context, void *buffer, int count, MPI_Datatype datatype, int root,
                            MPI_Comm comm);

int QMPI_Bsend(void *context, const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
               MPI_Comm comm) MPICH_API_PUBLIC;
typedef int (QMPI_Bsend_t) (void *context, const void *buf, int count, MPI_Datatype datatype,
                            int dest, int tag, MPI_Comm comm);

int QMPI_Bsend_init(void *context, const void *buf, int count, MPI_Datatype datatype, int dest,
                    int tag, MPI_Comm comm, MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_Bsend_init_t) (void *context, const void *buf, int count, MPI_Datatype datatype,
                                 int dest, int tag, MPI_Comm comm, MPI_Request * request);

int QMPI_Buffer_attach(void *context, void *buffer, int size) MPICH_API_PUBLIC;
typedef int (QMPI_Buffer_attach_t) (void *context, void *buffer, int size);

int QMPI_Buffer_detach(void *context, void *buffer_addr, int *size) MPICH_API_PUBLIC;
typedef int (QMPI_Buffer_detach_t) (void *context, void *buffer_addr, int *size);

int QMPI_Cancel(void *context, MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_Cancel_t) (void *context, MPI_Request * request);

int QMPI_Cart_coords(void *context, MPI_Comm comm, int rank, int maxdims,
                     int coords[]) MPICH_API_PUBLIC;
typedef int (QMPI_Cart_coords_t) (void *context, MPI_Comm comm, int rank, int maxdims,
                                  int coords[]);

int QMPI_Cart_create(void *context, MPI_Comm comm_old, int ndims, const int dims[],
                     const int periods[], int reorder, MPI_Comm * comm_cart) MPICH_API_PUBLIC;
typedef int (QMPI_Cart_create_t) (void *context, MPI_Comm comm_old, int ndims, const int dims[],
                                  const int periods[], int reorder, MPI_Comm * comm_cart);

int QMPI_Cart_get(void *context, MPI_Comm comm, int maxdims, int dims[], int periods[],
                  int coords[]) MPICH_API_PUBLIC;
typedef int (QMPI_Cart_get_t) (void *context, MPI_Comm comm, int maxdims, int dims[], int periods[],
                               int coords[]);

int QMPI_Cart_map(void *context, MPI_Comm comm, int ndims, const int dims[], const int periods[],
                  int *newrank) MPICH_API_PUBLIC;
typedef int (QMPI_Cart_map_t) (void *context, MPI_Comm comm, int ndims, const int dims[],
                               const int periods[], int *newrank);

int QMPI_Cart_rank(void *context, MPI_Comm comm, const int coords[], int *rank) MPICH_API_PUBLIC;
typedef int (QMPI_Cart_rank_t) (void *context, MPI_Comm comm, const int coords[], int *rank);

int QMPI_Cart_shift(void *context, MPI_Comm comm, int direction, int disp, int *rank_source,
                    int *rank_dest) MPICH_API_PUBLIC;
typedef int (QMPI_Cart_shift_t) (void *context, MPI_Comm comm, int direction, int disp,
                                 int *rank_source, int *rank_dest);

int QMPI_Cart_sub(void *context, MPI_Comm comm, const int remain_dims[],
                  MPI_Comm * newcomm) MPICH_API_PUBLIC;
typedef int (QMPI_Cart_sub_t) (void *context, MPI_Comm comm, const int remain_dims[],
                               MPI_Comm * newcomm);

int QMPI_Cartdim_get(void *context, MPI_Comm comm, int *ndims) MPICH_API_PUBLIC;
typedef int (QMPI_Cartdim_get_t) (void *context, MPI_Comm comm, int *ndims);

int QMPI_Close_port(void *context, const char *port_name) MPICH_API_PUBLIC;
typedef int (QMPI_Close_port_t) (void *context, const char *port_name);

int QMPI_Comm_accept(void *context, const char *port_name, MPI_Info info, int root, MPI_Comm comm,
                     MPI_Comm * newcomm) MPICH_API_PUBLIC;
typedef int (QMPI_Comm_accept_t) (void *context, const char *port_name, MPI_Info info, int root,
                                  MPI_Comm comm, MPI_Comm * newcomm);

int QMPI_Comm_call_errhandler(void *context, MPI_Comm comm, int errorcode) MPICH_API_PUBLIC;
typedef int (QMPI_Comm_call_errhandler_t) (void *context, MPI_Comm comm, int errorcode);

int QMPI_Comm_compare(void *context, MPI_Comm comm1, MPI_Comm comm2, int *result) MPICH_API_PUBLIC;
typedef int (QMPI_Comm_compare_t) (void *context, MPI_Comm comm1, MPI_Comm comm2, int *result);

int QMPI_Comm_connect(void *context, const char *port_name, MPI_Info info, int root, MPI_Comm comm,
                      MPI_Comm * newcomm) MPICH_API_PUBLIC;
typedef int (QMPI_Comm_connect_t) (void *context, const char *port_name, MPI_Info info, int root,
                                   MPI_Comm comm, MPI_Comm * newcomm);

int QMPI_Comm_create(void *context, MPI_Comm comm, MPI_Group group,
                     MPI_Comm * newcomm) MPICH_API_PUBLIC;
typedef int (QMPI_Comm_create_t) (void *context, MPI_Comm comm, MPI_Group group,
                                  MPI_Comm * newcomm);

int QMPI_Comm_create_errhandler(void *context, MPI_Comm_errhandler_function * comm_errhandler_fn,
                                MPI_Errhandler * errhandler) MPICH_API_PUBLIC;
typedef int (QMPI_Comm_create_errhandler_t) (void *context,
                                             MPI_Comm_errhandler_function * comm_errhandler_fn,
                                             MPI_Errhandler * errhandler);

int QMPI_Comm_create_group(void *context, MPI_Comm comm, MPI_Group group, int tag,
                           MPI_Comm * newcomm) MPICH_API_PUBLIC;
typedef int (QMPI_Comm_create_group_t) (void *context, MPI_Comm comm, MPI_Group group, int tag,
                                        MPI_Comm * newcomm);

int QMPI_Comm_create_keyval(void *context, MPI_Comm_copy_attr_function * comm_copy_attr_fn,
                            MPI_Comm_delete_attr_function * comm_delete_attr_fn, int *comm_keyval,
                            void *extra_state) MPICH_API_PUBLIC;
typedef int (QMPI_Comm_create_keyval_t) (void *context,
                                         MPI_Comm_copy_attr_function * comm_copy_attr_fn,
                                         MPI_Comm_delete_attr_function * comm_delete_attr_fn,
                                         int *comm_keyval, void *extra_state);

int QMPI_Comm_delete_attr(void *context, MPI_Comm comm, int comm_keyval) MPICH_API_PUBLIC;
typedef int (QMPI_Comm_delete_attr_t) (void *context, MPI_Comm comm, int comm_keyval);

int QMPI_Comm_disconnect(void *context, MPI_Comm * comm) MPICH_API_PUBLIC;
typedef int (QMPI_Comm_disconnect_t) (void *context, MPI_Comm * comm);

int QMPI_Comm_dup(void *context, MPI_Comm comm, MPI_Comm * newcomm) MPICH_API_PUBLIC;
typedef int (QMPI_Comm_dup_t) (void *context, MPI_Comm comm, MPI_Comm * newcomm);

int QMPI_Comm_dup_with_info(void *context, MPI_Comm comm, MPI_Info info,
                            MPI_Comm * newcomm) MPICH_API_PUBLIC;
typedef int (QMPI_Comm_dup_with_info_t) (void *context, MPI_Comm comm, MPI_Info info,
                                         MPI_Comm * newcomm);

int QMPI_Comm_free(void *context, MPI_Comm * comm) MPICH_API_PUBLIC;
typedef int (QMPI_Comm_free_t) (void *context, MPI_Comm * comm);

int QMPI_Comm_free_keyval(void *context, int *comm_keyval) MPICH_API_PUBLIC;
typedef int (QMPI_Comm_free_keyval_t) (void *context, int *comm_keyval);

int QMPI_Comm_get_attr(void *context, MPI_Comm comm, int comm_keyval, void *attribute_val,
                       int *flag) MPICH_API_PUBLIC;
typedef int (QMPI_Comm_get_attr_t) (void *context, MPI_Comm comm, int comm_keyval,
                                    void *attribute_val, int *flag);

int QMPI_Comm_get_errhandler(void *context, MPI_Comm comm,
                             MPI_Errhandler * errhandler) MPICH_API_PUBLIC;
typedef int (QMPI_Comm_get_errhandler_t) (void *context, MPI_Comm comm,
                                          MPI_Errhandler * errhandler);

int QMPI_Comm_get_info(void *context, MPI_Comm comm, MPI_Info * info_used) MPICH_API_PUBLIC;
typedef int (QMPI_Comm_get_info_t) (void *context, MPI_Comm comm, MPI_Info * info_used);

int QMPI_Comm_get_name(void *context, MPI_Comm comm, char *comm_name,
                       int *resultlen) MPICH_API_PUBLIC;
typedef int (QMPI_Comm_get_name_t) (void *context, MPI_Comm comm, char *comm_name, int *resultlen);

int QMPI_Comm_get_parent(void *context, MPI_Comm * parent) MPICH_API_PUBLIC;
typedef int (QMPI_Comm_get_parent_t) (void *context, MPI_Comm * parent);

int QMPI_Comm_group(void *context, MPI_Comm comm, MPI_Group * group) MPICH_API_PUBLIC;
typedef int (QMPI_Comm_group_t) (void *context, MPI_Comm comm, MPI_Group * group);

int QMPI_Comm_idup(void *context, MPI_Comm comm, MPI_Comm * newcomm,
                   MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_Comm_idup_t) (void *context, MPI_Comm comm, MPI_Comm * newcomm,
                                MPI_Request * request);

int QMPI_Comm_join(void *context, int fd, MPI_Comm * intercomm) MPICH_API_PUBLIC;
typedef int (QMPI_Comm_join_t) (void *context, int fd, MPI_Comm * intercomm);

int QMPI_Comm_rank(void *context, MPI_Comm comm, int *rank) MPICH_API_PUBLIC;
typedef int (QMPI_Comm_rank_t) (void *context, MPI_Comm comm, int *rank);

int QMPI_Comm_remote_group(void *context, MPI_Comm comm, MPI_Group * group) MPICH_API_PUBLIC;
typedef int (QMPI_Comm_remote_group_t) (void *context, MPI_Comm comm, MPI_Group * group);

int QMPI_Comm_remote_size(void *context, MPI_Comm comm, int *size) MPICH_API_PUBLIC;
typedef int (QMPI_Comm_remote_size_t) (void *context, MPI_Comm comm, int *size);

int QMPI_Comm_set_attr(void *context, MPI_Comm comm, int comm_keyval,
                       void *attribute_val) MPICH_API_PUBLIC;
typedef int (QMPI_Comm_set_attr_t) (void *context, MPI_Comm comm, int comm_keyval,
                                    void *attribute_val);

int QMPI_Comm_set_errhandler(void *context, MPI_Comm comm,
                             MPI_Errhandler errhandler) MPICH_API_PUBLIC;
typedef int (QMPI_Comm_set_errhandler_t) (void *context, MPI_Comm comm, MPI_Errhandler errhandler);

int QMPI_Comm_set_info(void *context, MPI_Comm comm, MPI_Info info) MPICH_API_PUBLIC;
typedef int (QMPI_Comm_set_info_t) (void *context, MPI_Comm comm, MPI_Info info);

int QMPI_Comm_set_name(void *context, MPI_Comm comm, const char *comm_name) MPICH_API_PUBLIC;
typedef int (QMPI_Comm_set_name_t) (void *context, MPI_Comm comm, const char *comm_name);

int QMPI_Comm_size(void *context, MPI_Comm comm, int *size) MPICH_API_PUBLIC;
typedef int (QMPI_Comm_size_t) (void *context, MPI_Comm comm, int *size);

int QMPI_Comm_spawn(void *context, const char *command, char *argv[], int maxprocs, MPI_Info info,
                    int root, MPI_Comm comm, MPI_Comm * intercomm,
                    int array_of_errcodes[]) MPICH_API_PUBLIC;
typedef int (QMPI_Comm_spawn_t) (void *context, const char *command, char *argv[], int maxprocs,
                                 MPI_Info info, int root, MPI_Comm comm, MPI_Comm * intercomm,
                                 int array_of_errcodes[]);

int QMPI_Comm_spawn_multiple(void *context, int count, char *array_of_commands[],
                             char **array_of_argv[], const int array_of_maxprocs[],
                             const MPI_Info array_of_info[], int root, MPI_Comm comm,
                             MPI_Comm * intercomm, int array_of_errcodes[]) MPICH_API_PUBLIC;
typedef int (QMPI_Comm_spawn_multiple_t) (void *context, int count, char *array_of_commands[],
                                          char **array_of_argv[], const int array_of_maxprocs[],
                                          const MPI_Info array_of_info[], int root, MPI_Comm comm,
                                          MPI_Comm * intercomm, int array_of_errcodes[]);

int QMPI_Comm_split(void *context, MPI_Comm comm, int color, int key,
                    MPI_Comm * newcomm) MPICH_API_PUBLIC;
typedef int (QMPI_Comm_split_t) (void *context, MPI_Comm comm, int color, int key,
                                 MPI_Comm * newcomm);

int QMPI_Comm_split_type(void *context, MPI_Comm comm, int split_type, int key, MPI_Info info,
                         MPI_Comm * newcomm) MPICH_API_PUBLIC;
typedef int (QMPI_Comm_split_type_t) (void *context, MPI_Comm comm, int split_type, int key,
                                      MPI_Info info, MPI_Comm * newcomm);

int QMPI_Comm_test_inter(void *context, MPI_Comm comm, int *flag) MPICH_API_PUBLIC;
typedef int (QMPI_Comm_test_inter_t) (void *context, MPI_Comm comm, int *flag);

int QMPI_Compare_and_swap(void *context, const void *origin_addr, const void *compare_addr,
                          void *result_addr, MPI_Datatype datatype, int target_rank,
                          MPI_Aint target_disp, MPI_Win win) MPICH_API_PUBLIC;
typedef int (QMPI_Compare_and_swap_t) (void *context, const void *origin_addr,
                                       const void *compare_addr, void *result_addr,
                                       MPI_Datatype datatype, int target_rank, MPI_Aint target_disp,
                                       MPI_Win win);

int QMPI_Dims_create(void *context, int nnodes, int ndims, int dims[]) MPICH_API_PUBLIC;
typedef int (QMPI_Dims_create_t) (void *context, int nnodes, int ndims, int dims[]);

int QMPI_Dist_graph_create(void *context, MPI_Comm comm_old, int n, const int sources[],
                           const int degrees[], const int destinations[], const int weights[],
                           MPI_Info info, int reorder, MPI_Comm * comm_dist_graph) MPICH_API_PUBLIC;
typedef int (QMPI_Dist_graph_create_t) (void *context, MPI_Comm comm_old, int n,
                                        const int sources[], const int degrees[],
                                        const int destinations[], const int weights[],
                                        MPI_Info info, int reorder, MPI_Comm * comm_dist_graph);

int QMPI_Dist_graph_create_adjacent(void *context, MPI_Comm comm_old, int indegree,
                                    const int sources[], const int sourceweights[], int outdegree,
                                    const int destinations[], const int destweights[],
                                    MPI_Info info, int reorder,
                                    MPI_Comm * comm_dist_graph) MPICH_API_PUBLIC;
typedef int (QMPI_Dist_graph_create_adjacent_t) (void *context, MPI_Comm comm_old, int indegree,
                                                 const int sources[], const int sourceweights[],
                                                 int outdegree, const int destinations[],
                                                 const int destweights[], MPI_Info info,
                                                 int reorder, MPI_Comm * comm_dist_graph);

int QMPI_Dist_graph_neighbors(void *context, MPI_Comm comm, int maxindegree, int sources[],
                              int sourceweights[], int maxoutdegree, int destinations[],
                              int destweights[]) MPICH_API_PUBLIC;
typedef int (QMPI_Dist_graph_neighbors_t) (void *context, MPI_Comm comm, int maxindegree,
                                           int sources[], int sourceweights[], int maxoutdegree,
                                           int destinations[], int destweights[]);

int QMPI_Dist_graph_neighbors_count(void *context, MPI_Comm comm, int *indegree, int *outdegree,
                                    int *weighted) MPICH_API_PUBLIC;
typedef int (QMPI_Dist_graph_neighbors_count_t) (void *context, MPI_Comm comm, int *indegree,
                                                 int *outdegree, int *weighted);

int QMPI_Errhandler_free(void *context, MPI_Errhandler * errhandler) MPICH_API_PUBLIC;
typedef int (QMPI_Errhandler_free_t) (void *context, MPI_Errhandler * errhandler);

int QMPI_Error_class(void *context, int errorcode, int *errorclass) MPICH_API_PUBLIC;
typedef int (QMPI_Error_class_t) (void *context, int errorcode, int *errorclass);

int QMPI_Error_string(void *context, int errorcode, char *string, int *resultlen) MPICH_API_PUBLIC;
typedef int (QMPI_Error_string_t) (void *context, int errorcode, char *string, int *resultlen);

int QMPI_Exscan(void *context, const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                MPI_Op op, MPI_Comm comm) MPICH_API_PUBLIC;
typedef int (QMPI_Exscan_t) (void *context, const void *sendbuf, void *recvbuf, int count,
                             MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);

int QMPI_Fetch_and_op(void *context, const void *origin_addr, void *result_addr,
                      MPI_Datatype datatype, int target_rank, MPI_Aint target_disp, MPI_Op op,
                      MPI_Win win) MPICH_API_PUBLIC;
typedef int (QMPI_Fetch_and_op_t) (void *context, const void *origin_addr, void *result_addr,
                                   MPI_Datatype datatype, int target_rank, MPI_Aint target_disp,
                                   MPI_Op op, MPI_Win win);

MPI_Fint QMPI_File_c2f(void *context, MPI_File file) MPICH_API_PUBLIC;
typedef MPI_Fint(QMPI_File_c2f_t) (void *context, MPI_File file);

int QMPI_File_call_errhandler(void *context, MPI_File fh, int errorcode) MPICH_API_PUBLIC;
typedef int (QMPI_File_call_errhandler_t) (void *context, MPI_File fh, int errorcode);

int QMPI_File_close(void *context, MPI_File * fh) MPICH_API_PUBLIC;
typedef int (QMPI_File_close_t) (void *context, MPI_File * fh);

int QMPI_File_create_errhandler(void *context, MPI_File_errhandler_function * file_errhandler_fn,
                                MPI_Errhandler * errhandler) MPICH_API_PUBLIC;
typedef int (QMPI_File_create_errhandler_t) (void *context,
                                             MPI_File_errhandler_function * file_errhandler_fn,
                                             MPI_Errhandler * errhandler);

int QMPI_File_delete(void *context, const char *filename, MPI_Info info) MPICH_API_PUBLIC;
typedef int (QMPI_File_delete_t) (void *context, const char *filename, MPI_Info info);

MPI_File QMPI_File_f2c(void *context, MPI_Fint file) MPICH_API_PUBLIC;
typedef MPI_File(QMPI_File_f2c_t) (void *context, MPI_Fint file);

int QMPI_File_get_amode(void *context, MPI_File fh, int *amode) MPICH_API_PUBLIC;
typedef int (QMPI_File_get_amode_t) (void *context, MPI_File fh, int *amode);

int QMPI_File_get_atomicity(void *context, MPI_File fh, int *flag) MPICH_API_PUBLIC;
typedef int (QMPI_File_get_atomicity_t) (void *context, MPI_File fh, int *flag);

int QMPI_File_get_byte_offset(void *context, MPI_File fh, MPI_Offset offset,
                              MPI_Offset * disp) MPICH_API_PUBLIC;
typedef int (QMPI_File_get_byte_offset_t) (void *context, MPI_File fh, MPI_Offset offset,
                                           MPI_Offset * disp);

int QMPI_File_get_errhandler(void *context, MPI_File file,
                             MPI_Errhandler * errhandler) MPICH_API_PUBLIC;
typedef int (QMPI_File_get_errhandler_t) (void *context, MPI_File file,
                                          MPI_Errhandler * errhandler);

int QMPI_File_get_group(void *context, MPI_File fh, MPI_Group * group) MPICH_API_PUBLIC;
typedef int (QMPI_File_get_group_t) (void *context, MPI_File fh, MPI_Group * group);

int QMPI_File_get_info(void *context, MPI_File fh, MPI_Info * info_used) MPICH_API_PUBLIC;
typedef int (QMPI_File_get_info_t) (void *context, MPI_File fh, MPI_Info * info_used);

int QMPI_File_get_position(void *context, MPI_File fh, MPI_Offset * offset) MPICH_API_PUBLIC;
typedef int (QMPI_File_get_position_t) (void *context, MPI_File fh, MPI_Offset * offset);

int QMPI_File_get_position_shared(void *context, MPI_File fh, MPI_Offset * offset) MPICH_API_PUBLIC;
typedef int (QMPI_File_get_position_shared_t) (void *context, MPI_File fh, MPI_Offset * offset);

int QMPI_File_get_size(void *context, MPI_File fh, MPI_Offset * size) MPICH_API_PUBLIC;
typedef int (QMPI_File_get_size_t) (void *context, MPI_File fh, MPI_Offset * size);

int QMPI_File_get_type_extent(void *context, MPI_File fh, MPI_Datatype datatype,
                              MPI_Aint * extent) MPICH_API_PUBLIC;
typedef int (QMPI_File_get_type_extent_t) (void *context, MPI_File fh, MPI_Datatype datatype,
                                           MPI_Aint * extent);

int QMPI_File_get_view(void *context, MPI_File fh, MPI_Offset * disp, MPI_Datatype * etype,
                       MPI_Datatype * filetype, char *datarep) MPICH_API_PUBLIC;
typedef int (QMPI_File_get_view_t) (void *context, MPI_File fh, MPI_Offset * disp,
                                    MPI_Datatype * etype, MPI_Datatype * filetype, char *datarep);

int QMPI_File_iread(void *context, MPI_File fh, void *buf, int count, MPI_Datatype datatype,
                    MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_File_iread_t) (void *context, MPI_File fh, void *buf, int count,
                                 MPI_Datatype datatype, MPI_Request * request);

int QMPI_File_iread_all(void *context, MPI_File fh, void *buf, int count, MPI_Datatype datatype,
                        MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_File_iread_all_t) (void *context, MPI_File fh, void *buf, int count,
                                     MPI_Datatype datatype, MPI_Request * request);

int QMPI_File_iread_at(void *context, MPI_File fh, MPI_Offset offset, void *buf, int count,
                       MPI_Datatype datatype, MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_File_iread_at_t) (void *context, MPI_File fh, MPI_Offset offset, void *buf,
                                    int count, MPI_Datatype datatype, MPI_Request * request);

int QMPI_File_iread_at_all(void *context, MPI_File fh, MPI_Offset offset, void *buf, int count,
                           MPI_Datatype datatype, MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_File_iread_at_all_t) (void *context, MPI_File fh, MPI_Offset offset, void *buf,
                                        int count, MPI_Datatype datatype, MPI_Request * request);

int QMPI_File_iread_shared(void *context, MPI_File fh, void *buf, int count, MPI_Datatype datatype,
                           MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_File_iread_shared_t) (void *context, MPI_File fh, void *buf, int count,
                                        MPI_Datatype datatype, MPI_Request * request);

int QMPI_File_iwrite(void *context, MPI_File fh, const void *buf, int count, MPI_Datatype datatype,
                     MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_File_iwrite_t) (void *context, MPI_File fh, const void *buf, int count,
                                  MPI_Datatype datatype, MPI_Request * request);

int QMPI_File_iwrite_all(void *context, MPI_File fh, const void *buf, int count,
                         MPI_Datatype datatype, MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_File_iwrite_all_t) (void *context, MPI_File fh, const void *buf, int count,
                                      MPI_Datatype datatype, MPI_Request * request);

int QMPI_File_iwrite_at(void *context, MPI_File fh, MPI_Offset offset, const void *buf, int count,
                        MPI_Datatype datatype, MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_File_iwrite_at_t) (void *context, MPI_File fh, MPI_Offset offset, const void *buf,
                                     int count, MPI_Datatype datatype, MPI_Request * request);

int QMPI_File_iwrite_at_all(void *context, MPI_File fh, MPI_Offset offset, const void *buf,
                            int count, MPI_Datatype datatype,
                            MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_File_iwrite_at_all_t) (void *context, MPI_File fh, MPI_Offset offset,
                                         const void *buf, int count, MPI_Datatype datatype,
                                         MPI_Request * request);

int QMPI_File_iwrite_shared(void *context, MPI_File fh, const void *buf, int count,
                            MPI_Datatype datatype, MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_File_iwrite_shared_t) (void *context, MPI_File fh, const void *buf, int count,
                                         MPI_Datatype datatype, MPI_Request * request);

int QMPI_File_open(void *context, MPI_Comm comm, const char *filename, int amode, MPI_Info info,
                   MPI_File * fh) MPICH_API_PUBLIC;
typedef int (QMPI_File_open_t) (void *context, MPI_Comm comm, const char *filename, int amode,
                                MPI_Info info, MPI_File * fh);

int QMPI_File_preallocate(void *context, MPI_File fh, MPI_Offset size) MPICH_API_PUBLIC;
typedef int (QMPI_File_preallocate_t) (void *context, MPI_File fh, MPI_Offset size);

int QMPI_File_read(void *context, MPI_File fh, void *buf, int count, MPI_Datatype datatype,
                   MPI_Status * status) MPICH_API_PUBLIC;
typedef int (QMPI_File_read_t) (void *context, MPI_File fh, void *buf, int count,
                                MPI_Datatype datatype, MPI_Status * status);

int QMPI_File_read_all(void *context, MPI_File fh, void *buf, int count, MPI_Datatype datatype,
                       MPI_Status * status) MPICH_API_PUBLIC;
typedef int (QMPI_File_read_all_t) (void *context, MPI_File fh, void *buf, int count,
                                    MPI_Datatype datatype, MPI_Status * status);

int QMPI_File_read_all_begin(void *context, MPI_File fh, void *buf, int count,
                             MPI_Datatype datatype) MPICH_API_PUBLIC;
typedef int (QMPI_File_read_all_begin_t) (void *context, MPI_File fh, void *buf, int count,
                                          MPI_Datatype datatype);

int QMPI_File_read_all_end(void *context, MPI_File fh, void *buf,
                           MPI_Status * status) MPICH_API_PUBLIC;
typedef int (QMPI_File_read_all_end_t) (void *context, MPI_File fh, void *buf, MPI_Status * status);

int QMPI_File_read_at(void *context, MPI_File fh, MPI_Offset offset, void *buf, int count,
                      MPI_Datatype datatype, MPI_Status * status) MPICH_API_PUBLIC;
typedef int (QMPI_File_read_at_t) (void *context, MPI_File fh, MPI_Offset offset, void *buf,
                                   int count, MPI_Datatype datatype, MPI_Status * status);

int QMPI_File_read_at_all(void *context, MPI_File fh, MPI_Offset offset, void *buf, int count,
                          MPI_Datatype datatype, MPI_Status * status) MPICH_API_PUBLIC;
typedef int (QMPI_File_read_at_all_t) (void *context, MPI_File fh, MPI_Offset offset, void *buf,
                                       int count, MPI_Datatype datatype, MPI_Status * status);

int QMPI_File_read_at_all_begin(void *context, MPI_File fh, MPI_Offset offset, void *buf, int count,
                                MPI_Datatype datatype) MPICH_API_PUBLIC;
typedef int (QMPI_File_read_at_all_begin_t) (void *context, MPI_File fh, MPI_Offset offset,
                                             void *buf, int count, MPI_Datatype datatype);

int QMPI_File_read_at_all_end(void *context, MPI_File fh, void *buf,
                              MPI_Status * status) MPICH_API_PUBLIC;
typedef int (QMPI_File_read_at_all_end_t) (void *context, MPI_File fh, void *buf,
                                           MPI_Status * status);

int QMPI_File_read_ordered(void *context, MPI_File fh, void *buf, int count, MPI_Datatype datatype,
                           MPI_Status * status) MPICH_API_PUBLIC;
typedef int (QMPI_File_read_ordered_t) (void *context, MPI_File fh, void *buf, int count,
                                        MPI_Datatype datatype, MPI_Status * status);

int QMPI_File_read_ordered_begin(void *context, MPI_File fh, void *buf, int count,
                                 MPI_Datatype datatype) MPICH_API_PUBLIC;
typedef int (QMPI_File_read_ordered_begin_t) (void *context, MPI_File fh, void *buf, int count,
                                              MPI_Datatype datatype);

int QMPI_File_read_ordered_end(void *context, MPI_File fh, void *buf,
                               MPI_Status * status) MPICH_API_PUBLIC;
typedef int (QMPI_File_read_ordered_end_t) (void *context, MPI_File fh, void *buf,
                                            MPI_Status * status);

int QMPI_File_read_shared(void *context, MPI_File fh, void *buf, int count, MPI_Datatype datatype,
                          MPI_Status * status) MPICH_API_PUBLIC;
typedef int (QMPI_File_read_shared_t) (void *context, MPI_File fh, void *buf, int count,
                                       MPI_Datatype datatype, MPI_Status * status);

int QMPI_File_seek(void *context, MPI_File fh, MPI_Offset offset, int whence) MPICH_API_PUBLIC;
typedef int (QMPI_File_seek_t) (void *context, MPI_File fh, MPI_Offset offset, int whence);

int QMPI_File_seek_shared(void *context, MPI_File fh, MPI_Offset offset,
                          int whence) MPICH_API_PUBLIC;
typedef int (QMPI_File_seek_shared_t) (void *context, MPI_File fh, MPI_Offset offset, int whence);

int QMPI_File_set_atomicity(void *context, MPI_File fh, int flag) MPICH_API_PUBLIC;
typedef int (QMPI_File_set_atomicity_t) (void *context, MPI_File fh, int flag);

int QMPI_File_set_errhandler(void *context, MPI_File file,
                             MPI_Errhandler errhandler) MPICH_API_PUBLIC;
typedef int (QMPI_File_set_errhandler_t) (void *context, MPI_File file, MPI_Errhandler errhandler);

int QMPI_File_set_info(void *context, MPI_File fh, MPI_Info info) MPICH_API_PUBLIC;
typedef int (QMPI_File_set_info_t) (void *context, MPI_File fh, MPI_Info info);

int QMPI_File_set_size(void *context, MPI_File fh, MPI_Offset size) MPICH_API_PUBLIC;
typedef int (QMPI_File_set_size_t) (void *context, MPI_File fh, MPI_Offset size);

int QMPI_File_set_view(void *context, MPI_File fh, MPI_Offset disp, MPI_Datatype etype,
                       MPI_Datatype filetype, const char *datarep, MPI_Info info) MPICH_API_PUBLIC;
typedef int (QMPI_File_set_view_t) (void *context, MPI_File fh, MPI_Offset disp, MPI_Datatype etype,
                                    MPI_Datatype filetype, const char *datarep, MPI_Info info);

int QMPI_File_sync(void *context, MPI_File fh) MPICH_API_PUBLIC;
typedef int (QMPI_File_sync_t) (void *context, MPI_File fh);

int QMPI_File_write(void *context, MPI_File fh, const void *buf, int count, MPI_Datatype datatype,
                    MPI_Status * status) MPICH_API_PUBLIC;
typedef int (QMPI_File_write_t) (void *context, MPI_File fh, const void *buf, int count,
                                 MPI_Datatype datatype, MPI_Status * status);

int QMPI_File_write_all(void *context, MPI_File fh, const void *buf, int count,
                        MPI_Datatype datatype, MPI_Status * status) MPICH_API_PUBLIC;
typedef int (QMPI_File_write_all_t) (void *context, MPI_File fh, const void *buf, int count,
                                     MPI_Datatype datatype, MPI_Status * status);

int QMPI_File_write_all_begin(void *context, MPI_File fh, const void *buf, int count,
                              MPI_Datatype datatype) MPICH_API_PUBLIC;
typedef int (QMPI_File_write_all_begin_t) (void *context, MPI_File fh, const void *buf, int count,
                                           MPI_Datatype datatype);

int QMPI_File_write_all_end(void *context, MPI_File fh, const void *buf,
                            MPI_Status * status) MPICH_API_PUBLIC;
typedef int (QMPI_File_write_all_end_t) (void *context, MPI_File fh, const void *buf,
                                         MPI_Status * status);

int QMPI_File_write_at(void *context, MPI_File fh, MPI_Offset offset, const void *buf, int count,
                       MPI_Datatype datatype, MPI_Status * status) MPICH_API_PUBLIC;
typedef int (QMPI_File_write_at_t) (void *context, MPI_File fh, MPI_Offset offset, const void *buf,
                                    int count, MPI_Datatype datatype, MPI_Status * status);

int QMPI_File_write_at_all(void *context, MPI_File fh, MPI_Offset offset, const void *buf,
                           int count, MPI_Datatype datatype, MPI_Status * status) MPICH_API_PUBLIC;
typedef int (QMPI_File_write_at_all_t) (void *context, MPI_File fh, MPI_Offset offset,
                                        const void *buf, int count, MPI_Datatype datatype,
                                        MPI_Status * status);

int QMPI_File_write_at_all_begin(void *context, MPI_File fh, MPI_Offset offset, const void *buf,
                                 int count, MPI_Datatype datatype) MPICH_API_PUBLIC;
typedef int (QMPI_File_write_at_all_begin_t) (void *context, MPI_File fh, MPI_Offset offset,
                                              const void *buf, int count, MPI_Datatype datatype);

int QMPI_File_write_at_all_end(void *context, MPI_File fh, const void *buf,
                               MPI_Status * status) MPICH_API_PUBLIC;
typedef int (QMPI_File_write_at_all_end_t) (void *context, MPI_File fh, const void *buf,
                                            MPI_Status * status);

int QMPI_File_write_ordered(void *context, MPI_File fh, const void *buf, int count,
                            MPI_Datatype datatype, MPI_Status * status) MPICH_API_PUBLIC;
typedef int (QMPI_File_write_ordered_t) (void *context, MPI_File fh, const void *buf, int count,
                                         MPI_Datatype datatype, MPI_Status * status);

int QMPI_File_write_ordered_begin(void *context, MPI_File fh, const void *buf, int count,
                                  MPI_Datatype datatype) MPICH_API_PUBLIC;
typedef int (QMPI_File_write_ordered_begin_t) (void *context, MPI_File fh, const void *buf,
                                               int count, MPI_Datatype datatype);

int QMPI_File_write_ordered_end(void *context, MPI_File fh, const void *buf,
                                MPI_Status * status) MPICH_API_PUBLIC;
typedef int (QMPI_File_write_ordered_end_t) (void *context, MPI_File fh, const void *buf,
                                             MPI_Status * status);

int QMPI_File_write_shared(void *context, MPI_File fh, const void *buf, int count,
                           MPI_Datatype datatype, MPI_Status * status) MPICH_API_PUBLIC;
typedef int (QMPI_File_write_shared_t) (void *context, MPI_File fh, const void *buf, int count,
                                        MPI_Datatype datatype, MPI_Status * status);

int QMPI_Finalize(void *context) MPICH_API_PUBLIC;
typedef int (QMPI_Finalize_t) (void *context);

int QMPI_Finalized(void *context, int *flag) MPICH_API_PUBLIC;
typedef int (QMPI_Finalized_t) (void *context, int *flag);

int QMPI_Free_mem(void *context, void *base) MPICH_API_PUBLIC;
typedef int (QMPI_Free_mem_t) (void *context, void *base);

int QMPI_Gather(void *context, const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                void *recvbuf, int recvcount, MPI_Datatype recvtype, int root,
                MPI_Comm comm) MPICH_API_PUBLIC;
typedef int (QMPI_Gather_t) (void *context, const void *sendbuf, int sendcount,
                             MPI_Datatype sendtype, void *recvbuf, int recvcount,
                             MPI_Datatype recvtype, int root, MPI_Comm comm);

int QMPI_Gatherv(void *context, const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                 void *recvbuf, const int recvcounts[], const int displs[], MPI_Datatype recvtype,
                 int root, MPI_Comm comm) MPICH_API_PUBLIC;
typedef int (QMPI_Gatherv_t) (void *context, const void *sendbuf, int sendcount,
                              MPI_Datatype sendtype, void *recvbuf, const int recvcounts[],
                              const int displs[], MPI_Datatype recvtype, int root, MPI_Comm comm);

int QMPI_Get(void *context, void *origin_addr, int origin_count, MPI_Datatype origin_datatype,
             int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype,
             MPI_Win win) MPICH_API_PUBLIC;
typedef int (QMPI_Get_t) (void *context, void *origin_addr, int origin_count,
                          MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp,
                          int target_count, MPI_Datatype target_datatype, MPI_Win win);

int QMPI_Get_accumulate(void *context, const void *origin_addr, int origin_count,
                        MPI_Datatype origin_datatype, void *result_addr, int result_count,
                        MPI_Datatype result_datatype, int target_rank, MPI_Aint target_disp,
                        int target_count, MPI_Datatype target_datatype, MPI_Op op,
                        MPI_Win win) MPICH_API_PUBLIC;
typedef int (QMPI_Get_accumulate_t) (void *context, const void *origin_addr, int origin_count,
                                     MPI_Datatype origin_datatype, void *result_addr,
                                     int result_count, MPI_Datatype result_datatype,
                                     int target_rank, MPI_Aint target_disp, int target_count,
                                     MPI_Datatype target_datatype, MPI_Op op, MPI_Win win);

int QMPI_Get_address(void *context, const void *location, MPI_Aint * address) MPICH_API_PUBLIC;
typedef int (QMPI_Get_address_t) (void *context, const void *location, MPI_Aint * address);

int QMPI_Get_count(void *context, const MPI_Status * status, MPI_Datatype datatype,
                   int *count) MPICH_API_PUBLIC;
typedef int (QMPI_Get_count_t) (void *context, const MPI_Status * status, MPI_Datatype datatype,
                                int *count);

int QMPI_Get_elements(void *context, const MPI_Status * status, MPI_Datatype datatype,
                      int *count) MPICH_API_PUBLIC;
typedef int (QMPI_Get_elements_t) (void *context, const MPI_Status * status, MPI_Datatype datatype,
                                   int *count);

int QMPI_Get_elements_x(void *context, const MPI_Status * status, MPI_Datatype datatype,
                        MPI_Count * count) MPICH_API_PUBLIC;
typedef int (QMPI_Get_elements_x_t) (void *context, const MPI_Status * status,
                                     MPI_Datatype datatype, MPI_Count * count);

int QMPI_Get_library_version(void *context, char *version, int *resultlen) MPICH_API_PUBLIC;
typedef int (QMPI_Get_library_version_t) (void *context, char *version, int *resultlen);

int QMPI_Get_processor_name(void *context, char *name, int *resultlen) MPICH_API_PUBLIC;
typedef int (QMPI_Get_processor_name_t) (void *context, char *name, int *resultlen);

int QMPI_Get_version(void *context, int *version, int *subversion) MPICH_API_PUBLIC;
typedef int (QMPI_Get_version_t) (void *context, int *version, int *subversion);

int QMPI_Graph_create(void *context, MPI_Comm comm_old, int nnodes, const int index[],
                      const int edges[], int reorder, MPI_Comm * comm_graph) MPICH_API_PUBLIC;
typedef int (QMPI_Graph_create_t) (void *context, MPI_Comm comm_old, int nnodes, const int index[],
                                   const int edges[], int reorder, MPI_Comm * comm_graph);

int QMPI_Graph_get(void *context, MPI_Comm comm, int maxindex, int maxedges, int index[],
                   int edges[]) MPICH_API_PUBLIC;
typedef int (QMPI_Graph_get_t) (void *context, MPI_Comm comm, int maxindex, int maxedges,
                                int index[], int edges[]);

int QMPI_Graph_map(void *context, MPI_Comm comm, int nnodes, const int index[], const int edges[],
                   int *newrank) MPICH_API_PUBLIC;
typedef int (QMPI_Graph_map_t) (void *context, MPI_Comm comm, int nnodes, const int index[],
                                const int edges[], int *newrank);

int QMPI_Graph_neighbors(void *context, MPI_Comm comm, int rank, int maxneighbors,
                         int neighbors[]) MPICH_API_PUBLIC;
typedef int (QMPI_Graph_neighbors_t) (void *context, MPI_Comm comm, int rank, int maxneighbors,
                                      int neighbors[]);

int QMPI_Graph_neighbors_count(void *context, MPI_Comm comm, int rank,
                               int *nneighbors) MPICH_API_PUBLIC;
typedef int (QMPI_Graph_neighbors_count_t) (void *context, MPI_Comm comm, int rank,
                                            int *nneighbors);

int QMPI_Graphdims_get(void *context, MPI_Comm comm, int *nnodes, int *nedges) MPICH_API_PUBLIC;
typedef int (QMPI_Graphdims_get_t) (void *context, MPI_Comm comm, int *nnodes, int *nedges);

int QMPI_Grequest_complete(void *context, MPI_Request request) MPICH_API_PUBLIC;
typedef int (QMPI_Grequest_complete_t) (void *context, MPI_Request request);

int QMPI_Grequest_start(void *context, MPI_Grequest_query_function * query_fn,
                        MPI_Grequest_free_function * free_fn,
                        MPI_Grequest_cancel_function * cancel_fn, void *extra_state,
                        MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_Grequest_start_t) (void *context, MPI_Grequest_query_function * query_fn,
                                     MPI_Grequest_free_function * free_fn,
                                     MPI_Grequest_cancel_function * cancel_fn, void *extra_state,
                                     MPI_Request * request);

int QMPI_Group_compare(void *context, MPI_Group group1, MPI_Group group2,
                       int *result) MPICH_API_PUBLIC;
typedef int (QMPI_Group_compare_t) (void *context, MPI_Group group1, MPI_Group group2, int *result);

int QMPI_Group_difference(void *context, MPI_Group group1, MPI_Group group2,
                          MPI_Group * newgroup) MPICH_API_PUBLIC;
typedef int (QMPI_Group_difference_t) (void *context, MPI_Group group1, MPI_Group group2,
                                       MPI_Group * newgroup);

int QMPI_Group_excl(void *context, MPI_Group group, int n, const int ranks[],
                    MPI_Group * newgroup) MPICH_API_PUBLIC;
typedef int (QMPI_Group_excl_t) (void *context, MPI_Group group, int n, const int ranks[],
                                 MPI_Group * newgroup);

int QMPI_Group_free(void *context, MPI_Group * group) MPICH_API_PUBLIC;
typedef int (QMPI_Group_free_t) (void *context, MPI_Group * group);

int QMPI_Group_incl(void *context, MPI_Group group, int n, const int ranks[],
                    MPI_Group * newgroup) MPICH_API_PUBLIC;
typedef int (QMPI_Group_incl_t) (void *context, MPI_Group group, int n, const int ranks[],
                                 MPI_Group * newgroup);

int QMPI_Group_intersection(void *context, MPI_Group group1, MPI_Group group2,
                            MPI_Group * newgroup) MPICH_API_PUBLIC;
typedef int (QMPI_Group_intersection_t) (void *context, MPI_Group group1, MPI_Group group2,
                                         MPI_Group * newgroup);

int QMPI_Group_range_excl(void *context, MPI_Group group, int n, int ranges[][3],
                          MPI_Group * newgroup) MPICH_API_PUBLIC;
typedef int (QMPI_Group_range_excl_t) (void *context, MPI_Group group, int n, int ranges[][3],
                                       MPI_Group * newgroup);

int QMPI_Group_range_incl(void *context, MPI_Group group, int n, int ranges[][3],
                          MPI_Group * newgroup) MPICH_API_PUBLIC;
typedef int (QMPI_Group_range_incl_t) (void *context, MPI_Group group, int n, int ranges[][3],
                                       MPI_Group * newgroup);

int QMPI_Group_rank(void *context, MPI_Group group, int *rank) MPICH_API_PUBLIC;
typedef int (QMPI_Group_rank_t) (void *context, MPI_Group group, int *rank);

int QMPI_Group_size(void *context, MPI_Group group, int *size) MPICH_API_PUBLIC;
typedef int (QMPI_Group_size_t) (void *context, MPI_Group group, int *size);

int QMPI_Group_translate_ranks(void *context, MPI_Group group1, int n, const int ranks1[],
                               MPI_Group group2, int ranks2[]) MPICH_API_PUBLIC;
typedef int (QMPI_Group_translate_ranks_t) (void *context, MPI_Group group1, int n,
                                            const int ranks1[], MPI_Group group2, int ranks2[]);

int QMPI_Group_union(void *context, MPI_Group group1, MPI_Group group2,
                     MPI_Group * newgroup) MPICH_API_PUBLIC;
typedef int (QMPI_Group_union_t) (void *context, MPI_Group group1, MPI_Group group2,
                                  MPI_Group * newgroup);

int QMPI_Iallgather(void *context, const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                    void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm,
                    MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_Iallgather_t) (void *context, const void *sendbuf, int sendcount,
                                 MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                 MPI_Datatype recvtype, MPI_Comm comm, MPI_Request * request);

int QMPI_Iallgatherv(void *context, const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                     void *recvbuf, const int recvcounts[], const int displs[],
                     MPI_Datatype recvtype, MPI_Comm comm, MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_Iallgatherv_t) (void *context, const void *sendbuf, int sendcount,
                                  MPI_Datatype sendtype, void *recvbuf, const int recvcounts[],
                                  const int displs[], MPI_Datatype recvtype, MPI_Comm comm,
                                  MPI_Request * request);

int QMPI_Iallreduce(void *context, const void *sendbuf, void *recvbuf, int count,
                    MPI_Datatype datatype, MPI_Op op, MPI_Comm comm,
                    MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_Iallreduce_t) (void *context, const void *sendbuf, void *recvbuf, int count,
                                 MPI_Datatype datatype, MPI_Op op, MPI_Comm comm,
                                 MPI_Request * request);

int QMPI_Ialltoall(void *context, const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                   void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm,
                   MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_Ialltoall_t) (void *context, const void *sendbuf, int sendcount,
                                MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                MPI_Datatype recvtype, MPI_Comm comm, MPI_Request * request);

int QMPI_Ialltoallv(void *context, const void *sendbuf, const int sendcounts[], const int sdispls[],
                    MPI_Datatype sendtype, void *recvbuf, const int recvcounts[],
                    const int rdispls[], MPI_Datatype recvtype, MPI_Comm comm,
                    MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_Ialltoallv_t) (void *context, const void *sendbuf, const int sendcounts[],
                                 const int sdispls[], MPI_Datatype sendtype, void *recvbuf,
                                 const int recvcounts[], const int rdispls[], MPI_Datatype recvtype,
                                 MPI_Comm comm, MPI_Request * request);

int QMPI_Ialltoallw(void *context, const void *sendbuf, const int sendcounts[], const int sdispls[],
                    const MPI_Datatype sendtypes[], void *recvbuf, const int recvcounts[],
                    const int rdispls[], const MPI_Datatype recvtypes[], MPI_Comm comm,
                    MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_Ialltoallw_t) (void *context, const void *sendbuf, const int sendcounts[],
                                 const int sdispls[], const MPI_Datatype sendtypes[], void *recvbuf,
                                 const int recvcounts[], const int rdispls[],
                                 const MPI_Datatype recvtypes[], MPI_Comm comm,
                                 MPI_Request * request);

int QMPI_Ibarrier(void *context, MPI_Comm comm, MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_Ibarrier_t) (void *context, MPI_Comm comm, MPI_Request * request);

int QMPI_Ibcast(void *context, void *buffer, int count, MPI_Datatype datatype, int root,
                MPI_Comm comm, MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_Ibcast_t) (void *context, void *buffer, int count, MPI_Datatype datatype,
                             int root, MPI_Comm comm, MPI_Request * request);

int QMPI_Ibsend(void *context, const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
                MPI_Comm comm, MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_Ibsend_t) (void *context, const void *buf, int count, MPI_Datatype datatype,
                             int dest, int tag, MPI_Comm comm, MPI_Request * request);

int QMPI_Iexscan(void *context, const void *sendbuf, void *recvbuf, int count,
                 MPI_Datatype datatype, MPI_Op op, MPI_Comm comm,
                 MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_Iexscan_t) (void *context, const void *sendbuf, void *recvbuf, int count,
                              MPI_Datatype datatype, MPI_Op op, MPI_Comm comm,
                              MPI_Request * request);

int QMPI_Igather(void *context, const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                 void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm,
                 MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_Igather_t) (void *context, const void *sendbuf, int sendcount,
                              MPI_Datatype sendtype, void *recvbuf, int recvcount,
                              MPI_Datatype recvtype, int root, MPI_Comm comm,
                              MPI_Request * request);

int QMPI_Igatherv(void *context, const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                  void *recvbuf, const int recvcounts[], const int displs[], MPI_Datatype recvtype,
                  int root, MPI_Comm comm, MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_Igatherv_t) (void *context, const void *sendbuf, int sendcount,
                               MPI_Datatype sendtype, void *recvbuf, const int recvcounts[],
                               const int displs[], MPI_Datatype recvtype, int root, MPI_Comm comm,
                               MPI_Request * request);

int QMPI_Improbe(void *context, int source, int tag, MPI_Comm comm, int *flag,
                 MPI_Message * message, MPI_Status * status) MPICH_API_PUBLIC;
typedef int (QMPI_Improbe_t) (void *context, int source, int tag, MPI_Comm comm, int *flag,
                              MPI_Message * message, MPI_Status * status);

int QMPI_Imrecv(void *context, void *buf, int count, MPI_Datatype datatype, MPI_Message * message,
                MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_Imrecv_t) (void *context, void *buf, int count, MPI_Datatype datatype,
                             MPI_Message * message, MPI_Request * request);

int QMPI_Ineighbor_allgather(void *context, const void *sendbuf, int sendcount,
                             MPI_Datatype sendtype, void *recvbuf, int recvcount,
                             MPI_Datatype recvtype, MPI_Comm comm,
                             MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_Ineighbor_allgather_t) (void *context, const void *sendbuf, int sendcount,
                                          MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                          MPI_Datatype recvtype, MPI_Comm comm,
                                          MPI_Request * request);

int QMPI_Ineighbor_allgatherv(void *context, const void *sendbuf, int sendcount,
                              MPI_Datatype sendtype, void *recvbuf, const int recvcounts[],
                              const int displs[], MPI_Datatype recvtype, MPI_Comm comm,
                              MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_Ineighbor_allgatherv_t) (void *context, const void *sendbuf, int sendcount,
                                           MPI_Datatype sendtype, void *recvbuf,
                                           const int recvcounts[], const int displs[],
                                           MPI_Datatype recvtype, MPI_Comm comm,
                                           MPI_Request * request);

int QMPI_Ineighbor_alltoall(void *context, const void *sendbuf, int sendcount,
                            MPI_Datatype sendtype, void *recvbuf, int recvcount,
                            MPI_Datatype recvtype, MPI_Comm comm,
                            MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_Ineighbor_alltoall_t) (void *context, const void *sendbuf, int sendcount,
                                         MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                         MPI_Datatype recvtype, MPI_Comm comm,
                                         MPI_Request * request);

int QMPI_Ineighbor_alltoallv(void *context, const void *sendbuf, const int sendcounts[],
                             const int sdispls[], MPI_Datatype sendtype, void *recvbuf,
                             const int recvcounts[], const int rdispls[], MPI_Datatype recvtype,
                             MPI_Comm comm, MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_Ineighbor_alltoallv_t) (void *context, const void *sendbuf,
                                          const int sendcounts[], const int sdispls[],
                                          MPI_Datatype sendtype, void *recvbuf,
                                          const int recvcounts[], const int rdispls[],
                                          MPI_Datatype recvtype, MPI_Comm comm,
                                          MPI_Request * request);

int QMPI_Ineighbor_alltoallw(void *context, const void *sendbuf, const int sendcounts[],
                             const MPI_Aint sdispls[], const MPI_Datatype sendtypes[],
                             void *recvbuf, const int recvcounts[], const MPI_Aint rdispls[],
                             const MPI_Datatype recvtypes[], MPI_Comm comm,
                             MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_Ineighbor_alltoallw_t) (void *context, const void *sendbuf,
                                          const int sendcounts[], const MPI_Aint sdispls[],
                                          const MPI_Datatype sendtypes[], void *recvbuf,
                                          const int recvcounts[], const MPI_Aint rdispls[],
                                          const MPI_Datatype recvtypes[], MPI_Comm comm,
                                          MPI_Request * request);

int QMPI_Info_create(void *context, MPI_Info * info) MPICH_API_PUBLIC;
typedef int (QMPI_Info_create_t) (void *context, MPI_Info * info);

int QMPI_Info_delete(void *context, MPI_Info info, const char *key) MPICH_API_PUBLIC;
typedef int (QMPI_Info_delete_t) (void *context, MPI_Info info, const char *key);

int QMPI_Info_dup(void *context, MPI_Info info, MPI_Info * newinfo) MPICH_API_PUBLIC;
typedef int (QMPI_Info_dup_t) (void *context, MPI_Info info, MPI_Info * newinfo);

int QMPI_Info_free(void *context, MPI_Info * info) MPICH_API_PUBLIC;
typedef int (QMPI_Info_free_t) (void *context, MPI_Info * info);

int QMPI_Info_get(void *context, MPI_Info info, const char *key, int valuelen, char *value,
                  int *flag) MPICH_API_PUBLIC;
typedef int (QMPI_Info_get_t) (void *context, MPI_Info info, const char *key, int valuelen,
                               char *value, int *flag);

int QMPI_Info_get_nkeys(void *context, MPI_Info info, int *nkeys) MPICH_API_PUBLIC;
typedef int (QMPI_Info_get_nkeys_t) (void *context, MPI_Info info, int *nkeys);

int QMPI_Info_get_nthkey(void *context, MPI_Info info, int n, char *key) MPICH_API_PUBLIC;
typedef int (QMPI_Info_get_nthkey_t) (void *context, MPI_Info info, int n, char *key);

int QMPI_Info_get_valuelen(void *context, MPI_Info info, const char *key, int *valuelen,
                           int *flag) MPICH_API_PUBLIC;
typedef int (QMPI_Info_get_valuelen_t) (void *context, MPI_Info info, const char *key,
                                        int *valuelen, int *flag);

int QMPI_Info_set(void *context, MPI_Info info, const char *key,
                  const char *value) MPICH_API_PUBLIC;
typedef int (QMPI_Info_set_t) (void *context, MPI_Info info, const char *key, const char *value);

int QMPI_Init(void *context, int *argc, char ***argv) MPICH_API_PUBLIC;
typedef int (QMPI_Init_t) (void *context, int *argc, char ***argv);

int QMPI_Init_thread(void *context, int *argc, char ***argv, int required,
                     int *provided) MPICH_API_PUBLIC;
typedef int (QMPI_Init_thread_t) (void *context, int *argc, char ***argv, int required,
                                  int *provided);

int QMPI_Initialized(void *context, int *flag) MPICH_API_PUBLIC;
typedef int (QMPI_Initialized_t) (void *context, int *flag);

int QMPI_Intercomm_create(void *context, MPI_Comm local_comm, int local_leader, MPI_Comm peer_comm,
                          int remote_leader, int tag, MPI_Comm * newintercomm) MPICH_API_PUBLIC;
typedef int (QMPI_Intercomm_create_t) (void *context, MPI_Comm local_comm, int local_leader,
                                       MPI_Comm peer_comm, int remote_leader, int tag,
                                       MPI_Comm * newintercomm);

int QMPI_Intercomm_merge(void *context, MPI_Comm intercomm, int high,
                         MPI_Comm * newintracomm) MPICH_API_PUBLIC;
typedef int (QMPI_Intercomm_merge_t) (void *context, MPI_Comm intercomm, int high,
                                      MPI_Comm * newintracomm);

int QMPI_Iprobe(void *context, int source, int tag, MPI_Comm comm, int *flag,
                MPI_Status * status) MPICH_API_PUBLIC;
typedef int (QMPI_Iprobe_t) (void *context, int source, int tag, MPI_Comm comm, int *flag,
                             MPI_Status * status);

int QMPI_Irecv(void *context, void *buf, int count, MPI_Datatype datatype, int source, int tag,
               MPI_Comm comm, MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_Irecv_t) (void *context, void *buf, int count, MPI_Datatype datatype, int source,
                            int tag, MPI_Comm comm, MPI_Request * request);

int QMPI_Ireduce(void *context, const void *sendbuf, void *recvbuf, int count,
                 MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm,
                 MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_Ireduce_t) (void *context, const void *sendbuf, void *recvbuf, int count,
                              MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm,
                              MPI_Request * request);

int QMPI_Ireduce_scatter(void *context, const void *sendbuf, void *recvbuf, const int recvcounts[],
                         MPI_Datatype datatype, MPI_Op op, MPI_Comm comm,
                         MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_Ireduce_scatter_t) (void *context, const void *sendbuf, void *recvbuf,
                                      const int recvcounts[], MPI_Datatype datatype, MPI_Op op,
                                      MPI_Comm comm, MPI_Request * request);

int QMPI_Ireduce_scatter_block(void *context, const void *sendbuf, void *recvbuf, int recvcount,
                               MPI_Datatype datatype, MPI_Op op, MPI_Comm comm,
                               MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_Ireduce_scatter_block_t) (void *context, const void *sendbuf, void *recvbuf,
                                            int recvcount, MPI_Datatype datatype, MPI_Op op,
                                            MPI_Comm comm, MPI_Request * request);

int QMPI_Irsend(void *context, const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
                MPI_Comm comm, MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_Irsend_t) (void *context, const void *buf, int count, MPI_Datatype datatype,
                             int dest, int tag, MPI_Comm comm, MPI_Request * request);

int QMPI_Is_thread_main(void *context, int *flag) MPICH_API_PUBLIC;
typedef int (QMPI_Is_thread_main_t) (void *context, int *flag);

int QMPI_Iscan(void *context, const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
               MPI_Op op, MPI_Comm comm, MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_Iscan_t) (void *context, const void *sendbuf, void *recvbuf, int count,
                            MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request * request);

int QMPI_Iscatter(void *context, const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                  void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm,
                  MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_Iscatter_t) (void *context, const void *sendbuf, int sendcount,
                               MPI_Datatype sendtype, void *recvbuf, int recvcount,
                               MPI_Datatype recvtype, int root, MPI_Comm comm,
                               MPI_Request * request);

int QMPI_Iscatterv(void *context, const void *sendbuf, const int sendcounts[], const int displs[],
                   MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype,
                   int root, MPI_Comm comm, MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_Iscatterv_t) (void *context, const void *sendbuf, const int sendcounts[],
                                const int displs[], MPI_Datatype sendtype, void *recvbuf,
                                int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm,
                                MPI_Request * request);

int QMPI_Isend(void *context, const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
               MPI_Comm comm, MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_Isend_t) (void *context, const void *buf, int count, MPI_Datatype datatype,
                            int dest, int tag, MPI_Comm comm, MPI_Request * request);

int QMPI_Issend(void *context, const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
                MPI_Comm comm, MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_Issend_t) (void *context, const void *buf, int count, MPI_Datatype datatype,
                             int dest, int tag, MPI_Comm comm, MPI_Request * request);

int QMPI_Keyval_create(void *context, MPI_Copy_function * copy_fn, MPI_Delete_function * delete_fn,
                       int *keyval, void *extra_state) MPICH_API_PUBLIC;
typedef int (QMPI_Keyval_create_t) (void *context, MPI_Copy_function * copy_fn,
                                    MPI_Delete_function * delete_fn, int *keyval,
                                    void *extra_state);

int QMPI_Keyval_free(void *context, int *keyval) MPICH_API_PUBLIC;
typedef int (QMPI_Keyval_free_t) (void *context, int *keyval);

int QMPI_Lookup_name(void *context, const char *service_name, MPI_Info info,
                     char *port_name) MPICH_API_PUBLIC;
typedef int (QMPI_Lookup_name_t) (void *context, const char *service_name, MPI_Info info,
                                  char *port_name);

int QMPI_Mprobe(void *context, int source, int tag, MPI_Comm comm, MPI_Message * message,
                MPI_Status * status) MPICH_API_PUBLIC;
typedef int (QMPI_Mprobe_t) (void *context, int source, int tag, MPI_Comm comm,
                             MPI_Message * message, MPI_Status * status);

int QMPI_Mrecv(void *context, void *buf, int count, MPI_Datatype datatype, MPI_Message * message,
               MPI_Status * status) MPICH_API_PUBLIC;
typedef int (QMPI_Mrecv_t) (void *context, void *buf, int count, MPI_Datatype datatype,
                            MPI_Message * message, MPI_Status * status);

int QMPI_Neighbor_allgather(void *context, const void *sendbuf, int sendcount,
                            MPI_Datatype sendtype, void *recvbuf, int recvcount,
                            MPI_Datatype recvtype, MPI_Comm comm) MPICH_API_PUBLIC;
typedef int (QMPI_Neighbor_allgather_t) (void *context, const void *sendbuf, int sendcount,
                                         MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                         MPI_Datatype recvtype, MPI_Comm comm);

int QMPI_Neighbor_allgatherv(void *context, const void *sendbuf, int sendcount,
                             MPI_Datatype sendtype, void *recvbuf, const int recvcounts[],
                             const int displs[], MPI_Datatype recvtype,
                             MPI_Comm comm) MPICH_API_PUBLIC;
typedef int (QMPI_Neighbor_allgatherv_t) (void *context, const void *sendbuf, int sendcount,
                                          MPI_Datatype sendtype, void *recvbuf,
                                          const int recvcounts[], const int displs[],
                                          MPI_Datatype recvtype, MPI_Comm comm);

int QMPI_Neighbor_alltoall(void *context, const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                           void *recvbuf, int recvcount, MPI_Datatype recvtype,
                           MPI_Comm comm) MPICH_API_PUBLIC;
typedef int (QMPI_Neighbor_alltoall_t) (void *context, const void *sendbuf, int sendcount,
                                        MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                        MPI_Datatype recvtype, MPI_Comm comm);

int QMPI_Neighbor_alltoallv(void *context, const void *sendbuf, const int sendcounts[],
                            const int sdispls[], MPI_Datatype sendtype, void *recvbuf,
                            const int recvcounts[], const int rdispls[], MPI_Datatype recvtype,
                            MPI_Comm comm) MPICH_API_PUBLIC;
typedef int (QMPI_Neighbor_alltoallv_t) (void *context, const void *sendbuf, const int sendcounts[],
                                         const int sdispls[], MPI_Datatype sendtype, void *recvbuf,
                                         const int recvcounts[], const int rdispls[],
                                         MPI_Datatype recvtype, MPI_Comm comm);

int QMPI_Neighbor_alltoallw(void *context, const void *sendbuf, const int sendcounts[],
                            const MPI_Aint sdispls[], const MPI_Datatype sendtypes[], void *recvbuf,
                            const int recvcounts[], const MPI_Aint rdispls[],
                            const MPI_Datatype recvtypes[], MPI_Comm comm) MPICH_API_PUBLIC;
typedef int (QMPI_Neighbor_alltoallw_t) (void *context, const void *sendbuf, const int sendcounts[],
                                         const MPI_Aint sdispls[], const MPI_Datatype sendtypes[],
                                         void *recvbuf, const int recvcounts[],
                                         const MPI_Aint rdispls[], const MPI_Datatype recvtypes[],
                                         MPI_Comm comm);

int QMPI_Op_commutative(void *context, MPI_Op op, int *commute) MPICH_API_PUBLIC;
typedef int (QMPI_Op_commutative_t) (void *context, MPI_Op op, int *commute);

int QMPI_Op_create(void *context, MPI_User_function * user_fn, int commute,
                   MPI_Op * op) MPICH_API_PUBLIC;
typedef int (QMPI_Op_create_t) (void *context, MPI_User_function * user_fn, int commute,
                                MPI_Op * op);

int QMPI_Op_free(void *context, MPI_Op * op) MPICH_API_PUBLIC;
typedef int (QMPI_Op_free_t) (void *context, MPI_Op * op);

int QMPI_Open_port(void *context, MPI_Info info, char *port_name) MPICH_API_PUBLIC;
typedef int (QMPI_Open_port_t) (void *context, MPI_Info info, char *port_name);

int QMPI_Pack(void *context, const void *inbuf, int incount, MPI_Datatype datatype, void *outbuf,
              int outsize, int *position, MPI_Comm comm) MPICH_API_PUBLIC;
typedef int (QMPI_Pack_t) (void *context, const void *inbuf, int incount, MPI_Datatype datatype,
                           void *outbuf, int outsize, int *position, MPI_Comm comm);

int QMPI_Pack_external(void *context, const char datarep[], const void *inbuf, int incount,
                       MPI_Datatype datatype, void *outbuf, MPI_Aint outsize,
                       MPI_Aint * position) MPICH_API_PUBLIC;
typedef int (QMPI_Pack_external_t) (void *context, const char datarep[], const void *inbuf,
                                    int incount, MPI_Datatype datatype, void *outbuf,
                                    MPI_Aint outsize, MPI_Aint * position);

int QMPI_Pack_external_size(void *context, const char datarep[], int incount, MPI_Datatype datatype,
                            MPI_Aint * size) MPICH_API_PUBLIC;
typedef int (QMPI_Pack_external_size_t) (void *context, const char datarep[], int incount,
                                         MPI_Datatype datatype, MPI_Aint * size);

int QMPI_Pack_size(void *context, int incount, MPI_Datatype datatype, MPI_Comm comm,
                   int *size) MPICH_API_PUBLIC;
typedef int (QMPI_Pack_size_t) (void *context, int incount, MPI_Datatype datatype, MPI_Comm comm,
                                int *size);

int QMPI_Pcontrol(void *context, const int level, ...) MPICH_API_PUBLIC;
typedef int (QMPI_Pcontrol_t) (void *context, const int level, ...);

int QMPI_Probe(void *context, int source, int tag, MPI_Comm comm,
               MPI_Status * status) MPICH_API_PUBLIC;
typedef int (QMPI_Probe_t) (void *context, int source, int tag, MPI_Comm comm, MPI_Status * status);

int QMPI_Publish_name(void *context, const char *service_name, MPI_Info info,
                      const char *port_name) MPICH_API_PUBLIC;
typedef int (QMPI_Publish_name_t) (void *context, const char *service_name, MPI_Info info,
                                   const char *port_name);

int QMPI_Put(void *context, const void *origin_addr, int origin_count, MPI_Datatype origin_datatype,
             int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype,
             MPI_Win win) MPICH_API_PUBLIC;
typedef int (QMPI_Put_t) (void *context, const void *origin_addr, int origin_count,
                          MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp,
                          int target_count, MPI_Datatype target_datatype, MPI_Win win);

int QMPI_Query_thread(void *context, int *provided) MPICH_API_PUBLIC;
typedef int (QMPI_Query_thread_t) (void *context, int *provided);

int QMPI_Raccumulate(void *context, const void *origin_addr, int origin_count,
                     MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp,
                     int target_count, MPI_Datatype target_datatype, MPI_Op op, MPI_Win win,
                     MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_Raccumulate_t) (void *context, const void *origin_addr, int origin_count,
                                  MPI_Datatype origin_datatype, int target_rank,
                                  MPI_Aint target_disp, int target_count,
                                  MPI_Datatype target_datatype, MPI_Op op, MPI_Win win,
                                  MPI_Request * request);

int QMPI_Recv(void *context, void *buf, int count, MPI_Datatype datatype, int source, int tag,
              MPI_Comm comm, MPI_Status * status) MPICH_API_PUBLIC;
typedef int (QMPI_Recv_t) (void *context, void *buf, int count, MPI_Datatype datatype, int source,
                           int tag, MPI_Comm comm, MPI_Status * status);

int QMPI_Recv_init(void *context, void *buf, int count, MPI_Datatype datatype, int source, int tag,
                   MPI_Comm comm, MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_Recv_init_t) (void *context, void *buf, int count, MPI_Datatype datatype,
                                int source, int tag, MPI_Comm comm, MPI_Request * request);

int QMPI_Reduce(void *context, const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                MPI_Op op, int root, MPI_Comm comm) MPICH_API_PUBLIC;
typedef int (QMPI_Reduce_t) (void *context, const void *sendbuf, void *recvbuf, int count,
                             MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm);

int QMPI_Reduce_local(void *context, const void *inbuf, void *inoutbuf, int count,
                      MPI_Datatype datatype, MPI_Op op) MPICH_API_PUBLIC;
typedef int (QMPI_Reduce_local_t) (void *context, const void *inbuf, void *inoutbuf, int count,
                                   MPI_Datatype datatype, MPI_Op op);

int QMPI_Reduce_scatter(void *context, const void *sendbuf, void *recvbuf, const int recvcounts[],
                        MPI_Datatype datatype, MPI_Op op, MPI_Comm comm) MPICH_API_PUBLIC;
typedef int (QMPI_Reduce_scatter_t) (void *context, const void *sendbuf, void *recvbuf,
                                     const int recvcounts[], MPI_Datatype datatype, MPI_Op op,
                                     MPI_Comm comm);

int QMPI_Reduce_scatter_block(void *context, const void *sendbuf, void *recvbuf, int recvcount,
                              MPI_Datatype datatype, MPI_Op op, MPI_Comm comm) MPICH_API_PUBLIC;
typedef int (QMPI_Reduce_scatter_block_t) (void *context, const void *sendbuf, void *recvbuf,
                                           int recvcount, MPI_Datatype datatype, MPI_Op op,
                                           MPI_Comm comm);

int QMPI_Request_free(void *context, MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_Request_free_t) (void *context, MPI_Request * request);

int QMPI_Request_get_status(void *context, MPI_Request request, int *flag,
                            MPI_Status * status) MPICH_API_PUBLIC;
typedef int (QMPI_Request_get_status_t) (void *context, MPI_Request request, int *flag,
                                         MPI_Status * status);

int QMPI_Rget(void *context, void *origin_addr, int origin_count, MPI_Datatype origin_datatype,
              int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype,
              MPI_Win win, MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_Rget_t) (void *context, void *origin_addr, int origin_count,
                           MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp,
                           int target_count, MPI_Datatype target_datatype, MPI_Win win,
                           MPI_Request * request);

int QMPI_Rget_accumulate(void *context, const void *origin_addr, int origin_count,
                         MPI_Datatype origin_datatype, void *result_addr, int result_count,
                         MPI_Datatype result_datatype, int target_rank, MPI_Aint target_disp,
                         int target_count, MPI_Datatype target_datatype, MPI_Op op, MPI_Win win,
                         MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_Rget_accumulate_t) (void *context, const void *origin_addr, int origin_count,
                                      MPI_Datatype origin_datatype, void *result_addr,
                                      int result_count, MPI_Datatype result_datatype,
                                      int target_rank, MPI_Aint target_disp, int target_count,
                                      MPI_Datatype target_datatype, MPI_Op op, MPI_Win win,
                                      MPI_Request * request);

int QMPI_Rput(void *context, const void *origin_addr, int origin_count,
              MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp, int target_count,
              MPI_Datatype target_datatype, MPI_Win win, MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_Rput_t) (void *context, const void *origin_addr, int origin_count,
                           MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp,
                           int target_count, MPI_Datatype target_datatype, MPI_Win win,
                           MPI_Request * request);

int QMPI_Rsend(void *context, const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
               MPI_Comm comm) MPICH_API_PUBLIC;
typedef int (QMPI_Rsend_t) (void *context, const void *buf, int count, MPI_Datatype datatype,
                            int dest, int tag, MPI_Comm comm);

int QMPI_Rsend_init(void *context, const void *buf, int count, MPI_Datatype datatype, int dest,
                    int tag, MPI_Comm comm, MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_Rsend_init_t) (void *context, const void *buf, int count, MPI_Datatype datatype,
                                 int dest, int tag, MPI_Comm comm, MPI_Request * request);

int QMPI_Scan(void *context, const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
              MPI_Op op, MPI_Comm comm) MPICH_API_PUBLIC;
typedef int (QMPI_Scan_t) (void *context, const void *sendbuf, void *recvbuf, int count,
                           MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);

int QMPI_Scatter(void *context, const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                 void *recvbuf, int recvcount, MPI_Datatype recvtype, int root,
                 MPI_Comm comm) MPICH_API_PUBLIC;
typedef int (QMPI_Scatter_t) (void *context, const void *sendbuf, int sendcount,
                              MPI_Datatype sendtype, void *recvbuf, int recvcount,
                              MPI_Datatype recvtype, int root, MPI_Comm comm);

int QMPI_Scatterv(void *context, const void *sendbuf, const int sendcounts[], const int displs[],
                  MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype,
                  int root, MPI_Comm comm) MPICH_API_PUBLIC;
typedef int (QMPI_Scatterv_t) (void *context, const void *sendbuf, const int sendcounts[],
                               const int displs[], MPI_Datatype sendtype, void *recvbuf,
                               int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm);

int QMPI_Send(void *context, const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
              MPI_Comm comm) MPICH_API_PUBLIC;
typedef int (QMPI_Send_t) (void *context, const void *buf, int count, MPI_Datatype datatype,
                           int dest, int tag, MPI_Comm comm);

int QMPI_Send_init(void *context, const void *buf, int count, MPI_Datatype datatype, int dest,
                   int tag, MPI_Comm comm, MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_Send_init_t) (void *context, const void *buf, int count, MPI_Datatype datatype,
                                int dest, int tag, MPI_Comm comm, MPI_Request * request);

int QMPI_Sendrecv(void *context, const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                  int dest, int sendtag, void *recvbuf, int recvcount, MPI_Datatype recvtype,
                  int source, int recvtag, MPI_Comm comm, MPI_Status * status) MPICH_API_PUBLIC;
typedef int (QMPI_Sendrecv_t) (void *context, const void *sendbuf, int sendcount,
                               MPI_Datatype sendtype, int dest, int sendtag, void *recvbuf,
                               int recvcount, MPI_Datatype recvtype, int source, int recvtag,
                               MPI_Comm comm, MPI_Status * status);

int QMPI_Sendrecv_replace(void *context, void *buf, int count, MPI_Datatype datatype, int dest,
                          int sendtag, int source, int recvtag, MPI_Comm comm,
                          MPI_Status * status) MPICH_API_PUBLIC;
typedef int (QMPI_Sendrecv_replace_t) (void *context, void *buf, int count, MPI_Datatype datatype,
                                       int dest, int sendtag, int source, int recvtag,
                                       MPI_Comm comm, MPI_Status * status);

int QMPI_Ssend(void *context, const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
               MPI_Comm comm) MPICH_API_PUBLIC;
typedef int (QMPI_Ssend_t) (void *context, const void *buf, int count, MPI_Datatype datatype,
                            int dest, int tag, MPI_Comm comm);

int QMPI_Ssend_init(void *context, const void *buf, int count, MPI_Datatype datatype, int dest,
                    int tag, MPI_Comm comm, MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_Ssend_init_t) (void *context, const void *buf, int count, MPI_Datatype datatype,
                                 int dest, int tag, MPI_Comm comm, MPI_Request * request);

int QMPI_Start(void *context, MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_Start_t) (void *context, MPI_Request * request);

int QMPI_Startall(void *context, int count, MPI_Request array_of_requests[]) MPICH_API_PUBLIC;
typedef int (QMPI_Startall_t) (void *context, int count, MPI_Request array_of_requests[]);

int QMPI_Status_set_cancelled(void *context, MPI_Status * status, int flag) MPICH_API_PUBLIC;
typedef int (QMPI_Status_set_cancelled_t) (void *context, MPI_Status * status, int flag);

int QMPI_Status_set_elements(void *context, MPI_Status * status, MPI_Datatype datatype,
                             int count) MPICH_API_PUBLIC;
typedef int (QMPI_Status_set_elements_t) (void *context, MPI_Status * status, MPI_Datatype datatype,
                                          int count);

int QMPI_Status_set_elements_x(void *context, MPI_Status * status, MPI_Datatype datatype,
                               MPI_Count count) MPICH_API_PUBLIC;
typedef int (QMPI_Status_set_elements_x_t) (void *context, MPI_Status * status,
                                            MPI_Datatype datatype, MPI_Count count);

int QMPI_T_category_changed(void *context, int *stamp) MPICH_API_PUBLIC;
typedef int (QMPI_T_category_changed_t) (void *context, int *stamp);

int QMPI_T_category_get_categories(void *context, int cat_index, int len,
                                   int indices[]) MPICH_API_PUBLIC;
typedef int (QMPI_T_category_get_categories_t) (void *context, int cat_index, int len,
                                                int indices[]);

int QMPI_T_category_get_cvars(void *context, int cat_index, int len,
                              int indices[]) MPICH_API_PUBLIC;
typedef int (QMPI_T_category_get_cvars_t) (void *context, int cat_index, int len, int indices[]);

int QMPI_T_category_get_index(void *context, const char *name, int *cat_index) MPICH_API_PUBLIC;
typedef int (QMPI_T_category_get_index_t) (void *context, const char *name, int *cat_index);

int QMPI_T_category_get_info(void *context, int cat_index, char *name, int *name_len, char *desc,
                             int *desc_len, int *num_cvars, int *num_pvars,
                             int *num_categories) MPICH_API_PUBLIC;
typedef int (QMPI_T_category_get_info_t) (void *context, int cat_index, char *name, int *name_len,
                                          char *desc, int *desc_len, int *num_cvars, int *num_pvars,
                                          int *num_categories);

int QMPI_T_category_get_num(void *context, int *num_cat) MPICH_API_PUBLIC;
typedef int (QMPI_T_category_get_num_t) (void *context, int *num_cat);

int QMPI_T_category_get_pvars(void *context, int cat_index, int len,
                              int indices[]) MPICH_API_PUBLIC;
typedef int (QMPI_T_category_get_pvars_t) (void *context, int cat_index, int len, int indices[]);

int QMPI_T_cvar_get_index(void *context, const char *name, int *cvar_index) MPICH_API_PUBLIC;
typedef int (QMPI_T_cvar_get_index_t) (void *context, const char *name, int *cvar_index);

int QMPI_T_cvar_get_info(void *context, int cvar_index, char *name, int *name_len, int *verbosity,
                         MPI_Datatype * datatype, MPI_T_enum * enumtype, char *desc, int *desc_len,
                         int *bind, int *scope) MPICH_API_PUBLIC;
typedef int (QMPI_T_cvar_get_info_t) (void *context, int cvar_index, char *name, int *name_len,
                                      int *verbosity, MPI_Datatype * datatype,
                                      MPI_T_enum * enumtype, char *desc, int *desc_len, int *bind,
                                      int *scope);

int QMPI_T_cvar_get_num(void *context, int *num_cvar) MPICH_API_PUBLIC;
typedef int (QMPI_T_cvar_get_num_t) (void *context, int *num_cvar);

int QMPI_T_cvar_handle_alloc(void *context, int cvar_index, void *obj_handle,
                             MPI_T_cvar_handle * handle, int *count) MPICH_API_PUBLIC;
typedef int (QMPI_T_cvar_handle_alloc_t) (void *context, int cvar_index, void *obj_handle,
                                          MPI_T_cvar_handle * handle, int *count);

int QMPI_T_cvar_handle_free(void *context, MPI_T_cvar_handle * handle) MPICH_API_PUBLIC;
typedef int (QMPI_T_cvar_handle_free_t) (void *context, MPI_T_cvar_handle * handle);

int QMPI_T_cvar_read(void *context, MPI_T_cvar_handle handle, void *buf) MPICH_API_PUBLIC;
typedef int (QMPI_T_cvar_read_t) (void *context, MPI_T_cvar_handle handle, void *buf);

int QMPI_T_cvar_write(void *context, MPI_T_cvar_handle handle, const void *buf) MPICH_API_PUBLIC;
typedef int (QMPI_T_cvar_write_t) (void *context, MPI_T_cvar_handle handle, const void *buf);

int QMPI_T_enum_get_info(void *context, MPI_T_enum enumtype, int *num, char *name,
                         int *name_len) MPICH_API_PUBLIC;
typedef int (QMPI_T_enum_get_info_t) (void *context, MPI_T_enum enumtype, int *num, char *name,
                                      int *name_len);

int QMPI_T_enum_get_item(void *context, MPI_T_enum enumtype, int index, int *value, char *name,
                         int *name_len) MPICH_API_PUBLIC;
typedef int (QMPI_T_enum_get_item_t) (void *context, MPI_T_enum enumtype, int index, int *value,
                                      char *name, int *name_len);

int QMPI_T_finalize(void *context) MPICH_API_PUBLIC;
typedef int (QMPI_T_finalize_t) (void *context);

int QMPI_T_init_thread(void *context, int required, int *provided) MPICH_API_PUBLIC;
typedef int (QMPI_T_init_thread_t) (void *context, int required, int *provided);

int QMPI_T_pvar_get_index(void *context, const char *name, int var_class,
                          int *pvar_index) MPICH_API_PUBLIC;
typedef int (QMPI_T_pvar_get_index_t) (void *context, const char *name, int var_class,
                                       int *pvar_index);

int QMPI_T_pvar_get_info(void *context, int pvar_index, char *name, int *name_len, int *verbosity,
                         int *var_class, MPI_Datatype * datatype, MPI_T_enum * enumtype, char *desc,
                         int *desc_len, int *bind, int *readonly, int *continuous,
                         int *atomic) MPICH_API_PUBLIC;
typedef int (QMPI_T_pvar_get_info_t) (void *context, int pvar_index, char *name, int *name_len,
                                      int *verbosity, int *var_class, MPI_Datatype * datatype,
                                      MPI_T_enum * enumtype, char *desc, int *desc_len, int *bind,
                                      int *readonly, int *continuous, int *atomic);

int QMPI_T_pvar_get_num(void *context, int *num_pvar) MPICH_API_PUBLIC;
typedef int (QMPI_T_pvar_get_num_t) (void *context, int *num_pvar);

int QMPI_T_pvar_handle_alloc(void *context, MPI_T_pvar_session session, int pvar_index,
                             void *obj_handle, MPI_T_pvar_handle * handle,
                             int *count) MPICH_API_PUBLIC;
typedef int (QMPI_T_pvar_handle_alloc_t) (void *context, MPI_T_pvar_session session, int pvar_index,
                                          void *obj_handle, MPI_T_pvar_handle * handle, int *count);

int QMPI_T_pvar_handle_free(void *context, MPI_T_pvar_session session,
                            MPI_T_pvar_handle * handle) MPICH_API_PUBLIC;
typedef int (QMPI_T_pvar_handle_free_t) (void *context, MPI_T_pvar_session session,
                                         MPI_T_pvar_handle * handle);

int QMPI_T_pvar_read(void *context, MPI_T_pvar_session session, MPI_T_pvar_handle handle,
                     void *buf) MPICH_API_PUBLIC;
typedef int (QMPI_T_pvar_read_t) (void *context, MPI_T_pvar_session session,
                                  MPI_T_pvar_handle handle, void *buf);

int QMPI_T_pvar_readreset(void *context, MPI_T_pvar_session session, MPI_T_pvar_handle handle,
                          void *buf) MPICH_API_PUBLIC;
typedef int (QMPI_T_pvar_readreset_t) (void *context, MPI_T_pvar_session session,
                                       MPI_T_pvar_handle handle, void *buf);

int QMPI_T_pvar_reset(void *context, MPI_T_pvar_session session,
                      MPI_T_pvar_handle handle) MPICH_API_PUBLIC;
typedef int (QMPI_T_pvar_reset_t) (void *context, MPI_T_pvar_session session,
                                   MPI_T_pvar_handle handle);

int QMPI_T_pvar_session_create(void *context, MPI_T_pvar_session * session) MPICH_API_PUBLIC;
typedef int (QMPI_T_pvar_session_create_t) (void *context, MPI_T_pvar_session * session);

int QMPI_T_pvar_session_free(void *context, MPI_T_pvar_session * session) MPICH_API_PUBLIC;
typedef int (QMPI_T_pvar_session_free_t) (void *context, MPI_T_pvar_session * session);

int QMPI_T_pvar_start(void *context, MPI_T_pvar_session session,
                      MPI_T_pvar_handle handle) MPICH_API_PUBLIC;
typedef int (QMPI_T_pvar_start_t) (void *context, MPI_T_pvar_session session,
                                   MPI_T_pvar_handle handle);

int QMPI_T_pvar_stop(void *context, MPI_T_pvar_session session,
                     MPI_T_pvar_handle handle) MPICH_API_PUBLIC;
typedef int (QMPI_T_pvar_stop_t) (void *context, MPI_T_pvar_session session,
                                  MPI_T_pvar_handle handle);

int QMPI_T_pvar_write(void *context, MPI_T_pvar_session session, MPI_T_pvar_handle handle,
                      const void *buf) MPICH_API_PUBLIC;
typedef int (QMPI_T_pvar_write_t) (void *context, MPI_T_pvar_session session,
                                   MPI_T_pvar_handle handle, const void *buf);

int QMPI_Test(void *context, MPI_Request * request, int *flag,
              MPI_Status * status) MPICH_API_PUBLIC;
typedef int (QMPI_Test_t) (void *context, MPI_Request * request, int *flag, MPI_Status * status);

int QMPI_Test_cancelled(void *context, const MPI_Status * status, int *flag) MPICH_API_PUBLIC;
typedef int (QMPI_Test_cancelled_t) (void *context, const MPI_Status * status, int *flag);

int QMPI_Testall(void *context, int count, MPI_Request array_of_requests[], int *flag,
                 MPI_Status array_of_statuses[]) MPICH_API_PUBLIC;
typedef int (QMPI_Testall_t) (void *context, int count, MPI_Request array_of_requests[], int *flag,
                              MPI_Status array_of_statuses[]);

int QMPI_Testany(void *context, int count, MPI_Request array_of_requests[], int *index, int *flag,
                 MPI_Status * status) MPICH_API_PUBLIC;
typedef int (QMPI_Testany_t) (void *context, int count, MPI_Request array_of_requests[], int *index,
                              int *flag, MPI_Status * status);

int QMPI_Testsome(void *context, int incount, MPI_Request array_of_requests[], int *outcount,
                  int array_of_indices[], MPI_Status array_of_statuses[]) MPICH_API_PUBLIC;
typedef int (QMPI_Testsome_t) (void *context, int incount, MPI_Request array_of_requests[],
                               int *outcount, int array_of_indices[],
                               MPI_Status array_of_statuses[]);

int QMPI_Topo_test(void *context, MPI_Comm comm, int *status) MPICH_API_PUBLIC;
typedef int (QMPI_Topo_test_t) (void *context, MPI_Comm comm, int *status);

int QMPI_Type_commit(void *context, MPI_Datatype * datatype) MPICH_API_PUBLIC;
typedef int (QMPI_Type_commit_t) (void *context, MPI_Datatype * datatype);

int QMPI_Type_contiguous(void *context, int count, MPI_Datatype oldtype,
                         MPI_Datatype * newtype) MPICH_API_PUBLIC;
typedef int (QMPI_Type_contiguous_t) (void *context, int count, MPI_Datatype oldtype,
                                      MPI_Datatype * newtype);

int QMPI_Type_create_darray(void *context, int size, int rank, int ndims,
                            const int array_of_gsizes[], const int array_of_distribs[],
                            const int array_of_dargs[], const int array_of_psizes[], int order,
                            MPI_Datatype oldtype, MPI_Datatype * newtype) MPICH_API_PUBLIC;
typedef int (QMPI_Type_create_darray_t) (void *context, int size, int rank, int ndims,
                                         const int array_of_gsizes[], const int array_of_distribs[],
                                         const int array_of_dargs[], const int array_of_psizes[],
                                         int order, MPI_Datatype oldtype, MPI_Datatype * newtype);

int QMPI_Type_create_f90_complex(void *context, int p, int r,
                                 MPI_Datatype * newtype) MPICH_API_PUBLIC;
typedef int (QMPI_Type_create_f90_complex_t) (void *context, int p, int r, MPI_Datatype * newtype);

int QMPI_Type_create_f90_integer(void *context, int r, MPI_Datatype * newtype) MPICH_API_PUBLIC;
typedef int (QMPI_Type_create_f90_integer_t) (void *context, int r, MPI_Datatype * newtype);

int QMPI_Type_create_f90_real(void *context, int p, int r, MPI_Datatype * newtype) MPICH_API_PUBLIC;
typedef int (QMPI_Type_create_f90_real_t) (void *context, int p, int r, MPI_Datatype * newtype);

int QMPI_Type_create_hindexed(void *context, int count, const int array_of_blocklengths[],
                              const MPI_Aint array_of_displacements[], MPI_Datatype oldtype,
                              MPI_Datatype * newtype) MPICH_API_PUBLIC;
typedef int (QMPI_Type_create_hindexed_t) (void *context, int count,
                                           const int array_of_blocklengths[],
                                           const MPI_Aint array_of_displacements[],
                                           MPI_Datatype oldtype, MPI_Datatype * newtype);

int QMPI_Type_create_hindexed_block(void *context, int count, int blocklength,
                                    const MPI_Aint array_of_displacements[], MPI_Datatype oldtype,
                                    MPI_Datatype * newtype) MPICH_API_PUBLIC;
typedef int (QMPI_Type_create_hindexed_block_t) (void *context, int count, int blocklength,
                                                 const MPI_Aint array_of_displacements[],
                                                 MPI_Datatype oldtype, MPI_Datatype * newtype);

int QMPI_Type_create_hvector(void *context, int count, int blocklength, MPI_Aint stride,
                             MPI_Datatype oldtype, MPI_Datatype * newtype) MPICH_API_PUBLIC;
typedef int (QMPI_Type_create_hvector_t) (void *context, int count, int blocklength,
                                          MPI_Aint stride, MPI_Datatype oldtype,
                                          MPI_Datatype * newtype);

int QMPI_Type_create_indexed_block(void *context, int count, int blocklength,
                                   const int array_of_displacements[], MPI_Datatype oldtype,
                                   MPI_Datatype * newtype) MPICH_API_PUBLIC;
typedef int (QMPI_Type_create_indexed_block_t) (void *context, int count, int blocklength,
                                                const int array_of_displacements[],
                                                MPI_Datatype oldtype, MPI_Datatype * newtype);

int QMPI_Type_create_keyval(void *context, MPI_Type_copy_attr_function * type_copy_attr_fn,
                            MPI_Type_delete_attr_function * type_delete_attr_fn, int *type_keyval,
                            void *extra_state) MPICH_API_PUBLIC;
typedef int (QMPI_Type_create_keyval_t) (void *context,
                                         MPI_Type_copy_attr_function * type_copy_attr_fn,
                                         MPI_Type_delete_attr_function * type_delete_attr_fn,
                                         int *type_keyval, void *extra_state);

int QMPI_Type_create_resized(void *context, MPI_Datatype oldtype, MPI_Aint lb, MPI_Aint extent,
                             MPI_Datatype * newtype) MPICH_API_PUBLIC;
typedef int (QMPI_Type_create_resized_t) (void *context, MPI_Datatype oldtype, MPI_Aint lb,
                                          MPI_Aint extent, MPI_Datatype * newtype);

int QMPI_Type_create_struct(void *context, int count, const int array_of_blocklengths[],
                            const MPI_Aint array_of_displacements[],
                            const MPI_Datatype array_of_types[],
                            MPI_Datatype * newtype) MPICH_API_PUBLIC;
typedef int (QMPI_Type_create_struct_t) (void *context, int count,
                                         const int array_of_blocklengths[],
                                         const MPI_Aint array_of_displacements[],
                                         const MPI_Datatype array_of_types[],
                                         MPI_Datatype * newtype);

int QMPI_Type_create_subarray(void *context, int ndims, const int array_of_sizes[],
                              const int array_of_subsizes[], const int array_of_starts[], int order,
                              MPI_Datatype oldtype, MPI_Datatype * newtype) MPICH_API_PUBLIC;
typedef int (QMPI_Type_create_subarray_t) (void *context, int ndims, const int array_of_sizes[],
                                           const int array_of_subsizes[],
                                           const int array_of_starts[], int order,
                                           MPI_Datatype oldtype, MPI_Datatype * newtype);

int QMPI_Type_delete_attr(void *context, MPI_Datatype datatype, int type_keyval) MPICH_API_PUBLIC;
typedef int (QMPI_Type_delete_attr_t) (void *context, MPI_Datatype datatype, int type_keyval);

int QMPI_Type_dup(void *context, MPI_Datatype oldtype, MPI_Datatype * newtype) MPICH_API_PUBLIC;
typedef int (QMPI_Type_dup_t) (void *context, MPI_Datatype oldtype, MPI_Datatype * newtype);

int QMPI_Type_free(void *context, MPI_Datatype * datatype) MPICH_API_PUBLIC;
typedef int (QMPI_Type_free_t) (void *context, MPI_Datatype * datatype);

int QMPI_Type_free_keyval(void *context, int *type_keyval) MPICH_API_PUBLIC;
typedef int (QMPI_Type_free_keyval_t) (void *context, int *type_keyval);

int QMPI_Type_get_attr(void *context, MPI_Datatype datatype, int type_keyval, void *attribute_val,
                       int *flag) MPICH_API_PUBLIC;
typedef int (QMPI_Type_get_attr_t) (void *context, MPI_Datatype datatype, int type_keyval,
                                    void *attribute_val, int *flag);

int QMPI_Type_get_contents(void *context, MPI_Datatype datatype, int max_integers,
                           int max_addresses, int max_datatypes, int array_of_integers[],
                           MPI_Aint array_of_addresses[],
                           MPI_Datatype array_of_datatypes[]) MPICH_API_PUBLIC;
typedef int (QMPI_Type_get_contents_t) (void *context, MPI_Datatype datatype, int max_integers,
                                        int max_addresses, int max_datatypes,
                                        int array_of_integers[], MPI_Aint array_of_addresses[],
                                        MPI_Datatype array_of_datatypes[]);

int QMPI_Type_get_envelope(void *context, MPI_Datatype datatype, int *num_integers,
                           int *num_addresses, int *num_datatypes, int *combiner) MPICH_API_PUBLIC;
typedef int (QMPI_Type_get_envelope_t) (void *context, MPI_Datatype datatype, int *num_integers,
                                        int *num_addresses, int *num_datatypes, int *combiner);

int QMPI_Type_get_extent(void *context, MPI_Datatype datatype, MPI_Aint * lb,
                         MPI_Aint * extent) MPICH_API_PUBLIC;
typedef int (QMPI_Type_get_extent_t) (void *context, MPI_Datatype datatype, MPI_Aint * lb,
                                      MPI_Aint * extent);

int QMPI_Type_get_extent_x(void *context, MPI_Datatype datatype, MPI_Count * lb,
                           MPI_Count * extent) MPICH_API_PUBLIC;
typedef int (QMPI_Type_get_extent_x_t) (void *context, MPI_Datatype datatype, MPI_Count * lb,
                                        MPI_Count * extent);

int QMPI_Type_get_name(void *context, MPI_Datatype datatype, char *type_name,
                       int *resultlen) MPICH_API_PUBLIC;
typedef int (QMPI_Type_get_name_t) (void *context, MPI_Datatype datatype, char *type_name,
                                    int *resultlen);

int QMPI_Type_get_true_extent(void *context, MPI_Datatype datatype, MPI_Aint * true_lb,
                              MPI_Aint * true_extent) MPICH_API_PUBLIC;
typedef int (QMPI_Type_get_true_extent_t) (void *context, MPI_Datatype datatype, MPI_Aint * true_lb,
                                           MPI_Aint * true_extent);

int QMPI_Type_get_true_extent_x(void *context, MPI_Datatype datatype, MPI_Count * true_lb,
                                MPI_Count * true_extent) MPICH_API_PUBLIC;
typedef int (QMPI_Type_get_true_extent_x_t) (void *context, MPI_Datatype datatype,
                                             MPI_Count * true_lb, MPI_Count * true_extent);

int QMPI_Type_indexed(void *context, int count, const int array_of_blocklengths[],
                      const int array_of_displacements[], MPI_Datatype oldtype,
                      MPI_Datatype * newtype) MPICH_API_PUBLIC;
typedef int (QMPI_Type_indexed_t) (void *context, int count, const int array_of_blocklengths[],
                                   const int array_of_displacements[], MPI_Datatype oldtype,
                                   MPI_Datatype * newtype);

int QMPI_Type_match_size(void *context, int typeclass, int size,
                         MPI_Datatype * datatype) MPICH_API_PUBLIC;
typedef int (QMPI_Type_match_size_t) (void *context, int typeclass, int size,
                                      MPI_Datatype * datatype);

int QMPI_Type_set_attr(void *context, MPI_Datatype datatype, int type_keyval,
                       void *attribute_val) MPICH_API_PUBLIC;
typedef int (QMPI_Type_set_attr_t) (void *context, MPI_Datatype datatype, int type_keyval,
                                    void *attribute_val);

int QMPI_Type_set_name(void *context, MPI_Datatype datatype,
                       const char *type_name) MPICH_API_PUBLIC;
typedef int (QMPI_Type_set_name_t) (void *context, MPI_Datatype datatype, const char *type_name);

int QMPI_Type_size(void *context, MPI_Datatype datatype, int *size) MPICH_API_PUBLIC;
typedef int (QMPI_Type_size_t) (void *context, MPI_Datatype datatype, int *size);

int QMPI_Type_size_x(void *context, MPI_Datatype datatype, MPI_Count * size) MPICH_API_PUBLIC;
typedef int (QMPI_Type_size_x_t) (void *context, MPI_Datatype datatype, MPI_Count * size);

int QMPI_Type_vector(void *context, int count, int blocklength, int stride, MPI_Datatype oldtype,
                     MPI_Datatype * newtype) MPICH_API_PUBLIC;
typedef int (QMPI_Type_vector_t) (void *context, int count, int blocklength, int stride,
                                  MPI_Datatype oldtype, MPI_Datatype * newtype);

int QMPI_Unpack(void *context, const void *inbuf, int insize, int *position, void *outbuf,
                int outcount, MPI_Datatype datatype, MPI_Comm comm) MPICH_API_PUBLIC;
typedef int (QMPI_Unpack_t) (void *context, const void *inbuf, int insize, int *position,
                             void *outbuf, int outcount, MPI_Datatype datatype, MPI_Comm comm);

int QMPI_Unpack_external(void *context, const char datarep[], const void *inbuf, MPI_Aint insize,
                         MPI_Aint * position, void *outbuf, int outcount,
                         MPI_Datatype datatype) MPICH_API_PUBLIC;
typedef int (QMPI_Unpack_external_t) (void *context, const char datarep[], const void *inbuf,
                                      MPI_Aint insize, MPI_Aint * position, void *outbuf,
                                      int outcount, MPI_Datatype datatype);

int QMPI_Unpublish_name(void *context, const char *service_name, MPI_Info info,
                        const char *port_name) MPICH_API_PUBLIC;
typedef int (QMPI_Unpublish_name_t) (void *context, const char *service_name, MPI_Info info,
                                     const char *port_name);

int QMPI_Wait(void *context, MPI_Request * request, MPI_Status * status) MPICH_API_PUBLIC;
typedef int (QMPI_Wait_t) (void *context, MPI_Request * request, MPI_Status * status);

int QMPI_Waitall(void *context, int count, MPI_Request array_of_requests[],
                 MPI_Status array_of_statuses[]) MPICH_API_PUBLIC;
typedef int (QMPI_Waitall_t) (void *context, int count, MPI_Request array_of_requests[],
                              MPI_Status array_of_statuses[]);

int QMPI_Waitany(void *context, int count, MPI_Request array_of_requests[], int *index,
                 MPI_Status * status) MPICH_API_PUBLIC;
typedef int (QMPI_Waitany_t) (void *context, int count, MPI_Request array_of_requests[], int *index,
                              MPI_Status * status);

int QMPI_Waitsome(void *context, int incount, MPI_Request array_of_requests[], int *outcount,
                  int array_of_indices[], MPI_Status array_of_statuses[]) MPICH_API_PUBLIC;
typedef int (QMPI_Waitsome_t) (void *context, int incount, MPI_Request array_of_requests[],
                               int *outcount, int array_of_indices[],
                               MPI_Status array_of_statuses[]);

int QMPI_Win_allocate(void *context, MPI_Aint size, int disp_unit, MPI_Info info, MPI_Comm comm,
                      void *baseptr, MPI_Win * win) MPICH_API_PUBLIC;
typedef int (QMPI_Win_allocate_t) (void *context, MPI_Aint size, int disp_unit, MPI_Info info,
                                   MPI_Comm comm, void *baseptr, MPI_Win * win);

int QMPI_Win_allocate_shared(void *context, MPI_Aint size, int disp_unit, MPI_Info info,
                             MPI_Comm comm, void *baseptr, MPI_Win * win) MPICH_API_PUBLIC;
typedef int (QMPI_Win_allocate_shared_t) (void *context, MPI_Aint size, int disp_unit,
                                          MPI_Info info, MPI_Comm comm, void *baseptr,
                                          MPI_Win * win);

int QMPI_Win_attach(void *context, MPI_Win win, void *base, MPI_Aint size) MPICH_API_PUBLIC;
typedef int (QMPI_Win_attach_t) (void *context, MPI_Win win, void *base, MPI_Aint size);

int QMPI_Win_call_errhandler(void *context, MPI_Win win, int errorcode) MPICH_API_PUBLIC;
typedef int (QMPI_Win_call_errhandler_t) (void *context, MPI_Win win, int errorcode);

int QMPI_Win_complete(void *context, MPI_Win win) MPICH_API_PUBLIC;
typedef int (QMPI_Win_complete_t) (void *context, MPI_Win win);

int QMPI_Win_create(void *context, void *base, MPI_Aint size, int disp_unit, MPI_Info info,
                    MPI_Comm comm, MPI_Win * win) MPICH_API_PUBLIC;
typedef int (QMPI_Win_create_t) (void *context, void *base, MPI_Aint size, int disp_unit,
                                 MPI_Info info, MPI_Comm comm, MPI_Win * win);

int QMPI_Win_create_dynamic(void *context, MPI_Info info, MPI_Comm comm,
                            MPI_Win * win) MPICH_API_PUBLIC;
typedef int (QMPI_Win_create_dynamic_t) (void *context, MPI_Info info, MPI_Comm comm,
                                         MPI_Win * win);

int QMPI_Win_create_errhandler(void *context, MPI_Win_errhandler_function * win_errhandler_fn,
                               MPI_Errhandler * errhandler) MPICH_API_PUBLIC;
typedef int (QMPI_Win_create_errhandler_t) (void *context,
                                            MPI_Win_errhandler_function * win_errhandler_fn,
                                            MPI_Errhandler * errhandler);

int QMPI_Win_create_keyval(void *context, MPI_Win_copy_attr_function * win_copy_attr_fn,
                           MPI_Win_delete_attr_function * win_delete_attr_fn, int *win_keyval,
                           void *extra_state) MPICH_API_PUBLIC;
typedef int (QMPI_Win_create_keyval_t) (void *context,
                                        MPI_Win_copy_attr_function * win_copy_attr_fn,
                                        MPI_Win_delete_attr_function * win_delete_attr_fn,
                                        int *win_keyval, void *extra_state);

int QMPI_Win_delete_attr(void *context, MPI_Win win, int win_keyval) MPICH_API_PUBLIC;
typedef int (QMPI_Win_delete_attr_t) (void *context, MPI_Win win, int win_keyval);

int QMPI_Win_detach(void *context, MPI_Win win, const void *base) MPICH_API_PUBLIC;
typedef int (QMPI_Win_detach_t) (void *context, MPI_Win win, const void *base);

int QMPI_Win_fence(void *context, int assert, MPI_Win win) MPICH_API_PUBLIC;
typedef int (QMPI_Win_fence_t) (void *context, int assert, MPI_Win win);

int QMPI_Win_flush(void *context, int rank, MPI_Win win) MPICH_API_PUBLIC;
typedef int (QMPI_Win_flush_t) (void *context, int rank, MPI_Win win);

int QMPI_Win_flush_all(void *context, MPI_Win win) MPICH_API_PUBLIC;
typedef int (QMPI_Win_flush_all_t) (void *context, MPI_Win win);

int QMPI_Win_flush_local(void *context, int rank, MPI_Win win) MPICH_API_PUBLIC;
typedef int (QMPI_Win_flush_local_t) (void *context, int rank, MPI_Win win);

int QMPI_Win_flush_local_all(void *context, MPI_Win win) MPICH_API_PUBLIC;
typedef int (QMPI_Win_flush_local_all_t) (void *context, MPI_Win win);

int QMPI_Win_free(void *context, MPI_Win * win) MPICH_API_PUBLIC;
typedef int (QMPI_Win_free_t) (void *context, MPI_Win * win);

int QMPI_Win_free_keyval(void *context, int *win_keyval) MPICH_API_PUBLIC;
typedef int (QMPI_Win_free_keyval_t) (void *context, int *win_keyval);

int QMPI_Win_get_attr(void *context, MPI_Win win, int win_keyval, void *attribute_val,
                      int *flag) MPICH_API_PUBLIC;
typedef int (QMPI_Win_get_attr_t) (void *context, MPI_Win win, int win_keyval, void *attribute_val,
                                   int *flag);

int QMPI_Win_get_errhandler(void *context, MPI_Win win,
                            MPI_Errhandler * errhandler) MPICH_API_PUBLIC;
typedef int (QMPI_Win_get_errhandler_t) (void *context, MPI_Win win, MPI_Errhandler * errhandler);

int QMPI_Win_get_group(void *context, MPI_Win win, MPI_Group * group) MPICH_API_PUBLIC;
typedef int (QMPI_Win_get_group_t) (void *context, MPI_Win win, MPI_Group * group);

int QMPI_Win_get_info(void *context, MPI_Win win, MPI_Info * info_used) MPICH_API_PUBLIC;
typedef int (QMPI_Win_get_info_t) (void *context, MPI_Win win, MPI_Info * info_used);

int QMPI_Win_get_name(void *context, MPI_Win win, char *win_name, int *resultlen) MPICH_API_PUBLIC;
typedef int (QMPI_Win_get_name_t) (void *context, MPI_Win win, char *win_name, int *resultlen);

int QMPI_Win_lock(void *context, int lock_type, int rank, int assert, MPI_Win win) MPICH_API_PUBLIC;
typedef int (QMPI_Win_lock_t) (void *context, int lock_type, int rank, int assert, MPI_Win win);

int QMPI_Win_lock_all(void *context, int assert, MPI_Win win) MPICH_API_PUBLIC;
typedef int (QMPI_Win_lock_all_t) (void *context, int assert, MPI_Win win);

int QMPI_Win_post(void *context, MPI_Group group, int assert, MPI_Win win) MPICH_API_PUBLIC;
typedef int (QMPI_Win_post_t) (void *context, MPI_Group group, int assert, MPI_Win win);

int QMPI_Win_set_attr(void *context, MPI_Win win, int win_keyval,
                      void *attribute_val) MPICH_API_PUBLIC;
typedef int (QMPI_Win_set_attr_t) (void *context, MPI_Win win, int win_keyval, void *attribute_val);

int QMPI_Win_set_errhandler(void *context, MPI_Win win, MPI_Errhandler errhandler) MPICH_API_PUBLIC;
typedef int (QMPI_Win_set_errhandler_t) (void *context, MPI_Win win, MPI_Errhandler errhandler);

int QMPI_Win_set_info(void *context, MPI_Win win, MPI_Info info) MPICH_API_PUBLIC;
typedef int (QMPI_Win_set_info_t) (void *context, MPI_Win win, MPI_Info info);

int QMPI_Win_set_name(void *context, MPI_Win win, const char *win_name) MPICH_API_PUBLIC;
typedef int (QMPI_Win_set_name_t) (void *context, MPI_Win win, const char *win_name);

int QMPI_Win_shared_query(void *context, MPI_Win win, int rank, MPI_Aint * size, int *disp_unit,
                          void *baseptr) MPICH_API_PUBLIC;
typedef int (QMPI_Win_shared_query_t) (void *context, MPI_Win win, int rank, MPI_Aint * size,
                                       int *disp_unit, void *baseptr);

int QMPI_Win_start(void *context, MPI_Group group, int assert, MPI_Win win) MPICH_API_PUBLIC;
typedef int (QMPI_Win_start_t) (void *context, MPI_Group group, int assert, MPI_Win win);

int QMPI_Win_sync(void *context, MPI_Win win) MPICH_API_PUBLIC;
typedef int (QMPI_Win_sync_t) (void *context, MPI_Win win);

int QMPI_Win_test(void *context, MPI_Win win, int *flag) MPICH_API_PUBLIC;
typedef int (QMPI_Win_test_t) (void *context, MPI_Win win, int *flag);

int QMPI_Win_unlock(void *context, int rank, MPI_Win win) MPICH_API_PUBLIC;
typedef int (QMPI_Win_unlock_t) (void *context, int rank, MPI_Win win);

int QMPI_Win_unlock_all(void *context, MPI_Win win) MPICH_API_PUBLIC;
typedef int (QMPI_Win_unlock_all_t) (void *context, MPI_Win win);

int QMPI_Win_wait(void *context, MPI_Win win) MPICH_API_PUBLIC;
typedef int (QMPI_Win_wait_t) (void *context, MPI_Win win);

double QMPI_Wtick(void *context) MPICH_API_PUBLIC;
typedef double (QMPI_Wtick_t) (void *context);

double QMPI_Wtime(void *context) MPICH_API_PUBLIC;
typedef double (QMPI_Wtime_t) (void *context);

#define QMPI_MAX_TOOL_NAME_LENGTH 256

int QMPI_Register_tool_name(const char *tool_name,
                            void (*init_function_ptr) (int tool_id)) MPICH_API_PUBLIC;
int QMPI_Register_tool_storage(int tool_id, void *tool_storage) MPICH_API_PUBLIC;
int QMPI_Register_function(int calling_tool_id, enum QMPI_Functions_enum function_enum,
                           void (*function_ptr) (void)) MPICH_API_PUBLIC;

#include <stddef.h>
extern MPICH_API_PUBLIC void (**MPIR_QMPI_pointers) (void);
extern MPICH_API_PUBLIC void **MPIR_QMPI_storage;
static inline void QMPI_Get_function(int calling_tool_id, enum QMPI_Functions_enum function_enum,
                                     void (**function_ptr) (void), void **next_tool_context)
{
    for (int i = calling_tool_id - 1; i >= 0; i--) {
        if (MPIR_QMPI_pointers[i * MPI_LAST_FUNC_T + function_enum] != NULL) {
            *function_ptr = MPIR_QMPI_pointers[i * MPI_LAST_FUNC_T + function_enum];
            *next_tool_context = MPIR_QMPI_storage[i];
            return;
        }
    }
}
#endif /* QMPI_H */

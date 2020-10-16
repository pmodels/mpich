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

typedef struct {
    void *storage_stack;
} QMPI_Context;

int QMPI_Abort(QMPI_Context context, int tool_id, MPI_Comm comm, int errorcode) MPICH_API_PUBLIC;
typedef int (QMPI_Abort_t) (QMPI_Context context, int tool_id, MPI_Comm comm, int errorcode);

int QMPI_Accumulate(QMPI_Context context, int tool_id, const void *origin_addr, int origin_count,
                    MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp,
                    int target_count, MPI_Datatype target_datatype, MPI_Op op,
                    MPI_Win win) MPICH_API_PUBLIC;
typedef int (QMPI_Accumulate_t) (QMPI_Context context, int tool_id, const void *origin_addr,
                                 int origin_count, MPI_Datatype origin_datatype, int target_rank,
                                 MPI_Aint target_disp, int target_count,
                                 MPI_Datatype target_datatype, MPI_Op op, MPI_Win win);

int QMPI_Add_error_class(QMPI_Context context, int tool_id, int *errorclass) MPICH_API_PUBLIC;
typedef int (QMPI_Add_error_class_t) (QMPI_Context context, int tool_id, int *errorclass);

int QMPI_Add_error_code(QMPI_Context context, int tool_id, int errorclass,
                        int *errorcode) MPICH_API_PUBLIC;
typedef int (QMPI_Add_error_code_t) (QMPI_Context context, int tool_id, int errorclass,
                                     int *errorcode);

int QMPI_Add_error_string(QMPI_Context context, int tool_id, int errorcode,
                          const char *string) MPICH_API_PUBLIC;
typedef int (QMPI_Add_error_string_t) (QMPI_Context context, int tool_id, int errorcode,
                                       const char *string);

MPI_Aint QMPI_Aint_add(QMPI_Context context, int tool_id, MPI_Aint base,
                       MPI_Aint disp) MPICH_API_PUBLIC;
typedef MPI_Aint(QMPI_Aint_add_t) (QMPI_Context context, int tool_id, MPI_Aint base, MPI_Aint disp);

MPI_Aint QMPI_Aint_diff(QMPI_Context context, int tool_id, MPI_Aint addr1,
                        MPI_Aint addr2) MPICH_API_PUBLIC;
typedef MPI_Aint(QMPI_Aint_diff_t) (QMPI_Context context, int tool_id, MPI_Aint addr1,
                                    MPI_Aint addr2);

int QMPI_Allgather(QMPI_Context context, int tool_id, const void *sendbuf, int sendcount,
                   MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype,
                   MPI_Comm comm) MPICH_API_PUBLIC;
typedef int (QMPI_Allgather_t) (QMPI_Context context, int tool_id, const void *sendbuf,
                                int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                MPI_Datatype recvtype, MPI_Comm comm);

int QMPI_Allgatherv(QMPI_Context context, int tool_id, const void *sendbuf, int sendcount,
                    MPI_Datatype sendtype, void *recvbuf, const int recvcounts[],
                    const int displs[], MPI_Datatype recvtype, MPI_Comm comm) MPICH_API_PUBLIC;
typedef int (QMPI_Allgatherv_t) (QMPI_Context context, int tool_id, const void *sendbuf,
                                 int sendcount, MPI_Datatype sendtype, void *recvbuf,
                                 const int recvcounts[], const int displs[], MPI_Datatype recvtype,
                                 MPI_Comm comm);

int QMPI_Alloc_mem(QMPI_Context context, int tool_id, MPI_Aint size, MPI_Info info,
                   void *baseptr) MPICH_API_PUBLIC;
typedef int (QMPI_Alloc_mem_t) (QMPI_Context context, int tool_id, MPI_Aint size, MPI_Info info,
                                void *baseptr);

int QMPI_Allreduce(QMPI_Context context, int tool_id, const void *sendbuf, void *recvbuf, int count,
                   MPI_Datatype datatype, MPI_Op op, MPI_Comm comm) MPICH_API_PUBLIC;
typedef int (QMPI_Allreduce_t) (QMPI_Context context, int tool_id, const void *sendbuf,
                                void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op,
                                MPI_Comm comm);

int QMPI_Alltoall(QMPI_Context context, int tool_id, const void *sendbuf, int sendcount,
                  MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype,
                  MPI_Comm comm) MPICH_API_PUBLIC;
typedef int (QMPI_Alltoall_t) (QMPI_Context context, int tool_id, const void *sendbuf,
                               int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount,
                               MPI_Datatype recvtype, MPI_Comm comm);

int QMPI_Alltoallv(QMPI_Context context, int tool_id, const void *sendbuf, const int sendcounts[],
                   const int sdispls[], MPI_Datatype sendtype, void *recvbuf,
                   const int recvcounts[], const int rdispls[], MPI_Datatype recvtype,
                   MPI_Comm comm) MPICH_API_PUBLIC;
typedef int (QMPI_Alltoallv_t) (QMPI_Context context, int tool_id, const void *sendbuf,
                                const int sendcounts[], const int sdispls[], MPI_Datatype sendtype,
                                void *recvbuf, const int recvcounts[], const int rdispls[],
                                MPI_Datatype recvtype, MPI_Comm comm);

int QMPI_Alltoallw(QMPI_Context context, int tool_id, const void *sendbuf, const int sendcounts[],
                   const int sdispls[], const MPI_Datatype sendtypes[], void *recvbuf,
                   const int recvcounts[], const int rdispls[], const MPI_Datatype recvtypes[],
                   MPI_Comm comm) MPICH_API_PUBLIC;
typedef int (QMPI_Alltoallw_t) (QMPI_Context context, int tool_id, const void *sendbuf,
                                const int sendcounts[], const int sdispls[],
                                const MPI_Datatype sendtypes[], void *recvbuf,
                                const int recvcounts[], const int rdispls[],
                                const MPI_Datatype recvtypes[], MPI_Comm comm);

int QMPI_Attr_delete(QMPI_Context context, int tool_id, MPI_Comm comm, int keyval) MPICH_API_PUBLIC;
typedef int (QMPI_Attr_delete_t) (QMPI_Context context, int tool_id, MPI_Comm comm, int keyval);

int QMPI_Attr_get(QMPI_Context context, int tool_id, MPI_Comm comm, int keyval, void *attribute_val,
                  int *flag) MPICH_API_PUBLIC;
typedef int (QMPI_Attr_get_t) (QMPI_Context context, int tool_id, MPI_Comm comm, int keyval,
                               void *attribute_val, int *flag);

int QMPI_Attr_put(QMPI_Context context, int tool_id, MPI_Comm comm, int keyval,
                  void *attribute_val) MPICH_API_PUBLIC;
typedef int (QMPI_Attr_put_t) (QMPI_Context context, int tool_id, MPI_Comm comm, int keyval,
                               void *attribute_val);

int QMPI_Barrier(QMPI_Context context, int tool_id, MPI_Comm comm) MPICH_API_PUBLIC;
typedef int (QMPI_Barrier_t) (QMPI_Context context, int tool_id, MPI_Comm comm);

int QMPI_Bcast(QMPI_Context context, int tool_id, void *buffer, int count, MPI_Datatype datatype,
               int root, MPI_Comm comm) MPICH_API_PUBLIC;
typedef int (QMPI_Bcast_t) (QMPI_Context context, int tool_id, void *buffer, int count,
                            MPI_Datatype datatype, int root, MPI_Comm comm);

int QMPI_Bsend(QMPI_Context context, int tool_id, const void *buf, int count, MPI_Datatype datatype,
               int dest, int tag, MPI_Comm comm) MPICH_API_PUBLIC;
typedef int (QMPI_Bsend_t) (QMPI_Context context, int tool_id, const void *buf, int count,
                            MPI_Datatype datatype, int dest, int tag, MPI_Comm comm);

int QMPI_Bsend_init(QMPI_Context context, int tool_id, const void *buf, int count,
                    MPI_Datatype datatype, int dest, int tag, MPI_Comm comm,
                    MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_Bsend_init_t) (QMPI_Context context, int tool_id, const void *buf, int count,
                                 MPI_Datatype datatype, int dest, int tag, MPI_Comm comm,
                                 MPI_Request * request);

int QMPI_Buffer_attach(QMPI_Context context, int tool_id, void *buffer, int size) MPICH_API_PUBLIC;
typedef int (QMPI_Buffer_attach_t) (QMPI_Context context, int tool_id, void *buffer, int size);

int QMPI_Buffer_detach(QMPI_Context context, int tool_id, void *buffer_addr,
                       int *size) MPICH_API_PUBLIC;
typedef int (QMPI_Buffer_detach_t) (QMPI_Context context, int tool_id, void *buffer_addr,
                                    int *size);

int QMPI_Cancel(QMPI_Context context, int tool_id, MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_Cancel_t) (QMPI_Context context, int tool_id, MPI_Request * request);

int QMPI_Cart_coords(QMPI_Context context, int tool_id, MPI_Comm comm, int rank, int maxdims,
                     int coords[]) MPICH_API_PUBLIC;
typedef int (QMPI_Cart_coords_t) (QMPI_Context context, int tool_id, MPI_Comm comm, int rank,
                                  int maxdims, int coords[]);

int QMPI_Cart_create(QMPI_Context context, int tool_id, MPI_Comm comm_old, int ndims,
                     const int dims[], const int periods[], int reorder,
                     MPI_Comm * comm_cart) MPICH_API_PUBLIC;
typedef int (QMPI_Cart_create_t) (QMPI_Context context, int tool_id, MPI_Comm comm_old, int ndims,
                                  const int dims[], const int periods[], int reorder,
                                  MPI_Comm * comm_cart);

int QMPI_Cart_get(QMPI_Context context, int tool_id, MPI_Comm comm, int maxdims, int dims[],
                  int periods[], int coords[]) MPICH_API_PUBLIC;
typedef int (QMPI_Cart_get_t) (QMPI_Context context, int tool_id, MPI_Comm comm, int maxdims,
                               int dims[], int periods[], int coords[]);

int QMPI_Cart_map(QMPI_Context context, int tool_id, MPI_Comm comm, int ndims, const int dims[],
                  const int periods[], int *newrank) MPICH_API_PUBLIC;
typedef int (QMPI_Cart_map_t) (QMPI_Context context, int tool_id, MPI_Comm comm, int ndims,
                               const int dims[], const int periods[], int *newrank);

int QMPI_Cart_rank(QMPI_Context context, int tool_id, MPI_Comm comm, const int coords[],
                   int *rank) MPICH_API_PUBLIC;
typedef int (QMPI_Cart_rank_t) (QMPI_Context context, int tool_id, MPI_Comm comm,
                                const int coords[], int *rank);

int QMPI_Cart_shift(QMPI_Context context, int tool_id, MPI_Comm comm, int direction, int disp,
                    int *rank_source, int *rank_dest) MPICH_API_PUBLIC;
typedef int (QMPI_Cart_shift_t) (QMPI_Context context, int tool_id, MPI_Comm comm, int direction,
                                 int disp, int *rank_source, int *rank_dest);

int QMPI_Cart_sub(QMPI_Context context, int tool_id, MPI_Comm comm, const int remain_dims[],
                  MPI_Comm * newcomm) MPICH_API_PUBLIC;
typedef int (QMPI_Cart_sub_t) (QMPI_Context context, int tool_id, MPI_Comm comm,
                               const int remain_dims[], MPI_Comm * newcomm);

int QMPI_Cartdim_get(QMPI_Context context, int tool_id, MPI_Comm comm, int *ndims) MPICH_API_PUBLIC;
typedef int (QMPI_Cartdim_get_t) (QMPI_Context context, int tool_id, MPI_Comm comm, int *ndims);

int QMPI_Close_port(QMPI_Context context, int tool_id, const char *port_name) MPICH_API_PUBLIC;
typedef int (QMPI_Close_port_t) (QMPI_Context context, int tool_id, const char *port_name);

int QMPI_Comm_accept(QMPI_Context context, int tool_id, const char *port_name, MPI_Info info,
                     int root, MPI_Comm comm, MPI_Comm * newcomm) MPICH_API_PUBLIC;
typedef int (QMPI_Comm_accept_t) (QMPI_Context context, int tool_id, const char *port_name,
                                  MPI_Info info, int root, MPI_Comm comm, MPI_Comm * newcomm);

int QMPI_Comm_call_errhandler(QMPI_Context context, int tool_id, MPI_Comm comm,
                              int errorcode) MPICH_API_PUBLIC;
typedef int (QMPI_Comm_call_errhandler_t) (QMPI_Context context, int tool_id, MPI_Comm comm,
                                           int errorcode);

int QMPI_Comm_compare(QMPI_Context context, int tool_id, MPI_Comm comm1, MPI_Comm comm2,
                      int *result) MPICH_API_PUBLIC;
typedef int (QMPI_Comm_compare_t) (QMPI_Context context, int tool_id, MPI_Comm comm1,
                                   MPI_Comm comm2, int *result);

int QMPI_Comm_connect(QMPI_Context context, int tool_id, const char *port_name, MPI_Info info,
                      int root, MPI_Comm comm, MPI_Comm * newcomm) MPICH_API_PUBLIC;
typedef int (QMPI_Comm_connect_t) (QMPI_Context context, int tool_id, const char *port_name,
                                   MPI_Info info, int root, MPI_Comm comm, MPI_Comm * newcomm);

int QMPI_Comm_create(QMPI_Context context, int tool_id, MPI_Comm comm, MPI_Group group,
                     MPI_Comm * newcomm) MPICH_API_PUBLIC;
typedef int (QMPI_Comm_create_t) (QMPI_Context context, int tool_id, MPI_Comm comm, MPI_Group group,
                                  MPI_Comm * newcomm);

int QMPI_Comm_create_errhandler(QMPI_Context context, int tool_id,
                                MPI_Comm_errhandler_function * comm_errhandler_fn,
                                MPI_Errhandler * errhandler) MPICH_API_PUBLIC;
typedef int (QMPI_Comm_create_errhandler_t) (QMPI_Context context, int tool_id,
                                             MPI_Comm_errhandler_function * comm_errhandler_fn,
                                             MPI_Errhandler * errhandler);

int QMPI_Comm_create_group(QMPI_Context context, int tool_id, MPI_Comm comm, MPI_Group group,
                           int tag, MPI_Comm * newcomm) MPICH_API_PUBLIC;
typedef int (QMPI_Comm_create_group_t) (QMPI_Context context, int tool_id, MPI_Comm comm,
                                        MPI_Group group, int tag, MPI_Comm * newcomm);

int QMPI_Comm_create_keyval(QMPI_Context context, int tool_id,
                            MPI_Comm_copy_attr_function * comm_copy_attr_fn,
                            MPI_Comm_delete_attr_function * comm_delete_attr_fn, int *comm_keyval,
                            void *extra_state) MPICH_API_PUBLIC;
typedef int (QMPI_Comm_create_keyval_t) (QMPI_Context context, int tool_id,
                                         MPI_Comm_copy_attr_function * comm_copy_attr_fn,
                                         MPI_Comm_delete_attr_function * comm_delete_attr_fn,
                                         int *comm_keyval, void *extra_state);

int QMPI_Comm_delete_attr(QMPI_Context context, int tool_id, MPI_Comm comm,
                          int comm_keyval) MPICH_API_PUBLIC;
typedef int (QMPI_Comm_delete_attr_t) (QMPI_Context context, int tool_id, MPI_Comm comm,
                                       int comm_keyval);

int QMPI_Comm_disconnect(QMPI_Context context, int tool_id, MPI_Comm * comm) MPICH_API_PUBLIC;
typedef int (QMPI_Comm_disconnect_t) (QMPI_Context context, int tool_id, MPI_Comm * comm);

int QMPI_Comm_dup(QMPI_Context context, int tool_id, MPI_Comm comm,
                  MPI_Comm * newcomm) MPICH_API_PUBLIC;
typedef int (QMPI_Comm_dup_t) (QMPI_Context context, int tool_id, MPI_Comm comm,
                               MPI_Comm * newcomm);

int QMPI_Comm_dup_with_info(QMPI_Context context, int tool_id, MPI_Comm comm, MPI_Info info,
                            MPI_Comm * newcomm) MPICH_API_PUBLIC;
typedef int (QMPI_Comm_dup_with_info_t) (QMPI_Context context, int tool_id, MPI_Comm comm,
                                         MPI_Info info, MPI_Comm * newcomm);

int QMPI_Comm_free(QMPI_Context context, int tool_id, MPI_Comm * comm) MPICH_API_PUBLIC;
typedef int (QMPI_Comm_free_t) (QMPI_Context context, int tool_id, MPI_Comm * comm);

int QMPI_Comm_free_keyval(QMPI_Context context, int tool_id, int *comm_keyval) MPICH_API_PUBLIC;
typedef int (QMPI_Comm_free_keyval_t) (QMPI_Context context, int tool_id, int *comm_keyval);

int QMPI_Comm_get_attr(QMPI_Context context, int tool_id, MPI_Comm comm, int comm_keyval,
                       void *attribute_val, int *flag) MPICH_API_PUBLIC;
typedef int (QMPI_Comm_get_attr_t) (QMPI_Context context, int tool_id, MPI_Comm comm,
                                    int comm_keyval, void *attribute_val, int *flag);

int QMPI_Comm_get_errhandler(QMPI_Context context, int tool_id, MPI_Comm comm,
                             MPI_Errhandler * errhandler) MPICH_API_PUBLIC;
typedef int (QMPI_Comm_get_errhandler_t) (QMPI_Context context, int tool_id, MPI_Comm comm,
                                          MPI_Errhandler * errhandler);

int QMPI_Comm_get_info(QMPI_Context context, int tool_id, MPI_Comm comm,
                       MPI_Info * info_used) MPICH_API_PUBLIC;
typedef int (QMPI_Comm_get_info_t) (QMPI_Context context, int tool_id, MPI_Comm comm,
                                    MPI_Info * info_used);

int QMPI_Comm_get_name(QMPI_Context context, int tool_id, MPI_Comm comm, char *comm_name,
                       int *resultlen) MPICH_API_PUBLIC;
typedef int (QMPI_Comm_get_name_t) (QMPI_Context context, int tool_id, MPI_Comm comm,
                                    char *comm_name, int *resultlen);

int QMPI_Comm_get_parent(QMPI_Context context, int tool_id, MPI_Comm * parent) MPICH_API_PUBLIC;
typedef int (QMPI_Comm_get_parent_t) (QMPI_Context context, int tool_id, MPI_Comm * parent);

int QMPI_Comm_group(QMPI_Context context, int tool_id, MPI_Comm comm,
                    MPI_Group * group) MPICH_API_PUBLIC;
typedef int (QMPI_Comm_group_t) (QMPI_Context context, int tool_id, MPI_Comm comm,
                                 MPI_Group * group);

int QMPI_Comm_idup(QMPI_Context context, int tool_id, MPI_Comm comm, MPI_Comm * newcomm,
                   MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_Comm_idup_t) (QMPI_Context context, int tool_id, MPI_Comm comm,
                                MPI_Comm * newcomm, MPI_Request * request);

int QMPI_Comm_join(QMPI_Context context, int tool_id, int fd,
                   MPI_Comm * intercomm) MPICH_API_PUBLIC;
typedef int (QMPI_Comm_join_t) (QMPI_Context context, int tool_id, int fd, MPI_Comm * intercomm);

int QMPI_Comm_rank(QMPI_Context context, int tool_id, MPI_Comm comm, int *rank) MPICH_API_PUBLIC;
typedef int (QMPI_Comm_rank_t) (QMPI_Context context, int tool_id, MPI_Comm comm, int *rank);

int QMPI_Comm_remote_group(QMPI_Context context, int tool_id, MPI_Comm comm,
                           MPI_Group * group) MPICH_API_PUBLIC;
typedef int (QMPI_Comm_remote_group_t) (QMPI_Context context, int tool_id, MPI_Comm comm,
                                        MPI_Group * group);

int QMPI_Comm_remote_size(QMPI_Context context, int tool_id, MPI_Comm comm,
                          int *size) MPICH_API_PUBLIC;
typedef int (QMPI_Comm_remote_size_t) (QMPI_Context context, int tool_id, MPI_Comm comm, int *size);

int QMPI_Comm_set_attr(QMPI_Context context, int tool_id, MPI_Comm comm, int comm_keyval,
                       void *attribute_val) MPICH_API_PUBLIC;
typedef int (QMPI_Comm_set_attr_t) (QMPI_Context context, int tool_id, MPI_Comm comm,
                                    int comm_keyval, void *attribute_val);

int QMPI_Comm_set_errhandler(QMPI_Context context, int tool_id, MPI_Comm comm,
                             MPI_Errhandler errhandler) MPICH_API_PUBLIC;
typedef int (QMPI_Comm_set_errhandler_t) (QMPI_Context context, int tool_id, MPI_Comm comm,
                                          MPI_Errhandler errhandler);

int QMPI_Comm_set_info(QMPI_Context context, int tool_id, MPI_Comm comm,
                       MPI_Info info) MPICH_API_PUBLIC;
typedef int (QMPI_Comm_set_info_t) (QMPI_Context context, int tool_id, MPI_Comm comm,
                                    MPI_Info info);

int QMPI_Comm_set_name(QMPI_Context context, int tool_id, MPI_Comm comm,
                       const char *comm_name) MPICH_API_PUBLIC;
typedef int (QMPI_Comm_set_name_t) (QMPI_Context context, int tool_id, MPI_Comm comm,
                                    const char *comm_name);

int QMPI_Comm_size(QMPI_Context context, int tool_id, MPI_Comm comm, int *size) MPICH_API_PUBLIC;
typedef int (QMPI_Comm_size_t) (QMPI_Context context, int tool_id, MPI_Comm comm, int *size);

int QMPI_Comm_spawn(QMPI_Context context, int tool_id, const char *command, char *argv[],
                    int maxprocs, MPI_Info info, int root, MPI_Comm comm, MPI_Comm * intercomm,
                    int array_of_errcodes[]) MPICH_API_PUBLIC;
typedef int (QMPI_Comm_spawn_t) (QMPI_Context context, int tool_id, const char *command,
                                 char *argv[], int maxprocs, MPI_Info info, int root, MPI_Comm comm,
                                 MPI_Comm * intercomm, int array_of_errcodes[]);

int QMPI_Comm_spawn_multiple(QMPI_Context context, int tool_id, int count,
                             char *array_of_commands[], char **array_of_argv[],
                             const int array_of_maxprocs[], const MPI_Info array_of_info[],
                             int root, MPI_Comm comm, MPI_Comm * intercomm,
                             int array_of_errcodes[]) MPICH_API_PUBLIC;
typedef int (QMPI_Comm_spawn_multiple_t) (QMPI_Context context, int tool_id, int count,
                                          char *array_of_commands[], char **array_of_argv[],
                                          const int array_of_maxprocs[],
                                          const MPI_Info array_of_info[], int root, MPI_Comm comm,
                                          MPI_Comm * intercomm, int array_of_errcodes[]);

int QMPI_Comm_split(QMPI_Context context, int tool_id, MPI_Comm comm, int color, int key,
                    MPI_Comm * newcomm) MPICH_API_PUBLIC;
typedef int (QMPI_Comm_split_t) (QMPI_Context context, int tool_id, MPI_Comm comm, int color,
                                 int key, MPI_Comm * newcomm);

int QMPI_Comm_split_type(QMPI_Context context, int tool_id, MPI_Comm comm, int split_type, int key,
                         MPI_Info info, MPI_Comm * newcomm) MPICH_API_PUBLIC;
typedef int (QMPI_Comm_split_type_t) (QMPI_Context context, int tool_id, MPI_Comm comm,
                                      int split_type, int key, MPI_Info info, MPI_Comm * newcomm);

int QMPI_Comm_test_inter(QMPI_Context context, int tool_id, MPI_Comm comm,
                         int *flag) MPICH_API_PUBLIC;
typedef int (QMPI_Comm_test_inter_t) (QMPI_Context context, int tool_id, MPI_Comm comm, int *flag);

int QMPI_Compare_and_swap(QMPI_Context context, int tool_id, const void *origin_addr,
                          const void *compare_addr, void *result_addr, MPI_Datatype datatype,
                          int target_rank, MPI_Aint target_disp, MPI_Win win) MPICH_API_PUBLIC;
typedef int (QMPI_Compare_and_swap_t) (QMPI_Context context, int tool_id, const void *origin_addr,
                                       const void *compare_addr, void *result_addr,
                                       MPI_Datatype datatype, int target_rank, MPI_Aint target_disp,
                                       MPI_Win win);

int QMPI_Dims_create(QMPI_Context context, int tool_id, int nnodes, int ndims,
                     int dims[]) MPICH_API_PUBLIC;
typedef int (QMPI_Dims_create_t) (QMPI_Context context, int tool_id, int nnodes, int ndims,
                                  int dims[]);

int QMPI_Dist_graph_create(QMPI_Context context, int tool_id, MPI_Comm comm_old, int n,
                           const int sources[], const int degrees[], const int destinations[],
                           const int weights[], MPI_Info info, int reorder,
                           MPI_Comm * comm_dist_graph) MPICH_API_PUBLIC;
typedef int (QMPI_Dist_graph_create_t) (QMPI_Context context, int tool_id, MPI_Comm comm_old, int n,
                                        const int sources[], const int degrees[],
                                        const int destinations[], const int weights[],
                                        MPI_Info info, int reorder, MPI_Comm * comm_dist_graph);

int QMPI_Dist_graph_create_adjacent(QMPI_Context context, int tool_id, MPI_Comm comm_old,
                                    int indegree, const int sources[], const int sourceweights[],
                                    int outdegree, const int destinations[],
                                    const int destweights[], MPI_Info info, int reorder,
                                    MPI_Comm * comm_dist_graph) MPICH_API_PUBLIC;
typedef int (QMPI_Dist_graph_create_adjacent_t) (QMPI_Context context, int tool_id,
                                                 MPI_Comm comm_old, int indegree,
                                                 const int sources[], const int sourceweights[],
                                                 int outdegree, const int destinations[],
                                                 const int destweights[], MPI_Info info,
                                                 int reorder, MPI_Comm * comm_dist_graph);

int QMPI_Dist_graph_neighbors(QMPI_Context context, int tool_id, MPI_Comm comm, int maxindegree,
                              int sources[], int sourceweights[], int maxoutdegree,
                              int destinations[], int destweights[]) MPICH_API_PUBLIC;
typedef int (QMPI_Dist_graph_neighbors_t) (QMPI_Context context, int tool_id, MPI_Comm comm,
                                           int maxindegree, int sources[], int sourceweights[],
                                           int maxoutdegree, int destinations[], int destweights[]);

int QMPI_Dist_graph_neighbors_count(QMPI_Context context, int tool_id, MPI_Comm comm, int *indegree,
                                    int *outdegree, int *weighted) MPICH_API_PUBLIC;
typedef int (QMPI_Dist_graph_neighbors_count_t) (QMPI_Context context, int tool_id, MPI_Comm comm,
                                                 int *indegree, int *outdegree, int *weighted);

int QMPI_Errhandler_free(QMPI_Context context, int tool_id,
                         MPI_Errhandler * errhandler) MPICH_API_PUBLIC;
typedef int (QMPI_Errhandler_free_t) (QMPI_Context context, int tool_id,
                                      MPI_Errhandler * errhandler);

int QMPI_Error_class(QMPI_Context context, int tool_id, int errorcode,
                     int *errorclass) MPICH_API_PUBLIC;
typedef int (QMPI_Error_class_t) (QMPI_Context context, int tool_id, int errorcode,
                                  int *errorclass);

int QMPI_Error_string(QMPI_Context context, int tool_id, int errorcode, char *string,
                      int *resultlen) MPICH_API_PUBLIC;
typedef int (QMPI_Error_string_t) (QMPI_Context context, int tool_id, int errorcode, char *string,
                                   int *resultlen);

int QMPI_Exscan(QMPI_Context context, int tool_id, const void *sendbuf, void *recvbuf, int count,
                MPI_Datatype datatype, MPI_Op op, MPI_Comm comm) MPICH_API_PUBLIC;
typedef int (QMPI_Exscan_t) (QMPI_Context context, int tool_id, const void *sendbuf, void *recvbuf,
                             int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);

int QMPI_Fetch_and_op(QMPI_Context context, int tool_id, const void *origin_addr, void *result_addr,
                      MPI_Datatype datatype, int target_rank, MPI_Aint target_disp, MPI_Op op,
                      MPI_Win win) MPICH_API_PUBLIC;
typedef int (QMPI_Fetch_and_op_t) (QMPI_Context context, int tool_id, const void *origin_addr,
                                   void *result_addr, MPI_Datatype datatype, int target_rank,
                                   MPI_Aint target_disp, MPI_Op op, MPI_Win win);

MPI_Fint QMPI_File_c2f(QMPI_Context context, int tool_id, MPI_File file) MPICH_API_PUBLIC;
typedef MPI_Fint(QMPI_File_c2f_t) (QMPI_Context context, int tool_id, MPI_File file);

int QMPI_File_call_errhandler(QMPI_Context context, int tool_id, MPI_File fh,
                              int errorcode) MPICH_API_PUBLIC;
typedef int (QMPI_File_call_errhandler_t) (QMPI_Context context, int tool_id, MPI_File fh,
                                           int errorcode);

int QMPI_File_close(QMPI_Context context, int tool_id, MPI_File * fh) MPICH_API_PUBLIC;
typedef int (QMPI_File_close_t) (QMPI_Context context, int tool_id, MPI_File * fh);

int QMPI_File_create_errhandler(QMPI_Context context, int tool_id,
                                MPI_File_errhandler_function * file_errhandler_fn,
                                MPI_Errhandler * errhandler) MPICH_API_PUBLIC;
typedef int (QMPI_File_create_errhandler_t) (QMPI_Context context, int tool_id,
                                             MPI_File_errhandler_function * file_errhandler_fn,
                                             MPI_Errhandler * errhandler);

int QMPI_File_delete(QMPI_Context context, int tool_id, const char *filename,
                     MPI_Info info) MPICH_API_PUBLIC;
typedef int (QMPI_File_delete_t) (QMPI_Context context, int tool_id, const char *filename,
                                  MPI_Info info);

MPI_File QMPI_File_f2c(QMPI_Context context, int tool_id, MPI_Fint file) MPICH_API_PUBLIC;
typedef MPI_File(QMPI_File_f2c_t) (QMPI_Context context, int tool_id, MPI_Fint file);

int QMPI_File_get_amode(QMPI_Context context, int tool_id, MPI_File fh,
                        int *amode) MPICH_API_PUBLIC;
typedef int (QMPI_File_get_amode_t) (QMPI_Context context, int tool_id, MPI_File fh, int *amode);

int QMPI_File_get_atomicity(QMPI_Context context, int tool_id, MPI_File fh,
                            int *flag) MPICH_API_PUBLIC;
typedef int (QMPI_File_get_atomicity_t) (QMPI_Context context, int tool_id, MPI_File fh, int *flag);

int QMPI_File_get_byte_offset(QMPI_Context context, int tool_id, MPI_File fh, MPI_Offset offset,
                              MPI_Offset * disp) MPICH_API_PUBLIC;
typedef int (QMPI_File_get_byte_offset_t) (QMPI_Context context, int tool_id, MPI_File fh,
                                           MPI_Offset offset, MPI_Offset * disp);

int QMPI_File_get_errhandler(QMPI_Context context, int tool_id, MPI_File file,
                             MPI_Errhandler * errhandler) MPICH_API_PUBLIC;
typedef int (QMPI_File_get_errhandler_t) (QMPI_Context context, int tool_id, MPI_File file,
                                          MPI_Errhandler * errhandler);

int QMPI_File_get_group(QMPI_Context context, int tool_id, MPI_File fh,
                        MPI_Group * group) MPICH_API_PUBLIC;
typedef int (QMPI_File_get_group_t) (QMPI_Context context, int tool_id, MPI_File fh,
                                     MPI_Group * group);

int QMPI_File_get_info(QMPI_Context context, int tool_id, MPI_File fh,
                       MPI_Info * info_used) MPICH_API_PUBLIC;
typedef int (QMPI_File_get_info_t) (QMPI_Context context, int tool_id, MPI_File fh,
                                    MPI_Info * info_used);

int QMPI_File_get_position(QMPI_Context context, int tool_id, MPI_File fh,
                           MPI_Offset * offset) MPICH_API_PUBLIC;
typedef int (QMPI_File_get_position_t) (QMPI_Context context, int tool_id, MPI_File fh,
                                        MPI_Offset * offset);

int QMPI_File_get_position_shared(QMPI_Context context, int tool_id, MPI_File fh,
                                  MPI_Offset * offset) MPICH_API_PUBLIC;
typedef int (QMPI_File_get_position_shared_t) (QMPI_Context context, int tool_id, MPI_File fh,
                                               MPI_Offset * offset);

int QMPI_File_get_size(QMPI_Context context, int tool_id, MPI_File fh,
                       MPI_Offset * size) MPICH_API_PUBLIC;
typedef int (QMPI_File_get_size_t) (QMPI_Context context, int tool_id, MPI_File fh,
                                    MPI_Offset * size);

int QMPI_File_get_type_extent(QMPI_Context context, int tool_id, MPI_File fh, MPI_Datatype datatype,
                              MPI_Aint * extent) MPICH_API_PUBLIC;
typedef int (QMPI_File_get_type_extent_t) (QMPI_Context context, int tool_id, MPI_File fh,
                                           MPI_Datatype datatype, MPI_Aint * extent);

int QMPI_File_get_view(QMPI_Context context, int tool_id, MPI_File fh, MPI_Offset * disp,
                       MPI_Datatype * etype, MPI_Datatype * filetype,
                       char *datarep) MPICH_API_PUBLIC;
typedef int (QMPI_File_get_view_t) (QMPI_Context context, int tool_id, MPI_File fh,
                                    MPI_Offset * disp, MPI_Datatype * etype,
                                    MPI_Datatype * filetype, char *datarep);

int QMPI_File_iread(QMPI_Context context, int tool_id, MPI_File fh, void *buf, int count,
                    MPI_Datatype datatype, MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_File_iread_t) (QMPI_Context context, int tool_id, MPI_File fh, void *buf,
                                 int count, MPI_Datatype datatype, MPI_Request * request);

int QMPI_File_iread_all(QMPI_Context context, int tool_id, MPI_File fh, void *buf, int count,
                        MPI_Datatype datatype, MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_File_iread_all_t) (QMPI_Context context, int tool_id, MPI_File fh, void *buf,
                                     int count, MPI_Datatype datatype, MPI_Request * request);

int QMPI_File_iread_at(QMPI_Context context, int tool_id, MPI_File fh, MPI_Offset offset, void *buf,
                       int count, MPI_Datatype datatype, MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_File_iread_at_t) (QMPI_Context context, int tool_id, MPI_File fh,
                                    MPI_Offset offset, void *buf, int count, MPI_Datatype datatype,
                                    MPI_Request * request);

int QMPI_File_iread_at_all(QMPI_Context context, int tool_id, MPI_File fh, MPI_Offset offset,
                           void *buf, int count, MPI_Datatype datatype,
                           MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_File_iread_at_all_t) (QMPI_Context context, int tool_id, MPI_File fh,
                                        MPI_Offset offset, void *buf, int count,
                                        MPI_Datatype datatype, MPI_Request * request);

int QMPI_File_iread_shared(QMPI_Context context, int tool_id, MPI_File fh, void *buf, int count,
                           MPI_Datatype datatype, MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_File_iread_shared_t) (QMPI_Context context, int tool_id, MPI_File fh, void *buf,
                                        int count, MPI_Datatype datatype, MPI_Request * request);

int QMPI_File_iwrite(QMPI_Context context, int tool_id, MPI_File fh, const void *buf, int count,
                     MPI_Datatype datatype, MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_File_iwrite_t) (QMPI_Context context, int tool_id, MPI_File fh, const void *buf,
                                  int count, MPI_Datatype datatype, MPI_Request * request);

int QMPI_File_iwrite_all(QMPI_Context context, int tool_id, MPI_File fh, const void *buf, int count,
                         MPI_Datatype datatype, MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_File_iwrite_all_t) (QMPI_Context context, int tool_id, MPI_File fh,
                                      const void *buf, int count, MPI_Datatype datatype,
                                      MPI_Request * request);

int QMPI_File_iwrite_at(QMPI_Context context, int tool_id, MPI_File fh, MPI_Offset offset,
                        const void *buf, int count, MPI_Datatype datatype,
                        MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_File_iwrite_at_t) (QMPI_Context context, int tool_id, MPI_File fh,
                                     MPI_Offset offset, const void *buf, int count,
                                     MPI_Datatype datatype, MPI_Request * request);

int QMPI_File_iwrite_at_all(QMPI_Context context, int tool_id, MPI_File fh, MPI_Offset offset,
                            const void *buf, int count, MPI_Datatype datatype,
                            MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_File_iwrite_at_all_t) (QMPI_Context context, int tool_id, MPI_File fh,
                                         MPI_Offset offset, const void *buf, int count,
                                         MPI_Datatype datatype, MPI_Request * request);

int QMPI_File_iwrite_shared(QMPI_Context context, int tool_id, MPI_File fh, const void *buf,
                            int count, MPI_Datatype datatype,
                            MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_File_iwrite_shared_t) (QMPI_Context context, int tool_id, MPI_File fh,
                                         const void *buf, int count, MPI_Datatype datatype,
                                         MPI_Request * request);

int QMPI_File_open(QMPI_Context context, int tool_id, MPI_Comm comm, const char *filename,
                   int amode, MPI_Info info, MPI_File * fh) MPICH_API_PUBLIC;
typedef int (QMPI_File_open_t) (QMPI_Context context, int tool_id, MPI_Comm comm,
                                const char *filename, int amode, MPI_Info info, MPI_File * fh);

int QMPI_File_preallocate(QMPI_Context context, int tool_id, MPI_File fh,
                          MPI_Offset size) MPICH_API_PUBLIC;
typedef int (QMPI_File_preallocate_t) (QMPI_Context context, int tool_id, MPI_File fh,
                                       MPI_Offset size);

int QMPI_File_read(QMPI_Context context, int tool_id, MPI_File fh, void *buf, int count,
                   MPI_Datatype datatype, MPI_Status * status) MPICH_API_PUBLIC;
typedef int (QMPI_File_read_t) (QMPI_Context context, int tool_id, MPI_File fh, void *buf,
                                int count, MPI_Datatype datatype, MPI_Status * status);

int QMPI_File_read_all(QMPI_Context context, int tool_id, MPI_File fh, void *buf, int count,
                       MPI_Datatype datatype, MPI_Status * status) MPICH_API_PUBLIC;
typedef int (QMPI_File_read_all_t) (QMPI_Context context, int tool_id, MPI_File fh, void *buf,
                                    int count, MPI_Datatype datatype, MPI_Status * status);

int QMPI_File_read_all_begin(QMPI_Context context, int tool_id, MPI_File fh, void *buf, int count,
                             MPI_Datatype datatype) MPICH_API_PUBLIC;
typedef int (QMPI_File_read_all_begin_t) (QMPI_Context context, int tool_id, MPI_File fh, void *buf,
                                          int count, MPI_Datatype datatype);

int QMPI_File_read_all_end(QMPI_Context context, int tool_id, MPI_File fh, void *buf,
                           MPI_Status * status) MPICH_API_PUBLIC;
typedef int (QMPI_File_read_all_end_t) (QMPI_Context context, int tool_id, MPI_File fh, void *buf,
                                        MPI_Status * status);

int QMPI_File_read_at(QMPI_Context context, int tool_id, MPI_File fh, MPI_Offset offset, void *buf,
                      int count, MPI_Datatype datatype, MPI_Status * status) MPICH_API_PUBLIC;
typedef int (QMPI_File_read_at_t) (QMPI_Context context, int tool_id, MPI_File fh,
                                   MPI_Offset offset, void *buf, int count, MPI_Datatype datatype,
                                   MPI_Status * status);

int QMPI_File_read_at_all(QMPI_Context context, int tool_id, MPI_File fh, MPI_Offset offset,
                          void *buf, int count, MPI_Datatype datatype,
                          MPI_Status * status) MPICH_API_PUBLIC;
typedef int (QMPI_File_read_at_all_t) (QMPI_Context context, int tool_id, MPI_File fh,
                                       MPI_Offset offset, void *buf, int count,
                                       MPI_Datatype datatype, MPI_Status * status);

int QMPI_File_read_at_all_begin(QMPI_Context context, int tool_id, MPI_File fh, MPI_Offset offset,
                                void *buf, int count, MPI_Datatype datatype) MPICH_API_PUBLIC;
typedef int (QMPI_File_read_at_all_begin_t) (QMPI_Context context, int tool_id, MPI_File fh,
                                             MPI_Offset offset, void *buf, int count,
                                             MPI_Datatype datatype);

int QMPI_File_read_at_all_end(QMPI_Context context, int tool_id, MPI_File fh, void *buf,
                              MPI_Status * status) MPICH_API_PUBLIC;
typedef int (QMPI_File_read_at_all_end_t) (QMPI_Context context, int tool_id, MPI_File fh,
                                           void *buf, MPI_Status * status);

int QMPI_File_read_ordered(QMPI_Context context, int tool_id, MPI_File fh, void *buf, int count,
                           MPI_Datatype datatype, MPI_Status * status) MPICH_API_PUBLIC;
typedef int (QMPI_File_read_ordered_t) (QMPI_Context context, int tool_id, MPI_File fh, void *buf,
                                        int count, MPI_Datatype datatype, MPI_Status * status);

int QMPI_File_read_ordered_begin(QMPI_Context context, int tool_id, MPI_File fh, void *buf,
                                 int count, MPI_Datatype datatype) MPICH_API_PUBLIC;
typedef int (QMPI_File_read_ordered_begin_t) (QMPI_Context context, int tool_id, MPI_File fh,
                                              void *buf, int count, MPI_Datatype datatype);

int QMPI_File_read_ordered_end(QMPI_Context context, int tool_id, MPI_File fh, void *buf,
                               MPI_Status * status) MPICH_API_PUBLIC;
typedef int (QMPI_File_read_ordered_end_t) (QMPI_Context context, int tool_id, MPI_File fh,
                                            void *buf, MPI_Status * status);

int QMPI_File_read_shared(QMPI_Context context, int tool_id, MPI_File fh, void *buf, int count,
                          MPI_Datatype datatype, MPI_Status * status) MPICH_API_PUBLIC;
typedef int (QMPI_File_read_shared_t) (QMPI_Context context, int tool_id, MPI_File fh, void *buf,
                                       int count, MPI_Datatype datatype, MPI_Status * status);

int QMPI_File_seek(QMPI_Context context, int tool_id, MPI_File fh, MPI_Offset offset,
                   int whence) MPICH_API_PUBLIC;
typedef int (QMPI_File_seek_t) (QMPI_Context context, int tool_id, MPI_File fh, MPI_Offset offset,
                                int whence);

int QMPI_File_seek_shared(QMPI_Context context, int tool_id, MPI_File fh, MPI_Offset offset,
                          int whence) MPICH_API_PUBLIC;
typedef int (QMPI_File_seek_shared_t) (QMPI_Context context, int tool_id, MPI_File fh,
                                       MPI_Offset offset, int whence);

int QMPI_File_set_atomicity(QMPI_Context context, int tool_id, MPI_File fh,
                            int flag) MPICH_API_PUBLIC;
typedef int (QMPI_File_set_atomicity_t) (QMPI_Context context, int tool_id, MPI_File fh, int flag);

int QMPI_File_set_errhandler(QMPI_Context context, int tool_id, MPI_File file,
                             MPI_Errhandler errhandler) MPICH_API_PUBLIC;
typedef int (QMPI_File_set_errhandler_t) (QMPI_Context context, int tool_id, MPI_File file,
                                          MPI_Errhandler errhandler);

int QMPI_File_set_info(QMPI_Context context, int tool_id, MPI_File fh,
                       MPI_Info info) MPICH_API_PUBLIC;
typedef int (QMPI_File_set_info_t) (QMPI_Context context, int tool_id, MPI_File fh, MPI_Info info);

int QMPI_File_set_size(QMPI_Context context, int tool_id, MPI_File fh,
                       MPI_Offset size) MPICH_API_PUBLIC;
typedef int (QMPI_File_set_size_t) (QMPI_Context context, int tool_id, MPI_File fh,
                                    MPI_Offset size);

int QMPI_File_set_view(QMPI_Context context, int tool_id, MPI_File fh, MPI_Offset disp,
                       MPI_Datatype etype, MPI_Datatype filetype, const char *datarep,
                       MPI_Info info) MPICH_API_PUBLIC;
typedef int (QMPI_File_set_view_t) (QMPI_Context context, int tool_id, MPI_File fh, MPI_Offset disp,
                                    MPI_Datatype etype, MPI_Datatype filetype, const char *datarep,
                                    MPI_Info info);

int QMPI_File_sync(QMPI_Context context, int tool_id, MPI_File fh) MPICH_API_PUBLIC;
typedef int (QMPI_File_sync_t) (QMPI_Context context, int tool_id, MPI_File fh);

int QMPI_File_write(QMPI_Context context, int tool_id, MPI_File fh, const void *buf, int count,
                    MPI_Datatype datatype, MPI_Status * status) MPICH_API_PUBLIC;
typedef int (QMPI_File_write_t) (QMPI_Context context, int tool_id, MPI_File fh, const void *buf,
                                 int count, MPI_Datatype datatype, MPI_Status * status);

int QMPI_File_write_all(QMPI_Context context, int tool_id, MPI_File fh, const void *buf, int count,
                        MPI_Datatype datatype, MPI_Status * status) MPICH_API_PUBLIC;
typedef int (QMPI_File_write_all_t) (QMPI_Context context, int tool_id, MPI_File fh,
                                     const void *buf, int count, MPI_Datatype datatype,
                                     MPI_Status * status);

int QMPI_File_write_all_begin(QMPI_Context context, int tool_id, MPI_File fh, const void *buf,
                              int count, MPI_Datatype datatype) MPICH_API_PUBLIC;
typedef int (QMPI_File_write_all_begin_t) (QMPI_Context context, int tool_id, MPI_File fh,
                                           const void *buf, int count, MPI_Datatype datatype);

int QMPI_File_write_all_end(QMPI_Context context, int tool_id, MPI_File fh, const void *buf,
                            MPI_Status * status) MPICH_API_PUBLIC;
typedef int (QMPI_File_write_all_end_t) (QMPI_Context context, int tool_id, MPI_File fh,
                                         const void *buf, MPI_Status * status);

int QMPI_File_write_at(QMPI_Context context, int tool_id, MPI_File fh, MPI_Offset offset,
                       const void *buf, int count, MPI_Datatype datatype,
                       MPI_Status * status) MPICH_API_PUBLIC;
typedef int (QMPI_File_write_at_t) (QMPI_Context context, int tool_id, MPI_File fh,
                                    MPI_Offset offset, const void *buf, int count,
                                    MPI_Datatype datatype, MPI_Status * status);

int QMPI_File_write_at_all(QMPI_Context context, int tool_id, MPI_File fh, MPI_Offset offset,
                           const void *buf, int count, MPI_Datatype datatype,
                           MPI_Status * status) MPICH_API_PUBLIC;
typedef int (QMPI_File_write_at_all_t) (QMPI_Context context, int tool_id, MPI_File fh,
                                        MPI_Offset offset, const void *buf, int count,
                                        MPI_Datatype datatype, MPI_Status * status);

int QMPI_File_write_at_all_begin(QMPI_Context context, int tool_id, MPI_File fh, MPI_Offset offset,
                                 const void *buf, int count,
                                 MPI_Datatype datatype) MPICH_API_PUBLIC;
typedef int (QMPI_File_write_at_all_begin_t) (QMPI_Context context, int tool_id, MPI_File fh,
                                              MPI_Offset offset, const void *buf, int count,
                                              MPI_Datatype datatype);

int QMPI_File_write_at_all_end(QMPI_Context context, int tool_id, MPI_File fh, const void *buf,
                               MPI_Status * status) MPICH_API_PUBLIC;
typedef int (QMPI_File_write_at_all_end_t) (QMPI_Context context, int tool_id, MPI_File fh,
                                            const void *buf, MPI_Status * status);

int QMPI_File_write_ordered(QMPI_Context context, int tool_id, MPI_File fh, const void *buf,
                            int count, MPI_Datatype datatype, MPI_Status * status) MPICH_API_PUBLIC;
typedef int (QMPI_File_write_ordered_t) (QMPI_Context context, int tool_id, MPI_File fh,
                                         const void *buf, int count, MPI_Datatype datatype,
                                         MPI_Status * status);

int QMPI_File_write_ordered_begin(QMPI_Context context, int tool_id, MPI_File fh, const void *buf,
                                  int count, MPI_Datatype datatype) MPICH_API_PUBLIC;
typedef int (QMPI_File_write_ordered_begin_t) (QMPI_Context context, int tool_id, MPI_File fh,
                                               const void *buf, int count, MPI_Datatype datatype);

int QMPI_File_write_ordered_end(QMPI_Context context, int tool_id, MPI_File fh, const void *buf,
                                MPI_Status * status) MPICH_API_PUBLIC;
typedef int (QMPI_File_write_ordered_end_t) (QMPI_Context context, int tool_id, MPI_File fh,
                                             const void *buf, MPI_Status * status);

int QMPI_File_write_shared(QMPI_Context context, int tool_id, MPI_File fh, const void *buf,
                           int count, MPI_Datatype datatype, MPI_Status * status) MPICH_API_PUBLIC;
typedef int (QMPI_File_write_shared_t) (QMPI_Context context, int tool_id, MPI_File fh,
                                        const void *buf, int count, MPI_Datatype datatype,
                                        MPI_Status * status);

int QMPI_Finalize(QMPI_Context context, int tool_id) MPICH_API_PUBLIC;
typedef int (QMPI_Finalize_t) (QMPI_Context context, int tool_id);

int QMPI_Finalized(QMPI_Context context, int tool_id, int *flag) MPICH_API_PUBLIC;
typedef int (QMPI_Finalized_t) (QMPI_Context context, int tool_id, int *flag);

int QMPI_Free_mem(QMPI_Context context, int tool_id, void *base) MPICH_API_PUBLIC;
typedef int (QMPI_Free_mem_t) (QMPI_Context context, int tool_id, void *base);

int QMPI_Gather(QMPI_Context context, int tool_id, const void *sendbuf, int sendcount,
                MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype,
                int root, MPI_Comm comm) MPICH_API_PUBLIC;
typedef int (QMPI_Gather_t) (QMPI_Context context, int tool_id, const void *sendbuf, int sendcount,
                             MPI_Datatype sendtype, void *recvbuf, int recvcount,
                             MPI_Datatype recvtype, int root, MPI_Comm comm);

int QMPI_Gatherv(QMPI_Context context, int tool_id, const void *sendbuf, int sendcount,
                 MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int displs[],
                 MPI_Datatype recvtype, int root, MPI_Comm comm) MPICH_API_PUBLIC;
typedef int (QMPI_Gatherv_t) (QMPI_Context context, int tool_id, const void *sendbuf, int sendcount,
                              MPI_Datatype sendtype, void *recvbuf, const int recvcounts[],
                              const int displs[], MPI_Datatype recvtype, int root, MPI_Comm comm);

int QMPI_Get(QMPI_Context context, int tool_id, void *origin_addr, int origin_count,
             MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp, int target_count,
             MPI_Datatype target_datatype, MPI_Win win) MPICH_API_PUBLIC;
typedef int (QMPI_Get_t) (QMPI_Context context, int tool_id, void *origin_addr, int origin_count,
                          MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp,
                          int target_count, MPI_Datatype target_datatype, MPI_Win win);

int QMPI_Get_accumulate(QMPI_Context context, int tool_id, const void *origin_addr,
                        int origin_count, MPI_Datatype origin_datatype, void *result_addr,
                        int result_count, MPI_Datatype result_datatype, int target_rank,
                        MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype,
                        MPI_Op op, MPI_Win win) MPICH_API_PUBLIC;
typedef int (QMPI_Get_accumulate_t) (QMPI_Context context, int tool_id, const void *origin_addr,
                                     int origin_count, MPI_Datatype origin_datatype,
                                     void *result_addr, int result_count,
                                     MPI_Datatype result_datatype, int target_rank,
                                     MPI_Aint target_disp, int target_count,
                                     MPI_Datatype target_datatype, MPI_Op op, MPI_Win win);

int QMPI_Get_address(QMPI_Context context, int tool_id, const void *location,
                     MPI_Aint * address) MPICH_API_PUBLIC;
typedef int (QMPI_Get_address_t) (QMPI_Context context, int tool_id, const void *location,
                                  MPI_Aint * address);

int QMPI_Get_count(QMPI_Context context, int tool_id, const MPI_Status * status,
                   MPI_Datatype datatype, int *count) MPICH_API_PUBLIC;
typedef int (QMPI_Get_count_t) (QMPI_Context context, int tool_id, const MPI_Status * status,
                                MPI_Datatype datatype, int *count);

int QMPI_Get_elements(QMPI_Context context, int tool_id, const MPI_Status * status,
                      MPI_Datatype datatype, int *count) MPICH_API_PUBLIC;
typedef int (QMPI_Get_elements_t) (QMPI_Context context, int tool_id, const MPI_Status * status,
                                   MPI_Datatype datatype, int *count);

int QMPI_Get_elements_x(QMPI_Context context, int tool_id, const MPI_Status * status,
                        MPI_Datatype datatype, MPI_Count * count) MPICH_API_PUBLIC;
typedef int (QMPI_Get_elements_x_t) (QMPI_Context context, int tool_id, const MPI_Status * status,
                                     MPI_Datatype datatype, MPI_Count * count);

int QMPI_Get_library_version(QMPI_Context context, int tool_id, char *version,
                             int *resultlen) MPICH_API_PUBLIC;
typedef int (QMPI_Get_library_version_t) (QMPI_Context context, int tool_id, char *version,
                                          int *resultlen);

int QMPI_Get_processor_name(QMPI_Context context, int tool_id, char *name,
                            int *resultlen) MPICH_API_PUBLIC;
typedef int (QMPI_Get_processor_name_t) (QMPI_Context context, int tool_id, char *name,
                                         int *resultlen);

int QMPI_Get_version(QMPI_Context context, int tool_id, int *version,
                     int *subversion) MPICH_API_PUBLIC;
typedef int (QMPI_Get_version_t) (QMPI_Context context, int tool_id, int *version, int *subversion);

int QMPI_Graph_create(QMPI_Context context, int tool_id, MPI_Comm comm_old, int nnodes,
                      const int index[], const int edges[], int reorder,
                      MPI_Comm * comm_graph) MPICH_API_PUBLIC;
typedef int (QMPI_Graph_create_t) (QMPI_Context context, int tool_id, MPI_Comm comm_old, int nnodes,
                                   const int index[], const int edges[], int reorder,
                                   MPI_Comm * comm_graph);

int QMPI_Graph_get(QMPI_Context context, int tool_id, MPI_Comm comm, int maxindex, int maxedges,
                   int index[], int edges[]) MPICH_API_PUBLIC;
typedef int (QMPI_Graph_get_t) (QMPI_Context context, int tool_id, MPI_Comm comm, int maxindex,
                                int maxedges, int index[], int edges[]);

int QMPI_Graph_map(QMPI_Context context, int tool_id, MPI_Comm comm, int nnodes, const int index[],
                   const int edges[], int *newrank) MPICH_API_PUBLIC;
typedef int (QMPI_Graph_map_t) (QMPI_Context context, int tool_id, MPI_Comm comm, int nnodes,
                                const int index[], const int edges[], int *newrank);

int QMPI_Graph_neighbors(QMPI_Context context, int tool_id, MPI_Comm comm, int rank,
                         int maxneighbors, int neighbors[]) MPICH_API_PUBLIC;
typedef int (QMPI_Graph_neighbors_t) (QMPI_Context context, int tool_id, MPI_Comm comm, int rank,
                                      int maxneighbors, int neighbors[]);

int QMPI_Graph_neighbors_count(QMPI_Context context, int tool_id, MPI_Comm comm, int rank,
                               int *nneighbors) MPICH_API_PUBLIC;
typedef int (QMPI_Graph_neighbors_count_t) (QMPI_Context context, int tool_id, MPI_Comm comm,
                                            int rank, int *nneighbors);

int QMPI_Graphdims_get(QMPI_Context context, int tool_id, MPI_Comm comm, int *nnodes,
                       int *nedges) MPICH_API_PUBLIC;
typedef int (QMPI_Graphdims_get_t) (QMPI_Context context, int tool_id, MPI_Comm comm, int *nnodes,
                                    int *nedges);

int QMPI_Grequest_complete(QMPI_Context context, int tool_id, MPI_Request request) MPICH_API_PUBLIC;
typedef int (QMPI_Grequest_complete_t) (QMPI_Context context, int tool_id, MPI_Request request);

int QMPI_Grequest_start(QMPI_Context context, int tool_id, MPI_Grequest_query_function * query_fn,
                        MPI_Grequest_free_function * free_fn,
                        MPI_Grequest_cancel_function * cancel_fn, void *extra_state,
                        MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_Grequest_start_t) (QMPI_Context context, int tool_id,
                                     MPI_Grequest_query_function * query_fn,
                                     MPI_Grequest_free_function * free_fn,
                                     MPI_Grequest_cancel_function * cancel_fn, void *extra_state,
                                     MPI_Request * request);

int QMPI_Group_compare(QMPI_Context context, int tool_id, MPI_Group group1, MPI_Group group2,
                       int *result) MPICH_API_PUBLIC;
typedef int (QMPI_Group_compare_t) (QMPI_Context context, int tool_id, MPI_Group group1,
                                    MPI_Group group2, int *result);

int QMPI_Group_difference(QMPI_Context context, int tool_id, MPI_Group group1, MPI_Group group2,
                          MPI_Group * newgroup) MPICH_API_PUBLIC;
typedef int (QMPI_Group_difference_t) (QMPI_Context context, int tool_id, MPI_Group group1,
                                       MPI_Group group2, MPI_Group * newgroup);

int QMPI_Group_excl(QMPI_Context context, int tool_id, MPI_Group group, int n, const int ranks[],
                    MPI_Group * newgroup) MPICH_API_PUBLIC;
typedef int (QMPI_Group_excl_t) (QMPI_Context context, int tool_id, MPI_Group group, int n,
                                 const int ranks[], MPI_Group * newgroup);

int QMPI_Group_free(QMPI_Context context, int tool_id, MPI_Group * group) MPICH_API_PUBLIC;
typedef int (QMPI_Group_free_t) (QMPI_Context context, int tool_id, MPI_Group * group);

int QMPI_Group_incl(QMPI_Context context, int tool_id, MPI_Group group, int n, const int ranks[],
                    MPI_Group * newgroup) MPICH_API_PUBLIC;
typedef int (QMPI_Group_incl_t) (QMPI_Context context, int tool_id, MPI_Group group, int n,
                                 const int ranks[], MPI_Group * newgroup);

int QMPI_Group_intersection(QMPI_Context context, int tool_id, MPI_Group group1, MPI_Group group2,
                            MPI_Group * newgroup) MPICH_API_PUBLIC;
typedef int (QMPI_Group_intersection_t) (QMPI_Context context, int tool_id, MPI_Group group1,
                                         MPI_Group group2, MPI_Group * newgroup);

int QMPI_Group_range_excl(QMPI_Context context, int tool_id, MPI_Group group, int n,
                          int ranges[][3], MPI_Group * newgroup) MPICH_API_PUBLIC;
typedef int (QMPI_Group_range_excl_t) (QMPI_Context context, int tool_id, MPI_Group group, int n,
                                       int ranges[][3], MPI_Group * newgroup);

int QMPI_Group_range_incl(QMPI_Context context, int tool_id, MPI_Group group, int n,
                          int ranges[][3], MPI_Group * newgroup) MPICH_API_PUBLIC;
typedef int (QMPI_Group_range_incl_t) (QMPI_Context context, int tool_id, MPI_Group group, int n,
                                       int ranges[][3], MPI_Group * newgroup);

int QMPI_Group_rank(QMPI_Context context, int tool_id, MPI_Group group, int *rank) MPICH_API_PUBLIC;
typedef int (QMPI_Group_rank_t) (QMPI_Context context, int tool_id, MPI_Group group, int *rank);

int QMPI_Group_size(QMPI_Context context, int tool_id, MPI_Group group, int *size) MPICH_API_PUBLIC;
typedef int (QMPI_Group_size_t) (QMPI_Context context, int tool_id, MPI_Group group, int *size);

int QMPI_Group_translate_ranks(QMPI_Context context, int tool_id, MPI_Group group1, int n,
                               const int ranks1[], MPI_Group group2, int ranks2[]) MPICH_API_PUBLIC;
typedef int (QMPI_Group_translate_ranks_t) (QMPI_Context context, int tool_id, MPI_Group group1,
                                            int n, const int ranks1[], MPI_Group group2,
                                            int ranks2[]);

int QMPI_Group_union(QMPI_Context context, int tool_id, MPI_Group group1, MPI_Group group2,
                     MPI_Group * newgroup) MPICH_API_PUBLIC;
typedef int (QMPI_Group_union_t) (QMPI_Context context, int tool_id, MPI_Group group1,
                                  MPI_Group group2, MPI_Group * newgroup);

int QMPI_Iallgather(QMPI_Context context, int tool_id, const void *sendbuf, int sendcount,
                    MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype,
                    MPI_Comm comm, MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_Iallgather_t) (QMPI_Context context, int tool_id, const void *sendbuf,
                                 int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                 MPI_Datatype recvtype, MPI_Comm comm, MPI_Request * request);

int QMPI_Iallgatherv(QMPI_Context context, int tool_id, const void *sendbuf, int sendcount,
                     MPI_Datatype sendtype, void *recvbuf, const int recvcounts[],
                     const int displs[], MPI_Datatype recvtype, MPI_Comm comm,
                     MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_Iallgatherv_t) (QMPI_Context context, int tool_id, const void *sendbuf,
                                  int sendcount, MPI_Datatype sendtype, void *recvbuf,
                                  const int recvcounts[], const int displs[], MPI_Datatype recvtype,
                                  MPI_Comm comm, MPI_Request * request);

int QMPI_Iallreduce(QMPI_Context context, int tool_id, const void *sendbuf, void *recvbuf,
                    int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm,
                    MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_Iallreduce_t) (QMPI_Context context, int tool_id, const void *sendbuf,
                                 void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op,
                                 MPI_Comm comm, MPI_Request * request);

int QMPI_Ialltoall(QMPI_Context context, int tool_id, const void *sendbuf, int sendcount,
                   MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype,
                   MPI_Comm comm, MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_Ialltoall_t) (QMPI_Context context, int tool_id, const void *sendbuf,
                                int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                MPI_Datatype recvtype, MPI_Comm comm, MPI_Request * request);

int QMPI_Ialltoallv(QMPI_Context context, int tool_id, const void *sendbuf, const int sendcounts[],
                    const int sdispls[], MPI_Datatype sendtype, void *recvbuf,
                    const int recvcounts[], const int rdispls[], MPI_Datatype recvtype,
                    MPI_Comm comm, MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_Ialltoallv_t) (QMPI_Context context, int tool_id, const void *sendbuf,
                                 const int sendcounts[], const int sdispls[], MPI_Datatype sendtype,
                                 void *recvbuf, const int recvcounts[], const int rdispls[],
                                 MPI_Datatype recvtype, MPI_Comm comm, MPI_Request * request);

int QMPI_Ialltoallw(QMPI_Context context, int tool_id, const void *sendbuf, const int sendcounts[],
                    const int sdispls[], const MPI_Datatype sendtypes[], void *recvbuf,
                    const int recvcounts[], const int rdispls[], const MPI_Datatype recvtypes[],
                    MPI_Comm comm, MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_Ialltoallw_t) (QMPI_Context context, int tool_id, const void *sendbuf,
                                 const int sendcounts[], const int sdispls[],
                                 const MPI_Datatype sendtypes[], void *recvbuf,
                                 const int recvcounts[], const int rdispls[],
                                 const MPI_Datatype recvtypes[], MPI_Comm comm,
                                 MPI_Request * request);

int QMPI_Ibarrier(QMPI_Context context, int tool_id, MPI_Comm comm,
                  MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_Ibarrier_t) (QMPI_Context context, int tool_id, MPI_Comm comm,
                               MPI_Request * request);

int QMPI_Ibcast(QMPI_Context context, int tool_id, void *buffer, int count, MPI_Datatype datatype,
                int root, MPI_Comm comm, MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_Ibcast_t) (QMPI_Context context, int tool_id, void *buffer, int count,
                             MPI_Datatype datatype, int root, MPI_Comm comm, MPI_Request * request);

int QMPI_Ibsend(QMPI_Context context, int tool_id, const void *buf, int count,
                MPI_Datatype datatype, int dest, int tag, MPI_Comm comm,
                MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_Ibsend_t) (QMPI_Context context, int tool_id, const void *buf, int count,
                             MPI_Datatype datatype, int dest, int tag, MPI_Comm comm,
                             MPI_Request * request);

int QMPI_Iexscan(QMPI_Context context, int tool_id, const void *sendbuf, void *recvbuf, int count,
                 MPI_Datatype datatype, MPI_Op op, MPI_Comm comm,
                 MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_Iexscan_t) (QMPI_Context context, int tool_id, const void *sendbuf, void *recvbuf,
                              int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm,
                              MPI_Request * request);

int QMPI_Igather(QMPI_Context context, int tool_id, const void *sendbuf, int sendcount,
                 MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype,
                 int root, MPI_Comm comm, MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_Igather_t) (QMPI_Context context, int tool_id, const void *sendbuf, int sendcount,
                              MPI_Datatype sendtype, void *recvbuf, int recvcount,
                              MPI_Datatype recvtype, int root, MPI_Comm comm,
                              MPI_Request * request);

int QMPI_Igatherv(QMPI_Context context, int tool_id, const void *sendbuf, int sendcount,
                  MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int displs[],
                  MPI_Datatype recvtype, int root, MPI_Comm comm,
                  MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_Igatherv_t) (QMPI_Context context, int tool_id, const void *sendbuf,
                               int sendcount, MPI_Datatype sendtype, void *recvbuf,
                               const int recvcounts[], const int displs[], MPI_Datatype recvtype,
                               int root, MPI_Comm comm, MPI_Request * request);

int QMPI_Improbe(QMPI_Context context, int tool_id, int source, int tag, MPI_Comm comm, int *flag,
                 MPI_Message * message, MPI_Status * status) MPICH_API_PUBLIC;
typedef int (QMPI_Improbe_t) (QMPI_Context context, int tool_id, int source, int tag, MPI_Comm comm,
                              int *flag, MPI_Message * message, MPI_Status * status);

int QMPI_Imrecv(QMPI_Context context, int tool_id, void *buf, int count, MPI_Datatype datatype,
                MPI_Message * message, MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_Imrecv_t) (QMPI_Context context, int tool_id, void *buf, int count,
                             MPI_Datatype datatype, MPI_Message * message, MPI_Request * request);

int QMPI_Ineighbor_allgather(QMPI_Context context, int tool_id, const void *sendbuf, int sendcount,
                             MPI_Datatype sendtype, void *recvbuf, int recvcount,
                             MPI_Datatype recvtype, MPI_Comm comm,
                             MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_Ineighbor_allgather_t) (QMPI_Context context, int tool_id, const void *sendbuf,
                                          int sendcount, MPI_Datatype sendtype, void *recvbuf,
                                          int recvcount, MPI_Datatype recvtype, MPI_Comm comm,
                                          MPI_Request * request);

int QMPI_Ineighbor_allgatherv(QMPI_Context context, int tool_id, const void *sendbuf, int sendcount,
                              MPI_Datatype sendtype, void *recvbuf, const int recvcounts[],
                              const int displs[], MPI_Datatype recvtype, MPI_Comm comm,
                              MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_Ineighbor_allgatherv_t) (QMPI_Context context, int tool_id, const void *sendbuf,
                                           int sendcount, MPI_Datatype sendtype, void *recvbuf,
                                           const int recvcounts[], const int displs[],
                                           MPI_Datatype recvtype, MPI_Comm comm,
                                           MPI_Request * request);

int QMPI_Ineighbor_alltoall(QMPI_Context context, int tool_id, const void *sendbuf, int sendcount,
                            MPI_Datatype sendtype, void *recvbuf, int recvcount,
                            MPI_Datatype recvtype, MPI_Comm comm,
                            MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_Ineighbor_alltoall_t) (QMPI_Context context, int tool_id, const void *sendbuf,
                                         int sendcount, MPI_Datatype sendtype, void *recvbuf,
                                         int recvcount, MPI_Datatype recvtype, MPI_Comm comm,
                                         MPI_Request * request);

int QMPI_Ineighbor_alltoallv(QMPI_Context context, int tool_id, const void *sendbuf,
                             const int sendcounts[], const int sdispls[], MPI_Datatype sendtype,
                             void *recvbuf, const int recvcounts[], const int rdispls[],
                             MPI_Datatype recvtype, MPI_Comm comm,
                             MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_Ineighbor_alltoallv_t) (QMPI_Context context, int tool_id, const void *sendbuf,
                                          const int sendcounts[], const int sdispls[],
                                          MPI_Datatype sendtype, void *recvbuf,
                                          const int recvcounts[], const int rdispls[],
                                          MPI_Datatype recvtype, MPI_Comm comm,
                                          MPI_Request * request);

int QMPI_Ineighbor_alltoallw(QMPI_Context context, int tool_id, const void *sendbuf,
                             const int sendcounts[], const MPI_Aint sdispls[],
                             const MPI_Datatype sendtypes[], void *recvbuf, const int recvcounts[],
                             const MPI_Aint rdispls[], const MPI_Datatype recvtypes[],
                             MPI_Comm comm, MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_Ineighbor_alltoallw_t) (QMPI_Context context, int tool_id, const void *sendbuf,
                                          const int sendcounts[], const MPI_Aint sdispls[],
                                          const MPI_Datatype sendtypes[], void *recvbuf,
                                          const int recvcounts[], const MPI_Aint rdispls[],
                                          const MPI_Datatype recvtypes[], MPI_Comm comm,
                                          MPI_Request * request);

int QMPI_Info_create(QMPI_Context context, int tool_id, MPI_Info * info) MPICH_API_PUBLIC;
typedef int (QMPI_Info_create_t) (QMPI_Context context, int tool_id, MPI_Info * info);

int QMPI_Info_delete(QMPI_Context context, int tool_id, MPI_Info info,
                     const char *key) MPICH_API_PUBLIC;
typedef int (QMPI_Info_delete_t) (QMPI_Context context, int tool_id, MPI_Info info,
                                  const char *key);

int QMPI_Info_dup(QMPI_Context context, int tool_id, MPI_Info info,
                  MPI_Info * newinfo) MPICH_API_PUBLIC;
typedef int (QMPI_Info_dup_t) (QMPI_Context context, int tool_id, MPI_Info info,
                               MPI_Info * newinfo);

int QMPI_Info_free(QMPI_Context context, int tool_id, MPI_Info * info) MPICH_API_PUBLIC;
typedef int (QMPI_Info_free_t) (QMPI_Context context, int tool_id, MPI_Info * info);

int QMPI_Info_get(QMPI_Context context, int tool_id, MPI_Info info, const char *key, int valuelen,
                  char *value, int *flag) MPICH_API_PUBLIC;
typedef int (QMPI_Info_get_t) (QMPI_Context context, int tool_id, MPI_Info info, const char *key,
                               int valuelen, char *value, int *flag);

int QMPI_Info_get_nkeys(QMPI_Context context, int tool_id, MPI_Info info,
                        int *nkeys) MPICH_API_PUBLIC;
typedef int (QMPI_Info_get_nkeys_t) (QMPI_Context context, int tool_id, MPI_Info info, int *nkeys);

int QMPI_Info_get_nthkey(QMPI_Context context, int tool_id, MPI_Info info, int n,
                         char *key) MPICH_API_PUBLIC;
typedef int (QMPI_Info_get_nthkey_t) (QMPI_Context context, int tool_id, MPI_Info info, int n,
                                      char *key);

int QMPI_Info_get_valuelen(QMPI_Context context, int tool_id, MPI_Info info, const char *key,
                           int *valuelen, int *flag) MPICH_API_PUBLIC;
typedef int (QMPI_Info_get_valuelen_t) (QMPI_Context context, int tool_id, MPI_Info info,
                                        const char *key, int *valuelen, int *flag);

int QMPI_Info_set(QMPI_Context context, int tool_id, MPI_Info info, const char *key,
                  const char *value) MPICH_API_PUBLIC;
typedef int (QMPI_Info_set_t) (QMPI_Context context, int tool_id, MPI_Info info, const char *key,
                               const char *value);

int QMPI_Init(QMPI_Context context, int tool_id, int *argc, char ***argv) MPICH_API_PUBLIC;
typedef int (QMPI_Init_t) (QMPI_Context context, int tool_id, int *argc, char ***argv);

int QMPI_Init_thread(QMPI_Context context, int tool_id, int *argc, char ***argv, int required,
                     int *provided) MPICH_API_PUBLIC;
typedef int (QMPI_Init_thread_t) (QMPI_Context context, int tool_id, int *argc, char ***argv,
                                  int required, int *provided);

int QMPI_Initialized(QMPI_Context context, int tool_id, int *flag) MPICH_API_PUBLIC;
typedef int (QMPI_Initialized_t) (QMPI_Context context, int tool_id, int *flag);

int QMPI_Intercomm_create(QMPI_Context context, int tool_id, MPI_Comm local_comm, int local_leader,
                          MPI_Comm peer_comm, int remote_leader, int tag,
                          MPI_Comm * newintercomm) MPICH_API_PUBLIC;
typedef int (QMPI_Intercomm_create_t) (QMPI_Context context, int tool_id, MPI_Comm local_comm,
                                       int local_leader, MPI_Comm peer_comm, int remote_leader,
                                       int tag, MPI_Comm * newintercomm);

int QMPI_Intercomm_merge(QMPI_Context context, int tool_id, MPI_Comm intercomm, int high,
                         MPI_Comm * newintracomm) MPICH_API_PUBLIC;
typedef int (QMPI_Intercomm_merge_t) (QMPI_Context context, int tool_id, MPI_Comm intercomm,
                                      int high, MPI_Comm * newintracomm);

int QMPI_Iprobe(QMPI_Context context, int tool_id, int source, int tag, MPI_Comm comm, int *flag,
                MPI_Status * status) MPICH_API_PUBLIC;
typedef int (QMPI_Iprobe_t) (QMPI_Context context, int tool_id, int source, int tag, MPI_Comm comm,
                             int *flag, MPI_Status * status);

int QMPI_Irecv(QMPI_Context context, int tool_id, void *buf, int count, MPI_Datatype datatype,
               int source, int tag, MPI_Comm comm, MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_Irecv_t) (QMPI_Context context, int tool_id, void *buf, int count,
                            MPI_Datatype datatype, int source, int tag, MPI_Comm comm,
                            MPI_Request * request);

int QMPI_Ireduce(QMPI_Context context, int tool_id, const void *sendbuf, void *recvbuf, int count,
                 MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm,
                 MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_Ireduce_t) (QMPI_Context context, int tool_id, const void *sendbuf, void *recvbuf,
                              int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm,
                              MPI_Request * request);

int QMPI_Ireduce_scatter(QMPI_Context context, int tool_id, const void *sendbuf, void *recvbuf,
                         const int recvcounts[], MPI_Datatype datatype, MPI_Op op, MPI_Comm comm,
                         MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_Ireduce_scatter_t) (QMPI_Context context, int tool_id, const void *sendbuf,
                                      void *recvbuf, const int recvcounts[], MPI_Datatype datatype,
                                      MPI_Op op, MPI_Comm comm, MPI_Request * request);

int QMPI_Ireduce_scatter_block(QMPI_Context context, int tool_id, const void *sendbuf,
                               void *recvbuf, int recvcount, MPI_Datatype datatype, MPI_Op op,
                               MPI_Comm comm, MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_Ireduce_scatter_block_t) (QMPI_Context context, int tool_id, const void *sendbuf,
                                            void *recvbuf, int recvcount, MPI_Datatype datatype,
                                            MPI_Op op, MPI_Comm comm, MPI_Request * request);

int QMPI_Irsend(QMPI_Context context, int tool_id, const void *buf, int count,
                MPI_Datatype datatype, int dest, int tag, MPI_Comm comm,
                MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_Irsend_t) (QMPI_Context context, int tool_id, const void *buf, int count,
                             MPI_Datatype datatype, int dest, int tag, MPI_Comm comm,
                             MPI_Request * request);

int QMPI_Is_thread_main(QMPI_Context context, int tool_id, int *flag) MPICH_API_PUBLIC;
typedef int (QMPI_Is_thread_main_t) (QMPI_Context context, int tool_id, int *flag);

int QMPI_Iscan(QMPI_Context context, int tool_id, const void *sendbuf, void *recvbuf, int count,
               MPI_Datatype datatype, MPI_Op op, MPI_Comm comm,
               MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_Iscan_t) (QMPI_Context context, int tool_id, const void *sendbuf, void *recvbuf,
                            int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm,
                            MPI_Request * request);

int QMPI_Iscatter(QMPI_Context context, int tool_id, const void *sendbuf, int sendcount,
                  MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype,
                  int root, MPI_Comm comm, MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_Iscatter_t) (QMPI_Context context, int tool_id, const void *sendbuf,
                               int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount,
                               MPI_Datatype recvtype, int root, MPI_Comm comm,
                               MPI_Request * request);

int QMPI_Iscatterv(QMPI_Context context, int tool_id, const void *sendbuf, const int sendcounts[],
                   const int displs[], MPI_Datatype sendtype, void *recvbuf, int recvcount,
                   MPI_Datatype recvtype, int root, MPI_Comm comm,
                   MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_Iscatterv_t) (QMPI_Context context, int tool_id, const void *sendbuf,
                                const int sendcounts[], const int displs[], MPI_Datatype sendtype,
                                void *recvbuf, int recvcount, MPI_Datatype recvtype, int root,
                                MPI_Comm comm, MPI_Request * request);

int QMPI_Isend(QMPI_Context context, int tool_id, const void *buf, int count, MPI_Datatype datatype,
               int dest, int tag, MPI_Comm comm, MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_Isend_t) (QMPI_Context context, int tool_id, const void *buf, int count,
                            MPI_Datatype datatype, int dest, int tag, MPI_Comm comm,
                            MPI_Request * request);

int QMPI_Issend(QMPI_Context context, int tool_id, const void *buf, int count,
                MPI_Datatype datatype, int dest, int tag, MPI_Comm comm,
                MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_Issend_t) (QMPI_Context context, int tool_id, const void *buf, int count,
                             MPI_Datatype datatype, int dest, int tag, MPI_Comm comm,
                             MPI_Request * request);

int QMPI_Keyval_create(QMPI_Context context, int tool_id, MPI_Copy_function * copy_fn,
                       MPI_Delete_function * delete_fn, int *keyval,
                       void *extra_state) MPICH_API_PUBLIC;
typedef int (QMPI_Keyval_create_t) (QMPI_Context context, int tool_id, MPI_Copy_function * copy_fn,
                                    MPI_Delete_function * delete_fn, int *keyval,
                                    void *extra_state);

int QMPI_Keyval_free(QMPI_Context context, int tool_id, int *keyval) MPICH_API_PUBLIC;
typedef int (QMPI_Keyval_free_t) (QMPI_Context context, int tool_id, int *keyval);

int QMPI_Lookup_name(QMPI_Context context, int tool_id, const char *service_name, MPI_Info info,
                     char *port_name) MPICH_API_PUBLIC;
typedef int (QMPI_Lookup_name_t) (QMPI_Context context, int tool_id, const char *service_name,
                                  MPI_Info info, char *port_name);

int QMPI_Mprobe(QMPI_Context context, int tool_id, int source, int tag, MPI_Comm comm,
                MPI_Message * message, MPI_Status * status) MPICH_API_PUBLIC;
typedef int (QMPI_Mprobe_t) (QMPI_Context context, int tool_id, int source, int tag, MPI_Comm comm,
                             MPI_Message * message, MPI_Status * status);

int QMPI_Mrecv(QMPI_Context context, int tool_id, void *buf, int count, MPI_Datatype datatype,
               MPI_Message * message, MPI_Status * status) MPICH_API_PUBLIC;
typedef int (QMPI_Mrecv_t) (QMPI_Context context, int tool_id, void *buf, int count,
                            MPI_Datatype datatype, MPI_Message * message, MPI_Status * status);

int QMPI_Neighbor_allgather(QMPI_Context context, int tool_id, const void *sendbuf, int sendcount,
                            MPI_Datatype sendtype, void *recvbuf, int recvcount,
                            MPI_Datatype recvtype, MPI_Comm comm) MPICH_API_PUBLIC;
typedef int (QMPI_Neighbor_allgather_t) (QMPI_Context context, int tool_id, const void *sendbuf,
                                         int sendcount, MPI_Datatype sendtype, void *recvbuf,
                                         int recvcount, MPI_Datatype recvtype, MPI_Comm comm);

int QMPI_Neighbor_allgatherv(QMPI_Context context, int tool_id, const void *sendbuf, int sendcount,
                             MPI_Datatype sendtype, void *recvbuf, const int recvcounts[],
                             const int displs[], MPI_Datatype recvtype,
                             MPI_Comm comm) MPICH_API_PUBLIC;
typedef int (QMPI_Neighbor_allgatherv_t) (QMPI_Context context, int tool_id, const void *sendbuf,
                                          int sendcount, MPI_Datatype sendtype, void *recvbuf,
                                          const int recvcounts[], const int displs[],
                                          MPI_Datatype recvtype, MPI_Comm comm);

int QMPI_Neighbor_alltoall(QMPI_Context context, int tool_id, const void *sendbuf, int sendcount,
                           MPI_Datatype sendtype, void *recvbuf, int recvcount,
                           MPI_Datatype recvtype, MPI_Comm comm) MPICH_API_PUBLIC;
typedef int (QMPI_Neighbor_alltoall_t) (QMPI_Context context, int tool_id, const void *sendbuf,
                                        int sendcount, MPI_Datatype sendtype, void *recvbuf,
                                        int recvcount, MPI_Datatype recvtype, MPI_Comm comm);

int QMPI_Neighbor_alltoallv(QMPI_Context context, int tool_id, const void *sendbuf,
                            const int sendcounts[], const int sdispls[], MPI_Datatype sendtype,
                            void *recvbuf, const int recvcounts[], const int rdispls[],
                            MPI_Datatype recvtype, MPI_Comm comm) MPICH_API_PUBLIC;
typedef int (QMPI_Neighbor_alltoallv_t) (QMPI_Context context, int tool_id, const void *sendbuf,
                                         const int sendcounts[], const int sdispls[],
                                         MPI_Datatype sendtype, void *recvbuf,
                                         const int recvcounts[], const int rdispls[],
                                         MPI_Datatype recvtype, MPI_Comm comm);

int QMPI_Neighbor_alltoallw(QMPI_Context context, int tool_id, const void *sendbuf,
                            const int sendcounts[], const MPI_Aint sdispls[],
                            const MPI_Datatype sendtypes[], void *recvbuf, const int recvcounts[],
                            const MPI_Aint rdispls[], const MPI_Datatype recvtypes[],
                            MPI_Comm comm) MPICH_API_PUBLIC;
typedef int (QMPI_Neighbor_alltoallw_t) (QMPI_Context context, int tool_id, const void *sendbuf,
                                         const int sendcounts[], const MPI_Aint sdispls[],
                                         const MPI_Datatype sendtypes[], void *recvbuf,
                                         const int recvcounts[], const MPI_Aint rdispls[],
                                         const MPI_Datatype recvtypes[], MPI_Comm comm);

int QMPI_Op_commutative(QMPI_Context context, int tool_id, MPI_Op op,
                        int *commute) MPICH_API_PUBLIC;
typedef int (QMPI_Op_commutative_t) (QMPI_Context context, int tool_id, MPI_Op op, int *commute);

int QMPI_Op_create(QMPI_Context context, int tool_id, MPI_User_function * user_fn, int commute,
                   MPI_Op * op) MPICH_API_PUBLIC;
typedef int (QMPI_Op_create_t) (QMPI_Context context, int tool_id, MPI_User_function * user_fn,
                                int commute, MPI_Op * op);

int QMPI_Op_free(QMPI_Context context, int tool_id, MPI_Op * op) MPICH_API_PUBLIC;
typedef int (QMPI_Op_free_t) (QMPI_Context context, int tool_id, MPI_Op * op);

int QMPI_Open_port(QMPI_Context context, int tool_id, MPI_Info info,
                   char *port_name) MPICH_API_PUBLIC;
typedef int (QMPI_Open_port_t) (QMPI_Context context, int tool_id, MPI_Info info, char *port_name);

int QMPI_Pack(QMPI_Context context, int tool_id, const void *inbuf, int incount,
              MPI_Datatype datatype, void *outbuf, int outsize, int *position,
              MPI_Comm comm) MPICH_API_PUBLIC;
typedef int (QMPI_Pack_t) (QMPI_Context context, int tool_id, const void *inbuf, int incount,
                           MPI_Datatype datatype, void *outbuf, int outsize, int *position,
                           MPI_Comm comm);

int QMPI_Pack_external(QMPI_Context context, int tool_id, const char datarep[], const void *inbuf,
                       int incount, MPI_Datatype datatype, void *outbuf, MPI_Aint outsize,
                       MPI_Aint * position) MPICH_API_PUBLIC;
typedef int (QMPI_Pack_external_t) (QMPI_Context context, int tool_id, const char datarep[],
                                    const void *inbuf, int incount, MPI_Datatype datatype,
                                    void *outbuf, MPI_Aint outsize, MPI_Aint * position);

int QMPI_Pack_external_size(QMPI_Context context, int tool_id, const char datarep[], int incount,
                            MPI_Datatype datatype, MPI_Aint * size) MPICH_API_PUBLIC;
typedef int (QMPI_Pack_external_size_t) (QMPI_Context context, int tool_id, const char datarep[],
                                         int incount, MPI_Datatype datatype, MPI_Aint * size);

int QMPI_Pack_size(QMPI_Context context, int tool_id, int incount, MPI_Datatype datatype,
                   MPI_Comm comm, int *size) MPICH_API_PUBLIC;
typedef int (QMPI_Pack_size_t) (QMPI_Context context, int tool_id, int incount,
                                MPI_Datatype datatype, MPI_Comm comm, int *size);

int QMPI_Pcontrol(QMPI_Context context, int tool_id, const int level, ...) MPICH_API_PUBLIC;
typedef int (QMPI_Pcontrol_t) (QMPI_Context context, int tool_id, const int level, ...);

int QMPI_Probe(QMPI_Context context, int tool_id, int source, int tag, MPI_Comm comm,
               MPI_Status * status) MPICH_API_PUBLIC;
typedef int (QMPI_Probe_t) (QMPI_Context context, int tool_id, int source, int tag, MPI_Comm comm,
                            MPI_Status * status);

int QMPI_Publish_name(QMPI_Context context, int tool_id, const char *service_name, MPI_Info info,
                      const char *port_name) MPICH_API_PUBLIC;
typedef int (QMPI_Publish_name_t) (QMPI_Context context, int tool_id, const char *service_name,
                                   MPI_Info info, const char *port_name);

int QMPI_Put(QMPI_Context context, int tool_id, const void *origin_addr, int origin_count,
             MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp, int target_count,
             MPI_Datatype target_datatype, MPI_Win win) MPICH_API_PUBLIC;
typedef int (QMPI_Put_t) (QMPI_Context context, int tool_id, const void *origin_addr,
                          int origin_count, MPI_Datatype origin_datatype, int target_rank,
                          MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype,
                          MPI_Win win);

int QMPI_Query_thread(QMPI_Context context, int tool_id, int *provided) MPICH_API_PUBLIC;
typedef int (QMPI_Query_thread_t) (QMPI_Context context, int tool_id, int *provided);

int QMPI_Raccumulate(QMPI_Context context, int tool_id, const void *origin_addr, int origin_count,
                     MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp,
                     int target_count, MPI_Datatype target_datatype, MPI_Op op, MPI_Win win,
                     MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_Raccumulate_t) (QMPI_Context context, int tool_id, const void *origin_addr,
                                  int origin_count, MPI_Datatype origin_datatype, int target_rank,
                                  MPI_Aint target_disp, int target_count,
                                  MPI_Datatype target_datatype, MPI_Op op, MPI_Win win,
                                  MPI_Request * request);

int QMPI_Recv(QMPI_Context context, int tool_id, void *buf, int count, MPI_Datatype datatype,
              int source, int tag, MPI_Comm comm, MPI_Status * status) MPICH_API_PUBLIC;
typedef int (QMPI_Recv_t) (QMPI_Context context, int tool_id, void *buf, int count,
                           MPI_Datatype datatype, int source, int tag, MPI_Comm comm,
                           MPI_Status * status);

int QMPI_Recv_init(QMPI_Context context, int tool_id, void *buf, int count, MPI_Datatype datatype,
                   int source, int tag, MPI_Comm comm, MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_Recv_init_t) (QMPI_Context context, int tool_id, void *buf, int count,
                                MPI_Datatype datatype, int source, int tag, MPI_Comm comm,
                                MPI_Request * request);

int QMPI_Reduce(QMPI_Context context, int tool_id, const void *sendbuf, void *recvbuf, int count,
                MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm) MPICH_API_PUBLIC;
typedef int (QMPI_Reduce_t) (QMPI_Context context, int tool_id, const void *sendbuf, void *recvbuf,
                             int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm);

int QMPI_Reduce_local(QMPI_Context context, int tool_id, const void *inbuf, void *inoutbuf,
                      int count, MPI_Datatype datatype, MPI_Op op) MPICH_API_PUBLIC;
typedef int (QMPI_Reduce_local_t) (QMPI_Context context, int tool_id, const void *inbuf,
                                   void *inoutbuf, int count, MPI_Datatype datatype, MPI_Op op);

int QMPI_Reduce_scatter(QMPI_Context context, int tool_id, const void *sendbuf, void *recvbuf,
                        const int recvcounts[], MPI_Datatype datatype, MPI_Op op,
                        MPI_Comm comm) MPICH_API_PUBLIC;
typedef int (QMPI_Reduce_scatter_t) (QMPI_Context context, int tool_id, const void *sendbuf,
                                     void *recvbuf, const int recvcounts[], MPI_Datatype datatype,
                                     MPI_Op op, MPI_Comm comm);

int QMPI_Reduce_scatter_block(QMPI_Context context, int tool_id, const void *sendbuf, void *recvbuf,
                              int recvcount, MPI_Datatype datatype, MPI_Op op,
                              MPI_Comm comm) MPICH_API_PUBLIC;
typedef int (QMPI_Reduce_scatter_block_t) (QMPI_Context context, int tool_id, const void *sendbuf,
                                           void *recvbuf, int recvcount, MPI_Datatype datatype,
                                           MPI_Op op, MPI_Comm comm);

int QMPI_Request_free(QMPI_Context context, int tool_id, MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_Request_free_t) (QMPI_Context context, int tool_id, MPI_Request * request);

int QMPI_Request_get_status(QMPI_Context context, int tool_id, MPI_Request request, int *flag,
                            MPI_Status * status) MPICH_API_PUBLIC;
typedef int (QMPI_Request_get_status_t) (QMPI_Context context, int tool_id, MPI_Request request,
                                         int *flag, MPI_Status * status);

int QMPI_Rget(QMPI_Context context, int tool_id, void *origin_addr, int origin_count,
              MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp, int target_count,
              MPI_Datatype target_datatype, MPI_Win win, MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_Rget_t) (QMPI_Context context, int tool_id, void *origin_addr, int origin_count,
                           MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp,
                           int target_count, MPI_Datatype target_datatype, MPI_Win win,
                           MPI_Request * request);

int QMPI_Rget_accumulate(QMPI_Context context, int tool_id, const void *origin_addr,
                         int origin_count, MPI_Datatype origin_datatype, void *result_addr,
                         int result_count, MPI_Datatype result_datatype, int target_rank,
                         MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype,
                         MPI_Op op, MPI_Win win, MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_Rget_accumulate_t) (QMPI_Context context, int tool_id, const void *origin_addr,
                                      int origin_count, MPI_Datatype origin_datatype,
                                      void *result_addr, int result_count,
                                      MPI_Datatype result_datatype, int target_rank,
                                      MPI_Aint target_disp, int target_count,
                                      MPI_Datatype target_datatype, MPI_Op op, MPI_Win win,
                                      MPI_Request * request);

int QMPI_Rput(QMPI_Context context, int tool_id, const void *origin_addr, int origin_count,
              MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp, int target_count,
              MPI_Datatype target_datatype, MPI_Win win, MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_Rput_t) (QMPI_Context context, int tool_id, const void *origin_addr,
                           int origin_count, MPI_Datatype origin_datatype, int target_rank,
                           MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype,
                           MPI_Win win, MPI_Request * request);

int QMPI_Rsend(QMPI_Context context, int tool_id, const void *buf, int count, MPI_Datatype datatype,
               int dest, int tag, MPI_Comm comm) MPICH_API_PUBLIC;
typedef int (QMPI_Rsend_t) (QMPI_Context context, int tool_id, const void *buf, int count,
                            MPI_Datatype datatype, int dest, int tag, MPI_Comm comm);

int QMPI_Rsend_init(QMPI_Context context, int tool_id, const void *buf, int count,
                    MPI_Datatype datatype, int dest, int tag, MPI_Comm comm,
                    MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_Rsend_init_t) (QMPI_Context context, int tool_id, const void *buf, int count,
                                 MPI_Datatype datatype, int dest, int tag, MPI_Comm comm,
                                 MPI_Request * request);

int QMPI_Scan(QMPI_Context context, int tool_id, const void *sendbuf, void *recvbuf, int count,
              MPI_Datatype datatype, MPI_Op op, MPI_Comm comm) MPICH_API_PUBLIC;
typedef int (QMPI_Scan_t) (QMPI_Context context, int tool_id, const void *sendbuf, void *recvbuf,
                           int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);

int QMPI_Scatter(QMPI_Context context, int tool_id, const void *sendbuf, int sendcount,
                 MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype,
                 int root, MPI_Comm comm) MPICH_API_PUBLIC;
typedef int (QMPI_Scatter_t) (QMPI_Context context, int tool_id, const void *sendbuf, int sendcount,
                              MPI_Datatype sendtype, void *recvbuf, int recvcount,
                              MPI_Datatype recvtype, int root, MPI_Comm comm);

int QMPI_Scatterv(QMPI_Context context, int tool_id, const void *sendbuf, const int sendcounts[],
                  const int displs[], MPI_Datatype sendtype, void *recvbuf, int recvcount,
                  MPI_Datatype recvtype, int root, MPI_Comm comm) MPICH_API_PUBLIC;
typedef int (QMPI_Scatterv_t) (QMPI_Context context, int tool_id, const void *sendbuf,
                               const int sendcounts[], const int displs[], MPI_Datatype sendtype,
                               void *recvbuf, int recvcount, MPI_Datatype recvtype, int root,
                               MPI_Comm comm);

int QMPI_Send(QMPI_Context context, int tool_id, const void *buf, int count, MPI_Datatype datatype,
              int dest, int tag, MPI_Comm comm) MPICH_API_PUBLIC;
typedef int (QMPI_Send_t) (QMPI_Context context, int tool_id, const void *buf, int count,
                           MPI_Datatype datatype, int dest, int tag, MPI_Comm comm);

int QMPI_Send_init(QMPI_Context context, int tool_id, const void *buf, int count,
                   MPI_Datatype datatype, int dest, int tag, MPI_Comm comm,
                   MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_Send_init_t) (QMPI_Context context, int tool_id, const void *buf, int count,
                                MPI_Datatype datatype, int dest, int tag, MPI_Comm comm,
                                MPI_Request * request);

int QMPI_Sendrecv(QMPI_Context context, int tool_id, const void *sendbuf, int sendcount,
                  MPI_Datatype sendtype, int dest, int sendtag, void *recvbuf, int recvcount,
                  MPI_Datatype recvtype, int source, int recvtag, MPI_Comm comm,
                  MPI_Status * status) MPICH_API_PUBLIC;
typedef int (QMPI_Sendrecv_t) (QMPI_Context context, int tool_id, const void *sendbuf,
                               int sendcount, MPI_Datatype sendtype, int dest, int sendtag,
                               void *recvbuf, int recvcount, MPI_Datatype recvtype, int source,
                               int recvtag, MPI_Comm comm, MPI_Status * status);

int QMPI_Sendrecv_replace(QMPI_Context context, int tool_id, void *buf, int count,
                          MPI_Datatype datatype, int dest, int sendtag, int source, int recvtag,
                          MPI_Comm comm, MPI_Status * status) MPICH_API_PUBLIC;
typedef int (QMPI_Sendrecv_replace_t) (QMPI_Context context, int tool_id, void *buf, int count,
                                       MPI_Datatype datatype, int dest, int sendtag, int source,
                                       int recvtag, MPI_Comm comm, MPI_Status * status);

int QMPI_Ssend(QMPI_Context context, int tool_id, const void *buf, int count, MPI_Datatype datatype,
               int dest, int tag, MPI_Comm comm) MPICH_API_PUBLIC;
typedef int (QMPI_Ssend_t) (QMPI_Context context, int tool_id, const void *buf, int count,
                            MPI_Datatype datatype, int dest, int tag, MPI_Comm comm);

int QMPI_Ssend_init(QMPI_Context context, int tool_id, const void *buf, int count,
                    MPI_Datatype datatype, int dest, int tag, MPI_Comm comm,
                    MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_Ssend_init_t) (QMPI_Context context, int tool_id, const void *buf, int count,
                                 MPI_Datatype datatype, int dest, int tag, MPI_Comm comm,
                                 MPI_Request * request);

int QMPI_Start(QMPI_Context context, int tool_id, MPI_Request * request) MPICH_API_PUBLIC;
typedef int (QMPI_Start_t) (QMPI_Context context, int tool_id, MPI_Request * request);

int QMPI_Startall(QMPI_Context context, int tool_id, int count,
                  MPI_Request array_of_requests[]) MPICH_API_PUBLIC;
typedef int (QMPI_Startall_t) (QMPI_Context context, int tool_id, int count,
                               MPI_Request array_of_requests[]);

int QMPI_Status_set_cancelled(QMPI_Context context, int tool_id, MPI_Status * status,
                              int flag) MPICH_API_PUBLIC;
typedef int (QMPI_Status_set_cancelled_t) (QMPI_Context context, int tool_id, MPI_Status * status,
                                           int flag);

int QMPI_Status_set_elements(QMPI_Context context, int tool_id, MPI_Status * status,
                             MPI_Datatype datatype, int count) MPICH_API_PUBLIC;
typedef int (QMPI_Status_set_elements_t) (QMPI_Context context, int tool_id, MPI_Status * status,
                                          MPI_Datatype datatype, int count);

int QMPI_Status_set_elements_x(QMPI_Context context, int tool_id, MPI_Status * status,
                               MPI_Datatype datatype, MPI_Count count) MPICH_API_PUBLIC;
typedef int (QMPI_Status_set_elements_x_t) (QMPI_Context context, int tool_id, MPI_Status * status,
                                            MPI_Datatype datatype, MPI_Count count);

int QMPI_T_category_changed(QMPI_Context context, int tool_id, int *stamp) MPICH_API_PUBLIC;
typedef int (QMPI_T_category_changed_t) (QMPI_Context context, int tool_id, int *stamp);

int QMPI_T_category_get_categories(QMPI_Context context, int tool_id, int cat_index, int len,
                                   int indices[]) MPICH_API_PUBLIC;
typedef int (QMPI_T_category_get_categories_t) (QMPI_Context context, int tool_id, int cat_index,
                                                int len, int indices[]);

int QMPI_T_category_get_cvars(QMPI_Context context, int tool_id, int cat_index, int len,
                              int indices[]) MPICH_API_PUBLIC;
typedef int (QMPI_T_category_get_cvars_t) (QMPI_Context context, int tool_id, int cat_index,
                                           int len, int indices[]);

int QMPI_T_category_get_index(QMPI_Context context, int tool_id, const char *name,
                              int *cat_index) MPICH_API_PUBLIC;
typedef int (QMPI_T_category_get_index_t) (QMPI_Context context, int tool_id, const char *name,
                                           int *cat_index);

int QMPI_T_category_get_info(QMPI_Context context, int tool_id, int cat_index, char *name,
                             int *name_len, char *desc, int *desc_len, int *num_cvars,
                             int *num_pvars, int *num_categories) MPICH_API_PUBLIC;
typedef int (QMPI_T_category_get_info_t) (QMPI_Context context, int tool_id, int cat_index,
                                          char *name, int *name_len, char *desc, int *desc_len,
                                          int *num_cvars, int *num_pvars, int *num_categories);

int QMPI_T_category_get_num(QMPI_Context context, int tool_id, int *num_cat) MPICH_API_PUBLIC;
typedef int (QMPI_T_category_get_num_t) (QMPI_Context context, int tool_id, int *num_cat);

int QMPI_T_category_get_pvars(QMPI_Context context, int tool_id, int cat_index, int len,
                              int indices[]) MPICH_API_PUBLIC;
typedef int (QMPI_T_category_get_pvars_t) (QMPI_Context context, int tool_id, int cat_index,
                                           int len, int indices[]);

int QMPI_T_cvar_get_index(QMPI_Context context, int tool_id, const char *name,
                          int *cvar_index) MPICH_API_PUBLIC;
typedef int (QMPI_T_cvar_get_index_t) (QMPI_Context context, int tool_id, const char *name,
                                       int *cvar_index);

int QMPI_T_cvar_get_info(QMPI_Context context, int tool_id, int cvar_index, char *name,
                         int *name_len, int *verbosity, MPI_Datatype * datatype,
                         MPI_T_enum * enumtype, char *desc, int *desc_len, int *bind,
                         int *scope) MPICH_API_PUBLIC;
typedef int (QMPI_T_cvar_get_info_t) (QMPI_Context context, int tool_id, int cvar_index, char *name,
                                      int *name_len, int *verbosity, MPI_Datatype * datatype,
                                      MPI_T_enum * enumtype, char *desc, int *desc_len, int *bind,
                                      int *scope);

int QMPI_T_cvar_get_num(QMPI_Context context, int tool_id, int *num_cvar) MPICH_API_PUBLIC;
typedef int (QMPI_T_cvar_get_num_t) (QMPI_Context context, int tool_id, int *num_cvar);

int QMPI_T_cvar_handle_alloc(QMPI_Context context, int tool_id, int cvar_index, void *obj_handle,
                             MPI_T_cvar_handle * handle, int *count) MPICH_API_PUBLIC;
typedef int (QMPI_T_cvar_handle_alloc_t) (QMPI_Context context, int tool_id, int cvar_index,
                                          void *obj_handle, MPI_T_cvar_handle * handle, int *count);

int QMPI_T_cvar_handle_free(QMPI_Context context, int tool_id,
                            MPI_T_cvar_handle * handle) MPICH_API_PUBLIC;
typedef int (QMPI_T_cvar_handle_free_t) (QMPI_Context context, int tool_id,
                                         MPI_T_cvar_handle * handle);

int QMPI_T_cvar_read(QMPI_Context context, int tool_id, MPI_T_cvar_handle handle,
                     void *buf) MPICH_API_PUBLIC;
typedef int (QMPI_T_cvar_read_t) (QMPI_Context context, int tool_id, MPI_T_cvar_handle handle,
                                  void *buf);

int QMPI_T_cvar_write(QMPI_Context context, int tool_id, MPI_T_cvar_handle handle,
                      const void *buf) MPICH_API_PUBLIC;
typedef int (QMPI_T_cvar_write_t) (QMPI_Context context, int tool_id, MPI_T_cvar_handle handle,
                                   const void *buf);

int QMPI_T_enum_get_info(QMPI_Context context, int tool_id, MPI_T_enum enumtype, int *num,
                         char *name, int *name_len) MPICH_API_PUBLIC;
typedef int (QMPI_T_enum_get_info_t) (QMPI_Context context, int tool_id, MPI_T_enum enumtype,
                                      int *num, char *name, int *name_len);

int QMPI_T_enum_get_item(QMPI_Context context, int tool_id, MPI_T_enum enumtype, int index,
                         int *value, char *name, int *name_len) MPICH_API_PUBLIC;
typedef int (QMPI_T_enum_get_item_t) (QMPI_Context context, int tool_id, MPI_T_enum enumtype,
                                      int index, int *value, char *name, int *name_len);

int QMPI_T_finalize(QMPI_Context context, int tool_id) MPICH_API_PUBLIC;
typedef int (QMPI_T_finalize_t) (QMPI_Context context, int tool_id);

int QMPI_T_init_thread(QMPI_Context context, int tool_id, int required,
                       int *provided) MPICH_API_PUBLIC;
typedef int (QMPI_T_init_thread_t) (QMPI_Context context, int tool_id, int required, int *provided);

int QMPI_T_pvar_get_index(QMPI_Context context, int tool_id, const char *name, int var_class,
                          int *pvar_index) MPICH_API_PUBLIC;
typedef int (QMPI_T_pvar_get_index_t) (QMPI_Context context, int tool_id, const char *name,
                                       int var_class, int *pvar_index);

int QMPI_T_pvar_get_info(QMPI_Context context, int tool_id, int pvar_index, char *name,
                         int *name_len, int *verbosity, int *var_class, MPI_Datatype * datatype,
                         MPI_T_enum * enumtype, char *desc, int *desc_len, int *bind, int *readonly,
                         int *continuous, int *atomic) MPICH_API_PUBLIC;
typedef int (QMPI_T_pvar_get_info_t) (QMPI_Context context, int tool_id, int pvar_index, char *name,
                                      int *name_len, int *verbosity, int *var_class,
                                      MPI_Datatype * datatype, MPI_T_enum * enumtype, char *desc,
                                      int *desc_len, int *bind, int *readonly, int *continuous,
                                      int *atomic);

int QMPI_T_pvar_get_num(QMPI_Context context, int tool_id, int *num_pvar) MPICH_API_PUBLIC;
typedef int (QMPI_T_pvar_get_num_t) (QMPI_Context context, int tool_id, int *num_pvar);

int QMPI_T_pvar_handle_alloc(QMPI_Context context, int tool_id, MPI_T_pvar_session session,
                             int pvar_index, void *obj_handle, MPI_T_pvar_handle * handle,
                             int *count) MPICH_API_PUBLIC;
typedef int (QMPI_T_pvar_handle_alloc_t) (QMPI_Context context, int tool_id,
                                          MPI_T_pvar_session session, int pvar_index,
                                          void *obj_handle, MPI_T_pvar_handle * handle, int *count);

int QMPI_T_pvar_handle_free(QMPI_Context context, int tool_id, MPI_T_pvar_session session,
                            MPI_T_pvar_handle * handle) MPICH_API_PUBLIC;
typedef int (QMPI_T_pvar_handle_free_t) (QMPI_Context context, int tool_id,
                                         MPI_T_pvar_session session, MPI_T_pvar_handle * handle);

int QMPI_T_pvar_read(QMPI_Context context, int tool_id, MPI_T_pvar_session session,
                     MPI_T_pvar_handle handle, void *buf) MPICH_API_PUBLIC;
typedef int (QMPI_T_pvar_read_t) (QMPI_Context context, int tool_id, MPI_T_pvar_session session,
                                  MPI_T_pvar_handle handle, void *buf);

int QMPI_T_pvar_readreset(QMPI_Context context, int tool_id, MPI_T_pvar_session session,
                          MPI_T_pvar_handle handle, void *buf) MPICH_API_PUBLIC;
typedef int (QMPI_T_pvar_readreset_t) (QMPI_Context context, int tool_id,
                                       MPI_T_pvar_session session, MPI_T_pvar_handle handle,
                                       void *buf);

int QMPI_T_pvar_reset(QMPI_Context context, int tool_id, MPI_T_pvar_session session,
                      MPI_T_pvar_handle handle) MPICH_API_PUBLIC;
typedef int (QMPI_T_pvar_reset_t) (QMPI_Context context, int tool_id, MPI_T_pvar_session session,
                                   MPI_T_pvar_handle handle);

int QMPI_T_pvar_session_create(QMPI_Context context, int tool_id,
                               MPI_T_pvar_session * session) MPICH_API_PUBLIC;
typedef int (QMPI_T_pvar_session_create_t) (QMPI_Context context, int tool_id,
                                            MPI_T_pvar_session * session);

int QMPI_T_pvar_session_free(QMPI_Context context, int tool_id,
                             MPI_T_pvar_session * session) MPICH_API_PUBLIC;
typedef int (QMPI_T_pvar_session_free_t) (QMPI_Context context, int tool_id,
                                          MPI_T_pvar_session * session);

int QMPI_T_pvar_start(QMPI_Context context, int tool_id, MPI_T_pvar_session session,
                      MPI_T_pvar_handle handle) MPICH_API_PUBLIC;
typedef int (QMPI_T_pvar_start_t) (QMPI_Context context, int tool_id, MPI_T_pvar_session session,
                                   MPI_T_pvar_handle handle);

int QMPI_T_pvar_stop(QMPI_Context context, int tool_id, MPI_T_pvar_session session,
                     MPI_T_pvar_handle handle) MPICH_API_PUBLIC;
typedef int (QMPI_T_pvar_stop_t) (QMPI_Context context, int tool_id, MPI_T_pvar_session session,
                                  MPI_T_pvar_handle handle);

int QMPI_T_pvar_write(QMPI_Context context, int tool_id, MPI_T_pvar_session session,
                      MPI_T_pvar_handle handle, const void *buf) MPICH_API_PUBLIC;
typedef int (QMPI_T_pvar_write_t) (QMPI_Context context, int tool_id, MPI_T_pvar_session session,
                                   MPI_T_pvar_handle handle, const void *buf);

int QMPI_Test(QMPI_Context context, int tool_id, MPI_Request * request, int *flag,
              MPI_Status * status) MPICH_API_PUBLIC;
typedef int (QMPI_Test_t) (QMPI_Context context, int tool_id, MPI_Request * request, int *flag,
                           MPI_Status * status);

int QMPI_Test_cancelled(QMPI_Context context, int tool_id, const MPI_Status * status,
                        int *flag) MPICH_API_PUBLIC;
typedef int (QMPI_Test_cancelled_t) (QMPI_Context context, int tool_id, const MPI_Status * status,
                                     int *flag);

int QMPI_Testall(QMPI_Context context, int tool_id, int count, MPI_Request array_of_requests[],
                 int *flag, MPI_Status array_of_statuses[]) MPICH_API_PUBLIC;
typedef int (QMPI_Testall_t) (QMPI_Context context, int tool_id, int count,
                              MPI_Request array_of_requests[], int *flag,
                              MPI_Status array_of_statuses[]);

int QMPI_Testany(QMPI_Context context, int tool_id, int count, MPI_Request array_of_requests[],
                 int *index, int *flag, MPI_Status * status) MPICH_API_PUBLIC;
typedef int (QMPI_Testany_t) (QMPI_Context context, int tool_id, int count,
                              MPI_Request array_of_requests[], int *index, int *flag,
                              MPI_Status * status);

int QMPI_Testsome(QMPI_Context context, int tool_id, int incount, MPI_Request array_of_requests[],
                  int *outcount, int array_of_indices[],
                  MPI_Status array_of_statuses[]) MPICH_API_PUBLIC;
typedef int (QMPI_Testsome_t) (QMPI_Context context, int tool_id, int incount,
                               MPI_Request array_of_requests[], int *outcount,
                               int array_of_indices[], MPI_Status array_of_statuses[]);

int QMPI_Topo_test(QMPI_Context context, int tool_id, MPI_Comm comm, int *status) MPICH_API_PUBLIC;
typedef int (QMPI_Topo_test_t) (QMPI_Context context, int tool_id, MPI_Comm comm, int *status);

int QMPI_Type_commit(QMPI_Context context, int tool_id, MPI_Datatype * datatype) MPICH_API_PUBLIC;
typedef int (QMPI_Type_commit_t) (QMPI_Context context, int tool_id, MPI_Datatype * datatype);

int QMPI_Type_contiguous(QMPI_Context context, int tool_id, int count, MPI_Datatype oldtype,
                         MPI_Datatype * newtype) MPICH_API_PUBLIC;
typedef int (QMPI_Type_contiguous_t) (QMPI_Context context, int tool_id, int count,
                                      MPI_Datatype oldtype, MPI_Datatype * newtype);

int QMPI_Type_create_darray(QMPI_Context context, int tool_id, int size, int rank, int ndims,
                            const int array_of_gsizes[], const int array_of_distribs[],
                            const int array_of_dargs[], const int array_of_psizes[], int order,
                            MPI_Datatype oldtype, MPI_Datatype * newtype) MPICH_API_PUBLIC;
typedef int (QMPI_Type_create_darray_t) (QMPI_Context context, int tool_id, int size, int rank,
                                         int ndims, const int array_of_gsizes[],
                                         const int array_of_distribs[], const int array_of_dargs[],
                                         const int array_of_psizes[], int order,
                                         MPI_Datatype oldtype, MPI_Datatype * newtype);

int QMPI_Type_create_f90_complex(QMPI_Context context, int tool_id, int p, int r,
                                 MPI_Datatype * newtype) MPICH_API_PUBLIC;
typedef int (QMPI_Type_create_f90_complex_t) (QMPI_Context context, int tool_id, int p, int r,
                                              MPI_Datatype * newtype);

int QMPI_Type_create_f90_integer(QMPI_Context context, int tool_id, int r,
                                 MPI_Datatype * newtype) MPICH_API_PUBLIC;
typedef int (QMPI_Type_create_f90_integer_t) (QMPI_Context context, int tool_id, int r,
                                              MPI_Datatype * newtype);

int QMPI_Type_create_f90_real(QMPI_Context context, int tool_id, int p, int r,
                              MPI_Datatype * newtype) MPICH_API_PUBLIC;
typedef int (QMPI_Type_create_f90_real_t) (QMPI_Context context, int tool_id, int p, int r,
                                           MPI_Datatype * newtype);

int QMPI_Type_create_hindexed(QMPI_Context context, int tool_id, int count,
                              const int array_of_blocklengths[],
                              const MPI_Aint array_of_displacements[], MPI_Datatype oldtype,
                              MPI_Datatype * newtype) MPICH_API_PUBLIC;
typedef int (QMPI_Type_create_hindexed_t) (QMPI_Context context, int tool_id, int count,
                                           const int array_of_blocklengths[],
                                           const MPI_Aint array_of_displacements[],
                                           MPI_Datatype oldtype, MPI_Datatype * newtype);

int QMPI_Type_create_hindexed_block(QMPI_Context context, int tool_id, int count, int blocklength,
                                    const MPI_Aint array_of_displacements[], MPI_Datatype oldtype,
                                    MPI_Datatype * newtype) MPICH_API_PUBLIC;
typedef int (QMPI_Type_create_hindexed_block_t) (QMPI_Context context, int tool_id, int count,
                                                 int blocklength,
                                                 const MPI_Aint array_of_displacements[],
                                                 MPI_Datatype oldtype, MPI_Datatype * newtype);

int QMPI_Type_create_hvector(QMPI_Context context, int tool_id, int count, int blocklength,
                             MPI_Aint stride, MPI_Datatype oldtype,
                             MPI_Datatype * newtype) MPICH_API_PUBLIC;
typedef int (QMPI_Type_create_hvector_t) (QMPI_Context context, int tool_id, int count,
                                          int blocklength, MPI_Aint stride, MPI_Datatype oldtype,
                                          MPI_Datatype * newtype);

int QMPI_Type_create_indexed_block(QMPI_Context context, int tool_id, int count, int blocklength,
                                   const int array_of_displacements[], MPI_Datatype oldtype,
                                   MPI_Datatype * newtype) MPICH_API_PUBLIC;
typedef int (QMPI_Type_create_indexed_block_t) (QMPI_Context context, int tool_id, int count,
                                                int blocklength, const int array_of_displacements[],
                                                MPI_Datatype oldtype, MPI_Datatype * newtype);

int QMPI_Type_create_keyval(QMPI_Context context, int tool_id,
                            MPI_Type_copy_attr_function * type_copy_attr_fn,
                            MPI_Type_delete_attr_function * type_delete_attr_fn, int *type_keyval,
                            void *extra_state) MPICH_API_PUBLIC;
typedef int (QMPI_Type_create_keyval_t) (QMPI_Context context, int tool_id,
                                         MPI_Type_copy_attr_function * type_copy_attr_fn,
                                         MPI_Type_delete_attr_function * type_delete_attr_fn,
                                         int *type_keyval, void *extra_state);

int QMPI_Type_create_resized(QMPI_Context context, int tool_id, MPI_Datatype oldtype, MPI_Aint lb,
                             MPI_Aint extent, MPI_Datatype * newtype) MPICH_API_PUBLIC;
typedef int (QMPI_Type_create_resized_t) (QMPI_Context context, int tool_id, MPI_Datatype oldtype,
                                          MPI_Aint lb, MPI_Aint extent, MPI_Datatype * newtype);

int QMPI_Type_create_struct(QMPI_Context context, int tool_id, int count,
                            const int array_of_blocklengths[],
                            const MPI_Aint array_of_displacements[],
                            const MPI_Datatype array_of_types[],
                            MPI_Datatype * newtype) MPICH_API_PUBLIC;
typedef int (QMPI_Type_create_struct_t) (QMPI_Context context, int tool_id, int count,
                                         const int array_of_blocklengths[],
                                         const MPI_Aint array_of_displacements[],
                                         const MPI_Datatype array_of_types[],
                                         MPI_Datatype * newtype);

int QMPI_Type_create_subarray(QMPI_Context context, int tool_id, int ndims,
                              const int array_of_sizes[], const int array_of_subsizes[],
                              const int array_of_starts[], int order, MPI_Datatype oldtype,
                              MPI_Datatype * newtype) MPICH_API_PUBLIC;
typedef int (QMPI_Type_create_subarray_t) (QMPI_Context context, int tool_id, int ndims,
                                           const int array_of_sizes[],
                                           const int array_of_subsizes[],
                                           const int array_of_starts[], int order,
                                           MPI_Datatype oldtype, MPI_Datatype * newtype);

int QMPI_Type_delete_attr(QMPI_Context context, int tool_id, MPI_Datatype datatype,
                          int type_keyval) MPICH_API_PUBLIC;
typedef int (QMPI_Type_delete_attr_t) (QMPI_Context context, int tool_id, MPI_Datatype datatype,
                                       int type_keyval);

int QMPI_Type_dup(QMPI_Context context, int tool_id, MPI_Datatype oldtype,
                  MPI_Datatype * newtype) MPICH_API_PUBLIC;
typedef int (QMPI_Type_dup_t) (QMPI_Context context, int tool_id, MPI_Datatype oldtype,
                               MPI_Datatype * newtype);

int QMPI_Type_free(QMPI_Context context, int tool_id, MPI_Datatype * datatype) MPICH_API_PUBLIC;
typedef int (QMPI_Type_free_t) (QMPI_Context context, int tool_id, MPI_Datatype * datatype);

int QMPI_Type_free_keyval(QMPI_Context context, int tool_id, int *type_keyval) MPICH_API_PUBLIC;
typedef int (QMPI_Type_free_keyval_t) (QMPI_Context context, int tool_id, int *type_keyval);

int QMPI_Type_get_attr(QMPI_Context context, int tool_id, MPI_Datatype datatype, int type_keyval,
                       void *attribute_val, int *flag) MPICH_API_PUBLIC;
typedef int (QMPI_Type_get_attr_t) (QMPI_Context context, int tool_id, MPI_Datatype datatype,
                                    int type_keyval, void *attribute_val, int *flag);

int QMPI_Type_get_contents(QMPI_Context context, int tool_id, MPI_Datatype datatype,
                           int max_integers, int max_addresses, int max_datatypes,
                           int array_of_integers[], MPI_Aint array_of_addresses[],
                           MPI_Datatype array_of_datatypes[]) MPICH_API_PUBLIC;
typedef int (QMPI_Type_get_contents_t) (QMPI_Context context, int tool_id, MPI_Datatype datatype,
                                        int max_integers, int max_addresses, int max_datatypes,
                                        int array_of_integers[], MPI_Aint array_of_addresses[],
                                        MPI_Datatype array_of_datatypes[]);

int QMPI_Type_get_envelope(QMPI_Context context, int tool_id, MPI_Datatype datatype,
                           int *num_integers, int *num_addresses, int *num_datatypes,
                           int *combiner) MPICH_API_PUBLIC;
typedef int (QMPI_Type_get_envelope_t) (QMPI_Context context, int tool_id, MPI_Datatype datatype,
                                        int *num_integers, int *num_addresses, int *num_datatypes,
                                        int *combiner);

int QMPI_Type_get_extent(QMPI_Context context, int tool_id, MPI_Datatype datatype, MPI_Aint * lb,
                         MPI_Aint * extent) MPICH_API_PUBLIC;
typedef int (QMPI_Type_get_extent_t) (QMPI_Context context, int tool_id, MPI_Datatype datatype,
                                      MPI_Aint * lb, MPI_Aint * extent);

int QMPI_Type_get_extent_x(QMPI_Context context, int tool_id, MPI_Datatype datatype, MPI_Count * lb,
                           MPI_Count * extent) MPICH_API_PUBLIC;
typedef int (QMPI_Type_get_extent_x_t) (QMPI_Context context, int tool_id, MPI_Datatype datatype,
                                        MPI_Count * lb, MPI_Count * extent);

int QMPI_Type_get_name(QMPI_Context context, int tool_id, MPI_Datatype datatype, char *type_name,
                       int *resultlen) MPICH_API_PUBLIC;
typedef int (QMPI_Type_get_name_t) (QMPI_Context context, int tool_id, MPI_Datatype datatype,
                                    char *type_name, int *resultlen);

int QMPI_Type_get_true_extent(QMPI_Context context, int tool_id, MPI_Datatype datatype,
                              MPI_Aint * true_lb, MPI_Aint * true_extent) MPICH_API_PUBLIC;
typedef int (QMPI_Type_get_true_extent_t) (QMPI_Context context, int tool_id, MPI_Datatype datatype,
                                           MPI_Aint * true_lb, MPI_Aint * true_extent);

int QMPI_Type_get_true_extent_x(QMPI_Context context, int tool_id, MPI_Datatype datatype,
                                MPI_Count * true_lb, MPI_Count * true_extent) MPICH_API_PUBLIC;
typedef int (QMPI_Type_get_true_extent_x_t) (QMPI_Context context, int tool_id,
                                             MPI_Datatype datatype, MPI_Count * true_lb,
                                             MPI_Count * true_extent);

int QMPI_Type_indexed(QMPI_Context context, int tool_id, int count,
                      const int array_of_blocklengths[], const int array_of_displacements[],
                      MPI_Datatype oldtype, MPI_Datatype * newtype) MPICH_API_PUBLIC;
typedef int (QMPI_Type_indexed_t) (QMPI_Context context, int tool_id, int count,
                                   const int array_of_blocklengths[],
                                   const int array_of_displacements[], MPI_Datatype oldtype,
                                   MPI_Datatype * newtype);

int QMPI_Type_match_size(QMPI_Context context, int tool_id, int typeclass, int size,
                         MPI_Datatype * datatype) MPICH_API_PUBLIC;
typedef int (QMPI_Type_match_size_t) (QMPI_Context context, int tool_id, int typeclass, int size,
                                      MPI_Datatype * datatype);

int QMPI_Type_set_attr(QMPI_Context context, int tool_id, MPI_Datatype datatype, int type_keyval,
                       void *attribute_val) MPICH_API_PUBLIC;
typedef int (QMPI_Type_set_attr_t) (QMPI_Context context, int tool_id, MPI_Datatype datatype,
                                    int type_keyval, void *attribute_val);

int QMPI_Type_set_name(QMPI_Context context, int tool_id, MPI_Datatype datatype,
                       const char *type_name) MPICH_API_PUBLIC;
typedef int (QMPI_Type_set_name_t) (QMPI_Context context, int tool_id, MPI_Datatype datatype,
                                    const char *type_name);

int QMPI_Type_size(QMPI_Context context, int tool_id, MPI_Datatype datatype,
                   int *size) MPICH_API_PUBLIC;
typedef int (QMPI_Type_size_t) (QMPI_Context context, int tool_id, MPI_Datatype datatype,
                                int *size);

int QMPI_Type_size_x(QMPI_Context context, int tool_id, MPI_Datatype datatype,
                     MPI_Count * size) MPICH_API_PUBLIC;
typedef int (QMPI_Type_size_x_t) (QMPI_Context context, int tool_id, MPI_Datatype datatype,
                                  MPI_Count * size);

int QMPI_Type_vector(QMPI_Context context, int tool_id, int count, int blocklength, int stride,
                     MPI_Datatype oldtype, MPI_Datatype * newtype) MPICH_API_PUBLIC;
typedef int (QMPI_Type_vector_t) (QMPI_Context context, int tool_id, int count, int blocklength,
                                  int stride, MPI_Datatype oldtype, MPI_Datatype * newtype);

int QMPI_Unpack(QMPI_Context context, int tool_id, const void *inbuf, int insize, int *position,
                void *outbuf, int outcount, MPI_Datatype datatype, MPI_Comm comm) MPICH_API_PUBLIC;
typedef int (QMPI_Unpack_t) (QMPI_Context context, int tool_id, const void *inbuf, int insize,
                             int *position, void *outbuf, int outcount, MPI_Datatype datatype,
                             MPI_Comm comm);

int QMPI_Unpack_external(QMPI_Context context, int tool_id, const char datarep[], const void *inbuf,
                         MPI_Aint insize, MPI_Aint * position, void *outbuf, int outcount,
                         MPI_Datatype datatype) MPICH_API_PUBLIC;
typedef int (QMPI_Unpack_external_t) (QMPI_Context context, int tool_id, const char datarep[],
                                      const void *inbuf, MPI_Aint insize, MPI_Aint * position,
                                      void *outbuf, int outcount, MPI_Datatype datatype);

int QMPI_Unpublish_name(QMPI_Context context, int tool_id, const char *service_name, MPI_Info info,
                        const char *port_name) MPICH_API_PUBLIC;
typedef int (QMPI_Unpublish_name_t) (QMPI_Context context, int tool_id, const char *service_name,
                                     MPI_Info info, const char *port_name);

int QMPI_Wait(QMPI_Context context, int tool_id, MPI_Request * request,
              MPI_Status * status) MPICH_API_PUBLIC;
typedef int (QMPI_Wait_t) (QMPI_Context context, int tool_id, MPI_Request * request,
                           MPI_Status * status);

int QMPI_Waitall(QMPI_Context context, int tool_id, int count, MPI_Request array_of_requests[],
                 MPI_Status array_of_statuses[]) MPICH_API_PUBLIC;
typedef int (QMPI_Waitall_t) (QMPI_Context context, int tool_id, int count,
                              MPI_Request array_of_requests[], MPI_Status array_of_statuses[]);

int QMPI_Waitany(QMPI_Context context, int tool_id, int count, MPI_Request array_of_requests[],
                 int *index, MPI_Status * status) MPICH_API_PUBLIC;
typedef int (QMPI_Waitany_t) (QMPI_Context context, int tool_id, int count,
                              MPI_Request array_of_requests[], int *index, MPI_Status * status);

int QMPI_Waitsome(QMPI_Context context, int tool_id, int incount, MPI_Request array_of_requests[],
                  int *outcount, int array_of_indices[],
                  MPI_Status array_of_statuses[]) MPICH_API_PUBLIC;
typedef int (QMPI_Waitsome_t) (QMPI_Context context, int tool_id, int incount,
                               MPI_Request array_of_requests[], int *outcount,
                               int array_of_indices[], MPI_Status array_of_statuses[]);

int QMPI_Win_allocate(QMPI_Context context, int tool_id, MPI_Aint size, int disp_unit,
                      MPI_Info info, MPI_Comm comm, void *baseptr, MPI_Win * win) MPICH_API_PUBLIC;
typedef int (QMPI_Win_allocate_t) (QMPI_Context context, int tool_id, MPI_Aint size, int disp_unit,
                                   MPI_Info info, MPI_Comm comm, void *baseptr, MPI_Win * win);

int QMPI_Win_allocate_shared(QMPI_Context context, int tool_id, MPI_Aint size, int disp_unit,
                             MPI_Info info, MPI_Comm comm, void *baseptr,
                             MPI_Win * win) MPICH_API_PUBLIC;
typedef int (QMPI_Win_allocate_shared_t) (QMPI_Context context, int tool_id, MPI_Aint size,
                                          int disp_unit, MPI_Info info, MPI_Comm comm,
                                          void *baseptr, MPI_Win * win);

int QMPI_Win_attach(QMPI_Context context, int tool_id, MPI_Win win, void *base,
                    MPI_Aint size) MPICH_API_PUBLIC;
typedef int (QMPI_Win_attach_t) (QMPI_Context context, int tool_id, MPI_Win win, void *base,
                                 MPI_Aint size);

int QMPI_Win_call_errhandler(QMPI_Context context, int tool_id, MPI_Win win,
                             int errorcode) MPICH_API_PUBLIC;
typedef int (QMPI_Win_call_errhandler_t) (QMPI_Context context, int tool_id, MPI_Win win,
                                          int errorcode);

int QMPI_Win_complete(QMPI_Context context, int tool_id, MPI_Win win) MPICH_API_PUBLIC;
typedef int (QMPI_Win_complete_t) (QMPI_Context context, int tool_id, MPI_Win win);

int QMPI_Win_create(QMPI_Context context, int tool_id, void *base, MPI_Aint size, int disp_unit,
                    MPI_Info info, MPI_Comm comm, MPI_Win * win) MPICH_API_PUBLIC;
typedef int (QMPI_Win_create_t) (QMPI_Context context, int tool_id, void *base, MPI_Aint size,
                                 int disp_unit, MPI_Info info, MPI_Comm comm, MPI_Win * win);

int QMPI_Win_create_dynamic(QMPI_Context context, int tool_id, MPI_Info info, MPI_Comm comm,
                            MPI_Win * win) MPICH_API_PUBLIC;
typedef int (QMPI_Win_create_dynamic_t) (QMPI_Context context, int tool_id, MPI_Info info,
                                         MPI_Comm comm, MPI_Win * win);

int QMPI_Win_create_errhandler(QMPI_Context context, int tool_id,
                               MPI_Win_errhandler_function * win_errhandler_fn,
                               MPI_Errhandler * errhandler) MPICH_API_PUBLIC;
typedef int (QMPI_Win_create_errhandler_t) (QMPI_Context context, int tool_id,
                                            MPI_Win_errhandler_function * win_errhandler_fn,
                                            MPI_Errhandler * errhandler);

int QMPI_Win_create_keyval(QMPI_Context context, int tool_id,
                           MPI_Win_copy_attr_function * win_copy_attr_fn,
                           MPI_Win_delete_attr_function * win_delete_attr_fn, int *win_keyval,
                           void *extra_state) MPICH_API_PUBLIC;
typedef int (QMPI_Win_create_keyval_t) (QMPI_Context context, int tool_id,
                                        MPI_Win_copy_attr_function * win_copy_attr_fn,
                                        MPI_Win_delete_attr_function * win_delete_attr_fn,
                                        int *win_keyval, void *extra_state);

int QMPI_Win_delete_attr(QMPI_Context context, int tool_id, MPI_Win win,
                         int win_keyval) MPICH_API_PUBLIC;
typedef int (QMPI_Win_delete_attr_t) (QMPI_Context context, int tool_id, MPI_Win win,
                                      int win_keyval);

int QMPI_Win_detach(QMPI_Context context, int tool_id, MPI_Win win,
                    const void *base) MPICH_API_PUBLIC;
typedef int (QMPI_Win_detach_t) (QMPI_Context context, int tool_id, MPI_Win win, const void *base);

int QMPI_Win_fence(QMPI_Context context, int tool_id, int assert, MPI_Win win) MPICH_API_PUBLIC;
typedef int (QMPI_Win_fence_t) (QMPI_Context context, int tool_id, int assert, MPI_Win win);

int QMPI_Win_flush(QMPI_Context context, int tool_id, int rank, MPI_Win win) MPICH_API_PUBLIC;
typedef int (QMPI_Win_flush_t) (QMPI_Context context, int tool_id, int rank, MPI_Win win);

int QMPI_Win_flush_all(QMPI_Context context, int tool_id, MPI_Win win) MPICH_API_PUBLIC;
typedef int (QMPI_Win_flush_all_t) (QMPI_Context context, int tool_id, MPI_Win win);

int QMPI_Win_flush_local(QMPI_Context context, int tool_id, int rank, MPI_Win win) MPICH_API_PUBLIC;
typedef int (QMPI_Win_flush_local_t) (QMPI_Context context, int tool_id, int rank, MPI_Win win);

int QMPI_Win_flush_local_all(QMPI_Context context, int tool_id, MPI_Win win) MPICH_API_PUBLIC;
typedef int (QMPI_Win_flush_local_all_t) (QMPI_Context context, int tool_id, MPI_Win win);

int QMPI_Win_free(QMPI_Context context, int tool_id, MPI_Win * win) MPICH_API_PUBLIC;
typedef int (QMPI_Win_free_t) (QMPI_Context context, int tool_id, MPI_Win * win);

int QMPI_Win_free_keyval(QMPI_Context context, int tool_id, int *win_keyval) MPICH_API_PUBLIC;
typedef int (QMPI_Win_free_keyval_t) (QMPI_Context context, int tool_id, int *win_keyval);

int QMPI_Win_get_attr(QMPI_Context context, int tool_id, MPI_Win win, int win_keyval,
                      void *attribute_val, int *flag) MPICH_API_PUBLIC;
typedef int (QMPI_Win_get_attr_t) (QMPI_Context context, int tool_id, MPI_Win win, int win_keyval,
                                   void *attribute_val, int *flag);

int QMPI_Win_get_errhandler(QMPI_Context context, int tool_id, MPI_Win win,
                            MPI_Errhandler * errhandler) MPICH_API_PUBLIC;
typedef int (QMPI_Win_get_errhandler_t) (QMPI_Context context, int tool_id, MPI_Win win,
                                         MPI_Errhandler * errhandler);

int QMPI_Win_get_group(QMPI_Context context, int tool_id, MPI_Win win,
                       MPI_Group * group) MPICH_API_PUBLIC;
typedef int (QMPI_Win_get_group_t) (QMPI_Context context, int tool_id, MPI_Win win,
                                    MPI_Group * group);

int QMPI_Win_get_info(QMPI_Context context, int tool_id, MPI_Win win,
                      MPI_Info * info_used) MPICH_API_PUBLIC;
typedef int (QMPI_Win_get_info_t) (QMPI_Context context, int tool_id, MPI_Win win,
                                   MPI_Info * info_used);

int QMPI_Win_get_name(QMPI_Context context, int tool_id, MPI_Win win, char *win_name,
                      int *resultlen) MPICH_API_PUBLIC;
typedef int (QMPI_Win_get_name_t) (QMPI_Context context, int tool_id, MPI_Win win, char *win_name,
                                   int *resultlen);

int QMPI_Win_lock(QMPI_Context context, int tool_id, int lock_type, int rank, int assert,
                  MPI_Win win) MPICH_API_PUBLIC;
typedef int (QMPI_Win_lock_t) (QMPI_Context context, int tool_id, int lock_type, int rank,
                               int assert, MPI_Win win);

int QMPI_Win_lock_all(QMPI_Context context, int tool_id, int assert, MPI_Win win) MPICH_API_PUBLIC;
typedef int (QMPI_Win_lock_all_t) (QMPI_Context context, int tool_id, int assert, MPI_Win win);

int QMPI_Win_post(QMPI_Context context, int tool_id, MPI_Group group, int assert,
                  MPI_Win win) MPICH_API_PUBLIC;
typedef int (QMPI_Win_post_t) (QMPI_Context context, int tool_id, MPI_Group group, int assert,
                               MPI_Win win);

int QMPI_Win_set_attr(QMPI_Context context, int tool_id, MPI_Win win, int win_keyval,
                      void *attribute_val) MPICH_API_PUBLIC;
typedef int (QMPI_Win_set_attr_t) (QMPI_Context context, int tool_id, MPI_Win win, int win_keyval,
                                   void *attribute_val);

int QMPI_Win_set_errhandler(QMPI_Context context, int tool_id, MPI_Win win,
                            MPI_Errhandler errhandler) MPICH_API_PUBLIC;
typedef int (QMPI_Win_set_errhandler_t) (QMPI_Context context, int tool_id, MPI_Win win,
                                         MPI_Errhandler errhandler);

int QMPI_Win_set_info(QMPI_Context context, int tool_id, MPI_Win win,
                      MPI_Info info) MPICH_API_PUBLIC;
typedef int (QMPI_Win_set_info_t) (QMPI_Context context, int tool_id, MPI_Win win, MPI_Info info);

int QMPI_Win_set_name(QMPI_Context context, int tool_id, MPI_Win win,
                      const char *win_name) MPICH_API_PUBLIC;
typedef int (QMPI_Win_set_name_t) (QMPI_Context context, int tool_id, MPI_Win win,
                                   const char *win_name);

int QMPI_Win_shared_query(QMPI_Context context, int tool_id, MPI_Win win, int rank, MPI_Aint * size,
                          int *disp_unit, void *baseptr) MPICH_API_PUBLIC;
typedef int (QMPI_Win_shared_query_t) (QMPI_Context context, int tool_id, MPI_Win win, int rank,
                                       MPI_Aint * size, int *disp_unit, void *baseptr);

int QMPI_Win_start(QMPI_Context context, int tool_id, MPI_Group group, int assert,
                   MPI_Win win) MPICH_API_PUBLIC;
typedef int (QMPI_Win_start_t) (QMPI_Context context, int tool_id, MPI_Group group, int assert,
                                MPI_Win win);

int QMPI_Win_sync(QMPI_Context context, int tool_id, MPI_Win win) MPICH_API_PUBLIC;
typedef int (QMPI_Win_sync_t) (QMPI_Context context, int tool_id, MPI_Win win);

int QMPI_Win_test(QMPI_Context context, int tool_id, MPI_Win win, int *flag) MPICH_API_PUBLIC;
typedef int (QMPI_Win_test_t) (QMPI_Context context, int tool_id, MPI_Win win, int *flag);

int QMPI_Win_unlock(QMPI_Context context, int tool_id, int rank, MPI_Win win) MPICH_API_PUBLIC;
typedef int (QMPI_Win_unlock_t) (QMPI_Context context, int tool_id, int rank, MPI_Win win);

int QMPI_Win_unlock_all(QMPI_Context context, int tool_id, MPI_Win win) MPICH_API_PUBLIC;
typedef int (QMPI_Win_unlock_all_t) (QMPI_Context context, int tool_id, MPI_Win win);

int QMPI_Win_wait(QMPI_Context context, int tool_id, MPI_Win win) MPICH_API_PUBLIC;
typedef int (QMPI_Win_wait_t) (QMPI_Context context, int tool_id, MPI_Win win);

double QMPI_Wtick(QMPI_Context context, int tool_id) MPICH_API_PUBLIC;
typedef double (QMPI_Wtick_t) (QMPI_Context context, int tool_id);

double QMPI_Wtime(QMPI_Context context, int tool_id) MPICH_API_PUBLIC;
typedef double (QMPI_Wtime_t) (QMPI_Context context, int tool_id);

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
                                     void (**function_ptr) (void), QMPI_Context * next_tool_context,
                                     int *next_tool_id)
{
    QMPI_Context context;
    context.storage_stack = MPIR_QMPI_storage;
    for (int i = calling_tool_id - 1; i >= 0; i--) {
        if (MPIR_QMPI_pointers[i * MPI_LAST_FUNC_T + function_enum] != NULL) {
            *function_ptr = MPIR_QMPI_pointers[i * MPI_LAST_FUNC_T + function_enum];
            *next_tool_context = context;
            *next_tool_id = i;
            return;
        }
    }
}
#endif /* QMPI_H */

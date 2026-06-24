/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIX_H_INCLUDED
#define MPIX_H_INCLUDED

/* GPU extensions */
#define MPIX_GPU_SUPPORT_CUDA  (0)
#define MPIX_GPU_SUPPORT_ZE    (1)
#define MPIX_GPU_SUPPORT_HIP   (2)

/* Generalized requests extensions */
typedef int MPIX_Grequest_class;
/* MPI stream objects */
typedef int MPIX_Stream;

/* MPIX Async extensions */
/* poll_fn return following states. */
enum {
    MPIX_ASYNC_NOPROGRESS = 0,
    MPIX_ASYNC_DONE = 1,
};

typedef struct MPIR_Async_thing *MPIX_Async_thing;

/* same as struct iovec, provided to avoid header dependency */
typedef struct MPIX_Iov {
    void *iov_base;
    MPI_Aint iov_len;
} MPIX_Iov;

/* the _x callbacks accepts scalar inputs and an additional "void *" extra_state */
typedef void (MPIX_Comm_errhandler_function_x) (MPI_Comm, int, void *);
typedef void (MPIX_File_errhandler_function_x) (MPI_File, int, void *);
typedef void (MPIX_Win_errhandler_function_x) (MPI_Win, int, void *);
typedef void (MPIX_Session_errhandler_function_x) (MPI_Session, int, void *);

/* the _x callbacks accepts scalar inputs and an additional "void *" extra_state */
typedef void (MPIX_User_function_x) (void *, void *, MPI_Count, MPI_Datatype, void *);

/* User destructor */
typedef void (MPIX_Destructor_function) (void *);

typedef int (MPIX_Grequest_poll_function) (void *, MPI_Status *);
typedef int (MPIX_Grequest_wait_function) (int, void **, double, MPI_Status *);
typedef int (MPIX_Async_poll_function) (MPIX_Async_thing);


int MPI_Address(void *location, MPI_Aint * address);
int MPI_Type_hindexed(int count, int array_of_blocklengths[], MPI_Aint array_of_displacements[],
                      MPI_Datatype oldtype, MPI_Datatype * newtype);
int MPI_Type_hvector(int count, int blocklength, MPI_Aint stride, MPI_Datatype oldtype,
                     MPI_Datatype * newtype);
int MPI_Type_struct(int count, int array_of_blocklengths[], MPI_Aint array_of_displacements[],
                    MPI_Datatype array_of_types[], MPI_Datatype * newtype);
int MPI_Type_extent(MPI_Datatype datatype, MPI_Aint * extent);
int MPI_Type_lb(MPI_Datatype datatype, MPI_Aint * displacement);
int MPI_Type_ub(MPI_Datatype datatype, MPI_Aint * displacement);
int MPI_Errhandler_create(MPI_Comm_errhandler_function * comm_errhandler_fn,
                          MPI_Errhandler * errhandler);
int MPI_Errhandler_get(MPI_Comm comm, MPI_Errhandler * errhandler);
int MPI_Errhandler_set(MPI_Comm comm, MPI_Errhandler errhandler);

int MPIX_Async_start(MPIX_Async_poll_function * poll_fn, void *extra_state, MPIX_Stream stream);
int MPIX_Async_spawn(MPIX_Async_thing async_thing, MPIX_Async_poll_function * poll_fn,
                     void *extra_state, MPIX_Stream stream);
int MPIX_Op_create_x(MPIX_User_function_x * user_fn_x, MPIX_Destructor_function * destructor_fn,
                     int commute, void *extra_state, MPI_Op * op);
int MPIX_Comm_create_errhandler_x(MPIX_Comm_errhandler_function_x * comm_errhandler_fn_x,
                                  MPIX_Destructor_function * destructor_fn, void *extra_state,
                                  MPI_Errhandler * errhandler);
int MPIX_Win_create_errhandler_x(MPIX_Win_errhandler_function_x * comm_errhandler_fn_x,
                                 MPIX_Destructor_function * destructor_fn, void *extra_state,
                                 MPI_Errhandler * errhandler);
int MPIX_File_create_errhandler_x(MPIX_File_errhandler_function_x * comm_errhandler_fn_x,
                                  MPIX_Destructor_function * destructor_fn, void *extra_state,
                                  MPI_Errhandler * errhandler);
int MPIX_Session_create_errhandler_x(MPIX_Session_errhandler_function_x * comm_errhandler_fn_x,
                                     MPIX_Destructor_function * destructor_fn, void *extra_state,
                                     MPI_Errhandler * errhandler);
int MPIX_Comm_create_keyval_x(MPI_Comm_copy_attr_function * comm_copy_attr_fn,
                              MPI_Comm_delete_attr_function * comm_delete_attr_fn,
                              MPIX_Destructor_function * destructor_fn, int *comm_keyval,
                              void *extra_state);
int MPIX_Type_create_keyval_x(MPI_Type_copy_attr_function * type_copy_attr_fn,
                              MPI_Type_delete_attr_function * type_delete_attr_fn,
                              MPIX_Destructor_function * destructor_fn, int *type_keyval,
                              void *extra_state);
int MPIX_Win_create_keyval_x(MPI_Win_copy_attr_function * win_copy_attr_fn,
                             MPI_Win_delete_attr_function * win_delete_attr_fn,
                             MPIX_Destructor_function * destructor_fn, int *win_keyval,
                             void *extra_state);
int MPIX_Comm_get_attr_as_fortran(MPI_Comm comm, int comm_keyval, void *attribute_val, int *flag);
int MPIX_Comm_set_attr_as_fortran(MPI_Comm comm, int comm_keyval, void *attribute_val);
int MPIX_Type_get_attr_as_fortran(MPI_Datatype datatype, int type_keyval, void *attribute_val,
                                  int *flag);
int MPIX_Type_set_attr_as_fortran(MPI_Datatype datatype, int type_keyval, void *attribute_val);
int MPIX_Win_get_attr_as_fortran(MPI_Win win, int win_keyval, void *attribute_val, int *flag);
int MPIX_Win_set_attr_as_fortran(MPI_Win win, int win_keyval, void *attribute_val);
int MPIX_Comm_test_threadcomm(MPI_Comm comm, int *flag);
int MPIX_Comm_revoke(MPI_Comm comm);
int MPIX_Comm_shrink(MPI_Comm comm, MPI_Comm * newcomm);
int MPIX_Comm_failure_ack(MPI_Comm comm);
int MPIX_Comm_failure_get_acked(MPI_Comm comm, MPI_Group * failedgrp);
int MPIX_Comm_agree(MPI_Comm comm, int *flag);
int MPIX_Comm_get_failed(MPI_Comm comm, MPI_Group * failedgrp);
int MPIX_Type_iov_len(MPI_Datatype datatype, MPI_Count max_iov_bytes, MPI_Count * iov_len,
                      MPI_Count * actual_iov_bytes);
int MPIX_Type_iov(MPI_Datatype datatype, MPI_Count iov_offset, MPIX_Iov * iov,
                  MPI_Count max_iov_len, MPI_Count * actual_iov_len);
int MPIX_Info_set_hex(MPI_Info info, const char *key, const void *value, int value_size);
int MPIX_GPU_query_support(int gpu_type, int *is_supported);
int MPIX_Query_cuda_support(void);
int MPIX_Query_ze_support(void);
int MPIX_Query_hip_support(void);
int MPIX_Request_is_complete(MPI_Request request);
int MPIX_Stream_create(MPI_Info info, MPIX_Stream * stream);
int MPIX_Stream_free(MPIX_Stream * stream);
int MPIX_Stream_comm_create(MPI_Comm comm, MPIX_Stream stream, MPI_Comm * newcomm);
int MPIX_Stream_comm_create_multiplex(MPI_Comm comm, int count, MPIX_Stream array_of_streams[],
                                      MPI_Comm * newcomm);
int MPIX_Comm_get_stream(MPI_Comm comm, int idx, MPIX_Stream * stream);
int MPIX_Stream_progress(MPIX_Stream stream);
int MPIX_Start_progress_thread(MPIX_Stream stream);
int MPIX_Stop_progress_thread(MPIX_Stream stream);
int MPIX_Stream_send(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
                     MPI_Comm comm, int source_stream_index, int dest_stream_index);
int MPIX_Stream_isend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
                      MPI_Comm comm, int source_stream_index, int dest_stream_index,
                      MPI_Request * request);
int MPIX_Stream_recv(void *buf, int count, MPI_Datatype datatype, int source, int tag,
                     MPI_Comm comm, int source_stream_index, int dest_stream_index,
                     MPI_Status * status);
int MPIX_Stream_irecv(void *buf, int count, MPI_Datatype datatype, int source, int tag,
                      MPI_Comm comm, int source_stream_index, int dest_stream_index,
                      MPI_Request * request);
int MPIX_Send_enqueue(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
                      MPI_Comm comm);
int MPIX_Recv_enqueue(void *buf, int count, MPI_Datatype datatype, int source, int tag,
                      MPI_Comm comm, MPI_Status * status);
int MPIX_Isend_enqueue(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
                       MPI_Comm comm, MPI_Request * request);
int MPIX_Irecv_enqueue(void *buf, int count, MPI_Datatype datatype, int source, int tag,
                       MPI_Comm comm, MPI_Request * request);
int MPIX_Wait_enqueue(MPI_Request * request, MPI_Status * status);
int MPIX_Waitall_enqueue(int count, MPI_Request array_of_requests[],
                         MPI_Status * array_of_statuses);
int MPIX_Allreduce_enqueue(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                           MPI_Op op, MPI_Comm comm);
int MPIX_Threadcomm_init(MPI_Comm comm, int num_threads, MPI_Comm * newthreadcomm);
int MPIX_Threadcomm_free(MPI_Comm * threadcomm);
int MPIX_Threadcomm_start(MPI_Comm threadcomm);
int MPIX_Threadcomm_finish(MPI_Comm threadcomm);
int MPIX_Grequest_start(MPI_Grequest_query_function * query_fn,
                        MPI_Grequest_free_function * free_fn,
                        MPI_Grequest_cancel_function * cancel_fn,
                        MPIX_Grequest_poll_function * poll_fn,
                        MPIX_Grequest_wait_function * wait_fn, void *extra_state,
                        MPI_Request * request);
int MPIX_Grequest_class_create(MPI_Grequest_query_function * query_fn,
                               MPI_Grequest_free_function * free_fn,
                               MPI_Grequest_cancel_function * cancel_fn,
                               MPIX_Grequest_poll_function * poll_fn,
                               MPIX_Grequest_wait_function * wait_fn,
                               MPIX_Grequest_class * greq_class);
int MPIX_Grequest_class_allocate(MPIX_Grequest_class greq_class, void *extra_state,
                                 MPI_Request * request);
int MPIX_Stream_send_c(const void *buf, MPI_Count count, MPI_Datatype datatype, int dest, int tag,
                       MPI_Comm comm, int source_stream_index, int dest_stream_index);
int MPIX_Stream_isend_c(const void *buf, MPI_Count count, MPI_Datatype datatype, int dest, int tag,
                        MPI_Comm comm, int source_stream_index, int dest_stream_index,
                        MPI_Request * request);
int MPIX_Stream_recv_c(void *buf, MPI_Count count, MPI_Datatype datatype, int source, int tag,
                       MPI_Comm comm, int source_stream_index, int dest_stream_index,
                       MPI_Status * status);
int MPIX_Stream_irecv_c(void *buf, MPI_Count count, MPI_Datatype datatype, int source, int tag,
                        MPI_Comm comm, int source_stream_index, int dest_stream_index,
                        MPI_Request * request);
int MPIX_Send_enqueue_c(const void *buf, MPI_Count count, MPI_Datatype datatype, int dest, int tag,
                        MPI_Comm comm);
int MPIX_Recv_enqueue_c(void *buf, MPI_Count count, MPI_Datatype datatype, int source, int tag,
                        MPI_Comm comm, MPI_Status * status);
int MPIX_Isend_enqueue_c(const void *buf, MPI_Count count, MPI_Datatype datatype, int dest, int tag,
                         MPI_Comm comm, MPI_Request * request);
int MPIX_Irecv_enqueue_c(void *buf, MPI_Count count, MPI_Datatype datatype, int source, int tag,
                         MPI_Comm comm, MPI_Request * request);
int MPIX_Allreduce_enqueue_c(const void *sendbuf, void *recvbuf, MPI_Count count,
                             MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);

int PMPI_Address(void *location, MPI_Aint * address);
int PMPI_Type_hindexed(int count, int array_of_blocklengths[], MPI_Aint array_of_displacements[],
                       MPI_Datatype oldtype, MPI_Datatype * newtype);
int PMPI_Type_hvector(int count, int blocklength, MPI_Aint stride, MPI_Datatype oldtype,
                      MPI_Datatype * newtype);
int PMPI_Type_struct(int count, int array_of_blocklengths[], MPI_Aint array_of_displacements[],
                     MPI_Datatype array_of_types[], MPI_Datatype * newtype);
int PMPI_Type_extent(MPI_Datatype datatype, MPI_Aint * extent);
int PMPI_Type_lb(MPI_Datatype datatype, MPI_Aint * displacement);
int PMPI_Type_ub(MPI_Datatype datatype, MPI_Aint * displacement);
int PMPI_Errhandler_create(MPI_Comm_errhandler_function * comm_errhandler_fn,
                           MPI_Errhandler * errhandler);
int PMPI_Errhandler_get(MPI_Comm comm, MPI_Errhandler * errhandler);
int PMPI_Errhandler_set(MPI_Comm comm, MPI_Errhandler errhandler);

int PMPIX_Async_start(MPIX_Async_poll_function * poll_fn, void *extra_state, MPIX_Stream stream);
int PMPIX_Async_spawn(MPIX_Async_thing async_thing, MPIX_Async_poll_function * poll_fn,
                      void *extra_state, MPIX_Stream stream);
int PMPIX_Op_create_x(MPIX_User_function_x * user_fn_x, MPIX_Destructor_function * destructor_fn,
                      int commute, void *extra_state, MPI_Op * op);
int PMPIX_Comm_create_errhandler_x(MPIX_Comm_errhandler_function_x * comm_errhandler_fn_x,
                                   MPIX_Destructor_function * destructor_fn, void *extra_state,
                                   MPI_Errhandler * errhandler);
int PMPIX_Win_create_errhandler_x(MPIX_Win_errhandler_function_x * comm_errhandler_fn_x,
                                  MPIX_Destructor_function * destructor_fn, void *extra_state,
                                  MPI_Errhandler * errhandler);
int PMPIX_File_create_errhandler_x(MPIX_File_errhandler_function_x * comm_errhandler_fn_x,
                                   MPIX_Destructor_function * destructor_fn, void *extra_state,
                                   MPI_Errhandler * errhandler);
int PMPIX_Session_create_errhandler_x(MPIX_Session_errhandler_function_x * comm_errhandler_fn_x,
                                      MPIX_Destructor_function * destructor_fn, void *extra_state,
                                      MPI_Errhandler * errhandler);
int PMPIX_Comm_create_keyval_x(MPI_Comm_copy_attr_function * comm_copy_attr_fn,
                               MPI_Comm_delete_attr_function * comm_delete_attr_fn,
                               MPIX_Destructor_function * destructor_fn, int *comm_keyval,
                               void *extra_state);
int PMPIX_Type_create_keyval_x(MPI_Type_copy_attr_function * type_copy_attr_fn,
                               MPI_Type_delete_attr_function * type_delete_attr_fn,
                               MPIX_Destructor_function * destructor_fn, int *type_keyval,
                               void *extra_state);
int PMPIX_Win_create_keyval_x(MPI_Win_copy_attr_function * win_copy_attr_fn,
                              MPI_Win_delete_attr_function * win_delete_attr_fn,
                              MPIX_Destructor_function * destructor_fn, int *win_keyval,
                              void *extra_state);
int PMPIX_Comm_get_attr_as_fortran(MPI_Comm comm, int comm_keyval, void *attribute_val, int *flag);
int PMPIX_Comm_set_attr_as_fortran(MPI_Comm comm, int comm_keyval, void *attribute_val);
int PMPIX_Type_get_attr_as_fortran(MPI_Datatype datatype, int type_keyval, void *attribute_val,
                                   int *flag);
int PMPIX_Type_set_attr_as_fortran(MPI_Datatype datatype, int type_keyval, void *attribute_val);
int PMPIX_Win_get_attr_as_fortran(MPI_Win win, int win_keyval, void *attribute_val, int *flag);
int PMPIX_Win_set_attr_as_fortran(MPI_Win win, int win_keyval, void *attribute_val);
int PMPIX_Comm_test_threadcomm(MPI_Comm comm, int *flag);
int PMPIX_Comm_revoke(MPI_Comm comm);
int PMPIX_Comm_shrink(MPI_Comm comm, MPI_Comm * newcomm);
int PMPIX_Comm_failure_ack(MPI_Comm comm);
int PMPIX_Comm_failure_get_acked(MPI_Comm comm, MPI_Group * failedgrp);
int PMPIX_Comm_agree(MPI_Comm comm, int *flag);
int PMPIX_Comm_get_failed(MPI_Comm comm, MPI_Group * failedgrp);
int PMPIX_Type_iov_len(MPI_Datatype datatype, MPI_Count max_iov_bytes, MPI_Count * iov_len,
                       MPI_Count * actual_iov_bytes);
int PMPIX_Type_iov(MPI_Datatype datatype, MPI_Count iov_offset, MPIX_Iov * iov,
                   MPI_Count max_iov_len, MPI_Count * actual_iov_len);
int PMPIX_Info_set_hex(MPI_Info info, const char *key, const void *value, int value_size);
int PMPIX_GPU_query_support(int gpu_type, int *is_supported);
int PMPIX_Query_cuda_support(void);
int PMPIX_Query_ze_support(void);
int PMPIX_Query_hip_support(void);
int PMPIX_Request_is_complete(MPI_Request request);
int PMPIX_Stream_create(MPI_Info info, MPIX_Stream * stream);
int PMPIX_Stream_free(MPIX_Stream * stream);
int PMPIX_Stream_comm_create(MPI_Comm comm, MPIX_Stream stream, MPI_Comm * newcomm);
int PMPIX_Stream_comm_create_multiplex(MPI_Comm comm, int count, MPIX_Stream array_of_streams[],
                                       MPI_Comm * newcomm);
int PMPIX_Comm_get_stream(MPI_Comm comm, int idx, MPIX_Stream * stream);
int PMPIX_Stream_progress(MPIX_Stream stream);
int PMPIX_Start_progress_thread(MPIX_Stream stream);
int PMPIX_Stop_progress_thread(MPIX_Stream stream);
int PMPIX_Stream_send(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
                      MPI_Comm comm, int source_stream_index, int dest_stream_index);
int PMPIX_Stream_isend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
                       MPI_Comm comm, int source_stream_index, int dest_stream_index,
                       MPI_Request * request);
int PMPIX_Stream_recv(void *buf, int count, MPI_Datatype datatype, int source, int tag,
                      MPI_Comm comm, int source_stream_index, int dest_stream_index,
                      MPI_Status * status);
int PMPIX_Stream_irecv(void *buf, int count, MPI_Datatype datatype, int source, int tag,
                       MPI_Comm comm, int source_stream_index, int dest_stream_index,
                       MPI_Request * request);
int PMPIX_Send_enqueue(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
                       MPI_Comm comm);
int PMPIX_Recv_enqueue(void *buf, int count, MPI_Datatype datatype, int source, int tag,
                       MPI_Comm comm, MPI_Status * status);
int PMPIX_Isend_enqueue(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
                        MPI_Comm comm, MPI_Request * request);
int PMPIX_Irecv_enqueue(void *buf, int count, MPI_Datatype datatype, int source, int tag,
                        MPI_Comm comm, MPI_Request * request);
int PMPIX_Wait_enqueue(MPI_Request * request, MPI_Status * status);
int PMPIX_Waitall_enqueue(int count, MPI_Request array_of_requests[],
                          MPI_Status * array_of_statuses);
int PMPIX_Allreduce_enqueue(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                            MPI_Op op, MPI_Comm comm);
int PMPIX_Threadcomm_init(MPI_Comm comm, int num_threads, MPI_Comm * newthreadcomm);
int PMPIX_Threadcomm_free(MPI_Comm * threadcomm);
int PMPIX_Threadcomm_start(MPI_Comm threadcomm);
int PMPIX_Threadcomm_finish(MPI_Comm threadcomm);
int PMPIX_Grequest_start(MPI_Grequest_query_function * query_fn,
                         MPI_Grequest_free_function * free_fn,
                         MPI_Grequest_cancel_function * cancel_fn,
                         MPIX_Grequest_poll_function * poll_fn,
                         MPIX_Grequest_wait_function * wait_fn, void *extra_state,
                         MPI_Request * request);
int PMPIX_Grequest_class_create(MPI_Grequest_query_function * query_fn,
                                MPI_Grequest_free_function * free_fn,
                                MPI_Grequest_cancel_function * cancel_fn,
                                MPIX_Grequest_poll_function * poll_fn,
                                MPIX_Grequest_wait_function * wait_fn,
                                MPIX_Grequest_class * greq_class);
int PMPIX_Grequest_class_allocate(MPIX_Grequest_class greq_class, void *extra_state,
                                  MPI_Request * request);
int PMPIX_Stream_send_c(const void *buf, MPI_Count count, MPI_Datatype datatype, int dest,
                        int tag, MPI_Comm comm, int source_stream_index, int dest_stream_index);
int PMPIX_Stream_isend_c(const void *buf, MPI_Count count, MPI_Datatype datatype, int dest, int tag,
                         MPI_Comm comm, int source_stream_index, int dest_stream_index,
                         MPI_Request * request);
int PMPIX_Stream_recv_c(void *buf, MPI_Count count, MPI_Datatype datatype, int source, int tag,
                        MPI_Comm comm, int source_stream_index, int dest_stream_index,
                        MPI_Status * status);
int PMPIX_Stream_irecv_c(void *buf, MPI_Count count, MPI_Datatype datatype, int source, int tag,
                         MPI_Comm comm, int source_stream_index, int dest_stream_index,
                         MPI_Request * request);
int PMPIX_Send_enqueue_c(const void *buf, MPI_Count count, MPI_Datatype datatype, int dest, int tag,
                         MPI_Comm comm);
int PMPIX_Recv_enqueue_c(void *buf, MPI_Count count, MPI_Datatype datatype, int source, int tag,
                         MPI_Comm comm, MPI_Status * status);
int PMPIX_Isend_enqueue_c(const void *buf, MPI_Count count, MPI_Datatype datatype, int dest,
                          int tag, MPI_Comm comm, MPI_Request * request);
int PMPIX_Irecv_enqueue_c(void *buf, MPI_Count count, MPI_Datatype datatype, int source, int tag,
                          MPI_Comm comm, MPI_Request * request);
int PMPIX_Allreduce_enqueue_c(const void *sendbuf, void *recvbuf, MPI_Count count,
                              MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);

#endif /* MPIX_H_INCLUDED */

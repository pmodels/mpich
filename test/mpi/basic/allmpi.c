/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
/* necessary to get the conditional definitions:
   HAVE_FORTRAN_BINDING
   MPI_MPI_IO
*/
#include "mpitestconf.h"
#include <stdio.h>
#include <string.h>

void handler( MPI_Comm *comm_ptr, int *int_ptr, ... ) {}
int comm_copy_attr_fn(MPI_Comm comm, int i, void * buf1, void *buf2, void *buf3, int *int_ptr) {return 0;}
int comm_delete_attr_fn(MPI_Comm comm, int i, void *buf1, void *buf2) {return 0;}
int tcopy_attr_fn(MPI_Datatype dtype, int i, void *buf1, void *buf2, void *buf3, int *int_ptr) {return 0;}
int tdelete_attr_fn(MPI_Datatype dtype, int i, void *buf1, void *buf2) {return 0;}
int win_copy_attr_fn(MPI_Win win, int i, void *buf1, void *buf2, void *buf3, int *int_ptr) {return 0;}
int win_delete_attr_fn(MPI_Win win, int i, void *buf1, void *buf2) {return 0;}
void comm_errhan(MPI_Comm *comm_ptr, int *int_ptr, ...) {}
#ifdef HAVE_MPI_IO
void file_errhan(MPI_File *file_ptr, int *int_ptr, ...) {}
#endif
void win_errhan(MPI_Win *win_ptr, int *int_ptr, ...) {}
void user_fn( void *buf1, void *buf2, int *int_ptr, MPI_Datatype *dtype_ptr) {}
int copy_fn(MPI_Comm comm, int i, void *buf1, void *buf2, void *buf3, int *int_ptr) {return 0;}
int delete_fn(MPI_Comm comm, int i, void *buf1, void *buf2) {return 0;}
int cancel_fn(void *buf, int i) {return 0;}
int free_fn(void *buf) {return 0;}
int query_fn(void *buf, MPI_Status *status_ptr) {return 0;}
int conversion_fn(void *buf1, MPI_Datatype dtype, int i, void *buf2, MPI_Offset offset, void *buf3) {return 0;}
int extent_fn(MPI_Datatype dtype, MPI_Aint *aint_ptr, void *buf) {return 0;}

void testAll(void)
{
    char *cbuf = NULL;
    int *ibuf = NULL;
    void *vbuf = NULL;
    int i = 0;
    int rank = 0;
    int tag = 0;
    MPI_Datatype dtype = MPI_BYTE;
    MPI_Comm comm = MPI_COMM_WORLD;
    MPI_Request request = MPI_REQUEST_NULL;
    MPI_Status status;
    MPI_Group group = MPI_GROUP_NULL;
    MPI_Win win = MPI_WIN_NULL;
    MPI_User_function user_fn;
    MPI_Op op = MPI_SUM;
    MPI_Aint aint = 0;
    MPI_Fint fint = 0;
    MPI_Copy_function copy_fn;
    MPI_Delete_function delete_fn;
    MPI_Errhandler errhan;
    int a3[1][3];
    int argc = 0;
    char **argv = NULL;
    char ***argvp = NULL;
    MPI_Info info = MPI_INFO_NULL;
    char *cmd = NULL;
    char *type_name = NULL;
#ifdef HAVE_MPI_IO
    MPI_File file = MPI_FILE_NULL;
    char *filename = NULL;
    int amode = 0;
    MPI_Offset size = 0, offset = 0;
    MPI_Request iorequest;
#endif

    MPI_Send(vbuf, i, dtype, i, i, comm);
    MPI_Recv(vbuf, i, dtype, rank, tag, comm, &status);
    MPI_Get_count(&status, dtype, &i);
    MPI_Bsend(vbuf, i, dtype, rank, tag, comm);
    MPI_Ssend(vbuf, i, dtype, rank, tag, comm);
    MPI_Rsend(vbuf, i, dtype, rank, tag, comm);
    MPI_Buffer_attach( vbuf, i);
    MPI_Buffer_detach( vbuf, &i);
    MPI_Isend(vbuf, i, dtype, rank, tag, comm, &request);
    MPI_Ibsend(vbuf, i, dtype, rank, tag, comm, &request);
    MPI_Issend(vbuf, i, dtype, rank, tag, comm, &request);
    MPI_Irsend(vbuf, i, dtype, rank, tag, comm, &request);
    MPI_Irecv(vbuf, i, dtype, rank, tag, comm, &request);
    MPI_Wait(&request, &status);
    MPI_Test(&request, &i, &status);
    MPI_Request_free(&request);
    MPI_Waitany(i, &request, &i, &status);
    MPI_Testany(i, &request, &i, &i, &status);
    MPI_Waitall(i, &request, &status);
    MPI_Testall(i, &request, &i, &status);
    MPI_Waitsome(i, &request, &i, &i, &status);
    MPI_Testsome(i, &request, &i, &i, &status);
    MPI_Iprobe(i, i, comm, &i, &status);
    MPI_Probe(i, i, comm, &status);
    MPI_Cancel(&request);
    MPI_Test_cancelled(&status, &i);
    MPI_Send_init(vbuf, i, dtype, rank, tag, comm, &request);
    MPI_Bsend_init(vbuf, i, dtype, i,i, comm, &request);
    MPI_Ssend_init(vbuf, i, dtype, i,i, comm, &request);
    MPI_Rsend_init(vbuf, i, dtype, i,i, comm, &request);
    MPI_Recv_init(vbuf, i, dtype, i,i, comm, &request);
    MPI_Start(&request);
    MPI_Startall(i, &request);
    MPI_Sendrecv(vbuf, i, dtype,rank, tag, vbuf, i, dtype, rank, tag, comm, &status);
    MPI_Sendrecv_replace(vbuf, i, dtype, rank, tag, rank, tag, comm, &status);
    MPI_Type_contiguous(i, dtype, &dtype);
    MPI_Type_vector(i, i, i, dtype, &dtype);
    MPI_Type_hvector(i, i, aint, dtype, &dtype);
    MPI_Type_indexed(i, &i, &i, dtype, &dtype);
    MPI_Type_hindexed(i, &i, &aint, dtype, &dtype);
    MPI_Type_struct(i, &i, &aint, &dtype, &dtype);
    MPI_Address(vbuf, &aint);
    MPI_Type_extent(dtype, &aint);
    MPI_Type_size(dtype, &i);
    MPI_Type_lb(dtype, &aint);
    MPI_Type_ub(dtype, &aint);
    MPI_Type_commit(&dtype);
    MPI_Type_free(&dtype);
    MPI_Get_elements(&status, dtype, &i);
    MPI_Pack(vbuf, i, dtype, vbuf, i, &i,  comm);
    MPI_Unpack(vbuf, i, &i, vbuf, i, dtype, comm);
    MPI_Pack_size(i, dtype, comm, &i);
    MPI_Barrier(comm );
    MPI_Bcast(vbuf, i, dtype, i, comm );
    MPI_Gather(vbuf , i, dtype, vbuf, i, dtype, i, comm); 
    MPI_Gatherv(vbuf , i, dtype, vbuf, &i, &i, dtype, i, comm); 
    MPI_Scatter(vbuf , i, dtype, vbuf, i, dtype, i, comm);
    MPI_Scatterv(vbuf , &i, &i,  dtype, vbuf, i, dtype, i, comm);
    MPI_Allgather(vbuf , i, dtype, vbuf, i, dtype, comm);
    MPI_Allgatherv(vbuf , i, dtype, vbuf, &i, &i, dtype, comm);
    MPI_Alltoall(vbuf , i, dtype, vbuf, i, dtype, comm);
    MPI_Alltoallv(vbuf , &i, &i, dtype, vbuf, &i, &i, dtype, comm);
    MPI_Reduce(vbuf , vbuf, i, dtype, op, i, comm);
    MPI_Op_create(&user_fn, i, &op);
    MPI_Op_free( &op);
    MPI_Allreduce(vbuf , vbuf, i, dtype, op, comm);
    MPI_Reduce_scatter(vbuf , vbuf, &i, dtype, op, comm);
    MPI_Scan(vbuf , vbuf, i, dtype, op, comm );
    MPI_Group_size(group, &i);
    MPI_Group_rank(group, &i);
    MPI_Group_translate_ranks (group, i, &i, group, &i);
    MPI_Group_compare(group, group, &i);
    MPI_Comm_group(comm, &group);
    MPI_Group_union(group, group, &group);
    MPI_Group_intersection(group, group, &group);
    MPI_Group_difference(group, group, &group);
    MPI_Group_incl(group, i, &i, &group);
    MPI_Group_excl(group, i, &i, &group);
    MPI_Group_range_incl(group, i, a3, &group);
    MPI_Group_range_excl(group, i, a3, &group);
    MPI_Group_free(&group);
    MPI_Comm_size(comm, &i);
    MPI_Comm_rank(comm, &i);
    MPI_Comm_compare(comm, comm, &i);
    MPI_Comm_dup(comm, &comm);
    MPI_Comm_create(comm, group, &comm);
    MPI_Comm_split(comm, i, i, &comm);
    MPI_Comm_free(&comm);
    MPI_Comm_test_inter(comm, &i);
    MPI_Comm_remote_size(comm, &i);
    MPI_Comm_remote_group(comm, &group);
    MPI_Intercomm_create(comm, i, comm, i, i, &comm );
    MPI_Intercomm_merge(comm, i, &comm);
    MPI_Keyval_create(&copy_fn, &delete_fn, &i, vbuf);
    MPI_Keyval_free(&i);
    MPI_Attr_put(comm, i, vbuf);
    MPI_Attr_get(comm, i, vbuf, &i);
    MPI_Attr_delete(comm, i);
    MPI_Topo_test(comm, &i);
    MPI_Cart_create(comm, i, &i, &i, i, &comm);
    MPI_Dims_create(i, i, &i);
    MPI_Graph_create(comm, i, &i, &i, i, &comm);
    MPI_Graphdims_get(comm, &i, &i);
    MPI_Graph_get(comm, i, i, &i, &i);
    MPI_Cartdim_get(comm, &i);
    MPI_Cart_get(comm, i, &i, &i, &i);
    MPI_Cart_rank(comm, &i, &i);
    MPI_Cart_coords(comm, i, i, &i);
    MPI_Graph_neighbors_count(comm, i, &i);
    MPI_Graph_neighbors(comm, i, i, &i);
    MPI_Cart_shift(comm, i, i, &i, &i);
    MPI_Cart_sub(comm, &i, &comm);
    MPI_Cart_map(comm, i, &i, &i, &i);
    MPI_Graph_map(comm, i, &i, &i, &i);
    MPI_Get_processor_name(cbuf, &i);
    MPI_Get_version(&i, &i);
    MPI_Errhandler_create(&handler, &errhan);
    MPI_Errhandler_set(comm, errhan);
    MPI_Errhandler_get(comm, &errhan);
    MPI_Errhandler_free(&errhan);
    MPI_Error_string(i, cbuf, &i);
    MPI_Error_class(i, &i);
    MPI_Wtime();
    MPI_Wtick();
    MPI_Init(&argc, &argv);
    MPI_Finalize();
    MPI_Initialized(&i);
    MPI_Abort(comm, i);
    MPI_Pcontrol(0);
    MPI_DUP_FN( comm, i, vbuf, vbuf, vbuf, &i );
    MPI_Close_port(cbuf);
    MPI_Comm_accept(cbuf, info, i, comm, &comm);
    MPI_Comm_connect(cbuf, info, i, comm, &comm);
    MPI_Comm_disconnect(&comm);
    MPI_Comm_get_parent(&comm);
    MPI_Comm_join(i, &comm);
    MPI_Comm_spawn(cbuf, &cmd, i, info, i, comm, &comm, &i);
    MPI_Comm_spawn_multiple(i, &cmd, argvp, &i, &info, i, comm, &comm, &i); 
    MPI_Lookup_name(cbuf, info, cbuf);
    MPI_Open_port(info, cbuf);
    MPI_Publish_name(cbuf, info, cbuf);
    MPI_Unpublish_name(cbuf, info, cbuf);
    MPI_Accumulate(vbuf, i, dtype, i, aint, i, dtype,  op, win);
    MPI_Get(vbuf, i, dtype, i, aint, i, dtype, win);
    MPI_Put(vbuf, i, dtype, i, aint, i, dtype, win);
    MPI_Win_complete(win);
    MPI_Win_create(vbuf, aint, i, info, comm, &win);
    MPI_Win_fence(i, win);
    MPI_Win_free(&win);
    MPI_Win_get_group(win, &group);
    MPI_Win_lock(i, i, i, win);
    MPI_Win_post(group, i, win);
    MPI_Win_start(group, i, win);
    MPI_Win_test(win, &i);
    MPI_Win_unlock(i, win);
    MPI_Win_wait(win);
    MPI_Alltoallw(vbuf, &i, &i, &dtype, vbuf, &i, &i, &dtype, comm);
    MPI_Exscan(vbuf, vbuf, i, dtype, op, comm) ;
    MPI_Add_error_class(&i);
    MPI_Add_error_code(i, &i);
    MPI_Add_error_string(i, cbuf);
    MPI_Comm_call_errhandler(comm, i);
    MPI_Comm_create_keyval(&comm_copy_attr_fn, &comm_delete_attr_fn, &i, vbuf);
    MPI_Comm_delete_attr(comm, i);
    MPI_Comm_free_keyval(&i);
    MPI_Comm_get_attr(comm, i, vbuf, &i);
    MPI_Comm_get_name(comm, cbuf, &i);
    MPI_Comm_set_attr(comm, i, vbuf);
    MPI_Comm_set_name(comm, cbuf);
#ifdef HAVE_MPI_IO
    MPI_File_call_errhandler(file, i);
#endif
    MPI_Grequest_complete(request);
    MPI_Grequest_start(&query_fn, &free_fn, &cancel_fn, vbuf, &request);
    MPI_Init_thread(&argc, &argv, i, &i);
    MPI_Is_thread_main(&i);
    MPI_Query_thread(&i);
    MPI_Status_set_cancelled(&status, i);
    MPI_Status_set_elements(&status, dtype, i);
    MPI_Type_create_keyval(&tcopy_attr_fn, &tdelete_attr_fn, &i, vbuf);
    MPI_Type_delete_attr(dtype, i);
    MPI_Type_dup(dtype, &dtype);
    MPI_Type_free_keyval(&i);
    MPI_Type_get_attr(dtype, i, vbuf, &i);
    MPI_Type_get_contents(dtype, i, i, i, &i, &aint, &dtype);
    MPI_Type_get_envelope(dtype, &i, &i, &i, &i);
    MPI_Type_get_name(dtype, cbuf, &i);
    MPI_Type_set_attr(dtype, i, vbuf);
    MPI_Type_set_name(dtype, type_name);
    MPI_Win_call_errhandler(win, i);
    MPI_Win_create_keyval(&win_copy_attr_fn, win_delete_attr_fn, &i, vbuf);
    MPI_Win_delete_attr(win, i);
    MPI_Win_free_keyval(&i);
    MPI_Win_get_attr(win, i, vbuf, &i);
    MPI_Win_get_name(win, cbuf, &i);
    MPI_Win_set_attr(win, i, vbuf);
    MPI_Win_set_name(win, cbuf);
    MPI_Alloc_mem(aint, info, vbuf);
    MPI_Comm_create_errhandler(&comm_errhan, &errhan);
    MPI_Comm_get_errhandler(comm, &errhan);
    MPI_Comm_set_errhandler(comm, errhan);
#ifdef HAVE_MPI_IO
    MPI_File_create_errhandler(&file_errhan, &errhan);
    MPI_File_get_errhandler(file, &errhan);
    MPI_File_set_errhandler(file, errhan);
#endif
    MPI_Finalized(&i);
    MPI_Free_mem(vbuf);
    MPI_Get_address(vbuf, &aint);
    MPI_Info_create(&info);
    MPI_Info_delete(info, cbuf);
    MPI_Info_dup(info, &info);
    MPI_Info_free(&info);
    MPI_Info_get(info, cbuf, i, cbuf, &i);
    MPI_Info_get_nkeys(info, &i);
    MPI_Info_get_nthkey(info, i, cbuf);
    MPI_Info_get_valuelen(info, cbuf, &i, &i);
    MPI_Info_set(info, cbuf, cbuf);
    MPI_Pack_external(cbuf, vbuf, i, dtype, vbuf, aint, &aint); 
    MPI_Pack_external_size(cbuf, i, dtype, &aint); 
    MPI_Request_get_status(request, &i, &status);
#ifdef HAVE_FORTRAN_BINDING
    MPI_Status_c2f(&status, &fint);
    MPI_Status_f2c(&fint, &status);
#endif
    MPI_Type_create_darray(i, i, i, &i, &i, &i, &i, i, dtype, &dtype);
    MPI_Type_create_subarray(i, &i, &i, &i, i, dtype, &dtype);
    MPI_Type_create_hindexed(i, &i, &aint, dtype, &dtype);
    MPI_Type_create_hvector(i, i, aint, dtype, &dtype);
    MPI_Type_create_indexed_block(i, i, &i, dtype, &dtype);
    MPI_Type_create_resized(dtype, aint, aint, &dtype);
    MPI_Type_create_struct(i, &i, &aint, &dtype, &dtype);
    MPI_Type_get_extent(dtype, &aint, &aint);
    MPI_Type_get_true_extent(dtype, &aint, &aint);
    MPI_Unpack_external(cbuf, vbuf, aint, &aint, vbuf, i, dtype); 
    MPI_Win_create_errhandler(&win_errhan, &errhan);
    MPI_Win_get_errhandler(win, &errhan);
    MPI_Win_set_errhandler(win, errhan);
#ifdef HAVE_MPI_IO
    MPI_File_open(comm, filename, amode, info, &file);
    MPI_File_close(&file);
    MPI_File_delete(filename, info);
    MPI_File_set_size(file, size);
    MPI_File_preallocate(file, size);
    MPI_File_get_size(file, &size);
    MPI_File_get_group(file, &group);
    MPI_File_get_amode(file, &amode);
    MPI_File_set_info(file, info);
    MPI_File_get_info(file, &info);
    MPI_File_set_view(file, offset, dtype, dtype, cbuf, info);
    MPI_File_get_view(file, &offset, &dtype, &dtype, cbuf);
    MPI_File_read_at(file, offset, vbuf, i, dtype, &status);
    MPI_File_read_at_all(file, offset, vbuf, i, dtype, &status);
    MPI_File_write_at(file, offset, vbuf, i, dtype, &status);
    MPI_File_write_at_all(file, offset, vbuf, i, dtype, &status);
    MPI_File_iread_at(file, offset, vbuf, i, dtype, &iorequest);
    MPI_File_iwrite_at(file, offset, vbuf, i, dtype, &iorequest);
    MPI_File_read(file, vbuf, i, dtype, &status); 
    MPI_File_read_all(file, vbuf, i, dtype, &status);
    MPI_File_write(file, vbuf, i, dtype, &status);
    MPI_File_write_all(file, vbuf, i, dtype, &status);
    MPI_File_iread(file, vbuf, i, dtype, &iorequest);
    MPI_File_iwrite(file, vbuf, i, dtype, &iorequest);
    MPI_File_seek(file, offset, i);
    MPI_File_get_position(file, &offset);
    MPI_File_get_byte_offset(file, offset, &offset);
    MPI_File_read_shared(file, vbuf, i, dtype, &status);
    MPI_File_write_shared(file, vbuf, i, dtype, &status);
    MPI_File_iread_shared(file, vbuf, i, dtype, &iorequest);
    MPI_File_iwrite_shared(file, vbuf, i, dtype, &iorequest);
    MPI_File_read_ordered(file, vbuf, i, dtype, &status);
    MPI_File_write_ordered(file, vbuf, i, dtype, &status);
    MPI_File_seek_shared(file, offset, i);
    MPI_File_get_position_shared(file, &offset);
    MPI_File_read_at_all_begin(file, offset, vbuf, i, dtype);
    MPI_File_read_at_all_end(file, vbuf, &status);
    MPI_File_write_at_all_begin(file, offset, vbuf, i, dtype);
    MPI_File_write_at_all_end(file, vbuf, &status);
    MPI_File_read_all_begin(file, vbuf, i, dtype);
    MPI_File_read_all_end(file, vbuf, &status);
    MPI_File_write_all_begin(file, vbuf, i, dtype);
    MPI_File_write_all_end(file, vbuf, &status);
    MPI_File_read_ordered_begin(file, vbuf, i, dtype);
    MPI_File_read_ordered_end(file, vbuf, &status);
    MPI_File_write_ordered_begin(file, vbuf, i, dtype);
    MPI_File_write_ordered_end(file, vbuf, &status);
    MPI_File_get_type_extent(file, dtype, &aint);
    MPI_File_set_atomicity(file, i);
    MPI_File_get_atomicity(file, &i);
    MPI_File_sync(file);
    MPI_File_set_errhandler(file, errhan);
    MPI_File_get_errhandler(file, &errhan);
    MPI_Type_create_subarray(i, &i, &i, &i, i, dtype, &dtype);
    MPI_Type_create_darray(i, i, i, &i, &i, &i, &i, i, dtype, &dtype);
    MPI_File_f2c(fint);
    MPI_File_c2f(file);
#endif
}

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);

    /* make it possible to call testAll so the compiler doesn't optimize out the code? */
    if (argc > 1 && strcmp(argv[1], "flooglebottom") == 0)
	testAll();

    MPI_Finalize();
    return 0;
}

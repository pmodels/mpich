/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPI_PROTO_H_INCLUDED
#define MPI_PROTO_H_INCLUDED

/* *INDENT-OFF* */

/* Begin Prototypes */
int MPI_Send(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
             MPI_Comm comm) MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3) MPICH_API_PUBLIC;
int MPI_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag,
             MPI_Comm comm, MPI_Status *status) MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3) MPICH_API_PUBLIC;
int MPI_Get_count(const MPI_Status *status, MPI_Datatype datatype, int *count) MPICH_API_PUBLIC;
int MPI_Bsend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
              MPI_Comm comm) MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3) MPICH_API_PUBLIC;
int MPI_Ssend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
              MPI_Comm comm) MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3) MPICH_API_PUBLIC;
int MPI_Rsend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
              MPI_Comm comm) MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3) MPICH_API_PUBLIC;
int MPI_Buffer_attach(void *buffer, int size) MPICH_API_PUBLIC;
int MPI_Buffer_detach(void *buffer_addr, int *size) MPICH_API_PUBLIC;
int MPI_Isend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
              MPI_Comm comm, MPI_Request *request) MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3) MPICH_API_PUBLIC;
int MPI_Ibsend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
               MPI_Comm comm, MPI_Request *request) MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3) MPICH_API_PUBLIC;
int MPI_Issend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
               MPI_Comm comm, MPI_Request *request) MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3) MPICH_API_PUBLIC;
int MPI_Irsend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
               MPI_Comm comm, MPI_Request *request) MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3) MPICH_API_PUBLIC;
int MPI_Irecv(void *buf, int count, MPI_Datatype datatype, int source, int tag,
              MPI_Comm comm, MPI_Request *request) MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3) MPICH_API_PUBLIC;
int MPI_Wait(MPI_Request *request, MPI_Status *status) MPICH_API_PUBLIC;
int MPI_Test(MPI_Request *request, int *flag, MPI_Status *status) MPICH_API_PUBLIC;
int MPI_Request_free(MPI_Request *request) MPICH_API_PUBLIC;
int MPI_Waitany(int count, MPI_Request array_of_requests[], int *indx, MPI_Status *status) MPICH_API_PUBLIC;
int MPI_Testany(int count, MPI_Request array_of_requests[], int *indx, int *flag,
                MPI_Status *status) MPICH_API_PUBLIC;
int MPI_Waitall(int count, MPI_Request array_of_requests[], MPI_Status array_of_statuses[]) MPICH_API_PUBLIC;
int MPI_Testall(int count, MPI_Request array_of_requests[], int *flag,
                MPI_Status array_of_statuses[]) MPICH_API_PUBLIC;
int MPI_Waitsome(int incount, MPI_Request array_of_requests[], int *outcount,
                 int array_of_indices[], MPI_Status array_of_statuses[]) MPICH_API_PUBLIC;
int MPI_Testsome(int incount, MPI_Request array_of_requests[], int *outcount,
                 int array_of_indices[], MPI_Status array_of_statuses[]) MPICH_API_PUBLIC;
int MPI_Iprobe(int source, int tag, MPI_Comm comm, int *flag, MPI_Status *status) MPICH_API_PUBLIC;
int MPI_Probe(int source, int tag, MPI_Comm comm, MPI_Status *status) MPICH_API_PUBLIC;
int MPI_Cancel(MPI_Request *request) MPICH_API_PUBLIC;
int MPI_Test_cancelled(const MPI_Status *status, int *flag) MPICH_API_PUBLIC;
int MPI_Send_init(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
                  MPI_Comm comm, MPI_Request *request) MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3) MPICH_API_PUBLIC;
int MPI_Bsend_init(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
                   MPI_Comm comm, MPI_Request *request) MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3) MPICH_API_PUBLIC;
int MPI_Ssend_init(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
                   MPI_Comm comm, MPI_Request *request) MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3) MPICH_API_PUBLIC;
int MPI_Rsend_init(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
                   MPI_Comm comm, MPI_Request *request) MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3) MPICH_API_PUBLIC;
int MPI_Recv_init(void *buf, int count, MPI_Datatype datatype, int source, int tag,
                  MPI_Comm comm, MPI_Request *request) MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3) MPICH_API_PUBLIC;
int MPI_Start(MPI_Request *request) MPICH_API_PUBLIC;
int MPI_Startall(int count, MPI_Request array_of_requests[]) MPICH_API_PUBLIC;
int MPI_Sendrecv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, int dest,
                 int sendtag, void *recvbuf, int recvcount, MPI_Datatype recvtype,
                 int source, int recvtag, MPI_Comm comm, MPI_Status *status)
                 MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3) MPICH_ATTR_POINTER_WITH_TYPE_TAG(6,8) MPICH_API_PUBLIC;
int MPI_Sendrecv_replace(void *buf, int count, MPI_Datatype datatype, int dest,
                         int sendtag, int source, int recvtag, MPI_Comm comm,
                         MPI_Status *status) MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3) MPICH_API_PUBLIC;
int MPI_Isendrecv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, int dest,
                  int sendtag, void *recvbuf, int recvcount, MPI_Datatype recvtype,
                  int source, int recvtag, MPI_Comm comm, MPI_Request *request)
                  MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3) MPICH_ATTR_POINTER_WITH_TYPE_TAG(6,8) MPICH_API_PUBLIC;
int MPI_Isendrecv_replace(void *buf, int count, MPI_Datatype datatype, int dest,
                          int sendtag, int source, int recvtag, MPI_Comm comm,
                          MPI_Request *request) MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3) MPICH_API_PUBLIC;
int MPI_Type_contiguous(int count, MPI_Datatype oldtype, MPI_Datatype *newtype) MPICH_API_PUBLIC;
int MPI_Type_vector(int count, int blocklength, int stride, MPI_Datatype oldtype,
                    MPI_Datatype *newtype) MPICH_API_PUBLIC;
int MPI_Type_hvector(int count, int blocklength, MPI_Aint stride, MPI_Datatype oldtype,
                     MPI_Datatype *newtype) MPICH_API_PUBLIC;
int MPI_Type_indexed(int count, const int *array_of_blocklengths,
                     const int *array_of_displacements, MPI_Datatype oldtype,
                     MPI_Datatype *newtype) MPICH_API_PUBLIC;
int MPI_Type_hindexed(int count, int *array_of_blocklengths,
                      MPI_Aint *array_of_displacements, MPI_Datatype oldtype,
                      MPI_Datatype *newtype) MPICH_API_PUBLIC;
int MPI_Type_struct(int count, int *array_of_blocklengths,
                    MPI_Aint *array_of_displacements,
                    MPI_Datatype *array_of_types, MPI_Datatype *newtype) MPICH_API_PUBLIC;
int MPI_Address(void *location, MPI_Aint *address) MPICH_API_PUBLIC;
int MPI_Type_extent(MPI_Datatype datatype, MPI_Aint *extent) MPICH_API_PUBLIC;
int MPI_Type_size(MPI_Datatype datatype, int *size) MPICH_API_PUBLIC;
int MPI_Type_lb(MPI_Datatype datatype, MPI_Aint *displacement) MPICH_API_PUBLIC;
int MPI_Type_ub(MPI_Datatype datatype, MPI_Aint *displacement) MPICH_API_PUBLIC;
int MPI_Type_commit(MPI_Datatype *datatype) MPICH_API_PUBLIC;
int MPI_Type_free(MPI_Datatype *datatype) MPICH_API_PUBLIC;
int MPI_Get_elements(const MPI_Status *status, MPI_Datatype datatype, int *count) MPICH_API_PUBLIC;
int MPI_Pack(const void *inbuf, int incount, MPI_Datatype datatype, void *outbuf,
             int outsize, int *position, MPI_Comm comm) MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3) MPICH_API_PUBLIC;
int MPI_Unpack(const void *inbuf, int insize, int *position, void *outbuf, int outcount,
               MPI_Datatype datatype, MPI_Comm comm) MPICH_ATTR_POINTER_WITH_TYPE_TAG(4,6) MPICH_API_PUBLIC;
int MPI_Pack_size(int incount, MPI_Datatype datatype, MPI_Comm comm, int *size) MPICH_API_PUBLIC;
int MPI_Barrier(MPI_Comm comm) MPICH_API_PUBLIC;
int MPI_Bcast(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm)
              MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3) MPICH_API_PUBLIC;
int MPI_Gather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
               int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm)
               MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3) MPICH_ATTR_POINTER_WITH_TYPE_TAG(4,6) MPICH_API_PUBLIC;
int MPI_Gatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                const int *recvcounts, const int *displs, MPI_Datatype recvtype, int root,
                MPI_Comm comm)
                MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3) MPICH_ATTR_POINTER_WITH_TYPE_TAG(4,7) MPICH_API_PUBLIC;
int MPI_Scatter(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm)
                MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3) MPICH_ATTR_POINTER_WITH_TYPE_TAG(4,6) MPICH_API_PUBLIC;
int MPI_Scatterv(const void *sendbuf, const int *sendcounts, const int *displs,
                 MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype,
                 int root, MPI_Comm comm)
                 MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,4) MPICH_ATTR_POINTER_WITH_TYPE_TAG(5,7) MPICH_API_PUBLIC;
int MPI_Allgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                  int recvcount, MPI_Datatype recvtype, MPI_Comm comm)
                  MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3) MPICH_ATTR_POINTER_WITH_TYPE_TAG(4,6) MPICH_API_PUBLIC;
int MPI_Allgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                   const int *recvcounts, const int *displs, MPI_Datatype recvtype, MPI_Comm comm)
                   MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3) MPICH_ATTR_POINTER_WITH_TYPE_TAG(4,7) MPICH_API_PUBLIC;
int MPI_Alltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                 int recvcount, MPI_Datatype recvtype, MPI_Comm comm)
                 MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3) MPICH_ATTR_POINTER_WITH_TYPE_TAG(4,6) MPICH_API_PUBLIC;
int MPI_Alltoallv(const void *sendbuf, const int *sendcounts, const int *sdispls,
                  MPI_Datatype sendtype, void *recvbuf, const int *recvcounts,
                  const int *rdispls, MPI_Datatype recvtype, MPI_Comm comm)
                  MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,4) MPICH_ATTR_POINTER_WITH_TYPE_TAG(5,8) MPICH_API_PUBLIC;
int MPI_Alltoallw(const void *sendbuf, const int sendcounts[], const int sdispls[],
                  const MPI_Datatype sendtypes[], void *recvbuf, const int recvcounts[],
                  const int rdispls[], const MPI_Datatype recvtypes[], MPI_Comm comm) MPICH_API_PUBLIC;
int MPI_Exscan(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
               MPI_Op op, MPI_Comm comm)
               MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,4) MPICH_ATTR_POINTER_WITH_TYPE_TAG(2,4) MPICH_API_PUBLIC;
int MPI_Reduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
               MPI_Op op, int root, MPI_Comm comm)
               MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,4) MPICH_ATTR_POINTER_WITH_TYPE_TAG(2,4) MPICH_API_PUBLIC;
int MPI_Op_create(MPI_User_function *user_fn, int commute, MPI_Op *op) MPICH_API_PUBLIC;
int MPI_Op_free(MPI_Op *op) MPICH_API_PUBLIC;
int MPI_Allreduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                  MPI_Op op, MPI_Comm comm)
                  MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,4) MPICH_ATTR_POINTER_WITH_TYPE_TAG(2,4) MPICH_API_PUBLIC;
int MPI_Reduce_scatter(const void *sendbuf, void *recvbuf, const int recvcounts[],
                       MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
                       MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,4) MPICH_ATTR_POINTER_WITH_TYPE_TAG(2,4) MPICH_API_PUBLIC;
int MPI_Scan(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op,
             MPI_Comm comm)
             MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,4) MPICH_ATTR_POINTER_WITH_TYPE_TAG(2,4) MPICH_API_PUBLIC;
int MPI_Group_size(MPI_Group group, int *size) MPICH_API_PUBLIC;
int MPI_Group_rank(MPI_Group group, int *rank) MPICH_API_PUBLIC;
int MPI_Group_translate_ranks(MPI_Group group1, int n, const int ranks1[], MPI_Group group2,
                              int ranks2[]) MPICH_API_PUBLIC;
int MPI_Group_compare(MPI_Group group1, MPI_Group group2, int *result) MPICH_API_PUBLIC;
int MPI_Comm_group(MPI_Comm comm, MPI_Group *group) MPICH_API_PUBLIC;
int MPI_Group_union(MPI_Group group1, MPI_Group group2, MPI_Group *newgroup) MPICH_API_PUBLIC;
int MPI_Group_intersection(MPI_Group group1, MPI_Group group2, MPI_Group *newgroup) MPICH_API_PUBLIC;
int MPI_Group_difference(MPI_Group group1, MPI_Group group2, MPI_Group *newgroup) MPICH_API_PUBLIC;
int MPI_Group_incl(MPI_Group group, int n, const int ranks[], MPI_Group *newgroup) MPICH_API_PUBLIC;
int MPI_Group_excl(MPI_Group group, int n, const int ranks[], MPI_Group *newgroup) MPICH_API_PUBLIC;
int MPI_Group_range_incl(MPI_Group group, int n, int ranges[][3], MPI_Group *newgroup) MPICH_API_PUBLIC;
int MPI_Group_range_excl(MPI_Group group, int n, int ranges[][3], MPI_Group *newgroup) MPICH_API_PUBLIC;
int MPI_Group_free(MPI_Group *group) MPICH_API_PUBLIC;
int MPI_Comm_size(MPI_Comm comm, int *size) MPICH_API_PUBLIC;
int MPI_Comm_rank(MPI_Comm comm, int *rank) MPICH_API_PUBLIC;
int MPI_Comm_compare(MPI_Comm comm1, MPI_Comm comm2, int *result) MPICH_API_PUBLIC;
int MPI_Comm_dup(MPI_Comm comm, MPI_Comm *newcomm) MPICH_API_PUBLIC;
int MPI_Comm_dup_with_info(MPI_Comm comm, MPI_Info info, MPI_Comm *newcomm) MPICH_API_PUBLIC;
int MPI_Comm_create(MPI_Comm comm, MPI_Group group, MPI_Comm *newcomm) MPICH_API_PUBLIC;
int MPI_Comm_split(MPI_Comm comm, int color, int key, MPI_Comm *newcomm) MPICH_API_PUBLIC;
int MPI_Comm_free(MPI_Comm *comm) MPICH_API_PUBLIC;
int MPI_Comm_test_inter(MPI_Comm comm, int *flag) MPICH_API_PUBLIC;
int MPI_Comm_remote_size(MPI_Comm comm, int *size) MPICH_API_PUBLIC;
int MPI_Comm_remote_group(MPI_Comm comm, MPI_Group *group) MPICH_API_PUBLIC;
int MPI_Intercomm_create(MPI_Comm local_comm, int local_leader, MPI_Comm peer_comm,
                         int remote_leader, int tag, MPI_Comm *newintercomm) MPICH_API_PUBLIC;
int MPI_Intercomm_merge(MPI_Comm intercomm, int high, MPI_Comm *newintracomm) MPICH_API_PUBLIC;
int MPI_Keyval_create(MPI_Copy_function *copy_fn, MPI_Delete_function *delete_fn,
                      int *keyval, void *extra_state) MPICH_API_PUBLIC;
int MPI_Keyval_free(int *keyval) MPICH_API_PUBLIC;
int MPI_Attr_put(MPI_Comm comm, int keyval, void *attribute_val) MPICH_API_PUBLIC;
int MPI_Attr_get(MPI_Comm comm, int keyval, void *attribute_val, int *flag) MPICH_API_PUBLIC;
int MPI_Attr_delete(MPI_Comm comm, int keyval) MPICH_API_PUBLIC;
int MPI_Topo_test(MPI_Comm comm, int *status) MPICH_API_PUBLIC;
int MPI_Cart_create(MPI_Comm comm_old, int ndims, const int dims[], const int periods[],
                    int reorder, MPI_Comm *comm_cart) MPICH_API_PUBLIC;
int MPI_Dims_create(int nnodes, int ndims, int dims[]) MPICH_API_PUBLIC;
int MPI_Graph_create(MPI_Comm comm_old, int nnodes, const int indx[], const int edges[],
                     int reorder, MPI_Comm *comm_graph) MPICH_API_PUBLIC;
int MPI_Graphdims_get(MPI_Comm comm, int *nnodes, int *nedges) MPICH_API_PUBLIC;
int MPI_Graph_get(MPI_Comm comm, int maxindex, int maxedges, int indx[], int edges[]) MPICH_API_PUBLIC;
int MPI_Cartdim_get(MPI_Comm comm, int *ndims) MPICH_API_PUBLIC;
int MPI_Cart_get(MPI_Comm comm, int maxdims, int dims[], int periods[], int coords[]) MPICH_API_PUBLIC;
int MPI_Cart_rank(MPI_Comm comm, const int coords[], int *rank) MPICH_API_PUBLIC;
int MPI_Cart_coords(MPI_Comm comm, int rank, int maxdims, int coords[]) MPICH_API_PUBLIC;
int MPI_Graph_neighbors_count(MPI_Comm comm, int rank, int *nneighbors) MPICH_API_PUBLIC;
int MPI_Graph_neighbors(MPI_Comm comm, int rank, int maxneighbors, int neighbors[]) MPICH_API_PUBLIC;
int MPI_Cart_shift(MPI_Comm comm, int direction, int disp, int *rank_source, int *rank_dest) MPICH_API_PUBLIC;
int MPI_Cart_sub(MPI_Comm comm, const int remain_dims[], MPI_Comm *newcomm) MPICH_API_PUBLIC;
int MPI_Cart_map(MPI_Comm comm, int ndims, const int dims[], const int periods[], int *newrank) MPICH_API_PUBLIC;
int MPI_Graph_map(MPI_Comm comm, int nnodes, const int indx[], const int edges[], int *newrank) MPICH_API_PUBLIC;
int MPI_Get_processor_name(char *name, int *resultlen) MPICH_API_PUBLIC;
int MPI_Get_version(int *version, int *subversion) MPICH_API_PUBLIC;
int MPI_Get_library_version(char *version, int *resultlen) MPICH_API_PUBLIC;
int MPI_Errhandler_create(MPI_Handler_function *function, MPI_Errhandler *errhandler) MPICH_API_PUBLIC;
int MPI_Errhandler_set(MPI_Comm comm, MPI_Errhandler errhandler) MPICH_API_PUBLIC;
int MPI_Errhandler_get(MPI_Comm comm, MPI_Errhandler *errhandler) MPICH_API_PUBLIC;
int MPI_Errhandler_free(MPI_Errhandler *errhandler) MPICH_API_PUBLIC;
int MPI_Error_string(int errorcode, char *string, int *resultlen) MPICH_API_PUBLIC;
int MPI_Error_class(int errorcode, int *errorclass) MPICH_API_PUBLIC;
double MPI_Wtime(void) MPICH_API_PUBLIC;
double MPI_Wtick(void) MPICH_API_PUBLIC;
int MPI_Init(int *argc, char ***argv) MPICH_API_PUBLIC;
int MPI_Finalize(void) MPICH_API_PUBLIC;
int MPI_Initialized(int *flag) MPICH_API_PUBLIC;
int MPI_Abort(MPI_Comm comm, int errorcode) MPICH_API_PUBLIC;

/* Note that we may need to define a @PCONTROL_LIST@ depending on whether
   stdargs are supported */
int MPI_Pcontrol(const int level, ...) MPICH_API_PUBLIC;
int MPI_DUP_FN(MPI_Comm oldcomm, int keyval, void *extra_state, void *attribute_val_in,
               void *attribute_val_out, int *flag) MPICH_API_PUBLIC;

/* Process Creation and Management */
int MPI_Close_port(const char *port_name) MPICH_API_PUBLIC;
int MPI_Comm_accept(const char *port_name, MPI_Info info, int root, MPI_Comm comm,
                    MPI_Comm *newcomm) MPICH_API_PUBLIC;
int MPI_Comm_connect(const char *port_name, MPI_Info info, int root, MPI_Comm comm,
                     MPI_Comm *newcomm) MPICH_API_PUBLIC;
int MPI_Comm_disconnect(MPI_Comm *comm) MPICH_API_PUBLIC;
int MPI_Comm_get_parent(MPI_Comm *parent) MPICH_API_PUBLIC;
int MPI_Comm_join(int fd, MPI_Comm *intercomm) MPICH_API_PUBLIC;
int MPI_Comm_spawn(const char *command, char *argv[], int maxprocs, MPI_Info info, int root,
                   MPI_Comm comm, MPI_Comm *intercomm, int array_of_errcodes[]) MPICH_API_PUBLIC;
int MPI_Comm_spawn_multiple(int count, char *array_of_commands[], char **array_of_argv[],
                            const int array_of_maxprocs[], const MPI_Info array_of_info[],
                            int root, MPI_Comm comm, MPI_Comm *intercomm, int array_of_errcodes[]) MPICH_API_PUBLIC;
int MPI_Lookup_name(const char *service_name, MPI_Info info, char *port_name) MPICH_API_PUBLIC;
int MPI_Open_port(MPI_Info info, char *port_name) MPICH_API_PUBLIC;
int MPI_Publish_name(const char *service_name, MPI_Info info, const char *port_name) MPICH_API_PUBLIC;
int MPI_Unpublish_name(const char *service_name, MPI_Info info, const char *port_name) MPICH_API_PUBLIC;
int MPI_Comm_set_info(MPI_Comm comm, MPI_Info info) MPICH_API_PUBLIC;
int MPI_Comm_get_info(MPI_Comm comm, MPI_Info *info) MPICH_API_PUBLIC;

/* One-Sided Communications */
int MPI_Accumulate(const void *origin_addr, int origin_count, MPI_Datatype origin_datatype,
                   int target_rank, MPI_Aint target_disp, int target_count,
                   MPI_Datatype target_datatype, MPI_Op op, MPI_Win win)
                   MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3) MPICH_API_PUBLIC;
int MPI_Get(void *origin_addr, int origin_count, MPI_Datatype origin_datatype,
            int target_rank, MPI_Aint target_disp, int target_count,
            MPI_Datatype target_datatype, MPI_Win win) MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3) MPICH_API_PUBLIC;
int MPI_Put(const void *origin_addr, int origin_count, MPI_Datatype origin_datatype,
            int target_rank, MPI_Aint target_disp, int target_count,
            MPI_Datatype target_datatype, MPI_Win win) MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3) MPICH_API_PUBLIC;
int MPI_Win_complete(MPI_Win win) MPICH_API_PUBLIC;
int MPI_Win_create(void *base, MPI_Aint size, int disp_unit, MPI_Info info, MPI_Comm comm,
                   MPI_Win *win) MPICH_API_PUBLIC;
int MPI_Win_fence(int assert, MPI_Win win) MPICH_API_PUBLIC;
int MPI_Win_free(MPI_Win *win) MPICH_API_PUBLIC;
int MPI_Win_get_group(MPI_Win win, MPI_Group *group) MPICH_API_PUBLIC;
int MPI_Win_lock(int lock_type, int rank, int assert, MPI_Win win) MPICH_API_PUBLIC;
int MPI_Win_post(MPI_Group group, int assert, MPI_Win win) MPICH_API_PUBLIC;
int MPI_Win_start(MPI_Group group, int assert, MPI_Win win) MPICH_API_PUBLIC;
int MPI_Win_test(MPI_Win win, int *flag) MPICH_API_PUBLIC;
int MPI_Win_unlock(int rank, MPI_Win win) MPICH_API_PUBLIC;
int MPI_Win_wait(MPI_Win win) MPICH_API_PUBLIC;

/* MPI-3 One-Sided Communication Routines */
int MPI_Win_allocate(MPI_Aint size, int disp_unit, MPI_Info info, MPI_Comm comm, void *baseptr,
                     MPI_Win *win) MPICH_API_PUBLIC;
int MPI_Win_allocate_shared(MPI_Aint size, int disp_unit, MPI_Info info, MPI_Comm comm,
                            void *baseptr, MPI_Win *win) MPICH_API_PUBLIC;
int MPI_Win_shared_query(MPI_Win win, int rank, MPI_Aint *size, int *disp_unit, void *baseptr) MPICH_API_PUBLIC;
int MPI_Win_create_dynamic(MPI_Info info, MPI_Comm comm, MPI_Win *win) MPICH_API_PUBLIC;
int MPI_Win_attach(MPI_Win win, void *base, MPI_Aint size) MPICH_API_PUBLIC;
int MPI_Win_detach(MPI_Win win, const void *base) MPICH_API_PUBLIC;
int MPI_Win_get_info(MPI_Win win, MPI_Info *info_used) MPICH_API_PUBLIC;
int MPI_Win_set_info(MPI_Win win, MPI_Info info) MPICH_API_PUBLIC;
int MPI_Get_accumulate(const void *origin_addr, int origin_count,
                        MPI_Datatype origin_datatype, void *result_addr, int result_count,
                        MPI_Datatype result_datatype, int target_rank, MPI_Aint target_disp,
                        int target_count, MPI_Datatype target_datatype, MPI_Op op, MPI_Win win)
                        MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3)
                        MPICH_ATTR_POINTER_WITH_TYPE_TAG(4,6) MPICH_API_PUBLIC;
int MPI_Fetch_and_op(const void *origin_addr, void *result_addr,
                      MPI_Datatype datatype, int target_rank, MPI_Aint target_disp,
                      MPI_Op op, MPI_Win win)
                      MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3) MPICH_API_PUBLIC;
int MPI_Compare_and_swap(const void *origin_addr, const void *compare_addr,
                          void *result_addr, MPI_Datatype datatype, int target_rank,
                          MPI_Aint target_disp, MPI_Win win)
                          MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,4)
                          MPICH_ATTR_POINTER_WITH_TYPE_TAG(2,4)
                          MPICH_ATTR_POINTER_WITH_TYPE_TAG(3,4) MPICH_API_PUBLIC;
int MPI_Rput(const void *origin_addr, int origin_count,
              MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp,
              int target_count, MPI_Datatype target_datatype, MPI_Win win,
              MPI_Request *request)
              MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3) MPICH_API_PUBLIC;
int MPI_Rget(void *origin_addr, int origin_count,
              MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp,
              int target_count, MPI_Datatype target_datatype, MPI_Win win,
              MPI_Request *request)
              MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3) MPICH_API_PUBLIC;
int MPI_Raccumulate(const void *origin_addr, int origin_count,
                     MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp,
                     int target_count, MPI_Datatype target_datatype, MPI_Op op, MPI_Win win,
                     MPI_Request *request)
                     MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3) MPICH_API_PUBLIC;
int MPI_Rget_accumulate(const void *origin_addr, int origin_count,
                         MPI_Datatype origin_datatype, void *result_addr, int result_count,
                         MPI_Datatype result_datatype, int target_rank, MPI_Aint target_disp,
                         int target_count, MPI_Datatype target_datatype, MPI_Op op, MPI_Win win,
                         MPI_Request *request)
                         MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3)
                         MPICH_ATTR_POINTER_WITH_TYPE_TAG(4,6) MPICH_API_PUBLIC;
int MPI_Win_lock_all(int assert, MPI_Win win) MPICH_API_PUBLIC;
int MPI_Win_unlock_all(MPI_Win win) MPICH_API_PUBLIC;
int MPI_Win_flush(int rank, MPI_Win win) MPICH_API_PUBLIC;
int MPI_Win_flush_all(MPI_Win win) MPICH_API_PUBLIC;
int MPI_Win_flush_local(int rank, MPI_Win win) MPICH_API_PUBLIC;
int MPI_Win_flush_local_all(MPI_Win win) MPICH_API_PUBLIC;
int MPI_Win_sync(MPI_Win win) MPICH_API_PUBLIC;

/* External Interfaces */
int MPI_Add_error_class(int *errorclass) MPICH_API_PUBLIC;
int MPIX_Delete_error_class(int errorclass) MPICH_API_PUBLIC;
int MPI_Add_error_code(int errorclass, int *errorcode) MPICH_API_PUBLIC;
int MPIX_Delete_error_code(int errorcode) MPICH_API_PUBLIC;
int MPI_Add_error_string(int errorcode, const char *string) MPICH_API_PUBLIC;
int MPIX_Delete_error_string(int errorcode) MPICH_API_PUBLIC;
int MPI_Comm_call_errhandler(MPI_Comm comm, int errorcode) MPICH_API_PUBLIC;
int MPI_Comm_create_keyval(MPI_Comm_copy_attr_function *comm_copy_attr_fn,
                           MPI_Comm_delete_attr_function *comm_delete_attr_fn, int *comm_keyval,
                           void *extra_state) MPICH_API_PUBLIC;
int MPI_Comm_delete_attr(MPI_Comm comm, int comm_keyval) MPICH_API_PUBLIC;
int MPI_Comm_free_keyval(int *comm_keyval) MPICH_API_PUBLIC;
int MPI_Comm_get_attr(MPI_Comm comm, int comm_keyval, void *attribute_val, int *flag) MPICH_API_PUBLIC;
int MPI_Comm_get_name(MPI_Comm comm, char *comm_name, int *resultlen) MPICH_API_PUBLIC;
int MPI_Comm_set_attr(MPI_Comm comm, int comm_keyval, void *attribute_val) MPICH_API_PUBLIC;
int MPI_Comm_set_name(MPI_Comm comm, const char *comm_name) MPICH_API_PUBLIC;
int MPI_File_call_errhandler(MPI_File fh, int errorcode) MPICH_API_PUBLIC;
int MPI_Grequest_complete(MPI_Request request) MPICH_API_PUBLIC;
int MPI_Grequest_start(MPI_Grequest_query_function *query_fn, MPI_Grequest_free_function *free_fn,
                       MPI_Grequest_cancel_function *cancel_fn, void *extra_state,
                       MPI_Request *request) MPICH_API_PUBLIC;
int MPI_Init_thread(int *argc, char ***argv, int required, int *provided) MPICH_API_PUBLIC;
int MPI_Is_thread_main(int *flag) MPICH_API_PUBLIC;
int MPI_Query_thread(int *provided) MPICH_API_PUBLIC;
int MPI_Status_set_cancelled(MPI_Status *status, int flag) MPICH_API_PUBLIC;
int MPI_Status_set_elements(MPI_Status *status, MPI_Datatype datatype, int count) MPICH_API_PUBLIC;
int MPI_Type_create_keyval(MPI_Type_copy_attr_function *type_copy_attr_fn,
                           MPI_Type_delete_attr_function *type_delete_attr_fn,
                           int *type_keyval, void *extra_state) MPICH_API_PUBLIC;
int MPI_Type_delete_attr(MPI_Datatype datatype, int type_keyval) MPICH_API_PUBLIC;
int MPI_Type_dup(MPI_Datatype oldtype, MPI_Datatype *newtype) MPICH_API_PUBLIC;
int MPI_Type_free_keyval(int *type_keyval) MPICH_API_PUBLIC;
int MPI_Type_get_attr(MPI_Datatype datatype, int type_keyval, void *attribute_val, int *flag) MPICH_API_PUBLIC;
int MPI_Type_get_contents(MPI_Datatype datatype, int max_integers, int max_addresses,
                          int max_datatypes, int array_of_integers[],
                          MPI_Aint array_of_addresses[], MPI_Datatype array_of_datatypes[]) MPICH_API_PUBLIC;
int MPI_Type_get_envelope(MPI_Datatype datatype, int *num_integers, int *num_addresses,
                          int *num_datatypes, int *combiner) MPICH_API_PUBLIC;
int MPI_Type_get_name(MPI_Datatype datatype, char *type_name, int *resultlen) MPICH_API_PUBLIC;
int MPI_Type_set_attr(MPI_Datatype datatype, int type_keyval, void *attribute_val) MPICH_API_PUBLIC;
int MPI_Type_set_name(MPI_Datatype datatype, const char *type_name) MPICH_API_PUBLIC;
int MPI_Type_match_size(int typeclass, int size, MPI_Datatype *datatype) MPICH_API_PUBLIC;
int MPI_Win_call_errhandler(MPI_Win win, int errorcode) MPICH_API_PUBLIC;
int MPI_Win_create_keyval(MPI_Win_copy_attr_function *win_copy_attr_fn,
                          MPI_Win_delete_attr_function *win_delete_attr_fn, int *win_keyval,
                          void *extra_state) MPICH_API_PUBLIC;
int MPI_Win_delete_attr(MPI_Win win, int win_keyval) MPICH_API_PUBLIC;
int MPI_Win_free_keyval(int *win_keyval) MPICH_API_PUBLIC;
int MPI_Win_get_attr(MPI_Win win, int win_keyval, void *attribute_val, int *flag) MPICH_API_PUBLIC;
int MPI_Win_get_name(MPI_Win win, char *win_name, int *resultlen) MPICH_API_PUBLIC;
int MPI_Win_set_attr(MPI_Win win, int win_keyval, void *attribute_val) MPICH_API_PUBLIC;
int MPI_Win_set_name(MPI_Win win, const char *win_name) MPICH_API_PUBLIC;

int MPI_Alloc_mem(MPI_Aint size, MPI_Info info, void *baseptr) MPICH_API_PUBLIC;
int MPI_Comm_create_errhandler(MPI_Comm_errhandler_function *comm_errhandler_fn,
                               MPI_Errhandler *errhandler) MPICH_API_PUBLIC;
int MPI_Comm_get_errhandler(MPI_Comm comm, MPI_Errhandler *errhandler) MPICH_API_PUBLIC;
int MPI_Comm_set_errhandler(MPI_Comm comm, MPI_Errhandler errhandler) MPICH_API_PUBLIC;
int MPI_File_create_errhandler(MPI_File_errhandler_function *file_errhandler_fn,
                               MPI_Errhandler *errhandler) MPICH_API_PUBLIC;
int MPI_File_get_errhandler(MPI_File file, MPI_Errhandler *errhandler) MPICH_API_PUBLIC;
int MPI_File_set_errhandler(MPI_File file, MPI_Errhandler errhandler) MPICH_API_PUBLIC;
int MPI_Finalized(int *flag) MPICH_API_PUBLIC;
int MPI_Free_mem(void *base) MPICH_API_PUBLIC;
int MPI_Get_address(const void *location, MPI_Aint *address) MPICH_API_PUBLIC;
int MPI_Info_create(MPI_Info *info) MPICH_API_PUBLIC;
int MPI_Info_create_env(int *argc, char ***argv, MPI_Info *info) MPICH_API_PUBLIC;
int MPI_Info_delete(MPI_Info info, const char *key) MPICH_API_PUBLIC;
int MPI_Info_dup(MPI_Info info, MPI_Info *newinfo) MPICH_API_PUBLIC;
int MPI_Info_free(MPI_Info *info) MPICH_API_PUBLIC;
int MPI_Info_get(MPI_Info info, const char *key, int valuelen, char *value, int *flag) MPICH_API_PUBLIC;
int MPI_Info_get_nkeys(MPI_Info info, int *nkeys) MPICH_API_PUBLIC;
int MPI_Info_get_nthkey(MPI_Info info, int n, char *key) MPICH_API_PUBLIC;
int MPI_Info_get_valuelen(MPI_Info info, const char *key, int *valuelen, int *flag) MPICH_API_PUBLIC;
int MPI_Info_get_string(MPI_Info info, const char *key, int *buflen, char *value, int *flag) MPICH_API_PUBLIC;
int MPI_Info_set(MPI_Info info, const char *key, const char *value) MPICH_API_PUBLIC;
int MPI_Pack_external(const char datarep[], const void *inbuf, int incount,
                      MPI_Datatype datatype, void *outbuf, MPI_Aint outsize, MPI_Aint *position)
                      MPICH_ATTR_POINTER_WITH_TYPE_TAG(2,4) MPICH_API_PUBLIC;
int MPI_Pack_external_size(const char datarep[], int incount, MPI_Datatype datatype,
                           MPI_Aint *size) MPICH_API_PUBLIC;
int MPI_Request_get_status(MPI_Request request, int *flag, MPI_Status *status) MPICH_API_PUBLIC;
int MPI_Status_c2f(const MPI_Status *c_status, MPI_Fint *f_status) MPICH_API_PUBLIC;
int MPI_Status_f2c(const MPI_Fint *f_status, MPI_Status *c_status) MPICH_API_PUBLIC;
int MPI_Type_create_darray(int size, int rank, int ndims, const int array_of_gsizes[],
                           const int array_of_distribs[], const int array_of_dargs[],
                           const int array_of_psizes[], int order, MPI_Datatype oldtype,
                           MPI_Datatype *newtype) MPICH_API_PUBLIC;
int MPI_Type_create_hindexed(int count, const int array_of_blocklengths[],
                             const MPI_Aint array_of_displacements[], MPI_Datatype oldtype,
                             MPI_Datatype *newtype) MPICH_API_PUBLIC;
int MPI_Type_create_hvector(int count, int blocklength, MPI_Aint stride, MPI_Datatype oldtype,
                            MPI_Datatype *newtype) MPICH_API_PUBLIC;
int MPI_Type_create_indexed_block(int count, int blocklength, const int array_of_displacements[],
                                  MPI_Datatype oldtype, MPI_Datatype *newtype) MPICH_API_PUBLIC;
int MPI_Type_create_hindexed_block(int count, int blocklength,
                                   const MPI_Aint array_of_displacements[],
                                   MPI_Datatype oldtype, MPI_Datatype *newtype) MPICH_API_PUBLIC;
int MPI_Type_create_resized(MPI_Datatype oldtype, MPI_Aint lb, MPI_Aint extent,
                            MPI_Datatype *newtype) MPICH_API_PUBLIC;
int MPI_Type_create_struct(int count, const int array_of_blocklengths[],
                           const MPI_Aint array_of_displacements[],
                           const MPI_Datatype array_of_types[], MPI_Datatype *newtype) MPICH_API_PUBLIC;
int MPI_Type_create_subarray(int ndims, const int array_of_sizes[],
                             const int array_of_subsizes[], const int array_of_starts[],
                             int order, MPI_Datatype oldtype, MPI_Datatype *newtype) MPICH_API_PUBLIC;
int MPI_Type_get_extent(MPI_Datatype datatype, MPI_Aint *lb, MPI_Aint *extent) MPICH_API_PUBLIC;
int MPI_Type_get_true_extent(MPI_Datatype datatype, MPI_Aint *true_lb, MPI_Aint *true_extent) MPICH_API_PUBLIC;
int MPI_Unpack_external(const char datarep[], const void *inbuf, MPI_Aint insize,
                        MPI_Aint *position, void *outbuf, int outcount, MPI_Datatype datatype)
                        MPICH_ATTR_POINTER_WITH_TYPE_TAG(5,7) MPICH_API_PUBLIC;
int MPI_Win_create_errhandler(MPI_Win_errhandler_function *win_errhandler_fn,
                              MPI_Errhandler *errhandler) MPICH_API_PUBLIC;
int MPI_Win_get_errhandler(MPI_Win win, MPI_Errhandler *errhandler) MPICH_API_PUBLIC;
int MPI_Win_set_errhandler(MPI_Win win, MPI_Errhandler errhandler) MPICH_API_PUBLIC;

/* Fortran 90-related functions.  These routines are available only if
   Fortran 90 support is enabled
*/
int MPI_Type_create_f90_integer(int range, MPI_Datatype *newtype) MPICH_API_PUBLIC;
int MPI_Type_create_f90_real(int precision, int range, MPI_Datatype *newtype) MPICH_API_PUBLIC;
int MPI_Type_create_f90_complex(int precision, int range, MPI_Datatype *newtype) MPICH_API_PUBLIC;

int MPI_Reduce_local(const void *inbuf, void *inoutbuf, int count, MPI_Datatype datatype,
                     MPI_Op op)
                     MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,4) MPICH_ATTR_POINTER_WITH_TYPE_TAG(2,4) MPICH_API_PUBLIC;
int MPI_Op_commutative(MPI_Op op, int *commute) MPICH_API_PUBLIC;
int MPI_Reduce_scatter_block(const void *sendbuf, void *recvbuf, int recvcount,
                             MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
                             MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,4)
                             MPICH_ATTR_POINTER_WITH_TYPE_TAG(2,4) MPICH_API_PUBLIC;
int MPI_Dist_graph_create_adjacent(MPI_Comm comm_old, int indegree, const int sources[],
                                   const int sourceweights[], int outdegree,
                                   const int destinations[], const int destweights[],
                                   MPI_Info info, int reorder, MPI_Comm *comm_dist_graph) MPICH_API_PUBLIC;
int MPI_Dist_graph_create(MPI_Comm comm_old, int n, const int sources[], const int degrees[],
                          const int destinations[], const int weights[], MPI_Info info,
                          int reorder, MPI_Comm *comm_dist_graph) MPICH_API_PUBLIC;
int MPI_Dist_graph_neighbors_count(MPI_Comm comm, int *indegree, int *outdegree, int *weighted) MPICH_API_PUBLIC;
int MPI_Dist_graph_neighbors(MPI_Comm comm, int maxindegree, int sources[], int sourceweights[],
                             int maxoutdegree, int destinations[], int destweights[]) MPICH_API_PUBLIC;

/* Matched probe functionality */
int MPI_Improbe(int source, int tag, MPI_Comm comm, int *flag, MPI_Message *message,
                MPI_Status *status) MPICH_API_PUBLIC;
int MPI_Imrecv(void *buf, int count, MPI_Datatype datatype, MPI_Message *message,
               MPI_Request *request) MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3) MPICH_API_PUBLIC;
int MPI_Mprobe(int source, int tag, MPI_Comm comm, MPI_Message *message, MPI_Status *status) MPICH_API_PUBLIC;
int MPI_Mrecv(void *buf, int count, MPI_Datatype datatype, MPI_Message *message,
              MPI_Status *status) MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3) MPICH_API_PUBLIC;

/* Nonblocking collectives */
int MPI_Comm_idup(MPI_Comm comm, MPI_Comm *newcomm, MPI_Request *request) MPICH_API_PUBLIC;
int MPI_Comm_idup_with_info(MPI_Comm comm, MPI_Info info, MPI_Comm *newcomm,
                            MPI_Request *request) MPICH_API_PUBLIC;
int MPI_Ibarrier(MPI_Comm comm, MPI_Request *request) MPICH_API_PUBLIC;
int MPI_Ibcast(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm,
               MPI_Request *request) MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3) MPICH_API_PUBLIC;
int MPI_Igather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm,
                MPI_Request *request)
                MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3) MPICH_ATTR_POINTER_WITH_TYPE_TAG(4,6) MPICH_API_PUBLIC;
int MPI_Igatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                 const int recvcounts[], const int displs[], MPI_Datatype recvtype, int root,
                 MPI_Comm comm, MPI_Request *request)
                 MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3) MPICH_ATTR_POINTER_WITH_TYPE_TAG(4,7) MPICH_API_PUBLIC;
int MPI_Iscatter(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                 int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm,
                 MPI_Request *request)
                 MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3) MPICH_ATTR_POINTER_WITH_TYPE_TAG(4,6) MPICH_API_PUBLIC;
int MPI_Iscatterv(const void *sendbuf, const int sendcounts[], const int displs[],
                  MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype,
                  int root, MPI_Comm comm, MPI_Request *request)
                  MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,4) MPICH_ATTR_POINTER_WITH_TYPE_TAG(5,7) MPICH_API_PUBLIC;
int MPI_Iallgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                   int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request)
                   MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3) MPICH_ATTR_POINTER_WITH_TYPE_TAG(4,6) MPICH_API_PUBLIC;
int MPI_Iallgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                    const int recvcounts[], const int displs[], MPI_Datatype recvtype,
                    MPI_Comm comm, MPI_Request *request)
                    MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3) MPICH_ATTR_POINTER_WITH_TYPE_TAG(4,7) MPICH_API_PUBLIC;
int MPI_Ialltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                  int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request)
                  MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3) MPICH_ATTR_POINTER_WITH_TYPE_TAG(4,6) MPICH_API_PUBLIC;
int MPI_Ialltoallv(const void *sendbuf, const int sendcounts[], const int sdispls[],
                   MPI_Datatype sendtype, void *recvbuf, const int recvcounts[],
                   const int rdispls[], MPI_Datatype recvtype, MPI_Comm comm,
                   MPI_Request *request)
                   MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,4) MPICH_ATTR_POINTER_WITH_TYPE_TAG(5,8) MPICH_API_PUBLIC;
int MPI_Ialltoallw(const void *sendbuf, const int sendcounts[], const int sdispls[],
                   const MPI_Datatype sendtypes[], void *recvbuf, const int recvcounts[],
                   const int rdispls[], const MPI_Datatype recvtypes[], MPI_Comm comm,
                   MPI_Request *request) MPICH_API_PUBLIC;
int MPI_Ireduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                MPI_Op op, int root, MPI_Comm comm, MPI_Request *request)
                MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,4) MPICH_ATTR_POINTER_WITH_TYPE_TAG(2,4) MPICH_API_PUBLIC;
int MPI_Iallreduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                   MPI_Op op, MPI_Comm comm, MPI_Request *request)
                   MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,4) MPICH_ATTR_POINTER_WITH_TYPE_TAG(2,4) MPICH_API_PUBLIC;
int MPI_Ireduce_scatter(const void *sendbuf, void *recvbuf, const int recvcounts[],
                        MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request)
                        MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,4) MPICH_ATTR_POINTER_WITH_TYPE_TAG(2,4) MPICH_API_PUBLIC;
int MPI_Ireduce_scatter_block(const void *sendbuf, void *recvbuf, int recvcount,
                              MPI_Datatype datatype, MPI_Op op, MPI_Comm comm,
                              MPI_Request *request)
                              MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,4)
                              MPICH_ATTR_POINTER_WITH_TYPE_TAG(2,4) MPICH_API_PUBLIC;
int MPI_Iscan(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op,
              MPI_Comm comm, MPI_Request *request)
              MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,4) MPICH_ATTR_POINTER_WITH_TYPE_TAG(2,4) MPICH_API_PUBLIC;
int MPI_Iexscan(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                MPI_Op op, MPI_Comm comm, MPI_Request *request)
                MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,4) MPICH_ATTR_POINTER_WITH_TYPE_TAG(2,4) MPICH_API_PUBLIC;

/* Neighborhood collectives */
int MPI_Ineighbor_allgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                            void *recvbuf, int recvcount, MPI_Datatype recvtype,
                            MPI_Comm comm, MPI_Request *request)
                            MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3)
                            MPICH_ATTR_POINTER_WITH_TYPE_TAG(4,6) MPICH_API_PUBLIC;
int MPI_Ineighbor_allgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                             void *recvbuf, const int recvcounts[], const int displs[],
                             MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request)
                             MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3)
                             MPICH_ATTR_POINTER_WITH_TYPE_TAG(4,7) MPICH_API_PUBLIC;
int MPI_Ineighbor_alltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                           void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm,
                           MPI_Request *request)
                           MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3)
                           MPICH_ATTR_POINTER_WITH_TYPE_TAG(4,6) MPICH_API_PUBLIC;
int MPI_Ineighbor_alltoallv(const void *sendbuf, const int sendcounts[], const int sdispls[],
                            MPI_Datatype sendtype, void *recvbuf, const int recvcounts[],
                            const int rdispls[], MPI_Datatype recvtype, MPI_Comm comm,
                            MPI_Request *request)
                            MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,4)
                            MPICH_ATTR_POINTER_WITH_TYPE_TAG(5,8) MPICH_API_PUBLIC;
int MPI_Ineighbor_alltoallw(const void *sendbuf, const int sendcounts[],
                            const MPI_Aint sdispls[], const MPI_Datatype sendtypes[],
                            void *recvbuf, const int recvcounts[], const MPI_Aint rdispls[],
                            const MPI_Datatype recvtypes[], MPI_Comm comm, MPI_Request *request) MPICH_API_PUBLIC;
int MPI_Neighbor_allgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                           void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm)
                           MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3)
                           MPICH_ATTR_POINTER_WITH_TYPE_TAG(4,6) MPICH_API_PUBLIC;
int MPI_Neighbor_allgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                            void *recvbuf, const int recvcounts[], const int displs[],
                            MPI_Datatype recvtype, MPI_Comm comm)
                            MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3)
                            MPICH_ATTR_POINTER_WITH_TYPE_TAG(4,7) MPICH_API_PUBLIC;
int MPI_Neighbor_alltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                          void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm)
                          MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3)
                          MPICH_ATTR_POINTER_WITH_TYPE_TAG(4,6) MPICH_API_PUBLIC;
int MPI_Neighbor_alltoallv(const void *sendbuf, const int sendcounts[], const int sdispls[],
                           MPI_Datatype sendtype, void *recvbuf, const int recvcounts[],
                           const int rdispls[], MPI_Datatype recvtype, MPI_Comm comm)
                           MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,4)
                           MPICH_ATTR_POINTER_WITH_TYPE_TAG(5,8) MPICH_API_PUBLIC;
int MPI_Neighbor_alltoallw(const void *sendbuf, const int sendcounts[], const MPI_Aint sdispls[],
                           const MPI_Datatype sendtypes[], void *recvbuf, const int recvcounts[],
                           const MPI_Aint rdispls[], const MPI_Datatype recvtypes[], MPI_Comm comm) MPICH_API_PUBLIC;

/* Shared memory */
int MPI_Comm_split_type(MPI_Comm comm, int split_type, int key, MPI_Info info, MPI_Comm *newcomm) MPICH_API_PUBLIC;

/* MPI-3 "large count" routines */
int MPI_Get_elements_x(const MPI_Status *status, MPI_Datatype datatype, MPI_Count *count) MPICH_API_PUBLIC;
int MPI_Status_set_elements_x(MPI_Status *status, MPI_Datatype datatype, MPI_Count count) MPICH_API_PUBLIC;
int MPI_Type_get_extent_x(MPI_Datatype datatype, MPI_Count *lb, MPI_Count *extent) MPICH_API_PUBLIC;
int MPI_Type_get_true_extent_x(MPI_Datatype datatype, MPI_Count *lb, MPI_Count *extent) MPICH_API_PUBLIC;
int MPI_Type_size_x(MPI_Datatype datatype, MPI_Count *size) MPICH_API_PUBLIC;

/* Noncollective communicator creation */
int MPI_Comm_create_group(MPI_Comm comm, MPI_Group group, int tag, MPI_Comm *newcomm) MPICH_API_PUBLIC;

/* MPI_Aint addressing arithmetic */
MPI_Aint MPI_Aint_add(MPI_Aint base, MPI_Aint disp) MPICH_API_PUBLIC;
MPI_Aint MPI_Aint_diff(MPI_Aint addr1, MPI_Aint addr2) MPICH_API_PUBLIC;

/* MPI_T interface */
/* The MPI_T routines are available only in C bindings - tell tools that they
   can skip these prototypes */
/* Begin Skip Prototypes */
int MPI_T_init_thread(int required, int *provided) MPICH_API_PUBLIC;
int MPI_T_finalize(void) MPICH_API_PUBLIC;
int MPI_T_enum_get_info(MPI_T_enum enumtype, int *num, char *name, int *name_len) MPICH_API_PUBLIC;
int MPI_T_enum_get_item(MPI_T_enum enumtype, int indx, int *value, char *name, int *name_len) MPICH_API_PUBLIC;
int MPI_T_cvar_get_num(int *num_cvar) MPICH_API_PUBLIC;
int MPI_T_cvar_get_info(int cvar_index, char *name, int *name_len, int *verbosity,
                        MPI_Datatype *datatype, MPI_T_enum *enumtype, char *desc, int *desc_len,
                        int *binding, int *scope) MPICH_API_PUBLIC;
int MPI_T_cvar_handle_alloc(int cvar_index, void *obj_handle, MPI_T_cvar_handle *handle,
                            int *count) MPICH_API_PUBLIC;
int MPI_T_cvar_handle_free(MPI_T_cvar_handle *handle) MPICH_API_PUBLIC;
int MPI_T_cvar_read(MPI_T_cvar_handle handle, void *buf) MPICH_API_PUBLIC;
int MPI_T_cvar_write(MPI_T_cvar_handle handle, const void *buf) MPICH_API_PUBLIC;
int MPI_T_pvar_get_num(int *num_pvar) MPICH_API_PUBLIC;
int MPI_T_pvar_get_info(int pvar_index, char *name, int *name_len, int *verbosity, int *var_class,
                        MPI_Datatype *datatype, MPI_T_enum *enumtype, char *desc, int *desc_len,
                        int *binding, int *readonly, int *continuous, int *atomic) MPICH_API_PUBLIC;
int MPI_T_pvar_session_create(MPI_T_pvar_session *session) MPICH_API_PUBLIC;
int MPI_T_pvar_session_free(MPI_T_pvar_session *session) MPICH_API_PUBLIC;
int MPI_T_pvar_handle_alloc(MPI_T_pvar_session session, int pvar_index, void *obj_handle,
                            MPI_T_pvar_handle *handle, int *count) MPICH_API_PUBLIC;
int MPI_T_pvar_handle_free(MPI_T_pvar_session session, MPI_T_pvar_handle *handle) MPICH_API_PUBLIC;
int MPI_T_pvar_start(MPI_T_pvar_session session, MPI_T_pvar_handle handle) MPICH_API_PUBLIC;
int MPI_T_pvar_stop(MPI_T_pvar_session session, MPI_T_pvar_handle handle) MPICH_API_PUBLIC;
int MPI_T_pvar_read(MPI_T_pvar_session session, MPI_T_pvar_handle handle, void *buf) MPICH_API_PUBLIC;
int MPI_T_pvar_write(MPI_T_pvar_session session, MPI_T_pvar_handle handle, const void *buf) MPICH_API_PUBLIC;
int MPI_T_pvar_reset(MPI_T_pvar_session session, MPI_T_pvar_handle handle) MPICH_API_PUBLIC;
int MPI_T_pvar_readreset(MPI_T_pvar_session session, MPI_T_pvar_handle handle, void *buf) MPICH_API_PUBLIC;
int MPI_T_category_get_num(int *num_cat) MPICH_API_PUBLIC;
int MPI_T_category_get_info(int cat_index, char *name, int *name_len, char *desc, int *desc_len,
                            int *num_cvars, int *num_pvars, int *num_categories) MPICH_API_PUBLIC;
int MPI_T_category_get_cvars(int cat_index, int len, int indices[]) MPICH_API_PUBLIC;
int MPI_T_category_get_pvars(int cat_index, int len, int indices[]) MPICH_API_PUBLIC;
int MPI_T_category_get_categories(int cat_index, int len, int indices[]) MPICH_API_PUBLIC;
int MPI_T_category_changed(int *stamp) MPICH_API_PUBLIC;
int MPI_T_cvar_get_index(const char *name, int *cvar_index) MPICH_API_PUBLIC;
int MPI_T_pvar_get_index(const char *name, int var_class, int *pvar_index) MPICH_API_PUBLIC;
int MPI_T_category_get_index(const char *name, int *cat_index) MPICH_API_PUBLIC;
/* End Skip Prototypes */


/* Non-standard but public extensions to MPI */
/* Fault Tolerance Extensions */
int MPIX_Comm_failure_ack(MPI_Comm comm) MPICH_API_PUBLIC;
int MPIX_Comm_failure_get_acked(MPI_Comm comm, MPI_Group *failedgrp) MPICH_API_PUBLIC;
int MPIX_Comm_revoke(MPI_Comm comm) MPICH_API_PUBLIC;
int MPIX_Comm_shrink(MPI_Comm comm, MPI_Comm *newcomm) MPICH_API_PUBLIC;
int MPIX_Comm_agree(MPI_Comm comm, int *flag) MPICH_API_PUBLIC;

/* GPU extensions */
int MPIX_GPU_query_support(int gpu_type, int *is_supported) MPICH_API_PUBLIC;
int MPIX_Query_cuda_support(void) MPICH_API_PUBLIC;

/* End Prototypes */

/* Here are the bindings of the profiling routines */
#if !defined(MPI_BUILD_PROFILING)
int PMPI_Send(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
              MPI_Comm comm) MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3) MPICH_API_PUBLIC;
int PMPI_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag,
              MPI_Comm comm, MPI_Status *status) MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3) MPICH_API_PUBLIC;
int PMPI_Get_count(const MPI_Status *status, MPI_Datatype datatype, int *count) MPICH_API_PUBLIC;
int PMPI_Bsend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
               MPI_Comm comm) MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3) MPICH_API_PUBLIC;
int PMPI_Ssend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
               MPI_Comm comm) MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3) MPICH_API_PUBLIC;
int PMPI_Rsend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
               MPI_Comm comm) MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3) MPICH_API_PUBLIC;
int PMPI_Buffer_attach(void *buffer, int size) MPICH_API_PUBLIC;
int PMPI_Buffer_detach(void *buffer_addr, int *size) MPICH_API_PUBLIC;
int PMPI_Isend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
               MPI_Comm comm, MPI_Request *request) MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3) MPICH_API_PUBLIC;
int PMPI_Ibsend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
                MPI_Comm comm, MPI_Request *request) MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3) MPICH_API_PUBLIC;
int PMPI_Issend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
                MPI_Comm comm, MPI_Request *request) MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3) MPICH_API_PUBLIC;
int PMPI_Irsend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
                MPI_Comm comm, MPI_Request *request) MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3) MPICH_API_PUBLIC;
int PMPI_Irecv(void *buf, int count, MPI_Datatype datatype, int source, int tag,
               MPI_Comm comm, MPI_Request *request) MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3) MPICH_API_PUBLIC;
int PMPI_Wait(MPI_Request *request, MPI_Status *status) MPICH_API_PUBLIC;
int PMPI_Test(MPI_Request *request, int *flag, MPI_Status *status) MPICH_API_PUBLIC;
int PMPI_Request_free(MPI_Request *request) MPICH_API_PUBLIC;
int PMPI_Waitany(int count, MPI_Request array_of_requests[], int *indx, MPI_Status *status) MPICH_API_PUBLIC;
int PMPI_Testany(int count, MPI_Request array_of_requests[], int *indx, int *flag,
                 MPI_Status *status) MPICH_API_PUBLIC;
int PMPI_Waitall(int count, MPI_Request array_of_requests[], MPI_Status array_of_statuses[]) MPICH_API_PUBLIC;
int PMPI_Testall(int count, MPI_Request array_of_requests[], int *flag,
                 MPI_Status array_of_statuses[]) MPICH_API_PUBLIC;
int PMPI_Waitsome(int incount, MPI_Request array_of_requests[], int *outcount,
                  int array_of_indices[], MPI_Status array_of_statuses[]) MPICH_API_PUBLIC;
int PMPI_Testsome(int incount, MPI_Request array_of_requests[], int *outcount,
                  int array_of_indices[], MPI_Status array_of_statuses[]) MPICH_API_PUBLIC;
int PMPI_Iprobe(int source, int tag, MPI_Comm comm, int *flag, MPI_Status *status) MPICH_API_PUBLIC;
int PMPI_Probe(int source, int tag, MPI_Comm comm, MPI_Status *status) MPICH_API_PUBLIC;
int PMPI_Cancel(MPI_Request *request) MPICH_API_PUBLIC;
int PMPI_Test_cancelled(const MPI_Status *status, int *flag) MPICH_API_PUBLIC;
int PMPI_Send_init(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
                   MPI_Comm comm, MPI_Request *request) MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3) MPICH_API_PUBLIC;
int PMPI_Bsend_init(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
                    MPI_Comm comm, MPI_Request *request) MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3) MPICH_API_PUBLIC;
int PMPI_Ssend_init(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
                    MPI_Comm comm, MPI_Request *request) MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3) MPICH_API_PUBLIC;
int PMPI_Rsend_init(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
                    MPI_Comm comm, MPI_Request *request) MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3) MPICH_API_PUBLIC;
int PMPI_Recv_init(void *buf, int count, MPI_Datatype datatype, int source, int tag,
                   MPI_Comm comm, MPI_Request *request) MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3) MPICH_API_PUBLIC;
int PMPI_Start(MPI_Request *request) MPICH_API_PUBLIC;
int PMPI_Startall(int count, MPI_Request array_of_requests[]) MPICH_API_PUBLIC;
int PMPI_Sendrecv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, int dest,
                  int sendtag, void *recvbuf, int recvcount, MPI_Datatype recvtype,
                  int source, int recvtag, MPI_Comm comm, MPI_Status *status)
                  MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3)
                  MPICH_ATTR_POINTER_WITH_TYPE_TAG(6,8) MPICH_API_PUBLIC;
int PMPI_Sendrecv_replace(void *buf, int count, MPI_Datatype datatype, int dest,
                          int sendtag, int source, int recvtag, MPI_Comm comm,
                          MPI_Status *status) MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3) MPICH_API_PUBLIC;
int PMPI_Isendrecv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, int dest,
                   int sendtag, void *recvbuf, int recvcount, MPI_Datatype recvtype,
                   int source, int recvtag, MPI_Comm comm, MPI_Request *request)
                   MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3)
                   MPICH_ATTR_POINTER_WITH_TYPE_TAG(6,8) MPICH_API_PUBLIC;
int PMPI_Isendrecv_replace(void *buf, int count, MPI_Datatype datatype, int dest,
                           int sendtag, int source, int recvtag, MPI_Comm comm,
                           MPI_Request *request) MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3) MPICH_API_PUBLIC;
int PMPI_Type_contiguous(int count, MPI_Datatype oldtype, MPI_Datatype *newtype) MPICH_API_PUBLIC;
int PMPI_Type_vector(int count, int blocklength, int stride, MPI_Datatype oldtype,
                     MPI_Datatype *newtype) MPICH_API_PUBLIC;
int PMPI_Type_hvector(int count, int blocklength, MPI_Aint stride, MPI_Datatype oldtype,
                      MPI_Datatype *newtype) MPICH_API_PUBLIC;
int PMPI_Type_indexed(int count, const int *array_of_blocklengths,
                      const int *array_of_displacements, MPI_Datatype oldtype,
                      MPI_Datatype *newtype) MPICH_API_PUBLIC;
int PMPI_Type_hindexed(int count, int *array_of_blocklengths,
                       MPI_Aint *array_of_displacements, MPI_Datatype oldtype,
                       MPI_Datatype *newtype) MPICH_API_PUBLIC;
int PMPI_Type_struct(int count, int *array_of_blocklengths,
                     MPI_Aint *array_of_displacements,
                     MPI_Datatype *array_of_types, MPI_Datatype *newtype) MPICH_API_PUBLIC;
int PMPI_Address(void *location, MPI_Aint *address) MPICH_API_PUBLIC;
int PMPI_Type_extent(MPI_Datatype datatype, MPI_Aint *extent) MPICH_API_PUBLIC;
int PMPI_Type_size(MPI_Datatype datatype, int *size) MPICH_API_PUBLIC;
int PMPI_Type_lb(MPI_Datatype datatype, MPI_Aint *displacement) MPICH_API_PUBLIC;
int PMPI_Type_ub(MPI_Datatype datatype, MPI_Aint *displacement) MPICH_API_PUBLIC;
int PMPI_Type_commit(MPI_Datatype *datatype) MPICH_API_PUBLIC;
int PMPI_Type_free(MPI_Datatype *datatype) MPICH_API_PUBLIC;
int PMPI_Get_elements(const MPI_Status *status, MPI_Datatype datatype, int *count) MPICH_API_PUBLIC;
int PMPI_Pack(const void *inbuf, int incount, MPI_Datatype datatype, void *outbuf,
              int outsize, int *position, MPI_Comm comm) MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3) MPICH_API_PUBLIC;
int PMPI_Unpack(const void *inbuf, int insize, int *position, void *outbuf, int outcount,
                MPI_Datatype datatype, MPI_Comm comm) MPICH_ATTR_POINTER_WITH_TYPE_TAG(4,6) MPICH_API_PUBLIC;
int PMPI_Pack_size(int incount, MPI_Datatype datatype, MPI_Comm comm, int *size) MPICH_API_PUBLIC;
int PMPI_Barrier(MPI_Comm comm) MPICH_API_PUBLIC;
int PMPI_Bcast(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm)
               MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3) MPICH_API_PUBLIC;
int PMPI_Gather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm)
                MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3) MPICH_ATTR_POINTER_WITH_TYPE_TAG(4,6) MPICH_API_PUBLIC;
int PMPI_Gatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                 const int *recvcounts, const int *displs, MPI_Datatype recvtype, int root,
                 MPI_Comm comm)
                 MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3) MPICH_ATTR_POINTER_WITH_TYPE_TAG(4,7) MPICH_API_PUBLIC;
int PMPI_Scatter(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                 int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm)
                 MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3) MPICH_ATTR_POINTER_WITH_TYPE_TAG(4,6) MPICH_API_PUBLIC;
int PMPI_Scatterv(const void *sendbuf, const int *sendcounts, const int *displs,
                  MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype,
                  int root, MPI_Comm comm)
                  MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,4) MPICH_ATTR_POINTER_WITH_TYPE_TAG(5,7) MPICH_API_PUBLIC;
int PMPI_Allgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                   int recvcount, MPI_Datatype recvtype, MPI_Comm comm)
                   MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3) MPICH_ATTR_POINTER_WITH_TYPE_TAG(4,6) MPICH_API_PUBLIC;
int PMPI_Allgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                    const int *recvcounts, const int *displs, MPI_Datatype recvtype, MPI_Comm comm)
                    MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3) MPICH_ATTR_POINTER_WITH_TYPE_TAG(4,7) MPICH_API_PUBLIC;
int PMPI_Alltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                  int recvcount, MPI_Datatype recvtype, MPI_Comm comm)
                  MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3) MPICH_ATTR_POINTER_WITH_TYPE_TAG(4,6) MPICH_API_PUBLIC;
int PMPI_Alltoallv(const void *sendbuf, const int *sendcounts, const int *sdispls,
                   MPI_Datatype sendtype, void *recvbuf, const int *recvcounts,
                   const int *rdispls, MPI_Datatype recvtype, MPI_Comm comm)
                   MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,4) MPICH_ATTR_POINTER_WITH_TYPE_TAG(5,8) MPICH_API_PUBLIC;
int PMPI_Alltoallw(const void *sendbuf, const int sendcounts[], const int sdispls[],
                   const MPI_Datatype sendtypes[], void *recvbuf, const int recvcounts[],
                   const int rdispls[], const MPI_Datatype recvtypes[], MPI_Comm comm) MPICH_API_PUBLIC;
int PMPI_Exscan(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                MPI_Op op, MPI_Comm comm)
                MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,4) MPICH_ATTR_POINTER_WITH_TYPE_TAG(2,4) MPICH_API_PUBLIC;
int PMPI_Reduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                MPI_Op op, int root, MPI_Comm comm)
                MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,4) MPICH_ATTR_POINTER_WITH_TYPE_TAG(2,4) MPICH_API_PUBLIC;
int PMPI_Op_create(MPI_User_function *user_fn, int commute, MPI_Op *op) MPICH_API_PUBLIC;
int PMPI_Op_free(MPI_Op *op) MPICH_API_PUBLIC;
int PMPI_Allreduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                   MPI_Op op, MPI_Comm comm)
                   MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,4) MPICH_ATTR_POINTER_WITH_TYPE_TAG(2,4) MPICH_API_PUBLIC;
int PMPI_Reduce_scatter(const void *sendbuf, void *recvbuf, const int recvcounts[],
                        MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
                        MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,4) MPICH_ATTR_POINTER_WITH_TYPE_TAG(2,4) MPICH_API_PUBLIC;
int PMPI_Scan(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op,
              MPI_Comm comm)
              MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,4) MPICH_ATTR_POINTER_WITH_TYPE_TAG(2,4) MPICH_API_PUBLIC;
int PMPI_Group_size(MPI_Group group, int *size) MPICH_API_PUBLIC;
int PMPI_Group_rank(MPI_Group group, int *rank) MPICH_API_PUBLIC;
int PMPI_Group_translate_ranks(MPI_Group group1, int n, const int ranks1[], MPI_Group group2,
                               int ranks2[]) MPICH_API_PUBLIC;
int PMPI_Group_compare(MPI_Group group1, MPI_Group group2, int *result) MPICH_API_PUBLIC;
int PMPI_Comm_group(MPI_Comm comm, MPI_Group *group) MPICH_API_PUBLIC;
int PMPI_Group_union(MPI_Group group1, MPI_Group group2, MPI_Group *newgroup) MPICH_API_PUBLIC;
int PMPI_Group_intersection(MPI_Group group1, MPI_Group group2, MPI_Group *newgroup) MPICH_API_PUBLIC;
int PMPI_Group_difference(MPI_Group group1, MPI_Group group2, MPI_Group *newgroup) MPICH_API_PUBLIC;
int PMPI_Group_incl(MPI_Group group, int n, const int ranks[], MPI_Group *newgroup) MPICH_API_PUBLIC;
int PMPI_Group_excl(MPI_Group group, int n, const int ranks[], MPI_Group *newgroup) MPICH_API_PUBLIC;
int PMPI_Group_range_incl(MPI_Group group, int n, int ranges[][3], MPI_Group *newgroup) MPICH_API_PUBLIC;
int PMPI_Group_range_excl(MPI_Group group, int n, int ranges[][3], MPI_Group *newgroup) MPICH_API_PUBLIC;
int PMPI_Group_free(MPI_Group *group) MPICH_API_PUBLIC;
int PMPI_Comm_size(MPI_Comm comm, int *size) MPICH_API_PUBLIC;
int PMPI_Comm_rank(MPI_Comm comm, int *rank) MPICH_API_PUBLIC;
int PMPI_Comm_compare(MPI_Comm comm1, MPI_Comm comm2, int *result) MPICH_API_PUBLIC;
int PMPI_Comm_dup(MPI_Comm comm, MPI_Comm *newcomm) MPICH_API_PUBLIC;
int PMPI_Comm_dup_with_info(MPI_Comm comm, MPI_Info info, MPI_Comm *newcomm) MPICH_API_PUBLIC;
int PMPI_Comm_create(MPI_Comm comm, MPI_Group group, MPI_Comm *newcomm) MPICH_API_PUBLIC;
int PMPI_Comm_split(MPI_Comm comm, int color, int key, MPI_Comm *newcomm) MPICH_API_PUBLIC;
int PMPI_Comm_free(MPI_Comm *comm) MPICH_API_PUBLIC;
int PMPI_Comm_test_inter(MPI_Comm comm, int *flag) MPICH_API_PUBLIC;
int PMPI_Comm_remote_size(MPI_Comm comm, int *size) MPICH_API_PUBLIC;
int PMPI_Comm_remote_group(MPI_Comm comm, MPI_Group *group) MPICH_API_PUBLIC;
int PMPI_Intercomm_create(MPI_Comm local_comm, int local_leader, MPI_Comm peer_comm,
                          int remote_leader, int tag, MPI_Comm *newintercomm) MPICH_API_PUBLIC;
int PMPI_Intercomm_merge(MPI_Comm intercomm, int high, MPI_Comm *newintracomm) MPICH_API_PUBLIC;
int PMPI_Keyval_create(MPI_Copy_function *copy_fn, MPI_Delete_function *delete_fn,
                       int *keyval, void *extra_state) MPICH_API_PUBLIC;
int PMPI_Keyval_free(int *keyval) MPICH_API_PUBLIC;
int PMPI_Attr_put(MPI_Comm comm, int keyval, void *attribute_val) MPICH_API_PUBLIC;
int PMPI_Attr_get(MPI_Comm comm, int keyval, void *attribute_val, int *flag) MPICH_API_PUBLIC;
int PMPI_Attr_delete(MPI_Comm comm, int keyval) MPICH_API_PUBLIC;
int PMPI_Topo_test(MPI_Comm comm, int *status) MPICH_API_PUBLIC;
int PMPI_Cart_create(MPI_Comm comm_old, int ndims, const int dims[], const int periods[],
                     int reorder, MPI_Comm *comm_cart) MPICH_API_PUBLIC;
int PMPI_Dims_create(int nnodes, int ndims, int dims[]) MPICH_API_PUBLIC;
int PMPI_Graph_create(MPI_Comm comm_old, int nnodes, const int indx[], const int edges[],
                      int reorder, MPI_Comm *comm_graph) MPICH_API_PUBLIC;
int PMPI_Graphdims_get(MPI_Comm comm, int *nnodes, int *nedges) MPICH_API_PUBLIC;
int PMPI_Graph_get(MPI_Comm comm, int maxindex, int maxedges, int indx[], int edges[]) MPICH_API_PUBLIC;
int PMPI_Cartdim_get(MPI_Comm comm, int *ndims) MPICH_API_PUBLIC;
int PMPI_Cart_get(MPI_Comm comm, int maxdims, int dims[], int periods[], int coords[]) MPICH_API_PUBLIC;
int PMPI_Cart_rank(MPI_Comm comm, const int coords[], int *rank) MPICH_API_PUBLIC;
int PMPI_Cart_coords(MPI_Comm comm, int rank, int maxdims, int coords[]) MPICH_API_PUBLIC;
int PMPI_Graph_neighbors_count(MPI_Comm comm, int rank, int *nneighbors) MPICH_API_PUBLIC;
int PMPI_Graph_neighbors(MPI_Comm comm, int rank, int maxneighbors, int neighbors[]) MPICH_API_PUBLIC;
int PMPI_Cart_shift(MPI_Comm comm, int direction, int disp, int *rank_source, int *rank_dest) MPICH_API_PUBLIC;
int PMPI_Cart_sub(MPI_Comm comm, const int remain_dims[], MPI_Comm *newcomm) MPICH_API_PUBLIC;
int PMPI_Cart_map(MPI_Comm comm, int ndims, const int dims[], const int periods[], int *newrank) MPICH_API_PUBLIC;
int PMPI_Graph_map(MPI_Comm comm, int nnodes, const int indx[], const int edges[], int *newrank) MPICH_API_PUBLIC;
int PMPI_Get_processor_name(char *name, int *resultlen) MPICH_API_PUBLIC;
int PMPI_Get_version(int *version, int *subversion) MPICH_API_PUBLIC;
int PMPI_Get_library_version(char *version, int *resultlen) MPICH_API_PUBLIC;
int PMPI_Errhandler_create(MPI_Handler_function *function, MPI_Errhandler *errhandler) MPICH_API_PUBLIC;
int PMPI_Errhandler_set(MPI_Comm comm, MPI_Errhandler errhandler) MPICH_API_PUBLIC;
int PMPI_Errhandler_get(MPI_Comm comm, MPI_Errhandler *errhandler) MPICH_API_PUBLIC;
int PMPI_Errhandler_free(MPI_Errhandler *errhandler) MPICH_API_PUBLIC;
int PMPI_Error_string(int errorcode, char *string, int *resultlen) MPICH_API_PUBLIC;
int PMPI_Error_class(int errorcode, int *errorclass) MPICH_API_PUBLIC;
double PMPI_Wtime(void) MPICH_API_PUBLIC;
double PMPI_Wtick(void) MPICH_API_PUBLIC;
int PMPI_Init(int *argc, char ***argv) MPICH_API_PUBLIC;
int PMPI_Finalize(void) MPICH_API_PUBLIC;
int PMPI_Initialized(int *flag) MPICH_API_PUBLIC;
int PMPI_Abort(MPI_Comm comm, int errorcode) MPICH_API_PUBLIC;

/* Note that we may need to define a @PCONTROL_LIST@ depending on whether
   stdargs are supported */
int PMPI_Pcontrol(const int level, ...) MPICH_API_PUBLIC;

/* Process Creation and Management */
int PMPI_Close_port(const char *port_name) MPICH_API_PUBLIC;
int PMPI_Comm_accept(const char *port_name, MPI_Info info, int root, MPI_Comm comm,
                     MPI_Comm *newcomm) MPICH_API_PUBLIC;
int PMPI_Comm_connect(const char *port_name, MPI_Info info, int root, MPI_Comm comm,
                      MPI_Comm *newcomm) MPICH_API_PUBLIC;
int PMPI_Comm_disconnect(MPI_Comm *comm) MPICH_API_PUBLIC;
int PMPI_Comm_get_parent(MPI_Comm *parent) MPICH_API_PUBLIC;
int PMPI_Comm_join(int fd, MPI_Comm *intercomm) MPICH_API_PUBLIC;
int PMPI_Comm_spawn(const char *command, char *argv[], int maxprocs, MPI_Info info, int root,
                    MPI_Comm comm, MPI_Comm *intercomm, int array_of_errcodes[]) MPICH_API_PUBLIC;
int PMPI_Comm_spawn_multiple(int count, char *array_of_commands[], char **array_of_argv[],
                             const int array_of_maxprocs[], const MPI_Info array_of_info[],
                             int root, MPI_Comm comm, MPI_Comm *intercomm, int array_of_errcodes[]) MPICH_API_PUBLIC;
int PMPI_Lookup_name(const char *service_name, MPI_Info info, char *port_name) MPICH_API_PUBLIC;
int PMPI_Open_port(MPI_Info info, char *port_name) MPICH_API_PUBLIC;
int PMPI_Publish_name(const char *service_name, MPI_Info info, const char *port_name) MPICH_API_PUBLIC;
int PMPI_Unpublish_name(const char *service_name, MPI_Info info, const char *port_name) MPICH_API_PUBLIC;
int PMPI_Comm_set_info(MPI_Comm comm, MPI_Info info) MPICH_API_PUBLIC;
int PMPI_Comm_get_info(MPI_Comm comm, MPI_Info *info) MPICH_API_PUBLIC;

/* One-Sided Communications */
int PMPI_Accumulate(const void *origin_addr, int origin_count, MPI_Datatype origin_datatype,
                    int target_rank, MPI_Aint target_disp, int target_count,
                    MPI_Datatype target_datatype, MPI_Op op, MPI_Win win)
                    MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3) MPICH_API_PUBLIC;
int PMPI_Get(void *origin_addr, int origin_count, MPI_Datatype origin_datatype,
             int target_rank, MPI_Aint target_disp, int target_count,
             MPI_Datatype target_datatype, MPI_Win win) MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3) MPICH_API_PUBLIC;
int PMPI_Put(const void *origin_addr, int origin_count, MPI_Datatype origin_datatype,
             int target_rank, MPI_Aint target_disp, int target_count,
             MPI_Datatype target_datatype, MPI_Win win) MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3) MPICH_API_PUBLIC;
int PMPI_Win_complete(MPI_Win win) MPICH_API_PUBLIC;
int PMPI_Win_create(void *base, MPI_Aint size, int disp_unit, MPI_Info info, MPI_Comm comm,
                    MPI_Win *win) MPICH_API_PUBLIC;
int PMPI_Win_fence(int assert, MPI_Win win) MPICH_API_PUBLIC;
int PMPI_Win_free(MPI_Win *win) MPICH_API_PUBLIC;
int PMPI_Win_get_group(MPI_Win win, MPI_Group *group) MPICH_API_PUBLIC;
int PMPI_Win_lock(int lock_type, int rank, int assert, MPI_Win win) MPICH_API_PUBLIC;
int PMPI_Win_post(MPI_Group group, int assert, MPI_Win win) MPICH_API_PUBLIC;
int PMPI_Win_start(MPI_Group group, int assert, MPI_Win win) MPICH_API_PUBLIC;
int PMPI_Win_test(MPI_Win win, int *flag) MPICH_API_PUBLIC;
int PMPI_Win_unlock(int rank, MPI_Win win) MPICH_API_PUBLIC;
int PMPI_Win_wait(MPI_Win win) MPICH_API_PUBLIC;

/* MPI-3 One-Sided Communication Routines */
int PMPI_Win_allocate(MPI_Aint size, int disp_unit, MPI_Info info, MPI_Comm comm, void *baseptr,
                      MPI_Win *win) MPICH_API_PUBLIC;
int PMPI_Win_allocate_shared(MPI_Aint size, int disp_unit, MPI_Info info, MPI_Comm comm,
                             void *baseptr, MPI_Win *win) MPICH_API_PUBLIC;
int PMPI_Win_shared_query(MPI_Win win, int rank, MPI_Aint *size, int *disp_unit, void *baseptr) MPICH_API_PUBLIC;
int PMPI_Win_create_dynamic(MPI_Info info, MPI_Comm comm, MPI_Win *win) MPICH_API_PUBLIC;
int PMPI_Win_attach(MPI_Win win, void *base, MPI_Aint size) MPICH_API_PUBLIC;
int PMPI_Win_detach(MPI_Win win, const void *base) MPICH_API_PUBLIC;
int PMPI_Win_get_info(MPI_Win win, MPI_Info *info_used) MPICH_API_PUBLIC;
int PMPI_Win_set_info(MPI_Win win, MPI_Info info) MPICH_API_PUBLIC;
int PMPI_Get_accumulate(const void *origin_addr, int origin_count,
                         MPI_Datatype origin_datatype, void *result_addr, int result_count,
                         MPI_Datatype result_datatype, int target_rank, MPI_Aint target_disp,
                         int target_count, MPI_Datatype target_datatype, MPI_Op op, MPI_Win win)
                         MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3)
                         MPICH_ATTR_POINTER_WITH_TYPE_TAG(4,6) MPICH_API_PUBLIC;
int PMPI_Fetch_and_op(const void *origin_addr, void *result_addr,
                       MPI_Datatype datatype, int target_rank, MPI_Aint target_disp,
                       MPI_Op op, MPI_Win win)
                       MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3) MPICH_API_PUBLIC;
int PMPI_Compare_and_swap(const void *origin_addr, const void *compare_addr,
                           void *result_addr, MPI_Datatype datatype, int target_rank,
                           MPI_Aint target_disp, MPI_Win win)
                           MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,4)
                           MPICH_ATTR_POINTER_WITH_TYPE_TAG(2,4)
                           MPICH_ATTR_POINTER_WITH_TYPE_TAG(3,4) MPICH_API_PUBLIC;
int PMPI_Rput(const void *origin_addr, int origin_count,
               MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp,
               int target_count, MPI_Datatype target_datatype, MPI_Win win,
               MPI_Request *request)
               MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3) MPICH_API_PUBLIC;
int PMPI_Rget(void *origin_addr, int origin_count,
               MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp,
               int target_count, MPI_Datatype target_datatype, MPI_Win win,
               MPI_Request *request)
               MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3) MPICH_API_PUBLIC;
int PMPI_Raccumulate(const void *origin_addr, int origin_count,
                      MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp,
                      int target_count, MPI_Datatype target_datatype, MPI_Op op, MPI_Win win,
                      MPI_Request *request)
                      MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3) MPICH_API_PUBLIC;
int PMPI_Rget_accumulate(const void *origin_addr, int origin_count,
                          MPI_Datatype origin_datatype, void *result_addr, int result_count,
                          MPI_Datatype result_datatype, int target_rank, MPI_Aint target_disp,
                          int target_count, MPI_Datatype target_datatype, MPI_Op op, MPI_Win win,
                          MPI_Request *request)
                          MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3)
                          MPICH_ATTR_POINTER_WITH_TYPE_TAG(4,6) MPICH_API_PUBLIC;
int PMPI_Win_lock_all(int assert, MPI_Win win) MPICH_API_PUBLIC;
int PMPI_Win_unlock_all(MPI_Win win) MPICH_API_PUBLIC;
int PMPI_Win_flush(int rank, MPI_Win win) MPICH_API_PUBLIC;
int PMPI_Win_flush_all(MPI_Win win) MPICH_API_PUBLIC;
int PMPI_Win_flush_local(int rank, MPI_Win win) MPICH_API_PUBLIC;
int PMPI_Win_flush_local_all(MPI_Win win) MPICH_API_PUBLIC;
int PMPI_Win_sync(MPI_Win win) MPICH_API_PUBLIC;

/* External Interfaces */
int PMPI_Add_error_class(int *errorclass) MPICH_API_PUBLIC;
int PMPIX_Delete_error_class(int errorclass) MPICH_API_PUBLIC;
int PMPI_Add_error_code(int errorclass, int *errorcode) MPICH_API_PUBLIC;
int PMPIX_Delete_error_code(int errorcode) MPICH_API_PUBLIC;
int PMPI_Add_error_string(int errorcode, const char *string) MPICH_API_PUBLIC;
int PMPIX_Delete_error_string(int errorcode) MPICH_API_PUBLIC;
int PMPI_Comm_call_errhandler(MPI_Comm comm, int errorcode) MPICH_API_PUBLIC;
int PMPI_Comm_create_keyval(MPI_Comm_copy_attr_function *comm_copy_attr_fn,
                            MPI_Comm_delete_attr_function *comm_delete_attr_fn, int *comm_keyval,
                            void *extra_state) MPICH_API_PUBLIC;
int PMPI_Comm_delete_attr(MPI_Comm comm, int comm_keyval) MPICH_API_PUBLIC;
int PMPI_Comm_free_keyval(int *comm_keyval) MPICH_API_PUBLIC;
int PMPI_Comm_get_attr(MPI_Comm comm, int comm_keyval, void *attribute_val, int *flag) MPICH_API_PUBLIC;
int PMPI_Comm_get_name(MPI_Comm comm, char *comm_name, int *resultlen) MPICH_API_PUBLIC;
int PMPI_Comm_set_attr(MPI_Comm comm, int comm_keyval, void *attribute_val) MPICH_API_PUBLIC;
int PMPI_Comm_set_name(MPI_Comm comm, const char *comm_name) MPICH_API_PUBLIC;
int PMPI_File_call_errhandler(MPI_File fh, int errorcode) MPICH_API_PUBLIC;
int PMPI_Grequest_complete(MPI_Request request) MPICH_API_PUBLIC;
int PMPI_Grequest_start(MPI_Grequest_query_function *query_fn, MPI_Grequest_free_function *free_fn,
                        MPI_Grequest_cancel_function *cancel_fn, void *extra_state,
                        MPI_Request *request) MPICH_API_PUBLIC;
int PMPI_Init_thread(int *argc, char ***argv, int required, int *provided) MPICH_API_PUBLIC;
int PMPI_Is_thread_main(int *flag) MPICH_API_PUBLIC;
int PMPI_Query_thread(int *provided) MPICH_API_PUBLIC;
int PMPI_Status_set_cancelled(MPI_Status *status, int flag) MPICH_API_PUBLIC;
int PMPI_Status_set_elements(MPI_Status *status, MPI_Datatype datatype, int count) MPICH_API_PUBLIC;
int PMPI_Type_create_keyval(MPI_Type_copy_attr_function *type_copy_attr_fn,
                            MPI_Type_delete_attr_function *type_delete_attr_fn,
                            int *type_keyval, void *extra_state) MPICH_API_PUBLIC;
int PMPI_Type_delete_attr(MPI_Datatype datatype, int type_keyval) MPICH_API_PUBLIC;
int PMPI_Type_dup(MPI_Datatype oldtype, MPI_Datatype *newtype) MPICH_API_PUBLIC;
int PMPI_Type_free_keyval(int *type_keyval) MPICH_API_PUBLIC;
int PMPI_Type_get_attr(MPI_Datatype datatype, int type_keyval, void *attribute_val, int *flag) MPICH_API_PUBLIC;
int PMPI_Type_get_contents(MPI_Datatype datatype, int max_integers, int max_addresses,
                           int max_datatypes, int array_of_integers[],
                           MPI_Aint array_of_addresses[], MPI_Datatype array_of_datatypes[]) MPICH_API_PUBLIC;
int PMPI_Type_get_envelope(MPI_Datatype datatype, int *num_integers, int *num_addresses,
                           int *num_datatypes, int *combiner) MPICH_API_PUBLIC;
int PMPI_Type_get_name(MPI_Datatype datatype, char *type_name, int *resultlen) MPICH_API_PUBLIC;
int PMPI_Type_set_attr(MPI_Datatype datatype, int type_keyval, void *attribute_val) MPICH_API_PUBLIC;
int PMPI_Type_set_name(MPI_Datatype datatype, const char *type_name) MPICH_API_PUBLIC;
int PMPI_Type_match_size(int typeclass, int size, MPI_Datatype *datatype) MPICH_API_PUBLIC;
int PMPI_Win_call_errhandler(MPI_Win win, int errorcode) MPICH_API_PUBLIC;
int PMPI_Win_create_keyval(MPI_Win_copy_attr_function *win_copy_attr_fn,
                           MPI_Win_delete_attr_function *win_delete_attr_fn, int *win_keyval,
                           void *extra_state) MPICH_API_PUBLIC;
int PMPI_Win_delete_attr(MPI_Win win, int win_keyval) MPICH_API_PUBLIC;
int PMPI_Win_free_keyval(int *win_keyval) MPICH_API_PUBLIC;
int PMPI_Win_get_attr(MPI_Win win, int win_keyval, void *attribute_val, int *flag) MPICH_API_PUBLIC;
int PMPI_Win_get_name(MPI_Win win, char *win_name, int *resultlen) MPICH_API_PUBLIC;
int PMPI_Win_set_attr(MPI_Win win, int win_keyval, void *attribute_val) MPICH_API_PUBLIC;
int PMPI_Win_set_name(MPI_Win win, const char *win_name) MPICH_API_PUBLIC;

int PMPI_Alloc_mem(MPI_Aint size, MPI_Info info, void *baseptr) MPICH_API_PUBLIC;
int PMPI_Comm_create_errhandler(MPI_Comm_errhandler_function *comm_errhandler_fn,
                                MPI_Errhandler *errhandler) MPICH_API_PUBLIC;
int PMPI_Comm_get_errhandler(MPI_Comm comm, MPI_Errhandler *errhandler) MPICH_API_PUBLIC;
int PMPI_Comm_set_errhandler(MPI_Comm comm, MPI_Errhandler errhandler) MPICH_API_PUBLIC;
int PMPI_File_create_errhandler(MPI_File_errhandler_function *file_errhandler_fn,
                                MPI_Errhandler *errhandler) MPICH_API_PUBLIC;
int PMPI_File_get_errhandler(MPI_File file, MPI_Errhandler *errhandler) MPICH_API_PUBLIC;
int PMPI_File_set_errhandler(MPI_File file, MPI_Errhandler errhandler) MPICH_API_PUBLIC;
int PMPI_Finalized(int *flag) MPICH_API_PUBLIC;
int PMPI_Free_mem(void *base) MPICH_API_PUBLIC;
int PMPI_Get_address(const void *location, MPI_Aint *address) MPICH_API_PUBLIC;
int PMPI_Info_create(MPI_Info *info) MPICH_API_PUBLIC;
int PMPI_Info_create_env(int *argc, char ***argv, MPI_Info *info) MPICH_API_PUBLIC;
int PMPI_Info_delete(MPI_Info info, const char *key) MPICH_API_PUBLIC;
int PMPI_Info_dup(MPI_Info info, MPI_Info *newinfo) MPICH_API_PUBLIC;
int PMPI_Info_free(MPI_Info *info) MPICH_API_PUBLIC;
int PMPI_Info_get(MPI_Info info, const char *key, int valuelen, char *value, int *flag) MPICH_API_PUBLIC;
int PMPI_Info_get_nkeys(MPI_Info info, int *nkeys) MPICH_API_PUBLIC;
int PMPI_Info_get_nthkey(MPI_Info info, int n, char *key) MPICH_API_PUBLIC;
int PMPI_Info_get_valuelen(MPI_Info info, const char *key, int *valuelen, int *flag) MPICH_API_PUBLIC;
int PMPI_Info_get_string(MPI_Info info, const char *key, int *buflen, char *value, int *flag) MPICH_API_PUBLIC;
int PMPI_Info_set(MPI_Info info, const char *key, const char *value) MPICH_API_PUBLIC;
int PMPI_Pack_external(const char datarep[], const void *inbuf, int incount,
                       MPI_Datatype datatype, void *outbuf, MPI_Aint outsize, MPI_Aint *position)
                       MPICH_ATTR_POINTER_WITH_TYPE_TAG(2,4) MPICH_API_PUBLIC;
int PMPI_Pack_external_size(const char datarep[], int incount, MPI_Datatype datatype,
                            MPI_Aint *size) MPICH_API_PUBLIC;
int PMPI_Request_get_status(MPI_Request request, int *flag, MPI_Status *status) MPICH_API_PUBLIC;
int PMPI_Status_c2f(const MPI_Status *c_status, MPI_Fint *f_status) MPICH_API_PUBLIC;
int PMPI_Status_f2c(const MPI_Fint *f_status, MPI_Status *c_status) MPICH_API_PUBLIC;
int PMPI_Type_create_darray(int size, int rank, int ndims, const int array_of_gsizes[],
                            const int array_of_distribs[], const int array_of_dargs[],
                            const int array_of_psizes[], int order, MPI_Datatype oldtype,
                            MPI_Datatype *newtype) MPICH_API_PUBLIC;
int PMPI_Type_create_hindexed(int count, const int array_of_blocklengths[],
                              const MPI_Aint array_of_displacements[], MPI_Datatype oldtype,
                              MPI_Datatype *newtype) MPICH_API_PUBLIC;
int PMPI_Type_create_hvector(int count, int blocklength, MPI_Aint stride, MPI_Datatype oldtype,
                             MPI_Datatype *newtype) MPICH_API_PUBLIC;
int PMPI_Type_create_indexed_block(int count, int blocklength, const int array_of_displacements[],
                                   MPI_Datatype oldtype, MPI_Datatype *newtype) MPICH_API_PUBLIC;
int PMPI_Type_create_hindexed_block(int count, int blocklength,
                                    const MPI_Aint array_of_displacements[],
                                    MPI_Datatype oldtype, MPI_Datatype *newtype) MPICH_API_PUBLIC;
int PMPI_Type_create_resized(MPI_Datatype oldtype, MPI_Aint lb, MPI_Aint extent,
                             MPI_Datatype *newtype) MPICH_API_PUBLIC;
int PMPI_Type_create_struct(int count, const int array_of_blocklengths[],
                            const MPI_Aint array_of_displacements[],
                            const MPI_Datatype array_of_types[], MPI_Datatype *newtype) MPICH_API_PUBLIC;
int PMPI_Type_create_subarray(int ndims, const int array_of_sizes[],
                              const int array_of_subsizes[], const int array_of_starts[],
                              int order, MPI_Datatype oldtype, MPI_Datatype *newtype) MPICH_API_PUBLIC;
int PMPI_Type_get_extent(MPI_Datatype datatype, MPI_Aint *lb, MPI_Aint *extent) MPICH_API_PUBLIC;
int PMPI_Type_get_true_extent(MPI_Datatype datatype, MPI_Aint *true_lb, MPI_Aint *true_extent) MPICH_API_PUBLIC;
int PMPI_Unpack_external(const char datarep[], const void *inbuf, MPI_Aint insize,
                         MPI_Aint *position, void *outbuf, int outcount, MPI_Datatype datatype)
                         MPICH_ATTR_POINTER_WITH_TYPE_TAG(5,7) MPICH_API_PUBLIC;
int PMPI_Win_create_errhandler(MPI_Win_errhandler_function *win_errhandler_fn,
                               MPI_Errhandler *errhandler) MPICH_API_PUBLIC;
int PMPI_Win_get_errhandler(MPI_Win win, MPI_Errhandler *errhandler) MPICH_API_PUBLIC;
int PMPI_Win_set_errhandler(MPI_Win win, MPI_Errhandler errhandler) MPICH_API_PUBLIC;

/* Fortran 90-related functions.  These routines are available only if
   Fortran 90 support is enabled
*/
int PMPI_Type_create_f90_integer(int r, MPI_Datatype *newtype) MPICH_API_PUBLIC;
int PMPI_Type_create_f90_real(int p, int r, MPI_Datatype *newtype) MPICH_API_PUBLIC;
int PMPI_Type_create_f90_complex(int p, int r, MPI_Datatype *newtype) MPICH_API_PUBLIC;

int PMPI_Reduce_local(const void *inbuf, void *inoutbuf, int count, MPI_Datatype datatype,
                      MPI_Op op)
                      MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,4) MPICH_ATTR_POINTER_WITH_TYPE_TAG(2,4) MPICH_API_PUBLIC;
int PMPI_Op_commutative(MPI_Op op, int *commute) MPICH_API_PUBLIC;
int PMPI_Reduce_scatter_block(const void *sendbuf, void *recvbuf, int recvcount,
                              MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
                              MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,4)
                              MPICH_ATTR_POINTER_WITH_TYPE_TAG(2,4) MPICH_API_PUBLIC;
int PMPI_Dist_graph_create_adjacent(MPI_Comm comm_old, int indegree, const int sources[],
                                    const int sourceweights[], int outdegree,
                                    const int destinations[], const int destweights[],
                                    MPI_Info info, int reorder, MPI_Comm *comm_dist_graph) MPICH_API_PUBLIC;
int PMPI_Dist_graph_create(MPI_Comm comm_old, int n, const int sources[], const int degrees[],
                           const int destinations[], const int weights[], MPI_Info info,
                           int reorder, MPI_Comm *comm_dist_graph) MPICH_API_PUBLIC;
int PMPI_Dist_graph_neighbors_count(MPI_Comm comm, int *indegree, int *outdegree, int *weighted) MPICH_API_PUBLIC;
int PMPI_Dist_graph_neighbors(MPI_Comm comm, int maxindegree, int sources[], int sourceweights[],
                              int maxoutdegree, int destinations[], int destweights[]) MPICH_API_PUBLIC;

/* Matched probe functionality */
int PMPI_Improbe(int source, int tag, MPI_Comm comm, int *flag, MPI_Message *message,
                 MPI_Status *status) MPICH_API_PUBLIC;
int PMPI_Imrecv(void *buf, int count, MPI_Datatype datatype, MPI_Message *message,
                MPI_Request *request) MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3) MPICH_API_PUBLIC;
int PMPI_Mprobe(int source, int tag, MPI_Comm comm, MPI_Message *message, MPI_Status *status) MPICH_API_PUBLIC;
int PMPI_Mrecv(void *buf, int count, MPI_Datatype datatype, MPI_Message *message,
               MPI_Status *status) MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3) MPICH_API_PUBLIC;

/* Nonblocking collectives */
int PMPI_Comm_idup(MPI_Comm comm, MPI_Comm *newcomm, MPI_Request *request) MPICH_API_PUBLIC;
int PMPI_Comm_idup_with_info(MPI_Comm comm, MPI_Info info, MPI_Comm *newcomm,
                             MPI_Request *request) MPICH_API_PUBLIC;
int PMPI_Ibarrier(MPI_Comm comm, MPI_Request *request) MPICH_API_PUBLIC;
int PMPI_Ibcast(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm,
                MPI_Request *request) MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3) MPICH_API_PUBLIC;
int PMPI_Igather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                 int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm,
                 MPI_Request *request)
                 MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3) MPICH_ATTR_POINTER_WITH_TYPE_TAG(4,6) MPICH_API_PUBLIC;
int PMPI_Igatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                  const int recvcounts[], const int displs[], MPI_Datatype recvtype, int root,
                  MPI_Comm comm, MPI_Request *request)
                  MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3) MPICH_ATTR_POINTER_WITH_TYPE_TAG(4,7) MPICH_API_PUBLIC;
int PMPI_Iscatter(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                  int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm,
                  MPI_Request *request)
                  MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3) MPICH_ATTR_POINTER_WITH_TYPE_TAG(4,6) MPICH_API_PUBLIC;
int PMPI_Iscatterv(const void *sendbuf, const int sendcounts[], const int displs[],
                   MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype,
                   int root, MPI_Comm comm, MPI_Request *request)
                   MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,4) MPICH_ATTR_POINTER_WITH_TYPE_TAG(5,7) MPICH_API_PUBLIC;
int PMPI_Iallgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                    int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request)
                    MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3) MPICH_ATTR_POINTER_WITH_TYPE_TAG(4,6) MPICH_API_PUBLIC;
int PMPI_Iallgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                     const int recvcounts[], const int displs[], MPI_Datatype recvtype,
                     MPI_Comm comm, MPI_Request *request)
                     MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3) MPICH_ATTR_POINTER_WITH_TYPE_TAG(4,7) MPICH_API_PUBLIC;
int PMPI_Ialltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                   int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request)
                   MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3) MPICH_ATTR_POINTER_WITH_TYPE_TAG(4,6) MPICH_API_PUBLIC;
int PMPI_Ialltoallv(const void *sendbuf, const int sendcounts[], const int sdispls[],
                    MPI_Datatype sendtype, void *recvbuf, const int recvcounts[],
                    const int rdispls[], MPI_Datatype recvtype, MPI_Comm comm,
                    MPI_Request *request)
                    MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,4) MPICH_ATTR_POINTER_WITH_TYPE_TAG(5,8) MPICH_API_PUBLIC;
int PMPI_Ialltoallw(const void *sendbuf, const int sendcounts[], const int sdispls[],
                    const MPI_Datatype sendtypes[], void *recvbuf, const int recvcounts[],
                    const int rdispls[], const MPI_Datatype recvtypes[], MPI_Comm comm,
                    MPI_Request *request) MPICH_API_PUBLIC;
int PMPI_Ireduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                 MPI_Op op, int root, MPI_Comm comm, MPI_Request *request)
                 MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,4) MPICH_ATTR_POINTER_WITH_TYPE_TAG(2,4) MPICH_API_PUBLIC;
int PMPI_Iallreduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                    MPI_Op op, MPI_Comm comm, MPI_Request *request)
                    MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,4) MPICH_ATTR_POINTER_WITH_TYPE_TAG(2,4) MPICH_API_PUBLIC;
int PMPI_Ireduce_scatter(const void *sendbuf, void *recvbuf, const int recvcounts[],
                         MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request)
                         MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,4)
                         MPICH_ATTR_POINTER_WITH_TYPE_TAG(2,4) MPICH_API_PUBLIC;
int PMPI_Ireduce_scatter_block(const void *sendbuf, void *recvbuf, int recvcount,
                               MPI_Datatype datatype, MPI_Op op, MPI_Comm comm,
                               MPI_Request *request)
                               MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,4)
                               MPICH_ATTR_POINTER_WITH_TYPE_TAG(2,4) MPICH_API_PUBLIC;
int PMPI_Iscan(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op,
               MPI_Comm comm, MPI_Request *request)
               MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,4) MPICH_ATTR_POINTER_WITH_TYPE_TAG(2,4) MPICH_API_PUBLIC;
int PMPI_Iexscan(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                 MPI_Op op, MPI_Comm comm, MPI_Request *request)
                 MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,4) MPICH_ATTR_POINTER_WITH_TYPE_TAG(2,4) MPICH_API_PUBLIC;

/* Neighborhood collectives */
int PMPI_Ineighbor_allgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                             void *recvbuf, int recvcount, MPI_Datatype recvtype,
                             MPI_Comm comm, MPI_Request *request)
                             MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3)
                             MPICH_ATTR_POINTER_WITH_TYPE_TAG(4,6) MPICH_API_PUBLIC;
int PMPI_Ineighbor_allgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                              void *recvbuf, const int recvcounts[], const int displs[],
                              MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request)
                              MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3)
                              MPICH_ATTR_POINTER_WITH_TYPE_TAG(4,7) MPICH_API_PUBLIC;
int PMPI_Ineighbor_alltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                            void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm,
                            MPI_Request *request)
                            MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3)
                            MPICH_ATTR_POINTER_WITH_TYPE_TAG(4,6) MPICH_API_PUBLIC;
int PMPI_Ineighbor_alltoallv(const void *sendbuf, const int sendcounts[], const int sdispls[],
                             MPI_Datatype sendtype, void *recvbuf, const int recvcounts[],
                             const int rdispls[], MPI_Datatype recvtype, MPI_Comm comm,
                             MPI_Request *request)
                             MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,4)
                             MPICH_ATTR_POINTER_WITH_TYPE_TAG(5,8) MPICH_API_PUBLIC;
int PMPI_Ineighbor_alltoallw(const void *sendbuf, const int sendcounts[],
                             const MPI_Aint sdispls[], const MPI_Datatype sendtypes[],
                             void *recvbuf, const int recvcounts[], const MPI_Aint rdispls[],
                             const MPI_Datatype recvtypes[], MPI_Comm comm, MPI_Request *request) MPICH_API_PUBLIC;
int PMPI_Neighbor_allgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                            void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm)
                            MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3)
                            MPICH_ATTR_POINTER_WITH_TYPE_TAG(4,6) MPICH_API_PUBLIC;
int PMPI_Neighbor_allgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                             void *recvbuf, const int recvcounts[], const int displs[],
                             MPI_Datatype recvtype, MPI_Comm comm)
                             MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3)
                             MPICH_ATTR_POINTER_WITH_TYPE_TAG(4,7) MPICH_API_PUBLIC;
int PMPI_Neighbor_alltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                           void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm)
                           MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,3)
                           MPICH_ATTR_POINTER_WITH_TYPE_TAG(4,6) MPICH_API_PUBLIC;
int PMPI_Neighbor_alltoallv(const void *sendbuf, const int sendcounts[], const int sdispls[],
                            MPI_Datatype sendtype, void *recvbuf, const int recvcounts[],
                            const int rdispls[], MPI_Datatype recvtype, MPI_Comm comm)
                            MPICH_ATTR_POINTER_WITH_TYPE_TAG(1,4)
                            MPICH_ATTR_POINTER_WITH_TYPE_TAG(5,8) MPICH_API_PUBLIC;
int PMPI_Neighbor_alltoallw(const void *sendbuf, const int sendcounts[], const MPI_Aint sdispls[],
                            const MPI_Datatype sendtypes[], void *recvbuf, const int recvcounts[],
                            const MPI_Aint rdispls[], const MPI_Datatype recvtypes[],
                            MPI_Comm comm) MPICH_API_PUBLIC;

/* Shared memory */
int PMPI_Comm_split_type(MPI_Comm comm, int split_type, int key, MPI_Info info, MPI_Comm *newcomm) MPICH_API_PUBLIC;

/* Noncollective communicator creation */
int PMPI_Comm_create_group(MPI_Comm comm, MPI_Group group, int tag, MPI_Comm *newcomm) MPICH_API_PUBLIC;

/* MPI-3 "large count" routines */
int PMPI_Get_elements_x(const MPI_Status *status, MPI_Datatype datatype, MPI_Count *count) MPICH_API_PUBLIC;
int PMPI_Status_set_elements_x(MPI_Status *status, MPI_Datatype datatype, MPI_Count count) MPICH_API_PUBLIC;
int PMPI_Type_get_extent_x(MPI_Datatype datatype, MPI_Count *lb, MPI_Count *extent) MPICH_API_PUBLIC;
int PMPI_Type_get_true_extent_x(MPI_Datatype datatype, MPI_Count *lb, MPI_Count *extent) MPICH_API_PUBLIC;
int PMPI_Type_size_x(MPI_Datatype datatype, MPI_Count *size) MPICH_API_PUBLIC;

/* MPI_Aint addressing arithmetic */
MPI_Aint PMPI_Aint_add(MPI_Aint base, MPI_Aint disp) MPICH_API_PUBLIC;
MPI_Aint PMPI_Aint_diff(MPI_Aint addr1, MPI_Aint addr2) MPICH_API_PUBLIC;

/* MPI_T interface */
/* The MPI_T routines are available only in C bindings - tell tools that they
   can skip these prototypes */
/* Begin Skip Prototypes */
int PMPI_T_init_thread(int required, int *provided) MPICH_API_PUBLIC;
int PMPI_T_finalize(void) MPICH_API_PUBLIC;
int PMPI_T_enum_get_info(MPI_T_enum enumtype, int *num, char *name, int *name_len) MPICH_API_PUBLIC;
int PMPI_T_enum_get_item(MPI_T_enum enumtype, int indx, int *value, char *name, int *name_len) MPICH_API_PUBLIC;
int PMPI_T_cvar_get_num(int *num_cvar) MPICH_API_PUBLIC;
int PMPI_T_cvar_get_info(int cvar_index, char *name, int *name_len, int *verbosity,
                         MPI_Datatype *datatype, MPI_T_enum *enumtype, char *desc, int *desc_len,
                         int *binding, int *scope) MPICH_API_PUBLIC;
int PMPI_T_cvar_handle_alloc(int cvar_index, void *obj_handle, MPI_T_cvar_handle *handle,
                             int *count) MPICH_API_PUBLIC;
int PMPI_T_cvar_handle_free(MPI_T_cvar_handle *handle) MPICH_API_PUBLIC;
int PMPI_T_cvar_read(MPI_T_cvar_handle handle, void *buf) MPICH_API_PUBLIC;
int PMPI_T_cvar_write(MPI_T_cvar_handle handle, const void *buf) MPICH_API_PUBLIC;
int PMPI_T_pvar_get_num(int *num_pvar) MPICH_API_PUBLIC;
int PMPI_T_pvar_get_info(int pvar_index, char *name, int *name_len, int *verbosity, int *var_class,
                         MPI_Datatype *datatype, MPI_T_enum *enumtype, char *desc, int *desc_len,
                         int *binding, int *readonly, int *continuous, int *atomic) MPICH_API_PUBLIC;
int PMPI_T_pvar_session_create(MPI_T_pvar_session *session) MPICH_API_PUBLIC;
int PMPI_T_pvar_session_free(MPI_T_pvar_session *session) MPICH_API_PUBLIC;
int PMPI_T_pvar_handle_alloc(MPI_T_pvar_session session, int pvar_index, void *obj_handle,
                             MPI_T_pvar_handle *handle, int *count) MPICH_API_PUBLIC;
int PMPI_T_pvar_handle_free(MPI_T_pvar_session session, MPI_T_pvar_handle *handle) MPICH_API_PUBLIC;
int PMPI_T_pvar_start(MPI_T_pvar_session session, MPI_T_pvar_handle handle) MPICH_API_PUBLIC;
int PMPI_T_pvar_stop(MPI_T_pvar_session session, MPI_T_pvar_handle handle) MPICH_API_PUBLIC;
int PMPI_T_pvar_read(MPI_T_pvar_session session, MPI_T_pvar_handle handle, void *buf) MPICH_API_PUBLIC;
int PMPI_T_pvar_write(MPI_T_pvar_session session, MPI_T_pvar_handle handle, const void *buf) MPICH_API_PUBLIC;
int PMPI_T_pvar_reset(MPI_T_pvar_session session, MPI_T_pvar_handle handle) MPICH_API_PUBLIC;
int PMPI_T_pvar_readreset(MPI_T_pvar_session session, MPI_T_pvar_handle handle, void *buf) MPICH_API_PUBLIC;
int PMPI_T_category_get_num(int *num_cat) MPICH_API_PUBLIC;
int PMPI_T_category_get_info(int cat_index, char *name, int *name_len, char *desc, int *desc_len,
                             int *num_cvars, int *num_pvars, int *num_categories) MPICH_API_PUBLIC;
int PMPI_T_category_get_cvars(int cat_index, int len, int indices[]) MPICH_API_PUBLIC;
int PMPI_T_category_get_pvars(int cat_index, int len, int indices[]) MPICH_API_PUBLIC;
int PMPI_T_category_get_categories(int cat_index, int len, int indices[]) MPICH_API_PUBLIC;
int PMPI_T_category_changed(int *stamp) MPICH_API_PUBLIC;
int PMPI_T_cvar_get_index(const char *name, int *cvar_index) MPICH_API_PUBLIC;
int PMPI_T_pvar_get_index(const char *name, int var_class, int *pvar_index) MPICH_API_PUBLIC;
int PMPI_T_category_get_index(const char *name, int *cat_index) MPICH_API_PUBLIC;
/* End Skip Prototypes */


/* Non-standard but public extensions to MPI */
/* Fault Tolerance Extensions */
int PMPIX_Comm_failure_ack(MPI_Comm comm) MPICH_API_PUBLIC;
int PMPIX_Comm_failure_get_acked(MPI_Comm comm, MPI_Group *failedgrp) MPICH_API_PUBLIC;
int PMPIX_Comm_revoke(MPI_Comm comm) MPICH_API_PUBLIC;
int PMPIX_Comm_shrink(MPI_Comm comm, MPI_Comm *newcomm) MPICH_API_PUBLIC;
int PMPIX_Comm_agree(MPI_Comm comm, int *flag) MPICH_API_PUBLIC;

int PMPIX_GPU_query_support(int gpu_type, int *is_supported) MPICH_API_PUBLIC;
int PMPIX_Query_cuda_support(void) MPICH_API_PUBLIC;

#endif /* MPI_BUILD_PROFILING */

int MPIX_Grequest_class_create(MPI_Grequest_query_function *query_fn,
                               MPI_Grequest_free_function *free_fn,
                               MPI_Grequest_cancel_function *cancel_fn,
                               MPIX_Grequest_poll_function *poll_fn,
                               MPIX_Grequest_wait_function *wait_fn,
                               MPIX_Grequest_class *greq_class) MPICH_API_PUBLIC;
int MPIX_Grequest_class_allocate(MPIX_Grequest_class greq_class, void *extra_state,
                                 MPI_Request *request) MPICH_API_PUBLIC;
int MPIX_Grequest_start(MPI_Grequest_query_function *query_fn,
                        MPI_Grequest_free_function *free_fn,
                        MPI_Grequest_cancel_function *cancel_fn,
                        MPIX_Grequest_poll_function *poll_fn,
                        MPIX_Grequest_wait_function *wait_fn, void *extra_state,
                        MPI_Request *request) MPICH_API_PUBLIC;

#if !defined(MPI_BUILD_PROFILING)
/* Generalized requests extensions */
int PMPIX_Grequest_class_create(MPI_Grequest_query_function *query_fn,
                                MPI_Grequest_free_function *free_fn,
                                MPI_Grequest_cancel_function *cancel_fn,
                                MPIX_Grequest_poll_function *poll_fn,
                                MPIX_Grequest_wait_function *wait_fn,
                                MPIX_Grequest_class *greq_class) MPICH_API_PUBLIC;
int PMPIX_Grequest_class_allocate(MPIX_Grequest_class greq_class, void *extra_state,
                                  MPI_Request *request) MPICH_API_PUBLIC;
int PMPIX_Grequest_start(MPI_Grequest_query_function *query_fn,
                         MPI_Grequest_free_function *free_fn,
                         MPI_Grequest_cancel_function *cancel_fn,
                         MPIX_Grequest_poll_function *poll_fn,
                         MPIX_Grequest_wait_function *wait_fn, void *extra_state,
                         MPI_Request *request) MPICH_API_PUBLIC;
#endif /* MPI_BUILD_PROFILING */

/* *INDENT-ON* */

#endif /* MPI_PROTO_H_INCLUDED */

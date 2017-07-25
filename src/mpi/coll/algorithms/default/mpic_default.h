#ifndef MPIC_DEFAULT_H_INCLUDED
#define MPIC_DEFAULT_H_INCLUDED

/* Allgather functions */
int MPIC_DEFAULT_Allgather_intra(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIC_DEFAULT_Allgather_inter(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIC_DEFAULT_Allgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);

/* Allgatherv functions */
int MPIC_DEFAULT_Allgatherv_intra(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIC_DEFAULT_Allgatherv_inter(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIC_DEFAULT_Allgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);

/* Allreduce functions */
int MPIC_DEFAULT_Allreduce_recursive_doubling( const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIC_DEFAULT_Allreduce_reduce_scatter_allgather( const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIC_DEFAULT_Allreduce_intra ( const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag );
int MPIC_DEFAULT_Allreduce_inter(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIC_DEFAULT_Allreduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);

/* Alltoall functions */
int MPIC_DEFAULT_Alltoall_intra(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIC_DEFAULT_Alltoall_inter(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIC_DEFAULT_Alltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);

/* Alltoallv functions */
int MPIC_DEFAULT_Alltoallv_inter(const void *sendbuf, const int *sendcounts, const int *sdispls, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *rdispls, MPI_Datatype recvtype, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIC_DEFAULT_Alltoallv_intra(const void *sendbuf, const int *sendcounts, const int *sdispls, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *rdispls, MPI_Datatype recvtype, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIC_DEFAULT_Alltoallv(const void *sendbuf, const int *sendcounts, const int *sdispls, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *rdispls, MPI_Datatype recvtype, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);

/* Alltoallw functions */
int MPIC_DEFAULT_Alltoallw_intra(const void *sendbuf, const int sendcounts[], const int sdispls[], const MPI_Datatype sendtypes[], void *recvbuf, const int recvcounts[], const int rdispls[], const MPI_Datatype recvtypes[], MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIC_DEFAULT_Alltoallw_inter(const void *sendbuf, const int sendcounts[], const int sdispls[], const MPI_Datatype sendtypes[], void *recvbuf, const int recvcounts[], const int rdispls[], const MPI_Datatype recvtypes[], MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIC_DEFAULT_Alltoallw(const void *sendbuf, const int sendcounts[], const int sdispls[], const MPI_Datatype sendtypes[], void *recvbuf, const int recvcounts[], const int rdispls[], const MPI_Datatype recvtypes[], MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);

/* Barrier functions */
int MPIC_DEFAULT_Barrier_recursive_doubling(MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIC_DEFAULT_Barrier_intra(MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIC_DEFAULT_Barrier_inter(MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIC_DEFAULT_Barrier(MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);

/* Bcast functions */
int MPIC_DEFAULT_Bcast_binomial(void *buffer, int count, MPI_Datatype datatype, int root, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int scatter_for_bcast(void *buffer ATTRIBUTE((unused)), int count ATTRIBUTE((unused)), MPI_Datatype datatype ATTRIBUTE((unused)), int root, MPIR_Comm * comm_ptr, int nbytes, void *tmp_buf, int is_contig, int is_homogeneous, MPIR_Errflag_t * errflag);
int MPIC_DEFAULT_Bcast_scatter_doubling_allgather(void *buffer, int count, MPI_Datatype datatype, int root, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIC_DEFAULT_Bcast_scatter_ring_allgather(void *buffer, int count, MPI_Datatype datatype, int root, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIC_DEFAULT_SMP_Bcast(void *buffer, int count, MPI_Datatype datatype, int root, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIC_DEFAULT_Bcast_intra(void *buffer, int count, MPI_Datatype datatype, int root, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIC_DEFAULT_Bcast_inter(void *buffer, int count, MPI_Datatype datatype, int root, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIC_DEFAULT_Bcast(void *buffer, int count, MPI_Datatype datatype, int root, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);

/* Gather functions */
int MPIC_DEFAULT_Gather_intra(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIC_DEFAULT_Gather_inter(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIC_DEFAULT_Gather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);

/* Gatherv functions */
int MPIC_DEFAULT_Gatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, int root, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);

/* ReduceScatter_block functions */
int MPIC_DEFAULT_Reduce_scatter_block_noncomm(const void *sendbuf, void *recvbuf, int recvcount, MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIC_DEFAULT_Reduce_scatter_block_intra(const void *sendbuf, void *recvbuf, int recvcount, MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIC_DEFAULT_Reduce_scatter_block_inter(const void *sendbuf, void *recvbuf, int recvcount, MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIC_DEFAULT_Reduce_scatter_block(const void *sendbuf, void *recvbuf, int recvcount, MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);

/* Reducescatter functions */
int MPIC_DEFAULT_Reduce_scatter_noncomm(const void *sendbuf, void *recvbuf, const int recvcounts[], MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIC_DEFAULT_Reduce_scatter_intra(const void *sendbuf, void *recvbuf, const int recvcounts[], MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIC_DEFAULT_Reduce_scatter_inter(const void *sendbuf, void *recvbuf, const int recvcounts[], MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIC_DEFAULT_Reduce_scatter(const void *sendbuf, void *recvbuf, const int recvcounts[], MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIC_DEFAULT_Reduce_scatter_noncomm(const void *sendbuf, void *recvbuf, const int recvcounts[], MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIC_DEFAULT_Reduce_scatter_inter(const void *sendbuf, void *recvbuf, const int recvcounts[], MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIC_DEFAULT_Reduce_scatter(const void *sendbuf, void *recvbuf, const int recvcounts[], MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);

/* Reduce functions */
int MPIC_DEFAULT_Reduce_binomial(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIC_DEFAULT_Reduce_redscat_gather(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIC_DEFAULT_Reduce_intra(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIC_DEFAULT_Reduce_inter(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIC_DEFAULT_Reduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);

/* Scatter functions */
int MPIC_DEFAULT_Scatter_intra(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIC_DEFAULT_Scatter_inter(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIC_DEFAULT_Scatter(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);

/* Scatterv functions */
int MPIC_DEFAULT_Scatterv(const void *sendbuf, const int *sendcounts, const int *displs, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);

/* Scan functions */
int MPIC_DEFAULT_Scan(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);

/* Exscan functions */
int MPIC_DEFAULT_Exscan (const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);

#endif /* MPIC_DEFAULT_H_INCLUDED */

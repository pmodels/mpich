/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file include/mpidi_coll_prototypes.h
 * \brief ???
 */

#ifndef _MPIDI_COLL_PROTOTYPES_
#define _MPIDI_COLL_PROTOTYPES_

#include "mpido_coll.h"

/* Bcast prototypes */
int MPIDO_Bcast_tree(void *, int, int, MPID_Comm *);
int MPIDO_Bcast_CCMI_tree(void *, int, int, MPID_Comm *);
int MPIDO_Bcast_CCMI_tree_dput(void *, int, int, MPID_Comm *);
int MPIDO_Bcast_binom_sync(void *, int, int, MPID_Comm *);
int MPIDO_Bcast_rect_sync(void *, int, int, MPID_Comm *);
int MPIDO_Bcast_binom_async(void *, int, int, MPID_Comm *);
int MPIDO_Bcast_rect_async(void *, int, int, MPID_Comm *);
int MPIDO_Bcast_scatter_gather(void *, int, int, MPID_Comm *);
int MPIDO_Bcast_flattree(void *, int, int, MPID_Comm *);
int MPIDO_Bcast_rect_dput(void *, int, int, MPID_Comm *);
int MPIDO_Bcast_rect_singleth(void *, int, int, MPID_Comm *);
int MPIDO_Bcast_binom_singleth(void *, int, int, MPID_Comm *);

/* Allreduce  prototypes */
int MPIDO_Allreduce_global_tree(void *, void *, int, DCMF_Dt, DCMF_Op,
				MPI_Datatype, MPID_Comm *);

int MPIDO_Allreduce_pipelined_tree(void *, void *, int, DCMF_Dt, DCMF_Op,
				   MPI_Datatype, MPID_Comm *);
int MPIDO_Allreduce_tree(void *, void *, int, DCMF_Dt, DCMF_Op,
			 MPI_Datatype, MPID_Comm *);
int MPIDO_Allreduce_binom(void *, void *, int, DCMF_Dt, DCMF_Op,
			  MPI_Datatype, MPID_Comm *);
int MPIDO_Allreduce_rect(void *, void *, int, DCMF_Dt, DCMF_Op,
			 MPI_Datatype, MPID_Comm *);
int MPIDO_Allreduce_rectring(void *, void *, int, DCMF_Dt, DCMF_Op,
			     MPI_Datatype, MPID_Comm *);
int MPIDO_Allreduce_async_binom(void *, void *, int, DCMF_Dt, DCMF_Op,
				MPI_Datatype, MPID_Comm *);
int MPIDO_Allreduce_async_rect(void *, void *, int, DCMF_Dt, DCMF_Op,
			       MPI_Datatype, MPID_Comm *);
int MPIDO_Allreduce_async_rectring(void *, void *, int, DCMF_Dt, DCMF_Op,
				   MPI_Datatype, MPID_Comm *);
int MPIDO_Allreduce_rring_dput_singleth(void *, void *, int, DCMF_Dt, DCMF_Op,
				   MPI_Datatype, MPID_Comm *);
int MPIDO_Allreduce_short_async_rect(void *, void *, int, DCMF_Dt, DCMF_Op,
				   MPI_Datatype, MPID_Comm *);
int MPIDO_Allreduce_short_async_binom(void *, void *, int, DCMF_Dt, DCMF_Op,
				   MPI_Datatype, MPID_Comm *);



/* Alltoall prototypes */
int MPIDO_Alltoall_torus(void *, int, MPI_Datatype,
			 void *, int, MPI_Datatype,
			 MPID_Comm *);
int MPIDO_Alltoall_simple(void *, int, MPI_Datatype,
			  void *, int, MPI_Datatype,
			  MPID_Comm *);

/* Alltoallw prototypes */

int MPIDO_Alltoallw(void *sendbuf, int *sendcounts, int *senddispls,
                     MPI_Datatype *sendtypes,
                     void *recvbuf, int *recvcounts, int *recvdispls,
                     MPI_Datatype *recvtypes,
                     MPID_Comm *comm_ptr);
int MPIDO_Alltoallw_torus(void *sendbuf, int *sendcounts, int *senddispls,
                     MPI_Datatype *sendtypes,
                     void *recvbuf, int *recvcounts, int *recvdispls,
                     MPI_Datatype *recvtypes,
                     MPID_Comm *comm_ptr);

/* Alltoallv prototypes */
int MPIDO_Alltoallv(void *sendbuf, int *sendcounts, int *senddispls,
                     MPI_Datatype sendtype,
                     void *recvbuf, int *recvcounts, int *recvdispls,
                     MPI_Datatype recvtype,
                     MPID_Comm *comm_ptr);
int MPIDO_Alltoallv_torus(void *sendbuf, int *sendcounts, int *senddispls,
                     MPI_Datatype sendtype,
                     void *recvbuf, int *recvcounts, int *recvdispls,
                     MPI_Datatype recvtype,
                     MPID_Comm *comm_ptr);
/* Allgather prototypes */
int MPIDO_Allgather_allreduce(void *,
			      int,
			      MPI_Datatype,
			      void *,
			      int,
			      MPI_Datatype,
			      MPI_Aint,
			      MPI_Aint,
			      size_t,
			      size_t,
			      MPID_Comm * comm);

int MPIDO_Allgather_alltoall(void *,
			     int,
			     MPI_Datatype,
			     void *,
			     int,
			     MPI_Datatype,
			     MPI_Aint,
			     MPI_Aint,
			     size_t,
			     size_t,
			     MPID_Comm * comm);

int MPIDO_Allgather_bcast(void *,
			  int,
			  MPI_Datatype,
			  void *,
			  int,
			  MPI_Datatype,
			  MPI_Aint,
			  MPI_Aint,
			  size_t,
			  size_t,
			  MPID_Comm * comm);

int MPIDO_Allgather_bcast_rect_async(void *sendbuf,
                                     int sendcount,
                                     MPI_Datatype sendtype,
                                     void *recvbuf,
                                     int recvcount,
                                     MPI_Datatype recvtype,
                                     MPI_Aint send_true_lb,
                                     MPI_Aint recv_true_lb,
                                     size_t send_size,
                                     size_t recv_size,
                                     MPID_Comm *comm_ptr);

int MPIDO_Allgather_bcast_binom_async(void *sendbuf,
                                      int sendcount,
                                      MPI_Datatype sendtype,
                                      void *recvbuf,
                                      int recvcount,
                                      MPI_Datatype recvtype,
                                      MPI_Aint send_true_lb,
                                      MPI_Aint recv_true_lb,
                                      size_t send_size,
                                      size_t recv_size,
                                      MPID_Comm *comm_ptr);


/* Allgatherv prototypes */
int MPIDO_Allgatherv_allreduce(void *,
			       int,
			       MPI_Datatype,
			       void *,
			       int *,
			       int,
			       int *,
			       MPI_Datatype,
			       MPI_Aint,
			       MPI_Aint,
			       size_t,
			       size_t,
			       MPID_Comm * comm);

int MPIDO_Allgatherv_alltoall(void *,
			      int,
			      MPI_Datatype,
			      void *,
			      int *,
			      int,
			      int *,
			      MPI_Datatype,
			      MPI_Aint,
			      MPI_Aint,
			      size_t,
			      size_t,
			      MPID_Comm * comm);

int MPIDO_Allgatherv_bcast_rect_async(void *,
				      int,
				      MPI_Datatype,
				      void *,
				      int *,
				      int,
				      int *,
				      MPI_Datatype,
				      MPI_Aint,
				      MPI_Aint,
				      size_t,
				      size_t,
				      MPID_Comm * comm);

int MPIDO_Allgatherv_bcast_binom_async(void *,
				       int,
				       MPI_Datatype,
				       void *,
				       int *,
				       int,
				       int *,
				       MPI_Datatype,
				       MPI_Aint,
				       MPI_Aint,
				       size_t,
				       size_t,
				       MPID_Comm * comm);

int MPIDO_Allgatherv_bcast(void *,
			   int,
			   MPI_Datatype,
			   void *,
			   int *,
			   int,
			   int *,
			   MPI_Datatype,
			   MPI_Aint,
			   MPI_Aint,
			   size_t,
			   size_t,
			   MPID_Comm * comm);

/* Gather prototypes */
int MPIDO_Gather_reduce(void *, int, MPI_Datatype,
			void *, int, MPI_Datatype,
			int root, MPID_Comm *);

/* Barrier prototypes */
int MPIDO_Barrier_gi(MPID_Comm *);
int MPIDO_Barrier_dcmf(MPID_Comm *);

/* Scatter  prototypes */
int MPIDO_Scatter_bcast(void *, int, MPI_Datatype,
			void *, int, MPI_Datatype,
			int, MPID_Comm *);

/* Scatterv prototypes */
int MPIDO_Scatterv_bcast(void *sendbuf, int *sendcounts, int *displs,
                         MPI_Datatype sendtype, void *recvbuf, int recvcount,
                         MPI_Datatype recvtype, int root, MPID_Comm *comm_ptr);
int MPIDO_Scatterv_alltoallv(void *sendbuf, int *sendcounts, int *displs,
                         MPI_Datatype sendtype, void *recvbuf, int recvcount,
                         MPI_Datatype recvtype, int root, MPID_Comm *comm_ptr);

/* Reduce prototypes */
int MPIDO_Reduce_rectring(void *, void *, int, DCMF_Dt, DCMF_Op, MPI_Datatype,
			  int, MPID_Comm *);

int MPIDO_Reduce_rect(void *, void *, int, DCMF_Dt, DCMF_Op, MPI_Datatype,
		      int, MPID_Comm *);

int MPIDO_Reduce_binom(void *, void *, int, DCMF_Dt, DCMF_Op, MPI_Datatype,
		       int, MPID_Comm *);

int MPIDO_Reduce_tree(void *, void *, int, DCMF_Dt, DCMF_Op, MPI_Datatype,
		      int, MPID_Comm *);

int MPIDO_Reduce_global_tree(void *, void *, int, DCMF_Dt, DCMF_Op, 
			     MPI_Datatype, int, MPID_Comm *);

#endif

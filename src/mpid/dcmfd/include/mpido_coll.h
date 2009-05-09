/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file include/mpido_coll.h
 * \brief Function prototypes for the optimized collective routines
 */

#ifndef MPICH_DCMF_MPIDO_COLL_H_INCLUDED
#define MPICH_DCMF_MPIDO_COLL_H_INCLUDED

#include "mpidimpl.h"

#define NOTTREEOP 1

typedef struct {
  int send_contig;
  int recv_contig;
  int recv_continuous;
  int largecount;
  int mediumcount;
} MPIDO_Coll_config;

/* Helpers */
int MPIDO_AllocateAlltoallBuffers(MPID_Comm * comm);

int MPIDI_ConvertMPItoDCMF(MPI_Op op,
                           DCMF_Op *dcmf_op,
                           MPI_Datatype datatype,
                           DCMF_Dt *dcmf_dt);

int MPIDI_IsTreeOp(MPI_Op op, MPI_Datatype datatype);

/* Alltoall */
int MPIDO_Alltoall(void *sendbuf,
                   int sendcount,
                   MPI_Datatype sendtype,
                   void *recvbuf,
                   int recvcount,
                   MPI_Datatype recvtype,
                   MPID_Comm *comm_ptr);


/* Alltoallv */
int MPIDO_Alltoallv(void *sendbuf,
                    int *sendcounts,
                    int *senddispls,
                    MPI_Datatype sendtype,
                    void *recvbuf,
                    int *recvcounts,
                    int *recvdispls,
                    MPI_Datatype recvtype,
                    MPID_Comm *comm_ptr);

/* Alltoallw */
int MPIDO_Alltoallw(void *sendbuf,
                    int *sendcounts,
                    int *senddispls,
                    MPI_Datatype *sendtypes,
                    void *recvbuf,
                    int *recvcounts,
                    int *recvdispls,
                    MPI_Datatype *recvtypes,
                    MPID_Comm *comm_ptr);
/* barrier */
int   MPIDO_Barrier(MPID_Comm *comm_ptr);


/* bcast */
int MPIDO_Bcast(void * buffer,
                int count,
                MPI_Datatype datatype,
                int root,
                MPID_Comm * comm_ptr);

int MPIDO_AsyncBcast(void * buffer,
                int count,
                MPI_Datatype datatype,
                int root,
                MPID_Comm * comm_ptr,
                int num_outstanding);

/* allreduce */
int MPIDO_Allreduce(void * sendbuf,
                    void * recvbuf,
                    int count,
                    MPI_Datatype datatype,
                    MPI_Op op,
                    MPID_Comm * comm_ptr);

/* reduce */
int MPIDO_Reduce(void * sendbuf,
                 void * recvbuf,
                 int count,
                 MPI_Datatype datatype,
                 MPI_Op op,
                 int root,
                 MPID_Comm * comm_ptr);

/* allgather */
int MPIDO_Allgather(void *sendbuf,
                    int sendcount,
                    MPI_Datatype sendtype,
                    void *recvbuf,
                    int recvcount,
                    MPI_Datatype recvtype,
                    MPID_Comm * comm_ptr);

/* allgather */
int MPIDO_Allgatherv(void *sendbuf,
                     int sendcount,
                     MPI_Datatype sendtype,
                     void *recvbuf,
                     int *recvcounts,
                     int *displs,
                     MPI_Datatype recvtype,
                     MPID_Comm * comm_ptr);

/* these aren't optimized, but are included for completeness */
int MPIDO_Gather(void *sendbuf,
                 int sendcount,
                 MPI_Datatype sendtype,
                 void *recvbuf,
                 int recvcount,
                 MPI_Datatype recvtype,
                 int root,
                 MPID_Comm * comm_ptr);

int MPIDO_Gatherv(void *sendbuf,
                  int sendcount,
                  MPI_Datatype sendtype,
                  void *recvbuf,
                  int *recvcounts,
                  int *displs,
                  MPI_Datatype recvtype,
                  int root,
                  MPID_Comm * comm_ptr);

int MPIDO_Scatter(void *sendbuf,
                  int sendcount,
                  MPI_Datatype sendtype,
                  void *recvbuf,
                  int recvcount,
                  MPI_Datatype recvtype,
                  int root,
                  MPID_Comm * comm_ptr);

int MPIDO_Scatterv(void *sendbuf,
                   int *sendcounts,
                   int *displs,
                   MPI_Datatype sendtype,
                   void *recvbuf,
                   int recvcount,
                   MPI_Datatype recvtype,
                   int root,
                   MPID_Comm * comm_ptr);

int MPIDO_Reduce_scatter(void *sendbuf,
                         void *recvbuf,
                         int *recvcounts,
                         MPI_Datatype datatype,
                         MPI_Op op,
                         MPID_Comm * comm_ptr);
int MPIDO_Scan(void *sendbuf,
               void *recvbuf,
               int count,
               MPI_Datatype datatype,
               MPI_Op op,
               MPID_Comm * comm_ptr);

int MPIDO_Exscan(void *sendbuf,
                 void *recvbuf,
                 int count,
                 MPI_Datatype datatype,
                 MPI_Op op,
                 MPID_Comm * comm_ptr);



#endif

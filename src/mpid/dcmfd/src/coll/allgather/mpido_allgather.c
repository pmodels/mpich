/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file src/coll/allgather/mpido_allgather.c
 * \brief ???
 */

#include "mpido_coll.h"

#pragma weak PMPIDO_Allgather = MPIDO_Allgather


/* ****************************************************************** */
/**
* \brief Use (tree) MPIDO_Allreduce() to do a fast Allgather operation
*
* \note This function requires that:
*       - The send/recv data types are contiguous
*       - Tree allreduce is availible (for max performance)
*       - The datatype parameters needed added to the function signature
*/
/* ****************************************************************** */
int MPIDO_Allgather_Allreduce(void *sendbuf,
                              int sendcount,
                              MPI_Datatype sendtype,
                              void *recvbuf,
                              int recvcount,
                              MPI_Datatype recvtype,
                              MPID_Comm * comm_ptr,
                              MPI_Aint send_true_lb,
                              MPI_Aint recv_true_lb,
                              size_t send_size,
                              size_t recv_size)

{
   char *startbuf = NULL;
   char *destbuf = NULL;
   startbuf   = (char *) recvbuf + recv_true_lb;
   destbuf    = startbuf + comm_ptr->rank * send_size;
   
   memset(startbuf, 0, comm_ptr->rank * send_size);
   memset(destbuf + send_size, 0, recv_size - (comm_ptr->rank + 1) * send_size);
   if (sendbuf != MPI_IN_PLACE)
   {
      char *outputbuf = (char *) sendbuf + send_true_lb;
      memcpy(destbuf, outputbuf, send_size);
   }
   
   return MPIDO_Allreduce(MPI_IN_PLACE,
                          startbuf,
                          recv_size/4,
                          MPI_INT,
                          MPI_BOR,
                          comm_ptr);
}


/* ****************************************************************** */
/**
* \brief Use (tree/rect) MPIDO_Bcast() to do a fast Allgather operation
*
* \note This function requires one of these (for max performance):
*       - Tree broadcast
*/
/* ****************************************************************** */
int MPIDO_Allgather_Bcast(void *sendbuf,
                          int sendcount,
                          MPI_Datatype sendtype,
                          void *recvbuf,
                          int recvcount,
                          MPI_Datatype recvtype,
                          MPID_Comm * comm_ptr)
{
   int i;
   MPI_Aint extent;
   MPID_Datatype_get_extent_macro(recvtype, extent);
   if (sendbuf != MPI_IN_PLACE)
   {
      void *destbuf = recvbuf + comm_ptr->rank * recvcount * extent;
      MPIR_Localcopy(sendbuf,
                     sendcount,
                     sendtype,
                     destbuf,
                     recvcount,
                     recvtype);
   }

   for (i = 0; i < comm_ptr->local_size; i++)
   {
      void *destbuf = recvbuf + i * recvcount * extent;
      MPIDO_Bcast(destbuf,
                  recvcount,
                  recvtype,
                  i,
                  comm_ptr);
   }

   return MPI_SUCCESS;
}
   
/* ****************************************************************** */
/**
* \brief Use (tree/rect) MPIDO_Alltoall() to do a fast Allgather operation
*
* \note This function requires that:
*       - The send/recv data types are contiguous
*       - DMA alltoallv is availible (for max performance)
*       - The datatype parameters needed added to the function signature
*/
/* ****************************************************************** */
int MPIDO_Allgather_Alltoall(void *sendbuf,
                             int sendcount,
                             MPI_Datatype sendtype,
                             void *recvbuf,
                             int recvcount,
                             MPI_Datatype recvtype,
                             MPID_Comm * comm_ptr,
                             MPI_Aint send_true_lb,
                             MPI_Aint recv_true_lb,
                             size_t send_size,
                             size_t recv_size)
{
   int i;
   void *a2a_sendbuf = NULL;
   char *destbuf=NULL;
   char *startbuf=NULL;

   int a2a_sendcounts[comm_ptr->local_size];
   int a2a_senddispls[comm_ptr->local_size];
   int a2a_recvcounts[comm_ptr->local_size];
   int a2a_recvdispls[comm_ptr->local_size];

   for (i = 0; i < comm_ptr->local_size; ++i)
   {
      a2a_sendcounts[i] = send_size;
      a2a_senddispls[i] = 0;
      a2a_recvcounts[i] = recvcount;
      a2a_recvdispls[i] = recvcount * i;
   }
   if (sendbuf != MPI_IN_PLACE)
   {
      a2a_sendbuf = sendbuf + send_true_lb;
   }
   else
   {
      startbuf = (char *) recvbuf + recv_true_lb;
      destbuf = startbuf + comm_ptr->rank * send_size;
      a2a_sendbuf = destbuf;
      a2a_sendcounts[comm_ptr->rank] = 0;

      a2a_recvcounts[comm_ptr->rank] = 0;
   }

   return MPIDO_Alltoallv(a2a_sendbuf,
                          a2a_sendcounts,
                          a2a_senddispls,
                          MPI_CHAR,
                          recvbuf,
                          a2a_recvcounts,
                          a2a_recvdispls,
                          recvtype,
                          comm_ptr);
}


int
MPIDO_Allgather(void *sendbuf,
               int sendcount,
               MPI_Datatype sendtype,
               void *recvbuf,
               int recvcount,
               MPI_Datatype recvtype,
               MPID_Comm * comm_ptr)
{
  /* *********************************
   * Check the nature of the buffers
   * *********************************
   */

  MPIDO_Coll_config config = {1,1,1};
  MPID_Datatype *dt_null = NULL;
  MPI_Aint send_true_lb = 0;
  MPI_Aint recv_true_lb = 0;
  size_t send_size = 0;
  size_t recv_size = 0;


  int      result        = MPI_SUCCESS;

   /* no optimized allgather, punt to mpich */
  if(MPIDI_CollectiveProtocols.optallgather &&
      (MPIDI_CollectiveProtocols.allgather.useallreduce ||
       MPIDI_CollectiveProtocols.allgather.usebcast ||
       MPIDI_CollectiveProtocols.allgather.usealltoallv) == 0)
   {
      return MPIR_Allgather(sendbuf,
                            sendcount,
                            sendtype,
                            recvbuf,
                            recvcount,
                            recvtype,
                            comm_ptr);
   }


  MPIDI_Datatype_get_info(recvcount,
                          recvtype,
                          config.recv_contig,
                          recv_size,
                          dt_null,
                          recv_true_lb);
  send_size = recv_size;
  recv_size *= comm_ptr->local_size;
  if (sendbuf != MPI_IN_PLACE)
      MPIDI_Datatype_get_info(sendcount,
                            sendtype,
                            config.send_contig,
                            send_size,
                            dt_null,
                            send_true_lb);

   /* verify everyone's datatype contiguity */
  MPIDO_Allreduce(MPI_IN_PLACE,
                  &config,
                  3,
                  MPI_INT,
                  MPI_BAND,
                  comm_ptr);

   /* determine which protocol to use */
   /* 1) Tree allreduce
    *    a) Need tree allreduce for this communicator, otherwise it is silly
    *    b) User must be ok with allgather via allreduce
    *    c) Datatypes must be continguous
    *    d) Count must be a multiple of 4 since tree doesn't support
    *    chars right now
    */
   char treereduce = comm_ptr->dcmf.allreducetree &&
                     MPIDI_CollectiveProtocols.allgather.useallreduce &&
                     config.recv_contig && config.send_contig && 
                     config.recv_continuous && recv_size % 4 ==0;

   /* 2) Tree bcast
    *    a) Need tree bcast for this communicator, otherwise performance sucks
    *    b) User must be ok with allgather via bcast
    */
   char treebcast = comm_ptr->dcmf.bcasttree &&
                     MPIDI_CollectiveProtocols.allgather.usebcast;

   /* 3) Alltoall
    *    a) Need torus alltoall for this communicator
    *    b) User must be ok with allgather via alltoall
    *    c) Need contiguous datatypes
    */
   char usealltoall = comm_ptr->dcmf.alltoalls &&
                     MPIDI_CollectiveProtocols.allgather.usealltoallv &&
                     config.recv_contig && config.send_contig;


   /* Benchmark data shows bcast is faster for larger messages, so if
    * both bcast and reduce are available, use bcast >32768
    */
   if(treereduce && treebcast && recvcount > 32768)
      result = MPIDO_Allgather_Bcast(sendbuf,
                                     sendcount,
                                     sendtype,
                                     recvbuf,
                                     recvcount,
                                     recvtype,
                                     comm_ptr);
   else if(treereduce && treebcast && recvcount <= 32768)
      result = MPIDO_Allgather_Allreduce(sendbuf,
                                         sendcount,
                                         sendtype,
                                         recvbuf,
                                         recvcount,
                                         recvtype,
                                         comm_ptr,
                                         send_true_lb,
                                         recv_true_lb,
                                         send_size,
                                         recv_size);
   /* we only can use allreduce, so use it regardless of size */
   else if(treereduce)
      result = MPIDO_Allgather_Allreduce(sendbuf,
                                         sendcount,
                                         sendtype,
                                         recvbuf,
                                         recvcount,
                                         recvtype,
                                         comm_ptr,
                                         send_true_lb,
                                         recv_true_lb,
                                         send_size,
                                         recv_size);
   /* or, we can only use bcast, so use it regardless of size */
   else if(treebcast)
      result = MPIDO_Allgather_Bcast(sendbuf,
                                     sendcount,
                                     sendtype,
                                     recvbuf,
                                     recvcount,
                                     recvtype,
                                     comm_ptr);
   /* no tree protocols (probably not comm_world) so use alltoall */
   else if(usealltoall)
      result = MPIDO_Allgather_Alltoall(sendbuf,
                                        sendcount,
                                        sendtype,
                                        recvbuf,
                                        recvcount,
                                        recvtype,
                                        comm_ptr,
                                        send_true_lb,
                                        recv_true_lb,
                                        send_size,
                                        recv_size);
   /* don't even have alltoall, so use mpich */
   else 
      result = MPIR_Allgather(sendbuf,
                              sendcount,
                              sendtype,
                              recvbuf,
                              recvcount,
                              recvtype,
                              comm_ptr);

   return result;
}

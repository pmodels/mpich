/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file src/coll/scatterv/mpido_scatterv.c
 * \brief ???
 */

#include "mpido_coll.h"

#pragma weak PMPIDO_Scatterv = MPIDO_Scatterv

/* this guy requires quite a few buffers. maybe
 * we should somehow "steal" the comm_ptr alltoall ones? */
int MPIDO_Scatterv_alltoallv(void * sendbuf,
                             int * sendcounts,
                             int * displs,
                             MPI_Datatype sendtype,
                             void * recvbuf,
                             int recvcount,
                             MPI_Datatype recvtype,
                             int root,
                             MPID_Comm * comm_ptr)
{
   int rank = comm_ptr->rank;
   int size = comm_ptr->local_size;

   int *sdispls, *scounts;
   int *rdispls, *rcounts;
   char *sbuf, *rbuf;
   int contig, rbytes, sbytes;

   MPID_Datatype *dt_ptr;
   MPI_Aint dt_lb=0;

   MPIDI_Datatype_get_info(recvcount,
                           recvtype,
                           contig,
                           rbytes,
                           dt_ptr,
                           dt_lb);

   if(rank == root)
      MPIDI_Datatype_get_info(1, sendtype, contig, sbytes, dt_ptr, dt_lb);

   rbuf = MPIU_Malloc(size * rbytes * sizeof(char));
   if(!rbuf)
   {
      return MPIR_Err_create_code(MPI_SUCCESS,
                                  MPIR_ERR_RECOVERABLE,
                                  "MPI_Scatterv",
                                  __LINE__, MPI_ERR_OTHER, "**nomem", 0);
   }
//   memset(rbuf, 0, rbytes * size * sizeof(char));

   if(rank == root)
   {
      sdispls = displs;
      scounts = sendcounts;
      sbuf = sendbuf;
   }
   else
   {
      sdispls = MPIU_Malloc(size * sizeof(int));
      scounts = MPIU_Malloc(size * sizeof(int));
      sbuf = MPIU_Malloc(rbytes * sizeof(char));
      if(!sdispls || !scounts || !sbuf)
      {
         if(sdispls)
            MPIU_Free(sdispls);
         if(scounts)
            MPIU_Free(scounts);
         return MPIR_Err_create_code(MPI_SUCCESS,
                                     MPIR_ERR_RECOVERABLE,
                                     "MPI_Scatterv",
                                     __LINE__, MPI_ERR_OTHER, "**nomem", 0);
      }
      memset(sdispls, 0, size*sizeof(int));
      memset(scounts, 0, size*sizeof(int));
//      memset(sbuf, 0, rbytes * sizeof(char));
   }

   rdispls = MPIU_Malloc(size * sizeof(int));
   rcounts = MPIU_Malloc(size * sizeof(int));
   if(!rdispls || !rcounts)
   {
      if(rdispls)
         MPIU_Free(rdispls);
      return MPIR_Err_create_code(MPI_SUCCESS,
                                  MPIR_ERR_RECOVERABLE,
                                  "MPI_Scatterv",
                                  __LINE__, MPI_ERR_OTHER, "**nomem", 0);
   }

   memset(rdispls, 0, size*sizeof(unsigned));
   memset(rcounts, 0, size*sizeof(unsigned));

   rcounts[root] = rbytes;

   MPIDO_Alltoallv(sbuf,
                   scounts,
                   sdispls,
                   sendtype,
                   rbuf,
                   rcounts,
                   rdispls,
                   MPI_CHAR,
                   comm_ptr);

   if(rank == root && recvbuf == MPI_IN_PLACE)
   {
      MPIU_Free(rbuf);
      MPIU_Free(rdispls);
      MPIU_Free(rcounts);
      return MPI_SUCCESS;
   }
   else
   {
//      memcpy(recvbuf, rbuf+(root*rbytes), rbytes);
      memcpy(recvbuf, rbuf, rbytes);
      MPIU_Free(rbuf);
      MPIU_Free(rdispls);
      MPIU_Free(rcounts);
      if(rank != root)
      {
         MPIU_Free(sbuf);
         MPIU_Free(sdispls);
         MPIU_Free(scounts);
      }
   }

   return MPI_SUCCESS;
}

      

int MPIDO_Scatterv(void *sendbuf,
                   int *sendcounts, 
                   int *displs,
                   MPI_Datatype sendtype,
                   void *recvbuf,
                   int recvcount,
                   MPI_Datatype recvtype,
                   int root,
                   MPID_Comm *comm_ptr)
{
   int rank = comm_ptr->rank;
   int nbytes;
   MPID_Datatype *dt_ptr;
   MPI_Aint true_lb=0; 
   int contig;

   int optscatterv = (MPIDI_CollectiveProtocols.scatterv.usealltoallv &&
                      comm_ptr->dcmf.alltoalls);

   if(comm_ptr->comm_kind != MPID_INTRACOMM || !optscatterv)
      return MPIR_Scatterv(sendbuf, sendcounts, displs, sendtype,
                           recvbuf, recvcount, recvtype,
                           root, comm_ptr);

   if(rank == root)
   {
      MPIDI_Datatype_get_info(1,
                              sendtype,
                              contig,
                              nbytes,
                              dt_ptr,
                              true_lb);
      if(recvtype == MPI_DATATYPE_NULL || recvcount <=0 || !contig)
         optscatterv = 0;
   }
   else
   {
      MPIDI_Datatype_get_info(1,
                              recvtype,
                              contig,
                              nbytes,
                              dt_ptr,
                              true_lb);
      if(sendtype == MPI_DATATYPE_NULL || !contig)
         optscatterv = 0;
   }

   /* Make sure parameters are the same on all the nodes */
   /* specifically, noncontig on the receive */
   MPIDO_Allreduce(MPI_IN_PLACE,
                   &optscatterv,
                   1,
                   MPI_INT,
                   MPI_BAND,
                   comm_ptr);


   if(optscatterv)
   {
      if(rank == root)
      {
         MPID_Ensure_Aint_fits_in_pointer(MPI_VOID_PTR_CAST_TO_MPI_AINT sendbuf
                                          + true_lb);
         sendbuf = (char *)sendbuf + true_lb;
      }
      else
      {
         if(recvbuf != MPI_IN_PLACE)
         {
            MPID_Ensure_Aint_fits_in_pointer(MPI_VOID_PTR_CAST_TO_MPI_AINT recvbuf
                                          + true_lb);
            recvbuf = (char *)recvbuf + true_lb;
         }
      }
      return MPIDO_Scatterv_alltoallv(sendbuf,
                                      sendcounts,
                                      displs,
                                      sendtype,
                                      recvbuf,
                                      recvcount,
                                      recvtype,
                                      root,
                                      comm_ptr);
   }
   else
   {
      return MPIR_Scatterv(sendbuf, sendcounts, displs, sendtype,
                           recvbuf, recvcount, recvtype,
                           root, comm_ptr);
   }
}
      

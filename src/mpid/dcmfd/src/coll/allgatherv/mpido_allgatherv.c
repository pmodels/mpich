/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file src/coll/allgatherv/mpido_allgatherv.c
 * \brief ???
 */

#include "mpido_coll.h"

#pragma weak PMPIDO_Allgatherv = MPIDO_Allgatherv

/* \brief Callback for async bcast. MPIDO_Bcast call wouldn't be appropriate
 * here, so we just use call DCMF_AsyncBroadcast directly
 */

static void async_done (void *clientdata)
{
   volatile unsigned *work_left = (unsigned *)clientdata;
   (*work_left)--;
   MPID_Progress_signal();
   return;
}


/* ****************************************************************** */
/**
* \brief Use (tree) MPIDO_Allreduce() to do a fast Allgatherv operation
*
* \note This function requires that:
*       - The send/recv data types are contiguous
*       - The recv buffer is continuous
*       - Tree allreduce is availible (for max performance)
*/
/* ****************************************************************** */
int MPIDO_Allgatherv_Allreduce(void *sendbuf,
                               int sendcount,
                               MPI_Datatype sendtype,
                               void *recvbuf,
                               int *recvcounts,
                               int *displs,
                               MPI_Datatype recvtype,
                               MPID_Comm * comm_ptr,
                               MPI_Aint send_true_lb,
                               MPI_Aint recv_true_lb,
                               size_t send_size,
                               size_t recv_size,
                               int buffer_sum)
{
   int start;
   int length;
   char *startbuf = NULL;
   char *destbuf = NULL;

   startbuf = (char *) recvbuf + recv_true_lb;
   destbuf = startbuf + displs[comm_ptr->rank] * recv_size;

   start = 0;
   length = displs[comm_ptr->rank] * recv_size;
   memset(startbuf + start, 0, length);

   start  = (displs[comm_ptr->rank] + 
                   recvcounts[comm_ptr->rank]) * recv_size;
   length = buffer_sum - (displs[comm_ptr->rank] + 
                   recvcounts[comm_ptr->rank]) * recv_size;
   memset(startbuf + start, 0, length);

   if (sendbuf != MPI_IN_PLACE)
   {
      char *outputbuf = (char *) sendbuf + send_true_lb;
      memcpy(destbuf, outputbuf, send_size);
   }

   //if (0==comm_ptr->rank) puts("allreduce allgatherv");
   return MPIDO_Allreduce(MPI_IN_PLACE,
                          startbuf,
                          buffer_sum/4,
                          MPI_INT,
                          MPI_BOR,
                          comm_ptr);
}


int MPIDO_Allgatherv_Async_bcast(void *sendbuf,
                                 int sendcount,
                                 MPI_Datatype sendtype,
                                 void *recvbuf,
                                 int *recvcounts,
                                 int *displs,
                                 MPI_Datatype recvtype,
                                 MPID_Comm *comm_ptr,
                                 DCMF_CollectiveProtocol_t *protocol)
{
   int i, j, max_size, left_over;
   int dt_contig, data_sz;
   MPID_Datatype *dt_ptr;
   MPI_Aint extent, dt_true_lb;
   volatile unsigned active = 0;
   
   int numrequests = MPIDI_CollectiveProtocols.numrequests;
   DCMF_CollectiveRequest_t *requests;
   DCMF_Callback_t callback = {async_done, (void *)&active};
   
   void *destbuf;
   
   MPID_Datatype_get_extent_macro(recvtype, extent);
   MPIDI_Datatype_get_info(1,
   recvtype,
   dt_contig,
   data_sz,
   dt_ptr,
   dt_true_lb);
      
   MPID_Ensure_Aint_fits_in_pointer (
   (MPI_VOID_PTR_CAST_TO_MPI_AINT recvbuf +
   displs[comm_ptr->rank] * extent));
   
   if(sendbuf != MPI_IN_PLACE)
   {
      MPIR_Localcopy(sendbuf,
      sendcount,
      sendtype,
      recvbuf + displs[comm_ptr->rank] * extent,
      recvcounts[comm_ptr->rank],
      recvtype);
   }
   
   requests = (DCMF_CollectiveRequest_t *)malloc(numrequests *
   sizeof(DCMF_CollectiveRequest_t));
   
   max_size = ((int)comm_ptr->local_size/numrequests)*numrequests;
   left_over = comm_ptr->local_size - max_size;
   
   for(i=0;i<max_size ;i+=numrequests)
   {
      active = numrequests;
      for(j=0;j<numrequests;j++)
      {
         destbuf = recvbuf + displs[i+j] * extent;
         if(recvcounts[i+j])
            DCMF_AsyncBroadcast(protocol,
                                &requests[j],
                                callback,
                                DCMF_MATCH_CONSISTENCY,
                                &comm_ptr->dcmf.geometry,
                                comm_ptr->vcr[i+j]->lpid,
                                destbuf,
                                data_sz * recvcounts[i+j]);
         else 
            active--;
      }
      MPID_PROGRESS_WAIT_WHILE(active);
   }

   if(left_over)
   {
      active = left_over;
      for(i=0;i<left_over;i++)
      {
         destbuf = recvbuf + displs[max_size + i] * extent;
         if(recvcounts[max_size+i])
            DCMF_AsyncBroadcast(protocol,
                                &requests[i],
                                callback,
                                DCMF_MATCH_CONSISTENCY,
                                &comm_ptr->dcmf.geometry,
                                comm_ptr->vcr[max_size+i]->lpid,
                                destbuf,
                                data_sz * recvcounts[max_size+i]);
         else
            active--;
      }
      MPID_PROGRESS_WAIT_WHILE(active);
   }

   free(requests);

   return MPI_SUCCESS;

}


/* ****************************************************************** */
/**
* \brief Use (tree/rect) MPIDO_Bcast() to do a fast Allgatherv operation
*
* \note This function requires one of these (for max performance):
*       - Tree broadcast
*       - Rect broadcast
*       ? Binomial broadcast
*/
/* ****************************************************************** */
int MPIDO_Allgatherv_Bcast(void *sendbuf,
                           int sendcount,
                           MPI_Datatype sendtype,
                           void *recvbuf,
                           int *recvcounts,
                           int *displs,
                           MPI_Datatype recvtype,
                           MPID_Comm * comm_ptr)
{
   int i;
   MPI_Aint extent;
   MPID_Datatype_get_extent_macro(recvtype, extent);
   /* This isn't technically big enough, but we don't want to
    * add an assert inside the loop */
   MPID_Ensure_Aint_fits_in_pointer (
      (MPI_VOID_PTR_CAST_TO_MPI_AINT recvbuf + 
         displs[comm_ptr->rank] * extent));

   if (sendbuf != MPI_IN_PLACE)
   {
      void *destbuffer = recvbuf + displs[comm_ptr->rank] * extent;
      MPIR_Localcopy(sendbuf,
                     sendcount,
                     sendtype,
                     destbuffer,
                     recvcounts[comm_ptr->rank],
                     recvtype);
   }

   for (i = 0; i < comm_ptr->local_size; i++)
   {
      void *destbuffer = recvbuf + displs[i] * extent;
      MPIDO_Bcast(destbuffer,
                  recvcounts[i],
                  recvtype,
                  i,
                  comm_ptr);
   }
   //if (0==comm_ptr->rank) puts("bcast allgatherv");
   return MPI_SUCCESS;
}

/* ****************************************************************** */
/**
* \brief Use (tree/rect) MPIDO_Alltoall() to do a fast Allgatherv operation
*
* \note This function requires that:
*       - The send/recv data types are contiguous
*       - DMA alltoallv is availible (for max performance)
*/
/* ****************************************************************** */
int MPIDO_Allgatherv_Alltoall(void *sendbuf,
                              int sendcount,
                              MPI_Datatype sendtype,
                              void *recvbuf,
                              int *recvcounts,
                              int *displs,
                              MPI_Datatype recvtype,
                              MPID_Comm * comm_ptr,
                              MPI_Aint send_true_lb,
                              MPI_Aint recv_true_lb,
                              size_t recv_size)
{
   size_t send_size;
   char *startbuf;
   char *destbuf;
   int i;
   int aresult;
   int my_recvcounts = -1;
   void *a2a_sendbuf = NULL;
   int a2a_sendcounts[comm_ptr->local_size];
   int a2a_senddispls[comm_ptr->local_size];

   send_size = recvcounts[comm_ptr->rank] * recv_size;
   for (i = 0; i < comm_ptr->local_size; ++i)
   {
      a2a_sendcounts[i] = send_size;
      a2a_senddispls[i] = 0;
   }
   if (sendbuf != MPI_IN_PLACE)
   {
      a2a_sendbuf = sendbuf + send_true_lb;
   }
   else
   {
      startbuf = (char *) recvbuf + recv_true_lb;
      destbuf = startbuf + displs[comm_ptr->rank] * recv_size;
      a2a_sendbuf = destbuf;
      a2a_sendcounts[comm_ptr->rank] = 0;
      my_recvcounts = recvcounts[comm_ptr->rank];
      recvcounts[comm_ptr->rank] = 0;
   }

   //if (0==comm_ptr->rank) puts("all2all allgatherv");
   aresult = MPIDO_Alltoallv(a2a_sendbuf,
                             a2a_sendcounts,
                             a2a_senddispls,
                             MPI_CHAR,
                             recvbuf,
                             recvcounts,
                             displs,
                             recvtype,
                             comm_ptr);
   if (sendbuf == MPI_IN_PLACE)
      recvcounts[comm_ptr->rank] = my_recvcounts;

   return aresult;
}



int
MPIDO_Allgatherv(void *sendbuf,
                int sendcount,
                MPI_Datatype sendtype,
                void *recvbuf,
                int *recvcounts,
                int *displs,
                MPI_Datatype recvtype,
                MPID_Comm * comm_ptr)
{
  /* *********************************
   * Check the nature of the buffers
   * *********************************
   */

   MPID_Datatype *dt_null = NULL;
   MPI_Aint send_true_lb  = 0;
   MPI_Aint recv_true_lb  = 0;
   size_t   send_size     = 0;
   size_t   recv_size     = 0;
   MPIDO_Coll_config config = {1,1,1,1};

   int      result        = MPI_SUCCESS;
   if(MPIDI_CollectiveProtocols.optallgatherv &&
      (MPIDI_CollectiveProtocols.allgatherv.useallreduce ||
       MPIDI_CollectiveProtocols.allgatherv.usebcast ||
       MPIDI_CollectiveProtocols.allgatherv.usealltoallv ||
       MPIDI_CollectiveProtocols.allgatherv.useasyncbcast) == 0)
   {
      return MPIR_Allgatherv(sendbuf,
                             sendcount,
                             sendtype,
                             recvbuf,
                             recvcounts,
                             displs,
                             recvtype,
                             comm_ptr);
  }

  MPIDI_Datatype_get_info(1,
                          recvtype,
                          config.recv_contig,
                          recv_size,
                          dt_null,
                          recv_true_lb);


   if (sendbuf != MPI_IN_PLACE)
   {
      MPIDI_Datatype_get_info(sendcount,
                              sendtype,
                              config.send_contig,
                              send_size,
                              dt_null,
                              send_true_lb);
      MPID_Ensure_Aint_fits_in_pointer(
            (MPI_VOID_PTR_CAST_TO_MPI_AINT sendbuf + send_true_lb));
   }

   int buffer_sum = 0;
   {
      int i = 0;
      if (0 != displs[0])
         config.recv_continuous = 0;
      for (i = 1; i < comm_ptr->local_size; ++i)
      {
         buffer_sum += recvcounts[i - 1];
         if (buffer_sum != displs[i])
            config.recv_continuous = 0;
         if (!config.recv_continuous)
            break;
      }
      buffer_sum += recvcounts[comm_ptr->local_size - 1];
   }
   buffer_sum *= recv_size;

   config.largecount = (sendcount > 65536);

   MPIDO_Allreduce(MPI_IN_PLACE,
                   &config,
                   4,
                   MPI_INT,
                   MPI_BAND,
                   comm_ptr);
   MPID_Ensure_Aint_fits_in_pointer(
         (MPI_VOID_PTR_CAST_TO_MPI_AINT recvbuf + recv_true_lb + buffer_sum));

   /* determine which protocol to use */
   /* 1) Tree allreduce
   *     a) Need tree allreduce for this communicator
   *     b) User must be ok with allgatherv via allreduce
   *     c) Datatypes must be continguous
   *     d) Count must be a multiple of 4 since tree doesn't support
   *     chars right now
   */
   int treereduce = comm_ptr->dcmf.allreducetree &&
                    MPIDI_CollectiveProtocols.allgatherv.useallreduce &&
                    config.recv_contig && config.send_contig &&
                    config.recv_continuous && buffer_sum % 4 ==0;
   /* 2) Tree bcast
   *     a) Need tree bcast for this communicator
   *     b) User must be ok with allgatherv via bcast
   */     
   int treebcast = comm_ptr->dcmf.bcasttree &&
                   MPIDI_CollectiveProtocols.allgatherv.usebcast;

   /* 3) Alltoall
   *     a) Need torus alltoall for this communicator
   *     b) User must be ok with allgatherv via alltoall
   *     c) Need contiguous datatypes
   */
   int usealltoall = comm_ptr->dcmf.alltoalls &&
                     MPIDI_CollectiveProtocols.allgatherv.usealltoallv &&
                     config.recv_contig && config.send_contig;

   /* 4) async bcast
    *    a) need an async bcast protocol
    *    b) contiguous datatypes
    *    c) user must be ok with it
    */
   char asyncbinom = MPIDI_CollectiveProtocols.allgatherv.useasyncbcast &&
                     MPIDI_CollectiveProtocols.broadcast.useasyncbinom &&
                     config.recv_contig && config.send_contig &&
                     DCMF_Geometry_analyze(&comm_ptr->dcmf.geometry,
                        &MPIDI_CollectiveProtocols.broadcast.binomial);

   char asyncrect = MPIDI_CollectiveProtocols.allgatherv.useasyncbcast &&
                    MPIDI_CollectiveProtocols.broadcast.useasyncrect &&
                    config.recv_contig && config.send_contig &&
                    DCMF_Geometry_analyze(&comm_ptr->dcmf.geometry,
                        &MPIDI_CollectiveProtocols.broadcast.rectangle);


#warning assume same cutoff for allgather
   if(treereduce && treebcast && config.largecount)
   {
//      if(comm_ptr->rank ==0 )fprintf(stderr,"sendcount: %d, calling bcast\n", sendcount);
         result = MPIDO_Allgatherv_Bcast(sendbuf,
                                         sendcount,
                                         sendtype,
                                         recvbuf,
                                         recvcounts,
                                         displs,
                                         recvtype,
                                         comm_ptr);
                                         }
   else if(treereduce && treebcast && !config.largecount)
   {
//      if(comm_ptr->rank ==0 )fprintf(stderr,"sendcount: %d, calling allreduce\n", sendcount);
         result = MPIDO_Allgatherv_Allreduce(sendbuf,
                                             sendcount,
                                             sendtype,
                                             recvbuf,
                                             recvcounts,
                                             displs,
                                             recvtype,
                                             comm_ptr,
                                             send_true_lb,
                                             recv_true_lb,
                                             send_size,
                                             recv_size,
                                             buffer_sum);
   }
   else if(treereduce)
   {
//      if(comm_ptr->rank ==0 )fprintf(stderr,"sendcount: %d, only tree allreduce\n", sendcount);

         result = MPIDO_Allgatherv_Allreduce(sendbuf,
                                             sendcount,
                                             sendtype,
                                             recvbuf,
                                             recvcounts,
                                             displs,
                                             recvtype,
                                             comm_ptr,
                                             send_true_lb,
                                             recv_true_lb,
                                             send_size,
                                             recv_size,
                                             buffer_sum);
                                             }
   else if(treebcast)
   {
//      if(comm_ptr->rank ==0 )fprintf(stderr,"sendcount: %d, only tree bcast\n", sendcount);
         result = MPIDO_Allgatherv_Bcast(sendbuf,
                                         sendcount,
                                         sendtype,
                                         recvbuf,
                                         recvcounts,
                                         displs,
                                         recvtype,
                                         comm_ptr);
   }
   else if(asyncrect)
   {
      result = MPIDO_Allgatherv_Async_bcast(sendbuf,
                                            sendcount,
                                            sendtype,
                                            recvbuf,
                                            recvcounts,
                                            displs,
                                            recvtype,
                                            comm_ptr,
               &MPIDI_CollectiveProtocols.broadcast.async_rectangle);
   }

   else if(asyncbinom)
   {
      result = MPIDO_Allgatherv_Async_bcast(sendbuf,
                                            sendcount,
                                            sendtype,
                                            recvbuf,
                                            recvcounts,
                                            displs,
                                            recvtype,
                                            comm_ptr,
               &MPIDI_CollectiveProtocols.broadcast.async_binomial);
   }

   else if(usealltoall)
         result = MPIDO_Allgatherv_Alltoall(sendbuf,
                                            sendcount,
                                            sendtype,
                                            recvbuf,
                                            recvcounts,
                                            displs,
                                            recvtype,
                                            comm_ptr,
                                            send_true_lb,
                                            recv_true_lb,
                                            recv_size);
   else
         return MPIR_Allgatherv(sendbuf,
                                sendcount,
                                sendtype,
                                recvbuf,
                                recvcounts,
                                displs,
                                recvtype,
                                comm_ptr);
   return result;
}

#if 0



   /* not worth doing on the torus */
   if (MPIDI_CollectiveProtocols.allgatherv.useallreduce &&
         comm_ptr->dcmf.allreducetree &&
         config.recv_contig &&
         config.send_contig &&
         config.recv_continuous &&
         buffer_sum % 4 == 0)
    {
      //if (0==comm_ptr->rank) puts("allreduce allgatherv");
      result = MPIDO_Allgatherv_Allreduce(sendbuf,
                                          sendcount,
                                          sendtype,
                                          recvbuf,
                                          recvcounts,
                                          displs,
                                          recvtype,
                                          comm_ptr,
                                          send_true_lb,
                                          recv_true_lb,
                                          send_size,
                                          recv_size,
                                          buffer_sum);
    }
    /* again, too slow if we only have a rectangle bcast */
   else if (MPIDI_CollectiveProtocols.allgatherv.usebcast &&
               comm_ptr->dcmf.bcasttree)
   {
      //if (0==comm_ptr->rank) puts("bcast allgatherv");
      result = MPIDO_Allgatherv_Bcast(sendbuf,
                                      sendcount,
                                      sendtype,
                                      recvbuf,
                                      recvcounts,
                                      displs,
                                      recvtype,
                                      comm_ptr);
   }
   else if (MPIDI_CollectiveProtocols.allgatherv.usealltoallv &&
               comm_ptr->dcmf.alltoalls &&
               config.recv_contig &&
               config.send_contig)
   {
      //if (0==comm_ptr->rank) puts("all2all allgatherv");
      result = MPIDO_Allgatherv_Alltoall(sendbuf,
                                         sendcount,
                                         sendtype,
                                         recvbuf,
                                         recvcounts,
                                         displs,
                                         recvtype,
                                         comm_ptr,
                                         send_true_lb,
                                         recv_true_lb,
                                         recv_size);
   }
   else
   {
      //if (0==comm_ptr->rank) puts("mpich2 allgatherv");
      return MPIR_Allgatherv(sendbuf,
                             sendcount,
                             sendtype,
                             recvbuf,
                             recvcounts,
                             displs,
                             recvtype,
                             comm_ptr);
   }

   return result;
}
#endif

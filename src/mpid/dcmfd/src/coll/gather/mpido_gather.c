/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file src/coll/gather/mpido_gather.c
 * \brief ???
 */

#include "mpido_coll.h"

#pragma weak PMPIDO_Gather = MPIDO_Gather


int MPIDO_Gather_reduce(void *sendbuf,
                        int sbytes,
                        void *recvbuf,
                        int rbytes,
                        int root,
                        MPID_Comm *comm_ptr)
{
   int rank = comm_ptr->rank;
   int size = comm_ptr->local_size;
   int rc;
   char *tempbuf = NULL;
   char *inplacetemp = NULL;

   if(rank == root)
   {
      tempbuf = recvbuf;
      /* zero the buffer if we aren't in place */
      if(sendbuf != MPI_IN_PLACE)
      {
         memset(tempbuf, 0, sbytes * size * sizeof(char));
         memcpy(tempbuf+(rank * sbytes), sendbuf, sbytes);
      }
      /* copy our contribution temporarily, zero the buffer, put it back */
      /* this will likely suck */
      else
      {
         inplacetemp = MPIU_Malloc(rbytes * sizeof(char));
         memcpy(inplacetemp, recvbuf+(rank * rbytes), rbytes);
         memset(tempbuf, 0, sbytes * size * sizeof(char));
         memcpy(tempbuf+(rank * rbytes), inplacetemp, rbytes);
         MPIU_Free(inplacetemp);
      }
   }
   /* everyone might need to speciifcally allocate a tempbuf, or 
    * we might need to make sure we don't end up at mpich in the
    * mpido_() functions - seems to be a problem?
    */

   /* If we aren't root, malloc tempbuf and zero it, 
    * then copy our contribution to the right spot in the big buffer */
   else
   {
      tempbuf = MPIU_Malloc(sbytes * size * sizeof(char));
      if(!tempbuf)
         return MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, 
                                     "MPI_Gather", __LINE__, MPI_ERR_OTHER,
                                     "**nomem", 0);

      memset(tempbuf, 0, sbytes * size * sizeof(char));
      memcpy(tempbuf+(rank*sbytes), sendbuf, sbytes);
   }

   rc = MPIDO_Reduce(MPI_IN_PLACE, 
                     tempbuf, 
                     (sbytes * size)/4, 
                     MPI_INT, 
                     MPI_BOR, 
                     root, 
                     comm_ptr);

   if(rank != root)
      MPIU_Free(tempbuf);

   return rc;
}

/* works for simple data types, assumes fast reduce is available */

int MPIDO_Gather(void *sendbuf,
                  int sendcount,
                  MPI_Datatype sendtype,
                  void *recvbuf,
                  int recvcount,
                  MPI_Datatype recvtype,
                  int root,
                  MPID_Comm *comm_ptr)
{
   int rank = comm_ptr->rank;
   int size = comm_ptr->local_size;
   int sbytes, rbytes=0;
   MPID_Datatype *dt_ptr;
   MPI_Aint true_lb=0; 
   int contig;

   /* tree bcast is faster. rectangle is good for latency though */
   /* might need to change this later for a cutoff selection */
   int bcastgather = 
      MPIDI_CollectiveProtocols.gather.usereduce &&
      comm_ptr->dcmf.reducetree;

   /* MPICH beats this for latency on smaller values (2048), so use it */
   if(comm_ptr->comm_kind != MPID_INTRACOMM || sendcount<2048 || !bcastgather)
      return MPIR_Gather(sendbuf, sendcount, sendtype,
                          recvbuf, recvcount, recvtype,
                          root, comm_ptr);

   /* we can have different arguments on root node but we need to ensure
    * all nodes do the same thing - optimized or not optimized, so
    * we have to be pretty harsh on these checks 
    */

   /* Everyone, including root has a send buffer */
   /* We need size or sbytes %4 to do tree MPI_INT operations */
   /* 2048 is a reasonable cutoff. mpich has lower latency than this */

    /* I could probably pack noncontig types, but that's a good
     * contrib thing */

   MPIDI_Datatype_get_info(sendcount,
                           sendtype,
                           contig,
                           sbytes,
                           dt_ptr,
                           true_lb);

   if(sendtype == MPI_DATATYPE_NULL || sendcount <= 0 || 
      !contig || (sbytes*size) %4 !=0)
         bcastgather = 0;

   if(rank == root)
   {
      MPIDI_Datatype_get_info(recvcount,
                              recvtype,
                              contig,
                              rbytes,
                              dt_ptr,
                              true_lb);
      if(recvtype == MPI_DATATYPE_NULL || recvcount <=0 || !contig)
         bcastgather = 0;
   }


   /* we need a allreduce step to make sure the arguments are well behaved.
    * across all nodes. see also mpido_allgather(v) 
    * this sucks, but you can have different arguments on root vs !root 
    */
   MPIDO_Allreduce(MPI_IN_PLACE,
                   &bcastgather,
                   1,
                   MPI_INT,
                   MPI_BAND,
                   comm_ptr);


   if(bcastgather)
   {
      if(sendbuf != MPI_IN_PLACE)
      {
         MPID_Ensure_Aint_fits_in_pointer(MPI_VOID_PTR_CAST_TO_MPI_AINT sendbuf + 
                                       true_lb);
         sendbuf = (char *)sendbuf + true_lb;
      }
      MPID_Ensure_Aint_fits_in_pointer(MPI_VOID_PTR_CAST_TO_MPI_AINT recvbuf + 
                                       true_lb);
      recvbuf = (char *)recvbuf + true_lb;

      return MPIDO_Gather_reduce(sendbuf,
                                 sbytes,
                                 recvbuf,
                                 rbytes,
                                 root,
                                 comm_ptr);
      
   }
   else
      return MPIR_Gather(sendbuf, sendcount, sendtype,
                          recvbuf, recvcount, recvtype,
                          root, comm_ptr);
}
      

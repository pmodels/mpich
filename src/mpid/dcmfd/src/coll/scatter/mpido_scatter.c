/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file src/coll/scatter/mpido_scatter.c
 * \brief ???
 */

#include "mpido_coll.h"

#pragma weak PMPIDO_Scatter = MPIDO_Scatter


int MPIDO_Scatter_Bcast(void *sendbuf,
                        int nbytes,
                        void *recvbuf,
                        int root,
                        MPID_Comm *comm_ptr)
{
   /* Pretty simple - bcast a temp buffer and copy our little chunk out */
   int rank = comm_ptr->rank;
   int size = comm_ptr->local_size;
   char *tempbuf = NULL;

   if(rank == root)
      tempbuf = sendbuf;
   else
   {
      tempbuf = MPIU_Malloc(nbytes*size);
      if(!tempbuf)
         return MPIR_Err_create_code(MPI_SUCCESS,
                                     MPIR_ERR_RECOVERABLE,
                                     "MPI_Scatter",
                                     __LINE__, MPI_ERR_OTHER, "**nomem", 0);
   }

   MPIDO_Bcast(tempbuf, nbytes*size, MPI_CHAR, root, comm_ptr);

   if(rank == root && recvbuf == MPI_IN_PLACE)
      return MPI_SUCCESS;
   else
      memcpy(recvbuf, tempbuf+(rank*nbytes), nbytes);

   if(rank!=root)
      MPIU_Free(tempbuf);

   return MPI_SUCCESS;
}

/* works for simple data types, assumes fast bcast is available */

int MPIDO_Scatter(void *sendbuf,
                  int sendcount,
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

   /* Pretty much needs tree bcast to beat mpich across the board */
   int bcastscatter = 
      MPIDI_CollectiveProtocols.scatter.usebcast && comm_ptr->dcmf.bcasttree;

   if(comm_ptr->comm_kind != MPID_INTRACOMM || !bcastscatter)
      return MPIR_Scatter(sendbuf, sendcount, sendtype,
                          recvbuf, recvcount, recvtype,
                          root, comm_ptr);

   /* we can have different arguments on root node but we need to ensure
    * all nodes do the same thing - optimized or not optimized, so
    * we have to be pretty harsh on these checks 
    */
    /* I could probably pack noncontig types, but that's a good
     * contrib thing */

   if(rank == root)
   {
      MPIDI_Datatype_get_info(sendcount,
                              sendtype,
                              contig,
                              nbytes,
                              dt_ptr,
                              true_lb);
      if(recvtype == MPI_DATATYPE_NULL || recvcount <=0 || !contig)
         bcastscatter = 0;
   }
   else
   {
      MPIDI_Datatype_get_info(recvcount,
                               recvtype,
                               contig,
                               nbytes,
                               dt_ptr,
                               true_lb);
      if(sendtype == MPI_DATATYPE_NULL || sendcount <=0 || !contig)
         bcastscatter = 0;
   }

   /* we need a allreduce step to make sure the arguments are well behaved.
    * across all nodes. see also mpido_allgather(v) 
    */
   MPIDO_Allreduce(MPI_IN_PLACE,
                   &bcastscatter,
                   1,
                   MPI_INT,
                   MPI_BAND,
                   comm_ptr);


   if(bcastscatter)
   {
      MPID_Ensure_Aint_fits_in_pointer (MPI_VOID_PTR_CAST_TO_MPI_AINT sendbuf +
                                        true_lb);
      sendbuf = (char *)sendbuf + true_lb;
      if(recvbuf != MPI_IN_PLACE)
      {
         MPID_Ensure_Aint_fits_in_pointer (MPI_VOID_PTR_CAST_TO_MPI_AINT recvbuf +
                                        true_lb);
         recvbuf = (char *)recvbuf + true_lb;
      }

      return MPIDO_Scatter_Bcast(sendbuf,
                                 nbytes,
                                 recvbuf,
                                 root,
                                 comm_ptr);
      
   }
   else
      return MPIR_Scatter(sendbuf, sendcount, sendtype,
                          recvbuf, recvcount, recvtype,
                          root, comm_ptr);
}
      

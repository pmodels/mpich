/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file src/coll/reduce_scatter/mpido_reduce_scatter.c
 * \brief Function prototypes for the optimized collective routines
 */

#include "mpido_coll.h"

#pragma weak PMPIDO_Reduce_scatter = MPIDO_Reduce_scatter

/* Call optimized reduce+scatterv - should be faster than pt2pt on
 * larger partitions 
 */


int MPIDO_Reduce_scatter(void *sendbuf,
                         void *recvbuf,
                         int *recvcounts,
                         MPI_Datatype datatype,
                         MPI_Op op,
                         MPID_Comm * comm_ptr)
{
   int tcount=0, i, rc;

   int contig, nbytes;
   MPID_Datatype *dt_ptr;
   MPI_Aint dt_lb=0;

   char *tempbuf;
   int *displs;
   int size=comm_ptr->local_size;

   /* need a fast alltoall, and a fast reduce to make this viable */
   int optreducescatter = 
      MPIDI_CollectiveProtocols.reduce_scatter.usereducescatter &&
      MPIDI_CollectiveProtocols.optscatterv &&
      MPIDI_CollectiveProtocols.optreduce &&
      comm_ptr->dcmf.alltoalls &&
      (comm_ptr->dcmf.reducetree || comm_ptr->dcmf.reduceccmitree);

   if(comm_ptr->comm_kind != MPID_INTRACOMM)
      optreducescatter = 0;

   /* MPICH has better latency for smaller messages */
   /* Don't bother if we can't use the tree */
   if(recvcounts[0] < 256 || !MPIDI_IsTreeOp(op, datatype))
      optreducescatter = 0;


   if(!optreducescatter)
      return MPIR_Reduce_scatter(sendbuf, 
                                 recvbuf, 
                                 recvcounts, 
                                 datatype, 
                                 op, 
                                 comm_ptr);


   MPIDI_Datatype_get_info(1, 
                           datatype, 
                           contig, 
                           nbytes, 
                           dt_ptr, 
                           dt_lb);

   for(i=0;i<size;i++)
      tcount+=recvcounts[i];

   tempbuf = MPIU_Malloc(nbytes * sizeof(char) * tcount);
   displs = MPIU_Malloc(size * sizeof(int));
   if(!tempbuf || !displs)
   {
      if(tempbuf)
         MPIU_Free(tempbuf);
      return MPIR_Err_create_code(MPI_SUCCESS, 
                                  MPIR_ERR_RECOVERABLE,
                                  "MPI_Reduce_scatter",
                                  __LINE__, MPI_ERR_OTHER, "**nomem", 0);
   }

   memset(displs, 0, size*sizeof(int));

   MPID_Ensure_Aint_fits_in_pointer (MPI_VOID_PTR_CAST_TO_MPI_AINT sendbuf + 
                                     dt_lb);
   sendbuf = (char *)sendbuf + dt_lb;

   rc = MPIDO_Reduce(sendbuf, 
                     tempbuf, 
                     tcount, 
                     datatype, 
                     op, 
                     0, 
                     comm_ptr);

   /* rank 0 has the entire buffer, need to split out our individual piece */
   /* does recvbuf need a dt_lb added? */
   if(rc == MPI_SUCCESS)
   {
      rc = MPIDO_Scatterv(tempbuf, 
                          recvcounts, 
                          displs, 
                          datatype,
                          recvbuf, 
                          tcount/size, 
                          datatype, 
                          0, 
                          comm_ptr);
   }
   MPIU_Free(tempbuf);
   MPIU_Free(displs);

   return rc;

}

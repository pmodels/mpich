/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file src/coll/scatterv/mpido_scatterv.c
 * \brief ???
 */

#include "mpido_coll.h"
#include "mpidi_star.h"
#include "mpidi_coll_prototypes.h"


#ifdef USE_CCMI_COLL

#pragma weak PMPIDO_Scatterv = MPIDO_Scatterv

int MPIDO_Scatterv(void *sendbuf,
                   int *sendcounts,
                   int *displs,
                   MPI_Datatype sendtype,
                   void *recvbuf,
                   int recvcount,
                   MPI_Datatype recvtype,
                   int root,
                   MPID_Comm *comm)
{
  MPIDO_Embedded_Info_Set * properties = &(comm->dcmf.properties);
  int rank = comm->rank, size = comm->local_size;
  int i, nbytes, sum=0, contig;
  MPID_Datatype *dt_ptr;
  MPI_Aint true_lb=0;

  if (MPIDO_INFO_ISSET(properties, MPIDO_USE_MPICH_SCATTERV) ||
      comm->comm_kind != MPID_INTRACOMM)
  {
    comm->dcmf.last_algorithm = MPIDO_USE_MPICH_SCATTERV;
    return MPIR_Scatterv(sendbuf, sendcounts, displs, sendtype,
                         recvbuf, recvcount, recvtype, 
                         root, comm);
  }

    
  /* we can't call scatterv-via-bcast unless we know all nodes have
   * valid sendcount arrays. so the user must explicitly ask for it.
   */

   /* optscatterv[0] == optscatterv bcast? 
    * optscatterv[1] == optscatterv alltoall? 
    *  (having both allows cutoff agreement)
    * optscatterv[2] == sum of sendcounts 
    */
   int optscatterv[3];

   optscatterv[0] = !MPIDO_INFO_ISSET(properties, MPIDO_USE_ALLTOALL_SCATTERV);
   optscatterv[1] = !MPIDO_INFO_ISSET(properties, MPIDO_USE_BCAST_SCATTERV);
   optscatterv[2] = 1;

   if(rank == root)
   {
      if(!optscatterv[1])
      {
         if(sendcounts)
         {
            for(i=0;i<size;i++)
               sum+=sendcounts[i];
         }
         optscatterv[2] = sum;
      }
  
      MPIDI_Datatype_get_info(1,
                            sendtype,
                            contig,
                            nbytes,
                            dt_ptr,
                            true_lb);
      if(recvtype == MPI_DATATYPE_NULL || recvcount <= 0 || !contig)
      {
         optscatterv[0] = 1;
         optscatterv[1] = 1;
      }
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
      {
         optscatterv[0] = 1;
         optscatterv[1] = 1;
      }
   }

  /* Make sure parameters are the same on all the nodes */
  /* specifically, noncontig on the receive */
  /* set the internal control flow to disable internal star tuning */
  STAR_info.internal_control_flow = 1;

   if(MPIDO_INFO_ISSET(properties, MPIDO_USE_PREALLREDUCE_SCATTERV))
   {
      MPIDO_Allreduce(MPI_IN_PLACE,
		  optscatterv,
		  3,
		  MPI_INT,
		  MPI_BOR,
		  comm);
   }
  /* reset flag */
  STAR_info.internal_control_flow = 0;  

   if(!optscatterv[0] || (!optscatterv[1]))
   {
      char *newrecvbuf = recvbuf;
      char *newsendbuf = sendbuf;
      if(rank == root)
      {
        MPIDI_VerifyBuffer(sendbuf, newsendbuf, true_lb);
      }
      else
      {
        MPIDI_VerifyBuffer(recvbuf, newrecvbuf, true_lb);
      }
      if(!optscatterv[0])
      {
        comm->dcmf.last_algorithm = MPIDO_USE_ALLTOALL_SCATTERV;
        return MPIDO_Scatterv_alltoallv(newsendbuf,
                                        sendcounts,
                                        displs,
                                        sendtype,
                                        newrecvbuf,
                                        recvcount,
                                        recvtype,
                                        root,
                                        comm);
        
      }
      else
      {
        comm->dcmf.last_algorithm = MPIDO_USE_BCAST_SCATTERV;
        return MPIDO_Scatterv_bcast(newsendbuf,
                                    sendcounts,
                                    displs,
                                    sendtype,
                                    newrecvbuf,
                                    recvcount,
                                    recvtype,
                                    root,
                                    comm);
      }
   } /* nothing valid to try, go to mpich */
   else
   {
      comm->dcmf.last_algorithm = MPIDO_USE_MPICH_SCATTERV;
      return MPIR_Scatterv(sendbuf, sendcounts, displs, sendtype,
                           recvbuf, recvcount, recvtype,
                           root, comm);
   }
}

#else /* !USE_CCMI_COLL */

int MPIDO_Scatterv(void *sendbuf,
                   int *sendcounts, 
                   int *displs,
                   MPI_Datatype sendtype,
                   void *recvbuf,
                   int recvcount,
                   MPI_Datatype recvtype,
                   int root,
                   MPID_Comm *comm)
{
  MPID_abort();
}
#endif /* !USE_CCMI_COLL */

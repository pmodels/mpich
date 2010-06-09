/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file src/coll/scatter/mpido_scatter.c
 * \brief ???
 */

#include "mpido_coll.h"
#include "mpidi_star.h"
#include "mpidi_coll_prototypes.h"

#ifdef USE_CCMI_COLL


#pragma weak PMPIDO_Scatter = MPIDO_Scatter
/* works for simple data types, assumes fast bcast is available */

int MPIDO_Scatter(void *sendbuf,
                  int sendcount,
                  MPI_Datatype sendtype,
                  void *recvbuf,
                  int recvcount,
                  MPI_Datatype recvtype,
                  int root,
                  MPID_Comm * comm)
{
  MPIDO_Embedded_Info_Set * properties = &(comm->dcmf.properties);
  MPID_Datatype * data_ptr;
  MPI_Aint true_lb = 0;
  char *sbuf = sendbuf, *rbuf = recvbuf;
  int contig, nbytes = 0, rc = 0;
  int rank = comm->rank;
  int success = 1;

  if (rank == root)
  {
    if (recvtype != MPI_DATATYPE_NULL && recvcount >= 0)
    {
      MPIDI_Datatype_get_info(sendcount, sendtype, contig,
                              nbytes, data_ptr, true_lb);
      if (!contig) success = 0;
    }
    else
      success = 0;

    if (success)
    {
      if (recvtype != MPI_DATATYPE_NULL && recvcount >= 0)
      {
        MPIDI_Datatype_get_info(recvcount, recvtype, contig,
                                nbytes, data_ptr, true_lb);
        if (!contig) success = 0;
      }
      else success = 0;
    }
  }

  else
  {
    if (sendtype != MPI_DATATYPE_NULL && sendcount >= 0)
    {
      MPIDI_Datatype_get_info(recvcount, recvtype, contig,
                              nbytes, data_ptr, true_lb);
      if (!contig) success = 0;
    }
    else
      success = 0;
  }

  if (MPIDO_INFO_ISSET(properties, MPIDO_USE_MPICH_SCATTER) ||
      MPIDO_INFO_ISSET(properties, MPIDO_IRREG_COMM) ||
      (!MPIDO_INFO_ISSET(properties, MPIDO_USE_TREE_BCAST) && nbytes <= 64))
  {
    comm->dcmf.last_algorithm = MPIDO_USE_MPICH_SCATTER;
    return MPIR_Scatter_intra(sendbuf, sendcount, sendtype,
                              recvbuf, recvcount, recvtype,
                              root, comm);
  }
  /* set the internal control flow to disable internal star tuning */
  STAR_info.internal_control_flow = 1;

  MPIDO_Allreduce(MPI_IN_PLACE, &success, 1, MPI_INT, MPI_BAND, comm);

  /* reset flag */
  STAR_info.internal_control_flow = 0;

  if (!success)
    return MPIR_Scatter_intra(sendbuf, sendcount, sendtype,
                              recvbuf, recvcount, recvtype,
                              root, comm);

  MPIDI_VerifyBuffer(sendbuf, sbuf, true_lb);
  MPIDI_VerifyBuffer(recvbuf, rbuf, true_lb);
  
  if (!STAR_info.enabled || STAR_info.internal_control_flow ||
      STAR_info.scatter_algorithms == 1)
  {
    if (MPIDO_INFO_ISSET(properties, MPIDO_USE_BCAST_SCATTER))
    {
      comm->dcmf.last_algorithm = MPIDO_USE_BCAST_SCATTER;
      return MPIDO_Scatter_bcast(sbuf, sendcount, sendtype,
                                 rbuf, recvcount, recvtype,
                                 root, comm);
    }
  }
  else
  {
    int id;
    unsigned char same_callsite = 1;

    void ** tb_ptr = (void **) MPIU_Malloc(sizeof(void *) *
                                           STAR_info.traceback_levels);

    /* set the internal control flow to disable internal star tuning */
    STAR_info.internal_control_flow = 1;

    /* get backtrace info for caller to this func, use that as callsite_id */
    backtrace(tb_ptr, STAR_info.traceback_levels);

    id = (int) tb_ptr[STAR_info.traceback_levels - 1];

    /* find out if all participants agree on the callsite id */
    if (STAR_info.agree_on_callsite)
    {
      int tmp[2], result[2];
      tmp[0] = id;
      tmp[1] = ~id;
      MPIDO_Allreduce(tmp, result, 2, MPI_UNSIGNED_LONG, MPI_MAX, comm);
      if (result[0] != (~result[1]))
        same_callsite = 0;
    }

    if (same_callsite)
    {
      STAR_Callsite collective_site;

      /* create a signature callsite info for this particular call site */
      collective_site.call_type = SCATTER_CALL;
      collective_site.comm = comm;
      collective_site.bytes = nbytes;
      collective_site.op_type_support = MPIDO_SUPPORT_NOT_NEEDED;
      collective_site.id = id;
	  
      rc = STAR_Scatter(sbuf, sendcount, sendtype,
                        rbuf, recvcount, recvtype,
                        root, &collective_site,
                        STAR_scatter_repository,
                        STAR_info.scatter_algorithms);
    }
      
    if (rc == STAR_FAILURE || !same_callsite)
      rc = MPIR_Scatter_intra(sendbuf, sendcount, sendtype,
                              recvbuf, recvcount, recvtype,
                              root, comm);

    /* unset the internal control flow */
    STAR_info.internal_control_flow = 0;

    MPIU_Free(tb_ptr);    
  }
   return rc;
}

#endif

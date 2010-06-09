/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file src/coll/gather/mpido_gather.c
 * \brief ???
 */

#include "mpido_coll.h"
#include "mpidi_star.h"
#include "mpidi_coll_prototypes.h"

#ifdef USE_CCMI_COLL

#pragma weak PMPIDO_Gather = MPIDO_Gather

/* works for simple data types, assumes fast reduce is available */

int MPIDO_Gather(void *sendbuf,
                 int sendcount,
                 MPI_Datatype sendtype,
                 void *recvbuf,
                 int recvcount,
                 MPI_Datatype recvtype,
                 int root,
                 MPID_Comm *comm)
{
  MPIDO_Embedded_Info_Set * properties = &(comm->dcmf.properties);
  MPID_Datatype * data_ptr;
  MPI_Aint true_lb = 0;
  char *sbuf = sendbuf, *rbuf = recvbuf;
  int success = 1, contig, send_bytes=-1, recv_bytes = 0;
  int rc = 0, rank = comm->rank;

  if (sendtype != MPI_DATATYPE_NULL && sendcount >= 0)
  {
    MPIDI_Datatype_get_info(sendcount, sendtype, contig,
                            send_bytes, data_ptr, true_lb);
    if (!contig || ((send_bytes * comm->local_size) % sizeof(int)))
      success = 0;
  }
  else
    success = 0;

  if (success && rank == root)
  {
    if (recvtype != MPI_DATATYPE_NULL && recvcount >= 0)
    {
      MPIDI_Datatype_get_info(recvcount, recvtype, contig,
                              recv_bytes, data_ptr, true_lb);
      if (!contig) success = 0;
    }
    else
      success = 0;
  }
  
  if (MPIDO_INFO_ISSET(properties, MPIDO_IRREG_COMM) ||
      MPIDO_INFO_ISSET(properties, MPIDO_USE_MPICH_GATHER) ||
      MPIDO_INFO_ISSET(properties, MPIDO_USE_MPICH_REDUCE) ||
      !MPIDO_INFO_ISSET(properties, MPIDO_TREE_COMM) ||
      !MPIDO_INFO_ISSET(properties, MPIDO_USE_REDUCE_GATHER) ||
      mpid_hw.tSize > 1)
  {
    comm->dcmf.last_algorithm = MPIDO_USE_MPICH_GATHER;
    return MPIR_Gather_intra(sendbuf, sendcount, sendtype,
                             recvbuf, recvcount, recvtype,
                             root, comm);
  }

  /* set the internal control flow to disable internal star tuning */
  STAR_info.internal_control_flow = 1;

  MPIDO_Allreduce(MPI_IN_PLACE, &success, 1, MPI_INT, MPI_BAND, comm);

  STAR_info.internal_control_flow = 0;

  MPID_Ensure_Aint_fits_in_pointer(MPI_VOID_PTR_CAST_TO_MPI_AINT recvbuf +
				   true_lb);
  if (!success)
    return MPIR_Gather_intra(sendbuf, sendcount, sendtype,
                             recvbuf, recvcount, recvtype,
                             root, comm);

  MPIDI_VerifyBuffer(sendbuf, sbuf, true_lb);
  MPIDI_VerifyBuffer(recvbuf, rbuf, true_lb);
  
  if (!STAR_info.enabled || STAR_info.internal_control_flow ||
      STAR_info.gather_algorithms == 1)
  {
    if (MPIDO_INFO_ISSET(properties, MPIDO_USE_REDUCE_GATHER))
    {
      comm->dcmf.last_algorithm = MPIDO_USE_REDUCE_GATHER;
      return MPIDO_Gather_reduce(sbuf, sendcount, sendtype,
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
      collective_site.call_type = GATHER_CALL;
      collective_site.comm = comm;
      collective_site.bytes = send_bytes;
      collective_site.op_type_support = MPIDO_SUPPORT_NOT_NEEDED;
      collective_site.id = id;

      rc = STAR_Gather(sbuf, sendcount, sendtype,
                       rbuf, recvcount, recvtype,
                       root, &collective_site,
                       STAR_gather_repository,
                       STAR_info.gather_algorithms);
    }

    if (rc == STAR_FAILURE || !same_callsite)
      rc = MPIR_Gather_intra(sendbuf, sendcount, sendtype,
                             recvbuf, recvcount, recvtype,
                             root, comm);

    /* unset the internal control flow */
    STAR_info.internal_control_flow = 0;

    MPIU_Free(tb_ptr);

    return rc;    
  }
  /* this should never be reached, but we don't want compiler warnings either*/
   return rc;
}

#endif

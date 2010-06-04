/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file src/coll/allgather/mpido_allgather.c
 * \brief ???
 */

#include "mpidi_star.h"
#include "mpido_coll.h"
#include "mpidi_coll_prototypes.h"


#ifdef USE_CCMI_COLL

#pragma weak PMPIDO_Allgather = MPIDO_Allgather

int
MPIDO_Allgather(void *sendbuf,
                int sendcount,
                MPI_Datatype sendtype,
                void *recvbuf,
                int recvcount,
                MPI_Datatype recvtype,
                MPID_Comm * comm)
{
  /* *********************************
   * Check the nature of the buffers
   * *********************************
   */
  allgather_fptr func = NULL;
  MPIDO_Embedded_Info_Set * coll_prop = &MPIDI_CollectiveProtocols.properties;
  MPIDO_Embedded_Info_Set * comm_prop = &(comm->dcmf.properties);
  MPIDO_Coll_config config = {1,1,1,1,1};
  MPID_Datatype * dt_null = NULL;
  MPI_Aint send_true_lb = 0;
  MPI_Aint recv_true_lb = 0;
  int rc, comm_size = comm->local_size;
  size_t send_size = 0;
  size_t recv_size = 0;

  unsigned char userenvset = MPIDO_INFO_ISSET(comm_prop,
                                             MPIDO_ALLGATHER_ENVVAR);

  char use_tree_reduce, use_alltoall, use_rect_async, use_bcast;
  char *sbuf, *rbuf;

  /* no optimized allgather, punt to mpich */
  if (MPIDO_INFO_ISSET(comm_prop, MPIDO_USE_MPICH_ALLGATHER))
  {
    comm->dcmf.last_algorithm = MPIDO_USE_MPICH_ALLGATHER;
    return MPIR_Allgather_intra(sendbuf, sendcount, sendtype,
                                recvbuf, recvcount, recvtype,
                                comm);
  }
  if ((sendcount < 1 && sendbuf != MPI_IN_PLACE) || recvcount < 1)
    return MPI_SUCCESS;
   
  MPIDI_Datatype_get_info(recvcount,
			  recvtype,
			  config.recv_contig,
			  recv_size,
			  dt_null,
			  recv_true_lb);
  send_size = recv_size;
  recv_size *= comm_size;

  MPIDI_VerifyBuffer(recvbuf, rbuf, (recv_true_lb + comm_size * send_size));
  
  if (sendbuf != MPI_IN_PLACE)
  {
    MPIDI_Datatype_get_info(sendcount,
                            sendtype,
                            config.send_contig,
                            send_size,
                            dt_null,
                            send_true_lb);
    MPIDI_VerifyBuffer(sendbuf, sbuf, send_true_lb);
  }

  /* verify everyone's datatype contiguity */
  if (MPIDO_INFO_ISSET(coll_prop, MPIDO_USE_PREALLREDUCE_ALLGATHER))
  {
    STAR_info.internal_control_flow = 1;
    MPIDO_Allreduce(MPI_IN_PLACE, &config, 5, MPI_INT, MPI_BAND, comm);
    STAR_info.internal_control_flow = 0;
  }

  /* Here is the Default code path or if coming from within another coll */
  if (!STAR_info.enabled || STAR_info.internal_control_flow ||
      send_size < STAR_info.allgather_threshold) 
  {
    use_alltoall = 
      MPIDO_INFO_ISSET(comm_prop, MPIDO_USE_TORUS_ALLTOALL) &&
      MPIDO_INFO_ISSET(comm_prop, MPIDO_USE_ALLTOALL_ALLGATHER) &&
      config.recv_contig && config.send_contig;
    /* The tree doesn't support reduce of chars for the operation we need,
     * so we change to ints. Therefore the size needs to be a multiple of
     * sizeof(int) */
    use_bcast = MPIDO_INFO_ISSET(comm_prop, MPIDO_USE_BCAST_ALLGATHER); 

    use_tree_reduce = 
      MPIDO_INFO_ISSET(comm_prop, MPIDO_USE_TREE_ALLREDUCE) &&
      MPIDO_INFO_ISSET(comm_prop, MPIDO_USE_ALLREDUCE_ALLGATHER) &&
      config.recv_contig && config.send_contig && 
      config.recv_continuous && (recv_size % sizeof(int) == 0);

    use_rect_async = 
      MPIDO_INFO_ISSET(comm_prop, MPIDO_USE_ARECT_BCAST_ALLGATHER) &&
      MPIDO_INFO_ISSET(comm_prop, MPIDO_USE_ARECT_BCAST) &&
      config.recv_contig && config.send_contig;

    if(userenvset)
    {
      if(use_tree_reduce)
      {
        func = MPIDO_Allgather_allreduce;
        comm->dcmf.last_algorithm = MPIDO_USE_ALLREDUCE_ALLGATHER;
      }
      if(use_alltoall)
      {
        func = MPIDO_Allgather_alltoall;
        comm->dcmf.last_algorithm = MPIDO_USE_ALLTOALL_ALLGATHER;
      }
      if(use_rect_async)
      {
        func = MPIDO_Allgather_bcast_rect_async;
        comm->dcmf.last_algorithm = MPIDO_USE_ARECT_BCAST_ALLGATHER;
      }
      if(use_bcast)
      {
        func = MPIDO_Allgather_bcast;
        comm->dcmf.last_algorithm = MPIDO_USE_BCAST_ALLGATHER;
      }
    }
    else
    {
      if (!MPIDO_INFO_ISSET(comm_prop, MPIDO_IRREG_COMM))
      {
        if (comm_size <= 512)
        {
          if (use_tree_reduce && sendcount < 128 * comm_size)
          {
            func = MPIDO_Allgather_allreduce;
            comm->dcmf.last_algorithm = MPIDO_USE_ALLREDUCE_ALLGATHER;
          }
          if (!func && use_bcast && sendcount >= 128 * comm_size)
          {
            func = MPIDO_Allgather_bcast;
            comm->dcmf.last_algorithm = MPIDO_USE_BCAST_ALLGATHER;
          }
          if (!func && use_alltoall &&
              sendcount > 128 && sendcount <= 8*comm_size)
          {
            func = MPIDO_Allgather_alltoall;
            comm->dcmf.last_algorithm = MPIDO_USE_ALLTOALL_ALLGATHER;
          }
          if (!func && use_rect_async && sendcount > 8*comm_size)
          {
            func = MPIDO_Allgather_bcast_rect_async;
            comm->dcmf.last_algorithm = MPIDO_USE_ARECT_BCAST_ALLGATHER;
          }
        }
        else
        {
          if (use_tree_reduce && sendcount < 512)
          {
            func = MPIDO_Allgather_allreduce;
            comm->dcmf.last_algorithm = MPIDO_USE_ALLREDUCE_ALLGATHER;
          }
          if (!func && use_alltoall &&
              sendcount > 128 * (512.0 / (float) comm_size) &&
              sendcount <= 128)
          {
            func = MPIDO_Allgather_alltoall;
            comm->dcmf.last_algorithm = MPIDO_USE_ALLTOALL_ALLGATHER;
          }
          if (!func && use_rect_async &&
              sendcount >= 512 && sendcount <= 65536)
          {
            func = MPIDO_Allgather_bcast_rect_async;
            comm->dcmf.last_algorithm = MPIDO_USE_ARECT_BCAST_ALLGATHER;
          }
          if (!func && use_bcast && sendcount > 65536)
          {
            func = MPIDO_Allgather_bcast;
            comm->dcmf.last_algorithm = MPIDO_USE_BCAST_ALLGATHER;
          }
        }
      }
      else
      {
        if (sendcount >= 64 && use_alltoall)
        {
          func = MPIDO_Allgather_alltoall;
          comm->dcmf.last_algorithm = MPIDO_USE_ALLTOALL_ALLGATHER;
        }
      }
    }
         
    if (!func)
    {
      comm->dcmf.last_algorithm = MPIDO_USE_MPICH_ALLGATHER;
      return MPIR_Allgather_intra(sendbuf, sendcount, sendtype,
                                  recvbuf, recvcount, recvtype,
                                  comm);
    }
    
    rc = (func)(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype,
                send_true_lb, recv_true_lb, send_size, recv_size, comm);
  }  
  else
  {
    STAR_Callsite collective_site;
    void ** tb_ptr = (void **) MPIU_Malloc(sizeof(void *) *
                                           STAR_info.traceback_levels);

    /* set the internal control flow to disable internal star tuning */
    STAR_info.internal_control_flow = 1;
    
    /* get backtrace info for caller to this func, use that as callsite_id */
    backtrace(tb_ptr, STAR_info.traceback_levels);
    
    /* create a signature callsite info for this particular call site */
    collective_site.call_type = ALLGATHER_CALL;
    collective_site.comm = comm;
    collective_site.bytes = recv_size;
    collective_site.op_type_support = MPIDO_SUPPORT_NOT_NEEDED;
    collective_site.buff_attributes[0] = config.send_contig;
    collective_site.buff_attributes[1] = config.recv_contig;
    collective_site.buff_attributes[2] = config.recv_continuous;
    
    /* decide buffer alignment */
    collective_site.buff_attributes[3] = 1; /* assume aligned */
    if (((unsigned)sendbuf & 0x0F) || ((unsigned)recvbuf & 0x0F))
      collective_site.buff_attributes[3] = 0; /* set to not aligned */
    
    collective_site.id = (int) tb_ptr[STAR_info.traceback_levels - 1];
    
    rc = STAR_Allgather(sendbuf,
                        sendcount,
                        sendtype,
                        recvbuf,
                        recvcount,
                        recvtype,
                        send_true_lb,
                        recv_true_lb,
                        send_size,
                        recv_size,
                        &collective_site,
                        STAR_allgather_repository,
                        STAR_info.allgather_algorithms);
      
    /* unset the internal control flow */
    STAR_info.internal_control_flow = 0;
      
    if (rc == STAR_FAILURE)
      rc = MPIR_Allgather_intra(sendbuf, sendcount, sendtype,
                                recvbuf, recvcount, recvtype,
                                comm);
    MPIU_Free(tb_ptr);
  }
  
  return rc;
}

#else /* !USE_CCMI_COLL */

int MPIDO_Allgather(void *sendbuf,
                    int sendcount,
                    MPI_Datatype sendtype,
                    void *recvbuf,
                    int recvcount,
                    MPI_Datatype recvtype,
                    MPID_Comm * comm_ptr)
{
  MPID_abort();
}
#endif /* !USE_CCMI_COLL */

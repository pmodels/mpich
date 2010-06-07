/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file src/coll/allreduce/mpido_allreduce.c
 * \brief ???
 */

#include "mpidi_star.h"
#include "mpido_coll.h"
#include "mpidi_coll_prototypes.h"


#ifdef USE_CCMI_COLL

#pragma weak PMPIDO_Allreduce = MPIDO_Allreduce

int
MPIDO_Allreduce(void * sendbuf,
		void * recvbuf,
		int count,
		MPI_Datatype datatype,
		MPI_Op op,
		MPID_Comm * comm)
{
  allreduce_fptr func = NULL;
  MPIDO_Embedded_Info_Set * properties = &(comm->dcmf.properties);
  DCMF_Dt dcmf_data = DCMF_UNDEFINED_DT;
  DCMF_Op dcmf_op = DCMF_UNDEFINED_OP;
  MPI_Aint data_true_lb, data_true_extent;
  int rc, op_type_support, data_size;
  char *sbuf = sendbuf;
  char *rbuf = recvbuf;

  /* Did the user want to force a specific algorithm? */
  int userenvset = MPIDO_INFO_ISSET(properties, MPIDO_ALLREDUCE_ENVVAR);
  int dput_available, buffer_aligned = 0;
  
  if(count == 0)
    return MPI_SUCCESS;

  op_type_support = MPIDI_ConvertMPItoDCMF(op, &dcmf_op, datatype, &dcmf_data);

  /* quick exit conditions */
  if (MPIDO_INFO_ISSET(properties, MPIDO_USE_MPICH_ALLREDUCE) ||
      //MPIDO_INFO_ISSET(properties, MPIDO_IRREG_COMM) ||
      HANDLE_GET_KIND(op) != HANDLE_KIND_BUILTIN ||
      (op_type_support == MPIDO_NOT_SUPPORTED))
  {
    comm->dcmf.last_algorithm = MPIDO_USE_MPICH_ALLREDUCE;
    return MPIR_Allreduce_intra(sendbuf, recvbuf, count, datatype, op, comm);
  }
  /* Type_get_extent should return the proper value */
  PMPI_Type_get_extent(datatype, &data_true_lb, &data_true_extent);
  MPID_Ensure_Aint_fits_in_int(data_true_extent);
  data_size = count * data_true_extent;

  MPIDI_VerifyBuffer(recvbuf, rbuf, data_true_lb);
  MPIDI_VerifyBuffer(sendbuf, sbuf, data_true_lb);
  if (sendbuf == MPI_IN_PLACE)
    sbuf = rbuf;

  buffer_aligned = !((unsigned)sbuf & 0x0F) && !((unsigned)rbuf & 0x0F);
  dput_available = buffer_aligned &&
                   MPIDO_INFO_ISSET(properties,
                                   MPIDO_USE_RRING_DPUT_SINGLETH_ALLREDUCE);

  if (!STAR_info.enabled || STAR_info.internal_control_flow ||
      data_size < STAR_info.allreduce_threshold)
  {
    if(!userenvset)
    {
      if ((op_type_support == MPIDO_TREE_SUPPORT ||
           op_type_support == MPIDO_TREE_MIN_SUPPORT) &&
          MPIDO_INFO_ISSET(properties, MPIDO_TREE_COMM))
      {
        if (MPIDO_INFO_ISSET(properties, MPIDO_USE_TREE_ALLREDUCE))
        {
          func = MPIDO_Allreduce_global_tree;
          comm->dcmf.last_algorithm = MPIDO_USE_TREE_ALLREDUCE;
        }
        else if (MPIDO_INFO_ISSET(properties, MPIDO_USE_CCMI_TREE_ALLREDUCE))
        {
          func = MPIDO_Allreduce_tree;
          comm->dcmf.last_algorithm = MPIDO_USE_CCMI_TREE_ALLREDUCE;
        }
        else if (MPIDO_INFO_ISSET(properties,
                                 MPIDO_USE_PIPELINED_TREE_ALLREDUCE))
        {
          func = MPIDO_Allreduce_pipelined_tree;
          comm->dcmf.last_algorithm = MPIDO_USE_PIPELINED_TREE_ALLREDUCE;
        }
        if (dput_available && data_size >= 524288 &&
            op_type_support != MPIDO_TREE_SUPPORT)
          func = NULL;
      }

      if(!func && (op_type_support != MPIDO_NOT_SUPPORTED))
      {
        
        if (data_size < 208)
        {
               
          if(MPIDO_INFO_ISSET(properties, 
                             MPIDO_USE_SHORT_ASYNC_RECT_ALLREDUCE))
          {
            func = MPIDO_Allreduce_short_async_rect;
            comm->dcmf.last_algorithm = MPIDO_USE_SHORT_ASYNC_RECT_ALLREDUCE;
          }
          
          if(!func && MPIDO_INFO_ISSET(properties, 
                                      MPIDO_USE_SHORT_ASYNC_BINOM_ALLREDUCE))
          {
            func = MPIDO_Allreduce_short_async_binom;
            comm->dcmf.last_algorithm = MPIDO_USE_SHORT_ASYNC_BINOM_ALLREDUCE;
          }
          if (!func && MPIDO_INFO_ISSET(properties, 
                                       MPIDO_USE_ABINOM_ALLREDUCE))
          {
            func = MPIDO_Allreduce_async_binom;
            comm->dcmf.last_algorithm = MPIDO_USE_ABINOM_ALLREDUCE;
          }
        }
        
        if(!func && data_size <= 16384)
        {
          if (MPIDO_INFO_ISSET(properties, MPIDO_USE_ARECT_ALLREDUCE))
          {
            func = MPIDO_Allreduce_async_rect;
            comm->dcmf.last_algorithm = MPIDO_USE_ARECT_ALLREDUCE;
          }
        }
        
        if(!func && data_size > 16384)
        {
          if(MPIDO_INFO_ISSET(properties, 
                             MPIDO_USE_RRING_DPUT_SINGLETH_ALLREDUCE) &&
             !((unsigned)sbuf & 0x0F) && !((unsigned)rbuf & 0x0F))
          {
            func = MPIDO_Allreduce_rring_dput_singleth;
            comm->dcmf.last_algorithm = MPIDO_USE_RRING_DPUT_SINGLETH_ALLREDUCE;
          }
          if(!func && MPIDO_INFO_ISSET(properties, 
                                      MPIDO_USE_ARECTRING_ALLREDUCE))
          {
            func = MPIDO_Allreduce_async_rectring;
            comm->dcmf.last_algorithm = MPIDO_USE_ARECTRING_ALLREDUCE;
          }
        }
      }
    }
    else
    {
      if (op_type_support == MPIDO_TREE_SUPPORT ||
          op_type_support == MPIDO_TREE_MIN_SUPPORT)
      {
        if (MPIDO_INFO_ISSET(properties, MPIDO_USE_TREE_ALLREDUCE))
        {
          func = MPIDO_Allreduce_global_tree;
          comm->dcmf.last_algorithm = MPIDO_USE_TREE_ALLREDUCE;
        }
        if (!func && MPIDO_INFO_ISSET(properties, MPIDO_USE_CCMI_TREE_ALLREDUCE))
        {
          func = MPIDO_Allreduce_tree;
          comm->dcmf.last_algorithm = MPIDO_USE_CCMI_TREE_ALLREDUCE;
        }
        if (!func &&
            MPIDO_INFO_ISSET(properties, MPIDO_USE_PIPELINED_TREE_ALLREDUCE))
        {
          func = MPIDO_Allreduce_pipelined_tree;
          comm->dcmf.last_algorithm = MPIDO_USE_PIPELINED_TREE_ALLREDUCE;
        }
      }
      
      if(!func && MPIDO_INFO_ISSET(properties, MPIDO_USE_ABINOM_ALLREDUCE))
      {
        func = MPIDO_Allreduce_async_binom;
        comm->dcmf.last_algorithm = MPIDO_USE_ABINOM_ALLREDUCE;
      }
      if(!func &&
         MPIDO_INFO_ISSET(properties, MPIDO_USE_SHORT_ASYNC_RECT_ALLREDUCE) &&
         data_size < 208)
      {  
        func = MPIDO_Allreduce_short_async_rect;
        comm->dcmf.last_algorithm = MPIDO_USE_SHORT_ASYNC_RECT_ALLREDUCE;
      }
      if(!func &&
         MPIDO_INFO_ISSET(properties, MPIDO_USE_SHORT_ASYNC_BINOM_ALLREDUCE) &&
         data_size < 208)
      {
        func = MPIDO_Allreduce_short_async_binom;
        comm->dcmf.last_algorithm = MPIDO_USE_SHORT_ASYNC_BINOM_ALLREDUCE;
      }

      if(!func &&
         MPIDO_INFO_ISSET(properties, MPIDO_USE_RRING_DPUT_SINGLETH_ALLREDUCE) &&
         !((unsigned)sbuf & 0x0F) && !((unsigned)rbuf & 0x0F))
      {
        func = MPIDO_Allreduce_rring_dput_singleth;
        comm->dcmf.last_algorithm = MPIDO_USE_RRING_DPUT_SINGLETH_ALLREDUCE;
      }
      if (!func &&
	  MPIDO_INFO_ISSET(properties, MPIDO_USE_ARECT_ALLREDUCE))
      {
	func = MPIDO_Allreduce_async_rect;
        comm->dcmf.last_algorithm = MPIDO_USE_ARECT_ALLREDUCE;
      }
      if(!func &&
         MPIDO_INFO_ISSET(properties, MPIDO_USE_ARECTRING_ALLREDUCE))
      {
        func = MPIDO_Allreduce_async_rectring;
        comm->dcmf.last_algorithm = MPIDO_USE_ARECTRING_ALLREDUCE;
      }
    }
         
    if (func)
      rc = (func)(sbuf,
                  rbuf,
                  count,
                  dcmf_data,
                  dcmf_op,
                  datatype,
                  comm);

    /*
      punt to MPICH in the case no optimized func is found or in the case
      generic op/type is used
    */
    else
    {
      rc = MPIR_Allreduce_intra(sendbuf, recvbuf, count, datatype, op, comm);
      comm->dcmf.last_algorithm = MPIDO_USE_MPICH_ALLREDUCE;
    }
  }

  else
  {
    void ** tb_ptr;
    STAR_Callsite collective_site;

    /* set the internal control flow to disable internal star tuning */
    STAR_info.internal_control_flow = 1;

    tb_ptr = (void **) MPIU_Malloc(sizeof(void *) *
                                   STAR_info.traceback_levels);

    /* get backtrace info for caller to this func, use that as callsite_id */
    backtrace(tb_ptr, STAR_info.traceback_levels);

    /* create a signature callsite info for this particular call site */
    collective_site.call_type = ALLREDUCE_CALL;
    collective_site.comm = comm;
    collective_site.bytes = data_size;
    collective_site.id = (int) tb_ptr[STAR_info.traceback_levels - 1];
    collective_site.op_type_support = op_type_support;

    /* decide buffer alignment */
    collective_site.buff_attributes[3] = buffer_aligned;

    rc = STAR_Allreduce(sbuf, rbuf, count, dcmf_data, dcmf_op,
                        datatype, &collective_site,
                        STAR_allreduce_repository,
                        STAR_info.allreduce_algorithms);

    if (rc == STAR_FAILURE)
    {
      rc = MPIR_Allreduce_intra(sendbuf, recvbuf, count, datatype, op, comm);
    }

    MPIU_Free(tb_ptr);

    /* unset the internal control flow */
    STAR_info.internal_control_flow = 0;
  }


  return rc;
}

#else /* !USE_CCMI_COLL */

int MPIDO_Allreduce(
                    void * sendbuf,
                    void * recvbuf,
                    int count,
                    MPI_Datatype datatype,
                    MPI_Op op,
                    MPID_Comm * comm_ptr)
{
  MPID_abort();
}

#endif /* !USE_CCMI_COLL */

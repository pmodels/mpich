/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file src/coll/reduce/mpido_reduce.c
 * \brief ???
 */

#include "mpidi_star.h"
#include "mpido_coll.h"
#include "mpidi_coll_prototypes.h"


#ifdef USE_CCMI_COLL

#pragma weak PMPIDO_Reduce = MPIDO_Reduce

int MPIDO_Reduce(void * sendbuf,
                 void * recvbuf,
                 int count,
                 MPI_Datatype datatype,
                 MPI_Op op,
                 int root,
                 MPID_Comm * comm)
{
  reduce_fptr func = NULL;
  MPIDO_Embedded_Info_Set * properties = &(comm->dcmf.properties);
  int success = 1, rc = 0, op_type_support;
  int use_allreduce = 0, data_contig, data_size = 0;
  int userenvset = MPIDO_INFO_ISSET(properties, MPIDO_REDUCE_ENVVAR);
  MPID_Datatype * data_ptr;
  MPI_Aint data_true_lb = 0;
  char *sbuf = sendbuf, *rbuf = recvbuf, *tmpbuf = recvbuf;

  DCMF_Dt dcmf_data = DCMF_UNDEFINED_DT;
  DCMF_Op dcmf_op = DCMF_UNDEFINED_OP;

  if (datatype != MPI_DATATYPE_NULL && count > 0)
  {
    MPIDI_Datatype_get_info(count,
                            datatype,
                            data_contig,
                            data_size,
                            data_ptr,
                            data_true_lb);
    if (!data_contig) success = 0;
  }
  else
    success = 0;

  /* quick exit conditions */
  if (MPIDO_INFO_ISSET(properties, MPIDO_USE_MPICH_REDUCE) ||
      HANDLE_GET_KIND(op) != HANDLE_KIND_BUILTIN || !success)
  {
    comm->dcmf.last_algorithm = MPIDO_USE_MPICH_REDUCE;
    return MPIR_Reduce_intra(sendbuf, recvbuf, count, datatype, op, root, comm);
  }
  op_type_support = MPIDI_ConvertMPItoDCMF(op, &dcmf_op, datatype, &dcmf_data);

  MPIDI_VerifyBuffer(recvbuf, rbuf, data_true_lb);

  MPIDI_VerifyBuffer(sendbuf, sbuf, data_true_lb);
  if (sendbuf == MPI_IN_PLACE)
    sbuf = rbuf;

  if (!STAR_info.enabled || STAR_info.internal_control_flow ||
      (((op_type_support == MPIDO_TREE_SUPPORT ||
         op_type_support == MPIDO_TREE_MIN_SUPPORT) &&
        MPIDO_INFO_ISSET(properties, MPIDO_TREE_COMM)) ||
       data_size < STAR_info.reduce_threshold))
  {
    if(!userenvset)
    {

      /*
        we need to see when reduce via allreduce is good, susect around 32K
        and then basically turn it on. It is off by default.
      */
      if (op_type_support == MPIDO_TREE_SUPPORT ||
           op_type_support == MPIDO_TREE_MIN_SUPPORT)
	{
	  if (MPIDO_INFO_ISSET(properties, MPIDO_USE_TREE_REDUCE))
	    {
	      func = MPIDO_Reduce_global_tree;
	      comm->dcmf.last_algorithm = MPIDO_USE_TREE_REDUCE;
	    }
	  else if(MPIDO_INFO_ISSET(properties, MPIDO_USE_CCMI_TREE_REDUCE))
	    {
	      func = MPIDO_Reduce_tree;
	      comm->dcmf.last_algorithm = MPIDO_USE_CCMI_TREE_REDUCE;
	    }
	}
       
      if (!func && op_type_support != MPIDO_NOT_SUPPORTED)
      {
        if (MPIDO_INFO_ISSET(properties, MPIDO_IRREG_COMM))
        {
          func = MPIDO_Reduce_binom;
          comm->dcmf.last_algorithm = MPIDO_USE_BINOM_REDUCE;
        }
        if (!func && (data_size <= 32768))
        {
          if (MPIDO_INFO_ISSET(properties, MPIDO_USE_BINOM_REDUCE))
          {
            func = MPIDO_Reduce_binom;
            comm->dcmf.last_algorithm = MPIDO_USE_BINOM_REDUCE;
          }
        }
         
        if (!func && (data_size > 32768))
        {
          if (MPIDO_INFO_ISSET(properties, MPIDO_USE_RECTRING_REDUCE))
          {
            func = MPIDO_Reduce_rectring;
            comm->dcmf.last_algorithm = MPIDO_USE_RECTRING_REDUCE;
          }
        }
      }
    }
    else
    {
      if(MPIDO_INFO_ISSET(properties, MPIDO_USE_BINOM_REDUCE))
      {
        func = MPIDO_Reduce_binom;
        comm->dcmf.last_algorithm = MPIDO_USE_BINOM_REDUCE;
      }
      if(!func &&
         MPIDO_INFO_ISSET(properties, MPIDO_USE_TREE_REDUCE) &&
         (op_type_support == MPIDO_TREE_SUPPORT ||
         op_type_support == MPIDO_TREE_MIN_SUPPORT))
      {
        func = MPIDO_Reduce_global_tree;
        comm->dcmf.last_algorithm = MPIDO_USE_TREE_REDUCE;
      }
      if(!func &&
         MPIDO_INFO_ISSET(properties, MPIDO_USE_CCMI_TREE_REDUCE) &&
         (op_type_support == MPIDO_TREE_SUPPORT ||
         op_type_support == MPIDO_TREE_MIN_SUPPORT))
      {
        func = MPIDO_Reduce_tree;
        comm->dcmf.last_algorithm = MPIDO_USE_CCMI_TREE_REDUCE;
      }
      if(!func &&
         MPIDO_INFO_ISSET(properties, MPIDO_USE_RECTRING_REDUCE))
      {
        func = MPIDO_Reduce_rectring;
        comm->dcmf.last_algorithm = MPIDO_USE_RECTRING_REDUCE;
      }
      if (!func && MPIDO_INFO_ISSET(properties, MPIDO_USE_ALLREDUCE_REDUCE))
      {
        comm->dcmf.last_algorithm = MPIDO_USE_ALLREDUCE_REDUCE;
        use_allreduce = 1;
      }
    }
     
    if (func)
      rc = (func)(sbuf, rbuf, count, dcmf_data,
                  dcmf_op, datatype, root, comm);      

    else if (use_allreduce)
    {
      if (comm->rank != root)
      {
        if (!(tmpbuf = MPIU_Malloc(data_size)))
          return MPIR_Err_create_code(MPI_SUCCESS,
                                      MPIR_ERR_RECOVERABLE,
                                      "MPI_REDUCE",
                                      __LINE__, MPI_ERR_OTHER, "**nomem", 0);
        
      }

      rc = MPIDO_Allreduce(sbuf, tmpbuf, count, datatype, op, comm);

      if (comm->rank != root)
        MPIU_Free(tmpbuf);
    }
    else
    {
      rc = MPIR_Reduce_intra(sendbuf, recvbuf, count, datatype, op, root, comm);
      comm->dcmf.last_algorithm = MPIDO_USE_MPICH_REDUCE;
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
      collective_site.call_type = REDUCE_CALL;
      collective_site.comm = comm;
      collective_site.bytes = data_size;
      collective_site.id = id;
      collective_site.op_type_support = op_type_support;


      rc = STAR_Reduce(sbuf, rbuf, count, dcmf_data, dcmf_op,
                       datatype, root, &collective_site,
                       STAR_reduce_repository,
                       STAR_info.reduce_algorithms);
    }

    if (rc == STAR_FAILURE || !same_callsite)
    {
      rc = MPIR_Reduce_intra(sendbuf, recvbuf, count, datatype, op, root, comm);
    }
    /* unset the internal control flow */
    STAR_info.internal_control_flow = 0;

    MPIU_Free(tb_ptr);
  }  

  
  return rc;
}

#else /* !USE_CCMI_COLL */

int MPIDO_Reduce(void * sendbuf,
                 void * recvbuf,
                 int count,
                 MPI_Datatype datatype,
                 MPI_Op op,
                 int root,
                 MPID_Comm * comm_ptr)
{
  MPID_abort();
}
#endif /* !USE_CCMI_COLL */

/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file src/mpid/dcmfd/src/coll/bcast/mpido_bcast.c
 * \brief ???
 */

#include "mpido_coll.h"
#include "mpidi_star.h"
#include "mpidi_coll_prototypes.h"
#include "mpix.h"


#ifdef USE_CCMI_COLL

#pragma weak PMPIDO_Bcast = MPIDO_Bcast

int
MPIDO_Bcast(void *buffer,
            int count, MPI_Datatype datatype, int root, MPID_Comm * comm)
{
  bcast_fptr func = NULL;
  MPIDO_Embedded_Info_Set *properties = &(comm->dcmf.properties);

  int data_size, data_contig, rc = MPI_ERR_INTERN;
  char *data_buffer = NULL, *noncontig_buff = NULL;
  unsigned char userenvset = MPIDO_INFO_ISSET(properties, MPIDO_BCAST_ENVVAR);
  
  MPI_Aint data_true_lb = 0;
  MPID_Datatype *data_ptr;
  MPID_Segment segment;
     
  if (count==0)
    return MPI_SUCCESS;

  if (MPIDO_INFO_ISSET(properties, MPIDO_USE_MPICH_BCAST))
    return MPIR_Bcast_intra(buffer, count, datatype, root, comm);

  MPIDI_Datatype_get_info(count,
                          datatype,
                          data_contig, data_size, data_ptr, data_true_lb);


  MPIDI_VerifyBuffer(buffer, data_buffer, data_true_lb);

  if (!data_contig)
  {
    noncontig_buff = MPIU_Malloc(data_size);
    data_buffer = noncontig_buff;
    if (noncontig_buff == NULL)
    {
      fprintf(stderr,
              "Pack: Tree Bcast cannot allocate local non-contig pack"
              " buffer\n");
      MPIX_Dump_stacks();
      MPID_Abort(NULL, MPI_ERR_NO_SPACE, 1,
                 "Fatal:  Cannot allocate pack buffer");
    }

    if (comm->rank == root)
    {
      DLOOP_Offset last = data_size;
      MPID_Segment_init(buffer, count, datatype, &segment, 0);
      MPID_Segment_pack(&segment, 0, &last, noncontig_buff);
    }
  }
  if (!STAR_info.enabled || STAR_info.internal_control_flow ||
      data_size < STAR_info.bcast_threshold)
  {
    if (data_size <= 1024 || userenvset)
    {
      if (MPIDO_INFO_ISSET(properties, MPIDO_USE_TREE_BCAST))
      {
        func = MPIDO_Bcast_tree;
        comm->dcmf.last_algorithm = MPIDO_USE_TREE_BCAST;
      }
      
      if ((!func || userenvset) &&
          MPIDO_INFO_ISSET(properties, MPIDO_USE_RECT_SINGLETH_BCAST) &&
          data_size > 128)
      {
        func = MPIDO_Bcast_rect_singleth;
        comm->dcmf.last_algorithm = MPIDO_USE_RECT_SINGLETH_BCAST;
      }
      if ((!func || userenvset) &&
          MPIDO_INFO_ISSET(properties, MPIDO_USE_ARECT_BCAST))
      {
        func = MPIDO_Bcast_rect_async;
        comm->dcmf.last_algorithm = MPIDO_USE_ARECT_BCAST;
      }
      if ((!func || userenvset) &&
          MPIDO_INFO_ISSET(properties, MPIDO_USE_ABINOM_BCAST))
      {
        func = MPIDO_Bcast_binom_async;
        comm->dcmf.last_algorithm = MPIDO_USE_ABINOM_BCAST;
      }
      if ((!func || userenvset) &&
          MPIDO_INFO_ISSET(properties, MPIDO_USE_SCATTER_GATHER_BCAST))
      {
        func = MPIDO_Bcast_scatter_gather;
        comm->dcmf.last_algorithm = MPIDO_USE_SCATTER_GATHER_BCAST;
      }
      if ((!func || userenvset) &&
          MPIDO_INFO_ISSET(properties, MPIDO_USE_CCMI_TREE_BCAST))
      {
        func = MPIDO_Bcast_CCMI_tree;
        comm->dcmf.last_algorithm = MPIDO_USE_CCMI_TREE_BCAST;
      }
      if ((!func || userenvset) &&
          MPIDO_INFO_ISSET(properties, MPIDO_USE_CCMI_TREE_DPUT_BCAST))
      {
        func = MPIDO_Bcast_CCMI_tree_dput;
        comm->dcmf.last_algorithm = MPIDO_USE_CCMI_TREE_DPUT_BCAST;
      }
    }

    if(!func && (data_size <= 8192 || userenvset))
    {
      if (MPIDO_INFO_ISSET(properties, MPIDO_USE_TREE_BCAST))
      {
        func = MPIDO_Bcast_tree;
        comm->dcmf.last_algorithm = MPIDO_USE_TREE_BCAST;
      }
      if ((!func || userenvset) &&
          MPIDO_INFO_ISSET(properties, MPIDO_USE_RECT_SINGLETH_BCAST))
      {
        func = MPIDO_Bcast_rect_singleth;
        comm->dcmf.last_algorithm = MPIDO_USE_RECT_SINGLETH_BCAST;
      }
      if ((!func  || userenvset) &&
          MPIDO_INFO_ISSET(properties, MPIDO_USE_RECT_BCAST))
      {
        func = MPIDO_Bcast_rect_sync;
        comm->dcmf.last_algorithm = MPIDO_USE_RECT_BCAST;
      }
      if ((!func || userenvset) &&
          MPIDO_INFO_ISSET(properties, MPIDO_USE_ABINOM_BCAST))
      {
        func = MPIDO_Bcast_binom_async;
        comm->dcmf.last_algorithm = MPIDO_USE_ABINOM_BCAST;
      }
      if ((!func || userenvset) &&
          MPIDO_INFO_ISSET(properties, MPIDO_USE_SCATTER_GATHER_BCAST))
      {
        func = MPIDO_Bcast_scatter_gather;
        comm->dcmf.last_algorithm = MPIDO_USE_SCATTER_GATHER_BCAST;
      }
      if ((!func || userenvset) &&
          MPIDO_INFO_ISSET(properties, MPIDO_USE_CCMI_TREE_BCAST))
      {
        func = MPIDO_Bcast_CCMI_tree;
        comm->dcmf.last_algorithm = MPIDO_USE_CCMI_TREE_BCAST;
      }
      if ((!func || userenvset) &&
          MPIDO_INFO_ISSET(properties, MPIDO_USE_CCMI_TREE_DPUT_BCAST))
      {
        func = MPIDO_Bcast_CCMI_tree_dput;
        comm->dcmf.last_algorithm = MPIDO_USE_CCMI_TREE_DPUT_BCAST;
      }
    }

    if(!func && (data_size <= 65536 || userenvset))
    {
      if (MPIDO_INFO_ISSET(properties, MPIDO_USE_TREE_BCAST))
      {
        func = MPIDO_Bcast_tree;
        comm->dcmf.last_algorithm = MPIDO_USE_TREE_BCAST;
      }
      if ((!func  || userenvset) &&
          MPIDO_INFO_ISSET(properties, MPIDO_USE_RECT_DPUT_BCAST) &&
          !((unsigned) data_buffer & 0x0F))
      {
        func = MPIDO_Bcast_rect_dput;
        comm->dcmf.last_algorithm = MPIDO_USE_RECT_DPUT_BCAST;
      }
      if ((!func || userenvset) &&
          MPIDO_INFO_ISSET(properties, MPIDO_USE_RECT_BCAST))
      {
        func = MPIDO_Bcast_rect_sync;
        comm->dcmf.last_algorithm = MPIDO_USE_RECT_BCAST;
      }
      if ((!func  || userenvset) &&
          MPIDO_INFO_ISSET(properties, MPIDO_USE_BINOM_SINGLETH_BCAST))
      {
        func = MPIDO_Bcast_binom_singleth;
        comm->dcmf.last_algorithm = MPIDO_USE_BINOM_SINGLETH_BCAST;
      }
      if ((!func || userenvset) &&
          MPIDO_INFO_ISSET(properties, MPIDO_USE_ABINOM_BCAST) &&
          data_size < 16384)
      {
        func = MPIDO_Bcast_binom_async;
        comm->dcmf.last_algorithm = MPIDO_USE_ABINOM_BCAST;
      }
      if ((!func || userenvset) &&
          MPIDO_INFO_ISSET(properties, MPIDO_USE_BINOM_BCAST))
      {
        func = MPIDO_Bcast_binom_sync;      
        comm->dcmf.last_algorithm = MPIDO_USE_BINOM_BCAST;
      }
      if ((!func || userenvset) &&
          MPIDO_INFO_ISSET(properties, MPIDO_USE_SCATTER_GATHER_BCAST))
      {
        func = MPIDO_Bcast_scatter_gather;
        comm->dcmf.last_algorithm = MPIDO_USE_SCATTER_GATHER_BCAST;
      }
      if ((!func || userenvset) &&
          MPIDO_INFO_ISSET(properties, MPIDO_USE_CCMI_TREE_BCAST))
      {
        func = MPIDO_Bcast_CCMI_tree;
        comm->dcmf.last_algorithm = MPIDO_USE_CCMI_TREE_BCAST;
      }
      if ((!func || userenvset) &&
          MPIDO_INFO_ISSET(properties, MPIDO_USE_CCMI_TREE_DPUT_BCAST))
      {
        func = MPIDO_Bcast_CCMI_tree_dput;
        comm->dcmf.last_algorithm = MPIDO_USE_CCMI_TREE_DPUT_BCAST;
      }
    }

    if(!func && (data_size > 65536 || userenvset))
    {
      if (MPIDO_INFO_ISSET(properties, MPIDO_USE_RECT_DPUT_BCAST) &&
          !((unsigned) data_buffer & 0x0F))
      {
        func = MPIDO_Bcast_rect_dput;
        comm->dcmf.last_algorithm = MPIDO_USE_RECT_DPUT_BCAST;
      }
      if ((!func || userenvset) &&
          MPIDO_INFO_ISSET(properties, MPIDO_USE_TREE_BCAST))
      {
        func = MPIDO_Bcast_tree;
        comm->dcmf.last_algorithm = MPIDO_USE_TREE_BCAST;
      }
      if ((!func || userenvset) &&
          MPIDO_INFO_ISSET(properties, MPIDO_USE_RECT_BCAST))
      {
        func = MPIDO_Bcast_rect_sync;
        comm->dcmf.last_algorithm = MPIDO_USE_RECT_BCAST;
      }
      if ((!func || userenvset) &&
          MPIDO_INFO_ISSET(properties, MPIDO_USE_BINOM_SINGLETH_BCAST))
      {
        func = MPIDO_Bcast_binom_singleth;
        comm->dcmf.last_algorithm = MPIDO_USE_BINOM_SINGLETH_BCAST;
      }
      if ((!func || userenvset) &&
          MPIDO_INFO_ISSET(properties, MPIDO_USE_BINOM_BCAST))
      {
        func = MPIDO_Bcast_binom_sync;
        comm->dcmf.last_algorithm = MPIDO_USE_BINOM_BCAST;
      }
      if ((!func || userenvset) &&
          MPIDO_INFO_ISSET(properties, MPIDO_USE_SCATTER_GATHER_BCAST))
      {
        func = MPIDO_Bcast_scatter_gather;
        comm->dcmf.last_algorithm = MPIDO_USE_SCATTER_GATHER_BCAST;
      }
      if ((!func || userenvset) &&
          MPIDO_INFO_ISSET(properties, MPIDO_USE_CCMI_TREE_BCAST))
      {
        func = MPIDO_Bcast_CCMI_tree;
        comm->dcmf.last_algorithm = MPIDO_USE_CCMI_TREE_BCAST;
      }
      if ((!func || userenvset) &&
          MPIDO_INFO_ISSET(properties, MPIDO_USE_CCMI_TREE_DPUT_BCAST))
      {
        func = MPIDO_Bcast_CCMI_tree_dput;
        comm->dcmf.last_algorithm = MPIDO_USE_CCMI_TREE_DPUT_BCAST;
      }
    }
    
    if (!func)
    {
      comm->dcmf.last_algorithm = MPIDO_USE_MPICH_BCAST;
      return MPIR_Bcast_intra(buffer, count, datatype, root, comm);
    }
    
    rc = (func) (data_buffer, data_size, root, comm);
  }
      
  else
  {
    int id;
    unsigned char same_callsite = 1;

    STAR_Callsite collective_site;

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
      /* create a signature callsite info for this particular call site */
      collective_site.call_type = BCAST_CALL;
      collective_site.comm = comm;
      collective_site.bytes = data_size;
      collective_site.op_type_support = MPIDO_SUPPORT_NOT_NEEDED;
      collective_site.id = id;

      /* decide buffer alignment */
      collective_site.buff_attributes[3] = 1; /* assume aligned */
      if ((unsigned)data_buffer & 0x0F)
        collective_site.buff_attributes[3] = 0; /* set to not aligned */

      rc = STAR_Bcast(data_buffer, root, &collective_site,
                      STAR_bcast_repository,
                      STAR_info.bcast_algorithms);
    }

    if (rc == STAR_FAILURE || !same_callsite)
    {
      rc = MPIR_Bcast_intra(buffer, count, datatype, root, comm);
    }
    
    /* unset the internal control flow */
    STAR_info.internal_control_flow = 0;

    MPIU_Free(tb_ptr);    
  }
  if (!data_contig)
  {
    if (comm->rank != root)
    {
      int smpi_errno, rmpi_errno;
      MPIDI_msg_sz_t rcount;
      MPIDI_DCMF_Buffer_copy(noncontig_buff, data_size, MPI_CHAR,
                             &smpi_errno, buffer, count, datatype,
                             &rcount, &rmpi_errno);
    }
    MPIU_Free(noncontig_buff);
  }

  return rc;
}

#else /* !USE_CCMI_COLL */

int
MPIDO_Bcast(void *buffer,
            int count, MPI_Datatype datatype, int root, MPID_Comm * comm_ptr)
{
  MPID_abort();
}
#endif /* !USE_CCMI_COLL */

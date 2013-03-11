/* begin_generated_IBM_copyright_prolog                             */
/*                                                                  */
/* This is an automatically generated copyright prolog.             */
/* After initializing,  DO NOT MODIFY OR MOVE                       */
/*  --------------------------------------------------------------- */
/* Licensed Materials - Property of IBM                             */
/* Blue Gene/Q 5765-PER 5765-PRP                                    */
/*                                                                  */
/* (C) Copyright IBM Corp. 2011, 2012 All Rights Reserved           */
/* US Government Users Restricted Rights -                          */
/* Use, duplication, or disclosure restricted                       */
/* by GSA ADP Schedule Contract with IBM Corp.                      */
/*                                                                  */
/*  --------------------------------------------------------------- */
/*                                                                  */
/* end_generated_IBM_copyright_prolog                               */
/*  (C)Copyright IBM Corp.  2007, 2011  */
/**
 * \file src/comm/mpid_optcolls.c
 * \brief Code for setting up optimized collectives per geometry/communicator
 * \brief currently uses strcmp and known protocol performance
 */



/*#define TRACE_ON */

#include <mpidimpl.h>


static int MPIDI_Check_FCA_envvar(char *string, int *user_range_hi)
{
  char *env = getenv("MP_MPI_PAMI_FOR");
  if(env != NULL)
  {
    if(strcasecmp(env, "ALL") == 0)
      return 1;
    int len = strlen(env);
    len++;
    char *temp = MPIU_Malloc(sizeof(char) * len);
    char *ptrToFree = temp;
    strcpy(temp, env);
    char *sepptr;
    for(sepptr = temp; (sepptr = strsep(&temp, ",")) != NULL ; )
    {
      char *subsepptr, *temp_sepptr;
      temp_sepptr = sepptr;
      subsepptr = strsep(&temp_sepptr, ":");
      if(temp_sepptr != NULL)/* SSS: There is a a colon for this collective */
      {
        if(strcasecmp(subsepptr, string) == 0)
        {
          *user_range_hi = atoi(temp_sepptr);
          MPIU_Free(ptrToFree);
          return 1;
        }
        else
          sepptr++;
      }
      else
      {
        if(strcasecmp(sepptr, string) == 0)
        {
          *user_range_hi = -1;
          MPIU_Free(ptrToFree);
          return 1;
        }
        else
          sepptr++;
      }
    }
    /* We didn't find it, but the end var was set, so return 0 */
    MPIU_Free(ptrToFree);
    return 0;
  }
  if(MPIDI_Process.optimized.collectives == MPID_COLL_FCA)
    return 1; /* To have gotten this far, opt colls are on. If the env doesn't exist,
                 we should use the FCA protocol for "string" */
  else
    return -1; /* we don't have any FCA things */
}

static inline void 
MPIDI_Coll_comm_check_FCA(char *coll_name, 
                          char *protocol_name, 
                          int pami_xfer, 
                          int query_type,
                          int proto_num,
                          MPID_Comm *comm_ptr)
{
  int opt_proto = -1;
  int i;
  int user_range_hi = -1;/* SSS: By default we assume user hasn't defined a range_hi (cutoff_size) */
#ifdef TRACE_ON
  char *envstring = getenv("MP_MPI_PAMI_FOR");
#endif
  TRACE_ERR("Checking for %s in %s\n", coll_name, envstring);
  int check_var = MPIDI_Check_FCA_envvar(coll_name, &user_range_hi);
  if(check_var == 1)
  {
    TRACE_ERR("Found %s\n",coll_name);
    /* Look for protocol_name in the "always works list */
    for(i = 0; i <comm_ptr->mpid.coll_count[pami_xfer][0]; i++)
    {
      if(strcasecmp(comm_ptr->mpid.coll_metadata[pami_xfer][0][i].name, protocol_name) == 0)
      {
        opt_proto = i;
        break;
      }
    }
    if(opt_proto != -1) /* we found it, so copy it to the optimized var */
    {
      TRACE_ERR("Memcpy protocol type %d, number %d (%s) to optimized protocol\n",
                pami_xfer, opt_proto,
                comm_ptr->mpid.coll_metadata[pami_xfer][0][opt_proto].name);
      comm_ptr->mpid.opt_protocol[pami_xfer][proto_num] =
      comm_ptr->mpid.coll_algorithm[pami_xfer][0][opt_proto];
      memcpy(&comm_ptr->mpid.opt_protocol_md[pami_xfer][proto_num],
             &comm_ptr->mpid.coll_metadata[pami_xfer][0][opt_proto],
             sizeof(pami_metadata_t));
      comm_ptr->mpid.must_query[pami_xfer][proto_num] = query_type;
      if(user_range_hi != -1)
        comm_ptr->mpid.cutoff_size[pami_xfer][proto_num] = user_range_hi;
      else
        comm_ptr->mpid.cutoff_size[pami_xfer][proto_num] = 0;
      comm_ptr->mpid.user_selected_type[pami_xfer] = MPID_COLL_OPTIMIZED;
    }
    else /* see if it is in the must query list instead */
    {
      for(i = 0; i <comm_ptr->mpid.coll_count[pami_xfer][1]; i++)
      {
        if(strcasecmp(comm_ptr->mpid.coll_metadata[pami_xfer][1][i].name, protocol_name) == 0)
        {
          opt_proto = i;
          break;
        }
      }
      if(opt_proto != -1) /* ok, it was in the must query list */
      {
        TRACE_ERR("Memcpy protocol type %d, number %d (%s) to optimized protocol\n",
                  pami_xfer, opt_proto,
                  comm_ptr->mpid.coll_metadata[pami_xfer][1][opt_proto].name);
        comm_ptr->mpid.opt_protocol[pami_xfer][proto_num] =
        comm_ptr->mpid.coll_algorithm[pami_xfer][1][opt_proto];
        memcpy(&comm_ptr->mpid.opt_protocol_md[pami_xfer][proto_num],
               &comm_ptr->mpid.coll_metadata[pami_xfer][1][opt_proto],
               sizeof(pami_metadata_t));
        comm_ptr->mpid.must_query[pami_xfer][proto_num] = query_type;
        if(user_range_hi != -1)
          comm_ptr->mpid.cutoff_size[pami_xfer][proto_num] = user_range_hi;
        else
          comm_ptr->mpid.cutoff_size[pami_xfer][proto_num] = 0;
        comm_ptr->mpid.user_selected_type[pami_xfer] = MPID_COLL_OPTIMIZED;
      }
      else /* that protocol doesn't exist */
      {
        TRACE_ERR("Couldn't find %s in the list for %s, reverting to MPICH\n",protocol_name,coll_name);
        comm_ptr->mpid.user_selected_type[pami_xfer] = MPID_COLL_USE_MPICH;
      }
    }
  }
  else if(check_var == 0)/* The env var was set, but wasn't set for coll_name */
  {
    TRACE_ERR("Couldn't find any optimal %s protocols or user chose not to set it. Selecting MPICH\n",coll_name);
    comm_ptr->mpid.user_selected_type[pami_xfer] = MPID_COLL_USE_MPICH;
  }
  else
    return; 
}


void MPIDI_Comm_coll_select(MPID_Comm *comm_ptr)
{
  TRACE_ERR("Entering MPIDI_Comm_coll_select\n");
  int opt_proto = -1;
  int mustquery = 0;
  int i;
  int use_threaded_collectives = 1;

  /* Some highly optimized protocols (limited resource) do not
     support MPI_THREAD_MULTIPLE semantics so do not enable them
     except on COMM_WORLD.
     NOTE: we are not checking metadata because these are known,
     hardcoded optimized protocols.
  */
  if((MPIR_ThreadInfo.thread_provided == MPI_THREAD_MULTIPLE) &&
     (comm_ptr != MPIR_Process.comm_world)) use_threaded_collectives = 0;
  if(unlikely(MPIDI_Process.verbose >= MPIDI_VERBOSE_DETAILS_0 && comm_ptr->rank == 0))
    fprintf(stderr, "thread_provided=%s, %scomm_world, use_threaded_collectives %u\n", 
            MPIR_ThreadInfo.thread_provided == MPI_THREAD_MULTIPLE? "MPI_THREAD_MULTIPLE":
            MPIR_ThreadInfo.thread_provided == MPI_THREAD_SINGLE?"MPI_THREAD_SINGLE":
            MPIR_ThreadInfo.thread_provided == MPI_THREAD_FUNNELED?"MPI_THREAD_FUNNELED":
            MPIR_ThreadInfo.thread_provided == MPI_THREAD_SERIALIZED?"MPI_THREAD_SERIALIZED":
            "??", 
            (comm_ptr != MPIR_Process.comm_world)?"!":"",
            use_threaded_collectives);

  /* First, setup the (easy, allreduce is complicated) FCA collectives if there 
   * are any because they are always usable when they are on */
  if(comm_ptr->mpid.user_selected_type[PAMI_XFER_REDUCE] == MPID_COLL_NOSELECTION)
  {
    MPIDI_Coll_comm_check_FCA("REDUCE","I1:Reduce:FCA:FCA",PAMI_XFER_REDUCE,MPID_COLL_CHECK_FN_REQUIRED, 0, comm_ptr);
  }
  if(comm_ptr->mpid.user_selected_type[PAMI_XFER_ALLGATHER] == MPID_COLL_NOSELECTION)
  {
    MPIDI_Coll_comm_check_FCA("ALLGATHER","I1:Allgather:FCA:FCA",PAMI_XFER_ALLGATHER,MPID_COLL_NOQUERY, 0, comm_ptr);
  }
  if(comm_ptr->mpid.user_selected_type[PAMI_XFER_ALLGATHERV_INT] == MPID_COLL_NOSELECTION)
  {
    MPIDI_Coll_comm_check_FCA("ALLGATHERV","I1:AllgathervInt:FCA:FCA",PAMI_XFER_ALLGATHERV_INT,MPID_COLL_NOQUERY, 0, comm_ptr);
  }
  if(comm_ptr->mpid.user_selected_type[PAMI_XFER_BROADCAST] == MPID_COLL_NOSELECTION)
  {
    MPIDI_Coll_comm_check_FCA("BCAST", "I1:Broadcast:FCA:FCA", PAMI_XFER_BROADCAST, MPID_COLL_NOQUERY, 0, comm_ptr);
    MPIDI_Coll_comm_check_FCA("BCAST", "I1:Broadcast:FCA:FCA", PAMI_XFER_BROADCAST, MPID_COLL_NOQUERY, 1, comm_ptr);
  }
  if(comm_ptr->mpid.user_selected_type[PAMI_XFER_BARRIER] == MPID_COLL_NOSELECTION)
  {
    MPIDI_Coll_comm_check_FCA("BARRIER","I1:Barrier:FCA:FCA",PAMI_XFER_BARRIER,MPID_COLL_NOQUERY, 0, comm_ptr);
  }
  /* SSS: There isn't really an FCA Gatherv protocol. We do this call to force the use of MPICH for gatherv
   * when FCA is enabled so we don't have to use PAMI protocol.  */
  if(comm_ptr->mpid.user_selected_type[PAMI_XFER_GATHERV_INT] == MPID_COLL_NOSELECTION)
  {
    MPIDI_Coll_comm_check_FCA("GATHERV","I1:GathervInt:FCA:FCA",PAMI_XFER_GATHERV_INT,MPID_COLL_NOQUERY, 0, comm_ptr);
  }

  opt_proto = -1;
  mustquery = 0;
  /* So, several protocols are really easy. Tackle them first. */
  if(comm_ptr->mpid.user_selected_type[PAMI_XFER_ALLGATHERV_INT] == MPID_COLL_NOSELECTION)
  {
    TRACE_ERR("No allgatherv[int] env var, so setting optimized allgatherv[int]\n");
    /* Use I0:RectangleDput */
    for(i = 0; i < comm_ptr->mpid.coll_count[PAMI_XFER_ALLGATHERV_INT][1]; i++)
    {
      if(strcasecmp(comm_ptr->mpid.coll_metadata[PAMI_XFER_ALLGATHERV_INT][1][i].name, "I0:RectangleDput:SHMEM:MU") == 0)
      {
        opt_proto = i;
        mustquery = 1;
        break;
      }
    }
    if(opt_proto != -1)
    {
      TRACE_ERR("Memcpy protocol type %d number %d (%s) to optimized protocol\n",
                PAMI_XFER_ALLGATHERV_INT, opt_proto,
                comm_ptr->mpid.coll_metadata[PAMI_XFER_ALLGATHERV_INT][mustquery][opt_proto].name);
      comm_ptr->mpid.opt_protocol[PAMI_XFER_ALLGATHERV_INT][0] =
      comm_ptr->mpid.coll_algorithm[PAMI_XFER_ALLGATHERV_INT][mustquery][opt_proto];
      memcpy(&comm_ptr->mpid.opt_protocol_md[PAMI_XFER_ALLGATHERV_INT][0], 
             &comm_ptr->mpid.coll_metadata[PAMI_XFER_ALLGATHERV_INT][mustquery][opt_proto], 
             sizeof(pami_metadata_t));
      comm_ptr->mpid.must_query[PAMI_XFER_ALLGATHERV_INT][0] = mustquery?MPID_COLL_ALWAYS_QUERY:MPID_COLL_NOQUERY;
      comm_ptr->mpid.user_selected_type[PAMI_XFER_ALLGATHERV_INT] = MPID_COLL_OPTIMIZED;
    }
    else /* no optimized allgatherv? */
    {
      TRACE_ERR("Couldn't find optimial allgatherv[int] protocol\n");
      comm_ptr->mpid.user_selected_type[PAMI_XFER_ALLGATHERV_INT] = MPID_COLL_USE_MPICH;
      comm_ptr->mpid.opt_protocol[PAMI_XFER_ALLGATHERV_INT][0] = 0;
      comm_ptr->mpid.allgathervs[0] = 1; /* Use GLUE_ALLREDUCE */
    }
    TRACE_ERR("Done setting optimized allgatherv[int]\n");
  }

  if(comm_ptr->mpid.user_selected_type[PAMI_XFER_GATHER] == MPID_COLL_NOSELECTION)
  {
    TRACE_ERR("Default gather to  MPICH\n");
    comm_ptr->mpid.user_selected_type[PAMI_XFER_GATHER] = MPID_COLL_USE_MPICH;
    comm_ptr->mpid.opt_protocol[PAMI_XFER_GATHER][0] = 0;
  }

  opt_proto = -1;
  mustquery = 0;
  /* Scatter */
  /* If the user has forced a selection, don't bother setting it here */
  if(comm_ptr->mpid.user_selected_type[PAMI_XFER_SCATTER] == MPID_COLL_NOSELECTION)
  {
    TRACE_ERR("No scatter env var, so setting optimized scatter\n");
    /* The best scatter seems to be I0:Binomial (at least better than I0:Flat) */
    /* Loop over the protocols until we find the one we want */
      for(i = 0; i < comm_ptr->mpid.coll_count[PAMI_XFER_SCATTER][0]; i++)
      {
        if(strcasecmp(comm_ptr->mpid.coll_metadata[PAMI_XFER_SCATTER][0][i].name, "I0:Binomial:-:MU") == 0)
        {
          opt_proto = i;
          break;
        }
        if(strcasecmp(comm_ptr->mpid.coll_metadata[PAMI_XFER_SCATTER][0][i].name, "I0:Binomial:SHMEM:MU") == 0)
        {
          opt_proto = i;
          break;
        }
      }
    if(opt_proto == -1) /* check other list */
      for(i = 0; i < comm_ptr->mpid.coll_count[PAMI_XFER_SCATTER][1]; i++)
      {
        if(strcasecmp(comm_ptr->mpid.coll_metadata[PAMI_XFER_SCATTER][1][i].name, "I0:Binomial:-:MU") == 0)
        {
          opt_proto = i;
          mustquery = 1;
          break;
        }
        if(strcasecmp(comm_ptr->mpid.coll_metadata[PAMI_XFER_SCATTER][1][i].name, "I0:Binomial:SHMEM:MU") == 0)
        {
          opt_proto = i;
          mustquery = 1;
          break;
        }
      }  
    /* Actually, MPICH looks better right now, so use MPICH 
    if(opt_proto != -1)
    {
      TRACE_ERR("Memcpy protocol type %d, number %d (%s) to optimized protocol\n",
                PAMI_XFER_SCATTER, opt_proto, 
                comm_ptr->mpid.coll_metadata[PAMI_XFER_SCATTER][mustquery][opt_proto].name);
      comm_ptr->mpid.opt_protocol[PAMI_XFER_SCATTER][0] =
      comm_ptr->mpid.coll_algorithm[PAMI_XFER_SCATTER][mustquery][opt_proto];
      memcpy(&comm_ptr->mpid.opt_protocol_md[PAMI_XFER_SCATTER][0], 
             &comm_ptr->mpid.coll_metadata[PAMI_XFER_SCATTER][mustquery][opt_proto], 
             sizeof(pami_metadata_t));
      comm_ptr->mpid.must_query[PAMI_XFER_SCATTER][0] = mustquery?MPID_COLL_ALWAYS_QUERY:MPID_COLL_NOQUERY;
      comm_ptr->mpid.user_selected_type[PAMI_XFER_SCATTER] = MPID_COLL_OPTIMIZED;
    }
    else
    */
    {
      TRACE_ERR("Scatter reverting to MPICH\n");
      comm_ptr->mpid.user_selected_type[PAMI_XFER_SCATTER] = MPID_COLL_USE_MPICH;
      comm_ptr->mpid.opt_protocol[PAMI_XFER_SCATTER][0] = 0;
    }
    TRACE_ERR("Done setting optimized scatter\n");
  }
  opt_proto = -1;
  mustquery = 0;
  /* Scatterv */
  /* If the user has forced a selection, don't bother setting it here */
  if(comm_ptr->mpid.user_selected_type[PAMI_XFER_SCATTERV_INT] == MPID_COLL_NOSELECTION)
  {
    TRACE_ERR("No scatterv env var, so setting optimized scatterv\n");
    /* The best scatterv seems to be I0:SoftwareTree:  */
    /* Loop over the protocols until we find the one we want */
      for(i = 0; i < comm_ptr->mpid.coll_count[PAMI_XFER_SCATTERV_INT][0]; i++)
      {
	  if(strcasecmp(comm_ptr->mpid.coll_metadata[PAMI_XFER_SCATTERV_INT][0][i].name, "I0:SoftwareTree:-:MU") == 0)
        {
          opt_proto = i;
          break;
        }
	  if(strcasecmp(comm_ptr->mpid.coll_metadata[PAMI_XFER_SCATTERV_INT][0][i].name, "I0:SoftwareTree:SHMEM:MU") == 0)
        {
          opt_proto = i;
          break;
        }
      }
    if(opt_proto == -1) /* check other list */
      for(i = 0; i < comm_ptr->mpid.coll_count[PAMI_XFER_SCATTERV_INT][1]; i++)
      {
        if(strcasecmp(comm_ptr->mpid.coll_metadata[PAMI_XFER_SCATTERV_INT][1][i].name, "I0:SoftwareTree:-:MU") == 0)
        {
          opt_proto = i;
          mustquery = 1;
          break;
        }
        if(strcasecmp(comm_ptr->mpid.coll_metadata[PAMI_XFER_SCATTERV_INT][1][i].name, "I0:SoftwareTree:SHMEM:MU") == 0)
        {
          opt_proto = i;
          mustquery = 1;
          break;
        }
      }  
    if(opt_proto != -1)
    {
      TRACE_ERR("Memcpy protocol type %d, number %d (%s) to optimized protocol\n",
                PAMI_XFER_SCATTERV_INT, opt_proto, 
                comm_ptr->mpid.coll_metadata[PAMI_XFER_SCATTERV_INT][mustquery][opt_proto].name);
       comm_ptr->mpid.opt_protocol[PAMI_XFER_SCATTERV_INT][0] =
      comm_ptr->mpid.coll_algorithm[PAMI_XFER_SCATTERV_INT][mustquery][opt_proto];
      memcpy(&comm_ptr->mpid.opt_protocol_md[PAMI_XFER_SCATTERV_INT][0], 
             &comm_ptr->mpid.coll_metadata[PAMI_XFER_SCATTERV_INT][mustquery][opt_proto], 
             sizeof(pami_metadata_t));
      comm_ptr->mpid.must_query[PAMI_XFER_SCATTERV_INT][0] = mustquery?MPID_COLL_ALWAYS_QUERY:MPID_COLL_NOQUERY;
      comm_ptr->mpid.user_selected_type[PAMI_XFER_SCATTERV_INT] = MPID_COLL_OPTIMIZED;
    }
    else
    {
      TRACE_ERR("Scatterv reverting to MPICH\n");
      comm_ptr->mpid.user_selected_type[PAMI_XFER_SCATTERV_INT] = MPID_COLL_USE_MPICH;
      comm_ptr->mpid.opt_protocol[PAMI_XFER_SCATTERV_INT][0] = 0;
    }
    TRACE_ERR("Done setting optimized scatterv\n");
  }


  opt_proto = -1;
  mustquery = 0;
  /* Alltoall */
  /* If the user has forced a selection, don't bother setting it here */
  if(comm_ptr->mpid.user_selected_type[PAMI_XFER_ALLTOALL] == MPID_COLL_NOSELECTION)
  {
    TRACE_ERR("No alltoall env var, so setting optimized alltoall\n");
    /* The best alltoall is always I0:M2MComposite:MU:MU */
    /* Loop over the protocols until we find the one we want */
    if(use_threaded_collectives)
      for(i = 0; i < comm_ptr->mpid.coll_count[PAMI_XFER_ALLTOALL][0]; i++)
      {
        if(strcasecmp(comm_ptr->mpid.coll_metadata[PAMI_XFER_ALLTOALL][0][i].name, "I0:M2MComposite:MU:MU") == 0)
        {
          opt_proto = i;
          break;
        }
      }
    if(use_threaded_collectives && (opt_proto == -1)) /* check other list */
      for(i = 0; i < comm_ptr->mpid.coll_count[PAMI_XFER_ALLTOALL][1]; i++)
      {
        if(strcasecmp(comm_ptr->mpid.coll_metadata[PAMI_XFER_ALLTOALL][1][i].name, "I0:M2MComposite:MU:MU") == 0)
        {
          opt_proto = i;
          mustquery = 1;
          break;
        }
      }    
    if(opt_proto != -1)
    {
      TRACE_ERR("Memcpy protocol type %d, number %d (%s) to optimized protocol\n",
                PAMI_XFER_ALLTOALL, opt_proto, 
                comm_ptr->mpid.coll_metadata[PAMI_XFER_ALLTOALL][mustquery][opt_proto].name);
      comm_ptr->mpid.opt_protocol[PAMI_XFER_ALLTOALL][0] =
      comm_ptr->mpid.coll_algorithm[PAMI_XFER_ALLTOALL][mustquery][opt_proto];
      memcpy(&comm_ptr->mpid.opt_protocol_md[PAMI_XFER_ALLTOALL][0], 
             &comm_ptr->mpid.coll_metadata[PAMI_XFER_ALLTOALL][mustquery][opt_proto], 
             sizeof(pami_metadata_t));
      comm_ptr->mpid.must_query[PAMI_XFER_ALLTOALL][0] = mustquery?MPID_COLL_ALWAYS_QUERY:MPID_COLL_NOQUERY;
      comm_ptr->mpid.user_selected_type[PAMI_XFER_ALLTOALL] = MPID_COLL_OPTIMIZED;
    }
    else
    {
      TRACE_ERR("Couldn't find I0:M2MComposite:MU:MU in the list for alltoall, reverting to MPICH\n");
      comm_ptr->mpid.user_selected_type[PAMI_XFER_ALLTOALL] = MPID_COLL_USE_MPICH;
      comm_ptr->mpid.opt_protocol[PAMI_XFER_ALLTOALL][0] = 0;
    }
    TRACE_ERR("Done setting optimized alltoall\n");
  }


  opt_proto = -1;
  mustquery = 0;
  /* Alltoallv */
  if(comm_ptr->mpid.user_selected_type[PAMI_XFER_ALLTOALLV_INT] == MPID_COLL_NOSELECTION)
  {
    TRACE_ERR("No alltoallv env var, so setting optimized alltoallv\n");
    /* The best alltoall is always I0:M2MComposite:MU:MU */
    /* Loop over the protocols until we find the one we want */
    if(use_threaded_collectives)
      for(i = 0; i < comm_ptr->mpid.coll_count[PAMI_XFER_ALLTOALLV_INT][0]; i++)
      {
        if(strcasecmp(comm_ptr->mpid.coll_metadata[PAMI_XFER_ALLTOALLV_INT][0][i].name, "I0:M2MComposite:MU:MU") == 0)
        {
          opt_proto = i;
          break;
        }
      }
    if(use_threaded_collectives && (opt_proto == -1)) /* check other list */
      for(i = 0; i <comm_ptr->mpid.coll_count[PAMI_XFER_ALLTOALLV_INT][1]; i++)
      {
        if(strcasecmp(comm_ptr->mpid.coll_metadata[PAMI_XFER_ALLTOALLV_INT][1][i].name, "I0:M2MComposite:MU:MU") == 0)
        {
          opt_proto = i;
          mustquery = 1;
          break;
        }
      }
    if(opt_proto != -1)
    {
      TRACE_ERR("Memcpy protocol type %d, number %d (%s) to optimized protocol\n",
                PAMI_XFER_ALLTOALLV_INT, opt_proto, 
                comm_ptr->mpid.coll_metadata[PAMI_XFER_ALLTOALLV_INT][mustquery][opt_proto].name);
      comm_ptr->mpid.opt_protocol[PAMI_XFER_ALLTOALLV_INT][0] =
      comm_ptr->mpid.coll_algorithm[PAMI_XFER_ALLTOALLV_INT][mustquery][opt_proto];
      memcpy(&comm_ptr->mpid.opt_protocol_md[PAMI_XFER_ALLTOALLV_INT][0], 
             &comm_ptr->mpid.coll_metadata[PAMI_XFER_ALLTOALLV_INT][mustquery][opt_proto], 
             sizeof(pami_metadata_t));
      comm_ptr->mpid.must_query[PAMI_XFER_ALLTOALLV_INT][0] = mustquery?MPID_COLL_ALWAYS_QUERY:MPID_COLL_NOQUERY;
      comm_ptr->mpid.user_selected_type[PAMI_XFER_ALLTOALLV_INT] = MPID_COLL_OPTIMIZED;
    }
    else
    {
      TRACE_ERR("Couldn't find I0:M2MComposite:MU:MU in the list for alltoallv, reverting to MPICH\n");
      comm_ptr->mpid.user_selected_type[PAMI_XFER_ALLTOALLV_INT] = MPID_COLL_USE_MPICH;
      comm_ptr->mpid.opt_protocol[PAMI_XFER_ALLTOALLV_INT][0] = 0;
    }
    TRACE_ERR("Done setting optimized alltoallv\n");
  }

  opt_proto = -1;
  mustquery = 0;
  /* Barrier */
  if(comm_ptr->mpid.user_selected_type[PAMI_XFER_BARRIER] == MPID_COLL_NOSELECTION)
  {
    TRACE_ERR("No barrier env var, so setting optimized barrier\n");
    /* For 1ppn, I0:MultiSync:-:GI is best, followed by
     * I0:RectangleMultiSync:-:MU, followed by
     * I0:OptBinomial:P2P:P2P
     */
    /* For 16 and 64 ppn, I0:MultiSync2Device:SHMEM:GI (which doesn't exist at 1ppn)
    * is best, followed by
    * I0:RectangleMultiSync2Device:SHMEM:MU for rectangles, followed by
    * I0:OptBinomial:P2P:P2P */
    /* So we basically check for the protocols in reverse-optimal order */


    /* In order, >1ppn we use
     * I0:MultiSync2Device:SHMEM:GI
     * I0:RectangleMultiSync2Device:SHMEM:MU
     * I0:OptBinomial:P2P:P2P
     * MPICH
     *
     * In order 1ppn we use
     * I0:MultiSync:-:GI
     * I0:RectangleMultiSync:-:MU
     * I0:OptBinomial:P2P:P2P
     * MPICH
     */
    if(use_threaded_collectives)
      for(i = 0 ; i < comm_ptr->mpid.coll_count[PAMI_XFER_BARRIER][0]; i++)
      {
        /* These two are mutually exclusive */
        if(strcasecmp(comm_ptr->mpid.coll_metadata[PAMI_XFER_BARRIER][0][i].name, "I0:MultiSync2Device:SHMEM:GI") == 0)
        {
          opt_proto = i;
        }
        if(strcasecmp(comm_ptr->mpid.coll_metadata[PAMI_XFER_BARRIER][0][i].name, "I0:MultiSync:-:GI") == 0)
        {
          opt_proto = i;
        }
      }
    /* Next best rectangular to check */
    if(opt_proto == -1)
    {
      if(use_threaded_collectives)
        for(i = 0 ; i < comm_ptr->mpid.coll_count[PAMI_XFER_BARRIER][0]; i++)
        {
          if(strcasecmp(comm_ptr->mpid.coll_metadata[PAMI_XFER_BARRIER][0][i].name, "I0:RectangleMultiSync2Device:SHMEM:MU") == 0)
            opt_proto = i;
          if(strcasecmp(comm_ptr->mpid.coll_metadata[PAMI_XFER_BARRIER][0][i].name, "I0:RectangleMultiSync:-:MU") == 0)
            opt_proto = i;
        }
    }
    /* Finally, see if we have opt binomial */
    if(opt_proto == -1)
    {
      for(i = 0 ; i < comm_ptr->mpid.coll_count[PAMI_XFER_BARRIER][0]; i++)
      {
        if(strcasecmp(comm_ptr->mpid.coll_metadata[PAMI_XFER_BARRIER][0][i].name, "I0:OptBinomial:P2P:P2P") == 0)
          opt_proto = i;
      }
    }

    if(opt_proto != -1)
    {
      TRACE_ERR("Memcpy protocol type %d, number %d (%s) to optimize protocol\n",
                PAMI_XFER_BARRIER, opt_proto, 
                comm_ptr->mpid.coll_metadata[PAMI_XFER_BARRIER][0][opt_proto].name);

      comm_ptr->mpid.opt_protocol[PAMI_XFER_BARRIER][0] =
      comm_ptr->mpid.coll_algorithm[PAMI_XFER_BARRIER][0][opt_proto]; 
      memcpy(&comm_ptr->mpid.opt_protocol_md[PAMI_XFER_BARRIER][0], 
             &comm_ptr->mpid.coll_metadata[PAMI_XFER_BARRIER][0][opt_proto], 
             sizeof(pami_metadata_t));
      comm_ptr->mpid.must_query[PAMI_XFER_BARRIER][0] = MPID_COLL_NOQUERY;
      comm_ptr->mpid.user_selected_type[PAMI_XFER_BARRIER] = MPID_COLL_OPTIMIZED;
    }
    else
    {
      TRACE_ERR("Couldn't find any optimal barrier protocols. Selecting MPICH\n");
      comm_ptr->mpid.user_selected_type[PAMI_XFER_BARRIER] = MPID_COLL_USE_MPICH;
      comm_ptr->mpid.opt_protocol[PAMI_XFER_BARRIER][0] = 0;
    }

    TRACE_ERR("Done setting optimized barrier\n");
  }

  opt_proto = -1;
  mustquery = 0;

  /* This becomes messy when we have to message sizes. If we were gutting the 
   * existing framework, it might be easier, but I think the existing framework
   * is useful for future collective selection libraries/mechanisms, so I'd
   * rather leave it in place and deal with it here instead.... */

  /* Broadcast */
  /* 1ppn */
  /* small messages: I0:MulticastDput:-:MU
   * if it exists, I0:RectangleDput:MU:MU for >64k */

  /* 16ppn */
  /* small messages: I0:MultiCastDput:SHMEM:MU
   * for 16ppn/1node: I0:MultiCastDput:SHMEM:- perhaps
   * I0:RectangleDput:SHMEM:MU for >128k */
  /* nonrect(?): I0:MultiCast2DeviceDput:SHMEM:MU 
   * no hw: I0:Sync2-nary:Shmem:MUDput */
  /* 64ppn */
  /* all sizes: I0:MultiCastDput:SHMEM:MU */
  /* otherwise, I0:2-nomial:SHMEM:MU */



  /* First, set up small message bcasts */
  if(comm_ptr->mpid.user_selected_type[PAMI_XFER_BROADCAST] == MPID_COLL_NOSELECTION)
  {
    /* Complicated exceptions: */
    /* I0:RankBased_Binomial:-:ShortMU is good on irregular for <256 bytes */
    /* I0:MultiCast:SHMEM:- is good at 1 node/16ppn, which is a SOW point */
    TRACE_ERR("No bcast env var, so setting optimized bcast\n");

    if(use_threaded_collectives)
      for(i = 0 ; i < comm_ptr->mpid.coll_count[PAMI_XFER_BROADCAST][1]; i++)
      {
        /* These two are mutually exclusive */
        if(strcasecmp(comm_ptr->mpid.coll_metadata[PAMI_XFER_BROADCAST][1][i].name, "I0:MultiCastDput:-:MU") == 0)
        {
          opt_proto = i;
          mustquery = 1;
          break;
        }
        if(strcasecmp(comm_ptr->mpid.coll_metadata[PAMI_XFER_BROADCAST][1][i].name, "I0:MultiCastDput:SHMEM:MU") == 0)
        {
          opt_proto = i;
          mustquery = 1;
          break;
        }
      }
    /* Next best MU 2 device to check */
    if(use_threaded_collectives)
      if(opt_proto == -1)
      {
        if(use_threaded_collectives)
          for(i = 0; i < comm_ptr->mpid.coll_count[PAMI_XFER_BROADCAST][1]; i++)
          {
            if(strcasecmp(comm_ptr->mpid.coll_metadata[PAMI_XFER_BROADCAST][1][i].name, "I0:MultiCast2DeviceDput:SHMEM:MU") == 0)
            {
              opt_proto = i;
              mustquery = 1;
              break;
            }
          }
      }
      /* Check for  rectangle */
    if(use_threaded_collectives)
      if(opt_proto == -1)
      {
        unsigned len = strlen("I0:RectangleDput:");
        if(use_threaded_collectives)
          for(i = 0; i < comm_ptr->mpid.coll_count[PAMI_XFER_BROADCAST][1]; i++)
          {
            if(strcasecmp (comm_ptr->mpid.coll_metadata[PAMI_XFER_BROADCAST][1][i].name, "I0:RectangleDput:SHMEM:MU") == 0)
            { /* Prefer the :SHMEM:MU so break when it's found */
              opt_proto = i; 
              mustquery = 1;
              break;
            }
            /* Otherwise any RectangleDput is better than nothing. */
            if(strncasecmp(comm_ptr->mpid.coll_metadata[PAMI_XFER_BROADCAST][1][i].name, "I0:RectangleDput:",len) == 0)
            {
              opt_proto = i;
              mustquery = 1;
            }
          }
      }
    if(opt_proto == -1)
    {
      /* this protocol is sometimes query, sometimes always works so check both lists but prefer always works */
      for(i = 0; i < comm_ptr->mpid.coll_count[PAMI_XFER_BROADCAST][0]; i++)
      {
        /* This is a good choice for small messages only */
        if(strcasecmp(comm_ptr->mpid.coll_metadata[PAMI_XFER_BROADCAST][0][i].name, "I0:SequenceBased_Binomial:SHMEM:MU") == 0)
        {
          opt_proto = i;
          comm_ptr->mpid.cutoff_size[PAMI_XFER_BROADCAST][0] = 256;
          break;
        }
      }
      if(opt_proto == -1) for(i = 0; i < comm_ptr->mpid.coll_count[PAMI_XFER_BROADCAST][1]; i++)
      {
        /* This is a good choice for small messages only */
        if(strcasecmp(comm_ptr->mpid.coll_metadata[PAMI_XFER_BROADCAST][1][i].name, "I0:SequenceBased_Binomial:SHMEM:MU") == 0)
        {
          opt_proto = i;
          mustquery = 1;
          comm_ptr->mpid.cutoff_size[PAMI_XFER_BROADCAST][0] = 256;
          break;
        }
      }
    }
    /* Next best to check */
    if(opt_proto == -1)
    {
      /* this protocol is sometimes query, sometimes always works so check both lists but prefer always works */
      for(i = 0; i < comm_ptr->mpid.coll_count[PAMI_XFER_BROADCAST][0]; i++)
      {
        if(strcasecmp(comm_ptr->mpid.coll_metadata[PAMI_XFER_BROADCAST][0][i].name, "I0:2-nomial:SHMEM:MU") == 0)
          opt_proto = i;
      }
      if(opt_proto == -1) for(i = 0; i < comm_ptr->mpid.coll_count[PAMI_XFER_BROADCAST][1]; i++)
      {
        if(strcasecmp(comm_ptr->mpid.coll_metadata[PAMI_XFER_BROADCAST][1][i].name, "I0:2-nomial:SHMEM:MU") == 0)
          opt_proto = i;
          mustquery = 1;
      }
    }

    /* These protocols are good for most message sizes, but there are some
     * better choices for larger messages */
    /* Set opt_proto for bcast[0] right now */
    if(opt_proto != -1)
    {
      TRACE_ERR("Memcpy protocol type %d, number %d (%s) to optimize protocol 0\n",
                PAMI_XFER_BROADCAST, opt_proto, 
                comm_ptr->mpid.coll_metadata[PAMI_XFER_BROADCAST][mustquery][opt_proto].name);

      comm_ptr->mpid.opt_protocol[PAMI_XFER_BROADCAST][0] = 
      comm_ptr->mpid.coll_algorithm[PAMI_XFER_BROADCAST][mustquery][opt_proto];
      memcpy(&comm_ptr->mpid.opt_protocol_md[PAMI_XFER_BROADCAST][0], 
             &comm_ptr->mpid.coll_metadata[PAMI_XFER_BROADCAST][mustquery][opt_proto], 
             sizeof(pami_metadata_t));
      comm_ptr->mpid.must_query[PAMI_XFER_BROADCAST][0] = mustquery?MPID_COLL_ALWAYS_QUERY:MPID_COLL_NOQUERY;
      comm_ptr->mpid.user_selected_type[PAMI_XFER_BROADCAST] = MPID_COLL_OPTIMIZED;
    }
    else
    {
      TRACE_ERR("Couldn't find any optimal bcast protocols. Selecting MPICH\n");
      comm_ptr->mpid.user_selected_type[PAMI_XFER_BROADCAST] = MPID_COLL_USE_MPICH;
      comm_ptr->mpid.opt_protocol[PAMI_XFER_BROADCAST][0] = 0;
    }

    TRACE_ERR("Done setting optimized bcast 0\n");

    /* Now, look into large message bcasts */
    opt_proto = -1;
    mustquery = 0;
    /* If bcast0 is I0:MultiCastDput:-:MU, and I0:RectangleDput:MU:MU is available, use
     * it for 64k messages */
    if(comm_ptr->mpid.user_selected_type[PAMI_XFER_BROADCAST] != MPID_COLL_USE_MPICH)
    {
      if(use_threaded_collectives)
        if(strcasecmp(comm_ptr->mpid.opt_protocol_md[PAMI_XFER_BROADCAST][0].name, "I0:MultiCastDput:-:MU") == 0)
        {
          /* See if I0:RectangleDput:MU:MU is available */
          for(i = 0; i < comm_ptr->mpid.coll_count[PAMI_XFER_BROADCAST][1]; i++)
          {
            if(strcasecmp(comm_ptr->mpid.coll_metadata[PAMI_XFER_BROADCAST][1][i].name, "I0:RectangleDput:MU:MU") == 0)
            {
              opt_proto = i;
              mustquery = 1;
              comm_ptr->mpid.cutoff_size[PAMI_XFER_BROADCAST][0] = 65536;
              break;
            }
          }
        }
        /* small messages: I0:MultiCastDput:SHMEM:MU*/
        /* I0:RectangleDput:SHMEM:MU for >128k */
      if(use_threaded_collectives)
        if(strcasecmp(comm_ptr->mpid.opt_protocol_md[PAMI_XFER_BROADCAST][0].name, "I0:MultiCastDput:SHMEM:MU") == 0)
        {
          /* See if I0:RectangleDput:SHMEM:MU is available */
          for(i = 0; i < comm_ptr->mpid.coll_count[PAMI_XFER_BROADCAST][1]; i++)
          {
            if(strcasecmp(comm_ptr->mpid.coll_metadata[PAMI_XFER_BROADCAST][1][i].name, "I0:RectangleDput:SHMEM:MU") == 0)
            {
              opt_proto = i;
              mustquery = 1;
              comm_ptr->mpid.cutoff_size[PAMI_XFER_BROADCAST][0] = 131072;
              break;
            }
          }
        }
      if(strcasecmp(comm_ptr->mpid.opt_protocol_md[PAMI_XFER_BROADCAST][0].name, "I0:SequenceBased_Binomial:SHMEM:MU") == 0)
      {
        /* This protocol was only good for up to 256, and it was an irregular, so let's set
         * 2-nomial for larger message sizes. Cutoff should have already been set to 256 too */
        /* this protocol is sometimes query, sometimes always works so check both lists but prefer always works */
        for(i = 0; i < comm_ptr->mpid.coll_count[PAMI_XFER_BROADCAST][0]; i++)
        {
          if(strcasecmp(comm_ptr->mpid.coll_metadata[PAMI_XFER_BROADCAST][0][i].name, "I0:2-nomial:SHMEM:MU") == 0)
            opt_proto = i;
        }
        if(opt_proto == -1) for(i = 0; i < comm_ptr->mpid.coll_count[PAMI_XFER_BROADCAST][1]; i++)
        {
          if(strcasecmp(comm_ptr->mpid.coll_metadata[PAMI_XFER_BROADCAST][1][i].name, "I0:2-nomial:SHMEM:MU") == 0)
            opt_proto = i;
            mustquery = 1;
        }
      }

      if(opt_proto != -1)
      {
        if(MPIDI_Process.verbose >= MPIDI_VERBOSE_DETAILS_0 && comm_ptr->rank == 0)
        {
          fprintf(stderr,"Selecting %s as optimal broadcast 1 (above %d)\n", 
                  comm_ptr->mpid.coll_metadata[PAMI_XFER_BROADCAST][mustquery][opt_proto].name, 
                  comm_ptr->mpid.cutoff_size[PAMI_XFER_BROADCAST][0]);
        }
        TRACE_ERR("Memcpy protocol type %d, number %d (%s) to optimize protocol 1 (above %d)\n",
                  PAMI_XFER_BROADCAST, opt_proto, 
                  comm_ptr->mpid.coll_metadata[PAMI_XFER_BROADCAST][mustquery][opt_proto].name,
                  comm_ptr->mpid.cutoff_size[PAMI_XFER_BROADCAST][0]);

        comm_ptr->mpid.opt_protocol[PAMI_XFER_BROADCAST][1] =
        comm_ptr->mpid.coll_algorithm[PAMI_XFER_BROADCAST][mustquery][opt_proto];
        memcpy(&comm_ptr->mpid.opt_protocol_md[PAMI_XFER_BROADCAST][1], 
               &comm_ptr->mpid.coll_metadata[PAMI_XFER_BROADCAST][mustquery][opt_proto], 
               sizeof(pami_metadata_t));
        comm_ptr->mpid.must_query[PAMI_XFER_BROADCAST][1] = mustquery?MPID_COLL_ALWAYS_QUERY:MPID_COLL_NOQUERY;
        /* This should already be set... */
        /* comm_ptr->mpid.user_selected_type[PAMI_XFER_BROADCAST] = MPID_COLL_OPTIMIZED; */
      }
      else
      {
        TRACE_ERR("Secondary bcast protocols unavilable; using primary for all sizes\n");

        TRACE_ERR("Duplicating protocol type %d, number %d (%s) to optimize protocol 1 (above %d)\n",
                  PAMI_XFER_BROADCAST, 0, 
                  comm_ptr->mpid.opt_protocol_md[PAMI_XFER_BROADCAST][0].name,
                  comm_ptr->mpid.cutoff_size[PAMI_XFER_BROADCAST][0]);

        comm_ptr->mpid.opt_protocol[PAMI_XFER_BROADCAST][1] = 
        comm_ptr->mpid.opt_protocol[PAMI_XFER_BROADCAST][0];
        memcpy(&comm_ptr->mpid.opt_protocol_md[PAMI_XFER_BROADCAST][1], 
               &comm_ptr->mpid.opt_protocol_md[PAMI_XFER_BROADCAST][0],
               sizeof(pami_metadata_t));
        comm_ptr->mpid.must_query[PAMI_XFER_BROADCAST][1] = comm_ptr->mpid.must_query[PAMI_XFER_BROADCAST][0];
      }
    }
    TRACE_ERR("Done with bcast protocol selection\n");
  }

  opt_proto = -1;
  mustquery = 0;
  /* The most fun... allreduce */
  /* 512-way data: */
  /* For starters, Amith's protocol works on doubles on sum/min/max. Because
   * those are targetted types/ops, we will pre-cache it.
   * That protocol works on ints, up to 8k/ppn max message size. We'll precache 
   * it too
   */

  if(comm_ptr->mpid.user_selected_type[PAMI_XFER_ALLREDUCE] == MPID_COLL_NOSELECTION)
  {
    /* the user hasn't selected a protocol, so we can NULL the protocol/metadatas */
    comm_ptr->mpid.query_cached_allreduce = MPID_COLL_USE_MPICH;

    comm_ptr->mpid.cutoff_size[PAMI_XFER_ALLREDUCE][0] = 0;
    comm_ptr->mpid.must_query[PAMI_XFER_ALLREDUCE][0] = MPID_COLL_USE_MPICH;
    comm_ptr->mpid.must_query[PAMI_XFER_ALLREDUCE][1] = MPID_COLL_USE_MPICH;
    /* For BGQ */
    /*  1ppn: I0:MultiCombineDput:-:MU if it is available, but it has a check_fn
     *  since it is MU-based*/
    /*  Next best is I1:ShortAllreduce:P2P:P2P for short messages, then MPICH is best*/
    /*  I0:MultiCombineDput:-:MU could be used in the i/dsmm cached protocols, so we'll do that */
    /*  First, look in the 'must query' list and see i I0:MultiCombine:Dput:-:MU is there */

    char *pname="...none...";
    int user_range_hi = -1;
    int fca_enabled = 0;
    if((fca_enabled = MPIDI_Check_FCA_envvar("ALLREDUCE", &user_range_hi)) == 1)
      pname = "I1:Allreduce:FCA:FCA";
    else if(use_threaded_collectives)
      pname = "I0:MultiCombineDput:-:MU";
    /*SSS: Any "MU" protocol will not be available on non-BG systems. I just need to check for FCA in the 
                 first if only. No need to do another check since the second if will never succeed for PE systems*/
    for(i = 0; i < comm_ptr->mpid.coll_count[PAMI_XFER_ALLREDUCE][1]; i++)
    {
      if(strcasecmp(comm_ptr->mpid.coll_metadata[PAMI_XFER_ALLREDUCE][1][i].name, pname) == 0)
      {
        /* So, this should be fine for the i/dsmm protocols. everything else needs to call the check function */
        /* This also works for all message sizes, so no need to deal with it specially for query */
        comm_ptr->mpid.cached_allreduce = 
        comm_ptr->mpid.coll_algorithm[PAMI_XFER_ALLREDUCE][1][i];
        memcpy(&comm_ptr->mpid.cached_allreduce_md,
               &comm_ptr->mpid.coll_metadata[PAMI_XFER_ALLREDUCE][1][i],
               sizeof(pami_metadata_t));
        comm_ptr->mpid.query_cached_allreduce = MPID_COLL_QUERY;

        comm_ptr->mpid.user_selected_type[PAMI_XFER_ALLREDUCE] = MPID_COLL_OPTIMIZED;
        if(fca_enabled && user_range_hi != -1)
          comm_ptr->mpid.cutoff_size[PAMI_XFER_ALLREDUCE][0] = user_range_hi;
        opt_proto = i;

      }
      if(use_threaded_collectives)
        if(strcasecmp(comm_ptr->mpid.coll_metadata[PAMI_XFER_ALLREDUCE][1][i].name, "I0:MultiCombineDput:SHMEM:MU") == 0)
        {
          /* This works well for doubles sum/min/max but has trouble with int > 8k/ppn */
          comm_ptr->mpid.cached_allreduce =
          comm_ptr->mpid.coll_algorithm[PAMI_XFER_ALLREDUCE][1][i];
          memcpy(&comm_ptr->mpid.cached_allreduce_md,
                 &comm_ptr->mpid.coll_metadata[PAMI_XFER_ALLREDUCE][1][i],
                 sizeof(pami_metadata_t));
          comm_ptr->mpid.query_cached_allreduce = MPID_COLL_CHECK_FN_REQUIRED;

          comm_ptr->mpid.user_selected_type[PAMI_XFER_ALLREDUCE] = MPID_COLL_OPTIMIZED;
          opt_proto = i;
        }
    }
    /* At this point, if opt_proto != -1, we have must-query protocols in the i/dsmm caches */
    /* We should pick a backup, non-must query */
    /* I0:ShortAllreduce:P2P:P2P < 128, then mpich*/

    /*SSS: ShortAllreduce is available on both BG and PE. I have to pick just one to check for in this case. 
                However, I need to add FCA for both opt_protocol[0]and[1] to cover all data sizes*/
    if(fca_enabled == 1)
      pname = "I1:Allreduce:FCA:FCA";
    else
      pname = "I1:ShortAllreduce:P2P:P2P";

    for(i = 0; i < comm_ptr->mpid.coll_count[PAMI_XFER_ALLREDUCE][1]; i++)
    {
      if(strcasecmp(comm_ptr->mpid.coll_metadata[PAMI_XFER_ALLREDUCE][1][i].name, pname) == 0)
      {
        comm_ptr->mpid.opt_protocol[PAMI_XFER_ALLREDUCE][0] =
        comm_ptr->mpid.coll_algorithm[PAMI_XFER_ALLREDUCE][1][i];
        memcpy(&comm_ptr->mpid.opt_protocol_md[PAMI_XFER_ALLREDUCE][0],
               &comm_ptr->mpid.coll_metadata[PAMI_XFER_ALLREDUCE][1][i],
               sizeof(pami_metadata_t));
        if(fca_enabled == 1)
        {
          comm_ptr->mpid.must_query[PAMI_XFER_ALLREDUCE][0] = MPID_COLL_CHECK_FN_REQUIRED;
          if(user_range_hi != -1)
            comm_ptr->mpid.cutoff_size[PAMI_XFER_ALLREDUCE][0] = user_range_hi;
          /*SSS: Otherwise another protocol may get selected in mpido_allreduce if we don't set this flag here*/
          comm_ptr->mpid.must_query[PAMI_XFER_ALLREDUCE][1] = MPID_COLL_CHECK_FN_REQUIRED;
        }
        else
        {
          /* Short is good for up to 512 bytes... but it's a query protocol */
          comm_ptr->mpid.must_query[PAMI_XFER_ALLREDUCE][0] = MPID_COLL_QUERY;
          /* MPICH above that ... when short query fails */
          comm_ptr->mpid.must_query[PAMI_XFER_ALLREDUCE][1] = MPID_COLL_USE_MPICH;
        }
        comm_ptr->mpid.user_selected_type[PAMI_XFER_ALLREDUCE] = MPID_COLL_OPTIMIZED;

        opt_proto = i;
      }
    }
    if(opt_proto == -1)
    {
      if(MPIDI_Process.verbose >= MPIDI_VERBOSE_DETAILS_0 && comm_ptr->rank == 0)
        fprintf(stderr,"Optimized allreduce falls back to MPICH\n");
      comm_ptr->mpid.must_query[PAMI_XFER_ALLREDUCE][0] = MPID_COLL_USE_MPICH;
      comm_ptr->mpid.must_query[PAMI_XFER_ALLREDUCE][1] = MPID_COLL_USE_MPICH;
    }
    TRACE_ERR("Done setting optimized allreduce protocols\n");
  }


  if(MPIDI_Process.optimized.select_colls != 2)
  {
    for(i = 0; i < PAMI_XFER_COUNT; i++)
    {
      if(i == PAMI_XFER_AMBROADCAST || i == PAMI_XFER_AMSCATTER ||
         i == PAMI_XFER_AMGATHER || i == PAMI_XFER_AMREDUCE)
        continue;
      if(comm_ptr->mpid.user_selected_type[i] != MPID_COLL_OPTIMIZED)
      {
        if(MPIDI_Process.verbose >= MPIDI_VERBOSE_DETAILS_0 && comm_ptr->rank == 0)
          fprintf(stderr, "Collective wasn't selected for type %d,using MPICH (comm %p)\n", i, comm_ptr);
        comm_ptr->mpid.user_selected_type[i] = MPID_COLL_USE_MPICH;
      }
    }
  }


  if(MPIDI_Process.verbose >= MPIDI_VERBOSE_DETAILS_0 && comm_ptr->rank == 0)
  {
    if(comm_ptr->mpid.user_selected_type[PAMI_XFER_BARRIER] == MPID_COLL_OPTIMIZED)
      fprintf(stderr,"Selecting %s for opt barrier comm %p\n", comm_ptr->mpid.opt_protocol_md[PAMI_XFER_BARRIER][0].name, comm_ptr);
    if(comm_ptr->mpid.user_selected_type[PAMI_XFER_ALLGATHERV_INT] == MPID_COLL_OPTIMIZED)
      fprintf(stderr,"Selecting %s for opt allgatherv comm %p\n", comm_ptr->mpid.opt_protocol_md[PAMI_XFER_ALLGATHERV_INT][0].name, comm_ptr);
    if(comm_ptr->mpid.must_query[PAMI_XFER_ALLGATHERV_INT][0] == MPID_COLL_USE_MPICH)
      fprintf(stderr,"Selecting MPICH for allgatherv below %d size comm %p\n", comm_ptr->mpid.cutoff_size[PAMI_XFER_ALLGATHERV_INT][0], comm_ptr);
    if(comm_ptr->mpid.user_selected_type[PAMI_XFER_GATHER] == MPID_COLL_USE_MPICH)
      fprintf(stderr,"Selecting MPICH for gather comm %p\n", comm_ptr);
    if(comm_ptr->mpid.user_selected_type[PAMI_XFER_SCATTER] == MPID_COLL_USE_MPICH)
      fprintf(stderr,"Selecting MPICH for scatter comm %p\n", comm_ptr);
    else if(comm_ptr->mpid.user_selected_type[PAMI_XFER_SCATTER] == MPID_COLL_OPTIMIZED)
      fprintf(stderr,"Selecting %s for scatter comm %p\n", 
	      comm_ptr->mpid.opt_protocol_md[PAMI_XFER_SCATTER][0].name,
	      comm_ptr);
    if(comm_ptr->mpid.user_selected_type[PAMI_XFER_SCATTERV_INT] == MPID_COLL_USE_MPICH)
      fprintf(stderr,"Selecting MPICH for scatterv comm %p\n", comm_ptr);
    else if(comm_ptr->mpid.user_selected_type[PAMI_XFER_SCATTERV_INT] == MPID_COLL_OPTIMIZED)
      fprintf(stderr,"Selecting %s for scatterv comm %p\n", 
	      comm_ptr->mpid.opt_protocol_md[PAMI_XFER_SCATTERV_INT][0].name,
	      comm_ptr);
    if(comm_ptr->mpid.user_selected_type[PAMI_XFER_BROADCAST] == MPID_COLL_OPTIMIZED)
      fprintf(stderr,"Selecting %s for opt bcast up to size %d comm %p\n", comm_ptr->mpid.opt_protocol_md[PAMI_XFER_BROADCAST][0].name,
              comm_ptr->mpid.cutoff_size[PAMI_XFER_BROADCAST][0], comm_ptr);
    if((comm_ptr->mpid.must_query[PAMI_XFER_BROADCAST][1] == MPID_COLL_NOQUERY) ||
       (comm_ptr->mpid.must_query[PAMI_XFER_BROADCAST][1] == MPID_COLL_ALWAYS_QUERY))
      fprintf(stderr,"Selecting %s (mustquery=%d) for opt bcast above size %d comm %p\n",
              comm_ptr->mpid.opt_protocol_md[PAMI_XFER_BROADCAST][1].name,
              comm_ptr->mpid.must_query[PAMI_XFER_BROADCAST][1],
              comm_ptr->mpid.cutoff_size[PAMI_XFER_BROADCAST][0], comm_ptr);
    if(comm_ptr->mpid.user_selected_type[PAMI_XFER_ALLTOALLV_INT] == MPID_COLL_OPTIMIZED)
      fprintf(stderr,"Selecting %s for opt alltoallv comm %p\n", comm_ptr->mpid.opt_protocol_md[PAMI_XFER_ALLTOALLV_INT][0].name, comm_ptr);
    if(comm_ptr->mpid.user_selected_type[PAMI_XFER_ALLTOALL] == MPID_COLL_OPTIMIZED)
      fprintf(stderr,"Selecting %s for opt alltoall comm %p\n", comm_ptr->mpid.opt_protocol_md[PAMI_XFER_ALLTOALL][0].name, comm_ptr);
    if(comm_ptr->mpid.must_query[PAMI_XFER_ALLREDUCE][0] == MPID_COLL_USE_MPICH)
      fprintf(stderr,"Selecting MPICH for allreduce below %d size comm %p\n", comm_ptr->mpid.cutoff_size[PAMI_XFER_ALLREDUCE][0], comm_ptr);
    else
    {
      if(comm_ptr->mpid.query_cached_allreduce != MPID_COLL_USE_MPICH)
      {
        fprintf(stderr,"Selecting %s for double sum/min/max ops allreduce, query: %d comm %p\n",
                comm_ptr->mpid.cached_allreduce_md.name, comm_ptr->mpid.query_cached_allreduce, comm_ptr);
      }
      fprintf(stderr,"Selecting %s for other operations allreduce up to %d comm %p\n",
              comm_ptr->mpid.opt_protocol_md[PAMI_XFER_ALLREDUCE][0].name, 
              comm_ptr->mpid.cutoff_size[PAMI_XFER_ALLREDUCE][0], comm_ptr);
    }
    if(comm_ptr->mpid.must_query[PAMI_XFER_ALLREDUCE][1] == MPID_COLL_USE_MPICH)
      fprintf(stderr,"Selecting MPICH for allreduce above %d size comm %p\n", comm_ptr->mpid.cutoff_size[PAMI_XFER_ALLREDUCE][0], comm_ptr);
    else
    {
      if(comm_ptr->mpid.query_cached_allreduce != MPID_COLL_USE_MPICH)
      {
        fprintf(stderr,"Selecting %s for double sum/min/max ops allreduce, above %d query: %d comm %p\n",
                comm_ptr->mpid.cached_allreduce_md.name, 
                comm_ptr->mpid.cutoff_size[PAMI_XFER_ALLREDUCE][0],
                comm_ptr->mpid.query_cached_allreduce, comm_ptr);
      }
      else
      {
        fprintf(stderr,"Selecting MPICH for double sum/min/max ops allreduce, above %d size comm %p\n",
                comm_ptr->mpid.cutoff_size[PAMI_XFER_ALLREDUCE][0], comm_ptr);
      }
      fprintf(stderr,"Selecting %s for other operations allreduce over %d comm %p\n",
              comm_ptr->mpid.opt_protocol_md[PAMI_XFER_ALLREDUCE][1].name,
              comm_ptr->mpid.cutoff_size[PAMI_XFER_ALLREDUCE][0], comm_ptr);
    }
  }

  TRACE_ERR("Leaving MPIDI_Comm_coll_select\n");

}


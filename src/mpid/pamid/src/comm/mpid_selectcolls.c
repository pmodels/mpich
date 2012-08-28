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
 * \file src/comm/mpid_collselect.c
 * \brief Code for setting up collectives per geometry/communicator
 */

/*#define TRACE_ON */

#include <mpidimpl.h>

static char* MPIDI_Coll_type_name(int i)
{
   switch(i)
   {
      case 0: return("Broadcast");
      case 1: return("Allreduce");
      case 2: return("Reduce");
      case 3: return("Allgather");
      case 4: return("Allgatherv");
      case 5: return("Allgatherv_int");
      case 6: return("Scatter");
      case 7: return("Scatterv");
      case 8: return("Scatterv_int");
      case 9: return("Gather");
      case 10: return("Gatherv");
      case 11: return("Gatherv_int");
      case 12: return("Barrier");
      case 13: return("Alltoall");
      case 14: return("Alltoallv");
      case 15: return("Alltoallv_int");
      case 16: return("Scan");
      case 17: return("Reduce_scatter");
      default: return("AM Collective");
   }
}
static void MPIDI_Update_coll(pami_algorithm_t coll, 
                              int type,
                              int index,
                              MPID_Comm *comm);

static void MPIDI_Update_coll(pami_algorithm_t coll, 
                       int type,  /* must query vs always works */
                       int index,
                       MPID_Comm *comm)
{

   comm->mpid.user_selected_type[coll] = type;
   TRACE_ERR("Update_coll for protocol %s, type: %d index: %d\n", 
      comm->mpid.coll_metadata[coll][type][index].name, type, index);

   /* Are we in the 'must query' list? If so determine how "bad" it is */
   if(type == MPID_COLL_QUERY)
   {
      /* First, is a check always required? */
      if(comm->mpid.coll_metadata[coll][type][index].check_correct.values.checkrequired)
      {
         TRACE_ERR("Protocol %s check_fn required always\n", comm->mpid.coll_metadata[coll][type][index].name);
         /* We must have a check_fn */
         MPID_assert_always(comm->mpid.coll_metadata[coll][type][index].check_fn !=NULL);
         comm->mpid.user_selected_type[coll] = MPID_COLL_CHECK_FN_REQUIRED;
      }
      else if(comm->mpid.coll_metadata[coll][type][index].check_fn != NULL)
      {
         /* For now, if there's a check_fn we will always call it and not cache.
            We *could* be smarter about this eventually.                        */
         TRACE_ERR("Protocol %s setting to always query\n", comm->mpid.coll_metadata[coll][type][index].name);
         comm->mpid.user_selected_type[coll] = MPID_COLL_ALWAYS_QUERY;
      }

   }

   comm->mpid.user_selected[coll] = 
      comm->mpid.coll_algorithm[coll][type][index];

   memcpy(&comm->mpid.user_metadata[coll],
          &comm->mpid.coll_metadata[coll][type][index],
          sizeof(pami_metadata_t));
}
   
static void MPIDI_Check_preallreduce(char *env, MPID_Comm *comm, char *name, int constant)
{
   /* If a given protocol only requires a check for nonlocal conditions and preallreduce
    * is turned off, we can consider it a always works protocol instead 
    */
   char *envopts = getenv(env);
   if(envopts != NULL)
   {
      if(strncasecmp(env, "N", 1) == 0)
      {
         if(MPIDI_Process.verbose >= MPIDI_VERBOSE_DETAILS_0 && comm->rank == 0)
            fprintf(stderr,"Bypassing breallreduce for %s\n", name);
         comm->mpid.preallreduces[constant] = 0;
      }
   }
}
static int MPIDI_Check_protocols(char *names[], MPID_Comm *comm, char *name, int constant)
{
   int i = 0;
   char *envopts;
   for(;; ++i) 
   {
      if(names[i] == NULL)
         return 0;
      envopts = getenv(names[i]);
      if(envopts != NULL)
         break;
   }
   /* Now, change it if we have a match */
   if(envopts != NULL)
   {
      if(MPIDI_Process.verbose >= MPIDI_VERBOSE_DETAILS_0 && comm->rank == 0)
         fprintf(stderr,"Checking %s against known %s protocols\n", envopts, name);
      /* Check if MPICH was specifically requested */
      if(strcasecmp(envopts, "MPICH") == 0)
      {
         TRACE_ERR("Selecting MPICH for %s\n", name);
         if(MPIDI_Process.verbose >= MPIDI_VERBOSE_DETAILS_0 && comm->rank == 0)
            fprintf(stderr,"Selecting MPICH for %d (%s)\n", constant, name);
         comm->mpid.user_selected_type[constant] = MPID_COLL_USE_MPICH;
         comm->mpid.user_selected[constant] = 0;
         return 0;
      }

      for(i=0; i < comm->mpid.coll_count[constant][0];i++)
      {
         if(strncasecmp(envopts, comm->mpid.coll_metadata[constant][0][i].name,strlen(envopts)) == 0)
         {
            MPIDI_Update_coll(constant, MPID_COLL_NOQUERY, i, comm);
            if(MPIDI_Process.verbose >= MPIDI_VERBOSE_DETAILS_0 && comm->rank == 0)
               fprintf(stderr,"setting %s as default %s for comm %p\n", comm->mpid.coll_metadata[constant][0][i].name, name, comm);
            return 0;
         }
      }
      for(i=0; i < comm->mpid.coll_count[constant][1];i++)
      {
         if(strncasecmp(envopts, comm->mpid.coll_metadata[constant][1][i].name,strlen(envopts)) == 0)
         {
            TRACE_ERR("Calling updatecoll...\n");
            MPIDI_Update_coll(constant, MPID_COLL_QUERY, i, comm);
            TRACE_ERR("Done calling update coll\n");
            if(MPIDI_Process.verbose >= MPIDI_VERBOSE_DETAILS_0 && comm->rank == 0)
               fprintf(stderr,"setting (query required protocol) %s as default %s for comm %p\n", comm->mpid.coll_metadata[constant][1][i].name, name, comm);
            return 0;
         }
      }
      /* An envvar was specified, so we should probably use MPICH of we can't find
       * the specified protocol */
      TRACE_ERR("Specified protocol %s was unavailable; using MPICH for %s\n", envopts, name);
      comm->mpid.user_selected_type[constant] = MPID_COLL_USE_MPICH;
      comm->mpid.user_selected[constant] = 0;
      return 0;
   }
   /* Looks like we didn't get anything, set NOSELECTION so automated selection can pick something */
   comm->mpid.user_selected_type[constant] = MPID_COLL_NOSELECTION; 
   comm->mpid.user_selected[constant] = 0;
   return 0;
}

void MPIDI_Comm_coll_envvars(MPID_Comm *comm)
{
   char *envopts;
   int i;
   MPID_assert_always(comm!=NULL);
   TRACE_ERR("MPIDI_Comm_coll_envvars enter\n");

   /* Set up always-works defaults */
   for(i = 0; i < PAMI_XFER_COUNT; i++)
   {
      if(i == PAMI_XFER_AMBROADCAST || i == PAMI_XFER_AMSCATTER ||
         i == PAMI_XFER_AMGATHER || i == PAMI_XFER_AMREDUCE)
         continue;

      /* Initialize to noselection instead of noquery for PE/FCA stuff. Is this the right answer? */
      comm->mpid.user_selected_type[i] = MPID_COLL_NOSELECTION;
         if(MPIDI_Process.verbose >= MPIDI_VERBOSE_DETAILS_0 && comm->rank == 0)
            fprintf(stderr,"Setting up collective %d on comm %p\n", i, comm);
      if(comm->mpid.coll_count[i][0] == 0)
      {
         if(MPIDI_Process.verbose >= MPIDI_VERBOSE_DETAILS_0 && comm->rank == 0)
            fprintf(stderr,"There are no 'always works' protocols of type %d. This could be a problem later in your app\n", i);
         comm->mpid.user_selected_type[i] = MPID_COLL_USE_MPICH;
         comm->mpid.user_selected[i] = 0;
      }
      else
      {
         comm->mpid.user_selected[i] = comm->mpid.coll_algorithm[i][0][0];
         memcpy(&comm->mpid.user_metadata[i], &comm->mpid.coll_metadata[i][0][0],
               sizeof(pami_metadata_t));
      }
   }


   TRACE_ERR("Checking env vars\n");

   MPIDI_Check_preallreduce("PAMID_COLLECTIVE_ALLGATHER_PREALLREDUCE", comm, "allgather",
         MPID_ALLGATHER_PREALLREDUCE);

   MPIDI_Check_preallreduce("PAMID_COLLECTIVE_ALLGATHERV_PREALLREDUCE", comm, "allgatherv",
         MPID_ALLGATHERV_PREALLREDUCE);

   MPIDI_Check_preallreduce("PAMID_COLLECTIVE_ALLREDUCE_PREALLREDUCE", comm, "allreduce",
         MPID_ALLREDUCE_PREALLREDUCE);

   MPIDI_Check_preallreduce("PAMID_COLLECTIVE_BCAST_PREALLREDUCE", comm, "broadcast",
         MPID_BCAST_PREALLREDUCE);

   MPIDI_Check_preallreduce("PAMID_COLLECTIVE_SCATTERV_PREALLREDUCE", comm, "scatterv",
         MPID_SCATTERV_PREALLREDUCE);

   {
      TRACE_ERR("Checking bcast\n");
      char* names[] = {"PAMID_COLLECTIVE_BCAST", "MP_S_MPI_BCAST", NULL};
      MPIDI_Check_protocols(names, comm, "broadcast", PAMI_XFER_BROADCAST);
   }
   {
      TRACE_ERR("Checking allreduce\n");
      char* names[] = {"PAMID_COLLECTIVE_ALLREDUCE", "MP_S_MPI_ALLREDUCE", NULL};
      MPIDI_Check_protocols(names, comm, "allreduce", PAMI_XFER_ALLREDUCE);
   }
   {
      TRACE_ERR("Checking barrier\n");
      char* names[] = {"PAMID_COLLECTIVE_BARRIER", "MP_S_MPI_BARRIER", NULL};
      MPIDI_Check_protocols(names, comm, "barrier", PAMI_XFER_BARRIER);
   }
   {
      TRACE_ERR("Checking alltaoll\n");
      char* names[] = {"PAMID_COLLECTIVE_ALLTOALL", NULL};
      MPIDI_Check_protocols(names, comm, "alltoall", PAMI_XFER_ALLTOALL);
   }
   {
      TRACE_ERR("Checking reduce\n");
      char* names[] = {"PAMID_COLLECTIVE_REDUCE", "MP_S_MPI_REDUCE", NULL};
      MPIDI_Check_protocols(names, comm, "reduce", PAMI_XFER_REDUCE);
   }
   {
      TRACE_ERR("Checking alltoallv\n");
      char* names[] = {"PAMID_COLLECTIVE_ALLTOALLV", NULL};
      MPIDI_Check_protocols(names, comm, "alltoallv", PAMI_XFER_ALLTOALLV_INT);
   }
   {
      TRACE_ERR("Checking gatherv\n");
      char* names[] = {"PAMID_COLLECTIVE_GATHERV",  NULL};
      MPIDI_Check_protocols(names, comm, "gatherv", PAMI_XFER_GATHERV_INT);
   }
   {
      TRACE_ERR("Checking scan\n");
      char* names[] = {"PAMID_COLLECTIVE_SCAN", NULL};
      MPIDI_Check_protocols(names, comm, "scan", PAMI_XFER_SCAN);
   }

   comm->mpid.scattervs[0] = comm->mpid.scattervs[1] = 0;

   TRACE_ERR("Checking scatterv\n");
   envopts = getenv("PAMID_COLLECTIVE_SCATTERV");
   if(envopts != NULL)
   {
      if(strcasecmp(envopts, "GLUE_BCAST") == 0)
      {
         if(MPIDI_Process.verbose >= MPIDI_VERBOSE_DETAILS_0 && comm->rank == 0)
            fprintf(stderr,"Selecting glue bcast for scatterv\n");
         comm->mpid.scattervs[0] = 1;
      }
      else if(strcasecmp(envopts, "GLUE_ALLTOALLV") == 0)
      {
         if(MPIDI_Process.verbose >= MPIDI_VERBOSE_DETAILS_0 && comm->rank == 0)
            fprintf(stderr,"Selecting glue alltoallv for scatterv\n");
         comm->mpid.scattervs[1] = 1;
      }
   }
   { /* In addition to glue protocols, check for other PAMI protocols and check for PE now */
      char* names[] = {"PAMID_COLLECTIVE_SCATTERV", NULL};
      MPIDI_Check_protocols(names, comm, "scatterv", PAMI_XFER_SCATTERV_INT);

      /* Use MPICH on large communicators (Issue 7516 and ticket 595)*/
      if((comm->mpid.user_selected_type[PAMI_XFER_SCATTERV_INT] == 
	  MPID_COLL_NOSELECTION) /* no env var selected */
	 && (comm->local_size > (16*1024))) /* and > 16k ranks */
	{
         comm->mpid.user_selected_type[PAMI_XFER_SCATTERV_INT] = MPID_COLL_USE_MPICH;
         comm->mpid.user_selected[PAMI_XFER_SCATTERV_INT] = 0;
	}
   }
   
   TRACE_ERR("Checking scatter\n");
   comm->mpid.optscatter = 0;
   envopts = getenv("PAMID_COLLECTIVE_SCATTER");
   if(envopts != NULL)
   {
      if(strcasecmp(envopts, "GLUE_BCAST") == 0)
      {
         if(MPIDI_Process.verbose >= MPIDI_VERBOSE_DETAILS_0 && comm->rank == 0)
            fprintf(stderr,"Selecting glue_bcast for scatter\n");
         comm->mpid.optscatter = 1;
      }
   }
   { /* In addition to glue protocols, check for other PAMI protocols and check for PE now */
      char* names[] = {"PAMID_COLLECTIVE_SCATTER", NULL};
      MPIDI_Check_protocols(names, comm, "scatter", PAMI_XFER_SCATTER);
   }

   TRACE_ERR("Checking allgather\n");
   comm->mpid.allgathers[0] = comm->mpid.allgathers[1] = comm->mpid.allgathers[2] = 0;
   envopts = getenv("PAMID_COLLECTIVE_ALLGATHER");
   if(envopts != NULL)
   {
      if(strcasecmp(envopts, "GLUE_ALLREDUCE") == 0)
      {
         if(MPIDI_Process.verbose >= MPIDI_VERBOSE_DETAILS_0 && comm->rank == 0)
            fprintf(stderr,"Selecting glue_allreduce for allgather\n");
         comm->mpid.allgathers[0] = 1;
      }

      else if(strcasecmp(envopts, "GLUE_BCAST") == 0)
      {
         if(MPIDI_Process.verbose >= MPIDI_VERBOSE_DETAILS_0 && comm->rank == 0)
            fprintf(stderr,"Selecting glue_bcast for allgather\n");
         comm->mpid.allgathers[1] = 1;
      }

      else if(strcasecmp(envopts, "GLUE_ALLTOALL") == 0)
      {
         if(MPIDI_Process.verbose >= MPIDI_VERBOSE_DETAILS_0 && comm->rank == 0)
            fprintf(stderr,"Selecting glue_alltoall for allgather\n");
         comm->mpid.allgathers[2] = 1;
      }
   }
   { /* In addition to glue protocols, check for other PAMI protocols and check for PE now */
      char* names[] = {"PAMID_COLLECTIVE_ALLGATHER", "MP_S_MPI_ALLGATHER", NULL};
      MPIDI_Check_protocols(names, comm, "allgather", PAMI_XFER_ALLGATHER);
   }

   TRACE_ERR("Checking allgatherv\n");
   comm->mpid.allgathervs[0] = comm->mpid.allgathervs[1] = comm->mpid.allgathervs[2] = 0;
   envopts = getenv("PAMID_COLLECTIVE_ALLGATHERV");
   if(envopts != NULL)
   {
      if(strcasecmp(envopts, "GLUE_ALLREDUCE") == 0)
      {
         if(MPIDI_Process.verbose >= MPIDI_VERBOSE_DETAILS_0 && comm->rank == 0)
            fprintf(stderr,"Selecting glue_allreduce for allgatherv\n");
         comm->mpid.allgathervs[0] = 1;
      }

      else if(strcasecmp(envopts, "GLUE_BCAST") == 0)
      {
         if(MPIDI_Process.verbose >= MPIDI_VERBOSE_DETAILS_0 && comm->rank == 0)
            fprintf(stderr,"Selecting glue_bcast for allgatherv\n");
         comm->mpid.allgathervs[1] = 1;
      }

      else if(strcasecmp(envopts, "GLUE_ALLTOALL") == 0)
      {
         if(MPIDI_Process.verbose >= MPIDI_VERBOSE_DETAILS_0 && comm->rank == 0)
            fprintf(stderr,"Selecting glue_alltoall for allgatherv\n");
         comm->mpid.allgathervs[2] = 1;
      }
   }
   { /* In addition to glue protocols, check for other PAMI protocols and check for PE now */
      char* names[] = {"PAMID_COLLECTIVE_ALLGATHERV", "MP_S_MPI_ALLGATHERV", NULL};
      MPIDI_Check_protocols(names, comm, "allgatherv", PAMI_XFER_ALLGATHERV_INT);
   }

   TRACE_ERR("CHecking gather\n");
   comm->mpid.optgather = 0;
   envopts = getenv("PAMID_COLLECTIVE_GATHER");
   if(envopts != NULL)
   {
      if(strcasecmp(envopts, "GLUE_REDUCE") == 0)
      {
         if(MPIDI_Process.verbose >= MPIDI_VERBOSE_DETAILS_0 && comm->rank == 0)
            fprintf(stderr,"using glue_reduce for gather\n");
         comm->mpid.optgather = 1;
      }
   }
   { /* In addition to glue protocols, check for other PAMI protocols and check for PE now */
      char* names[] = {"PAMID_COLLECTIVE_GATHER", NULL};
      MPIDI_Check_protocols(names, comm, "gather", PAMI_XFER_GATHER);
   }

   TRACE_ERR("MPIDI_Comm_coll_envvars exit\n");
}


/* Determine how many of each collective type this communicator supports */
void MPIDI_Comm_coll_query(MPID_Comm *comm)
{
   TRACE_ERR("MPIDI_Comm_coll_query enter\n");
   int rc = 0, i, j;
   size_t num_algorithms[2];
   TRACE_ERR("Getting geometry from comm %p\n", comm);
   pami_geometry_t *geom = comm->mpid.geometry;;
   for(i = 0; i < PAMI_XFER_COUNT; i++)
   {
      if(i == PAMI_XFER_AMBROADCAST || i == PAMI_XFER_AMSCATTER ||
         i == PAMI_XFER_AMGATHER || i == PAMI_XFER_AMREDUCE)
         continue;
      rc = PAMI_Geometry_algorithms_num(geom,
                                        i,
                                        num_algorithms);
      TRACE_ERR("Num algorithms of type %d: %zd %zd\n", i, num_algorithms[0], num_algorithms[1]);
      if(rc != PAMI_SUCCESS)
      {
         fprintf(stderr,"PAMI_Geometry_algorithms_num returned %d for type %d\n", rc, i);
         continue;
      }

      comm->mpid.coll_count[i][0] = 0;
      comm->mpid.coll_count[i][1] = 0;
      if(num_algorithms[0] || num_algorithms[1])
      {
         comm->mpid.coll_algorithm[i][0] = (pami_algorithm_t *)
               MPIU_Malloc(sizeof(pami_algorithm_t) * num_algorithms[0]);
         comm->mpid.coll_metadata[i][0] = (pami_metadata_t *)
               MPIU_Malloc(sizeof(pami_metadata_t) * num_algorithms[0]);
         comm->mpid.coll_algorithm[i][1] = (pami_algorithm_t *)
               MPIU_Malloc(sizeof(pami_algorithm_t) * num_algorithms[1]);
         comm->mpid.coll_metadata[i][1] = (pami_metadata_t *)
               MPIU_Malloc(sizeof(pami_metadata_t) * num_algorithms[1]);
         comm->mpid.coll_count[i][0] = num_algorithms[0];
         comm->mpid.coll_count[i][1] = num_algorithms[1];

         /* Despite the bad name, this looks at algorithms associated with
          * the geometry, NOT inherent physical properties of the geometry*/

         /* BES TODO I am assuming all contexts have the same algorithms. Probably
          * need to investigate that assumption
          */
         rc = PAMI_Geometry_algorithms_query(geom,
                                             i,
                                             comm->mpid.coll_algorithm[i][0],
                                             comm->mpid.coll_metadata[i][0],
                                             num_algorithms[0],
                                             comm->mpid.coll_algorithm[i][1],
                                             comm->mpid.coll_metadata[i][1],
                                             num_algorithms[1]);
         if(rc != PAMI_SUCCESS)
         {
            fprintf(stderr,"PAMI_Geometry_algorithms_query returned %d for type %d\n", rc, i);
            continue;
         }

         if(MPIDI_Process.verbose >= MPIDI_VERBOSE_DETAILS_0 && comm->rank == 0)
         {
            for(j = 0; j < num_algorithms[0]; j++)
               fprintf(stderr,"comm[%p] coll type %d (%s), algorithm %d 0: %s\n", comm, i, MPIDI_Coll_type_name(i), j, comm->mpid.coll_metadata[i][0][j].name);
            for(j = 0; j < num_algorithms[1]; j++)
               fprintf(stderr,"comm[%p] coll type %d (%s), algorithm %d 1: %s\n", comm, i, MPIDI_Coll_type_name(i), j, comm->mpid.coll_metadata[i][1][j].name);
            if(i == PAMI_XFER_ALLGATHERV_INT || i == PAMI_XFER_ALLGATHER)
            {
               fprintf(stderr,"comm[%p] coll type %d (%s), \"glue\" algorithm: GLUE_ALLREDUCE\n", comm, i, MPIDI_Coll_type_name(i));
               fprintf(stderr,"comm[%p] coll type %d (%s), \"glue\" algorithm: GLUE_BCAST\n", comm, i, MPIDI_Coll_type_name(i));
               fprintf(stderr,"comm[%p] coll type %d (%s), \"glue\" algorithm: GLUE_ALLTOALL\n", comm, i, MPIDI_Coll_type_name(i));
            }
            if(i == PAMI_XFER_SCATTERV_INT)
            {
               fprintf(stderr,"comm[%p] coll type %d (%s), \"glue\" algorithm: GLUE_BCAST\n", comm, i, MPIDI_Coll_type_name(i));
               fprintf(stderr,"comm[%p] coll type %d (%s), \"glue\" algorithm: GLUE_ALLTOALLV\n", comm, i, MPIDI_Coll_type_name(i));
            }
            if(i == PAMI_XFER_SCATTER)
            {
               fprintf(stderr,"comm[%p] coll type %d (%s), \"glue\" algorithm: GLUE_BCAST\n", comm, i, MPIDI_Coll_type_name(i));
            }
         }
      }
   }
   /* Determine if we have protocols for these maybe, rather than just setting them? */
   comm->coll_fns->Barrier      = MPIDO_Barrier;
   comm->coll_fns->Bcast        = MPIDO_Bcast;
   comm->coll_fns->Allreduce    = MPIDO_Allreduce;
   comm->coll_fns->Allgather    = MPIDO_Allgather;
   comm->coll_fns->Allgatherv   = MPIDO_Allgatherv;
   comm->coll_fns->Scatterv     = MPIDO_Scatterv;
   comm->coll_fns->Scatter      = MPIDO_Scatter;
   comm->coll_fns->Gather       = MPIDO_Gather;
   comm->coll_fns->Alltoallv    = MPIDO_Alltoallv;
   comm->coll_fns->Alltoall     = MPIDO_Alltoall;
   comm->coll_fns->Gatherv      = MPIDO_Gatherv;
   comm->coll_fns->Reduce       = MPIDO_Reduce;
   comm->coll_fns->Scan         = MPIDO_Scan;
   comm->coll_fns->Exscan       = MPIDO_Exscan;

   TRACE_ERR("MPIDI_Comm_coll_query exit\n");
}


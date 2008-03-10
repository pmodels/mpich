/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file src/comm/mpid_comm.c
 * \brief Communicator setup
 */
#include "mpido_coll.h"

/** \brief Hook function to handle communicator-specific optimization (creation) */
void
MPIDI_Comm_create (MPID_Comm *comm)
{
  MPIDI_Coll_Comm_create(comm);
  MPIDI_Topo_Comm_create(comm);
}


/** \brief Hook function to handle communicator-specific optimization (destruction) */
void
MPIDI_Comm_destroy (MPID_Comm *comm)
{
  MPIDI_Coll_Comm_destroy(comm);
  MPIDI_Topo_Comm_destroy(comm);
}


static inline int
ENV_Int(char * env, int * dval)
{
  int result;
  if(env != NULL)
    result = atoi(env);
  else
    result = *dval;
  return *dval = result;
}

static inline int
ENV_Bool(char * env, int * dval)
{
  int result = *dval;
  if(env != NULL)
    {
      if (strcmp(env, "0") == 0)
        result = 0;
      else if  (strcmp(env, "0") == 1)
        result = 1;
    }
  return *dval = result;
}

/** \brief Checks the Environment variables at initialization and stores the results */
void
MPIDI_Env_setup()
{
  int   dval = 0;
  char *envopts;

   /* Initialize selection variables */
   /* turn these off in MPIDI_Coll_register if registration fails 
    * or based on env vars (below) */
   MPIDI_CollectiveProtocols.numcolors = 0;
   MPIDI_CollectiveProtocols.barrier.usegi = 1;
   MPIDI_CollectiveProtocols.barrier.usebinom = 1;
   MPIDI_CollectiveProtocols.optbarrier = 1;
   
   MPIDI_CollectiveProtocols.localbarrier.uselockbox = 1;
   MPIDI_CollectiveProtocols.localbarrier.usebinom = 1;
   
   MPIDI_CollectiveProtocols.broadcast.usetree = 1;
   MPIDI_CollectiveProtocols.broadcast.userect = 1;
   MPIDI_CollectiveProtocols.broadcast.usebinom = 1;
   MPIDI_CollectiveProtocols.optbroadcast = 1;
   
   MPIDI_CollectiveProtocols.allreduce.reusestorage = 1;
   MPIDI_CollectiveProtocols.allreduce.usetree = 1;
   MPIDI_CollectiveProtocols.allreduce.usepipelinedtree = 0; // defaults to off
   MPIDI_CollectiveProtocols.allreduce.useccmitree = 0; // defaults to off
   MPIDI_CollectiveProtocols.allreduce.userect = 1;
   MPIDI_CollectiveProtocols.allreduce.userectring = 0; // defaults to off
   MPIDI_CollectiveProtocols.allreduce.usebinom = 1;
   MPIDI_CollectiveProtocols.optallreduce = 1;
   
   MPIDI_CollectiveProtocols.reduce.reusestorage = 1;
   MPIDI_CollectiveProtocols.reduce.usetree = 1;
   MPIDI_CollectiveProtocols.reduce.useccmitree = 0; // defaults to off
   MPIDI_CollectiveProtocols.reduce.userect = 1;
   MPIDI_CollectiveProtocols.reduce.userectring = 0; // defaults to off
   MPIDI_CollectiveProtocols.reduce.usebinom = 1;
   MPIDI_CollectiveProtocols.optreduce = 1;
   
   MPIDI_CollectiveProtocols.optallgather            = 1;
   MPIDI_CollectiveProtocols.allgather.useallreduce  = 1;
   MPIDI_CollectiveProtocols.allgather.usebcast      = 1;
   MPIDI_CollectiveProtocols.allgather.usealltoallv  = 1;
   MPIDI_CollectiveProtocols.optallgatherv           = 1;
   MPIDI_CollectiveProtocols.allgatherv.useallreduce = 1;
   MPIDI_CollectiveProtocols.allgatherv.usebcast     = 1;
   MPIDI_CollectiveProtocols.allgatherv.usealltoallv = 1;
   
   MPIDI_CollectiveProtocols.alltoallv.usetorus = 1;
   MPIDI_CollectiveProtocols.alltoallw.usetorus = 1;
   MPIDI_CollectiveProtocols.alltoall.usetorus = 1;
   MPIDI_CollectiveProtocols.alltoall.premalloc = 1;
   
  /* Set the verbose level */
  dval = 0;
  ENV_Int(getenv("DCMF_VERBOSE"), &dval);
  MPIDI_Process.verbose = dval;

  /* Enable the statistics  */
  dval = 0;
  ENV_Bool(getenv("DCMF_STATISTICS"), &dval);
  MPIDI_Process.statistics = dval;

  /* Determine eager limit */
  dval = 1200;
  ENV_Int(getenv("DCMF_RVZ"),   &dval);
  ENV_Int(getenv("DCMF_RZV"),   &dval);
  ENV_Int(getenv("DCMF_EAGER"), &dval);
  MPIDI_Process.eager_limit = dval;

  /* Determine interrupt mode */
  dval = 0;
  ENV_Bool(getenv("DCMF_INTERRUPT"),  &dval);
  ENV_Bool(getenv("DCMF_INTERRUPTS"), &dval);
  MPIDI_Process.use_interrupts = dval;

  /* Set the status of the optimized topology functions */
  dval = 1;
  ENV_Bool(getenv("DCMF_TOPOLOGY"), &dval);
  MPIDI_Process.optimized.topology = dval;

  /* Set the status of the optimized collectives */
  dval = 1;
  ENV_Bool(getenv("DCMF_COLLECTIVE"),  &dval);
  ENV_Bool(getenv("DCMF_COLLECTIVES"), &dval);
  MPIDI_Process.optimized.collectives = dval;
  dval = 1000;
  ENV_Int(getenv("DCMF_RMA_PENDING"), &dval);
  MPIDI_Process.rma_pending = dval;

   envopts = getenv("DCMF_BCAST");
   if(envopts != NULL)
   {
      if(strncasecmp(envopts, "M", 1) == 0) /* MPICH */
      {
         MPIDI_CollectiveProtocols.broadcast.usetree = 0;
         MPIDI_CollectiveProtocols.broadcast.userect = 0;
         MPIDI_CollectiveProtocols.broadcast.usebinom = 0;
         MPIDI_CollectiveProtocols.optbroadcast = 0;
      }
      else if(strncasecmp(envopts, "R", 1) == 0) /* Rectangle */
      {
         MPIDI_CollectiveProtocols.broadcast.usetree = 0;
         MPIDI_CollectiveProtocols.broadcast.usebinom = 0;
      }
      else if(strncasecmp(envopts, "B", 1) == 0) /* Binomial */
      {
         MPIDI_CollectiveProtocols.broadcast.usetree = 0;
         MPIDI_CollectiveProtocols.broadcast.userect = 0;
      }
      else if(strncasecmp(envopts, "T", 1) == 0) /* Tree */
      {
         MPIDI_CollectiveProtocols.broadcast.userect = 0;
         MPIDI_CollectiveProtocols.broadcast.usebinom = 0;
      }
      else
         fprintf(stderr,"Invalid DCMF_BCAST option\n");
   }



   envopts = getenv("DCMF_NUMCOLORS");
   if(envopts != NULL)
   {
      int colors = atoi(envopts);
      if(colors < 0 || colors > 3)
         fprintf(stderr,"Invalid DCMF_NUMCOLORS option\n");
      else
         MPIDI_CollectiveProtocols.numcolors = colors;
   }


   envopts = getenv("DCMF_ALLTOALL");
   if(envopts != NULL)
   {
      if(strncasecmp(envopts, "M", 1) == 0) /* MPICH */
         MPIDI_CollectiveProtocols.alltoall.usetorus = 0;
      else if(strncasecmp(envopts, "T", 1) == 0) /* Torus */
      ;
         /* This is on by default in MPIDI_Coll_register */
/*         MPIDI_CollectiveProtocols.alltoall.usetorus = 1; */
      else
         fprintf(stderr,"Invalid DCMF_ALLTOALL option\n");
   }

   envopts = getenv("DCMF_ALLTOALL_PREMALLOC");
   if(envopts == NULL)
      envopts = getenv("DCMF_ALLTOALLV_PREMALLOC");
   if(envopts == NULL)
      envopts = getenv("DCMF_ALLTOALLW_PREMALLOC");
   if(envopts != NULL)
   {  /* Do not reuse the malloc'd storage */
      if(strncasecmp(envopts, "N", 1) == 0)
      {
         MPIDI_CollectiveProtocols.alltoall.premalloc = 0;
      }
      else if(strncasecmp(envopts, "Y", 1) == 0) /* defaults to Y */
      {
         MPIDI_CollectiveProtocols.alltoall.premalloc = 1;
      }
      else
         fprintf(stderr,"Invalid DCMF_ALLTOALL(VW)_PREMALLOC option\n");
   }


   envopts = getenv("DCMF_ALLTOALLV");
   if(envopts != NULL)
   {
      if(strncasecmp(envopts, "M", 1) == 0) /* MPICH */
         MPIDI_CollectiveProtocols.alltoallv.usetorus = 0;
      else if(strncasecmp(envopts, "T", 1) == 0) /* Torus */
         ;
      /* This is on by default in MPIDI_Coll_register */
/*         MPIDI_CollectiveProtocols.alltoallv.usetorus = 1;*/
      else
         fprintf(stderr,"Invalid DCMF_ALLTOALLV option\n");
   }

   envopts = getenv("DCMF_ALLTOALLW");
   if(envopts != NULL)
   {
      if(strncasecmp(envopts, "M", 1) == 0) /* MPICH */
         MPIDI_CollectiveProtocols.alltoallw.usetorus = 0;
      else if(strncasecmp(envopts, "T", 1) == 0) /* Torus */
         ;
      /* This is on by default in MPIDI_Coll_register */
/*         MPIDI_CollectiveProtocols.alltoallw.usetorus = 1;*/
      else
         fprintf(stderr,"Invalid DCMF_ALLTOALLW option\n");
   }

   envopts = getenv("DCMF_ALLGATHER");
   if(envopts != NULL)
     {
       if(strncasecmp(envopts, "M", 1) == 0) /* MPICH */
         MPIDI_CollectiveProtocols.optallgather = 0;
       else if(strncasecmp(envopts, "ALLR", 4) == 0) /* ALLREDUCE */
         {
           MPIDI_CollectiveProtocols.allgather.useallreduce  = 1;
           MPIDI_CollectiveProtocols.allgather.usebcast      = 0;
           MPIDI_CollectiveProtocols.allgather.usealltoallv  = 0;
         }
       else if(strncasecmp(envopts, "BCAST", 1) == 0) /* BCAST */
         {
           MPIDI_CollectiveProtocols.allgather.useallreduce  = 0;
           MPIDI_CollectiveProtocols.allgather.usebcast      = 1;
           MPIDI_CollectiveProtocols.allgather.usealltoallv  = 0;
         }
       else if(strncasecmp(envopts, "ALLT", 4) == 0) /* ALLTOALL */
         {
           MPIDI_CollectiveProtocols.allgather.useallreduce  = 0;
           MPIDI_CollectiveProtocols.allgather.usebcast      = 0;
           MPIDI_CollectiveProtocols.allgather.usealltoallv  = 1;
         }
       else
         fprintf(stderr,"Invalid DCMF_ALLGATHER option\n");
     }

   envopts = getenv("DCMF_ALLGATHERV");
   if(envopts != NULL)
     {
       if(strncasecmp(envopts, "M", 1) == 0) /* MPICH */
         MPIDI_CollectiveProtocols.optallgatherv = 0;
       else if(strncasecmp(envopts, "ALLR", 4) == 0) /* ALLREDUCE */
         {
           MPIDI_CollectiveProtocols.allgatherv.useallreduce  = 1;
           MPIDI_CollectiveProtocols.allgatherv.usebcast      = 0;
           MPIDI_CollectiveProtocols.allgatherv.usealltoallv  = 0;
         }
       else if(strncasecmp(envopts, "BCAST", 1) == 0) /* BCAST */
         {
           MPIDI_CollectiveProtocols.allgatherv.useallreduce  = 0;
           MPIDI_CollectiveProtocols.allgatherv.usebcast      = 1;
           MPIDI_CollectiveProtocols.allgatherv.usealltoallv  = 0;
         }
       else if(strncasecmp(envopts, "ALLT", 4) == 0) /* ALLTOALL */
         {
           MPIDI_CollectiveProtocols.allgatherv.useallreduce  = 0;
           MPIDI_CollectiveProtocols.allgatherv.usebcast      = 0;
           MPIDI_CollectiveProtocols.allgatherv.usealltoallv  = 1;
         }
       else
         fprintf(stderr,"Invalid DCMF_ALLGATHERV option\n");
     }

   envopts = getenv("DCMF_ALLREDUCE");
   if(envopts != NULL)
   {
      if(strncasecmp(envopts, "M", 1) == 0) /* MPICH */
      {
         MPIDI_CollectiveProtocols.allreduce.usetree = 0;
         MPIDI_CollectiveProtocols.allreduce.userect = 0;
         MPIDI_CollectiveProtocols.allreduce.usebinom = 0;
         MPIDI_CollectiveProtocols.optallreduce = 0;
      }
      else if(strncasecmp(envopts, "RI", 2) == 0) /* Rectangle Ring*/
      {
         MPIDI_CollectiveProtocols.allreduce.userectring = 1;// defaults to off
         MPIDI_CollectiveProtocols.allreduce.userect = 0;
         MPIDI_CollectiveProtocols.allreduce.usetree = 0;
         MPIDI_CollectiveProtocols.allreduce.usebinom = 0;
      }
      else if(strncasecmp(envopts, "R", 1) == 0) /* Rectangle */
      {
         MPIDI_CollectiveProtocols.allreduce.usetree = 0;
         MPIDI_CollectiveProtocols.allreduce.usebinom = 0;
      }
      else if(strncasecmp(envopts, "B", 1) == 0) /* Binomial */
      {
         MPIDI_CollectiveProtocols.allreduce.usetree = 0;
         MPIDI_CollectiveProtocols.allreduce.userect = 0;
      }
      else if(strncasecmp(envopts, "T", 1) == 0) /* Tree */
      {
         MPIDI_CollectiveProtocols.allreduce.usebinom = 0;
         MPIDI_CollectiveProtocols.allreduce.userect = 0;
      }
      else if(strncasecmp(envopts, "C", 1) == 0) /* CCMI Tree */
      {
         MPIDI_CollectiveProtocols.allreduce.useccmitree = 1;// defaults to off
         MPIDI_CollectiveProtocols.allreduce.usetree = 0;
         MPIDI_CollectiveProtocols.allreduce.usebinom = 0;
         MPIDI_CollectiveProtocols.allreduce.userect = 0;
      }
      else if(strncasecmp(envopts, "P", 1) == 0) /* CCMI Pipelined Tree */
      {
         MPIDI_CollectiveProtocols.allreduce.usepipelinedtree = 1;// defaults to off
         MPIDI_CollectiveProtocols.allreduce.usetree = 0;
         MPIDI_CollectiveProtocols.allreduce.usebinom = 0;
         MPIDI_CollectiveProtocols.allreduce.userect = 0;
      }
      else
         fprintf(stderr,"Invalid DCMF_ALLREDUCE option\n");
   }

   envopts = getenv("DCMF_ALLREDUCE_REUSE_STORAGE");
   if(envopts != NULL)
   {
      if(strncasecmp(envopts, "N", 1) == 0) /* Do not reuse the malloc'd storage */
      {
         MPIDI_CollectiveProtocols.allreduce.reusestorage = 0;
         fprintf(stderr, "N allreduce.reusestorage %X\n",
                 MPIDI_CollectiveProtocols.allreduce.reusestorage);
      }
      else 
        if(strncasecmp(envopts, "Y", 1) == 0); /* defaults to Y */
      else
         fprintf(stderr,"Invalid DCMF_ALLREDUCE_REUSE_STORAGE option\n");
   }

   envopts = getenv("DCMF_REDUCE");
   if(envopts != NULL)
   {
      if(strncasecmp(envopts, "M", 1) == 0) /* MPICH */
      {
         MPIDI_CollectiveProtocols.reduce.usetree = 0;
         MPIDI_CollectiveProtocols.reduce.userect = 0;
         MPIDI_CollectiveProtocols.reduce.usebinom = 0;
         MPIDI_CollectiveProtocols.optreduce = 0;
      }
      else if(strncasecmp(envopts, "RI", 2) == 0) /* Rectangle Ring*/
      {
         MPIDI_CollectiveProtocols.reduce.userectring = 1;// defaults to off
         MPIDI_CollectiveProtocols.reduce.userect = 0;
         MPIDI_CollectiveProtocols.reduce.usetree = 0;
         MPIDI_CollectiveProtocols.reduce.usebinom = 0;
      }
      else if(strncasecmp(envopts, "R", 1) == 0) /* Rectangle */
      {
         MPIDI_CollectiveProtocols.reduce.usetree = 0;
         MPIDI_CollectiveProtocols.reduce.usebinom = 0;
      }
      else if(strncasecmp(envopts, "B", 1) == 0) /* Binomial */
      {
         MPIDI_CollectiveProtocols.reduce.usetree = 0;
         MPIDI_CollectiveProtocols.reduce.userect = 0;
      }
      else if(strncasecmp(envopts, "T", 1) == 0) /* Tree */
      {
         MPIDI_CollectiveProtocols.reduce.usebinom = 0;
         MPIDI_CollectiveProtocols.reduce.userect = 0;
      }
      else if(strncasecmp(envopts, "C", 1) == 0) /* CCMI Tree */
      {
         MPIDI_CollectiveProtocols.reduce.useccmitree = 1;// defaults to off
         MPIDI_CollectiveProtocols.reduce.usetree = 0;
         MPIDI_CollectiveProtocols.reduce.usebinom = 0;
         MPIDI_CollectiveProtocols.reduce.userect = 0;
      }
      else
         fprintf(stderr,"Invalid DCMF_REDUCE option\n");
   }

   envopts = getenv("DCMF_REDUCE_REUSE_STORAGE");
   if(envopts != NULL)
   {
      if(strncasecmp(envopts, "N", 1) == 0) /* Do not reuse the malloc'd storage */
      {
         MPIDI_CollectiveProtocols.reduce.reusestorage = 0;
         fprintf(stderr, "N protocol.reusestorage %X\n",
                 MPIDI_CollectiveProtocols.reduce.reusestorage);
      }
      else 
        if(strncasecmp(envopts, "Y", 1) == 0); /* defaults to Y */
      else
         fprintf(stderr,"Invalid DCMF_REDUCE_REUSE_STORAGE option\n");
   }

   envopts = getenv("DCMF_BARRIER");
   if(envopts != NULL)
   {
      if(strncasecmp(envopts, "M", 1) == 0) /* MPICH */
      {
         /* still need to register a barrier for DCMF collectives */
         MPIDI_CollectiveProtocols.barrier.usebinom = 1;
         /* MPIDI_Coll_register changes this state for us */
         /* MPIDI_CollectiveProtocols.barrier.usegi = 1; */
         MPIDI_CollectiveProtocols.optbarrier = 0;
      }
      else if(strncasecmp(envopts, "B", 1) == 0) /* Binomial */
      {
         MPIDI_CollectiveProtocols.barrier.usegi = 0;
      }
      else if(strncasecmp(envopts, "G", 1) == 0) /* GI */
      {
         MPIDI_CollectiveProtocols.barrier.usebinom = 0;
      }
      else
         fprintf(stderr,"Invalid DCMF_BARRIER option\n");
   }

   envopts = getenv("DCMF_LOCALBARRIER");
   if(envopts != NULL)
   {
      if(strncasecmp(envopts, "B", 1) == 0) /* Binomial */
      {
         MPIDI_CollectiveProtocols.localbarrier.uselockbox = 0;
      }
      else if(strncasecmp(envopts, "L", 1) == 0) /* Lockbox */
      {
         MPIDI_CollectiveProtocols.localbarrier.usebinom = 0;
      }
      else
         fprintf(stderr,"Invalid DCMF_LOCALBARRIER option\n");
   }
}


unsigned *
MPIDI_Comm_worldranks_init(MPID_Comm *comm_ptr)
{
  unsigned *worldranks = NULL;
  int lrank, numprocs  = comm_ptr->local_size;

  worldranks = comm_ptr->dcmf.worldranks;
  if (worldranks == NULL)
    {
      worldranks = comm_ptr->dcmf.worldranks = MPIU_Malloc(numprocs * sizeof(int));
      MPID_assert(worldranks != NULL);
      for (lrank = 0; lrank < numprocs; lrank++)
        worldranks[lrank] = comm_ptr->vcr[lrank]->lpid;
    }

  return worldranks;
}

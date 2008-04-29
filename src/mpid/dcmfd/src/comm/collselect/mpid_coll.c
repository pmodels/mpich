/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file src/comm/collselect/mpid_coll.c
 * \brief Collective setup
 */
#include "mpido_coll.h"


#warning reasonable hack for now
#define MAXGEOMETRIES 65536


static DCMF_Geometry_t *mpid_geometrytable[MAXGEOMETRIES];
MPIDI_CollectiveProtocol_t MPIDI_CollectiveProtocols;

/*
 * geometries have a 'comm' ID which needs to be equivalently unique as
 * MPIs context_ids. So, we set geometry comm to context_id. Unfortunately
 * there is no trivial way to convert a context_id back to which MPI comm
 * it belongs to so this gross table is here for now. It will be replaced
 * probably with a lazy allocated list. Whatever goes here will have to be
 * cleaned up in comm_destroy as well
 */
static DCMF_Geometry_t *
getGeometryRequest(int comm)
{
   assert(mpid_geometrytable[comm%MAXGEOMETRIES] != NULL);
   return mpid_geometrytable[comm%MAXGEOMETRIES];
}



static int barriers_num=0;
static DCMF_CollectiveProtocol_t *barriers[DCMF_NUM_BARRIER_PROTOCOLS];

static inline int BARRIER_REGISTER(DCMF_Barrier_Protocol proto,
                             DCMF_CollectiveProtocol_t *proto_ptr,
                             DCMF_Barrier_Configuration_t *config)
{
  int rc;
  config->protocol = proto;
  rc = DCMF_Barrier_register(proto_ptr, config);
  if (rc == DCMF_SUCCESS)
    barriers[barriers_num++] = proto_ptr;
  MPID_assert_debug(barriers_num <= DCMF_NUM_BARRIER_PROTOCOLS);
  return rc;
}

static int local_barriers_num=0;
/* Local barriers PLUS room for one standard/global barrier (DCMF_TORUS_BINOMIAL_BARRIER_PROTOCOL)*/
static DCMF_CollectiveProtocol_t *local_barriers[DCMF_NUM_LOCAL_BARRIER_PROTOCOLS+1];

static inline int LOCAL_BARRIER_REGISTER(DCMF_Barrier_Protocol proto,
                                         DCMF_CollectiveProtocol_t *proto_ptr,
                                         DCMF_Barrier_Configuration_t *config)
{
  int rc;
  config->protocol = proto;
  rc = DCMF_Barrier_register(proto_ptr, config);
  if (rc == DCMF_SUCCESS)
    local_barriers[local_barriers_num++] = proto_ptr;
  MPID_assert_debug(local_barriers_num <= DCMF_NUM_LOCAL_BARRIER_PROTOCOLS+1);
  return rc;
}

static inline int BROADCAST_REGISTER(DCMF_Broadcast_Protocol proto,
                              DCMF_CollectiveProtocol_t *proto_ptr,
                              DCMF_Broadcast_Configuration_t *config)
{
   config->protocol = proto;
   return DCMF_Broadcast_register(proto_ptr, config);
}

static inline int ALLREDUCE_REGISTER(DCMF_Allreduce_Protocol proto,
                              DCMF_CollectiveProtocol_t *proto_ptr,
                              DCMF_Allreduce_Configuration_t *config)
{
   config->protocol = proto;
   return DCMF_Allreduce_register(proto_ptr, config);
}

static inline int ALLTOALLV_REGISTER(DCMF_Alltoallv_Protocol proto,
                              DCMF_CollectiveProtocol_t *proto_ptr,
                              DCMF_Alltoallv_Configuration_t *config)
{
   config->protocol = proto;
   return DCMF_Alltoallv_register(proto_ptr, config);
}

static inline int REDUCE_REGISTER(DCMF_Reduce_Protocol proto,
                              DCMF_CollectiveProtocol_t *proto_ptr,
                              DCMF_Reduce_Configuration_t *config)
{
   config->protocol = proto;
   return DCMF_Reduce_register(proto_ptr, config);
}



/** \brief Helper used to register all the collective protocols at initialization */
void MPIDI_Coll_register(void)
{
   DCMF_Barrier_Configuration_t   barrier_config;
   DCMF_Broadcast_Configuration_t broadcast_config;
   DCMF_Allreduce_Configuration_t allreduce_config;
   DCMF_Alltoallv_Configuration_t alltoallv_config;
   DCMF_Reduce_Configuration_t    reduce_config;
   DCMF_GlobalBarrier_Configuration_t gbarrier_config;
   DCMF_GlobalBcast_Configuration_t gbcast_config;
   DCMF_GlobalAllreduce_Configuration_t gallreduce_config;

   DCMF_Result rc;

   /* Register the global functions first */
   
   /* ---------------------------------- */
   /* Register global barrier          */
   /* ---------------------------------- */
   gbarrier_config.protocol = DCMF_GI_GLOBALBARRIER_PROTOCOL;
   rc = DCMF_GlobalBarrier_register(&MPIDI_Protocols.globalbarrier,
                                    &gbarrier_config);
   /* registering the global barrier failed, so don't use it */
   if(rc != DCMF_SUCCESS)
   {
      MPIDI_CollectiveProtocols.barrier.usegi = 0;
   }
   
   
   /* ---------------------------------- */
   /* Register global broadcast          */
   /* ---------------------------------- */
   gbcast_config.protocol = DCMF_TREE_GLOBALBCAST_PROTOCOL;
   rc = DCMF_GlobalBcast_register(&MPIDI_Protocols.globalbcast, &gbcast_config);
   
   /* most likely, we lack shared memory and therefore can't use this */
   if(rc != DCMF_SUCCESS)
   {
      MPIDI_CollectiveProtocols.broadcast.usetree = 0;
   }
   
   /* ---------------------------------- */
   /* Register global allreduce          */
   /* ---------------------------------- */
   gallreduce_config.protocol = DCMF_TREE_GLOBALALLREDUCE_PROTOCOL;
   rc = DCMF_GlobalAllreduce_register(&MPIDI_Protocols.globalallreduce,
                                      &gallreduce_config);
   
   /* most likely, we lack shared memory and therefore can't use this */
   /* reduce uses the allreduce protocol */
   if(rc != DCMF_SUCCESS)
   {
      /* Try the ccmi tree if we were trying global tree */
      MPIDI_CollectiveProtocols.allreduce.useccmitree = MPIDI_CollectiveProtocols.allreduce.usetree;
      MPIDI_CollectiveProtocols.reduce.useccmitree    = MPIDI_CollectiveProtocols.reduce.usetree;
      MPIDI_CollectiveProtocols.allreduce.usetree     = 0;
      MPIDI_CollectiveProtocols.reduce.usetree        = 0;
   }
   

   /* register first barrier protocols now */
   barrier_config.cb_geometry = getGeometryRequest;

   /* set the function that will find the [all]reduce geometry on unexpected callbacks*/
   allreduce_config.cb_geometry = getGeometryRequest;
   reduce_config.cb_geometry = getGeometryRequest;

   /* set configuration flags in the config*/
   allreduce_config.reuse_storage = MPIDI_CollectiveProtocols.allreduce.reusestorage;
   reduce_config.reuse_storage = MPIDI_CollectiveProtocols.reduce.reusestorage;

   /* Other env vars can be checked at communicator creation time
    * but barriers are associated with a geometry and this knowledge
    * isn't available to mpido_barrier
    */
   if(MPIDI_CollectiveProtocols.barrier.usegi)
   {
      if(BARRIER_REGISTER(DCMF_GI_BARRIER_PROTOCOL,
                 &MPIDI_CollectiveProtocols.barrier.gi,
                 &barrier_config) != DCMF_SUCCESS)
         MPIDI_CollectiveProtocols.barrier.usegi = 0;
   }

   /* 
    * Always register a binomial barrier for collectives in subcomms, just
    * choose not to use it at mpido_barrier
    */
   if(BARRIER_REGISTER(DCMF_TORUS_BINOMIAL_BARRIER_PROTOCOL,
              &MPIDI_CollectiveProtocols.barrier.binomial,
              &barrier_config) != DCMF_SUCCESS)
      MPIDI_CollectiveProtocols.barrier.usebinom = 0;

   /* if we don't even get a binomial barrier, we are in trouble */
   MPID_assert_debug(barriers_num >  0);

   /* 
    * Register local barriers for the geometry.  
    * Both a true local lockbox barrier and a global binomial
    * barrier (which can be used non-optimally).  The geometry
    * will decide internally if/which to use.
    * They are not used directly by MPICH but must be initialized.
    */
   if(MPIDI_CollectiveProtocols.localbarrier.uselockbox)
   {
      if(LOCAL_BARRIER_REGISTER(DCMF_LOCKBOX_BARRIER_PROTOCOL,
                 &MPIDI_CollectiveProtocols.localbarrier.lockbox,
                 &barrier_config) != DCMF_SUCCESS)
        MPIDI_CollectiveProtocols.localbarrier.uselockbox = 0;
   }

   /* 
    * Always register a binomial barrier for collectives in subcomms
    */
  if(LOCAL_BARRIER_REGISTER(DCMF_TORUS_BINOMIAL_BARRIER_PROTOCOL,
                &MPIDI_CollectiveProtocols.localbarrier.binomial,
                &barrier_config) != DCMF_SUCCESS)
    MPIDI_CollectiveProtocols.localbarrier.usebinom = 0;

   /* MPID doesn't care if this actually works.  Let someone else 
    * handle problems as needed.  
    * MPID_assert_debug(local_barriers_num >  0); 
    */


   /* Register broadcast protocols */
   if(MPIDI_CollectiveProtocols.broadcast.usetree)
   {
      if(BROADCAST_REGISTER(DCMF_TREE_BROADCAST_PROTOCOL,
                         &MPIDI_CollectiveProtocols.broadcast.tree,
                         &broadcast_config) != DCMF_SUCCESS)
         MPIDI_CollectiveProtocols.broadcast.usetree = 0;
   }

   if(BROADCAST_REGISTER(DCMF_TORUS_RECTANGLE_BROADCAST_PROTOCOL,
                      &MPIDI_CollectiveProtocols.broadcast.rectangle,
                      &broadcast_config) != DCMF_SUCCESS)
      MPIDI_CollectiveProtocols.broadcast.userect = 0;

//   BROADCAST_REGISTER(DCMF_TORUS_RECT_BCAST_3COLOR_PROTOCOL,
//                     &MPIDI_CollectiveProtocols.broadcast.rectangle.threecolor,
//                     &broadcast_config);

   if(BROADCAST_REGISTER(DCMF_TORUS_BINOMIAL_BROADCAST_PROTOCOL,
                      &MPIDI_CollectiveProtocols.broadcast.binomial,
                      &broadcast_config) != DCMF_SUCCESS)
      MPIDI_CollectiveProtocols.broadcast.usebinom = 0;


   /* Register allreduce protocols */
   if(MPIDI_CollectiveProtocols.allreduce.usetree ||
      MPIDI_CollectiveProtocols.allreduce.useccmitree)
   {
      if(ALLREDUCE_REGISTER(DCMF_TREE_ALLREDUCE_PROTOCOL,
                         &MPIDI_CollectiveProtocols.allreduce.tree,
                         &allreduce_config) != DCMF_SUCCESS)
      {
           MPIDI_CollectiveProtocols.allreduce.usetree = 0;
           MPIDI_CollectiveProtocols.allreduce.useccmitree = 0;
      }
   }

   if(ALLREDUCE_REGISTER(DCMF_TREE_PIPELINED_ALLREDUCE_PROTOCOL,
                      &MPIDI_CollectiveProtocols.allreduce.pipelinedtree,
                      &allreduce_config) != DCMF_SUCCESS)
      MPIDI_CollectiveProtocols.allreduce.usepipelinedtree = 0;

   if(ALLREDUCE_REGISTER(DCMF_TORUS_RECTANGLE_ALLREDUCE_PROTOCOL,
                      &MPIDI_CollectiveProtocols.allreduce.rectangle,
                      &allreduce_config) != DCMF_SUCCESS)
      MPIDI_CollectiveProtocols.allreduce.userect = 0;

   if(ALLREDUCE_REGISTER(DCMF_TORUS_RECTANGLE_RING_ALLREDUCE_PROTOCOL,
                      &MPIDI_CollectiveProtocols.allreduce.rectanglering,
                      &allreduce_config) != DCMF_SUCCESS)
      MPIDI_CollectiveProtocols.allreduce.userectring = 0;

   if(ALLREDUCE_REGISTER(DCMF_TORUS_BINOMIAL_ALLREDUCE_PROTOCOL,
                      &MPIDI_CollectiveProtocols.allreduce.binomial,
                      &allreduce_config) != DCMF_SUCCESS)
         MPIDI_CollectiveProtocols.allreduce.usebinom = 0;

   /* Register alltoallv protocols */
   if(ALLTOALLV_REGISTER(DCMF_TORUS_ALLTOALLV_PROTOCOL,
                      &MPIDI_CollectiveProtocols.alltoallv.torus,
                      &alltoallv_config) != DCMF_SUCCESS)
   {
         MPIDI_CollectiveProtocols.alltoallv.usetorus = 0;
         MPIDI_CollectiveProtocols.alltoallw.usetorus = 0;
         MPIDI_CollectiveProtocols.alltoall.usetorus = 0;
   }

   /* Register reduce protocols */
   if(MPIDI_CollectiveProtocols.reduce.usetree ||
      MPIDI_CollectiveProtocols.reduce.useccmitree)
   {
      if(REDUCE_REGISTER(DCMF_TREE_REDUCE_PROTOCOL,
                      &MPIDI_CollectiveProtocols.reduce.tree,
                      &reduce_config) != DCMF_SUCCESS)
      {
              MPIDI_CollectiveProtocols.reduce.usetree = 0;
              MPIDI_CollectiveProtocols.reduce.useccmitree = 0;
      }
   }

   if(REDUCE_REGISTER(DCMF_TORUS_BINOMIAL_REDUCE_PROTOCOL,
                   &MPIDI_CollectiveProtocols.reduce.binomial,
                   &reduce_config) != DCMF_SUCCESS)
         MPIDI_CollectiveProtocols.reduce.usebinom = 0;

   if(REDUCE_REGISTER(DCMF_TORUS_RECTANGLE_REDUCE_PROTOCOL,
                   &MPIDI_CollectiveProtocols.reduce.rectangle,
                   &reduce_config) != DCMF_SUCCESS)
         MPIDI_CollectiveProtocols.reduce.userect = 0;

   if(REDUCE_REGISTER(DCMF_TORUS_RECTANGLE_RING_REDUCE_PROTOCOL,
                   &MPIDI_CollectiveProtocols.reduce.rectanglering,
                   &reduce_config) != DCMF_SUCCESS)
         MPIDI_CollectiveProtocols.reduce.userectring = 0;

}


/**
 * \brief Create collective communicators
 *
 * Hook function to handle collective-specific optimization during communicator creation
 */
void MPIDI_Coll_Comm_create (MPID_Comm *comm)
{
  MPID_assert (comm!= NULL);

  int global=0;
  MPID_Comm *comm_world;

  if (comm->coll_fns) MPIU_Free(comm->coll_fns);
  comm->coll_fns=NULL;   /* !!! Intercomm_merge does not NULL the fcns,
                          * leading to stale functions for new comms.
                          * We'll null it here until argonne confirms
                          * this is the correct behavior of merge
                          */

   /* comm-specific protocol flags */
   comm->dcmf.allreducetree = 1;
   comm->dcmf.reducetree = 1;
   comm->dcmf.allreduceccmitree = 1;
   comm->dcmf.reduceccmitree = 1;
   comm->dcmf.bcasttree = 1; 
   comm->dcmf.alltoalls = 1;

  /* ****************************************** */
  /* Allocate space for the collective pointers */
  /* ****************************************** */

   comm->coll_fns = (MPID_Collops *)MPIU_Malloc(sizeof(MPID_Collops));
   MPID_assert(comm->coll_fns != NULL);
   memset(comm->coll_fns, 0, sizeof(MPID_Collops));


  /* ****************************************** */
  /* Set all defaults                           */
  /* ****************************************** */
  comm->dcmf.worldranks              = NULL;

  /* If we are an intracomm, MPICH should handle */
  if (comm->comm_kind != MPID_INTRACOMM) return;
  /* User may disable all collectives */
  if (!MPIDI_Process.optimized.collectives) return;

   MPID_Comm_get_ptr(MPI_COMM_WORLD, comm_world);
   MPID_assert_debug(comm_world != NULL);

   /* creates ranks array in comm, geometry stores pointer to this array */
   /* should we pass the returned int * to geometry_init? */
  MPIDI_Comm_worldranks_init(comm);



   if(MPIR_ThreadInfo.thread_provided == MPI_THREAD_MULTIPLE)
   {
      if(comm != comm_world)
      {
         global = 0;
         /* alltoall protocols not entirely thread-safe so turn off for this
          * communicator
          */ 
         comm->dcmf.alltoalls = 0;
      }
      /* we are comm_world */
      else
         global = 1;
   }
   else /* single MPI thread. */
   {
      /* and if we are a dupe of comm_world, global context is also safe */
     if(comm->local_size == comm_world->local_size)
       global = 1;
     else
       global = 0;
   }


  /* ****************************************** */
  /* These are ALL the pointers in the object   */
  /* ****************************************** */

      comm->coll_fns->Barrier        = MPIDO_Barrier;
      comm->coll_fns->Bcast          = MPIDO_Bcast;
      comm->coll_fns->Reduce         = MPIDO_Reduce;
      comm->coll_fns->Allreduce      = MPIDO_Allreduce;
      comm->coll_fns->Alltoall       = MPIDO_Alltoall;
      comm->coll_fns->Alltoallv      = MPIDO_Alltoallv;
      comm->coll_fns->Alltoallw       = MPIDO_Alltoallw;
      comm->coll_fns->Allgather      = MPIDO_Allgather;
      comm->coll_fns->Allgatherv     = MPIDO_Allgatherv;
  comm->coll_fns->Gather         = MPIDO_Gather;
  comm->coll_fns->Gatherv        = MPIDO_Gatherv;
  comm->coll_fns->Scatter        = MPIDO_Scatter;
  comm->coll_fns->Scatterv       = MPIDO_Scatterv;
  comm->coll_fns->Reduce_scatter = MPIDO_Reduce_scatter;
  comm->coll_fns->Scan           = MPIDO_Scan;
  comm->coll_fns->Exscan         = MPIDO_Exscan;


   

  /* ******************************************************* */
  /* Setup Barriers and geometry for this communicator       */
  /* ******************************************************* */
  DCMF_Geometry_initialize(
             &comm->dcmf.geometry,
             comm->context_id,
             comm->dcmf.worldranks,
             comm->local_size,
             barriers,
             barriers_num,
             local_barriers,
             local_barriers_num,
             &comm->dcmf.barrier,
             MPIDI_CollectiveProtocols.numcolors,
             global);

//   fprintf(stderr,
//       "context_id: %d, context_id mod MAX: %d\n",
//       comm->context_id, (comm->context_id) % MAXGEOMETRIES);
   mpid_geometrytable[(comm->context_id)%MAXGEOMETRIES] = &comm->dcmf.geometry;


   if((MPIDI_CollectiveProtocols.allreduce.usetree ||
       MPIDI_CollectiveProtocols.allreduce.useccmitree) &&
      !DCMF_Geometry_analyze(&comm->dcmf.geometry,
			     &MPIDI_CollectiveProtocols.allreduce.tree))
   {
       comm->dcmf.allreducetree = 0;
       comm->dcmf.allreduceccmitree = 0;
   }
   else
   {
       comm->dcmf.allreducetree = MPIDI_CollectiveProtocols.allreduce.usetree;
       comm->dcmf.allreduceccmitree = MPIDI_CollectiveProtocols.allreduce.useccmitree;
   }

   if((MPIDI_CollectiveProtocols.reduce.usetree ||
       MPIDI_CollectiveProtocols.reduce.useccmitree) &&
      !DCMF_Geometry_analyze(&comm->dcmf.geometry,
			     &MPIDI_CollectiveProtocols.reduce.tree))
   {
       comm->dcmf.reducetree = 0;
       comm->dcmf.reduceccmitree = 0;
   }
   else
   {
       comm->dcmf.reducetree = MPIDI_CollectiveProtocols.reduce.usetree;
       comm->dcmf.reduceccmitree = MPIDI_CollectiveProtocols.reduce.useccmitree;
   }
   
   if(MPIDI_CollectiveProtocols.allreduce.usepipelinedtree &&
      !DCMF_Geometry_analyze(&comm->dcmf.geometry,
			     &MPIDI_CollectiveProtocols.allreduce.pipelinedtree))
     comm->dcmf.allreducepipelinedtree = 0;
   else
     comm->dcmf.allreducepipelinedtree = MPIDI_CollectiveProtocols.allreduce.usepipelinedtree;

   if(MPIDI_CollectiveProtocols.broadcast.usetree &&
      !DCMF_Geometry_analyze(&comm->dcmf.geometry,
			     &MPIDI_CollectiveProtocols.broadcast.tree))
     comm->dcmf.bcasttree = 0;
   else
     comm->dcmf.bcasttree = MPIDI_CollectiveProtocols.broadcast.usetree;

   comm->dcmf.sndlen = NULL;
   comm->dcmf.rcvlen = NULL;
   comm->dcmf.sdispls = NULL;
   comm->dcmf.rdispls = NULL;
   comm->dcmf.sndcounters = NULL;
   comm->dcmf.rcvcounters = NULL;
   if(MPIDI_CollectiveProtocols.alltoall.premalloc)
   {
      comm->dcmf.sndlen = MPIU_Malloc(sizeof(unsigned) * comm->local_size);
      comm->dcmf.rcvlen = MPIU_Malloc(sizeof(unsigned) * comm->local_size);
      comm->dcmf.sdispls = MPIU_Malloc(sizeof(unsigned) * comm->local_size);
      comm->dcmf.rdispls = MPIU_Malloc(sizeof(unsigned) * comm->local_size);
      comm->dcmf.sndcounters = MPIU_Malloc(sizeof(unsigned) * comm->local_size);
      comm->dcmf.rcvcounters = MPIU_Malloc(sizeof(unsigned) * comm->local_size);
   }

    MPIR_Barrier(comm);
}


/**
 * \brief Destroy a communicator
 *
 * Hook function to handle collective-specific optimization during communicator destruction
 *
 * \note  We want to free the associated coll_fns buffer at this time.
 */
void MPIDI_Coll_Comm_destroy (MPID_Comm *comm)
{
  MPID_assert (comm != NULL);
  if (comm->coll_fns) MPIU_Free(comm->coll_fns); comm->coll_fns = NULL;
  if(comm->dcmf.worldranks) MPIU_Free(comm->dcmf.worldranks);
  DCMF_Geometry_free(&comm->dcmf.geometry);
  comm->dcmf.worldranks=NULL;
  if(comm->dcmf.sndlen)
   MPIU_Free(comm->dcmf.sndlen);
  if(comm->dcmf.rcvlen)
   MPIU_Free(comm->dcmf.rcvlen);
  if(comm->dcmf.sdispls)
   MPIU_Free(comm->dcmf.sdispls);
  if(comm->dcmf.rdispls)
   MPIU_Free(comm->dcmf.rdispls);
  if(comm->dcmf.sndcounters)
   MPIU_Free(comm->dcmf.sndcounters);
  if(comm->dcmf.rcvcounters)
   MPIU_Free(comm->dcmf.rcvcounters);

  comm->dcmf.sndlen = comm->dcmf.rcvlen =
  comm->dcmf.sdispls = comm->dcmf.rdispls =
  comm->dcmf.sndcounters = comm->dcmf.rcvcounters = NULL;

}

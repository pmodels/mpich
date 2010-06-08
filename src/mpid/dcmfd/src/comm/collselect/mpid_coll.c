/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file src/comm/collselect/mpid_coll.c
 * \brief Collective setup
 */
#include "mpido_coll.h"
#include "mpidi_star.h"
#include "mpix.h"

MPIDI_CollectiveProtocol_t MPIDI_CollectiveProtocols;
char* MPID_Executable_name = NULL;


#ifdef USE_CCMI_COLL

/* #warning reasonable hack for now */
#define MAXGEOMETRIES 65536

static DCMF_Geometry_t *mpid_geometrytable[MAXGEOMETRIES];

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
/* Local barriers PLUS room for one standard/global barrier 
   (DCMF_TORUS_BINOMIAL_BARRIER_PROTOCOL)*/
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


static inline int ASYNC_BROADCAST_REGISTER(DCMF_AsyncBroadcast_Protocol proto,
					   DCMF_CollectiveProtocol_t *proto_ptr,
					   DCMF_AsyncBroadcast_Configuration_t *config)
{
  config->protocol = proto;
  config->isBuffered = 1;
  config->cb_geometry = getGeometryRequest;
  return DCMF_AsyncBroadcast_register(proto_ptr, config);
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
#endif /* USE_CCMI_COLL */



/** \brief Helper used to register all the collective protocols at initialization */
void MPIDI_Coll_register(void)
{
#ifdef USE_CCMI_COLL
  MPIDO_Embedded_Info_Set * properties = &MPIDI_CollectiveProtocols.properties;
  DCMF_Barrier_Configuration_t   barrier_config;
  DCMF_Broadcast_Configuration_t broadcast_config;
  DCMF_AsyncBroadcast_Configuration_t a_broadcast_config;
  DCMF_Allreduce_Configuration_t allreduce_config;
  DCMF_Alltoallv_Configuration_t alltoallv_config;
  DCMF_Reduce_Configuration_t    reduce_config;
  DCMF_GlobalBarrier_Configuration_t gbarrier_config;
  DCMF_GlobalBcast_Configuration_t gbcast_config;
  DCMF_GlobalAllreduce_Configuration_t gallreduce_config;

  DCMF_Result rc;

  DCMF_Configure_t messager_config;

  DCMF_Messager_configure(NULL, &messager_config);

  /* Register the global functions first */
   
  /* ---------------------------------- */
  /* Register global barrier          */
  /* ---------------------------------- */
  if(MPIDO_INFO_ISSET(properties, MPIDO_USE_GI_BARRIER))
  {
    gbarrier_config.protocol = DCMF_GI_GLOBALBARRIER_PROTOCOL;
    rc = DCMF_GlobalBarrier_register(&MPIDI_Protocols.globalbarrier,
                                     &gbarrier_config);
    /* registering the global barrier failed, so don't use it */
    if(rc != DCMF_SUCCESS)
    {
      MPIDO_INFO_UNSET(properties, MPIDO_USE_GI_BARRIER);
    }
  }


  /* ---------------------------------- */
  /* Register global broadcast          */
  /* ---------------------------------- */
  if(MPIDO_INFO_ISSET(properties, MPIDO_USE_TREE_BCAST))
  {
    gbcast_config.protocol = DCMF_TREE_GLOBALBCAST_PROTOCOL;

    /* Check if we are in dual or vn mode and that all ranks in the system */
    /* are physically located on one node. */
    unsigned size = DCMF_Messager_size ();
    if (size <= mpid_hw.tSize)
    {
      size_t rank;
      gbcast_config.protocol = DCMF_INTRANODE_GLOBALBCAST_PROTOCOL;
 
      DCMF_NetworkCoord_t addr[4];
      DCMF_Messager_rank2network(0, DCMF_TORUS_NETWORK, &addr[0]);

      for (rank = 1; rank < size; rank++)
      {
        DCMF_Messager_rank2network (rank, DCMF_TORUS_NETWORK, &addr[rank]);
        if ((addr[rank-1].torus.x != addr[rank].torus.x) ||
            (addr[rank-1].torus.y != addr[rank].torus.y) ||
            (addr[rank-1].torus.z != addr[rank].torus.z))
        {
          gbcast_config.protocol = DCMF_TREE_GLOBALBCAST_PROTOCOL;
          break;
        }
      }
    }

    rc = DCMF_GlobalBcast_register(&MPIDI_Protocols.globalbcast, &gbcast_config);

    /* most likely, we lack shared memory and therefore can't use this */
    if(rc != DCMF_SUCCESS)
    {
      MPIDO_INFO_UNSET(properties, MPIDO_USE_TREE_BCAST);
    }   
  }

  /* ---------------------------------- */
  /* Register global allreduce          */
  /* ---------------------------------- */
  if((MPIDO_INFO_ISSET(properties, MPIDO_USE_TREE_ALLREDUCE)) || 
     (MPIDO_INFO_ISSET(properties, MPIDO_USE_TREE_REDUCE)))
  {
    gallreduce_config.protocol = DCMF_TREE_GLOBALALLREDUCE_PROTOCOL;
    rc = DCMF_GlobalAllreduce_register(&MPIDI_Protocols.globalallreduce,
                                       &gallreduce_config);

    /* most likely, we lack shared memory and therefore can't use this */
    /* reduce uses the allreduce protocol */
    if(rc != DCMF_SUCCESS)
    {
      /* Try the ccmi tree if we were trying global tree */
      if (MPIDO_INFO_ISSET(properties, MPIDO_USE_TREE_ALLREDUCE))
        MPIDO_INFO_SET(properties, MPIDO_USE_CCMI_TREE_ALLREDUCE);
      if (MPIDO_INFO_ISSET(properties, MPIDO_USE_TREE_REDUCE))
        MPIDO_INFO_SET(properties, MPIDO_USE_CCMI_TREE_REDUCE);
          
      MPIDO_INFO_UNSET(properties, MPIDO_USE_TREE_ALLREDUCE);
      MPIDO_INFO_UNSET(properties, MPIDO_USE_TREE_REDUCE);
    }
  }
      

  /* register first barrier protocols now */
  barrier_config.cb_geometry = getGeometryRequest;

  /* set the function that will find the [all]reduce geometry on unexpected callbacks*/
  allreduce_config.cb_geometry = getGeometryRequest;
  reduce_config.cb_geometry = getGeometryRequest;

  /* set configuration flags in the config*/
  allreduce_config.reuse_storage = 
    MPIDO_INFO_ISSET(properties, MPIDO_USE_STORAGE_ALLREDUCE);

  reduce_config.reuse_storage = 
    MPIDO_INFO_ISSET(properties, MPIDO_USE_STORAGE_REDUCE);

  /* ---------------------------------- */
  /* Register multi-node barriers       */
  /* ---------------------------------- */
  /* Other env vars can be checked at communicator creation time
   * but barriers are associated with a geometry and this knowledge
   * isn't available to mpido_barrier
   * If in single thread mode, register gi, rect (via rectlockbox), bino
   * else register gi, rect (via rect barrier), bino 
   */
  DCMF_Barrier_Protocol barrier_proto;

  if(messager_config.thread_level != DCMF_THREAD_MULTIPLE)
    barrier_proto = DCMF_TORUS_RECTANGLELOCKBOX_BARRIER_PROTOCOL_SINGLETH;
  else
    barrier_proto = DCMF_TORUS_RECTANGLE_BARRIER_PROTOCOL; 


   if (MPIDO_INFO_ISSET(properties, MPIDO_USE_CCMI_GI_BARRIER))
   {
      if (BARRIER_REGISTER(DCMF_GI_BARRIER_PROTOCOL,
         &MPIDI_CollectiveProtocols.gi_barrier,
         &barrier_config) != DCMF_SUCCESS)
      MPIDO_INFO_UNSET(properties, MPIDO_USE_CCMI_GI_BARRIER);
   }
  if (!MPIDO_INFO_ISSET(properties, MPIDO_USE_RECT_BARRIER) &&
      MPIDO_INFO_ISSET(properties, MPIDO_USE_BINOM_BARRIER))
  {
    /*
     * Always register a binomial barrier for collectives in subcomms, just
     * choose not to use it at mpido_barrier
     */
    if (BARRIER_REGISTER(DCMF_TORUS_BINOMIAL_BARRIER_PROTOCOL,
                         &MPIDI_CollectiveProtocols.binomial_barrier,
                         &barrier_config) != DCMF_SUCCESS)
      MPIDO_INFO_UNSET(properties, MPIDO_USE_BINOM_BARRIER);

  }
   
  else if (MPIDO_INFO_ISSET(properties, MPIDO_USE_RECT_BARRIER) &&
           !MPIDO_INFO_ISSET(properties, MPIDO_USE_BINOM_BARRIER))
  {
    if (BARRIER_REGISTER(barrier_proto,
                         &MPIDI_CollectiveProtocols.rect_barrier,
                         &barrier_config) != DCMF_SUCCESS)
    {
      MPIDO_INFO_UNSET(properties, MPIDO_USE_RECT_BARRIER);

      /* if rect barrier does not work, then try binom */
      if (BARRIER_REGISTER(DCMF_TORUS_BINOMIAL_BARRIER_PROTOCOL,
                           &MPIDI_CollectiveProtocols.binomial_barrier,
                           &barrier_config) != DCMF_SUCCESS)
        MPIDO_INFO_UNSET(properties, MPIDO_USE_BINOM_BARRIER);
    }
  }

  else
  {
    if (BARRIER_REGISTER(barrier_proto,
                         &MPIDI_CollectiveProtocols.rect_barrier,
                         &barrier_config) != DCMF_SUCCESS)
      MPIDO_INFO_UNSET(properties, MPIDO_USE_RECT_BARRIER);
    if (BARRIER_REGISTER(DCMF_TORUS_BINOMIAL_BARRIER_PROTOCOL,
                         &MPIDI_CollectiveProtocols.binomial_barrier,
                         &barrier_config) != DCMF_SUCCESS)
      MPIDO_INFO_UNSET(properties, MPIDO_USE_BINOM_BARRIER);
  }

  /* if we don't even get a binomial barrier, we are in trouble */
  MPID_assert_debug(barriers_num >  0);
   
  /* ---------------------------------- */
  /* Register local barriers            */
  /* ---------------------------------- */
  /*
   * Register local barriers for the geometry.
   * Both a true local lockbox barrier and a global binomial
   * barrier (which can be used non-optimally).  The geometry
   * will decide internally if/which to use.
   * They are not used directly by MPICH but must be initialized.
   */
  if(MPIDO_INFO_ISSET(properties, MPIDO_USE_LOCKBOX_LBARRIER))
  {
    if(LOCAL_BARRIER_REGISTER(DCMF_LOCKBOX_BARRIER_PROTOCOL,
                              &MPIDI_CollectiveProtocols.lockbox_localbarrier,
                              &barrier_config) != DCMF_SUCCESS)
      MPIDO_INFO_UNSET(properties, MPIDO_USE_LOCKBOX_LBARRIER);
  }

  /*
   * Always register a binomial barrier for collectives in subcomms
   */
  if(LOCAL_BARRIER_REGISTER(DCMF_TORUS_BINOMIAL_BARRIER_PROTOCOL,
			    &MPIDI_CollectiveProtocols.binomial_localbarrier,
			    &barrier_config) != DCMF_SUCCESS)
    MPIDO_INFO_UNSET(properties, MPIDO_USE_BINOM_LBARRIER);

   /* MPID doesn't care if this actually works.  Let someone else
    * handle problems as needed.
    * MPID_assert_debug(local_barriers_num >  0);
    */

   /* -------------------------------------------------------------- */
   /* Register single-thread (memory optimized) protocols if desired */
   /* -------------------------------------------------------------- */
   /* Sort out the single thread memory optimizations first. If we
    * are single threaed, we want to register the single thread versions
    * to save memory */

   if(MPIDO_INFO_ISSET(properties, MPIDO_USE_RECT_DPUT_BCAST) &&
  messager_config.thread_level != DCMF_THREAD_MULTIPLE)
   {
      if(BROADCAST_REGISTER(
            DCMF_TORUS_RECTANGLE_BROADCAST_PROTOCOL_DPUT_SINGLETH,
            &MPIDI_CollectiveProtocols.rectangle_bcast_dput,
            &broadcast_config) != DCMF_SUCCESS)
         MPIDO_INFO_UNSET(properties, MPIDO_USE_RECT_DPUT_BCAST);
   }

   if(MPIDO_INFO_ISSET(properties, MPIDO_USE_RECT_SINGLETH_BCAST) &&
  messager_config.thread_level != DCMF_THREAD_MULTIPLE)
   {
      if(BROADCAST_REGISTER(
            DCMF_TORUS_RECTANGLE_BROADCAST_PROTOCOL_SINGLETH,
            &MPIDI_CollectiveProtocols.rectangle_bcast_singleth,
            &broadcast_config) != DCMF_SUCCESS)
         MPIDO_INFO_UNSET(properties, MPIDO_USE_RECT_SINGLETH_BCAST);
   }

   if(MPIDO_INFO_ISSET(properties, MPIDO_USE_BINOM_SINGLETH_BCAST) &&
  messager_config.thread_level != DCMF_THREAD_MULTIPLE)
   {
      if(BROADCAST_REGISTER(
            DCMF_TORUS_BINOMIAL_BROADCAST_PROTOCOL_SINGLETH,
            &MPIDI_CollectiveProtocols.binomial_bcast_singleth,
            &broadcast_config) != DCMF_SUCCESS)
         MPIDO_INFO_UNSET(properties, MPIDO_USE_BINOM_SINGLETH_BCAST);
   }
   /* --------------------------------------------------- */
   /* Register all other bcast protocols needed/requested */
   /* --------------------------------------------------- */
   if(MPIDO_INFO_ISSET(properties, MPIDO_USE_CCMI_TREE_BCAST))
     {
       if(BROADCAST_REGISTER(DCMF_TREE_BROADCAST_PROTOCOL,
			     &MPIDI_CollectiveProtocols.tree_bcast,
			     &broadcast_config) != DCMF_SUCCESS)
	 MPIDO_INFO_UNSET(properties, MPIDO_USE_CCMI_TREE_BCAST);
     }

   if(MPIDO_INFO_ISSET(properties, MPIDO_USE_CCMI_TREE_DPUT_BCAST))
     {
       if(BROADCAST_REGISTER(DCMF_TREE_DPUT_BROADCAST_PROTOCOL,
			     &MPIDI_CollectiveProtocols.tree_dput_bcast,
			     &broadcast_config) != DCMF_SUCCESS)
	 MPIDO_INFO_UNSET(properties, MPIDO_USE_CCMI_TREE_DPUT_BCAST);
     }

   if(BROADCAST_REGISTER(DCMF_TORUS_RECTANGLE_BROADCAST_PROTOCOL,
			 &MPIDI_CollectiveProtocols.rectangle_bcast,
			 &broadcast_config) != DCMF_SUCCESS)
     MPIDO_INFO_UNSET(properties, MPIDO_USE_RECT_BCAST);

   if(ASYNC_BROADCAST_REGISTER(DCMF_TORUS_ASYNCBROADCAST_RECTANGLE_PROTOCOL,
			    &MPIDI_CollectiveProtocols.async_rectangle_bcast,
			    &a_broadcast_config) != DCMF_SUCCESS)
      MPIDO_INFO_UNSET(properties, MPIDO_USE_ARECT_BCAST);

  if(BROADCAST_REGISTER(DCMF_TORUS_BINOMIAL_BROADCAST_PROTOCOL,
                        &MPIDI_CollectiveProtocols.binomial_bcast,
                        &broadcast_config) != DCMF_SUCCESS)
    MPIDO_INFO_UNSET(properties, MPIDO_USE_BINOM_BCAST);

  if(ASYNC_BROADCAST_REGISTER(DCMF_TORUS_ASYNCBROADCAST_BINOMIAL_PROTOCOL,
                              &MPIDI_CollectiveProtocols.async_binomial_bcast,
                              &a_broadcast_config) != DCMF_SUCCESS)
    MPIDO_INFO_UNSET(properties, MPIDO_USE_ABINOM_BCAST);

  /* --------------------------------------------- */
  /* Register allreduce protocols needed/requested */
  /* --------------------------------------------- */
  if(MPIDO_INFO_ISSET(properties, MPIDO_USE_TREE_ALLREDUCE) ||
     MPIDO_INFO_ISSET(properties, MPIDO_USE_CCMI_TREE_ALLREDUCE))
   {
      if(ALLREDUCE_REGISTER(DCMF_TREE_ALLREDUCE_PROTOCOL,
               &MPIDI_CollectiveProtocols.tree_allreduce,
               &allreduce_config) != DCMF_SUCCESS)
      {

        MPIDO_INFO_UNSET(properties, MPIDO_USE_TREE_ALLREDUCE);
         MPIDO_INFO_UNSET(properties, MPIDO_USE_CCMI_TREE_ALLREDUCE);
      }
   }
   if(MPIDO_INFO_ISSET(properties, MPIDO_USE_SHORT_ASYNC_RECT_ALLREDUCE))
   {
      if(ALLREDUCE_REGISTER(DCMF_TORUS_ASYNC_SHORT_RECTANGLE_ALLREDUCE_PROTOCOL,
                            &MPIDI_CollectiveProtocols.short_async_rect_allreduce,
                            &allreduce_config) != DCMF_SUCCESS)
            MPIDO_INFO_UNSET(properties, MPIDO_USE_SHORT_ASYNC_RECT_ALLREDUCE);
   }

   if(MPIDO_INFO_ISSET(properties, MPIDO_USE_SHORT_ASYNC_BINOM_ALLREDUCE))
   {
      if(ALLREDUCE_REGISTER(DCMF_TORUS_ASYNC_SHORT_BINOMIAL_ALLREDUCE_PROTOCOL,
                  &MPIDI_CollectiveProtocols.short_async_binom_allreduce,
                  &allreduce_config) != DCMF_SUCCESS)
         MPIDO_INFO_UNSET(properties, MPIDO_USE_SHORT_ASYNC_BINOM_ALLREDUCE);
   }

   if(MPIDO_INFO_ISSET(properties, MPIDO_USE_RRING_DPUT_SINGLETH_ALLREDUCE))
   {
      if(ALLREDUCE_REGISTER(DCMF_TORUS_RRING_DPUT_ALLREDUCE_PROTOCOL_SINGLETH,
                            &MPIDI_CollectiveProtocols.rring_dput_allreduce_singleth,
                            &allreduce_config) != DCMF_SUCCESS)
         MPIDO_INFO_UNSET(properties, MPIDO_USE_RRING_DPUT_SINGLETH_ALLREDUCE);
   }
   
  if(MPIDO_INFO_ISSET(properties, MPIDO_USE_PIPELINED_TREE_ALLREDUCE))
  {
    if((ALLREDUCE_REGISTER(DCMF_TREE_PIPELINED_ALLREDUCE_PROTOCOL,
                           &MPIDI_CollectiveProtocols.pipelinedtree_allreduce,
                           &allreduce_config) != DCMF_SUCCESS) ||
       (ALLREDUCE_REGISTER(DCMF_TREE_DPUT_PIPELINED_ALLREDUCE_PROTOCOL,
                           &MPIDI_CollectiveProtocols.pipelinedtree_dput_allreduce,
                           &allreduce_config) != DCMF_SUCCESS) )
      MPIDO_INFO_UNSET(properties, MPIDO_USE_PIPELINED_TREE_ALLREDUCE);
  }


  if(ALLREDUCE_REGISTER(DCMF_TORUS_RECTANGLE_ALLREDUCE_PROTOCOL,
                        &MPIDI_CollectiveProtocols.rectangle_allreduce,
                        &allreduce_config) != DCMF_SUCCESS)
    MPIDO_INFO_UNSET(properties, MPIDO_USE_RECT_ALLREDUCE);

  if(ALLREDUCE_REGISTER(DCMF_TORUS_RECTANGLE_RING_ALLREDUCE_PROTOCOL,
                        &MPIDI_CollectiveProtocols.rectanglering_allreduce,
                        &allreduce_config) != DCMF_SUCCESS)
    MPIDO_INFO_UNSET(properties, MPIDO_USE_RECTRING_ALLREDUCE);

  if(ALLREDUCE_REGISTER(DCMF_TORUS_BINOMIAL_ALLREDUCE_PROTOCOL,
                        &MPIDI_CollectiveProtocols.binomial_allreduce,
                        &allreduce_config) != DCMF_SUCCESS)
    MPIDO_INFO_UNSET(properties, MPIDO_USE_BINOM_ALLREDUCE);

  if (ALLREDUCE_REGISTER(DCMF_TORUS_ASYNC_RECTANGLE_ALLREDUCE_PROTOCOL,
                         &MPIDI_CollectiveProtocols.async_rectangle_allreduce,
                         &allreduce_config) != DCMF_SUCCESS)
    MPIDO_INFO_UNSET(properties, MPIDO_USE_ARECT_ALLREDUCE);

  if (ALLREDUCE_REGISTER(DCMF_TORUS_ASYNC_RECTANGLE_RING_ALLREDUCE_PROTOCOL,
                         &MPIDI_CollectiveProtocols.async_ringrectangle_allreduce,
                         &allreduce_config) != DCMF_SUCCESS)
    MPIDO_INFO_UNSET(properties, MPIDO_USE_ARECTRING_ALLREDUCE);
     
  if (ALLREDUCE_REGISTER(DCMF_TORUS_ASYNC_BINOMIAL_ALLREDUCE_PROTOCOL,
                         &MPIDI_CollectiveProtocols.async_binomial_allreduce,
                         &allreduce_config) != DCMF_SUCCESS)
    MPIDO_INFO_UNSET(properties, MPIDO_USE_ABINOM_ALLREDUCE);
     
  /* ----------------------------------------------- */
  /* Register alltoallv protocol needed/requested    */
  /* This also covvers alltoall/alltoallw operations */
  /* ----------------------------------------------- */
  if(ALLTOALLV_REGISTER(DCMF_TORUS_ALLTOALLV_PROTOCOL,
                        &MPIDI_CollectiveProtocols.torus_alltoallv,
                        &alltoallv_config) != DCMF_SUCCESS)
  {
    MPIDO_INFO_UNSET(properties, MPIDO_USE_TORUS_ALLTOALL);
    MPIDO_INFO_UNSET(properties, MPIDO_USE_TORUS_ALLTOALLV);
    MPIDO_INFO_UNSET(properties, MPIDO_USE_TORUS_ALLTOALLW);
  }
   
  /* --------------------------------------------- */
  /* Register reduce protocols needed/requested    */
  /* --------------------------------------------- */
  if(MPIDO_INFO_ISSET(properties, MPIDO_USE_TREE_REDUCE) ||
     MPIDO_INFO_ISSET(properties, MPIDO_USE_CCMI_TREE_REDUCE))
  {
    if(REDUCE_REGISTER(DCMF_TREE_REDUCE_PROTOCOL,
                       &MPIDI_CollectiveProtocols.tree_reduce,
                       &reduce_config) != DCMF_SUCCESS)
    {
      MPIDO_INFO_UNSET(properties, MPIDO_USE_TREE_REDUCE);
      MPIDO_INFO_UNSET(properties, MPIDO_USE_CCMI_TREE_REDUCE);
    }
  }
   
  if(REDUCE_REGISTER(DCMF_TORUS_BINOMIAL_REDUCE_PROTOCOL,
                     &MPIDI_CollectiveProtocols.binomial_reduce,
                     &reduce_config) != DCMF_SUCCESS)
    MPIDO_INFO_UNSET(properties, MPIDO_USE_BINOM_REDUCE);
   
  if(REDUCE_REGISTER(DCMF_TORUS_RECTANGLE_REDUCE_PROTOCOL,
                     &MPIDI_CollectiveProtocols.rectangle_reduce,
                     &reduce_config) != DCMF_SUCCESS)
    MPIDO_INFO_UNSET(properties, MPIDO_USE_RECT_REDUCE);
   
  if(REDUCE_REGISTER(DCMF_TORUS_RECTANGLE_RING_REDUCE_PROTOCOL,
                     &MPIDI_CollectiveProtocols.rectanglering_reduce,
                     &reduce_config) != DCMF_SUCCESS)
    MPIDO_INFO_UNSET(properties, MPIDO_USE_RECTRING_REDUCE);
#endif /* USE_CCMI_COLL */
}


/**
 * \brief Create collective communicators
 *
 * Hook function to handle collective-specific optimization during communicator creation
 */
void MPIDI_Coll_Comm_create (MPID_Comm *comm)
{
  int global, x_size, y_size, z_size, t_size;
  unsigned my_coords[4], min_coords[4], max_coords[4];
  MPIDO_Embedded_Info_Set * comm_prop, * coll_prop;
  MPID_Comm *comm_world;
  DCMF_Configure_t dcmf_config;


  MPID_assert (comm!= NULL);



  if (comm->coll_fns) MPIU_Free(comm->coll_fns);
  comm->coll_fns=NULL;   /* !!! Intercomm_merge does not NULL the fcns,
                          * leading to stale functions for new comms.
                          * We'll null it here until argonne confirms
                          * this is the correct behavior of merge
                          */

  comm_prop = &(comm -> dcmf.properties);
  coll_prop = &MPIDI_CollectiveProtocols.properties;
  DCMF_Messager_configure(NULL, &dcmf_config);

  /* unset all properties of a comm by default */
  MPIDO_INFO_ZERO(comm_prop);
  comm -> dcmf.tuning_session = NULL;
  comm -> dcmf.bcast_iter = 0;

  comm->dcmf.last_algorithm = 0;
  
  /* ****************************************** */
  /* Allocate space for the collective pointers */
  /* ****************************************** */

  comm->coll_fns = (MPID_Collops *)MPIU_Malloc(sizeof(MPID_Collops));
  MPID_assert(comm->coll_fns != NULL);
  memset(comm->coll_fns, 0, sizeof(MPID_Collops));


  /* If we are an intracomm, MPICH should handle */
  if (comm->comm_kind != MPID_INTRACOMM) return;
  /* User may disable all collectives */
  if (!MPIDI_Process.optimized.collectives) return;

  MPID_Comm_get_ptr(MPI_COMM_WORLD, comm_world);
  MPID_assert_debug(comm_world != NULL);

  /* let us assume global context is comm_world */
  global = 1;

   /* assume single threaded mode until we find out differently */
   MPIDO_INFO_SET(comm_prop, MPIDO_SINGLE_THREAD_MODE);

   if(dcmf_config.thread_level == MPI_THREAD_MULTIPLE)
   {
      STAR_info.enabled = 0;
      MPIDO_INFO_UNSET(comm_prop, MPIDO_SINGLE_THREAD_MODE);
      if(comm!= comm_world)
         global = 0;
   }
   else /* single MPI thread. */
   {
      /* and if we are not dup of comm_world, global context is not safe */
      if(comm->local_size != comm_world->local_size)
         global = 0;
   }

#ifdef USE_CCMI_COLL
  /* ******************************************************* */
  /* Setup Barriers and geometry for this communicator       */
  /* ******************************************************* */
  DCMF_Geometry_initialize(&comm->dcmf.geometry,
			   comm->context_id,
			   (unsigned*)comm->vcr,
			   comm->local_size,
			   barriers,
			   barriers_num,
			   local_barriers,
			   local_barriers_num,
			   &comm->dcmf.barrier,
			   MPIDI_CollectiveProtocols.numcolors,
			   global);
  
  mpid_geometrytable[(comm->context_id)%MAXGEOMETRIES] = &comm->dcmf.geometry;

  /* ****************************************** */
  /* These are ALL the pointers in the object   */
  /* ****************************************** */
  
  comm->coll_fns->Barrier        = MPIDO_Barrier;
  comm->coll_fns->Bcast          = MPIDO_Bcast;
  comm->coll_fns->Reduce         = MPIDO_Reduce;
  comm->coll_fns->Allreduce      = MPIDO_Allreduce;
  comm->coll_fns->Alltoall       = MPIDO_Alltoall;
  comm->coll_fns->Alltoallv      = MPIDO_Alltoallv;
  comm->coll_fns->Alltoallw      = MPIDO_Alltoallw;
  comm->coll_fns->Allgather      = MPIDO_Allgather;
  comm->coll_fns->Allgatherv     = MPIDO_Allgatherv;
  comm->coll_fns->Gather         = MPIDO_Gather;
  comm->coll_fns->Gatherv        = MPIDO_Gatherv;
  comm->coll_fns->Scatter        = MPIDO_Scatter;
  comm->coll_fns->Scatterv       = MPIDO_Scatterv;
  comm->coll_fns->Reduce_scatter = MPIDO_Reduce_scatter;
  comm->coll_fns->Scan           = MPIDO_Scan;
  comm->coll_fns->Exscan         = MPIDO_Exscan;
  comm->coll_fns->Reduce_scatter_block = MPIDO_Reduce_scatter_block;

  /* set geometric properties of the communicator */
  if (MPIDO_ISPOF2(comm->local_size))
    MPIDO_INFO_SET(comm_prop, MPIDO_POF2_COMM);

  MPIDO_INFO_SET(comm_prop, MPIDO_TORUS_COMM);

  if (comm -> local_size == DCMF_Messager_size() &&
      !MPIDO_INFO_ISSET(comm_prop, MPIDO_USE_NOTREE_OPT_COLLECTIVES))
    MPIDO_INFO_SET(comm_prop, MPIDO_TREE_COMM);
  
  if (global)
    MPIDO_INFO_SET(comm_prop, MPIDO_GLOBAL_CONTEXT);

  MPIR_Barrier_intra(comm);

  MPIX_Comm_rank2torus(comm -> handle,
		       comm -> rank,
		       &my_coords[0], 
		       &my_coords[1],
		       &my_coords[2], 
		       &my_coords[3]);

  /* find if the communicator is a rectangle */
  MPIR_Allreduce_intra(my_coords, min_coords,4, MPI_UNSIGNED, MPI_MIN, comm);
  MPIR_Allreduce_intra(my_coords, max_coords,4, MPI_UNSIGNED, MPI_MAX, comm);
  
  t_size = (unsigned) (max_coords[3] - min_coords[3] + 1);
  z_size = (unsigned) (max_coords[2] - min_coords[2] + 1);
  y_size = (unsigned) (max_coords[1] - min_coords[1] + 1);
  x_size = (unsigned) (max_coords[0] - min_coords[0] + 1);

  if (x_size * y_size * z_size * t_size == comm -> local_size)
    {
      MPIDO_INFO_SET(comm_prop, MPIDO_RECT_COMM);
      comm->dcmf.comm_shape = 0; /* COMM WORLD */
      if (!global)
        comm->dcmf.comm_shape = 1; /* RECT subcomm */
    }
  else if (!global) /* if not rect and not global, it means comm is irreg */
    {
      MPIDO_INFO_SET(comm_prop, MPIDO_IRREG_COMM);
      comm->dcmf.comm_shape = 2; /* IRREG */
    }

  /* end of setting geometric properties of the communicator */

  /* now set the protocols and algorithms based on geometry bits info */
  
  MPIDI_Comm_setup_properties(comm, 1); /* 1 means this is initial setup */

  if (STAR_info.enabled && STAR_info.debug && 
      comm -> comm_kind == MPID_INTRACOMM &&
      comm -> rank == 0)
  {
    static unsigned char opened = 0;
    
    if (!opened && MPID_Executable_name)
    {
      MPID_Comm * comm_world;
      
      int length, cw_rank;
      char * tmp;
      length = strlen(MPID_Executable_name) + 25;
      
      tmp = (char *) malloc(sizeof(char) * length);
      
      MPID_Comm_get_ptr(MPI_COMM_WORLD, comm_world);
      cw_rank = comm_world -> rank;
      
      sprintf(tmp, "%s-star-rank%d.log", MPID_Executable_name, cw_rank);
      
      if (!(MPIDO_STAR_fd = fopen(tmp, "w")))
        fprintf(stderr, "Error openning STAR debugging file: %s\n", tmp);
      else
        opened = 1;
      free(tmp);
    }
  }

  
#else /* !USE_CCMI_COLL */

  comm->coll_fns->Barrier        = NULL;
  comm->coll_fns->Bcast          = NULL;
  comm->coll_fns->Reduce         = NULL;
  comm->coll_fns->Allreduce      = NULL;
  comm->coll_fns->Alltoall       = NULL;
  comm->coll_fns->Alltoallv      = NULL;
  comm->coll_fns->Alltoallw      = NULL;
  comm->coll_fns->Allgather      = NULL;
  comm->coll_fns->Allgatherv     = NULL;
  comm->coll_fns->Gather         = NULL;
  comm->coll_fns->Gatherv        = NULL;
  comm->coll_fns->Scatter        = NULL;
  comm->coll_fns->Scatterv       = NULL;
  comm->coll_fns->Reduce_scatter = NULL;
  comm->coll_fns->Scan           = NULL;
  comm->coll_fns->Exscan         = NULL;
  comm->coll_fns->Reduce_scatter_block = NULL;

#endif /* !USE_CCMI_COLL */

  comm -> dcmf.sndlen = NULL;
  comm -> dcmf.rcvlen = NULL;
  comm -> dcmf.sdispls = NULL;
  comm -> dcmf.rdispls = NULL;
  comm -> dcmf.sndcounters = NULL;
  comm -> dcmf.rcvcounters = NULL;
  
  if (MPIDO_INFO_ISSET(coll_prop, MPIDO_USE_PREMALLOC_ALLTOALL))
  {
    int type_sz = sizeof(unsigned);
    comm -> dcmf.sndlen = MPIU_Malloc(type_sz * comm->local_size);
    comm -> dcmf.rcvlen = MPIU_Malloc(type_sz * comm->local_size);
    comm -> dcmf.sdispls = MPIU_Malloc(type_sz * comm->local_size);
    comm -> dcmf.rdispls = MPIU_Malloc(type_sz * comm->local_size);
    comm -> dcmf.sndcounters = MPIU_Malloc(type_sz * comm->local_size);
    comm -> dcmf.rcvcounters = MPIU_Malloc(type_sz * comm->local_size);
  }

  MPIR_Barrier_intra(comm);
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
  if (comm->coll_fns)
    MPIU_Free(comm->coll_fns);
#ifdef USE_CCMI_COLL
  STAR_FreeMem(comm);
  DCMF_Geometry_free(&comm->dcmf.geometry);
#endif /* USE_CCMI_COLL */
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

  comm->dcmf.sndlen  =
    comm->dcmf.rcvlen  =
    comm->dcmf.sdispls =
    comm->dcmf.rdispls =
    comm->dcmf.sndcounters =
    comm->dcmf.rcvcounters = NULL;
}

void MPIDI_Comm_setup_properties(MPID_Comm * comm, int initial_setup)
{
  MPIDO_Embedded_Info_Set * comm_prop, * coll_prop;
  DCMF_Configure_t messager_config;

  DCMF_Messager_configure(NULL, &messager_config);

  comm_prop = &(comm -> dcmf.properties);
  coll_prop = &MPIDI_CollectiveProtocols.properties;

  /* 
     we basically be optimistic and assume all conditions are available 
     for all protocols based on the mpidi_protocol properties. As such, we 
     copy the informative bits from coll_prop to comm_prop. Then, we check 
     the geometry bits of the communicator to uncheck any bit for a 
     protocol 
  */
  
  if (initial_setup)
    MPIDO_INFO_OR(coll_prop, comm_prop);

    if(messager_config.thread_level == DCMF_THREAD_MULTIPLE)
    {
      MPIDO_INFO_UNSET(comm_prop, MPIDO_USE_RRING_DPUT_SINGLETH_ALLREDUCE);
      MPIDO_INFO_UNSET(comm_prop, MPIDO_USE_RECT_DPUT_BCAST);
      MPIDO_INFO_UNSET(comm_prop, MPIDO_USE_RECT_SINGLETH_BCAST);
      MPIDO_INFO_UNSET(comm_prop, MPIDO_USE_BINOM_SINGLETH_BCAST);
    }
    
    if (!MPIDO_INFO_ISSET(comm_prop, MPIDO_RECT_COMM))
    {
      MPIDO_INFO_UNSET(comm_prop, MPIDO_USE_RECT_BARRIER);
      MPIDO_INFO_UNSET(comm_prop, MPIDO_USE_ARECT_BCAST);
      MPIDO_INFO_UNSET(comm_prop, MPIDO_USE_RECT_BCAST);
      MPIDO_INFO_UNSET(comm_prop, MPIDO_USE_RECT_DPUT_BCAST);
      MPIDO_INFO_UNSET(comm_prop, MPIDO_USE_RECT_SINGLETH_BCAST);
      MPIDO_INFO_UNSET(comm_prop, MPIDO_USE_RECT_BCAST_ALLGATHER);
      MPIDO_INFO_UNSET(comm_prop, MPIDO_USE_ARECT_BCAST_ALLGATHER);
      MPIDO_INFO_UNSET(comm_prop, MPIDO_USE_RECT_BCAST_ALLGATHERV);
      MPIDO_INFO_UNSET(comm_prop, MPIDO_USE_ARECT_BCAST_ALLGATHERV);
      MPIDO_INFO_UNSET(comm_prop, MPIDO_USE_SHORT_ASYNC_RECT_ALLREDUCE);
      MPIDO_INFO_UNSET(comm_prop, MPIDO_USE_RRING_DPUT_SINGLETH_ALLREDUCE);
      MPIDO_INFO_UNSET(comm_prop, MPIDO_USE_RECT_ALLREDUCE);
      MPIDO_INFO_UNSET(comm_prop, MPIDO_USE_RECTRING_ALLREDUCE);
      MPIDO_INFO_UNSET(comm_prop, MPIDO_USE_ARECT_ALLREDUCE);
      MPIDO_INFO_UNSET(comm_prop, MPIDO_USE_ARECTRING_ALLREDUCE);
      MPIDO_INFO_UNSET(comm_prop, MPIDO_USE_RECT_REDUCE);
      MPIDO_INFO_UNSET(comm_prop, MPIDO_USE_RECTRING_REDUCE);
    }
   
   if(!MPIDO_INFO_ISSET(comm_prop, MPIDO_GLOBAL_CONTEXT) &&
      !MPIDO_INFO_ISSET(comm_prop, MPIDO_SINGLE_THREAD_MODE))
   {
      MPIDO_INFO_UNSET(comm_prop, MPIDO_USE_TORUS_ALLTOALL);
      MPIDO_INFO_UNSET(comm_prop, MPIDO_USE_TORUS_ALLTOALLW);
      MPIDO_INFO_UNSET(comm_prop, MPIDO_USE_TORUS_ALLTOALLV);
      MPIDO_INFO_UNSET(comm_prop, MPIDO_USE_ALLTOALL_SCATTERV);
      MPIDO_INFO_UNSET(comm_prop, MPIDO_USE_REDUCESCATTER);
    }
   if(!MPIDO_INFO_ISSET(comm_prop, MPIDO_GLOBAL_CONTEXT))
   {
     MPIDO_INFO_UNSET(comm_prop, MPIDO_USE_GI_BARRIER);
     MPIDO_INFO_UNSET(comm_prop, MPIDO_USE_CCMI_GI_BARRIER);
   }
   
   if (!MPIDO_INFO_ISSET(comm_prop, MPIDO_GLOBAL_CONTEXT) ||
       !MPIDO_INFO_ISSET(comm_prop, MPIDO_TREE_COMM) ||
       MPIDO_INFO_ISSET(comm_prop, MPIDO_USE_NOTREE_OPT_COLLECTIVES))
   {
     MPIDO_INFO_UNSET(comm_prop, MPIDO_USE_TREE_BCAST);
     MPIDO_INFO_UNSET(comm_prop, MPIDO_USE_TREE_ALLREDUCE);
     MPIDO_INFO_UNSET(comm_prop, MPIDO_USE_CCMI_TREE_ALLREDUCE);
     MPIDO_INFO_UNSET(comm_prop, MPIDO_USE_PIPELINED_TREE_ALLREDUCE);
     MPIDO_INFO_UNSET(comm_prop, MPIDO_USE_TREE_REDUCE);
     MPIDO_INFO_UNSET(comm_prop, MPIDO_USE_CCMI_TREE_REDUCE);
     MPIDO_INFO_UNSET(comm_prop, MPIDO_USE_REDUCE_GATHER);
     /*      MPIDO_INFO_UNSET(comm_prop, MPIDO_USE_BCAST_SCATTER); */
     MPIDO_INFO_UNSET(comm_prop, MPIDO_USE_REDUCESCATTER);
   }
  
  if (comm->comm_kind != MPID_INTRACOMM || comm->local_size <= 4)
    MPIDO_MSET_INFO(comm_prop,
                    MPIDO_USE_MPICH_BARRIER,
                    MPIDO_USE_MPICH_ALLTOALL,
                    MPIDO_USE_MPICH_ALLTOALLW,
                    MPIDO_USE_MPICH_ALLTOALLV,
                    MPIDO_USE_MPICH_ALLGATHER,
                    MPIDO_USE_MPICH_ALLGATHERV,
                    MPIDO_USE_MPICH_ALLREDUCE,
                    MPIDO_USE_MPICH_REDUCE,
                    MPIDO_USE_MPICH_GATHER,
                    MPIDO_USE_MPICH_SCATTER,
                    MPIDO_USE_MPICH_SCATTERV,
                    MPIDO_USE_MPICH_REDUCESCATTER,
                    MPIDO_END_ARGS);
  if (comm->comm_kind != MPID_INTRACOMM || comm->local_size <= 3)
    MPIDO_INFO_SET(comm_prop, MPIDO_USE_MPICH_BCAST);
	
}

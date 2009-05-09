/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file src/comm/mpid_comm.c
 * \brief Communicator setup
 */
#include "mpido_coll.h"
#include "mpidi_star.h"

/** \page env_vars Environment Variables
 * - DCMF_VERBOSE - Increases the amount of information dumped during an 
 *   MPI_Abort() call.  Possible values:
 *   - 0 - No additional information is dumped.
 *   - 1 - Additional information is dumped.
 *   - Default is 0.
 *
 * - DCMF_STATISTICS - Turns on statistics printing for the message layer
 *   such as the maximum receive queue depth.  Possible values:
 *   - 0 - No statistics are printed.
 *   - 1 - Statistics are printed.
 *   - Default is 0.
 *
 * - DCMF_EAGER -
 * - DCMF_RZV -
 * - DCMF_RVZ - Sets the cutoff for the switch to the rendezvous protocol.
 *   All three options are identical.  This takes an argument, in bytes,
 *   to switch from the eager protocol to the rendezvous protocol for point-to-point
 *   messaging.  Increasing the limit might help for larger partitions
 *   and if most of the communication is nearest neighbor.
 *   - Default is 1200 bytes.
 *
 * - DCMF_OPTRVZ -
 * - DCMF_OPTRZV - Determines the optimized rendezvous limit. Both options
 *   are identical.  This takes an argument, in bytes.  The
 *   optimized rendezvous protocol will be used if:
 *   eager_limit <= message_size < (eager_limit + DCMF_OPTRZV).
 *   For sending, one of three protocols will be used depending on the message 
 *   size:  The eager protocol for small messages, the optimized rendezvous 
 *   protocol for medium messages, and the default rendezvous protocol for 
 *   large messages. The optimized rendezvous protocol generally has less 
 *   latency than the default rendezvous protocol, but does not wait for a 
 *   receive to be posted first.  Therefore, unexpected messages in this size 
 *   range may be received, consuming storage until the receives are issued. 
 *   The default rendezvous protocol waits for a receive to be posted first. 
 *   Therefore, no unexpected messages in this size range will be received. 
 *   The optimized rendezvous protocol also avoids filling injection fifos 
 *   which can cause delays while larger fifos are allocated.  For example, 
 *   alltoall on large subcommunicators with thread mode multiple will 
 *   benefit from optimized rendezvous. 
 *   - Default is 0 bytes, meaning that optimized rendezvous is not used.
 *
 * - DCMF_NUMREQUESTS - Sets the number of outstanding asynchronous
 *   broadcasts to have before a barrier is called.  This is mostly
 *   used in allgather/allgatherv using asynchronous broadcasts. 
 *   Higher numbers can help on larger partitions and larger
 *   message sizes. 
 *   - Default is 32.
 *
 * - DCMF_RMA_PENDING - Maximum outstanding RMA requests.
 *   Limits number of DCMF_Request objects allocated by MPI Onesided operations.
 *   - Default is 1000.
 *
 * - DCMF_INTERRUPT -
 * - DCMF_INTERRUPTS - Turns on interrupt driven communications. This
 *   can be beneficial to some applications and is required if you are
 *   using Global Arrays or ARMCI.  (They force this on, regardless of
 *   the environment setting).  Possible values:
 *   - 0 - Interrupt driven communications is not used.
 *   - 1 - Interrupt driven communications is used.
 *   - Default is 0.
 *
 * - DCMF_SENDER_SIDE_MATCHING -
 * - DCMF_SSM - Turns on sender-side matching.  This can speed up
 *   point-to-point messaging in well-behaved applications, specifically
 *   those that do not do MPI_ANY_SOURCE receives.  Possible values:
 *   - 0 - Sender side matching is not used.
 *   - 1 - Sender side matching is used.
 *   - Default is 0.
 *
 * - DCMF_TOPOLOGY - Turns on optimized topology
 *   creation functions when using MPI_Cart_create with the
 *   reorder flag.  We attempt to create communicators similar
 *   to those requested, that match physical hardware as much
 *   as possible.  Possible values:
 *   - 0 - Optimized topology creation functions are not used.
 *   - 1 - Optimized topology creation functions are used.
 *   - Default is 1.
 *
 * - DCMF_COLLECTIVE - 
 * - DCMF_COLLECTIVES - Controls whether optimized collectives are used.
 *   Possible values:
 *   - 0 - Optimized collectives are not used.
 *   - 1 - Optimized collectives are used.
 *   - NOTREE.  Only collective network optimizations are not used.
 *   - Default is 1.
 *
 * - DCMF_ASYNCCUTOFF - Changes the cutoff point between
 *   asynchronous and synchronous rectangular/binomial broadcasts.
 *   This can be highly application dependent.
 *   - Default is 128k.
 *
 * - DCMF_SCATTER - Controls the protocol used for scatter.
 *   Possible values:
 *   - MPICH - Use the MPICH point-to-point protocol.
 *   - Default (or if anything else is specified) 
 *     is to use a broadcast-based scatter at a 2k or larger 
 *     message size. 
 *
 * - DCMF_SCATTERV - Controls the protocol used for scatterv.
 *   Possible values:
 *   - ALLTOALL - Use an all-to-all based protocol when the 
 *     message size is above 2k. This is optimal for larger messages 
 *     and larger partitions. 
 *   - BCAST - Use a broadcast-based scatterv.  This works well 
 *     for small messages. 
 *   - MPICH - Use the MPICH point-to-point protocol.
 *   - Default is ALLTOALL.
 *
 * - DCMF_GATHER - Controls the protocol used for gather.
 *   Possible values:
 *   - MPICH - Use the MPICH point-to-point protocol.
 *   - Default (or if anything else is specified) 
 *     is to use a reduce-based algorithm for larger message sizes. 
 *
 * - DCMF_REDUCESCATTER - Controls the protocol used for reduce_scatter
 *   operations.  The options for DCMF_SCATTERV and DCMF_REDUCE can change the 
 *   behavior of reduce_scatter.  Possible values:
 *   - MPICH - Use the MPICH point-to-point protocol.
 *   - Default (or if anything else is specified) 
 *     is to use an optimized reduce followed by an optimized scatterv.
 *     This works well for larger messages.
 *
 * - DCMF_BCAST - Controls the protocol used for broadcast.  Possible values:
 *   - MPICH - Turn off all optimizations for broadcast and use the MPICH
 *     point-to-point protocol.
 *   - TREE - Use the collective network. This is the default on MPI_COMM_WORLD and 
 *     duplicates of MPI_COMM_WORLD in MPI_THREAD_SINGLE mode.  This provides
 *     the fastest possible broadcast.
 *   - CCMI - Use the CCMI collective network protocol.  This is off by default.
 *   - CDPUT - Use the CCMI collective network protocol with
 *     DPUT. This is off by default.
 *   - AR - Use the asynchronous rectangle protocol. This is the default
 *     for small messages on rectangular subcommunicators.  The cutoff between
 *     async and sync can be controlled with DCMF_ASYNCCUTOFF.
 *   - AB - Use the asynchronous binomial protocol.  This is the default
 *     for irregularly shaped subcommunicators.  The cutoff between
 *     async and sync can be controlled with DCMF_ASYNCCUTOFF.
 *   - RECT - Use the rectangle protocol. This is the default for rectangularly
 *     shaped subcommunicators for large messages.  This disables the asynchronous
 *     protocol.
 *   - BINOM - Use the binomial protocol. This is the default for irregularly
 *     shaped subcommunicators for large messages.  This disables the asynchronous
 *     protocol.
 *   - Default varies based on the communicator.  See above.
 *
 * - DCMF_NUMCOLORS - Controls how many colors are used for rectangular
 *   broadcasts.  Possible values:
 *   - 0 - Let the lower-level messaging system decide.
 *   - 1, 2, or 3.
 *   - Default is 0.
 *
 * - DCMF_SAFEALLGATHER - The optimized allgather protocols require
 *   contiguous datatypes and similar datatypes on all nodes.  To verify
 *   this is true, we must do an allreduce at the beginning of the
 *   allgather call.  If the application uses "well behaved" datatypes, you can
 *   set this option to skip over the allreduce.  This is most useful in
 *   irregular subcommunicators where the allreduce can be expensive.
 *   Possible values:
 *   - N - Perform the allreduce.
 *   - Y - Skip the allreduce.  Setting this with "unsafe" datatypes will 
 *         yield unpredictable results, usually hangs.
 *   - Default is N.
 *
 * - DCMF_SAFEALLGATHERV - The optimized allgatherv protocols require
 *   contiguous datatypes and similar datatypes on all nodes.  Allgatherv
 *   also requires continuous displacements.  To verify
 *   this is true, we must do an allreduce at the beginning of the
 *   allgatherv call.  If the application uses "well behaved" datatypes and
 *   displacements, you can set this option to skip over the allreduce. 
 *   This is most useful in irregular subcommunicators where the allreduce 
 *   can be expensive.
 *   Possible values:
 *   - N - Perform the allreduce.
 *   - Y - Skip the allreduce.  Setting this with "unsafe" datatypes will 
 *         yield unpredictable results, usually hangs.
 *   - Default is N.
 *
 * - DCMF_SAFESCATTERV - The optimized scatterv protocol requires
 *   contiguous datatypes and similar datatypes on all nodes.  It
 *   also requires continuous displacements.  To verify
 *   this is true, we must do an allreduce at the beginning of the
 *   scatterv call.  If the application uses "well behaved" datatypes and
 *   displacements, you can set this option to skip over the allreduce. 
 *   This is most useful in irregular subcommunicators where the allreduce 
 *   can be expensive.
 *   Possible values:
 *   - N - Perform the allreduce.
 *   - Y - Skip the allreduce.  Setting this with "unsafe" datatypes will 
 *         yield unpredictable results, usually hangs.
 *   - Default is N.
 *
 * - DCMF_ALLTOALL -
 * - DCMF_ALLTOALLV - 
 * - DCMF_ALLTOALLW - Controls the protocol used for 
 *   alltoall/alltoallv/alltoallw.  Possible values:
 *   - MPICH - Turn off all optimizations and use the MPICH
 *     point-to-point protocol.
 *   - Default (or if anything else is specified) 
 *     is to use an optimized alltoall/alltoallv/alltoallw.
 *
 * - DCMF_ALLTOALL_PREMALLOC -
 * - DCMF_ALLTOALLV_PREMALLOC -
 * - DCMF_ALLTOALLW_PREMALLOC - These are equivalent options. 
 *   The alltoall protocols require 6 arrays to be setup
 *   before communication begins.  These 6 arrays are each of size
 *   (comm_size) so can be sizeable on large machines.  If your application
 *   does not use alltoall, or you need as much memory as possible, you can
 *   turn off pre-allocating these arrays.  By default, we allocate them
 *   once per communicator creation.  There is only one set, regardless of
 *   whether you are using alltoall, alltoallv, or alltoallw.
 *   Possible values:
 *   - Y - Premalloc the arrays.
 *   - N - Malloc and free on every alltoall operation.
 *   - Default is Y.
 *
 * - DCMF_ALLGATHER - Controls the protocol used for allgather.
 Possible values:
 *   - MPICH - Turn off all optimizations for allgather and use the MPICH 
 *     point-to-point protocol.
 *   - ALLREDUCE - Use a collective network based allreduce.  This is the default on 
 *     MPI_COMM_WORLD for smaller messages.
 *   - ALLTOALL - Use an all-to-all based algorithm.  This is the default on
 *     irregular communicators.  It works very well for larger messages.
 *   - BCAST - Use a broadcast.  This will use a collective network broadcast on MPI_COMM_WORLD.
 *     It is the default for larger messages on MPI_COMM_WORLD.  This can work well
 *     on rectangular subcommunicators for smaller messages.
 *   - ASYNC - Use an async broadcast.  This will use asynchronous broadcasts
 *     to do the allgather. This is a good option for small messages on
 *     rectangular or irregular subcommunicators.
 *   - Default varies based on the communicator.  See above.
 *
 * - DCMF_ALLGATHERV - Controls the protocol used for allgatherv. 
 Possible values:
 *   - MPICH - Turn off all optimizations for allgather and use the MPICH 
 *     point-to-point protocol.
 *   - ALLREDUCE - Use a collective network based allreduce.  This is the default on 
 *     MPI_COMM_WORLD for smaller messages.
 *   - ALLTOALL - Use an all-to-all based algorithm.  This is the default on
 *     irregular communicators.  It works very well for larger messages.
 *   - BCAST - Use a broadcast.  This will use a collective network broadcast on MPI_COMM_WORLD.
 *     It is the default for larger messages on MPI_COMM_WORLD.  This can work well
 *     on rectangular subcommunicators for smaller messages.
 *   - ASYNC - Use an async broadcast.  This will use asynchronous broadcasts
 *     to do the allgather. This is a good option for small messages on
 *     rectangular or irregular subcommunicators.
 *   - Default varies based on the communicator.  See above.
 *
 * - DCMF_ALLREDUCE - Controls the protocol used for allreduce.
 *   Possible values:
 *   - MPICH - Turn off all optimizations for allreduce and use the MPICH 
 *     point-to-point protocol.
 *   - RING - Use a rectangular ring protocol.  This is the default for
 *     rectangular subcommunicators.
 *   - RECT - Use a rectangular/binomial protocol.  This is off by default.
 *   - BINOM - Use a binomial protocol.  This is the default for irregular
 *     subcommunicators.
 *   - TREE - Use the collective network.  This is the default for MPI_COMM_WORLD and
 *     duplicates of MPI_COMM_WORLD in MPI_THREAD_SINGLE mode.
 *   - CCMI - Use the CCMI collective network protocol.  This is off by default.
 *   - PIPE - Use the pipelined CCMI collective network protocol. This is off by default.
 *   - ASYNC - Enable the asynchronous versions of BINOM, RING, and RECT 
 *     protocols (above). These are off by default.
 *   - ARING - Enable the asynchronous version of the rectangular ring protocol.  
 *     This is off by default.
 *   - ARECT - Enable the asynchronous version of the rectangular/binomial protocol.  
 *     This is off by default.
 *   - ABINOM - Enable the asynchronous version of the binomial protocol.  
 *     This is off by default.
 *   - Default varies based on the communicator.  See above.
 *
 * - DCMF_ALLREDUCE_REUSE_STORAGE - This allows the lower
 *   level protcols to reuse some storage instead of malloc/free
 *   on every allreduce call.
 *   Possible values:
 *   - Y - Does not malloc/free on every allreduce call.  This improves
 *         performance, but retains malloc'd memory between allreduce calls.
 *   - N - Malloc/free on every allreduce call.  This frees up storage for
 *         use between allreduce calls.
 *   - Default is Y.
 *
 * - DCMF_ALLREDUCE_REUSE_STORAGE_LIMIT - This specifies the upper limit
 *   of storage to save and reuse across allreduce calls when
 *   DCMF_ALLREDUCE_REUSE_STORAGE is set to Y. (This environment variable
 *   is processed within the DCMF_Allreduce_register() API, not in
 *   MPIDI_Env_setup().)
 *   - Default is 1048576 bytes.
 *
 * - DCMF_REDUCE - Controls the protocol used for reduce.
 *   Possible values:
 *   - MPICH - Turn off all optimizations and use the MPICH point-to-point
 *     protocol.
 *   - RECT - Use a rectangular/binomial protocol.  This is the default for
 *     rectangular subcommunicators.
 *   - BINOM - Use a binomial protocol.  This is the default for irregular
 *     subcommunicators.
 *   - TREE - Use the collective network.  This is the default for MPI_COMM_WORLD and
 *     duplicates of MPI_COMM_WORLD in MPI_THREAD_SINGLE mode.
 *   - CCMI - Use the CCMI collective network protocol.  This is off by default.
 *   - Default varies based on the communicator.  See above.
 *
 * - DCMF_REDUCE_REUSE_STORAGE - This allows the lower
 *   level protcols to reuse some storage instead of malloc/free
 *   on every reduce call.
 *   Possible values:
 *   - Y - Does not malloc/free on every reduce call.  This improves
 *         performance, but retains malloc'd memory between reduce calls.
 *   - N - Malloc/free on every reduce call.  This frees up storage for
 *         use between reduce calls.
 *   - Default is Y.
 *
 * - DCMF_REDUCE_REUSE_STORAGE_LIMIT - This specifies the upper limit
 *   of storage to save and reuse across allreduce calls when
 *   DCMF_REDUCE_REUSE_STORAGE is set to Y. (This environment variable
 *   is processed within the DCMF_Reduce_register() API, not in
 *   MPIDI_Env_setup().)
 *   - Default is 1048576 bytes.
 *
 * - DCMF_BARRIER - Controls the protocol used for barriers.
 *   Possible values:
 *   - MPICH - Turn off optimized barriers and use the MPICH 
 *     point-to-point protocol.
 *   - BINOM - Use the binomial barrier.  This is the default for all
 *     subcommunicators.
 *   - GI - Use the GI network.  This is the default for MPI_COMM_WORLD and
 *     duplicates of MPI_COMM_WORLD in MPI_THREAD_SINGLE mode.
 *   - CCMI - Use the CCMI GI network protocol.  This is off by
 *     default.
 *   - Default varies based on the communicator.  See above.
 *
 * - DCMF_STAR: turns on the STAR-MPI mechanism that tunes MPI collectives.
 *              STAR-MPI is turned off by default.
 *   Possible values:
 *   - 1 - turn on
 *
 * - DCMF_STAR_NUM_INVOCS: sets the number of invocations that STAR-MPI uses
 *                         to examine performance of each communication 
 *                         algorithm.
 *   - Possible values:
 *     - any integer value > 0 (default is 10).
 *
 * - DCMF_STAR_TRACEBACK_LEVEL: sets the traceback level of an MPI collective 
 *                              routine, which will be used to get the address 
 *                              of the caller to the collective routine. 
 *                              The default is 3 (MPI app, MPICH, MPIDO). 
 *                              If users utilize or write their own collective 
 *                              wrappers, then they must increase the traceback
 *                              level (beyond 3) depending on the extra levels 
 *                              they introduce by their wrappers.
 *   - Possible values:
 *     - any integer value > 3 (default is 3).
 *
 * - DCMF_STAR_CHECK_CALLSITE: turns on sanity check that makes sure all ranks 
 *                             are involved in the same collective call site. 
 *                             This is important in root-like call sites 
 *                             (Bcast, Reduce, Gather...etc) where the call 
 *                             site of the root might be different than non 
 *                             root ranks (different if 
 *                             statements).
 *   - Possible values:
 *     - 1 - (default is 1: on).
 *
 * - DCMF_STAR_VERBOSE: turns on verbosity of STAR-MPI by writing to an output 
 *                      file in the form "exec_name-star-rank#.log".  
 *                      This is turned off by default.
 *
 *   - Possible values:
 *     - 1 - (default is 0: off).
 *
 */

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
  MPIDO_Embedded_Info_Set * properties = &MPIDI_CollectiveProtocols.properties;
  int   dval = 0;
  char *envopts;
  
  /* unset all properties of a mpidi_coll_protocol by default */
  MPIDO_INFO_ZERO(properties);
  
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

  /* Determine number of outstanding bcast requets (for async protocols) */
  dval = 32;
  ENV_Int(getenv("DCMF_NUMREQUESTS"), &dval);
  MPIDI_CollectiveProtocols.numrequests = dval;

  dval = 0; /* Default is not to use optimized rendezvous. */
  ENV_Int(getenv("DCMF_OPTRVZ"),   &dval);
  ENV_Int(getenv("DCMF_OPTRZV"),   &dval);
  MPIDI_Process.optrzv_limit = MPIDI_Process.eager_limit + dval;

  /* Determine interrupt mode */
  dval = 0;
  ENV_Bool(getenv("DCMF_INTERRUPT"),  &dval);
  ENV_Bool(getenv("DCMF_INTERRUPTS"), &dval);
  MPIDI_Process.use_interrupts = dval;

  /* Enable Sender-side-matching of messages */
  dval = 0;
  ENV_Bool(getenv("DCMF_SENDER_SIDE_MATCHING"), &dval);
  ENV_Bool(getenv("DCMF_SSM"),  &dval);
  MPIDI_Process.use_ssm = dval;

  /* Set the status of the optimized topology functions */
  dval = 1;
  ENV_Bool(getenv("DCMF_TOPOLOGY"), &dval);
  MPIDI_Process.optimized.topology = dval;

  /* Set the status of the optimized collectives */
  MPIDI_Process.optimized.collectives = 1;
  MPIDI_Process.optimized.tree = 1;
  envopts = getenv("DCMF_COLLECTIVE");
  if(envopts == NULL)
    envopts = getenv("DCMF_COLLECTIVES");
  if(envopts != NULL)
  {
    if(strncasecmp(envopts, "NOT", 3) == 0) /* turn off just tree */
    {
      MPIDI_Process.optimized.collectives = 1;
      MPIDI_Process.optimized.tree = 0;
    }
    else if(atoi(envopts)==0)
    {
      MPIDI_Process.optimized.collectives = 0;
      MPIDI_Process.optimized.tree = 0;
    }
    else
    {
      MPIDI_Process.optimized.collectives = 1;
      MPIDI_Process.optimized.tree = 1;
    }
  }
  
  dval = 1000;
  ENV_Int(getenv("DCMF_RMA_PENDING"), &dval);
  MPIDI_Process.rma_pending = dval;
  
  
  /* Initialize selection variables */
  /* turn these off in MPIDI_Coll_register if registration fails
   * or based on env vars (below) */
  if(!MPIDI_Process.optimized.collectives)
    MPIDO_INFO_SET(properties, MPIDO_USE_MPICH); 
  if(!MPIDI_Process.optimized.tree)
    MPIDO_INFO_SET(properties, MPIDO_USE_NOTREE_OPT_COLLECTIVES); 
  
  /* default setting of flags for the mpidi_coll_protocol object */
  MPIDO_MSET_INFO(properties, 
                  MPIDO_USE_GI_BARRIER,
                  MPIDO_USE_CCMI_GI_BARRIER,
                  MPIDO_USE_RECT_BARRIER,
                  MPIDO_USE_BINOM_BARRIER,
                  MPIDO_USE_LOCKBOX_LBARRIER,
                  MPIDO_USE_BINOM_LBARRIER,
                  MPIDO_USE_RECT_DPUT_BCAST,
                  MPIDO_USE_RECT_SINGLETH_BCAST,
                  MPIDO_USE_BINOM_SINGLETH_BCAST,
                  MPIDO_USE_RECT_BCAST,
                  MPIDO_USE_ARECT_BCAST,
                  MPIDO_USE_BINOM_BCAST,
                  MPIDO_USE_ABINOM_BCAST,
                  MPIDO_USE_SCATTER_GATHER_BCAST,
                  MPIDO_USE_TREE_BCAST, 
                  MPIDO_USE_CCMI_TREE_BCAST, 
                  MPIDO_USE_CCMI_TREE_DPUT_BCAST, 
                  MPIDO_USE_STORAGE_ALLREDUCE,
                  MPIDO_USE_RECT_ALLREDUCE,
                  MPIDO_USE_SHORT_ASYNC_RECT_ALLREDUCE,
                  MPIDO_USE_RECTRING_ALLREDUCE,
                  MPIDO_USE_BINOM_ALLREDUCE,
                  MPIDO_USE_ARECT_ALLREDUCE,
                  MPIDO_USE_ARECTRING_ALLREDUCE,
                  MPIDO_USE_ABINOM_ALLREDUCE,
                  MPIDO_USE_RRING_DPUT_SINGLETH_ALLREDUCE,
                  MPIDO_USE_TREE_ALLREDUCE,
                  MPIDO_USE_PIPELINED_TREE_ALLREDUCE,
                  MPIDO_USE_CCMI_TREE_ALLREDUCE,                 
                  MPIDO_USE_STORAGE_REDUCE,
                  MPIDO_USE_RECT_REDUCE,
                  MPIDO_USE_RECTRING_REDUCE,
                  MPIDO_USE_BINOM_REDUCE,
                  MPIDO_USE_TREE_REDUCE, 
                  MPIDO_USE_ALLREDUCE_ALLGATHER,
                  MPIDO_USE_BCAST_ALLGATHER,
                  MPIDO_USE_ABCAST_ALLGATHER,
                  MPIDO_USE_ALLTOALL_ALLGATHER,
                  MPIDO_USE_PREALLREDUCE_ALLGATHER,
                  MPIDO_USE_ARECT_BCAST_ALLGATHER,
                  MPIDO_USE_ALLREDUCE_ALLGATHERV,
                  MPIDO_USE_BCAST_ALLGATHERV,
                  MPIDO_USE_ABCAST_ALLGATHERV,
                  MPIDO_USE_ALLTOALL_ALLGATHERV,
                  MPIDO_USE_PREALLREDUCE_ALLGATHERV,
                  MPIDO_USE_ARECT_BCAST_ALLGATHERV,
                  MPIDO_USE_ALLTOALL_SCATTERV,
                  MPIDO_USE_PREALLREDUCE_SCATTERV,
                  MPIDO_USE_BCAST_SCATTER,
                  MPIDO_USE_REDUCE_GATHER,
                  MPIDO_USE_REDUCESCATTER,
                  MPIDO_USE_TORUS_ALLTOALL,
                  MPIDO_USE_TORUS_ALLTOALLV,
                  MPIDO_USE_TORUS_ALLTOALLW,
                  MPIDO_USE_PREMALLOC_ALLTOALL,
                  MPIDO_END_ARGS);
  
  MPIDI_CollectiveProtocols.bcast_asynccutoff = 8192;
  MPIDI_CollectiveProtocols.allreduce_asynccutoff = 131072;
  MPIDI_CollectiveProtocols.numcolors = 0;

  dval = 8192;
  ENV_Int(getenv("DCMF_ASYNCCUTOFF"), &dval);
  MPIDI_CollectiveProtocols.bcast_asynccutoff = dval;
  
  envopts = getenv("DCMF_SCATTER");
  if(envopts != NULL)
  {
    if(strncasecmp(envopts, "M", 1) == 0) /* mpich */
    {
      MPIDO_INFO_SET(properties, MPIDO_USE_MPICH_SCATTER);
      MPIDO_INFO_UNSET(properties, MPIDO_USE_BCAST_SCATTER);
    }
  }

  envopts = getenv("DCMF_SCATTERV");
  if(envopts != NULL)
  {
    if(strncasecmp(envopts, "M", 1) == 0) /* mpich */
    {
      MPIDO_INFO_SET(properties, MPIDO_USE_MPICH_SCATTERV);
      MPIDO_INFO_UNSET(properties, MPIDO_USE_ALLTOALL_SCATTERV);
      MPIDO_INFO_UNSET(properties, MPIDO_USE_BCAST_SCATTERV);
    }
    else if(strncasecmp(envopts, "A", 1) == 0) /* alltoall */
    {
      MPIDO_INFO_UNSET(properties, MPIDO_USE_BCAST_SCATTERV);
      MPIDO_INFO_SET(properties, MPIDO_USE_ALLTOALL_SCATTERV);
    }
    else if(strncasecmp(envopts, "B", 1) == 0) /* bcast */
    {
      MPIDO_INFO_SET(properties, MPIDO_USE_BCAST_SCATTERV);
      MPIDO_INFO_UNSET(properties, MPIDO_USE_ALLTOALL_SCATTERV);
    }
  }

  envopts = getenv("DCMF_GATHER");
  if(envopts != NULL)
  {
    if(strncasecmp(envopts, "M", 1) == 0)
    {
      MPIDO_INFO_SET(properties, MPIDO_USE_MPICH_GATHER);
      MPIDO_INFO_UNSET(properties, MPIDO_USE_REDUCE_GATHER);
    }
  }
  
  envopts = getenv("DCMF_REDUCESCATTER");
  if(envopts != NULL)
  {
    if(strncasecmp(envopts, "M", 1) == 0) /* mpich */
    {
      MPIDO_INFO_SET(properties, MPIDO_USE_MPICH_REDUCESCATTER);
      MPIDO_INFO_UNSET(properties, MPIDO_USE_REDUCESCATTER);
    }
  }
  
  
  envopts = getenv("DCMF_BCAST");
  if(envopts != NULL)
  {
    MPIDO_INFO_UNSET(properties, MPIDO_USE_TREE_BCAST);
    MPIDO_INFO_UNSET(properties, MPIDO_USE_CCMI_TREE_BCAST);
    MPIDO_INFO_UNSET(properties, MPIDO_USE_CCMI_TREE_DPUT_BCAST);
    MPIDO_INFO_UNSET(properties, MPIDO_USE_RECT_BCAST);
    MPIDO_INFO_UNSET(properties, MPIDO_USE_ARECT_BCAST);
    MPIDO_INFO_UNSET(properties, MPIDO_USE_BINOM_BCAST);
    MPIDO_INFO_UNSET(properties, MPIDO_USE_ABINOM_BCAST);
    MPIDO_INFO_UNSET(properties, MPIDO_USE_BINOM_SINGLETH_BCAST);
    MPIDO_INFO_UNSET(properties, MPIDO_USE_RECT_DPUT_BCAST);
    MPIDO_INFO_UNSET(properties, MPIDO_USE_RECT_SINGLETH_BCAST);
    MPIDO_INFO_UNSET(properties, MPIDO_USE_SCATTER_GATHER_BCAST);
    MPIDO_INFO_SET(properties, MPIDO_BCAST_ENVVAR);

    if(strncasecmp(envopts, "M", 1) == 0) /* MPICH */
    {
      MPIDO_INFO_SET(properties, MPIDO_USE_MPICH_BCAST);
    }
    else if(strncasecmp(envopts, "AR", 2) == 0) /* async rectangle */
    {
      MPIDO_INFO_SET(properties, MPIDO_USE_ARECT_BCAST);
    }
    else if(strncasecmp(envopts, "AB", 2) == 0) /* async binomials */
    {
      MPIDO_INFO_SET(properties, MPIDO_USE_ABINOM_BCAST);
    }
    else if(strncasecmp(envopts, "R", 1) == 0) /* Rectangle */
    {
      MPIDO_INFO_SET(properties, MPIDO_USE_RECT_BCAST);
    }
    else if(strncasecmp(envopts, "B", 1) == 0) /* Binomial */
    {
      MPIDO_INFO_SET(properties, MPIDO_USE_BINOM_BCAST);
    }
    else if(strncasecmp(envopts, "T", 1) == 0) /* Global Tree */
    {
      MPIDO_INFO_SET(properties, MPIDO_USE_TREE_BCAST);
    }
    else if(strncasecmp(envopts, "D", 1) == 0) /* Direct put */
    {
      MPIDO_INFO_SET(properties, MPIDO_USE_RECT_DPUT_BCAST);
    }
    else if(strncasecmp(envopts, "SB", 2) == 0) /* Single thread, binomial */
    {
      MPIDO_INFO_SET(properties, MPIDO_USE_BINOM_SINGLETH_BCAST);
    }
    else if(strncasecmp(envopts, "SR", 2) == 0) /* Single thread, rect */
    {
      MPIDO_INFO_SET(properties, MPIDO_USE_RECT_SINGLETH_BCAST);
    }
    else if(strncasecmp(envopts, "SG", 2) == 0) /* Scatter/gather via pt2pt */
    {
      MPIDO_INFO_SET(properties, MPIDO_USE_SCATTER_GATHER_BCAST);
    }
    else if(strncasecmp(envopts, "CD", 2) == 0) /* CCMI Tree dput*/
    {
      MPIDO_INFO_SET(properties, MPIDO_USE_CCMI_TREE_DPUT_BCAST);
    }
    else if(strncasecmp(envopts, "C", 1) == 0) /* CCMI Tree */
    {
      MPIDO_INFO_SET(properties, MPIDO_USE_CCMI_TREE_BCAST);
    }
    else
    {
      fprintf(stderr,
              "Valid bcasts are: M, AR, AB, R, B, T, C, CD, D, SR, SB, and SG. Using MPICH\n");
      MPIDO_INFO_SET(properties, MPIDO_USE_MPICH_BCAST);
    }
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
  
  envopts = getenv("DCMF_SAFEALLGATHER");
  if(envopts != NULL)
  {
    if((strncasecmp(envopts, "Y", 1) == 0) || atoi(envopts)==1)
      MPIDO_INFO_UNSET(properties, MPIDO_USE_PREALLREDUCE_ALLGATHER);
  }
  
  envopts = getenv("DCMF_SAFEALLGATHERV");
  if(envopts != NULL)
  {
    if((strncasecmp(envopts, "Y", 1) == 0) || atoi(envopts)==1)
      MPIDO_INFO_UNSET(properties, MPIDO_USE_PREALLREDUCE_ALLGATHERV);
  }
  
  envopts = getenv("DCMF_SAFESCATTERV");
  if(envopts != NULL)
  {
    if((strncasecmp(envopts, "Y", 1) == 0) || atoi(envopts)==1)
      MPIDO_INFO_UNSET(properties, MPIDO_USE_PREALLREDUCE_SCATTERV);
  }
  
  
  envopts = getenv("DCMF_ALLTOALL");
  if(envopts != NULL)
  {
    if(strncasecmp(envopts, "M", 1) == 0) /* MPICH */
    {
      MPIDO_INFO_UNSET(properties, MPIDO_USE_TORUS_ALLTOALL);
      MPIDO_INFO_SET(properties, MPIDO_USE_MPICH_ALLTOALL);
    }
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
      MPIDO_INFO_UNSET(properties, MPIDO_USE_PREMALLOC_ALLTOALL);
    }
    else if(strncasecmp(envopts, "Y", 1) == 0) /* defaults to Y */
    {
      MPIDO_INFO_SET(properties, MPIDO_USE_PREMALLOC_ALLTOALL);
    }
    else
      fprintf(stderr,"Invalid DCMF_ALLTOALL(VW)_PREMALLOC option\n");
  }
  
  
  envopts = getenv("DCMF_ALLTOALLV");
  if(envopts != NULL)
  {
    if(strncasecmp(envopts, "M", 1) == 0) /* MPICH */
    {
      MPIDO_INFO_UNSET(properties, MPIDO_USE_TORUS_ALLTOALLV);
      MPIDO_INFO_SET(properties, MPIDO_USE_MPICH_ALLTOALLV);
    }
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
    {
      MPIDO_INFO_UNSET(properties, MPIDO_USE_TORUS_ALLTOALLW);
      MPIDO_INFO_SET(properties, MPIDO_USE_MPICH_ALLTOALLW);
    }
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
    /* The user specified something, so set the env var, then
     * turn off all options and pick the right one. */
    MPIDO_INFO_SET(properties, MPIDO_ALLGATHER_ENVVAR);
    MPIDO_INFO_UNSET(properties, MPIDO_USE_ABCAST_ALLGATHER);
    MPIDO_INFO_UNSET(properties, MPIDO_USE_BCAST_ALLGATHER);
    MPIDO_INFO_UNSET(properties, MPIDO_USE_RECT_BCAST_ALLGATHER);
    MPIDO_INFO_UNSET(properties, MPIDO_USE_ARECT_BCAST_ALLGATHER);
    MPIDO_INFO_UNSET(properties, MPIDO_USE_BINOM_BCAST_ALLGATHER);
    MPIDO_INFO_UNSET(properties, MPIDO_USE_ABINOM_BCAST_ALLGATHER);
    MPIDO_INFO_UNSET(properties, MPIDO_USE_ALLTOALL_ALLGATHER);
    MPIDO_INFO_UNSET(properties, MPIDO_USE_ALLREDUCE_ALLGATHER);

    if(strncasecmp(envopts, "M", 1) == 0) /* MPICH */
    {
      MPIDO_INFO_SET(properties, MPIDO_USE_MPICH_ALLGATHER);
    }
    else if(strncasecmp(envopts, "ALLR", 4) == 0) /* ALLREDUCE */
    {
      MPIDO_INFO_SET(properties, MPIDO_USE_ALLREDUCE_ALLGATHER);
    }
    /* Bcast is never used in the glue, just async bcasts */
    else if(strncasecmp(envopts, "B", 1) == 0) /* BCAST */
    {
      MPIDO_INFO_SET(properties, MPIDO_USE_BCAST_ALLGATHER);
      MPIDO_INFO_SET(properties, MPIDO_USE_RECT_BCAST_ALLGATHER);
      MPIDO_INFO_SET(properties, MPIDO_USE_BINOM_BCAST_ALLGATHER);
    }
    else if(strncasecmp(envopts, "ALLT", 4) == 0) /* ALLTOALL */
    {
      MPIDO_INFO_SET(properties, MPIDO_USE_ALLTOALL_ALLGATHER);
    }
    else if(strncasecmp(envopts, "AS", 2) == 0) /*Async bcast */
    {
      MPIDO_INFO_SET(properties, MPIDO_USE_ABCAST_ALLGATHER);
      MPIDO_INFO_SET(properties, MPIDO_USE_ARECT_BCAST_ALLGATHER);
      MPIDO_INFO_SET(properties, MPIDO_USE_ABINOM_BCAST_ALLGATHER);
    }
    else
    {
      fprintf(stderr,"DCMF_ALLGATHER options: ALLR, BCAST, ALLT, AS, M\n");
      MPIDO_INFO_SET(properties, MPIDO_USE_MPICH_ALLGATHER);
    }
  }
  
  envopts = getenv("DCMF_ALLGATHERV");
  if(envopts != NULL)
  {
    MPIDO_INFO_UNSET(properties, MPIDO_USE_ABCAST_ALLGATHERV);
    MPIDO_INFO_UNSET(properties, MPIDO_USE_BCAST_ALLGATHERV);
    MPIDO_INFO_UNSET(properties, MPIDO_USE_RECT_BCAST_ALLGATHERV);
    MPIDO_INFO_UNSET(properties, MPIDO_USE_ARECT_BCAST_ALLGATHERV);
    MPIDO_INFO_UNSET(properties, MPIDO_USE_BINOM_BCAST_ALLGATHERV);
    MPIDO_INFO_UNSET(properties, MPIDO_USE_ABINOM_BCAST_ALLGATHERV);
    MPIDO_INFO_UNSET(properties, MPIDO_USE_ALLTOALL_ALLGATHERV);
    MPIDO_INFO_UNSET(properties, MPIDO_USE_ALLREDUCE_ALLGATHERV);
    MPIDO_INFO_SET(properties, MPIDO_ALLGATHERV_ENVVAR);

    if(strncasecmp(envopts, "M", 1) == 0) /* MPICH */
    {
      MPIDO_INFO_SET(properties, MPIDO_USE_MPICH_ALLGATHERV);
    }
    else if(strncasecmp(envopts, "ALLR", 4) == 0) /* ALLREDUCE */
    {
      MPIDO_INFO_SET(properties, MPIDO_USE_ALLREDUCE_ALLGATHERV);
    }
    /* Bcast is never used in the glue, just async bcast now */
    else if(strncasecmp(envopts, "B", 1) == 0) /* BCAST */
    {
      MPIDO_INFO_SET(properties, MPIDO_USE_BCAST_ALLGATHERV);
      MPIDO_INFO_SET(properties, MPIDO_USE_RECT_BCAST_ALLGATHERV);
      MPIDO_INFO_SET(properties, MPIDO_USE_BINOM_BCAST_ALLGATHERV);
    }
    else if(strncasecmp(envopts, "ALLT", 4) == 0) /* ALLTOALL */
    {
      MPIDO_INFO_SET(properties, MPIDO_USE_ALLTOALL_ALLGATHERV);
    }
    else if(strncasecmp(envopts, "AS", 2) == 0) /*Async bcast */
    {
      MPIDO_INFO_SET(properties, MPIDO_USE_ABCAST_ALLGATHERV);
      MPIDO_INFO_SET(properties, MPIDO_USE_ARECT_BCAST_ALLGATHERV);
      MPIDO_INFO_SET(properties, MPIDO_USE_ABINOM_BCAST_ALLGATHERV);
    }
    else
    {
      fprintf(stderr,"DCMF_ALLGATHERV options: ALLR, BCAST, ALLT, AS, M\n");
      MPIDO_INFO_SET(properties, MPIDO_USE_MPICH_ALLGATHERV);
    }
  }
  
  envopts = getenv("DCMF_ALLREDUCE");
  if(envopts != NULL)
  {
    /* The user has specified something. Turn off all options for now */
    /* Then, turn on the specific option they want. If they specify an*/
    /* invalid option, use MPICH and let them know                    */
    MPIDO_INFO_UNSET(properties, MPIDO_USE_ARECT_ALLREDUCE);
    MPIDO_INFO_UNSET(properties, MPIDO_USE_ABINOM_ALLREDUCE);
    MPIDO_INFO_UNSET(properties, MPIDO_USE_ARECTRING_ALLREDUCE);
    MPIDO_INFO_UNSET(properties, MPIDO_USE_TREE_ALLREDUCE);
    MPIDO_INFO_UNSET(properties, MPIDO_USE_BINOM_ALLREDUCE);
    MPIDO_INFO_UNSET(properties, MPIDO_USE_RECTRING_ALLREDUCE);
    MPIDO_INFO_UNSET(properties, MPIDO_USE_RECT_ALLREDUCE);
    MPIDO_INFO_UNSET(properties, MPIDO_USE_SHORT_ASYNC_RECT_ALLREDUCE);
    MPIDO_INFO_UNSET(properties, MPIDO_USE_SHORT_ASYNC_BINOM_ALLREDUCE);
    MPIDO_INFO_UNSET(properties, MPIDO_USE_CCMI_TREE_ALLREDUCE);
    MPIDO_INFO_UNSET(properties, MPIDO_USE_PIPELINED_TREE_ALLREDUCE);
    MPIDO_INFO_UNSET(properties, MPIDO_USE_RRING_DPUT_SINGLETH_ALLREDUCE);
    MPIDO_INFO_SET(properties, MPIDO_ALLREDUCE_ENVVAR);

    if(strncasecmp(envopts, "ARI", 3) == 0)
      MPIDO_INFO_SET(properties, MPIDO_USE_ARECTRING_ALLREDUCE);
    else if(strncasecmp(envopts, "AR", 2) == 0)
      MPIDO_INFO_SET(properties, MPIDO_USE_ARECT_ALLREDUCE);
    else if(strncasecmp(envopts, "SR", 2) == 0) /* Short async rect */
      MPIDO_INFO_SET(properties, MPIDO_USE_SHORT_ASYNC_RECT_ALLREDUCE);
    else if(strncasecmp(envopts, "SB", 2) == 0) /* Short async binom */
      MPIDO_INFO_SET(properties, MPIDO_USE_SHORT_ASYNC_BINOM_ALLREDUCE);
    else if(strncasecmp(envopts, "AB", 2) == 0)
      MPIDO_INFO_SET(properties, MPIDO_USE_ABINOM_ALLREDUCE);
    else if(strncasecmp(envopts, "RI", 2) == 0)
      MPIDO_INFO_SET(properties, MPIDO_USE_RECTRING_ALLREDUCE);
    else if(strncasecmp(envopts, "R", 1) == 0)
      MPIDO_INFO_SET(properties, MPIDO_USE_RECT_ALLREDUCE);
    else if(strncasecmp(envopts, "C", 1) == 0)
      MPIDO_INFO_SET(properties, MPIDO_USE_CCMI_TREE_ALLREDUCE);
    else if(strncasecmp(envopts, "B", 1) == 0)
      MPIDO_INFO_SET(properties, MPIDO_USE_BINOM_ALLREDUCE);
    else if(strncasecmp(envopts, "M", 1) == 0) /* MPICH */
    {
      MPIDO_INFO_SET(properties, MPIDO_USE_MPICH_ALLREDUCE);
    }
    else if(strncasecmp(envopts, "T", 1) == 0) /* Tree */
      MPIDO_INFO_SET(properties, MPIDO_USE_TREE_ALLREDUCE);
    else if(strncasecmp(envopts, "P", 1) == 0) /* CCMI Pipelined Tree */
      MPIDO_INFO_SET(properties, MPIDO_USE_PIPELINED_TREE_ALLREDUCE);
    else if(strncasecmp(envopts, "D", 1) == 0) /* Rect dput */
      MPIDO_INFO_SET(properties, MPIDO_USE_RRING_DPUT_SINGLETH_ALLREDUCE);
    else
    {
      fprintf(stderr,
              "Invalid DCMF_ALLREDUCE option - %s. Using mpich\n", envopts);
      MPIDO_INFO_SET(properties, MPIDO_USE_MPICH_ALLREDUCE);
    }
  }
  
  envopts = getenv("DCMF_ALLREDUCE_REUSE_STORAGE");
  if(envopts != NULL)
  {
    if(strncasecmp(envopts, "N", 1) == 0) /* Do not reuse the malloc'd storage */
    {
      MPIDO_INFO_UNSET(properties, MPIDO_USE_STORAGE_ALLREDUCE);
      /*	  fprintf(stderr, "N allreduce.reusestorage %X\n", 0);	  */
    }
    else if (strncasecmp(envopts, "Y", 1) == 0); /* defaults to Y */
    else
      fprintf(stderr,"Invalid DCMF_ALLREDUCE_REUSE_STORAGE option\n");
  }

  envopts = getenv("DCMF_REDUCE_PREMALLOC");
  if(envopts != NULL)
  {
    if(strncasecmp(envopts, "N", 1) == 0)
    {
      MPIDO_INFO_UNSET(properties, MPIDO_USE_PREMALLOC_REDUCE);
      MPIDO_INFO_UNSET(properties, MPIDO_USE_ALLREDUCE_REDUCE);
    }
    else if(strncasecmp(envopts, "Y", 1) == 0) /* defaults to Y */
    {
      MPIDO_INFO_SET(properties, MPIDO_USE_PREMALLOC_REDUCE);
      MPIDO_INFO_SET(properties, MPIDO_USE_ALLREDUCE_REDUCE);
    }
    else
      fprintf(stderr,"Invalid DCMF_REDUCE_PREMALLOC option\n");
  }

  envopts = getenv("DCMF_REDUCE");
  if(envopts != NULL)
  {
    MPIDO_INFO_SET(properties, MPIDO_REDUCE_ENVVAR);
    MPIDO_INFO_UNSET(properties, MPIDO_USE_ALLREDUCE_REDUCE);
    MPIDO_INFO_UNSET(properties, MPIDO_USE_TREE_REDUCE);
    MPIDO_INFO_UNSET(properties, MPIDO_USE_BINOM_REDUCE);
    MPIDO_INFO_UNSET(properties, MPIDO_USE_RECTRING_REDUCE);
    MPIDO_INFO_UNSET(properties, MPIDO_USE_RECT_REDUCE);
    MPIDO_INFO_UNSET(properties, MPIDO_USE_CCMI_TREE_REDUCE);

    if(strncasecmp(envopts, "M", 1) == 0) /* MPICH */
    {
      MPIDO_INFO_SET(properties, MPIDO_USE_MPICH_REDUCE);
    }
    else if(strncasecmp(envopts, "RI", 2) == 0) /* Rectangle Ring*/
    {
      MPIDO_INFO_SET(properties, MPIDO_USE_RECTRING_REDUCE);
    }
    else if(strncasecmp(envopts, "A", 1) == 0) /* Allreduce */
    {
      MPIDO_INFO_SET(properties, MPIDO_USE_ALLREDUCE_REDUCE);
      MPIDO_INFO_SET(properties, MPIDO_USE_PREMALLOC_REDUCE);
    }
    else if(strncasecmp(envopts, "R", 1) == 0) /* Rectangle */
    {
      MPIDO_INFO_SET(properties, MPIDO_USE_RECT_REDUCE);
    }
    else if(strncasecmp(envopts, "B", 1) == 0) /* Binomial */
    {
      MPIDO_INFO_SET(properties, MPIDO_USE_BINOM_REDUCE);
    }
    else if(strncasecmp(envopts, "T", 1) == 0) /* Tree */
    {
      MPIDO_INFO_SET(properties, MPIDO_USE_TREE_REDUCE);
    }
    else if(strncasecmp(envopts, "C", 1) == 0) /* CCMI Tree */
    {
      MPIDO_INFO_SET(properties, MPIDO_USE_CCMI_TREE_REDUCE);
    }
    else
    {
      fprintf(stderr,"Invalid DCMF_REDUCE option. Defaulting to MPICH\n");
      MPIDO_INFO_SET(properties, MPIDO_USE_MPICH_REDUCE);
    }
  }
  
  envopts = getenv("DCMF_REDUCE_REUSE_STORAGE");
  if(envopts != NULL)
  {
    if(strncasecmp(envopts, "N", 1) == 0) /* Do not reuse the malloc'd storage */
    {
      MPIDO_INFO_UNSET(properties, MPIDO_USE_STORAGE_REDUCE);
      /*fprintf(stderr, "N protocol.reusestorage %X\n", 0);*/
    }
    else if(strncasecmp(envopts, "Y", 1) == 0); /* defaults to Y */
    else
      fprintf(stderr,"Invalid DCMF_REDUCE_REUSE_STORAGE option\n");
  }
  
  envopts = getenv("DCMF_BARRIER");
  if(envopts != NULL)
  {
    if(strncasecmp(envopts, "M", 1) == 0) /* MPICH */
    {
      MPIDO_INFO_SET(properties, MPIDO_USE_MPICH_BARRIER);

      /* still need to register a barrier for DCMF collectives */
      MPIDO_INFO_SET(properties, MPIDO_USE_BINOM_BARRIER);
         
      /* MPIDI_Coll_register changes this state for us */
      /* MPIDI_CollectiveProtocols.barrier.usegi = 1; */
    }
    else if(strncasecmp(envopts, "B", 1) == 0) /* Binomial */
    {
      MPIDO_INFO_UNSET(properties, MPIDO_USE_GI_BARRIER);
      MPIDO_INFO_UNSET(properties, MPIDO_USE_CCMI_GI_BARRIER);
      MPIDO_INFO_UNSET(properties, MPIDO_USE_RECT_BARRIER);
    }
    else if(strncasecmp(envopts, "G", 1) == 0) /* GI */
    {
      MPIDO_INFO_UNSET(properties, MPIDO_USE_CCMI_GI_BARRIER);
      MPIDO_INFO_UNSET(properties, MPIDO_USE_BINOM_BARRIER);
      MPIDO_INFO_UNSET(properties, MPIDO_USE_RECT_BARRIER);
    }
    else if(strncasecmp(envopts, "R", 1) == 0) /* Rect */
    {
      MPIDO_INFO_UNSET(properties, MPIDO_USE_CCMI_GI_BARRIER);
      MPIDO_INFO_UNSET(properties, MPIDO_USE_BINOM_BARRIER);
      MPIDO_INFO_UNSET(properties, MPIDO_USE_GI_BARRIER);
    }
    else if(strncasecmp(envopts, "C", 1) == 0) /* GI */
    {
      MPIDO_INFO_UNSET(properties, MPIDO_USE_BINOM_BARRIER);
      MPIDO_INFO_UNSET(properties, MPIDO_USE_RECT_BARRIER);
      MPIDO_INFO_UNSET(properties, MPIDO_USE_GI_BARRIER);
    }
    else
      fprintf(stderr,"Invalid DCMF_BARRIER option\n");
  }
  
  envopts = getenv("DCMF_LOCALBARRIER");
  if(envopts != NULL)
  {
    if(strncasecmp(envopts, "B", 1) == 0) /* Binomial */
    {
      MPIDO_INFO_UNSET(properties, MPIDO_USE_LOCKBOX_LBARRIER);
    }
    else if(strncasecmp(envopts, "L", 1) == 0) /* Lockbox */
    {
      MPIDO_INFO_UNSET(properties, MPIDO_USE_BINOM_LBARRIER);
    }
    else
      fprintf(stderr,"Invalid DCMF_LOCALBARRIER option\n");
  }

  /* star-mpi is off by default unless user requests it */
  dval = 0;
  ENV_Int(getenv("DCMF_STAR"), &dval);
  if (dval > 0 )
  {
    STAR_info.enabled = dval;

    /* initialize the repositories of STAR */
    STAR_InitRepositories();
  }

  dval = 0;
  ENV_Int(getenv("DCMF_STAR_NUM_INVOCS"), &dval);
  if (dval > 0 )
    STAR_info.invocs_per_algorithm = dval;
  
  dval = 0;
  ENV_Int(getenv("DCMF_STAR_TRACEBACK_LEVEL"), &dval);
  if (dval > 0)
    STAR_info.traceback_levels = dval;
  
  dval = 0;
  ENV_Int(getenv("DCMF_STAR_CHECK_CALLSITE"), &dval);
  if (dval > 0)
    STAR_info.agree_on_callsite = dval;

  ENV_Int(getenv("DCMF_STAR_VERBOSE"), &dval);
  if (STAR_info.enabled && dval > 0)
    STAR_info.debug = dval;
  dval = -1;
  ENV_Int(getenv("DCMF_STAR_ALLTOALL_THRESHOLD"), &dval);
  if (dval >= 0 )
    STAR_info.alltoall_threshold = dval;
  dval = -1;
  ENV_Int(getenv("DCMF_STAR_ALLGATHER_THRESHOLD"), &dval);
  if (dval >= 0 )
    STAR_info.allgather_threshold = dval;
  dval = -1;
  ENV_Int(getenv("DCMF_STAR_ALLGATHERV_THRESHOLD"), &dval);
  if (dval >= 0 )
    STAR_info.allgatherv_threshold = dval;
  dval = -1;
  ENV_Int(getenv("DCMF_STAR_ALLREDUCE_THRESHOLD"), &dval);
  if (dval >= 0 )
    STAR_info.allreduce_threshold = dval;
  dval = -1;
  ENV_Int(getenv("DCMF_STAR_REDUCE_THRESHOLD"), &dval);
  if (dval >= 0 )
    STAR_info.reduce_threshold = dval;
  dval = -1;
  ENV_Int(getenv("DCMF_STAR_BCAST_THRESHOLD"), &dval);
  if (dval >= 0 )
    STAR_info.bcast_threshold = dval;
  dval = -1;
  ENV_Int(getenv("DCMF_STAR_SCATTER_THRESHOLD"), &dval);
  if (dval >= 0 )
    STAR_info.scatter_threshold = dval;
  dval = -1;
  ENV_Int(getenv("DCMF_STAR_GATHER_THRESHOLD"), &dval);
  if (dval >= 0 )
    STAR_info.gather_threshold = dval;
   
  /* end of star stuff */  
}

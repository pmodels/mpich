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
 * \file src/mpidi_env.c
 * \brief Read Env Vars
 */

#include <mpidimpl.h>

/** \page env_vars Environment Variables
 ***************************************************************************
 *                             Pt2Pt Options                               *
 ***************************************************************************
 *
 * - PAMID_EAGER -
 * - PAMID_RZV - Sets the cutoff for the switch to the rendezvous protocol.
 *   The two options are identical.  This takes an argument, in bytes,
 *   to switch from the eager protocol to the rendezvous protocol for point-to-point
 *   messaging.  Increasing the limit might help for larger partitions
 *   and if most of the communication is nearest neighbor.
 *   - Default is 2049 bytes.
 *
 * - PAMID_EAGER_LOCAL -
 * - PAMID_RZV_LOCAL - Sets the cutoff for the switch to the rendezvous protocol
 *   when the destination rank is local. The two options are identical.  This
 *   takes an argument, in bytes, to switch from the eager protocol to the
 *   rendezvous protocol for point-to-point messaging.  The default value
 *   effectively disables the eager protocol for local transfers.
 *   - Default is 0 bytes.
 *
 * - PAMID_RMA_PENDING - Maximum outstanding RMA requests.
 *   Limits number of PAMI_Request objects allocated by MPI Onesided operations.
 *   - Default is 1000.
 *
 * - PAMID_SHMEM_PT2PT - Determines if intranode point-to-point communication will
 *   use the optimized shared memory protocols.
 *   - Default is 1.
 ***************************************************************************
 *                               High-level Options                        *
 ***************************************************************************
 *
 * - PAMID_THREAD_MULTIPLE - Specifies the messaging execution environment.
 *   It specifically selects whether there can be multiple independent
 *   communications occurring in parallel, driven by internal communications
 *   threads.
 *   - 0: The application threads drive the communications. No additional
 *        internal communications threads are used.
 *   - 1: There can be multiple independent communications occurring in
 *        parallel, driven by internal communications threads.
 *   Defaults to 0 when the application is linked with one of the "legacy"
 *   MPICH libraries (gcc.legacy, xl.legacy, xl.legacy.ndebug), or when
 *   MPI_Init_thread() is called without MPI_THREAD_MULTIPLE, and defaults
 *   to 1 when the application is linked with one of the "non-legacy" MPICH
 *   libraries (gcc, xl, xl.ndebug) and MPI_Init_thread() is called with
 *   MPI_THREAD_MULTIPLE.
 *   - NOTE: This environment variable has the same effect as setting
 *           MPICH_THREADLEVEL_DEFAULT=multiple
 *
 * - PAMID_CONTEXT_MAX - This variable sets the maximum allowable number
 *   of contexts. Contexts are a method of dividing hardware resources
 *   among a parallel active messaging interface (PAMI) client (for example,
 *   MPI) to set how many parallel operations can occur at one time. Contexts
 *   are similar to channels in a communications system. The practical
 *   maximum is usually 64 processes per node. The default depends on the
 *   processes per node.
 *
 * - PAMID_CONTEXT_POST - This variable is required to take advantage of
 *   the parallelism of multiple contexts. This variable can increase
 *   latency, but it is the only way to allow parallelism among contexts.
 *   - 0: Only one parallel communications context can be used. Each
 *        operation runs in the application thread.
 *   - 1: Multiple parallel communications contexts can be used. An
 *        operation is posted to one of the contexts, and communications
 *        for that context are driven by communications threads.
 *   The default value is 1 when using the gcc, xl, and xl.ndebug libraries
 *   and MPI_Init_thread(...  MPI_THREAD_MULTIPLE ...), and 0 otherwise.
 *
 * - PAMID_ASYNC_PROGRESS - This variable enables or disables the async
 *   progress extension.
 *
 * - PAMID_COLLECTIVES - Controls whether optimized collectives are used.
 *   Possible values:
 *   - 0 - Optimized collectives are not used.
 *   - 1 - Optimized collectives are used.
 *   If this is set to 0, only MPICH point-to-point based collectives will be used.
 *
 * - PAMID_COLLECTIVES_SELECTION - Turns on optimized collective selection. If this
 *   is not on, only the generic PGAS "always works" algorithms will be selected. This
 *   can still be better than PAMID_COLLECTIVES=0. Additionally, setting this off
 *   still allows environment variable selection of individual collectives protocols.
 *   - 0 - Optimized collective selection is not used.
 *   - 1 - Optimized collective selection is used. (default)
 *
 * - PAMID_VERBOSE - Increases the amount of information dumped during an
 *   MPI_Abort() call and during varoius MPI function calls.  Possible values:
 *   - 0 - No additional information is dumped.
 *   - 1 - Useful information is printed from MPI_Init(). This option does
 *   NOT impact performance (other than a tiny bit during MPI_Init()) and
 *   is highly recommended for all applications. It will display which PAMID_
 *   and BG_ env vars were used. However, it does NOT guarantee you typed the
 *   env vars in correctly :)
 *   - 2 - This turns on a lot of verbose output for collective selection,
 *   including a list of which protocols are viable for each communicator
 *   creation. This can be a lot of output, but typically only at 
 *   communicator creation. Additionally, if PAMID_STATISTICS are on,
 *   queue depths for point-to-point operations will be printed for each node 
 *   during MPI_Finalize();
 *   - 3 - This turns on a lot of per-node information (plus everything 
 *   at the verbose=2 level). This can be a lot of information and is
 *   rarely recommended.
 *   - Default is 0. However, setting to 1 is recommended.
 *
 * - PAMID_STATISTICS - Turns on statistics printing for the message layer
 *   such as the maximum receive queue depth.  Possible values:
 *   - 0 - No statistics are printedcmf_bcas.
 *   - 1 - Statistics are printed.
 *   - Default is 0.
 *
 ***************************************************************************
 ***************************************************************************
 **                           Collective Options                          **
 ***************************************************************************
 ***************************************************************************
 *
 ***************************************************************************
 *                       General Collectives Options                       *
 ***************************************************************************
 *
 * - PAMID_NUMREQUESTS - Sets the number of outstanding asynchronous
 *   broadcasts to have before a barrier is called.  This is mostly
 *   used in allgather/allgatherv using asynchronous broadcasts.
 *   Higher numbers can help on larger partitions and larger
 *   message sizes. This is also used for asynchronous broadcasts.
 *   After every {PAMID_NUMREQUESTS} async bcasts, the "glue" will call
 *   a barrier. See PAMID_BCAST and PAMID_ALLGATHER(V) for more information
 *   - Default is 32.
 *
 ***************************************************************************
 *                            "Safety" Options                             *
 ***************************************************************************
 *
 * - PAMID_SAFEALLGATHER - Some optimized allgather protocols require
 *   contiguous datatypes and similar datatypes on all nodes.  To verify
 *   this is true, we must do an allreduce at the beginning of the
 *   allgather call.  If the application uses "well behaved" datatypes, you can
 *   set this option to skip over the allreduce.  This is most useful in
 *   irregular subcommunicators where the allreduce can be expensive and in
 *   applications calling MPI_Allgather() with simple/regular datatypes.
 *   Datatypes must also be 16-byte aligned to use the optimized
 *   broadcast-based allgather for larger allgather sendcounts. See
 *   PAMID_PREALLREDUCE for more options
 *   Possible values:
 *   - N - Perform the preallreduce.
 *   - Y - Skip the preallreduce.  Setting this with "unsafe" datatypes will
 *         yield unpredictable results, usually hangs.
 *   - Default is N.
 *
 * - PAMID_SAFEALLGATHERV - The optimized allgatherv protocols require
 *   contiguous datatypes and similar datatypes on all nodes.  Allgatherv
 *   also requires continuous displacements.  To verify
 *   this is true, we must do an allreduce at the beginning of the
 *   allgatherv call.  If the application uses "well behaved" datatypes and
 *   displacements, you can set this option to skip over the allreduce.
 *   This is most useful in irregular subcommunicators where the allreduce
 *   can be expensive and in applications calling MPI_Allgatherv() with
 *   simple/regular datatypes.
 *   Datatypes must also be 16-byte aligned to use the optimized
 *   broadcast-based allgather for larger allgather sendcounts. See
 *   PAMID_PREALLREDUCE for more options
 *   Possible values:
 *   - N - Perform the allreduce.
 *   - Y - Skip the allreduce.  Setting this with "unsafe" datatypes will
 *         yield unpredictable results, usually hangs.
 *   - Default is N.
 *
 * - PAMID_SAFESCATTERV - The optimized scatterv protocol requires
 *   contiguous datatypes and similar datatypes on all nodes.  It
 *   also requires continuous displacements.  To verify
 *   this is true, we must do an allreduce at the beginning of the
 *   scatterv call.  If the application uses "well behaved" datatypes and
 *   displacements, you can set this option to skip over the allreduce.
 *   This is most useful in irregular subcommunicators where the allreduce
 *   can be expensive. We have seen more applications with strange datatypes
 *   passed to scatterv than allgather/allgatherv/bcast/allreduce, so it is
 *   more likely you need to leave this at the default setting. However, we
 *   encourage you to try turning this off and seeing if your application
 *   completes successfully.
 *   Possible values:
 *   - N - Perform the allreduce.
 *   - Y - Skip the allreduce.  Setting this with "unsafe" datatypes will
 *         yield unpredictable results, usually hangs.
 *   - Default is N.
 *
 * - PAMID_PREALLREDUCE - Controls the protocol used for the pre-allreducing
 *   employed by allgather(v), and scatterv. This option
 *   is independant from PAMID_ALLREDUCE. The available protocols can be 
 *   determined with PAMID_VERBOSE=2. If collective selection is on, we will
 *   attempt to use the fastest protocol for the given communicator.
 *
 ***************************************************************************
 *                      Specific Collectives Switches                      *
 ***************************************************************************
 *
 * - PAMID_ALLGATHER - Controls the protocol used for allgather.
 *   Possible values:
 *   - MPICH - Turn off all optimizations for allgather and use the MPICH
 *     point-to-point protocol. This can be very bad on larger partitions.
 *  - Other options can be determined by running with PAMID_VERBOSE=2.
 *   TODO: Implement the NO options
 *   NOTE: Individual options may be turned off by prefixing them with "NO".
 *   For example, PAMID_ALLGATHER=NO{specific protocol name} will prevent the 
 *   use of the specified protocol.
 *
 * - PAMID_ALLGATHERV - Controls the protocol used for allgatherv.
 *   Possible values:
 *   - MPICH - Turn off all optimizations for allgather and use the MPICH
 *     point-to-point protocol. This can be very bad on larger partitions.
 *  - Other options can be determined by running with PAMID_VERBOSE=2.
 *   TODO: Implement the NO options
 *   NOTE: Individual options may be turned off by prefixing them with "NO".
 *   For example, PAMID_ALLGATHERV=NO{specific protocol name} will prevent the 
 *   use of the specified protocol.
 *
 * - PAMID_ALLREDUCE - Controls the protocol used for allreduce.
 *   Possible values:
 *   - MPICH - Turn off all optimizations for allreduce and use the MPICH
 *     point-to-point protocol.
 *   - Other options can be determined by running with PAMID_VERBOSE=2.
 *   Note: Many allreduce algorithms are in the "must query" category and 
 *   might or might not be available for a specific callsite.
 *   TODO: Implement the NO options
 *   NOTE: Individual options may be turned off by prefixing them with "NO".
 *   For example, PAMID_ALLREDUCE=NO{specific protocol name} will prevent the 
 *   use of the specified protocol.
 *
 * - PAMID_ALLTOALL -
 * - PAMID_ALLTOALLV - Controls the protocol used for
 *   alltoall/alltoallv  Possible values:
 *   - MPICH - Turn off all optimizations and use the MPICH
 *     point-to-point protocol.
 *   - Other options can be determined by running with PAMID_VERBOSE=2.
 *   TODO: Implement the NO options
 *   NOTE: Individual options may be turned off by prefixing them with "NO".
 *   For example, PAMID_ALLTOALL=NO{specific protocol name} will prevent the 
 *   use of the specified protocol.
 *
 *
 * - PAMID_BARRIER - Controls the protocol used for barriers.
 *   Possible values:
 *   - MPICH - Turn off optimized barriers and use the MPICH
 *     point-to-point protocol.
 *   - Other options can be determined by running with PAMID_VERBOSE=2.
 *   TODO: Implement the NO options
 *   NOTE: Individual options may be turned off by prefixing them with "NO".
 *   For example, PAMID_BARRIER=NO{specific protocol name} will prevent the 
 *   use of the specified protocol.
 *
 * - PAMID_BCAST - Controls the protocol used for broadcast.  Possible values:
 *   - MPICH - Turn off all optimizations for broadcast and use the MPICH
 *     point-to-point protocol.
 *   - Other options can be determined by running with PAMID_VERBOSE=2.
 *   TODO: Implement the NO options
 *   NOTE: Individual options may be turned off by prefixing them with "NO".
 *   For example, PAMID_BCAST=NO{specific protocol name} will prevent the 
 *   use of the specified protocol.
 *
 * - PAMID_GATHER - Controls the protocol used for gather.
 *   Possible values:
 *   - MPICH - Use the MPICH point-to-point protocol.
 *   - Other options can be determined by running with PAMID_VERBOSE=2.
 *   TODO: Implement the NO options
 *   NOTE: Individual options may be turned off by prefixing them with "NO".
 *   For example, PAMID_GATHER=NO{specific protocol name} will prevent the 
 *   use of the specified protocol.
 *
 * - PAMID_REDUCE - Controls the protocol used for reduce.
 *   Possible values:
 *   - MPICH - Turn off all optimizations and use the MPICH point-to-point
 *     protocol.
 *   - Other options can be determined by running with PAMID_VERBOSE=2.
 *   TODO: Implement the NO options
 *   NOTE: Individual options may be turned off by prefixing them with "NO".
 *   For example, PAMID_REDUCE=NO{specific protocol name} will prevent the 
 *   use of the specified protocol.
 *
 * - PAMID_REDUCE_SCATTER - Controls the protocol used for reduce_scatter
 *   operations.  The options for PAMID_SCATTERV and PAMID_REDUCE can change the
 *   behavior of reduce_scatter.  Possible values:
 *   - MPICH - Use the MPICH point-to-point protocol.
 *   - Other options can be determined by running with PAMID_VERBOSE=2.
 *   TODO: Implement the NO options
 *   NOTE: Individual options may be turned off by prefixing them with "NO".
 *   For example, PAMID_REDUCESCATTER=NO{specific protocol name} will prevent the 
 *   use of the specified protocol.
 *
 * - PAMID_SCATTER - Controls the protocol used for scatter.
 *   Possible values:
 *   - MPICH - Use the MPICH point-to-point protocol.
 *   - Other options can be determined by running with PAMID_VERBOSE=2.
 *   TODO: Implement the NO options
 *   NOTE: Individual options may be turned off by prefixing them with "NO".
 *   For example, PAMID_SCATTER=NO{specific protocol name} will prevent the 
 *   use of the specified protocol.
 *
 * - PAMID_SCATTERV - Controls the protocol used for scatterv.
 *   Possible values:
 *   - MPICH - Use the MPICH point-to-point protocol.
 *   - Other options can be determined by running with PAMID_VERBOSE=2.
 *   TODO: Implement the NO options
 *   NOTE: Individual options may be turned off by prefixing them with "NO".
 *   For example, PAMID_SCATTERV=NO{specific protocol name} will prevent the 
 *   use of the specified protocol.
 *
 *
 */
#if (MPIDI_STATISTICS || MPIDI_PRINTENV)
int prtStat=0;
int prtEnv=0;
int numTasks=0;
MPIX_stats_t *mpid_statp=NULL;
extern MPIDI_printenv_t  *mpich_env;
#endif

#define ENV_Deprecated(a, b, c, d, e) ENV_Deprecated__(a, b, c, d, e)
static inline void
ENV_Deprecated__(char* name[], unsigned num_supported, unsigned* deprecated, int rank, int NA)
{
  if (name == NULL) return;

  unsigned i;
  char * env;

  for (i=0; i<num_supported; ++i)
    {
      if (name[i] == NULL) return;

      if (NA)
        {
          env = getenv(name[i]);
          if (env != NULL)
            {
              if (rank == 0)
                {
                  if (*deprecated == 0)
                    fprintf (stderr, "\n");

                  fprintf (stderr, "The environment variable \"%s\" is not applicable.\n", name[i]);
                }
              *deprecated = 1;
            }
        }
    }

  for (i=num_supported; name[i] != NULL; ++i)
    {
      env = getenv(name[i]);
      if (env != NULL)
        {
          if (rank == 0)
            {
              if (*deprecated == 0)
                fprintf (stderr, "\n");

              if (NA)
                fprintf (stderr, "The environment variable \"%s\" is deprecated.\n", name[i]);
              else
                {
                  char supported[10240];
                  int n, index = 0;
                  index += snprintf(&supported[index], 10240-index-1, "\"%s\"", name[0]);
                  for (n=1; n<num_supported; ++n)
                    index += snprintf(&supported[index], 10240-index-1, " or \"%s\"", name[n]);
                  
                  fprintf (stderr, "The environment variable \"%s\" is deprecated. Consider using %s instead.\n", name[i], supported);
                }
            }
          *deprecated = 1;
        }
    }
}

#define ENV_Unsigned(a, b, c, d, e) ENV_Unsigned__(a, b, #b, c, d, e, 0)

#define ENV_Unsigned_NA(a, b, c, d, e) ENV_Unsigned__(a, b, #b, c, d, e, 1)

static inline void
ENV_Unsigned__(char* name[], unsigned* val, char* string, unsigned num_supported, unsigned* deprecated, int rank, int NA)
{
  /* Check for deprecated environment variables. */
  ENV_Deprecated(name, num_supported, deprecated, rank, NA);

  char * env;

  unsigned i=0;
  for (;; ++i) {
    if (name[i] == NULL)
      return;
    env = getenv(name[i]);
    if (env != NULL)
      break;
  }

  *val = atoi(env);
  if (MPIDI_Process.verbose >= MPIDI_VERBOSE_DETAILS_ALL && rank == 0)
    fprintf(stderr, "%s = %u\n", string, *val);
}


#define ENV_Char(a, b) ENV_Char__(a, b, #b)
static inline void
ENV_Char__(char* name[], unsigned* val, char* string)
{
  char * env;
  unsigned i=0;
  for (;; ++i) {
    if (name[i] == NULL)
      return;
    env = getenv(name[i]);
    if (env != NULL)
      break;
  }

  if ((env[0]=='y')|| (env[0]=='Y')|| (env[0]=='p')|| (env[0]=='P') || (env[0]=='F')|| (env[0]=='f'))
          *val = 1;
  /*This may seem redundant; however, 
    in some cases we need to force val=0 if value = no/none*/
  if ((env[0]=='n')|| (env[0]=='N')) 
          *val = 0;  
  if (MPIDI_Process.verbose >= MPIDI_VERBOSE_DETAILS_ALL)
    fprintf(stderr, "%s = %u\n", string, *val);
}

#define ENV_Char2(a, b) ENV_Char2__(a, b, #b)
static inline void
ENV_Char2__(char* name[], char** val, char* string)
{
  unsigned i=0;
  for (;; ++i) {
    if (name[i] == NULL)
      return;
    *val = getenv(name[i]);
    if (*val != NULL)
      break;
  }

  if (MPIDI_Process.verbose >= MPIDI_VERBOSE_DETAILS_ALL)
    fprintf(stderr, "%s = %s\n", string, *val);
}

/** \brief Checks the Environment variables at initialization and stores the results.
 *
 * \param [in] rank       The process rank; used to limit output
 * \param [in] requested  The thread model requested by the user
 */
void
MPIDI_Env_setup(int rank, int requested)
{
  unsigned found_deprecated_env_var = 0;

  /* Set defaults for various environment variables */

  /* Set the verbosity level.  When set, this will print information at finalize. */
  {
    char* names[] = {"PAMID_VERBOSE", "PAMI_VERBOSE", NULL};
    ENV_Unsigned(names, &MPIDI_Process.verbose, 1, &found_deprecated_env_var, rank);
  }

  /* Enable statistics collection. */
  {
    char* names[] = {"PAMID_STATISTICS", "PAMI_STATISTICS", NULL};
    ENV_Unsigned(names, &MPIDI_Process.statistics, 1, &found_deprecated_env_var, rank);
  }

  /* "Globally" set the optimization flag for low-level collectives in geometry creation.
   * This is probably temporary. metadata should set this flag likely.
   */
  {
    /* THIS ENVIRONMENT VARIABLE NEEDS TO BE DOCUMENTED ABOVE */
    char *names[] = {"PAMID_OPTIMIZED_SUBCOMMS", "PAMI_OPTIMIZED_SUBCOMMS", NULL};
    ENV_Unsigned(names, &MPIDI_Process.optimized.subcomms, 1, &found_deprecated_env_var, rank);
    TRACE_ERR("MPIDI_Process.optimized.subcomms=%u\n", MPIDI_Process.optimized.subcomms);
  }

  /* Set the threads values from a single override. */
  {
    unsigned value = (unsigned)-1;
    char* names[] = {"PAMID_THREAD_MULTIPLE", NULL};
    ENV_Unsigned(names, &value, 1, &found_deprecated_env_var, rank);
#if (MPIU_THREAD_GRANULARITY == MPIU_THREAD_GRANULARITY_PER_OBJECT)
    if (value == 1)      /* force on  */
    {
      /* The default value for context post should be enabled only if the
       * default async progress mode is the 'locked' mode.
       */
      MPIDI_Process.perobj.context_post.requested =
        (ASYNC_PROGRESS_MODE_DEFAULT == ASYNC_PROGRESS_MODE_LOCKED);

      MPIDI_Process.async_progress.mode    = ASYNC_PROGRESS_MODE_DEFAULT;
      MPIDI_Process.avail_contexts         = MPIDI_MAX_CONTEXTS;
    }
    else if (value == 0) /* force off */
    {
      MPIDI_Process.perobj.context_post.requested = 0;
      MPIDI_Process.async_progress.mode    = ASYNC_PROGRESS_MODE_DISABLED;
      MPIDI_Process.avail_contexts         = 1;
    }
    else if (requested != MPI_THREAD_MULTIPLE)
    {
      /* The PAMID_THREAD_MULTIPLE override was not set, yet the application
       * requested a thread mode other than MPI_THREAD_MULTIPLE. Therefore,
       * assume that the application prefers the 'latency optimization' over
       * the 'throughput optimization' and set the async progress configuration
       * to disable context post.
       */
      MPIDI_Process.perobj.context_post.requested = 0;
#ifdef BGQ_SUPPORTS_TRIGGER_ASYNC_PROGRESS
      MPIDI_Process.async_progress.mode    = ASYNC_PROGRESS_MODE_TRIGGER;
      MPIDI_Process.avail_contexts         = MPIDI_MAX_CONTEXTS;
#else
      /* BGQ does not support the 'trigger' style of async progress. Until
       * this is implemented, set the async progress configuration to the
       * 'single threaded' defaults.
       */
      MPIDI_Process.async_progress.mode    = ASYNC_PROGRESS_MODE_DISABLED;
      MPIDI_Process.avail_contexts         = 1;
#endif
    }
#else
    /* The only valid async progress mode when using the 'global' mpich lock
     * mode is the 'trigger' async progress mode. Also, the 'global' mpich lock
     * mode only supports a single context.
     *
     * See discussions in mpich/src/mpid/pamid/src/mpid_init.c and
     * src/mpid/pamid/src/mpid_progress.h for more information.
     */
    if (value == 1)      /* force on  */
    {
      MPIDI_Process.async_progress.mode    = ASYNC_PROGRESS_MODE_TRIGGER;
      MPIDI_Process.avail_contexts         = 1;
    }
    else if (value == 0) /* force off */
    {
      MPIDI_Process.async_progress.mode    = ASYNC_PROGRESS_MODE_DISABLED;
      MPIDI_Process.avail_contexts         = 1;
    }
#endif
  }

  /* Set the upper-limit of number of PAMI Contexts. */
  {
    unsigned value = (unsigned)-1;
    char *names[] = {"PAMID_CONTEXT_MAX", "PAMI_MAXCONTEXT", "PAMI_MAXCONTEXTS", "PAMI_MAX_CONTEXT", "PAMI_MAX_CONTEXTS", NULL};
    ENV_Unsigned(names, &value, 1, &found_deprecated_env_var, rank);

    if (value != -1)
    {
#if (MPIU_THREAD_GRANULARITY != MPIU_THREAD_GRANULARITY_PER_OBJECT)
      /* The 'global' mpich lock mode only supports a single context.
       * See discussion in mpich/src/mpid/pamid/src/mpid_init.c for more
       * information.
       */
      if (value > 1)
      {
        found_deprecated_env_var++;
        if (MPIDI_Process.verbose >= MPIDI_VERBOSE_SUMMARY_0 && rank == 0)
          fprintf(stderr, "The environment variable \"PAMID_CONTEXT_MAX\" is invalid as this mpich library was configured.\n");
      }
#endif
      if (value == 0)
      {
        if (MPIDI_Process.verbose >= MPIDI_VERBOSE_SUMMARY_0 && rank == 0)
          fprintf(stderr, "The environment variable \"PAMID_CONTEXT_MAX\" must specify a value > 0. The number of contexts will be set to 1\n");

        value=1;
      }

      MPIDI_Process.avail_contexts=value;
    }

    TRACE_ERR("MPIDI_Process.avail_contexts=%u\n", MPIDI_Process.avail_contexts);
  }

  /* Enable context post. */
  {
    unsigned value = (unsigned)-1;
    char *names[] = {"PAMID_CONTEXT_POST", "PAMI_CONTEXT_POST", NULL};
    ENV_Unsigned(names, &value, 1, &found_deprecated_env_var, rank);
    if (value != -1)
    {
#if (MPIU_THREAD_GRANULARITY == MPIU_THREAD_GRANULARITY_PER_OBJECT)
      MPIDI_Process.perobj.context_post.requested = (value > 0);
#else
      found_deprecated_env_var++;
      if (MPIDI_Process.verbose >= MPIDI_VERBOSE_SUMMARY_0 && rank == 0)
        fprintf(stderr, "The environment variable \"PAMID_CONTEXT_POST\" is invalid as this mpich library was configured.\n");
#endif
    }
    TRACE_ERR("MPIDI_Process.perobj.context_post.requested=%u\n", MPIDI_Process.perobj.context_post.requested);
  }

  /* Enable/Disable asynchronous progress. */
  {
    /* THIS ENVIRONMENT VARIABLE NEEDS TO BE DOCUMENTED ABOVE */
    unsigned value = (unsigned)-1;
    char *names[] = {"PAMID_ASYNC_PROGRESS", "PAMID_COMMTHREADS", "PAMI_COMMTHREAD", "PAMI_COMMTHREADS", "PAMI_COMM_THREAD", "PAMI_COMM_THREADS", NULL};
    ENV_Unsigned(names, &value, 1, &found_deprecated_env_var, rank);

    if (value != (unsigned)-1)
    {
      if (value != ASYNC_PROGRESS_MODE_DISABLED)
      {
#if (MPIU_THREAD_GRANULARITY == MPIU_THREAD_GRANULARITY_PER_OBJECT)
        if (value == ASYNC_PROGRESS_MODE_LOCKED &&
            MPIDI_Process.perobj.context_post.requested == 0)
        {
          /* The 'locked' async progress mode requires context post.
           *
           * See discussion in src/mpid/pamid/src/mpid_progress.h for more
           * information.
           */
          found_deprecated_env_var++;
          if (MPIDI_Process.verbose >= MPIDI_VERBOSE_SUMMARY_0 && rank == 0)
            fprintf(stderr, "The environment variable \"PAMID_ASYNC_PROGRESS=1\" requires \"PAMID_CONTEXT_POST=1\".\n");
        }

#else /* (MPIU_THREAD_GRANULARITY != MPIU_THREAD_GRANULARITY_PER_OBJECT) */
        if (value == ASYNC_PROGRESS_MODE_LOCKED)
        {
          /* The only valid async progress mode when using the 'global' mpich
           * lock mode is the 'trigger' async progress mode.
           *
           * See discussion in src/mpid/pamid/src/mpid_progress.h for more
           * information.
           */
          found_deprecated_env_var++;
          if (MPIDI_Process.verbose >= MPIDI_VERBOSE_SUMMARY_0 && rank == 0)
            fprintf(stderr, "The environment variable \"PAMID_ASYNC_PROGRESS=1\" is invalid as this mpich library was configured.\n");

        }
#endif
      }

      MPIDI_Process.async_progress.mode = value;
    }
    TRACE_ERR("MPIDI_Process.async_progress.mode=%u\n", MPIDI_Process.async_progress.mode);
  }

  /* Determine short limit */
  {
    /* THIS ENVIRONMENT VARIABLE NEEDS TO BE DOCUMENTED ABOVE */
    char* names[] = {"PAMID_SHORT", "MP_S_SHORT_LIMIT", "PAMI_SHORT", NULL};
    ENV_Unsigned(names, &MPIDI_Process.short_limit, 2, &found_deprecated_env_var, rank);
  }

  /* Determine eager limit */
  {
    char* names[] = {"PAMID_EAGER", "PAMID_RZV", "MP_EAGER_LIMIT", "PAMI_RVZ", "PAMI_RZV", "PAMI_EAGER", NULL};
    ENV_Unsigned(names, &MPIDI_Process.eager_limit, 3, &found_deprecated_env_var, rank);
  }

  /* Determine 'local' eager limit */
  {
    char* names[] = {"PAMID_RZV_LOCAL", "PAMID_EAGER_LOCAL", "MP_EAGER_LIMIT_LOCAL", "PAMI_RVZ_LOCAL", "PAMI_RZV_LOCAL", "PAMI_EAGER_LOCAL", NULL};
    ENV_Unsigned(names, &MPIDI_Process.eager_limit_local, 3, &found_deprecated_env_var, rank);
  }

  /* Set the maximum number of outstanding RDMA requests */
  {
    char* names[] = {"PAMID_RMA_PENDING", "MP_RMA_PENDING", "PAMI_RMA_PENDING", NULL};
    ENV_Unsigned(names, &MPIDI_Process.rma_pending, 2, &found_deprecated_env_var, rank);
  }

  /* Set the status of the optimized collectives */
  {
    char* names[] = {"PAMID_COLLECTIVES", "PAMI_COLLECTIVE", "PAMI_COLLECTIVES", NULL};
    ENV_Unsigned(names, &MPIDI_Process.optimized.collectives, 1, &found_deprecated_env_var, rank);
    TRACE_ERR("MPIDI_Process.optimized.collectives=%u\n", MPIDI_Process.optimized.collectives);
  }

   /* First, if MP_COLLECTIVE_OFFLOAD is "on", then we want PE (FCA) collectives */
   {
      unsigned temp;
      temp = 0;
      char* names[] = {"MP_COLLECTIVE_OFFLOAD", NULL};
      ENV_Char(names, &temp);
      if(temp)
         MPIDI_Process.optimized.collectives = MPID_COLL_FCA;
   }
   
   /* However, MP_MP_PAMI_FOR can be set to "none" in which case we don't want PE (FCA) collectives */
   {
      char *env = getenv("MP_MPI_PAMI_FOR");
      if(env != NULL)
      {
         if(strncasecmp(env, "N", 1) == 1)
            MPIDI_Process.optimized.collectives = 0;
      }
   }

   /* Set the status for optimized selection of collectives */
   {
      char* names[] = {"PAMID_COLLECTIVES_SELECTION", NULL};
      ENV_Unsigned(names, &MPIDI_Process.optimized.select_colls, 1, &found_deprecated_env_var, rank);
      TRACE_ERR("MPIDI_Process.optimized.select_colls=%u\n", MPIDI_Process.optimized.select_colls);
   }


  /* Set the status of the optimized shared memory point-to-point functions */
  {
    char* names[] = {"PAMID_SHMEM_PT2PT", "MP_SHMEM_PT2PT", "PAMI_SHMEM_PT2PT", NULL};
    ENV_Unsigned(names, &MPIDI_Process.shmem_pt2pt, 2, &found_deprecated_env_var, rank);
  }

  /* Check for deprecated collectives environment variables. These variables are
   * used in src/mpid/pamid/src/comm/mpid_selectcolls.c */
  {
    unsigned tmp;
    {
      char* names[] = {"PAMID_COLLECTIVE_ALLGATHER", "PAMI_ALLGATHER", NULL};
      ENV_Unsigned(names, &tmp, 1, &found_deprecated_env_var, rank);
    }
    {
      char* names[] = {"PAMID_COLLECTIVE_ALLGATHER_PREALLREDUCE", "PAMI_ALLGATHER_PREALLREDUCE", NULL};
      ENV_Unsigned(names, &tmp, 1, &found_deprecated_env_var, rank);
    }
    {
      char* names[] = {"PAMID_COLLECTIVE_ALLGATHERV", "PAMI_ALLGATHERV", NULL};
      ENV_Unsigned(names, &tmp, 1, &found_deprecated_env_var, rank);
    }
    {
      char* names[] = {"PAMID_COLLECTIVE_ALLGATHERV_PREALLREDUCE", "PAMI_ALLGATHERV_PREALLREDUCE", NULL};
      ENV_Unsigned(names, &tmp, 1, &found_deprecated_env_var, rank);
    }
    {
      char* names[] = {"PAMID_COLLECTIVE_ALLREDUCE", "PAMI_ALLREDUCE", NULL};
      ENV_Unsigned(names, &tmp, 1, &found_deprecated_env_var, rank);
    }
    {
      char* names[] = {"PAMID_COLLECTIVE_ALLREDUCE_PREALLREDUCE", "PAMI_ALLREDUCE_PREALLREDUCE", NULL};
      ENV_Unsigned(names, &tmp, 1, &found_deprecated_env_var, rank);
    }
    {
      char* names[] = {"PAMID_COLLECTIVE_ALLTOALL", "PAMI_ALLTOALL", NULL};
      ENV_Unsigned(names, &tmp, 1, &found_deprecated_env_var, rank);
    }
    {
      char* names[] = {"PAMID_COLLECTIVE_ALLTOALLV", "PAMI_ALLTOALLV", NULL};
      ENV_Unsigned(names, &tmp, 1, &found_deprecated_env_var, rank);
    }
    {
      char* names[] = {"PAMID_COLLECTIVE_ALLTOALLV_INT", "PAMI_ALLTOALLV_INT", NULL};
      ENV_Unsigned(names, &tmp, 1, &found_deprecated_env_var, rank);
    }
    {
      char* names[] = {"PAMID_COLLECTIVE_BARRIER", "PAMI_BARRIER", NULL};
      ENV_Unsigned(names, &tmp, 1, &found_deprecated_env_var, rank);
    }
    {
      char* names[] = {"PAMID_COLLECTIVE_BCAST", "PAMI_BCAST", NULL};
      ENV_Unsigned(names, &tmp, 1, &found_deprecated_env_var, rank);
    }
    {
      char* names[] = {"PAMID_COLLECTIVE_BCAST_PREALLREDUCE", "PAMI_BCAST_PREALLREDUCE", NULL};
      ENV_Unsigned(names, &tmp, 1, &found_deprecated_env_var, rank);
    }
    {
      char* names[] = {"PAMID_COLLECTIVE_GATHER", "PAMI_GATHER", NULL};
      ENV_Unsigned(names, &tmp, 1, &found_deprecated_env_var, rank);
    }
    {
      char* names[] = {"PAMID_COLLECTIVE_GATHERV", "PAMI_GATHERV", NULL};
      ENV_Unsigned(names, &tmp, 1, &found_deprecated_env_var, rank);
    }
    {
      char* names[] = {"PAMID_COLLECTIVE_REDUCE", "PAMI_REDUCE", NULL};
      ENV_Unsigned(names, &tmp, 1, &found_deprecated_env_var, rank);
    }
    {
      char* names[] = {"PAMID_COLLECTIVE_SCAN", "PAMI_SCAN", NULL};
      ENV_Unsigned(names, &tmp, 1, &found_deprecated_env_var, rank);
    }
    {
      char* names[] = {"PAMID_COLLECTIVE_SCATTER", "PAMI_SCATTER", NULL};
      ENV_Unsigned(names, &tmp, 1, &found_deprecated_env_var, rank);
    }
    {
      char* names[] = {"PAMID_COLLECTIVE_SCATTERV", "PAMI_SCATTERV", NULL};
      ENV_Unsigned(names, &tmp, 1, &found_deprecated_env_var, rank);
    }
    {
      char* names[] = {"PAMID_COLLECTIVE_SCATTERV_PREALLREDUCE", "PAMI_SCATTERV_PREALLREDUCE", NULL};
      ENV_Unsigned(names, &tmp, 1, &found_deprecated_env_var, rank);
    }
    {
      char* names[] = {"PAMID_CORE_ON_ABORT", "PAMI_COREONABORT", "PAMI_COREONMPIABORT", "PAMI_COREONMPIDABORT", NULL};
      ENV_Unsigned(names, &tmp, 1, &found_deprecated_env_var, rank);
    }
#if (MPIDI_STATISTICS || MPIDI_PRINTENV)
    mpich_env=(MPIDI_printenv_t *) MPIU_Malloc(sizeof(MPIDI_printenv_t)+1);
    mpid_statp=(MPIX_stats_t *) MPIU_Malloc(sizeof(MPIX_stats_t)+1);
    memset((void *)mpich_env,0,sizeof(MPIDI_printenv_t));
    memset((void *)mpid_statp,0,sizeof(MPIX_stats_t));
    /* If MP_STATISTICS is set, each task prints statistics data at the end of an MPI jobs */
    {
      char* names[] = {"MP_STATISTICS", NULL};
      ENV_Char(names, &MPIDI_Process.mp_statistics);
      if (MPIDI_Process.mp_statistics) {
        prtStat=1;
      }
    }
    /* if MP_INFOLEVEL is greater than or equal to 1, the library will display the banner */
    {
      char* names[] = {"MP_INFOLEVEL", NULL};
      ENV_Unsigned(names, &MPIDI_Process.mp_infolevel, 1, &found_deprecated_env_var, rank);
    }
    /* If MP_PRINTENV is set, task 0 prints out the env. info after MPI_Init   */
    {
      char* names[] = {"MP_PRINTENV", NULL};
      ENV_Char(names, &MPIDI_Process.mp_printenv);
      if (MPIDI_Process.mp_printenv) {
        prtEnv=1;
      }
    }
    /*  MP_CSS_INTERRUPT                                                       */
    {
      char* names[] = {"MP_CSS_INTERRUPT", NULL};
      ENV_Char(names, &mpich_env->interrupts);
      if (mpich_env->interrupts == 1)      /* force on  */
      {
        MPIDI_Process.perobj.context_post.requested = 0;
        MPIDI_Process.async_progress.mode    = ASYNC_PROGRESS_MODE_TRIGGER;
#if (MPIU_THREAD_GRANULARITY == MPIU_THREAD_GRANULARITY_PER_OBJECT)
        MPIDI_Process.avail_contexts         = MPIDI_MAX_CONTEXTS;
#else
        MPIDI_Process.avail_contexts         = 1;
#endif
      }
      else if (mpich_env->interrupts == 0) /* force off */
      {
        MPIDI_Process.perobj.context_post.requested = 0;
        MPIDI_Process.async_progress.mode    = ASYNC_PROGRESS_MODE_DISABLED;
        MPIDI_Process.avail_contexts         = 1;
      }
    }
    /* MP_POLLING_INTERVAL                                                     */
    {
      char* names[] = {"MP_POLLING_INTERVAL", NULL};
      ENV_Unsigned(names,(unsigned int *) &mpich_env->polling_interval,1, &found_deprecated_env_var, rank);
    }

    /* MP_RETRANSMIT_INTERVAL                                                  */
    {
      char* names[] = {"MP_RETRANSMIT_INTERVAL", NULL};
      ENV_Unsigned(names, &mpich_env->retransmit_interval,1, &found_deprecated_env_var, rank);
    }

    /* MP_S_USE_PAMI_GET                                                       */
    {
      char* names[] = {"MP_S_USE_PAMI_GET", NULL};
      ENV_Char(names, &MPIDI_Process.mp_s_use_pami_get);
    }
#endif
  }
  /* Exit if any deprecated environment variables were specified. */
  if (found_deprecated_env_var)
    {
      if (rank == 0)
      {
        // Only rank 0 prints and exits.  sleep to make sure message is sent before exiting.
        // Other ranks will proceed, but will be waiting in a barrier in context create
        // when the exit occurs.
        fprintf (stderr, "\n"); fflush(stderr); sleep(1);
        exit(1);
      }
    }
}

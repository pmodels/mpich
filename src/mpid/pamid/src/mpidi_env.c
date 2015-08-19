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

#include "mpidi_util.h"

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
 *           MPIR_CVAR_DEFAULT_THREAD_LEVEL=multiple
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
 * - PAMID_COLLECTIVES_MEMORY_OPTIMIZED - Controls whether collectives are 
 *   optimized to reduce memory usage. This may disable some PAMI collectives.
 *   Possible values:
 *   - 0 - Collectives are not memory optimized.
 *   - n - Collectives are memory optimized. Levels are bitwise values :
 *        MPID_OPT_LVL_IRREG     = 1,   Do not optimize irregular communicators 
 *        MPID_OPT_LVL_NONCONTIG = 2,   Disable some non-contig collectives 
 *
 *   PAMID_OPTIMIZED_SUBCOMMS - Use PAMI 'optimized' collectives. Defaullt is 1.
 *   - 0 - Some optimized protocols may be disabled.
 *   - 1 - All performance optimized protocols will be enabled when available
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
 *   collectives to have before a barrier is called.  This is used when
 *   the PAMI collective metadata indicates 'asyncflowctl' may be needed 
 *   to avoid 'flooding' other participants with unexpected data. 
 *   Higher numbers can help on larger partitions and larger message sizes. 
 * 
 *   After every {PAMID_NUMREQUESTS} async collectives, the "glue" will call
 *   a barrier. 
 *   - Default is 1 (guaranteed functionality) 
 *   - N>1may used to tune performance
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

MPIX_stats_t *mpid_statp=NULL;
extern MPIDI_printenv_t  *mpich_env;
#endif

#define ENV_Deprecated(a, b, c, d, e) ENV_Deprecated__(a, b, c, d, e)

#ifdef TOKEN_FLOW_CONTORL
 extern int MPIDI_get_buf_mem(unsigned long *);
 extern int MPIDI_atoi(char* , int* );
#endif
 extern int application_set_eager_limit;

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
  if (deprecated != NULL)
    ENV_Deprecated(name, num_supported, deprecated, rank, NA);

  char * env;
  int  rc;

  unsigned i=0;
  for (;; ++i) {
    if (name[i] == NULL)
      return;
    env = getenv(name[i]);
    if (env != NULL)
      break;
  }

  unsigned oldval = *val;
  rc=MPIDI_atoi(env,val);
  if(rc != 0)
    {
      /* Something went wrong with the processing this integer
       * Print a warning, and restore the original value */
      *val = oldval;
      fprintf(stderr, "Warning:  Environment variable: %s should be an integer value:  defaulting to %d", string, *val);
      return;
    }

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

  if (  (env[0]=='y')|| (env[0]=='Y')
     || (env[0]=='p')|| (env[0]=='P') 
     || (env[0]=='t')|| (env[0]=='T')
     || (env[0]=='r')|| (env[0]=='R')
     || (env[0]=='F')|| (env[0]=='f'))
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

  /* Set async flow control - number of collectives between barriers */
  {
    char* names[] = {"PAMID_NUMREQUESTS", NULL};
    ENV_Unsigned(names, &MPIDI_Process.optimized.num_requests, 1, &found_deprecated_env_var, rank);
    TRACE_ERR("MPIDI_Process.optimized.num_requests=%u\n", MPIDI_Process.optimized.num_requests);
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
#if (MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY_PER_OBJECT)
    /* Any mpich work function posted to a context that eventually initiates
     * other communcation transfers will hang on a lock attempt if the
     * 'context post' feature is not enabled. Until this code flow is fixed
     * the context post must not be disabled.
     *
     * See discussion in src/mpid/pamid/include/mpidi_macros.h
     * -> MPIDI_Context_post()
     */
    MPIDI_Process.perobj.context_post.requested = 1;
    if (value == 1)      /* force on  */
    {
      MPIDI_Process.async_progress.mode    = ASYNC_PROGRESS_MODE_DEFAULT;
      MPIDI_Process.avail_contexts         = MPIDI_MAX_CONTEXTS;
    }
    else if (value == 0) /* force off */
    {
      MPIDI_Process.async_progress.mode    = ASYNC_PROGRESS_MODE_DISABLED;
      MPIDI_Process.avail_contexts         = 1;
    }
    else if (requested != MPI_THREAD_MULTIPLE)
    {
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
#if (MPICH_THREAD_GRANULARITY != MPICH_THREAD_GRANULARITY_PER_OBJECT)
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
#if (MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY_PER_OBJECT)
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
#if (MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY_PER_OBJECT)
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

#else /* (MPICH_THREAD_GRANULARITY != MPICH_THREAD_GRANULARITY_PER_OBJECT) */
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

  /*
   * Determine 'short' limit
   * - sets both the 'local' and 'remote' short limit, and
   * - sets both the 'application' and 'internal' short limit
   *
   * Identical to setting the PAMID_PT2PT_LIMITS environment variable as:
   *
   *   PAMID_PT2PT_LIMITS="::x:x:::x:x"
   */
  {
    /* THIS ENVIRONMENT VARIABLE NEEDS TO BE DOCUMENTED ABOVE */
    char* names[] = {"PAMID_SHORT", "MP_S_SHORT_LIMIT", "PAMI_SHORT", NULL};
    ENV_Unsigned(names, &MPIDI_Process.pt2pt.limits.application.immediate.remote, 2, &found_deprecated_env_var, rank);
    ENV_Unsigned(names, &MPIDI_Process.pt2pt.limits.application.immediate.local, 2, NULL, rank);
    ENV_Unsigned(names, &MPIDI_Process.pt2pt.limits.internal.immediate.remote, 2, NULL, rank);
    ENV_Unsigned(names, &MPIDI_Process.pt2pt.limits.internal.immediate.local, 2, NULL, rank);
  }

  /*
   * Determine 'remote' eager limit
   * - sets both the 'application' and 'internal' remote eager limit
   *
   * Identical to setting the PAMID_PT2PT_LIMITS environment variable as:
   *
   *   PAMID_PT2PT_LIMITS="x::::x:::"
   *   -- or --
   *   PAMID_PT2PT_LIMITS="x::::x"
   */
  {
    char* names[] = {"PAMID_EAGER", "PAMID_RZV", "MP_EAGER_LIMIT", "PAMI_RVZ", "PAMI_RZV", "PAMI_EAGER", NULL};
    ENV_Unsigned(names, &MPIDI_Process.pt2pt.limits.application.eager.remote, 3, &found_deprecated_env_var, rank);
    ENV_Unsigned(names, &MPIDI_Process.pt2pt.limits.internal.eager.remote, 3, NULL, rank);
  }
#if TOKEN_FLOW_CONTROL
  /* Determine if users set eager limit  */
  {
    MPIDI_set_eager_limit(&MPIDI_Process.pt2pt.limits.application.eager.remote);
  }
  /* Determine buffer memory for early arrivals */
  {
    int rc;
    rc=MPIDI_get_buf_mem(&MPIDI_Process.mp_buf_mem,&MPIDI_Process.mp_buf_mem_max);
    MPID_assert_always(rc == MPI_SUCCESS);
  }
#endif

  /*
   * Determine 'local' eager limit
   * - sets both the 'application' and 'internal' local eager limit
   *
   * Identical to setting the PAMID_PT2PT_LIMITS environment variable as:
   *
   *   PAMID_PT2PT_LIMITS=":x::::x::"
   *   -- or --
   *   PAMID_PT2PT_LIMITS=":x::::x"
   */
  {
    char* names[] = {"PAMID_RZV_LOCAL", "PAMID_EAGER_LOCAL", "MP_EAGER_LIMIT_LOCAL", "PAMI_RVZ_LOCAL", "PAMI_RZV_LOCAL", "PAMI_EAGER_LOCAL", NULL};
    ENV_Unsigned(names, &MPIDI_Process.pt2pt.limits.application.eager.local, 3, &found_deprecated_env_var, rank);
    ENV_Unsigned(names, &MPIDI_Process.pt2pt.limits.internal.eager.local, 3, NULL, rank);
  }

  /*
   * Determine *all* point-to-point limit overrides.
   *
   * The entire point-to-point limit set is determined by three boolean
   * configuration values:
   * - 'is non-local limit'   vs 'is local limit'
   * - 'is eager limit'       vs 'is immediate limit'
   * - 'is application limit' vs 'is internal limit'
   *
   * The point-to-point configuration limit values are specified in order and
   * are delimited by ':' characters. If a value is not specified for a given
   * configuration then the limit is not changed. All eight configuration
   * values are not required to be specified, although in order to set the
   * last (eighth) configuration value the previous seven configurations must
   * be listed. For example:
   *
   *    PAMID_PT2PT_LIMITS=":::::::10240"
   *
   * The configuration entries can be described as:
   *    0 - remote eager     application limit
   *    1 - local  eager     application limit
   *    2 - remote immediate application limit
   *    3 - local  immediate application limit
   *    4 - remote eager     internal    limit
   *    5 - local  eager     internal    limit
   *    6 - remote immediate internal    limit
   *    7 - local  immediate internal    limit
   *
   * Examples:
   *
   *    "10240"
   *      - sets the application internode eager (the "normal" eager limit)
   *
   *    "10240::64"
   *      - sets the application internode eager and immediate limits
   *
   *    "::::0:0:0:0"
   *      - disables 'eager' and 'immediate' for all internal point-to-point
   */
  {
    char * env = getenv("PAMID_PT2PT_LIMITS");
    if (env != NULL)
      {
        size_t i, n = strlen(env);
        char * tmp = (char *) MPIU_Malloc(n+1);
        strncpy(tmp,env,n);
        if (n>0) tmp[n]=0;

        char * tail  = tmp;
        char * token = tail;
        for (i = 0; token == tail; i++)
          {
            while (*tail != 0 && *tail != ':') tail++;
            if (*tail == ':')
              {
                *tail = 0;
                if (token != tail)
                  MPIDI_atoi(token, &MPIDI_Process.pt2pt.limits_array[i]);
                tail++;
                token = tail;
              }
            else
              {
                if (token != tail)
                  MPIDI_atoi(token, &MPIDI_Process.pt2pt.limits_array[i]);
              }
          }

        MPIU_Free (tmp);
      }
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
            MPIDI_Process.optimized.collectives = MPID_COLL_OFF;
      }
   }

   /* Set the status for optimized selection of collectives */
   {
      char* names[] = {"PAMID_COLLECTIVES_SELECTION", NULL};
      ENV_Unsigned(names, &MPIDI_Process.optimized.select_colls, 1, &found_deprecated_env_var, rank);
      TRACE_ERR("MPIDI_Process.optimized.select_colls=%u\n", MPIDI_Process.optimized.select_colls);
   }

   /* Finally, if MP_COLLECTIVE_SELECTION is "on", then we want to overwrite any other setting */
   {
      char *env = getenv("MP_COLLECTIVE_SELECTION");
      if(env != NULL)
      {
         pami_extension_t extension;
         pami_result_t status = PAMI_ERROR;
         status = PAMI_Extension_open (MPIDI_Client, "EXT_collsel", &extension);
         if(status == PAMI_SUCCESS)
         {
           char *env = getenv("MP_COLLECTIVE_SELECTION");
           if(env != NULL)
           {
             if(strncasecmp(env, "TUN", 3) == 0)
             {
               MPIDI_Process.optimized.auto_select_colls = MPID_AUTO_SELECT_COLLS_TUNE;
               if(MPIDI_Process.optimized.collectives   != MPID_COLL_FCA)
                 MPIDI_Process.optimized.collectives     = MPID_COLL_ON;
             }
             else if(strncasecmp(env, "YES", 3) == 0)
             {
               MPIDI_Process.optimized.auto_select_colls = MPID_AUTO_SELECT_COLLS_ALL; /* All collectives will be using auto coll sel.
                                                                                    We will check later on each individual coll. */
               if(MPIDI_Process.optimized.collectives   != MPID_COLL_FCA)
                 MPIDI_Process.optimized.collectives     = MPID_COLL_ON;
             }
             else
               MPIDI_Process.optimized.auto_select_colls = MPID_AUTO_SELECT_COLLS_NONE;

           }
         }
         else
           MPIDI_Process.optimized.auto_select_colls = MPID_AUTO_SELECT_COLLS_NONE;/* Auto coll sel is disabled for all */
      }
      else
         MPIDI_Process.optimized.auto_select_colls = MPID_AUTO_SELECT_COLLS_NONE;/* Auto coll sel is disabled for all */ 
   }
   
   /* Set the status for memory optimized collectives */
   {
      char* names[] = {"PAMID_COLLECTIVES_MEMORY_OPTIMIZED", NULL};
      ENV_Unsigned(names, &MPIDI_Process.optimized.memory, 1, &found_deprecated_env_var, rank);
      TRACE_ERR("MPIDI_Process.optimized.memory=%u\n", MPIDI_Process.optimized.memory);
   }

  /* Set the status of the optimized shared memory point-to-point functions */
  {
    char* names[] = {"PAMID_SHMEM_PT2PT", "PAMI_SHMEM_PT2PT", NULL};
    ENV_Unsigned(names, &MPIDI_Process.shmem_pt2pt, 2, &found_deprecated_env_var, rank);
  }

  /* MP_SHMEM_PT2PT = yes or no       */
  {
    char* names[] = {"MP_SHMEM_PT2PT", NULL};
      ENV_Char(names, &MPIDI_Process.shmem_pt2pt);
  }

  /* Enable MPIR_* implementations of non-blocking collectives */
  {
    char* names[] = {"PAMID_MPIR_NBC", NULL};
    ENV_Unsigned(names, &MPIDI_Process.mpir_nbc, 1, &found_deprecated_env_var, rank);
  }

  /* Enable typed PAMI calls for derived types within MPID_Put and MPID_Get. */
  {
    char* names[] = {"PAMID_TYPED_ONESIDED", NULL};
    ENV_Unsigned(names, &MPIDI_Process.typed_onesided, 1, &found_deprecated_env_var, rank);
  }
  /* Check for deprecated collectives environment variables. These variables are
   * used in src/mpid/pamid/src/comm/mpid_selectcolls.c */
  {
    {
      char* names[] = {"PAMID_COLLECTIVE_ALLGATHER", "PAMI_ALLGATHER", NULL};
      ENV_Deprecated(names, 1, &found_deprecated_env_var, rank, 0);
    }
    {
      char* names[] = {"PAMID_COLLECTIVE_ALLGATHER_PREALLREDUCE", "PAMI_ALLGATHER_PREALLREDUCE", NULL};
      ENV_Deprecated(names, 1, &found_deprecated_env_var, rank, 0);
    }
    {
      char* names[] = {"PAMID_COLLECTIVE_ALLGATHERV", "PAMI_ALLGATHERV", NULL};
      ENV_Deprecated(names, 1, &found_deprecated_env_var, rank, 0);
    }
    {
      char* names[] = {"PAMID_COLLECTIVE_ALLGATHERV_PREALLREDUCE", "PAMI_ALLGATHERV_PREALLREDUCE", NULL};
      ENV_Deprecated(names, 1, &found_deprecated_env_var, rank, 0);
    }
    {
      char* names[] = {"PAMID_COLLECTIVE_ALLREDUCE", "PAMI_ALLREDUCE", NULL};
      ENV_Deprecated(names, 1, &found_deprecated_env_var, rank, 0);
    }
    {
      char* names[] = {"PAMID_COLLECTIVE_ALLREDUCE_PREALLREDUCE", "PAMI_ALLREDUCE_PREALLREDUCE", NULL};
      ENV_Deprecated(names, 1, &found_deprecated_env_var, rank, 0);
    }
    {
      char* names[] = {"PAMID_COLLECTIVE_ALLTOALL", "PAMI_ALLTOALL", NULL};
      ENV_Deprecated(names, 1, &found_deprecated_env_var, rank, 0);
    }
    {
      char* names[] = {"PAMID_COLLECTIVE_ALLTOALLV", "PAMI_ALLTOALLV", NULL};
      ENV_Deprecated(names, 1, &found_deprecated_env_var, rank, 0);
    }
    {
      char* names[] = {"PAMID_COLLECTIVE_ALLTOALLV_INT", "PAMI_ALLTOALLV_INT", NULL};
      ENV_Deprecated(names, 1, &found_deprecated_env_var, rank, 0);
    }
    {
      char* names[] = {"PAMID_COLLECTIVE_BARRIER", "PAMI_BARRIER", NULL};
      ENV_Deprecated(names, 1, &found_deprecated_env_var, rank, 0);
    }
    {
      char* names[] = {"PAMID_COLLECTIVE_BCAST", "PAMI_BCAST", NULL};
      ENV_Deprecated(names, 1, &found_deprecated_env_var, rank, 0);
    }
    {
      char* names[] = {"PAMID_COLLECTIVE_BCAST_PREALLREDUCE", "PAMI_BCAST_PREALLREDUCE", NULL};
      ENV_Deprecated(names, 1, &found_deprecated_env_var, rank, 0);
    }
    {
      char* names[] = {"PAMID_COLLECTIVE_GATHER", "PAMI_GATHER", NULL};
      ENV_Deprecated(names, 1, &found_deprecated_env_var, rank, 0);
    }
    {
      char* names[] = {"PAMID_COLLECTIVE_GATHERV", "PAMI_GATHERV", NULL};
      ENV_Deprecated(names, 1, &found_deprecated_env_var, rank, 0);
    }
    {
      char* names[] = {"PAMID_COLLECTIVE_REDUCE", "PAMI_REDUCE", NULL};
      ENV_Deprecated(names, 1, &found_deprecated_env_var, rank, 0);
    }
    {
      char* names[] = {"PAMID_COLLECTIVE_SCAN", "PAMI_SCAN", NULL};
      ENV_Deprecated(names, 1, &found_deprecated_env_var, rank, 0);
    }
    {
      char* names[] = {"PAMID_COLLECTIVE_SCATTER", "PAMI_SCATTER", NULL};
      ENV_Deprecated(names, 1, &found_deprecated_env_var, rank, 0);
    }
    {
      char* names[] = {"PAMID_COLLECTIVE_SCATTERV", "PAMI_SCATTERV", NULL};
      ENV_Deprecated(names, 1, &found_deprecated_env_var, rank, 0);
    }
    {
      char* names[] = {"PAMID_COLLECTIVE_SCATTERV_PREALLREDUCE", "PAMI_SCATTERV_PREALLREDUCE", NULL};
      ENV_Deprecated(names, 1, &found_deprecated_env_var, rank, 0);
    }
    {
      char* names[] = {"PAMID_CORE_ON_ABORT", "PAMI_COREONABORT", "PAMI_COREONMPIABORT", "PAMI_COREONMPIDABORT", NULL};
      ENV_Deprecated(names, 1, &found_deprecated_env_var, rank, 0);
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
      char *cp=NULL, *cp1=NULL;
      int user_interrupts=0;
      char* names[] = {"MP_CSS_INTERRUPT", NULL};
      ENV_Char(names, &mpich_env->interrupts);
      if (mpich_env->interrupts == 1)      /* force on  */
      {
        cp = getenv("MP_CSS_INTERRUPT");
        if (*cp=='Y' || *cp=='y')
        {
          user_interrupts = ASYNC_PROGRESS_ALL;
        }
        else
        {
          char delimiter='+';
          cp1 = strchr(cp,delimiter);
          if (!cp1)  /* timer or receive  */
          {
             if ( (*cp == 't') || (*cp == 'T') )
                user_interrupts = PAMIX_PROGRESS_TIMER;
             else if ( (*cp == 'r') || (*cp == 'R') )
                user_interrupts = PAMIX_PROGRESS_RECV_INTERRUPT;
          }
          else   /* timer + receive   */
          {
            if ((( *cp == 't' || *cp == 'T') && ( *(cp1+1) == 'r' || *(cp1+1) == 'R')) ||
                (( *cp == 'r' || *cp == 'R') && ( *(cp1+1) == 't' || *(cp1+1) == 'T')))
                 user_interrupts = ASYNC_PROGRESS_ALL;
            else
            {
                TRACE_ERR("ERROR in MP_CSS_INTERRUPT %s(%d)\n",__FILE__,__LINE__);
                exit(1);
            }
          }
        }
        MPIDI_Process.mp_interrupts=user_interrupts;
        MPIDI_Process.perobj.context_post.requested = 0;
        MPIDI_Process.async_progress.mode    = ASYNC_PROGRESS_MODE_TRIGGER;
#if (MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY_PER_OBJECT)
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
    /* MP_S_SMP_DETECT                                                         */
    {
      char* names[] = {"MP_S_SMP_DETECT", "PAMID_SMP_DIRECT", NULL};
      ENV_Char(names, &MPIDI_Process.smp_detect);
      if(!MPIDI_Process.smp_detect)
        PAMIX_Extensions.is_local_task.node_info=NULL;
    }

  }
    {
#if TOKEN_FLOW_CONTROL
      char* names[] = {"MP_USE_TOKEN_FLOW_CONTROL", NULL};
      ENV_Char(names, &MPIDI_Process.is_token_flow_control_on);
      if (!MPIDI_Process.is_token_flow_control_on)
           MPIDI_Process.mp_buf_mem=0;
#endif
    }

#ifdef QUEUE_BINARY_SEARCH_SUPPORT
    char* names[] = {"MP_S_USE_QUEUE_BINARY_SEARCH_SUPPORT", NULL};
    ENV_Char(names, &MPIDI_Process.queue_binary_search_support_on);
#endif

#if CUDA_AWARE_SUPPORT
    char* names[] = {"MP_CUDA_AWARE", NULL};
    ENV_Char(names, &MPIDI_Process.cuda_aware_support_on);
    if(MPIDI_Process.cuda_aware_support_on && MPIDI_enable_cuda() == false)
    {
      MPIDI_Process.cuda_aware_support_on = false;
      if(rank == 0)
      {
        fprintf(stderr, "Error loading libcudart\n");fflush(stderr);sleep(1);exit(1);
      }
    }
    else if(MPIDI_Process.cuda_aware_support_on)
    {
      if(MPIDI_Process.optimized.collectives == MPID_COLL_FCA)
        if(rank == 0)
        {
          fprintf(stderr, "Warning: FCA is not supported with CUDA Aware support\n");fflush(stderr);
        }

      MPIDI_Process.optimized.collectives = MPID_COLL_CUDA;
      MPIDI_Process.optimized.select_colls = 0;
    }
#endif

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

#if TOKEN_FLOW_CONTROL
int  MPIDI_set_eager_limit(unsigned int *eager_limit)
{
     char *cp;
     int  val;
     int  numTasks=MPIDI_Process.numTasks;
     cp = getenv("MP_EAGER_LIMIT");
     if (cp)
       {
         application_set_eager_limit=1;
         if ( MPIDI_atoi(cp, &val) == 0 )
           *eager_limit=val;
       }
     else
       {
        /*  set default
        *  Number of tasks      MP_EAGER_LIMIT
        * -----------------     --------------
        *      1  -    256         32768
        *    257  -    512         16384
        *    513  -   1024          8192
        *   1025  -   2048          4096
        *   2049  -   4096          2048
        *   4097  &  above          1024
        */
       if      (numTasks <  257) *eager_limit = 32768;
       else if (numTasks <  513) *eager_limit = 16384;
       else if (numTasks < 1025) *eager_limit =  8192;
       else if (numTasks < 2049) *eager_limit =  4096;
       else if (numTasks < 4097) *eager_limit =  2048;
       else                      *eager_limit =  1024;

       }
     return 0;
}

   /******************************************************************/
   /*                                                                */
   /* Check for MP_BUFFER_MEM, if the value is not set by the user,  */
   /* then set the value with the default of 64 MB.                  */
   /* MP_BUFFER_MEM supports the following format:                   */
   /* MP_BUFFER_MEM=xxM                                              */
   /* MP_BUFFER_MEM=xxM,yyyM                                         */
   /* MP_BUFFER_MEM=xxM,yyyG                                         */
   /* MP_BUFFER_MEM=,yyyM                                            */
   /* xx:  pre allocated size  the max. allowable value is 256 MB    */
   /*      the space is allocated during the initialization.         */
   /*      the default is 64 MB                                      */
   /* yyy: maximum size - the maximum size to which the early arrival*/
   /*      buffer can temporarily grow when the preallocated portion */
   /*      of the EA buffer has been filled.                         */
   /*                                                                */
   /******************************************************************/
int  MPIDI_get_buf_mem(unsigned long *buf_mem,unsigned long *buf_mem_max)
    {
     char *cp;
     int  i;
     int args_in_error=0;
     char pre_alloc_buf[25], buf_max[25];
     char *buf_max_cp;
     int pre_alloc_val=0;
     unsigned long buf_max_val;
     int  has_error = 0;
     extern int application_set_buf_mem;

     if (cp = getenv("MP_BUFFER_MEM")) {
         pre_alloc_buf[24] = '\0';
         buf_max[24] = '\0';
         application_set_buf_mem=1;
         if ( (buf_max_cp = strchr(cp, ',')) ) {
           if ( *(++buf_max_cp)  == '\0' ) {
              /* Error: missing buffer_mem_max */
              has_error = 1;
           }
           else if ( cp[0] == ',' ) {
              /* Pre_alloc value is default -- use default   */
              pre_alloc_val = -1;
              strncpy(buf_max, buf_max_cp, 24);
              if ( MPIDI_atoll(buf_max, &buf_max_val) != 0 )
                 has_error = 1;
           }
           else {
              /* both values are present */
              for (i=0; ; i++ ) {
                 if ( (cp[i] != ',') && (i<24) )
                    pre_alloc_buf[i] = cp[i];
                 else {
                    pre_alloc_buf[i] = '\0';
                    break;
                 }
              }
              strncpy(buf_max, buf_max_cp, 24);
              if ( MPIDI_atoi(pre_alloc_buf, &pre_alloc_val) == 0 ) {
                 if ( MPIDI_atoll(buf_max, &buf_max_val) != 0 )
                    has_error = 1;
              }
              else
                 has_error = 1;
           }
        }
        else
         {
            /* Old single value format  */
            if ( MPIDI_atoi(cp, &pre_alloc_val) == 0 )
               buf_max_val = (unsigned long)pre_alloc_val;
            else
               has_error = 1;
         }
         if ( has_error == 0) {
             if ((int) pre_alloc_val != -1)  /* MP_BUFFER_MEM=,128MB  */
                 *buf_mem     = (int) pre_alloc_val;
             if (buf_max_val > ONE_SHARED_SEGMENT)
                 *buf_mem = ONE_SHARED_SEGMENT;
             if (buf_max_val != *buf_mem_max)
                  *buf_mem_max = buf_max_val;
         } else {
            args_in_error += 1;
            TRACE_ERR("ERROR in MP_BUFFER_MEM %s(%d)\n",__FILE__,__LINE__);
            return 1;
         }
        return 0;
     } else {
         /* MP_BUFFER_MEM is not specified by the user*/
         *buf_mem     = BUFFER_MEM_DEFAULT;
         TRACE_ERR("buffer_mem=%d  buffer_mem_max=%d\n",*buf_mem,*buf_mem_max);
         return 0;
     }
}
#endif

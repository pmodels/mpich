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
 * \file src/mpix/mpix.c
 * \brief This file is for MPI extensions that MPI apps are likely to use.
 */

#include <mpidimpl.h>

#ifdef __BGQ__
#include <stdlib.h>
#include <spi/include/kernel/location.h>
#endif /* __BGQ__ */

MPIX_Hardware_t MPIDI_HW;

/* Determine the number of torus dimensions. Implemented to keep this code
 * architecture-neutral. do we already have this cached somewhere?
 * this call is non-destructive (no asserts), however future calls are
 * destructive so the user needs to verify numdimensions != -1
 */


static void
MPIX_Init_hw(MPIX_Hardware_t *hw)
{
  memset(hw, 0, sizeof(MPIX_Hardware_t));

  hw->clockMHz = PAMIX_Client_query(MPIDI_Client, PAMI_CLIENT_CLOCK_MHZ).value.intval;
  hw->memSize  = PAMIX_Client_query(MPIDI_Client, PAMI_CLIENT_MEM_SIZE).value.intval;

#ifdef __BGQ__
  int i=0;
  const pamix_torus_info_t *info = PAMIX_Torus_info();
  /* The extension returns a "T" dimension */
  hw->torus_dimension = info->dims-1;

  hw->psize = info->size[info->dims-1];
  for(i=0;i<info->dims-1;i++)
    {
      hw->Size[i]    = info->size[i];
      hw->Coords[i]  = info->coord[i];
      hw->isTorus[i] = info->torus[i];
      hw->psize      = hw->psize * info->size[i];
    }

  hw->prank = info->coord[0];
  for(i=1;i<info->dims;i++)
    {
      hw->prank = (hw->prank * info->size[i] + info->coord[i]);
    }

  /* The torus extension returns "T" as the last element */
  hw->coreID = info->coord[info->dims-1];
  hw->ppn    = info->size[info->dims-1];
#endif
}


void
MPIX_Init()
{
  MPIX_Init_hw(&MPIDI_HW);
}


#if !defined(__AIX__)
extern int backtrace(void **buffer, int size);                  /**< GlibC backtrace support */
extern char **backtrace_symbols(void *const *buffer, int size); /**< GlibC backtrace support */
void MPIX_Dump_stacks()
{
  const size_t SIZE=32;
  void *array[SIZE];
  size_t i;
  size_t size    = backtrace(array, SIZE);
  char **bt_strings = backtrace_symbols(array, size);
  fprintf(stderr, "Dumping %zd frames:\n", size - 1);
  for (i = 1; i < size; i++)
    {
      if (bt_strings != NULL)
        fprintf(stderr, "\tFrame %zd: %p: %s\n", i, array[i], bt_strings[i]);
      else
        fprintf(stderr, "\tFrame %zd: %p\n", i, array[i]);
    }
  /* This is #ifdef'd out:  In debug libraries, it causes a compile error. */
#if 0
  free(bt_strings); /* Since this is not allocated by MPIU_Malloc, do not use MPIU_Free */
#endif
}
#else
void MPIX_Dump_stacks(){}
#endif


void
MPIX_Progress_poke()
{
  MPID_Progress_poke();
}


int
MPIX_Progress_quiesce(double timeout)
{
  int rc;
  timeout *= 1.0e6;	/* convert to uSec */
  unsigned long long cycles = timeout; /* convert to long long */
  /* default to 10mS */
  cycles = (cycles ? cycles : 10000) * MPIDI_HW.clockMHz;
  unsigned long long t0;
  t0 = PAMI_Wtimebase(MPIDI_Client);
  while (PAMI_Wtimebase(MPIDI_Client) - t0 < cycles) {
        rc = MPID_Progress_wait_inline(1);
        if (rc != MPI_SUCCESS) return rc;
  }
  return MPI_SUCCESS;
}


int
MPIX_Comm_rank2global(MPI_Comm comm, int crank, int *grank)
{
  if (grank == NULL)
    return MPI_ERR_ARG;

  MPID_Comm *comm_ptr = NULL;
  MPID_Comm_get_ptr(comm, comm_ptr);
  if (comm_ptr == NULL)
    return MPI_ERR_COMM;

  if (crank >= comm_ptr->local_size)
    return MPI_ERR_RANK;

  *grank = MPID_VCR_GET_LPID(comm_ptr->vcr, crank);
  return MPI_SUCCESS;
}


int
MPIX_Hardware(MPIX_Hardware_t *hw)
{
  if (hw == NULL)
    return MPI_ERR_ARG;

  /*
   * We've already initialized the hw structure in MPID_Init,
   * so just copy it to the users buffer
   */
  memcpy(hw, &MPIDI_HW, sizeof(MPIX_Hardware_t));
  return MPI_SUCCESS;
}

#if (MPIDI_PRINTENV || MPIDI_STATISTICS || MPIDI_BANNER)
void mpc_statistics_write() __attribute__ ((alias("MPIX_statistics_write")));
void mp_statistics_write() __attribute__ ((alias("MPIXF_statistics_write")));
void mp_statistics_write_() __attribute__ ((alias("MPIXF_statistics_write")));
void mp_statistics_write__() __attribute__ ((alias("MPIXF_statistics_write")));
void mpc_statistics_zero() __attribute__ ((alias("MPIX_statistics_zero")));
void mp_statistics_zero() __attribute__ ((alias("MPIXF_statistics_zero")));
void mp_statistics_zero_() __attribute__ ((alias("MPIXF_statistics_zero")));
void mp_statistics_zero__() __attribute__ ((alias("MPIXF_statistics_zero")));

  /* ------------------------------------------- */
  /* - MPIDI_Statistics_zero  and        -------- */
  /* - MPIDI_Statistics_write can be     -------- */
  /* - called during init and finalize   -------- */
  /* - PE utiliti routines               -------- */
  /* -------------------------------------------- */

int
MPIDI_Statistics_zero(void)
{
    int rc=0;

   mpid_statp->sends = 0;
   mpid_statp->sendsComplete = 0;
   mpid_statp->sendWaitsComplete = 0;
   mpid_statp->recvs = 0;
   mpid_statp->recvWaitsComplete = 0;
   mpid_statp->earlyArrivals = 0;
   mpid_statp->earlyArrivalsMatched = 0;
   mpid_statp->lateArrivals = 0;
   mpid_statp->unorderedMsgs = 0;

   return (rc); /* to map with current PE support */
}
 /***************************************************************************
 Function Name: _MPIX_statistics_zero

 Description: Call the corresponding MPIDI_statistics_zero function to initialize/clear
              statistics counter.

 Parameters:
 Name               Type         I/O
 void
 int                >0           Success
                    <0           statistics not enable
 ***************************************************************************/

int _MPIX_statistics_zero (void)
{
    int rc = MPIDI_Statistics_zero();
    if (rc < 0) {
        MPID_assert(rc == PAMI_SUCCESS);
    }
    return(rc);
}

int MPIX_statistics_zero(void)
{
    return(_MPIX_statistics_zero());
}

void MPIXF_statistics_zero(int *rc)
{
    *rc = _MPIX_statistics_zero();
}


int
MPIDI_Statistics_write(FILE *statfile) {

    int rc=-1;
    int i;
    char time_buf[201];
    extern pami_extension_t extension;
    pami_configuration_t  query_stat;
    pami_statistics_t *pami_stat;
    pami_counter_t *pami_counters;
    long long Tot_dup_pkt_cnt=0;
    long long Tot_retrans_pkt_cnt=0;
    long long Tot_gho_pkt_cnt=0;
    long long Tot_pkt_sent_cnt=0;
    long long Tot_pkt_recv_cnt=0;
    long long Tot_data_sent=0;
    long long Tot_data_recv=0;
    extern long mem_hwmark;

    memset(&time_buf,0, 201);
    sprintf(time_buf, __DATE__" "__TIME__);
    mpid_statp->buffer_mem_hwmark =  mem_hwmark;
    mpid_statp->sendWaitsComplete =  mpid_statp->sends - mpid_statp->sendsComplete;
    fprintf(statfile,"Start of task (pid=%d) statistics at %s \n", getpid(), time_buf);
    fprintf(statfile, "MPICH: sends = %ld\n", mpid_statp->sends);
    fprintf(statfile, "MPICH: sendsComplete = %ld\n", mpid_statp->sendsComplete);
    fprintf(statfile, "MPICH: sendWaitsComplete = %ld\n", mpid_statp->sendWaitsComplete);
    fprintf(statfile, "MPICH: recvs = %ld\n", mpid_statp->recvs);
    fprintf(statfile, "MPICH: recvWaitsComplete = %ld\n", mpid_statp->recvWaitsComplete);
    fprintf(statfile, "MPICH: earlyArrivals = %ld\n", mpid_statp->earlyArrivals);
    fprintf(statfile, "MPICH: earlyArrivalsMatched = %ld\n", mpid_statp->earlyArrivalsMatched);
    fprintf(statfile, "MPICH: lateArrivals = %ld\n", mpid_statp->lateArrivals);
    fprintf(statfile, "MPICH: unorderedMsgs = %ld\n", mpid_statp->unorderedMsgs);
    fprintf(statfile, "MPICH: buffer_mem_hwmark = %ld\n", mpid_statp->buffer_mem_hwmark);
    fflush(statfile);
    memset(&query_stat,0, sizeof(query_stat));
    query_stat.name =  (pami_attribute_name_t)PAMI_CONTEXT_STATISTICS ;
    rc = PAMI_Context_query(MPIDI_Context[0], &query_stat, 1);
    pami_stat = (pami_statistics_t*)query_stat.value.chararray;
    pami_counters = pami_stat->counters;
    if (!rc) {
        for (i = 0; i < pami_stat->count; i ++) {
             printf("+++%s:%llu\n", pami_counters[i].name, pami_counters[i].value);
             if (!strncasecmp("Duplicate Pkt Count",pami_counters[i].name,19)) {
                  Tot_dup_pkt_cnt=pami_counters[i].value;
             } else if (!strncasecmp("Retransmit Pkt Count",pami_counters[i].name,20)) {
                  Tot_retrans_pkt_cnt=pami_counters[i].value;
             } else if (!strncasecmp("Ghost Pkt Count",pami_counters[i].name,15)) {
                  Tot_gho_pkt_cnt=pami_counters[i].value;
             } else if (!strncasecmp("Packets Sent",pami_counters[i].name,12) &&
                        (!(strchr(pami_counters[i].name, 'v')))) {
                  /* Packets Sent, not Packets Sent via SHM   */
                  Tot_pkt_sent_cnt=pami_counters[i].value;
             } else if (!strncasecmp("Packets Received",pami_counters[i].name,16) &&
                        (!(strchr(pami_counters[i].name, 'S')))) {
                  /* Packets Received, not Packets Received via SHM   */
                  Tot_pkt_recv_cnt=pami_counters[i].value;
             } else if (!strncasecmp("Data Sent",pami_counters[i].name,9) &&
                        (!(strchr(pami_counters[i].name, 'v')))) {
                  /* Data Sent, not Data Sent via SHM   */
                  Tot_data_sent=pami_counters[i].value;
             } else if (!strncasecmp("Data Received",pami_counters[i].name,13) &&
                        (!(strchr(pami_counters[i].name, 'S')))) {
                  /* Data Received, not Data Received via SHM   */
                  Tot_data_recv=pami_counters[i].value;
             }
         }
         fprintf(statfile, "PAMI: Tot_dup_pkt_cnt=%lld\n", Tot_dup_pkt_cnt);
         fprintf(statfile, "PAMI: Tot_retrans_pkt_cnt=%lld\n", Tot_retrans_pkt_cnt);
         fprintf(statfile, "PAMI: Tot_gho_pkt_cnt=%lld\n", Tot_gho_pkt_cnt);
         fprintf(statfile, "PAMI: Tot_pkt_sent_cnt=%lld\n", Tot_pkt_sent_cnt);
         fprintf(statfile, "PAMI: Tot_pkt_recv_cnt=%lld\n", Tot_pkt_recv_cnt);
         fprintf(statfile, "PAMI: Tot_data_sent=%lld\n", Tot_data_sent);
         fprintf(statfile, "PAMI: Tot_data_recv=%lld\n", Tot_data_recv);
         fflush(statfile);
        } else {
         TRACE_ERR("PAMID: PAMI_Context_query() with PAMI_CONTEXT_STATISTICS failed rc =%d\
n",rc);
        }
   return (rc);
}
 /***************************************************************************
 Function Name: _MPIX_statistics_write
 Description: Call MPIDI_Statistics_write  to write statistical
              information to specified file descriptor.   
 Parameters:
 Name               Type         I/O
 fptr               FILE*        I    File pointer, can be stdout or stderr.
                                      If it is to a file, user has to open
                                      the file.
 rc (Fortran only)  int          0    Return sum from MPIDI_Statistics_write calls
 <returns> (C only)  0                Both MPICH and PAMI statistics
 ***************************************************************************/
int _MPIX_statistics_write(FILE* fptr)
{
    int rc = MPIDI_Statistics_write(fptr);
    if (rc < 0) {
        MPID_assert(rc == PAMI_SUCCESS);
    }
    return(rc);
}

int MPIX_statistics_write(FILE* fptr)
{
    return(_MPIX_statistics_write(fptr));
}

/* Fortran:  fdes is pointer to a file descriptor.
 *           rc   is pointer to buffer for storing return code.
 *
 * Note: Fortran app. will convert a Fortran I/O unit to a file
 *       descriptor by calling Fortran utilities, flush_ and getfd.
 *       When fdes=1, output is to STDOUT.  When fdes=2, output is to STDERR.
 */

void MPIXF_statistics_write(int *fdes, int *rc)
{
    FILE *fp;
    int  dup_fd;
    int  closefp=0;

    /* Convert the DUP file descriptor to a FILE pointer */
    dup_fd = dup(*fdes);
    if ( (fp = fdopen(dup_fd, "a")) != NULL )
       closefp = 1;
    else
       fp = stdout;    /* If fdopen failed then default to stdout */

    *rc = _MPIX_statistics_write(fp);

    /* The check is because I don't want to close stdout. */
    if ( closefp ) fclose(fp);
}

void MPIXF_statistics_write_(int *fdes, int *rc)
{
    FILE *fp;
    int  dup_fd;
    int  closefp=0;

    /* Convert the DUP file descriptor to a FILE pointer */
    dup_fd = dup(*fdes);
    if ( (fp = fdopen(dup_fd, "a")) != NULL )
       closefp = 1;
    else
       fp = stdout;    /* If fdopen failed then default to stdout */

    *rc = _MPIX_statistics_write(fp);

    /* The check is because I don't want to close stdout. */
    if ( closefp ) fclose(fp);
}
#endif


#ifdef __BGQ__

int
MPIX_Torus_ndims(int *numdimensions)
{
  const pamix_torus_info_t *info = PAMIX_Torus_info();
  *numdimensions = info->dims - 1;
  return MPI_SUCCESS;
}


int
MPIX_Rank2torus(int rank, int *coords)
{
  size_t coord_array[MPIDI_HW.torus_dimension+1];

  PAMIX_Task2torus(rank, coord_array);

  unsigned i;
  for(i=0; i<MPIDI_HW.torus_dimension+1; ++i)
    coords[i] = coord_array[i];

  return MPI_SUCCESS;
}


int
MPIX_Torus2rank(int *coords, int *rank)
{
  size_t coord_array[MPIDI_HW.torus_dimension+1];
  pami_task_t task;

  unsigned i;
  for(i=0; i<MPIDI_HW.torus_dimension+1; ++i)
    coord_array[i] = coords[i];

  PAMIX_Torus2task(coord_array, &task);
  *rank = task;

  return MPI_SUCCESS;
}


typedef struct
{
  pami_geometry_t geometry;
  pami_work_t state;
  pami_configuration_t config;
  size_t num_configs;
  pami_event_function fn;
  void* cookie;
} MPIX_Comm_update_data_t;

static void
MPIX_Comm_update_done(void *ctxt, void *data, pami_result_t err)
{
   int *active = (int *)data;
   (*active)--;
}

static pami_result_t
MPIX_Comm_update_post(pami_context_t context, void *cookie)
{
  MPIX_Comm_update_data_t *data = (MPIX_Comm_update_data_t *)cookie;

  return PAMI_Geometry_update(data->geometry,
                              &data->config,
                              data->num_configs,
                              context,
                              data->fn,
                              data->cookie);
}

int
MPIX_Comm_update(MPI_Comm comm, int optimize)
{
   MPID_Comm * comm_ptr;
   volatile int geom_update = 1;
   MPIX_Comm_update_data_t data;
   pami_configuration_t config;

   MPID_Comm_get_ptr(comm, comm_ptr);
   if (!comm_ptr || comm == MPI_COMM_NULL)
      return MPI_ERR_COMM;

   /* First, check if there is a geometry. When optimized collectives
    * are disabled, no geometry is created */
   if(comm_ptr->mpid.geometry == NULL)
      return MPI_ERR_COMM;

   config.name = PAMI_GEOMETRY_OPTIMIZE;
   config.value.intval = !!optimize;

   TRACE_ERR("About to %s geometry update function\n", MPIDI_Process.context_post.active>0?"post":"invoke");
   data.num_configs = 1;
   data.config.name = config.name;
   data.config.value.intval = config.value.intval;
   data.fn = MPIX_Comm_update_done;
   data.cookie = (void *)&geom_update;
   data.geometry = comm_ptr->mpid.geometry;
   MPIDI_Context_post(MPIDI_Context[0],
                      &data.state,
                      MPIX_Comm_update_post,
                      &data);
   TRACE_ERR("Geometry update function %s\n", MPIDI_Process.context_post.active>0?"posted":"invoked");

   TRACE_ERR("Waiting for geometry update to finish\n");

   MPID_PROGRESS_WAIT_WHILE(geom_update);

  MPIDI_Comm_coll_query(comm_ptr);
  MPIDI_Comm_coll_envvars(comm_ptr);
  if(MPIDI_Process.optimized.select_colls)
     MPIDI_Comm_coll_select(comm_ptr);

  return MPI_SUCCESS;
}

int
MPIX_Get_last_algorithm_name(MPI_Comm comm, char *protocol, int length)
{
   MPID_Comm *comm_ptr;
   MPID_Comm_get_ptr(comm, comm_ptr);

   if(!comm_ptr || comm == MPI_COMM_NULL)
      return MPI_ERR_COMM;
   if(!protocol || length <= 0)
      return MPI_ERR_ARG;
   strncpy(protocol, comm_ptr->mpid.last_algorithm, length);
   return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPIX_IO_node_id
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIX_IO_node_id ()
{
  static unsigned long IO_node_id = ULONG_MAX;

  if (IO_node_id != ULONG_MAX)
    return (int)(IO_node_id>>32);

  int rc;
  int fd;
  char* uci_str;
  char buffer[4096];

  fd = open("/dev/bgpers", O_RDONLY, 0);
  assert(fd>=0);
  rc = read(fd, buffer, sizeof(buffer));
  assert(rc>0);
  close(fd);

  uci_str = strstr(buffer, "BG_UCI=");
  assert(uci_str);
  uci_str += sizeof("BG_UCI=")-1;

  IO_node_id = strtoul(uci_str, NULL, 16);
  return (int)(IO_node_id>>32);
}

#undef FUNCNAME
#define FUNCNAME MPIX_IO_link_id
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIX_IO_link_id ()
{
  int nB,  nC,  nD,  nE;                /* Size of each torus dimension  */
  int brA, brB, brC, brD, brE;               /* The bridge node's coordinates */

  Personality_t personality;

  Kernel_GetPersonality(&personality, sizeof(personality));

  nB  = personality.Network_Config.Bnodes;
  nC  = personality.Network_Config.Cnodes;
  nD  = personality.Network_Config.Dnodes;
  nE  = personality.Network_Config.Enodes;

  brA = personality.Network_Config.cnBridge_A;
  brB = personality.Network_Config.cnBridge_B;
  brC = personality.Network_Config.cnBridge_C;
  brD = personality.Network_Config.cnBridge_D;
  brE = personality.Network_Config.cnBridge_E;

  /*
   * This is the bridge node, numbered in ABCDE order, E increments first.
   * It is considered the unique "io node route identifer" because each
   * bridge node only has one torus link to one io node.
   */
  return brE + brD*nE + brC*nD*nE + brB*nC*nD*nE + brA*nB*nC*nD*nE;
};

#undef FUNCNAME
#define FUNCNAME MPIX_IO_distance
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIX_IO_distance ()
{
  int iA,  iB,  iC,  iD,  iE;                /* The local node's coordinates  */
  int nA,  nB,  nC,  nD,  nE;                /* Size of each torus dimension  */
  int brA, brB, brC, brD, brE;               /* The bridge node's coordinates */
  int Nflags;
  int d1, d2;
  int dA, dB, dC, dD, dE;          /* distance from local node to bridge node */

  Personality_t personality;

  Kernel_GetPersonality(&personality, sizeof(personality));

  iA  = personality.Network_Config.Acoord;
  iB  = personality.Network_Config.Bcoord;
  iC  = personality.Network_Config.Ccoord;
  iD  = personality.Network_Config.Dcoord;
  iE  = personality.Network_Config.Ecoord;

  nA  = personality.Network_Config.Anodes;
  nB  = personality.Network_Config.Bnodes;
  nC  = personality.Network_Config.Cnodes;
  nD  = personality.Network_Config.Dnodes;
  nE  = personality.Network_Config.Enodes;

  brA = personality.Network_Config.cnBridge_A;
  brB = personality.Network_Config.cnBridge_B;
  brC = personality.Network_Config.cnBridge_C;
  brD = personality.Network_Config.cnBridge_D;
  brE = personality.Network_Config.cnBridge_E;

  Nflags = personality.Network_Config.NetFlags;

  dA = abs(iA - brA);
  if (Nflags & ND_ENABLE_TORUS_DIM_A)
  {
    d1 = dA;
    d2 = nA - d1;
    dA = (d1 < d2) ? d1 : d2;
  }

  dB = abs(iB - brB);
  if (Nflags & ND_ENABLE_TORUS_DIM_B)
  {
    d1 = dB;
    d2 = nB - d1;
    dB = (d1 < d2) ? d1 : d2;
  }

  dC = abs(iC - brC);
  if (Nflags & ND_ENABLE_TORUS_DIM_C)
  {
    d1 = dC;
    d2 = nC - d1;
    dC = (d1 < d2) ? d1 : d2;
  }

  dD = abs(iD - brD);
  if (Nflags & ND_ENABLE_TORUS_DIM_D)
  {
    d1 = dD;
    d2 = nD - d1;
    dD = (d1 < d2) ? d1 : d2;
  }

  dE = abs(iE - brE);
  if (Nflags & ND_ENABLE_TORUS_DIM_E)
  {
    d1 = dE;
    d2 = nE - d1;
    dE = (d1 < d2) ? d1 : d2;
  }

  /* This is the number of hops to the io node */
  return dA + dB + dC + dD + dE + 1;
};

#undef FUNCNAME
#define FUNCNAME MPIX_Pset_io_node
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
void
MPIX_Pset_io_node (int *io_node_route_id, int *distance_to_io_node)
{
  *io_node_route_id    = MPIX_IO_link_id ();
  *distance_to_io_node = MPIX_IO_distance ();

  return;
};

/**
 * \brief Create a communicator of ranks that have a common bridge node.
 *
 * \note This function is private to this source file.
 *
 * \param [in]  parent_comm_ptr  Pointer to the parent communicator
 * \param [out] pset_comm_ptr    Pointer to the new 'MPID' communicator
 *
 * \return MPI status
 */
int _MPIX_Pset_same_comm_create (MPID_Comm *parent_comm_ptr, MPID_Comm **pset_comm_ptr)
{
  int color, key;
  int mpi_errno;

  color = MPIX_IO_link_id ();
  key   = MPIX_IO_distance ();

  /*
   * Use MPIR_Comm_split_impl to make a communicator of all ranks in the parent
   * communicator that share the same bridge node; i.e. the 'color' is the
   * 'io node route identifer', which is unique to each BGQ bridge node.
   *
   * Setting the 'key' to the 'distance to io node' ensures that rank 0 in
   * the new communicator is on the bridge node, or as close to the bridge node
   * as possible.
   */

  *pset_comm_ptr = NULL;
  mpi_errno = MPI_SUCCESS;
  mpi_errno = MPIR_Comm_split_impl(parent_comm_ptr, color, key, pset_comm_ptr);

  return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIX_Pset_same_comm_create_from_parent
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int
MPIX_Pset_same_comm_create_from_parent (MPI_Comm parent_comm, MPI_Comm *pset_comm)
{
  int mpi_errno;
  MPID_Comm *parent_comm_ptr, *pset_comm_ptr;

  *pset_comm = MPI_COMM_NULL;

  /*
   * Convert the parent communicator object handle to an object pointer;
   * needed by the error handling code.
   */
  parent_comm_ptr = NULL;
  MPID_Comm_get_ptr(parent_comm, parent_comm_ptr);

  mpi_errno = MPI_SUCCESS;
  mpi_errno = _MPIX_Pset_same_comm_create (parent_comm_ptr, &pset_comm_ptr);
  if (mpi_errno) MPIU_ERR_POP(mpi_errno);
  if (pset_comm_ptr)
    MPIU_OBJ_PUBLISH_HANDLE(*pset_comm, pset_comm_ptr->handle);
  else
    goto fn_fail;

fn_exit:
  return mpi_errno;
fn_fail:
  mpi_errno = MPIR_Err_return_comm( parent_comm_ptr, FCNAME, mpi_errno );
  goto fn_exit;
};

#undef FUNCNAME
#define FUNCNAME MPIX_Pset_same_comm_create
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int
MPIX_Pset_same_comm_create (MPI_Comm *pset_comm)
{
  return MPIX_Pset_same_comm_create_from_parent (MPI_COMM_WORLD, pset_comm);
};


#undef FUNCNAME
#define FUNCNAME MPIX_Pset_diff_comm_create_from_parent
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int
MPIX_Pset_diff_comm_create_from_parent (MPI_Comm parent_comm, MPI_Comm *pset_comm)
{
  MPID_Comm *parent_comm_ptr, *pset_same_comm_ptr, *pset_diff_comm_ptr;
  int color, key;
  int mpi_errno;

  *pset_comm = MPI_COMM_NULL;

  /*
   * Convert the parent communicator object handle to an object pointer;
   * needed by the error handling code.
   */
  parent_comm_ptr = NULL;
  MPID_Comm_get_ptr(parent_comm, parent_comm_ptr);

  /*
   * Determine the 'color' of this rank to create the new communicator - which
   * is the rank in a (transient) communicator where all ranks share a common
   * bridge node.
   */
  mpi_errno = MPI_SUCCESS;
  mpi_errno = _MPIX_Pset_same_comm_create (parent_comm_ptr, &pset_same_comm_ptr);
  if (mpi_errno) MPIU_ERR_POP(mpi_errno);
  if (pset_same_comm_ptr == NULL)
    goto fn_fail;

  color = MPIR_Comm_rank(pset_same_comm_ptr) * MPIDI_HW.ppn + MPIDI_HW.coreID;

  /* Discard the 'pset_same_comm_ptr' .. it is no longer needed. */
  mpi_errno = MPIR_Comm_free_impl(pset_same_comm_ptr);
  if (mpi_errno) MPIU_ERR_POP(mpi_errno);

  /* Set the 'key' for this rank to order the ranks in the new communicator. */
  key = MPIR_Comm_rank(parent_comm_ptr);

  pset_diff_comm_ptr = NULL;
  mpi_errno = MPI_SUCCESS;
  mpi_errno = MPIR_Comm_split_impl(parent_comm_ptr, color, key, &pset_diff_comm_ptr);
  if (mpi_errno) MPIU_ERR_POP(mpi_errno);
  if (pset_diff_comm_ptr)
    MPIU_OBJ_PUBLISH_HANDLE(*pset_comm, pset_diff_comm_ptr->handle);
  else
    goto fn_fail;

fn_exit:
  return mpi_errno;
fn_fail:
  mpi_errno = MPIR_Err_return_comm( parent_comm_ptr, FCNAME, mpi_errno );
  goto fn_exit;
};

#undef FUNCNAME
#define FUNCNAME MPIX_Pset_diff_comm_create
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int
MPIX_Pset_diff_comm_create (MPI_Comm *pset_comm)
{
  return MPIX_Pset_diff_comm_create_from_parent (MPI_COMM_WORLD, pset_comm);
};



/**
 * \brief Compare each elemt of two six-element arrays
 * \param [in] A The first array
 * \param [in] B The first array
 * \return MPI_SUCCESS (does not return on failure)
 */
#define CMP_6(A,B)                              \
({                                              \
  assert(A[0] == B[0]);                         \
  assert(A[1] == B[1]);                         \
  assert(A[2] == B[2]);                         \
  assert(A[3] == B[3]);                         \
  assert(A[4] == B[4]);                         \
  assert(A[5] == B[5]);                         \
  MPI_SUCCESS;                                  \
})

#undef FUNCNAME
#define FUNCNAME MPIX_Cart_comm_create
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int
MPIX_Cart_comm_create (MPI_Comm *cart_comm)
{
  int result;
  int rank, numprocs,
      dims[6],
      wrap[6],
      coords[6];
  int new_rank1, new_rank2;
  MPI_Comm new_comm = MPI_COMM_NULL;
  int cart_rank,
      cart_dims[6],
      cart_wrap[6],
      cart_coords[6];
  int Nflags;

  *cart_comm = MPI_COMM_NULL;
  PMPI_Comm_rank(MPI_COMM_WORLD, &rank);
  PMPI_Comm_size(MPI_COMM_WORLD, &numprocs);

  Personality_t personality;

  Kernel_GetPersonality(&personality, sizeof(personality));

  dims[0] = personality.Network_Config.Anodes;
  dims[1] = personality.Network_Config.Bnodes;
  dims[2] = personality.Network_Config.Cnodes;
  dims[3] = personality.Network_Config.Dnodes;
  dims[4] = personality.Network_Config.Enodes;
  dims[5] = Kernel_ProcessCount();

  /* This only works if MPI_COMM_WORLD is the full partition */
  if (dims[5] * dims[4] * dims[3] * dims[2] * dims[1] * dims[0] != numprocs)
    return MPI_ERR_TOPOLOGY;

  Nflags = personality.Network_Config.NetFlags;
  wrap[0] = ((Nflags & ND_ENABLE_TORUS_DIM_A) != 0);
  wrap[1] = ((Nflags & ND_ENABLE_TORUS_DIM_B) != 0);
  wrap[2] = ((Nflags & ND_ENABLE_TORUS_DIM_C) != 0);
  wrap[3] = ((Nflags & ND_ENABLE_TORUS_DIM_D) != 0);
  wrap[4] = ((Nflags & ND_ENABLE_TORUS_DIM_E) != 0);
  wrap[5] = 1;

  coords[0] = personality.Network_Config.Acoord;
  coords[1] = personality.Network_Config.Bcoord;
  coords[2] = personality.Network_Config.Ccoord;
  coords[3] = personality.Network_Config.Dcoord;
  coords[4] = personality.Network_Config.Ecoord;
  coords[5] = Kernel_MyTcoord();

  new_rank1 =                                         coords[5] +
                                            dims[5] * coords[4] +
                                  dims[5] * dims[4] * coords[3] +
                        dims[5] * dims[4] * dims[3] * coords[2] +
              dims[5] * dims[4] * dims[3] * dims[2] * coords[1] +
    dims[5] * dims[4] * dims[3] * dims[2] * dims[1] * coords[0];

  result = PMPI_Comm_split(MPI_COMM_WORLD, 0, new_rank1, &new_comm);
  if (result != MPI_SUCCESS)
  {
     PMPI_Comm_free(&new_comm);
     return result;
  }
  PMPI_Comm_rank(new_comm, &new_rank2);
  assert(new_rank1 == new_rank2);

  result = PMPI_Cart_create(new_comm,
                            6,
                            dims,
                            wrap,
                            0,
                            cart_comm);
  if (result != MPI_SUCCESS)
    return result;

  PMPI_Comm_rank(*cart_comm, &cart_rank);
  PMPI_Cart_get (*cart_comm, 6, cart_dims, cart_wrap, cart_coords);

  CMP_6(dims,   cart_dims);
  CMP_6(wrap,   cart_wrap);
  CMP_6(coords, cart_coords);

  PMPI_Comm_free(&new_comm);
  return MPI_SUCCESS;
};

/**
 * \brief FORTRAN interface to MPIX_Comm_rank2global
 *
 * \param [in] comm  Communicator
 * \param [in] crank Pointer to the rank in the communicator variable
 * \param [out] grank Pointer tot he global rank variable
 *
 * \return status
 */
int mpix_comm_rank2global (MPI_Comm *comm, int *crank, int *grank)
{
  return MPIX_Comm_rank2global (*comm, *crank, grank);
}

/**
 * \brief FORTRAN interface to MPIX_Pset_same_comm_create
 *
 * \param [out] pset_comm  Communicator
 *
 * \return status
 */
int mpix_pset_same_comm_create (MPI_Comm *pset_comm)
{
  return MPIX_Pset_same_comm_create (pset_comm);
}

/**
 * \brief FORTRAN interface to MPIX_Pset_diff_comm_create
 *
 * \param [out] pset_comm  Communicator
 *
 * \return status
 */
int mpix_pset_diff_comm_create (MPI_Comm *pset_comm)
{
  return MPIX_Pset_diff_comm_create (pset_comm);
}

/**
 * \brief FORTRAN interface to MPIX_Pset_same_comm_create_from_parent
 *
 * \param [in]  parent_comm  Parent communicator
 * \param [out] pset_comm    New pset communicator
 *
 * \return status
 */
int mpix_pset_same_comm_create_from_parent (MPI_Comm *parent_comm, MPI_Comm *pset_comm)
{
  return MPIX_Pset_same_comm_create_from_parent (*parent_comm, pset_comm);
}

/**
 * \brief FORTRAN interface to MPIX_Pset_diff_comm_create_from_parent
 *
 * \param [in]  parent_comm  Parent communicator
 * \param [out] pset_comm    New pset communicator
 *
 * \return status
 */
int mpix_pset_diff_comm_create_from_parent (MPI_Comm *parent_comm, MPI_Comm *pset_comm)
{
  return MPIX_Pset_diff_comm_create_from_parent (*parent_comm, pset_comm);
}

/**
 * \brief FORTRAN interface to MPIX_IO_node_id
 *
 * \param [out] io_node_id    This rank's io node id
 */
void mpix_io_node_id (int *io_node_id) { *io_node_id = MPIX_IO_node_id(); }

/**
 * \brief FORTRAN interface to MPIX_IO_link_id
 *
 * \param [out] io_link_id    This rank's io link id
 */
void mpix_io_link_id (int *io_link_id) { *io_link_id = MPIX_IO_link_id(); }

/**
 * \brief FORTRAN interface to MPIX_IO_distance
 *
 * \param [out] io_distance   This rank's distance to the io node
 */
void mpix_io_distance (int *io_distance) { *io_distance = MPIX_IO_distance(); }

/**
 * \brief FORTRAN interface to MPIX_Cart_comm_create
 *
 * \param [out] cart_comm  Communicator to create
 *
 * \return status
 */
int mpix_cart_comm_create (MPI_Comm *cart_comm)
{
  return MPIX_Cart_comm_create (cart_comm);
}



#endif

#ifdef __PE__
void mpc_disableintr() __attribute__ ((alias("MPIX_disableintr")));
void mp_disableintr() __attribute__ ((alias("MPIXF_disableintr")));
void mp_disableintr_() __attribute__ ((alias("MPIXF_disableintr")));
void mp_disableintr__() __attribute__ ((alias("MPIXF_disableintr")));
void mpc_enableintr() __attribute__ ((alias("MPIX_enableintr")));
void mp_enableintr() __attribute__ ((alias("MPIXF_enableintr")));
void mp_enableintr_() __attribute__ ((alias("MPIXF_enableintr")));
void mp_enableintr__() __attribute__ ((alias("MPIXF_enableintr")));
void mpc_queryintr() __attribute__ ((weak,alias("MPIX_queryintr")));
void mp_queryintr() __attribute__ ((alias("MPIXF_queryintr")));
void mp_queryintr_() __attribute__ ((alias("MPIXF_queryintr")));
void mp_queryintr__() __attribute__ ((alias("MPIXF_queryintr")));

 /***************************************************************************
 Function Name: MPIX_disableintr

 Description: Call the pamid layer to disable interrupts.
              (Similar to setting MP_CSS_INTERRUPT to "no")

 Parameters: The Fortran versions have an int* parameter used to pass the
             return code to the calling program.

 Returns: 0     Success
         <0     Failure
 ***************************************************************************/

int
_MPIDI_disableintr()
{
        return(MPIDI_disableintr());
}

int
MPIX_disableintr()
{
        return(_MPIDI_disableintr());
}

void
MPIXF_disableintr(int *rc)
{
        *rc = _MPIDI_disableintr();
}

void
MPIXF_disableintr_(int *rc)
{
        *rc = _MPIDI_disableintr();
}

/*
 ** Called by: _mp_disableintr
 ** Purpose : Disables interrupts
 */
int
MPIDI_disableintr()
{
    pami_result_t rc=0;
    int i;

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    if (MPIDI_Process.mp_interrupts!= 0)
       {
         TRACE_ERR("Async advance beginning...\n");
         /* Enable async progress on all contexts.*/
         for (i=0; i<MPIDI_Process.avail_contexts; ++i)
         {
             PAMIX_Progress_disable(MPIDI_Context[i], PAMIX_PROGRESS_ALL);
          }
         TRACE_ERR("Async advance disabled\n");
         MPIDI_Process.mp_interrupts=0;
       }
    return(rc);
}
 /***************************************************************************
 Function Name: MPIX_enableintr

 Description: Call the pamid-layer function to enable interrupts.
              (Similar to setting MP_CSS_INTERRUPT to "yes")

 Parameters: The Fortran versions have an int* parameter used to pass the
             return code to the calling program.

 Returns: 0     Success
         <0     Failure
 ***************************************************************************/
int
_MPIDI_enableintr()
{
       return(MPIDI_enableintr());
}

/* C callable version           */
int
MPIX_enableintr()
{
        return(_MPIDI_enableintr());
}

/* Fortran callable version     */                  
void 
MPIXF_enableintr(int *rc)
{
        *rc = _MPIDI_enableintr();
}

/* Fortran callable version for -qEXTNAME support  */
void 
MPIXF_enableintr_(int *rc)
{
        *rc = _MPIDI_enableintr();
}

int
MPIDI_enableintr()
{
    pami_result_t rc=0;
    int i;

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    if (MPIDI_Process.mp_interrupts == 0)
       {
         /* Enable async progress on all contexts.*/
         for (i=0; i<MPIDI_Process.avail_contexts; ++i)
         {
             PAMIX_Progress_enable(MPIDI_Context[i], PAMIX_PROGRESS_ALL);
          }
         TRACE_ERR("Async advance enabled\n");
         MPIDI_Process.mp_interrupts=1;
       }
    MPID_assert(rc == PAMI_SUCCESS);
    return(rc);
}

 /***************************************************************************
 Function Name: MPIX_queryintr

 Description: Call the pamid-layer function to determine if
              interrupts are currently on or off.

 Parameters: The Fortran versions have an int* parameter used to pass the
             current interrupt setting to the calling program.
 Returns: 0     Indicates interrupts are currently off
          1     Indicates interrupts are currently on
         <0     Failure
 ***************************************************************************/
int
MPIDI_queryintr()
{
        return(MPIDI_Process.mp_interrupts);
}

int
_MPIDI_queryintr()
{
        return(MPIDI_queryintr());
}

int
MPIX_queryintr()
{
        return(_MPIDI_queryintr());
}

void
MPIXF_queryintr(int *rc)
{
        *rc = _MPIDI_queryintr();
}

void
MPIXF_queryintr_(int *rc)
{
        *rc = _MPIDI_queryintr();
}
#endif

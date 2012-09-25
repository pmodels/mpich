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
  /* ------------------------------------------- */
  /* - mpid_statistics_zero  and        -------- */
  /* - mpid_statistics_write can be     -------- */
  /* - called during init and finalize  -------- */
  /* - PE utiliti routines              -------- */
  /* ------------------------------------------- */

int
MPIX_Statistics_zero(void)
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
int
MPIX_Statistics_write (FILE *statfile) {

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

    memset(&time_buf,0, 201);
    sprintf(time_buf, __DATE__" "__TIME__);
    mpid_statp->sendWaitsComplete =  mpid_statp->sends - mpid_statp->sendsComplete;
    fprintf(statfile,"Start of task (pid=%d) statistics at %s \n", getpid(), time_buf);
    fprintf(statfile, "PAMID: sends = %ld\n", mpid_statp->sends);
    fprintf(statfile, "PAMID: sendsComplete = %ld\n", mpid_statp->sendsComplete);
    fprintf(statfile, "PAMID: sendWaitsComplete = %ld\n", mpid_statp->sendWaitsComplete);
    fprintf(statfile, "PAMID: recvs = %ld\n", mpid_statp->recvs);
    fprintf(statfile, "PAMID: recvWaitsComplete = %ld\n", mpid_statp->recvWaitsComplete);
    fprintf(statfile, "PAMID: earlyArrivals = %ld\n", mpid_statp->earlyArrivals);
    fprintf(statfile, "PAMID: earlyArrivalsMatched = %ld\n", mpid_statp->earlyArrivalsMatched);
    fprintf(statfile, "PAMID: lateArrivals = %ld\n", mpid_statp->lateArrivals);
    fprintf(statfile, "PAMID: unorderedMsgs = %ld\n", mpid_statp->unorderedMsgs);
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


#endif

/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file src/misc/mpid_init.c
 * \brief Normal job startup code
 */
#include "mpidimpl.h"
#include "mpidi_star.h"
#include <limits.h>

MPIDI_Protocol_t MPIDI_Protocols;
MPIDI_Process_t  MPIDI_Process;
DCMF_Hardware_t  mpid_hw;
MPID_Request __totalview_request_dummyvar;


void MPIDI_DCMF_Configure(int requested,
                          int * provided)
{
  DCMF_Configure_t dcmf_config;
  memset(&dcmf_config, 0x00, sizeof(DCMF_Configure_t));

  // When interrupts are on, must use MPI_THREAD_MULTIPLE
  // so locking is done to interlock between the main
  // thread and the interrupt handler thread.
  if ( MPIDI_Process.use_interrupts )
    dcmf_config.interrupts   = DCMF_INTERRUPTS_ON;
  else
    dcmf_config.interrupts   = DCMF_INTERRUPTS_OFF;

  // Attempt to set the same thread level as requestd
  dcmf_config.thread_level = requested;

  // Get the actual values back
  DCMF_Messager_configure(&dcmf_config, &dcmf_config);
  *provided = dcmf_config.thread_level;
  MPIR_ThreadInfo.thread_provided = *provided;
}


/**
 * \brief Initialize MPICH2 at ADI level.
 * \param[in,out] argc Unused
 * \param[in,out] argv Unused
 * \param[in]     requested The thread model requested by the user.
 * \param[out]    provided  The thread model provided to user.  This will be the same as requested, except in VNM.  This behavior is handled by DCMF_Messager_configure()
 * \param[out]    has_args  Set to TRUE
 * \param[out]    has_env   Set to TRUE
 * \return MPI_SUCCESS
 */
int MPID_Init(int * argc,
              char *** argv,
              int   requested,
              int * provided,
              int * has_args,
              int * has_env)
{
   int rank, size, i, rc;
   MPID_Comm * comm;
   DCMF_Result dcmf_rc;

   MPID_Executable_name = "FORTRAN";
   if (argc && *argv != NULL && *argv[0] != NULL)
      MPID_Executable_name = *argv[0];

  /* ------------------------- */
  /* initialize the statistics */
  /* ------------------------- */
  MPIDI_Statistics_init();

  /* ------------------------- */
  /* initialize request queues */
  /* ------------------------- */
  MPIDI_Recvq_init();

  /* ----------------------------------- */
  /* Read the ENV vars to setup defaults */
  /* ----------------------------------- */
  MPIDI_Env_setup();


  /* ----------------------------- */
  /* Initialize messager           */
  /* ----------------------------- */
  DCMF_Messager_initialize();
#ifdef USE_CCMI_COLL
  DCMF_Collective_initialize();
#endif /* USE_CCMI_COLL */
  DCMF_Hardware(&mpid_hw);

  if (MPIDI_Process.use_ssm)
    {
      DCMF_Put_Configuration_t ssm_put_config = { DCMF_DEFAULT_PUT_PROTOCOL, DCMF_DEFAULT_NETWORK };
      DCMF_Put_register (&MPIDI_Protocols.ssm_put, &ssm_put_config);
      DCMF_Send_Configuration_t ssm_msg_config =
        {
          DCMF_DEFAULT_SEND_PROTOCOL,
	  DCMF_DEFAULT_NETWORK,
          NULL,
          NULL,
          NULL,
          NULL,
        };
      ssm_msg_config.cb_recv_short = MPIDI_BG2S_SsmCtsCB;
      DCMF_Send_register (&MPIDI_Protocols.ssm_cts, &ssm_msg_config);
      ssm_msg_config.cb_recv_short = MPIDI_BG2S_SsmAckCB;
      DCMF_Send_register (&MPIDI_Protocols.ssm_ack, &ssm_msg_config);
    }
  else
    {
      /* ---------------------------------- */
      /* Register eager point-to-point send */
      /* ---------------------------------- */
      DCMF_Send_Configuration_t default_config =
	{
	  DCMF_DEFAULT_SEND_PROTOCOL,
	  DCMF_DEFAULT_NETWORK,
	  MPIDI_BG2S_RecvShortCB,
	  NULL,
	  MPIDI_BG2S_RecvCB,
	  NULL,
	};
      DCMF_Send_register (&MPIDI_Protocols.send, &default_config);

      /* ---------------------------------- */
      /* Register msg layer point-to-point send */
      /* ---------------------------------- */
      DCMF_Send_Configuration_t rzv_config =
	{
	  DCMF_RZV_SEND_PROTOCOL,
	  DCMF_DEFAULT_NETWORK,
	  MPIDI_BG2S_RecvShortCB,
	  NULL,
	  MPIDI_BG2S_RecvCB,
	  NULL,
	};
      dcmf_rc = DCMF_Send_register (&MPIDI_Protocols.mrzv, &rzv_config);
      /* If msg layer send failed to register (likely unimplemented),
       * set the limit to zero so it won't be used.
       */
      if ( dcmf_rc != DCMF_SUCCESS )
	MPIDI_Process.optrzv_limit = 0;

      /* ---------------------------------- */
      /* Register rzv point-to-point rts    */
      /* ---------------------------------- */
      default_config.cb_recv_short = MPIDI_BG2S_RecvRzvCB;
      default_config.cb_recv       = NULL;
      DCMF_Send_register (&MPIDI_Protocols.rzv, &default_config);
    }

  /* --------------------------- */
  /* Register point-to-point get */
  /* --------------------------- */
  DCMF_Get_Configuration_t get_config = { DCMF_DEFAULT_GET_PROTOCOL, DCMF_DEFAULT_NETWORK };
  DCMF_Get_register (&MPIDI_Protocols.get, &get_config);

  /* ---------------------------------- */
  /* Register control send              */
  /* ---------------------------------- */
  DCMF_Control_Configuration_t control_config =
    {
      DCMF_DEFAULT_CONTROL_PROTOCOL,
      DCMF_DEFAULT_NETWORK,
      MPIDI_BG2S_ControlCB, NULL
    };
  DCMF_Control_register (&MPIDI_Protocols.control, &control_config);

/* Set up interrupts and thread level before protocol registration */
  MPIDI_DCMF_Configure(requested, provided);

#ifdef USE_CCMI_COLL
  /* ---------------------------------- */
  /* Register the collectives           */
  /* ---------------------------------- */
  MPIDI_Coll_register();
#endif /* USE_CCMI_COLL */

  /* ------------------------------------------------------ */
  /* Set process attributes.                                */
  /* ------------------------------------------------------ */
  MPIR_Process.attrs.tag_ub = INT_MAX;
  MPIR_Process.attrs.wtime_is_global = 1;
  if (MPIDI_Process.optimized.topology)
    MPIR_Process.dimsCreate = MPID_Dims_create;


  /* ---------------------------------------- */
  /*  Get my rank and the process size        */
  /* ---------------------------------------- */
  rank = DCMF_Messager_rank();
  size = DCMF_Messager_size();


  /* -------------------------------- */
  /* Initialize MPI_COMM_WORLD object */
  /* -------------------------------- */

  comm = MPIR_Process.comm_world;
  comm->rank = rank;
  comm->remote_size = comm->local_size = size;
  rc = MPID_VCRT_Create(comm->remote_size, &comm->vcrt);
  MPID_assert(rc == MPI_SUCCESS);
  rc = MPID_VCRT_Get_ptr(comm->vcrt, &comm->vcr);
  MPID_assert(rc == MPI_SUCCESS);
  for (i=0; i<size; i++)
    comm->vcr[i] = i;

  /* comm_create for MPI_COMM_WORLD needs this information to ensure no
   * barriers are done in dual mode with multithreading
   * We don't get the thread_provided updated until AFTER MPID_Init is
   * finished so we need to know the requested thread level in comm_create
   */
  MPIDI_Comm_create(comm);

  /* ------------------------------- */
  /* Initialize MPI_COMM_SELF object */
  /* ------------------------------- */

  comm = MPIR_Process.comm_self;
  comm->rank = 0;
  comm->remote_size = comm->local_size = 1;
  rc = MPID_VCRT_Create(comm->remote_size, &comm->vcrt);
  MPID_assert(rc == MPI_SUCCESS);
  rc = MPID_VCRT_Get_ptr(comm->vcrt, &comm->vcr);
  MPID_assert(rc == MPI_SUCCESS);
  comm->vcr[0] = rank;

  /* ------------------------------- */
  /* Initialize timer data           */
  /* ------------------------------- */
  MPID_Wtime_init();

  /* ------------------------------- */
  *has_args = TRUE;
  *has_env  = TRUE;


  return MPI_SUCCESS;
}

/*
 * \brief This is called by MPI to let us know that MPI_Init is done.
 * We don't care.
 */
int MPID_InitCompleted(void)
{
  return MPI_SUCCESS;
}


int SSM_ABORT()
{
  fprintf(stderr, "Sender-side-matching is not yet supported\n");
  exit(1);
  return 1;
}

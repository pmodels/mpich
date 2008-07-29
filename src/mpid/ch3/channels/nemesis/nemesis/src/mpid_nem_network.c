/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpid_nem_impl.h"
#if(MPID_NEM_NET_MODULE == MPID_NEM_ERROR_MODULE)
#error Error in definition of MPID_NEM_*_MODULE macros
#elif  (MPID_NEM_NET_MODULE == MPID_NEM_GM_MODULE)
#include "gm_module.h"
#elif(MPID_NEM_NET_MODULE == MPID_NEM_MX_MODULE)
#include "mx_module.h"
#elif(MPID_NEM_NET_MODULE == MPID_NEM_TCP_MODULE)
#include "tcp_module.h"
#elif(MPID_NEM_NET_MODULE == MPID_NEM_ELAN_MODULE)
#include "elan_module.h"
#elif(MPID_NEM_NET_MODULE == MPID_NEM_NEWTCP_MODULE)
#include "newtcp_module.h"
#elif(MPID_NEM_NET_MODULE == MPID_NEM_NO_MODULE)
#include "dummy_module.h"
#elif(MPID_NEM_NET_MODULE == MPID_NEM_SCTP_MODULE)
#include "sctp_module.h"
#elif(MPID_NEM_NET_MODULE == MPID_NEM_IB_MODULE)
#include "ib_module.h"
#elif(MPID_NEM_NET_MODULE == MPID_NEM_PSM_MODULE)
#include "psm_module.h"
#else
#warning ">>>>>>>>>>>>>>>> WRONG NET MODULE SELECTION"
#endif

MPID_nem_net_module_init_t              MPID_nem_net_module_init              = 0;
MPID_nem_net_module_finalize_t          MPID_nem_net_module_finalize          = 0;
MPID_nem_net_module_ckpt_shutdown_t     MPID_nem_net_module_ckpt_shutdown     = 0;
MPID_nem_net_module_poll_t              MPID_nem_net_module_poll              = 0;
MPID_nem_net_module_send_t              MPID_nem_net_module_send              = 0;
MPID_nem_net_module_get_business_card_t MPID_nem_net_module_get_business_card = 0;
MPID_nem_net_module_connect_to_root_t   MPID_nem_net_module_connect_to_root   = 0;
MPID_nem_net_module_vc_init_t           MPID_nem_net_module_vc_init           = 0;
MPID_nem_net_module_vc_destroy_t        MPID_nem_net_module_vc_destroy        = 0;
MPID_nem_net_module_vc_terminate_t      MPID_nem_net_module_vc_terminate      = 0;

/* this should be assigned in netmod init, not by assign_functions, since not
   all netmods will implement it */
MPID_nem_net_module_vc_dbg_print_sendq_t  MPID_nem_net_module_vc_dbg_print_sendq = 0;

#define assign_functions(prefix) do {                                                           \
    MPID_nem_net_module_init              = MPID_nem_##prefix##_module_init;                    \
    MPID_nem_net_module_finalize          = MPID_nem_##prefix##_module_finalize;                \
    MPID_nem_net_module_ckpt_shutdown     = MPID_nem_##prefix##_module_ckpt_shutdown;           \
    MPID_nem_net_module_poll              = MPID_nem_##prefix##_module_poll;                    \
    MPID_nem_net_module_send              = MPID_nem_##prefix##_module_send;                    \
    MPID_nem_net_module_get_business_card = MPID_nem_##prefix##_module_get_business_card;       \
    MPID_nem_net_module_connect_to_root   = MPID_nem_##prefix##_module_connect_to_root;         \
    MPID_nem_net_module_vc_init           = MPID_nem_##prefix##_module_vc_init;                 \
    MPID_nem_net_module_vc_destroy        = MPID_nem_##prefix##_module_vc_destroy;              \
    MPID_nem_net_module_vc_terminate      = MPID_nem_##prefix##_module_vc_terminate;            \
} while (0)

#undef FUNCNAME
#define FUNCNAME MPID_nem_net_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_net_init( void)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_NET_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_NET_INIT);

#if(MPID_NEM_NET_MODULE == MPID_NEM_ERROR_MODULE)
#error Error in definition of MPID_NEM_*_MODULE macros
#elif (MPID_NEM_NET_MODULE == MPID_NEM_GM_MODULE)
  {
      assign_functions (gm);
  }
#elif (MPID_NEM_NET_MODULE == MPID_NEM_MX_MODULE)
  {
      assign_functions (mx);
  }
#elif (MPID_NEM_NET_MODULE == MPID_NEM_TCP_MODULE)
  {
      assign_functions (tcp);
  }
#elif (MPID_NEM_NET_MODULE == MPID_NEM_ELAN_MODULE)
  {
      assign_functions (elan);
  }
#elif (MPID_NEM_NET_MODULE == MPID_NEM_NEWTCP_MODULE)
  {
      assign_functions (newtcp);
  }
#elif (MPID_NEM_NET_MODULE == MPID_NEM_NO_MODULE)
  {
      assign_functions (dummy);
  }
#elif (MPID_NEM_NET_MODULE == MPID_NEM_SCTP_MODULE)
  {
      assign_functions (sctp);
  }
#elif (MPID_NEM_NET_MODULE == MPID_NEM_IB_MODULE)
  {
      assign_functions (ib);
  }
#elif (MPID_NEM_NET_MODULE == MPID_NEM_PSM_MODULE)
  {
      assign_functions (psm);
  }
#else
#warning ">>>>>>>>>>>>>>>> WRONG NET MODULE INITIALIZATION"
#endif

  MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_NET_INIT);
  return mpi_errno;
}

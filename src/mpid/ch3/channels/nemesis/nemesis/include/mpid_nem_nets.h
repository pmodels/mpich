/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPID_NEM_NETS_H
#define MPID_NEM_NETS_H

typedef int (* MPID_nem_net_module_init_t)(MPIDI_PG_t *pg_p, int pg_rank,
                                           char **bc_val_p, int *val_max_sz_p);
typedef int (* MPID_nem_net_module_finalize_t)(void);
#ifdef ENABLE_CHECKPOINTING
typedef int (* MPID_nem_net_module_ckpt_precheck_t)(void);
typedef int (* MPID_nem_net_module_ckpt_restart_t)(void);
typedef int (* MPID_nem_net_module_ckpt_continue_t)(void);
#endif
typedef int (* MPID_nem_net_module_poll_t)(int in_blocking_poll);
typedef int (* MPID_nem_net_module_get_business_card_t)(int my_rank, char **bc_val_p, int *val_max_sz_p);
typedef int (* MPID_nem_net_module_connect_to_root_t)(const char *business_card, MPIDI_VC_t *new_vc);
typedef int (* MPID_nem_net_module_vc_init_t)(MPIDI_VC_t *vc);
typedef int (* MPID_nem_net_module_vc_destroy_t)(MPIDI_VC_t *vc);
typedef int (* MPID_nem_net_module_vc_terminate_t)(MPIDI_VC_t *vc);
typedef int (* MPID_nem_net_module_anysource_iprobe_t)(int tag, MPID_Comm *comm, int context_offset, int *flag,
                                                       MPI_Status *status);

typedef void (* MPID_nem_net_module_vc_dbg_print_sendq_t)(FILE *stream, MPIDI_VC_t *vc);

typedef struct MPID_nem_netmod_funcs
{
    MPID_nem_net_module_init_t init;
    MPID_nem_net_module_finalize_t finalize;
#ifdef ENABLE_CHECKPOINTING
    MPID_nem_net_module_ckpt_precheck_t ckpt_precheck;
    MPID_nem_net_module_ckpt_restart_t ckpt_restart;
    MPID_nem_net_module_ckpt_continue_t ckpt_continue;
#endif
    MPID_nem_net_module_poll_t poll;
    MPID_nem_net_module_get_business_card_t get_business_card;
    MPID_nem_net_module_connect_to_root_t connect_to_root;
    MPID_nem_net_module_vc_init_t vc_init;
    MPID_nem_net_module_vc_destroy_t vc_destroy;
    MPID_nem_net_module_vc_terminate_t vc_terminate;
    MPID_nem_net_module_anysource_iprobe_t anysource_iprobe;
} MPID_nem_netmod_funcs_t;

extern MPID_nem_net_module_vc_dbg_print_sendq_t  MPID_nem_net_module_vc_dbg_print_sendq;

/* table of all netmod functions */
extern MPID_nem_netmod_funcs_t *MPID_nem_netmod_funcs[];
/* netmod functions for the netmod being used */
extern MPID_nem_netmod_funcs_t *MPID_nem_netmod_func;
extern int MPID_nem_netmod_id;
extern int MPID_nem_num_netmods;
#define MPID_NEM_MAX_NETMOD_STRING_LEN 64
extern char MPID_nem_netmod_strings[][MPID_NEM_MAX_NETMOD_STRING_LEN];

int MPID_nem_net_init(void);

#endif /* MPID_NEM_NETS_H */

/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "smpd.h"

#undef FCNAME
#define FCNAME "smpd_get_ccp_nodes"
int smpd_get_ccp_nodes(int *np, smpd_host_node_t **host_node_ptr_p)
{
    smpd_host_node_t *host_node_ptr=NULL, **host_list_tail_p=NULL;
    int smpd_node_cnt = 0;
    char *p=NULL, *tok=NULL, *next_tok=NULL;
    char seps[] = " ,\t\n";
    
    smpd_enter_fn(FCNAME);
    if(np == NULL){
        smpd_err_printf("Error: Pointer to num procs is NULL\n");
        smpd_exit_fn(FCNAME);
        return SMPD_FAIL;
    }
    if(host_node_ptr_p == NULL){
        smpd_err_printf("Error: Invalid pointer to host node ptr\n");
        smpd_exit_fn(FCNAME);
        return SMPD_FAIL;
    }
    host_list_tail_p = host_node_ptr_p;
    
    *np = -1;

    /* Get the CCP nodes list */
    p = MPIU_Strdup(getenv("CCP_NODES"));
    if(p == NULL){
        smpd_err_printf("Error: Unable to get the list of CCP nodes\n");
        smpd_exit_fn(FCNAME);
        return SMPD_FAIL;
    }
    smpd_dbg_printf("CCP_NODES = %s\n", p);
    tok = strtok_s(p, seps, &next_tok);
    while(tok != NULL){
        /* Allocate memory & set node name */
        host_node_ptr = (smpd_host_node_t *)MPIU_Malloc(sizeof(smpd_host_node_t));
        if(host_node_ptr == NULL){
            smpd_err_printf("Unable to allocate memory for smpd host node \n");
            MPIU_Free(p);
            smpd_exit_fn(FCNAME);
            return SMPD_FAIL;
        }
        host_node_ptr->next = NULL;
        host_node_ptr->left = NULL;
        host_node_ptr->right = NULL;
        host_node_ptr->connected = SMPD_FALSE;
        host_node_ptr->connect_cmd_tag = -1;
        host_node_ptr->nproc = 1;
        host_node_ptr->alt_host[0] = '\0';

        MPIU_Strncpy(host_node_ptr->host, tok, SMPD_MAX_HOST_LENGTH);

        /* Add the node to the tail of the list */

        *host_list_tail_p = host_node_ptr;
        host_list_tail_p = &(host_node_ptr->next);

        smpd_node_cnt++;
        /* Next node name */
        tok = strtok_s(NULL, seps, &next_tok);
    }

    *np = smpd_node_cnt;
    MPIU_Free(p);
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

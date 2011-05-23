/*
 * Copyright (C) 2010. See COPYRIGHT in top-level directory.
 */

#ifndef HAVE_ARMCI_MSG_H
#define HAVE_ARMCI_MSG_H

#include <armci.h>

/** Note on scopes:
  *
  * SCOPE_NODE    - Include all processes on the current node.  In the current
  *                 implementation we use MPI_COMM_SELF for this.
  * SCOPE_MASTERS - Includes one rank from every node.  Currently the same as
                    SCOPE_ALL.
  * SCOPE_ALL     - Includes all processes.
  */
enum armci_scope_e { SCOPE_ALL, SCOPE_NODE, SCOPE_MASTERS}; 

enum armci_type_e  { ARMCI_INT, ARMCI_LONG, ARMCI_LONG_LONG, ARMCI_FLOAT, ARMCI_DOUBLE };

/* Utility routines */

int  armci_msg_me(void);
int  armci_msg_nproc(void);

void armci_msg_abort(int code);
double armci_timer(void);

/* Send/Recv */

void armci_msg_snd(int tag, void *buffer, int len, int to);
void armci_msg_rcv(int tag, void *buffer, int buflen, int *msglen, int from);
int  armci_msg_rcvany(int tag, void *buffer, int buflen, int *msglen); 

/* Assorted Collectives */

void armci_msg_barrier(void);
void armci_msg_group_barrier(ARMCI_Group *group);
void armci_msg_bintree(int scope, int *Root, int *Up, int *Left, int *Right);

void armci_msg_bcast(void *buffer, int len, int root);
void armci_msg_bcast_scope(int scope, void *buffer, int len, int root);
void armci_msg_brdcst(void *buffer, int len, int root);
void armci_msg_group_bcast_scope(int scope, void *buf, int len, int root, ARMCI_Group *group);

/* TODO */ void armci_msg_reduce(void *x, int n, char *op, int type); 
/* TODO */ void armci_msg_reduce_scope(int scope, void *x, int n, char *op, int type); 

/* TODO */ void armci_msg_sel(void *x, int n, char *op, int type, int contribute);
/* TODO */ void armci_msg_sel_scope(int scope, void *x, int n, char *op, int type, int contribute);

/* TODO */ void armci_exchange_address(void *ptr_ar[], int n);

/* TODO */ void armci_msg_clus_brdcst(void *buf, int len);
/* TODO */ void armci_msg_clus_igop(int *x, int n, char *op); 
/* TODO */ void armci_msg_clus_fgop(float *x, int n, char *op); 
/* TODO */ void armci_msg_clus_lgop(long *x, int n, char *op); 
/* TODO */ void armci_msg_clus_llgop(long long *x, int n, char *op); 
/* TODO */ void armci_msg_clus_dgop(double *x, int n, char *op); 

/* TODO */ void armci_exchange_address_grp(void *ptr_arr[], int n, ARMCI_Group *group);
/* TODO */ void armci_grp_clus_brdcst(void *buf, int len, int grp_master, int grp_clus_nproc,ARMCI_Group *mastergroup);

/* Global Operations / Reduction Operations */

void armci_msg_gop_scope(int scope, void *x, int n, char *op, int type);
void armci_msg_igop(int *x, int n, char *op);
void armci_msg_lgop(long *x, int n, char *op);
void armci_msg_llgop(long long *x, int n, char *op);
void armci_msg_fgop(float *x, int n, char *op);
void armci_msg_dgop(double *x, int n, char *op);

void armci_msg_group_gop_scope(int scope, void *x, int n, char *op, int type, ARMCI_Group *group);
void armci_msg_group_igop(int *x, int n, char *op, ARMCI_Group *group);
void armci_msg_group_lgop(long *x, int n, char *op, ARMCI_Group *group);
void armci_msg_group_llgop(long long *x, int n, char *op, ARMCI_Group *group);
void armci_msg_group_fgop(float *x, int n, char *op, ARMCI_Group *group);
void armci_msg_group_dgop(double *x, int n,char *op, ARMCI_Group *group);

#endif /* HAVE_ARMCI_MSG_H */

/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*  
 *  (C) 2005 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef RM_H_INCLUDED
#define RM_H_INCLUDED
typedef struct {
    char *hostname;        /* Name of the machine (used for ssh etc) */
    int  np;               /* Number of processes on this machine */
    char *login;           /* Login name to use (if different).
			      Q: do we also want to provide for a password? */
    char *netname;         /* Interface name to use (if different from host) */
    /* Other resource descriptions would go here, such as memory, 
       software, file systems */
} MachineDesc;

typedef struct {
    int nHosts; 
    MachineDesc *desc;
} MachineTable;

MachineTable *MPIE_ReadMachines( const char *, int, void * );
int MPIE_FreeMachineTable( MachineTable * );
int MPIE_ChooseHosts( ProcessWorld *, 
		      MachineTable* (*)(const char *, int, void *), 
		      void * );
int MPIE_RMProcessArg( int, char *[], void * );

#endif

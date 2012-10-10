/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef NAMEPUB_H_INCLUDED
#define NAMEPUB_H_INCLUDED

typedef struct MPID_NS_Handle *MPID_NS_Handle;

int MPID_NS_Create( const MPID_Info *, MPID_NS_Handle * );
int MPID_NS_Publish( MPID_NS_Handle, const MPID_Info *, 
                     const char service_name[], const char port[] );
int MPID_NS_Lookup( MPID_NS_Handle, const MPID_Info *,
                    const char service_name[], char port[] );
int MPID_NS_Unpublish( MPID_NS_Handle, const MPID_Info *, 
                       const char service_name[] );
int MPID_NS_Free( MPID_NS_Handle * );

extern MPID_NS_Handle MPIR_Namepub;

#endif

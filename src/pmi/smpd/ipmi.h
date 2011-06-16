/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef IPMI_H
#define IPMI_H

#include "pmiconf.h" 

#include <stdio.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_STDARG_H
#include <stdarg.h>
#endif
#ifdef HAVE_DLFCN_H
#include <dlfcn.h>
#endif

#include "pmi.h"
/*
#include "mpidu_sock.h"
*/
#include "smpd.h"
#include "mpi.h"

#define PMI_SUCCESS     0
#define PMI_FAIL       -1

/* prototypes for the iPMI interface */

#if defined(__cplusplus)
extern "C" {
#endif

/* PMI Group functions */
int iPMI_Init( int *spawned );           /* initialize PMI for this process group
                                           The value of spawned indicates whether this process
                                           was created by PMI_Spawn_multiple. */
int iPMI_Initialized( int *initialized );            /* Return true if PMI has been initialized */
int iPMI_Finalize( void );               /* finalize PMI for this process group */
int iPMI_Get_size( int *size );          /* get size of process group */
int iPMI_Get_rank( int *rank );          /* get rank in process group */
int iPMI_Get_universe_size( int *size ); /* get the maximum allowed spawnable resources */
int iPMI_Get_appnum( int *appnum );      /* get the application number */
int iPMI_Get_id( char id_str[], int length );         /* get a string to uniquely identify the process group */
int iPMI_Get_kvs_domain_id( char id_str[], int length );     /* get a string to uniquely identify the kvs domain */
int iPMI_Get_id_length_max( int *maxlen);      /* get the maximum length the id string can be. Must return >= 40 */
int iPMI_Barrier( void );                /* barrier across processes in process group */
int iPMI_Get_clique_size( int *size );   /* get the number of processes on my node */
int iPMI_Get_clique_ranks( int ranks[], int length ); /* get the ranks on my node */
int iPMI_Abort(int exit_code, const char error_msg[]); /* abort this process group */

/* PMI Keymap functions */
int iPMI_KVS_Get_my_name( char kvsname[], int length );       /* get name of keyval space */
int iPMI_KVS_Get_name_length_max( int *maxlen );        /* needed to communicate keyval space */
int iPMI_KVS_Get_key_length_max( int *maxlen );         /* contents to a foreign domain */
int iPMI_KVS_Get_value_length_max( int *maxlen );
int iPMI_KVS_Create( char kvsname[], int length );            /* make a new one, get name */
int iPMI_KVS_Destroy( const char kvsname[] );     /* finish with one */
int iPMI_KVS_Put( const char kvsname[], const char key[],
                const char value[]);             /* put data */
int iPMI_KVS_Commit( const char kvsname[] );      /* block until all pending put
                                                   operations from this process
                                                   are complete.  This is a process
                                                   local operation. */
int iPMI_KVS_Get( const char kvsname[],
                 const char key[], char value[], int length); /* get value associated with key */
int iPMI_KVS_Iter_first(const char kvsname[], char key[], int key_len, char val[], int val_len);  /* loop through the */
int iPMI_KVS_Iter_next(const char kvsname[], char key[], int key_len, char val[], int val_len);   /* pairs in the kvs */

/* PMI Process Creation functions */

int iPMI_Spawn_multiple(int count,
                       const char * cmds[],
                       const char ** argvs[],
                       const int maxprocs[],
                       const int info_keyval_sizes[],
                       const PMI_keyval_t * info_keyval_vectors[],
                       int preput_keyval_size,
                       const PMI_keyval_t preput_keyval_vector[],
                       int errors[]);

int iPMI_Parse_option(int num_args, char *args[], int *num_parsed, PMI_keyval_t **keyvalp, int *size);
/* parse PMI implementation specific values into an info object that can then be passed to 
   PMI_Spawn_multiple.  Remove PMI implementation specific arguments from argc and argv */
int iPMI_Args_to_keyval(int *argcp, char *((*argvp)[]), PMI_keyval_t **keyvalp, int *size);
/* free the data returned by iPMI_Parse_option or iPMI_Args_to_keyval */
int iPMI_Free_keyvals(PMI_keyval_t keyvalp[], int size);

/* External Interface Functions */

int iPMI_Publish_name( const char service_name[], const char port[] );
int iPMI_Unpublish_name( const char service_name[] );
int iPMI_Lookup_name( const char service_name[], char port[] );

typedef struct ipmi_functions_t
{
    int (*PMI_Init)( int * );
    int (*PMI_Initialized)( int *);
    int (*PMI_Finalize)( void );
    int (*PMI_Get_size)( int * );
    int (*PMI_Get_rank)( int * );
    int (*PMI_Get_universe_size)( int * );
    int (*PMI_Get_appnum)( int * );
    int (*PMI_Get_id)( char[] , int );
    int (*PMI_Get_kvs_domain_id)( char [], int );
    int (*PMI_Get_id_length_max)( int *);
    int (*PMI_Barrier)( void );
    int (*PMI_Get_clique_size)( int * );
    int (*PMI_Get_clique_ranks)( int [], int );
    int (*PMI_Abort)( int, const char [] );
    int (*PMI_KVS_Get_my_name)( char [], int );
    int (*PMI_KVS_Get_name_length_max)( int * );
    int (*PMI_KVS_Get_key_length_max)( int * );
    int (*PMI_KVS_Get_value_length_max)( int * );
    int (*PMI_KVS_Create)( char [], int );
    int (*PMI_KVS_Destroy)( const char [] );
    int (*PMI_KVS_Put)( const char [], const char [], const char [] );
    int (*PMI_KVS_Commit)( const char [] );
    int (*PMI_KVS_Get)( const char [], const char [], char [], int );
    int (*PMI_KVS_Iter_first)( const char [], char [], int, char [], int );
    int (*PMI_KVS_Iter_next)( const char [], char [], int , char [], int );
    int (*PMI_Spawn_multiple)( int, const char *[], const char **[], const int [], const int [], const PMI_keyval_t *[], int, const PMI_keyval_t [], int [] );
    int (*PMI_Parse_option)( int, char *[], int *, PMI_keyval_t **, int * );
    int (*PMI_Args_to_keyval)( int *, char *((*)[]), PMI_keyval_t **, int * );
    int (*PMI_Free_keyvals)( PMI_keyval_t [], int );
    int (*PMI_Publish_name)( const char [], const char [] );
    int (*PMI_Unpublish_name)( const char [] );
    int (*PMI_Lookup_name)( const char [], char [] );
} ipmi_functions_t;

#if defined(__cplusplus)
}
#endif

#endif

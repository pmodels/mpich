/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*  
 *  (C) 2004 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef ENV_H_INCLUDED
#define ENV_H_INCLUDED

/* Element of the list of environment variables */
typedef struct EnvData {
    const char *name, *value;
    const char *envvalue;     /* name=value form, for putenv */
    struct EnvData *nextData;
} EnvData;

typedef struct EnvInfo {
    int includeAll;      /* true if all environment variables should be 
			    included, false if none (other than those
			    explicitly listed below) should be included */
    EnvData *envPairs;   /* List of name,value pairs to be included */
    EnvData *envNames;   /* List of names to be included, using the 
			    current value in the environment */
} EnvInfo;

int MPIE_ArgsCheckForEnv( int, char *[], ProcessWorld *, EnvInfo ** );
int MPIE_EnvSetup( ProcessState *, char *[], char *[], int );
int MPIE_EnvInitData( EnvData *, int );
int MPIE_Putenv( ProcessWorld *, const char * );
int MPIE_UnsetAllEnv( char *[] );

#endif

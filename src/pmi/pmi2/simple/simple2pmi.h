/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2007 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef SIMPLE2PMI_H_INCLUDED
#define SIMPLE2PMI_H_INCLUDED

#include "mpichconf.h"


#define PMII_COMMANDLEN_SIZE 6
#define PMII_MAX_COMMAND_LEN (64*1024)

static const char FULLINIT_CMD[]          = "fullinit";
static const char FULLINITRESP_CMD[]      = "fullinit-response";
static const char FINALIZE_CMD[]          = "finalize";
static const char FINALIZERESP_CMD[]      = "finalize-response";
static const char ABORT_CMD[]             = "abort";
static const char JOBGETID_CMD[]          = "job-getid";
static const char JOBGETIDRESP_CMD[]      = "job-getid-response";
static const char JOBCONNECT_CMD[]        = "job-connect";
static const char JOBCONNECTRESP_CMD[]    = "job-connect-response";
static const char JOBDISCONNECT_CMD[]     = "job-disconnect";
static const char JOBDISCONNECTRESP_CMD[] = "job-disconnect-response";
static const char KVSPUT_CMD[]            = "kvs-put";
static const char KVSPUTRESP_CMD[]        = "kvs-put-response";
static const char KVSFENCE_CMD[]          = "kvs-fence";
static const char KVSFENCERESP_CMD[]      = "kvs-fence-response";
static const char KVSGET_CMD[]            = "kvs-get";
static const char KVSGETRESP_CMD[]        = "kvs-get-response";
static const char GETNODEATTR_CMD[]       = "info-getnodeattr";
static const char GETNODEATTRRESP_CMD[]   = "info-getnodeattr-response";
static const char PUTNODEATTR_CMD[]       = "info-putnodeattr";
static const char PUTNODEATTRRESP_CMD[]   = "info-putnodeattr-response";
static const char GETJOBATTR_CMD[]        = "info-getjobattr";
static const char GETJOBATTRRESP_CMD[]    = "info-getjobattr-response";
static const char NAMEPUBLISH_CMD[]       = "name-publish";
static const char NAMEPUBLISHRESP_CMD[]   = "name-publish-response";
static const char NAMEUNPUBLISH_CMD[]     = "name-unpublish";
static const char NAMEUNPUBLISHRESP_CMD[] = "name-unpublish-response";
static const char NAMELOOKUP_CMD[]        = "name-lookup";
static const char NAMELOOKUPRESP_CMD[]    = "name-lookup-response";


static const char PMIJOBID_KEY[]     = "pmijobid";
static const char PMIRANK_KEY[]      = "pmirank";
static const char SRCID_KEY[]        = "srcid";
static const char THREADED_KEY[]     = "threaded";
static const char RC_KEY[]           = "rc";
static const char ERRMSG_KEY[]       = "errmsg";
static const char PMIVERSION_KEY[]   = "pmi-version";
static const char PMISUBVER_KEY[]    = "pmi-subversion";
static const char RANK_KEY[]         = "rank";
static const char SIZE_KEY[]         = "size";
static const char APPNUM_KEY[]       = "appnum";
static const char SPAWNERJOBID_KEY[] = "spawner-jobid";
static const char DEBUGGED_KEY[]     = "debugged";
static const char PMIVERBOSE_KEY[]   = "pmiverbose";
static const char ISWORLD_KEY[]      = "isworld";
static const char MSG_KEY[]          = "msg";
static const char JOBID_KEY[]        = "jobid";
static const char KVSCOPY_KEY[]      = "kvscopy";
static const char KEY_KEY[]          = "key";
static const char VALUE_KEY[]        = "value";
static const char FOUND_KEY[]        = "found";
static const char WAIT_KEY[]         = "wait";
static const char NAME_KEY[]         = "name";
static const char PORT_KEY[]         = "port";
static const char THRID_KEY[]        = "thrid";
static const char INFOKEYCOUNT_KEY[] = "infokeycount";
static const char INFOKEY_KEY[]      = "infokey%d";
static const char INFOVAL_KEY[]      = "infoval%d";

static const char TRUE_VAL[]  = "TRUE";
static const char FALSE_VAL[] = "FALSE";


/* Local types */

/* Parse commands are in this structure.  Fields in this structure are
   dynamically allocated as necessary */
typedef struct PMI2_Keyvalpair {
    const char *key;
    const char *value;
    int         valueLen;  /* Length of a value (values may contain nulls, so
                              we need this) */
    int         isCopy;    /* The value is a copy (and will need to be freed)
                              if this is true, otherwise,
                              it is a null-terminated string in the original
                              buffer */
} PMI2_Keyvalpair;

typedef struct PMI2_Command {
    int               nPairs;   /* Number of key=value pairs */
    char             *command;  /* Overall command buffer */
    PMI2_Keyvalpair **pairs;    /* Array of pointers to pairs */
    int               complete;
} PMI2_Command;

/* Debug value */
extern int MPIE_Debug;

/* For mpiexec, we should normally enable debugging.  Comment out this
   definition of HAVE_DEBUGGING if you want to leave these out of the code */
#define HAVE_DEBUGGING

#ifdef HAVE_DEBUGGING
#define DBG_COND(cond,stmt) {if (cond) { stmt;}}
#else
#define DBG_COND(cond,a)
#endif
#define DBG(stmt) DBG_COND(MPIE_Debug,stmt)
#define DBG_PRINTFCOND(cond,a)  DBG_COND(cond,printf a ; fflush(stdout))
#define DBG_EPRINTFCOND(cond,a) DBG_COND(cond,fprintf a ; fflush(stderr))
#define DBG_EPRINTF(a) DBG_EPRINTFCOND(MPIE_Debug,a)
#define DBG_PRINTF(a)  DBG_PRINTFCOND(MPIE_Debug,a)




#endif

/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"

/* ------------------------------------------------------------------------ */
/* Utilities related to test environment */

/* Some tests would like to test with large buffer, but an arbitary size may fail
   on machines that does not have sufficient memory or the OS may not support it.
   This routine provides a portable interface to get that max size.
   It is taking the simpliest solution here: default to 2GB, unless user set via
   environment variable -- MPITEST_MAXBUFFER
*/
MPI_Aint MTestDefaultMaxBufferSize()
{
    MPI_Aint max_size = 1073741824;
    char *envval = NULL;
    envval = getenv("MPITEST_MAXBUFFER");
    if (envval) {
        max_size = atol(envval);
    }
    return max_size;
}

/* ------------------------------------------------------------------------ */
/* Utilities to parse command line options */

/* Parses argument in the form of: -arg1=value1 -arg2=value2 ...
   Arguments can be supplied in any order, but missing argument will cause error.
*/
typedef struct MTestArgListEntry {
    char *arg;
    char *val;
    struct MTestArgListEntry *next;
} MTestArgListEntry;

static void MTestArgListInsert(MTestArgListEntry ** head, char *arg, char *val)
{
    MTestArgListEntry *tmp = *head;

    if (!tmp) {
        tmp = malloc(sizeof(MTestArgListEntry));
        tmp->arg = arg;
        tmp->val = val;
        tmp->next = NULL;
        *head = tmp;
        return;
    }

    while (tmp->next)
        tmp = tmp->next;

    tmp->next = malloc(sizeof(MTestArgListEntry));
    tmp->next->arg = arg;
    tmp->next->val = val;
    tmp->next->next = NULL;
}

static char *MTestArgListSearch(MTestArgListEntry * head, const char *arg)
{
    char *val = NULL;

    while (head && strcmp(head->arg, arg))
        head = head->next;

    if (head)
        val = head->val;

    return val;
}

static void MTestArgListPrintError(const char *arg)
{
    fprintf(stderr, "Error: argument -%s= has not been defined!\n", arg);
    exit(-1);
}

void MTestArgListDestroy(MTestArgList * head)
{
    MTestArgListEntry *cur = (MTestArgListEntry *) head;

    while (cur) {
        MTestArgListEntry *prev = cur;
        cur = cur->next;
        free(prev->arg);
        free(prev->val);
        free(prev);
    }
}

/*
 * following args are expected to be of the form: -arg=val
 */
MTestArgList *MTestArgListCreate(int argc, char *argv[])
{
    int i;
    char *string = NULL;
    char *tmp = NULL;
    char *arg = NULL;
    char *val = NULL;

    MTestArgListEntry *head = NULL;

    for (i = 1; i < argc; i++) {
        /* extract arg and val */
        string = strdup(argv[i]);
        tmp = strtok(string, "=");
        arg = strdup(tmp + 1);  /* skip prepending '-' */
        tmp = strtok(NULL, "=");
        val = strdup(tmp);

        MTestArgListInsert(&head, arg, val);

        free(string);
    }

    return head;
}

char *MTestArgListGetString(MTestArgList * head, const char *arg)
{
    char *tmp;

    if (!(tmp = MTestArgListSearch((MTestArgListEntry *) head, arg)))
        MTestArgListPrintError(arg);

    return tmp;
}

int MTestArgListGetInt(MTestArgList * head, const char *arg)
{
    return atoi(MTestArgListGetString(head, arg));
}

long MTestArgListGetLong(MTestArgList * head, const char *arg)
{
    return atol(MTestArgListGetString(head, arg));
}

int MTestIsBasicDtype(MPI_Datatype type)
{
    int numints, numaddrs, numtypes, combiner;
    MPI_Type_get_envelope(type, &numints, &numaddrs, &numtypes, &combiner);

    int is_basic = (combiner == MPI_COMBINER_NAMED);

    return is_basic;
}

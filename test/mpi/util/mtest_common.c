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

const char *MTestArgListGetString_with_default(MTestArgList * head, const char *arg,
                                               const char *default_str)
{
    char *tmp;

    if (!(tmp = MTestArgListSearch((MTestArgListEntry *) head, arg)))
        return default_str;

    return tmp;
}

int MTestArgListGetInt(MTestArgList * head, const char *arg)
{
    return atoi(MTestArgListGetString(head, arg));
}

int MTestArgListGetInt_with_default(MTestArgList * head, const char *arg, int default_val)
{
    const char *tmp = MTestArgListGetString_with_default(head, arg, NULL);
    if (tmp)
        return atoi(tmp);
    else
        return default_val;
}

long MTestArgListGetLong(MTestArgList * head, const char *arg)
{
    return atol(MTestArgListGetString(head, arg));
}

long MTestArgListGetLong_with_default(MTestArgList * head, const char *arg, long default_val)
{
    const char *tmp = MTestArgListGetString_with_default(head, arg, NULL);
    if (tmp)
        return atol(tmp);
    else
        return default_val;
}

mtest_mem_type_e MTestArgListGetMemType(MTestArgList * head, const char *arg)
{
    const char *memtype = MTestArgListGetString_with_default(head, arg, NULL);
    if (!memtype || strcmp(memtype, "host") == 0)
        return MTEST_MEM_TYPE__UNREGISTERED_HOST;
    else if (strcmp(memtype, "reg_host") == 0)
        return MTEST_MEM_TYPE__REGISTERED_HOST;
    else if (strcmp(memtype, "device") == 0)
        return MTEST_MEM_TYPE__DEVICE;
    else
        return MTEST_MEM_TYPE__UNSET;
}

int MTestIsBasicDtype(MPI_Datatype type)
{
    int numints, numaddrs, numtypes, combiner;
    MPI_Type_get_envelope(type, &numints, &numaddrs, &numtypes, &combiner);

    int is_basic = (combiner == MPI_COMBINER_NAMED);

    return is_basic;
}

/* ------------------------------------------------------------------------ */
/* Utilities to support device memory allocation */
#ifdef HAVE_CUDA
#include <cuda_runtime_api.h>
#include <assert.h>
int ndevices = -1;
int device_id = -1;
#endif

/* allocates memory of specified type */
void MTestAlloc(size_t size, mtest_mem_type_e type, void **hostbuf, void **devicebuf)
{
#ifdef HAVE_CUDA
    if (device_id == -1) {
        cudaGetDeviceCount(&ndevices);
        assert(ndevices != -1);
        if (ndevices > 1)
            /*
             * set initial device id to 1 to simulate the case where
             * the user is trying to move data from a device that is
             * not the "current" device
             */
            device_id = 1;
        else
            device_id = 0;
    }
#endif

    if (type == MTEST_MEM_TYPE__UNREGISTERED_HOST) {
        *devicebuf = malloc(size);
        if (hostbuf)
            *hostbuf = *devicebuf;
#ifdef HAVE_CUDA
    } else if (type == MTEST_MEM_TYPE__REGISTERED_HOST) {
        cudaMallocHost(devicebuf, size);
        if (hostbuf)
            *hostbuf = *devicebuf;
    } else if (type == MTEST_MEM_TYPE__DEVICE) {
        cudaSetDevice(device_id);
        cudaMalloc(devicebuf, size);
        if (hostbuf)
            cudaMallocHost(hostbuf, size);
        device_id++;
        device_id %= ndevices;
#endif
    } else {
        fprintf(stderr, "ERROR: unsupported memory type %d\n", type);
        exit(1);
    }
}

void MTestFree(mtest_mem_type_e type, void *hostbuf, void *devicebuf)
{
    if (type == MTEST_MEM_TYPE__UNREGISTERED_HOST) {
        free(hostbuf);
#ifdef HAVE_CUDA
    } else if (type == MTEST_MEM_TYPE__REGISTERED_HOST) {
        cudaFreeHost(devicebuf);
    } else if (type == MTEST_MEM_TYPE__DEVICE) {
        cudaFree(devicebuf);
        if (hostbuf) {
            cudaFreeHost(hostbuf);
        }
#endif
    }
}

void MTestCopyContent(const void *sbuf, void *dbuf, size_t size, mtest_mem_type_e type)
{
#ifdef HAVE_CUDA
    if (type == MTEST_MEM_TYPE__DEVICE) {
        cudaMemcpy(dbuf, sbuf, size, cudaMemcpyDefault);
    }
#endif
}

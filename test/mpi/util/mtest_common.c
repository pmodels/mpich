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
#if defined(HAVE_CUDA) || defined(HAVE_ZE)
#include <assert.h>
#ifdef HAVE_CUDA
#include <cuda_runtime_api.h>
#endif
int ndevices = -1;
int device_id = -1;
#ifdef HAVE_ZE
#include "level_zero/ze_api.h"
ze_driver_handle_t driver = NULL;
ze_device_handle_t *device = NULL;
ze_result_t zerr = ZE_RESULT_SUCCESS;
ze_command_list_handle_t command_list = NULL;
ze_event_pool_handle_t event_pool = NULL;
ze_event_handle_t event = NULL;
#endif
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

#ifdef HAVE_ZE
    if (device_id == -1) {
        /* Initilize ZE driver and device only at first call. */
        device_id = 0;
        zerr = zeInit(0);
        assert(zerr == ZE_RESULT_SUCCESS);

        /* Get driver for Intel GPUs by first discovering all the drivers,
         * and then picks the first driver that supports GPU devices */
        uint32_t num_drivers = 0;
        zerr = zeDriverGet(&num_drivers, NULL);
        assert(zerr == ZE_RESULT_SUCCESS);

        ze_driver_handle_t *all_drivers =
            (ze_driver_handle_t *) malloc(num_drivers * sizeof(ze_driver_handle_t));
        zerr = zeDriverGet(&num_drivers, all_drivers);
        assert(zerr == ZE_RESULT_SUCCESS);

        /* Find a driver for GPU device, and get handles to each of the
         * available devices */
        for (int i = 0; i < num_drivers; i++) {
            uint32_t num_devices = 0;
            zerr = zeDeviceGet(all_drivers[i], &num_devices, NULL);
            assert(zerr == ZE_RESULT_SUCCESS);

            ze_device_handle_t *all_devices =
                (ze_device_handle_t *) malloc(num_devices * sizeof(ze_device_handle_t));
            zerr = zeDeviceGet(all_drivers[i], &num_devices, all_devices);
            assert(zerr == ZE_RESULT_SUCCESS);

            for (int j = 0; j < num_devices; j++) {
                ze_device_properties_t device_properties;
                zerr = zeDeviceGetProperties(all_devices[j], &device_properties);
                assert(zerr == ZE_RESULT_SUCCESS);

                if (ZE_DEVICE_TYPE_GPU == device_properties.type) {
                    driver = all_drivers[i];
                    ndevices = num_devices;
                    device = all_devices;
                    break;
                }
            }

            if (NULL != driver) {
                break;
            } else {
                free(all_devices);
            }
        }

        free(all_drivers);

        /* Create command list, command queue, event pool and event, for device 0 only. */
        ze_command_queue_desc_t descriptor;
        descriptor.version = ZE_COMMAND_QUEUE_DESC_VERSION_CURRENT;
        descriptor.flags = 0;
        descriptor.mode = ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS;
        descriptor.priority = ZE_COMMAND_QUEUE_PRIORITY_NORMAL;
        descriptor.ordinal = 0; /* computeQueueGroupOrdinal */

        zerr = zeCommandListCreateImmediate(device[0], &descriptor, &command_list);
        assert(zerr == ZE_RESULT_SUCCESS);

        /* Create event pool and event */
        ze_event_pool_desc_t pool_desc;
        pool_desc.version = ZE_EVENT_POOL_DESC_VERSION_CURRENT;
        pool_desc.flags = 0;
        pool_desc.count = 1;

        zerr = zeEventPoolCreate(driver, &pool_desc, 1, &(device[0]), &event_pool);
        assert(zerr == ZE_RESULT_SUCCESS);

        ze_event_desc_t event_desc = {
            ZE_EVENT_DESC_VERSION_CURRENT,
            0,
            ZE_EVENT_SCOPE_FLAG_NONE,
            ZE_EVENT_SCOPE_FLAG_HOST
        };
        zerr = zeEventCreate(event_pool, &event_desc, &event);
        assert(zerr == ZE_RESULT_SUCCESS);
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

#ifdef HAVE_ZE
    } else if (type == MTEST_MEM_TYPE__REGISTERED_HOST) {
        size_t mem_alignment;
        ze_host_mem_alloc_desc_t host_desc;
        host_desc.flags = ZE_HOST_MEM_ALLOC_FLAG_DEFAULT;
        host_desc.version = ZE_HOST_MEM_ALLOC_DESC_VERSION_CURRENT;

        /* Currently ZE ignores this augument and uses an internal alignment
         * value. However, this behavior can change in the future. */
        mem_alignment = 1;
        zerr = zeDriverAllocHostMem(driver, &host_desc, size, mem_alignment, devicebuf);
        assert(zerr == ZE_RESULT_SUCCESS);

        if (hostbuf)
            *hostbuf = *devicebuf;
    } else if (type == MTEST_MEM_TYPE__DEVICE) {
        size_t mem_alignment;
        ze_device_mem_alloc_desc_t device_desc;
        device_desc.flags = ZE_DEVICE_MEM_ALLOC_FLAG_DEFAULT;
        device_desc.ordinal = 0;        /* We currently support a single memory type */
        device_desc.version = ZE_DEVICE_MEM_ALLOC_DESC_VERSION_CURRENT;
        /* Currently ZE ignores this augument and uses an internal alignment
         * value. However, this behavior can change in the future. */
        mem_alignment = 1;
        zerr =
            zeDriverAllocDeviceMem(driver, &device_desc, size, mem_alignment, device[device_id],
                                   devicebuf);
        assert(zerr == ZE_RESULT_SUCCESS);

        if (hostbuf) {
            ze_host_mem_alloc_desc_t host_desc;
            host_desc.flags = ZE_HOST_MEM_ALLOC_FLAG_DEFAULT;
            host_desc.version = ZE_HOST_MEM_ALLOC_DESC_VERSION_CURRENT;
            zerr = zeDriverAllocHostMem(driver, &host_desc, size, mem_alignment, hostbuf);
            assert(zerr == ZE_RESULT_SUCCESS);
        }
        /* FIXME: currently ZE only support device 0, so we cannot change device during test.
         * Shifting device_id similar to CUDA should be added in future. */
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
#ifdef HAVE_ZE
    } else if (type == MTEST_MEM_TYPE__REGISTERED_HOST) {
        zerr = zeDriverFreeMem(driver, devicebuf);
        assert(zerr == ZE_RESULT_SUCCESS);
    } else if (type == MTEST_MEM_TYPE__DEVICE) {
        zerr = zeDriverFreeMem(driver, devicebuf);
        assert(zerr == ZE_RESULT_SUCCESS);
        if (hostbuf) {
            zerr = zeDriverFreeMem(driver, hostbuf);
            assert(zerr == ZE_RESULT_SUCCESS);
        }
#endif
    }
}

void MTestCopyContent(const void *sbuf, void *dbuf, size_t size, mtest_mem_type_e type)
{
    if (type == MTEST_MEM_TYPE__DEVICE) {
#ifdef HAVE_CUDA
        cudaMemcpy(dbuf, sbuf, size, cudaMemcpyDefault);
#endif
#ifdef HAVE_ZE
        zerr = zeCommandListReset(command_list);
        assert(zerr == ZE_RESULT_SUCCESS);
        zerr = zeCommandListAppendMemoryCopy(command_list, dbuf, sbuf, size, NULL);
        assert(zerr == ZE_RESULT_SUCCESS);
        zerr = zeCommandListAppendSignalEvent(command_list, event);
        assert(zerr == ZE_RESULT_SUCCESS);
        zerr = zeEventHostSynchronize(event, UINT32_MAX);
        assert(zerr == ZE_RESULT_SUCCESS);
#endif
    }
}

void MTest_finalize_gpu()
{
#ifdef HAVE_ZE
    if (device_id != -1) {
        /* Free GPU resource */
        free(device);
        zerr = zeEventDestroy(event);
        assert(zerr == ZE_RESULT_SUCCESS);
        zerr = zeEventPoolDestroy(event_pool);
        assert(zerr == ZE_RESULT_SUCCESS);
        zeCommandListDestroy(command_list);
        assert(zerr == ZE_RESULT_SUCCESS);
    }
#endif
}

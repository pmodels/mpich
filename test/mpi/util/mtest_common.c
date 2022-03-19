/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"
#include "mtest_common.h"
#include <assert.h>
#include <string.h>

/* ------------------------------------------------------------------------ */
/* Utilities related to test environment */

/* Some tests would like to test with large buffer, but an arbitrary size may fail
   on machines that does not have sufficient memory or the OS may not support it.
   This routine provides a portable interface to get that max size.
   It is taking the simplest solution here: default to 2GB, unless user set via
   environment variable -- MPITEST_MAXBUFFER
*/
MPI_Aint MTestDefaultMaxBufferSize(void)
{
    MPI_Aint max_size = 1073741824;
    if (sizeof(void *) == 4) {
        /* 32-bit is very easy to overflow, which may still result in a
         * seemingly valid size. Use a smaller maximum to reduce the chance
         * -- an overflow integer is likely to be negative or very large. */
        max_size = 268435456;
    }
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

char *MTestArgListSearch(MTestArgList * dummy_head, const char *arg)
{
    MTestArgListEntry *head = dummy_head;
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
        assert(argv[i][0] == '-' && argv[i][1] != '-');
        string = strdup(argv[i]);
        tmp = strtok(string, "=");
        arg = strdup(tmp + 1);  /* skip prepending '-' */
        tmp = strtok(NULL, "=");
        assert(tmp != NULL);
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
    if (!memtype || strcmp(memtype, "host") == 0) {
        return MTEST_MEM_TYPE__UNREGISTERED_HOST;
    } else if (strcmp(memtype, "reg_host") == 0) {
        return MTEST_MEM_TYPE__REGISTERED_HOST;
    } else if (strcmp(memtype, "device") == 0) {
        return MTEST_MEM_TYPE__DEVICE;
    } else if (strcmp(memtype, "shared") == 0) {
        return MTEST_MEM_TYPE__SHARED;
    } else if (strcmp(memtype, "random") == 0) {
        return MTEST_MEM_TYPE__RANDOM;
    } else if (strcmp(memtype, "all") == 0) {
        return MTEST_MEM_TYPE__ALL;
    } else {
        return MTEST_MEM_TYPE__UNSET;
    }
}

mtest_mem_type_e MTest_memtype_random(void)
{
    /* roll our own rand() since we need prevent interfering with dtpools random pool.
     * Using the code from libc man page */
    static unsigned long number = 1;
    number = number * 1103515245 + 12345;
    return number / 65536 % 4 + MTEST_MEM_TYPE__UNREGISTERED_HOST;
}

const char *MTest_memtype_name(mtest_mem_type_e memtype)
{
    if (memtype == MTEST_MEM_TYPE__UNREGISTERED_HOST) {
        return "host";
    } else if (memtype == MTEST_MEM_TYPE__REGISTERED_HOST) {
        return "reg_host";
    } else if (memtype == MTEST_MEM_TYPE__DEVICE) {
        return "device";
    } else if (memtype == MTEST_MEM_TYPE__SHARED) {
        return "shared";
    } else if (memtype == MTEST_MEM_TYPE__RANDOM) {
        return "random";
    } else {
        return "INVALID";
    }
}

int MTestIsBasicDtype(MPI_Datatype type)
{
    int numints, numaddrs, numtypes, combiner;
    MPI_Type_get_envelope(type, &numints, &numaddrs, &numtypes, &combiner);

    int is_basic = (combiner == MPI_COMBINER_NAMED);

    return is_basic;
}

/* parse comma-separated integer string, return the integer array and
 * set num in output. User is responsible freeing the memory.
 */
int *MTestParseIntList(const char *str, int *num)
{
    int i = 0;
    for (const char *s = str; *s; s++) {
        if (*s == ',') {
            i++;
        }
    }
    *num = i + 1;

    int *ints = malloc((*num) * sizeof(int));

    i = 0;
    ints[i] = atoi(str);
    for (const char *s = str; *s; s++) {
        if (*s == ',') {
            i++;
            ints[i] = atoi(s + 1);
        }
    }
    return ints;
}

/* parse comma-separated string list, return the string array and
 * set num in output. Call MTestFreeStringList to free the returned
 * string list.
 */
char **MTestParseStringList(const char *str, int *num)
{
    int i, j;

    i = 0;
    for (const char *s = str; *s; s++) {
        if (*s == ',') {
            i++;
        }
    }
    *num = i + 1;

    char **strlist = malloc((*num) * sizeof(char *));

    i = 0;      /* index to strlist */
    j = 0;      /* index within the str */
    const char *s = str;
    while (true) {
        if (*s == '\0' || *s == ',') {
            /* previous str has j chars */
            strlist[i] = malloc(j + 1);
            strncpy(strlist[i], s - j, j);
            strlist[i][j] = '\0';
            i++;
            j = 0;
        } else {
            j++;
        }

        if (!*s) {
            break;
        } else {
            s++;
        }
    }

    return strlist;
}

void MTestFreeStringList(char **strlist, int num)
{
    for (int i = 0; i < num; i++) {
        free(strlist[i]);
    }
    free(strlist);
}

/* ------------------------------------------------------------------------ */
/* Utilities to support device memory allocation */
#if defined(HAVE_CUDA) || defined(HAVE_ZE) || defined(HAVE_HIP)
#include <assert.h>
#ifdef HAVE_CUDA
#include <cuda_runtime_api.h>
#endif
#ifdef HAVE_HIP
#include <hip/hip_runtime_api.h>
#endif
int ndevices = -1;
#ifdef HAVE_ZE
#include "level_zero/ze_api.h"
ze_driver_handle_t driver = NULL;
ze_context_handle_t context = NULL;
ze_device_handle_t *device = NULL;
ze_result_t zerr = ZE_RESULT_SUCCESS;
ze_command_list_handle_t *command_lists = NULL;
ze_event_pool_handle_t *event_pools = NULL;
#endif
#endif

/* allocates memory of specified type */
void MTestAlloc(size_t size, mtest_mem_type_e type, void **hostbuf, void **devicebuf,
                bool is_calloc, int device_id)
{

#ifdef HAVE_CUDA
    if (ndevices == -1) {
        cudaGetDeviceCount(&ndevices);
        assert(ndevices != -1);
    }
#endif

#ifdef HAVE_HIP
    if (ndevices == -1) {
        hipGetDeviceCount(&ndevices);
        assert(ndevices != -1);
    }
#endif

#ifdef HAVE_ZE
    if (ndevices == -1) {
        /* Initialize ZE driver and device only at first call. */
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
                device_properties.stype = ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES;
                device_properties.pNext = NULL;
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

        ze_context_desc_t contextDesc = {
            .stype = ZE_STRUCTURE_TYPE_CONTEXT_DESC,
            .pNext = NULL,
            .flags = 0,
        };
        zerr = zeContextCreate(driver, &contextDesc, &context);
        assert(zerr == ZE_RESULT_SUCCESS);

        /* Create command list, command queue, event pool and event, for device 0 only. */
        ze_command_queue_desc_t descriptor;
        descriptor.stype = ZE_STRUCTURE_TYPE_COMMAND_QUEUE_DESC;
        descriptor.pNext = NULL;
        descriptor.flags = 0;
        descriptor.index = 0;
        descriptor.mode = ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS;
        descriptor.priority = ZE_COMMAND_QUEUE_PRIORITY_NORMAL;

        uint32_t numQueueGroups = 0;
        zerr = zeDeviceGetCommandQueueGroupProperties(device[0], &numQueueGroups, NULL);
        assert(zerr == ZE_RESULT_SUCCESS && numQueueGroups);
        ze_command_queue_group_properties_t *queueProperties =
            (ze_command_queue_group_properties_t *)
            malloc(sizeof(ze_command_queue_group_properties_t) * numQueueGroups);
        for (int i = 0; i < numQueueGroups; i++) {
            queueProperties[i].stype = ZE_STRUCTURE_TYPE_COMMAND_QUEUE_GROUP_PROPERTIES;
            queueProperties[i].pNext = NULL;
        }
        zerr = zeDeviceGetCommandQueueGroupProperties(device[0], &numQueueGroups, queueProperties);
        descriptor.ordinal = -1;
        for (int i = 0; i < numQueueGroups; i++) {
            if (queueProperties[i].flags & ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COMPUTE) {
                descriptor.ordinal = i;
                break;
            }
        }
        assert(descriptor.ordinal != -1);

        command_lists =
            (ze_command_list_handle_t *) malloc(sizeof(ze_command_list_handle_t) * ndevices);
        for (int i = 0; i < ndevices; i++) {
            zerr = zeCommandListCreateImmediate(context, device[i], &descriptor, &command_lists[i]);
            assert(zerr == ZE_RESULT_SUCCESS);
        }

        /* Create event pool and event */
        ze_event_pool_desc_t pool_desc;
        pool_desc.stype = ZE_STRUCTURE_TYPE_EVENT_POOL_DESC;
        pool_desc.pNext = NULL;
        pool_desc.flags = 0;
        pool_desc.count = 1;

        event_pools = (ze_event_pool_handle_t *) malloc(sizeof(ze_event_pool_handle_t) * ndevices);
        for (int i = 0; i < ndevices; i++) {
            zerr = zeEventPoolCreate(context, &pool_desc, 1, &(device[i]), &event_pools[i]);
            assert(zerr == ZE_RESULT_SUCCESS);
        }
    }
#endif

    if (type == MTEST_MEM_TYPE__UNREGISTERED_HOST) {
        if (is_calloc)
            *devicebuf = calloc(size, 1);
        else
            *devicebuf = malloc(size);
        if (hostbuf)
            *hostbuf = *devicebuf;
#ifdef HAVE_CUDA
    } else if (type == MTEST_MEM_TYPE__REGISTERED_HOST) {
        cudaMallocHost(devicebuf, size);
        if (is_calloc)
            memset(*devicebuf, 0, size);
        if (hostbuf)
            *hostbuf = *devicebuf;
    } else if (type == MTEST_MEM_TYPE__DEVICE) {
        cudaSetDevice(device_id % ndevices);
        cudaMalloc(devicebuf, size);
        if (hostbuf) {
            cudaMallocHost(hostbuf, size);
            if (is_calloc)
                memset(*hostbuf, 0, size);
        }
    } else if (type == MTEST_MEM_TYPE__SHARED) {
        cudaSetDevice(device_id % ndevices);
        cudaMallocManaged(devicebuf, size, cudaMemAttachGlobal);
        if (hostbuf)
            *hostbuf = *devicebuf;
#endif

#ifdef HAVE_HIP
    } else if (type == MTEST_MEM_TYPE__REGISTERED_HOST) {
        hipHostMalloc(devicebuf, size, hipHostMallocDefault);
        if (is_calloc)
            memset(*devicebuf, 0, size);
        if (hostbuf)
            *hostbuf = *devicebuf;
    } else if (type == MTEST_MEM_TYPE__DEVICE) {
        hipSetDevice(device_id % ndevices);
        hipMalloc(devicebuf, size);
        if (hostbuf) {
            hipHostMalloc(hostbuf, size, hipHostMallocDefault);
            if (is_calloc)
                memset(*hostbuf, 0, size);
        }
    } else if (type == MTEST_MEM_TYPE__SHARED) {
        hipSetDevice(device_id % ndevices);
        hipMallocManaged(devicebuf, size, hipMemAttachGlobal);
        if (hostbuf)
            *hostbuf = *devicebuf;
#endif

#ifdef HAVE_ZE
    } else if (type == MTEST_MEM_TYPE__REGISTERED_HOST) {
        size_t mem_alignment;
        ze_host_mem_alloc_desc_t host_desc;
        host_desc.stype = ZE_STRUCTURE_TYPE_HOST_MEM_ALLOC_DESC;
        host_desc.pNext = NULL;
        host_desc.flags = 0;

        /* Currently ZE ignores this argument and uses an internal alignment
         * value. However, this behavior can change in the future. */
        mem_alignment = 1;
        zerr = zeMemAllocHost(context, &host_desc, size, mem_alignment, devicebuf);
        assert(zerr == ZE_RESULT_SUCCESS);
        if (is_calloc)
            memset(*devicebuf, 0, size);

        if (hostbuf)
            *hostbuf = *devicebuf;
    } else if (type == MTEST_MEM_TYPE__DEVICE) {
        size_t mem_alignment;
        ze_device_mem_alloc_desc_t device_desc;
        device_desc.stype = ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC;
        device_desc.pNext = NULL;
        device_desc.flags = 0;
        device_desc.ordinal = 0;        /* We currently support a single memory type */
        /* Currently ZE ignores this argument and uses an internal alignment
         * value. However, this behavior can change in the future. */
        mem_alignment = 1;
        zerr = zeMemAllocDevice(context, &device_desc, size, mem_alignment,
                                device[device_id % ndevices], devicebuf);
        assert(zerr == ZE_RESULT_SUCCESS);

        if (hostbuf) {
            ze_host_mem_alloc_desc_t host_desc;
            host_desc.stype = ZE_STRUCTURE_TYPE_HOST_MEM_ALLOC_DESC;
            host_desc.pNext = NULL;
            host_desc.flags = 0;
            zerr = zeMemAllocHost(context, &host_desc, size, mem_alignment, hostbuf);
            assert(zerr == ZE_RESULT_SUCCESS);
            if (is_calloc)
                memset(*hostbuf, 0, size);
        }
    } else if (type == MTEST_MEM_TYPE__SHARED) {
        size_t mem_alignment;
        ze_device_mem_alloc_desc_t device_desc;
        ze_host_mem_alloc_desc_t host_desc;
        device_desc.stype = ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC;
        device_desc.pNext = NULL;
        device_desc.flags = 0;
        device_desc.ordinal = 0;        /* We currently support a single memory type */
        host_desc.stype = ZE_STRUCTURE_TYPE_HOST_MEM_ALLOC_DESC;
        host_desc.pNext = NULL;
        host_desc.flags = 0;
        /* Currently ZE ignores this argument and uses an internal alignment
         * value. However, this behavior can change in the future. */
        mem_alignment = 1;
        zerr =
            zeMemAllocShared(context, &device_desc, &host_desc, size, mem_alignment,
                             device[device_id % ndevices], devicebuf);
        assert(zerr == ZE_RESULT_SUCCESS);
        if (hostbuf)
            *hostbuf = *devicebuf;
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
    } else if (type == MTEST_MEM_TYPE__SHARED) {
        cudaFree(devicebuf);
#endif
#ifdef HAVE_HIP
    } else if (type == MTEST_MEM_TYPE__REGISTERED_HOST) {
        hipHostFree(devicebuf);
    } else if (type == MTEST_MEM_TYPE__DEVICE) {
        hipFree(devicebuf);
        if (hostbuf) {
            hipHostFree(hostbuf);
        }
    } else if (type == MTEST_MEM_TYPE__SHARED) {
        hipFree(devicebuf);
#endif
#ifdef HAVE_ZE
    } else if (type == MTEST_MEM_TYPE__REGISTERED_HOST) {
        zerr = zeMemFree(context, devicebuf);
        assert(zerr == ZE_RESULT_SUCCESS);
    } else if (type == MTEST_MEM_TYPE__DEVICE) {
        zerr = zeMemFree(context, devicebuf);
        assert(zerr == ZE_RESULT_SUCCESS);
        if (hostbuf) {
            zerr = zeMemFree(context, hostbuf);
            assert(zerr == ZE_RESULT_SUCCESS);
        }
    } else if (type == MTEST_MEM_TYPE__SHARED) {
        zerr = zeMemFree(context, devicebuf);
        assert(zerr == ZE_RESULT_SUCCESS);
#endif
    }
}

void MTestCopyContent(const void *sbuf, void *dbuf, size_t size, mtest_mem_type_e type)
{
    if (type == MTEST_MEM_TYPE__DEVICE) {
#ifdef HAVE_CUDA
        cudaMemcpy(dbuf, sbuf, size, cudaMemcpyDefault);
#endif
#ifdef HAVE_HIP
        hipMemcpy(dbuf, sbuf, size, hipMemcpyDefault);
#endif
#ifdef HAVE_ZE
        int dev_id = -1, s_dev_id = -1, d_dev_id = -1;
        struct {
            ze_memory_allocation_properties_t prop;
            ze_device_handle_t device;
        } s_attr, d_attr;
        s_attr.prop.stype = ZE_STRUCTURE_TYPE_MEMORY_ALLOCATION_PROPERTIES;
        s_attr.prop.pNext = NULL;
        d_attr.prop.stype = ZE_STRUCTURE_TYPE_MEMORY_ALLOCATION_PROPERTIES;
        d_attr.prop.pNext = NULL;
        zerr = zeMemGetAllocProperties(context, sbuf, &s_attr.prop, &s_attr.device);
        assert(zerr == ZE_RESULT_SUCCESS);
        if (s_attr.device) {
            for (int i = 0; i < ndevices; i++) {
                if (device[i] == s_attr.device) {
                    s_dev_id = i;
                    break;
                }
            }
        }
        zerr = zeMemGetAllocProperties(context, dbuf, &d_attr.prop, &d_attr.device);
        assert(zerr == ZE_RESULT_SUCCESS);
        if (d_attr.device) {
            for (int i = 0; i < ndevices; i++) {
                if (device[i] == d_attr.device) {
                    d_dev_id = i;
                    break;
                }
            }
        }
        if (s_dev_id != -1 || d_dev_id != -1) {
            if (s_dev_id != -1)
                dev_id = s_dev_id;
            if (d_dev_id != -1) {
                if (dev_id != -1)
                    assert(s_dev_id == d_dev_id);
                else
                    dev_id = d_dev_id;
            }
        }
        assert(dev_id != -1);

        ze_event_handle_t event;
        ze_event_desc_t event_desc = {
            .stype = ZE_STRUCTURE_TYPE_EVENT_DESC,
            .pNext = NULL,
            .index = 0,
            .signal = ZE_EVENT_SCOPE_FLAG_HOST,
            .wait = ZE_EVENT_SCOPE_FLAG_HOST
        };
        zerr = zeEventCreate(event_pools[dev_id], &event_desc, &event);
        assert(zerr == ZE_RESULT_SUCCESS);

        zerr = zeCommandListReset(command_lists[dev_id]);
        assert(zerr == ZE_RESULT_SUCCESS);
        zerr =
            zeCommandListAppendMemoryCopy(command_lists[dev_id], dbuf, sbuf, size, event, 0, NULL);
        assert(zerr == ZE_RESULT_SUCCESS);
        zerr = zeEventHostSynchronize(event, UINT32_MAX);
        assert(zerr == ZE_RESULT_SUCCESS);

        zerr = zeEventDestroy(event);
        assert(zerr == ZE_RESULT_SUCCESS);
#endif
    }
}

void MTest_finalize_gpu(void)
{
#ifdef HAVE_ZE
    if (ndevices != -1) {
        /* Free GPU resource */
        free(device);
        assert(event_pools && command_lists);
        for (int i = 0; i < ndevices; i++) {
            zerr = zeEventPoolDestroy(event_pools[i]);
            assert(zerr == ZE_RESULT_SUCCESS);
            zerr = zeCommandListDestroy(command_lists[i]);
            assert(zerr == ZE_RESULT_SUCCESS);
        }
        free(event_pools);
        free(command_lists);
        zerr = zeContextDestroy(context);
    }
#endif
}

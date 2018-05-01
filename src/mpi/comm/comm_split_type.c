/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "mpicomm.h"

#ifdef HAVE_HWLOC
#include "hwloc.h"
#endif

/* -- Begin Profiling Symbol Block for routine MPI_Comm_split_type */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Comm_split_type = PMPI_Comm_split_type
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Comm_split_type  MPI_Comm_split_type
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Comm_split_type as PMPI_Comm_split_type
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Comm_split_type(MPI_Comm comm, int split_type, int key, MPI_Info info, MPI_Comm * newcomm)
    __attribute__ ((weak, alias("PMPI_Comm_split_type")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Comm_split_type
#define MPI_Comm_split_type PMPI_Comm_split_type

#undef FUNCNAME
#define FUNCNAME MPIR_Comm_split_type_self
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Comm_split_type_self(MPIR_Comm * user_comm_ptr, int split_type, int key,
                              MPIR_Comm ** newcomm_ptr)
{
    MPIR_Comm *comm_ptr = NULL;
    MPIR_Comm *comm_self_ptr;
    int mpi_errno = MPI_SUCCESS;

    /* split out the undefined processes */
    mpi_errno = MPIR_Comm_split_impl(user_comm_ptr, split_type == MPI_UNDEFINED ? MPI_UNDEFINED : 0,
                                     key, &comm_ptr);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    if (split_type == MPI_UNDEFINED) {
        *newcomm_ptr = NULL;
        goto fn_exit;
    }

    MPIR_Comm_get_ptr(MPI_COMM_SELF, comm_self_ptr);
    mpi_errno = MPIR_Comm_dup_impl(comm_self_ptr, newcomm_ptr);

    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    if (comm_ptr)
        MPIR_Comm_free_impl(comm_ptr);
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Comm_split_type_node
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Comm_split_type_node(MPIR_Comm * user_comm_ptr, int split_type, int key,
                              MPIR_Comm ** newcomm_ptr)
{
    MPIR_Comm *comm_ptr = NULL;
    int mpi_errno = MPI_SUCCESS;
    int color;

    /* split out the undefined processes */
    mpi_errno = MPIR_Comm_split_impl(user_comm_ptr, split_type == MPI_UNDEFINED ? MPI_UNDEFINED : 0,
                                     key, &comm_ptr);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    if (split_type == MPI_UNDEFINED) {
        *newcomm_ptr = NULL;
        goto fn_exit;
    }

    mpi_errno = MPID_Get_node_id(comm_ptr, comm_ptr->rank, &color);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    mpi_errno = MPIR_Comm_split_impl(comm_ptr, color, key, newcomm_ptr);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    if (comm_ptr)
        MPIR_Comm_free_impl(comm_ptr);
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

#ifdef HAVE_HWLOC

struct shmem_processor_info_table {
    const char *val;
    hwloc_obj_type_t obj_type;
};

/* hwloc processor object table */
static struct shmem_processor_info_table shmem_processor_info[] = {
    {"machine", HWLOC_OBJ_MACHINE},
    {"socket", HWLOC_OBJ_PACKAGE},
    {"package", HWLOC_OBJ_PACKAGE},
    {"numa", HWLOC_OBJ_NUMANODE},
    {"core", HWLOC_OBJ_CORE},
    {"hwthread", HWLOC_OBJ_PU},
    {"pu", HWLOC_OBJ_PU},
    {"l1dcache", HWLOC_OBJ_L1CACHE},
    {"l1ucache", HWLOC_OBJ_L1CACHE},
    {"l1icache", HWLOC_OBJ_L1ICACHE},
    {"l1cache", HWLOC_OBJ_L1CACHE},
    {"l2dcache", HWLOC_OBJ_L2CACHE},
    {"l2ucache", HWLOC_OBJ_L2CACHE},
    {"l2icache", HWLOC_OBJ_L2ICACHE},
    {"l2cache", HWLOC_OBJ_L2CACHE},
    {"l3dcache", HWLOC_OBJ_L3CACHE},
    {"l3ucache", HWLOC_OBJ_L3CACHE},
    {"l3icache", HWLOC_OBJ_L3ICACHE},
    {"l3cache", HWLOC_OBJ_L3CACHE},
    {"l4dcache", HWLOC_OBJ_L4CACHE},
    {"l4ucache", HWLOC_OBJ_L4CACHE},
    {"l4cache", HWLOC_OBJ_L4CACHE},
    {"l5dcache", HWLOC_OBJ_L5CACHE},
    {"l5ucache", HWLOC_OBJ_L5CACHE},
    {"l5cache", HWLOC_OBJ_L5CACHE},
    {NULL, HWLOC_OBJ_TYPE_MAX}
};

static int node_split_processor(MPIR_Comm * comm_ptr, int key, const char *hintval,
                                MPIR_Comm ** newcomm_ptr)
{
    int color;
    hwloc_obj_t obj_containing_cpuset;
    hwloc_obj_type_t query_obj_type = HWLOC_OBJ_TYPE_MAX;
    int i, mpi_errno = MPI_SUCCESS;

    /* assign the node id as the color, initially */
    MPID_Get_node_id(comm_ptr, comm_ptr->rank, &color);

    /* try to find the info value in the processor object
     * table */
    for (i = 0; shmem_processor_info[i].val; i++) {
        if (!strcmp(shmem_processor_info[i].val, hintval)) {
            query_obj_type = shmem_processor_info[i].obj_type;
            break;
        }
    }

    if (query_obj_type == HWLOC_OBJ_TYPE_MAX)
        goto split_id;

    obj_containing_cpuset =
        hwloc_get_obj_covering_cpuset(MPIR_Process.topology, MPIR_Process.bindset);
    MPIR_Assert(obj_containing_cpuset != NULL);
    if (obj_containing_cpuset->type == query_obj_type) {
        color = obj_containing_cpuset->logical_index;
    } else {
        hwloc_obj_t hobj = hwloc_get_ancestor_obj_by_type(MPIR_Process.topology, query_obj_type,
                                                          obj_containing_cpuset);
        if (hobj)
            color = hobj->logical_index;
        else
            color = MPI_UNDEFINED;
    }

  split_id:
    mpi_errno = MPIR_Comm_split_impl(comm_ptr, color, key, newcomm_ptr);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

static int node_split_pci_device(MPIR_Comm * comm_ptr, int key,
                                 const char *hintval, MPIR_Comm ** newcomm_ptr)
{
    hwloc_obj_t obj_containing_cpuset, io_device = NULL;
    int mpi_errno = MPI_SUCCESS;
    int color;

    obj_containing_cpuset =
        hwloc_get_obj_covering_cpuset(MPIR_Process.topology, MPIR_Process.bindset);
    MPIR_Assert(obj_containing_cpuset != NULL);

    io_device = hwloc_get_pcidev_by_busidstring(MPIR_Process.topology, hintval + strlen("pci:"));

    if (io_device != NULL) {
        hwloc_obj_t non_io_ancestor =
            hwloc_get_non_io_ancestor_obj(MPIR_Process.topology, io_device);

        /* An io object will never be the root of the topology and is
         * hence guaranteed to have a non io ancestor */
        MPIR_Assert(non_io_ancestor);

        if (hwloc_obj_is_in_subtree(MPIR_Process.topology, obj_containing_cpuset, non_io_ancestor)) {
            color = non_io_ancestor->logical_index;
        } else
            color = MPI_UNDEFINED;
    } else
        color = MPI_UNDEFINED;

    mpi_errno = MPIR_Comm_split_impl(comm_ptr, color, key, newcomm_ptr);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

static int io_device_found(const char *resource, const char *devname, hwloc_obj_t io_device,
                           hwloc_obj_osdev_type_t obj_type)
{
    if (!strncmp(resource, devname, strlen(devname))) {
        /* device type does not match */
        if (io_device->attr->osdev.type != obj_type)
            return 0;

        /* device prefix does not match */
        if (strncmp(io_device->name, devname, strlen(devname)))
            return 0;

        /* specific device is supplied, but does not match */
        if (strlen(resource) != strlen(devname) && strcmp(io_device->name, resource))
            return 0;
    }

    return 1;
}

static int node_split_network_device(MPIR_Comm * comm_ptr, int key,
                                     const char *hintval, MPIR_Comm ** newcomm_ptr)
{
    hwloc_obj_t obj_containing_cpuset, io_device = NULL;
    int mpi_errno = MPI_SUCCESS;
    int color;

    /* assign the node id as the color, initially */
    MPID_Get_node_id(comm_ptr, comm_ptr->rank, &color);

    obj_containing_cpuset =
        hwloc_get_obj_covering_cpuset(MPIR_Process.topology, MPIR_Process.bindset);
    MPIR_Assert(obj_containing_cpuset != NULL);

    color = MPI_UNDEFINED;
    while ((io_device = hwloc_get_next_osdev(MPIR_Process.topology, io_device))) {
        hwloc_obj_t non_io_ancestor;
        uint32_t depth;

        if (!io_device_found(hintval, "hfi", io_device, HWLOC_OBJ_OSDEV_OPENFABRICS))
            continue;
        if (!io_device_found(hintval, "ib", io_device, HWLOC_OBJ_OSDEV_NETWORK))
            continue;
        if (!io_device_found(hintval, "eth", io_device, HWLOC_OBJ_OSDEV_NETWORK) &&
            !io_device_found(hintval, "en", io_device, HWLOC_OBJ_OSDEV_NETWORK))
            continue;

        non_io_ancestor = hwloc_get_non_io_ancestor_obj(MPIR_Process.topology, io_device);
        while (!hwloc_obj_type_is_normal(non_io_ancestor->type))
            non_io_ancestor = non_io_ancestor->parent;
        MPIR_Assert(non_io_ancestor && non_io_ancestor->depth >= 0);

        if (!hwloc_obj_is_in_subtree(MPIR_Process.topology, obj_containing_cpuset, non_io_ancestor))
            continue;

        /* Get a unique ID for the non-IO object.  Use fixed width
         * unsigned integers, so bit shift operations are well
         * defined */
        depth = (uint32_t) non_io_ancestor->depth;
        color = (int) ((depth << 16) + non_io_ancestor->logical_index);
        break;
    }

    mpi_errno = MPIR_Comm_split_impl(comm_ptr, color, key, newcomm_ptr);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

static int node_split_gpu_device(MPIR_Comm * comm_ptr, int key,
                                 const char *hintval, MPIR_Comm ** newcomm_ptr)
{
    hwloc_obj_t obj_containing_cpuset, io_device = NULL;
    int mpi_errno = MPI_SUCCESS;
    int color;

    obj_containing_cpuset =
        hwloc_get_obj_covering_cpuset(MPIR_Process.topology, MPIR_Process.bindset);
    MPIR_Assert(obj_containing_cpuset != NULL);

    color = MPI_UNDEFINED;
    while ((io_device = hwloc_get_next_osdev(MPIR_Process.topology, io_device))
           != NULL) {
        if (io_device->attr->osdev.type == HWLOC_OBJ_OSDEV_GPU) {
            if ((*(hintval + strlen("gpu")) != '\0') &&
                atoi(hintval + strlen("gpu")) != io_device->logical_index)
                continue;
            hwloc_obj_t non_io_ancestor =
                hwloc_get_non_io_ancestor_obj(MPIR_Process.topology, io_device);
            MPIR_Assert(non_io_ancestor);
            if (hwloc_obj_is_in_subtree
                (MPIR_Process.topology, obj_containing_cpuset, non_io_ancestor)) {
                color =
                    (non_io_ancestor->type << (sizeof(int) * 4)) + non_io_ancestor->logical_index;
                break;
            }
        }
    }

    mpi_errno = MPIR_Comm_split_impl(comm_ptr, color, key, newcomm_ptr);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}
#endif /* HAVE_HWLOC */

static const char *SHMEM_INFO_KEY = "shmem_topo";

static int compare_info_args(const void *sendbuf, void *recvbuf, int count, MPIR_Comm * comm_ptr,
                             int *info_args_global)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    int info_args_are_equal = 0;

    mpi_errno = MPIR_Allreduce(sendbuf, recvbuf, count, MPI_BYTE, MPI_BAND, comm_ptr, &errflag);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    if (!memcmp(sendbuf, recvbuf, count))
        info_args_are_equal = 1;

    mpi_errno =
        MPIR_Allreduce(&info_args_are_equal, info_args_global, 1, MPI_INT, MPI_MIN, comm_ptr,
                       &errflag);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Comm_split_type_node_topo
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Comm_split_type_node_topo(MPIR_Comm * user_comm_ptr, int split_type, int key,
                                   MPIR_Info * info_ptr, MPIR_Comm ** newcomm_ptr)
{
    MPIR_Comm *comm_ptr;
    int mpi_errno = MPI_SUCCESS;
    int hintval_size, hintval_size_global;
    char *hintval_global = NULL;
    int flag = 0;
    char hintval[MPI_MAX_INFO_VAL + 1];
    int info_args_are_equal;
    *newcomm_ptr = NULL;

    mpi_errno = MPIR_Comm_split_type_node(user_comm_ptr, split_type, key, &comm_ptr);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    if (comm_ptr == NULL) {
        MPIR_Assert(split_type == MPI_UNDEFINED);
        *newcomm_ptr = NULL;
        goto fn_exit;
    }

    if (info_ptr) {
        MPIR_Info_get_impl(info_ptr, SHMEM_INFO_KEY, MPI_MAX_INFO_VAL, hintval, &flag);
    }

    if (!flag) {
        hintval[0] = '\0';
    }

    /* Compare hintval string length */
    hintval_size = strlen(hintval);
    mpi_errno =
        compare_info_args(&hintval_size, &hintval_size_global, sizeof(int) / sizeof(char), comm_ptr,
                          &info_args_are_equal);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    /* if all processes do not give the same length of hintval, skip
     * topology-aware comm split */
    if (!info_args_are_equal)
        goto use_node_comm;

    hintval_global = MPL_malloc(sizeof(hintval), MPL_MEM_OTHER);
    /* Compare the hintval strings */
    mpi_errno =
        compare_info_args(hintval, hintval_global, (strlen(hintval) + 1) * sizeof(char), comm_ptr,
                          &info_args_are_equal);

    if (hintval_global != NULL)
        MPL_free(hintval_global);

    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    /* if all processes do not have the same hintval, skip
     * topology-aware comm split */
    if (!info_args_are_equal)
        goto use_node_comm;

    /* if no info key is given, skip topology-aware comm split */
    if (!info_ptr)
        goto use_node_comm;

#ifdef HAVE_HWLOC
    /* if our bindset is not valid, skip topology-aware comm split */
    if (!MPIR_Process.bindset_is_valid)
        goto use_node_comm;

    if (flag) {
        if (!strncmp(hintval, "pci:", strlen("pci:")))
            mpi_errno = node_split_pci_device(comm_ptr, key, hintval, newcomm_ptr);
        else if (!strncmp(hintval, "ib", strlen("ib")) ||
                 !strncmp(hintval, "en", strlen("en")) ||
                 !strncmp(hintval, "eth", strlen("eth")) || !strncmp(hintval, "hfi", strlen("hfi")))
            mpi_errno = node_split_network_device(comm_ptr, key, hintval, newcomm_ptr);
        else if (!strncmp(hintval, "gpu", strlen("gpu")))
            mpi_errno = node_split_gpu_device(comm_ptr, key, hintval, newcomm_ptr);
        else
            mpi_errno = node_split_processor(comm_ptr, key, hintval, newcomm_ptr);

        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);

        MPIR_Comm_free_impl(comm_ptr);

        goto fn_exit;
    }
#endif /* HAVE_HWLOC */

  use_node_comm:
    *newcomm_ptr = comm_ptr;

  fn_exit:
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Comm_split_type
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Comm_split_type(MPIR_Comm * user_comm_ptr, int split_type, int key,
                         MPIR_Info * info_ptr, MPIR_Comm ** newcomm_ptr)
{
    MPIR_Comm *comm_ptr = NULL;
    int mpi_errno = MPI_SUCCESS;

    /* split out the undefined processes */
    mpi_errno = MPIR_Comm_split_impl(user_comm_ptr, split_type == MPI_UNDEFINED ? MPI_UNDEFINED : 0,
                                     key, &comm_ptr);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    if (split_type == MPI_UNDEFINED) {
        *newcomm_ptr = NULL;
        goto fn_exit;
    }

    if (split_type == MPI_COMM_TYPE_SHARED) {
        mpi_errno = MPIR_Comm_split_type_self(comm_ptr, split_type, key, newcomm_ptr);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    } else if (split_type == MPIX_COMM_TYPE_NEIGHBORHOOD) {
        int flag;
        char hintval[MPI_MAX_INFO_VAL + 1];

        /* We plan on dispatching different NEIGHBORHOOD support to
         * different parts of MPICH, based on the key provided in the
         * info object.  Right now, the one NEIGHBORHOOD we support is
         * "nbhd_common_dirname", implementation of which lives in ROMIO */

        MPIR_Info_get_impl(info_ptr, "nbhd_common_dirname", MPI_MAX_INFO_VAL, hintval, &flag);
        if (flag) {
#ifdef HAVE_ROMIO
            MPI_Comm dummycomm;
            MPIR_Comm *dummycomm_ptr;

            mpi_errno = MPIR_Comm_split_filesystem(comm_ptr->handle, key, hintval, &dummycomm);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);

            MPIR_Comm_get_ptr(dummycomm, dummycomm_ptr);
            *newcomm_ptr = dummycomm_ptr;
#endif
        } else {
            /* In the mean time, the user passed in
             * COMM_TYPE_NEIGHBORHOOD but did not give us an info we
             * know how to work with.  Throw up our hands and treat it
             * like UNDEFINED.  This will result in MPI_COMM_NULL
             * being returned to the user. */
            *newcomm_ptr = NULL;
        }
    } else {
        MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_ARG, "**arg");
    }

  fn_exit:
    if (comm_ptr)
        MPIR_Comm_free_impl(comm_ptr);
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Comm_split_type_impl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Comm_split_type_impl(MPIR_Comm * comm_ptr, int split_type, int key,
                              MPIR_Info * info_ptr, MPIR_Comm ** newcomm_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    /* Only MPI_COMM_TYPE_SHARED, MPI_UNDEFINED, and
     * NEIGHBORHOOD are supported */
    MPIR_Assert(split_type == MPI_COMM_TYPE_SHARED ||
                split_type == MPI_UNDEFINED || split_type == MPIX_COMM_TYPE_NEIGHBORHOOD);

    if (MPIR_Comm_fns != NULL && MPIR_Comm_fns->split_type != NULL) {
        mpi_errno = MPIR_Comm_fns->split_type(comm_ptr, split_type, key, info_ptr, newcomm_ptr);
    } else {
        mpi_errno = MPIR_Comm_split_type(comm_ptr, split_type, key, info_ptr, newcomm_ptr);
    }
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* MPICH_MPI_FROM_PMPI */

#undef FUNCNAME
#define FUNCNAME MPI_Comm_split_type
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@

MPI_Comm_split_type - Creates new communicators based on split types and keys

Input Parameters:
+ comm - communicator (handle)
. split_type - type of processes to be grouped together (nonnegative integer).
. key - control of rank assignment (integer)
- info - hints to improve communicator creation (handle)

Output Parameters:
. newcomm - new communicator (handle)

Notes:
  The 'split_type' must be non-negative or 'MPI_UNDEFINED'.

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
.N MPI_ERR_EXHAUSTED

.seealso: MPI_Comm_free
@*/
int MPI_Comm_split_type(MPI_Comm comm, int split_type, int key, MPI_Info info, MPI_Comm * newcomm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm_ptr = NULL, *newcomm_ptr;
    MPIR_Info *info_ptr = NULL;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_COMM_SPLIT_TYPE);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_COMM_SPLIT_TYPE);

    /* Validate parameters, especially handles needing to be converted */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_COMM(comm, mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }

#endif /* HAVE_ERROR_CHECKING */

    /* Get handles to MPI objects. */
    MPIR_Comm_get_ptr(comm, comm_ptr);
    MPIR_Info_get_ptr(info, info_ptr);

    /* Validate parameters and objects (post conversion) */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Validate comm_ptr */
            MPIR_Comm_valid_ptr(comm_ptr, mpi_errno, FALSE);
            /* If comm_ptr is not valid, it will be reset to null */
            if (mpi_errno)
                goto fn_fail;
            MPIR_ERRTEST_ARGNULL(newcomm, "newcomm", mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPIR_Comm_split_type_impl(comm_ptr, split_type, key, info_ptr, &newcomm_ptr);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
    if (newcomm_ptr)
        MPIR_OBJ_PUBLISH_HANDLE(*newcomm, newcomm_ptr->handle);
    else
        *newcomm = MPI_COMM_NULL;

    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_COMM_SPLIT_TYPE);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    {
        /* FIXME this error code is wrong, it's the error code for
         * regular MPI_Comm_split */
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__,
                                 MPI_ERR_OTHER, "**mpi_comm_split",
                                 "**mpi_comm_split %C %d %d %p", comm, split_type, key, newcomm);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

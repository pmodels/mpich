#ifndef MPI_ABI_UTIL_H_INCLUDED
#define MPI_ABI_UTIL_H_INCLUDED

#include "mpi_abi_internal.h"
#include "mpiimpl.h"

#define ABI_IS_BUILTIN(h)  ((intptr_t)(h) < 0x1000)

#define ABI_DATATYPE_BUILTIN_MASK 0x1ff
#define ABI_OP_BUILTIN_MASK 0x1f
#define ABI_MAX_DATATYPE_BUILTINS 512
#define ABI_MAX_OP_BUILTINS 32

/* only datatype and op have many builtins to deserve a table lookup */
extern MPI_Datatype abi_datatype_builtins[];
extern MPI_Op abi_op_builtins[];

void ABI_init_builtins(void);

static inline MPI_Comm ABI_Comm_to_mpi(ABI_Comm in)
{
    if (ABI_IS_BUILTIN(in)) {
        if (in == ABI_COMM_NULL) {
            return MPI_COMM_NULL;
        } else if (in == ABI_COMM_WORLD) {
            return MPI_COMM_WORLD;
        } else if (in == ABI_COMM_SELF) {
            return MPI_COMM_SELF;
        } else {
            return MPI_COMM_NULL;
        }
    }
    return ((MPIR_Comm *) in)->handle;
}

static inline ABI_Comm ABI_Comm_from_mpi(MPI_Comm in)
{
    if (in == MPI_COMM_NULL) {
        return ABI_COMM_NULL;
    } else if (in == MPI_COMM_WORLD) {
        return ABI_COMM_WORLD;
    } else if (in == MPI_COMM_SELF) {
        return ABI_COMM_SELF;
    }
    MPIR_Comm *ptr;
    MPIR_Comm_get_ptr(in, ptr);
    return (ABI_Comm) (void *) ptr;
}

static inline MPIR_Comm *ABI_Comm_ptr(ABI_Comm comm_abi)
{
    MPIR_Comm *comm_ptr = NULL;
    if (comm_abi != ABI_COMM_NULL) {
        MPI_Comm comm = ABI_Comm_to_mpi(comm_abi);
        MPIR_Comm_get_ptr(comm, comm_ptr);
        if (comm_ptr != NULL) {
            if (MPIR_Object_get_ref(comm_ptr) <= 0) {
                comm_ptr = NULL;
            }
        }
    }
    return comm_ptr;
}

static inline int ABI_Comm_rank(ABI_Comm comm_abi)
{
    MPIR_Comm *comm_ptr = ABI_Comm_ptr(comm_abi);
    int rank = MPI_PROC_NULL;
    if (comm_ptr != NULL) {
        MPIR_Comm_rank_impl(comm_ptr, &rank);
    }
    return rank;
}

static inline int ABI_Comm_peer_size(ABI_Comm comm_abi)
{
    MPIR_Comm *comm_ptr = ABI_Comm_ptr(comm_abi);
    int size = 0;
    if (comm_ptr != NULL) {
        int flag = 0;
        MPIR_Comm_test_inter_impl(comm_ptr, &flag);
        if (flag) {
            MPIR_Comm_remote_size_impl(comm_ptr, &size);
        } else {
            MPIR_Comm_size_impl(comm_ptr, &size);
        }
    }
    return size;
}

static inline void ABI_Comm_neighbors_count(ABI_Comm comm_abi, int *indegree, int *outdegree)
{
    MPIR_Comm *comm_ptr = ABI_Comm_ptr(comm_abi);
    int topo = MPI_UNDEFINED;
    int rank = MPI_PROC_NULL;
    int ival = 0;
    if (comm_ptr != NULL) {
        MPIR_Topo_test_impl(comm_ptr, &topo);
    }
    switch (topo) {
        case MPI_CART:
            MPIR_Cartdim_get_impl(comm_ptr, &ival);
            *indegree = *outdegree = 2 * ival;
            break;
        case MPI_GRAPH:
            MPIR_Comm_rank_impl(comm_ptr, &rank);
            MPIR_Graph_neighbors_count_impl(comm_ptr, rank, &ival);
            *indegree = *outdegree = ival;
            break;
        case MPI_DIST_GRAPH:
            *indegree = *outdegree = 0;
            MPIR_Dist_graph_neighbors_count_impl(comm_ptr, indegree, outdegree, &ival);
            break;
        default:
            *indegree = *outdegree = 0;
            break;
    }
}

static inline MPI_Datatype ABI_Datatype_to_mpi(ABI_Datatype in)
{
    if (ABI_IS_BUILTIN(in)) {
        /* TODO: error check */
        return abi_datatype_builtins[(intptr_t) in & ABI_DATATYPE_BUILTIN_MASK];
    }
    return ((MPIR_Datatype *) in)->handle;
}

static inline ABI_Datatype ABI_Datatype_from_mpi(MPI_Datatype in)
{
    if (in == MPI_DATATYPE_NULL) {
        return ABI_DATATYPE_NULL;
    }
    if (MPIR_DATATYPE_IS_PREDEFINED(in)) {
        for (int i = 0; i < ABI_MAX_DATATYPE_BUILTINS; i++) {
            if (abi_datatype_builtins[i] == in) {
                return (ABI_Datatype) ((intptr_t) ABI_DATATYPE_NULL + i);
            }
        }
        MPIR_Assert(0);
    }
    MPIR_Datatype *ptr;
    MPIR_Datatype_get_ptr(in, ptr);
    return (ABI_Datatype) (void *) ptr;
}

static inline MPI_Errhandler ABI_Errhandler_to_mpi(ABI_Errhandler in)
{
    if (in == ABI_ERRHANDLER_NULL) {
        return MPI_ERRHANDLER_NULL;
    } else if (in == ABI_ERRORS_ARE_FATAL) {
        return MPI_ERRORS_ARE_FATAL;
    } else if (in == ABI_ERRORS_ABORT) {
        return MPI_ERRORS_ABORT;
    } else if (in == ABI_ERRORS_RETURN) {
        return MPI_ERRORS_RETURN;
    }
    return ((MPIR_Errhandler *) in)->handle;
}

static inline ABI_Errhandler ABI_Errhandler_from_mpi(MPI_Errhandler in)
{
    if (in == MPI_ERRHANDLER_NULL) {
        return ABI_ERRHANDLER_NULL;
    } else if (in == MPI_ERRORS_ARE_FATAL) {
        return ABI_ERRORS_ARE_FATAL;
    } else if (in == MPI_ERRORS_ABORT) {
        return ABI_ERRORS_ABORT;
    } else if (in == MPI_ERRORS_RETURN) {
        return ABI_ERRORS_RETURN;
    }
    MPIR_Errhandler *ptr;
    MPIR_Errhandler_get_ptr(in, ptr);
    return (ABI_Errhandler) (void *) ptr;
}

static inline MPI_Group ABI_Group_to_mpi(ABI_Group in)
{
    if (ABI_IS_BUILTIN(in)) {
        if (in == ABI_GROUP_NULL) {
            return MPI_GROUP_NULL;
        } else if (in == ABI_GROUP_EMPTY) {
            return MPI_GROUP_EMPTY;
        } else {
            return MPI_GROUP_NULL;
        }
    }
    return ((MPIR_Group *) in)->handle;
}

static inline ABI_Group ABI_Group_from_mpi(MPI_Group in)
{
    if (in == MPI_GROUP_NULL) {
        return ABI_GROUP_NULL;
    } else if (in == MPI_GROUP_EMPTY) {
        return ABI_GROUP_EMPTY;
    }
    MPIR_Group *ptr;
    MPIR_Group_get_ptr(in, ptr);
    return (ABI_Group) (void *) ptr;
}

static inline MPI_Info ABI_Info_to_mpi(ABI_Info in)
{
    if (in == ABI_INFO_NULL) {
        return MPI_INFO_NULL;
    } else if (in == ABI_INFO_ENV) {
        return MPI_INFO_ENV;
    }
    return ((MPIR_Info *) in)->handle;
}

static inline ABI_Info ABI_Info_from_mpi(MPI_Info in)
{
    if (in == MPI_INFO_NULL) {
        return ABI_INFO_NULL;
    } else if (in == MPI_INFO_ENV) {
        return ABI_INFO_ENV;
    }
    MPIR_Info *ptr;
    MPIR_Info_get_ptr(in, ptr);
    return (ABI_Info) (void *) ptr;
}

static inline MPI_Message ABI_Message_to_mpi(ABI_Message in)
{
    if (in == ABI_MESSAGE_NULL) {
        return MPI_MESSAGE_NULL;
    } else if (in == ABI_MESSAGE_NO_PROC) {
        return MPI_MESSAGE_NO_PROC;
    }
    return ((MPIR_Request *) in)->handle;
}

static inline ABI_Message ABI_Message_from_mpi(MPI_Message in)
{
    if (in == MPI_MESSAGE_NULL) {
        return ABI_MESSAGE_NULL;
    } else if (in == MPI_MESSAGE_NO_PROC) {
        return ABI_MESSAGE_NO_PROC;
    }
    MPIR_Request *ptr;
    MPIR_Request_get_ptr(in, ptr);
    return (ABI_Message) (void *) ptr;
}

static inline MPI_Op ABI_Op_to_mpi(ABI_Op in)
{
    if (ABI_IS_BUILTIN(in)) {
        /* TODO: error check */
        return abi_op_builtins[(intptr_t) in & ABI_OP_BUILTIN_MASK];
    }
    return ((MPIR_Op *) in)->handle;
}

static inline ABI_Op ABI_Op_from_mpi(MPI_Op in)
{
    if (in == MPI_OP_NULL) {
        return ABI_OP_NULL;
    }
    if (HANDLE_IS_BUILTIN(in)) {
        for (int i = 0; i < ABI_MAX_OP_BUILTINS; i++) {
            if (abi_op_builtins[i] == in) {
                return (ABI_Op) ((intptr_t) ABI_OP_NULL + i);
            }
        }
    }
    MPIR_Op *ptr;
    MPIR_Op_get_ptr(in, ptr);
    return (ABI_Op) (void *) ptr;
}

static inline MPI_Request ABI_Request_to_mpi(ABI_Request in)
{
    if (in == ABI_REQUEST_NULL) {
        return MPI_REQUEST_NULL;
    }
    return ((MPIR_Request *) in)->handle;
}

static inline ABI_Request ABI_Request_from_mpi(MPI_Request in)
{
    if (in == MPI_REQUEST_NULL) {
        return ABI_REQUEST_NULL;
    }
    MPIR_Request *ptr;
    MPIR_Request_get_ptr(in, ptr);
    return (ABI_Request) (void *) ptr;
}

static inline MPI_Session ABI_Session_to_mpi(ABI_Session in)
{
    if (in == ABI_SESSION_NULL) {
        return MPI_SESSION_NULL;
    }
    return ((MPIR_Session *) in)->handle;
}

static inline ABI_Session ABI_Session_from_mpi(MPI_Session in)
{
    if (in == MPI_SESSION_NULL) {
        return ABI_SESSION_NULL;
    }
    MPIR_Session *ptr;
    MPIR_Session_get_ptr(in, ptr);
    return (ABI_Session) (void *) ptr;
}

static inline MPI_Win ABI_Win_to_mpi(ABI_Win in)
{
    if (in == ABI_WIN_NULL) {
        return MPI_WIN_NULL;
    }
    return ((MPIR_Win *) in)->handle;
}

static inline ABI_Win ABI_Win_from_mpi(MPI_Win in)
{
    if (in == MPI_WIN_NULL) {
        return ABI_WIN_NULL;
    }
    MPIR_Win *ptr;
    MPIR_Win_get_ptr(in, ptr);
    return (ABI_Win) (void *) ptr;
}

static inline MPI_File ABI_File_to_mpi(ABI_File in)
{
    if (in == ABI_FILE_NULL) {
        return MPI_FILE_NULL;
    }
    /* Both MPI_File in mpich and ABI_File are pointers */
    return (MPI_File) in;
}

static inline ABI_File ABI_File_from_mpi(MPI_File in)
{
    if (in == MPI_FILE_NULL) {
        return ABI_FILE_NULL;
    }
    /* Both MPI_File in mpich and ABI_File are pointers */
    return (ABI_File) in;
}

/* MPICH internal callbacks does not differentiate handle types, so we need
 * a general conversion routine */
static inline void *ABI_Handle_from_mpi(int in)
{
    switch (HANDLE_GET_MPI_KIND(in)) {
        case MPIR_COMM:
            return ABI_Comm_from_mpi(in);
        case MPIR_DATATYPE:
            return ABI_Datatype_from_mpi(in);
        case MPIR_WIN:
            return ABI_Win_from_mpi(in);
        case MPIR_SESSION:
            return ABI_Session_from_mpi(in);
        default:
            MPIR_Assert(0);
            return NULL;
    }
}

static inline int ABI_KEYVAL_to_mpi(int keyval)
{
    switch (keyval) {
        case ABI_KEYVAL_INVALID:
            return MPI_KEYVAL_INVALID;
        case ABI_TAG_UB:
            return MPI_TAG_UB;
        case ABI_HOST:
            return MPI_HOST;
        case ABI_IO:
            return MPI_IO;
        case ABI_WTIME_IS_GLOBAL:
            return MPI_WTIME_IS_GLOBAL;
        case ABI_UNIVERSE_SIZE:
            return MPI_UNIVERSE_SIZE;
        case ABI_LASTUSEDCODE:
            return MPI_LASTUSEDCODE;
        case ABI_APPNUM:
            return MPI_APPNUM;
        case ABI_WIN_BASE:
            return MPI_WIN_BASE;
        case ABI_WIN_SIZE:
            return MPI_WIN_SIZE;
        case ABI_WIN_DISP_UNIT:
            return MPI_WIN_DISP_UNIT;
        case ABI_WIN_CREATE_FLAVOR:
            return MPI_WIN_CREATE_FLAVOR;
        case ABI_WIN_MODEL:
            return MPI_WIN_MODEL;
        default:
            return keyval;
    }
}

static inline int ABI_KEYVAL_from_mpi(int keyval)
{
    switch (keyval) {
        case MPI_KEYVAL_INVALID:
            return ABI_KEYVAL_INVALID;
        case MPI_TAG_UB:
            return ABI_TAG_UB;
        case MPI_HOST:
            return ABI_HOST;
        case MPI_IO:
            return ABI_IO;
        case MPI_WTIME_IS_GLOBAL:
            return ABI_WTIME_IS_GLOBAL;
        case MPI_UNIVERSE_SIZE:
            return ABI_UNIVERSE_SIZE;
        case MPI_LASTUSEDCODE:
            return ABI_LASTUSEDCODE;
        case MPI_APPNUM:
            return ABI_APPNUM;
        case MPI_WIN_BASE:
            return ABI_WIN_BASE;
        case MPI_WIN_SIZE:
            return ABI_WIN_SIZE;
        case MPI_WIN_DISP_UNIT:
            return ABI_WIN_DISP_UNIT;
        case MPI_WIN_CREATE_FLAVOR:
            return ABI_WIN_CREATE_FLAVOR;
        case MPI_WIN_MODEL:
            return ABI_WIN_MODEL;
        default:
            return keyval;
    }
}

#endif /* MPI_ABI_UTIL_H_INCLUDED */

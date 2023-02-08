#ifndef MPI_ABI_UTIL_H_INCLUDED
#define MPI_ABI_UTIL_H_INCLUDED

#include "mpi_abi_internal.h"
#include "mpiimpl.h"

/* NOTE: compatible with MPIR_OBJECT_HEADER */
struct mpi_comm_t {
    int handle;
};
struct mpi_datatype_t {
    int handle;
};
struct mpi_errhandler_t {
    int handle;
};
struct mpi_file_t {
    int handle;
};
struct mpi_group_t {
    int handle;
};
struct mpi_info_t {
    int handle;
};
struct mpi_message_t {
    int handle;
};
struct mpi_op_t {
    int handle;
};
struct mpi_request_t {
    int handle;
};
struct mpi_session_t {
    int handle;
};
struct mpi_win_t {
    int handle;
};

void ABI_init_builtins(void);

static inline MPI_Comm ABI_Comm_to_mpi(ABI_Comm in)
{
    if (in == ABI_COMM_NULL) {
        return MPI_COMM_NULL;
    }
    return in->handle;
}

static inline ABI_Comm ABI_Comm_from_mpi(MPI_Comm in)
{
    if (in == MPI_COMM_NULL) {
        return ABI_COMM_NULL;
    }
    MPIR_Comm *ptr;
    MPIR_Comm_get_ptr(in, ptr);
    return (ABI_Comm) (void *) ptr;
}

static inline MPI_Datatype ABI_Datatype_to_mpi(ABI_Datatype in)
{
    if (in == ABI_DATATYPE_NULL) {
        return MPI_DATATYPE_NULL;
    }
    return in->handle;
}

static inline ABI_Datatype ABI_Datatype_from_mpi(MPI_Datatype in)
{
    if (in == MPI_DATATYPE_NULL) {
        return ABI_DATATYPE_NULL;
    }
    MPIR_Datatype *ptr;
    MPIR_Datatype_get_ptr(in, ptr);
    return (ABI_Datatype) (void *) ptr;
}

static inline MPI_Errhandler ABI_Errhandler_to_mpi(ABI_Errhandler in)
{
    if (in == ABI_ERRHANDLER_NULL) {
        return MPI_ERRHANDLER_NULL;
    }
    return in->handle;
}

static inline ABI_Errhandler ABI_Errhandler_from_mpi(MPI_Errhandler in)
{
    if (in == MPI_ERRHANDLER_NULL) {
        return ABI_ERRHANDLER_NULL;
    }
    MPIR_Errhandler *ptr;
    MPIR_Errhandler_get_ptr(in, ptr);
    return (ABI_Errhandler) (void *) ptr;
}

static inline MPI_File ABI_File_to_mpi(ABI_File in)
{
    if (in == ABI_FILE_NULL) {
        return MPI_FILE_NULL;
    }
    return (MPI_File) (void *) in;
}

static inline ABI_File ABI_File_from_mpi(MPI_File in)
{
    if (in == MPI_FILE_NULL) {
        return ABI_FILE_NULL;
    }
    return (ABI_File) (void *) in;
}

static inline MPI_Group ABI_Group_to_mpi(ABI_Group in)
{
    if (in == ABI_GROUP_NULL) {
        return MPI_GROUP_NULL;
    }
    return in->handle;
}

static inline ABI_Group ABI_Group_from_mpi(MPI_Group in)
{
    if (in == MPI_GROUP_NULL) {
        return ABI_GROUP_NULL;
    }
    MPIR_Group *ptr;
    MPIR_Group_get_ptr(in, ptr);
    return (ABI_Group) (void *) ptr;
}

static inline MPI_Info ABI_Info_to_mpi(ABI_Info in)
{
    if (in == ABI_INFO_NULL) {
        return MPI_INFO_NULL;
    }
    return in->handle;
}

static inline ABI_Info ABI_Info_from_mpi(MPI_Info in)
{
    if (in == MPI_INFO_NULL) {
        return ABI_INFO_NULL;
    }
    MPIR_Info *ptr;
    MPIR_Info_get_ptr(in, ptr);
    return (ABI_Info) (void *) ptr;
}

static inline MPI_Message ABI_Message_to_mpi(ABI_Message in)
{
    if (in == ABI_MESSAGE_NULL) {
        return MPI_MESSAGE_NULL;
    }
    return in->handle;
}

static inline ABI_Message ABI_Message_from_mpi(MPI_Message in)
{
    if (in == MPI_MESSAGE_NULL) {
        return ABI_MESSAGE_NULL;
    }
    MPIR_Request *ptr;
    MPIR_Request_get_ptr(in, ptr);
    return (ABI_Message) (void *) ptr;
}

static inline MPI_Op ABI_Op_to_mpi(ABI_Op in)
{
    if (in == ABI_OP_NULL) {
        return MPI_OP_NULL;
    }
    return in->handle;
}

static inline ABI_Op ABI_Op_from_mpi(MPI_Op in)
{
    if (in == MPI_OP_NULL) {
        return ABI_OP_NULL;
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
    return in->handle;
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
    return in->handle;
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
    return in->handle;
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

static inline void ABI_Status_to_mpi(const ABI_Status * in, MPI_Status * out)
{
    out->MPI_SOURCE = in->MPI_SOURCE;
    out->MPI_TAG = in->MPI_TAG;
    out->MPI_ERROR = in->MPI_ERROR;
    out->count_lo = in->reserved[0];
    out->count_hi_and_cancelled = in->reserved[1];
}

static inline void ABI_Status_from_mpi(const MPI_Status * in, ABI_Status * out)
{
    out->MPI_SOURCE = in->MPI_SOURCE;
    out->MPI_TAG = in->MPI_TAG;
    out->MPI_ERROR = in->MPI_ERROR;
    out->reserved[0] = in->count_lo;
    out->reserved[1] = in->count_hi_and_cancelled;
}

#endif /* MPI_ABI_UTIL_H_INCLUDED */

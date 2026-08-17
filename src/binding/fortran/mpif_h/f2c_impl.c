/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpi_fortimpl.h"

int MPIR_Status_f2c_impl(const MPI_Fint * f_status, MPI_Status * c_status)
{
    if (f_status == MPI_F_STATUS_IGNORE || f_status == MPI_F_STATUSES_IGNORE) {
        return MPI_ERR_ARG;
    }
#ifdef HAVE_FINT_IS_INT
    *c_status = *(MPI_Status *) f_status;
#else
    /* cast c_status as int array */
    int *p = (int *) c_status;
    for (int i = 0; i < MPI_F_STATUS_SIZE; i++) {
        p[i] = (int) f_status[i];
    }
#endif
    return MPI_SUCCESS;
}

int MPIR_Status_c2f_impl(const MPI_Status * c_status, MPI_Fint * f_status)
{
    if (c_status == MPI_STATUS_IGNORE || c_status == MPI_STATUSES_IGNORE) {
        return MPI_ERR_ARG;
    }
#ifdef HAVE_FINT_IS_INT
    *(MPI_Status *) f_status = *c_status;
#else
    /* cast c_status as int array */
    int *p = (int *) c_status;
    for (int i = 0; i < MPI_F_STATUS_SIZE; i++) {
        f_status[i] = (MPI_Fint) p[i];
    }
#endif
    return MPI_SUCCESS;
}

int MPIR_Status_f2f08_impl(const MPI_Fint * f_status, MPI_F08_status * f08_status)
{
    if (f_status == MPI_F_STATUS_IGNORE || f_status == MPI_F_STATUSES_IGNORE) {
        return MPI_ERR_ARG;
    }
    /* f_status and f08_status are always byte-equivalent */
    *f08_status = *(MPI_F08_status *) f_status;
    return MPI_SUCCESS;
}

int MPIR_Status_f082f_impl(const MPI_F08_status * f08_status, MPI_Fint * f_status)
{
    if (f08_status == MPI_F08_STATUS_IGNORE || f08_status == MPI_F08_STATUSES_IGNORE) {
        return MPI_ERR_ARG;
    }
    /* f_status and f08_status are always byte-equivalent */
    *(MPI_F08_status *) f_status = *f08_status;
    return MPI_SUCCESS;
}

int MPIR_Status_f082c_impl(const MPI_F08_status * f08_status, MPI_Status * c_status)
{
    if (f08_status == MPI_F08_STATUS_IGNORE || f08_status == MPI_F08_STATUSES_IGNORE) {
        return MPI_ERR_ARG;
    }
#ifdef HAVE_FINT_IS_INT
    *c_status = *(MPI_Status *) f08_status;
#else
    /* cast c_status as int array, f08_status as MPI_Fint array */
    int *p = (int *) c_status;
    int *q = (MPI_Fint *) f08_status;
    for (int i = 0; i < MPI_F_STATUS_SIZE; i++) {
        p[i] = (int) q[i];
    }
#endif
    return MPI_SUCCESS;
}

int MPIR_Status_c2f08_impl(const MPI_Status * c_status, MPI_F08_status * f08_status)
{
    if (c_status == MPI_STATUS_IGNORE || c_status == MPI_STATUSES_IGNORE) {
        return MPI_ERR_ARG;
    }
#ifdef HAVE_FINT_IS_INT
    *(MPI_Status *) f08_status = *c_status;
#else
    /* cast c_status as int array, f08_status as MPI_Fint array */
    int *p = (int *) c_status;
    int *q = (MPI_Fint *) f08_status;
    for (int i = 0; i < MPI_F_STATUS_SIZE; i++) {
        q[i] = (MPI_Fint) p[i];
    }
#endif
    return MPI_SUCCESS;
}

/* provide all the _f2c/c2f functions */
/* TODO: proper weak symbol support */
MPI_Fint MPI_Comm_c2f(MPI_Comm comm)
{
    return (MPI_Fint) MPI_Comm_toint(comm);
}

MPI_Comm MPI_Comm_f2c(MPI_Fint comm)
{
    return MPI_Comm_fromint(comm);
}

MPI_Fint PMPI_Comm_c2f(MPI_Comm comm)
{
    return (MPI_Fint) MPI_Comm_toint(comm);
}

MPI_Comm PMPI_Comm_f2c(MPI_Fint comm)
{
    return MPI_Comm_fromint(comm);
}

MPI_Fint MPI_Errhandler_c2f(MPI_Errhandler errhandler)
{
    return (MPI_Fint) MPI_Errhandler_toint(errhandler);
}

MPI_Errhandler MPI_Errhandler_f2c(MPI_Fint errhandler)
{
    return MPI_Errhandler_fromint(errhandler);
}

MPI_Fint PMPI_Errhandler_c2f(MPI_Errhandler errhandler)
{
    return (MPI_Fint) MPI_Errhandler_toint(errhandler);
}

MPI_Errhandler PMPI_Errhandler_f2c(MPI_Fint errhandler)
{
    return MPI_Errhandler_fromint(errhandler);
}

MPI_Fint MPI_File_c2f(MPI_File file)
{
    return (MPI_Fint) MPI_File_toint(file);
}

MPI_File MPI_File_f2c(MPI_Fint file)
{
    return MPI_File_fromint(file);
}

MPI_Fint PMPI_File_c2f(MPI_File file)
{
    return (MPI_Fint) MPI_File_toint(file);
}

MPI_File PMPI_File_f2c(MPI_Fint file)
{
    return MPI_File_fromint(file);
}

MPI_Fint MPI_Group_c2f(MPI_Group group)
{
    return (MPI_Fint) MPI_Group_toint(group);
}

MPI_Group MPI_Group_f2c(MPI_Fint group)
{
    return MPI_Group_fromint(group);
}

MPI_Fint PMPI_Group_c2f(MPI_Group group)
{
    return (MPI_Fint) MPI_Group_toint(group);
}

MPI_Group PMPI_Group_f2c(MPI_Fint group)
{
    return MPI_Group_fromint(group);
}

MPI_Fint MPI_Info_c2f(MPI_Info info)
{
    return (MPI_Fint) MPI_Info_toint(info);
}

MPI_Info MPI_Info_f2c(MPI_Fint info)
{
    return MPI_Info_fromint(info);
}

MPI_Fint PMPI_Info_c2f(MPI_Info info)
{
    return (MPI_Fint) MPI_Info_toint(info);
}

MPI_Info PMPI_Info_f2c(MPI_Fint info)
{
    return MPI_Info_fromint(info);
}

MPI_Fint MPI_Message_c2f(MPI_Message message)
{
    return (MPI_Fint) MPI_Message_toint(message);
}

MPI_Message MPI_Message_f2c(MPI_Fint message)
{
    return MPI_Message_fromint(message);
}

MPI_Fint PMPI_Message_c2f(MPI_Message message)
{
    return (MPI_Fint) MPI_Message_toint(message);
}

MPI_Message PMPI_Message_f2c(MPI_Fint message)
{
    return MPI_Message_fromint(message);
}

MPI_Fint MPI_Op_c2f(MPI_Op op)
{
    return (MPI_Fint) MPI_Op_toint(op);
}

MPI_Op MPI_Op_f2c(MPI_Fint op)
{
    return MPI_Op_fromint(op);
}

MPI_Fint PMPI_Op_c2f(MPI_Op op)
{
    return (MPI_Fint) MPI_Op_toint(op);
}

MPI_Op PMPI_Op_f2c(MPI_Fint op)
{
    return MPI_Op_fromint(op);
}

MPI_Fint MPI_Request_c2f(MPI_Request request)
{
    return (MPI_Fint) MPI_Request_toint(request);
}

MPI_Request MPI_Request_f2c(MPI_Fint request)
{
    return MPI_Request_fromint(request);
}

MPI_Fint PMPI_Request_c2f(MPI_Request request)
{
    return (MPI_Fint) MPI_Request_toint(request);
}

MPI_Request PMPI_Request_f2c(MPI_Fint request)
{
    return MPI_Request_fromint(request);
}

MPI_Fint MPI_Session_c2f(MPI_Session session)
{
    return (MPI_Fint) MPI_Session_toint(session);
}

MPI_Session MPI_Session_f2c(MPI_Fint session)
{
    return MPI_Session_fromint(session);
}

MPI_Fint PMPI_Session_c2f(MPI_Session session)
{
    return (MPI_Fint) MPI_Session_toint(session);
}

MPI_Session PMPI_Session_f2c(MPI_Fint session)
{
    return MPI_Session_fromint(session);
}

MPI_Fint MPI_Type_c2f(MPI_Datatype datatype)
{
    return (MPI_Fint) MPI_Type_toint(datatype);
}

MPI_Datatype MPI_Type_f2c(MPI_Fint datatype)
{
    return MPI_Type_fromint(datatype);
}

MPI_Fint PMPI_Type_c2f(MPI_Datatype datatype)
{
    return (MPI_Fint) MPI_Type_toint(datatype);
}

MPI_Datatype PMPI_Type_f2c(MPI_Fint datatype)
{
    return MPI_Type_fromint(datatype);
}

MPI_Fint MPI_Win_c2f(MPI_Win win)
{
    return (MPI_Fint) MPI_Win_toint(win);
}

MPI_Win MPI_Win_f2c(MPI_Fint win)
{
    return MPI_Win_fromint(win);
}

MPI_Fint PMPI_Win_c2f(MPI_Win win)
{
    return (MPI_Fint) MPI_Win_toint(win);
}

MPI_Win PMPI_Win_f2c(MPI_Fint win)
{
    return MPI_Win_fromint(win);
}

#ifdef MPICH_HAS_MPIX
MPI_Fint MPIX_Stream_c2f(MPIX_Stream stream)
{
    return (MPI_Fint) MPIX_Stream_toint(stream);
}

MPIX_Stream MPIX_Stream_f2c(MPI_Fint stream)
{
    return MPIX_Stream_fromint(stream);
}

MPI_Fint PMPIX_Stream_c2f(MPIX_Stream stream)
{
    return (MPI_Fint) MPIX_Stream_toint(stream);
}

MPIX_Stream PMPIX_Stream_f2c(MPI_Fint stream)
{
    return MPIX_Stream_fromint(stream);
}
#endif

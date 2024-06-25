#ifndef MPI_ABI_FORT_H
#define MPI_ABI_FORT_H

/* This header includes Fortran-related MPI API. It should be provided by
 * an MPI Fortran binding implementation. However, we expect an implementation
 * use this exact header. The constants must follow MPI ABI standard.
 */

/* MPI_Fint must match the Fortran default INTEGER kind. */
/* It is often equivalent to C int but most compilers support wider options. */
#if !defined(MPI_ABI_Fint)
#define MPI_ABI_Fint int
#endif
typedef MPI_ABI_Fint MPI_Fint;
#undef  MPI_ABI_Fint

#define MPI_LOGICAL                    ((MPI_Datatype)0x00000218)
#define MPI_INTEGER                    ((MPI_Datatype)0x00000219)
#define MPI_REAL                       ((MPI_Datatype)0x0000021a)
#define MPI_COMPLEX                    ((MPI_Datatype)0x0000021b)
#define MPI_DOUBLE_PRECISION           ((MPI_Datatype)0x0000021c)
#define MPI_DOUBLE_COMPLEX             ((MPI_Datatype)0x0000021d)

#define MPI_2REAL                      ((MPI_Datatype)0x00000230)
#define MPI_2DOUBLE_PRECISION          ((MPI_Datatype)0x00000231)
#define MPI_2INTEGER                   ((MPI_Datatype)0x00000232)

#define MPIX_LOGICAL1                  ((MPI_Datatype)0x000002c0)
#define MPI_INTEGER1                   ((MPI_Datatype)0x000002c1)
#define MPIX_REAL1                     ((MPI_Datatype)0x000002c2)
#define MPI_CHARACTER                  ((MPI_Datatype)0x000002c3)
#define MPIX_LOGICAL2                  ((MPI_Datatype)0x000002c8)
#define MPI_INTEGER2                   ((MPI_Datatype)0x000002c9)
#define MPI_REAL2                      ((MPI_Datatype)0x000002ca)
#define MPIX_LOGICAL4                  ((MPI_Datatype)0x000002d0)
#define MPI_INTEGER4                   ((MPI_Datatype)0x000002d1)
#define MPI_REAL4                      ((MPI_Datatype)0x000002d2)
#define MPI_COMPLEX4                   ((MPI_Datatype)0x000002d3)
#define MPIX_LOGICAL8                  ((MPI_Datatype)0x000002d8)
#define MPI_INTEGER8                   ((MPI_Datatype)0x000002d9)
#define MPI_REAL8                      ((MPI_Datatype)0x000002da)
#define MPI_COMPLEX8                   ((MPI_Datatype)0x000002db)
#define MPIX_LOGICAL16                 ((MPI_Datatype)0x000002e0)
#define MPI_INTEGER16                  ((MPI_Datatype)0x000002e1)
#define MPI_REAL16                     ((MPI_Datatype)0x000002e2)
#define MPI_COMPLEX16                  ((MPI_Datatype)0x000002e3)
#define MPI_COMPLEX32                  ((MPI_Datatype)0x000002eb)

/* Fortran 1977 Status Size and Indices */
enum {
    MPI_F_STATUS_SIZE = 8,
    MPI_F_SOURCE = 0,
    MPI_F_TAG = 1,
    MPI_F_ERROR = 2
};

/* Fortran 2008 Status Type */
typedef struct {
    MPI_Fint MPI_SOURCE;
    MPI_Fint MPI_TAG;
    MPI_Fint MPI_ERROR;
    MPI_Fint MPI_internal[5];
} MPI_F08_status;

/* MPI global variables */
extern MPI_Fint *MPI_F_STATUS_IGNORE;
extern MPI_Fint *MPI_F_STATUSES_IGNORE;
extern MPI_F08_status *MPI_F08_STATUS_IGNORE;
extern MPI_F08_status *MPI_F08_STATUSES_IGNORE;

int MPI_Status_c2f(const MPI_Status * c_status, MPI_Fint * f_status);
int MPI_Status_f2c(const MPI_Fint * f_status, MPI_Status * c_status);
int MPI_Status_c2f08(const MPI_Status * c_status, MPI_F08_status * f08_status);
int MPI_Status_f082c(const MPI_F08_status * f08_status, MPI_Status * c_status);
int MPI_Status_f2f08(const MPI_Fint * f_status, MPI_F08_status * f08_status);
int MPI_Status_f082f(const MPI_F08_status * f08_status, MPI_Fint * f_status);
MPI_Fint MPI_Comm_c2f(MPI_Comm comm);
MPI_Comm MPI_Comm_f2c(MPI_Fint comm);
MPI_Fint MPI_Errhandler_c2f(MPI_Errhandler errhandler);
MPI_Errhandler MPI_Errhandler_f2c(MPI_Fint errhandler);
MPI_Fint MPI_Type_c2f(MPI_Datatype datatype);
MPI_Datatype MPI_Type_f2c(MPI_Fint datatype);
MPI_Fint MPI_File_c2f(MPI_File file);
MPI_File MPI_File_f2c(MPI_Fint file);
MPI_Fint MPI_Group_c2f(MPI_Group group);
MPI_Group MPI_Group_f2c(MPI_Fint group);
MPI_Fint MPI_Info_c2f(MPI_Info info);
MPI_Info MPI_Info_f2c(MPI_Fint info);
MPI_Fint MPI_Message_c2f(MPI_Message message);
MPI_Message MPI_Message_f2c(MPI_Fint message);
MPI_Fint MPI_Op_c2f(MPI_Op op);
MPI_Op MPI_Op_f2c(MPI_Fint op);
MPI_Fint MPI_Request_c2f(MPI_Request request);
MPI_Request MPI_Request_f2c(MPI_Fint request);
MPI_Fint MPI_Session_c2f(MPI_Session session);
MPI_Session MPI_Session_f2c(MPI_Fint session);
MPI_Fint MPI_Win_c2f(MPI_Win win);
MPI_Win MPI_Win_f2c(MPI_Fint win);

int PMPI_Status_c2f(const MPI_Status * c_status, MPI_Fint * f_status);
int PMPI_Status_f2c(const MPI_Fint * f_status, MPI_Status * c_status);
int PMPI_Status_c2f08(const MPI_Status * c_status, MPI_F08_status * f08_status);
int PMPI_Status_f082c(const MPI_F08_status * f08_status, MPI_Status * c_status);
int PMPI_Status_f2f08(const MPI_Fint * f_status, MPI_F08_status * f08_status);
int PMPI_Status_f082f(const MPI_F08_status * f08_status, MPI_Fint * f_status);
MPI_Fint PMPI_Comm_c2f(MPI_Comm comm);
MPI_Comm PMPI_Comm_f2c(MPI_Fint comm);
MPI_Fint PMPI_Errhandler_c2f(MPI_Errhandler errhandler);
MPI_Errhandler PMPI_Errhandler_f2c(MPI_Fint errhandler);
MPI_Fint PMPI_Type_c2f(MPI_Datatype datatype);
MPI_Datatype PMPI_Type_f2c(MPI_Fint datatype);
MPI_Fint PMPI_File_c2f(MPI_File file);
MPI_File PMPI_File_f2c(MPI_Fint file);
MPI_Fint PMPI_Group_c2f(MPI_Group group);
MPI_Group PMPI_Group_f2c(MPI_Fint group);
MPI_Fint PMPI_Info_c2f(MPI_Info info);
MPI_Info PMPI_Info_f2c(MPI_Fint info);
MPI_Fint PMPI_Message_c2f(MPI_Message message);
MPI_Message PMPI_Message_f2c(MPI_Fint message);
MPI_Fint PMPI_Op_c2f(MPI_Op op);
MPI_Op PMPI_Op_f2c(MPI_Fint op);
MPI_Fint PMPI_Request_c2f(MPI_Request request);
MPI_Request PMPI_Request_f2c(MPI_Fint request);
MPI_Fint PMPI_Session_c2f(MPI_Session session);
MPI_Session PMPI_Session_f2c(MPI_Fint session);
MPI_Fint PMPI_Win_c2f(MPI_Win win);
MPI_Win PMPI_Win_f2c(MPI_Fint win);

#endif /* MPI_ABI_FORT_H */

/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
/*
 * This file contains the C routines used in testing the c2f and f2c
 * handle conversion functions, except for MPI_File and MPI_Win (to
 * allow working with MPI implementations that do not include those
 * features).
 *
 * The tests follow this pattern:
 *
 *  Fortran main program
 *     calls c routine with each handle type, with a prepared
 *     and valid handle (often requires constructing an object)
 *
 *     C routine uses xxx_f2c routine to get C handle, checks some
 *     properties (i.e., size and rank of communicator, contents of datatype)
 *
 *     Then the Fortran main program calls a C routine that provides
 *     a handle, and the Fortran program performs similar checks.
 *
 * We also assume that a C int is a Fortran integer.  If this is not the
 * case, these tests must be modified.
 */

/* style: allow:fprintf:10 sig:0 */
#include <stdio.h>
#include "mpi.h"
#include "../../include/mpitestconf.h"
#include <string.h>

/*
   Name mapping.  All routines are created with names that are lower case
   with a single trailing underscore.  This matches many compilers.
   We use #define to change the name for Fortran compilers that do
   not use the lowercase/underscore pattern
*/

#ifdef F77_NAME_UPPER
#define c2fcomm_ C2FCOMM
#define c2fgroup_ C2FGROUP
#define c2ftype_ C2FTYPE
#define c2finfo_ C2FINFO
#define c2frequest_ C2FREQUEST
#define c2fop_ C2FOP
#define c2ferrhandler_ C2FERRHANDLER

#define f2ccomm_ F2CCOMM
#define f2cgroup_ F2CGROUP
#define f2ctype_ F2CTYPE
#define f2cinfo_ F2CINFO
#define f2crequest_ F2CREQUEST
#define f2cop_ F2COP
#define f2cerrhandler_ F2CERRHANDLER

#elif defined(F77_NAME_LOWER) || defined(F77_NAME_MIXED)
/* Mixed is ok because we use lowercase in all uses */
#define c2fcomm_ c2fcomm
#define c2fgroup_ c2fgroup
#define c2ftype_ c2ftype
#define c2finfo_ c2finfo
#define c2frequest_ c2frequest
#define c2fop_ c2fop
#define c2ferrhandler_ c2ferrhandler

#define f2ccomm_ f2ccomm
#define f2cgroup_ f2cgroup
#define f2ctype_ f2ctype
#define f2cinfo_ f2cinfo
#define f2crequest_ f2crequest
#define f2cop_ f2cop
#define f2cerrhandler_ f2cerrhandler

#elif defined(F77_NAME_LOWER_2USCORE) || defined(F77_NAME_LOWER_USCORE) || \
      defined(F77_NAME_MIXED_USCORE)
/* Else leave name alone (routines have no underscore, so both
   of these map to a lowercase, single underscore) */
#else
#error 'Unrecognized Fortran name mapping'
#endif

/* Prototypes to keep compilers happy */
MPI_Fint c2fcomm_(MPI_Fint *);
MPI_Fint c2fgroup_(MPI_Fint *);
MPI_Fint c2finfo_(MPI_Fint *);
MPI_Fint c2frequest_(MPI_Fint *);
MPI_Fint c2ftype_(MPI_Fint *);
MPI_Fint c2fop_(MPI_Fint *);
MPI_Fint c2ferrhandler_(MPI_Fint *);

void f2ccomm_(MPI_Fint *);
void f2cgroup_(MPI_Fint *);
void f2cinfo_(MPI_Fint *);
void f2crequest_(MPI_Fint *);
void f2ctype_(MPI_Fint *);
void f2cop_(MPI_Fint *);
void f2cerrhandler_(MPI_Fint *);


MPI_Fint c2fcomm_(MPI_Fint * comm)
{
    MPI_Comm cComm = MPI_Comm_f2c(*comm);
    int cSize, wSize, cRank, wRank;

    MPI_Comm_size(MPI_COMM_WORLD, &wSize);
    MPI_Comm_rank(MPI_COMM_WORLD, &wRank);
    MPI_Comm_size(cComm, &cSize);
    MPI_Comm_rank(cComm, &cRank);

    if (wSize != cSize || wRank != cRank) {
        fprintf(stderr, "Comm: Did not get expected size,rank (got %d,%d)", cSize, cRank);
        return 1;
    }
    return 0;
}

MPI_Fint c2fgroup_(MPI_Fint * group)
{
    MPI_Group cGroup = MPI_Group_f2c(*group);
    int cSize, wSize, cRank, wRank;

    /* We pass in the group of comm world */
    MPI_Comm_size(MPI_COMM_WORLD, &wSize);
    MPI_Comm_rank(MPI_COMM_WORLD, &wRank);
    MPI_Group_size(cGroup, &cSize);
    MPI_Group_rank(cGroup, &cRank);

    if (wSize != cSize || wRank != cRank) {
        fprintf(stderr, "Group: Did not get expected size,rank (got %d,%d)", cSize, cRank);
        return 1;
    }
    return 0;
}

MPI_Fint c2ftype_(MPI_Fint * type)
{
    MPI_Datatype dtype = MPI_Type_f2c(*type);

    if (dtype != MPI_INTEGER) {
        fprintf(stderr, "Type: Did not get expected type\n");
        return 1;
    }
    return 0;
}

MPI_Fint c2finfo_(MPI_Fint * info)
{
    MPI_Info cInfo = MPI_Info_f2c(*info);
    int flag;
    char value[100];
    MPI_Fint errs = 0;

    MPI_Info_get(cInfo, (char *) "host", sizeof(value), value, &flag);
    if (!flag || strcmp(value, "myname") != 0) {
        fprintf(stderr, "Info: Wrong value or no value for host\n");
        errs++;
    }
    MPI_Info_get(cInfo, (char *) "wdir", sizeof(value), value, &flag);
    if (!flag || strcmp(value, "/rdir/foo") != 0) {
        fprintf(stderr, "Info: Wrong value of no value for wdir\n");
        errs++;
    }

    return errs;
}

MPI_Fint c2frequest_(MPI_Fint * request)
{
    MPI_Request req = MPI_Request_f2c(*request);
    MPI_Status status;
    int flag;
    MPI_Test(&req, &flag, &status);
    MPI_Test_cancelled(&status, &flag);
    if (!flag) {
        fprintf(stderr, "Request: Wrong value for flag\n");
        return 1;
    }
    else {
        *request = MPI_Request_c2f(req);
    }
    return 0;
}

MPI_Fint c2fop_(MPI_Fint * op)
{
    MPI_Op cOp = MPI_Op_f2c(*op);

    if (cOp != MPI_SUM) {
        fprintf(stderr, "Op: did not get sum\n");
        return 1;
    }
    return 0;
}

MPI_Fint c2ferrhandler_(MPI_Fint * errh)
{
    MPI_Errhandler errhand = MPI_Errhandler_f2c(*errh);

    if (errhand != MPI_ERRORS_RETURN) {
        fprintf(stderr, "Errhandler: did not get errors return\n");
        return 1;
    }

    return 0;
}

/*
 * The following routines provide handles to the calling Fortran program
 */
void f2ccomm_(MPI_Fint * comm)
{
    *comm = MPI_Comm_c2f(MPI_COMM_WORLD);
}

void f2cgroup_(MPI_Fint * group)
{
    MPI_Group wgroup;
    MPI_Comm_group(MPI_COMM_WORLD, &wgroup);
    *group = MPI_Group_c2f(wgroup);
}

void f2ctype_(MPI_Fint * type)
{
    *type = MPI_Type_c2f(MPI_INTEGER);
}

void f2cinfo_(MPI_Fint * info)
{
    MPI_Info cinfo;

    MPI_Info_create(&cinfo);
    MPI_Info_set(cinfo, (char *) "host", (char *) "myname");
    MPI_Info_set(cinfo, (char *) "wdir", (char *) "/rdir/foo");

    *info = MPI_Info_c2f(cinfo);
}

void f2crequest_(MPI_Fint * req)
{
    MPI_Request cReq;

    MPI_Irecv(NULL, 0, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &cReq);
    MPI_Cancel(&cReq);
    *req = MPI_Request_c2f(cReq);

}

void f2cop_(MPI_Fint * op)
{
    *op = MPI_Op_c2f(MPI_SUM);
}

void f2cerrhandler_(MPI_Fint * errh)
{
    *errh = MPI_Errhandler_c2f(MPI_ERRORS_RETURN);
}

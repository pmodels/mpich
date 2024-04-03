/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpi_fortimpl.h"

/* --------------------- */
struct F77_attr_state {
    int keyval;
    F77_CopyFunction *copyfn;
    F77_DeleteFunction *delfn;
    void *extra_state;
};

static int MPII_copy_proxy(MPI_Comm comm, int keyval, void *extra_state,
                           void *val_in, void *val_out, int *flag)
{
    struct F77_attr_state *p = extra_state;
    MPI_Fint ierr = 0;
    MPI_Fint fhandle = MPI_Comm_c2f(comm);
    MPI_Fint fkeyval = (MPI_Fint) keyval;
    MPI_Fint *fextra = (MPI_Fint *) p->extra_state;
    MPI_Fint fvalue = (MPI_Fint) (intptr_t) val_in;
    MPI_Fint fnew = 0;
    MPI_Fint fflag = 0;

    p->copyfn(&fhandle, &fkeyval, &fextra, &fvalue, &fnew, &fflag, &ierr);

    *flag = MPII_FROM_FLOG(fflag);
    *(void **) val_out = (void *) (intptr_t) fnew;
    return (int) ierr;
}

static int MPII_del_proxy(MPI_Comm comm, int keyval, void *value, void *extra_state)
{
    struct F77_attr_state *p = extra_state;
    MPI_Fint ierr = 0;
    MPI_Fint fhandle = MPI_Comm_c2f(comm);
    MPI_Fint fkeyval = (MPI_Fint) keyval;
    MPI_Fint fvalue = (int) (intptr_t) value;
    MPI_Fint *fextra = (MPI_Fint *) p->extra_state;

    p->delfn(&fhandle, &fkeyval, &fvalue, fextra, &ierr);
    return (int) ierr;
}

int MPII_keyval_create(F77_CopyFunction * copyfn,
                       F77_DeleteFunction * delfn, void *extra_state, int *keyval)
{
    struct F77_attr_state *p = malloc(sizeof(struct F77_attr_state));
    int ret = MPI_Comm_create_keyval(MPII_copy_proxy,
                                     MPII_del_proxy,
                                     &keyval, p);
    if (ret == MPI_SUCCESS) {
        p->keyval = keyval;
        p->copyfn = copyfn;
        p->delfn = delfn;
        p->extra_state = extra_state;
    }
    return ret;
}

/* --------------------- */
struct F90_attr_state {
    int keyval;
    F90_CopyFunction *copyfn;
    F90_DeleteFunction *delfn;
    void *extra_state;
};

static int MPII_comm_copy_proxy(MPI_Comm comm, int keyval, void *extra_state,
                                void *val_in, void *val_out, int *flag)
{
    struct F90_attr_state *p = extra_state;
    MPI_Fint ierr = 0;
    MPI_Fint fhandle = MPI_Comm_c2f(comm);
    MPI_Fint fkeyval = (MPI_Fint) keyval;
    MPI_Aint *fextra = (MPI_Aint *) p->extra_state;
    MPI_Aint fvalue = (MPI_Aint) (intptr_t) val_in;
    MPI_Aint fnew = 0;
    MPI_Fint fflag = 0;

    p->copyfn(&fhandle, &fkeyval, &fextra, &fvalue, &fnew, &fflag, &ierr);

    *flag = MPII_FROM_FLOG(fflag);
    *(void **) val_out = (void *) (intptr_t) fnew;
    return (int) ierr;
}

static int MPII_comm_del_proxy(MPI_Comm comm, int keyval, void *value, void *extra_state)
{
    struct F90_attr_state *p = extra_state;
    MPI_Fint ierr = 0;
    MPI_Fint fhandle = MPI_Comm_c2f(comm);
    MPI_Fint fkeyval = (MPI_Fint) keyval;
    MPI_Aint fvalue = (MPI_Aint) (intptr_t) value;
    MPI_Aint *fextra = (MPI_Aint *) p->extra_state;

    p->delfn(&fhandle, &fkeyval, &fvalue, fextra, &ierr);
    return (int) ierr;
}

int MPII_comm_keyval_create(F90_CopyFunction * copyfn,
                            F90_DeleteFunction * delfn, void *extra_state, int *keyval)
{
    struct F90_attr_state *p = malloc(sizeof(struct F90_attr_state));
    int ret = MPI_Comm_create_keyval(MPII_comm_copy_proxy,
                                     MPII_comm_del_proxy,
                                     &keyval, p);
    if (ret == MPI_SUCCESS) {
        p->keyval = keyval;
        p->copyfn = copyfn;
        p->delfn = delfn;
        p->extra_state = extra_state;
    } else {
        free(p);
    }
    return ret;
}

static int MPII_type_copy_proxy(MPI_Datatype type, int keyval, void *extra_state,
                                void *val_in, void *val_out, int *flag)
{
    struct F90_attr_state *p = extra_state;
    MPI_Fint ierr = 0;
    MPI_Fint fhandle = MPI_Type_c2f(type);
    MPI_Fint fkeyval = (MPI_Fint) keyval;
    MPI_Aint *fextra = (MPI_Aint *) p->extra_state;
    MPI_Aint fvalue = (MPI_Aint) (intptr_t) val_in;
    MPI_Aint fnew = 0;
    MPI_Fint fflag = 0;

    p->copyfn(&fhandle, &fkeyval, &fextra, &fvalue, &fnew, &fflag, &ierr);

    *flag = MPII_FROM_FLOG(fflag);
    *(void **) val_out = (void *) (intptr_t) fnew;
    return (int) ierr;
}

static int MPII_type_del_proxy(MPI_Datatype type, int keyval, void *value, void *extra_state)
{
    struct F90_attr_state *p = extra_state;
    MPI_Fint ierr = 0;
    MPI_Fint fhandle = MPI_Type_c2f(type);
    MPI_Fint fkeyval = (MPI_Fint) keyval;
    MPI_Aint fvalue = (MPI_Aint) (intptr_t) value;
    MPI_Aint *fextra = (MPI_Aint *) p->extra_state;

    p->delfn(&fhandle, &fkeyval, &fvalue, fextra, &ierr);
    return (int) ierr;
}

int MPII_type_keyval_create(F90_CopyFunction * copyfn,
                            F90_DeleteFunction * delfn, void *extra_state, int *keyval)
{
    struct F90_attr_state *p = malloc(sizeof(struct F90_attr_state));
    int ret = MPI_Type_create_keyval(MPII_type_copy_proxy,
                                     MPII_type_del_proxy,
                                     &keyval, p);
    if (ret == MPI_SUCCESS) {
        p->keyval = keyval;
        p->copyfn = copyfn;
        p->delfn = delfn;
        p->extra_state = extra_state;
    } else {
        free(p);
    }
    return ret;
}

static int MPII_win_copy_proxy(MPI_Win win, int keyval, void *extra_state,
                               void *val_in, void *val_out, int *flag)
{
    struct F90_attr_state *p = extra_state;
    MPI_Fint ierr = 0;
    MPI_Fint fhandle = MPI_Win_c2f(win);
    MPI_Fint fkeyval = (MPI_Fint) keyval;
    MPI_Aint *fextra = (MPI_Aint *) p->extra_state;
    MPI_Aint fvalue = (MPI_Aint) (intptr_t) val_in;
    MPI_Aint fnew = 0;
    MPI_Fint fflag = 0;

    p->copyfn(&fhandle, &fkeyval, &fextra, &fvalue, &fnew, &fflag, &ierr);

    *flag = MPII_FROM_FLOG(fflag);
    *(void **) val_out = (void *) (intptr_t) fnew;
    return (int) ierr;
}

static int MPII_win_del_proxy(MPI_Win win, int keyval, void *value, void *extra_state)
{
    struct F90_attr_state *p = extra_state;
    MPI_Fint ierr = 0;
    MPI_Fint fhandle = MPI_Win_c2f(win);
    MPI_Fint fkeyval = (MPI_Fint) keyval;
    MPI_Aint fvalue = (MPI_Aint) (intptr_t) value;
    MPI_Aint *fextra = (MPI_Aint *) p->extra_state;

    p->delfn(&fhandle, &fkeyval, &fvalue, fextra, &ierr);
    return (int) ierr;
}

int MPII_win_keyval_create(F90_CopyFunction * copyfn,
                           F90_DeleteFunction * delfn, void *extra_state, int *keyval)
{
    struct F90_attr_state *p = malloc(sizeof(struct F90_attr_state));
    int ret = MPI_Win_create_keyval(MPII_win_copy_proxy,
                                    MPII_win_del_proxy,
                                    &keyval, p);
    if (ret == MPI_SUCCESS) {
        p->keyval = keyval;
        p->copyfn = copyfn;
        p->delfn = delfn;
        p->extra_state = extra_state;
    } else {
        free(p);
    }
    return ret;
}

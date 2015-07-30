/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2012 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include "mpi.h"
#include <stdlib.h>
#include <string.h>
#include "../../include/mpitestconf.h"

/* Used to convert Fortran strings (which may not be null terminated) to
   C strings */
#define MAX_ATTRTEST_MSG 256

/*
 * FIXME: This code assumes that character strings are passed from Fortran
 * by placing the string length, as an int, at the end of the argument list
 * This is common but not universal.
 */

/*
   Name mapping.  All routines are created with names that are lower case
   with a single trailing underscore.  This matches many compilers.
   We use #define to change the name for Fortran compilers that do
   not use the lowercase/underscore pattern
*/

#ifdef F77_NAME_UPPER
#define cattrinit_   CATTRINIT
#define cgetenvbool_ CGETENVBOOL
#define cgetsizes_   CGETSIZES
#define ccreatekeys_ CCREATEKEYS
#define cfreekeys_   CFREEKEYS
#define ctoctest_    CTOCTEST
#define cmpif1read_  CMPIF1READ
#define cmpif2read_  CMPIF2READ
#define cmpif2readtype_  CMPIF2READTYPE
#define cmpif2readwin_   CMPIF2READWIN
#define csetmpi_     CSETMPI
#define csetmpi2_    CSETMPI2
#define csetmpitype_ CSETMPITYPE
#define csetmpiwin_  CSETMPIWIN

#elif defined(F77_NAME_LOWER) || defined(F77_NAME_MIXED)
/* Mixed is ok because we use lowercase in all uses */
#define cattrinit_   cattrinit
#define cgetenvbool_ cgetenvbool
#define cgetsizes_   cgetsizes
#define ccreatekeys_ ccreatekeys
#define cfreekeys_   cfreekeys
#define ctoctest_    ctoctest
#define cmpif1read_  cmpif1read
#define cmpif2read_  cmpif2read
#define cmpif2readtype_  cmpif2readtype
#define cmpif2readwin_   cmpif2readwin
#define csetmpi_     csetmpi
#define csetmpi2_    csetmpi2
#define csetmpitype_ csetmpitype
#define csetmpiwin_  csetmpiwin

#elif defined(F77_NAME_LOWER_2USCORE) || defined(F77_NAME_LOWER_USCORE) || \
      defined(F77_NAME_MIXED_USCORE)
/* Else leave name alone (routines have no underscore, so both
   of these map to a lowercase, single underscore) */
#else
#error 'Unrecognized Fortran name mapping'
#endif

/* */
static int ccomm1Key, ccomm2Key, ctype2Key, cwin2Key;
static int ccomm1Extra, ccomm2Extra, ctype2Extra, cwin2Extra;
static int verbose = 0;

/* Forward references */
int cmpi1read(MPI_Comm comm, int key, void *expected, const char *msg);
int cmpi2read(MPI_Comm comm, int key, void *expected, const char *msg);
int cmpi2readtype(MPI_Datatype dtype, int key, void *expected, const char *msg);

void ccompareint2aint_(MPI_Fint * in1, MPI_Aint * in2, MPI_Fint * result);
void ccompareint2void_(MPI_Fint * in1, void *in2, MPI_Fint * result);
void ccompareaint2void_(MPI_Aint * in1, void *in2, MPI_Fint * result);

/* ----------------------------------------------------------------------- */
/* Initialization functions                                                */
/* ----------------------------------------------------------------------- */
void cgetenvbool_(const char str[], MPI_Fint * val, int d)
{
    const char *envval;
    char envname[1024];
    /* Note that the Fortran string may not be null terminated; thus
     * we copy d characters and add a null just in case */
    if (d > sizeof(envname) - 1) {
        fprintf(stderr, "Environment variable name too long (%d)\n", d);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    strncpy(envname, str, d);
    envname[d] = 0;

    envval = getenv(envname);
    *val = 0;
    if (envval) {
        printf(" envval = %s\n", envval);
        if (strcmp(envval, "yes") == 0 || strcmp(envval, "YES") == 0 ||
            strcmp(envval, "true") == 0 || strcmp(envval, "TRUE") == 0)
            *val = 1;
    }
}

/* Keep our own copy of the "is verbose" state */
void cattrinit_(MPI_Fint * fverbose)
{
    verbose = (int) *fverbose;
}

/* Provide attribute sizes (C, Fortran 1, Fortran 2) */
void cgetsizes_(MPI_Fint * ptrSize, MPI_Fint * intSize, MPI_Fint * aintSize)
{
    *ptrSize = (MPI_Fint) sizeof(void *);
    *intSize = (MPI_Fint) sizeof(MPI_Fint);
    *aintSize = (MPI_Fint) sizeof(MPI_Aint);
}

/* ----------------------------------------------------------------------- */
/* Copy and delete functions attached to keyvals                           */
/* ----------------------------------------------------------------------- */
static int CMPI1_COPY_FN(MPI_Comm comm, int keyval, void *extra,
                         void *inval, void *outval, int *flag)
{
    int inValue = *(int *) inval;

    if (verbose)
        printf(" In C MPI-1 copy function: inval = %p, extra = %p\n", inval, extra);
    *flag = 1;
    /* We don't change the attribute */
    *(void **) outval = inval;
    /* But we do change what it points at */
    *(int *) inval = inValue + 1;
    return MPI_SUCCESS;
}

static int CMPI1_DELETE_FN(MPI_Comm comm, int keyval, void *outval, void *extra)
{
    if (verbose)
        printf(" In C MPI-1 delete function, extra = %p\n", extra);
    *(int *) outval = *(int *) outval - 1;
    return MPI_SUCCESS;
}

static int TYPE_COPY_FN(MPI_Datatype dtype, int keyval, void *extra,
                        void *inval, void *outval, int *flag)
{
    int inValue = *(int *) inval;

    if (verbose)
        printf(" In C MPI type copy function, inval = %p, extra = %p\n", inval, extra);
    *flag = 1;
    /* We don't change the attribute */
    *(void **) outval = inval;
    /* But we do change what it points at */
    *(int *) inval = inValue + 1;
    return MPI_SUCCESS;
}

static int TYPE_DELETE_FN(MPI_Datatype dtype, int keyval, void *outval, void *extra)
{
    if (verbose)
        printf(" In C MPI type delete function, extra = %p\n", extra);
    /* We reverse the incrment used in copy (checked after free of the type) */
    *(int *) outval = *(int *) outval - 1;
    return MPI_SUCCESS;
}

/* Note that this function cannot be called in MPI since there is no
   win_dup function */
static int WIN_COPY_FN(MPI_Win win, int keyval, void *extra, void *inval, void *outval, int *flag)
{
    int inValue = *(int *) inval;

    if (verbose)
        printf("PANIC: In C MPI win copy function (should never happen)\n");
    *flag = 1;
    return MPI_SUCCESS;
}

static int WIN_DELETE_FN(MPI_Win win, int keyval, void *outval, void *extra)
{
    if (verbose)
        printf(" In C MPI win delete function, extra = %p\n", extra);
    *(int *) outval = *(int *) outval - 1;
    return MPI_SUCCESS;
}

/* ----------------------------------------------------------------------- */
/* Routines to create keyvals in C (with C copy and delete functions       */
/* ----------------------------------------------------------------------- */

void ccreatekeys_(MPI_Fint * ccomm1_key, MPI_Fint * ccomm2_key,
                  MPI_Fint * ctype2_key, MPI_Fint * cwin2_key)
{
    MPI_Keyval_create(CMPI1_COPY_FN, CMPI1_DELETE_FN, &ccomm1Key, &ccomm1Extra);
    *ccomm1_key = (MPI_Fint) ccomm1Key;

    MPI_Comm_create_keyval(CMPI1_COPY_FN, CMPI1_DELETE_FN, &ccomm2Key, &ccomm2Extra);
    *ccomm2_key = (MPI_Fint) ccomm2Key;

    MPI_Type_create_keyval(TYPE_COPY_FN, TYPE_DELETE_FN, &ctype2Key, &ctype2Extra);
    *ctype2_key = (MPI_Fint) ctype2Key;

    MPI_Win_create_keyval(WIN_COPY_FN, WIN_DELETE_FN, &cwin2Key, &cwin2Extra);
    *cwin2_key = (MPI_Fint) cwin2Key;
}

void cfreekeys_(void)
{
    MPI_Keyval_free(&ccomm1Key);
    MPI_Comm_free_keyval(&ccomm2Key);
    MPI_Type_free_keyval(&ctype2Key);
    MPI_Win_free_keyval(&cwin2Key);
}

/* ----------------------------------------------------------------------- */
/* ----------------------------------------------------------------------- */

/* Test c-to-c attributes */
static int ccomm1Attr, ccomm2Attr, ctype2Attr, cwin2Attr;

void ctoctest_(MPI_Fint * errs)
{
    int errcnt = *errs;
    int baseattrval = (1 << (sizeof(int) * 8 - 2)) - 3;
    MPI_Datatype cduptype;
    MPI_Comm cdup;

    /* MPI-1 function */
    ccomm1Attr = baseattrval;
    MPI_Attr_put(MPI_COMM_SELF, ccomm1Key, &ccomm1Attr);
    /* Test that we have the same value */
    errcnt += cmpi1read(MPI_COMM_SELF, ccomm1Key, &ccomm1Attr, "C to C");

    /* Dup, check that the copy routine does what is expected */
    MPI_Comm_dup(MPI_COMM_SELF, &cdup);
    errcnt += cmpi1read(cdup, ccomm1Key, &ccomm1Attr, "C to C dup");
    if (ccomm1Attr != baseattrval + 1) {
        printf(" Did not increment int in C to C dup: %d %d\n", ccomm1Attr, baseattrval + 1);
        errcnt++;
    }

    MPI_Comm_free(&cdup);
    if (ccomm1Attr != baseattrval) {
        printf(" Did not increment int in C to C delete: %d %d\n", ccomm1Attr, baseattrval);
        errcnt++;
    }

    /* MPI-2 functions */
    ccomm1Attr = 0;
    ccomm2Attr = baseattrval;
    MPI_Comm_set_attr(MPI_COMM_SELF, ccomm2Key, &ccomm2Attr);
    /* Test that we have the same value */
    errcnt += cmpi2read(MPI_COMM_SELF, ccomm2Key, &ccomm2Attr, "C to C (2)");

    /* Dup, check that the copy routine does what is expected */
    MPI_Comm_dup(MPI_COMM_SELF, &cdup);
    errcnt += cmpi2read(cdup, ccomm2Key, &ccomm2Attr, "C to C dup (2)");
    if (ccomm2Attr != baseattrval + 1) {
        printf(" Did not increment int in C to C dup: %d %d\n", ccomm2Attr, baseattrval + 1);
        errcnt++;
    }

    MPI_Comm_free(&cdup);
    if (ccomm2Attr != baseattrval) {
        printf(" Did not increment int in C to C delete (2): %d %d\n", ccomm2Attr, baseattrval);
        errcnt++;
    }

    /* MPI-2 functions */
    ctype2Attr = baseattrval;
    MPI_Type_set_attr(MPI_INTEGER, ctype2Key, &ctype2Attr);
    /* Test that we have the same value */
    errcnt += cmpi2readtype(MPI_INTEGER, ctype2Key, &ctype2Attr, "C to C type (2)");

    /* Dup, check that the copy routine does what is expected */
    MPI_Type_dup(MPI_INTEGER, &cduptype);
    errcnt += cmpi2readtype(cduptype, ctype2Key, &ctype2Attr, "C to C typedup (2)");
    if (ctype2Attr != baseattrval + 1) {
        printf(" Did not increment int in C to C typedup: %d %d\n", ctype2Attr, baseattrval + 1);
        errcnt++;
    }
    ccomm1Attr = 0;

    MPI_Type_free(&cduptype);
    if (ctype2Attr != baseattrval) {
        printf(" Did not increment int in C to C typedelete (2): %d %d\n", ctype2Attr, baseattrval);
        errcnt++;
    }


    *errs = errcnt;
}

/* ----------------------------------------------------------------------- */
/* Routines to get and check an attribute value.  Returns the number       */
/*   of errors found                                                       */
/* ----------------------------------------------------------------------- */

int cmpi1read(MPI_Comm comm, int key, void *expected, const char *msg)
{
    void *attrval;
    int flag;
    MPI_Attr_get(comm, key, &attrval, &flag);
    if (!flag) {
        printf(" Error: flag false for Attr_get: %s\n", msg);
        return 1;
    }
    if (attrval != expected) {
        printf(" Error: expected %p but saw %p: %s\n", expected, attrval, msg);
        return 1;
    }
    return 0;
}

int cmpi2read(MPI_Comm comm, int key, void *expected, const char *msg)
{
    void *attrval;
    int flag;
    MPI_Comm_get_attr(comm, key, &attrval, &flag);
    if (!flag) {
        printf(" Error: flag false for Comm_get_attr: %s\n", msg);
        return 1;
    }
    if (attrval != expected) {
        printf(" Error: expected %p but saw %p: %s\n", expected, attrval, msg);
        return 1;
    }
    return 0;
}

int cmpi2readtype(MPI_Datatype dtype, int key, void *expected, const char *msg)
{
    void *attrval;
    int flag;
    MPI_Type_get_attr(dtype, key, &attrval, &flag);
    if (!flag) {
        printf(" Error: flag false for Type_get_attr: %s\n", msg);
        return 1;
    }
    if (attrval != expected) {
        printf(" Error: expected %p but saw %p: %s\n", expected, attrval, msg);
        return 1;
    }
    return 0;
}

int cmpi2readwin(MPI_Win win, int key, void *expected, const char *msg)
{
    void *attrval;
    int flag;
    MPI_Win_get_attr(win, key, &attrval, &flag);
    if (!flag) {
        printf(" Error: flag false for Win_get_attr: %s\n", msg);
        return 1;
    }
    if (attrval != expected) {
        printf(" Error: expected %p but saw %p: %s\n", expected, attrval, msg);
        return 1;
    }
    return 0;
}

/* Set in Fortran (MPI-1), read in C */
void cmpif1read_(MPI_Fint * fcomm, MPI_Fint * fkey, MPI_Fint * expected,
                 MPI_Fint * errs, const char *msg, int msglen)
{
    void *attrval;
    int flag, result;
    MPI_Comm comm = MPI_Comm_f2c(*fcomm);
    char lmsg[MAX_ATTRTEST_MSG];

    if (msglen > sizeof(lmsg) - 1) {
        fprintf(stderr, "Message too long for buffer (%d)\n", msglen);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    MPI_Attr_get(comm, *fkey, &attrval, &flag);
    if (!flag) {
        *errs = *errs + 1;
        strncpy(lmsg, msg, msglen);
        lmsg[msglen] = 0;
        printf(" Error: flag false for Attr_get (set in F1): %s\n", lmsg);
        return;
    }
    /* Must be careful to compare as required in the MPI specification */
    ccompareint2void_(expected, attrval, &result);
    if (!result) {
        *errs = *errs + 1;
        strncpy(lmsg, msg, msglen);
        lmsg[msglen] = 0;
        printf(" Error: (set in F1) expected %d but saw %d: %s\n",
               *expected, *(MPI_Fint *) attrval, lmsg);
        return;
    }
    return;
}

/* Set in Fortran (MPI-2), read in C */
void cmpif2read_(MPI_Fint * fcomm, MPI_Fint * fkey, MPI_Aint * expected,
                 MPI_Fint * errs, const char *msg, int msglen)
{
    void *attrval;
    int flag, result;
    MPI_Comm comm = MPI_Comm_f2c(*fcomm);
    char lmsg[MAX_ATTRTEST_MSG];

    if (msglen > sizeof(lmsg) - 1) {
        fprintf(stderr, "Message too long for buffer (%d)\n", msglen);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    MPI_Comm_get_attr(comm, *fkey, &attrval, &flag);
    if (!flag) {
        *errs = *errs + 1;
        strncpy(lmsg, msg, msglen);
        lmsg[msglen] = 0;
        printf(" Error: flag false for Comm_get_attr (set in F2): %s\n", lmsg);
        return;
    }
    ccompareaint2void_(expected, attrval, &result);
    if (!result) {
        *errs = *errs + 1;
        strncpy(lmsg, msg, msglen);
        lmsg[msglen] = 0;
        printf(" Error: (set in F2) expected %ld but saw %ld: %s\n",
               (long) *expected, (long) *(MPI_Aint *) attrval, lmsg);
        return;
    }
    return;
}

void cmpif2readtype_(MPI_Fint * ftype, MPI_Fint * fkey, MPI_Aint * expected,
                     MPI_Fint * errs, const char *msg, int msglen)
{
    void *attrval;
    int flag, result;
    MPI_Datatype dtype = MPI_Type_f2c(*ftype);
    char lmsg[MAX_ATTRTEST_MSG];

    if (msglen > sizeof(lmsg) - 1) {
        fprintf(stderr, "Message too long for buffer (%d)\n", msglen);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    MPI_Type_get_attr(dtype, *fkey, &attrval, &flag);
    if (!flag) {
        *errs = *errs + 1;
        strncpy(lmsg, msg, msglen);
        lmsg[msglen] = 0;
        printf(" Error: flag false for Type_get_attr (set in F2): %s\n", lmsg);
        return;
    }
    ccompareaint2void_(expected, attrval, &result);
    if (!result) {
        *errs = *errs + 1;
        strncpy(lmsg, msg, msglen);
        lmsg[msglen] = 0;
        printf(" Error: (set in F2/Type) expected %ld but saw %ld: %s\n",
               (long) *expected, (long) *(MPI_Aint *) attrval, lmsg);
        return;
    }
    return;
}

void cmpif2readwin_(MPI_Fint * fwin, MPI_Fint * fkey, MPI_Aint * expected,
                    MPI_Fint * errs, const char *msg, int msglen)
{
    void *attrval;
    int flag, result;
    MPI_Win win = MPI_Win_f2c(*fwin);
    char lmsg[MAX_ATTRTEST_MSG];

    if (msglen > sizeof(lmsg) - 1) {
        fprintf(stderr, "Message too long for buffer (%d)\n", msglen);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    MPI_Win_get_attr(win, *fkey, &attrval, &flag);
    if (!flag) {
        *errs = *errs + 1;
        strncpy(lmsg, msg, msglen);
        lmsg[msglen] = 0;
        printf(" Error: flag false for Win_get_attr (set in F2): %s\n", lmsg);
        return;
    }
    ccompareaint2void_(expected, attrval, &result);
    if (!result) {
        *errs = *errs + 1;
        strncpy(lmsg, msg, msglen);
        lmsg[msglen] = 0;
        printf(" Error: (set in F2/Win) expected %ld but saw %ld: %s\n",
               (long) *expected, (long) *(MPI_Aint *) attrval, lmsg);
        return;
    }
    return;
}

/* ----------------------------------------------------------------------- */
/* Given a Fortran attribute (pointer to the value to store), set it using */
/* the C attribute functions                                               */
/* ----------------------------------------------------------------------- */

void csetmpi_(MPI_Fint * fcomm, MPI_Fint * fkey, MPI_Fint * val, MPI_Fint * errs)
{
    MPI_Comm comm = MPI_Comm_f2c(*fcomm);

    MPI_Comm_set_attr(comm, *fkey, (void *) (MPI_Aint) * val);
}

void csetmpi2_(MPI_Fint * fcomm, MPI_Fint * fkey, MPI_Aint * val, MPI_Fint * errs)
{
    MPI_Comm comm = MPI_Comm_f2c(*fcomm);

    MPI_Comm_set_attr(comm, *fkey, (void *) *val);
}

void csetmpitype_(MPI_Fint * ftype, MPI_Fint * fkey, MPI_Aint * val, MPI_Fint * errs)
{
    MPI_Datatype dtype = MPI_Type_f2c(*ftype);

    MPI_Type_set_attr(dtype, *fkey, (void *) *val);
}

void csetmpiwin_(MPI_Fint * fwin, MPI_Fint * fkey, MPI_Aint * val, MPI_Fint * errs)
{
    MPI_Win win = MPI_Win_f2c(*fwin);

    MPI_Win_set_attr(win, *fkey, (void *) *val);
}

/* ----------------------------------------------------------------------- */
/* Comparisons                                                             */
/*    int with aint                                                        */
/*    int with void*                                                       */
/*    aint with void*                                                      */
/* All routines use similar interfaces, though the routines that involve   */
/* void * must be called from C                                            */
/* Defined to be callable from either C or Fortran                         */
/* Here is the rule, defined in the MPI standard:                          */
/*    If one item is shorter than the other, take the low bytes.           */
/*    If one item is longer than the other, sign extend                    */
/* ----------------------------------------------------------------------- */
void ccompareint2aint_(MPI_Fint * in1, MPI_Aint * in2, MPI_Fint * result)
{
    static int idx = -1;
    if (sizeof(MPI_Fint) == sizeof(MPI_Aint)) {
        *result = *in1 == *in2;
    }
    else if (sizeof(MPI_Fint) < sizeof(MPI_Aint)) {
        /* Assume Aint no smaller than Fint, and that size of aint
         * is a multiple of the size of fint) */
        MPI_Fint *v2 = (MPI_Fint *) in2;
        if (idx < 0) {
            MPI_Aint av = 1;
            MPI_Fint *fa = (MPI_Fint *) & av;
            if ((sizeof(MPI_Aint) % sizeof(MPI_Fint)) != 0) {
                fprintf(stderr,
                        "PANIC: size of MPI_Aint = %d not a multiple of MPI_Fint = %d\n",
                        (int) sizeof(MPI_Aint), (int) sizeof(MPI_Fint));
                MPI_Abort(MPI_COMM_WORLD, 1);
            }
            for (idx = sizeof(MPI_Aint) / sizeof(MPI_Fint); idx >= 0; idx--)
                if (fa[idx])
                    break;
            if (idx < 0) {
                fprintf(stderr, "Unable to determine low word of Fint in Aint\n");
                MPI_Abort(MPI_COMM_WORLD, 1);
            }
            *result = *in1 == v2[idx];
        }
    }
    else {
        fprintf(stderr, "PANIC: sizeof(MPI_Fint) = %d > sizeof(MPI_Aint) %d\n",
                (int) sizeof(MPI_Fint), (int) sizeof(MPI_Aint));
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
}

void ccompareint2void_(MPI_Fint * in1, void *in2, MPI_Fint * result)
{
    static int idx = -1;
    if (sizeof(MPI_Fint) == sizeof(void *)) {
        *result = *in1 == *(MPI_Fint *) in2;
    }
    else if (sizeof(MPI_Fint) < sizeof(void *)) {
        /* Assume void* no smaller than Fint, and that size of aint
         * is a multiple of the size of fint) */
        MPI_Fint *v2 = (MPI_Fint *) in2;
        if (idx < 0) {
            void *av = (void *) 1;
            MPI_Fint *fa = (MPI_Fint *) & av;
            if ((sizeof(void *) % sizeof(MPI_Fint)) != 0) {
                fprintf(stderr,
                        "PANIC: size of void * = %d not a multiple of MPI_Fint = %d\n",
                        (int) sizeof(void *), (int) sizeof(MPI_Fint));
                MPI_Abort(MPI_COMM_WORLD, 1);
            }
            for (idx = sizeof(void *) / sizeof(MPI_Fint); idx >= 0; idx--)
                if (fa[idx])
                    break;
            if (idx < 0) {
                fprintf(stderr, "Unable to determine low word of Fint in void*\n");
                MPI_Abort(MPI_COMM_WORLD, 1);
            }
            *result = *in1 == v2[idx];
        }
    }
    else {
        fprintf(stderr, "PANIC: sizeof(MPI_Fint) = %d > sizeof(void*) %d\n",
                (int) sizeof(MPI_Fint), (int) sizeof(void *));
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
}

void ccompareaint2void_(MPI_Aint * in1, void *in2, MPI_Fint * result)
{
    /* Note that an aint must be >= void * by definition */
    *result = *in1 == *(MPI_Aint *) in2;
}

/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* style: allow:printf:13 sig:0 */

#include "mpiimpl.h"

#include "mpi_interface.h"

/* This is from dbginit.c; it is not exported to other files */
typedef struct MPIR_Comm_list {
    int sequence_number;        /* Used to detect changes in the list */
    MPIR_Comm *head;            /* Head of the list */
} MPIR_Comm_list;

extern MPIR_Comm_list MPIR_All_communicators;

/*
   This file contains emulation routines for the methods and functions normally
   provided by the debugger.  This file is only used for testing the
   dll_mpich.c debugger interface.
 */

/* These are mock-ups of the find type and find offset routines.  Since
   there are no portable routines to access the symbol table in an image,
   we hand-code the specific types that we use in this code.
   These routines (more precisely, the field_offset routine) need to
   known the layout of the internal data structures */
enum { TYPE_UNKNOWN = 0,
    TYPE_MPIR_COMM = 1,
    TYPE_MPIR_COMM_LIST = 2,
    TYPE_MPIR_REQUEST = 3,
    TYPE_MPIR_DEBUGQ = 4,
    TYPE_MPIDIG_COMM_T = 5,
} KnownTypes;

/* The dll_mpich.c has a few places where it doesn't always use the most
   recent type, so a static current type will not work.  Instead, we
   have an example of each type, and return that value. */

static int knownTypesArray[] = { TYPE_UNKNOWN, TYPE_MPIR_COMM,
    TYPE_MPIR_COMM_LIST,
    TYPE_MPIR_REQUEST,
    TYPE_MPIR_DEBUGQ,
    TYPE_MPIDIG_COMM_T,
};

mqs_type *dbgrI_find_type(mqs_image * image, char *name, mqs_lang_code lang)
{
    int curType = TYPE_UNKNOWN;

    if (strcmp(name, "MPIR_Comm") == 0) {
        curType = TYPE_MPIR_COMM;
    } else if (strcmp(name, "MPIR_Comm_list") == 0) {
        curType = TYPE_MPIR_COMM_LIST;
    } else if (strcmp(name, "MPIR_Request") == 0) {
        curType = TYPE_MPIR_REQUEST;
    } else if (strcmp(name, "MPIR_Debugq") == 0) {
        curType = TYPE_MPIR_DEBUGQ;
    } else if (strcmp(name, "MPIDIG_comm_t") == 0) {
        curType = TYPE_MPIDIG_COMM_T;
    } else {
        curType = TYPE_UNKNOWN;
    }
    return (mqs_type *) & knownTypesArray[curType];
}

int dbgrI_field_offset(mqs_type * type, char *name)
{
    int off = -1;

    int curType = *(int *) type;

    /* printf ("curtype is %d\n", curType); */

    switch (curType) {
        case TYPE_MPIR_COMM:
            {
                MPIR_Comm c;
                if (strcmp(name, "name") == 0) {
                    off = ((char *) &(c.name[0]) - (char *) &c.handle);
                } else if (strcmp(name, "comm_next") == 0) {
                    off = ((char *) &c.comm_next - (char *) &c.handle);
                } else if (strcmp(name, "remote_size") == 0) {
                    off = ((char *) &c.remote_size - (char *) &c.handle);
                } else if (strcmp(name, "rank") == 0) {
                    off = ((char *) &c.rank - (char *) &c.handle);
                } else if (strcmp(name, "context_id") == 0) {
                    off = ((char *) &c.context_id - (char *) &c.handle);
                } else if (strcmp(name, "recvcontext_id") == 0) {
                    off = ((char *) &c.recvcontext_id - (char *) &c.handle);
                } else if (strcmp(name, "dev") == 0) {
                    off = ((char *) &c.dev - (char *) &c.handle);
                } else {
                    printf("Panic! Unrecognized COMM field %s\n", name);
                }
            }
            break;
        case TYPE_MPIR_COMM_LIST:
            {
                MPIR_Comm_list c;
                if (strcmp(name, "sequence_number") == 0) {
                    off = ((char *) &c.sequence_number - (char *) &c);
                } else if (strcmp(name, "head") == 0) {
                    off = ((char *) &c.head - (char *) &c);
                } else {
                    printf("Panic! Unrecognized Comm List field %s\n", name);
                }
            }
            break;
        case TYPE_MPIR_REQUEST:
            {
                MPIR_Request c;
                if (strcmp(name, "dev") == 0) {
                    off = ((char *) &c.dev - (char *) &c);
                } else if (strcmp(name, "status") == 0) {
                    off = ((char *) &c.status - (char *) &c);
                } else if (strcmp(name, "cc") == 0) {
                    off = ((char *) &c.cc - (char *) &c);
                } else if (strcmp(name, "next") == 0) {
                    off = ((char *) &c.next - (char *) &c);
                } else {
                    printf("Panic! Unrecognized request field %s\n", name);
                }
            }
            break;
        case TYPE_MPIR_DEBUGQ:
            {
                struct MPIR_Debugq c;
                if (strcmp(name, "next") == 0) {
                    off = ((char *) &c.next - (char *) &c);
                } else if (strcmp(name, "tag") == 0) {
                    off = ((char *) &c.tag - (char *) &c);
                } else if (strcmp(name, "rank") == 0) {
                    off = ((char *) &c.rank - (char *) &c);
                } else if (strcmp(name, "context_id") == 0) {
                    off = ((char *) &c.context_id - (char *) &c);
                } else if (strcmp(name, "buf") == 0) {
                    off = ((char *) &c.buf - (char *) &c);
                } else if (strcmp(name, "count") == 0) {
                    off = ((char *) &c.count - (char *) &c);
                } else if (strcmp(name, "req") == 0) {
                    off = ((char *) &c.req - (char *) &c);
                } else {
                    printf("Panic! Unrecognized request queue field %s\n", name);
                }
            }
            break;
        case TYPE_UNKNOWN:
            off = -1;
            break;
        default:
            off = -1;
            break;
    }
    return off;
}

/* Simulate converting name to the address of a variable/symbol */
int dbgrI_find_symbol(mqs_image * image, char *name, mqs_taddr_t * loc)
{
    if (strcmp(name, "MPIR_All_communicators") == 0) {
        *loc = (mqs_taddr_t) & MPIR_All_communicators;
        /* The following printfs can help is diagnosing problems when the
         * extern variable MPIR_All_communicators appears not to be
         * correctly resolved. */
        printf("all communicators head at %p\n", (void *) *loc);
        printf("all communicators head as pointer %p\n", &MPIR_All_communicators);
        printf("head is %p\n", MPIR_All_communicators.head);
        return mqs_ok;
    } else if (strcmp(name, "MPIR_Recvq_head") == 0) {
        *loc = (mqs_taddr_t) & MPIR_Recvq_head;
        return mqs_ok;
    } else if (strcmp(name, "MPIR_Unexpq_head") == 0) {
        *loc = (mqs_taddr_t) & MPIR_Unexpq_head;
        return mqs_ok;
    } else if (strcmp(name, "MPIR_Sendq_head") == 0) {
        *loc = (mqs_taddr_t) & MPIR_Sendq_head;
        return mqs_ok;
    } else {
        printf("Panic! Unrecognized symbol %s\n", name);
    }
    return 1;
}

/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2014 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include "mpitest.h"
#include <stdio.h>
#include <string.h>

/* Communicating a datatype built out of structs
 * This test was motivated by the failure of an example program for
 * RMA involving simple operations on a struct that included a struct
 *
 * The observed failure was a SEGV in the MPI_Get
 *
 *
 */
#define MAX_KEY_SIZE 64
#define MAX_VALUE_SIZE 256
typedef struct {
    MPI_Aint disp;
    int rank;
    void *lptr;
} Rptr;
typedef struct {
    Rptr next;
    char key[MAX_KEY_SIZE], value[MAX_VALUE_SIZE];
} ListElm;
Rptr nullDptr = { 0, -1, 0 };

int testCases = -1;
#define BYTE_ONLY 0x1
#define TWO_STRUCT 0x2
int isOneLevel = 0;

int main(int argc, char **argv)
{
    int errors = 0;
    Rptr headDptr;
    ListElm *headLptr = 0;
    int i, wrank;
    MPI_Datatype dptrType, listelmType;
    MPI_Win listwin;

    MTest_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &wrank);

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-byteonly") == 0) {
            testCases = BYTE_ONLY;
        }
        else if (strcmp(argv[i], "-twostruct") == 0) {
            testCases = TWO_STRUCT;
        }
        else if (strcmp(argv[i], "-onelevel") == 0) {
            isOneLevel = 1;
        }
        else {
            printf("Unrecognized argument %s\n", argv[i]);
        }
    }

    /* Create the datatypes that we will use to move the data */
    {
        int blens[3];
        MPI_Aint displ[3];
        MPI_Datatype dtypes[3];
        ListElm sampleElm;

        blens[0] = 1;
        blens[1] = 1;
        MPI_Get_address(&nullDptr.disp, &displ[0]);
        MPI_Get_address(&nullDptr.rank, &displ[1]);
        displ[1] = displ[1] - displ[0];
        displ[0] = 0;
        dtypes[0] = MPI_AINT;
        dtypes[1] = MPI_INT;
        MPI_Type_create_struct(2, blens, displ, dtypes, &dptrType);
        MPI_Type_commit(&dptrType);

        if (isOneLevel) {
            blens[0] = sizeof(nullDptr);
            dtypes[0] = MPI_BYTE;
        }
        else {
            blens[0] = 1;
            dtypes[0] = dptrType;
        }
        blens[1] = MAX_KEY_SIZE;
        dtypes[1] = MPI_CHAR;
        blens[2] = MAX_VALUE_SIZE;
        dtypes[2] = MPI_CHAR;
        MPI_Get_address(&sampleElm.next, &displ[0]);
        MPI_Get_address(&sampleElm.key[0], &displ[1]);
        MPI_Get_address(&sampleElm.value[0], &displ[2]);
        displ[2] -= displ[0];
        displ[1] -= displ[0];
        displ[0] = 0;
        for (i = 0; i < 3; i++) {
            MTestPrintfMsg(0, "%d:count=%d,disp=%ld\n", i, blens[i], displ[i]);
        }
        MPI_Type_create_struct(3, blens, displ, dtypes, &listelmType);
        MPI_Type_commit(&listelmType);
    }

    MPI_Win_create_dynamic(MPI_INFO_NULL, MPI_COMM_WORLD, &listwin);

    headDptr.rank = 0;
    if (wrank == 0) {
        /* Create 1 list element (the head) and initialize it */
        MPI_Alloc_mem(sizeof(ListElm), MPI_INFO_NULL, &headLptr);
        MPI_Get_address(headLptr, &headDptr.disp);
        headLptr->next.rank = -1;
        headLptr->next.disp = (MPI_Aint) MPI_BOTTOM;
        headLptr->next.lptr = 0;
        strncpy(headLptr->key, "key1", MAX_KEY_SIZE);
        strncpy(headLptr->value, "value1", MAX_VALUE_SIZE);
        MPI_Win_attach(listwin, headLptr, sizeof(ListElm));
    }
    MPI_Bcast(&headDptr.disp, 1, MPI_AINT, 0, MPI_COMM_WORLD);

    MPI_Barrier(MPI_COMM_WORLD);

    if (wrank == 1) {
        ListElm headcopy;

        MPI_Win_lock_all(0, listwin);
        /* Get head element with simple get of BYTES */
        if (testCases & BYTE_ONLY) {
            headcopy.next.rank = 100;
            headcopy.next.disp = 0xefefefef;
            MPI_Get(&headcopy, sizeof(ListElm), MPI_BYTE,
                    headDptr.rank, headDptr.disp, sizeof(ListElm), MPI_BYTE, listwin);
            MPI_Win_flush(headDptr.rank, listwin);
            if (headcopy.next.rank != -1 && headcopy.next.disp != (MPI_Aint) MPI_BOTTOM) {
                errors++;
                printf("MPI_BYTE: headcopy contains incorrect next:<%d,%ld>\n",
                       headcopy.next.rank, (long) headcopy.next.disp);
            }
        }

        if (testCases & TWO_STRUCT) {
            headcopy.next.rank = 100;
            headcopy.next.disp = 0xefefefef;
            /* Get head element using struct of struct type.  This is
             * not an identical get to the simple BYTE one above but should
             * work */
            MPI_Get(&headcopy, 1, listelmType, headDptr.rank, headDptr.disp,
                    1, listelmType, listwin);
            MPI_Win_flush(headDptr.rank, listwin);
            if (headcopy.next.rank != -1 && headcopy.next.disp != (MPI_Aint) MPI_BOTTOM) {
                errors++;
                printf("ListelmType: headcopy contains incorrect next:<%d,%ld>\n",
                       headcopy.next.rank, (long) headcopy.next.disp);
            }
        }

        MPI_Win_unlock_all(listwin);
    }
    MPI_Barrier(MPI_COMM_WORLD);
    if (wrank == 0) {
        MPI_Win_detach(listwin, headLptr);
        MPI_Free_mem(headLptr);
    }
    MPI_Win_free(&listwin);
    MPI_Type_free(&dptrType);
    MPI_Type_free(&listelmType);

    MTest_Finalize(errors);
    MPI_Finalize();
    return MTestReturnValue(errors);
}

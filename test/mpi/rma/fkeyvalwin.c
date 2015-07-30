/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include "mpitestconf.h"
#include "mpitest.h"
#ifdef HAVE_STRING_H
#include <string.h>
#endif

/*
static char MTestDescrip[] = "Test freeing keyvals while still attached to \
a win, then make sure that the keyval delete code are still \
executed";
*/

/* Copy increments the attribute value */
/* Note that we can really ignore this because there is no win dup */
int copy_fn(MPI_Win oldwin, int keyval, void *extra_state,
            void *attribute_val_in, void *attribute_val_out, int *flag);
int copy_fn(MPI_Win oldwin, int keyval, void *extra_state,
            void *attribute_val_in, void *attribute_val_out, int *flag)
{
    /* Copy the address of the attribute */
    *(void **) attribute_val_out = attribute_val_in;
    /* Change the value */
    *(int *) attribute_val_in = *(int *) attribute_val_in + 1;
    *flag = 1;
    return MPI_SUCCESS;
}

/* Delete decrements the attribute value */
int delete_fn(MPI_Win win, int keyval, void *attribute_val, void *extra_state);
int delete_fn(MPI_Win win, int keyval, void *attribute_val, void *extra_state)
{
    *(int *) attribute_val = *(int *) attribute_val - 1;
    return MPI_SUCCESS;
}

int main(int argc, char *argv[])
{
    int errs = 0;
    int attrval;
    int i, key[32], keyval;
    MPI_Win win;
    MTest_Init(&argc, &argv);

    while (MTestGetWin(&win, 0)) {
        if (win == MPI_WIN_NULL)
            continue;

        MPI_Win_create_keyval(copy_fn, delete_fn, &keyval, (void *) 0);
        attrval = 1;
        MPI_Win_set_attr(win, keyval, (void *) &attrval);
        /* See MPI-1, 5.7.1.  Freeing the keyval does not remove it if it
         * is in use in an attribute */
        MPI_Win_free_keyval(&keyval);

        /* We create some dummy keyvals here in case the same keyval
         * is reused */
        for (i = 0; i < 32; i++) {
            MPI_Win_create_keyval(MPI_NULL_COPY_FN, MPI_NULL_DELETE_FN, &key[i], (void *) 0);
        }

        MTestFreeWin(&win);

        /* Check that the original attribute was freed */
        if (attrval != 0) {
            errs++;
            printf("Attribute not decremented when win %s freed\n", MTestGetWinName());
        }
        /* Free those other keyvals */
        for (i = 0; i < 32; i++) {
            MPI_Win_free_keyval(&key[i]);
        }

    }
    MTest_Finalize(errs);
    MPI_Finalize();

    return 0;
}

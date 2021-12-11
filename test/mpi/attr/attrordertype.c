/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"

#ifdef MULTI_TESTS
#define run attr_attrordertype
int run(const char *arg);
#endif

/*
static char MTestDescrip[] = "Test creating and inserting attributes in \
different orders to ensure that the list management code handles all cases.";
*/

static int checkAttrs(MPI_Datatype type, int n, int key[], int attrval[]);
static int checkNoAttrs(MPI_Datatype type, int n, int key[]);

int run(const char *arg)
{
    int errs = 0;
    int key[3], attrval[3];
    int i;
    MPI_Datatype type;

    {
        type = MPI_INT;
        /* Create key values */
        for (i = 0; i < 3; i++) {
            MPI_Type_create_keyval(MPI_TYPE_NULL_COPY_FN, MPI_TYPE_NULL_DELETE_FN, &key[i],
                                   (void *) 0);
            attrval[i] = 1024 * i;
        }

        /* Insert attribute in several orders.  Test after put with get,
         * then delete, then confirm delete with get. */

        MPI_Type_set_attr(type, key[2], &attrval[2]);
        MPI_Type_set_attr(type, key[1], &attrval[1]);
        MPI_Type_set_attr(type, key[0], &attrval[0]);

        errs += checkAttrs(type, 3, key, attrval);

        MPI_Type_delete_attr(type, key[0]);
        MPI_Type_delete_attr(type, key[1]);
        MPI_Type_delete_attr(type, key[2]);

        errs += checkNoAttrs(type, 3, key);

        MPI_Type_set_attr(type, key[1], &attrval[1]);
        MPI_Type_set_attr(type, key[2], &attrval[2]);
        MPI_Type_set_attr(type, key[0], &attrval[0]);

        errs += checkAttrs(type, 3, key, attrval);

        MPI_Type_delete_attr(type, key[2]);
        MPI_Type_delete_attr(type, key[1]);
        MPI_Type_delete_attr(type, key[0]);

        errs += checkNoAttrs(type, 3, key);

        MPI_Type_set_attr(type, key[0], &attrval[0]);
        MPI_Type_set_attr(type, key[1], &attrval[1]);
        MPI_Type_set_attr(type, key[2], &attrval[2]);

        errs += checkAttrs(type, 3, key, attrval);

        MPI_Type_delete_attr(type, key[1]);
        MPI_Type_delete_attr(type, key[2]);
        MPI_Type_delete_attr(type, key[0]);

        errs += checkNoAttrs(type, 3, key);

        for (i = 0; i < 3; i++) {
            MPI_Type_free_keyval(&key[i]);
        }
    }

    return errs;
}

static int checkAttrs(MPI_Datatype type, int n, int key[], int attrval[])
{
    int errs = 0;
    int i, flag, *val_p;

    for (i = 0; i < n; i++) {
        MPI_Type_get_attr(type, key[i], &val_p, &flag);
        if (!flag) {
            errs++;
            fprintf(stderr, "Attribute for key %d not set\n", i);
        } else if (val_p != &attrval[i]) {
            errs++;
            fprintf(stderr, "Attribute value for key %d not correct\n", i);
        }
    }

    return errs;
}

static int checkNoAttrs(MPI_Datatype type, int n, int key[])
{
    int errs = 0;
    int i, flag, *val_p;

    for (i = 0; i < n; i++) {
        MPI_Type_get_attr(type, key[i], &val_p, &flag);
        if (flag) {
            errs++;
            fprintf(stderr, "Attribute for key %d set but should be deleted\n", i);
        }
    }

    return errs;
}

/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpi.h"
#include <stdio.h>
#include "mpitest.h"
#include "stdlib.h"
#include "dtpools.h"

/*
static char MTestDescrip[] = "Test freeing keyvals while still attached to \
a datatype, then make sure that the keyval delete and copy code are still \
executed";
*/



/* Copy increments the attribute value */
int copy_fn(MPI_Datatype oldtype, int keyval, void *extra_state,
            void *attribute_val_in, void *attribute_val_out, int *flag);
int copy_fn(MPI_Datatype oldtype, int keyval, void *extra_state,
            void *attribute_val_in, void *attribute_val_out, int *flag)
{
    /* Copy the address of the attribute */
    *(void **) attribute_val_out = attribute_val_in;
    /* Change the value */
    *(int *) attribute_val_in = *(int *) attribute_val_in + 1;
    /* set flag to 1 to tell comm dup to insert this attribute
     * into the new communicator */
    *flag = 1;
    return MPI_SUCCESS;
}

/* Delete decrements the attribute value */
int delete_fn(MPI_Datatype type, int keyval, void *attribute_val, void *extra_state);
int delete_fn(MPI_Datatype type, int keyval, void *attribute_val, void *extra_state)
{
    *(int *) attribute_val = *(int *) attribute_val - 1;
    return MPI_SUCCESS;
}

int main(int argc, char *argv[])
{
    int err, errs = 0;
    int attrval;
    int i, key[32], keyval, saveKeyval;
    int seed, testsize;
    int obj_idx;
    MPI_Aint count, maxbufsize;
    MPI_Datatype type, duptype;
    DTP_pool_s dtp;
    DTP_obj_s obj;
    char *basic_type;

    MTest_Init(&argc, &argv);

    MTestArgList *head = MTestArgListCreate(argc, argv);
    seed = MTestArgListGetInt(head, "seed");
    testsize = MTestArgListGetInt(head, "testsize");
    count = MTestArgListGetLong(head, "count");
    basic_type = MTestArgListGetString(head, "type");

    maxbufsize = MTestDefaultMaxBufferSize();

    err = DTP_pool_create(basic_type, count, seed, &dtp);
    if (err != DTP_SUCCESS) {
        fprintf(stderr, "Error while creating pool (%s,%ld)\n", basic_type, count);
        fflush(stderr);
    }

    MTestArgListDestroy(head);

    for (obj_idx = 0; obj_idx < testsize; obj_idx++) {
        err = DTP_obj_create(dtp, &obj, maxbufsize);
        if (err != DTP_SUCCESS) {
            errs++;
            break;
        }

        type = obj.DTP_datatype;
        MPI_Type_create_keyval(copy_fn, delete_fn, &keyval, (void *) 0);
        saveKeyval = keyval;    /* in case we need to free explicitly */
        attrval = 1;
        MPI_Type_set_attr(type, keyval, (void *) &attrval);
        /* See MPI-1, 5.7.1.  Freeing the keyval does not remove it if it
         * is in use in an attribute */
        MPI_Type_free_keyval(&keyval);

        /* We create some dummy keyvals here in case the same keyval
         * is reused */
        for (i = 0; i < 32; i++) {
            MPI_Type_create_keyval(MPI_TYPE_NULL_COPY_FN, MPI_TYPE_NULL_DELETE_FN, &key[i],
                                   (void *) 0);
        }

        if (attrval != 1) {
            errs++;
            printf("attrval is %d, should be 1, before dup in type %s\n", attrval,
                   DTP_obj_get_description(obj));
        }
        MPI_Type_dup(type, &duptype);
        /* Check that the attribute was copied */
        if (attrval != 2) {
            errs++;
            printf("Attribute not incremented when type dup'ed (%s)\n",
                   DTP_obj_get_description(obj));
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        MPI_Type_free(&duptype);
        if (attrval != 1) {
            errs++;
            printf("Attribute not decremented when duptype %s freed\n",
                   DTP_obj_get_description(obj));
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        /* Check that the attribute was freed in the duptype */

        if (obj.DTP_datatype != dtp.DTP_base_type) {
            DTP_obj_free(obj);
            if (attrval != 0) {
                errs++;
                fprintf(stderr, "Attribute not decremented when type %s freed\n",
                        DTP_obj_get_description(obj));
                MPI_Abort(MPI_COMM_WORLD, 1);
            }
        } else {
            MPI_Type_delete_attr(type, saveKeyval);
            DTP_obj_free(obj);
        }

        /* Free those other keyvals */
        for (i = 0; i < 32; i++) {
            MPI_Type_free_keyval(&key[i]);
        }
    }

    DTP_pool_free(dtp);

    MTest_Finalize(errs);

    return MTestReturnValue(errs);
}

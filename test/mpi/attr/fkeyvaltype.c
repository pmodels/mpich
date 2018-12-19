/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
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
    int i, j, key[32], keyval, saveKeyval;
    int obj_idx;
    int count;
    int tnlen;
    MPI_Datatype type, duptype;
    DTP_t dtp;
    char typename[MPI_MAX_OBJECT_NAME];

    MTest_Init(&argc, &argv);

#ifndef USE_DTP_POOL_TYPE__STRUCT       /* set in 'test/mpi/structtypetest.txt' to split tests */
    MPI_Datatype basic_type;

    err = MTestInitBasicSignature(argc, argv, &count, &basic_type);
    if (err)
        return MTestReturnValue(1);

    err = DTP_pool_create(basic_type, count, &dtp);
    if (err != DTP_SUCCESS) {
        MPI_Type_get_name(basic_type, typename, &tnlen);
        fprintf(stdout, "Error while creating pool (%s,%d)\n", typename, count);
        fflush(stdout);
    }
#else
    MPI_Datatype *basic_types = NULL;
    int basic_type_num;
    int *basic_type_counts = NULL;

    err = MTestInitStructSignature(argc, argv, &basic_type_num, &basic_type_counts, &basic_types);
    if (err)
        return MTestReturnValue(1);

    err = DTP_pool_create_struct(basic_type_num, basic_types, basic_type_counts, &dtp);
    if (err != DTP_SUCCESS) {
        fprintf(stdout, "Error while creating struct pool\n");
        fflush(stdout);
    }

    count = 0;
#endif

    for (obj_idx = 0; obj_idx < dtp->DTP_num_objs; obj_idx++) {
        err = DTP_obj_create(dtp, obj_idx, 0, 0, 0);
        if (err != DTP_SUCCESS) {
            errs++;
            break;
        }

        type = dtp->DTP_obj_array[obj_idx].DTP_obj_type;
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
            MPI_Type_create_keyval(MPI_NULL_COPY_FN, MPI_NULL_DELETE_FN, &key[i], (void *) 0);
        }

        if (attrval != 1) {
            errs++;
            MPI_Type_get_name(type, typename, &tnlen);
            printf("attrval is %d, should be 1, before dup in type %s\n", attrval, typename);
        }
        MPI_Type_dup(type, &duptype);
        /* Check that the attribute was copied */
        if (attrval != 2) {
            errs++;
            MPI_Type_get_name(type, typename, &tnlen);
            printf("Attribute not incremented when type dup'ed (%s)\n", typename);
        }
        MPI_Type_free(&duptype);
        if (attrval != 1) {
            errs++;
            MPI_Type_get_name(type, typename, &tnlen);
            printf("Attribute not decremented when duptype %s freed\n", typename);
        }
        /* Check that the attribute was freed in the duptype */

        DTP_obj_free(dtp, obj_idx);
        if (attrval != 0) {
            errs++;
            MPI_Type_get_name(type, typename, &tnlen);
            fprintf(stdout, "Attribute not decremented when type %s freed\n", typename);
        }

        /* Free those other keyvals */
        for (i = 0; i < 32; i++) {
            MPI_Type_free_keyval(&key[i]);
        }
    }

    DTP_pool_free(dtp);

#ifdef USE_DTP_POOL_TYPE__STRUCT
    /* cleanup array if any */
    if (basic_types) {
        free(basic_types);
    }
    if (basic_type_counts) {
        free(basic_type_counts);
    }
#endif

    MTest_Finalize(errs);

    return MTestReturnValue(errs);
}

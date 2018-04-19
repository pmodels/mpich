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

typedef struct {
    const char *typename;
    MPI_Datatype type;
} Type_t;

Type_t typelist[] = {
    {"MPI_CHAR", MPI_CHAR},
    {"MPI_BYTE", MPI_BYTE},
    {"MPI_WCHAR", MPI_WCHAR},
    {"MPI_SHORT", MPI_SHORT},
    {"MPI_INT", MPI_INT},
    {"MPI_LONG", MPI_LONG},
    {"MPI_LONG_LONG_INT", MPI_LONG_LONG_INT},
    {"MPI_UNSIGNED_CHAR", MPI_UNSIGNED_CHAR},
    {"MPI_UNSIGNED_SHORT", MPI_UNSIGNED_SHORT},
    {"MPI_UNSIGNED", MPI_UNSIGNED},
    {"MPI_UNSIGNED_LONG", MPI_UNSIGNED_LONG},
    {"MPI_UNSIGNED_LONG_LONG", MPI_UNSIGNED_LONG_LONG},
    {"MPI_FLOAT", MPI_FLOAT},
    {"MPI_DOUBLE", MPI_DOUBLE},
    {"MPI_LONG_DOUBLE", MPI_LONG_DOUBLE},
    {"MPI_INT8_T", MPI_INT8_T},
    {"MPI_INT16_T", MPI_INT16_T},
    {"MPI_INT32_T", MPI_INT32_T},
    {"MPI_INT64_T", MPI_INT64_T},
    {"MPI_UINT8_T", MPI_UINT8_T},
    {"MPI_UINT16_T", MPI_UINT16_T},
    {"MPI_UINT32_T", MPI_UINT32_T},
    {"MPI_UINT64_T", MPI_UINT64_T},
    {"MPI_C_COMPLEX", MPI_C_COMPLEX},
    {"MPI_C_FLOAT_COMPLEX", MPI_C_FLOAT_COMPLEX},
    {"MPI_C_DOUBLE_COMPLEX", MPI_C_DOUBLE_COMPLEX},
    {"MPI_C_LONG_DOUBLE_COMPLEX", MPI_C_LONG_DOUBLE_COMPLEX},
    {"MPI_FLOAT_INT", MPI_FLOAT_INT},
    {"MPI_DOUBLE_INT", MPI_DOUBLE_INT},
    {"MPI_LONG_INT", MPI_LONG_INT},
    {"MPI_2INT", MPI_2INT},
    {"MPI_SHORT_INT", MPI_SHORT_INT},
    {"MPI_LONG_DOUBLE_INT", MPI_LONG_DOUBLE_INT},
    {"MPI_DATATYPE_NULL", MPI_DATATYPE_NULL}
};

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

    /* TODO: parse input parameters using optarg */
    if (argc < 3) {
        fprintf(stdout, "Usage: %s -type=[TYPE] -count=[COUNT]\n", argv[0]);
        return MTestReturnValue(1);
    } else {
        for (i = 1; i < argc; i++) {
            if (!strncmp(argv[i], "-type=", strlen("-type="))) {
                j = 0;
                while (strcmp(typelist[j].typename, "MPI_DATATYPE_NULL") &&
                       strcmp(argv[i] + strlen("-type="), typelist[j].typename)) {
                    j++;
                }

                if (strcmp(typelist[j].typename, "MPI_DATATYPE_NULL")) {
                    basic_type = typelist[j].type;
                } else {
                    fprintf(stdout, "Error: datatype not recognized\n");
                    return MTestReturnValue(1);
                }
            } else if (!strncmp(argv[i], "-count=", strlen("-count="))) {
                count = atoi(argv[i] + strlen("-count="));
            }
        }
    }

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
    int k;
    char *input_string, *token;

    /* TODO: parse input parameters using optarg */
    if (argc < 4) {
        fprintf(stdout, "Usage: %s -numtypes=[NUM] -types=[TYPES] -counts=[COUNTS]\n", argv[0]);
        return MTestReturnValue(1);
    } else {
        for (i = 1; i < argc; i++) {
            if (!strncmp(argv[i], "-numtypes=", strlen("-numtypes="))) {
                basic_type_num = atoi(argv[i] + strlen("-numtypes="));

                /* allocate arrays */
                basic_type_counts = (int *) malloc(basic_type_num * sizeof(int));
                basic_types = (MPI_Datatype *) malloc(basic_type_num * sizeof(MPI_Datatype));
            } else if (!strncmp(argv[i], "-types=", strlen("-type="))) {
                input_string = strdup(argv[i] + strlen("-types="));
                for (k = 0, token = strtok(input_string, ","); token; token = strtok(NULL, ",")) {
                    j = 0;
                    while (strcmp(typelist[j].typename, "MPI_DATATYPE_NULL") &&
                           strcmp(token, typelist[j].typename)) {
                        j++;
                    }

                    if (strcmp(typelist[j].typename, "MPI_DATATYPE_NULL")) {
                        basic_types[k++] = typelist[j].type;
                    } else {
                        fprintf(stdout, "Error: datatype not recognized\n");
                        return MTestReturnValue(1);
                    }
                }
                free(input_string);
            } else if (!strncmp(argv[i], "-counts=", strlen("-counts="))) {
                input_string = strdup(argv[i] + strlen("-counts="));
                for (k = 0, token = strtok(input_string, ","); token; token = strtok(NULL, ",")) {
                    basic_type_counts[k++] = atoi(token);
                }
                free(input_string);
            }
        }
    }

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

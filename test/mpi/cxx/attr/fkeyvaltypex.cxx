/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpi.h"


#include "mpitestconf.h"
#ifdef HAVE_IOSTREAM
// Not all C++ compilers have iostream instead of iostream.h
#include <iostream>
#ifdef HAVE_NAMESPACE_STD
// Those that do often need the std namespace; otherwise, a bare "cerr"
// is likely to fail to compile
using namespace std;
#endif
#else
#include <iostream.h>
#endif
#include <string>
#include <stdio.h>
#include "mpitestcxx.h"
extern "C" {
#include "dtpools.h"
}
#include <stdlib.h>
static char MTestDescrip[] = "Test freeing keyvals while still attached to \
a datatype, then make sure that the keyval delete and copy code are still \
executed";


/* Copy increments the attribute value */
int copy_fn(const MPI::Datatype & oldtype, int keyval, void *extra_state,
            void *attribute_val_in, void *attribute_val_out, bool & flag)
{
    /* Copy the address of the attribute */
    *(void **) attribute_val_out = attribute_val_in;
    /* Change the value */
    *(int *) attribute_val_in = *(int *) attribute_val_in + 1;
    /* set flag to 1 to tell comm dup to insert this attribute
     * into the new communicator */
    flag = 1;
    return MPI::SUCCESS;
}

/* Delete decrements the attribute value */
int delete_fn(MPI::Datatype & type, int keyval, void *attribute_val, void *extra_state)
{
    *(int *) attribute_val = *(int *) attribute_val - 1;
    return MPI::SUCCESS;
}

int main(int argc, char *argv[])
{
    int err, errs = 0;
    int attrval;
    int i, j, key[32], keyval, saveKeyval;
    int obj_idx;
    int seed, testsize;
    MPI_Aint count, maxbufsize;
    MPI::Datatype type, duptype;
    char *basic_type;
    DTP_pool_s dtp;
    DTP_obj_s obj;
    int tnlen;
    MPI_Comm comm = MPI_COMM_WORLD;

    MTest_Init();

    MTestArgList *head = MTestArgListCreate(argc, argv);
    seed = MTestArgListGetInt(head, "seed");
    testsize = MTestArgListGetInt(head, "testsize");
    count = MTestArgListGetLong(head, "count");
    basic_type = MTestArgListGetString(head, "type");

    maxbufsize = MTestDefaultMaxBufferSize();

    err = DTP_pool_create(basic_type, count, seed, &dtp);
    if (err != DTP_SUCCESS) {
        cerr << "Error while creating pool (" << basic_type << "," << count << ")\n";
    }

    MTestArgListDestroy(head);

    for (obj_idx = 0; obj_idx < testsize; obj_idx++) {
        err = DTP_obj_create(dtp, &obj, maxbufsize);
        if (err != DTP_SUCCESS) {
            errs++;
            break;
        }

        type = obj.DTP_datatype;
        keyval = MPI::Datatype::Create_keyval(copy_fn, delete_fn, (void *) 0);
        saveKeyval = keyval;    /* in case we need to free explicitly */
        attrval = 1;
        type.Set_attr(keyval, &attrval);
        /* See MPI-1, 5.7.1.  Freeing the keyval does not remove it if it
         * is in use in an attribute */
        MPI::Datatype::Free_keyval(keyval);

        /* We create some dummy keyvals here in case the same keyval
         * is reused */
        for (i = 0; i < 32; i++) {
            key[i] =
                MPI::Datatype::Create_keyval(MPI::Datatype::NULL_COPY_FN,
                                             MPI::Datatype::NULL_DELETE_FN, (void *) 0);
        }

        if (attrval != 1) {
            errs++;
            cerr << "attrval is " << attrval << ", should be 1, before dup in type " << DTP_obj_get_description(obj) << "\n";
        }
        duptype = type.Dup();
        /* Check that the attribute was copied */
        if (attrval != 2) {
            errs++;
            cerr << "Attribute not incremented when type dup'ed (" << DTP_obj_get_description(obj) << ")\n";
        }
        duptype.Free();
        if (attrval != 1) {
            errs++;
            cerr << "Attribute not decremented when duptype " << DTP_obj_get_description(obj) << " freed\n";
        }
        /* Check that the attribute was freed in the duptype */

        if (obj.DTP_datatype != dtp.DTP_base_type) {
            DTP_obj_free(obj);
            if (attrval != 0) {
                errs++;
                cerr << "Attribute not decremented when type " << DTP_obj_get_description(obj) << "reed\n";
            }
        } else {
            MPI_Type_delete_attr(type, saveKeyval);
            DTP_obj_free(obj);
        }

        /* Free those other keyvals */
        for (i = 0; i < 32; i++) {
            MPI::Datatype::Free_keyval(key[i]);
        }
    }
    DTP_pool_free(dtp);

    MTest_Finalize(errs);

    return 0;

}

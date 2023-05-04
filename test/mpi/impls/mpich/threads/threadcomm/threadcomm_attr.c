/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"
#include <assert.h>

#define NTHREADS 4

MPI_Comm comm;
int key = 0;

struct perthread {
    int errs;
    int delete_fn_called;
};
static struct perthread data[NTHREADS];

static int key_delete_fn(MPI_Comm comm, int keyval, void *attribute_val, void *extra_state)
{
    struct perthread *my_data = attribute_val;
    my_data->delete_fn_called++;
    return MPI_SUCCESS;
}

static MTEST_THREAD_RETURN_TYPE test_thread(void *arg)
{
    struct perthread *my_data = arg;
    MPIX_Threadcomm_start(comm);

    MPI_Comm_set_attr(comm, key, arg);

    void *v;
    int flag;
    MPI_Comm_get_attr(comm, MPI_TAG_UB, &v, &flag);
    if (!flag) {
        my_data->errs++;
        printf("Could not get TAG_UB\n");
    } else {
        int vval = *(int *) v;
        if (vval < 32767) {
            my_data->errs++;
            printf("Got too-small value (%d) for TAG_UB\n", vval);
        }
    }

    MPI_Comm_get_attr(comm, key, &v, &flag);
    if (!flag) {
        my_data->errs++;
        printf("Could not get key\n");
    } else {
        if (v != arg) {
            my_data->errs++;
            printf("Retrieved incorrect key value\n");
        }
    }

    MPIX_Threadcomm_finish(comm);
}

int main(int argc, char *argv[])
{
    int errs = 0;
    MTest_Init(NULL, NULL);

    MPI_Comm_create_keyval(MPI_NULL_COPY_FN, key_delete_fn, &key, NULL);

    MPIX_Threadcomm_init(MPI_COMM_WORLD, NTHREADS, &comm);
    for (int i = 0; i < NTHREADS; i++) {
        MTest_Start_thread(test_thread, &data[i]);
    }
    MTest_Join_threads();
    MPIX_Threadcomm_free(&comm);

    for (int i = 0; i < NTHREADS; i++) {
        if (data[i].delete_fn_called != 1) {
            errs++;
            printf("The delete function for thread %d was not called\n", i);
        }
        errs += data[i].errs;
    }

    MPI_Comm_free_keyval(&key);
    MTest_Finalize(errs);
    return errs;
}

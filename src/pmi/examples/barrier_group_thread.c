/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "pmi.h"

#define NUM_THREADS 4
/* example using PMI_Barrier_group from PMI 1.2 */
void *thread_fn(void *arg);

int rank = -1;
int size = 0;
char kvsname[1024];

int errs = 0;

int main(int argc, char **argv)
{
    int has_parent;
    PMI_Init(&has_parent);
    PMI_Get_rank(&rank);
    PMI_Get_size(&size);
    if (size < 2) {
        if (rank == 0) {
            printf("Run this example with more than 2 processes.\n");
        }
        goto fn_exit;
    }

    if (PMI_Barrier_group(PMI_GROUP_SELF, 0, NULL) != PMI_SUCCESS) {
        if (rank == 0) {
            printf("PMI_Barrier_group not supported by PMI server!\n");
        }
        goto fn_exit;
    }

    PMI_KVS_Get_my_name(kvsname, 1024);

    pthread_t threads[NUM_THREADS];
    int thread_ids[NUM_THREADS];
    /* spawn threads to run concurrent PMI_Barrier_group between rank 0 and 1 */
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_ids[i] = i;
        pthread_create(&threads[i], NULL, thread_fn, &thread_ids[i]);
    }

    /* main thread run a concurrent PMI_Barrier */
    PMI_Barrier();

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    if (rank == 0 && errs == 0) {
        printf("No Errors\n");
    }

  fn_exit:
    PMI_Finalize();
    return 0;
}

void *thread_fn(void *arg)
{
    int thread_id = *(int *) arg;
    // printf("Hello from thread %d\n", thread_id);

    char key[20];
    char val[20];

    snprintf(key, 20, "key-%d", thread_id);
    snprintf(val, 20, "val-%d", thread_id);
    if (rank == 1) {
        /* NOTE: PMI-v1 does not allow space in both key and value */
        PMI_KVS_Put(kvsname, key, val);
    }

    if (rank == 0 || rank == 1) {
        int procs[2] = { 0, 1 };
        char tag[20];
        snprintf(tag, 20, "%d", thread_id);
        PMI_Barrier_group(procs, 2, tag);
    }

    if (rank == 0) {
        char buf[100];
        int pmi_errno = PMI_KVS_Get(kvsname, key, buf, 100);
        if (strcmp(buf, val) != 0) {
            errs++;
            printf("Test failed! pmi_errno = %d, buf=[%s], expect [%s]\n", pmi_errno, buf, val);
        }
    }

    return NULL;
}

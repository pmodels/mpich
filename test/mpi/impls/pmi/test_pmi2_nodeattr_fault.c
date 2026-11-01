#define _POSIX_C_SOURCE 200809L
/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "pmi2.h"

/* A waiter can exit before another rank publishes the node attribute.
 * Hydra must discard that request without losing the surviving waiter.
 *
 * This test requires Hydra -disable-auto-cleanup and co-located ranks.
 */
int main(int argc, char **argv)
{
    int spawned, size, rank, appnum;
    char value[64] = { 0 };
    int found = 0;
    int rc;

    signal(SIGUSR1, SIG_IGN);
    rc = PMI2_Init(&spawned, &size, &rank, &appnum);
    if (rc != PMI2_SUCCESS || size != 3)
        return 1;

    if (rank == 0) {
        char pid[32];
        snprintf(pid, sizeof(pid), "%ld", (long) getpid());
        if (PMI2_Info_PutNodeAttr("pmi2_fault_waiter_pid", pid) != PMI2_SUCCESS)
            return 2;

        /* This should remain blocked until the process is killed. */
        PMI2_Info_GetNodeAttr("pmi2_fault_value", value, sizeof(value), &found, 1);
        return 3;
    }

    if (rank == 1) {
        char *end;
        long pid;

        rc = PMI2_Info_GetNodeAttr("pmi2_fault_waiter_pid", value, sizeof(value), &found, 1);
        if (rc != PMI2_SUCCESS || !found)
            return 4;
        errno = 0;
        pid = strtol(value, &end, 10);
        if (errno || *end || pid <= 1)
            return 5;

        /* Allow the blocked GetNodeAttr to reach Hydra's pending queue. */
        sleep(2);
        if (kill((pid_t) pid, SIGKILL))
            return 6;

        /* Allow the proxy to process the closed socket first. */
        sleep(1);
        if (PMI2_Info_PutNodeAttr("pmi2_fault_value", "ready") != PMI2_SUCCESS)
            return 7;
    } else {
        rc = PMI2_Info_GetNodeAttr("pmi2_fault_value", value, sizeof(value), &found, 1);
        if (rc != PMI2_SUCCESS || !found || strcmp(value, "ready"))
            return 8;
    }

    if (PMI2_Finalize() != PMI2_SUCCESS)
        return 9;
    if (rank == 2)
        printf("No Errors\n");
    return 0;
}

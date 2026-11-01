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

#define CHECK_PMI2(call, exit_code) do { \
    int check_rc = (call); \
    if (check_rc != PMI2_SUCCESS) { \
        fprintf(stderr, "rank %d: %s failed (rc=%d)\n", rank, #call, check_rc); \
        return (exit_code); \
    } \
} while (0)

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
    if (rc != PMI2_SUCCESS) {
        fprintf(stderr, "PMI2_Init failed (rc=%d)\n", rc);
        return 1;
    }
    if (size != 3) {
        fprintf(stderr, "rank %d: expected size 3, got %d\n", rank, size);
        return 1;
    }

    if (rank == 0) {
        char pid[32];
        snprintf(pid, sizeof(pid), "%ld", (long) getpid());
        CHECK_PMI2(PMI2_Info_PutNodeAttr("pmi2_fault_waiter_pid", pid), 2);

        /* This should remain blocked until the process is killed. */
        rc = PMI2_Info_GetNodeAttr("pmi2_fault_value", value, sizeof(value), &found, 1);
        fprintf(stderr,
                "rank %d: unexpectedly returned from blocking GetNodeAttr (rc=%d found=%d)\n", rank,
                rc, found);
        return 3;
    }

    if (rank == 1) {
        char *end;
        long pid;

        rc = PMI2_Info_GetNodeAttr("pmi2_fault_waiter_pid", value, sizeof(value), &found, 1);
        if (rc != PMI2_SUCCESS || !found) {
            fprintf(stderr, "rank %d: waiter PID lookup rc=%d found=%d\n", rank, rc, found);
            return 4;
        }
        errno = 0;
        pid = strtol(value, &end, 10);
        if (errno || *end || pid <= 1) {
            fprintf(stderr, "rank %d: invalid waiter PID: %s\n", rank, value);
            return 5;
        }

        /* Allow the blocked GetNodeAttr to reach Hydra's pending queue. */
        sleep(2);
        if (kill((pid_t) pid, SIGKILL)) {
            fprintf(stderr, "rank %d: kill(%ld) failed: %s\n", rank, pid, strerror(errno));
            return 6;
        }

        /* Allow the proxy to process the closed socket first. */
        sleep(1);
        CHECK_PMI2(PMI2_Info_PutNodeAttr("pmi2_fault_value", "ready"), 7);
    } else {
        rc = PMI2_Info_GetNodeAttr("pmi2_fault_value", value, sizeof(value), &found, 1);
        if (rc != PMI2_SUCCESS || !found || strcmp(value, "ready")) {
            fprintf(stderr, "rank %d: surviving waiter rc=%d found=%d value=%s expected=ready\n",
                    rank, rc, found, value);
            return 8;
        }
    }

    CHECK_PMI2(PMI2_Finalize(), 9);
    if (rank == 2)
        printf("No Errors\n");
    return 0;
}

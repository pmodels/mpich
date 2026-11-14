/*
 * Copyright (C) by Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include "mpitest.h"

#ifdef MULTI_TESTS
#define run pt2pt_sendrecv_struct
int run(const char *arg);
#endif

/* Sends an array of a struct of mixed basic types (21 bytes of data, extent 24)
 * between two ranks: struct to struct, struct to MPI_PACKED and MPI_PACKED to
 * struct. The packed size doesn't divide the rendezvous chunk size, so with a
 * large enough count the chunk boundaries fall inside a field (#8142). */

struct particle {
    int64_t id;
    int a, b, c;
    unsigned char flag;
};

static void fill(struct particle *p, int n)
{
    for (int i = 0; i < n; i++) {
        p[i].id = i;
        p[i].a = i + 1;
        p[i].b = i + 2;
        p[i].c = i + 3;
        p[i].flag = i & 0xff;
    }
}

static int check(const struct particle *p, int n, const char *name)
{
    int errs = 0;
    for (int i = 0; i < n; i++) {
        if (p[i].id != i || p[i].a != i + 1 || p[i].b != i + 2 || p[i].c != i + 3 ||
            p[i].flag != (i & 0xff)) {
            if (errs < 5) {
                printf("%s: particle %d is wrong\n", name, i);
            }
            errs++;
        }
    }
    return errs;
}

int run(const char *arg)
{
    int errs = 0;
    int rank, size;

    MTestArgList *head = MTestArgListCreate_arg(arg);
    int count = MTestArgListGetInt_with_default(head, "count", 60000);
    MTestArgListDestroy(head);

    MPI_Comm comm = MPI_COMM_WORLD;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);
    if (size < 2) {
        printf("This test requires 2 processes\n");
        return 1;
    }

    int blens[5] = { 1, 1, 1, 1, 1 };
    MPI_Aint displs[5] = { offsetof(struct particle, id), offsetof(struct particle, a),
        offsetof(struct particle, b), offsetof(struct particle, c),
        offsetof(struct particle, flag)
    };
    MPI_Datatype types[5] = { MPI_INT64_T, MPI_INT, MPI_INT, MPI_INT, MPI_UNSIGNED_CHAR };
    MPI_Datatype tmp_type, particle_type;
    MPI_Type_create_struct(5, blens, displs, types, &tmp_type);
    MPI_Type_create_resized(tmp_type, 0, sizeof(struct particle), &particle_type);
    MPI_Type_commit(&particle_type);
    MPI_Type_free(&tmp_type);

    int pack_size, position;
    MPI_Pack_size(count, particle_type, comm, &pack_size);

    struct particle *p = calloc(count, sizeof(struct particle));
    char *packbuf = malloc(pack_size);

    /* struct to struct */
    if (rank == 0) {
        fill(p, count);
        MPI_Send(p, count, particle_type, 1, 0, comm);
    } else if (rank == 1) {
        MPI_Recv(p, count, particle_type, 0, 0, comm, MPI_STATUS_IGNORE);
        errs += check(p, count, "struct to struct");
    }

    /* struct to MPI_PACKED */
    if (rank == 0) {
        MPI_Send(p, count, particle_type, 1, 1, comm);
    } else if (rank == 1) {
        MPI_Recv(packbuf, pack_size, MPI_PACKED, 0, 1, comm, MPI_STATUS_IGNORE);
        for (int i = 0; i < count; i++) {
            p[i].id = -1;
        }
        position = 0;
        MPI_Unpack(packbuf, pack_size, &position, p, count, particle_type, comm);
        errs += check(p, count, "struct to MPI_PACKED");
    }

    /* MPI_PACKED to struct */
    if (rank == 0) {
        position = 0;
        MPI_Pack(p, count, particle_type, packbuf, pack_size, &position, comm);
        MPI_Send(packbuf, position, MPI_PACKED, 1, 2, comm);
    } else if (rank == 1) {
        for (int i = 0; i < count; i++) {
            p[i].id = -1;
        }
        MPI_Recv(p, count, particle_type, 0, 2, comm, MPI_STATUS_IGNORE);
        errs += check(p, count, "MPI_PACKED to struct");
    }

    free(packbuf);
    free(p);
    MPI_Type_free(&particle_type);

    return errs;
}

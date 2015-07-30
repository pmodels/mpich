/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2014 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include "mpitest.h"
#include <stdlib.h>
#include <stdio.h>
/* The next is for isprint */
#include <ctype.h>

int main(int argc, char *argv[])
{
    struct a {
        int i;
        char c;
    } s[10], s1[10];
    int j;
    int errs = 0, toterrs;
    int rank, size, tsize;
    MPI_Aint text;
    int blens[2];
    MPI_Aint disps[2];
    MPI_Datatype bases[2];
    MPI_Datatype str, con;
    char *buffer;
    int bufsize, position, insize;

    MTest_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    for (j = 0; j < 10; j++) {
        s[j].i = j + rank;
        s[j].c = j + rank + 'a';
    }

    blens[0] = blens[1] = 1;
    disps[0] = 0;
    disps[1] = sizeof(int);
    bases[0] = MPI_INT;
    bases[1] = MPI_CHAR;
    MPI_Type_struct(2, blens, disps, bases, &str);
    MPI_Type_commit(&str);
    MPI_Type_contiguous(10, str, &con);
    MPI_Type_commit(&con);
    MPI_Type_size(con, &tsize);
    MPI_Type_extent(con, &text);

#ifdef DEBUG
    printf("Size of MPI array is %d, extent is %d\n", tsize, text);
#endif

#ifdef DEBUG
    {
        void *p1, *p2;
        p1 = s;
        p2 = &(s[10].i);        /* This statement may fail on some systems */
        printf("C array starts at %p and ends at %p for a length of %d\n",
               s, &(s[9].c), (char *) p2 - (char *) p1);
    }
#endif
    MPI_Type_extent(str, &text);
#ifdef DEBUG
    MPI_Type_size(str, &tsize);
    printf("Size of MPI struct is %d, extent is %d\n", tsize, (int) text);
    printf("Size of C struct is %d\n", sizeof(struct a));
#endif
    if (text != sizeof(struct a)) {
        printf("Extent of struct a (%d) does not match sizeof (%d)\n",
               (int) text, (int) sizeof(struct a));
        errs++;
    }

    MPI_Pack_size(1, con, MPI_COMM_WORLD, &bufsize);
    buffer = (char *) malloc(bufsize);

    position = 0;
    MPI_Pack(s, 1, con, buffer, bufsize, &position, MPI_COMM_WORLD);
    insize = position;
    position = 0;
    MPI_Unpack(buffer, insize, &position, s1, 1, con, MPI_COMM_WORLD);

    for (j = 0; j < 10; j++) {
#ifdef DEBUG
        printf("%d Sent: %d %c, Got: %d %c\n", rank, s[j].i, s[j].c, s1[j].i, s1[j].c);
#endif
        if (s1[j].i != j + rank) {
            errs++;
            printf("Got s[%d].i = %d (%x); expected %d\n", j, s1[j].i, s1[j].i, j + rank);
        }
        if (s1[j].c != 'a' + j + rank) {
            errs++;
            /* If the character is not a printing character,
             * this can generate an file that diff, for example,
             * believes is a binary file */
            if (isprint((int) (s1[j].c))) {
                printf("Got s[%d].c = %c; expected %c\n", j, s1[j].c, j + rank + 'a');
            }
            else {
                printf("Got s[%d].c = %x; expected %c\n", j, (int) s1[j].c, j + rank + 'a');
            }
        }
    }

    MPI_Type_free(&str);
    MPI_Type_free(&con);
    MTest_Finalize(errs);
    MPI_Finalize();
    return 0;
}

/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* FIXME: We really should consider internationalizing the output from this
   program */
/* style: allow:fprintf:1 sig:0 */
/* style: allow:printf:8 sig:0 */

/*
 * This program reports on properties of the MPICH library, such as the
 * version, device, and what patches have been applied.  This is available
 * only since MPICH 1.0.6.
 *
 * The reason that this program doesn't directly include the info is that it
 * can be compiled and then linked with the MPICH library to discover
 * the information about the version of the library.  If built with shared
 * libraries, this will give the information about the currently installed
 * shared library.
 */

typedef enum {
    Version_number = 0,
    Date,
    Patches,
    Configure_args,
    Device,
    Compilers,
    Custom,
    Features,
    ABI,
    CC,
    CXX,
    F77,
    FC,
    LastField
} fields;

/*D
  mpichversion - Report on the MPICH version

  Command Line Arguments:
+ -version - Show the version of MPICH
. -date    - Show the release date of this version
. -patches - Show the identifiers for any applied patches
. -configure - Show the configure arguments used to build MPICH
- -device  - Show the device for which MPICH was configured

  Using this program:
  To use this program, link it against 'libmpi.a' (use 'mpicc' or
  the whichever compiler command is used to create MPICH programs)
  D*/

int main(int argc, char *argv[])
{
    char version[MPI_MAX_LIBRARY_VERSION_STRING];
    int versionlen;

    MPI_Get_library_version(version, &versionlen);
    if (argc <= 1 || strncmp("MPICH ", version, 6) != 0) {
        printf("%s\n", version);
        return 0;
    }

    /* parse the command line options */
    int flags[20];

    /* Show only requested values */
    for (int i = 0; i < LastField; i++)
        flags[i] = 0;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-version") == 0 ||
            strcmp(argv[i], "--version") == 0 || strcmp(argv[i], "-v") == 0)
            flags[Version_number] = 1;
        else if (strcmp(argv[i], "-date") == 0 ||
                 strcmp(argv[i], "--date") == 0 || strcmp(argv[i], "-D") == 0)
            flags[Date] = 1;
        else if (strcmp(argv[i], "-patches") == 0)
            flags[Patches] = 1;
        else if (strcmp(argv[i], "-configure") == 0 ||
                 strcmp(argv[i], "--configure") == 0 || strcmp(argv[i], "-c") == 0)
            flags[Configure_args] = 1;
        else if (strcmp(argv[i], "-device") == 0 ||
                 strcmp(argv[i], "--device") == 0 || strcmp(argv[i], "-d") == 0)
            flags[Device] = 1;
        else if (strcmp(argv[i], "-compiler") == 0 ||
                 strcmp(argv[i], "--compiler") == 0 || strcmp(argv[i], "-b") == 0)
            flags[Compilers] = 1;
        else if (strcmp(argv[i], "-custom") == 0 ||
                 strcmp(argv[i], "--custom") == 0 || strcmp(argv[i], "-u") == 0)
            flags[Custom] = 1;
        else if (strcmp(argv[i], "-features") == 0 ||
                 strcmp(argv[i], "--features") == 0 || strcmp(argv[i], "-f") == 0)
            flags[Features] = 1;
        else {
            fprintf(stderr, "Unrecognized argument %s\n", argv[i]);
            exit(1);
        }
    }

    /* parse MPICH version text */
    char *strs[20];
    for (int i = 0; i < 20; i++) {
        strs[i] = NULL;
    }

    char *s = version;

#define MATCH(head, idx) \
    else if (strncmp(s, head, strlen(head)) == 0) { \
        strs[idx] = s; \
        s += strlen(head); \
    }

    while (*s) {
        /* *INDENT-OFF* */
        if (*s == '\n' || *s == '\r') {
            /* terminate the sub-string */
            *s = '\0';
            s++;
        }
        MATCH("MPICH Version:", Version_number)
        MATCH("MPICH Release date:", Date)
        MATCH("MPICH Device:", Device)
        MATCH("MPICH configure:", Configure_args)
        MATCH("MPICH ABI:", ABI)
        MATCH("MPICH CC:", CC)
        MATCH("MPICH CXX:", CXX)
        MATCH("MPICH F77:", F77)
        MATCH("MPICH FC:", FC)
        MATCH("MPICH features:", Features)
        else if (strncmp(s, "MPICH Custom Information:", 25) == 0) {
            strs[Custom] = s;
            /* grab the rest of the text */
            break;
        } else {
            s++;
        }
        /* *INDENT-ON* */
    }

    /* Print out the information, one item per line */
    if (flags[Version_number] && strs[Version_number]) {
        printf("%s\n", strs[Version_number]);
    }
    if (flags[Date] && strs[Date]) {
        printf("%s\n", strs[Date]);
    }
    if (flags[Device] && strs[Device]) {
        printf("%s\n", strs[Device]);
    }
    if (flags[Configure_args] && strs[Configure_args]) {
        printf("%s\n", strs[Configure_args]);
    }
    if (flags[Compilers]) {
        if (strs[CC]) {
            printf("%s\n", strs[CC]);
        }
        if (strs[CXX]) {
            printf("%s\n", strs[CXX]);
        }
        if (strs[F77]) {
            printf("%s\n", strs[F77]);
        }
        if (strs[FC]) {
            printf("%s\n", strs[FC]);
        }
    }
    if (flags[Features] && strs[Features]) {
        printf("%s\n", strs[Features]);
    }
    if (flags[Custom] && strs[Custom]) {
        printf("%s\n", strs[Custom]);
    }

    return 0;
}

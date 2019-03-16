#ifndef INCLUDE_TEST_IO_H_
#define INCLUDE_TEST_IO_H_

#include <string.h>

/* Each IO tests need open a unique filename in order to be able to
   run multiple tests in parallel. This header implements a filename
   in the pattern of "testfile.id", where "id" is passed in from command
   line in the form of "-id=#". This argument is automatically generated
   by test framework, e.g. `runtests.pl`.
*/

#define INIT_FILENAME \
    char filename[50]="testfile"

#define GET_TEST_FILENAME \
    do { \
        int i = 1; \
        while (i < argc) { \
            if (strncmp(argv[i], "-id=", 4) == 0) { \
                strcat(filename, "."); \
                strcat(filename, argv[i]+4); \
                break; \
            } \
            i++; \
        } \
    } while (0)

#define GET_TEST_FILENAME_PER_RANK \
    GET_TEST_FILENAME; \
    sprintf(filename+strlen(filename), ".%d", rank)

#endif

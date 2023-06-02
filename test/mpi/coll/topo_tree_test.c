#include "mpi.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include "mpitest.h"


int tree_is_valid(const char *pattern)
{
    char cmd[1024];

    sprintf(cmd, "python validate_tree.py \"%s\" 2>&1", pattern);
    FILE *out = popen(cmd, "r");
    if (NULL == out) {
        fprintf(stderr, "popen \"%s\" failed: %s\n", cmd, strerror(errno));
        return false;
    } else {
        /* Dump all output from the command. We expect output only when there are errors. */
        char line[1024];
        while (NULL != fgets(line, sizeof(line), out)) {
            fprintf(stderr, line);
        }

        int result = pclose(out);

        return (0 == result);
    }
}


void remove_files(const char *pattern)
{
    char cmd[1024];
    sprintf(cmd, "/bin/rm -f %s 2>/dev/null", pattern);
    system(cmd);
}


int main(int argc, char **argv)
{
    int myrank;
    int errs = 0;

    const char *ACTUAL_TREE_ENV_VAR_NAME = "MPIR_CVAR_TREE_DUMP_FILE_PREFIX";
    const char *actual_tree_path_prefix = getenv(ACTUAL_TREE_ENV_VAR_NAME);
    char actual_tree_path_pattern[PATH_MAX];
    sprintf(actual_tree_path_pattern, "%s*.json", actual_tree_path_prefix);

    /* clean up files before running */
    if (actual_tree_path_prefix != NULL)
        remove_files(actual_tree_path_pattern);

    MTest_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank);

    int n = 0;
    MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);

    MTest_Finalize(errs);

    /* check after finalize to make sure all files are written */
    if (0 == myrank) {
        if (NULL == actual_tree_path_prefix || 0 == strlen(actual_tree_path_prefix)) {
            fprintf(stderr, "Error: expected %s variable is not set.\n", ACTUAL_TREE_ENV_VAR_NAME);
            errs++;
        } else if (!tree_is_valid(actual_tree_path_pattern)) {
            errs++;
        }
        /* TODO: Add a check that the actual tree matches the given expected tree. */
        if (actual_tree_path_prefix != NULL)
            remove_files(actual_tree_path_pattern);
    }

    return MTestReturnValue(errs);
}

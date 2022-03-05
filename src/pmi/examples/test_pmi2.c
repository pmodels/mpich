#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "pmi2.h"

int main(int argc, char **argv)
{
    char jobid[1024];
    int has_paraent, rank, size, appnum;

    PMI2_Init(&has_paraent, &size, &rank, &appnum);
    fprintf(stdout, "    :has_paraent=%d, rank=%d, size=%d, appnum=%d\n",
            has_paraent, rank, size, appnum);

    PMI2_Job_GetId(jobid, 1024);

    if (rank == 0) {
        PMI2_KVS_Put("key0", "got key0");
    }

    PMI2_KVS_Fence();

    if (rank == 1) {
        char buf[100];
        int out_len;

        PMI2_KVS_Get(jobid, 0, "key0", buf, 100, &out_len);
        fprintf(stdout, "    got key0: %s (%d bytes)\n", buf, out_len);
    }

    PMI2_Finalize();
    return 0;
}

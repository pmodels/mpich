#include <mpitest.h>
#include <stdio.h>

#define CHECK(errcode, fn) if (errcode != MPI_SUCCESS) handle_error(errcode, fn)

static void handle_error(int errcode, char *str)
{
    char msg[MPI_MAX_ERROR_STRING];
    int resultlen;
    MPI_Error_string(errcode, msg, &resultlen);
    fprintf(stderr, "%s: %s\n", str, msg);
    MPI_Abort(MPI_COMM_WORLD, 1);
}


int main(int argc, char *argv[])
{
    int errs = 0;
    int rc;
    int my_rank;
    MPI_Info info;
    MPI_Info info_used;
    MPI_File fh;

    MTest_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

    MPI_Info_create(&info);
    MPI_Info_set(info, "romio_no_indep_rw", "true");

    rc = MPI_File_open(MPI_COMM_WORLD, (char *) "test.txt", MPI_MODE_CREATE | MPI_MODE_WRONLY, info,
                       &fh);
    CHECK(rc, "MPI_File_open");

    rc = MPI_File_get_info(fh, &info_used);
    CHECK(rc, "MPI_File_get_info");

    if (info_used != MPI_INFO_NULL) {
        int i, nkeys;

        MPI_Info_get_nkeys(info_used, &nkeys);

        for (i = 0; i < nkeys; i++) {
            char key[MPI_MAX_INFO_KEY], value[MPI_MAX_INFO_VAL];
            int valuelen, flag;

            MPI_Info_get_nthkey(info_used, i, key);
            MPI_Info_get_valuelen(info_used, key, &valuelen, &flag);
            MPI_Info_get(info_used, key, valuelen + 1, value, &flag);

            MPI_Info_set(info, key, value);
        }

        MPI_Info_free(&info_used);
    }

    rc = MPI_File_set_view(fh, 0, MPI_INT, MPI_INT, "native", info);
    CHECK(rc, "MPI_File_set_view");

    rc = MPI_File_close(&fh);
    CHECK(rc, "MPI_File_close");

    MPI_Info_free(&info);

    MTest_Finalize(errs);

    return MTestReturnValue(errs);
}

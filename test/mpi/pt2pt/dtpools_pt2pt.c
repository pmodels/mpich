#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include "mpitest.h"
#include "dtpools.h"
#include <assert.h>

int main(int argc, char **argv)
{
    int err = 0;
    int errs = 0;


    MTest_Init(&argc, &argv);

    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    int count = 262144;
    DTP_pool_s dtp;
    err = DTP_pool_create("MPI_INT", count, 0, &dtp);
    assert(err == DTP_SUCCESS);

    /* *INDENT-OFF* */
    const char *s_send_obj = NULL;
    const char *s_recv_obj = "0: blkindx [numblks 1, blklen 131072, displs (0)]"
                             "1: resized [lb -400, extent 4]"
                             "2: resized [lb 0, extent 40]";
    /* *INDENT-ON* */
    DTP_obj_s dtp_obj;
    void *buf;
    if (rank == 0) {
        err = DTP_obj_create_custom(dtp, &dtp_obj, s_send_obj);
        assert(err == DTP_SUCCESS);
        buf = malloc(dtp_obj.DTP_bufsize);

        err = DTP_obj_buf_init(dtp_obj, buf, 0, 1, count);
        assert(err == DTP_SUCCESS);
        err =
            MPI_Send(buf + dtp_obj.DTP_buf_offset, dtp_obj.DTP_type_count, dtp_obj.DTP_datatype, 1,
                     0, MPI_COMM_WORLD);
        assert(err == MPI_SUCCESS);

        free(buf);
        DTP_obj_free(dtp_obj);
    } else if (rank == 1) {
        err = DTP_obj_create_custom(dtp, &dtp_obj, s_recv_obj);
        assert(err == DTP_SUCCESS);
        buf = malloc(dtp_obj.DTP_bufsize);

        err = DTP_obj_buf_init(dtp_obj, buf, -1, -1, count);
        assert(err == DTP_SUCCESS);
        err =
            MPI_Recv(buf + dtp_obj.DTP_buf_offset, dtp_obj.DTP_type_count, dtp_obj.DTP_datatype, 0,
                     0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        assert(err == MPI_SUCCESS);
        err = DTP_obj_buf_check(dtp_obj, buf, 0, 1, count);
        if (err) {
            errs++;
        }

        free(buf);
        DTP_obj_free(dtp_obj);
    }

    DTP_pool_free(dtp);

    MTest_Finalize(errs);
    return MTestReturnValue(errs);
    return 0;
}

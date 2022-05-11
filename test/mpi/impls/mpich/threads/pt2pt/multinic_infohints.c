/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"
#include "mpi.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* This test only works for ch4:ofi */
#ifndef MPICH_CH4_OFI
int main(void)
{
    printf("Test Skipped\n");
    return 0;
}
#else

#define MAX_NICS_SUPPORTED 8

/* Multinic Support: Using pref_close_nic user info hint set */

static unsigned long long nic_sent_bytes_count[MAX_NICS_SUPPORTED];
static unsigned long long nic_recvd_bytes_count[MAX_NICS_SUPPORTED];
static MPI_T_pvar_handle nsc_handle, nrc_handle;
static MPI_T_pvar_session session;
static int elems = 1000000;
static MPI_Request *reqs;
static MPI_Status *reqstat;
static int num_nics = 1;
static int enable_striping = 1;
static int enable_multiplexing = 0;
static int nprocs = 0;

static int check_nic_pvar(int rank, int num_nics, unsigned long long *nic_pvar_bytes_count)
{
    int idx;

    unsigned long long nic_expected_bytes_count[MAX_NICS_SUPPORTED];
    unsigned long long total_elems_byte_size = elems * sizeof(float);

    /* Expected byte count numbers based on the number of NICs in use.
     * /* The expected counts are determined assuming multi-nic striping and
     * * hashing are disabled because this test uses multi_nic_pref_nic hint
     * * to select nics. */
    assert(!enable_multiplexing && !enable_striping);
    for (idx = 0; idx < num_nics; ++idx) {
        nic_expected_bytes_count[idx] = total_elems_byte_size;
    }

    for (idx = 0; idx < num_nics; ++idx) {
        /* If the actual byte count does not match with any of the expected count in the list */
        /* return with error */
        if (rank == 0) {
            if (nic_expected_bytes_count[idx] != nic_pvar_bytes_count[idx]) {
                printf
                    ("rank=%d --> target=1: Actual sent byte count=%ld through NIC %d does not match with the "
                     "expected sent byte count=%ld\n", rank, nic_pvar_bytes_count[idx], idx,
                     nic_expected_bytes_count[idx]);
                return 1;
            }
        } else {
            if (nic_expected_bytes_count[idx] != nic_pvar_bytes_count[idx]) {
                printf
                    ("rank=%d --> target=0: Actual received byte count=%ld through NIC %d does not match with the "
                     "expected received byte count=%ld\n", rank, nic_pvar_bytes_count[idx],
                     idx, nic_expected_bytes_count[idx]);
                return 1;
            }
        }
    }
    return 0;
}

int main(int argc, char *argv[])
{
    int num;
#define STR_SZ (50)
    int name_len = STR_SZ;
    char name[STR_SZ] = "";
    int desc_len = STR_SZ;
    char desc[STR_SZ] = "";
    int verb;
    MPI_Datatype dtype;
    int count;
    int bind;
    int varclass;
    int readonly, continuous, atomic;
    MPI_T_enum enumtype;
    int nsc_continuous, nrc_continuous;
    int nsc_idx = -1, nrc_idx = -1;

    int required, provided;
    int errs = 0;
    int rank, i, j, k;
    float *in_buf, *out_buf;
    MPI_Comm comm;
    MPI_Info comm_info;
    MPI_Info comm_info_out;
    char query_key[MPI_MAX_INFO_KEY];
    char buf[MPI_MAX_INFO_VAL];
    char val[MPI_MAX_INFO_VAL];
    int flag;
    int target;
    static char pref_close_nic[50] = { 0 };

    /* Initialize the test */
    MTest_Init(&argc, &argv);

    /* Main comm */
    comm = MPI_COMM_WORLD;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &nprocs);

    if (nprocs != 2) {
        if (rank == 0) {
            fprintf(stderr, "This test requires exactly two processes\n");
        }

        MPI_Finalize();
        return MPI_ERR_OTHER;
    }

    /* Create info object */
    MPI_Info_create(&comm_info);

    MPI_Comm_get_info(comm, &comm_info_out);
    memset(buf, 0, MPI_MAX_INFO_VAL * sizeof(char));

    /* Determine the number of nics in use */
    MPI_Info_get(MPI_INFO_ENV, "num_nics", MPI_MAX_INFO_VAL, buf, &flag);
    if (!flag) {
        num_nics = 1;
    } else {
        num_nics = atoi(buf);
    }
    MTestPrintfMsg(1, "detected %d nics at rank %d\n", num_nics, rank);

    /* Determine the striping optimization is enabled */
    MPI_Info_get(comm_info_out, "enable_multi_nic_striping", MPI_MAX_INFO_VAL, buf, &flag);
    if (!flag) {
        fprintf(stderr, "Error while reading hint enable_striping\n");
        return MPI_ERR_OTHER;
    }
    enable_striping = (strcmp(buf, "true") == 0);

    /* Determine the multiplexing optimization is enabled */
    MPI_Info_get(comm_info_out, "enable_multi_nic_hashing", MPI_MAX_INFO_VAL, buf, &flag);
    if (!flag) {
        fprintf(stderr, "Error while reading hint enable_multiplexing\n");
        return MPI_ERR_OTHER;
    }
    enable_multiplexing = (strcmp(buf, "true") == 0);

    if (enable_multiplexing) {
        fprintf(stderr, "Multiplexing should be turned off for this test\n");
        return MPI_ERR_OTHER;
    }
    if (enable_striping) {
        fprintf(stderr, "Multi nic hashing should be turned off for this test\n");
        return MPI_ERR_OTHER;
    }

    MPI_Info_free(&comm_info_out);

    MPI_Info dup_comm_info;
    MPI_Info_create(&dup_comm_info);

    MPI_Comm world[num_nics];
    for (i = 0; i < num_nics; i++) {
        /* Create duplicate comms */
        MPI_Comm_dup(comm, &world[i]);
        /* Set info hints to all the comms created */
        MPI_Info_set(dup_comm_info, "mpi_assert_no_any_source", "true");
        MPI_Info_set(dup_comm_info, "mpi_assert_no_any_tag", "true");
        snprintf(pref_close_nic, sizeof(pref_close_nic), "%d", i);
        MPI_Info_set(dup_comm_info, "multi_nic_pref_nic", pref_close_nic);
        MPI_Comm_set_info(world[i], dup_comm_info);
    }

    MPI_Info_free(&dup_comm_info);

    required = MPI_THREAD_SINGLE;
    MPI_T_init_thread(required, &provided);
    MTest_Init_thread(&argc, &argv, required, &provided);
    MPI_T_pvar_get_num(&num);
    for (i = 0; i < num; ++i) {
        name_len = desc_len = STR_SZ;
        MPI_T_pvar_get_info(i, name, &name_len, &verb, &varclass, &dtype, &enumtype, desc,
                            &desc_len, &bind, &readonly, &continuous, &atomic);

        if (0 == strcmp(name, "nic_sent_bytes_count")) {
            nsc_idx = i;
            nsc_continuous = continuous;
        }
        if (0 == strcmp(name, "nic_recvd_bytes_count")) {
            nrc_idx = i;
            nrc_continuous = continuous;
        }
    }

    /* Execute only when PVARs are enabled */
    if (nsc_idx != -1 && nrc_idx != -1) {
        /* Setup a session and handles for the PVAR variables */
        session = MPI_T_PVAR_SESSION_NULL;
        MPI_T_pvar_session_create(&session);
        assert(session != MPI_T_PVAR_SESSION_NULL);

        nsc_handle = MPI_T_PVAR_HANDLE_NULL;
        MPI_T_pvar_handle_alloc(session, nsc_idx, NULL, &nsc_handle, &count);
        assert(count >= num_nics);
        assert(nsc_handle != MPI_T_PVAR_HANDLE_NULL);

        nrc_handle = MPI_T_PVAR_HANDLE_NULL;
        MPI_T_pvar_handle_alloc(session, nrc_idx, NULL, &nrc_handle, &count);
        assert(count >= num_nics);
        assert(nrc_handle != MPI_T_PVAR_HANDLE_NULL);

        if (!nsc_continuous)
            MPI_T_pvar_start(session, nsc_handle);
        if (!nrc_continuous)
            MPI_T_pvar_start(session, nrc_handle);
    }

    reqs = (MPI_Request *) malloc((nprocs - 1) * num_nics * sizeof(MPI_Request));
    in_buf = (float *) malloc(elems * num_nics * sizeof(float));
    out_buf = (float *) malloc(elems * num_nics * sizeof(float));
    MTEST_VG_MEM_INIT(out_buf, elems * num_nics * sizeof(float));

    if (rank == 0) {
        target = 1;
        for (i = 0; i < num_nics; i++) {
            MPI_Isend(&out_buf[elems * i], elems, MPI_FLOAT, target, i, world[i], &reqs[i]);
        }
        MPI_Waitall(num_nics, reqs, MPI_STATUS_IGNORE);
    } else {
        target = 0;
        for (i = 0; i < num_nics; i++) {
            MPI_Irecv(&in_buf[elems * i], elems, MPI_FLOAT, target, i, world[i], &reqs[i]);
        }
        MPI_Waitall(num_nics, reqs, MPI_STATUS_IGNORE);
    }

    free(reqs);
    free(in_buf);
    free(out_buf);

    /* Execute only when PVARs are enabled */
    if (nsc_idx != -1 && nrc_idx != -1) {

        if (!nsc_continuous)
            MPI_T_pvar_stop(session, nrc_handle);
        if (!nrc_continuous)
            MPI_T_pvar_stop(session, nsc_handle);

        /* read the pvars from the session and compare with expected counts */
        if (rank == 0) {
            MPI_T_pvar_read(session, nsc_handle, &nic_sent_bytes_count);
            errs += check_nic_pvar(rank, num_nics, nic_sent_bytes_count);
        } else {        /* rank == 1 */
            MPI_T_pvar_read(session, nrc_handle, &nic_recvd_bytes_count);
            errs += check_nic_pvar(rank, num_nics, nic_recvd_bytes_count);
        }

        /* Cleanup */
        MPI_T_pvar_handle_free(session, &nrc_handle);
        MPI_T_pvar_handle_free(session, &nsc_handle);
        MPI_T_pvar_session_free(&session);
    }
    MPI_T_finalize();
    MPI_Info_free(&comm_info);
    for (i = 0; i < num_nics; i++) {
        MPI_Comm_free(&world[i]);
    }
    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
#endif /* MPICH_CH4_OFI */

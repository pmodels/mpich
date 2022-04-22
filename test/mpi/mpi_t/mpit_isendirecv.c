/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpi.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "mpitest.h"

#define MAX_NICS_SUPPORTED 8

static unsigned long long nic_sent_bytes_count[MAX_NICS_SUPPORTED];
static unsigned long long nic_recvd_bytes_count[MAX_NICS_SUPPORTED];
static unsigned long long striped_nic_sent_bytes_count[MAX_NICS_SUPPORTED];
static unsigned long long striped_nic_recvd_bytes_count[MAX_NICS_SUPPORTED];
static MPI_T_pvar_handle nsc_handle, nrc_handle, snsc_handle, snrc_handle;
static MPI_T_pvar_session session;
static int elems = 1000000;
static MPI_Request *reqs;
static int enable_striping = 1;
static int enable_hashing = 0;
static int num_nodes = 0;
static int num_procs = 0;
static int num_procs_per_node = 0;
static int num_pairs = 0;

/* removes duplicates in the char array */
static int remove_duplicates(char nodenames[][MPI_MAX_PROCESSOR_NAME], int size)
{
    int i, j, k;

    for (i = 0; i < size; i++) {
        for (j = i + 1, k = j; j < size; j++) {
            /* strings do not match */
            if (strcmp(nodenames[i], nodenames[j])) {
                snprintf(nodenames[k], sizeof(nodenames[j]), "%s", nodenames[j]);
                k++;
            }
        }
        size -= j - k;
    }
    return size;
}

static int compare_vars(int rank, int target, int num_nics)
{
    int idx;
    int errs = 0;

    /* expected output */
    struct ExpectedMapNicTestOutput {
        unsigned long long nic_sent_bytes_count[MAX_NICS_SUPPORTED];
        unsigned long long nic_recvd_bytes_count[MAX_NICS_SUPPORTED];
        unsigned long long striped_nic_sent_bytes_count[MAX_NICS_SUPPORTED];
        unsigned long long striped_nic_recvd_bytes_count[MAX_NICS_SUPPORTED];
    };

    unsigned long long total_elems_byte_size = elems * sizeof(float);
    int init_normal_msg_send_bytes = 2048;

    /* expected byte count numbers based on the number of NICs in use */
    /* the number of send and recv byte count between each pair must match */
    struct ExpectedMapNicTestOutput expected_mapping[2] = { 0 };
    for (idx = 0; idx < num_nics; ++idx) {
        int bytes_through_each_nic = (num_nics > idx ? total_elems_byte_size : 0);
        if (enable_hashing && !enable_striping) {
            expected_mapping[0].nic_sent_bytes_count[idx] += bytes_through_each_nic;
            expected_mapping[1].nic_recvd_bytes_count[idx] += bytes_through_each_nic;
        } else if (!enable_hashing && enable_striping) {
            expected_mapping[1].nic_recvd_bytes_count[idx] +=
                bytes_through_each_nic - init_normal_msg_send_bytes;
            expected_mapping[1].striped_nic_recvd_bytes_count[idx] +=
                bytes_through_each_nic - init_normal_msg_send_bytes;
        } else if (enable_hashing && enable_striping) {
            expected_mapping[0].nic_sent_bytes_count[idx] += init_normal_msg_send_bytes;
            expected_mapping[1].nic_recvd_bytes_count[idx] += bytes_through_each_nic;
            expected_mapping[0].striped_nic_sent_bytes_count[idx] += init_normal_msg_send_bytes;
            expected_mapping[1].striped_nic_recvd_bytes_count[idx] +=
                bytes_through_each_nic - init_normal_msg_send_bytes;
        } else if (!enable_hashing && !enable_striping) {
            if (rank < num_pairs) {
                /* map the ranks such that it falls within the range of number of NICs available */
                expected_mapping[0].nic_sent_bytes_count[(rank % num_procs_per_node) %
                                                         num_nics] += bytes_through_each_nic;
            } else if (rank < num_pairs * 2) {
                /* map the ranks such that it falls within the range of number of NICs available */
                expected_mapping[1].nic_recvd_bytes_count[((rank - num_pairs)
                                                           % num_procs_per_node) %
                                                          num_nics] += bytes_through_each_nic;
            }
        }
    }

    /* expected initial send message count when striping is enabled
     * based on rank<->NIC mapping */
    if (!enable_hashing && enable_striping) {
        if (rank < num_pairs) {
            expected_mapping[0].nic_sent_bytes_count[rank] += (2048 * num_nics);
            expected_mapping[0].striped_nic_sent_bytes_count[rank] += (2048 * num_nics);
        } else if (rank < num_pairs * 2) {
            expected_mapping[1].nic_recvd_bytes_count[rank - num_pairs] += (2048 * num_nics);
        }
    }

    /* read the pvars from the session */
    MPI_T_pvar_read(session, nsc_handle, &nic_sent_bytes_count);
    MPI_T_pvar_read(session, nrc_handle, &nic_recvd_bytes_count);
    MPI_T_pvar_read(session, snsc_handle, &striped_nic_sent_bytes_count);
    MPI_T_pvar_read(session, snrc_handle, &striped_nic_recvd_bytes_count);

    for (idx = 0; idx < num_nics; ++idx) {
        /* if the actual byte count does not match with any of the expected count in the list
         * return with error */
        if (rank < num_pairs) {
            if (expected_mapping[0].nic_sent_bytes_count[idx] != nic_sent_bytes_count[idx]) {
                printf
                    ("rank=%d --> target=%d: Actual sent byte count=%llu through NIC %d does not match with the "
                     "expected sent byte count=%llu\n", rank, target, nic_sent_bytes_count[idx],
                     idx, expected_mapping[0].nic_sent_bytes_count[idx]);
                errs++;
            }
            if (expected_mapping[0].nic_recvd_bytes_count[idx] != nic_recvd_bytes_count[idx]) {
                printf
                    ("rank=%d --> target=%d: Actual received byte count=%llu through NIC %d does not match with the "
                     "expected received byte count=%llu\n", rank, target,
                     nic_recvd_bytes_count[idx], idx,
                     expected_mapping[0].nic_recvd_bytes_count[idx]);
                errs++;
            }
            if (expected_mapping[0].striped_nic_sent_bytes_count[idx] !=
                striped_nic_sent_bytes_count[idx]) {
                printf
                    ("rank=%d --> target=%d: Actual striped sent byte count=%llu through NIC %d does not match with the "
                     "expected striped sent byte count=%llu\n", rank, target,
                     striped_nic_sent_bytes_count[idx], idx,
                     expected_mapping[0].striped_nic_sent_bytes_count[idx]);
                return -1;
            }
            if (expected_mapping[0].striped_nic_recvd_bytes_count[idx] !=
                striped_nic_recvd_bytes_count[idx]) {
                printf
                    ("rank=%d --> target=%d: Actual striped received byte count=%llu through NIC %d does not match with the "
                     "expected striped received byte count=%llu\n", rank, target,
                     striped_nic_recvd_bytes_count[idx], idx,
                     expected_mapping[0].striped_nic_recvd_bytes_count[idx]);
                return -1;
            }
        } else if (rank < num_pairs * 2) {
            if (expected_mapping[1].nic_sent_bytes_count[idx] != nic_sent_bytes_count[idx]) {
                printf
                    ("rank=%d --> target=%d: Actual sent byte count=%llu through NIC %d does not match with the "
                     "expected sent byte count=%llu\n", rank, target, nic_sent_bytes_count[idx],
                     idx, expected_mapping[1].nic_sent_bytes_count[idx]);
                errs++;
            }
            if (expected_mapping[1].nic_recvd_bytes_count[idx] != nic_recvd_bytes_count[idx]) {
                printf
                    ("rank=%d --> target=%d: Actual received byte count=%llu through NIC %d does not match with the "
                     "expected received byte count=%llu\n", rank, target,
                     nic_recvd_bytes_count[idx], idx,
                     expected_mapping[1].nic_recvd_bytes_count[idx]);
                errs++;
            }
            if (expected_mapping[1].striped_nic_sent_bytes_count[idx] !=
                striped_nic_sent_bytes_count[idx]) {
                printf
                    ("rank=%d --> target=%d: Actual striped sent byte count=%llu through NIC %d does not match with the "
                     "expected striped sent byte count=%llu\n", rank, target,
                     striped_nic_sent_bytes_count[idx], idx,
                     expected_mapping[1].striped_nic_sent_bytes_count[idx]);
                return -1;
            }
            if (expected_mapping[1].striped_nic_recvd_bytes_count[idx] !=
                striped_nic_recvd_bytes_count[idx]) {
                printf
                    ("rank=%d --> target=%d: Actual striped received byte count=%llu through NIC %d does not match with the "
                     "expected striped received byte count=%llu\n", rank, target,
                     striped_nic_recvd_bytes_count[idx], idx,
                     expected_mapping[1].striped_nic_recvd_bytes_count[idx]);
                return -1;
            }
        }
    }
    return errs;
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
    int nsc_continuous, nrc_continuous, snsc_continuous, snrc_continuous;
    int nsc_idx = -1, nrc_idx = -1, snsc_idx = -1, snrc_idx = -1;
    int num_nics = 1;

    int required, provided;
    int errs = 0;
    int rank, i;
    float *in_buf, *out_buf;
    MPI_Comm comm;
    MPI_Info comm_info;
    MPI_Info comm_info_out;
    char query_key[MPI_MAX_INFO_KEY];
    char buf[MPI_MAX_INFO_VAL];
    char val[MPI_MAX_INFO_VAL];
    int flag;
    int target;

    MTest_Init(&argc, &argv);

    comm = MPI_COMM_WORLD;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &num_procs);

    if (argc != 3) {
        if (rank == 0) {
            fprintf(stderr, "Expected 2 arguments.\n");
            fprintf(stderr, "    Usage: ./mpit_isendirecv <enable_striping> <enable_hashing>\n");
        }
        exit(1);
    } else {
        enable_striping = atoi(argv[1]);
        enable_hashing = atoi(argv[2]);
    }

    /* compute total number of nodes */
    char nodenames[num_procs][MPI_MAX_PROCESSOR_NAME];
    int len;
    MPI_Get_processor_name(nodenames[rank], &len);
    MPI_Allgather(MPI_IN_PLACE, 0, 0, &nodenames, MPI_MAX_PROCESSOR_NAME, MPI_CHAR, MPI_COMM_WORLD);

    /* remove duplicate node names */
    num_nodes = remove_duplicates(nodenames, num_procs);

    /* number of processes per node */
    num_procs_per_node = num_procs / num_nodes;

    /* number of pairs */
    num_pairs = num_procs / 2;

    MPI_Info_create(&comm_info);
    MPI_Info_set(comm_info, "mpi_assert_no_any_source", "true");
    MPI_Info_set(comm_info, "mpi_assert_no_any_tag", "true");
    MPI_Info_set(comm_info, "enable_multi_nic_striping", enable_striping ? "true" : "false");
    MPI_Info_set(comm_info, "enable_multi_nic_hashing", enable_hashing ? "true" : "false");
    MPI_Comm_set_info(comm, comm_info);

    /* determine the number of nics in use */
    memset(buf, 0, MPI_MAX_INFO_VAL * sizeof(char));
    MPI_Info_get(MPI_INFO_ENV, "num_nics", MPI_MAX_INFO_VAL, buf, &flag);
    if (!flag) {
        num_nics = 1;
    } else {
        num_nics = atoi(buf);
    }

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
        if (0 == strcmp(name, "striped_nic_sent_bytes_count")) {
            snsc_idx = i;
            snsc_continuous = continuous;
        }
        if (0 == strcmp(name, "striped_nic_recvd_bytes_count")) {
            snrc_idx = i;
            snrc_continuous = continuous;
        }
    }

    /* execute only when PVARs are enabled */
    if (nsc_idx != -1 && nrc_idx != -1 && snsc_idx != -1 && snrc_idx != -1) {
        /* setup a session and handles for the PVAR variables */
        session = MPI_T_PVAR_SESSION_NULL;
        MPI_T_pvar_session_create(&session);
        assert(session != MPI_T_PVAR_SESSION_NULL);

        nsc_handle = MPI_T_PVAR_HANDLE_NULL;
        MPI_T_pvar_handle_alloc(session, nsc_idx, NULL, &nsc_handle, &count);
        assert(count = 1);
        assert(nsc_handle != MPI_T_PVAR_HANDLE_NULL);

        nrc_handle = MPI_T_PVAR_HANDLE_NULL;
        MPI_T_pvar_handle_alloc(session, nrc_idx, NULL, &nrc_handle, &count);
        assert(count = 1);
        assert(nrc_handle != MPI_T_PVAR_HANDLE_NULL);

        snsc_handle = MPI_T_PVAR_HANDLE_NULL;
        MPI_T_pvar_handle_alloc(session, snsc_idx, NULL, &snsc_handle, &count);
        assert(count = 1);
        assert(snsc_handle != MPI_T_PVAR_HANDLE_NULL);

        snrc_handle = MPI_T_PVAR_HANDLE_NULL;
        MPI_T_pvar_handle_alloc(session, snrc_idx, NULL, &snrc_handle, &count);
        assert(count = 1);
        assert(snrc_handle != MPI_T_PVAR_HANDLE_NULL);

        if (!nsc_continuous) {
            MPI_T_pvar_start(session, nsc_handle);
        }
        if (!nrc_continuous) {
            MPI_T_pvar_start(session, nrc_handle);
        }
        if (!snsc_continuous) {
            MPI_T_pvar_start(session, snsc_handle);
        }
        if (!snrc_continuous) {
            MPI_T_pvar_start(session, snrc_handle);
        }
    }

    reqs = (MPI_Request *) malloc(num_nics * sizeof(MPI_Request));
    in_buf = (float *) malloc(elems * num_nics * sizeof(float));
    out_buf = (float *) malloc(elems * num_nics * sizeof(float));
    MTEST_VG_MEM_INIT(out_buf, elems * num_nics * sizeof(float));

    if (rank < num_pairs) {
        target = rank + num_pairs;
        for (i = 0; i < num_nics; ++i) {
            MPI_Isend(&out_buf[elems * i], elems, MPI_FLOAT, target, i, comm, &reqs[i]);
        }

        MPI_Waitall(num_nics, reqs, MPI_STATUSES_IGNORE);
    } else if (rank < num_pairs * 2) {
        target = rank - num_pairs;
        for (i = 0; i < num_nics; ++i) {
            MPI_Irecv(&in_buf[elems * i], elems, MPI_FLOAT, target, i, comm, &reqs[i]);
        }
        MPI_Waitall(num_nics, reqs, MPI_STATUSES_IGNORE);
    }

    free(reqs);
    free(in_buf);
    free(out_buf);

    /* execute only when PVARs are enabled */
    if (nsc_idx != -1 && nrc_idx != -1 && snsc_idx != -1 && snrc_idx != -1) {
        if (!nrc_continuous) {
            MPI_T_pvar_stop(session, nrc_handle);
        }
        if (!nsc_continuous) {
            MPI_T_pvar_stop(session, nsc_handle);
        }
        if (!snrc_continuous) {
            MPI_T_pvar_stop(session, snrc_handle);
        }
        if (!snsc_continuous) {
            MPI_T_pvar_stop(session, snsc_handle);
        }

        /* compare actual the output with the expected output */
        errs += compare_vars(rank, target, num_nics);

        /* cleanup */
        MPI_T_pvar_handle_free(session, &nrc_handle);
        MPI_T_pvar_handle_free(session, &nsc_handle);
        MPI_T_pvar_handle_free(session, &snrc_handle);
        MPI_T_pvar_handle_free(session, &snsc_handle);
        MPI_T_pvar_session_free(&session);
    }
    MPI_T_finalize();
    MPI_Info_free(&comm_info);
    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}

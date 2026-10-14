/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "sock_impl.h"
#include <sys/resource.h>
#include <fcntl.h>
#include <unistd.h>

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

categories :
    - name : CH4_SOCK
      description : A category for CH4 SOCK netmod variables

cvars:
    - name        : MPIR_CVAR_CH4_SOCK_EAGER_SIZE
      category    : CH4_SOCK
      type        : int
      default     : 16384
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        The max size in bytes of the active message header plus data that the
        sock netmod sends as an eager message. Larger messages use the
        rendezvous protocol. It is capped by MPIR_CVAR_CH4_PACK_BUFFER_SIZE.

    - name        : MPIR_CVAR_CH4_SOCK_IFACE
      category    : CH4_SOCK
      type        : string
      default     : NULL
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        If non-NULL, the name of the network interface (e.g. "eth1") whose
        address other processes use to connect to this process. By default
        it is the first interface that is not the loopback.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

static void raise_fd_limit(rlim_t * p_fd_limit);
static int init_listener(void);
static void av_insert(MPIDI_av_entry_t * av, MPIR_Lpid lpid, const MPIDI_SOCK_name_t * name);

/* Each connection uses a file descriptor and the soft limit is often low
 * (e.g. 1024), so raise it to the hard limit. It is not an error if we can't. */
static void raise_fd_limit(rlim_t * p_fd_limit)
{
    struct rlimit rlim;

    *p_fd_limit = 0;
    if (getrlimit(RLIMIT_NOFILE, &rlim) != 0) {
        return;
    }
    if (rlim.rlim_cur < rlim.rlim_max) {
        rlim_t orig = rlim.rlim_cur;
        rlim.rlim_cur = rlim.rlim_max;
        if (setrlimit(RLIMIT_NOFILE, &rlim) != 0) {
            rlim.rlim_cur = orig;
        }
    }
    *p_fd_limit = rlim.rlim_cur;
}

/* Open the listening socket and set MPIDI_SOCK_global.name */
static int init_listener(void)
{
    int mpi_errno = MPI_SUCCESS;
    char strerrbuf[MPIR_STRERROR_BUF_SIZE] ATTRIBUTE((unused));

    int fd = MPL_socket();
    MPIR_ERR_CHKANDJUMP2(fd < 0, mpi_errno, MPI_ERR_OTHER, "**sock_create", "**sock_create %s %d",
                         MPIR_Strerror(errno, strerrbuf, MPIR_STRERROR_BUF_SIZE), errno);
    MPIDI_SOCK_global.listen_fd = fd;

    int low = MPIR_CVAR_PORT_RANGE.low;
    int high = MPIR_CVAR_PORT_RANGE.high;
    MPIR_ERR_CHKANDJUMP(low < 0 || low > high || high > 65535, mpi_errno, MPI_ERR_OTHER,
                        "**badportrange");

    unsigned short port;
    int ret;
    if (low == 0) {
        ret = MPL_listen_anyport(fd, &port);
    } else {
        ret = MPL_listen_portrange(fd, &port, low, high);
    }
    MPIR_ERR_CHKANDJUMP4(ret != 0, mpi_errno, MPI_ERR_OTHER, "**ch4_sock_listen",
                         "**ch4_sock_listen %d %d %s %d", low, high,
                         MPIR_Strerror(errno, strerrbuf, MPIR_STRERROR_BUF_SIZE), errno);

    int flags = fcntl(fd, F_GETFL, 0);
    ret = (flags == -1) ? -1 : fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    MPIR_ERR_CHKANDJUMP2(ret == -1, mpi_errno, MPI_ERR_OTHER, "**ch4_sock_nonblock",
                         "**ch4_sock_nonblock %s %d",
                         MPIR_Strerror(errno, strerrbuf, MPIR_STRERROR_BUF_SIZE), errno);

    /* The name is compared as bytes. MPL_get_sockaddr_iface zeros saddr. */
    memset(&MPIDI_SOCK_global.name, 0, sizeof(MPIDI_SOCK_name_t));
    ret = MPL_get_sockaddr_iface(MPIR_CVAR_CH4_SOCK_IFACE, &MPIDI_SOCK_global.name.saddr);
    if (ret != 0 && MPIR_CVAR_CH4_SOCK_IFACE) {
        MPIR_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**ch4_sock_iface", "**ch4_sock_iface %s",
                             MPIR_CVAR_CH4_SOCK_IFACE);
    }
    MPIR_ERR_CHKANDJUMP(ret != 0, mpi_errno, MPI_ERR_OTHER, "**ch4_sock_iface");
    MPIDI_SOCK_global.name.port = port;

  fn_exit:
    return mpi_errno;
  fn_fail:
    if (MPIDI_SOCK_global.listen_fd >= 0) {
        close(MPIDI_SOCK_global.listen_fd);
        MPIDI_SOCK_global.listen_fd = -1;
    }
    goto fn_exit;
}

/* Record the address of a process. No connection is made until the first send. */
static void av_insert(MPIDI_av_entry_t * av, MPIR_Lpid lpid, const MPIDI_SOCK_name_t * name)
{
    if (MPIDI_SOCK_AV(av).name.port == 0) {
        MPIDI_SOCK_AV(av).name = *name;
        MPIDI_SOCK_AV(av).conn = NULL;
        MPIDIU_upidhash_add(name, sizeof(MPIDI_SOCK_name_t), lpid);
    }
}

int MPIDI_SOCK_init_local(int *tag_bits)
{
    int mpi_errno = MPI_SUCCESS;

    MPIDI_SOCK_global.listen_fd = -1;
    MPIDI_SOCK_global.eager_size = MPL_MIN(MPIR_CVAR_CH4_SOCK_EAGER_SIZE,
                                           MPIR_CVAR_CH4_PACK_BUFFER_SIZE);

    rlim_t fd_limit;
    raise_fd_limit(&fd_limit);

    mpi_errno = init_listener();
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIDI_SOCK_conn_init();
    MPIR_ERR_CHECK(mpi_errno);

    /* insert self address */
    MPIR_Lpid lpid = MPIR_Process.rank;
    av_insert(MPIDIU_lpid_to_av(lpid), lpid, &MPIDI_SOCK_global.name);

    /* tags are matched in MPIDIG */
    *tag_bits = MPIR_TAG_BITS_DEFAULT;

    if (MPIR_CVAR_DEBUG_SUMMARY && MPIR_Process.rank == 0) {
        char addr_str[100];
        MPL_sockaddr_to_str(&MPIDI_SOCK_global.name.saddr, addr_str, sizeof(addr_str));
        printf("==== SOCK netmod Capability ====\n");
        printf("listening address: %s port %d\n", addr_str, MPIDI_SOCK_global.name.port);
        printf("fd limit: %lu\n", (unsigned long) fd_limit);
        printf("eager size: %ld\n", (long) MPIDI_SOCK_global.eager_size);
        printf("tag_bits: %d\n", *tag_bits);
        printf("================================\n");
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* Exchange the addresses of all processes in a bootstrap comm. This follows
 * the same steps as MPIDI_UCX_comm_addr_exchange. */
int MPIDI_SOCK_comm_addr_exchange(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_CHKLMEM_DECL();

    MPIR_Assert(comm->attr & MPIR_COMM_ATTR__HIERARCHY);

    char *addrname = (void *) &MPIDI_SOCK_global.name;
    int addrnamelen = sizeof(MPIDI_SOCK_name_t);

    int local_rank = comm->local_rank;
    int external_size = comm->num_external;

    if (external_size == 1) {
        /* skip root addrexch if we are the only node */
        goto all_addrexch;
    }

    /* PMI allgather over node roots and av_insert to activate node_roots_comm */
    char *roots_names = NULL;
    if (local_rank == 0) {
        MPIR_CHKLMEM_MALLOC(roots_names, external_size * addrnamelen);
    }

    mpi_errno = MPIDIU_bc_exchange_node_roots(comm, addrname, addrnamelen, roots_names);
    MPIR_ERR_CHECK(mpi_errno);

    if (local_rank == 0) {
        /* insert av and activate node_roots_comm */
        MPIR_Comm *node_roots_comm = MPIR_Comm_get_node_roots_comm(comm);
        for (int i = 0; i < external_size; i++) {
            MPIDI_SOCK_name_t name;
            memcpy(&name, roots_names + i * addrnamelen, addrnamelen);
            MPIDI_av_entry_t *av = MPIDIU_comm_rank_to_av(node_roots_comm, i);
            MPIR_Lpid lpid = MPIR_comm_rank_to_lpid(node_roots_comm, i);
            av_insert(av, lpid, &name);
        }
    }

  all_addrexch:
    if (external_size == comm->local_size) {
        /* if no local, we are done. */
        goto fn_exit;
    }

    /* -- rest of the addr exchange over node_comm and node_roots_comm -- */
    /* since the orders will be rearranged by nodes, let's exchange rank along with name */
    struct rankname {
        int rank;
        MPIDI_SOCK_name_t name;
    };
    int rankname_len = sizeof(struct rankname);

    struct rankname *my_rankname, *all_ranknames;
    MPIR_CHKLMEM_MALLOC(my_rankname, rankname_len);
    MPIR_CHKLMEM_MALLOC(all_ranknames, comm->local_size * rankname_len);

    memset(my_rankname, 0, rankname_len);
    my_rankname->rank = comm->rank;
    my_rankname->name = MPIDI_SOCK_global.name;

    /* Use an smp algorithm explicitly that only require a working node_comm and node_roots_comm. */
    mpi_errno = MPIR_Allgather_intra_smp_no_order(my_rankname, rankname_len, MPIR_BYTE_INTERNAL,
                                                  all_ranknames, rankname_len, MPIR_BYTE_INTERNAL,
                                                  comm, MPIR_COLL_ATTR_SYNC);
    MPIR_ERR_CHECK(mpi_errno);

    /* insert av, skipping existing entries */
    for (int i = 0; i < comm->local_size; i++) {
        struct rankname *p = &all_ranknames[i];
        MPIDI_av_entry_t *av = MPIDIU_comm_rank_to_av(comm, p->rank);
        MPIR_Lpid lpid = MPIR_comm_rank_to_lpid(comm, p->rank);
        av_insert(av, lpid, &p->name);
    }

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_SOCK_mpi_finalize_hook(void)
{
    int mpi_errno = MPI_SUCCESS;

    /* write out everything that is still queued */
    mpi_errno = MPIDI_SOCK_flush();
    MPIR_ERR_CHECK(mpi_errno);

    if (!MPIR_CVAR_NO_COLLECTIVE_FINALIZE) {
        /* make sure no process closes its sockets while others may still use them */
        mpi_errno = MPIR_pmi_barrier();
        MPIR_ERR_CHECK(mpi_errno);
    }

    mpi_errno = MPIDI_SOCK_conn_finalize();
    MPIR_ERR_CHECK(mpi_errno);

    MPIDI_SOCK_dynamic_finalize();

    if (MPIDI_SOCK_global.listen_fd >= 0) {
        close(MPIDI_SOCK_global.listen_fd);
        MPIDI_SOCK_global.listen_fd = -1;
    }

    MPIDIU_upidhash_free();

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_SOCK_mpi_free_mem(void *ptr)
{
    return MPIDIG_mpi_free_mem(ptr);
}

void *MPIDI_SOCK_mpi_alloc_mem(MPI_Aint size, MPIR_Info * info_ptr)
{
    return MPIDIG_mpi_alloc_mem(size, info_ptr);
}

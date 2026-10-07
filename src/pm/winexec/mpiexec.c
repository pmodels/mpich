/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* A minimal single-node process manager for native Windows.
 *
 * Starts N copies of a program with CreateProcess and serves the PMI-1 wire
 * protocol over a loopback TCP port (PMI_PORT/PMI_ID, the same handshake
 * gforker uses with MPIEXEC_USE_PORT).  All ranks are placed in a job
 * object so that they are killed if mpiexec exits or is killed.
 *
 *   mpiexec [-n N] [-env NAME VALUE]... [-v] program [args...]
 */

#include "mpl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE   4096
#define MAX_KEY    64
#define MAX_VAL    1024
#define KVSNAME_MAX 256

typedef struct {
    int fd;
    int rank;                   /* -1 until initack */
    char buf[MAX_LINE];
    int buflen;
    int in_mcmd;                /* inside a multi-line mcmd=... command */
} conn_t;

typedef struct kvpair {
    char *key;
    char *val;
    struct kvpair *next;
} kvpair_t;

static int nprocs = 1;
static int verbose = 0;
static char kvsname[KVSNAME_MAX];
static kvpair_t *kvs = NULL;
static conn_t *conns = NULL;
static int nconns = 0;
static int n_in_barrier = 0;
static HANDLE job = NULL;
static HANDLE *procs = NULL;
static DWORD *exit_codes = NULL;
/* per rank: connected to the PMI server, and called PMI finalize */
static char *rank_initialized = NULL;
static char *rank_finalized = NULL;
/* serializes writes to our stdout/stderr */
static CRITICAL_SECTION out_lock;

static void usage(void)
{
    fprintf(stderr, "Usage: mpiexec [-n N] [-env NAME VALUE]... [-v] program [args...]\n");
    exit(1);
}

/* growable string */
typedef struct {
    char *s;
    size_t len, cap;
} strbuf_t;

static void sb_putc(strbuf_t * b, char ch)
{
    if (b->len + 2 > b->cap) {
        b->cap = b->cap ? 2 * b->cap : 256;
        b->s = realloc(b->s, b->cap);
        if (!b->s) {
            fprintf(stderr, "[mpiexec] out of memory\n");
            exit(1);
        }
    }
    b->s[b->len++] = ch;
    b->s[b->len] = '\0';
}

/* Append one argument, quoted following the rules of CommandLineToArgvW */
static void append_quoted(strbuf_t * cmd, const char *arg)
{
    if (cmd->len)
        sb_putc(cmd, ' ');
    if (*arg && !strpbrk(arg, " \t\n\v\"")) {
        for (const char *p = arg; *p; p++)
            sb_putc(cmd, *p);
        return;
    }
    sb_putc(cmd, '"');
    for (const char *p = arg;; p++) {
        int nbs = 0;
        while (*p == '\\') {
            p++;
            nbs++;
        }
        /* backslashes are literal unless they precede a quote */
        int n = (*p == '\0') ? 2 * nbs : (*p == '"') ? 2 * nbs + 1 : nbs;
        for (int i = 0; i < n; i++)
            sb_putc(cmd, '\\');
        if (*p == '\0')
            break;
        sb_putc(cmd, *p);
    }
    sb_putc(cmd, '"');
}

static void conn_write(conn_t * c, const char *line)
{
    size_t len = strlen(line), off = 0;
    if (verbose)
        fprintf(stderr, "[mpiexec] -> %d: %s", c->rank, line);
    /* client sockets are blocking; replies are small */
    while (off < len) {
        ssize_t n = MPL_sock_write(c->fd, line + off, len - off);
        if (n <= 0) {
            if (n < 0 && errno == EINTR)
                continue;
            return;
        }
        off += n;
    }
}

/* Extract the value of key from a PMI-1 command line.  Values may contain
 * backslash-escaped spaces, which are passed through unchanged. */
static int getval(const char *line, const char *key, char *val, size_t vallen)
{
    size_t klen = strlen(key);
    const char *p = line;
    while ((p = strstr(p, key)) != NULL) {
        if ((p == line || p[-1] == ' ') && p[klen] == '=') {
            p += klen + 1;
            size_t n = 0;
            while (*p && *p != '\n' && !(*p == ' ' && p[-1] != '\\') && n < vallen - 1)
                val[n++] = *p++;
            val[n] = '\0';
            return 1;
        }
        p += klen;
    }
    val[0] = '\0';
    return 0;
}

static const char *kvs_get(const char *key)
{
    for (kvpair_t * kv = kvs; kv; kv = kv->next)
        if (strcmp(kv->key, key) == 0)
            return kv->val;
    return NULL;
}

static void kvs_put(const char *key, const char *val)
{
    for (kvpair_t * kv = kvs; kv; kv = kv->next) {
        if (strcmp(kv->key, key) == 0) {
            free(kv->val);
            kv->val = strdup(val);
            return;
        }
    }
    kvpair_t *kv = malloc(sizeof(*kv));
    kv->key = strdup(key);
    kv->val = strdup(val);
    kv->next = kvs;
    kvs = kv;
}

static void kill_all(int code)
{
    if (job)
        TerminateJobObject(job, code);
}

static void handle_cmd(conn_t * c, char *line)
{
    char cmd[64], out[MAX_LINE], key[256], val[MAX_VAL + 1];

    if (verbose)
        fprintf(stderr, "[mpiexec] <- %d: %s\n", c->rank, line);

    /* multi-line commands (mcmd=spawn ... endcmd) are only used by spawn,
     * which is not supported; consume them and reply with an error */
    if (strncmp(line, "mcmd=", 5) == 0) {
        c->in_mcmd = 1;
        return;
    }
    if (c->in_mcmd) {
        if (strcmp(line, "endcmd") == 0) {
            c->in_mcmd = 0;
            conn_write(c, "cmd=spawn_result rc=-1 msg=spawn_not_supported_by_this_mpiexec\n");
        }
        return;
    }

    getval(line, "cmd", cmd, sizeof(cmd));

    if (strcmp(cmd, "initack") == 0) {
        getval(line, "pmiid", val, sizeof(val));
        c->rank = atoi(val);
        if (c->rank >= 0 && c->rank < nprocs) {
            /* a rank may initialize again after finalizing (sessions) */
            rank_initialized[c->rank] = 1;
            rank_finalized[c->rank] = 0;
        }
        /* -env sets the Win32 environment, which the CRT getenv() does not see */
        char dbg[16] = "0";
        GetEnvironmentVariableA("PMI_DEBUG", dbg, sizeof(dbg));
        /* same sequence as pm/util/pmiserv.c */
        conn_write(c, "cmd=initack\n");
        snprintf(out, sizeof(out), "cmd=set size=%d\n", nprocs);
        conn_write(c, out);
        snprintf(out, sizeof(out), "cmd=set rank=%d\n", c->rank);
        conn_write(c, out);
        snprintf(out, sizeof(out), "cmd=set debug=%d\n", atoi(dbg));
        conn_write(c, out);
    } else if (strcmp(cmd, "init") == 0) {
        conn_write(c, "cmd=response_to_init pmi_version=1 pmi_subversion=1 rc=0\n");
    } else if (strcmp(cmd, "get_maxes") == 0) {
        snprintf(out, sizeof(out), "cmd=maxes kvsname_max=%d keylen_max=%d vallen_max=%d\n",
                 KVSNAME_MAX, MAX_KEY, MAX_VAL);
        conn_write(c, out);
    } else if (strcmp(cmd, "get_appnum") == 0) {
        conn_write(c, "cmd=appnum appnum=0\n");
    } else if (strcmp(cmd, "get_universe_size") == 0) {
        snprintf(out, sizeof(out), "cmd=universe_size size=%d\n", nprocs);
        conn_write(c, out);
    } else if (strcmp(cmd, "get_my_kvsname") == 0) {
        snprintf(out, sizeof(out), "cmd=my_kvsname kvsname=%s\n", kvsname);
        conn_write(c, out);
    } else if (strcmp(cmd, "put") == 0) {
        getval(line, "key", key, sizeof(key));
        getval(line, "value", val, sizeof(val));
        kvs_put(key, val);
        conn_write(c, "cmd=put_result rc=0 msg=success\n");
    } else if (strcmp(cmd, "get") == 0) {
        getval(line, "key", key, sizeof(key));
        const char *v = kvs_get(key);
        if (v)
            snprintf(out, sizeof(out), "cmd=get_result rc=0 msg=success value=%s\n", v);
        else
            snprintf(out, sizeof(out), "cmd=get_result rc=-1 msg=key_%s_not_found value=unknown\n",
                     key);
        conn_write(c, out);
    } else if (strcmp(cmd, "barrier_in") == 0) {
        if (++n_in_barrier == nprocs) {
            n_in_barrier = 0;
            for (int i = 0; i < nconns; i++)
                if (conns[i].fd >= 0)
                    conn_write(&conns[i], "cmd=barrier_out\n");
        }
    } else if (strcmp(cmd, "finalize") == 0) {
        if (c->rank >= 0 && c->rank < nprocs)
            rank_finalized[c->rank] = 1;
        conn_write(c, "cmd=finalize_ack\n");
    } else if (strcmp(cmd, "abort") == 0) {
        getval(line, "exitcode", val, sizeof(val));
        int code = val[0] ? atoi(val) : 1;
        fprintf(stderr, "[mpiexec] rank %d aborted with exit code %d\n", c->rank, code);
        kill_all(code);
        exit(code);
    } else {
        /* Reply with an error so that the client does not wait forever.
         * The PMI-1 responses are named <cmd>_result, except for the
         * name service commands. */
        const char *resp = NULL;
        if (strcmp(cmd, "publish_name") == 0)
            resp = "publish_result";
        else if (strcmp(cmd, "unpublish_name") == 0)
            resp = "unpublish_result";
        else if (strcmp(cmd, "lookup_name") == 0)
            resp = "lookup_result";
        if (resp)
            snprintf(out, sizeof(out), "cmd=%s rc=-1 msg=not_supported_by_this_mpiexec\n", resp);
        else
            snprintf(out, sizeof(out), "cmd=%.40s_result rc=-1 msg=not_supported_by_this_mpiexec\n",
                     cmd);
        EnterCriticalSection(&out_lock);
        fprintf(stderr, "[mpiexec] unsupported PMI command from rank %d: %s\n", c->rank, line);
        fflush(stderr);
        LeaveCriticalSection(&out_lock);
        conn_write(c, out);
    }
}

/* Read what is available and dispatch complete lines; returns 0 on EOF/error */
static int conn_read(conn_t * c)
{
    ssize_t n = MPL_sock_read(c->fd, c->buf + c->buflen, sizeof(c->buf) - 1 - c->buflen);
    if (n <= 0)
        return 0;
    c->buflen += n;
    c->buf[c->buflen] = '\0';
    char *start = c->buf, *nl;
    while ((nl = strchr(start, '\n')) != NULL) {
        *nl = '\0';
        if (nl > start)
            handle_cmd(c, start);
        start = nl + 1;
    }
    c->buflen -= (start - c->buf);
    memmove(c->buf, start, c->buflen);
    if (c->buflen == sizeof(c->buf) - 1) {
        fprintf(stderr, "[mpiexec] PMI line too long from rank %d\n", c->rank);
        return 0;
    }
    return 1;
}

/* In a Python environment the MPI DLLs live in <prefix>\Library\bin while
 * mpiexec is in <prefix>\Scripts; make the DLLs visible to the ranks. */
static void add_dll_dir_to_path(void)
{
    char exe[MAX_PATH], dir[MAX_PATH + 32], probe[MAX_PATH + 64];
    DWORD n = GetModuleFileNameA(NULL, exe, sizeof(exe));
    if (n == 0 || n == sizeof(exe))
        return;
    char *slash = strrchr(exe, '\\');
    if (!slash)
        return;
    *slash = '\0';
    snprintf(dir, sizeof(dir), "%s\\..\\Library\\bin", exe);
    snprintf(probe, sizeof(probe), "%s\\libmpi-0.dll", dir);
    if (GetFileAttributesA(probe) == INVALID_FILE_ATTRIBUTES)
        return;

    DWORD len = GetEnvironmentVariableA("PATH", NULL, 0);
    char *path = malloc(strlen(dir) + 1 + len + 1);
    strcpy(path, dir);
    if (len) {
        strcat(path, ";");
        GetEnvironmentVariableA("PATH", path + strlen(path), len);
    }
    SetEnvironmentVariableA("PATH", path);
    free(path);
}

/* Output forwarding.  Like hydra, each rank writes to its own pipes and
 * mpiexec forwards complete lines, so that output from different ranks
 * is not interleaved within a line. */
typedef struct {
    HANDLE pipe;                /* read end of a rank's stdout or stderr */
    HANDLE out;                 /* our stdout or stderr */
} fwd_t;

static HANDLE *fwd_threads;
static int nfwd = 0;

static void write_all(HANDLE h, const char *buf, DWORD len)
{
    DWORD n;
    while (len > 0 && WriteFile(h, buf, len, &n, NULL) && n > 0) {
        buf += n;
        len -= n;
    }
}

static DWORD WINAPI forward_output(LPVOID arg)
{
    fwd_t *f = arg;
    size_t cap = 64 * 1024, len = 0;
    char *buf = malloc(cap);
    DWORD n;

    while (ReadFile(f->pipe, buf + len, (DWORD) (cap - len), &n, NULL) && n > 0) {
        len += n;
        /* forward everything up to the last newline; a line that does not
         * fit into the buffer is forwarded as is */
        size_t end = len;
        while (end > 0 && buf[end - 1] != '\n')
            end--;
        if (end == 0 && len == cap)
            end = len;
        if (end > 0) {
            EnterCriticalSection(&out_lock);
            write_all(f->out, buf, (DWORD) end);
            LeaveCriticalSection(&out_lock);
            memmove(buf, buf + end, len - end);
            len -= end;
        }
    }
    if (len > 0) {
        EnterCriticalSection(&out_lock);
        write_all(f->out, buf, (DWORD) len);
        LeaveCriticalSection(&out_lock);
    }
    CloseHandle(f->pipe);
    free(buf);
    free(f);
    return 0;
}

/* Create a pipe whose write end the rank inherits; returns the write end */
static HANDLE forwarded_pipe(HANDLE out)
{
    SECURITY_ATTRIBUTES sa = { sizeof(sa), NULL, TRUE };
    HANDLE rd, wr;
    if (!CreatePipe(&rd, &wr, &sa, 0))
        return INVALID_HANDLE_VALUE;
    SetHandleInformation(rd, HANDLE_FLAG_INHERIT, 0);
    fwd_t *f = malloc(sizeof(*f));
    f->pipe = rd;
    f->out = out;
    fwd_threads[nfwd++] = CreateThread(NULL, 0, forward_output, f, 0, NULL);
    return wr;
}

static int start_ranks(int argc, char **argv, int port)
{
    strbuf_t cmdline = { NULL, 0, 0 };
    char buf[64];
    STARTUPINFOA si;
    SECURITY_ATTRIBUTES sa = { sizeof(sa), NULL, TRUE };
    HANDLE in = GetStdHandle(STD_INPUT_HANDLE);
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    HANDLE err = GetStdHandle(STD_ERROR_HANDLE);
    /* as with hydra, only rank 0 reads our stdin */
    HANDLE nul = CreateFileA("NUL", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, &sa,
                             OPEN_EXISTING, 0, NULL);

    for (int i = 0; i < argc; i++)
        append_quoted(&cmdline, argv[i]);
    if (cmdline.len >= 32767) {
        /* the limit of CreateProcess */
        fprintf(stderr, "[mpiexec] command line too long\n");
        return -1;
    }
    add_dll_dir_to_path();

    fwd_threads = calloc(2 * nprocs, sizeof(HANDLE));
    SetHandleInformation(in, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);

    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;

    snprintf(buf, sizeof(buf), "127.0.0.1:%d", port);
    SetEnvironmentVariableA("PMI_PORT", buf);
    /* all ranks are local: communicate over loopback only (ch3:sock then
     * listens on 127.0.0.1, which also avoids Windows firewall prompts) */
    if (GetEnvironmentVariableA("MPICH_INTERFACE_HOSTNAME", NULL, 0) == 0)
        SetEnvironmentVariableA("MPICH_INTERFACE_HOSTNAME", "127.0.0.1");
    snprintf(buf, sizeof(buf), "%d", nprocs);
    SetEnvironmentVariableA("PMI_SIZE", buf);

    for (int r = 0; r < nprocs; r++) {
        PROCESS_INFORMATION pi;
        char *cmd = strdup(cmdline.s);  /* CreateProcess may modify it */

        snprintf(buf, sizeof(buf), "%d", r);
        SetEnvironmentVariableA("PMI_ID", buf);
        SetEnvironmentVariableA("PMI_RANK", buf);
        /* Ranks are started one at a time and our copies of the pipe write
         * ends are closed right after, so each rank only inherits its own. */
        si.hStdInput = (r == 0) ? in : nul;
        si.hStdOutput = forwarded_pipe(out);
        si.hStdError = forwarded_pipe(err);
        BOOL ok = CreateProcessA(NULL, cmd, NULL, NULL, TRUE, CREATE_SUSPENDED, NULL, NULL,
                                 &si, &pi);
        CloseHandle(si.hStdOutput);
        CloseHandle(si.hStdError);
        free(cmd);
        if (!ok) {
            fprintf(stderr, "[mpiexec] unable to start '%s' (error %lu)\n", argv[0],
                    (unsigned long) GetLastError());
            return -1;
        }
        AssignProcessToJobObject(job, pi.hProcess);
        ResumeThread(pi.hThread);
        CloseHandle(pi.hThread);
        procs[r] = pi.hProcess;
    }
    return 0;
}

int main(int argc, char **argv)
{
    int i, listen_fd;
    unsigned short port;

    InitializeCriticalSection(&out_lock);

    for (i = 1; i < argc && argv[i][0] == '-'; i++) {
        if ((!strcmp(argv[i], "-n") || !strcmp(argv[i], "-np")) && i + 1 < argc) {
            nprocs = atoi(argv[++i]);
        } else if ((!strcmp(argv[i], "-env") || !strcmp(argv[i], "-genv")) && i + 2 < argc) {
            SetEnvironmentVariableA(argv[i + 1], argv[i + 2]);
            i += 2;
        } else if (!strcmp(argv[i], "-v")) {
            verbose = 1;
        } else {
            usage();
        }
    }
    if (i >= argc || nprocs < 1)
        usage();

    if (MPL_sock_init()) {
        fprintf(stderr, "[mpiexec] WSAStartup failed\n");
        return 1;
    }
    snprintf(kvsname, sizeof(kvsname), "kvs_%lu_0", (unsigned long) GetCurrentProcessId());
    {
        char mapping[64];
        snprintf(mapping, sizeof(mapping), "(vector,(0,1,%d))", nprocs);
        kvs_put("PMI_process_mapping", mapping);
    }

    /* PMI listener on loopback, not inherited by the ranks */
    listen_fd = MPL_socket();
    if (listen_fd < 0) {
        fprintf(stderr, "[mpiexec] socket failed: %s\n", strerror(errno));
        return 1;
    }
    MPL_sock_set_cloexec(listen_fd);
    MPL_LISTEN_PUSH(1, SOMAXCONN);
    if (MPL_listen_anyport(listen_fd, &port)) {
        fprintf(stderr, "[mpiexec] listen failed: %s\n", strerror(errno));
        return 1;
    }
    MPL_LISTEN_POP;

    /* kill all ranks if mpiexec goes away */
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION jeli;
    job = CreateJobObjectA(NULL, NULL);
    memset(&jeli, 0, sizeof(jeli));
    jeli.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    SetInformationJobObject(job, JobObjectExtendedLimitInformation, &jeli, sizeof(jeli));

    procs = calloc(nprocs, sizeof(HANDLE));
    exit_codes = calloc(nprocs, sizeof(DWORD));
    rank_initialized = calloc(nprocs, 1);
    rank_finalized = calloc(nprocs, 1);
    /* A rank that re-initializes (MPI sessions) reconnects, possibly before
     * its previous connection has been seen to close, so grow as needed. */
    int conns_cap = nprocs;
    conns = calloc(conns_cap, sizeof(conn_t));
    if (start_ranks(argc - i, argv + i, port)) {
        kill_all(1);
        return 1;
    }

    /* MPIEXEC_TIMEOUT (seconds), as honored by hydra and gforker */
    char tmo[32] = "";
    GetEnvironmentVariableA("MPIEXEC_TIMEOUT", tmo, sizeof(tmo));
    ULONGLONG deadline = atoi(tmo) > 0 ? GetTickCount64() + 1000ULL * atoi(tmo) : 0;

    struct pollfd *pfds = calloc(conns_cap + 1, sizeof(struct pollfd));
    int nalive = nprocs, rc = 0;
    while (nalive > 0) {
        if (deadline && GetTickCount64() > deadline) {
            fprintf(stderr, "[mpiexec] Timeout of %d seconds expired; job aborted\n", atoi(tmo));
            kill_all(1);
            return 1;
        }
        int np = 0;
        pfds[np].fd = listen_fd;
        pfds[np++].events = POLLIN;
        for (int c = 0; c < nconns; c++) {
            pfds[np].fd = conns[c].fd;
            pfds[np++].events = POLLIN;
        }
        int n = MPL_sock_poll(pfds, np, 100);
        if (n > 0) {
            if (pfds[0].revents & (POLLIN | POLLERR | POLLHUP)) {
                int fd = MPL_sock_accept(listen_fd, NULL, NULL);
                if (fd >= 0) {
                    if (nconns == conns_cap) {
                        conns_cap *= 2;
                        conns = realloc(conns, conns_cap * sizeof(conn_t));
                        pfds = realloc(pfds, (conns_cap + 1) * sizeof(struct pollfd));
                    }
                    MPL_sock_set_cloexec(fd);
                    memset(&conns[nconns], 0, sizeof(conn_t));
                    conns[nconns].fd = fd;
                    conns[nconns].rank = -1;
                    nconns++;
                }
            }
            for (int p = 1; p < np; p++) {
                conn_t *c = &conns[p - 1];
                if (c->fd >= 0 && pfds[p].revents & (POLLIN | POLLERR | POLLHUP)) {
                    /* a rank that fails is detected below via its exit code */
                    if (!conn_read(c)) {
                        MPL_sock_close(c->fd);
                        c->fd = -1;
                    }
                }
            }
            /* drop closed connections from the poll set */
            int k = 0;
            for (int c = 0; c < nconns; c++)
                if (conns[c].fd >= 0)
                    conns[k++] = conns[c];
            nconns = k;
        }

        /* reap exited ranks */
        for (int r = 0; r < nprocs; r++) {
            if (procs[r] && WaitForSingleObject(procs[r], 0) == WAIT_OBJECT_0) {
                GetExitCodeProcess(procs[r], &exit_codes[r]);
                CloseHandle(procs[r]);
                procs[r] = NULL;
                nalive--;
                /* like hydra, treat a rank that connected to PMI but exited
                 * without finalizing as failed, even with exit code 0, as
                 * the other ranks may otherwise wait for it forever */
                int unfinalized = rank_initialized[r] && !rank_finalized[r];
                if (exit_codes[r] != 0 || unfinalized) {
                    EnterCriticalSection(&out_lock);
                    if (exit_codes[r] != 0)
                        fprintf(stderr, "[mpiexec] rank %d exited with code %lu\n", r,
                                (unsigned long) exit_codes[r]);
                    else
                        fprintf(stderr, "[mpiexec] rank %d exited without calling MPI_Finalize\n",
                                r);
                    fflush(stderr);
                    LeaveCriticalSection(&out_lock);
                    if (!rc)
                        rc = exit_codes[r] != 0 ? (int) exit_codes[r] : 1;
                    /* like other MPICH process managers, a failed rank
                     * brings down the job */
                    kill_all(rc);
                }
            }
        }
    }

    MPL_sock_close(listen_fd);

    /* Let the forwarders drain the pipes.  They see EOF once the ranks are
     * gone, unless a rank left a child behind that inherited its pipes. */
    for (int t = 0; t < nfwd; t++)
        WaitForSingleObject(fwd_threads[t], 5000);
    return rc;
}

/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpid_nem_impl.h"

#define printf_dd(x...) /*printf (x) */

int MPID_nem_ckpt_logging_messages = 0; /* are we in logging-message-mode? */
int MPID_nem_ckpt_sending_markers = 0; /* are we in the process of sending markers? */
struct cli_message_log_total *MPID_nem_ckpt_message_log = 0; /* are we replaying messages? */

#ifdef ENABLED_CHECKPOINTING
#include "cli.h"

static int* log_msg;
static int* sent_marker;
static unsigned short current_wave;
static int next_marker_dest;

static void checkpoint_shutdown();
static void restore_env (int rank);

extern unsigned short *MPID_nem_recv_seqno;

static int _rank;

#undef FUNCNAME
#define FUNCNAME MPID_nem_ckpt_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_ckpt_init (int ckpt_restart)
{
    int mpi_errno = MPI_SUCCESS;
    int num_procs;
    int rank;
    int i;
    MPIU_CHKPMEM_DECL(3);
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_CKPT_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_CKPT_INIT);

    num_procs = MPID_nem_mem_region.num_procs;
    rank = _rank = MPID_nem_mem_region.rank;

    if (!ckpt_restart)
    {
	process_id_t *procids;

        MPIU_CHKPMEM_MALLOC (procids, process_id_t *, sizeof (*procids) * num_procs, mpi_errno, "procids");

	for (i = 0; i < num_procs; ++i)
	    procids[i] = i;
	
	cli_init (num_procs, procids, rank, checkpoint_shutdown);
    }

    MPIU_CHKPMEM_MALLOC (log_msg, int *, sizeof (*log_msg) * num_procs, mpi_errno, "log_msg");
    MPIU_CHKPMEM_MALLOC (sent_marker, int *, sizeof (*sent_marker) * num_procs, mpi_errno, "sent_marker");

    for (i = 0; i < num_procs; ++i)
    {
	log_msg[i] = 0;
	sent_marker[i] = 0;
    }

    MPID_nem_ckpt_logging_messages = 0;
    MPID_nem_ckpt_sending_markers = 0;

    current_wave = 0;

    MPIU_CHKPMEM_COMMIT();
 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_CKPT_INIT);
    return mpi_errno;
 fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_ckpt_finalize
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_ckpt_finalize()
{
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_CKPT_FINALIZE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_CKPT_FINALIZE);

    MPIU_Free (log_msg);
    MPIU_Free (sent_marker);

    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_CKPT_FINALIZE);
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_ckpt_maybe_take_checkpoint
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_ckpt_maybe_take_checkpoint()
{
    int mpi_errno = MPI_SUCCESS;
    int ret;
    struct cli_marker marker;
    int num_procs;
    int rank;
    int i;
    MPIU_CHKLMEM_DECL(1);
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_CKPT_MAYBE_TAKE_CHECKPOINT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_CKPT_MAYBE_TAKE_CHECKPOINT);

    num_procs = MPID_nem_mem_region.num_procs;
    rank = MPID_nem_mem_region.rank;

    ret = cli_check_for_checkpoint_start (&marker);

    switch (ret)
    {
    case CLI_CP_BEGIN:	
	MPIU_Assert (MPID_nem_ckpt_logging_messages == 0);

	/* initialize the state */
	MPID_nem_ckpt_logging_messages = num_procs;
	MPID_nem_ckpt_sending_markers = num_procs;
	for (i = 0; i < num_procs; ++i)
	{
	    log_msg[i] = 1;
	    sent_marker[i] = 0;
	}
	
	/* we don't send messages to ourselves, so we pretend we sent and received a marker */
	--MPID_nem_ckpt_sending_markers;
	ret = cli_on_marker_receive (&marker, rank);
        MPIU_ERR_CHKANDJUMP (ret != CLI_CP_MARKED, mpi_errno, MPI_ERR_OTHER, "**intern");
	--MPID_nem_ckpt_logging_messages;
	log_msg[rank] = 0;
	sent_marker[rank] = 1;

	next_marker_dest = 0;
	current_wave = marker.checkpoint_wave_number;
	mpi_errno = MPID_nem_ckpt_send_markers();
        if (mpi_errno) MPIU_ERR_POP (mpi_errno);
	break;
    case CLI_RESTART:
	{
	    int newrank;
	    int newsize;
	    struct cli_emitter_based_message_log *per_rank_log;

	    mpi_errno = restore_env (rank);
            if (mpi_errno) MPIU_ERR_POP (mpi_errno);

	    _MPID_nem_init (0, NULL, &newrank, &newsize, 1);
	    MPIU_Assert (newrank == rank);
	    MPIU_Assert (newsize == num_procs);

            /* we don't use the per_rank_log, so we discard it */
            MPIU_CHKLMEM_MALLOC (per_rank_log, struct cli_emitter_based_message_log *, sizeof (struct cli_emitter_based_message_log) * num_procs, mpi_errno, "per rank log");
	    MPID_nem_ckpt_message_log = cli_get_network_state (per_rank_log);
	    break;
	}
    case CLI_NOTHING:
	break;
    default:
        MPIU_ERR_CHKANDJUMP (ret != CLI_CP_MARKED, mpi_errno, MPI_ERR_OTHER, "**intern");
    }

 fn_exit:
    MPIU_CHKLMEM_FREEALL();
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_CKPT_MAYBE_TAKE_CHECKPOINT);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_ckpt_got_marker
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void
MPID_nem_ckpt_got_marker (MPID_nem_cell_ptr_t *cell, int *in_fbox)
{
    int mpi_errno = MPI_SUCCESS;
    int ret;
    int source;
    struct cli_marker marker;
    int num_procs;
    int rank;
    int i;
    MPIDI_VC_t *vc;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_CKPT_GOT_MARKER);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_CKPT_GOT_MARKER);

    num_procs = MPID_nem_mem_region.num_procs;
    rank = MPID_nem_mem_region.rank;

    marker.checkpoint_wave_number = (*cell)->pkt.ckpt.wave;
    source = (*cell)->pkt.ckpt.source;

    if (!*in_fbox)
    {
	MPIDI_PG_Get_vc (MPIDI_Process.my_pg, source, &vc);
	MPID_nem_mpich2_release_cell (*cell, vc);
    }
    else
    {
	MPID_nem_mpich2_release_fbox (*cell);
    }

    *cell = NULL;
    *in_fbox = 0;

    ret = cli_on_marker_receive (&marker, source);

    switch (ret)
    {
    case CLI_CP_BEGIN:
	MPIU_Assert (MPID_nem_ckpt_logging_messages == 0);

	/* initialize the state */
	MPID_nem_ckpt_logging_messages = num_procs;
	MPID_nem_ckpt_sending_markers = num_procs;
	for (i = 0; i < num_procs; ++i)
	{
	    log_msg[i] = 1;
	    sent_marker[i] = 0;
	}
	
	/* we received one from the source of this message */
	--MPID_nem_ckpt_logging_messages;
	log_msg[source] = 0;
	
	/* we don't send messages to ourselves, so we pretend we sent and received a marker */
	--MPID_nem_ckpt_sending_markers;
	ret = cli_on_marker_receive (&marker, rank);
        MPIU_ERR_CHKANDJUMP (ret != CLI_CP_MARKED, mpi_errno, MPI_ERR_OTHER, "**intern");
	--MPID_nem_ckpt_logging_messages;
	log_msg[rank] = 0;
	sent_marker[rank] = 1;

	next_marker_dest = 0;
	current_wave = marker.checkpoint_wave_number;
	mpi_errno = MPID_nem_ckpt_send_markers();
        if (mpi_errno) MPIU_ERR_POP (mpi_errno);
	break;
    case CLI_CP_MARKED:
	MPIU_Assert (MPID_nem_ckpt_logging_messages);
	MPIU_Assert (log_msg[source]);

	--MPID_nem_ckpt_logging_messages;
	log_msg[source] = 0;
	break;
    case CLI_RESTART:
	{
	    int newrank;
	    int newsize;
	    struct cli_emitter_based_message_log *per_rank_log;

	    --MPID_nem_recv_seqno[source]; /* Will thie really work????? */

	    printf_dd ("%d: cli_on_marker_recv: CLI_CP_RESTART wave = %d\n", rank, marker.checkpoint_wave_number);
	    restore_env (rank);
	    printf_dd ("%d: before _MPID_nem_init\n", rank);
	    _MPID_nem_init (0, NULL, &newrank, &newsize, 1);
	    printf_dd ("%d: after _MPID_nem_init\n", rank);
	    MPIU_Assert (newrank == rank);
	    MPIU_Assert (newsize == num_procs);
	    printf_dd ("%d: _MPID_nem_init done\n", rank);

            /* we don't use the per_rank_log, so we discard it */
            MPIU_CHKLMEM_MALLOC (per_rank_log, struct cli_emitter_based_message_log *, sizeof (struct cli_emitter_based_message_log) * num_procs, mpi_errno, "per rank log");
	    MPID_nem_ckpt_message_log = cli_get_network_state (per_rank_log);
	    break;
	}
    default:
        MPIU_ERR_CHKANDJUMP (ret != CLI_CP_MARKED, mpi_errno, MPI_ERR_OTHER, "**intern");
	return;
    }

 fn_exit:
    MPIU_CHKLMEM_FREEALL();
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_CKPT_GOT_MARKER);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_ckpt_log_message
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_ckpt_log_message (MPID_nem_cell_ptr_t cell)
{
    int mpi_errno = MPI_SUCCESS;
    int source;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_CKPT_LOG_MESSAGE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_CKPT_LOG_MESSAGE);

    source = cell->pkt.mpich2.source;
    if (log_msg[source])
    {
	cli_log_message (cell, cell->pkt.header.datalen + sizeof(MPIDI_CH3_Pkt_t) + MPID_NEM_OFFSETOF (MPID_nem_cell_t, pkt.mpich2.payload), source);
    }

    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_CKPT_LOG_MESSAGE);
    return mpi_errno;
}

/* MPID_nem_ckpt_send_markers() -- keep sending markers until we're done or out of cells */
#undef FUNCNAME
#define FUNCNAME MPID_nem_ckpt_send_markers
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_ckpt_send_markers()
{
    int mpi_errno = MPI_SUCCESS;
    int try_again;
    int num_procs = MPID_nem_mem_region.num_procs;
    MPIDI_VC_t vc;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_CKPT_SEND_MARKERS);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_CKPT_SEND_MARKERS);

    MPIU_Assert (MPID_nem_ckpt_sending_markers);
    MPIU_Assert (next_marker_dest < num_procs);

    while (next_marker_dest < num_procs)
    {
	if (sent_marker[next_marker_dest])
	{
	    ++next_marker_dest;
	    continue;
	}
	
	MPIDI_PG_Get_vc (MPIDI_Process.my_pg, next_marker_dest, &vc);
	mpi_errno = MPID_nem_mpich2_send_ckpt_marker (current_wave, vc, &try_again);
        if (mpi_errno) MPIU_ERR_POP (mpi_errno);
        if (try_again)
            break;

	sent_marker[next_marker_dest] = 1;
	++next_marker_dest;
	--MPID_nem_ckpt_sending_markers;
    }

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_CKPT_SEND_MARKERS);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_ckpt_replay_message
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_ckpt_replay_message (MPID_nem_cell_ptr_t *cell)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_CKPT_REPLAY_MESSAGE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_CKPT_REPLAY_MESSAGE);

    MPIU_Assert (MPID_nem_ckpt_message_log);

    *cell = (MPID_nem_cell_ptr_t)MPID_nem_ckpt_message_log->ptr;
    (*cell)->pkt.header.type = MPID_NEM_PKT_CKPT_REPLAY;

    MPID_nem_ckpt_message_log = MPID_nem_ckpt_message_log->next;

    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_CKPT_REPLAY_MESSAGE);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_ckpt_free_msg_log
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void
MPID_nem_ckpt_free_msg_log()
{
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_CKPT_FREE_MSG_LOG);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_CKPT_FREE_MSG_LOG);

    MPIU_Assert (!MPID_nem_ckpt_message_log);
    cli_message_log_free();

    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_CKPT_FREE_MSG_LOG);
}

#undef FUNCNAME
#define FUNCNAME checkpoint_shutdown
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int
checkpoint_shutdown()
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPID_nem_ckpt_shutdown ();
    if (mpi_errno) MPIU_ERR_POP (mpi_errno);

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#define MAX_STR_LEN 256

#undef FUNCNAME
#define FUNCNAME restore_env
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int
restore_env (int rank)
{
    int mpi_errno = MPI_SUCCESS;
    FILE *fd;
    char env_filename[MAX_STR_LEN];
    char var[MAX_STR_LEN], val[MAX_STR_LEN];
    int ret;


    MPIU_Snprintf (env_filename, MAX_STR_LEN, "/tmp/cli-restart-env:%d", rank);

    fd = fopen (env_filename, "r");
    MPIU_ERR_CHKANDJUMP1 (!fd, mpi_errno, MPI_ERR_OTHER, "**open", "**open %s", strerror (errno));

    /* { */
/* 	int i; */

/* 	for (i = 0; i < sizeof(FILE); ++i) */
/* 	{ */
/* 	    if (!(i%8)) */
/* 		printf (" "); */
/* 	    if (!(i%64)) */
/* 		printf ("\n"); */
/* 	    printf ("%02x", ((unsigned char *)fd)[i]); */
/* 	} */
/* 	printf ("\n--\n"); */

/* 	fgets (var, MAX_STR_LEN, fd); */
/* 	printf_dd ("var = %s\n", var); */
/*     } */

    ret = fscanf (fd, "%[^=]=%[^\n]\n", var, val);

    while (ret != EOF)
    {
	if (ret == 2)
	{
	    ret = setenv (var, val, 1);
            MPIU_ERR_CHKANDJUMP (ret != 0, mpi_errno, MPI_ERR_OTHER, "**setenv");
	}
	ret = fscanf (fd, "%[^=]=%[^\n]\n", var, val);
    }

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}
#endif

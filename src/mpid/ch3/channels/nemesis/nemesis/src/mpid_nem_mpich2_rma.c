/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpid_nem_impl.h"
#include "mpid_nem_nets.h"
#include <string.h>
#include <errno.h>

#define FNAME_TEMPLATE "/tmp/SHAR_TMPXXXXXX"

#undef FUNCNAME
#define FUNCNAME MPID_nem_mpich2_alloc_win
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_mpich2_alloc_win (void **buf, int len, MPID_nem_mpich2_win_t **win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIU_CHKPMEM_DECL(1);
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_MPICH2_ALLOC_WIN);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_MPICH2_ALLOC_WIN);

    MPIU_CHKPMEM_MALLOC (*win, MPID_nem_mpich2_win_t *, sizeof (MPID_nem_mpich2_win_t), mpi_errno, "rma win object");

    mpi_errno = MPID_nem_allocate_shared_memory ((char **)buf, len, &(*win)->handle);
    if (mpi_errno) MPIU_ERR_POP (mpi_errno);

    (*win)->proc = MPID_nem_mem_region.rank;
    (*win)->home_address = *buf;
    (*win)->len = len;
    (*win)->local_address = *buf;

    MPIU_CHKPMEM_COMMIT();
 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_MPICH2_ALLOC_WIN);
    return mpi_errno;
 fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_mpich2_free_win
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_mpich2_free_win (MPID_nem_mpich2_win_t *win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_MPICH2_FREE_WIN);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_MPICH2_FREE_WIN);

    MPIU_Assert (win->proc == MPID_nem_mem_region.rank);

    mpi_errno = MPID_nem_remove_shared_memory (win->handle);
    if (mpi_errno) MPIU_ERR_POP (mpi_errno);

    mpi_errno = MPID_nem_detach_shared_memory (win->home_address, win->len);
    if (mpi_errno) MPIU_ERR_POP (mpi_errno);

    MPIU_Free (win->handle);
    MPIU_Free (win);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_MPICH2_FREE_WIN);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_mpich2_attach_win
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_mpich2_attach_win (void **buf, MPID_nem_mpich2_win_t *remote_win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_MPICH2_ATTACH_WIN);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_MPICH2_ATTACH_WIN);

    if (remote_win->proc == MPID_nem_mem_region.rank)
    {
        /* don't actually attach if the window is local */
	*buf = remote_win->home_address;
    }
    else
    {
        mpi_errno = MPID_nem_attach_shared_memory ((char **)buf, remote_win->len, remote_win->handle);
        if (mpi_errno) MPIU_ERR_POP (mpi_errno);
    }

    remote_win->local_address = *buf;

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_MPICH2_ATTACH_WIN);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_mpich2_detach_win
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_mpich2_detach_win (MPID_nem_mpich2_win_t *remote_win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_MPICH2_DETACH_WIN);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_MPICH2_DETACH_WIN);

    if (remote_win->proc != MPID_nem_mem_region.rank)
    {
        /* we didn't actually attach if the "remote window" is local */
        mpi_errno = MPID_nem_detach_shared_memory (remote_win->local_address, remote_win->len);
        if (mpi_errno) MPIU_ERR_POP (mpi_errno);
    }

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_MPICH2_DETACH_WIN);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_mpich2_win_put
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_mpich2_win_put (void *s_buf, void *d_buf, int len, MPID_nem_mpich2_win_t *remote_win)
{
    int mpi_errno = MPI_SUCCESS;
    char *_d_buf = d_buf;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_MPICH2_WIN_PUT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_MPICH2_WIN_PUT);

    _d_buf += (char *)remote_win->local_address - (char *)remote_win->home_address;

    if (_d_buf < (char *)remote_win->local_address || _d_buf + len > (char *)remote_win->local_address + remote_win->len)
        MPIU_ERR_SETANDJUMP (mpi_errno, MPI_ERR_OTHER, "**winput_oob");

    MPIU_Memcpy (_d_buf, s_buf, len);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_MPICH2_WIN_PUT);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_mpich2_win_putv
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_mpich2_win_putv (struct iovec **s_iov, int *s_niov, struct iovec **d_iov, int *d_niov, MPID_nem_mpich2_win_t *remote_win)
{
    int mpi_errno = MPI_SUCCESS;
    size_t diff;
    int len;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_MPICH2_WIN_PUTV);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_MPICH2_WIN_PUTV);

    diff = (char *)remote_win->local_address - (char *)remote_win->home_address;

    if ((char *)(*d_iov)->iov_base + diff < (char *)remote_win->local_address ||
	(char *)(*d_iov)->iov_base + diff + (*d_iov)->iov_len > (char *)remote_win->local_address + remote_win->len)
        MPIU_ERR_SETANDJUMP (mpi_errno, MPI_ERR_OTHER, "**winput_oob");

    while (*s_niov && *d_niov)
    {
	if ((*s_iov)->iov_len > (*d_iov)->iov_len)
	{
	    len = (*d_iov)->iov_len;
	    MPIU_Memcpy ((char*)((*d_iov)->iov_base) + diff, (*s_iov)->iov_base, len);

	    (*s_iov)->iov_base = (char *)(*s_iov)->iov_base + len;
	    (*s_iov)->iov_len =- len;

	    ++(*d_iov);
	    --(*d_niov);
	    if ((char *)(*d_iov)->iov_base + diff < (char *)remote_win->local_address ||
		(char *)(*d_iov)->iov_base + diff + (*d_iov)->iov_len > (char *)remote_win->local_address + remote_win->len)
                MPIU_ERR_SETANDJUMP (mpi_errno, MPI_ERR_OTHER, "**winput_oob");
	}
	else if ((*s_iov)->iov_len > (*d_iov)->iov_len)
	{
	    len = (*s_iov)->iov_len;
	    MPIU_Memcpy ((char*)((*d_iov)->iov_base) + diff, (*s_iov)->iov_base, len);

	    ++(*s_iov);
	    --(*s_niov);

	    (*d_iov)->iov_base = (char *)(*d_iov)->iov_base + len;
	    (*d_iov)->iov_len =- len;
	}
	else
	{
	    len = (*s_iov)->iov_len;
	    MPIU_Memcpy ((char*)((*d_iov)->iov_base) + diff, (*s_iov)->iov_base, len);

	    ++(*s_iov);
	    --(*s_niov);

	    ++(*d_iov);
	    --(*d_niov);
	    if ((char *)(*d_iov)->iov_base + diff < (char *)remote_win->local_address ||
		(char *)(*d_iov)->iov_base + diff + (*d_iov)->iov_len > (char *)remote_win->local_address + remote_win->len)
                MPIU_ERR_SETANDJUMP (mpi_errno, MPI_ERR_OTHER, "**winput_oob");
	}
    }

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_MPICH2_WIN_PUTV);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_mpich2_win_get
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_mpich2_win_get (void *s_buf, void *d_buf, int len, MPID_nem_mpich2_win_t *remote_win)
{
    int mpi_errno = MPI_SUCCESS;
    char *_s_buf = s_buf;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_MPICH2_WIN_GET);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_MPICH2_WIN_GET);

    _s_buf += (char *)remote_win->local_address - (char *)remote_win->home_address;

    if (_s_buf < (char *)remote_win->local_address || _s_buf + len > (char *)remote_win->local_address + remote_win->len)
        MPIU_ERR_SETANDJUMP (mpi_errno, MPI_ERR_OTHER, "**winget_oob");

    MPIU_Memcpy (d_buf, _s_buf, len);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_MPICH2_WIN_GET);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_mpich2_win_getv
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_mpich2_win_getv (struct iovec **s_iov, int *s_niov, struct iovec **d_iov, int *d_niov, MPID_nem_mpich2_win_t *remote_win)
{
    int mpi_errno = MPI_SUCCESS;
    size_t diff;
    int len;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_MPICH2_WIN_GETV);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_MPICH2_WIN_GETV);

    diff = (char *)remote_win->local_address - (char *)remote_win->home_address;

    if ((char *)(*s_iov)->iov_base + diff < (char *)remote_win->local_address ||
	(char *)(*s_iov)->iov_base + diff + (*s_iov)->iov_len > (char *)remote_win->local_address + remote_win->len)
        MPIU_ERR_SETANDJUMP (mpi_errno, MPI_ERR_OTHER, "**winget_oob");

    while (*s_niov && *d_niov)
    {
	if ((*d_iov)->iov_len > (*s_iov)->iov_len)
	{
	    len = (*s_iov)->iov_len;
	    MPIU_Memcpy ((char*)((*d_iov)->iov_base) + diff, (*s_iov)->iov_base, len);

	    (*d_iov)->iov_base = (char *)(*d_iov)->iov_base + len;
	    (*d_iov)->iov_len =- len;

	    ++(*s_iov);
	    --(*s_niov);
	    if ((char *)(*s_iov)->iov_base + diff < (char *)remote_win->local_address ||
		(char *)(*s_iov)->iov_base + diff + (*s_iov)->iov_len > (char *)remote_win->local_address + remote_win->len)
	        MPIU_ERR_SETANDJUMP (mpi_errno, MPI_ERR_OTHER, "**winget_oob");
	}
	else if ((*d_iov)->iov_len > (*s_iov)->iov_len)
	{
	    len = (*d_iov)->iov_len;
	    MPIU_Memcpy ((char*)((*d_iov)->iov_base) + diff, (*s_iov)->iov_base, len);

	    ++(*d_iov);
	    --(*d_niov);

	    (*s_iov)->iov_base = (char *)(*s_iov)->iov_base + len;
	    (*s_iov)->iov_len =- len;
	}
	else
	{
	    len = (*d_iov)->iov_len;
	    MPIU_Memcpy ((char*)((*d_iov)->iov_base) + diff, (*s_iov)->iov_base, len);

	    ++(*d_iov);
	    --(*d_niov);

	    ++(*s_iov);
	    --(*s_niov);
	    if ((char *)(*s_iov)->iov_base + diff < (char *)remote_win->local_address ||
		(char *)(*s_iov)->iov_base + diff + (*s_iov)->iov_len > (char *)remote_win->local_address + remote_win->len)
                MPIU_ERR_SETANDJUMP (mpi_errno, MPI_ERR_OTHER, "**winget_oob");
	}
    }

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_MPICH2_WIN_GETV);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}
#define WIN_HANLEN_KEY     "hanlen"
#define WIN_HANDLE_KEY     "handle"
#define WIN_PROC_KEY       "proc"
#define WIN_HOME_ADDR_KEY  "homeaddr"
#define WIN_LEN_KEY        "len"
#define WIN_LOCAL_ADDR_KEY "localaddr"

#undef FUNCNAME
#define FUNCNAME MPID_nem_mpich2_serialize_win
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_mpich2_serialize_win (void *buf, int buf_len, MPID_nem_mpich2_win_t *win, int *len)
{
    int mpi_errno = MPI_SUCCESS;
    int str_errno;
    int bl = buf_len;
    char *b = (char *)buf;
    int handle_len;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_MPICH2_SERIALIZE_WIN);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_MPICH2_SERIALIZE_WIN);

    handle_len = strlen (win->handle);
    str_errno = MPIU_Str_add_int_arg (&b, &bl, WIN_HANLEN_KEY, handle_len);
    MPIU_ERR_CHKANDJUMP (str_errno == MPIU_STR_NOMEM, mpi_errno, MPI_ERR_OTHER, "**nomem");
    MPIU_ERR_CHKANDJUMP (str_errno == MPIU_STR_FAIL, mpi_errno, MPI_ERR_OTHER, "**winserialize");
    str_errno = MPIU_Str_add_string_arg(&b, &bl, WIN_HANDLE_KEY, win->handle);
    MPIU_ERR_CHKANDJUMP (str_errno == MPIU_STR_NOMEM, mpi_errno, MPI_ERR_OTHER, "**nomem");
    MPIU_ERR_CHKANDJUMP (str_errno == MPIU_STR_FAIL, mpi_errno, MPI_ERR_OTHER, "**winserialize");
    str_errno = MPIU_Str_add_int_arg (&b, &bl, WIN_PROC_KEY, win->proc);
    MPIU_ERR_CHKANDJUMP (str_errno == MPIU_STR_NOMEM, mpi_errno, MPI_ERR_OTHER, "**nomem");
    MPIU_ERR_CHKANDJUMP (str_errno == MPIU_STR_FAIL, mpi_errno, MPI_ERR_OTHER, "**winserialize");
    str_errno = MPIU_Str_add_binary_arg (&b, &bl, WIN_HOME_ADDR_KEY, win->home_address, sizeof (win->home_address));
    MPIU_ERR_CHKANDJUMP (str_errno == MPIU_STR_NOMEM, mpi_errno, MPI_ERR_OTHER, "**nomem");
    MPIU_ERR_CHKANDJUMP (str_errno == MPIU_STR_FAIL, mpi_errno, MPI_ERR_OTHER, "**winserialize");
    str_errno = MPIU_Str_add_int_arg (&b, &bl, WIN_LEN_KEY, win->len);
    MPIU_ERR_CHKANDJUMP (str_errno == MPIU_STR_NOMEM, mpi_errno, MPI_ERR_OTHER, "**nomem");
    MPIU_ERR_CHKANDJUMP (str_errno == MPIU_STR_FAIL, mpi_errno, MPI_ERR_OTHER, "**winserialize");

    *len = buf_len - bl;

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_MPICH2_SERIALIZE_WIN);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_mpich2_serialize_win
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_mpich2_deserialize_win (void *buf, int buf_len, MPID_nem_mpich2_win_t **win)
{
    int mpi_errno = MPI_SUCCESS;
    int str_errno;
    int ol;
    char *b = (char *)buf;
    int handle_len;
    MPIU_CHKPMEM_DECL(2);
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_MPICH2_DESERIALIZE_WIN);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_MPICH2_DESERIALIZE_WIN);

    MPIU_CHKPMEM_MALLOC (*win, MPID_nem_mpich2_win_t *, sizeof (MPID_nem_mpich2_win_t), mpi_errno, "win object");

    str_errno = MPIU_Str_get_int_arg (b, WIN_HANLEN_KEY, &handle_len);
    MPIU_ERR_CHKANDJUMP (str_errno == MPIU_STR_NOMEM, mpi_errno, MPI_ERR_OTHER, "**nomem");
    MPIU_ERR_CHKANDJUMP (str_errno == MPIU_STR_FAIL, mpi_errno, MPI_ERR_OTHER, "**windeserialize");
    MPIU_CHKPMEM_MALLOC ((*win)->handle, char *, handle_len, mpi_errno, "window handle");

    str_errno = MPIU_Str_get_string_arg(b, WIN_HANDLE_KEY, (*win)->handle, handle_len);
    MPIU_ERR_CHKANDJUMP (str_errno == MPIU_STR_NOMEM, mpi_errno, MPI_ERR_OTHER, "**nomem");
    MPIU_ERR_CHKANDJUMP (str_errno == MPIU_STR_FAIL, mpi_errno, MPI_ERR_OTHER, "**windeserialize");
    str_errno = MPIU_Str_get_int_arg (b, WIN_PROC_KEY, &(*win)->proc);
    MPIU_ERR_CHKANDJUMP (str_errno == MPIU_STR_NOMEM, mpi_errno, MPI_ERR_OTHER, "**nomem");
    MPIU_ERR_CHKANDJUMP (str_errno == MPIU_STR_FAIL, mpi_errno, MPI_ERR_OTHER, "**windeserialize");
    str_errno = MPIU_Str_get_binary_arg (b, WIN_HOME_ADDR_KEY, (char *)&(*win)->home_address, sizeof ((*win)->home_address), &ol);
    MPIU_ERR_CHKANDJUMP (str_errno == MPIU_STR_NOMEM, mpi_errno, MPI_ERR_OTHER, "**nomem");
    MPIU_ERR_CHKANDJUMP (str_errno == MPIU_STR_FAIL || ol != sizeof ((*win)->home_address), mpi_errno, MPI_ERR_OTHER, "**windeserialize");
    str_errno = MPIU_Str_get_int_arg (b, WIN_LEN_KEY, &(*win)->len);
    MPIU_ERR_CHKANDJUMP (str_errno == MPIU_STR_NOMEM, mpi_errno, MPI_ERR_OTHER, "**nomem");
    MPIU_ERR_CHKANDJUMP (str_errno == MPIU_STR_FAIL, mpi_errno, MPI_ERR_OTHER, "**windeserialize");

    (*win)->local_address = 0;

    MPIU_CHKPMEM_COMMIT();
 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_MPICH2_DESERIALIZE_WIN);
    return mpi_errno;
 fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_mpich2_register_memory
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_mpich2_register_memory (void *buf, int len)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_MPICH2_REGISTER_MEMORY);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_MPICH2_REGISTER_MEMORY);

/*     if (MPID_NEM_NET_MODULE == MPID_NEM_GM_MODULE) */
/*     { */
/* 	/\*return MPID_nem_gm_module_register_mem (buf, len);*\/ */
/*     } */

    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_MPICH2_REGISTER_MEMORY);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_mpich2_deregister_memory
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_mpich2_deregister_memory (void *buf, int len)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_MPICH2_DEREGISTER_MEMORY);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_MPICH2_DEREGISTER_MEMORY);

/*     if (MPID_NEM_NET_MODULE == MPID_NEM_GM_MODULE) */
/*     { */
/* 	/\*return MPID_nem_gm_module_deregister_mem (buf, len);*\/ */
/*     } */

    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_MPICH2_DEREGISTER_MEMORY);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_mpich2_put
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_mpich2_put (void *s_buf, void *d_buf, int len, int proc, int *completion_ctr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_MPICH2_PUT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_MPICH2_PUT);

    if (MPID_NEM_IS_LOCAL (proc))
    {
        MPIU_ERR_SETANDJUMP (mpi_errno, MPI_ERR_OTHER, "**notimpl");
    }
    else
    {
      /*
	switch (MPID_NEM_NET_MODULE)
	{
	case MPID_NEM_GM_MODULE:
	    MPID_NEM_ATOMIC_INC (completion_ctr);
	    return MPID_nem_gm_module_put (d_buf, s_buf, len, proc, completion_ctr);
	    break;
	default:
            MPIU_ERR_SETANDJUMP (mpi_errno, MPI_ERR_OTHER, "**notimpl");
	    break;
	}
      */
    }
 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_MPICH2_PUT);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_mpich2_putv
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_mpich2_putv (struct iovec **s_iov, int *s_niov, struct iovec **d_iov, int *d_niov, int proc,
		   int *completion_ctr)
{
    int mpi_errno = MPI_SUCCESS;
    /* int len;*/
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_MPICH2_PUTV);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_MPICH2_PUTV);

    if (MPID_NEM_IS_LOCAL (proc))
    {
        MPIU_ERR_SETANDJUMP (mpi_errno, MPI_ERR_OTHER, "**notimpl");
    }
    else
    {
      /*
	switch (MPID_NEM_NET_MODULE)
	{
	case MPID_NEM_GM_MODULE:
	    while (*s_niov && *d_niov)
	    {
		if ((*d_iov)->iov_len > (*s_iov)->iov_len)
		{
		    len = (*s_iov)->iov_len;
		    MPID_NEM_ATOMIC_INC (completion_ctr);
		    MPID_nem_gm_module_put ((*d_iov)->iov_base, (*s_iov)->iov_base, len, proc, completion_ctr);

		    (*d_iov)->iov_base = (char *)(*d_iov)->iov_base + len;
		    (*d_iov)->iov_len =- len;

		    ++(*s_iov);
		    --(*s_niov);
		}
		else if ((*d_iov)->iov_len > (*s_iov)->iov_len)
		{
		    len = (*d_iov)->iov_len;
		    MPID_NEM_ATOMIC_INC (completion_ctr);
		    MPID_nem_gm_module_put ((*d_iov)->iov_base, (*s_iov)->iov_base, len, proc, completion_ctr);

		    ++(*d_iov);
		    --(*d_niov);

		    (*s_iov)->iov_base = (char *)(*s_iov)->iov_base + len;
		    (*s_iov)->iov_len =- len;
		}
		else
		{
		    len = (*d_iov)->iov_len;
		    MPID_NEM_ATOMIC_INC (completion_ctr);
		    MPID_nem_gm_module_put ((*d_iov)->iov_base, (*s_iov)->iov_base, len, proc, completion_ctr);

		    ++(*d_iov);
		    --(*d_niov);

		    ++(*s_iov);
		    --(*s_niov);
		}
	    }
	    break;
	default:
	    MPIU_ERR_SETANDJUMP (mpi_errno, MPI_ERR_OTHER, "**notimpl");
	    break;
	}
      */
    }
 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_MPICH2_PUTV);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_mpich2_get
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_mpich2_get (void *s_buf, void *d_buf, int len, int proc, int *completion_ctr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_MPICH2_GET);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_MPICH2_GET);

    if (MPID_NEM_IS_LOCAL (proc))
    {
        MPIU_ERR_SETANDJUMP (mpi_errno, MPI_ERR_OTHER, "**notimpl");
    }
    else
    {
      /*
      switch (MPID_NEM_NET_MODULE)
	{
	case MPID_NEM_GM_MODULE:
	    MPID_NEM_ATOMIC_INC (completion_ctr);
	    return MPID_nem_gm_module_get (d_buf, s_buf, len, proc, completion_ctr);
	    break;
	default:
	    MPIU_ERR_SETANDJUMP (mpi_errno, MPI_ERR_OTHER, "**notimpl");
	    break;
	}
      */
    }
 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_MPICH2_GET);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_mpich2_getv
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_mpich2_getv (struct iovec **s_iov, int *s_niov, struct iovec **d_iov, int *d_niov, int proc,
		   int *completion_ctr)
{
    int mpi_errno = MPI_SUCCESS;
    /* int len; */
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_MPICH2_GETV);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_MPICH2_GETV);

    if (MPID_NEM_IS_LOCAL (proc))
    {
        MPIU_ERR_SETANDJUMP (mpi_errno, MPI_ERR_OTHER, "**notimpl");
    }
    else
    {
      /*
	switch (MPID_NEM_NET_MODULE)
	{
	case MPID_NEM_GM_MODULE:
	    while (*s_niov && *d_niov)
	    {
		if ((*d_iov)->iov_len > (*s_iov)->iov_len)
		{
		    len = (*s_iov)->iov_len;
		    MPID_NEM_ATOMIC_INC (completion_ctr);
		    MPID_nem_gm_module_get ((*d_iov)->iov_base, (*s_iov)->iov_base, len, proc, completion_ctr);

		    (*d_iov)->iov_base = (char *)(*d_iov)->iov_base + len;
		    (*d_iov)->iov_len =- len;

		    ++(*s_iov);
		    --(*s_niov);
		}
		else if ((*d_iov)->iov_len > (*s_iov)->iov_len)
		{
		    len = (*d_iov)->iov_len;
		    MPID_NEM_ATOMIC_INC (completion_ctr);
		    MPID_nem_gm_module_get ((*d_iov)->iov_base, (*s_iov)->iov_base, len, proc, completion_ctr);

		    ++(*d_iov);
		    --(*d_niov);

		    (*s_iov)->iov_base = (char *)(*s_iov)->iov_base + len;
		    (*s_iov)->iov_len =- len;
		}
		else
		{
		    len = (*d_iov)->iov_len;
		    MPID_NEM_ATOMIC_INC (completion_ctr);
		    MPID_nem_gm_module_get ((*d_iov)->iov_base, (*s_iov)->iov_base, len, proc, completion_ctr);

		    ++(*d_iov);
		    --(*d_niov);

		    ++(*s_iov);
		    --(*s_niov);
		}
	    }
	    break;
	default:
	    MPIU_ERR_SETANDJUMP (mpi_errno, MPI_ERR_OTHER, "**notimpl");
	    break;
	}
      */
    }
 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_MPICH2_GETV);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
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
#define FUNCNAME MPID_nem_mpich_alloc_win
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int
MPID_nem_mpich_alloc_win (void **buf, int len, MPID_nem_mpich_win_t **win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_CHKPMEM_DECL(1);
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEM_MPICH_ALLOC_WIN);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEM_MPICH_ALLOC_WIN);

    MPIR_CHKPMEM_MALLOC (*win, MPID_nem_mpich_win_t *, sizeof (MPID_nem_mpich_win_t), mpi_errno, "rma win object", MPL_MEM_RMA);

    mpi_errno = MPID_nem_allocate_shared_memory ((char **)buf, len, &(*win)->handle);
    if (mpi_errno) MPIR_ERR_POP (mpi_errno);

    (*win)->proc = MPID_nem_mem_region.rank;
    (*win)->home_address = *buf;
    (*win)->len = len;
    (*win)->local_address = *buf;

    MPIR_CHKPMEM_COMMIT();
 fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEM_MPICH_ALLOC_WIN);
    return mpi_errno;
 fn_fail:
    MPIR_CHKPMEM_REAP();
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_mpich_free_win
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int
MPID_nem_mpich_free_win (MPID_nem_mpich_win_t *win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEM_MPICH_FREE_WIN);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEM_MPICH_FREE_WIN);

    MPIR_Assert (win->proc == MPID_nem_mem_region.rank);

    mpi_errno = MPID_nem_remove_shared_memory (win->handle);
    if (mpi_errno) MPIR_ERR_POP (mpi_errno);

    mpi_errno = MPID_nem_detach_shared_memory (win->home_address, win->len);
    if (mpi_errno) MPIR_ERR_POP (mpi_errno);

    MPL_free (win->handle);
    MPL_free (win);

 fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEM_MPICH_FREE_WIN);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_mpich_attach_win
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int
MPID_nem_mpich_attach_win (void **buf, MPID_nem_mpich_win_t *remote_win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEM_MPICH_ATTACH_WIN);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEM_MPICH_ATTACH_WIN);

    if (remote_win->proc == MPID_nem_mem_region.rank)
    {
        /* don't actually attach if the window is local */
	*buf = remote_win->home_address;
    }
    else
    {
        mpi_errno = MPID_nem_attach_shared_memory ((char **)buf, remote_win->len, remote_win->handle);
        if (mpi_errno) MPIR_ERR_POP (mpi_errno);
    }

    remote_win->local_address = *buf;

 fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEM_MPICH_ATTACH_WIN);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_mpich_detach_win
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int
MPID_nem_mpich_detach_win (MPID_nem_mpich_win_t *remote_win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEM_MPICH_DETACH_WIN);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEM_MPICH_DETACH_WIN);

    if (remote_win->proc != MPID_nem_mem_region.rank)
    {
        /* we didn't actually attach if the "remote window" is local */
        mpi_errno = MPID_nem_detach_shared_memory (remote_win->local_address, remote_win->len);
        if (mpi_errno) MPIR_ERR_POP (mpi_errno);
    }

 fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEM_MPICH_DETACH_WIN);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_mpich_win_put
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int
MPID_nem_mpich_win_put (void *s_buf, void *d_buf, int len, MPID_nem_mpich_win_t *remote_win)
{
    int mpi_errno = MPI_SUCCESS;
    char *_d_buf = d_buf;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEM_MPICH_WIN_PUT);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEM_MPICH_WIN_PUT);

    _d_buf += (char *)remote_win->local_address - (char *)remote_win->home_address;

    if (_d_buf < (char *)remote_win->local_address || _d_buf + len > (char *)remote_win->local_address + remote_win->len)
        MPIR_ERR_SETANDJUMP (mpi_errno, MPI_ERR_OTHER, "**winput_oob");

    MPIR_Memcpy (_d_buf, s_buf, len);

 fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEM_MPICH_WIN_PUT);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_mpich_win_putv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int
MPID_nem_mpich_win_putv (struct iovec **s_iov, int *s_niov, struct iovec **d_iov, int *d_niov, MPID_nem_mpich_win_t *remote_win)
{
    int mpi_errno = MPI_SUCCESS;
    size_t diff;
    int len;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEM_MPICH_WIN_PUTV);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEM_MPICH_WIN_PUTV);

    diff = (char *)remote_win->local_address - (char *)remote_win->home_address;

    if ((char *)(*d_iov)->iov_base + diff < (char *)remote_win->local_address ||
	(char *)(*d_iov)->iov_base + diff + (*d_iov)->iov_len > (char *)remote_win->local_address + remote_win->len)
        MPIR_ERR_SETANDJUMP (mpi_errno, MPI_ERR_OTHER, "**winput_oob");

    while (*s_niov && *d_niov)
    {
	if ((*s_iov)->iov_len > (*d_iov)->iov_len)
	{
	    len = (*d_iov)->iov_len;
	    MPIR_Memcpy ((char*)((*d_iov)->iov_base) + diff, (*s_iov)->iov_base, len);

	    (*s_iov)->iov_base = (char *)(*s_iov)->iov_base + len;
	    (*s_iov)->iov_len =- len;

	    ++(*d_iov);
	    --(*d_niov);
	    if ((char *)(*d_iov)->iov_base + diff < (char *)remote_win->local_address ||
		(char *)(*d_iov)->iov_base + diff + (*d_iov)->iov_len > (char *)remote_win->local_address + remote_win->len)
                MPIR_ERR_SETANDJUMP (mpi_errno, MPI_ERR_OTHER, "**winput_oob");
	}
	else if ((*s_iov)->iov_len > (*d_iov)->iov_len)
	{
	    len = (*s_iov)->iov_len;
	    MPIR_Memcpy ((char*)((*d_iov)->iov_base) + diff, (*s_iov)->iov_base, len);

	    ++(*s_iov);
	    --(*s_niov);

	    (*d_iov)->iov_base = (char *)(*d_iov)->iov_base + len;
	    (*d_iov)->iov_len =- len;
	}
	else
	{
	    len = (*s_iov)->iov_len;
	    MPIR_Memcpy ((char*)((*d_iov)->iov_base) + diff, (*s_iov)->iov_base, len);

	    ++(*s_iov);
	    --(*s_niov);

	    ++(*d_iov);
	    --(*d_niov);
	    if ((char *)(*d_iov)->iov_base + diff < (char *)remote_win->local_address ||
		(char *)(*d_iov)->iov_base + diff + (*d_iov)->iov_len > (char *)remote_win->local_address + remote_win->len)
                MPIR_ERR_SETANDJUMP (mpi_errno, MPI_ERR_OTHER, "**winput_oob");
	}
    }

 fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEM_MPICH_WIN_PUTV);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_mpich_win_get
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int
MPID_nem_mpich_win_get (void *s_buf, void *d_buf, int len, MPID_nem_mpich_win_t *remote_win)
{
    int mpi_errno = MPI_SUCCESS;
    char *_s_buf = s_buf;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEM_MPICH_WIN_GET);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEM_MPICH_WIN_GET);

    _s_buf += (char *)remote_win->local_address - (char *)remote_win->home_address;

    if (_s_buf < (char *)remote_win->local_address || _s_buf + len > (char *)remote_win->local_address + remote_win->len)
        MPIR_ERR_SETANDJUMP (mpi_errno, MPI_ERR_OTHER, "**winget_oob");

    MPIR_Memcpy (d_buf, _s_buf, len);

 fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEM_MPICH_WIN_GET);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_mpich_win_getv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int
MPID_nem_mpich_win_getv (struct iovec **s_iov, int *s_niov, struct iovec **d_iov, int *d_niov, MPID_nem_mpich_win_t *remote_win)
{
    int mpi_errno = MPI_SUCCESS;
    size_t diff;
    int len;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEM_MPICH_WIN_GETV);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEM_MPICH_WIN_GETV);

    diff = (char *)remote_win->local_address - (char *)remote_win->home_address;

    if ((char *)(*s_iov)->iov_base + diff < (char *)remote_win->local_address ||
	(char *)(*s_iov)->iov_base + diff + (*s_iov)->iov_len > (char *)remote_win->local_address + remote_win->len)
        MPIR_ERR_SETANDJUMP (mpi_errno, MPI_ERR_OTHER, "**winget_oob");

    while (*s_niov && *d_niov)
    {
	if ((*d_iov)->iov_len > (*s_iov)->iov_len)
	{
	    len = (*s_iov)->iov_len;
	    MPIR_Memcpy ((char*)((*d_iov)->iov_base) + diff, (*s_iov)->iov_base, len);

	    (*d_iov)->iov_base = (char *)(*d_iov)->iov_base + len;
	    (*d_iov)->iov_len =- len;

	    ++(*s_iov);
	    --(*s_niov);
	    if ((char *)(*s_iov)->iov_base + diff < (char *)remote_win->local_address ||
		(char *)(*s_iov)->iov_base + diff + (*s_iov)->iov_len > (char *)remote_win->local_address + remote_win->len)
	        MPIR_ERR_SETANDJUMP (mpi_errno, MPI_ERR_OTHER, "**winget_oob");
	}
	else if ((*d_iov)->iov_len > (*s_iov)->iov_len)
	{
	    len = (*d_iov)->iov_len;
	    MPIR_Memcpy ((char*)((*d_iov)->iov_base) + diff, (*s_iov)->iov_base, len);

	    ++(*d_iov);
	    --(*d_niov);

	    (*s_iov)->iov_base = (char *)(*s_iov)->iov_base + len;
	    (*s_iov)->iov_len =- len;
	}
	else
	{
	    len = (*d_iov)->iov_len;
	    MPIR_Memcpy ((char*)((*d_iov)->iov_base) + diff, (*s_iov)->iov_base, len);

	    ++(*d_iov);
	    --(*d_niov);

	    ++(*s_iov);
	    --(*s_niov);
	    if ((char *)(*s_iov)->iov_base + diff < (char *)remote_win->local_address ||
		(char *)(*s_iov)->iov_base + diff + (*s_iov)->iov_len > (char *)remote_win->local_address + remote_win->len)
                MPIR_ERR_SETANDJUMP (mpi_errno, MPI_ERR_OTHER, "**winget_oob");
	}
    }

 fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEM_MPICH_WIN_GETV);
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
#define FUNCNAME MPID_nem_mpich_serialize_win
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int
MPID_nem_mpich_serialize_win (void *buf, int buf_len, MPID_nem_mpich_win_t *win, int *len)
{
    int mpi_errno = MPI_SUCCESS;
    int str_errno;
    int bl = buf_len;
    char *b = (char *)buf;
    int handle_len;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEM_MPICH_SERIALIZE_WIN);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEM_MPICH_SERIALIZE_WIN);

    handle_len = strlen (win->handle);
    str_errno = MPL_str_add_int_arg (&b, &bl, WIN_HANLEN_KEY, handle_len);
    MPIR_ERR_CHKANDJUMP (str_errno == MPL_STR_NOMEM, mpi_errno, MPI_ERR_OTHER, "**nomem");
    MPIR_ERR_CHKANDJUMP (str_errno == MPL_STR_FAIL, mpi_errno, MPI_ERR_OTHER, "**winserialize");
    str_errno = MPL_str_add_string_arg(&b, &bl, WIN_HANDLE_KEY, win->handle);
    MPIR_ERR_CHKANDJUMP (str_errno == MPL_STR_NOMEM, mpi_errno, MPI_ERR_OTHER, "**nomem");
    MPIR_ERR_CHKANDJUMP (str_errno == MPL_STR_FAIL, mpi_errno, MPI_ERR_OTHER, "**winserialize");
    str_errno = MPL_str_add_int_arg (&b, &bl, WIN_PROC_KEY, win->proc);
    MPIR_ERR_CHKANDJUMP (str_errno == MPL_STR_NOMEM, mpi_errno, MPI_ERR_OTHER, "**nomem");
    MPIR_ERR_CHKANDJUMP (str_errno == MPL_STR_FAIL, mpi_errno, MPI_ERR_OTHER, "**winserialize");
    str_errno = MPL_str_add_binary_arg (&b, &bl, WIN_HOME_ADDR_KEY, win->home_address, sizeof (win->home_address));
    MPIR_ERR_CHKANDJUMP (str_errno == MPL_STR_NOMEM, mpi_errno, MPI_ERR_OTHER, "**nomem");
    MPIR_ERR_CHKANDJUMP (str_errno == MPL_STR_FAIL, mpi_errno, MPI_ERR_OTHER, "**winserialize");
    str_errno = MPL_str_add_int_arg (&b, &bl, WIN_LEN_KEY, win->len);
    MPIR_ERR_CHKANDJUMP (str_errno == MPL_STR_NOMEM, mpi_errno, MPI_ERR_OTHER, "**nomem");
    MPIR_ERR_CHKANDJUMP (str_errno == MPL_STR_FAIL, mpi_errno, MPI_ERR_OTHER, "**winserialize");

    *len = buf_len - bl;

 fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEM_MPICH_SERIALIZE_WIN);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_mpich_serialize_win
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int
MPID_nem_mpich_deserialize_win (void *buf, int buf_len, MPID_nem_mpich_win_t **win)
{
    int mpi_errno = MPI_SUCCESS;
    int str_errno;
    int ol;
    char *b = (char *)buf;
    int handle_len;
    MPIR_CHKPMEM_DECL(2);
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEM_MPICH_DESERIALIZE_WIN);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEM_MPICH_DESERIALIZE_WIN);

    MPIR_CHKPMEM_MALLOC (*win, MPID_nem_mpich_win_t *, sizeof (MPID_nem_mpich_win_t), mpi_errno, "win object", MPL_MEM_RMA);

    str_errno = MPL_str_get_int_arg (b, WIN_HANLEN_KEY, &handle_len);
    MPIR_ERR_CHKANDJUMP (str_errno == MPL_STR_NOMEM, mpi_errno, MPI_ERR_OTHER, "**nomem");
    MPIR_ERR_CHKANDJUMP (str_errno == MPL_STR_FAIL, mpi_errno, MPI_ERR_OTHER, "**windeserialize");
    MPIR_CHKPMEM_MALLOC ((*win)->handle, char *, handle_len, mpi_errno, "window handle", MPL_MEM_RMA);

    str_errno = MPL_str_get_string_arg(b, WIN_HANDLE_KEY, (*win)->handle, handle_len);
    MPIR_ERR_CHKANDJUMP (str_errno == MPL_STR_NOMEM, mpi_errno, MPI_ERR_OTHER, "**nomem");
    MPIR_ERR_CHKANDJUMP (str_errno == MPL_STR_FAIL, mpi_errno, MPI_ERR_OTHER, "**windeserialize");
    str_errno = MPL_str_get_int_arg (b, WIN_PROC_KEY, &(*win)->proc);
    MPIR_ERR_CHKANDJUMP (str_errno == MPL_STR_NOMEM, mpi_errno, MPI_ERR_OTHER, "**nomem");
    MPIR_ERR_CHKANDJUMP (str_errno == MPL_STR_FAIL, mpi_errno, MPI_ERR_OTHER, "**windeserialize");
    str_errno = MPL_str_get_binary_arg (b, WIN_HOME_ADDR_KEY, (char *)&(*win)->home_address, sizeof ((*win)->home_address), &ol);
    MPIR_ERR_CHKANDJUMP (str_errno == MPL_STR_NOMEM, mpi_errno, MPI_ERR_OTHER, "**nomem");
    MPIR_ERR_CHKANDJUMP (str_errno == MPL_STR_FAIL || ol != sizeof ((*win)->home_address), mpi_errno, MPI_ERR_OTHER, "**windeserialize");
    str_errno = MPL_str_get_int_arg (b, WIN_LEN_KEY, &(*win)->len);
    MPIR_ERR_CHKANDJUMP (str_errno == MPL_STR_NOMEM, mpi_errno, MPI_ERR_OTHER, "**nomem");
    MPIR_ERR_CHKANDJUMP (str_errno == MPL_STR_FAIL, mpi_errno, MPI_ERR_OTHER, "**windeserialize");

    (*win)->local_address = 0;

    MPIR_CHKPMEM_COMMIT();
 fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEM_MPICH_DESERIALIZE_WIN);
    return mpi_errno;
 fn_fail:
    MPIR_CHKPMEM_REAP();
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_mpich_register_memory
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int
MPID_nem_mpich_register_memory (void *buf, int len)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEM_MPICH_REGISTER_MEMORY);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEM_MPICH_REGISTER_MEMORY);

/*     if (MPID_NEM_NET_MODULE == MPID_NEM_GM_MODULE) */
/*     { */
/* 	/\*return MPID_nem_gm_module_register_mem (buf, len);*\/ */
/*     } */

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEM_MPICH_REGISTER_MEMORY);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_mpich_deregister_memory
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int
MPID_nem_mpich_deregister_memory (void *buf, int len)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEM_MPICH_DEREGISTER_MEMORY);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEM_MPICH_DEREGISTER_MEMORY);

/*     if (MPID_NEM_NET_MODULE == MPID_NEM_GM_MODULE) */
/*     { */
/* 	/\*return MPID_nem_gm_module_deregister_mem (buf, len);*\/ */
/*     } */

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEM_MPICH_DEREGISTER_MEMORY);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_mpich_put
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int
MPID_nem_mpich_put (void *s_buf, void *d_buf, int len, int proc, int *completion_ctr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEM_MPICH_PUT);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEM_MPICH_PUT);

    if (MPID_NEM_IS_LOCAL (proc))
    {
        MPIR_ERR_SETANDJUMP (mpi_errno, MPI_ERR_OTHER, "**notimpl");
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
            MPIR_ERR_SETANDJUMP (mpi_errno, MPI_ERR_OTHER, "**notimpl");
	    break;
	}
      */
    }
 fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEM_MPICH_PUT);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_mpich_putv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int
MPID_nem_mpich_putv (struct iovec **s_iov, int *s_niov, struct iovec **d_iov, int *d_niov, int proc,
		   int *completion_ctr)
{
    int mpi_errno = MPI_SUCCESS;
    /* int len;*/
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEM_MPICH_PUTV);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEM_MPICH_PUTV);

    if (MPID_NEM_IS_LOCAL (proc))
    {
        MPIR_ERR_SETANDJUMP (mpi_errno, MPI_ERR_OTHER, "**notimpl");
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
	    MPIR_ERR_SETANDJUMP (mpi_errno, MPI_ERR_OTHER, "**notimpl");
	    break;
	}
      */
    }
 fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEM_MPICH_PUTV);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_mpich_get
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int
MPID_nem_mpich_get (void *s_buf, void *d_buf, int len, int proc, int *completion_ctr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEM_MPICH_GET);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEM_MPICH_GET);

    if (MPID_NEM_IS_LOCAL (proc))
    {
        MPIR_ERR_SETANDJUMP (mpi_errno, MPI_ERR_OTHER, "**notimpl");
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
	    MPIR_ERR_SETANDJUMP (mpi_errno, MPI_ERR_OTHER, "**notimpl");
	    break;
	}
      */
    }
 fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEM_MPICH_GET);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_mpich_getv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int
MPID_nem_mpich_getv (struct iovec **s_iov, int *s_niov, struct iovec **d_iov, int *d_niov, int proc,
		   int *completion_ctr)
{
    int mpi_errno = MPI_SUCCESS;
    /* int len; */
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEM_MPICH_GETV);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEM_MPICH_GETV);

    if (MPID_NEM_IS_LOCAL (proc))
    {
        MPIR_ERR_SETANDJUMP (mpi_errno, MPI_ERR_OTHER, "**notimpl");
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
	    MPIR_ERR_SETANDJUMP (mpi_errno, MPI_ERR_OTHER, "**notimpl");
	    break;
	}
      */
    }
 fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEM_MPICH_GETV);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if defined (MPL_USE_DBG_LOGGING)
MPL_dbg_class MPIDU_DBG_SOCK_CONNECT;
#endif /* MPL_USE_DBG_LOGGING */

/* FIXME: The usual missing documentation (what are these routines for?
   preconditions?  who calls? post conditions? */
#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_init
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDU_Sock_init(void)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDU_SOCK_INIT);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDU_SOCK_INIT);

#if defined (MPL_USE_DBG_LOGGING)
    MPIDU_DBG_SOCK_CONNECT = MPL_dbg_class_alloc("SOCK_CONNECT", "sock_connect");
#endif

    MPIDU_Socki_initialized++;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDU_SOCK_INIT);
    return MPI_SUCCESS;
}

/* FIXME: Who calls?  When?  Should this be a finalize handler instead? */
#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_finalize
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDU_Sock_finalize(void)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDU_SOCK_FINALIZE);

    MPIDU_SOCKI_VERIFY_INIT(mpi_errno, fn_exit);
    
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDU_SOCK_FINALIZE);

    MPIDU_Socki_initialized--;

    if (MPIDU_Socki_initialized == 0)
    {
	MPIDU_Socki_free_eventq_mem();
    }

#ifdef USE_SOCK_VERIFY
  fn_exit:
#endif
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDU_SOCK_FINALIZE);
    return mpi_errno;
}

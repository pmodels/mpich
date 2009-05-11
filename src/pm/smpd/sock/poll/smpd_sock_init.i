/* -*- Mode: C; c-basic-offset:4 ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* FIXME: The usual missing documentation (what are these routines for?
   preconditions?  who calls? post conditions? */
#undef FUNCNAME
#define FUNCNAME SMPDU_Sock_init
#undef FCNAME
#define FCNAME SMPDI_QUOTE(FUNCNAME)
int SMPDU_Sock_init(void)
{

    smpd_enter_fn(FCNAME);

    SMPDU_Socki_initialized++;

    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

/* FIXME: Who calls?  When?  Should this be a finalize handler instead? */
#undef FUNCNAME
#define FUNCNAME SMPDU_Sock_finalize
#undef FCNAME
#define FCNAME SMPDI_QUOTE(FUNCNAME)
int SMPDU_Sock_finalize(void)
{
    int result = SMPD_SUCCESS;

    SMPDU_SOCKI_VERIFY_INIT(result, fn_exit);
    
    smpd_enter_fn(FCNAME);

    SMPDU_Socki_initialized--;

    if (SMPDU_Socki_initialized == 0)
    {
	SMPDU_Socki_free_eventq_mem();
    }

#ifdef USE_SOCK_VERIFY
  fn_exit:
#endif
    smpd_exit_fn(FCNAME);
    return result;
}

/* -*- Mode: C; c-basic-offset:4 ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#undef FUNCNAME
#define FUNCNAME SMPDU_Sock_create_set
#undef FCNAME
#define FCNAME SMPDI_QUOTE(FUNCNAME)
int SMPDU_Sock_create_set(struct SMPDU_Sock_set ** sock_setp)
{
    struct SMPDU_Sock_set * sock_set = NULL;
    int result = SMPD_SUCCESS;

    smpd_enter_fn(FCNAME);
    
    SMPDU_SOCKI_VERIFY_INIT(result, fn_exit);

    /*
     * Allocate and initialized a new sock set structure
     */
    sock_set = MPIU_Malloc(sizeof(struct SMPDU_Sock_set));
    /* --BEGIN ERROR HANDLING-- */
    if (sock_set == NULL)
    { 
        result = SMPD_FAIL;
        smpd_err_printf("Allocating memory for sock set failed, errno = %d(%s) \n", errno, strerror(errno));
	goto fn_fail;
    }
    /* --END ERROR HANDLING-- */
    
    sock_set->id = SMPDU_Socki_set_next_id++;
    sock_set->poll_array_sz = 0;
    sock_set->poll_array_elems = 0;
    sock_set->starting_elem = 0;
    sock_set->pollfds = NULL;
    sock_set->pollinfos = NULL;
    sock_set->eventq_head = NULL;
    sock_set->eventq_tail = NULL;
    /* FIXME: Move the thread-specific operations into thread-specific
       routines (to allow for alternative thread sync models and
       for runtime control of thread level) */

    *sock_setp = sock_set;

  fn_exit:
    smpd_exit_fn(FCNAME);
    return result;

    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    if (sock_set != NULL)
    {
	MPIU_Free(sock_set);
    }

    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME SMPDU_Sock_destroy_set
#undef FCNAME
#define FCNAME SMPDI_QUOTE(FUNCNAME)
int SMPDU_Sock_destroy_set(struct SMPDU_Sock_set * sock_set)
{
    int elem;
    struct SMPDU_Sock_event event;
    int result = SMPD_SUCCESS;

    smpd_enter_fn(FCNAME);

    SMPDU_SOCKI_VERIFY_INIT(result, fn_exit);

    /*
     * FIXME: check for open socks and return an error if any are found
     */
    
    /*
     * FIXME: verify no other thread is blocked in poll().  wake it up and 
     * get it to exit.
     */
    
    /*
     * Close pipe for interrupting a blocking poll()
     */

    /*
     * Clear the event queue to eliminate memory leaks
     */
    while (SMPDU_Socki_event_dequeue(sock_set, &elem, &event) == SMPD_SUCCESS);

    /*
     * Free structures used by the sock set
     */
    MPIU_Free(sock_set->pollinfos);
    MPIU_Free(sock_set->pollfds);

    /*
     * Reset the sock set fields
     */
    sock_set->id = ~0;
    sock_set->poll_array_sz = 0;
    sock_set->poll_array_elems = 0;
    sock_set->starting_elem = 0;
    sock_set->pollfds = NULL;
    sock_set->pollinfos = NULL;
    sock_set->eventq_head = NULL;
    sock_set->eventq_tail = NULL;

    /*
     * Free the structure
     */
    MPIU_Free(sock_set);
    
#ifdef USE_SOCK_VERIFY
  fn_exit:
#endif
    smpd_exit_fn(FCNAME);
    return result;
}

/*
 * NOTE: branch delay slots may be filled in later by the optimizer.
 */

/* 
 * Initialize the lock object
 */
void MPIDUi_Process_lock_init(int * lock);
.inline MPIDUi_Process_lock_init, 4
    clrb [%o0]
.end


/*
 * Destroy the lock object
 */
void MPIDUi_Process_lock_free(int * lock);
.inline MPIDUi_Process_lock_free, 4
    set 0xaa, %o1
    stub %o1, [%o0]
.end


/*
 * Acquire the lock object
 */
void MPIDUi_Process_lock(int * lock);
.inline MPIDUi_Process_lock, 4
 1:
    /* first try to acquire the lock */
    ldstub [%o0], %o1
    tst %o1
    be 3f
    nop

 2:
    /* wait for the lock to become available; this avoids unnecessary stores,
       thereby reducing cache invalidations and extra bus traffic */
    ldub [%o0], %o1
    tst %o1
    be 1b
    nop
    ba 2b
    nop

 3:
    /* lock has been acquired; return */
.end


/*
 * Release the lock object
 */
void MPIDUi_Process_unlock(int * lock);
.inline MPIDUi_Process_unlock, 4
    /* release the lock, but only after insuring that all previous stores
       have completed */
    stbar
    clrb [%o0]
.end


/*
 * Wait for the process holding the lock to release it; this allows the lock
 * to be used as a synchronization object
 */
void MPIDUi_Process_busy_wait(int * lock);
.inline MPIDUi_Process_busy_wait, 4
 1:
    ldub [%o0], %o1
    tst %o1
    bne 1b
    nop
.end

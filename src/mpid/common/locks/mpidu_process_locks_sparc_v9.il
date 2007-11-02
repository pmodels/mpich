/*
 * NOTE: branch delay slots may be filled in later by the optimizer.
 */

/* 
 * Initialize the lock object
 */
void MPIDUi_Process_lock_init(int * lock);
.inline MPIDUi_Process_lock_init, 4
    clr [%o0]
.end


/*
 * Destroy the lock object
 */
void MPIDUi_Process_lock_free(int * lock);
.inline MPIDUi_Process_lock_free, 4
    set 0xbad1bad2, %o1
    stuw %o1, [%o0]
.end


/*
 * Acquire the lock object
 */
void MPIDUi_Process_lock(int * lock);
.inline MPIDUi_Process_lock, 4
 1:
    /* first try to acquire the lock */
    set 1, %o1
    cas [%o0], %g0, %o1
    tst %o1
    be 3f
    nop

 2:
    /* wait for the lock to become available; the cas operation must acquire
       exclusive access to the memory containing the lock in order perform the
       store atomically (if necessary); this code reduces continuous cache
       invalidations and extra bus traffic */
    lduw [%o0], %o1
    tst %o1
    be 1b
    nop
    ba 2b
    nop

 3:
    /* lock has been acquired; insure that no memory operations occuring
       programmatically after the the lock aquistion are allowed to proceed
       before this point; cas is treated as both a load and store, so it is not
       necessary to specify #StoreLoad and #StoreStore as well */
    membar #LoadLoad | #LoadStore
.end


/*
 * Release the lock object
 */
void MPIDUi_Process_unlock(int * lock);
.inline MPIDUi_Process_unlock, 4
    /* release the lock, but only after insuring that all previous memory
       operations have completed */
    membar #StoreStore | #LoadStore
    clr [%o0]
.end


/*
 * Wait for the process holding the lock to release it; this allows the lock
 * to be used as a synchronization object
 */
void MPIDUi_Process_busy_wait(int * lock);
.inline MPIDUi_Process_busy_wait, 4
 1:
    lduw [%o0], %o1
    tst %o1
    bne 1b
    nop
.end

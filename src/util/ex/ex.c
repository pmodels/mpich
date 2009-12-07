/* -*- Mode: C; c-basic-offset:4 ; -*-
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include <mpichconf.h>
#include <mpiutil.h>
#include <mpimem.h>
#include <mpi.h>
#include <mpiu_ex.h>
#ifdef USE_DBG_LOGGING
    #include <mpidbg.h>
#endif
#ifdef HAVE_WINNT_H
#include <winnt.h>
#endif

static
int
ExpKeyZeroCompletionProcessor(
    DWORD BytesTransfered,
    PVOID pOverlapped
    );


#if 0
//
// The only completion port, supporting all events
//
static HANDLE s_port;
#endif

//
// The registered processors. This module supports up to 4 processors where
// processor zero is pre-registered for overlapped operations completion.
// the completion processor is registered by the completion key.
//
static MPIU_ExCompletionProcessor s_processors[] = {

    ExpKeyZeroCompletionProcessor,
    NULL,
    NULL,
    NULL,
};


static inline BOOL IsValidSet(MPIU_ExSetHandle_t Set)
{
    return (Set != NULL && Set != INVALID_HANDLE_VALUE);
}


MPIU_ExSetHandle_t
MPIU_ExCreateSet(
    void
    )
{
    MPIU_ExSetHandle_t Set;
    Set = CreateIoCompletionPort(
            INVALID_HANDLE_VALUE,   // FileHandle
            NULL,   // ExistingCompletionPort
            0,      // CompletionKey
            0       // NumberOfConcurrentThreads
            );

    return Set;
}


void
MPIU_ExCloseSet(
    MPIU_ExSetHandle_t Set
    )
{
    MPIU_Assert(IsValidSet(Set));
    CloseHandle(Set);
}


void
MPIU_ExRegisterCompletionProcessor(
    ULONG_PTR Key,
    MPIU_ExCompletionProcessor pfnCompletionProcessor
    )
{
    MPIU_Assert(Key > 0);
    MPIU_Assert(Key < _countof(s_processors));
    MPIU_Assert(s_processors[Key] == NULL);
    s_processors[Key] = pfnCompletionProcessor;
}


void
MPIU_ExUnregisterCompletionProcessor(
    ULONG_PTR Key
    )
{
    MPIU_Assert(Key > 0);
    MPIU_Assert(Key < _countof(s_processors));
    MPIU_Assert(s_processors[Key] != NULL);
    s_processors[Key] = NULL;
}


void
MPIU_ExPostCompletion(
    MPIU_ExSetHandle_t Set,
    ULONG_PTR Key,
    PVOID pOverlapped,
    DWORD BytesTransfered
    )
{
    MPIU_Assert(IsValidSet(Set));

    for(;;)
    {
        if(PostQueuedCompletionStatus(Set, BytesTransfered, Key, pOverlapped))
            return;

        MPIU_Assert(GetLastError() == ERROR_NO_SYSTEM_RESOURCES);
        Sleep(10);
    }
}


ULONG
MPIU_ExGetPortValue(
    MPIU_ExSetHandle_t Set
    )
{
    MPIU_Assert(IsValidSet(Set));
    return HandleToUlong(Set);
}


int
MPIU_ExInitialize(
    void
    )
{
#if 0
    MPIU_Assert(s_port == NULL);
    s_port = CreateIoCompletionPort(
                INVALID_HANDLE_VALUE,   // FileHandle
                NULL,   // ExistingCompletionPort
                0,      // CompletionKey
                0       // NumberOfConcurrentThreads
                );

    if(s_port != NULL)
        return MPI_SUCCESS;

    return MPI_ERR_INTERN;
#endif

    return MPI_SUCCESS;
}


int
MPIU_ExFinalize(
    void
    )
{
#if 0
    MPIU_Assert(s_port != NULL);
    CloseHandle(s_port);
    s_port = NULL;
#endif
    return MPI_SUCCESS;
}


int
MPIU_ExProcessCompletions(
    MPIU_ExSetHandle_t Set,
    BOOL *fWaitForEventAndStatus
    )
{
    DWORD Timeout;

    MPIU_Assert(fWaitForEventAndStatus != NULL);
    MPIU_Assert(IsValidSet(Set));

    Timeout =  (*fWaitForEventAndStatus) ? INFINITE : 0;

    for (;;)
    {
        BOOL fSucc;
        DWORD BytesTransfered = 0;
        ULONG_PTR Key;
        OVERLAPPED* pOverlapped = NULL;

        fSucc = GetQueuedCompletionStatus(
                    Set,
                    &BytesTransfered,
                    &Key,
                    &pOverlapped,
                    Timeout
                    );

        if(!fSucc){
            /* Could be a timeout or an error */
            *fWaitForEventAndStatus = FALSE;
        }
        else{
            /* An Event completed */
            *fWaitForEventAndStatus = TRUE;
        }
        if(!fSucc && pOverlapped == NULL)
        {
            //
            // Return success on timeout per caller request. The Executive progress
            // engine will not wait for the async processing to complete
            //
            DWORD gle = GetLastError();
            if (gle == WAIT_TIMEOUT)
                return MPI_SUCCESS;

            /* FIXME: Should'nt there be a retry count ? */
            //
            // Io Completion port internal error, try again
            //
            continue;
        }
        
        MPIU_Assert(Key < _countof(s_processors));
        MPIU_Assert(s_processors[Key] != NULL);


        //
        // Call the completion processor and return the result.
        //
        return s_processors[Key](BytesTransfered, pOverlapped);
    }
}


//----------------------------------------------------------------------------
//
// Preregistered completion processor for Key-Zero
//

static
int
ExpKeyZeroCompletionProcessor(
    DWORD BytesTransfered,
    PVOID pOverlapped
    )
{
    MPIU_EXOVERLAPPED* pov = CONTAINING_RECORD(pOverlapped, MPIU_EXOVERLAPPED, ov);
    return MPIU_ExCompleteOverlapped(pov);
}


void
MPIU_ExPostOverlapped(
    MPIU_ExSetHandle_t Set,
    MPIU_EXOVERLAPPED* pOverlapped
    )
{
    MPIU_Assert(IsValidSet(Set));

    MPIU_ExPostCompletion(
        Set,
        0, // Key,
        &pOverlapped->ov,
        0 // BytesTransfered
        );
}


void
MPIU_ExAttachHandle(
    MPIU_ExSetHandle_t Set,
    HANDLE Handle
    )
{
    MPIU_Assert(IsValidSet(Set));

    for(;;)
    {
        HANDLE hPort;
        hPort = CreateIoCompletionPort(
                    Handle, // FileHandle
                    Set,    // ExistingCompletionPort
                    0,      // CompletionKey
                    0       // NumberOfConcurrentThreads
                    );

        if(hPort != NULL)
        {
            MPIU_Assert(hPort == Set);
            return;
        }

        MPIU_Assert(GetLastError() == ERROR_NO_SYSTEM_RESOURCES);
        Sleep(10);
    }
}


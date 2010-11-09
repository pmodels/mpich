/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef MPIU_EX_H_INCLUDED
#define MPIU_EX_H_INCLUDED

#define WIN32_LEAN_AND_MEAN
#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif

/*----------------------------------------------------------------------------
//
// The Executive, a generic progress engine
//
// Overview:
// The generic progress engine, the Executive, implements a simple asynchronous
// processing model. In this model records indicating events get queued up for
// processing. The Executive invokes the appropriate registered handler for the
// event to take further action if required. The executive exits when a sequence
// of events, representing a complete logical function is complete.
//
// This model is a producers-consumers model with a single events queue. The
// events are being processed in the order they have been queued.
// *** temporary extended to support multiple queues until the clients can ***
// *** use a single event queue. (e.g., PMI client implementation)         ***
//
// The function ExProcessCompletions is the heart of the consumers model. It
// dequeues the events records and invokes the appropriate completion processor.
// Multiple threads can call this function for processing. The Executive will
// run as many concurrent threads as there are processors in the system. When
// concurrent threads are running, the completion processors should be able to
// handle the concurrency and out-of-order execution.
//
// The Executive pre-registers a completion processor for Key-Zero with a simple
// handler signature. see description below.
//
//----------------------------------------------------------------------------*/


/*
    MPIU_ExSetHandle_t  *** temp extenssion ***
    Represents the set object
*/
typedef HANDLE MPIU_ExSetHandle_t;
#define MPIU_EX_INVALID_SET NULL

/*
    MPIU_ExCreateSet  *** temp extenssion ***

    Create the set object.
    An MPIU_EX_INVALID_SET value is returned for an out of memory condition.
*/
MPIU_ExSetHandle_t
MPIU_ExCreateSet(
    void
    );

/*
    MPIU_ExCloseSet *** temp extenssion ***

    Close the set object. 
*/
void
MPIU_ExCloseSet(
    MPIU_ExSetHandle_t Set
    );

/*
    MPIU_ExCompletionProcessor, function prototype

    The completion processor function is called by ExProcessCompletions function
    to process a completion event. The completion processor indicates whether the
    sequence of asynchronous events is complete or whether further processing is
    required.

    Parameters:
     BytesTransferred
        A DWORD value posted to the completion port. Usually the number of bytes
        transferred in this operation

     pOverlapped
        A pointer value posted to the completion port. Usually a pointer to the
        OVERLAPPED structure

    Return Value:
        MPI error value indicating the result of the "logical" async function.
        This value is meaningful only when the completion processor returns TRUE.
*/
typedef
int
(__cdecl * MPIU_ExCompletionProcessor)(
    DWORD BytesTransferred,
    PVOID pOverlapped
    );


/*
    MPIU_ExRegisterCompletionProcessor

    Register a completion processor for a specific Key.
    N.B. Current implementation supports keys 0 - 3, where key 0/1 is reserved.
*/
void
MPIU_ExRegisterCompletionProcessor(
    ULONG_PTR Key,
    MPIU_ExCompletionProcessor pfnCompletionProcessor
    );

/* Predefined Completion processor keys */
#define MPIU_EX_GENERIC_COMP_PROC_KEY   0x0
#define MPIU_EX_WIN32_COMP_PROC_KEY     0x1

/*
    MPIU_ExRegisterNextCompletionProcessor

    Register the next completion processor and return the Key.
    Returns an MPI error code on error
    N.B. Current implementation supports keys 0 - 3, where key 0 is reserved.
*/
int
MPIU_ExRegisterNextCompletionProcessor(
    ULONG_PTR *Key,
    MPIU_ExCompletionProcessor pfnCompletionProcessor
    );

/*
    MPIU_ExUnregisterCompletionProcessor

    Remove a registered completion processor
*/
void
MPIU_ExUnregisterCompletionProcessor(
    ULONG_PTR Key
    );


/*
    MPIU_ExPostCompletion

    Post an event completion to the completion queue. The appropriate
    completion processor will be invoked by ExProcessCompletions thread
    with the passed in parameters.
*/
void
MPIU_ExPostCompletion(
    MPIU_ExSetHandle_t Set,
    ULONG_PTR Key,
    PVOID pOverlapped,
    DWORD BytesTransferred
    );


/*
    MPIU_ExGetPortValue

    Returns the value of the completion queue handle
*/
ULONG
MPIU_ExGetPortValue(
    MPIU_ExSetHandle_t Set
    );

/*
    MPIU_ExProcessCompletions

    Process all completion event types by invoking the appropriate completion
    processor function. This routine continues to process all events until an
    asynchronous sequence is complete (function with several async stages).
    If the caller indicated "no blocking" this routine will exit when no more
    events to process are available, regardles of the completion processor
    indication to continue.

    Parameters:
        fWaitForEventAndStatus -
            If Set to TRUE, Block until an event sequence is complete
            If Set to FALSE, Does not block for an event seq to complete
            On return, if set to TRUE, An event seq was completed
            On return, if set to FALSE, An event seq did not complete

    Return Value:
        The result of the asynchronous function last to complete.
        if fWaitForEventAndStatus is FALSE and no more events to process, the value
        returned is MPI_SUCCESS.
*/
int
MPIU_ExProcessCompletions(
    MPIU_ExSetHandle_t Set,
    BOOL *fWaitForEventAndStatus
    );

/*
    MPIU_ExInitialize

    Initialize the completion queue. This function can only be called once before
    before Finialize.
*/
int
MPIU_ExInitialize(
    void
    );


/*
    MPIU_ExFinitialize

    Close the completion queue progress engine. This function can only be called
    once.
*/
int
MPIU_ExFinalize(
    void
    );


/*----------------------------------------------------------------------------
//
// The Executive Key-Zero completion processor
//
// Overview:
// The Key-Zero completion processor enables the users of the Executive to
// associate a different completion routine with each operation rather than
// use a single completion processor per handle. The Key-Zero processor works
// with user supplied OVERLAPPED structure. It cannot work with system generated
// completion events (e.g., Job Object enets), or other external events.
//
// The Key-Zero processor uses the EXOVERLAPPED data structure which embeds the
// user success and failure completion routines. When the Key-Zero completion
// processor is invoked it calls the user success or failure routine base on
// the result of the async operation.
//
//----------------------------------------------------------------------------

//
// MPIU_ExCompletionRoutine function prototype
//
// The ExCompletionRoutine callback routine is invoked by the built-in Key-Zero
// completion processor. The information required for processing the event is
// packed with the EXOVERLAPED structure and can be accessed with the Ex utility
// functions. The callback routine returns the logical error value.
//
// Parameters:
//   pOverlapped
//      A pointer to an EXOVERLAPPED structure associated with the completed
//      operation. This pointer is used to extract the caller context with the
//      CONTAINING_RECORD macro.
//
// Return Value:
//      MPI error value indicating the result of the "logical" async function.
*/
struct MPIU_EXOVERLAPPED;

typedef
int
(* MPIU_ExCompletionRoutine)(
    struct MPIU_EXOVERLAPPED* pOverlapped
    );


/*
    struct MPIU_EXOVERLAPPED

    The data structure used for Key-Zero completions processing. The pfnSuccess
    and pfnFailure are set by the caller before calling an async operation.
    The pfnSuccess is called if the async operation was successful.
    The pfnFailure is called if the async operation was unsuccessful.
*/
typedef struct MPIU_EXOVERLAPPED {

    OVERLAPPED ov;
    MPIU_ExCompletionRoutine pfnSuccess;
    MPIU_ExCompletionRoutine pfnFailure;
    /*  FIXME: Should we allow a user_ctxt here ?
        This will allow us to implement finding the record containing executive
        overlapped structure in systems/compilers that don't implement mechanisms
        similar to CONTAINING_RECORD.
     */
    /* void *user_ctxt; */

} MPIU_EXOVERLAPPED;

/* Get pointer to OS overlapped from EX overlapped pointer*/
#define MPIU_EX_GET_OVERLAPPED_PTR(ex_ovp) (&((ex_ovp)->ov))

/*
    MPIU_ExInitOverlapped

    Initialize the success & failure callback function fields
    Rest the hEvent field of the OVERLAPPED, make it ready for use with the OS
    overlapped API's.
*/

static
inline
void
MPIU_ExInitOverlapped(
    MPIU_EXOVERLAPPED* pOverlapped,
    MPIU_ExCompletionRoutine pfnSuccess,
    MPIU_ExCompletionRoutine pfnFailure
    )
{
    pOverlapped->ov.hEvent = NULL;
    pOverlapped->pfnSuccess = pfnSuccess;
    pOverlapped->pfnFailure = pfnFailure;
}

/*
    MPIU_ExReInitOverlapped ==> ONLY REQD for manual events

    Re-Initialize the success & failure callback function fields

    Re-init the event
*/

static
inline
BOOL
MPIU_ExReInitOverlapped(
    MPIU_EXOVERLAPPED* pOverlapped,
    MPIU_ExCompletionRoutine pfnSuccess,
    MPIU_ExCompletionRoutine pfnFailure
    )
{
    /* Re-init succ/fail handlers only if they are ~null */
    if(pfnSuccess){
        pOverlapped->pfnSuccess = pfnSuccess;
    }
    if(pfnFailure){
        pOverlapped->pfnFailure = pfnFailure;
    }

    if(pOverlapped->ov.hEvent)
        return(ResetEvent(pOverlapped->ov.hEvent));
    else
        return TRUE;
}

/*
    MPIU_ExPostOverlapped

    Post an EXOVERLAPPED completion to the completion queue to be invoked by
    ExProcessCompletions.
*/
void
MPIU_ExPostOverlapped(
    MPIU_ExSetHandle_t Set,
    ULONG_PTR key,
    MPIU_EXOVERLAPPED* pOverlapped
    );


/*
    MPIU_ExPostOverlappedResult

    Post an EXOVERLAPPED completion to the completion queue to be invoked by
    ExProcessCompletions. Set the status and bytes transferred count.
*/
static
inline
void
MPIU_ExPostOverlappedResult(
    MPIU_ExSetHandle_t Set,
    ULONG_PTR key,
    MPIU_EXOVERLAPPED* pOverlapped,
    HRESULT Status,
    DWORD BytesTransferred
    )
{
    pOverlapped->ov.Internal = Status;
    pOverlapped->ov.InternalHigh = BytesTransferred;
    MPIU_ExPostOverlapped(Set, key, pOverlapped);
}


/*
    MPIU_ExAttachHandle

    Associate an OS handle with the Executive completion queue. All asynchronous
    operations using the attached handle are processed with the Key-Zero
    completion processor.
    Use an EXOVERLAPPED data structure when calling an asynchronous operation
    with that Handle.
*/
void
MPIU_ExAttachHandle(
    MPIU_ExSetHandle_t Set,
    ULONG_PTR key,
    HANDLE Handle
    );


/*
    MPIU_ExGetBytesTransferred

    Get the number of bytes transferred from the overlapped structure
*/
static
inline
DWORD
MPIU_ExGetBytesTransferred(
    MPIU_EXOVERLAPPED* pOverlapped
    )
{
    return (DWORD)pOverlapped->ov.InternalHigh;
}

#define MPIU_EX_STATUS_IO_ABORT  (0x80000000 | (FACILITY_WIN32 << 16) | ERROR_OPERATION_ABORTED)
#define MPIU_EX_STATUS_TO_ERRNO(ex_status) HRESULT_CODE(~0x80000000 & ex_status)
typedef HRESULT MPIU_Ex_status_t;

/*
    MPIU_ExGetStatus

    Get the status return value from the overlapped structure
*/
static
inline
HRESULT
MPIU_ExGetStatus(
    MPIU_EXOVERLAPPED* pOverlapped
    )
{
    return (HRESULT)pOverlapped->ov.Internal;
}


/*
    MPIU_ExCallSuccess

    Set the completion status and bytes transferred and execute the EXOVERLAPPED
    Success completion routine
*/
static
inline
int
MPIU_ExCallSuccess(
    MPIU_EXOVERLAPPED* pOverlapped,
    HRESULT Status,
    DWORD BytesTransferred
    )
{
    pOverlapped->ov.Internal = Status;
    pOverlapped->ov.InternalHigh = BytesTransferred;
    return pOverlapped->pfnSuccess(pOverlapped);
}


/*
    MPIU_ExCallFailure

    Set the completion status and bytes transferred and execute the EXOVERLAPPED
    Failure completion routine
*/
static
inline
int
MPIU_ExCallFailure(
    MPIU_EXOVERLAPPED* pOverlapped,
    HRESULT Status,
    DWORD BytesTransferred
    )
{
    pOverlapped->ov.Internal = Status;
    pOverlapped->ov.InternalHigh = BytesTransferred;
    return pOverlapped->pfnFailure(pOverlapped);
}

/*
    MPIU_ExCompleteOverlapped

    Execute the EXOVERLAPPED success or failure completion routine based
    on the overlapped status value.
*/
static
inline
int
MPIU_ExCompleteOverlapped(
    MPIU_EXOVERLAPPED* pOverlapped, int BytesTransferred
    )
{
    pOverlapped->ov.InternalHigh = BytesTransferred;

    if(SUCCEEDED(MPIU_ExGetStatus(pOverlapped))){
        return pOverlapped->pfnSuccess(pOverlapped);
    }
    else{
        return pOverlapped->pfnFailure(pOverlapped);
    }
}

/*
    MPIU_ExWin32CompleteOverlapped

    Execute the EXOVERLAPPED success or failure completion routine based
    on the overlapped status value.
*/
static
inline
int
MPIU_ExWin32CompleteOverlapped(
    MPIU_EXOVERLAPPED* pOverlapped, int BytesTransferred
    )
{
    pOverlapped->ov.InternalHigh = BytesTransferred;

    if(SUCCEEDED(MPIU_ExGetStatus(pOverlapped))){
        pOverlapped->ov.Internal = S_OK;
        return pOverlapped->pfnSuccess(pOverlapped);
    }
    else{
        pOverlapped->ov.Internal = HRESULT_FROM_WIN32(GetLastError());
        return pOverlapped->pfnFailure(pOverlapped);
    }
}

#endif /* MPIU_EX_H_INCLUDED */

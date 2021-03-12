/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <mpiimpl.h>
#include "mpi_init.h"

#ifdef HAVE_WINDOWS_H
/* User-defined abort hook function.  Exiting here will prevent the system from
 * bringing up an error dialog box.
 */
/* style: allow:fprintf:1 sig:0 */
static int assert_hook(int reportType, char *message, int *returnValue)
{
    MPL_UNREFERENCED_ARG(reportType);
    fprintf(stderr, "%s", message);
    if (returnValue != NULL)
        ExitProcess((UINT) (*returnValue));
    ExitProcess((UINT) (-1));
    return TRUE;
}

/* MPICH dll entry point */
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
    BOOL result = TRUE;
    hinstDLL;
    lpReserved;

    switch (fdwReason) {
        case DLL_PROCESS_ATTACH:
            break;

        case DLL_THREAD_ATTACH:
            /* allocate thread specific data */
            break;

        case DLL_THREAD_DETACH:
            /* free thread specific data */
            break;

        case DLL_PROCESS_DETACH:
            break;
    }
    return result;
}

void MPII_init_windows(void)
{

    /* FIXME: Move to os-dependent interface? */
#ifdef HAVE_WINDOWS_H
    /* prevent the process from bringing up an error message window if mpich
     * asserts */
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
    _CrtSetReportHook2(_CRT_RPTHOOK_INSTALL, assert_hook);
#ifdef _WIN64
    {
        /* FIXME: (Windows) This severely degrades performance but fixes alignment
         * issues with the datatype code. */
        /* Prevent misaligned faults on Win64 machines */
        UINT mode, old_mode;

        old_mode = SetErrorMode(SEM_NOALIGNMENTFAULTEXCEPT);
        mode = old_mode | SEM_NOALIGNMENTFAULTEXCEPT;
        SetErrorMode(mode);
    }
#endif
#endif
}

#else
/* no HAVE_WINDOWS_H, empty stubs */
void MPII_init_windows(void)
{
}
#endif

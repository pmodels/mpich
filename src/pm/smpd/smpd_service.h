/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef SMPD_SERVICE_H
#define SMPD_SERVICE_H

#include "smpd.h"

/* name of the service */
#define SMPD_SERVICE_NAME         "mpich2_smpd"
#define SMPD_SERVICE_NAMEW       L"mpich2_smpd"
/* displayed name of the service */
#define SMPD_SERVICE_DISPLAY_NAME   "MPICH2 Process Manager, Argonne National Lab"
#define SMPD_SERVICE_DISPLAY_NAMEW L"MPICH2 Process Manager, Argonne National Lab"
/* guid to represent the service */
#define SMPD_SERVICE_GUID   "5722fe5f-cf46-4594-af7c-0997ca2e9d72"
#define SMPD_SERVICE_GUIDW L"5722fe5f-cf46-4594-af7c-0997ca2e9d72"
/* guid to represent the service vendor */
#define SMPD_SERVICE_VENDOR_GUID   "c08206aa-590e-41f4-b8ce-ddffe8132b9a"
#define SMPD_SERVICE_VENDOR_GUIDW L"c08206aa-590e-41f4-b8ce-ddffe8132b9a"
/* vendor */
#define SMPD_PRODUCT_VENDOR   "Argonne National Lab"
#define SMPD_PRODUCT_VENDORW L"Argonne National Lab"
/* product */
#define SMPD_PRODUCT   "smpd"
#define SMPD_PRODUCTW L"smpd"
/* registry key */
#define SMPD_SERVICE_REGISTRY_KEY "Software\\MPICH\\SMPD"

void smpd_install_service(SMPD_BOOL interact, SMPD_BOOL bSetupRestart, SMPD_BOOL bSetupScp);
SMPD_BOOL smpd_remove_service(SMPD_BOOL bErrorOnNotInstalled);
void smpd_stop_service();
void smpd_start_service();
void smpd_service_main(int argc, char *argv[]);
void smpd_service_stop();
void smpd_add_error_to_message_log(char *msg);
SMPD_BOOL smpd_report_status_to_sc_mgr(DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint);

#endif

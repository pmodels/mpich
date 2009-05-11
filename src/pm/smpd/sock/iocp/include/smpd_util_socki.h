/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef SMPD_UTIL_SOCKI_H_INCLUDED
#define SMPD_UTIL_SOCKI_H_INCLUDED

#include "smpdconf.h"
#include <winsock2.h>
#include <windows.h>
#include <mswsock.h>
/* 
	ws2tcpip.h is a C++ header file. This header file,
	mpidu_socki.h, if included within a extern "C" {}
	block would give an error if ws2tcpip is not
	included in a extern "C++" {} block.
   
 */
#ifdef __cplusplus
extern "C++" {
#endif
    #include <ws2tcpip.h>
#ifdef __cplusplus
}
#endif
#include "mpimem.h"

#define SMPDU_SOCK_INVALID_SOCK   NULL
#define SMPDU_SOCK_INVALID_SET    NULL
#define SMPDU_SOCK_INFINITE_TIME   INFINITE
#define inline __inline

typedef HANDLE SMPDU_SOCK_NATIVE_FD;
typedef HANDLE SMPDU_Sock_set_t;
typedef struct sock_state_t * SMPDU_Sock_t;
typedef DWORD SMPDU_Sock_size_t;
#endif

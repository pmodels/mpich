/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2010 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef HYDT_FTB_H_INCLUDED
#define HYDT_FTB_H_INCLUDED

#if defined ENABLE_FTB
#include <libftb.h>
#define HYDT_FTB_MAX_PAYLOAD_DATA FTB_MAX_PAYLOAD_DATA
#else
#define HYDT_FTB_MAX_PAYLOAD_DATA (1024)
#endif /* ENABLE_FTB */

HYD_status HYDT_ftb_init(void);
HYD_status HYDT_ftb_finalize(void);
HYD_status HYDT_ftb_publish(const char *event_name, const char *event_payload);

#endif /* HYDT_FTB_H_INCLUDED */

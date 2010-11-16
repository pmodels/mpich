/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef HYDT_FTB_H_INCLUDED
#define HYDT_FTB_H_INCLUDED

#if defined ENABLE_FTB
#include <libftb.h>

#define HYDT_FTB_MAX_PAYLOAD_DATA FTB_MAX_PAYLOAD_DATA

HYD_status HYDT_ftb_init(void);
HYD_status HYDT_ftb_finalize(void);
HYD_status HYDT_ftb_publish(const char *event_name, const char *event_payload);

#else

#define HYDT_FTB_MAX_PAYLOAD_DATA (1024)

static HYD_status HYDT_ftb_init(void)
{
    return HYD_SUCCESS;
}

static HYD_status HYDT_ftb_finalize(void)
{
    return HYD_SUCCESS;
}

static HYD_status HYDT_ftb_publish(const char *event_name, const char *event_payload)
{
    return HYD_SUCCESS;
}

#endif /* ENABLE_FTB */

#endif /* HYDT_FTB_H_INCLUDED */

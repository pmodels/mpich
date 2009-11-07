/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPICH2_FTB_H_INCLUDED
#define MPICH2_FTB_H_INCLUDED

/* FTB events we can throw */
#define MPIDU_FTB_EV_OTHER         "OTHER"
#define MPIDU_FTB_EV_RESOURCES     "RESOURCES"
#define MPIDU_FTB_EV_UNREACHABLE   "UNREACHABLE"
#define MPIDU_FTB_EV_COMMUNICATION "COMUNICATION"
#define MPIDU_FTB_EV_ABORT         "ABORT"

#ifdef ENABLE_FTB
/* prototypes */
int MPIDU_Ftb_init(void);
void MPIDU_Ftb_publish(const char *event_name, const char *event_payload);
void MPIDU_Ftb_finalize(void);
#else /* ENABLE_FTB */
#define MPIDU_Ftb_init() (MPI_SUCCESS)
#define MPIDU_Ftb_publish(event_name, event_payload) do {} while(0)
#define MPIDU_Ftb_finalize() do {} while(0)
#endif /* ENABLE_FTB */

#endif /* MPICH2_FTB_H_INCLUDED */

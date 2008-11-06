/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
/* Allow the printf's that describe the contents of the file.  Error 
   messages use the printf routines */
/* style: allow:printf:9 sig:0 */

#include "rlog.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void PrintRecord(IRLOG_IOStruct *pInput)
{
    switch (pInput->header.type)
    {
    case RLOG_EVENT_TYPE:
	printf("RLOG_EVENT  -");
	printf(" %3d:%04d, start: %10g, end: %10g, %d\n", 
	    pInput->record.event.rank,
	    pInput->record.event.event, 
	    pInput->record.event.start_time, 
	    pInput->record.event.end_time,
	    pInput->record.event.recursion);
	break;
    case RLOG_IARROW_TYPE:
	printf("RLOG_IARROW -");
	printf(" %3d %s %3d, tag: %3d, length: %d\n", 
	    pInput->record.iarrow.rank,
	    (pInput->record.iarrow.sendrecv == RLOG_SENDER) ? "->" : "<-",
	    pInput->record.iarrow.remote,
	    pInput->record.iarrow.tag,
	    pInput->record.iarrow.length);
	break;
    case RLOG_COMM_TYPE:
	printf("RLOG_COMM   -");
	printf(" %3d, newcomm: %d\n",
	    pInput->record.comm.rank,
	    pInput->record.comm.newcomm);
	break;
    case RLOG_STATE_TYPE:
	printf("RLOG_STATE  -");
	printf(" id: %3d, %12s : %s\n",
	    pInput->record.state.event,
	    pInput->record.state.color,
	    pInput->record.state.description);
	break;
    case RLOG_ENDLOG_TYPE:
	printf("RLOG_ENDLOG\n");
	break;
    default:
	printf("unknown record type %d\n", pInput->header.type);
    }
}

int main(int argc, char *argv[])
{
    int nNumInputs = 0;
    IRLOG_IOStruct *pInput = NULL;
    int rec_type;
    int bAllRecords = 1;
    int bExcludeType = 0;

    if (argc < 2)
    {
	printf("%s irlogfile [REC_TYPE]\n", argv[0]);
	return -1;
    }

    pInput = IRLOG_CreateInputStruct(argv[1]);
    if (pInput == NULL)
    {
	printf("Error setting up file '%s'\n", argv[1]);
	return -1;
    }

    if (argc > 2)
    {
	if (strcmp(argv[2], "RLOG_EVENT") == 0)
	{
	    bAllRecords = 0;
	    rec_type = RLOG_EVENT_TYPE;
	}
	if (strcmp(argv[2], "RLOG_ARROW") == 0)
	{
	    bAllRecords = 0;
	    rec_type = RLOG_ARROW_TYPE;
	}
	if (strcmp(argv[2], "RLOG_STATE") == 0)
	{
	    bAllRecords = 0;
	    rec_type = RLOG_STATE_TYPE;
	}
	if (strcmp(argv[2], "RLOG_COMM") == 0)
	{
	    bAllRecords = 0;
	    rec_type = RLOG_COMM_TYPE;
	}
	if (strcmp(argv[2], "RLOG_ENDLOG") == 0)
	{
	    bAllRecords = 0;
	    rec_type = RLOG_ENDLOG_TYPE;
	}
    }
    if (argc > 3)
    {
	if (strcmp(&argv[3][1], "exclude") == 0)
	    bExcludeType = 1;
    }

    while (1)
    {
	if (bExcludeType)
	{
	    if (pInput->header.type != rec_type)
		PrintRecord(pInput);
	}
	else
	{
	    if ((bAllRecords) || (pInput->header.type == rec_type))
		PrintRecord(pInput);
	}
	if (IRLOG_GetNextRecord(pInput))
	{
	    IRLOG_CloseInputStruct(&pInput);
	    break;
	}
    }

    return 0;
}

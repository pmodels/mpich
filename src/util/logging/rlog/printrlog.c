/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
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

#define HEADER_BIT   0x01
#define STATE_BIT    0x02
#define COMM_BIT     0x04
#define ARROW_BIT    0x08
#define EVENT_BIT    0x10

void PrintState(RLOG_STATE *pState)
{
    printf("RLOG_STATE -");
    printf(" id: %3d, %12s : %s\n",
	pState->event,
	pState->color,
	pState->description);
}

void PrintEvent(RLOG_EVENT *pEvent)
{
    printf("RLOG_EVENT -");
    printf(" %3d:%04d, start: %4.07g, end: %4.07g, %d\n", 
	pEvent->rank,
	pEvent->event, 
	pEvent->start_time, 
	pEvent->end_time,
	pEvent->recursion);
}

void PrintEventAndIndex(RLOG_EVENT *pEvent, int index)
{
    printf("RLOG_EVENT -");
    printf(" %3d:%04d, start: % 10f, end: % 10f, %d, #%d\n", 
	pEvent->rank,
	pEvent->event, 
	pEvent->start_time, 
	pEvent->end_time,
	pEvent->recursion,
	index);
}

void PrintArrow(RLOG_ARROW *pArrow)
{
    printf("RLOG_ARROW -");
    printf(" %2d -> %2d, %s tag:%3d, len: %d, start: %g, end: %g\n", 
	pArrow->src,
	pArrow->dest,
	(pArrow->leftright == RLOG_ARROW_RIGHT) ? "RIGHT" : "LEFT ",
	pArrow->tag,
	pArrow->length,
	pArrow->start_time,
	pArrow->end_time);
}

void PrintComm(RLOG_COMM *pComm)
{
    printf("RLOG_COMM  -");
    printf(" %3d, newcomm: %d\n",
	pComm->rank,
	pComm->newcomm);
}
/* FIXME: Add a structured comment for the man page generate to
 * create the basic documentation on this routine, particularly 
 * since this routine only prints a subset of information by default */
int main(int argc, char *argv[])
{
    RLOG_IOStruct *pInput;
    RLOG_FILE_HEADER header;
    int num_levels;
    int num_states;
    int num_arrows;
    int num_events;
    int total_num_events = 0;
    int i, j, k;
    unsigned int nMask = 0;
    int bSummary = 1;
    RLOG_STATE state;
    RLOG_EVENT event, lastevent;
    RLOG_ARROW arrow, lastarrow;
    int bFindEvent = 0;
    double dFindTime = 0.0;
    int bValidate = 0;
    int bValidateArrows = 0;
    int bOrder = 0;
    int bJumpCheck = 0;
    double dJump = 0.0;

    /* FIXME: This should also check for the GNU-standard --help, --usage,
     * and -h options.  */
    if (argc < 2)
    {
	/* FIXME: What is the default behavior with just an rlogfile? */
	printf("printrlog rlogfile [EVENTS | STATES | ARROWS | HEADER | COMM | ALL | SUMMARY ]\n");
	printf("printrlog rlogfile find endtime\n");
	printf("printrlog rlogfile validate\n");
	printf("printrlog rlogfile order\n");
	printf("printrlog rlogfile arroworder\n");
	return -1;
    }

    if (argc > 2)
    {
	nMask = 0;
	bSummary = 0;
	for (i=2; i<argc; i++)
	{
	    if (strcmp(argv[i], "EVENTS") == 0)
		nMask |= EVENT_BIT;
	    if (strcmp(argv[i], "STATES") == 0)
		nMask |= STATE_BIT;
	    if (strcmp(argv[i], "ARROWS") == 0)
		nMask |= ARROW_BIT;
	    if (strcmp(argv[i], "HEADER") == 0)
		nMask |= HEADER_BIT;
	    if (strcmp(argv[i], "COMM") == 0)
		nMask |= COMM_BIT;
	    if (strcmp(argv[i], "ALL") == 0)
		nMask = HEADER_BIT | STATE_BIT | COMM_BIT | ARROW_BIT | EVENT_BIT;
	    if (strcmp(argv[i], "SUMMARY") == 0)
	    {
		bSummary = 1;
		nMask = 0;
	    }
	    if (strcmp(argv[i], "find") == 0)
	    {
		bFindEvent = 1;
		dFindTime = atof(argv[i+1]);
	    }
	    if (strcmp(argv[i], "validate") == 0)
	    {
		bValidate = 1;
	    }
	    if (strcmp(argv[i], "order") == 0)
	    {
		bOrder = 1;
	    }
	    if (strcmp(argv[i], "arroworder") == 0)
	    {
		bValidateArrows = 1;
	    }
	    if (strcmp(argv[i], "jump") == 0)
	    {
		bJumpCheck = 1;
		if (argv[i+1])
		    dJump = atof(argv[i+1]);
	    }
	}
    }

    pInput = RLOG_CreateInputStruct(argv[1]);
    if (pInput == NULL)
    {
	printf("Error opening '%s'\n", argv[1]);
	return -1;
    }

    if (bValidateArrows)
    {
	num_arrows = RLOG_GetNumArrows(pInput);
	if (num_arrows)
	{
	    printf("num arrows: %d\n", num_arrows);
	    RLOG_GetNextArrow(pInput, &lastarrow);
	    if (lastarrow.start_time > lastarrow.end_time)
		printf("start > end: %g > %g\n", lastarrow.start_time, lastarrow.end_time);
	    while (RLOG_GetNextArrow(pInput, &arrow) == 0)
	    {
		if (arrow.start_time > arrow.end_time)
		    printf("start > end: %g > %g\n", arrow.start_time, arrow.end_time);
		if (arrow.end_time < lastarrow.end_time)
		    printf("arrows out of order: %d < %d\n", arrow.end_time, lastarrow.end_time);
		lastarrow = arrow;
	    }
	}
	RLOG_CloseInputStruct(&pInput);
	return 0;
    }

    if (bValidate)
    {
	num_arrows = RLOG_GetNumArrows(pInput);
	if (num_arrows)
	{
	    printf("num arrows: %d\n", num_arrows);
	    RLOG_GetNextArrow(pInput, &lastarrow);
	    if (lastarrow.start_time > lastarrow.end_time)
	    {
		printf("Error, arrows endtime before starttime: %g < %g\n", lastarrow.end_time, lastarrow.start_time);
		PrintArrow(&arrow);
	    }
	    while (RLOG_GetNextArrow(pInput, &arrow) == 0)
	    {
		if (lastarrow.end_time > arrow.end_time)
		{
		    printf("Error, arrows out of order: %g > %g\n", lastarrow.end_time, arrow.end_time);
		    PrintArrow(&lastarrow);
		    PrintArrow(&arrow);
		}
		if (arrow.start_time > arrow.end_time)
		{
		    printf("Error, arrows endtime before starttime: %g < %g\n", arrow.end_time, arrow.start_time);
		    PrintArrow(&arrow);
		}
		lastarrow = arrow;
	    }
	}
	for (j=pInput->header.nMinRank; j<=pInput->header.nMaxRank; j++)
	{
	    num_levels = RLOG_GetNumEventRecursions(pInput, j);
	    for (i=0; i<num_levels; i++)
	    {
		printf("Validating events in level %d:%d\n", j, i);
		if (RLOG_GetNextEvent(pInput, j, i, &event) == 0)
		{
		    if (event.end_time < event.start_time)
		    {
			printf("Error, event endtime before starttime: %g < %g\n", event.end_time, event.start_time);
		    }
		    lastevent = event;
		    while (RLOG_GetNextEvent(pInput, j, i, &event) == 0)
		    {
			if (lastevent.start_time > event.start_time)
			{
			    printf("Error, events out of order: %g > %g\n", lastevent.start_time, event.start_time);
			    PrintEvent(&lastevent);
			    PrintEvent(&event);
			}
			else if (lastevent.end_time > event.start_time)
			{
			    printf("Error, starttime before previous endtime: %g > %g\n", lastevent.end_time, event.start_time);
			    PrintEvent(&lastevent);
			    PrintEvent(&event);
			}
			if (event.end_time < event.start_time)
			{
			    printf("Error, event endtime before starttime: %g < %g\n", event.end_time, event.start_time);
			    PrintEvent(&event);
			}
			lastevent = event;
		    }
		}
	    }
	}
	RLOG_CloseInputStruct(&pInput);
	return 0;
    }

    if (bOrder)
    {
	int count = 0;
	if (RLOG_GetNextGlobalEvent(pInput, &event) != 0)
	{
	    RLOG_CloseInputStruct(&pInput);
	    return 0;
	}
	count++;
	lastevent = event;
	PrintEvent(&event);
	while (RLOG_GetNextGlobalEvent(pInput, &event) == 0)
	{
	    if (lastevent.start_time > event.start_time)
	    {
		printf("Error, events out of order: %g > %g\n", lastevent.start_time, event.start_time);
		PrintEvent(&lastevent);
		PrintEvent(&event);
	    }
	    if (event.end_time < event.start_time)
	    {
		printf("Error, event endtime before starttime: %g < %g\n", event.end_time, event.start_time);
		PrintEvent(&event);
	    }
	    lastevent = event;
	    PrintEvent(&event);
	    count++;
	}
	RLOG_CloseInputStruct(&pInput);
	printf("%d events traversed\n", count);
	return 0;
    }

    if (bFindEvent)
    {
	for (j=pInput->header.nMinRank; j<=pInput->header.nMaxRank; j++)
	{
	    printf("rank %d\n", j);
	    num_levels = RLOG_GetNumEventRecursions(pInput, j);
	    for (i=0; i<num_levels; i++)
	    {
		RLOG_FindEventBeforeTimestamp(pInput, j, i, dFindTime, &event, &k);
		PrintEventAndIndex(&event, k);
	    }
	}
	RLOG_CloseInputStruct(&pInput);
	return 0;
    }

    if (bJumpCheck)
    {
	printf("Start:\n");
	RLOG_FindGlobalEventBeforeTimestamp(pInput, dJump, &event);
	PrintEvent(&event);
	/*RLOG_PrintGlobalState(pInput);*/
	printf("Previous 10:\n");
	for (i=0; i<10; i++)
	{
	    RLOG_GetPreviousGlobalEvent(pInput, &event);
	    PrintEvent(&event);
	}
	printf("Start:\n");
	RLOG_FindGlobalEventBeforeTimestamp(pInput, dJump, &event);
	PrintEvent(&event);
	printf("Next 10:\n");
	for (i=0; i<10; i++)
	{
	    RLOG_GetNextGlobalEvent(pInput, &event);
	    PrintEvent(&event);
	}
	return 0;
    }

    if (RLOG_GetFileHeader(pInput, &header))
    {
	printf("unable to read the file header\n");
	RLOG_CloseInputStruct(&pInput);
	return -1;
    }
    if (nMask & HEADER_BIT || bSummary)
    {
	printf("min rank: %d\n", header.nMinRank);
	printf("max rank: %d\n", header.nMaxRank);
    }

    if (nMask & STATE_BIT || bSummary)
    {
	num_states = RLOG_GetNumStates(pInput);
	if (num_states)
	{
	    printf("num states: %d\n", num_states);
	    if (nMask & STATE_BIT)
	    {
		for (i=0; i<num_states; i++)
		{
		    RLOG_GetNextState(pInput, &state);
		    PrintState(&state);
		}
	    }
	}
    }

    if (nMask & ARROW_BIT || bSummary)
    {
	num_arrows = RLOG_GetNumArrows(pInput);
	if (num_arrows)
	{
	    printf("num arrows: %d\n", num_arrows);
	    if (nMask & ARROW_BIT)
	    {
		for (i=0; i<num_arrows; i++)
		{
		    RLOG_GetNextArrow(pInput, &arrow);
		    PrintArrow(&arrow);
		}
	    }
	}
    }

    if (nMask & EVENT_BIT || bSummary)
    {
	for (k=pInput->header.nMinRank; k<=pInput->header.nMaxRank; k++)
	{
	    total_num_events = 0;
	    num_levels = RLOG_GetNumEventRecursions(pInput, k);
	    if (num_levels > 0)
	    {
		printf("rank %d\n", k);
		printf("num event recursions: %d\n", num_levels);
		for (i=0; i<num_levels; i++)
		{
		    num_events = RLOG_GetNumEvents(pInput, k, i);
		    total_num_events += num_events;
		    printf(" level %d, num events: %d\n", i, num_events);
		    if (nMask & EVENT_BIT)
		    {
			for (j=0; j<num_events; j++)
			{
			    RLOG_GetNextEvent(pInput, k, i, &event);
			    PrintEvent(&event);
			}
		    }
		}
		printf("num events total: %d\n", total_num_events);
	    }
	}
    }

    RLOG_CloseInputStruct(&pInput);

    return 0;
}

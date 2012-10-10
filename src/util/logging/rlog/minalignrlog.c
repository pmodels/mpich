/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
/* Allow the printf's that describe the contents of the file.  Error 
   messages use the printf routines */
/* style: allow:printf:12 sig:0 */

#include "rlog.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "mpimem.h"

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

int main(int argc, char *argv[])
{
    RLOG_IOStruct *pInput;
    int num_arrows;
    int total_num_events = 0;
    int i, j, range;
    int bSummary = 1;
    RLOG_ARROW arrow, lastarrow;
    int bInvalidArrowFound = 0;
    double *pOffset = NULL;
    int nNumStates = 0;
    int nNumLevels = 0;
    int nTotalNumEvents = 0;

    if (argc < 2)
    {
	printf("minalignrlog rlogfile\n");
	return -1;
    }

    pInput = RLOG_CreateInputStruct(argv[1]);
    if (pInput == NULL)
    {
	printf("Error opening '%s'\n", argv[1]);
	return -1;
    }

    range = pInput->header.nMaxRank - pInput->header.nMinRank + 1;

    /*if (bValidateArrows)*/
    {
	num_arrows = RLOG_GetNumArrows(pInput);
	if (num_arrows)
	{
	    printf("num arrows: %d\n", num_arrows);
	    RLOG_GetNextArrow(pInput, &lastarrow);
	    if (lastarrow.start_time > lastarrow.end_time)
	    {
		printf("start > end: %g > %g\n", lastarrow.start_time, lastarrow.end_time);
		bInvalidArrowFound = 1;
	    }
	    while (RLOG_GetNextArrow(pInput, &arrow) == 0)
	    {
		if (arrow.start_time > arrow.end_time)
		{
		    printf("start > end: %g > %g\n", arrow.start_time, arrow.end_time);
		    bInvalidArrowFound = 1;
		}
		if (arrow.end_time < lastarrow.end_time)
		{
		    printf("arrows out of order: %g < %g\n", arrow.end_time, lastarrow.end_time);
		    bInvalidArrowFound = 1;
		}
		lastarrow = arrow;
	    }
	}
	RLOG_ResetArrowIter(pInput);
	if (bInvalidArrowFound)
	{
	    printf("Invalid arrows found in the rlog file\n");
	    RLOG_CloseInputStruct(&pInput);
	    return -1;
	}
    }

    pOffset = (double*)MPIU_Malloc(range * sizeof(double));
    if (pOffset == NULL)
    {
	printf("malloc failed\n");
	return -1;
    }

    for (i=0; i<range; i++)
	pOffset[i] = 0.0;
    num_arrows = RLOG_GetNumArrows(pInput);
    if (num_arrows)
    {
	/*printf("num arrows: %d\n", num_arrows);*/
	while (RLOG_GetNextArrow(pInput, &arrow) == 0)
	{
	    if (arrow.leftright == RLOG_ARROW_LEFT)
	    {
		arrow.start_time += pOffset[arrow.dest];
		arrow.end_time += pOffset[arrow.src];
		if (arrow.start_time < arrow.end_time)
		{
		    pOffset[arrow.dest] += arrow.end_time - arrow.start_time;
		}
	    }
	    else
	    {
		arrow.start_time += pOffset[arrow.src];
		arrow.end_time += pOffset[arrow.dest];
		if (arrow.start_time > arrow.end_time)
		{
		    pOffset[arrow.dest] += arrow.start_time - arrow.end_time;
		}
	    }
	}

	for (j=0; j<range; j++)
	{
	    printf("[%d] -> %g\n", j, pOffset[j]);
	}

	RLOG_CloseInputStruct(&pInput);

	/* modify all the events */
	RLOG_ModifyEvents(argv[1], pOffset, range);
    }
    else
    {
	RLOG_CloseInputStruct(&pInput);
    }

    return 0;
}

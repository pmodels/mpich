/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "rlog.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
/* Needed for unlink */
#include <unistd.h>
#endif
#include <errno.h>
#include <ctype.h> /* isdigit */
#include "mpichconf.h" /* HAVE_SNPRINTF */
#include "mpimem.h" /* MPL_snprintf */

#ifndef BOOL
#define BOOL int
#endif
#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

#define MAX_RANK 1024*1024

typedef struct RLOG_State_list
{
    RLOG_STATE state;
    struct RLOG_State_list *next;
} RLOG_State_list;

typedef struct RecursionStruct
{
    char filename[1024];
    int rank;
    int level;
    int num_events;
    FILE *fout;
    struct RecursionStruct *next;
} RecursionStruct;

typedef struct StartArrowStruct
{
    double start_time;
    int src, tag, length;
    struct StartArrowStruct *next;
} StartArrowStruct;

typedef struct EndArrowStruct
{
    int src, tag;
    double timestamp;
    struct EndArrowStruct *next;
} EndArrowStruct;

typedef struct ArrowNode
{
    int rank;
    StartArrowStruct *pStartList;
    EndArrowStruct *pEndList;
    struct ArrowNode *pNext;
} ArrowNode;

ArrowNode *g_pArrowList = NULL;
RecursionStruct *g_pLevel = NULL;
FILE *g_fArrow = NULL;
char g_pszArrowFilename[1024];
RLOG_State_list *g_pList = NULL;

/* Prototypes */
BOOL GetNextArrow(IRLOG_IOStruct *pInput);
void ReadAllArrows(IRLOG_IOStruct **ppInput, int nNumInputs);
BOOL IsNumber(char *str);
void GenerateNewArgv(int *pargc, char ***pargv, int n);
int FindMinRank(RecursionStruct *pLevel);
int FindMaxRank(RecursionStruct *pLevel);
ArrowNode *GetArrowNode(int rank);
EndArrowStruct *ExtractEndNode(ArrowNode *pNode, int src, int tag);
StartArrowStruct *ExtractStartNode(ArrowNode *pNode, int src, int tag);
void SaveArrow(RLOG_IARROW *pArrow);
RecursionStruct *GetLevel(int rank, int recursion);
void SaveEvent(RLOG_EVENT *pEvent);
void SaveState(RLOG_STATE *pState);
void AppendFile(FILE *fout, FILE *fin);
RecursionStruct *FindMinLevel(RecursionStruct *pLevel);
int FindMaxRecursion(RecursionStruct *pLevel, int rank);
void RemoveLevel(int rank);


static int ReadFileData(char *pBuffer, int length, FILE *fin)
{
    int num_read;

    while (length)
    {
	num_read = fread(pBuffer, 1, length, fin);
	if (num_read == -1)
	{
	    MPL_error_printf("Error: fread failed - %s\n", strerror(errno));
	    return errno;
	}

	/*printf("fread(%d)", num_read);fflush(stdout);*/

	length -= num_read;
	pBuffer += num_read;
    }
    return 0;
}

static int WriteFileData(const void *pvBuffer, int length, FILE *fout)
{
    int num_written;
    const char *pBuffer = pvBuffer;

    while (length)
    {
	num_written = fwrite(pBuffer, 1, length, fout);
	if (num_written == -1)
	{
	    MPL_error_printf("Error: fwrite failed - %s\n", strerror(errno));
	    return errno;
	}

	/*printf("fwrite(%d)", num_written);fflush(stdout);*/

	length -= num_written;
	pBuffer += num_written;
    }
    return 0;
}

ArrowNode *GetArrowNode(int rank)
{
    ArrowNode *pNode = g_pArrowList;

    while (pNode)
    {
	if (pNode->rank == rank)
	    return pNode;
	pNode = pNode->pNext;
    }

    pNode = (ArrowNode *)MPIU_Malloc(sizeof(ArrowNode));
    pNode->pEndList = NULL;
    pNode->pStartList = NULL;
    pNode->rank = rank;
    pNode->pNext = g_pArrowList;
    g_pArrowList = pNode;

    return pNode;
}

EndArrowStruct *ExtractEndNode(ArrowNode *pNode, int src, int tag)
{
    EndArrowStruct *pIter, *pRet;
    if (pNode->pEndList == NULL)
	return NULL;
    if ((pNode->pEndList->src == src) && (pNode->pEndList->tag == tag))
    {
	pRet = pNode->pEndList;
	pNode->pEndList = pNode->pEndList->next;
	return pRet;
    }
    pIter = pNode->pEndList;
    while (pIter->next != NULL)
    {
	if ((pIter->next->src == src) && (pIter->next->tag == tag))
	{
	    pRet = pIter->next;
	    pIter->next = pIter->next->next;
	    return pRet;
	}
	pIter = pIter->next;
    }
    return NULL;
}

StartArrowStruct *ExtractStartNode(ArrowNode *pNode, int src, int tag)
{
    StartArrowStruct *pIter, *pRet;
    if (pNode->pStartList == NULL)
	return NULL;
    if ((pNode->pStartList->src == src) && (pNode->pStartList->tag == tag))
    {
	pRet = pNode->pStartList;
	pNode->pStartList = pNode->pStartList->next;
	return pRet;
    }
    pIter = pNode->pStartList;
    while (pIter->next != NULL)
    {
	if ((pIter->next->src == src) && (pIter->next->tag == tag))
	{
	    pRet = pIter->next;
	    pIter->next = pIter->next->next;
	    return pRet;
	}
	pIter = pIter->next;
    }
    return NULL;
}

void SaveArrow(RLOG_IARROW *pArrow)
{
    ArrowNode *pNode;
    StartArrowStruct *pStart, *pStartIter;
    EndArrowStruct *pEnd, *pEndIter;
    RLOG_ARROW arrow;

    if (g_fArrow == NULL)
    {
	MPIU_Strncpy(g_pszArrowFilename, "ArrowFile.tmp", 1024);
	g_fArrow = fopen(g_pszArrowFilename, "w+b");
	if (g_fArrow == NULL)
	{
	    MPL_error_printf("unable to open ArrowFile.tmp\n");
	    return;
	}
    }

    if (pArrow->sendrecv == RLOG_SENDER)
    {
	pNode = GetArrowNode(pArrow->remote);
	pEnd = ExtractEndNode(pNode, pArrow->rank, pArrow->tag);
	if (pEnd == NULL)
	{
	    pStart = (StartArrowStruct *)MPIU_Malloc(sizeof(StartArrowStruct));
	    pStart->src = pArrow->rank;
	    pStart->tag = pArrow->tag;
	    pStart->length = pArrow->length;
	    pStart->start_time = pArrow->timestamp;
	    pStart->next = NULL;
	    if (pNode->pStartList == NULL)
	    {
		pNode->pStartList = pStart;
	    }
	    else
	    {
		pStartIter = pNode->pStartList;
		while (pStartIter->next != NULL)
		    pStartIter = pStartIter->next;
		pStartIter->next = pStart;
	    }
	    return;
	}
	arrow.src = pArrow->rank;
	arrow.dest = pArrow->remote;
	arrow.length = pArrow->length;
	arrow.start_time = pEnd->timestamp;
	arrow.end_time = pArrow->timestamp;
	arrow.tag = pArrow->tag;
	arrow.leftright = RLOG_ARROW_LEFT;
	/* fwrite(&arrow, sizeof(RLOG_ARROW), 1, g_fArrow); */
	WriteFileData(&arrow, sizeof(RLOG_ARROW), g_fArrow);
	MPIU_Free(pEnd);
    }
    else
    {
	arrow.dest = pArrow->rank;
	arrow.end_time = pArrow->timestamp;
	arrow.tag = pArrow->tag;
	arrow.length = pArrow->length;

	pNode = GetArrowNode(pArrow->rank);
	pStart = ExtractStartNode(pNode, pArrow->remote, pArrow->tag);
	if (pStart != NULL)
	{
	    arrow.src = pStart->src;
	    arrow.start_time = pStart->start_time;
	    arrow.length = pStart->length; /* the sender length is more accurate than the receiver length */
	    arrow.leftright = RLOG_ARROW_RIGHT;
	    MPIU_Free(pStart);
	    /* fwrite(&arrow, sizeof(RLOG_ARROW), 1, g_fArrow); */
	    WriteFileData(&arrow, sizeof(RLOG_ARROW), g_fArrow);
	}
	else
	{
	    pEnd = (EndArrowStruct *)MPIU_Malloc(sizeof(EndArrowStruct));
	    pEnd->src = pArrow->remote;
	    pEnd->tag = pArrow->tag;
	    pEnd->timestamp = pArrow->timestamp;
	    pEnd->next = NULL;
	    if (pNode->pEndList == NULL)
	    {
		pNode->pEndList = pEnd;
	    }
	    else
	    {
		pEndIter = pNode->pEndList;
		while (pEndIter->next != NULL)
		    pEndIter = pEndIter->next;
		pEndIter->next = pEnd;
	    }
	}
    }

    /* fwrite(pArrow, sizeof(RLOG_IARROW), 1, g_fArrow); */
}

RecursionStruct *GetLevel(int rank, int recursion)
{
    RecursionStruct *pLevel;

    pLevel = g_pLevel;

    while (pLevel)
    {
	if (pLevel->level == recursion && pLevel->rank == rank)
	    return pLevel;
	pLevel = pLevel->next;
    }

    pLevel = (RecursionStruct*)MPIU_Malloc(sizeof(RecursionStruct));
    MPL_snprintf(pLevel->filename, 1024, "irlog.%d.%d.tmp", rank, recursion);
    pLevel->fout = fopen(pLevel->filename, "w+b");
    pLevel->rank = rank;
    pLevel->level = recursion;
    pLevel->num_events = 0;
    pLevel->next = g_pLevel;
    g_pLevel = pLevel;

    return pLevel;
}

void SaveEvent(RLOG_EVENT *pEvent)
{
    RecursionStruct *pLevel;

    pLevel = GetLevel(pEvent->rank, pEvent->recursion);
    pLevel->num_events++;
    /* fwrite(pEvent, sizeof(RLOG_EVENT), 1, pLevel->fout); */
    WriteFileData(pEvent, sizeof(RLOG_EVENT), pLevel->fout);
}

void SaveState(RLOG_STATE *pState)
{
    RLOG_State_list *pIter;

    pIter = g_pList;
    while (pIter)
    {
	if (pIter->state.event == pState->event)
	{
	    /* replace old with new */
	    memcpy(&pIter->state, pState, sizeof(RLOG_STATE));
	    return;
	}
	pIter = pIter->next;
    }

    pIter = (RLOG_State_list*)MPIU_Malloc(sizeof(RLOG_State_list));
    memcpy(&pIter->state, pState, sizeof(RLOG_STATE));
    pIter->next = g_pList;
    g_pList = pIter;
}

/*
FILE *OpenRlogFile(char *filename)
{
    char out_filename[1024];
    char *pExt;

    pExt = strstr(filename, ".irlog");
    if (pExt)
    {
	memcpy(out_filename, filename, pExt-filename);
	strcpy(&out_filename[pExt-filename], ".rlog");
	MPL_msg_printf("out_filename: %s\n", out_filename);
    }
    else
    {
	sprintf(out_filename, "%s.rlog", filename);
    }

    return fopen(out_filename, "wb");
}
*/

#define BUFFER_SIZE 16*1024*1024

void AppendFile(FILE *fout, FILE *fin)
{
    int total;
    int num_read, num_written;
    char *buffer, *buf;

    buffer = (char*)MPIU_Malloc(sizeof(char) * BUFFER_SIZE);

    total = ftell(fin);
    fseek(fin, 0L, SEEK_SET);

    while (total)
    {
	num_read = fread(buffer, 1, min(BUFFER_SIZE, total), fin);
	if (num_read == 0)
	{
	    MPL_error_printf("failed to read from input file\n");
	    return;
	}
	total -= num_read;
	buf = buffer;
	while (num_read)
	{
	    num_written = fwrite(buf, 1, num_read, fout);
	    if (num_written == 0)
	    {
		MPL_error_printf("failed to write to output file\n");
		return;
	    }
	    num_read -= num_written;
	    buf += num_written;
	}
    }

    MPIU_Free(buffer);
}

int FindMinRank(RecursionStruct *pLevel)
{
    int rank = pLevel->rank;
    while (pLevel)
    {
	if (pLevel->rank < rank)
	    rank = pLevel->rank;
	pLevel = pLevel->next;
    }
    return rank;
}

int FindMaxRank(RecursionStruct *pLevel)
{
    int rank = pLevel->rank;
    while (pLevel)
    {
	if (pLevel->rank > rank)
	    rank = pLevel->rank;
	pLevel = pLevel->next;
    }
    return rank;
}

RecursionStruct *FindMinLevel(RecursionStruct *pLevel)
{
    RecursionStruct *pRet;
    pRet = pLevel;
    while (pLevel)
    {
	if (pLevel->rank < pRet->rank)
	{
	    pRet = pLevel;
	}
	else
	{
	    if (pLevel->rank == pRet->rank && pLevel->level < pRet->level)
	    {
		pRet = pLevel;
	    }
	}
	pLevel = pLevel->next;
    }
    return pRet;
}

int FindMaxRecursion(RecursionStruct *pLevel, int rank)
{
    int nMax = 0;
    while (pLevel)
    {
	if (pLevel->rank == rank)
	{
	    nMax = (pLevel->level > nMax) ? pLevel->level : nMax;
	}
	pLevel = pLevel->next;
    }
    return nMax+1; /* return nMax + 1 because the recursion levels begin with zero */
}

void RemoveLevel(int rank)
{
    RecursionStruct *pLevel = g_pLevel, *pTrailer = g_pLevel;
    while (pLevel)
    {
	if (pLevel->rank == rank)
	{
	    if (pLevel == pTrailer)
	    {
		if (g_pLevel == pLevel)
		    g_pLevel = g_pLevel->next;
		pLevel = pLevel->next;
		fclose(pTrailer->fout);
		unlink(pTrailer->filename);
		MPIU_Free(pTrailer);
		pTrailer = pLevel;
	    }
	    else
	    {
		pTrailer->next = pLevel->next;
		fclose(pLevel->fout);
		unlink(pLevel->filename);
		MPIU_Free(pLevel);
		pLevel = pTrailer->next;
	    }
	}
	else
	{
	    if (pTrailer != pLevel)
		pTrailer = pTrailer->next;
	    pLevel = pLevel->next;
	}
    }
}

BOOL GetNextArrow(IRLOG_IOStruct *pInput)
{
    do
    {
	if (IRLOG_GetNextRecord(pInput))
	{
	    IRLOG_CloseInputStruct(&pInput);
	    return FALSE;
	}
    } while (pInput->header.type != RLOG_IARROW_TYPE);
    return TRUE;
}

void ReadAllArrows(IRLOG_IOStruct **ppInput, int nNumInputs)
{
    int i, j;
    double dMin;
    int index;

    if (nNumInputs < 1)
	return;

    /* make sure all the inputs are currently on an iarrow record */
    for (i=0; i<nNumInputs; i++)
    {
	if (ppInput[i]->header.type != RLOG_IARROW_TYPE)
	{
	    if (!GetNextArrow(ppInput[i]))
	    {
		for (j=i; j<nNumInputs-1; j++)
		    ppInput[j] = ppInput[j+1];
		nNumInputs--;
		i--;
	    }
	}
    }

    /* read iarrow records and save them to the arrow file */
    /* until all the inputs are exhausted */
    while (nNumInputs > 0)
    {
	/* find the earliest record */
	dMin = ppInput[0]->record.iarrow.timestamp;
	index = 0;
	for (i=1; i<nNumInputs; i++)
	{
	    if (ppInput[i]->record.iarrow.timestamp < dMin)
	    {
		dMin = ppInput[i]->record.iarrow.timestamp;
		index = i;
	    }
	}
	/* save the record */
	SaveArrow(&ppInput[index]->record.iarrow);
	/* read the next iarrow record from the same source */
	if (!GetNextArrow(ppInput[index]))
	{
	    for (i=index; i<nNumInputs-1; i++)
		ppInput[i] = ppInput[i+1];
	    nNumInputs--;
	}
    }
}

BOOL IsNumber(char *str)
{
    if (str == NULL)
	return FALSE;
    while (*str != '\0')
    {
	if (!isdigit(*str))
	    return FALSE;
	str++;
    }
    return TRUE;
}

static BOOL s_bFreeArgv = FALSE;
void GenerateNewArgv(int *pargc, char ***pargv, int n)
{
    int argc;
    char **argv;
    int length, i;
    char *buffer, *str;

    length = (sizeof(char*) * (n+3)) +strlen((*pargv)[0]) + 1 + strlen((*pargv)[1]) + 1 + (15 * n);
    buffer = (char*)MPIU_Malloc(length);

    argc = n+2;
    argv = (char**)buffer;
    str = buffer + (sizeof(char*) * (n+4));
    argv[0] = str;
    str += sprintf(str, "%s", (*pargv)[0]);
    *str++ = '\0';
    argv[1] = str;
    str += sprintf(str, "%s", (*pargv)[1]);
    *str++ = '\0';
    for (i=0; i<n; i++)
    {
	argv[i+2] = str;
	str += sprintf(str, "log%d.irlog", i);
	*str++ = '\0';
    }
    argv[n+3] = NULL;

    *pargc = argc;
    *pargv = argv;
    s_bFreeArgv = TRUE;
}

int main(int argc, char *argv[])
{
    RLOG_FILE_HEADER header;
    RLOG_State_list *pState;

    int nNumInputs;
    IRLOG_IOStruct *pInput;
    IRLOG_IOStruct **ppInput;

    int nMaxRank = 0;
    int nMinRank = MAX_RANK;
    int nNumStates = 0;
    int type, length;
    int nMaxLevel = 0;
    int nNumLevels = 0;
    int nTotalNumEvents = 0;
    int nNumEvents;
    RecursionStruct *pLevel;
    FILE *fout;
    int i, j;
    int nRank;

    if (argc < 3)
    {
	MPL_error_printf("Usage:\nirlog2rlog out.rlog in0.irlog in1.irlog ...\nirlog2rlog out.rlog n\n");
	return 0;
    }

    if (argc == 3 && IsNumber(argv[2]))
    {
	GenerateNewArgv(&argc, &argv, atoi(argv[2]));
    }

    nNumInputs = argc - 2;

    /* open the output rlog file */
    fout = fopen(argv[1], "wb");
    if (fout == NULL)
    {
	MPL_error_printf("unable to open output file '%s'\n", argv[1]);
	return -1;
    }

    /* read the arrows from all the files in order */
    ppInput = (IRLOG_IOStruct**)MPIU_Malloc(nNumInputs * sizeof(IRLOG_IOStruct*));
    for (i=0; i<nNumInputs; i++)
    {
	ppInput[i] = IRLOG_CreateInputStruct(argv[i+2]);
	if (ppInput[i] == NULL)
	{
	    MPL_error_printf("Unable to create an input structure for '%s', skipping\n", argv[i+2]);
	}
    }
    for (i=0; i<nNumInputs; i++)
    {
	if (ppInput[i] == NULL)
	{
	    for (j=i; j<nNumInputs-1; j++)
		ppInput[j] = ppInput[j+1];
	    nNumInputs--;
	    i--;
	}
    }
    MPL_msg_printf("reading the arrows from all the input files.\n");fflush(stdout);
    ReadAllArrows(ppInput, nNumInputs);

    nNumInputs = argc - 2;

    /* read, parse and save all the data from the irlog files */
    for (i=0; i<nNumInputs; i++)
    {
	pInput = IRLOG_CreateInputStruct(argv[i+2]);
	if (pInput == NULL)
	{
	    MPL_error_printf("Unable to create an input structure for '%s', skipping\n", argv[i+2]);
	}
	else
	{
	    MPL_msg_printf("reading irlog file: %s\n", argv[i+2]);fflush(stdout);
	    for(;;)
	    {
		switch (pInput->header.type)
		{
		case RLOG_STATE_TYPE:
		    SaveState(&pInput->record.state);
		    break;
		case RLOG_COMM_TYPE:
		    nMaxRank = (pInput->record.comm.rank > nMaxRank) ? pInput->record.comm.rank : nMaxRank;
		    nMinRank = (pInput->record.comm.rank < nMinRank) ? pInput->record.comm.rank : nMinRank;
		    break;
		case RLOG_IARROW_TYPE:
		    /* SaveArrow(&pInput->record.iarrow); */
		    break;
		case RLOG_EVENT_TYPE:
		    SaveEvent(&pInput->record.event);
		    break;
		default:
		    MPL_error_printf("Unknown irlog record type: %d\n", pInput->header.type);
		    break;
		}
		
		if (IRLOG_GetNextRecord(pInput))
		{
		    IRLOG_CloseInputStruct(&pInput);
		    break;
		}
	    }
	}
    }

    /* set the fields in the header */
    header.nMinRank = FindMinRank(g_pLevel);
    header.nMaxRank = FindMaxRank(g_pLevel);
    if (nMinRank != header.nMinRank)
	MPL_error_printf("minimum event rank %d does not equal the minimum comm record rank %d\n", header.nMinRank, nMinRank);
    if (nMaxRank != header.nMaxRank)
	MPL_error_printf("maximum event rank %d does not equal the maximum comm record rank %d\n", header.nMaxRank, nMaxRank);

    /* write header */
    MPL_msg_printf("writing header.\n");fflush(stdout);
    type = RLOG_HEADER_SECTION;
    length = sizeof(RLOG_FILE_HEADER);
    /* fwrite(&type, sizeof(int), 1, fout); */
    WriteFileData(&type, sizeof(int), fout);
    /* fwrite(&length, sizeof(int), 1, fout);*/
    WriteFileData(&length, sizeof(int), fout);
    /* fwrite(&header, sizeof(RLOG_FILE_HEADER), 1, fout); */
    WriteFileData(&header, sizeof(RLOG_FILE_HEADER), fout);

    /* write states */
    if (g_pList)
    {
	MPL_msg_printf("writing states.\n");fflush(stdout);
    }
    pState = g_pList;
    while (pState)
    {
	nNumStates++;
	pState = pState->next;
    }
    type = RLOG_STATE_SECTION;
    length = nNumStates * sizeof(RLOG_STATE);
    /* fwrite(&type, sizeof(int), 1, fout); */
    WriteFileData(&type, sizeof(int), fout);
    /* fwrite(&length, sizeof(int), 1, fout); */
    WriteFileData(&length, sizeof(int), fout);
    pState = g_pList;
    while (pState)
    {
	/* fwrite(pState, sizeof(RLOG_STATE), 1, fout); */
	WriteFileData(pState, sizeof(RLOG_STATE), fout);
	pState = pState->next;
    }

    /* write arrows */
    if (g_fArrow)
    {
	MPL_msg_printf("writing arrows.\n");fflush(stdout);
	type = RLOG_ARROW_SECTION;
	length = ftell(g_fArrow);
	/* fwrite(&type, sizeof(int), 1, fout); */
	WriteFileData(&type, sizeof(int), fout);
	/* fwrite(&length, sizeof(int), 1, fout); */
	WriteFileData(&length, sizeof(int), fout);
	AppendFile(fout, g_fArrow);
    }

    /* write events */
    while (g_pLevel)
    {
	pLevel = FindMinLevel(g_pLevel);
	nNumLevels = FindMaxRecursion(g_pLevel, pLevel->rank);
	nRank = pLevel->rank;

	/* count the events for this rank */
	nNumEvents = 0;
	for (i=0; i<nNumLevels; i++)
	{
	    pLevel = GetLevel(pLevel->rank, i);
	    nNumEvents += pLevel->num_events;
	}
	/* write an event section for this rank */
	type = RLOG_EVENT_SECTION;
	length = sizeof(int) + sizeof(int) + (nNumLevels * sizeof(int)) + (nNumEvents * sizeof(RLOG_EVENT));
	/* fwrite(&type, sizeof(int), 1, fout); */
	WriteFileData(&type, sizeof(int), fout);
	/* fwrite(&length, sizeof(int), 1, fout); */
	WriteFileData(&length, sizeof(int), fout);
        /* fwrite(&nRank, sizeof(int), 1, fout); */
	WriteFileData(&nRank, sizeof(int), fout);
	/* fwrite(&nNumLevels, sizeof(int), 1, fout); */
	WriteFileData(&nNumLevels, sizeof(int), fout);
	for (i=0; i<nNumLevels; i++)
	{
	    pLevel = GetLevel(nRank, i);
	    /* fwrite(&pLevel->num_events, sizeof(int), 1, fout); */
	    WriteFileData(&pLevel->num_events, sizeof(int), fout);
	}
	for (i=0; i<nNumLevels; i++)
	{
	    MPL_msg_printf("writing event level %d:%d\n", nRank, i);fflush(stdout);
	    pLevel = GetLevel(nRank, i);
	    AppendFile(fout, pLevel->fout);
	}
	/* remove this rank from the list of levels */
	RemoveLevel(nRank);
    }

    /* free resources */
    while (g_pList)
    {
	pState = g_pList;
	g_pList = g_pList->next;
	MPIU_Free(pState);
    }
    if (g_fArrow)
    {
	fclose(g_fArrow);
	unlink(g_pszArrowFilename);
    }

    if (s_bFreeArgv)
	MPIU_Free(argv);

    return 0;
}

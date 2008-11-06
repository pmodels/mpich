/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "rlog.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "mpimem.h"

static int ReadFileData(char *pBuffer, int length, FILE *fin)
{
    int num_read;

    while (length)
    {
	num_read = (int)fread(pBuffer, 1, length, fin);
	if (num_read == -1)
	{
	    MPIU_Error_printf("Error: fread failed - %s\n", strerror(errno));
	    return errno;
	}

	/*printf("fread(%d)", num_read);fflush(stdout);*/

	length -= num_read;
	pBuffer += num_read;
    }
    return 0;
}

static int WriteFileData(const char *pBuffer, int length, FILE *fout)
{
    int num_written;

    while (length)
    {
	num_written = (int)fwrite(pBuffer, 1, length, fout);
	if (num_written == -1)
	{
	    MPIU_Error_printf("Error: fwrite failed - %s\n", strerror(errno));
	    return errno;
	}

	/*printf("fwrite(%d)", num_written);fflush(stdout);*/

	length -= num_written;
	pBuffer += num_written;
    }
    return 0;
}

IRLOG_IOStruct *IRLOG_CreateInputStruct(const char *filename)
{
    int num_read;
    IRLOG_IOStruct *pInput;

    /* allocate an input structure */
    pInput = (IRLOG_IOStruct*)MPIU_Malloc(sizeof(IRLOG_IOStruct));
    if (pInput == NULL)
    {
	MPIU_Error_printf("malloc failed - %s\n", strerror(errno));
	return NULL;
    }
    /* open the input clog file */
    pInput->f = fopen(filename, "rb");
    if (pInput->f == NULL)
    {
	MPIU_Error_printf("fopen(%s) failed, error: %s\n", filename, strerror(errno));
	MPIU_Free(pInput);
	return NULL;
    }
    /* read some data */
    num_read = (int)fread(pInput->buffer, 1, RLOG_BUFFSIZE, pInput->f);
    if (num_read == 0)
    {
	MPIU_Error_printf("Unable to read data from the input file.\n");
	fclose(pInput->f);
	MPIU_Free(pInput);
	return NULL;
    }
    /* set the data fields and get the first record */
    pInput->pCurHeader = pInput->buffer;
    pInput->pNextHeader = pInput->buffer;
    pInput->pEnd = pInput->buffer + num_read;
    if (IRLOG_GetNextRecord(pInput))
    {
	MPIU_Error_printf("Unable to get the first record from the file.\n");
	fclose(pInput->f);
	MPIU_Free(pInput);
	return NULL;
    }
    return pInput;
}

IRLOG_IOStruct *IRLOG_CreateOutputStruct(const char *filename)
{
    IRLOG_IOStruct *pOutput = NULL;

    /* allocate a data structure */
    pOutput = (IRLOG_IOStruct*)MPIU_Malloc(sizeof(IRLOG_IOStruct));
    if (pOutput == NULL)
    {
	MPIU_Error_printf("malloc failed - %s\n", strerror(errno));
	return NULL;
    }

    /* open the output clog file */
    pOutput->f = fopen(filename, "wb");
    if (pOutput->f == NULL)
    {
	MPIU_Error_printf("Unable to open output file '%s' - %s\n", filename, strerror(errno));
	MPIU_Free(pOutput);
	return NULL;
    }

    /* set all the data fields */
    pOutput->header.type = RLOG_INVALID_TYPE;
    pOutput->pCurHeader = pOutput->buffer;
    pOutput->pNextHeader = pOutput->buffer;
    pOutput->pEnd = &pOutput->buffer[RLOG_BUFFSIZE];

    return pOutput;
}

int IRLOG_GetNextRecord(IRLOG_IOStruct *pInput)
{
    int num_valid, num_read;

    pInput->pCurHeader = pInput->pNextHeader;

    if (pInput->pEnd - pInput->pCurHeader < sizeof(RLOG_HEADER))
    {
	num_valid = (int)(pInput->pEnd - pInput->pCurHeader);
	if (pInput->pCurHeader != pInput->buffer)
	    memcpy(pInput->buffer, pInput->pCurHeader, num_valid);
	ReadFileData(pInput->buffer + num_valid, sizeof(RLOG_HEADER) - num_valid, pInput->f);
	pInput->pCurHeader = pInput->buffer;
	pInput->pNextHeader = pInput->buffer;
	pInput->pEnd = pInput->buffer + sizeof(RLOG_HEADER);
    }

    /* copy the current header into a temporary variable so the bytes can be manipulated */
    memcpy(&pInput->header, pInput->pCurHeader, sizeof(RLOG_HEADER));
    /*
    CLOGByteSwapDouble(&(header.timestamp), 1);
    CLOGByteSwapInt(&(header.rectype), 1);
    CLOGByteSwapInt(&(header.length), 1);
    */

    while (pInput->pCurHeader + pInput->header.length > pInput->pEnd)
    {
	num_valid = (int)(pInput->pEnd - pInput->pCurHeader);
	if (pInput->pCurHeader != pInput->buffer)
	    memcpy(pInput->buffer, pInput->pCurHeader, num_valid);
	num_read = (int)fread(pInput->buffer + num_valid, 1, RLOG_BUFFSIZE - num_valid, pInput->f);
	if (num_read == 0)
	{
	    MPIU_Error_printf("RLOG Error: unable to get the next record.\n");
	    return 1;
	}
	pInput->pEnd = pInput->buffer + num_valid + num_read;
	pInput->pCurHeader = pInput->buffer;
    }

    pInput->pNextHeader = pInput->pCurHeader + pInput->header.length;

    switch (pInput->header.type)
    {
    case RLOG_INVALID_TYPE:
	MPIU_Error_printf("RLOG Error: invalid record type.\n");
	return 1;
	break;
    case RLOG_ENDLOG_TYPE:
	return 1;
	break;
    case RLOG_EVENT_TYPE:
	memcpy(&pInput->record.event, pInput->pCurHeader + sizeof(RLOG_HEADER), sizeof(RLOG_EVENT));
	break;
    case RLOG_IARROW_TYPE:
	memcpy(&pInput->record.iarrow, pInput->pCurHeader + sizeof(RLOG_HEADER), sizeof(RLOG_IARROW));
	break;
    case RLOG_STATE_TYPE:
	memcpy(&pInput->record.state, pInput->pCurHeader + sizeof(RLOG_HEADER), sizeof(RLOG_STATE));
	break;
    case RLOG_COMM_TYPE:
	memcpy(&pInput->record.comm, pInput->pCurHeader + sizeof(RLOG_HEADER), sizeof(RLOG_COMM));
	break;
    default:
	MPIU_Error_printf("RLOG Error: unknown record type %d.\n", pInput->header.type);
	return 1;
	break;
    }

    return 0;
}

int IRLOG_WriteRecord(RLOG_HEADER *pRecord, IRLOG_IOStruct *pOutput)
{
    if (pOutput->pCurHeader + pRecord->length > pOutput->pEnd)
    {
	WriteFileData(pOutput->buffer, (int)(pOutput->pCurHeader - pOutput->buffer), pOutput->f);
	pOutput->pCurHeader = pOutput->buffer;
    }

    /* copy the record into the output buffer */
    memcpy(pOutput->pCurHeader, pRecord, pRecord->length);
    /* advance the current position pointer */
    pOutput->pCurHeader = pOutput->pCurHeader + pRecord->length;

    return 0;
}

int IRLOG_CloseInputStruct(IRLOG_IOStruct **ppInput)
{
    fclose((*ppInput)->f);
    MPIU_Free(*ppInput);
    *ppInput = NULL;
    return 0;
}

int IRLOG_CloseOutputStruct(IRLOG_IOStruct **ppOutput)
{
    WriteFileData((*ppOutput)->buffer, (int)((*ppOutput)->pCurHeader - (*ppOutput)->buffer), (*ppOutput)->f);
    fclose((*ppOutput)->f);
    MPIU_Free(*ppOutput);
    *ppOutput = NULL;
    return 0;
}

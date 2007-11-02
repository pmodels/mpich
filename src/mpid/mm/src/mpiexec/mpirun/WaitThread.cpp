/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id: WaitThread.cpp,v 1.2 2002/09/27 21:11:12 toonen Exp $
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "WaitThread.h"

struct WaitThreadArg
{
	int n;
	HANDLE *pHandle;
};

void WaitThreadFunction(WaitThreadArg *arg)
{
	WaitForMultipleObjects(arg->n, arg->pHandle, TRUE, INFINITE);
	delete arg;
}

void WaitForLotsOfObjects(int nHandles, HANDLE *pHandle)
{
	int i;
	if (nHandles <= MAXIMUM_WAIT_OBJECTS)
		WaitForMultipleObjects(nHandles, pHandle, TRUE, INFINITE);
	else
	{
		int num = (nHandles / MAXIMUM_WAIT_OBJECTS) + 1;
		HANDLE *hThread = new HANDLE[num];
		for (i=0; i<num; i++)
		{
			WaitThreadArg *arg = new WaitThreadArg;
			if (i == num-1)
				arg->n = nHandles % MAXIMUM_WAIT_OBJECTS;
			else
				arg->n = MAXIMUM_WAIT_OBJECTS;
			arg->pHandle = &pHandle[i*MAXIMUM_WAIT_OBJECTS];
			DWORD dwThreadID;
			hThread[i] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)WaitThreadFunction, arg, 0, &dwThreadID);
		}
		WaitForMultipleObjects(num, hThread, TRUE, INFINITE);
		for (i=0; i<num; i++)
			CloseHandle(hThread[i]);
		delete hThread;
	}
}

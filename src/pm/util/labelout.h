/*
 * Copyright 2026 Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LABELOUT_H_INCLUDED
#define LABELOUT_H_INCLUDED

typedef struct {
    int readOut[2], readErr[2];
} IOLabelSetup;

void IOLabelSetDefault(int flag);
int IOLabelSetupFDs(IOLabelSetup *);
int IOLabelSetupInClient(IOLabelSetup *);
int IOLabelSetupFinishInServer(IOLabelSetup *, ProcessState *);
int IOLabelCheckEnv(void);

#endif /* LABELOUT_H_INCLUDED */

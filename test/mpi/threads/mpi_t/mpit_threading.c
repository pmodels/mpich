/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2013 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* This test is adapted from test/mpi/mpi_t/mpit_vars.c.
   But this is a multithreading version in which multiple threads will
   call MPI_T routines.

   If verbose is set, thread 0 will prints out MPI_T control variables,
   performance variables and their categories.
 */

#include "mpi.h"
#include "mpitest.h"
#include "mpithreadtest.h"
#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <stdlib.h>

/* Num of threads to spawn */
const int NTHREADS = 8;

/* Use macro to control printing. Threads must define myThreadId.
 */
static int verbose = 0;
#define DOPRINT (verbose && myThreadId == 0)

char *mpit_scopeToStr(int scope);
char *mpit_bindingToStr(int binding);
char *mpit_validDtypeStr(MPI_Datatype datatype);
char *mpit_varclassToStr(int varClass);
char *mpit_verbosityToStr(int verbosity);
int perfvarReadInt(int pvarIndex, int isContinuous, int *found);
unsigned int perfvarReadUint(int pvarIndex, int isContinuous, int *found);
double perfvarReadDouble(int pvarIndex, int isContinuous, int *found);
int PrintControlVars(FILE * fp, int myThreadId);
int PrintPerfVars(FILE * fp, int myThreadId);
int PrintCategories(FILE * fp, int myThreadId);

MTEST_THREAD_RETURN_TYPE RunTest(void *p)
{
    int myThreadId = (int) (long) p;

    PrintControlVars(stdout, myThreadId);
    if (DOPRINT)
        fprintf(stdout, "\n");

    PrintPerfVars(stdout, myThreadId);
    if (DOPRINT)
        fprintf(stdout, "\n");

    PrintCategories(stdout, myThreadId);

    return MTEST_THREAD_RETVAL_IGN;
}

int main(int argc, char *argv[])
{
    int i, required, provided;
    required = MPI_THREAD_MULTIPLE;

    MPI_T_init_thread(required, &provided);
    if (provided != MPI_THREAD_MULTIPLE) {
        fprintf(stderr, "MPI_THREAD_MULTIPLE not supported by the MPI_T implementation\n");
        MPI_Abort(MPI_COMM_WORLD, -1);
    }

    MPI_Init_thread(&argc, &argv, required, &provided);
    if (provided != MPI_THREAD_MULTIPLE) {
        fprintf(stderr, "MPI_THREAD_MULTIPLE not supported by the MPI implementation\n");
        MPI_Abort(MPI_COMM_WORLD, -1);
    }

    if (getenv("MPITEST_VERBOSE"))
        verbose = 1;

    /* Spawn threads */
    for (i = 0; i < NTHREADS; i++) {
        MTest_Start_thread(RunTest, (void *) (long) i);
    }

    MTest_Join_threads();

    /* Put MPI_T_finalize() after MPI_Finalize() will cause mpich memory
     * tracing facility falsely reports memory leaks, though these memories
     * are freed in MPI_T_finalize().
     */
    MPI_T_finalize();
    MPI_Finalize();

    fprintf(stdout, " No Errors\n");

    return 0;
}

int PrintControlVars(FILE * fp, int myThreadId)
{
    int i, num_cvar, nameLen, verbosity, descLen, binding, scope;
    int ival, hasValue;
    char name[128], desc[1024];
    MPI_T_enum enumtype = MPI_T_ENUM_NULL;
    MPI_Datatype datatype;

    MPI_T_cvar_get_num(&num_cvar);
    if (DOPRINT)
        fprintf(fp, "%d MPI Control Variables\n", num_cvar);
    for (i = 0; i < num_cvar; i++) {
        hasValue = 0;
        nameLen = sizeof(name);
        descLen = sizeof(desc);
        MPI_T_cvar_get_info(i, name, &nameLen, &verbosity, &datatype,
                            &enumtype, desc, &descLen, &binding, &scope);
        if (datatype == MPI_INT && enumtype != MPI_T_ENUM_NULL) {
            int enameLen, enumber;
            char ename[128];
            enameLen = sizeof(ename);
            /* TODO: Extract a useful string to show for an enum */
            MPI_T_enum_get_info(enumtype, &enumber, ename, &enameLen);
        }
        if (datatype == MPI_INT && binding == MPI_T_BIND_NO_OBJECT) {
            int count;
            MPI_T_cvar_handle chandle;
            MPI_T_cvar_handle_alloc(i, NULL, &chandle, &count);
            if (count == 1) {
                MPI_T_cvar_read(chandle, &ival);
                hasValue = 1;
            }
            MPI_T_cvar_handle_free(&chandle);
        }

        if (hasValue && DOPRINT) {
            fprintf(fp, "\t%s=%d\t%s\t%s\t%s\t%s\t%s\n",
                    name,
                    ival,
                    mpit_scopeToStr(scope),
                    mpit_bindingToStr(binding),
                    mpit_validDtypeStr(datatype), mpit_verbosityToStr(verbosity), desc);
        }
        else if (DOPRINT) {
            fprintf(fp, "\t%s\t%s\t%s\t%s\t%s\t%s\n",
                    name,
                    mpit_scopeToStr(scope),
                    mpit_bindingToStr(binding),
                    mpit_validDtypeStr(datatype), mpit_verbosityToStr(verbosity), desc);
        }
    }

    return 0;
}

int PrintPerfVars(FILE * fp, int myThreadId)
{
    int i, numPvar, nameLen, descLen, verbosity, varClass;
    int binding, isReadonly, isContinuous, isAtomic;
    char name[128], desc[1024];
    MPI_T_enum enumtype;
    MPI_Datatype datatype;

    MPI_T_pvar_get_num(&numPvar);
    if (DOPRINT)
        fprintf(fp, "%d MPI Performance Variables\n", numPvar);

    for (i = 0; i < numPvar; i++) {
        nameLen = sizeof(name);
        descLen = sizeof(desc);
        MPI_T_pvar_get_info(i, name, &nameLen, &verbosity, &varClass,
                            &datatype, &enumtype, desc, &descLen, &binding,
                            &isReadonly, &isContinuous, &isAtomic);

        if (DOPRINT)
            fprintf(fp, "\t%s\t%s\t%s\t%s\t%s\tReadonly=%s\tContinuous=%s\tAtomic=%s\t%s\n",
                    name,
                    mpit_varclassToStr(varClass),
                    mpit_bindingToStr(binding),
                    mpit_validDtypeStr(datatype),
                    mpit_verbosityToStr(verbosity),
                    isReadonly ? "T" : "F", isContinuous ? "T" : "F", isAtomic ? "T" : "F", desc);

        if (datatype == MPI_INT) {
            int val, isFound;
            val = perfvarReadInt(i, isContinuous, &isFound);
            if (isFound && DOPRINT)
                fprintf(fp, "\tValue = %d\n", val);
        }
        else if (datatype == MPI_UNSIGNED) {
            int isFound;
            unsigned int val;
            val = perfvarReadUint(i, isContinuous, &isFound);
            if (isFound && DOPRINT)
                fprintf(fp, "\tValue = %u\n", val);
        }
        else if (datatype == MPI_DOUBLE) {
            int isFound;
            double val;
            val = perfvarReadDouble(i, isContinuous, &isFound);
            if (isFound && DOPRINT)
                fprintf(fp, "\tValue = %e\n", val);
        }
    }
    return 0;
}

int PrintCategories(FILE * fp, int myThreadId)
{
    int i, j, numCat, nameLen, descLen, numCvars, numPvars, numSubcat;
    char name[128], desc[1024];

    MPI_T_category_get_num(&numCat);
    if (DOPRINT) {
        if (numCat > 0)
            fprintf(fp, "%d MPI_T categories\n", numCat);
        else
            fprintf(fp, "No categories defined\n");
    }

    for (i = 0; i < numCat; i++) {
        nameLen = sizeof(name);
        descLen = sizeof(desc);
        MPI_T_category_get_info(i, name, &nameLen, desc, &descLen, &numCvars,
                                &numPvars, &numSubcat);
        if (DOPRINT)
            fprintf(fp,
                    "Category %s has %d control variables, %d performance variables, %d subcategories\n",
                    name, numCvars, numPvars, numSubcat);

        if (numCvars > 0) {
            if (DOPRINT)
                fprintf(fp, "\tControl variables include: ");
            int *cvarIndex = (int *) malloc(numCvars * sizeof(int));
            MPI_T_category_get_cvars(i, numCvars, cvarIndex);
            for (j = 0; j < numCvars; j++) {
                /* Get just the variable name */
                int varnameLen, verb, binding, scope;
                MPI_Datatype datatype;
                char varname[128];
                varnameLen = sizeof(varname);
                MPI_T_cvar_get_info(cvarIndex[j], varname, &varnameLen,
                                    &verb, &datatype, NULL, NULL, NULL, &binding, &scope);
                if (DOPRINT)
                    fprintf(fp, "%s, ", varname);
            }
            free(cvarIndex);
            if (DOPRINT)
                fprintf(fp, "\n");
        }

        if (numPvars > 0) {
            if (DOPRINT)
                fprintf(fp, "\tPerformance variables include: ");

            int *pvarIndex = (int *) malloc(numPvars * sizeof(int));
            MPI_T_category_get_pvars(i, numPvars, pvarIndex);
            for (j = 0; j < numPvars; j++) {
                int varnameLen, verb, varclass, binding;
                int isReadonly, isContinuous, isAtomic;
                MPI_Datatype datatype;
                char varname[128];
                varnameLen = sizeof(varname);
                MPI_T_pvar_get_info(pvarIndex[j], varname, &varnameLen, &verb,
                                    &varclass, &datatype, NULL, NULL, NULL, &binding,
                                    &isReadonly, &isContinuous, &isAtomic);
                if (DOPRINT)
                    fprintf(fp, "%s, ", varname);

            }
            free(pvarIndex);
            if (DOPRINT)
                fprintf(fp, "\n");
        }

        if (numSubcat > 0) {
            if (DOPRINT)
                fprintf(fp, "\tSubcategories include: ");

            int *subcatIndex = (int *) malloc(numSubcat * sizeof(int));
            MPI_T_category_get_categories(i, numSubcat, subcatIndex);
            for (j = 0; j < numSubcat; j++) {
                int catnameLen, ncvars, npvars, nsubcats;
                char catname[128];
                catnameLen = sizeof(catname);
                MPI_T_category_get_info(subcatIndex[j], catname, &catnameLen, NULL, NULL,
                                        &ncvars, &npvars, &nsubcats);
                if (DOPRINT)
                    fprintf(fp, "%s, ", catname);

            }
            free(subcatIndex);
            if (DOPRINT)
                fprintf(fp, "\n");
        }
    }

    return 0;
}


/* --- Support routines --- */

char *mpit_validDtypeStr(MPI_Datatype datatype)
{
    char *p = 0;
    if (datatype == MPI_INT)
        p = "MPI_INT";
    else if (datatype == MPI_UNSIGNED)
        p = "MPI_UNSIGNED";
    else if (datatype == MPI_UNSIGNED_LONG)
        p = "MPI_UNSIGNED_LONG";
    else if (datatype == MPI_UNSIGNED_LONG_LONG)
        p = "MPI_UNSIGNED_LONG_LONG";
    else if (datatype == MPI_COUNT)
        p = "MPI_COUNT";
    else if (datatype == MPI_CHAR)
        p = "MPI_CHAR";
    else if (datatype == MPI_DOUBLE)
        p = "MPI_DOUBLE";
    else {
        if (datatype == MPI_DATATYPE_NULL) {
            p = "Invalid MPI datatype:NULL";
        }
        else {
            static char typename[MPI_MAX_OBJECT_NAME + 9];
            int tlen;
            strncpy(typename, "Invalid:", MPI_MAX_OBJECT_NAME);
            MPI_Type_get_name(datatype, typename + 8, &tlen);
            if (typename[0])
                p = typename;
        }
    }

    return p;
}

char *mpit_scopeToStr(int scope)
{
    char *p = 0;
    switch (scope) {
    case MPI_T_SCOPE_CONSTANT:
        p = "SCOPE_CONSTANT";
        break;
    case MPI_T_SCOPE_READONLY:
        p = "SCOPE_READONLY";
        break;
    case MPI_T_SCOPE_LOCAL:
        p = "SCOPE_LOCAL";
        break;
    case MPI_T_SCOPE_GROUP:
        p = "SCOPE_GROUP";
        break;
    case MPI_T_SCOPE_GROUP_EQ:
        p = "SCOPE_GROUP_EQ";
        break;
    case MPI_T_SCOPE_ALL:
        p = "SCOPE_ALL";
        break;
    case MPI_T_SCOPE_ALL_EQ:
        p = "SCOPE_ALL_EQ";
        break;
    default:
        p = "Unrecoginized scope";
        break;
    }
    return p;
}

char *mpit_bindingToStr(int binding)
{
    char *p;
    switch (binding) {
    case MPI_T_BIND_NO_OBJECT:
        p = "NO_OBJECT";
        break;
    case MPI_T_BIND_MPI_COMM:
        p = "MPI_COMM";
        break;
    case MPI_T_BIND_MPI_DATATYPE:
        p = "MPI_DATATYPE";
        break;
    case MPI_T_BIND_MPI_ERRHANDLER:
        p = "MPI_ERRHANDLER";
        break;
    case MPI_T_BIND_MPI_FILE:
        p = "MPI_FILE";
        break;
    case MPI_T_BIND_MPI_GROUP:
        p = "MPI_GROUP";
        break;
    case MPI_T_BIND_MPI_OP:
        p = "MPI_OP";
        break;
    case MPI_T_BIND_MPI_REQUEST:
        p = "MPI_REQUEST";
        break;
    case MPI_T_BIND_MPI_WIN:
        p = "MPI_WIN";
        break;
    case MPI_T_BIND_MPI_MESSAGE:
        p = "MPI_MESSAGE";
        break;
    case MPI_T_BIND_MPI_INFO:
        p = "MPI_INFO";
        break;
    default:
        p = "Unknown object binding";
    }
    return p;
}

char *mpit_varclassToStr(int varClass)
{
    char *p = 0;
    switch (varClass) {
    case MPI_T_PVAR_CLASS_STATE:
        p = "CLASS_STATE";
        break;
    case MPI_T_PVAR_CLASS_LEVEL:
        p = "CLASS_LEVEL";
        break;
    case MPI_T_PVAR_CLASS_SIZE:
        p = "CLASS_SIZE";
        break;
    case MPI_T_PVAR_CLASS_PERCENTAGE:
        p = "CLASS_PERCENTAGE";
        break;
    case MPI_T_PVAR_CLASS_HIGHWATERMARK:
        p = "CLASS_HIGHWATERMARK";
        break;
    case MPI_T_PVAR_CLASS_LOWWATERMARK:
        p = "CLASS_LOWWATERMARK";
        break;
    case MPI_T_PVAR_CLASS_COUNTER:
        p = "CLASS_COUNTER";
        break;
    case MPI_T_PVAR_CLASS_AGGREGATE:
        p = "CLASS_AGGREGATE";
        break;
    case MPI_T_PVAR_CLASS_TIMER:
        p = "CLASS_TIMER";
        break;
    case MPI_T_PVAR_CLASS_GENERIC:
        p = "CLASS_GENERIC";
        break;
    default:
        p = "Unrecognized pvar class";
        break;
    }
    return p;
}

char *mpit_verbosityToStr(int verbosity)
{
    char *p = 0;
    switch (verbosity) {
    case MPI_T_VERBOSITY_USER_BASIC:
        p = "VERBOSITY_USER_BASIC";
        break;
    case MPI_T_VERBOSITY_USER_DETAIL:
        p = "VERBOSITY_USER_DETAIL";
        break;
    case MPI_T_VERBOSITY_USER_ALL:
        p = "VERBOSITY_USER_ALL";
        break;
    case MPI_T_VERBOSITY_TUNER_BASIC:
        p = "VERBOSITY_TUNER_BASIC";
        break;
    case MPI_T_VERBOSITY_TUNER_DETAIL:
        p = "VERBOSITY_TUNER_DETAIL";
        break;
    case MPI_T_VERBOSITY_TUNER_ALL:
        p = "VERBOSITY_TUNER_ALL";
        break;
    case MPI_T_VERBOSITY_MPIDEV_BASIC:
        p = "VERBOSITY_MPIDEV_BASIC";
        break;
    case MPI_T_VERBOSITY_MPIDEV_DETAIL:
        p = "VERBOSITY_MPIDEV_DETAIL";
        break;
    case MPI_T_VERBOSITY_MPIDEV_ALL:
        p = "VERBOSITY_MPIDEV_ALL";
        break;
    default:
        p = "Invalid verbosity";
        break;
    }
    return p;
}

char *mpit_errclassToStr(int err)
{
    char *p = 0;
    switch (err) {
    case MPI_T_ERR_MEMORY:
        p = "ERR_MEMORY";
        break;
    case MPI_T_ERR_NOT_INITIALIZED:
        p = "ERR_NOT_INITIALIZED";
        break;
    case MPI_T_ERR_CANNOT_INIT:
        p = "ERR_CANNOT_INIT";
        break;
    case MPI_T_ERR_INVALID_INDEX:
        p = "ERR_INVALID_INDEX";
        break;
    case MPI_T_ERR_INVALID_ITEM:
        p = "ERR_INVALID_ITEM";
        break;
    case MPI_T_ERR_INVALID_HANDLE:
        p = "ERR_INVALID_HANDLE";
        break;
    case MPI_T_ERR_OUT_OF_HANDLES:
        p = "ERR_OUT_OF_HANDLES";
        break;
    case MPI_T_ERR_OUT_OF_SESSIONS:
        p = "ERR_OUT_OF_SESSIONS";
        break;
    case MPI_T_ERR_INVALID_SESSION:
        p = "ERR_INVALID_SESSION";
        break;
    case MPI_T_ERR_CVAR_SET_NOT_NOW:
        p = "ERR_CVAR_SET_NOT_NOW";
        break;
    case MPI_T_ERR_CVAR_SET_NEVER:
        p = "ERR_CVAR_SET_NEVER";
        break;
    case MPI_T_ERR_PVAR_NO_STARTSTOP:
        p = "ERR_PVAR_NO_STARTSTOP";
        break;
    case MPI_T_ERR_PVAR_NO_WRITE:
        p = "ERR_PVAR_NO_WRITE";
        break;
    case MPI_T_ERR_PVAR_NO_ATOMIC:
        p = "ERR_PVAR_NO_ATOMIC";
        break;
    default:
        p = "Unknown MPI_T_ERR class";
        break;
    }
    return p;
}

/* Return the value of the performance variable as the value */
int perfvarReadInt(int pvarIndex, int isContinuous, int *found)
{
    int count, val = -1;
    int err1 = MPI_SUCCESS;
    int err2 = MPI_SUCCESS;
    MPI_T_pvar_session session;
    MPI_T_pvar_handle pvarHandle;
    MPI_T_pvar_session_create(&session);
    MPI_T_pvar_handle_alloc(session, pvarIndex, NULL, &pvarHandle, &count);
    if (count == 1) {
        *found = 1;
        if (!isContinuous) {
            /* start and stop the variable (just because we can) */
            err1 = MPI_T_pvar_start(session, pvarHandle);
            err2 = MPI_T_pvar_stop(session, pvarHandle);
        }
        MPI_T_pvar_read(session, pvarHandle, &val);
    }
    MPI_T_pvar_handle_free(session, &pvarHandle);
    MPI_T_pvar_session_free(&session);

    /* Above codes imply that err1 and err2 should be MPI_SUCCESS.
     * If not, catch errors here, e.g., when MPI_ERR_INTERN is returned.
     */
    if (err1 != MPI_SUCCESS || err2 != MPI_SUCCESS) {
        fprintf(stderr, "Unexpected MPI_T_pvar_start/stop return code\n");
        abort();
    }

    return val;
}

/* Return the value of the performance variable as the value */
unsigned int perfvarReadUint(int pvarIndex, int isContinuous, int *found)
{
    int count;
    unsigned int val = 0;
    int err1 = MPI_SUCCESS;
    int err2 = MPI_SUCCESS;
    MPI_T_pvar_session session;
    MPI_T_pvar_handle pvarHandle;

    *found = 0;
    MPI_T_pvar_session_create(&session);
    MPI_T_pvar_handle_alloc(session, pvarIndex, NULL, &pvarHandle, &count);
    if (count == 1) {
        *found = 1;
        if (!isContinuous) {
            /* start and stop the variable (just because we can) */
            err1 = MPI_T_pvar_start(session, pvarHandle);
            err2 = MPI_T_pvar_stop(session, pvarHandle);
        }
        MPI_T_pvar_read(session, pvarHandle, &val);
    }
    MPI_T_pvar_handle_free(session, &pvarHandle);
    MPI_T_pvar_session_free(&session);

    /* Above codes imply that err1 and err2 should be MPI_SUCCESS.
     * If not, catch errors here, e.g., when MPI_ERR_INTERN is returned.
     */
    if (err1 != MPI_SUCCESS || err2 != MPI_SUCCESS) {
        fprintf(stderr, "Unexpected MPI_T_pvar_start/stop return code\n");
        abort();
    }

    return val;
}

double perfvarReadDouble(int pvarIndex, int isContinuous, int *found)
{
    int count;
    double val = 0.0;
    int err1 = MPI_SUCCESS;
    int err2 = MPI_SUCCESS;
    MPI_T_pvar_session session;
    MPI_T_pvar_handle pvarHandle;

    *found = 0;
    MPI_T_pvar_session_create(&session);
    MPI_T_pvar_handle_alloc(session, pvarIndex, NULL, &pvarHandle, &count);
    if (count == 1) {
        *found = 1;
        if (!isContinuous) {
            /* start and stop the variable (just because we can) */
            err1 = MPI_T_pvar_start(session, pvarHandle);
            err2 = MPI_T_pvar_stop(session, pvarHandle);
        }
        MPI_T_pvar_read(session, pvarHandle, &val);
    }
    MPI_T_pvar_handle_free(session, &pvarHandle);
    MPI_T_pvar_session_free(&session);

    /* Catch errors if MPI_T_pvar_start/stop are not properly implemented */
    if (err1 != MPI_SUCCESS || err2 != MPI_SUCCESS) {
        fprintf(stderr, "Unknown MPI_T return code when starting/stopping double pvar\n");
        abort();
    }

    return val;
}

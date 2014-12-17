/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2014 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* style:allow:snprintf:9 sig:0 */
/* style:allow:strncpy:3 sig:0 */
/* style:allow:fprintf:26 sig:0 */
/* style:allow:free:3 sig:0 */
/* style:allow:malloc:3 sig:0 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "mpi.h"

/* At least one early implementation of MPIT failed to set the
   enumtype and thus dirty data was returned.  This should not be needed,
   but is provided in case an implementation returns too many
   bogus results.  In that case, define NEEDS_MPIT_ENUM_LIMIT */
#ifdef NEEDS_MPIT_ENUM_LIMIT
#define MAX_ENUM_NUMBER 10
#endif

/* Lengths for statically allocated character arrays */
#define MAX_NAME_LEN 128
#define MAX_DESC_LEN 1024

/* Function Prototypes */
/* Top level routines for printing MPI_T variables */
int PrintControlVars( FILE *fp );
int PrintPerfVars( FILE *fp );
int PrintCategories( FILE *fp );

/* Functions to print MPI_T objects */
int PrintEnum( FILE *fp, MPI_T_enum enumtype );

/* Functions to convert enums or handles to strings for printing */
const char *mpit_scopeToStr( int scope );
const char *mpit_bindingToStr( int binding );
const char *mpit_validDtypeStr( MPI_Datatype datatype );
const char *mpit_varclassToStr( int varClass );
const char *mpit_verbosityToStr( int verbosity );
const char *mpit_errclasscheck( int err );

/* Function to convert MPI_T variables to strings for printing */
int getCvarValueAsStr( int idx, MPI_Datatype dataytpe,
                       char varValue[], int varValueLen );
int getPvarValueAsStr( int idx, MPI_Datatype datatype, int isContinuous,
                       char varValue[], int varValueLen );

/* Functions for checking on correctness */
int perfCheckVarType( int varClass, MPI_Datatype datatype );
int checkStringLen( const char *str, int expectedLen, const char *strName );

/* Variable descriptions are sometimes long - this variable controls
   whether they are printed */
static int showDesc = 1;

int main( int argc, char *argv[] )
{
    int required, provided, i, wrank;
    required = MPI_THREAD_SINGLE;

    /* The MPI standard permits MPI_T_init_thread to be called before or
       after MPI_Init_thread.  In some cases, a user may need to
       call MPI_T_init_thread and set some MPI_T variables before calling
       MPI_Init_thread to change the behavior of the initialization. */
    MPI_Init_thread( &argc, &argv, required, &provided );

    MPI_T_init_thread( required, &provided );

    MPI_Comm_rank( MPI_COMM_WORLD, &wrank );

    /* Check for a few command-line controls */
    /* Assumes that all processes see this */
    for (i=1; i<argc; i++) {
        /* Check for "no descriptions" */
        if (strcmp( argv[i], "--nodesc" ) == 0 ||
            strcmp( argv[i], "-nodesc" ) == 0) showDesc = 0;
        else {
            if (wrank == 0) {
                fprintf( stderr, "Unrecognized command line argument %s\n",
                         argv[i] );
                fprintf( stderr, "Usage: mpivars [ -nodesc ]\n" );
                MPI_Abort( MPI_COMM_WORLD, 1 );
                return 1;
            }
        }
    }

    PrintControlVars( stdout );
    fprintf( stdout, "\n" );
    PrintPerfVars( stdout );
    fprintf( stdout, "\n" );
    PrintCategories( stdout );

    /* Finalize MPI_T before MPI in case MPI_Finalize checks for any
       allocated resources (MPICH can be configured to do this) */
    MPI_T_finalize();

    MPI_Finalize();

    return 0;
}

/* Print all of the MPI_T control variables, along with their major properties
   and if possible a value (if possible depends on both the complexity of the
   variable and whether a value is defined) */
int PrintControlVars( FILE *fp )
{
    int          i, num_cvar, nameLen, verbosity, descLen, binding, scope;
    int          hasValue;
    char         name[MAX_NAME_LEN], desc[MAX_DESC_LEN], varValue[512];
    MPI_T_enum   enumtype;
    MPI_Datatype datatype;

    MPI_T_cvar_get_num( &num_cvar );
    fprintf( fp, "%d MPI Control Variables\n", num_cvar );
    for (i=0; i<num_cvar; i++) {
	hasValue = 0;
	nameLen = sizeof(name);
	descLen = sizeof(desc);
	MPI_T_cvar_get_info( i, name, &nameLen, &verbosity, &datatype,
			     &enumtype, desc, &descLen, &binding, &scope );
        /* Check on correct output */
        checkStringLen( desc, descLen, "descLen" );

        /* Attempt to get a value */
        if (binding == MPI_T_BIND_NO_OBJECT) {
            hasValue = getCvarValueAsStr( i, datatype,
                                          varValue, sizeof(varValue) );
        }

	if (hasValue) {
	    fprintf( fp, "\t%-32s=%s\t%s\t%s\t%s\t%s\t%s\n", name, varValue,
		     mpit_scopeToStr( scope ),
		     mpit_bindingToStr( binding ),
		     mpit_validDtypeStr( datatype ),
                     mpit_verbosityToStr( verbosity ),
		     showDesc ? desc : "" );
	}
	else {
	    fprintf( fp, "\t%-32s\t%s\t%s\t%s\t%s\t%s\n", name,
                     mpit_scopeToStr( scope ),
		     mpit_bindingToStr( binding ),
		     mpit_validDtypeStr( datatype ),
                     mpit_verbosityToStr( verbosity ),
		     showDesc ? desc : "" );
	}
	if (datatype == MPI_INT && enumtype != MPI_T_ENUM_NULL) {
            PrintEnum( fp, enumtype );
	}

    }

    return 0;
}

/* Print all of the MPI_T performance variables, along with their
   major properties and if possible a value (if possible depends on
   both the complexity of the variable and whether a value is
   defined) */
int PrintPerfVars( FILE *fp )
{
    int          i, numPvar, nameLen, descLen, verbosity, varClass;
    int          binding, isReadonly, isContinuous, isAtomic;
    char         name[MAX_NAME_LEN], desc[MAX_DESC_LEN], varValue[512];
    MPI_T_enum   enumtype;
    MPI_Datatype datatype;

    MPI_T_pvar_get_num( &numPvar );
    fprintf( fp, "%d MPI Performance Variables\n", numPvar );

    for (i=0; i<numPvar; i++) {
	nameLen = sizeof(name);
	descLen = sizeof(desc);
	MPI_T_pvar_get_info( i, name, &nameLen, &verbosity, &varClass,
			     &datatype, &enumtype, desc, &descLen, &binding,
			     &isReadonly, &isContinuous, &isAtomic );
        /* Check for correct return values */
        checkStringLen( name, nameLen, "nameLen" );
        perfCheckVarType( varClass, datatype );

	fprintf( fp, "\t%-32s\t%s\t%s\t%s\t%s\tReadonly=%s\tContinuous=%s\tAtomic=%s\t%s\n",
                 name,
		 mpit_varclassToStr( varClass ),
		 mpit_bindingToStr( binding ),
		 mpit_validDtypeStr( datatype ),
                 mpit_verbosityToStr( verbosity ),
                 isReadonly ? "T" : "F",
                 isContinuous ? "T" : "F",
                 isAtomic ? "T" : "F",
		 showDesc ? desc : "" );

        if (getPvarValueAsStr( i, datatype, isContinuous,
                               varValue, sizeof(varValue) )) {
            fprintf( fp, "\tValue = %s\n", varValue );
        }
        /* A special case for MPI_INT */
        if (datatype == MPI_INT && enumtype != MPI_T_ENUM_NULL) {
            PrintEnum( fp, enumtype );
        }
    }
    return 0;
}

/* Print the categories of MPI_T variables defined by the implementation. */
int PrintCategories( FILE *fp )
{
    int i, j, numCat, nameLen, descLen, numCvars, numPvars, numSubcat;
    char         name[MAX_NAME_LEN], desc[MAX_DESC_LEN];

    MPI_T_category_get_num( &numCat );
    if (numCat > 0) fprintf( fp, "%d MPI_T categories\n", numCat );
    else fprintf( fp, "No categories defined\n" );

    for (i=0; i<numCat; i++) {
        nameLen = sizeof(name);
        descLen = sizeof(desc);
        MPI_T_category_get_info( i, name, &nameLen, desc, &descLen, &numCvars,
                                 &numPvars, &numSubcat );
        /* Check on correct output */
        checkStringLen( name, nameLen, "nameLen" );

        if (numCvars > 0 || numPvars > 0 || numSubcat > 0) {
            fprintf( fp, "Category %s has %d control variables, %d performance variables, and %d subcategories\n",
                     name, numCvars, numPvars, numSubcat );
        }
        else {
            fprintf( fp, "Category %s defined but has no content\n", name );
        }
        /* Output information about the categories */
        if (numCvars > 0) {
            int *cvarIndex = (int *)malloc( numCvars * sizeof(int) );
            MPI_T_category_get_cvars( i, numCvars, cvarIndex );
            fprintf( fp, "\tControl Variables:\n" );
            for (j=0; j<numCvars; j++) {
                int varnameLen, verbose, binding, scope;
                MPI_Datatype datatype;
                char varname[MAX_NAME_LEN];
                varnameLen = sizeof(varname);
                MPI_T_cvar_get_info( cvarIndex[j], varname, &varnameLen,
                                     &verbose, &datatype, NULL, NULL, NULL,
                                     &binding, &scope );
                fprintf( fp, "\t%-32s:\t%s\t%s\t%s\t%s\n", varname,
                         mpit_scopeToStr( scope ),
                         mpit_bindingToStr( binding ),
                         mpit_validDtypeStr( datatype ),
                         mpit_verbosityToStr( verbose ) );
            }
            free( cvarIndex );
        }
        if (numPvars > 0) {
            int *pvarIndex = (int *)malloc( numPvars * sizeof(int) );
            MPI_T_category_get_pvars( i, numPvars, pvarIndex );
            fprintf( fp, "\tPerformance Variables:\n" );
            for (j=0; j<numPvars; j++) {
                int varnameLen, verbose, binding, varclass;
                int isReadonly, isContinuous, isAtomic;
                MPI_Datatype datatype;
                char varname[MAX_NAME_LEN];
                varnameLen = sizeof(varname);
                MPI_T_pvar_get_info( pvarIndex[j], varname, &varnameLen,
                                     &verbose, &varclass, &datatype,
                                     NULL, NULL, NULL, &binding,
                                     &isReadonly, &isContinuous, &isAtomic );
                fprintf( fp, "\t%-32s:\t%s\t%s\t%s\t%s\n", varname,
                         mpit_varclassToStr( varclass ),
                         mpit_bindingToStr( binding ),
                         mpit_validDtypeStr( datatype ),
                         mpit_verbosityToStr( verbose ) );
            }
            free( pvarIndex );
        }
        if (numSubcat > 0) {
            int *subcatIndex = (int *) malloc(numSubcat * sizeof(int));
            MPI_T_category_get_categories(i, numSubcat, subcatIndex);
            fprintf( fp, "\tSubcategories:\n" );
            for (j=0; j<numSubcat; j++) {
                int catnameLen, ncvars, npvars, nsubcats;
                char catname[MAX_NAME_LEN];
                catnameLen = sizeof(catname);
                MPI_T_category_get_info(subcatIndex[j], catname, &catnameLen,
                                        NULL, NULL,
                                        &ncvars, &npvars, &nsubcats);
                fprintf(fp, "\t%s\n", catname);
            }
            free( subcatIndex );
        }
    }

    return 0;
}

/* Print any MPI_T enum values defined by the implementation. */
int PrintEnum( FILE *fp, MPI_T_enum enumtype )
{
    int i, enumber, enameLen, enumval;
    char ename[MAX_NAME_LEN];

    enameLen = sizeof(ename);
    MPI_T_enum_get_info( enumtype, &enumber, ename, &enameLen);

    fprintf( fp, "Enum %s (%d) values: ", ename, enumber );
#ifdef MAX_ENUM_NUMBER
    if (enumber > MAX_ENUM_NUMBER) {
        fprintf( stderr, "Max number of enum values = %d, > max permitted of %d\n",
                 enumber, MAX_ENUM_NUMBER );
        enumber = MAX_ENUM_NUMBER;
    }
#endif
    for (i=0; i<enumber; i++) {
        enameLen = sizeof(ename);
        MPI_T_enum_get_item( enumtype, i, &enumval, ename, &enameLen );
        fprintf( fp, "%s(%d)%c", ename, enumval, (i != enumber-1) ? ',' : ' ' );
    }
    fprintf( fp, "\n" );

    return 0;
}

/* --- Support routines --- */

const char *mpit_scopeToStr( int scope )
{
    const char *p = 0;
    switch (scope) {
    case MPI_T_SCOPE_CONSTANT: p = "SCOPE_CONSTANT"; break;
    case MPI_T_SCOPE_READONLY: p = "SCOPE_READONLY"; break;
    case MPI_T_SCOPE_LOCAL:    p = "SCOPE_LOCAL";    break;
    case MPI_T_SCOPE_GROUP:    p = "SCOPE_GROUP";    break;
    case MPI_T_SCOPE_GROUP_EQ: p = "SCOPE_GROUP_EQ"; break;
    case MPI_T_SCOPE_ALL:      p = "SCOPE_ALL";      break;
    case MPI_T_SCOPE_ALL_EQ:   p = "SCOPE_ALL_EQ";   break;
    default:                   p = "Unrecoginized scope"; break;
    }
    return p;
}

const char *mpit_bindingToStr( int binding )
 {
     const char *p;
     switch (binding) {
     case MPI_T_BIND_NO_OBJECT:      p = "No-object"; break;
     case MPI_T_BIND_MPI_COMM:       p = "MPI_COMM";  break;
     case MPI_T_BIND_MPI_DATATYPE:   p = "MPI_DATATYPE"; break;
     case MPI_T_BIND_MPI_ERRHANDLER: p = "MPI_ERRHANDLER"; break;
     case MPI_T_BIND_MPI_FILE:       p = "MPI_FILE"; break;
     case MPI_T_BIND_MPI_GROUP:      p = "MPI_GROUP"; break;
     case MPI_T_BIND_MPI_OP:         p = "MPI_OP"; break;
     case MPI_T_BIND_MPI_REQUEST:    p = "MPI_REQUEST"; break;
     case MPI_T_BIND_MPI_WIN:        p = "MPI_WIN"; break;
     case MPI_T_BIND_MPI_MESSAGE:    p = "MPI_MESSAGE"; break;
     case MPI_T_BIND_MPI_INFO:       p = "MPI_INFO"; break;
     default:                        p = "Unknown object binding";
     }
     return p;
 }

const char *mpit_varclassToStr( int varClass )
{
    const char *p=0;
    switch (varClass) {
    case MPI_T_PVAR_CLASS_STATE:         p = "CLASS_STATE"; break;
    case MPI_T_PVAR_CLASS_LEVEL:         p = "CLASS_LEVEL"; break;
    case MPI_T_PVAR_CLASS_SIZE:          p = "CLASS_SIZE";  break;
    case MPI_T_PVAR_CLASS_PERCENTAGE:    p = "CLASS_PERCENTAGE"; break;
    case MPI_T_PVAR_CLASS_HIGHWATERMARK: p = "CLASS_HIGHWATERMARK"; break;
    case MPI_T_PVAR_CLASS_LOWWATERMARK:  p = "CLASS_LOWWATERMARK"; break;
    case MPI_T_PVAR_CLASS_COUNTER:       p = "CLASS_COUNTER"; break;
    case MPI_T_PVAR_CLASS_AGGREGATE:     p = "CLASS_AGGREGATE"; break;
    case MPI_T_PVAR_CLASS_TIMER:         p = "CLASS_TIMER"; break;
    case MPI_T_PVAR_CLASS_GENERIC:       p = "CLASS_GENERIC"; break;
    default:                             p = "Unrecognized pvar class"; break;
    }
    return p;
}

const char *mpit_validDtypeStr( MPI_Datatype datatype )
{
    const char *p = 0;
    if (datatype == MPI_INT)                     p = "MPI_INT";
    else if (datatype == MPI_UNSIGNED)           p = "MPI_UNSIGNED";
    else if (datatype == MPI_UNSIGNED_LONG)      p = "MPI_UNSIGNED_LONG";
    else if (datatype == MPI_UNSIGNED_LONG_LONG) p = "MPI_UNSIGNED_LONG_LONG";
    else if (datatype == MPI_COUNT)              p = "MPI_COUNT";
    else if (datatype == MPI_CHAR)               p = "MPI_CHAR";
    else if (datatype == MPI_DOUBLE)             p = "MPI_DOUBLE";
    else {
	if (datatype == MPI_DATATYPE_NULL) {
	    p = "Invalid MPI datatype:NULL";
	}
	else {
	    static char typename[MPI_MAX_OBJECT_NAME+9];
	    int  tlen;
	    strncpy( typename, "Invalid:", MPI_MAX_OBJECT_NAME );
	    MPI_Type_get_name( datatype, typename+8, &tlen );
            /* We must check location typename[8] to see if
               MPI_Type_get_name returned a name (not all datatypes
               have names).  If it did not, then we indicate that
               with a different message */
	    if (typename[8]) p = typename;
            else p = "Invalid: Unknown datatype name";
	}
    }

    return p;
}

const char *mpit_verbosityToStr( int verbosity )
{
    const char *p = 0;
    switch (verbosity) {
    case MPI_T_VERBOSITY_USER_BASIC:    p = "VERBOSITY_USER_BASIC"; break;
    case MPI_T_VERBOSITY_USER_DETAIL:   p = "VERBOSITY_USER_DETAIL"; break;
    case MPI_T_VERBOSITY_USER_ALL:      p = "VERBOSITY_USER_ALL"; break;
    case MPI_T_VERBOSITY_TUNER_BASIC:   p = "VERBOSITY_TUNER_BASIC"; break;
    case MPI_T_VERBOSITY_TUNER_DETAIL:  p = "VERBOSITY_TUNER_DETAIL"; break;
    case MPI_T_VERBOSITY_TUNER_ALL:     p = "VERBOSITY_TUNER_ALL"; break;
    case MPI_T_VERBOSITY_MPIDEV_BASIC:  p = "VERBOSITY_MPIDEV_BASIC"; break;
    case MPI_T_VERBOSITY_MPIDEV_DETAIL: p = "VERBOSITY_MPIDEV_DETAIL"; break;
    case MPI_T_VERBOSITY_MPIDEV_ALL:    p = "VERBOSITY_MPIDEV_ALL"; break;
    default:                            p = "Invalid verbosity"; break;
    }
    return p;
}

/* Provide English strings for the MPI_T error codes */
const char *mpit_errclasscheck( int err )
{
    const char *p = 0;
    switch (err) {
    case MPI_T_ERR_CVAR_SET_NOT_NOW:  p = "MPI_T cvar not set"; break;
    case MPI_T_ERR_CVAR_SET_NEVER:    p = "MPI_T cvar was never set"; break;
    case MPI_T_ERR_PVAR_NO_STARTSTOP: p = "MPI_T pvar does not support start and stop"; break;
    case MPI_T_ERR_PVAR_NO_WRITE:     p = "MPI_T pvar cannot be written"; break;
    case MPI_T_ERR_PVAR_NO_ATOMIC:    p = "MPI_T pvar not atomic"; break;
    case MPI_T_ERR_MEMORY:            p = "MPI_T out of memory"; break;
    case MPI_T_ERR_NOT_INITIALIZED:   p = "MPI_T not initialized"; break;
    case MPI_T_ERR_CANNOT_INIT:       p = "MPI_T not initializable"; break;
    case MPI_T_ERR_INVALID_INDEX:     p = "MPI_T index invalid";break;
    case MPI_T_ERR_INVALID_ITEM:      p = "MPI_T item index out of range"; break;
    case MPI_T_ERR_INVALID_HANDLE:    p = "MPI_T handle invalid"; break;
    case MPI_T_ERR_OUT_OF_HANDLES:    p = "MPI_T out of handles"; break;
    case MPI_T_ERR_OUT_OF_SESSIONS:   p = "MPI_T out of sessions"; break;
    case MPI_T_ERR_INVALID_SESSION:   p = "MPI_T invalid session"; break;
    default:                          p = "Unknown MPI_T_ERR class"; break;
    }
    return p;
}

/* Read a control variable value and return the value as a null terminated
   string.
   For numeric values, only return a value for scalars (count == 1).
   For character data, return the string if it fits in the available space.
*/
int getCvarValueAsStr( int idx, MPI_Datatype datatype,
                       char varValue[], int varValueLen )
{
    int count, hasValue = 0;
    int ival;
    unsigned uval;
    unsigned long ulval;
    unsigned long long ullval;
    MPI_T_cvar_handle chandle;

    MPI_T_cvar_handle_alloc( idx, NULL, &chandle, &count );
    if (count == 1 || (datatype==MPI_CHAR && count < varValueLen)) {
        if (MPI_INT == datatype) {
            MPI_T_cvar_read( chandle, &ival );
            snprintf( varValue, varValueLen, "%d", ival );
            hasValue = 1;
        } else if (MPI_UNSIGNED == datatype) {
            MPI_T_cvar_read( chandle, &uval );
            snprintf( varValue, varValueLen, "%u", uval );
            hasValue = 1;
        } else if (MPI_UNSIGNED_LONG == datatype) {
            MPI_T_cvar_read( chandle, &ulval );
            snprintf( varValue, varValueLen, "%lu", ulval );
            hasValue = 1;
        } else if (MPI_UNSIGNED_LONG_LONG == datatype) {
            MPI_T_cvar_read( chandle, &ullval );
            snprintf( varValue, varValueLen, "%llu", ullval );
            hasValue = 1;
        } else if (MPI_CHAR == datatype) {
            MPI_T_cvar_read( chandle, varValue );
            hasValue = 1;
        }
    }
    MPI_T_cvar_handle_free( &chandle );

    return hasValue;
}

/* Read a performance variable value and return the value as a null terminated
   string.
   For numeric values, only return a value for scalars (count == 1).
   For character data, return the string if it fits in the available space.
*/
int getPvarValueAsStr( int idx, MPI_Datatype datatype, int isContinuous,
                       char varValue[], int varValueLen )
{
    int err, count, hasValue = 0;
    int ival;
    unsigned uval;
    unsigned long ulval;
    unsigned long long ullval;
    double dval;
    MPI_T_pvar_session session;
    MPI_T_pvar_handle phandle;

    MPI_T_pvar_session_create( &session );
    MPI_T_pvar_handle_alloc( session, idx, NULL, &phandle, &count );
    if (count == 1 || (datatype==MPI_CHAR && count < varValueLen)) {
        if (!isContinuous) {
            err = MPI_T_pvar_start( session, phandle );
            if (err != MPI_SUCCESS) {
                strncpy( varValue, "Failed to start pvar", varValueLen );
                goto fn_fail;
            }
            err = MPI_T_pvar_stop( session, phandle );
            if (err != MPI_SUCCESS) {
                strncpy( varValue, "Failed to stop pvar", varValueLen );
                goto fn_fail;
            }
        }
        if (MPI_INT == datatype) {
            MPI_T_pvar_read( session, phandle, &ival );
            snprintf( varValue, varValueLen, "%d", ival );
            hasValue = 1;
        } else if (MPI_UNSIGNED == datatype) {
            MPI_T_pvar_read( session, phandle, &uval );
            snprintf( varValue, varValueLen, "%u", uval );
            hasValue = 1;
        } else if (MPI_UNSIGNED_LONG == datatype) {
            MPI_T_pvar_read( session, phandle, &ulval );
            snprintf( varValue, varValueLen, "%lu", ulval );
            hasValue = 1;
        } else if (MPI_UNSIGNED_LONG_LONG == datatype) {
            MPI_T_pvar_read( session, phandle, &ullval );
            snprintf( varValue, varValueLen, "%llu", ullval );
            hasValue = 1;
        } else if (MPI_DOUBLE == datatype) {
            MPI_T_pvar_read( session, phandle, &dval );
            snprintf( varValue, varValueLen, "%e", dval );
            hasValue = 1;
        } else if (MPI_CHAR == datatype) {
            MPI_T_pvar_read( session, phandle, varValue );
            hasValue = 1;
        }
    }
 fn_fail:
    MPI_T_pvar_handle_free( session, &phandle );
    MPI_T_pvar_session_free( &session );

    return hasValue;
}


/* Confirm that the datatype is valid for the given performance variable type */
int perfCheckVarType( int varClass, MPI_Datatype datatype )
{
    const char *p = 0;
    switch (varClass) {
        /* INT only */
    case MPI_T_PVAR_CLASS_STATE:
        if (datatype != MPI_INT) {
            p = "Invalid datatype for CLASS_STATE: %s\n";
        }
        break;

        /* Arithmetic */
    case MPI_T_PVAR_CLASS_LEVEL:
    case MPI_T_PVAR_CLASS_SIZE:
    case MPI_T_PVAR_CLASS_HIGHWATERMARK:
    case MPI_T_PVAR_CLASS_LOWWATERMARK:
    case MPI_T_PVAR_CLASS_AGGREGATE:
    case MPI_T_PVAR_CLASS_TIMER:
        if (datatype != MPI_UNSIGNED && datatype != MPI_UNSIGNED_LONG &&
            datatype != MPI_UNSIGNED_LONG_LONG && datatype != MPI_DOUBLE)
            p = "Invalid datatype for arithmetic valued class: %s\n";
        break;
        /* DOUBLE only */
    case MPI_T_PVAR_CLASS_PERCENTAGE:
        if (datatype != MPI_DOUBLE)
            p = "Invalid datatype for CLASS_PERCENTAGE: %s\n";
        break;

        /* Integer valued */
    case MPI_T_PVAR_CLASS_COUNTER:
        if (datatype != MPI_UNSIGNED && datatype != MPI_UNSIGNED_LONG &&
            datatype != MPI_UNSIGNED_LONG_LONG)
            p = "Invalid datatype for integer valued CLASS_COUNTER: %s\n";
        break;

        /* Any */
    case MPI_T_PVAR_CLASS_GENERIC:
        break;

    default:
        p = "Invalid class! (datatype = %s)\n";
        break;
    }
    if (p) {
        const char *p1 = 0;
        if (datatype == MPI_INT) p1 = "MPI_INT";
        else if (datatype == MPI_UNSIGNED) p1 = "MPI_UNSIGNED";
        else if (datatype == MPI_UNSIGNED_LONG) p1 = "MPI_UNSIGNED_LONG";
        else if (datatype == MPI_UNSIGNED_LONG_LONG) p1 = "MPI_UNSIGNED_LONG_LONG";
        else if (datatype == MPI_COUNT) p1 = "MPI_COUNT";
        else if (datatype == MPI_CHAR)  p1 = "MPI_CHAR";
        else if (datatype == MPI_DOUBLE) p1 = "MPI_DOUBLE";
        else {
            if (datatype == MPI_DATATYPE_NULL) {
                p1 = "Invalid MPI datatype:NULL";
            }
            else {
                static char typename[MPI_MAX_OBJECT_NAME];
                int  tlen;
                MPI_Type_get_name( datatype, typename, &tlen );
                if (typename[0]) p1 = typename;
                else p1 = "Invalid: Unknown datatype name";
            }
        }
        fprintf( stderr, p, p1 );
    }
    return 0;
}

/* Several of the MPI_T routines return a string length as well as the string.
   Test that the value returned is correct (yes, at least one implementation
   got this wrong). */
int checkStringLen( const char *str, int expectedLen, const char *strName )
{
    int actLen = strlen(str) + 1;
    if (expectedLen != actLen) {
        fprintf( stderr,
                 "Incorrect return value for %s = %d, should = %d\n",
                 strName, expectedLen, actLen );
        return 1;
    }
    return 0;
}

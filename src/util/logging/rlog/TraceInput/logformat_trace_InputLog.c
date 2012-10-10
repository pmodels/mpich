/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

#include <jni.h>
#include <stdio.h>
#include "logformat_trace_InputLog.h"
#include "trace_API.h"
#include <stdlib.h>
#include "mpimem.h"

/* style: allow:fprintf:20 sig:0 */

#define ACHAR_LENGTH   256
#define AINT_LENGTH    100
#define ADOUBLE_LENGTH 100

/* File descriptor for the output stream */
#define  outfile  stdout 
/* File descriptor for the error stream */
#define  errfile  stderr

/* JNI Global Reference Variables */
       jfieldID   fid4filehandle   = NULL;
static jclass     cid4String       = NULL;
static jclass     cid4DobjDef      = NULL;    /* for Category */
static jmethodID  mid4NewDobjDef   = NULL;
static jclass     cid4YMap         = NULL;    /* for YCoordMap */
static jmethodID  mid4NewYMap      = NULL;
static jclass     cid4Prime        = NULL;    /* for Primitive */
static jmethodID  mid4NewPrime     = NULL;
static jclass     cid4Cmplx        = NULL;    /* for Composite */
static jmethodID  mid4NewCmplx     = NULL;

JNIEXPORT void JNICALL
Java_logformat_trace_InputLog_initIDs( JNIEnv *env , jclass myclass )
{
    fid4filehandle = (*env)->GetFieldID( env, myclass, "filehandle", "J" );
}

/*
 *   tracefile != NULL, ierr == any  => Continue
 *   tracefile == NULL, ierr ==  0   => help message, exit normally, TRUE
 *   tracefile == NULL, ierr !=  0   => error message, exit w/ error, FALSE
 */
JNIEXPORT jboolean JNICALL
Java_logformat_trace_InputLog_open( JNIEnv *env, jobject this )
{
    jclass       myclass;
    jfieldID     fid4filespec;
    jstring      j_filespec;
    const char  *c_filespec;
    TRACE_file   tracefile;
    int          ierr;        

    myclass       = (*env)->GetObjectClass( env, this );
    fid4filespec  = (*env)->GetFieldID( env, myclass, "filespec",
                                        "Ljava/lang/String;" );
    if ( fid4filespec == NULL )
        (*env)->SetLongField( env, this, fid4filehandle, (jlong) 0 );

    j_filespec  = (*env)->GetObjectField( env, this, fid4filespec );
    c_filespec  = (*env)->GetStringUTFChars( env, j_filespec, NULL );
    /* c_filespec = JNU_GetStringNativeChars( env, jfilespec ); */
    ierr  = TRACE_Open( c_filespec, &tracefile );
    /* Set the private variable "filehandle" in java class */
    if ( tracefile != NULL ) {
        fprintf( outfile, "C: Opening trace %s ..... \n", c_filespec );
        fflush( outfile );
        (*env)->SetLongField( env, this, fid4filehandle, (jlong) tracefile );
        return JNI_TRUE;
    }
    else {
        if ( ierr == 0 ) {    /* if ( tracefile == NULL && ierr == 0 ) */
            (*env)->SetLongField( env, this, fid4filehandle, (jlong) 0 );
            fprintf( outfile, "%s\n", TRACE_Get_err_string( ierr ) );
            fflush( outfile );
            return JNI_TRUE;
        }
        else {                /* if ( tracefile == NULL && ierr != 0 ) */
            (*env)->SetLongField( env, this, fid4filehandle, (jlong) 0 );
            fprintf( errfile, "%s\n", TRACE_Get_err_string( ierr ) );
            fflush( errfile );
            return JNI_FALSE;
        }
    }
}

JNIEXPORT jboolean JNICALL
Java_logformat_trace_InputLog_close( JNIEnv *env, jobject this )
{
    TRACE_file    tracefile;
    jlong         filehandle;
    int           ierr;

    /* Clean up all created Global References */
    if ( cid4String != NULL ) {
        (*env)->DeleteGlobalRef( env, cid4String );
    }
    if ( cid4DobjDef != NULL ) {
        (*env)->DeleteGlobalRef( env, cid4DobjDef );
        /*
        if ( mid4NewDobjDef != NULL )
            (*env)->DeleteGlobalRef( env, mid4NewDobjDef );
        */
    }
    if ( cid4YMap != NULL ) {
        (*env)->DeleteGlobalRef( env, cid4YMap );
        /*
        if ( mid4NewYMap != NULL )
            (*env)->DeleteGlobalRef( env, mid4NewYMap );
        */
    }
    if ( cid4Prime != NULL ) {
        (*env)->DeleteGlobalRef( env, cid4Prime );
        /*
        if ( mid4NewPrime != NULL )
            (*env)->DeleteGlobalRef( env, mid4NewPrime );
        */
    }
    if ( cid4Cmplx != NULL ) {
        (*env)->DeleteGlobalRef( env, cid4Cmplx );
        /*
        if ( mid4NewCmplx != NULL )
            (*env)->DeleteGlobalRef( env, mid4NewCmplx );
        */
    }

    filehandle = (*env)->GetLongField( env, this, fid4filehandle );
    if ( filehandle == 0 ) {
        fprintf( errfile, "Java_logformat_trace_InputLog_close(): "
                          "Inaccessible filehandle in Java side\n" );
        return JNI_FALSE;
    }   
    tracefile  = (TRACE_file) filehandle;
    fprintf( outfile, "C: Closing trace ..... \n" );
    fflush( outfile );
    ierr  = TRACE_Close( &tracefile );
    if ( ierr == 0 || tracefile == NULL )
        return JNI_TRUE;
    else {
        fprintf( errfile, "%s\n", TRACE_Get_err_string( ierr ) );
        fflush( errfile );
        return JNI_FALSE;
    }
}

JNIEXPORT jint JNICALL 
Java_logformat_trace_InputLog_peekNextKindIndex( JNIEnv *env, jobject this )
{
    TRACE_file        tracefile;
    jlong             filehandle;
    TRACE_Rec_Kind_t  next_kind;
    int               ierr;

    filehandle = (*env)->GetLongField( env, this, fid4filehandle );
    if ( filehandle == 0 ) {
        fprintf( errfile, "Java_logformat_trace_InputLog_peekNextKindIndex(): "
                          "Inaccessible filehandle in Java side\n" );
        return TRACE_EOF;
    }   
    tracefile  = (TRACE_file) filehandle;

    ierr = TRACE_Peek_next_kind( tracefile, &next_kind );
    if ( ierr != 0 ) {
        fprintf( errfile, "%s\n", TRACE_Get_err_string( ierr ) );
        fflush( errfile );
        return TRACE_EOF;
    }
	
	return next_kind;
}

JNIEXPORT jobject JNICALL 
Java_logformat_trace_InputLog_getNextCategory( JNIEnv *env, jobject this )
{
    TRACE_file              tracefile;
    jlong                   filehandle;
    TRACE_Category_head_t   type_head;
    int                     legend_sz, legend_pos, legend_max;
    char                   *legend_base; 
    jstring                 jlegend;
    int                     label_sz, label_pos, label_max = 0;
    char                   *label_base;
    jstring                 jlabel;
    int                     methods_sz, methods_pos, methods_max = 0;
    int                    *methods_base; 
    jintArray               jmethods;
    jclass                  cid_local;
    jobject                 objdef;
    int                     ierr;
    char slegend_base[ACHAR_LENGTH];
    char slabel_base[ACHAR_LENGTH];
    int smethods_base[AINT_LENGTH];

    filehandle = (*env)->GetLongField( env, this, fid4filehandle );
    if ( filehandle == 0 ) {
        fprintf( errfile, "Java_logformat_trace_InputLog_getNextCategory(): "
                          "Inaccessible filehandle in Java side\n" );
        return NULL;
    }   
    tracefile  = (TRACE_file) filehandle;

    label_sz   = 0;
    legend_sz  = 0;
    methods_sz = 0;
    ierr = TRACE_Peek_next_category( tracefile,
                                     &legend_sz, &label_sz, &methods_sz );
    if ( ierr != 0 || legend_sz <= 0 ) {
        fprintf( errfile, "%s\n", TRACE_Get_err_string( ierr ) );
        fflush( errfile );
        return NULL;
    }

    legend_pos  = 0;
    if ( legend_sz ) {
        legend_max  = legend_sz+1;
	if (legend_max > ACHAR_LENGTH)
	    legend_base = (char *) MPIU_Malloc( legend_max * sizeof( char ) );
	else
	    legend_base = slegend_base;
    }
    else
        legend_base = NULL;

    label_pos   = 0;
    if ( label_sz > 0 ) {
        label_max   = label_sz+1;
	if (label_max > ACHAR_LENGTH)
	    label_base  = (char *) MPIU_Malloc( label_max * sizeof( char ) );
	else
	    label_base = slabel_base;
    }
    else
        label_base  = NULL;

   	methods_pos  = 0;
    if ( methods_sz > 0 ) {
        methods_max  = methods_sz;
	if (methods_max > AINT_LENGTH)
	    methods_base = (int *)  MPIU_Malloc( methods_max * sizeof( int ) );
	else
	    methods_base = smethods_base;
    }
    else
        methods_base = NULL;

    ierr = TRACE_Get_next_category( tracefile, &type_head,
                                    &legend_sz, legend_base,
                                    &legend_pos, legend_max,
                                    &label_sz, label_base,
                                    &label_pos, label_max,
                                    &methods_sz, methods_base,
                                    &methods_pos, methods_max );
    if ( ierr != 0 || legend_pos <= 0 ) {
        fprintf( errfile, "%s\n", TRACE_Get_err_string( ierr ) );
        fflush( errfile );
	if ( legend_base != NULL && legend_base != slegend_base )
	    MPIU_Free( legend_base );
	if ( label_base != NULL && label_base != slabel_base )
	    MPIU_Free( label_base );
	if ( methods_base != NULL && methods_base != smethods_base )
	    MPIU_Free( methods_base );
        return NULL;
    }

    /* Obtain various array references for calling DobjDef's constructor */
    if ( legend_base != NULL && legend_pos > 0 ) {
        legend_base[ legend_pos ]  = '\0';
        jlegend  = (*env)->NewStringUTF( env, legend_base );
    }
    else
        jlegend  = NULL;

    if ( label_base != NULL && label_pos > 0 ) {
        label_base[ label_pos ]  = '\0';
        jlabel   = (*env)->NewStringUTF( env, label_base );
    }
    else
        jlabel   = NULL;

    if ( methods_base != NULL && methods_pos > 0 ) {
        jmethods = (*env)->NewIntArray( env, methods_sz );
        (*env)->SetIntArrayRegion( env, jmethods,
                                   0, methods_sz, (jint *) methods_base );
    }
    else
        jmethods = NULL;

    /* Cache DobjDef Class's constructor as Global Reference */
    if ( cid4DobjDef == NULL ) {
        cid_local = (*env)->FindClass( env, "logformat/trace/DobjDef" );
        if ( cid_local != NULL ) {
            cid4DobjDef    = (*env)->NewGlobalRef( env, cid_local );
            (*env)->DeleteLocalRef( env, cid_local );  /* avoid memory leak */
            mid4NewDobjDef = (*env)->GetMethodID( env, cid4DobjDef, "<init>",
                         "(ILjava/lang/String;IIIIIILjava/lang/String;[I)V" );
        }
    }

    /* Call DobjDef's constructor  */
    objdef = (*env)->NewObject( env, cid4DobjDef, mid4NewDobjDef,
                                type_head.index, jlegend, type_head.shape,
                                type_head.red, type_head.green, type_head.blue,
                                type_head.alpha, type_head.width,
                                jlabel, jmethods );

    /* Clean up the unused reference and free local memory */
    if ( jlegend != NULL )
        (*env)->DeleteLocalRef( env, jlegend );
    if ( legend_base != NULL && legend_base != slegend_base )
        MPIU_Free( legend_base );

    if ( jlabel != NULL )
        (*env)->DeleteLocalRef( env, jlabel );
    if ( label_base != NULL && label_base != slabel_base )
        MPIU_Free( label_base );

    if ( jmethods != NULL )
        (*env)->DeleteLocalRef( env, jmethods );
    if ( methods_base != NULL && methods_base != smethods_base )
        MPIU_Free( methods_base );

    return objdef;
}

JNIEXPORT jobject JNICALL
Java_logformat_trace_InputLog_getNextYCoordMap( JNIEnv *env, jobject this )
{
    TRACE_file              tracefile;
    jlong                   filehandle;
    int                     nrows, ncolumns;
    int                     max_column_name, max_title_name;
    char                   *title_name;
    char                  **column_names;
    int                     coordmap_sz, coordmap_pos, coordmap_max;
    int                    *coordmap_base;
    jintArray               j_coordmap_elems;
    int                     methods_sz, methods_pos, methods_max;
    int                    *methods_base; 
    jintArray               jmethods;

    jclass                  cid_local;
    jstring                 jtitle; 
    jstring                 jcolname;
    jobjectArray            jcolnames;
    jobject                 ycoordmap;

    int                     icol;
    int                     ierr;

    filehandle = (*env)->GetLongField( env, this, fid4filehandle );
    if ( filehandle == 0 ) {
        fprintf( errfile, "Java_logformat_trace_InputLog_getNextYCoordMap(): "
                          "Inaccessible filehandle in Java side\n" );
        return NULL;
    }   
    tracefile  = (TRACE_file) filehandle;

    nrows            = 0;
    ncolumns         = 0;
    max_column_name  = 0;
    max_title_name   = 0;
    methods_sz       = 0;
    ierr = TRACE_Peek_next_ycoordmap( tracefile, &nrows, &ncolumns,
                                      &max_column_name, &max_title_name,
                                      &methods_sz );
    if ( ierr != 0 ) {
        fprintf( errfile, "Error: %s\n", TRACE_Get_err_string( ierr ) );
        fflush( errfile );
        return NULL;
    }

    /* Prepare various arrays for C data */
    title_name    = (char *) MPIU_Malloc( max_title_name * sizeof(char) );
    column_names  = (char **) MPIU_Malloc( (ncolumns-1) * sizeof(char *) );
    for ( icol = 0; icol < ncolumns-1; icol++ )
        column_names[ icol ] = (char *) MPIU_Malloc( max_column_name
                                              * sizeof(char) );
    coordmap_max  = nrows * ncolumns;
    coordmap_base = (int *) MPIU_Malloc( coordmap_max * sizeof( int ) );
    coordmap_sz   = 0;
    coordmap_pos  = 0;

   	methods_pos  = 0;
    if ( methods_sz > 0 ) {
        methods_max  = methods_sz;
        methods_base = (int *) MPIU_Malloc( methods_max * sizeof( int ) );
    }
    else
        methods_base = NULL;

    ierr = TRACE_Get_next_ycoordmap( tracefile, title_name, column_names,
                                     &coordmap_sz, coordmap_base,
                                     &coordmap_pos, coordmap_max,
                                     &methods_sz, methods_base,
                                     &methods_pos, methods_max );
    if ( ierr != 0 ) {
        fprintf( errfile, "Error: %s\n", TRACE_Get_err_string( ierr ) );
        fflush( errfile );
	if ( coordmap_base != NULL )
	    MPIU_Free( coordmap_base );
	if ( title_name != NULL )
	    MPIU_Free( title_name );
	if ( column_names != NULL ) {
	    for ( icol = 0; icol < ncolumns-1; icol++ )
		if ( column_names[ icol ] != NULL )
		    MPIU_Free( column_names[ icol ] );
		MPIU_Free( column_names );
	}
	if ( methods_base != NULL )
	    MPIU_Free( methods_base );
        return NULL;
    }

    /* Title Label String */
    jtitle    = (*env)->NewStringUTF( env, title_name );

    /* Cache String class as a Global Reference */
    if ( cid4String == NULL ) {
        cid_local  = (*env)->FindClass( env, "java/lang/String" );
        if ( cid_local != NULL ) {
            cid4String  = (*env)->NewGlobalRef( env, cid_local );
            (*env)->DeleteLocalRef( env, cid_local );  /* avoid mem leak */
        }
    }
    /* Column Label String[ num_column - 1 ] */
    jcolnames = (*env)->NewObjectArray( env, ncolumns - 1, cid4String, NULL );
    for ( icol = 0; icol < ncolumns-1; icol++ ) {
        jcolname = (*env)->NewStringUTF( env, column_names[ icol ] );
        (*env)->SetObjectArrayElement( env, jcolnames, icol, jcolname );
    }

    /* YCoordMap element: int[] */
    if ( coordmap_pos > 0 ) {
        j_coordmap_elems = (*env)->NewIntArray( env, coordmap_sz );
        (*env)->SetIntArrayRegion( env, j_coordmap_elems,
                                   0, coordmap_sz, (jint *) coordmap_base );
    }
    else
        j_coordmap_elems = NULL;

    /* MethodID elements: int[] */
    if ( methods_base != NULL && methods_pos > 0 ) {
        jmethods = (*env)->NewIntArray( env, methods_sz );
        (*env)->SetIntArrayRegion( env, jmethods,
                                   0, methods_sz, (jint *) methods_base );
    }
    else
        jmethods = NULL;

    /* Cache YCoordMap Class's constructor as a Global Reference */
    if ( cid4YMap == NULL ) {
        cid_local  = (*env)->FindClass( env, "base/drawable/YCoordMap" );
        if ( cid_local != NULL ) {
            cid4YMap    = (*env)->NewGlobalRef( env, cid_local );
            (*env)->DeleteLocalRef( env, cid_local );  /* avoid mem leak */
            mid4NewYMap = (*env)->GetMethodID( env, cid4YMap, "<init>",
                          "(IILjava/lang/String;[Ljava/lang/String;[I[I)V" );
        }
    }
    /* Call YCoordMap's constructor */
    ycoordmap = (*env)->NewObject( env, cid4YMap, mid4NewYMap,
                                   (jint) nrows, (jint) ncolumns,
                                   jtitle, jcolnames,
                                   j_coordmap_elems, jmethods );

    /* Clean up the unused reference and free local memory */
    if ( coordmap_pos > 0 )
        (*env)->DeleteLocalRef( env, j_coordmap_elems );
    if ( coordmap_base != NULL )
        MPIU_Free( coordmap_base );

    if ( title_name != NULL )
        MPIU_Free( title_name );
    if ( column_names != NULL ) {
        for ( icol = 0; icol < ncolumns-1; icol++ )
            if ( column_names[ icol ] != NULL )
                MPIU_Free( column_names[ icol ] );
        MPIU_Free( column_names );
    }

    if ( jmethods != NULL )
        (*env)->DeleteLocalRef( env, jmethods );
    if ( methods_base != NULL )
        MPIU_Free( methods_base );

    return ycoordmap;
}

JNIEXPORT jobject JNICALL 
Java_logformat_trace_InputLog_getNextPrimitive( JNIEnv *env, jobject this )
{
    TRACE_file              tracefile;
    jlong                   filehandle;
    double                  starttime, endtime;
    int                     type_idx;
    int                     tcoord_sz, tcoord_pos, tcoord_max;
    double                 *tcoord_base;
    jdoubleArray            j_tcoords;
    int                     ycoord_sz, ycoord_pos, ycoord_max;
    int                    *ycoord_base;
    jintArray               j_ycoords;
    int                     info_sz, info_pos, info_max;
    char                   *info_base;
    jbyteArray              j_infos;
    jclass                  cid_local;
    jobject                 prime;
    int                     ierr;
    double stcoord_base[ADOUBLE_LENGTH];
    int sycoord_base[AINT_LENGTH];
    char sinfo_base[ACHAR_LENGTH];

    filehandle = (*env)->GetLongField( env, this, fid4filehandle );
    if ( filehandle == 0 ) {
        fprintf( errfile, "Java_logformat_trace_InputLog_getNextPrimitive(): "
                          "Inaccessible filehandle in Java side\n" );
        return NULL;
    }   
    tracefile  = (TRACE_file) filehandle;

    tcoord_sz  = 0;
    ycoord_sz  = 0;
    info_sz    = 0;
    ierr = TRACE_Peek_next_primitive( tracefile, &starttime, &endtime,
                                      &tcoord_sz, &ycoord_sz, &info_sz );
    if ( ierr != 0 || ( tcoord_sz <= 0 || ycoord_sz <= 0 ) ) {
        fprintf( errfile, "%s\n", TRACE_Get_err_string( ierr ) );
        fflush( errfile );
        return NULL;
    }

    tcoord_pos  = 0;
    tcoord_max  = tcoord_sz;
    if (tcoord_max > ADOUBLE_LENGTH)
	tcoord_base = (double *) MPIU_Malloc( tcoord_max * sizeof( double ) );
    else
	tcoord_base = stcoord_base;
    ycoord_pos  = 0;
    ycoord_max  = ycoord_sz;
    if (ycoord_max > AINT_LENGTH)
	ycoord_base = (int *)    MPIU_Malloc( ycoord_max * sizeof( int ) );
    else
	ycoord_base = sycoord_base;
    info_pos    = 0;
    info_max    = info_sz;
    if (info_max > AINT_LENGTH)
	info_base   = (char *)   MPIU_Malloc( info_max * sizeof( char ) );
    else
	info_base = sinfo_base;
    ierr = TRACE_Get_next_primitive( tracefile, &type_idx,
                                     &tcoord_sz, tcoord_base,
                                     &tcoord_pos, tcoord_max,
                                     &ycoord_sz, ycoord_base,
                                     &ycoord_pos, ycoord_max,
                                     &info_sz, info_base,
                                     &info_pos, info_max );
    if ( ierr != 0 || ( tcoord_pos <= 0 || ycoord_pos <= 0 ) ) {
        fprintf( errfile, "%s\n", TRACE_Get_err_string( ierr ) );
        fflush( errfile );
	if ( tcoord_base != NULL && tcoord_base != stcoord_base )
	    MPIU_Free( tcoord_base );
	if ( ycoord_base != NULL && ycoord_base != sycoord_base )
	    MPIU_Free( ycoord_base );
	if ( info_base != NULL && info_base != sinfo_base )
	    MPIU_Free( info_base );
        return NULL;
    }

    /*  Obtain primitive array references for calling Dobj's constructor */
    if ( tcoord_pos > 0 ) {
        j_tcoords = (*env)->NewDoubleArray( env, tcoord_sz );
        (*env)->SetDoubleArrayRegion( env, j_tcoords,
                                      0, tcoord_sz, tcoord_base );
    }
    else
        j_tcoords = NULL;

    if ( ycoord_pos > 0 ) {
        j_ycoords = (*env)->NewIntArray( env, ycoord_sz );
        (*env)->SetIntArrayRegion( env, j_ycoords,
                                   0, ycoord_sz, (jint *) ycoord_base );
    }
    else
        j_ycoords = NULL;

    if ( info_pos > 0 ) {
        j_infos = (*env)->NewByteArray( env, info_sz );
        (*env)->SetByteArrayRegion( env, j_infos,
                                    0, info_sz, (jbyte *) info_base );
    }
    else
        j_infos = NULL;

    /* Cache Primitive Class's constructor as a Global Reference */
    if ( cid4Prime == NULL ) {
        cid_local  = (*env)->FindClass( env, "base/drawable/Primitive" );
        if ( cid_local != NULL ) {
            cid4Prime    = (*env)->NewGlobalRef( env, cid_local );
            (*env)->DeleteLocalRef( env, cid_local );  /* avoid mem leak */
            mid4NewPrime = (*env)->GetMethodID( env, cid4Prime, "<init>",
                                                "(IDD[D[I[B)V" );
        }
    }
    /* Call Primitive's constructor */
    prime = (*env)->NewObject( env, cid4Prime, mid4NewPrime,
                               type_idx, starttime, endtime,
                               j_tcoords, j_ycoords, j_infos );

    /* Clean up the unused reference and free local memory */
    if ( tcoord_pos > 0 )
        (*env)->DeleteLocalRef( env, j_tcoords );
    if ( tcoord_base != NULL && tcoord_base != stcoord_base )
        MPIU_Free( tcoord_base );

    if ( ycoord_pos > 0 )
        (*env)->DeleteLocalRef( env, j_ycoords );
    if ( ycoord_base != NULL && ycoord_base != sycoord_base )
        MPIU_Free( ycoord_base );

    if ( info_pos > 0 )
        (*env)->DeleteLocalRef( env, j_infos );
    if ( info_base != NULL && info_base != sinfo_base )
        MPIU_Free( info_base );

    return prime;
}

JNIEXPORT jobject JNICALL 
Java_logformat_trace_InputLog_getNextComposite( JNIEnv *env, jobject this )
{
    TRACE_file              tracefile;
    jlong                   filehandle;
    double                  cmplx_starttime, cmplx_endtime;
    int                     cmplx_type_idx;
    int                     n_primitives;
    int                     cm_info_sz, cm_info_pos, cm_info_max;
    char                   *cm_info_base;
    jbyteArray              j_cm_infos;
    jobjectArray            primes;
    jobject                 prime;
    jobject                 cmplx;
    jclass                  cid_local;
    int                     idx, ierr;


    filehandle = (*env)->GetLongField( env, this, fid4filehandle );
    if ( filehandle == 0 ) {
        fprintf( errfile, "Java_logformat_trace_InputLog_getNextComposite(): "
                          "Inaccessible filehandle in Java side\n" );
        return NULL;
    }   
    tracefile  = (TRACE_file) filehandle;

    cm_info_sz = 0;
    ierr = TRACE_Peek_next_composite( tracefile,
                                      &cmplx_starttime, &cmplx_endtime,
                                      &n_primitives, &cm_info_sz );
    if ( ierr != 0 ) {
        fprintf( errfile, "%s\n", TRACE_Get_err_string( ierr ) );
        fflush( errfile );
        return NULL;
    }

    /* return when there is no primitives */
    if ( n_primitives <= 0 )
        return NULL;

    j_cm_infos = NULL;
    if ( cm_info_sz > 0 ) {
        cm_info_pos    = 0;
        cm_info_max    = cm_info_sz;
        cm_info_base   = (char *)   MPIU_Malloc( cm_info_max * sizeof( char ) );
        ierr = TRACE_Get_next_composite( tracefile, &cmplx_type_idx,
                                         &cm_info_sz, cm_info_base,
                                         &cm_info_pos, cm_info_max );
        if ( ierr != 0 ) {
            fprintf( errfile, "%s\n", TRACE_Get_err_string( ierr ) );
            fflush( errfile );
            return NULL;
        }
        if ( cm_info_pos > 0 ) {
            j_cm_infos = (*env)->NewByteArray( env, cm_info_sz );
            (*env)->SetByteArrayRegion( env, j_cm_infos,
                                        0, cm_info_sz, (jbyte *) cm_info_base );
        }
        else
            j_cm_infos = NULL;
    }

    /* Cache Primitive Class's constructor as a Global Reference */
    if ( cid4Prime == NULL ) {
        cid_local  = (*env)->FindClass( env, "base/drawable/Primitive" );
        if ( cid_local != NULL ) {
            cid4Prime    = (*env)->NewGlobalRef( env, cid_local );
            (*env)->DeleteLocalRef( env, cid_local );  /* avoid mem leak */
            mid4NewPrime = (*env)->GetMethodID( env, cid4Prime, "<init>",
                                                "(IDD[D[I[B)V" );
        }
    }

    /* Create Primitives[] */
    primes = (*env)->NewObjectArray( env, n_primitives, cid4Prime, NULL );
    if ( primes == NULL )
        return NULL;

    /*  Create the Primitive[] inside the Composite from the TRACE */
    for ( idx = 0; idx < n_primitives; idx++ ) {
         prime = Java_logformat_trace_InputLog_getNextPrimitive( env, this );
         (*env)->SetObjectArrayElement( env, primes, idx, prime ); 
    }

    /* Cache Composite Class's constructor as a Global Reference */
    if ( cid4Cmplx == NULL ) {
        cid_local  = (*env)->FindClass( env, "base/drawable/Composite" );
        if ( cid_local != NULL ) {
            cid4Cmplx    = (*env)->NewGlobalRef( env, cid_local );
            (*env)->DeleteLocalRef( env, cid_local );  /* avoid mem leak */
            mid4NewCmplx = (*env)->GetMethodID( env, cid4Cmplx, "<init>",
                                   "(IDD[Lbase/drawable/Primitive;[B)V" );
        }
    }

    /* Invoke the Composite's constructor */
    cmplx = (*env)->NewObject( env, cid4Cmplx, mid4NewCmplx,
                               cmplx_type_idx, cmplx_starttime, cmplx_endtime,
                               primes, j_cm_infos );

    /* Clean up the unused reference and free local memory */
    if ( cm_info_sz > 0 && cm_info_pos > 0 )
        (*env)->DeleteLocalRef( env, j_cm_infos );
    if ( cm_info_base != NULL )
        MPIU_Free( cm_info_base );

    return cmplx;
}

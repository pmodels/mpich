/*
 * Globus device code:          Copyright 2005 Northern Illinois University
 * Borrowed MPICH-G2 code:      Copyright 2000 Argonne National Laboratory and Northern Illinois University
 * Borrowed MPICH2 device code: Copyright 2001 Argonne National Laboratory
 *
 * XXX: INSERT POINTER TO OFFICIAL COPYRIGHT TEXT
 */


/*
 * This file contains the integration of MPICH2 segment (i.e., DATA LOOP) 
 * functions and Globus data conversion functions. 
 */


#include <stdio.h>
#include <stdlib.h>

#include <mpichconf.h>
#include <mpiimpl.h>
#include <mpid_dataloop.h>

#undef MPID_SP_VERBOSE
#undef MPID_SU_VERBOSE

/* MPID_Segment_piece_params
 *
 * This structure is used to pass function-specific parameters into our 
 * segment processing function.  This allows us to get additional parameters
 * to the functions it calls without changing the prototype.
 */
struct MPID_Segment_piece_params 
{
    globus_byte *src_buffer;
    int src_format;
    mpig_vc_t *vc;
}; /* end struct MPID_Segment_piece_params */

/* NICK: TODO get all enumerated types represented here  */ 
/* NICK: TODO handle default case as ERROR               */ 
#define F(s, d, dt) \
{ \
switch ((dt)) \
{ \
case CHAR_C:                *((char *) (d)) =           (char) (s); (d) += sizeof(char);           break; \
case SHORT_C:              *((short *) (d)) =          (short) (s); (d) += sizeof(short);          break; \
case INT_C:                  *((int *) (d)) =            (int) (s); (d) += sizeof(int);            break; \
case LONG_C:                *((long *) (d)) =           (long) (s); (d) += sizeof(long);           break; \
case LONG_LONG_C:      *((long long *) (d)) =      (long long) (s); (d) += sizeof(long long);      break; \
case FLOAT_C:              *((float *) (d)) =          (float) (s); (d) += sizeof(float);          break; \
case DOUBLE_C:            *((double *) (d)) =         (double) (s); (d) += sizeof(double);         break; \
case UCHAR_C:      *((unsigned char *) (d)) =  (unsigned char) (s); (d) += sizeof(unsigned char);  break; \
case USHORT_C:    *((unsigned short *) (d)) = (unsigned short) (s); (d) += sizeof(unsigned short); break; \
case UINT_C:        *((unsigned int *) (d)) =   (unsigned int) (s); (d) += sizeof(unsigned int);   break; \
case ULONG_C:      *((unsigned long *) (d)) =  (unsigned long) (s); (d) += sizeof(unsigned long);  break; \
default:                                                                                           break; \
} \
}

/* 
-- These from MPI 1.1 standard, "Annex A: Lanugage Binding", pp 212-213
elementary datatypes (C) MPI_CHAR MPI_SHORT MPI_INT MPI_LONG MPI_UNSIGNED_CHAR 
			 MPI_UNSIGNED_SHORT MPI_UNSIGNED MPI_UNSIGNED_LONG 
			 MPI_FLOAT MPI_DOUBLE MPI_LONG_DOUBLE MPI_BYTE 
			 MPI_PACKED 
optional datatypes (C) MPI_LONG_LONG_INT 
elementary datatypes (Fortran) MPI_INTEGER MPI_REAL MPI_DOUBLE_PRECISION 
			       MPI_COMPLEX MPI_LOGICAL MPI_CHARACTER 
			       MPI_BYTE MPI_PACKED 
                               MPI_DOUBLE_COMPLEX <-- moved to optional 
					    	      Fortran in MPI2.0
optional datatypes (Fortran) MPI_INTEGER1 MPI_INTEGER2 MPI_INTEGER4 MPI_REAL2 
			     MPI_REAL4 MPI_REAL8 
                             MPI_DOUBLE_COMPLEX <-- moved to optional 
						    Fortran in MPI2.0
datatypes for reduction functions (C) MPI_FLOAT_INT MPI_DOUBLE_INT 
				      MPI_LONG_INT MPI_2INT MPI_SHORT_INT 
				      MPI_LONG_DOUBLE_INT 
datatypes for reduction functions (Fortran) MPI_2REAL MPI_2DOUBLE_PRECISION 
                                            MPI_2INTEGER 

-- These from MPI 2.0 standard, "4.15 New Predefined Datatypes", pp 76-77
elementary datatype (C) MPI_WCHAR (corresponds to wchart_t in <stddef.h>)
                        MPI_SIGNED_CHAR (corresponds to 'signed char')
			MPI_UNSIGNED_LONG_LONG (ISO C9X cmte now includes 
			                        long long and 
						unsigned long long 
						in C Standard)

-- See also MPI 1.1, "Section 3.2 Blocking Send and Receive Operations", 
   p 18, for the following tables:

    Possible values of this argument for Fortran and the corresponding 
    Fortran types are listed below. 
    MPI datatype         Fortran datatype 
    ------------         ----------------
    MPI_INTEGER          INTEGER 
    MPI_REAL             REAL 
    MPI_DOUBLE_PRECISION DOUBLE PRECISION 
    MPI_COMPLEX          COMPLEX 
    MPI_LOGICAL          LOGICAL 
    MPI_CHARACTER        CHARACTER(1) 
    MPI_BYTE 
    MPI_PACKED 

    Possible values for this argument for C and the corresponding 
    C types are listed below. 
    MPI datatype       C datatype 
    ------------       ----------
    MPI_CHAR           signed char 
    MPI_SHORT          signed short int 
    MPI_INT            signed int 
    MPI_LONG           signed long int 
    MPI_UNSIGNED_CHAR  unsigned char 
    MPI_UNSIGNED_SHORT unsigned short int 
    MPI_UNSIGNED       unsigned int 
    MPI_UNSIGNED_LONG  unsigned long int 
    MPI_FLOAT          float 
    MPI_DOUBLE         double 
    MPI_LONG_DOUBLE    long double 
    MPI_BYTE 
    MPI_PACKED 

    The datatypes MPI_BYTE and MPI_PACKED do not correspond to a Fortran or 
    C datatype.  A value of type MPI_BYTE consists of a byte (8 binary digits). 
    A byte is uninterpreted and is different from a character.  Different 
    machines may have different representations for characters, or may use 
    more than one byte to represent characters.  On the other hand, a byte 
    has the same binary value on all machines.  The use of the type 
    MPI_PACKED is explained in Section 3.13. 

    MPI requires support of the datatypes listed above, which match the basic 
    datatypes of Fortran 77 and ANSI C.  Additional MPI datatypes should be 
    provided if the host language has additional data types: 
	MPI_LONG_LONG_INT for (64 bit) C integers declared to be of 
	                   type longlong int; 
	MPI_DOUBLE_COMPLEX for double precision complex in Fortran declared 
	                   to be of type DOUBLE COMPLEX; 
	MPI_REAL2, MPI_REAL4 and MPI_REAL8 for Fortran reals, declared to 
	                                   be of type REAL*2, REAL*4 and 
					   REAL*8, respectively; 
	MPI_INTEGER1 MPI_INTEGER2 and MPI_INTEGER4 for Fortran integers, 
	                                           declared to be of type 
						   INTEGER*1, INTEGER*2 and 
						   INTEGER*4, respectively; etc.

    -- from MPI 2.0 section 10.2.5 Additional Support for Fortran Numeric Intrinsic Types, pp 291-292

    Support for Size-specific MPI Datatypes MPI-1 provides named datatypes corresponding to optional 
    Fortran 77 numeric types that contain explicit byte lengths --- MPI_REAL4, MPI_INTEGER8, etc. 
    This section describes a mechanism that generalizes this model to support all Fortran numeric 
    intrinsic types. 
    We assume that for each typeclass (integer, real, complex) and each word size there is a unique 
    machine representation.  For every pair (typeclass, n) supported by a compiler, MPI must provide 
    a named size-specific datatype.  The name of this datatype is of the form MPI <TYPE>n in C and Fortran 
    and of the form MPI::<TYPE>n in C++ where <TYPE> is one of REAL, INTEGER and COMPLEX, and n is the 
    length in bytes of the machine representation.  This datatype locally matches all variables of type 
    (typeclass, n).  The list of names for such types includes: 

	MPI_REAL4 
	MPI_REAL8 
	MPI_REAL16 
	MPI_COMPLEX8 
	MPI_COMPLEX16 
	MPI_COMPLEX32 
	MPI_INTEGER1 
	MPI_INTEGER2 
	MPI_INTEGER4 
	MPI_INTEGER8 
	MPI_INTEGER16 

    In MPI-1 these datatypes are all optional and correspond to the optional, nonstandard declarations 
    supported by many Fortran compilers.  In MPI-2, one datatype is required for each representation 
    supported by the compiler.  To be backward compatible with the interpretation of these types in 
    MPI-1, we assume that the nonstandard declarations REAL*n, INTEGER*n, always create a variable 
    whose representation is of size n. All these datatypes are predefined. 

and finally, this from mpich2/src/include/mpi.h.in:
#define MPI_CHAR           ((MPI_Datatype)@MPI_CHAR@)
#define MPI_SIGNED_CHAR    ((MPI_Datatype)@MPI_SIGNED_CHAR@)
#define MPI_UNSIGNED_CHAR  ((MPI_Datatype)@MPI_UNSIGNED_CHAR@)
#define MPI_BYTE           ((MPI_Datatype)@MPI_BYTE@)
#define MPI_WCHAR          ((MPI_Datatype)@MPI_WCHAR@)
#define MPI_SHORT          ((MPI_Datatype)@MPI_SHORT@)
#define MPI_UNSIGNED_SHORT ((MPI_Datatype)@MPI_UNSIGNED_SHORT@)
#define MPI_INT            ((MPI_Datatype)@MPI_INT@)
#define MPI_UNSIGNED       ((MPI_Datatype)@MPI_UNSIGNED_INT@)
#define MPI_LONG           ((MPI_Datatype)@MPI_LONG@)
#define MPI_UNSIGNED_LONG  ((MPI_Datatype)@MPI_UNSIGNED_LONG@)
#define MPI_FLOAT          ((MPI_Datatype)@MPI_FLOAT@)
#define MPI_DOUBLE         ((MPI_Datatype)@MPI_DOUBLE@)
#define MPI_LONG_DOUBLE    ((MPI_Datatype)@MPI_LONG_DOUBLE@)
#define MPI_LONG_LONG_INT  ((MPI_Datatype)@MPI_LONG_LONG@)
#define MPI_UNSIGNED_LONG_LONG ((MPI_Datatype)@MPI_UNSIGNED_LONG_LONG@)
#define MPI_LONG_LONG      MPI_LONG_LONG_INT

#define MPI_PACKED         ((MPI_Datatype)@MPI_PACKED@)
#define MPI_LB             ((MPI_Datatype)@MPI_LB@)
#define MPI_UB             ((MPI_Datatype)@MPI_UB@)

   The layouts for the types MPI_DOUBLE_INT etc are simply
   struct { 
       double var;
       int    loc;
   }
   This is documented in the man pages on the various datatypes.   
#define MPI_FLOAT_INT         ((MPI_Datatype)@MPI_FLOAT_INT@)
#define MPI_DOUBLE_INT        ((MPI_Datatype)@MPI_DOUBLE_INT@)
#define MPI_LONG_INT          ((MPI_Datatype)@MPI_LONG_INT@)
#define MPI_SHORT_INT         ((MPI_Datatype)@MPI_SHORT_INT@)
#define MPI_2INT              ((MPI_Datatype)@MPI_2INT@)
#define MPI_LONG_DOUBLE_INT   ((MPI_Datatype)@MPI_LONG_DOUBLE_INT@)

--Fortran types--
#define MPI_COMPLEX           ((MPI_Datatype)@MPI_COMPLEX@)
#define MPI_DOUBLE_COMPLEX    ((MPI_Datatype)@MPI_DOUBLE_COMPLEX@)
#define MPI_LOGICAL           ((MPI_Datatype)@MPI_LOGICAL@)
#define MPI_REAL              ((MPI_Datatype)@MPI_REAL@)
#define MPI_DOUBLE_PRECISION  ((MPI_Datatype)@MPI_DOUBLE_PRECISION@)
#define MPI_INTEGER           ((MPI_Datatype)@MPI_INTEGER@)
#define MPI_2INTEGER          ((MPI_Datatype)@MPI_2INTEGER@)
#define MPI_2COMPLEX          ((MPI_Datatype)@MPI_2COMPLEX@)
#define MPI_2DOUBLE_COMPLEX   ((MPI_Datatype)@MPI_2DOUBLE_COMPLEX@)
#define MPI_2REAL             ((MPI_Datatype)@MPI_2REAL@)
#define MPI_2DOUBLE_PRECISION ((MPI_Datatype)@MPI_2DOUBLE_PRECISION@)
#define MPI_CHARACTER         ((MPI_Datatype)@MPI_CHARACTER@)

--Size-specific types (see MPI-2, 10.2.5)--
#define MPI_REAL4             ((MPI_Datatype)@MPI_REAL4@)
#define MPI_REAL8             ((MPI_Datatype)@MPI_REAL8@)
#define MPI_REAL16            ((MPI_Datatype)@MPI_REAL16@)
#define MPI_COMPLEX8          ((MPI_Datatype)@MPI_COMPLEX8@)
#define MPI_COMPLEX16         ((MPI_Datatype)@MPI_COMPLEX16@)
#define MPI_COMPLEX32         ((MPI_Datatype)@MPI_COMPLEX32@)
#define MPI_INTEGER1          ((MPI_Datatype)@MPI_INTEGER1@)
#define MPI_INTEGER2          ((MPI_Datatype)@MPI_INTEGER2@)
#define MPI_INTEGER4          ((MPI_Datatype)@MPI_INTEGER4@)
#define MPI_INTEGER8          ((MPI_Datatype)@MPI_INTEGER8@)
#define MPI_INTEGER16         ((MPI_Datatype)@MPI_INTEGER16@)
-- 
*/

/* 
    From MPI 1.1 standard, section 3.3.1 Type matching rules:
    ----------------------------------------------
	One can think of message transfer as consisting of the following three phases.

	    1. Data is pulled out of the send buffer and a message is assembled. 
	    2. A message is transferred from sender to receiver. 
	    3. Data is pulled from the incoming message and disassembled into the 
	       receive buffer.

	Type matching has to be observed at each of these three phases: 
	    The type of each variable in the sender buffer has to match the type specified 
		for that entry by the send operation; 
	    the type specified by the send operation has to match the type specified by the 
		receive operation; and 
	    the type of each variable in the receive buffer has to match the type specified 
		for that entry by the receive operation. 
	A program that fails to observe these three rules is erroneous.

	To define type matching more precisely, we need to deal with two issues: matching of 
	types of the host language with types specified in communication operations; and matching 
	of types at sender and receiver.

	The types of a send and receive match (phase two) if both operations use 
	identical names.  That is, MPI_INTEGER matches MPI_INTEGER, MPI_REAL matches 
	MPI_REAL, and so on.  There is one exception to this rule, discussed in 
	Sec. Pack and unpack, the type MPI_PACKED can match any other type.
    ----------------------------------------------
    This means that (ignoring MPI_PACKED for the moment) the application must make
    sure that the MPI datatype specified in the send matches exactly to the MPI
    datatype specified on the receive.  
*/

/* 
    From MPI 1.1 standard, section 3.3.2 Data Conversion:
    ----------------------------------------------
	One of the goals of MPI is to support parallel computations across 
	heterogeneous environments.  Communication in a heterogeneous environment 
	may require data conversions.  We use the following terminology.

	type conversion
	    changes the datatype of a value, e.g., by rounding a REAL to an INTEGER.

	representation conversion
	    changes the binary representation of a value, e.g., from Hex floating point 
	    to IEEE floating point.

	The type matching rules imply that MPI communication never entails type conversion. 
	On the other hand, MPI requires that a representation conversion be performed when 
	a typed value is transferred across environments that use different representations 
	for the datatype of this value.  MPI does not specify rules for representation 
	conversion.  Such conversion is expected to preserve integer, logical or character 
	values, and to convert a floating point value to the nearest value that can be 
	represented on the target system.  

	Overflow and underflow exceptions may occur during floating point conversions. 
	Conversion of integers or characters may also lead to exceptions when a value that 
	can be represented in one system cannot be represented in the other system.   An 
	exception occurring during representation conversion results in a failure of the 
	communication.  An error occurs either in the send operation, or the receive operation, 
	or both.

	If a value sent in a message is untyped (i.e., of type MPI_BYTE), then the binary 
	representation of the byte stored at the receiver is identical to the binary representation 
	of the byte loaded at the sender.  This holds true, whether sender and receiver run in the 
	same or in distinct environments.  No representation conversion is required. (Note that 
	representation conversion may occur when values of type MPI_CHARACTER or MPI_CHAR are 
	transferred, for example, from an EBCDIC encoding to an ASCII encoding.)
    ----------------------------------------------
    This means 
	(1) reiterates that the app must match types specified in send and recv,
        (2) if underflow or overflow occurs when converting floats or ints then
	    we are supposed to treat this as an error on the send, receive, or both, and
	(3) we never convert MPI_BYTE.
*/

/*******************/
/* LOCAL FUNCTIONS */
/*******************/

/* 
  NOTE: this function does _NOT_ return true for a pair NON-ident 
	C datatypes for reduction functions defined in MPI 1.1
        MPI_FLOAT_INT, MPI_DOUBLE_INT, MPI_LONG_INT, MPI_SHORT_INT, 
	and MPI_LONG_DOUBLE_INT
*/
MPIG_Static int MPID_Segment_datatype_is_two_ident_language_types(int t) 
{
    return (t == MPI_2INT 
	|| t == MPI_2REAL 
	|| t == MPI_2DOUBLE_PRECISION 
	|| t == MPI_2INTEGER 
	|| t == MPI_COMPLEX 
#if defined(MPI_COMPLEX8) && (MPI_COMPLEX8 != MPI_DATATYPE_NULL)
	|| t == MPI_COMPLEX8 
#endif 
#if defined(MPI_COMPLEX16) && (MPI_COMPLEX16 != MPI_DATATYPE_NULL)
	|| t == MPI_COMPLEX16 
#endif 
#if defined(MPI_COMPLEX32) && (MPI_COMPLEX32 != MPI_DATATYPE_NULL)
	|| t == MPI_COMPLEX32
#endif 
#if defined(MPI_DOUBLE_COMPLEX) && (MPI_DOUBLE_COMPLEX != MPI_DATATYPE_NULL) 
	|| t == MPI_DOUBLE_COMPLEX 
#endif 
	) ;
} /* end MPID_Segment_datatype_is_two_ident_language_types() */

MPIG_STATIC int MPID_Segment_local_size_of_ctype(enum c_datatype ct)
{
    int rc;
    /* MPIDI_STATE_DECL(MPID_STATE_MPID_SEGMENT_CONTIG_UNPACK_EXTERNAL32_TO_BUF); */
    /* MPIDI_FUNC_ENTER(MPID_STATE_MPID_SEGMENT_CONTIG_UNPACK_EXTERNAL32_TO_BUF); */

    /* NICK: TODO get all enumerated ctypes represented here */
    switch (ct)
    {
	case CHAR_C:      rc = sizeof(char);           break;
	case SHORT_C:     rc = sizeof(short);          break;
	case INT_C:       rc = sizeof(int);            break;
	case LONG_C:      rc = sizeof(long);           break;
	case LONG_LONG_C: rc = sizeof(long long);      break;
	case FLOAT_C:     rc = sizeof(float);          break;
	case DOUBLE_C:    rc = sizeof(double);         break;
	case UCHAR_C:     rc = sizeof(unsigned char);  break;
	case USHORT_C:    rc = sizeof(unsigned short); break;
	case UINT_C:      rc = sizeof(unsigned int);   break;
	case ULONG_C:     rc = sizeof(unsigned long);  break;
	default: /* NICK: TODO */ break;
    } /* end switch() */

    /* MPIDI_FUNC_EXIT(MPID_STATE_MPID_SEGMENT_CONTIG_UNPACK_EXTERNAL32_TO_BUF); */

    return rc;

} /* end local_size_of_ctype() */

/* 
 * data conversion routines available from Globus ... see globus_dc.h 
 *  globus_dc_get_{byte, char, short, int, long, long_long, float, double}
 *  globus_dc_get_u_{char, short, int, long}
 */

MPIG_STATIC int MPID_Segment_contig_unpack_remote_format_to_buf(DLOOP_Offset *blocks_p, /* count */
							    DLOOP_Type el_type,
							    DLOOP_Offset rel_off,  /* dest */
							    void *bufp,            /* dest */
							    void *v_paramp)
{
    struct MPID_Segment_piece_params *p = v_paramp;
    globus_byte_t **s; /* src        */
    char *d;           /* dest       */
    int f;             /* src format */
    int c;             /* count      */
    mpig_vc_t vc;
    /* MPIDI_STATE_DECL(MPID_STATE_MPID_SEGMENT_CONTIG_UNPACK_EXTERNAL32_TO_BUF); */
    /* MPIDI_FUNC_ENTER(MPID_STATE_MPID_SEGMENT_CONTIG_UNPACK_EXTERNAL32_TO_BUF); */

    MPIU_Assert(paramp);
    s  = &(p->src_buffer);
    f  = p->src_format;
    d  = ((char *) bufp) + rel_off;
    c  = *blocks_p;
    vc = p->vc;

    switch (el_type)
    {
      /* NICK: questionable types found in mpi.h.in: MPI_LB, MPI_UB */
      /*********************************************/
      /* Elementary C datatypes defined in MPI 1.1 */
      /*********************************************/
      case MPI_CHAR:           globus_dc_get_char   (s,           (char *) d, c, f); break;
      case MPI_SHORT:          globus_dc_get_short  (s,          (short *) d, c, f); break;
      case MPI_INT:            globus_dc_get_int    (s,            (int *) d, c, f); break;
      case MPI_LONG:           globus_dc_get_long   (s,           (long *) d, c, f); break;
      case MPI_UNSIGNED_CHAR:  globus_dc_get_u_char (s,  (unsigned char *) d, c, f); break;
      case MPI_UNSIGNED_SHORT: globus_dc_get_u_short(s, (unsigned short *) d, c, f); break;
      case MPI_UNSIGNED:       globus_dc_get_u_int  (s,   (unsigned int *) d, c, f); break;
      case MPI_UNSIGNED_LONG:  globus_dc_get_u_long (s,  (unsigned long *) d, c, f); break;
      case MPI_FLOAT:          globus_dc_get_float  (s,          (float *) d, c, f); break;
      case MPI_DOUBLE:         globus_dc_get_double (s,         (double *) d, c, f); break;
      case MPI_LONG_DOUBLE: /* NICK: TODO not supported by Globus */                 break;
      case MPI_PACKED: /* BOTH OF THESE MUST BE A MEMCPY, (i.e., NOT CONVERTED) */
      case MPI_BYTE:           memcpy((void *) d, (void *) *s, c); (*s) += c; break;

      /*******************************************/
      /* Optional C datatypes defined in MPI 1.1 */
      /*******************************************/
#if defined(MPI_LONG_LONG_INT) && (MPI_LONG_LONG_INT != MPI_DATATYPE_NULL)
      case MPI_LONG_LONG_INT: globus_dc_get_long_long(s, (long long *) d, c, f); break;
#endif

      /**********************************************************/
      /* C datatypes for reduction functions defined in MPI 1.1 */
      /**********************************************************/

      /* 
         MPI_{FLOAT,DOUBLE,LONG,SHORT,LONG_DOUBLE}_INT are all 
         constructed during intialization as derived datatypes
	 and are therefore decomposed by the segment code
	 and eventually handled by this function as elementary
	 datatypes ... in other words, this function does NOT
	 need to handle those cases explicitly.
      */
      case MPI_2INT: globus_dc_get_int(s, (int *) d, 2*c, f); break;

      /*************************************************/
      /* New Elementary C datatypes defined in MPI 2.0 */
      /*************************************************/
      case MPI_WCHAR:              /* NICK: TODO */ break;
      case MPI_SIGNED_CHAR:        /* NICK: TODO */ break;
      case MPI_UNSIGNED_LONG_LONG: /* NICK: TODO */ break;

#ifdef HAVE_FORTRAN_BINDING
      /*
	The Globus data conversion routines work only on C datatypes
	and so trying to use them on Fortran datatypes is a little
	tricky.  

	First, we need to map the Fortran datatype to its equivalent 
	C-type from the *sending* side (e.g., learn that an MPI_REAL 
	on the sending side is equivalent to a C-type float on the 
	sending side.  This will determine which Globus data conversion 
	routine to call (e.g., if the C-type is a float on the sending 
	side then we will call globus_dc_get_float()).  This is required 
	because we pass the format to the Globus data conversion routines
	and so if we call globus_dc_get_float() and pass the format then 
	globus_dc_get_float() will use the format to figure out how many 
	bytes a 'float' is on the sending side.

	Second, we need to take the size of the Fortran MPI datatype
	on the receiving side and compare that to the size on the
	receiving side of the C-type discovered in the first step.
	For example, if the MPI datatype is MPI_REAL and we learn
	that an MPI_REAL on the sending side corresponds to a 
	C-type float on the sending side then we must compare the 
	size of a C-type float on the receiving side to the
	size of an MPI_REAL on the receiving side.

              Sending side    |     Receiving side
              ----------------+--------------------
	                      |     MPI datatype
			      |       |    |   |
               C-type <-------+-------+    |   +--------+
                 |            |            V            |
                 +------------+---> local  local     (if the two local
                              |     size   size       sizes are NOT equal)
			      |                         |
			      |                         v
			      |                       C-type

	If the two sizes match then it is safe to make ONE call 
	to the Globus data conversion routine passing a pointer to
	the source, a pointer to the destination, and the count
	(e.g., converting a source float to a source float) thus
	converting the entire buffer (i.e., all 'count' elements)
	in a single function call.

	If, on the other hand, the two sizes do NOT match then 
	we must create a temporary C variable of the same
	C-type as the C-type representation of the Fortran
	datatype on the sending side (i.e., what we discovered
	in the first step) and learn the C-type equivlaent
	of the MPI Fortran datatype on the receiving side
	(e.g., an MPI_REAL is a C-type float on the receiving
	side).  We then convert the entire buffer by converting
	the elements one at a time into the temporary variable
	and then assigning the temp variable to the destination
	buffer with the appropriate cast.  For example, if an
	MPI_REAL is a C-type double (which forces us to call 
	globus_dc_get_double()) that is 8 bytes on the sending side 
	but a C-type float having only 4 bytes on the receiving side 
	then we would have to convert the entire buffer like this: 
	    double *s = src_buff;
	    double temp;
	    float *d = dest_buff;
	    for (i = 0; i < count; i ++) 
	    {
		globus_dc_get_double(s, &temp, 1, format);
		*d = (float) temp;
		d ++;
	    }

	This is all necessary because the Globus data conversion 
	routines expect the destination buffer to be a C-type
	that matches the function (e.g., globus_dc_get_float()
	requires a float destination buffer).

	NOTE: everything described above applies to both
	      integer and floating point datatypes.
      */

      /***************************************************/
      /* Elementary FORTRAN datatypes defined in MPI 1.1 */
      /***************************************************/
      case MPI_LOGICAL:          case MPI_INTEGER: case MPI_REAL:
      case MPI_DOUBLE_PRECISION: case MPI_COMPLEX: case MPI_CHARACTER:
      /* MPI_{BYTE, PACKED} handled above in C cases */
      /*************************************************/
      /* Optional FORTRAN datatypes defined in MPI 1.1 */
      /*************************************************/
#if defined(MPI_DOUBLE_COMPLEX) && (MPI_DOUBLE_COMPLEX != MPI_DATATYPE_NULL)
      case MPI_DOUBLE_COMPLEX:
#endif
#if defined(MPI_INTEGER1) && (MPI_INTEGER1 != MPI_DATATYPE_NULL)
      case MPI_INTEGER1:
#endif
#if defined(MPI_INTEGER2) && (MPI_INTEGER2 != MPI_DATATYPE_NULL)
      case MPI_INTEGER2:
#endif
#if defined(MPI_INTEGER4) && (MPI_INTEGER4 != MPI_DATATYPE_NULL)
      case MPI_INTEGER4:
#endif
#if defined(MPI_INTEGER8) && (MPI_INTEGER8 != MPI_DATATYPE_NULL)
      case MPI_INTEGER8:
#endif
#if defined(MPI_INTEGER16) && (MPI_INTEGER16 != MPI_DATATYPE_NULL)
      case MPI_INTEGER16
#endif
#if defined(MPI_REAL2) && (MPI_REAL2 != MPI_DATATYPE_NULL)
      case MPI_REAL2:
#endif
      /************************************************************/
      /* Additional optional FORTRAN datatypes defined in MPI 2.0 */
      /************************************************************/
#if defined(MPI_REAL4) && (MPI_REAL4 != MPI_DATATYPE_NULL)
      case MPI_REAL4:
#endif
#if defined(MPI_REAL8) && (MPI_REAL8 != MPI_DATATYPE_NULL)
      case MPI_REAL8:
#endif
#if defined(MPI_REAL16) && (MPI_REAL16 != MPI_DATATYPE_NULL)
      case MPI_REAL16:
#endif
#if defined(MPI_COMPLEX8) && (MPI_COMPLEX8 != MPI_DATATYPE_NULL)
      case MPI_COMPLEX8:
#endif
#if defined(MPI_COMPLEX16) && (MPI_COMPLEX16 != MPI_DATATYPE_NULL)
      case MPI_COMPLEX16:
#endif
#if defined(MPI_COMPLEX32) && (MPI_COMPLEX32 != MPI_DATATYPE_NULL)
      case MPI_COMPLEX32:
#endif
      /****************************************************************/
      /* FORTRAN datatypes for reduction functions defined in MPI 1.1 */
      /****************************************************************/
      case MPI_2REAL: case MPI_2DOUBLE_PRECISION: case MPI_2INTEGER:

      /* *ALL* the Fortran datatype cases are handled by the code in this block */
      { 

        enum c_datatype remote_ctype    = NEED_FROM_BRIAN_mpi_to_remote_c_datatype(el_type, vc);
        int local_csize_of_remote_ctype = MPID_Segment_local_size_of_ctype(remote_ctype);

	if (local_csize_of_remote_ctype == MPID_Datatype_get_basic_size(el_type))
	{
	    /* OK to receive directly into destination buffer */
	    int m = (MPID_Segment_datatype_is_two_ident_language_types(el_type) ? 2 : 1);

	    /* NICK: TODO get all enumerated types represented here */
	    switch (remote_ctype)
	    {
	      case CHAR_C:      globus_dc_get_char     (s,           (char *) d, m*c, f); break;
	      case SHORT_C:     globus_dc_get_short    (s,          (short *) d, m*c, f); break;
	      case INT_C:       globus_dc_get_int      (s,            (int *) d, m*c, f); break;
	      case LONG_C:      globus_dc_get_long     (s,           (long *) d, m*c, f); break;
	      case LONG_LONG_C: globus_dc_get_long_long(s,      (long long *) d, m*c, f); break;
	      case FLOAT_C:     globus_dc_get_float    (s,          (float *) d, m*c, f); break;
	      case DOUBLE_C:    globus_dc_get_double   (s,         (double *) d, m*c, f); break;
	      case UCHAR_C:     globus_dc_get_u_char   (s,  (unsigned char *) d, m*c, f); break;
	      case USHORT_C:    globus_dc_get_u_short  (s, (unsigned short *) d, m*c, f); break;
	      case UINT_C:      globus_dc_get_u_int    (s,   (unsinged int *) d, m*c, f); break;
	      case ULONG_C:     globus_dc_get_u_long   (s,  (unsinged long *) d, m*c, f); break;
	      default:          /* NICK: TODO */                        break;
	    } /* end switch() */
	}
	else
	{
	    /* 
	       --NOT-- OK to receive directly into destination buffer  ...
	       ... need to convert one at a time
	    */
	    enum c_datatype lct = NEED_FROM_BRIAN_mpi_to_local_c_datatype(el_type);
	    int i;
	    /* single-instance temp buffs */
	    char tc;
	    short ts;
	    int ti;
	    long tl;
	    long long tll;
	    float tf;
	    double tdb;
	    unsigned char tuc;
	    unsigned short tus;
	    unsigned int tui;
	    unsigned long tul;

	    for (i = 0; i < c; i ++)
	    {
		/* NICK: TODO get all enumerated types represented here */
		switch (remote_ctype)
		{
		  case CHAR_C:      globus_dc_get_char     (s, &tc,  1, f); F(tc,  d, lct); break;
		  case SHORT_C:     globus_dc_get_short    (s, &ts,  1, f); F(ts,  d, lct); break;
		  case INT_C:       globus_dc_get_int      (s, &ti,  1, f); F(ti,  d, lct); break;
		  case LONG_C:      globus_dc_get_long     (s, &tl,  1, f); F(tl,  d, lct); break;
		  case LONG_LONG_C: globus_dc_get_long_long(s, &tll, 1, f); F(tll, d, lct); break;
		  case FLOAT_C:     globus_dc_get_float    (s, &tf,  1, f); F(tf,  d, lct); break;
		  case DOUBLE_C:    globus_dc_get_double   (s, &tdb, 1, f); F(tdb, d, lct); break;
		  case UCHAR_C:     globus_dc_get_u_char   (s, &tuc, 1, f); F(tuc, d, lct); break;
		  case USHORT_C:    globus_dc_get_u_short  (s, &tus, 1, f); F(tus, d, lct); break;
		  case UINT_C:      globus_dc_get_u_int    (s, &tui, 1, f); F(tui, d, lct); break;
		  case ULONG_C:     globus_dc_get_u_long   (s, &tul, 1, f); F(tul, d, lct); break;
		  default:          /* NICK: TODO */                        break;
		} /* end switch() */

		if (MPID_Segment_datatype_is_two_ident_language_types(el_type))
		{
		    /* gotta do it one more time for this iteration */
		    switch (remote_ctype)
		    {
		      case CHAR_C:      globus_dc_get_char     (s, &tc,  1, f); F(tc,  d, lct); break;
		      case SHORT_C:     globus_dc_get_short    (s, &ts,  1, f); F(ts,  d, lct); break;
		      case INT_C:       globus_dc_get_int      (s, &ti,  1, f); F(ti,  d, lct); break;
		      case LONG_C:      globus_dc_get_long     (s, &tl,  1, f); F(tl,  d, lct); break;
		      case LONG_LONG_C: globus_dc_get_long_long(s, &tll, 1, f); F(tll, d, lct); break;
		      case FLOAT_C:     globus_dc_get_float    (s, &tf,  1, f); F(tf,  d, lct); break;
		      case DOUBLE_C:    globus_dc_get_double   (s, &tdb, 1, f); F(tdb, d, lct); break;
		      case UCHAR_C:     globus_dc_get_u_char   (s, &tuc, 1, f); F(tuc, d, lct); break;
		      case USHORT_C:    globus_dc_get_u_short  (s, &tus, 1, f); F(tus, d, lct); break;
		      case UINT_C:      globus_dc_get_u_int    (s, &tui, 1, f); F(tui, d, lct); break;
		      case ULONG_C:     globus_dc_get_u_long   (s, &tul, 1, f); F(tul, d, lct); break;
		      default:          /* NICK: TODO */                        break;
		    } /* end switch() */
		} /* endif */
	    } /* endfor */
	} /* endif */
      } /* end all Fortran dataype cases */
      break;
#endif
      default: /* NICK: TODO */ break;
    } /* end switch() */

    /* MPIDI_FUNC_EXIT(MPID_STATE_MPID_SEGMENT_CONTIG_UNPACK_EXTERNAL32_TO_BUF); */
    return 0;

} /* end MPID_Segment_contig_unpack_remote_format_to_buf() */

/* 
 * sizeof routines available from Globus ... see globus_dc.h 
 *  globus_dc_sizeof_remote_{byte, char, short, int, long, long_long, float, double}
 *  globus_dc_sizeof_remote_u_{char, short, int, long}
 */

MPIG_STATIC MPI_Aint MPIDI_Datatype_get_basic_size_globus_dc(MPI_Datatype el_type, void *v_paramp)
{
    struct MPID_Segment_piece_params *p = (struct MPID_Segment_piece_params *) v_paramp;
    int c = (MPID_Segment_datatype_is_two_ident_language_types(el_type) ? 2 : 1);
    MPI_Aint rc; /* element size */

    /* MPIDI_STATE_DECL(MPID_STATE_MPID_SEGMENT_CONTIG_UNPACK_EXTERNAL32_TO_BUF); */
    /* MPIDI_FUNC_ENTER(MPID_STATE_MPID_SEGMENT_CONTIG_UNPACK_EXTERNAL32_TO_BUF); */

    MPIU_Assert(p);
    f = p->src_format;










/* 
 * sizeof routines available from Globus ... see globus_dc.h 
 *  globus_dc_sizeof_remote_{byte, char, short, int, long, long_long, float, double}
 *  globus_dc_sizeof_remote_u_{char, short, int, long}
 */
    switch (el_type)
    {
      /*********************************************/
      /* Elementary C datatypes defined in MPI 1.1 */
      /*********************************************/
      case MPI_CHAR:           rc = (MPI_Aint) globus_dc_sizeof_remote_char(c,f);    break;
      case MPI_SHORT:          rc = (MPI_Aint) globus_dc_sizeof_remote_short(c,f);   break;
      case MPI_INT:            rc = (MPI_Aint) globus_dc_sizeof_remote_int(c,f);     break;
      case MPI_LONG:           rc = (MPI_Aint) globus_dc_sizeof_remote_long(c,f);    break;
      case MPI_UNSIGNED_CHAR:  rc = (MPI_Aint) globus_dc_sizeof_remote_u_char(c,f);  break;
      case MPI_UNSIGNED_SHORT: rc = (MPI_Aint) globus_dc_sizeof_remote_u_short(c,f); break;
      case MPI_UNSIGNED:       rc = (MPI_Aint) globus_dc_sizeof_remote_u_int(c,f);   break;
      case MPI_UNSIGNED_LONG:  rc = (MPI_Aint) globus_dc_sizeof_remote_u_long(c,f);  break;
      case MPI_FLOAT:          rc = (MPI_Aint) globus_dc_sizeof_remote_float(c,f);   break;
      case MPI_DOUBLE:         rc = (MPI_Aint) globus_dc_sizeof_remote_double(c,f);  break;
      case MPI_LONG_DOUBLE: /* NICK: TODO not supported by Globus */                 break;
      case MPI_PACKED: /* BOTH OF THESE MUST BE A MEMCPY, (i.e., NOT CONVERTED) */
      case MPI_BYTE:           rc = (MPI_Aint) 1;                                    break;

      /*******************************************/
      /* Optional C datatypes defined in MPI 1.1 */
      /*******************************************/
#if defined(MPI_LONG_LONG_INT) && (MPI_LONG_LONG_INT != MPI_DATATYPE_NULL)
      case MPI_LONG_LONG_INT: rc = (MPI_Aint) globus_dc_sizeof_remote_long_long(c,f); break;
#endif

      /**********************************************************/
      /* C datatypes for reduction functions defined in MPI 1.1 */
      /**********************************************************/

      /* 
         MPI_{FLOAT,DOUBLE,LONG,SHORT,LONG_DOUBLE}_INT are all 
         constructed during intialization as derived datatypes
	 and are therefore decomposed by the segment code
	 and eventually handled by this function as elementary
	 datatypes ... in other words, this function does NOT
	 need to handle those cases explicitly.
      */
      case MPI_2INT: rc = (MPI_Aint) globus_dc_sizeof_remote_int(c,f); break;

      /*************************************************/
      /* New Elementary C datatypes defined in MPI 2.0 */
      /*************************************************/
      case MPI_WCHAR:              /* NICK: TODO */ break;
      case MPI_SIGNED_CHAR:        /* NICK: TODO */ break;
      case MPI_UNSIGNED_LONG_LONG: /* NICK: TODO */ break;

#ifdef HAVE_FORTRAN_BINDING
      /***************************************************/
      /* Elementary FORTRAN datatypes defined in MPI 1.1 */
      /***************************************************/
      case MPI_LOGICAL:          case MPI_INTEGER: case MPI_REAL:
      case MPI_DOUBLE_PRECISION: case MPI_COMPLEX: case MPI_CHARACTER:
      /* MPI_{BYTE, PACKED} handled above in C cases */
      /*************************************************/
      /* Optional FORTRAN datatypes defined in MPI 1.1 */
      /*************************************************/
#if defined(MPI_DOUBLE_COMPLEX) && (MPI_DOUBLE_COMPLEX != MPI_DATATYPE_NULL)
      case MPI_DOUBLE_COMPLEX:
#endif
#if defined(MPI_INTEGER1) && (MPI_INTEGER1 != MPI_DATATYPE_NULL)
      case MPI_INTEGER1:
#endif
#if defined(MPI_INTEGER2) && (MPI_INTEGER2 != MPI_DATATYPE_NULL)
      case MPI_INTEGER2:
#endif
#if defined(MPI_INTEGER4) && (MPI_INTEGER4 != MPI_DATATYPE_NULL)
      case MPI_INTEGER4:
#endif
#if defined(MPI_INTEGER8) && (MPI_INTEGER8 != MPI_DATATYPE_NULL)
      case MPI_INTEGER8:
#endif
#if defined(MPI_INTEGER16) && (MPI_INTEGER16 != MPI_DATATYPE_NULL)
      case MPI_INTEGER16
#endif
#if defined(MPI_REAL2) && (MPI_REAL2 != MPI_DATATYPE_NULL)
      case MPI_REAL2:
#endif
      /************************************************************/
      /* Additional optional FORTRAN datatypes defined in MPI 2.0 */
      /************************************************************/
#if defined(MPI_REAL4) && (MPI_REAL4 != MPI_DATATYPE_NULL)
      case MPI_REAL4:
#endif
#if defined(MPI_REAL8) && (MPI_REAL8 != MPI_DATATYPE_NULL)
      case MPI_REAL8:
#endif
#if defined(MPI_REAL16) && (MPI_REAL16 != MPI_DATATYPE_NULL)
      case MPI_REAL16:
#endif
#if defined(MPI_COMPLEX8) && (MPI_COMPLEX8 != MPI_DATATYPE_NULL)
      case MPI_COMPLEX8:
#endif
#if defined(MPI_COMPLEX16) && (MPI_COMPLEX16 != MPI_DATATYPE_NULL)
      case MPI_COMPLEX16:
#endif
#if defined(MPI_COMPLEX32) && (MPI_COMPLEX32 != MPI_DATATYPE_NULL)
      case MPI_COMPLEX32:
#endif
      /****************************************************************/
      /* FORTRAN datatypes for reduction functions defined in MPI 1.1 */
      /****************************************************************/
      case MPI_2REAL: case MPI_2DOUBLE_PRECISION: case MPI_2INTEGER:

      /* *ALL* the Fortran datatype cases are handled by the code in this block */
      { 
	enum c_datatype remote_ctype NEED_FROM_BRIAN_mpi_to_remote_c_datatype(el_type, p->vc);

	/* NICK: TODO get all enumerated types represented here */
	switch (remote_ctype)
	{
	  case CHAR_C:      rc = (MPI_Aint) globus_dc_sizeof_remote_char(c,f);      break;
	  case SHORT_C:     rc = (MPI_Aint) globus_dc_sizeof_remote_short(c,f);     break;
	  case INT_C:       rc = (MPI_Aint) globus_dc_sizeof_remote_int(c,f);       break;
	  case LONG_C:      rc = (MPI_Aint) globus_dc_sizeof_remote_long(c,f);      break;
	  case LONG_LONG_C: rc = (MPI_Aint) globus_dc_sizeof_remote_long_long(c,f); break;
	  case FLOAT_C:     rc = (MPI_Aint) globus_dc_sizeof_remote_float(c,f);     break;
	  case DOUBLE_C:    rc = (MPI_Aint) globus_dc_sizeof_remote_double(c,f);    break;
	  case UCHAR_C:     rc = (MPI_Aint) globus_dc_sizeof_remote_u_char(c,f);    break;
	  case USHORT_C:    rc = (MPI_Aint) globus_dc_sizeof_remote_u_short(c,f);   break;
	  case UINT_C:      rc = (MPI_Aint) globus_dc_sizeof_remote_u_int(c,f);     break;
	  case ULONG_C:     rc = (MPI_Aint) globus_dc_sizeof_remote_u_long(c,f);    break;
	  default:          /* NICK: TODO */                                        break;
	} /* end switch() */
      } /* end all Fortran dataype cases */
      break;
#endif
      default: /* NICK: TODO */ break;
    } /* end switch() */

    /* MPIDI_FUNC_EXIT(MPID_STATE_MPID_SEGMENT_CONTIG_UNPACK_EXTERNAL32_TO_BUF); */

    return rc;

} /* end MPIDI_Datatype_get_basic_size_globus_dc() */

/********************************/
/* EXTERNALLY VISIBLE FUNCTIONS */
/********************************/

/* this is the one and only function we're here for :-) */
void MPID_Segment_unpack_globus_dc(struct DLOOP_Segment *segp,  /* dest */
				    DLOOP_Offset first,         /* src  */
				    DLOOP_Offset *lastp,        /* src  */
				    DLOOP_Buffer unpack_buffer, /* src  */
				    int src_format, 
				    mpig_vc_t *vc)
{
    struct MPID_Segment_piece_params unpack_params;
    /* MPIDI_STATE_DECL(MPID_STATE_MPID_SEGMENT_UNPACK_EXTERNAL); */
    
    /* MPIDI_FUNC_ENTER(MPID_STATE_MPID_SEGMENT_UNPACK_EXTERNAL); */

    unpack_params.src_buffer = unpack_buffer;
    unpack_params.src_format = src_format;
    unpack_params.vc         = vc;

    MPID_Segment_manipulate(segp,
			    first,
			    lastp,
			    /* function pointer we supply to convert contig data */
			    MPID_Segment_contig_unpack_remote_format_to_buf,
			    /* these next three are function pointers for vector,
			       block indexed, and index, respectively.  for each
			       one we have the option to supply our own routine
			       OR simply pass NULL in which case the segment
			       code will 'unwrap" each of the three down to 
			       their 'contig' elementary types and call our 
			       contig function above.  
			       we would choose to write our own functions for 
			       optimization (eliminate calling up and down the function 
			       stack multiple times) but that's probably not worth
			       the time to write ... the savings is miniscule compared
			       to the WAN/LAN latency and data conversion on these types.
			    */
                            NULL,  /* vector      */
			    NULL,  /* block index */
			    NULL,  /* index       */
                            /* 
			       func ptr we provide to return basic sizes of in the src buffer
			       (i.e., what the segment does NOT point to).  in our case, data sizes 
			       of MPI datatypes on the sending side.
			    */
                            MPIDI_Datatype_get_basic_size_globus_dc, 
			    &unpack_params);

    /* MPIDI_FUNC_EXIT(MPID_STATE_MPID_SEGMENT_UNPACK_EXTERNAL); */
    return;

} /* end MPID_Segment_unpack_globus_dc() */

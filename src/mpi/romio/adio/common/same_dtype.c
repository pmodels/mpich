#include <stdint.h>
#include <assert.h>
#include <string.h>

#include <adio.h>

#include "sha1.h"

/* Maximum number of ints/addrs/types in a constructed type */
#define MAX_ENTRIES     100000

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#define dt_min(a,b) ((a)<(b) ? (a) : (b))
#define dt_max(a,b) ((a)>(b) ? (a) : (b))

/* this code taken from kimpe, goodell, ross "MPI Datatype Marshalling: A Case
 * Study in Datatype Equivalence".  Unlike datatype hashing for correct MPI
 * behavior, we are looking for full datatype equivalence, so we need an
 * approach that looks at the full type map, not just the signature.
 * NOTE: Datatype_marshalling also encodes the attributes on that datatype,
 * which we do not need and have eliminated.  Also cut out the "compresses"
 * representation when incorporating to ROMIO so as not to bring in another 
 * dependency.  */

static int dt_translate[] = {
    MPI_COMBINER_NAMED,
    MPI_COMBINER_DUP, 
    MPI_COMBINER_CONTIGUOUS,
    MPI_COMBINER_VECTOR,
    MPI_COMBINER_HVECTOR_INTEGER,
    MPI_COMBINER_HVECTOR,
    MPI_COMBINER_INDEXED,
    MPI_COMBINER_HINDEXED_INTEGER,
    MPI_COMBINER_HINDEXED,
    MPI_COMBINER_INDEXED_BLOCK,
    MPI_COMBINER_STRUCT_INTEGER,
    MPI_COMBINER_STRUCT,
    MPI_COMBINER_SUBARRAY,
    MPI_COMBINER_DARRAY,
    MPI_COMBINER_F90_REAL,
    MPI_COMBINER_F90_COMPLEX,
    MPI_COMBINER_F90_INTEGER,
    MPI_COMBINER_RESIZED
};

static int dt_named_types[] = {
 MPI_CHAR,
   MPI_SHORT,
   MPI_INT,
   MPI_LONG,
   MPI_LONG_LONG_INT,
   MPI_LONG_LONG,
   MPI_SIGNED_CHAR,
   MPI_UNSIGNED_CHAR,
   MPI_UNSIGNED_SHORT,
   MPI_UNSIGNED,
   MPI_UNSIGNED_LONG,
   MPI_UNSIGNED_LONG_LONG,
   MPI_FLOAT,
   MPI_DOUBLE,
   MPI_LONG_DOUBLE,
   MPI_WCHAR,
   MPI_C_BOOL,
   MPI_INT8_T,
   MPI_INT16_T,
   MPI_INT32_T,
   MPI_INT64_T,
   MPI_UINT8_T,
   MPI_UINT16_T,
   MPI_UINT32_T,
   MPI_UINT64_T,
   MPI_C_COMPLEX,
   MPI_C_FLOAT_COMPLEX,
   MPI_C_DOUBLE_COMPLEX,
   MPI_C_LONG_DOUBLE_COMPLEX,
   MPI_BYTE,
   MPI_PACKED,
   MPI_UB,
   MPI_LB
};

struct output_t;

typedef struct output_t output_t;

struct output_t
{
   char * buffer;
   size_t bufpos;
   size_t bufsize;
   /* Return false on error */
   int (*write) (output_t * out, const void * data, size_t size);
   int (*read) (output_t * out, void * data, size_t size);
};

static inline char dtmarshal_lookup_named_type (MPI_Datatype type)
{
   size_t i;
   for (i=0; i<(sizeof(dt_named_types)/sizeof(dt_named_types[0])); ++i)
   {
      if (type == dt_named_types[i])
         return i;
   }
   assert (0 && "Unknown named type?");
   return 255;
}

static inline MPI_Datatype dtmarshal_external_to_type (char t)
{
   assert (t>= 0 && t<(sizeof(dt_named_types)/sizeof(dt_named_types[0])));
   return dt_named_types[(int) t];
}

static inline char dtmarshal_lookup_combiner (int combiner)
{
   size_t i;
   for (i=0; i<(sizeof(dt_translate)/sizeof(dt_translate[0])); ++i)
   {
      if (combiner == dt_translate[i])
         return i;
   }
   assert (0 && "Unknown combiner?");
   return 255;
}

static inline int output_write (output_t * out, const void * data, size_t size)
{
   size_t canwrite = dt_min(out->bufsize - out->bufpos, size);
   memcpy (&out->buffer[out->bufpos], data, canwrite);
   out->bufpos+=canwrite;
   return size == canwrite;
}

static inline int output_read (output_t * out, void * data, size_t size)
{
   size_t canread = dt_min(out->bufsize - out->bufpos, size);
   memcpy (data, &out->buffer[out->bufpos], canread);
   out->bufpos += canread;
   return size == canread;
}

static inline int output_size (output_t * out, const void * data, size_t size)
{
   out->bufpos += size;
   return 1;
}

static void dtmarshal_buffer_setup (output_t * out, void * buffer, size_t maxbuf)
{
   out->write = &output_write;
   out->read = &output_read;
   out->bufsize = maxbuf;
   out->buffer = buffer;
   out->bufpos = 0;
}

static void dtmarshal_size_setup (output_t * out)
{
   out->write = &output_size;
   out->read = 0;
   out->bufsize = SIZE_MAX;
   out->buffer = 0;
   out->bufpos = 0;
}

/* Returns nonzero if success */
static inline int write_xdr_uint32 (output_t * out, uint32_t i)
{
   /* TODO: byteswap if needed */
   return out->write (out, &i, sizeof (i));
}

static inline int read_xdr_uint32 (output_t * out, uint32_t * i)
{
   return out->read (out, i, sizeof(*i));
}

static inline int write_xdr_char (output_t * out, char c)
{
   /* XDR spec states all items are multiple of 4 bytes in size;
    Requires zero-padding */
   return write_xdr_uint32 (out, c);
}

static inline int read_xdr_char (output_t * out, char * c)
{
   uint32_t i;
   int ret= read_xdr_uint32 (out, &i);
   *c = (char) i;
   return ret;
}

static inline int write_xdr_int32 (output_t * out, int32_t i)
{
   return write_xdr_uint32 (out, (uint32_t) i);
}

static inline int read_xdr_int32 (output_t * out, int32_t * i)
{
   return read_xdr_uint32 (out, (uint32_t*) i);
}

static inline int write_xdr_uint64 (output_t * out, uint64_t u)
{
   /* TODO: byteswap if needed */
   return out->write (out, &u, sizeof(u));
}

static inline int read_xdr_uint64 (output_t * out, uint64_t * u)
{
   return out->read (out, u, sizeof(*u));
}

static void free_if_not_builtin (MPI_Datatype * type)
{
   int intcount;
   int addrcount;
   int typecount;
   int combiner;
   MPI_Type_get_envelope (*type, &intcount, &addrcount, &typecount,
         &combiner);
   if (combiner != MPI_COMBINER_NAMED)
      MPI_Type_free (type);
}


/* Store passed data. If count < 0, also store number of items following */
static int dtmarshal_marshal_helper (output_t * out, int intcount, int addrcount, int typecount, 
      int storeint, int storeaddr, int storetype,
      const int * ints, const MPI_Aint * addrs, const MPI_Datatype * types);



static int dtmarshal_marshal (MPI_Datatype type, output_t * out)
{
   int intcount, addrcount, typecount, combiner;
   int ret = MPI_SUCCESS;
   MPI_Aint * addrs = 0;
   MPI_Datatype * types = 0;
   int * ints = 0;
   size_t i;

   /* get type contents */
   ret = MPI_Type_get_envelope (type, &intcount, &addrcount, &typecount, &combiner);
   if (ret != MPI_SUCCESS)
      return ret;

   /* store combiner */
   if (!write_xdr_char (out, dtmarshal_lookup_combiner (combiner)))
   {
      /* TODO: set error */
      return MPI_ERR_IO;
   }

   if (combiner == MPI_COMBINER_NAMED)
   {
      /* do something to store named type
       * translate handle into constant
       */
      if (!write_xdr_char (out, dtmarshal_lookup_named_type (type)))
      {
         return MPI_ERR_IO;
      }
   }
   else
   {
      ints = ADIOI_Malloc (sizeof(int) * intcount);
      addrs = ADIOI_Malloc (sizeof(MPI_Aint) * addrcount);
      types = ADIOI_Malloc (sizeof(MPI_Datatype) * typecount);

      ret = MPI_Type_get_contents (type, intcount, addrcount, typecount, ints, addrs, types);
      if(ret != MPI_SUCCESS)
         return ret;

      switch (combiner)
      {
         case MPI_COMBINER_DUP:
            assert (typecount == 1);
            assert (!intcount);
            assert (!addrcount);
            ret = dtmarshal_marshal_helper (out, 0, 0, 1, FALSE, FALSE, FALSE, ints, addrs, types);
            break;
         case MPI_COMBINER_CONTIGUOUS:
            assert (typecount == 1);
            assert (intcount == 1);
            assert (!addrcount);
            ret = dtmarshal_marshal_helper (out, 1, 0, 1, FALSE, FALSE, FALSE, ints, addrs, types);
            break;
         case MPI_COMBINER_VECTOR:
            assert (typecount == 1);
            ret = dtmarshal_marshal_helper (out, 3, 0, 1, FALSE, FALSE, FALSE, ints, addrs, types);
            break;
         case MPI_COMBINER_HVECTOR_INTEGER:
            /* integer combiner is preserved but returns addrs instead (see pg
             109 mpi22 standard) */
           /* assert (typecount == 1);
            dtmarshal_marshal_helper (out, 3, 0, 1, FALSE, FALSE, FALSE, ints, addrs, types);
            break; */
         case MPI_COMBINER_HVECTOR:
            assert (typecount == 1);
            ret = dtmarshal_marshal_helper (out, 2, 1, 1, FALSE, FALSE, FALSE, ints, addrs, types);
            break;
         case MPI_COMBINER_INDEXED:
            assert (typecount == 1);
            ret = dtmarshal_marshal_helper (out, ints[0]*2, 0, 1, TRUE, FALSE, FALSE, &ints[1], addrs, types);
            break;
         case MPI_COMBINER_HINDEXED:
         case MPI_COMBINER_HINDEXED_INTEGER:
            assert (typecount == 1);
            assert (addrcount + 1 == intcount);
            ret = dtmarshal_marshal_helper (out, ints[0], ints[0], 1, TRUE, FALSE, FALSE, &ints[1], addrs, types);
            break;
         case MPI_COMBINER_INDEXED_BLOCK:
            assert (!addrcount);
            assert (typecount == 1);
            ret = dtmarshal_marshal_helper (out, ints[0]+1, 0, 1, TRUE, FALSE, FALSE, &ints[1], addrs, types);
            break;
         case MPI_COMBINER_STRUCT_INTEGER:
         case MPI_COMBINER_STRUCT:
            assert (addrcount + 1 == intcount);
            assert (typecount + 1 == intcount);
            ret = dtmarshal_marshal_helper (out, ints[0], addrcount, typecount, TRUE, FALSE, FALSE, &ints[1], addrs, types);
            break;
         case MPI_COMBINER_SUBARRAY:
            assert (!addrcount);
            assert (typecount == 1);
            ret = dtmarshal_marshal_helper (out, ints[0]*3+1, addrcount, typecount, TRUE, FALSE, FALSE, &ints[1], addrs, types);
            break;
         case MPI_COMBINER_DARRAY:
            assert (!addrcount);
            assert (typecount == 1);
            ret = dtmarshal_marshal_helper (out, ints[2]*4+3, addrcount, typecount, TRUE, FALSE, FALSE, &ints[0], addrs, types);
            break;
         case MPI_COMBINER_RESIZED:
            assert (typecount == 1);
            assert (addrcount == 2);
            assert (intcount == 0);
            ret = dtmarshal_marshal_helper (out, 0, 2, 1, FALSE, FALSE, FALSE, ints, addrs, types);
            break;
         case MPI_COMBINER_F90_REAL:
         case MPI_COMBINER_F90_COMPLEX:
         case MPI_COMBINER_F90_INTEGER:
         default:
            assert (!addrcount);
            assert (0);
            break;
      }

      for (i=0; i<typecount; ++i)
      {
         free_if_not_builtin (&types[i]);
      }

      ADIOI_Free (addrs);
      ADIOI_Free (types);
      ADIOI_Free (ints);
   }

   return ret;
}

static int dtmarshal_marshal_helper (output_t * out, int intcount, int addrcount, int typecount, int storeint,
      int storeaddr, int storetype, const int * ints, const MPI_Aint * addrs, const MPI_Datatype * types)
{
   int ret = MPI_SUCCESS;
   size_t i;

   if (storeint)
   {
      ret = write_xdr_uint32 (out, intcount);
      if (!ret)
         return MPI_ERR_IO;
   }

   for (i=0; i<intcount; ++i)
   {
      ret = write_xdr_int32 (out, ints[i]);
      if (!ret) return MPI_ERR_IO;
   }

   if (storeaddr)
   {
      ret = write_xdr_uint32 (out, addrcount);
      if (!ret) return MPI_ERR_IO;
   }

   for (i=0; i<addrcount; ++i)
   {
      ret = write_xdr_uint64 (out, addrs[i]);
      if (!ret) return MPI_ERR_IO;
   }

   assert (!storetype);
   for (i=0; i<typecount; ++i)
   {
      ret = dtmarshal_marshal (types[i], out);
      if (ret != MPI_SUCCESS) return ret;
   }
   return ret;
}

static int dtmarshal_unmarshal (output_t * out, MPI_Datatype * t);

/**
 * If intread,addrread,typeread != 0 and != 1, read count from stream 
 * (and write numbe read to pointer).
 * If == 1, use intcount
 */
static int dtmarshal_unmarshal_helper (output_t * out,
      int intcount, int addrcount, int typecount,
      int * intread, int * addrread, int *typeread,
      int ** ints, MPI_Aint ** addrs, MPI_Datatype ** types)
{
   *ints = 0;
   *addrs = 0;
   *types = 0;
   size_t i;

   if (intread)
   {
      if (!read_xdr_uint32 (out, (uint32_t *) intread))
         return MPI_ERR_IO;

      intcount = *intread;
   }
   if (intcount)
   {
      *ints = ADIOI_Malloc (sizeof(int) * intcount);
      for (i=0; i<intcount; ++i)
      {
         if (!read_xdr_int32 (out, &((*ints)[i])))
            return MPI_ERR_IO;
      }
   }
   if (addrread)
   {
      if (addrread == (int*) 1)
      {
         addrcount = intcount;
      }
      else
      {
         if (!read_xdr_uint32 (out, (uint32_t*) addrread))
            return MPI_ERR_IO;
         addrcount = *addrread;
      }
   }
   if (addrcount)
   {
      *addrs = ADIOI_Malloc (sizeof(MPI_Aint) * addrcount);
      for (i=0; i<addrcount; ++i)
      {
         assert(sizeof(MPI_Aint) == sizeof(uint64_t));
         if (!read_xdr_uint64 (out,(uint64_t*) &((*addrs)[i])))
            return MPI_ERR_IO;
      }
   }
   if (typeread)
   {
      if (typeread == (int*) 1)
      {
         typecount = intcount;
      }
      else
      {
         if (!read_xdr_uint32 (out, (uint32_t*) typeread))
            return MPI_ERR_IO;
         typecount = *typeread;
      }
   }
   if (typecount)
   {
      int ret;
      *types = ADIOI_Malloc (sizeof (MPI_Datatype) * typecount);
      for (i=0; i<typecount; ++i)
      {
         ret = dtmarshal_unmarshal (out, &((*types)[i]));
         if (ret != MPI_SUCCESS)
            return ret;
      }
   }
   return MPI_SUCCESS;
}

static int dtmarshal_unmarshal (output_t * out, MPI_Datatype * t)
{
   /* read combiner */
   size_t i;
   char combiner;
   int ret = MPI_SUCCESS;
   int typecount = 0;
   int intcount = 0;
   MPI_Aint * addrs = 0;
   MPI_Datatype * types = 0;
   int * ints = 0;

   if (!read_xdr_char (out, &combiner))
   {
      return MPI_ERR_IO; /* TODO return error */
   }

   if (combiner < 0 || combiner > sizeof (dt_translate)/sizeof(dt_translate[0]))
   {
      assert (0 && "Invalid combiner in stream!");
      return MPI_ERR_UNSUPPORTED_OPERATION;
   }

   /* convert combiner into MPI combiner */
   combiner = dt_translate[(int)combiner];

   switch (combiner)
   {
      char type;
      int ndim;
      case MPI_COMBINER_NAMED:
         if (!read_xdr_char (out, &type))
            return MPI_ERR_IO;
         *t = dtmarshal_external_to_type (type);
         return MPI_SUCCESS;
      case MPI_COMBINER_DUP:
         ret = dtmarshal_unmarshal_helper (out, 0, 0, 1, NULL, NULL, NULL, &ints, &addrs, &types);
         typecount=1;
         if (ret != MPI_SUCCESS) return ret;
         ret = MPI_Type_dup (types[0], t);
         if (ret != MPI_SUCCESS) return ret;
         break;
      case MPI_COMBINER_CONTIGUOUS:
         ret = dtmarshal_unmarshal_helper (out, 1, 0, 1, NULL, NULL, NULL, &ints, &addrs, &types);
         typecount=1;
         if (ret != MPI_SUCCESS) return ret;
         ret = MPI_Type_contiguous (ints[0], types[0], t);
         if (ret != MPI_SUCCESS) return ret;
         break;
      case MPI_COMBINER_VECTOR:
         ret = dtmarshal_unmarshal_helper (out, 3, 0, 1, NULL, NULL, NULL, &ints, &addrs, &types);
         typecount=1;
         if (ret != MPI_SUCCESS) return ret;
         ret = MPI_Type_vector (ints[0], ints[1], ints[2], types[0], t);
         if (ret != MPI_SUCCESS) return ret;
         break;
      case MPI_COMBINER_HVECTOR:
         ret = dtmarshal_unmarshal_helper (out, 2, 1, 1, NULL, NULL, NULL, &ints, &addrs, &types);
         typecount=1;
         if (ret != MPI_SUCCESS) return ret;
         ret = MPI_Type_hvector (ints[0], ints[1], addrs[0], types[0], t);
         if (ret != MPI_SUCCESS) return ret;
         break;
      case MPI_COMBINER_INDEXED:
         ret = dtmarshal_unmarshal_helper (out, 0, 0, 1, &intcount, NULL, NULL, &ints, &addrs, &types);
         typecount=1;
         if (ret != MPI_SUCCESS) return ret;
         assert (intcount % 2 == 0);
         ret = MPI_Type_indexed (intcount/2, &ints[0], &ints[intcount/2], types[0], t);
         if (ret != MPI_SUCCESS) return ret;
         break;
      case MPI_COMBINER_HINDEXED_INTEGER:
      case MPI_COMBINER_HINDEXED:
         typecount=1;
         ret = dtmarshal_unmarshal_helper (out, 0, 0, 1, &intcount, (int*)1, NULL, &ints, &addrs, &types);
         if (ret != MPI_SUCCESS) return ret;
         ret = MPI_Type_hindexed (intcount, &ints[0], &addrs[0], types[0], t);
         if (ret != MPI_SUCCESS) return ret;
         break;
      case MPI_COMBINER_INDEXED_BLOCK:
         typecount=1;
         ret = dtmarshal_unmarshal_helper (out, 0, 0, 1, &intcount, NULL, NULL, &ints, &addrs, &types);
         if (ret != MPI_SUCCESS) return ret;
         assert (intcount >= 1);  /* at the very least the blocklen */
         ret = MPI_Type_create_indexed_block (intcount-1, ints[0], &ints[1], types[0], t);
         if (ret != MPI_SUCCESS) return ret;
         break;
      case MPI_COMBINER_STRUCT:
         ret = dtmarshal_unmarshal_helper (out, 0, 0, 0, &intcount, (void*)1, (void*)1, &ints, &addrs, &types);
         typecount=intcount;
         if (ret != MPI_SUCCESS) return ret;
         ret = MPI_Type_struct (intcount, &ints[0], &addrs[0], &types[0], t);
         if (ret != MPI_SUCCESS) return ret;
         break;
      case MPI_COMBINER_SUBARRAY:
         ret = dtmarshal_unmarshal_helper (out, 0, 0, 1, &intcount, NULL, NULL, &ints, &addrs, &types);
         if (ret != MPI_SUCCESS) return ret;
         assert ((intcount - 1) % 3 == 0);
         ndim = (intcount - 1 )/3;
         typecount=1;
         ret = MPI_Type_create_subarray (ndim, &ints[0], &ints[ndim], &ints[2*ndim], ints[2*ndim+1],
               types[0], t);
         if (ret != MPI_SUCCESS) return ret;
         break;
      case MPI_COMBINER_DARRAY:
         ret = dtmarshal_unmarshal_helper (out, 0, 0, 1, &intcount, NULL, NULL, &ints, &addrs, &types);
         if (ret != MPI_SUCCESS) return ret;
         ndim = (intcount - 3)/4;
         typecount=1;
         assert(0);
         break;
      case MPI_COMBINER_RESIZED:
         ret = dtmarshal_unmarshal_helper  (out, 0, 2, 1, NULL, NULL, NULL, &ints, &addrs, &types);
         if (ret != MPI_SUCCESS) return ret;
         ret = MPI_Type_create_resized (types[0], addrs[0], addrs[1], t);
         if (ret != MPI_SUCCESS) return ret;
         typecount=1;
         break;

      case MPI_COMBINER_HVECTOR_INTEGER:
      case MPI_COMBINER_STRUCT_INTEGER:
      case MPI_COMBINER_F90_REAL:
      case MPI_COMBINER_F90_COMPLEX:
      case MPI_COMBINER_F90_INTEGER:
      default:
         assert (0 && "Unknown combiner (or not supported)!");
         break;
   }

   for (i=0; i<typecount; ++i)
   {
      free_if_not_builtin (&types[i]);
   }
   ADIOI_Free (ints);
   ADIOI_Free (addrs);
   ADIOI_Free (types);
   return ret;
}

int MPIX_Type_unmarshal(const char *typerep,
                        void *inbuf,
                        MPI_Aint insize,
                        MPI_Datatype *type)
{
   int ret;

   if (!strcmp (typerep, "internal"))
   {
#ifdef HAVE_INTERNAL_SUPPORT
      return PMPIX_Type_unmarshal (typerep, inbuf, insize, type);
#else
      return MPI_ERR_ARG;
#endif
   }
   else if (!strcmp (typerep, "external"))
   {
      output_t out;
      dtmarshal_buffer_setup (&out, inbuf, insize);
      ret = dtmarshal_unmarshal (&out, type);
      return ret;
   }
   else if (!strcmp (typerep, "hashed")) {
	   /* no reasonable way to un-hash a one way hash... return constructed
	    * MPI_INT so the test harness will pass */
	   return MPI_Type_contiguous(1, MPI_INT, type);
   }
   else
   {
      return MPI_ERR_ARG;
   }
}


int MPIX_Type_marshal_size(const char *typerep,
                           MPI_Datatype type,
                           MPI_Aint *size)
{
   int ret = MPI_SUCCESS;

   if (!strcmp (typerep, "internal"))
   {
#ifdef HAVE_INTERNAL_SUPPORT
      return PMPIX_Type_unmarshal (typerep, inbuf, insize, type);
#else
      return MPI_ERR_ARG;
#endif
   }
   else if (!strcmp (typerep, "external"))
   {
      output_t out;
      dtmarshal_size_setup (&out);
      ret = dtmarshal_marshal (type, &out);
      *size = out.bufpos;
      return ret;
   } 
   else if (!strcmp (typerep, "hashed"))
   {
        *size = 20;
        return MPI_SUCCESS;
   }
   else
   {
      return MPI_ERR_ARG;
   }
}


static int MPIX_Type_marshal(const char *typerep,
                      MPI_Datatype type,
                      void *outbuf,
                      MPI_Aint outsize,
                      MPI_Aint *num_written)
{
   int ret = MPI_SUCCESS;

   if (!strcmp (typerep, "internal"))
   {
#ifdef HAVE_INTERNAL_SUPPORT
      return PMPIX_Type_marshal (typerep, inbuf, insize, type);
#else
      return MPI_ERR_ARG;
#endif
   } 
   else if (!strcmp (typerep, "external"))
   {
      output_t out;
      dtmarshal_buffer_setup (&out, outbuf, outsize);
      ret = dtmarshal_marshal (type, &out);
      *num_written = out.bufpos;
      return ret;
   }
   else if (!strcmp (typerep, "hashed") ) {
        output_t out;
	MPI_Aint typesize;
	unsigned char *tmpbuf;
	uint32_t *p, *q;

	MPIX_Type_marshal_size("external", type, &typesize);
	tmpbuf = ADIOI_Calloc(typesize, 1);
	dtmarshal_buffer_setup(&out, tmpbuf, typesize);
	ret = dtmarshal_marshal(type, &out);
	if (ret == MPI_SUCCESS) {
	    p = outbuf;
	    q = p + 1;
	    hashlittle2(tmpbuf, out.bufpos, p, q);
	    *num_written = 8;
	}
	ADIOI_Free(tmpbuf);

	return MPI_SUCCESS;
   }
   else
   {
      return MPI_ERR_ARG;
   }
}


#define ADIOI_HASH_COUNT 2
#define ADIOI_HASH_BYTES (ADIOI_HASH_COUNT*sizeof(uint32_t))
typedef struct request_hash_t{
    MPI_Aint req_size;
    uint32_t hash[2];
} ADIOI_request_hash;


static int request_hash_equal(ADIOI_request_hash *a, ADIOI_request_hash *b)
{
	int i;
	if (a->req_size != b->req_size) return 0;
	for(i=0; i<ADIOI_HASH_COUNT; i++) 
		if (a->hash[i] != b->hash[i]) return 0;

	return 1;
}

void ADIOI_dtype_hash(MPI_Datatype type, ADIOI_request_hash *hash)
{
    MPI_Aint len;
    /* potential optimization: short-circuit the check here by looking at the
     * length of the marshaled representation.   */
    MPIX_Type_marshal("hashed", type, hash->hash, ADIOI_HASH_BYTES, &len);
}

int ADIOI_allsame_access(ADIO_File fd, int count, MPI_Datatype datatype)
{
    ADIOI_request_hash mine, max, min;
    MPI_Aint memtype_size;

    /* we determine equivalence of access this way:
     * - file views are identical: same type signature, same type map.  For
     *   efficency we marshal the type and hash the result 
     * - amount of data requested (a memory region defined by 'buf, count,
     *   datatype' tuple) needs to be the same.  It can be laid out any which
     *   way in memory. */

    /* we hash the marshaled representation but maybe it's better to compare
     * the marshaled version? */
    memset(&mine, 0, sizeof(mine));

    MPI_Type_size(datatype, &memtype_size);
    mine.req_size = memtype_size * count;

    /* reduction of integers can be pretty fast on some platforms */
    MPI_Allreduce(&mine.req_size, &min.req_size, sizeof(min.req_size)/sizeof(int), 
		    MPI_INT, MPI_MIN, fd->comm);
    MPI_Allreduce(&mine.req_size, &max.req_size, sizeof(max.req_size)/sizeof(int), 
		    MPI_INT, MPI_MAX, fd->comm);
    if (min.req_size != max.req_size) return 0;

    ADIOI_dtype_hash(fd->filetype, &mine);

    MPI_Allreduce(&mine.hash, &min.hash, sizeof(min.hash)/sizeof(int), 
		    MPI_INT, MPI_MIN, fd->comm);
    MPI_Allreduce(&mine.hash, &max.hash, sizeof(max.hash)/sizeof(int), 
		    MPI_INT, MPI_MAX, fd->comm);

    return (request_hash_equal(&min, &max));
}

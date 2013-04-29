/* begin_generated_IBM_copyright_prolog                             */
/*                                                                  */
/* This is an automatically generated copyright prolog.             */
/* After initializing,  DO NOT MODIFY OR MOVE                       */
/*  --------------------------------------------------------------- */
/* Licensed Materials - Property of IBM                             */
/* Blue Gene/Q 5765-PER 5765-PRP                                    */
/*                                                                  */
/* (C) Copyright IBM Corp. 2011, 2012 All Rights Reserved           */
/* US Government Users Restricted Rights -                          */
/* Use, duplication, or disclosure restricted                       */
/* by GSA ADP Schedule Contract with IBM Corp.                      */
/*                                                                  */
/*  --------------------------------------------------------------- */
/*                                                                  */
/* end_generated_IBM_copyright_prolog                               */
/*  (C)Copyright IBM Corp.  2007, 2011  */
/**
 * \file src/coll/coll_utils.c
 * \brief ???
 */

//#define TRACE_ON

#include <mpidimpl.h>

pami_result_t MPIDI_Pami_post_wrapper(pami_context_t context, void *cookie)
{
   TRACE_ERR("In post wrapper\n");
   TRACE_ERR("About to call collecive\n");
   PAMI_Collective(context, (pami_xfer_t *)cookie);
   TRACE_ERR("Done calling collective\n");
   return PAMI_SUCCESS;
}

/* some useful macros to make the comparisons less icky, esp given the */
/* explosion of datatypes in MPI2.2                                    */
#define isS_INT(x) ( (x)==MPI_INTEGER || \
                     (x) == MPI_INT32_T || (x) == MPI_INTEGER4 || \
                     (x) == MPI_INT)

#define isUS_INT(x) ( (x) == MPI_UINT32_T || (x) == MPI_UNSIGNED)

#define isS_SHORT(x) ( (x) == MPI_SHORT || (x) == MPI_INT16_T || \
                       (x) == MPI_INTEGER2)

#define isUS_SHORT(x) ( (x) == MPI_UNSIGNED_SHORT || (x) == MPI_UINT16_T)

#define isS_CHAR(x) ( (x) == MPI_SIGNED_CHAR || (x) == MPI_INT8_T || \
                      (x) == MPI_INTEGER1 || (x) == MPI_CHAR)

#define isUS_CHAR(x) ( (x) == MPI_BYTE || \
		       (x) == MPI_UNSIGNED_CHAR || (x) == MPI_UINT8_T )

/* sizeof(longlong) == sizeof(long) == sizeof(uint64) on bgq */
#define isS_LONG(x) ( (x) == MPI_LONG || (x) == MPI_AINT )

#define isUS_LONG(x) ( (x) == MPI_UNSIGNED_LONG )

#define isS_LONG_LONG(x) ( (x) == MPI_INT64_T || (x) == MPI_OFFSET || \
                      (x) == MPI_INTEGER8 || (x) == MPI_LONG_LONG || \
                      (x) == MPI_LONG_LONG_INT )                           

#define isUS_LONG_LONG(x) ( (x) == MPI_UINT64_T || (x) == MPI_UNSIGNED_LONG_LONG )  



#define isFLOAT(x) ( (x) == MPI_FLOAT || (x) == MPI_REAL)

#define isDOUBLE(x) ( (x) == MPI_DOUBLE || (x) == MPI_DOUBLE_PRECISION)

#define isLONG_DOUBLE(x) ( (x) == MPI_LONG_DOUBLE )

#define isLOC_TYPE(x) ( (x) == MPI_2REAL || (x) == MPI_2DOUBLE_PRECISION || \
                        (x) == MPI_2INTEGER || (x) == MPI_FLOAT_INT || \
                        (x) == MPI_DOUBLE_INT || (x) == MPI_LONG_INT || \
                        (x) == MPI_2INT || (x) == MPI_SHORT_INT || \
                        (x) == MPI_LONG_DOUBLE_INT )
						
#define isPAMI_LOC_TYPE(x) ( (x) == PAMI_TYPE_LOC_2INT || (x) == PAMI_TYPE_LOC_FLOAT_INT || \
                             (x) == PAMI_TYPE_LOC_DOUBLE_INT || (x) == PAMI_TYPE_LOC_SHORT_INT || \
                             (x) == PAMI_TYPE_LOC_LONG_INT || (x) == PAMI_TYPE_LOC_LONGDOUBLE_INT || \
                             (x) == PAMI_TYPE_LOC_2FLOAT || (x) == PAMI_TYPE_LOC_2DOUBLE )

#define isBOOL(x) ( (x) == MPI_C_BOOL )

#define isLOGICAL(x) ( (x) == MPI_LOGICAL )

#define isSINGLE_COMPLEX(x) ( (x) == MPI_COMPLEX || (x) == MPI_C_FLOAT_COMPLEX)

#define isDOUBLE_COMPLEX(x) ( (x) == MPI_DOUBLE_COMPLEX || (x) == MPI_COMPLEX8 ||\
                              (x) == MPI_C_DOUBLE_COMPLEX)

/* known missing types: MPI_C_LONG_DOUBLE_COMPLEX */


#define MUSUPPORTED 1
#define MUUNSUPPORTED 0

/* for easier debug */
void MPIDI_Op_to_string(MPI_Op op, char *string)
{
   switch(op)
   {
      case MPI_SUM: strcpy(string, "MPI_SUM"); break;
      case MPI_PROD: strcpy(string, "MPI_PROD"); break;
      case MPI_LAND: strcpy(string, "MPI_LAND"); break;
      case MPI_LOR: strcpy(string, "MPI_LOR"); break;
      case MPI_LXOR: strcpy(string, "MPI_LXOR"); break;
      case MPI_BAND: strcpy(string, "MPI_BAND"); break;
      case MPI_BOR: strcpy(string, "MPI_BOR"); break;
      case MPI_BXOR: strcpy(string, "MPI_BXOR"); break;
      case MPI_MAX: strcpy(string, "MPI_MAX"); break;
      case MPI_MIN: strcpy(string, "MPI_MIN"); break;
      case MPI_MINLOC: strcpy(string, "MPI_MINLOC"); break;
      case MPI_MAXLOC: strcpy(string, "MPI_MAXLOC"); break;
      default: strcpy(string, "Other"); break;
   }
}


int MPIDI_Datatype_to_pami(MPI_Datatype        dt,
                           pami_type_t        *pdt,
                           MPI_Op              op,
                           pami_data_function *pop,
                           int                *musupport)
{
   *musupport = MUSUPPORTED;
   *pdt = PAMI_TYPE_NULL;
   if(op != -1)
      *pop = PAMI_DATA_NOOP;
   if(isS_INT(dt))       *pdt = PAMI_TYPE_SIGNED_INT;
   else if(isUS_INT(dt)) *pdt = PAMI_TYPE_UNSIGNED_INT;
   else if(isFLOAT(dt))  *pdt = PAMI_TYPE_FLOAT;
   else if(isDOUBLE(dt)) *pdt = PAMI_TYPE_DOUBLE;
   else if(isLONG_DOUBLE(dt)) *pdt = PAMI_TYPE_LONG_DOUBLE;
   else
   {
      *musupport = MUUNSUPPORTED;
      if(isS_CHAR(dt))              *pdt = PAMI_TYPE_SIGNED_CHAR;
      else if(isUS_CHAR(dt))        *pdt = PAMI_TYPE_UNSIGNED_CHAR;
      else if(isS_SHORT(dt))        *pdt = PAMI_TYPE_SIGNED_SHORT;
      else if(isUS_SHORT(dt))       *pdt = PAMI_TYPE_UNSIGNED_SHORT;
      else if(isS_LONG(dt))         *pdt = PAMI_TYPE_SIGNED_LONG;
      else if(isUS_LONG(dt))        *pdt = PAMI_TYPE_UNSIGNED_LONG;
      else if(isS_LONG_LONG(dt))    *pdt = PAMI_TYPE_SIGNED_LONG_LONG;
      else if(isUS_LONG_LONG(dt))   *pdt = PAMI_TYPE_UNSIGNED_LONG_LONG;
      else if(isSINGLE_COMPLEX(dt)) *pdt = PAMI_TYPE_SINGLE_COMPLEX;
      else if(isDOUBLE_COMPLEX(dt)) *pdt = PAMI_TYPE_DOUBLE_COMPLEX;
      else if(isLOC_TYPE(dt))
      {
         switch(dt)
         {
            case MPI_2INT:              *pdt = PAMI_TYPE_LOC_2INT;       break;
            case MPI_FLOAT_INT:         *pdt = PAMI_TYPE_LOC_FLOAT_INT;  break;
            case MPI_DOUBLE_INT:        *pdt = PAMI_TYPE_LOC_DOUBLE_INT; break;
            case MPI_SHORT_INT:         *pdt = PAMI_TYPE_LOC_SHORT_INT;  break;

            case MPI_LONG_INT:          *pdt = PAMI_TYPE_LOC_LONG_INT;   break;
            case MPI_LONG_DOUBLE_INT:   *pdt = PAMI_TYPE_LOC_LONGDOUBLE_INT;  break;
            default:
              /* The following is needed to catch missing fortran bindings */
              if (dt == MPI_2REAL)
                *pdt = PAMI_TYPE_LOC_2FLOAT;
              else if (dt == MPI_2DOUBLE_PRECISION)
                *pdt = PAMI_TYPE_LOC_2DOUBLE;
              else if (dt == MPI_2INTEGER)
                *pdt = PAMI_TYPE_LOC_2INT;
         }
         /* 
          * There are some 2-element types that PAMI doesn't support, so we 
          * need to bail on anything that's left of the LOC_TYPEs 
          */
         if(*pdt == PAMI_TYPE_NULL) return -1;

         if(op == -1) return MPI_SUCCESS;
         if(op == MPI_MINLOC)
         {
            *pop = PAMI_DATA_MINLOC;
            return MPI_SUCCESS;
         }
         if(op == MPI_MAXLOC) 
         {
            *pop = PAMI_DATA_MAXLOC;
            return MPI_SUCCESS;
         }
         if(op == MPI_REPLACE) 
         {
            *pop = PAMI_DATA_COPY;
            return MPI_SUCCESS;
         }
         else return -1;
      }
      else if(isLOGICAL(dt))
      {
         *pdt = PAMI_TYPE_LOGICAL4;
         if(op == -1) return MPI_SUCCESS;
         if(op == MPI_LOR) 
         {   
            *pop = PAMI_DATA_LOR;
            return MPI_SUCCESS;
         }
         if(op == MPI_LAND) 
         {   
            *pop = PAMI_DATA_LAND;
            return MPI_SUCCESS;
         }
         if(op == MPI_LXOR) 
         {   
            *pop = PAMI_DATA_LXOR;
            return MPI_SUCCESS;
         }
         if(op == MPI_REPLACE) 
         {   
            *pop = PAMI_DATA_COPY;
            return MPI_SUCCESS;
         }
         return -1;
      }
      else if(isBOOL(dt))
      {
         *pdt = PAMI_TYPE_LOGICAL1;
         if(op == -1) return MPI_SUCCESS;
         if(op == MPI_LOR) 
         {   
            *pop = PAMI_DATA_LOR;
            return MPI_SUCCESS;
         }
         if(op == MPI_LAND) 
         {   
            *pop = PAMI_DATA_LAND;
            return MPI_SUCCESS;
         }
         if(op == MPI_LXOR) 
         {   
            *pop = PAMI_DATA_LXOR;
            return MPI_SUCCESS;
         }
         if(op == MPI_REPLACE) 
         {   
            *pop = PAMI_DATA_COPY;
            return MPI_SUCCESS;
         }
         return -1;
      }
   }

   if(*pdt == PAMI_TYPE_NULL) return -1;

   if(op == -1) return MPI_SUCCESS; /* just doing DT conversion */

   *pop = PAMI_DATA_NOOP;
   switch(op)
   {
      case MPI_SUM: *pop = PAMI_DATA_SUM; return MPI_SUCCESS; break;
      case MPI_PROD: *pop = PAMI_DATA_PROD; return MPI_SUCCESS; break;
      case MPI_MAX: *pop = PAMI_DATA_MAX; return MPI_SUCCESS; break;
      case MPI_MIN: *pop = PAMI_DATA_MIN; return MPI_SUCCESS; break;
      case MPI_BAND: *pop = PAMI_DATA_BAND; return MPI_SUCCESS; break;
      case MPI_BOR: *pop = PAMI_DATA_BOR; return MPI_SUCCESS; break;
      case MPI_BXOR: *pop = PAMI_DATA_BXOR; return MPI_SUCCESS; break;
      case MPI_LAND: 
         if(isLONG_DOUBLE(dt)) return -1; 
         *pop = PAMI_DATA_LAND; return MPI_SUCCESS; break;
      case MPI_LOR: 
         if(isLONG_DOUBLE(dt)) return -1; 
         *pop = PAMI_DATA_LOR; return MPI_SUCCESS; break;
      case MPI_LXOR: 
         if(isLONG_DOUBLE(dt)) return -1; 
         *pop = PAMI_DATA_LXOR; return MPI_SUCCESS; break;
      case MPI_REPLACE: *pop = PAMI_DATA_COPY; return MPI_SUCCESS; break;
   }
   if(*pop == PAMI_DATA_NOOP) return -1;

   return MPI_SUCCESS;
}

int MPIDI_Dtpami_to_dtmpi( pami_type_t         pdt,
                           MPI_Datatype       *dt,
                           pami_data_function  pop,
                           MPI_Op             *op)
{
   *dt = MPI_DATATYPE_NULL;
   if(pop != NULL)
      *op = MPI_OP_NULL;

   if(pdt == PAMI_TYPE_SIGNED_INT)               *dt = MPI_INT;
   else if(pdt == PAMI_TYPE_UNSIGNED_INT)        *dt = MPI_UNSIGNED;
   else if(pdt == PAMI_TYPE_FLOAT)               *dt = MPI_FLOAT; 
   else if(pdt == PAMI_TYPE_DOUBLE)              *dt = MPI_DOUBLE;
   else if(pdt == PAMI_TYPE_LONG_DOUBLE)         *dt = MPI_LONG_DOUBLE;
   else if(pdt == PAMI_TYPE_SIGNED_CHAR)         *dt = MPI_CHAR;
   else if(pdt == PAMI_TYPE_UNSIGNED_CHAR)       *dt = MPI_UNSIGNED_CHAR;
   else if(pdt == PAMI_TYPE_BYTE)                *dt = MPI_BYTE;
   else if(pdt == PAMI_TYPE_SIGNED_SHORT)        *dt = MPI_SHORT;
   else if(pdt == PAMI_TYPE_UNSIGNED_SHORT)      *dt = MPI_UNSIGNED_SHORT;
   else if(pdt == PAMI_TYPE_SIGNED_LONG_LONG)    *dt = MPI_LONG_LONG;
   else if(pdt == PAMI_TYPE_UNSIGNED_LONG_LONG)  *dt = MPI_UNSIGNED_LONG_LONG;
   else if(pdt == PAMI_TYPE_SINGLE_COMPLEX)      *dt = MPI_C_FLOAT_COMPLEX;
   else if(pdt == PAMI_TYPE_DOUBLE_COMPLEX)      *dt = MPI_C_DOUBLE_COMPLEX;
   else if(isPAMI_LOC_TYPE(pdt))
   {
     if(pdt == PAMI_TYPE_LOC_2INT)               *dt = MPI_2INT;
     else if(pdt == PAMI_TYPE_LOC_FLOAT_INT)     *dt = MPI_FLOAT_INT;
     else if(pdt == PAMI_TYPE_LOC_DOUBLE_INT)    *dt = MPI_DOUBLE_INT;
     else if(pdt == PAMI_TYPE_LOC_SHORT_INT)     *dt = MPI_SHORT_INT;
     else if(pdt == PAMI_TYPE_LOC_LONG_INT)      *dt = MPI_LONG_INT;
     else if(pdt == PAMI_TYPE_LOC_LONGDOUBLE_INT)*dt = MPI_LONG_DOUBLE_INT;
     else if(pdt == PAMI_TYPE_LOC_2FLOAT)        *dt = MPI_2REAL;
     else if(pdt == PAMI_TYPE_LOC_2DOUBLE)       *dt = MPI_2DOUBLE_PRECISION;

     if(pop == NULL) return MPI_SUCCESS;
     if(pop == PAMI_DATA_MINLOC)
     {
        *op = MPI_MINLOC;
        return MPI_SUCCESS;
     }
     if(pop == PAMI_DATA_MAXLOC) 
     {
        *op = MPI_MAXLOC;
        return MPI_SUCCESS;
     }
     if(pop == PAMI_DATA_COPY) 
     {
        *op = MPI_REPLACE;
        return MPI_SUCCESS;
     }
     else return -1;
   }
   else if(pdt == PAMI_TYPE_LOGICAL4)
   {
      *dt = MPI_LOGICAL;
      if(pop == NULL) return MPI_SUCCESS;
      if(pop == PAMI_DATA_LOR) 
      {   
         *op = MPI_LOR;
         return MPI_SUCCESS;
      }
      if(pop == PAMI_DATA_LAND) 
      {   
         *op = MPI_LAND;
         return MPI_SUCCESS;
      }
      if(pop == PAMI_DATA_LXOR) 
      {   
         *op = MPI_LXOR;
         return MPI_SUCCESS;
      }
      if(pop == PAMI_DATA_COPY) 
      {   
         *op = MPI_REPLACE;
         return MPI_SUCCESS;
      }
      return -1;
   }
   else if(pdt == PAMI_TYPE_LOGICAL1)
   {
     *dt = MPI_C_BOOL;
     if(pop == NULL) return MPI_SUCCESS;
     if(pop == PAMI_DATA_LOR) 
     {   
        *op = MPI_LOR;
        return MPI_SUCCESS;
     }
     if(pop == PAMI_DATA_LAND) 
     {   
        *op = MPI_LAND;
        return MPI_SUCCESS;
     }
     if(pop == PAMI_DATA_LXOR) 
     {   
        *op = MPI_LXOR;
        return MPI_SUCCESS;
     }
     if(pop == PAMI_DATA_COPY) 
     {   
        *op = MPI_REPLACE;
        return MPI_SUCCESS;
     }
     return -1;
   }
   
   
   if(*dt == MPI_DATATYPE_NULL) return -1;

   if(pop == NULL) return MPI_SUCCESS; /* just doing DT conversion */

   *op = MPI_OP_NULL;
   if(pop == PAMI_DATA_SUM)      *op = MPI_SUM;
   else if(pop ==PAMI_DATA_PROD) *op = MPI_PROD;
   else if(pop ==PAMI_DATA_MAX)  *op = MPI_MAX;
   else if(pop ==PAMI_DATA_MIN)  *op = MPI_MIN;
   else if(pop ==PAMI_DATA_BAND) *op = MPI_BAND;
   else if(pop ==PAMI_DATA_BOR)  *op = MPI_BOR;
   else if(pop ==PAMI_DATA_BXOR) *op = MPI_BXOR;
   else if(pop ==PAMI_DATA_LAND) *op = MPI_LAND;
   else if(pop ==PAMI_DATA_LOR)  *op = MPI_LOR;
   else if(pop ==PAMI_DATA_LXOR) *op = MPI_LXOR;
   else if(pop ==PAMI_DATA_COPY) *op = MPI_REPLACE;

   if(*op == MPI_OP_NULL) return -1;

   return MPI_SUCCESS;
}


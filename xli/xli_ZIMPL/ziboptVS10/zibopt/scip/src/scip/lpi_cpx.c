/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                           */
/*                  This file is part of the program and library             */
/*         SCIP --- Solving Constraint Integer Programs                      */
/*                                                                           */
/*    Copyright (C) 2002-2010 Konrad-Zuse-Zentrum                            */
/*                            fuer Informationstechnik Berlin                */
/*                                                                           */
/*  SCIP is distributed under the terms of the ZIB Academic License.         */
/*                                                                           */
/*  You should have received a copy of the ZIB Academic License              */
/*  along with SCIP; see the file COPYING. If not email to scip@zib.de.      */
/*                                                                           */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#pragma ident "@(#) $Id: lpi_cpx.c,v 1.138 2010/09/06 15:14:15 bzfgamra Exp $"

/**@file   lpi_cpx.c
 * @ingroup LPIS
 * @brief  LP interface for CPLEX >= 8.0
 * @author Tobias Achterberg
 */

/*--+----1----+----2----+----3----+----4----+----5----+----6----+----7----+----8----+----9----+----0----+----1----+----2*/

#include <assert.h>

#include "cplex.h"
#include "scip/bitencode.h"
#include "scip/lpi.h"
#include "scip/message.h"



#define CHECK_ZERO(x) { int _restat_;                                               \
                        if( (_restat_ = (x)) != 0 )                                 \
                        {                                                           \
                           SCIPerrorMessage("LP Error: CPLEX returned %d\n", _restat_); \
                           return SCIP_LPERROR;                                     \
                        }                                                           \
                      }

#define ABORT_ZERO(x) { int _restat_;                                               \
                        if( (_restat_ = (x)) != 0 )                                 \
                        {                                                           \
                           SCIPerrorMessage("LP Error: CPLEX returned %d\n", _restat_); \
                           SCIPABORT();                                                 \
                        }                                                           \
                      }

#define CPX_INT_MAX 2100000000 /* CPLEX doesn't accept larger values in integer parameters */


typedef SCIP_DUALPACKET COLPACKET;           /* each column needs two bits of information (basic/on_lower/on_upper) */
#define COLS_PER_PACKET SCIP_DUALPACKETSIZE
typedef SCIP_DUALPACKET ROWPACKET;           /* each row needs two bit of information (basic/on_lower/on_upper) */
#define ROWS_PER_PACKET SCIP_DUALPACKETSIZE

/* CPLEX parameter lists which can be changed */
#define NUMINTPARAM  10
static const int intparam[NUMINTPARAM] = {
   CPX_PARAM_ADVIND,
   CPX_PARAM_ITLIM,
   CPX_PARAM_FASTMIP,
   CPX_PARAM_SCAIND,
   CPX_PARAM_PREIND,
   CPX_PARAM_PPRIIND,
   CPX_PARAM_DPRIIND,
   CPX_PARAM_SIMDISPLAY,
   CPX_PARAM_SCRIND,
   CPX_PARAM_THREADS
};
#define NUMDBLPARAM  7
static const int dblparam[NUMDBLPARAM] = {
   CPX_PARAM_EPRHS,
   CPX_PARAM_EPOPT,
   CPX_PARAM_BAREPCOMP,
   CPX_PARAM_OBJLLIM,
   CPX_PARAM_OBJULIM,
   CPX_PARAM_TILIM,
   CPX_PARAM_EPMRK
};
static const double dblparammin[NUMDBLPARAM] = {
   +1e-09, /*CPX_PARAM_EPRHS*/
   +1e-09, /*CPX_PARAM_EPOPT*/
   +1e-12, /*CPX_PARAM_BAREPCOMP*/
   -1e+99, /*CPX_PARAM_OBJLLIM*/
   -1e+99, /*CPX_PARAM_OBJULIM*/
   -1e+99, /*CPX_PARAM_TILIM*/
   0.0001  /*CPX_PARAM_EPMRK*/
};

/** CPLEX parameter settings */
struct CPXParam
{
   int                   intparval[NUMINTPARAM]; /**< integer parameter values */
   double                dblparval[NUMDBLPARAM]; /**< double parameter values */
};
typedef struct CPXParam CPXPARAM;

/** LP interface */
struct SCIP_LPi
{
   CPXLPptr              cpxlp;              /**< CPLEX LP pointer */
   int                   solstat;            /**< solution status of last optimization call */
   CPXPARAM              cpxparam;           /**< current parameter values for this LP */
   char*                 larray;             /**< array with 'L' entries for changing lower bounds */
   char*                 uarray;             /**< array with 'U' entries for changing upper bounds */
   char*                 senarray;           /**< array for storing row senses */
   SCIP_Real*            rhsarray;           /**< array for storing rhs values */
   SCIP_Real*            rngarray;           /**< array for storing range values */
   SCIP_Real*            valarray;           /**< array for storing coefficient values */
   int*                  rngindarray;        /**< array for storing row indices with range values */
   int*                  cstat;              /**< array for storing column basis status */
   int*                  rstat;              /**< array for storing row basis status */
   int*                  indarray;           /**< array for storing coefficient indices */
   int                   boundchgsize;       /**< size of larray and uarray */
   int                   sidechgsize;        /**< size of senarray, rngarray, and rngindarray */
   int                   valsize;            /**< size of valarray and indarray */
   int                   cstatsize;          /**< size of cstat array */
   int                   rstatsize;          /**< size of rstat array */
   int                   iterations;         /**< number of iterations used in the last solving call */
   SCIP_PRICING          pricing;            /**< SCIP pricing setting  */
   SCIP_Bool             solisbasic;         /**< is current LP solution a basic solution? */
   SCIP_Bool             instabilityignored; /**< was the instability of the last LP ignored? */
#if (CPX_VERSION <= 1100)
   SCIP_Bool             rngfound;           /**< was ranged row found; scaling is disabled, because there is a bug 
                                              *   in the scaling algo for ranged rows in CPLEX up to version 11.0 */
#endif
#if (CPX_VERSION == 1100 || CPX_VERSION == 1220)
   int                   pseudonthreads;     /**< number of threads that SCIP set for the LP solver, but due to CPLEX bug,
                                              *   we set the thread count to 1. In order to fulfill assert in lp.c,
                                              *   we have to return the value set by SCIP and not the real thread count */
#endif
};

/** LPi state stores basis information */
struct SCIP_LPiState
{
   int                   ncols;              /**< number of LP columns */
   int                   nrows;              /**< number of LP rows */
   COLPACKET*            packcstat;          /**< column basis status in compressed form */
   ROWPACKET*            packrstat;          /**< row basis status in compressed form */
};


static CPXENVptr         cpxenv = NULL;      /**< CPLEX environment */
static CPXPARAM          defparam;           /**< default CPLEX parameters */
static CPXPARAM          curparam;           /**< current CPLEX parameters in the environment */
static int               numlp = 0;          /**< number of open LP objects */



/*
 * dynamic memory arrays
 */

/** resizes larray and uarray to have at least num entries */
static
SCIP_RETCODE ensureBoundchgMem(
   SCIP_LPI*             lpi,                /**< LP interface structure */
   int                   num                 /**< minimal number of entries in array */
   )
{
   assert(lpi != NULL);

   if( num > lpi->boundchgsize )
   {
      int newsize;
      int i;

      newsize = MAX(2*lpi->boundchgsize, num);
      SCIP_ALLOC( BMSreallocMemoryArray(&lpi->larray, newsize) );
      SCIP_ALLOC( BMSreallocMemoryArray(&lpi->uarray, newsize) );
      for( i = lpi->boundchgsize; i < newsize; ++i )
      {
         lpi->larray[i] = 'L';
         lpi->uarray[i] = 'U';
      }
      lpi->boundchgsize = newsize;
   }
   assert(num <= lpi->boundchgsize);

   return SCIP_OKAY;
}

/** resizes senarray, rngarray, and rngindarray to have at least num entries */
static
SCIP_RETCODE ensureSidechgMem(
   SCIP_LPI*             lpi,                /**< LP interface structure */
   int                   num                 /**< minimal number of entries in array */
   )
{
   assert(lpi != NULL);

   if( num > lpi->sidechgsize )
   {
      int newsize;

      newsize = MAX(2*lpi->sidechgsize, num);
      SCIP_ALLOC( BMSreallocMemoryArray(&lpi->senarray, newsize) );
      SCIP_ALLOC( BMSreallocMemoryArray(&lpi->rhsarray, newsize) );
      SCIP_ALLOC( BMSreallocMemoryArray(&lpi->rngarray, newsize) );
      SCIP_ALLOC( BMSreallocMemoryArray(&lpi->rngindarray, newsize) );
      lpi->sidechgsize = newsize;
   }
   assert(num <= lpi->sidechgsize);

   return SCIP_OKAY;
}

/** resizes valarray and indarray to have at least num entries */
static
SCIP_RETCODE ensureValMem(
   SCIP_LPI*             lpi,                /**< LP interface structure */
   int                   num                 /**< minimal number of entries in array */
   )
{
   assert(lpi != NULL);

   if( num > lpi->valsize )
   {
      int newsize;

      newsize = MAX(2*lpi->valsize, num);
      SCIP_ALLOC( BMSreallocMemoryArray(&lpi->valarray, newsize) );
      SCIP_ALLOC( BMSreallocMemoryArray(&lpi->indarray, newsize) );
      lpi->valsize = newsize;
   }
   assert(num <= lpi->valsize);

   return SCIP_OKAY;
}

/** resizes cstat array to have at least num entries */
static
SCIP_RETCODE ensureCstatMem(
   SCIP_LPI*             lpi,                /**< LP interface structure */
   int                   num                 /**< minimal number of entries in array */
   )
{
   assert(lpi != NULL);

   if( num > lpi->cstatsize )
   {
      int newsize;

      newsize = MAX(2*lpi->cstatsize, num);
      SCIP_ALLOC( BMSreallocMemoryArray(&lpi->cstat, newsize) );
      lpi->cstatsize = newsize;
   }
   assert(num <= lpi->cstatsize);

   return SCIP_OKAY;
}

/** resizes rstat array to have at least num entries */
static
SCIP_RETCODE ensureRstatMem(
   SCIP_LPI*             lpi,                /**< LP interface structure */
   int                   num                 /**< minimal number of entries in array */
   )
{
   assert(lpi != NULL);

   if( num > lpi->rstatsize )
   {
      int newsize;

      newsize = MAX(2*lpi->rstatsize, num);
      SCIP_ALLOC( BMSreallocMemoryArray(&lpi->rstat, newsize) );
      lpi->rstatsize = newsize;
   }
   assert(num <= lpi->rstatsize);

   return SCIP_OKAY;
}

/** stores current basis in internal arrays of LPI data structure */
static
SCIP_RETCODE getBase(
   SCIP_LPI*             lpi                 /**< LP interface structure */
   )
{
   int ncols;
   int nrows;

   assert(cpxenv != NULL);
   assert(lpi != NULL);

   SCIPdebugMessage("getBase()\n");

   ncols = CPXgetnumcols(cpxenv, lpi->cpxlp);
   nrows = CPXgetnumrows(cpxenv, lpi->cpxlp);

   /* allocate enough memory for storing uncompressed basis information */
   SCIP_CALL( ensureCstatMem(lpi, ncols) );
   SCIP_CALL( ensureRstatMem(lpi, nrows) );

   /* get unpacked basis information from CPLEX */
   CHECK_ZERO( CPXgetbase(cpxenv, lpi->cpxlp, lpi->cstat, lpi->rstat) );

   return SCIP_OKAY;
}

/** loads basis stored in internal arrays of LPI data structure into CPLEX */
static
SCIP_RETCODE setBase(
   SCIP_LPI*             lpi                 /**< LP interface structure */
   )
{
   assert(cpxenv != NULL);
   assert(lpi != NULL);

   SCIPdebugMessage("setBase()\n");

   /* load basis information into CPLEX */
   CHECK_ZERO( CPXcopybase(cpxenv, lpi->cpxlp, lpi->cstat, lpi->rstat) );

   return SCIP_OKAY;
}




/*
 * LPi state methods
 */

/** returns the number of packets needed to store column packet information */
static 
int colpacketNum(
   int                   ncols               /**< number of columns to store */
   )
{
   return (ncols+(int)COLS_PER_PACKET-1)/(int)COLS_PER_PACKET;
}

/** returns the number of packets needed to store row packet information */
static 
int rowpacketNum(
   int                   nrows               /**< number of rows to store */
   )
{
   return (nrows+(int)ROWS_PER_PACKET-1)/(int)ROWS_PER_PACKET;
}

/** store row and column basis status in a packed LPi state object */
static
void lpistatePack(
   SCIP_LPISTATE*       lpistate,            /**< pointer to LPi state data */
   const int*           cstat,               /**< basis status of columns in unpacked format */
   const int*           rstat                /**< basis status of rows in unpacked format */
   )
{
   assert(lpistate != NULL);
   assert(lpistate->packcstat != NULL);
   assert(lpistate->packrstat != NULL);

   SCIPencodeDualBit(cstat, lpistate->packcstat, lpistate->ncols);
   SCIPencodeDualBit(rstat, lpistate->packrstat, lpistate->nrows);
}

/** unpacks row and column basis status from a packed LPi state object */
static
void lpistateUnpack(
   const SCIP_LPISTATE* lpistate,            /**< pointer to LPi state data */
   int*                 cstat,               /**< buffer for storing basis status of columns in unpacked format */
   int*                 rstat                /**< buffer for storing basis status of rows in unpacked format */
   )
{
   assert(lpistate != NULL);
   assert(lpistate->packcstat != NULL);
   assert(lpistate->packrstat != NULL);

   SCIPdecodeDualBit(lpistate->packcstat, cstat, lpistate->ncols);
   SCIPdecodeDualBit(lpistate->packrstat, rstat, lpistate->nrows);
}

/** creates LPi state information object */
static
SCIP_RETCODE lpistateCreate(
   SCIP_LPISTATE**       lpistate,           /**< pointer to LPi state */
   BMS_BLKMEM*           blkmem,             /**< block memory */
   int                   ncols,              /**< number of columns to store */
   int                   nrows               /**< number of rows to store */
   )
{
   assert(lpistate != NULL);
   assert(blkmem != NULL);
   assert(ncols >= 0);
   assert(nrows >= 0);

   SCIP_ALLOC( BMSallocBlockMemory(blkmem, lpistate) );
   SCIP_ALLOC( BMSallocBlockMemoryArray(blkmem, &(*lpistate)->packcstat, colpacketNum(ncols)) );
   SCIP_ALLOC( BMSallocBlockMemoryArray(blkmem, &(*lpistate)->packrstat, rowpacketNum(nrows)) );

   return SCIP_OKAY;
}

/** frees LPi state information */
static
void lpistateFree(
   SCIP_LPISTATE**       lpistate,           /**< pointer to LPi state information (like basis information) */
   BMS_BLKMEM*           blkmem              /**< block memory */
   )
{
   assert(blkmem != NULL);
   assert(lpistate != NULL);
   assert(*lpistate != NULL);

   BMSfreeBlockMemoryArray(blkmem, &(*lpistate)->packcstat, colpacketNum((*lpistate)->ncols));
   BMSfreeBlockMemoryArray(blkmem, &(*lpistate)->packrstat, rowpacketNum((*lpistate)->nrows));
   BMSfreeBlockMemory(blkmem, lpistate);
}



/*
 * local methods
 */

/** gets all CPLEX parameters used in LPI */
static
SCIP_RETCODE getParameterValues(CPXPARAM* cpxparam)
{
   int i;
   
   assert(cpxenv != NULL);
   assert(cpxparam != NULL);

   SCIPdebugMessage("getParameterValues()\n");

   for( i = 0; i < NUMINTPARAM; ++i )
   {
      CHECK_ZERO( CPXgetintparam(cpxenv, intparam[i], &(cpxparam->intparval[i])) );
   }
   for( i = 0; i < NUMDBLPARAM; ++i )
   {
      CHECK_ZERO( CPXgetdblparam(cpxenv, dblparam[i], &(cpxparam->dblparval[i])) );
   }

   return SCIP_OKAY;
}

/** in debug mode, checks validity of CPLEX parameters */
static
SCIP_RETCODE checkParameterValues(void)
{
#ifndef NDEBUG
   CPXPARAM par;
   int i;
   
   SCIP_CALL( getParameterValues(&par) );
   for( i = 0; i < NUMINTPARAM; ++i )
      assert(curparam.intparval[i] == par.intparval[i]);
   for( i = 0; i < NUMDBLPARAM; ++i )
      assert(MAX(curparam.dblparval[i], dblparammin[i]) == par.dblparval[i]); /*lint !e777*/
#endif

   return SCIP_OKAY;
}

/** sets all CPLEX parameters used in LPI */
static
SCIP_RETCODE setParameterValues(const CPXPARAM* cpxparam)
{
   int i;
   
   assert(cpxenv != NULL);
   assert(cpxparam != NULL);
   
   SCIPdebugMessage("setParameterValues()\n");

   for( i = 0; i < NUMINTPARAM; ++i )
   {
      if( curparam.intparval[i] != cpxparam->intparval[i] )
      {
         SCIPdebugMessage("setting CPLEX int parameter %d from %d to %d\n", 
            intparam[i], curparam.intparval[i], cpxparam->intparval[i]);
         curparam.intparval[i] = cpxparam->intparval[i];
         CHECK_ZERO( CPXsetintparam(cpxenv, intparam[i], curparam.intparval[i]) );
      }
   }
   for( i = 0; i < NUMDBLPARAM; ++i )
   {
      if( curparam.dblparval[i] != cpxparam->dblparval[i] ) /*lint !e777*/
      {
         SCIPdebugMessage("setting CPLEX dbl parameter %d from %g to %g\n", 
            dblparam[i], curparam.dblparval[i], MAX(cpxparam->dblparval[i], dblparammin[i]));
         curparam.dblparval[i] = MAX(cpxparam->dblparval[i], dblparammin[i]);
         CHECK_ZERO( CPXsetdblparam(cpxenv, dblparam[i], curparam.dblparval[i]) );
      }
   }

   SCIP_CALL( checkParameterValues() );

   return SCIP_OKAY;
}

/** copies CPLEX parameters from source to dest */
static
void copyParameterValues(CPXPARAM* dest, const CPXPARAM* source)
{
   int i;

   for( i = 0; i < NUMINTPARAM; ++i )
      dest->intparval[i] = source->intparval[i];
   for( i = 0; i < NUMDBLPARAM; ++i )
      dest->dblparval[i] = source->dblparval[i];
}

/** gets a single integer parameter value */
static
int getIntParam(SCIP_LPI* lpi, const int param)
{
   int i;
   
   assert(lpi != NULL);

   for( i = 0; i < NUMINTPARAM; ++i )
      if( intparam[i] == param )
         return lpi->cpxparam.intparval[i];

   SCIPerrorMessage("unknown CPLEX integer parameter\n");
   SCIPABORT();
   return 0; /*lint !e527*/
}

/** gets a single double parameter value */
static
double getDblParam(SCIP_LPI* lpi, const int param)
{
   SCIP_Real val;
   int i;

   assert(lpi != NULL);

   for( i = 0; i < NUMDBLPARAM; ++i )
   {
      if( dblparam[i] == param )
      {
	 val = lpi->cpxparam.dblparval[i];
	 if( val >= CPX_INFBOUND )
	    return CPX_INFBOUND;
	 else if( val <= -CPX_INFBOUND )
	    return -CPX_INFBOUND;
	 else
	    return val;
      }
   }

   SCIPerrorMessage("unknown CPLEX double parameter\n");
   SCIPABORT();
   return 0.0; /*lint !e527*/
}

/** sets a single integer parameter value */
static
void setIntParam(SCIP_LPI* lpi, const int param, int parval)
{
   int i;

   assert(lpi != NULL);

   for( i = 0; i < NUMINTPARAM; ++i )
      if( intparam[i] == param )
      {
         lpi->cpxparam.intparval[i] = parval;
         return;
      }

   SCIPerrorMessage("unknown CPLEX integer parameter\n");
   SCIPABORT();
}

/** sets a single double parameter value */
static
void setDblParam(SCIP_LPI* lpi, const int param, double parval)
{
   int i;

   assert(lpi != NULL);

   if( parval >= CPX_INFBOUND )
      parval = 1e+75;
   else if( parval <= -CPX_INFBOUND )
      parval = -1e+75;

   for( i = 0; i < NUMDBLPARAM; ++i )
      if( dblparam[i] == param )
      {
	 lpi->cpxparam.dblparval[i] = parval;
	 return;
      }

   SCIPerrorMessage("unknown CPLEX double parameter\n");
   SCIPABORT();
}

/** marks the current LP to be unsolved */
static
void invalidateSolution(SCIP_LPI* lpi)
{
   assert(lpi != NULL);
   lpi->solstat = -1;
   lpi->instabilityignored = FALSE;
}

/** converts SCIP's objective sense into CPLEX's objective sense */
static
int cpxObjsen(SCIP_OBJSEN objsen)
{
   switch( objsen )
   {
   case SCIP_OBJSEN_MAXIMIZE:
      return CPX_MAX;
   case SCIP_OBJSEN_MINIMIZE:
      return CPX_MIN;
   default:
      SCIPerrorMessage("invalid objective sense\n");
      SCIPABORT();
      return 0; /*lint !e527*/
   }
}

/** converts SCIP's lhs/rhs pairs into CPLEX's sen/rhs/rng */
static
void convertSides(
   SCIP_LPI*             lpi,                /**< LP interface structure */
   int                   nrows,              /**< number of rows */
   const SCIP_Real*      lhs,                /**< left hand side vector */
   const SCIP_Real*      rhs,                /**< right hand side vector */
   int                   indoffset,          /**< index of first row in LP */
   int*                  rngcount            /**< pointer to store the number of range rows */
   )
{
   int i;

   assert(lpi != NULL);
   assert(nrows >= 0);
   assert(lhs != NULL);
   assert(rhs != NULL);
   assert(rngcount != NULL);

   /* convert lhs/rhs into sen/rhs/rng */
   *rngcount = 0;
   for( i = 0; i < nrows; ++i )
   {
      assert(lhs[i] <= rhs[i]);
      if( lhs[i] == rhs[i] ) /*lint !e777*/
      {
         assert(-CPX_INFBOUND < rhs[i] && rhs[i] < CPX_INFBOUND);
         lpi->senarray[i] = 'E';
         lpi->rhsarray[i] = rhs[i];
      }
      else if( lhs[i] <= -CPX_INFBOUND )
      {
         assert(-CPX_INFBOUND < rhs[i] && rhs[i] < CPX_INFBOUND);
         lpi->senarray[i] = 'L';
         lpi->rhsarray[i] = rhs[i];
      }
      else if( rhs[i] >= CPX_INFBOUND )
      {
         assert(-CPX_INFBOUND < lhs[i] && lhs[i] < CPX_INFBOUND);
         lpi->senarray[i] = 'G';
         lpi->rhsarray[i] = lhs[i];
      }
      else
      {
         /* CPLEX defines a ranged row to be within rhs and rhs+rng.
          * -> To keep SCIP's meaning of the rhs value, we would like to use negative range values: rng := lhs - rhs,
          *    but there seems to be a bug in CPLEX's presolve with negative range values:
          *    the ranged row
          *              0 <= -x <= 100000 with x >= 0 (rhs=0, rng=-100000) 
          *    would lead to the CPLEX row
          *              -x -Rg = 100000 
          *                  Rg = 0
          *    instead of the correct presolving implication  Rg = -100000.
          * -> Because of this bug, we have to use an additional rhsarray[] for the converted right hand sides and
          *    use rhsarray[i] = lhs[i] and rngarray[i] = rhs[i] - lhs[i] for ranged rows to keep the range values
          *    non-negative.
          */
         lpi->senarray[i] = 'R';
         lpi->rhsarray[i] = lhs[i];
         lpi->rngarray[*rngcount] = rhs[i] - lhs[i];
         lpi->rngindarray[*rngcount] = i + indoffset;
         (*rngcount)++;
      }
   }
}

/** converts CPLEX's sen/rhs/rng triplets into SCIP's lhs/rhs pairs */
static
void reconvertBothSides(
   SCIP_LPI*             lpi,                /**< LP interface structure */
   int                   nrows,              /**< number of rows */
   SCIP_Real*            lhs,                /**< buffer to store the left hand side vector */
   SCIP_Real*            rhs                 /**< buffer to store the right hand side vector */
   )
{
   int i;

   assert(lpi != NULL);
   assert(nrows >= 0);
   assert(lhs != NULL);
   assert(rhs != NULL);

   for( i = 0; i < nrows; ++i )
   {
      switch( lpi->senarray[i] )
      {
      case 'E':
         lhs[i] = lpi->rhsarray[i];
         rhs[i] = lpi->rhsarray[i];
         break;

      case 'L':
         lhs[i] = -CPX_INFBOUND;
         rhs[i] = lpi->rhsarray[i];
         break;

      case 'G':
         lhs[i] = lpi->rhsarray[i];
         rhs[i] = CPX_INFBOUND;
         break;

      case 'R':
         assert(lpi->rngarray[i] != 0.0);
         if( lpi->rngarray[i] > 0.0 )
         {
            lhs[i] = lpi->rhsarray[i];
            rhs[i] = lpi->rhsarray[i] + lpi->rngarray[i];
         }
         else
         {
            lhs[i] = lpi->rhsarray[i] + lpi->rngarray[i];
            rhs[i] = lpi->rhsarray[i];
         }
         break;
         
      default:
         SCIPerrorMessage("invalid row sense\n");
         SCIPABORT();
      }
      assert(lhs[i] <= rhs[i]);
   }
}

/** converts CPLEX's sen/rhs/rng triplets into SCIP's lhs/rhs pairs, only storing the left hand side */
static
void reconvertLhs(
   SCIP_LPI*             lpi,                /**< LP interface structure */
   int                   nrows,              /**< number of rows */
   SCIP_Real*            lhs                 /**< buffer to store the left hand side vector */
   )
{
   int i;

   assert(lpi != NULL);
   assert(nrows >= 0);
   assert(lhs != NULL);

   for( i = 0; i < nrows; ++i )
   {
      switch( lpi->senarray[i] )
      {
      case 'E':
         assert(lpi->rngarray[i] == 0.0);
         lhs[i] = lpi->rhsarray[i];
         break;

      case 'L':
         assert(lpi->rngarray[i] == 0.0);
         lhs[i] = -CPX_INFBOUND;
         break;

      case 'G':
         assert(lpi->rngarray[i] == 0.0);
         lhs[i] = lpi->rhsarray[i];
         break;

      case 'R':
         assert(lpi->rngarray[i] != 0.0);
         if( lpi->rngarray[i] > 0.0 )
            lhs[i] = lpi->rhsarray[i];
         else
            lhs[i] = lpi->rhsarray[i] + lpi->rngarray[i];
         break;
         
      default:
         SCIPerrorMessage("invalid row sense\n");
         SCIPABORT();
      }
   }
}

/** converts CPLEX's sen/rhs/rng triplets into SCIP's lhs/rhs pairs, only storing the right hand side */
static
void reconvertRhs(
   SCIP_LPI*             lpi,                /**< LP interface structure */
   int                   nrows,              /**< number of rows */
   SCIP_Real*            rhs                 /**< buffer to store the right hand side vector */
   )
{
   int i;

   assert(lpi != NULL);
   assert(nrows >= 0);
   assert(rhs != NULL);

   for( i = 0; i < nrows; ++i )
   {
      switch( lpi->senarray[i] )
      {
      case 'E':
         assert(lpi->rngarray[i] == 0.0);
         rhs[i] = lpi->rhsarray[i];
         break;

      case 'L':
         assert(lpi->rngarray[i] == 0.0);
         rhs[i] = lpi->rhsarray[i];
         break;

      case 'G':
         assert(lpi->rngarray[i] == 0.0);
         rhs[i] = CPX_INFBOUND;
         break;

      case 'R':
         assert(lpi->rngarray[i] != 0.0);
         if( lpi->rngarray[i] > 0.0 )
            rhs[i] = lpi->rhsarray[i] + lpi->rngarray[i];
         else
            rhs[i] = lpi->rhsarray[i];
         break;
         
      default:
         SCIPerrorMessage("invalid row sense\n");
         SCIPABORT();
      }
   }
}

/** converts CPLEX's sen/rhs/rng triplets into SCIP's lhs/rhs pairs */
static
void reconvertSides(
   SCIP_LPI*             lpi,                /**< LP interface structure */
   int                   nrows,              /**< number of rows */
   SCIP_Real*            lhs,                /**< buffer to store the left hand side vector, or NULL */
   SCIP_Real*            rhs                 /**< buffer to store the right hand side vector, or NULL */
   )
{
   if( lhs != NULL && rhs != NULL )
      reconvertBothSides(lpi, nrows, lhs, rhs);
   else if( lhs != NULL )
      reconvertLhs(lpi, nrows, lhs);
   else if( rhs != NULL )
      reconvertRhs(lpi, nrows, rhs);
}




/*
 * LP Interface Methods
 */


/*
 * Miscellaneous Methods
 */

static char cpxname[SCIP_MAXSTRLEN];

/**@name Miscellaneous Methods */
/**@{ */

/** gets name and version of LP solver */
const char* SCIPlpiGetSolverName(
   void
   )
{
   snprintf(cpxname, SCIP_MAXSTRLEN, "CPLEX %.2f", (SCIP_Real)CPX_VERSION/100.0);
   return cpxname;
}

/** gets description of LP solver (developer, webpage, ...) */
const char* SCIPlpiGetSolverDesc(
   void
   )
{
   return "Linear Programming Solver developed by IBM (www-01.ibm.com/software/integration/optimization/cplex-optimizer)";
}

/** gets pointer for LP solver - use only with great care 
 *
 *  Here we return the pointer to the LP environment.
 */
void* SCIPlpiGetSolverPointer(
   SCIP_LPI*             lpi                 /**< pointer to an LP interface structure */
   )
{
   return (void*) lpi->cpxlp;
}
/**@} */




/*
 * LPI Creation and Destruction Methods
 */

/**@name LPI Creation and Destruction Methods */
/**@{ */

/** creates an LP problem object */
SCIP_RETCODE SCIPlpiCreate(
   SCIP_LPI**            lpi,                /**< pointer to an LP interface structure */
   const char*           name,               /**< problem name */
   SCIP_OBJSEN           objsen              /**< objective sense */
   )
{
   int          restat;

   assert(sizeof(SCIP_Real) == sizeof(double)); /* CPLEX only works with doubles as floating points */
   assert(sizeof(SCIP_Bool) == sizeof(int));    /* CPLEX only works with ints as bools */
   assert(lpi != NULL);
   assert(numlp >= 0);

   SCIPdebugMessage("SCIPlpiCreate()\n");

   /* create environment */
   if( cpxenv == NULL )
   {
      assert(numlp == 0);
      cpxenv = CPXopenCPLEX(&restat);
      CHECK_ZERO( restat );

#if (CPX_VERSION == 1100 || CPX_VERSION == 1220)
      /* manually set number of threads to 1 to avoid huge system load due to CPLEX bug (version 1100) or segmentation fault (version 1220) */
      CHECK_ZERO( CPXsetintparam(cpxenv, CPX_PARAM_THREADS, 1) );
#endif

#if 0 /* turning presolve off seems to be faster than turning it off on demand (if presolve detects infeasibility) */
      /* turn presolve off, s.t. for an infeasible problem, a ray is always available */
      CHECK_ZERO( CPXsetintparam(cpxenv, CPX_PARAM_PREIND, CPX_OFF) );
#endif

      /* get default parameter values */
      SCIP_CALL( getParameterValues(&defparam) );
      copyParameterValues(&curparam, &defparam);
   }
   assert(cpxenv != NULL);

   /* create LP */
   SCIP_ALLOC( BMSallocMemory(lpi) );
   (*lpi)->larray = NULL;
   (*lpi)->uarray = NULL;
   (*lpi)->senarray = NULL;
   (*lpi)->rhsarray = NULL;
   (*lpi)->rngarray = NULL;
   (*lpi)->valarray = NULL;
   (*lpi)->rngindarray = NULL;
   (*lpi)->cstat = NULL;
   (*lpi)->rstat = NULL;
   (*lpi)->indarray = NULL;
   (*lpi)->boundchgsize = 0;
   (*lpi)->sidechgsize = 0;
   (*lpi)->valsize = 0;
   (*lpi)->cstatsize = 0;
   (*lpi)->rstatsize = 0;
   (*lpi)->iterations = 0;
   (*lpi)->pricing = SCIP_PRICING_LPIDEFAULT;
   (*lpi)->solisbasic = TRUE;
   (*lpi)->cpxlp = CPXcreateprob(cpxenv, &restat, name);
   (*lpi)->instabilityignored = FALSE;
#if (CPX_VERSION <= 1100)
   (*lpi)->rngfound = FALSE;
#endif
   CHECK_ZERO( restat );
   invalidateSolution(*lpi);
   copyParameterValues(&((*lpi)->cpxparam), &defparam);
   numlp++;

   /* set objective sense */
   SCIP_CALL( SCIPlpiChgObjsen(*lpi, objsen) );

   /* set default pricing */
   SCIP_CALL( SCIPlpiSetIntpar(*lpi, SCIP_LPPAR_PRICING, (int)(*lpi)->pricing) );

   return SCIP_OKAY;
}

/** deletes an LP problem object */
SCIP_RETCODE SCIPlpiFree(
   SCIP_LPI**            lpi                 /**< pointer to an LP interface structure */
   )
{
   assert(cpxenv != NULL);
   assert(lpi != NULL);
   assert(*lpi != NULL);

   SCIPdebugMessage("SCIPlpiFree()\n");

   /* free LP */
   CHECK_ZERO( CPXfreeprob(cpxenv, &((*lpi)->cpxlp)) );

   /* free memory */
   BMSfreeMemoryArrayNull(&(*lpi)->larray);
   BMSfreeMemoryArrayNull(&(*lpi)->uarray);
   BMSfreeMemoryArrayNull(&(*lpi)->senarray);
   BMSfreeMemoryArrayNull(&(*lpi)->rhsarray);
   BMSfreeMemoryArrayNull(&(*lpi)->rngarray);
   BMSfreeMemoryArrayNull(&(*lpi)->rngindarray);
   BMSfreeMemoryArrayNull(&(*lpi)->cstat);
   BMSfreeMemoryArrayNull(&(*lpi)->rstat);
   BMSfreeMemory(lpi);

   /* free environment */
   numlp--;
   if( numlp == 0 )
   {
      CHECK_ZERO( CPXcloseCPLEX(&cpxenv) );
   }

   return SCIP_OKAY;
}

/**@} */




/*
 * Modification Methods
 */

/**@name Modification Methods */
/**@{ */

/** copies LP data with column matrix into LP solver */
SCIP_RETCODE SCIPlpiLoadColLP(
   SCIP_LPI*             lpi,                /**< LP interface structure */
   SCIP_OBJSEN           objsen,             /**< objective sense */
   int                   ncols,              /**< number of columns */
   const SCIP_Real*      obj,                /**< objective function values of columns */
   const SCIP_Real*      lb,                 /**< lower bounds of columns */
   const SCIP_Real*      ub,                 /**< upper bounds of columns */
   char**                colnames,           /**< column names, or NULL */
   int                   nrows,              /**< number of rows */
   const SCIP_Real*      lhs,                /**< left hand sides of rows */
   const SCIP_Real*      rhs,                /**< right hand sides of rows */
   char**                rownames,           /**< row names, or NULL */
   int                   nnonz,              /**< number of nonzero elements in the constraint matrix */
   const int*            beg,                /**< start index of each column in ind- and val-array */
   const int*            ind,                /**< row indices of constraint matrix entries */
   const SCIP_Real*      val                 /**< values of constraint matrix entries */
   )
{
   int* cnt;
   int rngcount;
   int c;

   assert(cpxenv != NULL);
   assert(lpi != NULL);
   assert(lpi->cpxlp != NULL);

   SCIPdebugMessage("loading LP in column format into CPLEX: %d cols, %d rows\n", ncols, nrows);

   invalidateSolution(lpi);

   SCIP_CALL( ensureSidechgMem(lpi, nrows) );

   /* convert lhs/rhs into sen/rhs/range tuples */
   convertSides(lpi, nrows, lhs, rhs, 0, &rngcount);

   /* calculate column lengths */
   SCIP_ALLOC( BMSallocMemoryArray(&cnt, ncols) );
   for( c = 0; c < ncols-1; ++c )
   {
      cnt[c] = beg[c+1] - beg[c];
      assert(cnt[c] >= 0);
   }
   cnt[ncols-1] = nnonz - beg[ncols-1];
   assert(cnt[ncols-1] >= 0);

   /* copy data into CPLEX */
   CHECK_ZERO( CPXcopylpwnames(cpxenv, lpi->cpxlp, ncols, nrows, cpxObjsen(objsen), obj, 
                  lpi->rhsarray, lpi->senarray, beg, cnt, ind, val, lb, ub, lpi->rngarray, colnames, rownames) );

   /* free temporary memory */
   BMSfreeMemoryArray(&cnt);

   assert(CPXgetnumcols(cpxenv, lpi->cpxlp) == ncols);
   assert(CPXgetnumrows(cpxenv, lpi->cpxlp) == nrows);
   assert(CPXgetnumnz(cpxenv, lpi->cpxlp) == nnonz);

   return SCIP_OKAY;
}

/** adds columns to the LP */
SCIP_RETCODE SCIPlpiAddCols(
   SCIP_LPI*             lpi,                /**< LP interface structure */
   int                   ncols,              /**< number of columns to be added */
   const SCIP_Real*      obj,                /**< objective function values of new columns */
   const SCIP_Real*      lb,                 /**< lower bounds of new columns */
   const SCIP_Real*      ub,                 /**< upper bounds of new columns */
   char**                colnames,           /**< column names, or NULL */
   int                   nnonz,              /**< number of nonzero elements to be added to the constraint matrix */
   const int*            beg,                /**< start index of each column in ind- and val-array, or NULL if nnonz == 0 */
   const int*            ind,                /**< row indices of constraint matrix entries, or NULL if nnonz == 0 */
   const SCIP_Real*      val                 /**< values of constraint matrix entries, or NULL if nnonz == 0 */
   )
{
   assert(cpxenv != NULL);
   assert(lpi != NULL);
   assert(lpi->cpxlp != NULL);

   SCIPdebugMessage("adding %d columns with %d nonzeros to CPLEX\n", ncols, nnonz);

   invalidateSolution(lpi);

   if( nnonz > 0 )
   {
      CHECK_ZERO( CPXaddcols(cpxenv, lpi->cpxlp, ncols, nnonz, obj, beg, ind, val, lb, ub, colnames) );
   }
   else
   {
      CHECK_ZERO( CPXnewcols(cpxenv, lpi->cpxlp, ncols, obj, lb, ub, NULL, colnames) );
   }

   return SCIP_OKAY;
}

/** deletes all columns in the given range from LP */
SCIP_RETCODE SCIPlpiDelCols(
   SCIP_LPI*             lpi,                /**< LP interface structure */
   int                   firstcol,           /**< first column to be deleted */
   int                   lastcol             /**< last column to be deleted */
   )
{
   assert(cpxenv != NULL);
   assert(lpi != NULL);
   assert(lpi->cpxlp != NULL);
   assert(0 <= firstcol && firstcol <= lastcol && lastcol < CPXgetnumcols(cpxenv, lpi->cpxlp));

   SCIPdebugMessage("deleting %d columns from CPLEX\n", lastcol - firstcol + 1);

   invalidateSolution(lpi);

   CHECK_ZERO( CPXdelcols(cpxenv, lpi->cpxlp, firstcol, lastcol) );

   return SCIP_OKAY;   
}

/** deletes columns from SCIP_LP; the new position of a column must not be greater that its old position */
SCIP_RETCODE SCIPlpiDelColset(
   SCIP_LPI*             lpi,                /**< LP interface structure */
   int*                  dstat               /**< deletion status of columns
                                              *   input:  1 if column should be deleted, 0 if not
                                              *   output: new position of column, -1 if column was deleted */
   )
{
   assert(cpxenv != NULL);
   assert(lpi != NULL);
   assert(lpi->cpxlp != NULL);

   SCIPdebugMessage("deleting a column set from CPLEX\n");

   invalidateSolution(lpi);

   CHECK_ZERO( CPXdelsetcols(cpxenv, lpi->cpxlp, dstat) );

   return SCIP_OKAY;   
}

/** adds rows to the LP */
SCIP_RETCODE SCIPlpiAddRows(
   SCIP_LPI*             lpi,                /**< LP interface structure */
   int                   nrows,              /**< number of rows to be added */
   const SCIP_Real*      lhs,                /**< left hand sides of new rows */
   const SCIP_Real*      rhs,                /**< right hand sides of new rows */
   char**                rownames,           /**< row names, or NULL */
   int                   nnonz,              /**< number of nonzero elements to be added to the constraint matrix */
   const int*            beg,                /**< start index of each row in ind- and val-array, or NULL if nnonz == 0 */
   const int*            ind,                /**< column indices of constraint matrix entries, or NULL if nnonz == 0 */
   const SCIP_Real*      val                 /**< values of constraint matrix entries, or NULL if nnonz == 0 */
   )
{
   int rngcount;

   assert(cpxenv != NULL);
   assert(lpi != NULL);
   assert(lpi->cpxlp != NULL);

   SCIPdebugMessage("adding %d rows with %d nonzeros to CPLEX\n", nrows, nnonz);

   invalidateSolution(lpi);

   SCIP_CALL( ensureSidechgMem(lpi, nrows) );

   /* convert lhs/rhs into sen/rhs/range tuples */
   convertSides(lpi, nrows, lhs, rhs, CPXgetnumrows(cpxenv, lpi->cpxlp), &rngcount);

   /* add rows to LP */
   if( nnonz > 0 )
   {
      CHECK_ZERO( CPXaddrows(cpxenv, lpi->cpxlp, 0, nrows, nnonz, lpi->rhsarray, lpi->senarray, beg, ind, val, NULL,
            rownames) );
   }
   else
   {
      CHECK_ZERO( CPXnewrows(cpxenv, lpi->cpxlp, nrows, lpi->rhsarray, lpi->senarray, NULL, rownames) );
   }
   if( rngcount > 0 )
   {
#if (CPX_VERSION <= 1100)
      if( lpi->rngfound == FALSE )
      {
         SCIP_CALL( SCIPlpiSetIntpar(lpi, SCIP_LPPAR_SCALING, FALSE) );
         lpi->rngfound = TRUE;
      }
#endif
      CHECK_ZERO( CPXchgrngval(cpxenv, lpi->cpxlp, rngcount, lpi->rngindarray, lpi->rngarray) );
   }

   return SCIP_OKAY;
}

/** deletes all rows in the given range from LP */
SCIP_RETCODE SCIPlpiDelRows(
   SCIP_LPI*             lpi,                /**< LP interface structure */
   int                   firstrow,           /**< first row to be deleted */
   int                   lastrow             /**< last row to be deleted */
   )
{
   assert(cpxenv != NULL);
   assert(lpi != NULL);
   assert(lpi->cpxlp != NULL);
   assert(0 <= firstrow && firstrow <= lastrow && lastrow < CPXgetnumrows(cpxenv, lpi->cpxlp));

   SCIPdebugMessage("deleting %d rows from CPLEX\n", lastrow - firstrow + 1);

   invalidateSolution(lpi);

   CHECK_ZERO( CPXdelrows(cpxenv, lpi->cpxlp, firstrow, lastrow) );

   return SCIP_OKAY;   
}

/** deletes rows from SCIP_LP; the new position of a row must not be greater that its old position */
SCIP_RETCODE SCIPlpiDelRowset(
   SCIP_LPI*             lpi,                /**< LP interface structure */
   int*                  dstat               /**< deletion status of rows
                                              *   input:  1 if row should be deleted, 0 if not
                                              *   output: new position of row, -1 if row was deleted */
   )
{
   assert(cpxenv != NULL);
   assert(lpi != NULL);
   assert(lpi->cpxlp != NULL);

   SCIPdebugMessage("deleting a row set from CPLEX\n");

   invalidateSolution(lpi);

   CHECK_ZERO( CPXdelsetrows(cpxenv, lpi->cpxlp, dstat) );

   return SCIP_OKAY;   
}

/** clears the whole LP */
SCIP_RETCODE SCIPlpiClear(
   SCIP_LPI*             lpi                 /**< LP interface structure */
   )
{
   int ncols;
   int nrows;

   assert(cpxenv != NULL);
   assert(lpi != NULL);
   assert(lpi->cpxlp != NULL);

   SCIPdebugMessage("clearing CPLEX LP\n");

   invalidateSolution(lpi);

   ncols = CPXgetnumcols(cpxenv, lpi->cpxlp);
   nrows = CPXgetnumrows(cpxenv, lpi->cpxlp);
   if( ncols >= 1 )
   {
      CHECK_ZERO( CPXdelcols(cpxenv, lpi->cpxlp, 0, ncols-1) );
   }
   if( nrows >= 1 )
   {
      CHECK_ZERO( CPXdelrows(cpxenv, lpi->cpxlp, 0, nrows-1) );
   }

   return SCIP_OKAY;
}

/** changes lower and upper bounds of columns */
SCIP_RETCODE SCIPlpiChgBounds(
   SCIP_LPI*             lpi,                /**< LP interface structure */
   int                   ncols,              /**< number of columns to change bounds for */
   const int*            ind,                /**< column indices */
   const SCIP_Real*      lb,                 /**< values for the new lower bounds */
   const SCIP_Real*      ub                  /**< values for the new upper bounds */
   )
{
   assert(cpxenv != NULL);
   assert(lpi != NULL);
   assert(lpi->cpxlp != NULL);

   SCIPdebugMessage("changing %d bounds in CPLEX\n", ncols);
#ifdef SCIP_DEBUG
   {
      int i;
      for( i = 0; i < ncols; ++i )
         SCIPdebugPrintf("  col %d: [%g,%g]\n", ind[i], lb[i], ub[i]);
   }
#endif

   invalidateSolution(lpi);

   SCIP_CALL( ensureBoundchgMem(lpi, ncols) );

   CHECK_ZERO( CPXchgbds(cpxenv, lpi->cpxlp, ncols, ind, lpi->larray, (SCIP_Real*)lb) );
   CHECK_ZERO( CPXchgbds(cpxenv, lpi->cpxlp, ncols, ind, lpi->uarray, (SCIP_Real*)ub) );

   return SCIP_OKAY;
}

/** changes left and right hand sides of rows */
SCIP_RETCODE SCIPlpiChgSides(
   SCIP_LPI*             lpi,                /**< LP interface structure */
   int                   nrows,              /**< number of rows to change sides for */
   const int*            ind,                /**< row indices */
   const SCIP_Real*      lhs,                /**< new values for left hand sides */
   const SCIP_Real*      rhs                 /**< new values for right hand sides */
   )
{
   int rngcount;
   int i;

   assert(cpxenv != NULL);
   assert(lpi != NULL);
   assert(lpi->cpxlp != NULL);

   SCIPdebugMessage("changing %d sides in CPLEX\n", nrows);

   invalidateSolution(lpi);

   SCIP_CALL( ensureSidechgMem(lpi, nrows) );

   /* convert lhs/rhs into sen/rhs/range tuples */
   convertSides(lpi, nrows, lhs, rhs, 0, &rngcount);

   /* change row sides */
   CHECK_ZERO( CPXchgsense(cpxenv, lpi->cpxlp, nrows, ind, lpi->senarray) );
   CHECK_ZERO( CPXchgrhs(cpxenv, lpi->cpxlp, nrows, ind, lpi->rhsarray) );
   if( rngcount > 0 )
   {
      /* adjust the range count indices to the correct row indices */
      for( i = 0; i < rngcount; ++i )
      {
         assert(0 <= lpi->rngindarray[i] && lpi->rngindarray[i] < nrows);
         assert(lpi->senarray[lpi->rngindarray[i]] == 'R');
         lpi->rngindarray[i] = ind[lpi->rngindarray[i]];
      }

      /* change the range values in CPLEX */
      CHECK_ZERO( CPXchgrngval(cpxenv, lpi->cpxlp, rngcount, lpi->rngindarray, lpi->rngarray) );
   }

   return SCIP_OKAY;
}

/** changes a single coefficient */
SCIP_RETCODE SCIPlpiChgCoef(
   SCIP_LPI*             lpi,                /**< LP interface structure */
   int                   row,                /**< row number of coefficient to change */
   int                   col,                /**< column number of coefficient to change */
   SCIP_Real             newval              /**< new value of coefficient */
   )
{
   assert(cpxenv != NULL);
   assert(lpi != NULL);
   assert(lpi->cpxlp != NULL);

   SCIPdebugMessage("changing coefficient row %d, column %d in CPLEX to %g\n", row, col, newval);

   invalidateSolution(lpi);

   CHECK_ZERO( CPXchgcoef(cpxenv, lpi->cpxlp, row, col, newval) );

   return SCIP_OKAY;
}

/** changes the objective sense */
SCIP_RETCODE SCIPlpiChgObjsen(
   SCIP_LPI*             lpi,                /**< LP interface structure */
   SCIP_OBJSEN           objsen              /**< new objective sense */
   )
{
   assert(cpxenv != NULL);
   assert(lpi != NULL);
   assert(lpi->cpxlp != NULL);

   SCIPdebugMessage("changing objective sense in CPLEX to %d\n", objsen);

   invalidateSolution(lpi);
   
   CPXchgobjsen(cpxenv, lpi->cpxlp, cpxObjsen(objsen));

   return SCIP_OKAY;
}

/** changes objective values of columns in the LP */
SCIP_RETCODE SCIPlpiChgObj(
   SCIP_LPI*             lpi,                /**< LP interface structure */
   int                   ncols,              /**< number of columns to change objective value for */
   int*                  ind,                /**< column indices to change objective value for */
   SCIP_Real*            obj                 /**< new objective values for columns */
   )
{
   assert(cpxenv != NULL);
   assert(lpi != NULL);
   assert(lpi->cpxlp != NULL);

   SCIPdebugMessage("changing %d objective values in CPLEX\n", ncols);

   CHECK_ZERO( CPXchgobj(cpxenv, lpi->cpxlp, ncols, ind, obj) );

   return SCIP_OKAY;
}

/** multiplies a row with a non-zero scalar; for negative scalars, the row's sense is switched accordingly */
SCIP_RETCODE SCIPlpiScaleRow(
   SCIP_LPI*             lpi,                /**< LP interface structure */
   int                   row,                /**< row number to scale */
   SCIP_Real             scaleval            /**< scaling multiplier */
   )
{
   SCIP_Real lhs;
   SCIP_Real rhs;
   int nnonz;
   int beg;
   int i;

   assert(cpxenv != NULL);
   assert(lpi != NULL);
   assert(lpi->cpxlp != NULL);
   assert(scaleval != 0.0);

   SCIPdebugMessage("scaling row %d with factor %g in CPLEX\n", row, scaleval);

   invalidateSolution(lpi);

   SCIP_CALL( ensureValMem(lpi, CPXgetnumcols(cpxenv, lpi->cpxlp)) );

   /* get the row */
   SCIP_CALL( SCIPlpiGetRows(lpi, row, row, &lhs, &rhs, &nnonz, &beg, lpi->indarray, lpi->valarray) );

   /* scale row coefficients */
   for( i = 0; i < nnonz; ++i )
   {
      SCIP_CALL( SCIPlpiChgCoef(lpi, row, lpi->indarray[i], lpi->valarray[i] * scaleval) );
   }

   /* scale row sides */
   if( lhs > -CPX_INFBOUND )
      lhs *= scaleval;
   else if( scaleval < 0.0 )
      lhs = CPX_INFBOUND;
   if( rhs < CPX_INFBOUND )
      rhs *= scaleval;
   else if( scaleval < 0.0 )
      rhs = -CPX_INFBOUND;
   if( scaleval > 0.0 )
   {
      SCIP_CALL( SCIPlpiChgSides(lpi, 1, &row, &lhs, &rhs) );
   }
   else
   {
      SCIP_CALL( SCIPlpiChgSides(lpi, 1, &row, &rhs, &lhs) );
   }

   return SCIP_OKAY;
}

/** multiplies a column with a non-zero scalar; the objective value is multiplied with the scalar, and the bounds
 *  are divided by the scalar; for negative scalars, the column's bounds are switched
 */
SCIP_RETCODE SCIPlpiScaleCol(
   SCIP_LPI*             lpi,                /**< LP interface structure */
   int                   col,                /**< column number to scale */
   SCIP_Real             scaleval            /**< scaling multiplier */
   )
{
   SCIP_Real lb;
   SCIP_Real ub;
   SCIP_Real obj;
   int nnonz;
   int beg;
   int i;

   assert(cpxenv != NULL);
   assert(lpi != NULL);
   assert(lpi->cpxlp != NULL);
   assert(scaleval != 0.0);

   SCIPdebugMessage("scaling column %d with factor %g in CPLEX\n", col, scaleval);

   invalidateSolution(lpi);

   SCIP_CALL( ensureValMem(lpi, CPXgetnumcols(cpxenv, lpi->cpxlp)) );

   /* get the column */
   SCIP_CALL( SCIPlpiGetCols(lpi, col, col, &lb, &ub, &nnonz, &beg, lpi->indarray, lpi->valarray) );

   /** get objective coefficient */
   SCIP_CALL( SCIPlpiGetObj(lpi, col, col, &obj) );

   /* scale column coefficients */
   for( i = 0; i < nnonz; ++i )
   {
      SCIP_CALL( SCIPlpiChgCoef(lpi, lpi->indarray[i], col, lpi->valarray[i] * scaleval) );
   }

   /* scale objective value */
   obj *= scaleval;
   SCIP_CALL( SCIPlpiChgObj(lpi, 1, &col, &obj) );

   /* scale column bounds */
   if( lb > -CPX_INFBOUND )
      lb /= scaleval;
   else if( scaleval < 0.0 )
      lb = CPX_INFBOUND;
   if( ub < CPX_INFBOUND )
      ub /= scaleval;
   else if( scaleval < 0.0 )
      ub = -CPX_INFBOUND;
   if( scaleval > 0.0 )
   {
      SCIP_CALL( SCIPlpiChgBounds(lpi, 1, &col, &lb, &ub) );
   }
   else
   {
      SCIP_CALL( SCIPlpiChgBounds(lpi, 1, &col, &ub, &lb) );
   }

   return SCIP_OKAY;
}

/**@} */




/*
 * Data Accessing Methods
 */

/**@name Data Accessing Methods */
/**@{ */

/** gets the number of rows in the LP */
SCIP_RETCODE SCIPlpiGetNRows(
   SCIP_LPI*             lpi,                /**< LP interface structure */
   int*                  nrows               /**< pointer to store the number of rows */
   )
{
   assert(cpxenv != NULL);
   assert(lpi != NULL);
   assert(nrows != NULL);

   SCIPdebugMessage("getting number of rows\n");

   *nrows = CPXgetnumrows(cpxenv, lpi->cpxlp);

   return SCIP_OKAY;
}

/** gets the number of columns in the LP */
SCIP_RETCODE SCIPlpiGetNCols(
   SCIP_LPI*             lpi,                /**< LP interface structure */
   int*                  ncols               /**< pointer to store the number of cols */
   )
{
   assert(cpxenv != NULL);
   assert(lpi != NULL);
   assert(ncols != NULL);

   SCIPdebugMessage("getting number of columns\n");

   *ncols = CPXgetnumcols(cpxenv, lpi->cpxlp);

   return SCIP_OKAY;
}

/** gets the number of nonzero elements in the LP constraint matrix */
SCIP_RETCODE SCIPlpiGetNNonz(
   SCIP_LPI*             lpi,                /**< LP interface structure */
   int*                  nnonz               /**< pointer to store the number of nonzeros */
   )
{
   assert(cpxenv != NULL);
   assert(lpi != NULL);
   assert(nnonz != NULL);

   SCIPdebugMessage("getting number of non-zeros\n");

   *nnonz = CPXgetnumnz(cpxenv, lpi->cpxlp);

   return SCIP_OKAY;
}

/** gets columns from LP problem object; the arrays have to be large enough to store all values
 *  Either both, lb and ub, have to be NULL, or both have to be non-NULL,
 *  either nnonz, beg, ind, and val have to be NULL, or all of them have to be non-NULL.
 */
SCIP_RETCODE SCIPlpiGetCols(
   SCIP_LPI*             lpi,                /**< LP interface structure */
   int                   firstcol,           /**< first column to get from LP */
   int                   lastcol,            /**< last column to get from LP */
   SCIP_Real*            lb,                 /**< buffer to store the lower bound vector, or NULL */
   SCIP_Real*            ub,                 /**< buffer to store the upper bound vector, or NULL */
   int*                  nnonz,              /**< pointer to store the number of nonzero elements returned, or NULL */
   int*                  beg,                /**< buffer to store start index of each column in ind- and val-array, or NULL */
   int*                  ind,                /**< buffer to store column indices of constraint matrix entries, or NULL */
   SCIP_Real*            val                 /**< buffer to store values of constraint matrix entries, or NULL */
   )
{
   assert(cpxenv != NULL);
   assert(lpi != NULL);
   assert(lpi->cpxlp != NULL);
   assert(0 <= firstcol && firstcol <= lastcol && lastcol < CPXgetnumcols(cpxenv, lpi->cpxlp));

   SCIPdebugMessage("getting columns %d to %d\n", firstcol, lastcol);

   if( lb != NULL )
   {
      assert(ub != NULL);

      CHECK_ZERO( CPXgetlb(cpxenv, lpi->cpxlp, lb, firstcol, lastcol) );
      CHECK_ZERO( CPXgetub(cpxenv, lpi->cpxlp, ub, firstcol, lastcol) );
   }
   else
      assert(ub == NULL);

   if( nnonz != NULL )
   {
      int surplus;

      assert(beg != NULL);
      assert(ind != NULL);
      assert(val != NULL);

      /* get matrix entries */
      CHECK_ZERO( CPXgetcols(cpxenv, lpi->cpxlp, nnonz, beg, ind, val, CPXgetnumnz(cpxenv, lpi->cpxlp), &surplus, 
                     firstcol, lastcol) );
      assert(surplus >= 0);
   }
   else
   {
      assert(beg == NULL);
      assert(ind == NULL);
      assert(val == NULL);
   }

   return SCIP_OKAY;
}

/** gets rows from LP problem object; the arrays have to be large enough to store all values.
 *  Either both, lhs and rhs, have to be NULL, or both have to be non-NULL,
 *  either nnonz, beg, ind, and val have to be NULL, or all of them have to be non-NULL.
 */
SCIP_RETCODE SCIPlpiGetRows(
   SCIP_LPI*             lpi,                /**< LP interface structure */
   int                   firstrow,           /**< first row to get from LP */
   int                   lastrow,            /**< last row to get from LP */
   SCIP_Real*            lhs,                /**< buffer to store left hand side vector, or NULL */
   SCIP_Real*            rhs,                /**< buffer to store right hand side vector, or NULL */
   int*                  nnonz,              /**< pointer to store the number of nonzero elements returned, or NULL */
   int*                  beg,                /**< buffer to store start index of each row in ind- and val-array, or NULL */
   int*                  ind,                /**< buffer to store row indices of constraint matrix entries, or NULL */
   SCIP_Real*            val                 /**< buffer to store values of constraint matrix entries, or NULL */
   )
{
   int retcode;

   assert(cpxenv != NULL);
   assert(lpi != NULL);
   assert(lpi->cpxlp != NULL);
   assert(0 <= firstrow && firstrow <= lastrow && lastrow < CPXgetnumrows(cpxenv, lpi->cpxlp));

   SCIPdebugMessage("getting rows %d to %d\n", firstrow, lastrow);

   if( lhs != NULL || rhs != NULL )
   {
      /* get row sense, rhs, and ranges */
      SCIP_CALL( ensureSidechgMem(lpi, lastrow - firstrow + 1) );
      CHECK_ZERO( CPXgetsense(cpxenv, lpi->cpxlp, lpi->senarray, firstrow, lastrow) );
      CHECK_ZERO( CPXgetrhs(cpxenv, lpi->cpxlp, lpi->rhsarray, firstrow, lastrow) );
      retcode = CPXgetrngval(cpxenv, lpi->cpxlp, lpi->rngarray, firstrow, lastrow);
      if( retcode != CPXERR_NO_RNGVAL ) /* ignore "No range values" error */
      {
         CHECK_ZERO( retcode );
      }
      else
         BMSclearMemoryArray(lpi->rngarray, lastrow-firstrow+1);

      /* convert sen/rhs/range into lhs/rhs tuples */
      reconvertSides(lpi, lastrow - firstrow + 1, lhs, rhs);
   }

   if( nnonz != NULL )
   {
      int surplus;

      assert(beg != NULL);
      assert(ind != NULL);
      assert(val != NULL);

      /* get matrix entries */
      CHECK_ZERO( CPXgetrows(cpxenv, lpi->cpxlp, nnonz, beg, ind, val, CPXgetnumnz(cpxenv, lpi->cpxlp), &surplus, 
                     firstrow, lastrow) );
      assert(surplus >= 0);
   }
   else
   {
      assert(beg == NULL);
      assert(ind == NULL);
      assert(val == NULL);
   }

   return SCIP_OKAY;
}

/** gets column names */
SCIP_RETCODE SCIPlpiGetColNames(
   SCIP_LPI*             lpi,                /**< LP interface structure */
   int                   firstcol,           /**< first column to get name from LP */
   int                   lastcol,            /**< last column to get name from LP */
   char**                colnames,           /**< pointers to column names (of size at least lastcol-firstcol+1) */
   char*                 namestorage,        /**< storage for col names */
   int                   namestoragesize,    /**< size of namestorage (if 0, storageleft returns the storage needed) */
   int*                  storageleft         /**< amount of storage left (if < 0 the namestorage was not big enough) */
   )
{
   int retcode;

   assert( cpxenv != NULL );
   assert( lpi != NULL );
   assert( lpi->cpxlp != NULL );
   assert( colnames != NULL || namestoragesize == 0 );
   assert( namestorage != NULL || namestoragesize == 0 );
   assert( namestoragesize >= 0 );
   assert( storageleft != NULL );
   assert( 0 <= firstcol && firstcol <= lastcol && lastcol < CPXgetnumcols(cpxenv, lpi->cpxlp) );

   SCIPdebugMessage("getting column names %d to %d\n", firstcol, lastcol);

   retcode =  CPXgetcolname(cpxenv, lpi->cpxlp, colnames, namestorage, namestoragesize, storageleft, firstcol, lastcol);
   assert( namestoragesize != 0 || retcode == CPXERR_NEGATIVE_SURPLUS );
   if ( namestoragesize != 0 )
   {
      CHECK_ZERO( retcode );
   }

   return SCIP_OKAY;
}

/** gets row names */
SCIP_RETCODE SCIPlpiGetRowNames(
   SCIP_LPI*             lpi,                /**< LP interface structure */
   int                   firstrow,           /**< first row to get name from LP */
   int                   lastrow,            /**< last row to get name from LP */
   char**                rownames,           /**< pointers to row names (of size at least lastrow-firstrow+1) */
   char*                 namestorage,        /**< storage for row names */
   int                   namestoragesize,    /**< size of namestorage (if 0, -storageleft returns the storage needed) */
   int*                  storageleft         /**< amount of storage left (if < 0 the namestorage was not big enough) */
   )
{
   int retcode;

   assert( cpxenv != NULL );
   assert( lpi != NULL );
   assert( lpi->cpxlp != NULL );
   assert( rownames != NULL || namestoragesize == 0 );
   assert( namestorage != NULL || namestoragesize == 0 );
   assert( namestoragesize >= 0 );
   assert( storageleft != NULL );
   assert( 0 <= firstrow && firstrow <= lastrow && lastrow < CPXgetnumrows(cpxenv, lpi->cpxlp) );

   SCIPdebugMessage("getting row names %d to %d\n", firstrow, lastrow);

   retcode = CPXgetrowname(cpxenv, lpi->cpxlp, rownames, namestorage, namestoragesize, storageleft, firstrow, lastrow);
   assert( namestoragesize != 0 || retcode == CPXERR_NEGATIVE_SURPLUS );
   if ( namestoragesize != 0 )
   {
      CHECK_ZERO( retcode );
   }

   return SCIP_OKAY;
}

/** gets objective coefficients from LP problem object */
SCIP_RETCODE SCIPlpiGetObj(
   SCIP_LPI*             lpi,                /**< LP interface structure */
   int                   firstcol,           /**< first column to get objective coefficient for */
   int                   lastcol,            /**< last column to get objective coefficient for */
   SCIP_Real*            vals                /**< array to store objective coefficients */
   )
{
   assert(cpxenv != NULL);
   assert(lpi != NULL);
   assert(lpi->cpxlp != NULL);
   assert(firstcol <= lastcol);
   assert(vals != NULL);
   
   SCIPdebugMessage("getting objective values %d to %d\n", firstcol, lastcol);

   CHECK_ZERO( CPXgetobj(cpxenv, lpi->cpxlp, vals, firstcol, lastcol) );

   return SCIP_OKAY;
}

/** gets current bounds from LP problem object */
SCIP_RETCODE SCIPlpiGetBounds(
   SCIP_LPI*             lpi,                /**< LP interface structure */
   int                   firstcol,           /**< first column to get bounds for */
   int                   lastcol,            /**< last column to get bounds for */
   SCIP_Real*            lbs,                /**< array to store lower bound values, or NULL */
   SCIP_Real*            ubs                 /**< array to store upper bound values, or NULL */
   )
{
   assert(cpxenv != NULL);
   assert(lpi != NULL);
   assert(lpi->cpxlp != NULL);
   assert(firstcol <= lastcol);
   
   SCIPdebugMessage("getting bounds %d to %d\n", firstcol, lastcol);

   if( lbs != NULL )
   {
      CHECK_ZERO( CPXgetlb(cpxenv, lpi->cpxlp, lbs, firstcol, lastcol) );
   }

   if( ubs != NULL )
   {
      CHECK_ZERO( CPXgetub(cpxenv, lpi->cpxlp, ubs, firstcol, lastcol) );
   }

   return SCIP_OKAY;
}

/** gets current row sides from LP problem object */
SCIP_RETCODE SCIPlpiGetSides(
   SCIP_LPI*             lpi,                /**< LP interface structure */
   int                   firstrow,           /**< first row to get sides for */
   int                   lastrow,            /**< last row to get sides for */
   SCIP_Real*            lhss,               /**< array to store left hand side values, or NULL */
   SCIP_Real*            rhss                /**< array to store right hand side values, or NULL */
   )
{
   int retval;

   assert(cpxenv != NULL);
   assert(lpi != NULL);
   assert(lpi->cpxlp != NULL);
   assert(firstrow <= lastrow);
   
   SCIPdebugMessage("getting row sides %d to %d\n", firstrow, lastrow);

   /* get row sense, rhs, and ranges */
   SCIP_CALL( ensureSidechgMem(lpi, lastrow - firstrow + 1) );
   CHECK_ZERO( CPXgetsense(cpxenv, lpi->cpxlp, lpi->senarray, firstrow, lastrow) );
   CHECK_ZERO( CPXgetrhs(cpxenv, lpi->cpxlp, lpi->rhsarray, firstrow, lastrow) );
   retval = CPXgetrngval(cpxenv, lpi->cpxlp, lpi->rngarray, firstrow, lastrow);
   if( retval != CPXERR_NO_RNGVAL ) /* ignore "No range values" error */
   {
      CHECK_ZERO( retval );
   }
   else
      BMSclearMemoryArray(lpi->rngarray, lastrow-firstrow+1);
   
   /* convert sen/rhs/range into lhs/rhs tuples */
   reconvertSides(lpi, lastrow - firstrow + 1, lhss, rhss);

   return SCIP_OKAY;
}

/** gets a single coefficient */
SCIP_RETCODE SCIPlpiGetCoef(
   SCIP_LPI*             lpi,                /**< LP interface structure */
   int                   row,                /**< row number of coefficient */
   int                   col,                /**< column number of coefficient */
   SCIP_Real*            val                 /**< pointer to store the value of the coefficient */
   )
{
   assert(cpxenv != NULL);
   assert(lpi != NULL);
   assert(lpi->cpxlp != NULL);

   SCIPdebugMessage("getting coefficient of row %d col %d\n", row, col);

   CHECK_ZERO( CPXgetcoef(cpxenv, lpi->cpxlp, row, col, val) );

   return SCIP_OKAY;
}

/**@} */




/*
 * Solving Methods
 */

/**@name Solving Methods */
/**@{ */

/** calls primal simplex to solve the LP */
SCIP_RETCODE SCIPlpiSolvePrimal(
   SCIP_LPI*             lpi                 /**< LP interface structure */
   )
{
   int retval;
   int primalfeasible;
   int dualfeasible;

   assert(cpxenv != NULL);
   assert(lpi != NULL);
   assert(lpi->cpxlp != NULL);

   SCIPdebugMessage("calling CPLEX primal simplex: %d cols, %d rows\n",
      CPXgetnumcols(cpxenv, lpi->cpxlp), CPXgetnumrows(cpxenv, lpi->cpxlp));

   invalidateSolution(lpi);

   SCIP_CALL( setParameterValues(&(lpi->cpxparam)) );

   SCIPdebugMessage("calling CPXprimopt()\n");
   retval = CPXprimopt(cpxenv, lpi->cpxlp);
   lpi->iterations = CPXgetphase1cnt(cpxenv, lpi->cpxlp) + CPXgetitcnt(cpxenv, lpi->cpxlp);
   switch( retval  )
   {
   case 0:
      break;
   case CPXERR_NO_MEMORY:
      return SCIP_NOMEMORY;
   default:
      return SCIP_LPERROR;
   }

   lpi->solisbasic = TRUE;
   lpi->solstat = CPXgetstat(cpxenv, lpi->cpxlp);
   lpi->instabilityignored = FALSE;
   CHECK_ZERO( CPXsolninfo(cpxenv, lpi->cpxlp, NULL, NULL, &primalfeasible, &dualfeasible) );
   SCIPdebugMessage(" -> CPLEX returned solstat=%d, pfeas=%d, dfeas=%d (%d iterations)\n",
      lpi->solstat, primalfeasible, dualfeasible, lpi->iterations);

   if( lpi->solstat == CPX_STAT_INForUNBD
      || (lpi->solstat == CPX_STAT_INFEASIBLE && !dualfeasible)
      || (lpi->solstat == CPX_STAT_UNBOUNDED && !primalfeasible) )
   {
      if( getIntParam(lpi, CPX_PARAM_PREIND) == CPX_ON )
      {
         /* maybe the preprocessor solved the problem; but we need a solution, so solve again without preprocessing */
         SCIPdebugMessage("presolver may have solved the problem -> calling CPLEX primal simplex again without presolve\n");
         
         /* switch off preprocessing */
         setIntParam(lpi, CPX_PARAM_PREIND, CPX_OFF);
         SCIP_CALL( setParameterValues(&(lpi->cpxparam)) );
         
         retval = CPXprimopt(cpxenv, lpi->cpxlp);
         switch( retval  )
         {
         case 0:
            break;
         case CPXERR_NO_MEMORY:
            return SCIP_NOMEMORY;
         default:
            return SCIP_LPERROR;
         }

         lpi->iterations += CPXgetphase1cnt(cpxenv, lpi->cpxlp) + CPXgetitcnt(cpxenv, lpi->cpxlp);
         lpi->solstat = CPXgetstat(cpxenv, lpi->cpxlp);
	 lpi->instabilityignored = FALSE;
         SCIPdebugMessage(" -> CPLEX returned solstat=%d (%d iterations)\n", lpi->solstat, lpi->iterations);

         /* switch on preprocessing again */
         setIntParam(lpi, CPX_PARAM_PREIND, CPX_ON);
      }

      if( lpi->solstat == CPX_STAT_INForUNBD )
      {
         /* preprocessing was not the problem; issue a warning message and treat LP as infeasible */
         SCIPerrorMessage("CPLEX primal simplex returned CPX_STAT_INForUNBD after presolving was turned off\n");
      }
   }

   return SCIP_OKAY;
}

/** calls dual simplex to solve the LP */
SCIP_RETCODE SCIPlpiSolveDual(
   SCIP_LPI*             lpi                 /**< LP interface structure */
   )
{
   int retval;
   int primalfeasible;
   int dualfeasible;

   assert(cpxenv != NULL);
   assert(lpi != NULL);
   assert(lpi->cpxlp != NULL);

   SCIPdebugMessage("calling CPLEX dual simplex: %d cols, %d rows\n", 
      CPXgetnumcols(cpxenv, lpi->cpxlp), CPXgetnumrows(cpxenv, lpi->cpxlp));

   invalidateSolution(lpi);

   SCIP_CALL( setParameterValues(&(lpi->cpxparam)) );

   SCIPdebugMessage("calling CPXdualopt()\n");
   retval = CPXdualopt(cpxenv, lpi->cpxlp);
   lpi->iterations = CPXgetphase1cnt(cpxenv, lpi->cpxlp) + CPXgetitcnt(cpxenv, lpi->cpxlp);
   switch( retval  )
   {
   case 0:
      break;
   case CPXERR_NO_MEMORY:
      return SCIP_NOMEMORY;
   default:
      return SCIP_LPERROR;
   }

   lpi->solisbasic = TRUE;
   lpi->solstat = CPXgetstat(cpxenv, lpi->cpxlp);
   lpi->instabilityignored = FALSE;
   CHECK_ZERO( CPXsolninfo(cpxenv, lpi->cpxlp, NULL, NULL, &primalfeasible, &dualfeasible) );
   SCIPdebugMessage(" -> CPLEX returned solstat=%d, pfeas=%d, dfeas=%d (%d iterations)\n",
      lpi->solstat, primalfeasible, dualfeasible, lpi->iterations);

   if( lpi->solstat == CPX_STAT_INForUNBD
      || (lpi->solstat == CPX_STAT_INFEASIBLE && !dualfeasible)
      || (lpi->solstat == CPX_STAT_UNBOUNDED && !primalfeasible) )
   {
      if( getIntParam(lpi, CPX_PARAM_PREIND) == CPX_ON )
      {
         /* maybe the preprocessor solved the problem; but we need a solution, so solve again without preprocessing */
         SCIPdebugMessage("presolver may have solved the problem -> calling CPLEX dual simplex again without presolve\n");
         
         /* switch off preprocessing */
         setIntParam(lpi, CPX_PARAM_PREIND, CPX_OFF);
         SCIP_CALL( setParameterValues(&(lpi->cpxparam)) );
         
         retval = CPXdualopt(cpxenv, lpi->cpxlp);
         switch( retval  )
         {
         case 0:
            break;
         case CPXERR_NO_MEMORY:
            return SCIP_NOMEMORY;
         default:
            return SCIP_LPERROR;
         }

         lpi->iterations += CPXgetphase1cnt(cpxenv, lpi->cpxlp) + CPXgetitcnt(cpxenv, lpi->cpxlp);
         lpi->solstat = CPXgetstat(cpxenv, lpi->cpxlp);
	 lpi->instabilityignored = FALSE;
         CHECK_ZERO( CPXsolninfo(cpxenv, lpi->cpxlp, NULL, NULL, &primalfeasible, &dualfeasible) );
         SCIPdebugMessage(" -> CPLEX returned solstat=%d (%d iterations)\n", lpi->solstat, lpi->iterations);

         /* switch on preprocessing again */
         setIntParam(lpi, CPX_PARAM_PREIND, CPX_ON);
      }

      if( lpi->solstat == CPX_STAT_INForUNBD )
      {
         /* preprocessing was not the problem; issue a warning message and treat LP as infeasible */
         SCIPerrorMessage("CPLEX dual simplex returned CPX_STAT_INForUNBD after presolving was turned off\n");
      }
   }

#if 0
   /* this fixes the strange behaviour of CPLEX, that in case of the objective limit exceedance, it returns the
    * solution for the basis preceeding the one with exceeding objective limit
    * (using this "wrong" dual solution can cause column generation algorithms to fail to find an improving column)
    */
   if( SCIPlpiIsObjlimExc(lpi) )
   {
      SCIP_Real objval;
      SCIP_Real llim;
      SCIP_Real ulim;
      SCIP_Real eps;

      /* check, if the dual solution returned by CPLEX really exceeds the objective limit;
       * CPLEX usually returns the basis one iteration before the one that exceeds the limit
       */
      SCIP_CALL( SCIPlpiGetObjval(lpi, &objval) );
      llim = getDblParam(lpi, CPX_PARAM_OBJLLIM);
      ulim = getDblParam(lpi, CPX_PARAM_OBJULIM);
      eps = getDblParam(lpi, CPX_PARAM_EPOPT);
      if( objval >= llim - eps && objval <= ulim + eps )
      {
         int itlim;
         int advind;

         /* perform one additional simplex iteration without objective limit */
         SCIPdebugMessage("dual solution %g does not exceed objective limit [%g,%g] (%d iterations) -> calling CPLEX dual simplex again for one iteration\n",
            objval, llim, ulim, lpi->iterations);
         itlim = getIntParam(lpi, CPX_PARAM_ITLIM);
         setIntParam(lpi, CPX_PARAM_ITLIM, 1);
         advind = getIntParam(lpi, CPX_PARAM_ADVIND);
         setIntParam(lpi, CPX_PARAM_ADVIND, CPX_ON);
         setDblParam(lpi, CPX_PARAM_OBJLLIM, -CPX_INFBOUND);
         setDblParam(lpi, CPX_PARAM_OBJULIM, CPX_INFBOUND);
         SCIP_CALL( setParameterValues(&(lpi->cpxparam)) );
         CHECK_ZERO( CPXsetintparam(cpxenv, CPX_PARAM_FINALFACTOR, FALSE) );
         
         retval = CPXdualopt(cpxenv, lpi->cpxlp);
         switch( retval  )
         {
         case 0:
            break;
         case CPXERR_NO_MEMORY:
            return SCIP_NOMEMORY;
         default:
            return SCIP_LPERROR;
         }

         lpi->iterations += CPXgetphase1cnt(cpxenv, lpi->cpxlp) + CPXgetitcnt(cpxenv, lpi->cpxlp);

         /* reset the iteration limit and objective bounds */
         setIntParam(lpi, CPX_PARAM_ITLIM, itlim);
         setIntParam(lpi, CPX_PARAM_ADVIND, advind);
         setDblParam(lpi, CPX_PARAM_OBJLLIM, llim);
         setDblParam(lpi, CPX_PARAM_OBJULIM, ulim);
         SCIP_CALL( setParameterValues(&(lpi->cpxparam)) );
         CHECK_ZERO( CPXsetintparam(cpxenv, CPX_PARAM_FINALFACTOR, TRUE) );
         
         /* resolve LP again in order to restore the status of exceeded objective limit */
         retval = CPXdualopt(cpxenv, lpi->cpxlp);
         switch( retval  )
         {
         case 0:
            break;
         case CPXERR_NO_MEMORY:
            return SCIP_NOMEMORY;
         default:
            return SCIP_LPERROR;
         }

         lpi->iterations += CPXgetphase1cnt(cpxenv, lpi->cpxlp) + CPXgetitcnt(cpxenv, lpi->cpxlp);
         lpi->solstat = CPXgetstat(cpxenv, lpi->cpxlp);
	 lpi->instabilityignored = FALSE;
         SCIPdebugMessage(" -> CPLEX returned solstat=%d (%d iterations)\n", lpi->solstat, lpi->iterations);
      }
   }
#endif

   return SCIP_OKAY;
}

/** calls barrier or interior point algorithm to solve the LP with crossover to simplex basis */
SCIP_RETCODE SCIPlpiSolveBarrier(
   SCIP_LPI*             lpi,                /**< LP interface structure */
   SCIP_Bool             crossover            /**< perform crossover */
   )
{
   int retval;

   assert(cpxenv != NULL);
   assert(lpi != NULL);
   assert(lpi->cpxlp != NULL);

   SCIPdebugMessage("calling CPLEX barrier: %d cols, %d rows\n",
      CPXgetnumcols(cpxenv, lpi->cpxlp), CPXgetnumrows(cpxenv, lpi->cpxlp));

   invalidateSolution(lpi);

   SCIP_CALL( setParameterValues(&(lpi->cpxparam)) );

   SCIPdebugMessage("calling CPXhybaropt()\n");
   retval = CPXhybbaropt(cpxenv, lpi->cpxlp, crossover ? 0 : CPX_ALG_NONE);
   lpi->iterations = CPXgetbaritcnt(cpxenv, lpi->cpxlp);
   switch( retval  )
   {
   case 0:
      break;
   case CPXERR_NO_MEMORY:
      return SCIP_NOMEMORY;
   default:
      return SCIP_LPERROR;
   }

   lpi->solisbasic = crossover;
   lpi->solstat = CPXgetstat(cpxenv, lpi->cpxlp);
   lpi->instabilityignored = FALSE;
   SCIPdebugMessage(" -> CPLEX returned solstat=%d (%d iterations)\n", lpi->solstat, lpi->iterations);

   if( lpi->solstat == CPX_STAT_INForUNBD )
   {
      /* maybe the preprocessor solved the problem; but we need a solution, so solve again without preprocessing */
      SCIPdebugMessage("CPLEX returned INForUNBD -> calling CPLEX barrier again without presolve\n");
      
      /* switch off preprocessing */
      setIntParam(lpi, CPX_PARAM_PREIND, CPX_OFF);
      SCIP_CALL( setParameterValues(&(lpi->cpxparam)) );

      retval = CPXhybbaropt(cpxenv, lpi->cpxlp, crossover ? 0 : CPX_ALG_NONE);
      switch( retval  )
      {
      case 0:
         break;
      case CPXERR_NO_MEMORY:
         return SCIP_NOMEMORY;
      default:
         return SCIP_LPERROR;
      }

      lpi->iterations += CPXgetbaritcnt(cpxenv, lpi->cpxlp);
      lpi->solstat = CPXgetstat(cpxenv, lpi->cpxlp);
      lpi->instabilityignored = FALSE;
      SCIPdebugMessage(" -> CPLEX returned solstat=%d\n", lpi->solstat);

      if( lpi->solstat == CPX_STAT_INForUNBD )
      {
         /* preprocessing was not the problem; issue a warning message and treat LP as infeasible */
         SCIPerrorMessage("CPLEX barrier returned CPX_STAT_INForUNBD after presolving was turned off\n");
      }

      setIntParam(lpi, CPX_PARAM_PREIND, CPX_ON);
   }

   return SCIP_OKAY;
}

/** manually performs strong branching on one integral variable */
static
SCIP_RETCODE lpiStrongbranchIntegral(
   SCIP_LPI*             lpi,                /**< LP interface structure */
   int                   col,                /**< column to apply strong branching on */
   SCIP_Real             psol,               /**< current integral primal solution value of column */
   int                   itlim,              /**< iteration limit for strong branchings */
   SCIP_Real*            down,               /**< stores dual bound after branching column down */
   SCIP_Real*            up,                 /**< stores dual bound after branching column up */
   SCIP_Bool*            downvalid,          /**< stores whether the returned down value is a valid dual bound;
                                              *   otherwise, it can only be used as an estimate value */
   SCIP_Bool*            upvalid,            /**< stores whether the returned up value is a valid dual bound;
                                              *   otherwise, it can only be used as an estimate value */
   int*                  iter                /**< stores total number of strong branching iterations, or -1; may be NULL */
)
{
   const char lbound = 'L';
   const char ubound = 'U';
   SCIP_Real oldlb;
   SCIP_Real oldub;
   SCIP_Real newlb;
   SCIP_Real newub;
   int objsen;
   int olditlim;
   int it;
   
   SCIPdebugMessage(" -> strong branching on integral variable %d\n", col);

   assert( EPSISINT(psol, 1e-06) );

   objsen = CPXgetobjsen(cpxenv, lpi->cpxlp);

   /* results of CPLEX are valid in any case */
   *downvalid = TRUE;
   *upvalid = TRUE;

   /* save current LP basis and bounds*/
   SCIP_CALL( getBase(lpi) );
   CHECK_ZERO( CPXgetlb(cpxenv, lpi->cpxlp, &oldlb, col, col) );
   CHECK_ZERO( CPXgetub(cpxenv, lpi->cpxlp, &oldub, col, col) );

   /* save old iteration limit and set iteration limit to strong branching limit */
   if( itlim > CPX_INT_MAX )
      itlim = CPX_INT_MAX;
   olditlim = getIntParam(lpi, CPX_PARAM_ITLIM);
   setIntParam(lpi, CPX_PARAM_ITLIM, itlim);
      
   /* down branch */
   newub = EPSCEIL(psol-1.0, 1e-06);
   if( newub >= oldlb - 0.5 )
   {
      CHECK_ZERO( CPXchgbds(cpxenv, lpi->cpxlp, 1, &col, &ubound, &newub) );
      SCIP_CALL( SCIPlpiSolveDual(lpi) );
      if( SCIPlpiIsPrimalInfeasible(lpi) || SCIPlpiIsObjlimExc(lpi) )
         *down = objsen == CPX_MIN ? getDblParam(lpi, CPX_PARAM_OBJULIM) : getDblParam(lpi, CPX_PARAM_OBJLLIM);
      else if( SCIPlpiIsOptimal(lpi) || SCIPlpiIsIterlimExc(lpi) )
      {
         SCIP_CALL( SCIPlpiGetObjval(lpi, down) );
      }
      else
         *down = objsen == CPX_MIN ? getDblParam(lpi, CPX_PARAM_OBJLLIM) : getDblParam(lpi, CPX_PARAM_OBJULIM);
      if( iter != NULL )
      {
         SCIP_CALL( SCIPlpiGetIterations(lpi, &it) );
         *iter += it;
      }
      SCIPdebugMessage(" -> down (x%d <= %g): %g\n", col, newub, *down);
      
      CHECK_ZERO( CPXchgbds(cpxenv, lpi->cpxlp, 1, &col, &ubound, &oldub) );
      SCIP_CALL( setBase(lpi) );
   }
   else
      *down = objsen == CPX_MIN ? getDblParam(lpi, CPX_PARAM_OBJULIM) : getDblParam(lpi, CPX_PARAM_OBJLLIM);

   /* up branch */
   newlb = EPSFLOOR(psol+1.0, 1e-06);
   if( newlb <= oldub + 0.5 )
   {
      CHECK_ZERO( CPXchgbds(cpxenv, lpi->cpxlp, 1, &col, &lbound, &newlb) );
      SCIP_CALL( SCIPlpiSolveDual(lpi) );
      if( SCIPlpiIsPrimalInfeasible(lpi) || SCIPlpiIsObjlimExc(lpi) )
         *up = objsen == CPX_MIN ? getDblParam(lpi, CPX_PARAM_OBJULIM) : getDblParam(lpi, CPX_PARAM_OBJLLIM);
      else if( SCIPlpiIsOptimal(lpi) || SCIPlpiIsIterlimExc(lpi) )
      {
         SCIP_CALL( SCIPlpiGetObjval(lpi, up) );
      }
      else
         *up = objsen == CPX_MIN ? getDblParam(lpi, CPX_PARAM_OBJLLIM) : getDblParam(lpi, CPX_PARAM_OBJULIM);
      if( iter != NULL )
      {
         SCIP_CALL( SCIPlpiGetIterations(lpi, &it) );
         *iter += it;
      }
      SCIPdebugMessage(" -> up  (x%d >= %g): %g\n", col, newlb, *up);
      
      CHECK_ZERO( CPXchgbds(cpxenv, lpi->cpxlp, 1, &col, &lbound, &oldlb) );
      SCIP_CALL( setBase(lpi) );
   }
   else
      *up = objsen == CPX_MIN ? getDblParam(lpi, CPX_PARAM_OBJLLIM) : getDblParam(lpi, CPX_PARAM_OBJULIM);
   
   /* reset iteration limit */
   setIntParam(lpi, CPX_PARAM_ITLIM, olditlim);

   return SCIP_OKAY;
}

/** start strong branching */
SCIP_RETCODE SCIPlpiStartStrongbranch(
   SCIP_LPI*             lpi                 /**< LP interface structure */
   )
{  /* no work necessary */
   return SCIP_OKAY;
}

/** end strong branching */
SCIP_RETCODE SCIPlpiEndStrongbranch(
   SCIP_LPI*             lpi                 /**< LP interface structure */
   )
{  /* no work necessary */
   return SCIP_OKAY;
}

/** performs strong branching iterations on one @b fractional candidate */
SCIP_RETCODE SCIPlpiStrongbranchFrac(
   SCIP_LPI*             lpi,                /**< LP interface structure */
   int                   col,                /**< column to apply strong branching on */
   SCIP_Real             psol,               /**< fractional current primal solution value of column */
   int                   itlim,              /**< iteration limit for strong branchings */
   SCIP_Real*            down,               /**< stores dual bound after branching column down */
   SCIP_Real*            up,                 /**< stores dual bound after branching column up */
   SCIP_Bool*            downvalid,          /**< stores whether the returned down value is a valid dual bound;
                                              *   otherwise, it can only be used as an estimate value */
   SCIP_Bool*            upvalid,            /**< stores whether the returned up value is a valid dual bound;
                                              *   otherwise, it can only be used as an estimate value */
   int*                  iter                /**< stores total number of strong branching iterations, or -1; may be NULL */
   )
{
   int retval;

   assert(cpxenv != NULL);
   assert(lpi != NULL);
   assert(lpi->cpxlp != NULL);
   assert(down != NULL);
   assert(up != NULL);
   assert(downvalid != NULL);
   assert(upvalid != NULL);

   SCIPdebugMessage("calling CPLEX strongbranching on fractional variable %d (%d iterations)\n", col, itlim);

   assert( ! EPSISINT(psol, 1e-06) );

   /* results of CPLEX are valid in any case */
   *downvalid = TRUE;
   *upvalid = TRUE;

   SCIP_CALL( setParameterValues(&(lpi->cpxparam)) );

   retval = CPXstrongbranch(cpxenv, lpi->cpxlp, &col, 1, down, up, itlim);
   if( retval == CPXERR_NEED_OPT_SOLN )
   {
      SCIPdebugMessage(" -> no optimal solution available\n");
      return SCIP_LPERROR;
   }
   else if( retval == CPXERR_TILIM_STRONGBRANCH )
   {
      SCIPdebugMessage(" -> time limit exceeded during strong branching\n");
      return SCIP_LPERROR;
   }
   CHECK_ZERO( retval );
   SCIPdebugMessage(" -> down: %g, up:%g\n", *down, *up);

   /* CPLEX is not able to return the iteration counts in strong branching */
   if( iter != NULL )
      *iter = -1;

   return SCIP_OKAY;
}

/** performs strong branching iterations on given @b fractional candidates */
SCIP_RETCODE SCIPlpiStrongbranchesFrac(
   SCIP_LPI*             lpi,                /**< LP interface structure */
   int*                  cols,               /**< columns to apply strong branching on */
   int                   ncols,              /**< number of columns */
   SCIP_Real*            psols,              /**< fractional current primal solution values of columns */
   int                   itlim,              /**< iteration limit for strong branchings */
   SCIP_Real*            down,               /**< stores dual bounds after branching columns down */
   SCIP_Real*            up,                 /**< stores dual bounds after branching columns up */
   SCIP_Bool*            downvalid,          /**< stores whether the returned down values are valid dual bounds;
                                              *   otherwise, they can only be used as an estimate values */
   SCIP_Bool*            upvalid,            /**< stores whether the returned up values are a valid dual bounds;
                                              *   otherwise, they can only be used as an estimate values */
   int*                  iter                /**< stores total number of strong branching iterations, or -1; may be NULL */
   )
{
   int retval;
   int j;

   assert(cpxenv != NULL);
   assert(lpi != NULL);
   assert(lpi->cpxlp != NULL);
   assert(cols != NULL);
   assert(psols != NULL);
   assert(down != NULL);
   assert(up != NULL);
   assert(downvalid != NULL);
   assert(upvalid != NULL);

   SCIPdebugMessage("calling CPLEX strongbranching on %d fractional variables (%d iterations)\n", ncols, itlim);

   SCIP_CALL( setParameterValues(&(lpi->cpxparam)) );

   /* init */
   for (j = 0; j < ncols; ++j)
   {
      /* results of CPLEX are valid in any case */
      *downvalid = TRUE;
      *upvalid = TRUE;
      
      assert( ! EPSISINT(psols[j], 1e-06) );
   }

   retval = CPXstrongbranch(cpxenv, lpi->cpxlp, cols, ncols, down, up, itlim);
   if( retval == CPXERR_NEED_OPT_SOLN )
   {
      SCIPdebugMessage(" -> no optimal solution available\n");
      return SCIP_LPERROR;
   }
   else if( retval == CPXERR_TILIM_STRONGBRANCH )
   {
      SCIPdebugMessage(" -> time limit exceeded during strong branching\n");
      return SCIP_LPERROR;
   }
   CHECK_ZERO( retval );

   /* CPLEX is not able to return the iteration counts in strong branching */
   if( iter != NULL )
      *iter = -1;

   return SCIP_OKAY;
}

/** performs strong branching iterations on one candidate with @b integral value */
SCIP_RETCODE SCIPlpiStrongbranchInt(
   SCIP_LPI*             lpi,                /**< LP interface structure */
   int                   col,                /**< column to apply strong branching on */
   SCIP_Real             psol,               /**< current integral primal solution value of column */
   int                   itlim,              /**< iteration limit for strong branchings */
   SCIP_Real*            down,               /**< stores dual bound after branching column down */
   SCIP_Real*            up,                 /**< stores dual bound after branching column up */
   SCIP_Bool*            downvalid,          /**< stores whether the returned down value is a valid dual bound;
                                              *   otherwise, it can only be used as an estimate value */
   SCIP_Bool*            upvalid,            /**< stores whether the returned up value is a valid dual bound;
                                              *   otherwise, it can only be used as an estimate value */
   int*                  iter                /**< stores total number of strong branching iterations, or -1; may be NULL */
   )
{
   assert(cpxenv != NULL);
   assert(lpi != NULL);
   assert(lpi->cpxlp != NULL);
   assert(down != NULL);
   assert(up != NULL);
   assert(downvalid != NULL);
   assert(upvalid != NULL);

   SCIPdebugMessage("calling CPLEX strongbranching on variable %d with integral value (%d iterations)\n", col, itlim);

   assert( EPSISINT(psol, 1e-06) );

   SCIP_CALL( setParameterValues(&(lpi->cpxparam)) );

   if ( iter != NULL )
      *iter = 0;

   SCIP_CALL( lpiStrongbranchIntegral(lpi, col, psol, itlim, down, up, downvalid, upvalid, iter) );

   return SCIP_OKAY;
}

/** performs strong branching iterations on given candidates with @b integral values */
SCIP_RETCODE SCIPlpiStrongbranchesInt(
   SCIP_LPI*             lpi,                /**< LP interface structure */
   int*                  cols,               /**< columns to apply strong branching on */
   int                   ncols,              /**< number of columns */
   SCIP_Real*            psols,              /**< current integral primal solution values of columns */
   int                   itlim,              /**< iteration limit for strong branchings */
   SCIP_Real*            down,               /**< stores dual bounds after branching columns down */
   SCIP_Real*            up,                 /**< stores dual bounds after branching columns up */
   SCIP_Bool*            downvalid,          /**< stores whether the returned down values are valid dual bounds;
                                              *   otherwise, they can only be used as an estimate values */
   SCIP_Bool*            upvalid,            /**< stores whether the returned up values are a valid dual bounds;
                                              *   otherwise, they can only be used as an estimate values */
   int*                  iter                /**< stores total number of strong branching iterations, or -1; may be NULL */
   )
{
   int j;

   assert(cpxenv != NULL);
   assert(lpi != NULL);
   assert(lpi->cpxlp != NULL);
   assert(cols != NULL);
   assert(psols != NULL);
   assert(down != NULL);
   assert(up != NULL);
   assert(downvalid != NULL);
   assert(upvalid != NULL);

   SCIPdebugMessage("calling CPLEX strongbranching on %d variables with integer values (%d iterations)\n", ncols, itlim);

   SCIP_CALL( setParameterValues(&(lpi->cpxparam)) );

   if ( iter != NULL )
      *iter = 0;

   /* init */
   for (j = 0; j < ncols; ++j)
   {
      assert( EPSISINT(psols[j], 1e-06) );
      SCIP_CALL( lpiStrongbranchIntegral(lpi, cols[j], psols[j], itlim, &(down[j]), &(up[j]), &(downvalid[j]), &(upvalid[j]), iter) );
   }

   return SCIP_OKAY;
}
/**@} */




/*
 * Solution Information Methods
 */

/**@name Solution Information Methods */
/**@{ */

/** returns whether a solve method was called after the last modification of the LP */
SCIP_Bool SCIPlpiWasSolved(
   SCIP_LPI*             lpi                 /**< LP interface structure */
   )
{
   assert(lpi != NULL);

   return (lpi->solstat != -1);
}

/** gets information about primal and dual feasibility of the current LP solution */
SCIP_RETCODE SCIPlpiGetSolFeasibility(
   SCIP_LPI*             lpi,                /**< LP interface structure */
   SCIP_Bool*            primalfeasible,     /**< stores primal feasibility status */
   SCIP_Bool*            dualfeasible        /**< stores dual feasibility status */
   )
{
   int pfeas;
   int dfeas;

   assert(lpi != NULL);
   assert(lpi->cpxlp != NULL);
   assert(primalfeasible != NULL);
   assert(dualfeasible != NULL);

   SCIPdebugMessage("getting solution feasibility\n");

   CHECK_ZERO( CPXsolninfo(cpxenv, lpi->cpxlp, NULL, NULL, &pfeas, &dfeas) );
   *primalfeasible = (SCIP_Bool)pfeas;
   *dualfeasible = (SCIP_Bool)dfeas;

   return SCIP_OKAY;
}

/** returns TRUE iff LP is proven to have a primal unbounded ray (but not necessary a primal feasible point);
 *  this does not necessarily mean, that the solver knows and can return the primal ray
 */
SCIP_Bool SCIPlpiExistsPrimalRay(
   SCIP_LPI*             lpi                 /**< LP interface structure */
   )
{
   assert(cpxenv != NULL);
   assert(lpi != NULL);
   assert(lpi->cpxlp != NULL);
   assert(lpi->solstat >= 0);

   return (lpi->solstat == CPX_STAT_UNBOUNDED || lpi->solstat == CPX_STAT_OPTIMAL_FACE_UNBOUNDED);
}

/** returns TRUE iff LP is proven to have a primal unbounded ray (but not necessary a primal feasible point),
 *  and the solver knows and can return the primal ray
 */
SCIP_Bool SCIPlpiHasPrimalRay(
   SCIP_LPI*             lpi                 /**< LP interface structure */
   )
{
   assert(cpxenv != NULL);
   assert(lpi != NULL);
   assert(lpi->cpxlp != NULL);
   assert(lpi->solstat >= 0);

   return (lpi->solstat == CPX_STAT_UNBOUNDED && CPXgetmethod(cpxenv, lpi->cpxlp) == CPX_ALG_PRIMAL);
}

/** returns TRUE iff LP is proven to be primal unbounded */
SCIP_Bool SCIPlpiIsPrimalUnbounded(
   SCIP_LPI*             lpi                 /**< LP interface structure */
   )
{
   int primalfeasible;

   assert(cpxenv != NULL);
   assert(lpi != NULL);
   assert(lpi->cpxlp != NULL);
   assert(lpi->solstat >= 0);

   SCIPdebugMessage("checking for primal unboundness\n");

   ABORT_ZERO( CPXsolninfo(cpxenv, lpi->cpxlp, NULL, NULL, &primalfeasible, NULL) );
   
   /* If the solution status of CPLEX is CPX_STAT_UNBOUNDED, it only means, there is an unbounded ray,
    * but not necessarily a feasible primal solution. If primalfeasible == FALSE, we cannot conclude,
    * that the problem is unbounded
    */
   return ((primalfeasible && (lpi->solstat == CPX_STAT_UNBOUNDED || lpi->solstat == CPX_STAT_INForUNBD))
      || lpi->solstat == CPX_STAT_OPTIMAL_FACE_UNBOUNDED);
}

/** returns TRUE iff LP is proven to be primal infeasible */
SCIP_Bool SCIPlpiIsPrimalInfeasible(
   SCIP_LPI*             lpi                 /**< LP interface structure */
   )
{
   int dualfeasible;

   assert(cpxenv != NULL);
   assert(lpi != NULL);
   assert(lpi->cpxlp != NULL);
   assert(lpi->solstat >= 0);

   SCIPdebugMessage("checking for primal infeasibility\n");

   ABORT_ZERO( CPXsolninfo(cpxenv, lpi->cpxlp, NULL, NULL, NULL, &dualfeasible) );

   return (lpi->solstat == CPX_STAT_INFEASIBLE || (lpi->solstat == CPX_STAT_INForUNBD && dualfeasible));
}

/** returns TRUE iff LP is proven to be primal feasible */
SCIP_Bool SCIPlpiIsPrimalFeasible(
   SCIP_LPI*             lpi                 /**< LP interface structure */
   )
{
   int primalfeasible;

   assert(cpxenv != NULL);
   assert(lpi != NULL);
   assert(lpi->cpxlp != NULL);
   assert(lpi->solstat >= 0);

   SCIPdebugMessage("checking for primal feasibility\n");

   ABORT_ZERO( CPXsolninfo(cpxenv, lpi->cpxlp, NULL, NULL, &primalfeasible, NULL) );
   
   return (SCIP_Bool)primalfeasible;
}

/** returns TRUE iff LP is proven to have a dual unbounded ray (but not necessary a dual feasible point);
 *  this does not necessarily mean, that the solver knows and can return the dual ray
 */
SCIP_Bool SCIPlpiExistsDualRay(
   SCIP_LPI*             lpi                 /**< LP interface structure */
   )
{
   assert(cpxenv != NULL);
   assert(lpi != NULL);
   assert(lpi->cpxlp != NULL);
   assert(lpi->solstat >= 0);

   return (lpi->solstat == CPX_STAT_INFEASIBLE);
}

/** returns TRUE iff LP is proven to have a dual unbounded ray (but not necessary a dual feasible point),
 *  and the solver knows and can return the dual ray
 */
SCIP_Bool SCIPlpiHasDualRay(
   SCIP_LPI*             lpi                 /**< LP interface structure */
   )
{
   assert(cpxenv != NULL);
   assert(lpi != NULL);
   assert(lpi->cpxlp != NULL);
   assert(lpi->solstat >= 0);

   return (lpi->solstat == CPX_STAT_INFEASIBLE && CPXgetmethod(cpxenv, lpi->cpxlp) == CPX_ALG_DUAL);
}

/** returns TRUE iff LP is proven to be dual unbounded */
SCIP_Bool SCIPlpiIsDualUnbounded(
   SCIP_LPI*             lpi                 /**< LP interface structure */
   )
{
   int dualfeasible;

   assert(cpxenv != NULL);
   assert(lpi != NULL);
   assert(lpi->cpxlp != NULL);
   assert(lpi->solstat >= 0);

   SCIPdebugMessage("checking for dual unboundness\n");

   ABORT_ZERO( CPXsolninfo(cpxenv, lpi->cpxlp, NULL, NULL, NULL, &dualfeasible) );

   return (dualfeasible && (lpi->solstat == CPX_STAT_INFEASIBLE || lpi->solstat == CPX_STAT_INForUNBD));
}

/** returns TRUE iff LP is proven to be dual infeasible */
SCIP_Bool SCIPlpiIsDualInfeasible(
   SCIP_LPI*             lpi                 /**< LP interface structure */
   )
{
   int primalfeasible;

   assert(cpxenv != NULL);
   assert(lpi != NULL);
   assert(lpi->cpxlp != NULL);
   assert(lpi->solstat >= 0);

   SCIPdebugMessage("checking for dual infeasibility\n");

   ABORT_ZERO( CPXsolninfo(cpxenv, lpi->cpxlp, NULL, NULL, &primalfeasible, NULL) );

   return (lpi->solstat == CPX_STAT_UNBOUNDED
      || lpi->solstat == CPX_STAT_OPTIMAL_FACE_UNBOUNDED
      || (lpi->solstat == CPX_STAT_INForUNBD && primalfeasible));
}

/** returns TRUE iff LP is proven to be dual feasible */
SCIP_Bool SCIPlpiIsDualFeasible(
   SCIP_LPI*             lpi                 /**< LP interface structure */
   )
{
   int dualfeasible;

   assert(cpxenv != NULL);
   assert(lpi != NULL);
   assert(lpi->cpxlp != NULL);
   assert(lpi->solstat >= 0);

   SCIPdebugMessage("checking for dual feasibility\n");

   ABORT_ZERO( CPXsolninfo(cpxenv, lpi->cpxlp, NULL, NULL, NULL, &dualfeasible) );
   
   return (SCIP_Bool)dualfeasible;
}

/** returns TRUE iff LP was solved to optimality */
SCIP_Bool SCIPlpiIsOptimal(
   SCIP_LPI*             lpi                 /**< LP interface structure */
   )
{
   assert(cpxenv != NULL);
   assert(lpi != NULL);
   assert(lpi->cpxlp != NULL);
   assert(lpi->solstat >= 0);

   return (lpi->solstat == CPX_STAT_OPTIMAL);
}

/** returns TRUE iff current LP basis is stable */
SCIP_Bool SCIPlpiIsStable(
   SCIP_LPI*             lpi                 /**< LP interface structure */
   )
{
   assert(cpxenv != NULL);
   assert(lpi != NULL);
   assert(lpi->cpxlp != NULL);
   assert(lpi->solstat >= 0);

   SCIPdebugMessage("checking for stability: CPLEX solstat = %d\n", lpi->solstat);

   /* If the solution status of CPLEX is CPX_STAT_UNBOUNDED, it only means, there is an unbounded ray,
    * but not necessarily a feasible primal solution. If primalfeasible == FALSE, we interpret this
    * result as instability, s.t. the problem is resolved from scratch
    */
   if( lpi->solstat == CPX_STAT_UNBOUNDED )
   {
      int primalfeasible;
      
      ABORT_ZERO( CPXsolninfo(cpxenv, lpi->cpxlp, NULL, NULL, &primalfeasible, NULL) );

      if( !primalfeasible )
         return FALSE;
   }

   return (lpi->solstat != CPX_STAT_NUM_BEST && lpi->solstat != CPX_STAT_OPTIMAL_INFEAS);
}

/** returns TRUE iff the objective limit was reached */
SCIP_Bool SCIPlpiIsObjlimExc(
   SCIP_LPI*             lpi                 /**< LP interface structure */
   )
{
   assert(cpxenv != NULL);
   assert(lpi != NULL);
   assert(lpi->cpxlp != NULL);
   assert(lpi->solstat >= 0);

   return (lpi->solstat == CPX_STAT_ABORT_OBJ_LIM
      || lpi->solstat == CPX_STAT_ABORT_DUAL_OBJ_LIM
      || lpi->solstat == CPX_STAT_ABORT_PRIM_OBJ_LIM);
}

/** returns TRUE iff the iteration limit was reached */
SCIP_Bool SCIPlpiIsIterlimExc(
   SCIP_LPI*             lpi                 /**< LP interface structure */
   )
{
   assert(cpxenv != NULL);
   assert(lpi != NULL);
   assert(lpi->cpxlp != NULL);
   assert(lpi->solstat >= 0);

   return (lpi->solstat == CPX_STAT_ABORT_IT_LIM);
}

/** returns TRUE iff the time limit was reached */
SCIP_Bool SCIPlpiIsTimelimExc(
   SCIP_LPI*             lpi                 /**< LP interface structure */
   )
{
   assert(cpxenv != NULL);
   assert(lpi != NULL);
   assert(lpi->cpxlp != NULL);
   assert(lpi->solstat >= 0);

   return (lpi->solstat == CPX_STAT_ABORT_TIME_LIM);
}

/** returns the internal solution status of the solver */
int SCIPlpiGetInternalStatus(
   SCIP_LPI*             lpi                 /**< LP interface structure */
   )
{
   assert(cpxenv != NULL);
   assert(lpi != NULL);
   assert(lpi->cpxlp != NULL);

   return lpi->solstat;
}

/** tries to reset the internal status of the LP solver in order to ignore an instability of the last solving call */
SCIP_RETCODE SCIPlpiIgnoreInstability(
   SCIP_LPI*             lpi,                /**< LP interface structure */
   SCIP_Bool*            success             /**< pointer to store, whether the instability could be ignored */
   )
{
   assert(cpxenv != NULL);
   assert(lpi != NULL);
   assert(lpi->cpxlp != NULL);
   assert(success != NULL);
   assert(lpi->solstat == CPX_STAT_UNBOUNDED
      || lpi->solstat == CPX_STAT_NUM_BEST
      || lpi->solstat == CPX_STAT_OPTIMAL_INFEAS);

   /* replace instable status with optimal status */
   if( lpi->solstat == CPX_STAT_NUM_BEST || lpi->solstat == CPX_STAT_OPTIMAL_INFEAS )
      lpi->solstat = CPX_STAT_OPTIMAL;

   *success = TRUE;
   lpi->instabilityignored = TRUE;

   return SCIP_OKAY;
}

/** gets objective value of solution */
SCIP_RETCODE SCIPlpiGetObjval(
   SCIP_LPI*             lpi,                /**< LP interface structure */
   SCIP_Real*            objval              /**< stores the objective value */
   )
{
   assert(cpxenv != NULL);
   assert(lpi != NULL);
   assert(lpi->cpxlp != NULL);

   SCIPdebugMessage("getting solution's objective value\n");

   CHECK_ZERO( CPXgetobjval(cpxenv, lpi->cpxlp, objval) );

   return SCIP_OKAY;
}

/** gets primal and dual solution vectors */
SCIP_RETCODE SCIPlpiGetSol(
   SCIP_LPI*             lpi,                /**< LP interface structure */
   SCIP_Real*            objval,             /**< stores the objective value, may be NULL if not needed */
   SCIP_Real*            primsol,            /**< primal solution vector, may be NULL if not needed */
   SCIP_Real*            dualsol,            /**< dual solution vector, may be NULL if not needed */
   SCIP_Real*            activity,           /**< row activity vector, may be NULL if not needed */
   SCIP_Real*            redcost             /**< reduced cost vector, may be NULL if not needed */
   )
{
   int dummy;

   assert(cpxenv != NULL);
   assert(lpi != NULL);
   assert(lpi->cpxlp != NULL);
   assert(lpi->solstat >= 0);

   SCIPdebugMessage("getting solution\n");

   CHECK_ZERO( CPXsolution(cpxenv, lpi->cpxlp, &dummy, objval, primsol, dualsol, NULL, redcost) );
   assert(dummy == lpi->solstat || lpi->instabilityignored);

   if( activity != NULL )
   {
      CHECK_ZERO( CPXgetax(cpxenv, lpi->cpxlp, activity, 0, CPXgetnumrows(cpxenv, lpi->cpxlp)-1) );
   }

   return SCIP_OKAY;
}

/** gets primal ray for unbounded LPs */
SCIP_RETCODE SCIPlpiGetPrimalRay(
   SCIP_LPI*             lpi,                /**< LP interface structure */
   SCIP_Real*            ray                 /**< primal ray */
   )
{
   assert(cpxenv != NULL);
   assert(lpi != NULL);
   assert(lpi->cpxlp != NULL);
   assert(lpi->solstat >= 0);

   SCIPdebugMessage("calling CPLEX get primal ray: %d cols, %d rows\n",
      CPXgetnumcols(cpxenv, lpi->cpxlp), CPXgetnumrows(cpxenv, lpi->cpxlp));

   CHECK_ZERO( CPXgetray(cpxenv, lpi->cpxlp, ray) );

   return SCIP_OKAY;
}

/** gets dual farkas proof for infeasibility */
SCIP_RETCODE SCIPlpiGetDualfarkas(
   SCIP_LPI*             lpi,                /**< LP interface structure */
   SCIP_Real*            dualfarkas          /**< dual farkas row multipliers */
   )
{
   assert(cpxenv != NULL);
   assert(lpi != NULL);
   assert(lpi->cpxlp != NULL);
   assert(lpi->solstat >= 0);
   assert(dualfarkas != NULL);

   SCIPdebugMessage("calling CPLEX dual farkas: %d cols, %d rows\n",
      CPXgetnumcols(cpxenv, lpi->cpxlp), CPXgetnumrows(cpxenv, lpi->cpxlp));

   CHECK_ZERO( CPXdualfarkas(cpxenv, lpi->cpxlp, dualfarkas, NULL) );

   return SCIP_OKAY;
}

/** gets the number of LP iterations of the last solve call */
SCIP_RETCODE SCIPlpiGetIterations(
   SCIP_LPI*             lpi,                /**< LP interface structure */
   int*                  iterations          /**< pointer to store the number of iterations of the last solve call */
   )
{
   assert(cpxenv != NULL);
   assert(lpi != NULL);
   assert(lpi->cpxlp != NULL);
   assert(iterations != NULL);

   *iterations = lpi->iterations;

   return SCIP_OKAY;
}

/**@} */




/*
 * LP Basis Methods
 */

/**@name LP Basis Methods */
/**@{ */

/** gets current basis status for columns and rows; arrays must be large enough to store the basis status */
SCIP_RETCODE SCIPlpiGetBase(
   SCIP_LPI*             lpi,                /**< LP interface structure */
   int*                  cstat,              /**< array to store column basis status, or NULL */
   int*                  rstat               /**< array to store row basis status, or NULL */
   )
{
   assert(cpxenv != NULL);
   assert(lpi != NULL);
   assert(lpi->cpxlp != NULL);

   SCIPdebugMessage("saving CPLEX basis into %p/%p\n", cstat, rstat);

   CHECK_ZERO( CPXgetbase(cpxenv, lpi->cpxlp, cstat, rstat) );

   /* because the basis status values are equally defined in SCIP and CPLEX, they don't need to be transformed */
   assert((int)SCIP_BASESTAT_LOWER == CPX_AT_LOWER);
   assert((int)SCIP_BASESTAT_BASIC == CPX_BASIC);
   assert((int)SCIP_BASESTAT_UPPER == CPX_AT_UPPER);
   assert((int)SCIP_BASESTAT_ZERO == CPX_FREE_SUPER);

   return SCIP_OKAY;
}

/** sets current basis status for columns and rows */
SCIP_RETCODE SCIPlpiSetBase(
   SCIP_LPI*             lpi,                /**< LP interface structure */
   int*                  cstat,              /**< array with column basis status */
   int*                  rstat               /**< array with row basis status */
   )
{
   assert(cpxenv != NULL);
   assert(lpi != NULL);
   assert(lpi->cpxlp != NULL);
   assert(cstat != NULL);
   assert(rstat != NULL);

   SCIPdebugMessage("loading basis %p/%p into CPLEX\n", cstat, rstat);

   invalidateSolution(lpi);

   /* because the basis status values are equally defined in SCIP and CPLEX, they don't need to be transformed */
   assert((int)SCIP_BASESTAT_LOWER == CPX_AT_LOWER);
   assert((int)SCIP_BASESTAT_BASIC == CPX_BASIC);
   assert((int)SCIP_BASESTAT_UPPER == CPX_AT_UPPER);
   assert((int)SCIP_BASESTAT_ZERO == CPX_FREE_SUPER);

   CHECK_ZERO( CPXcopybase(cpxenv, lpi->cpxlp, cstat, rstat) );

   return SCIP_OKAY;
}

/** returns the indices of the basic columns and rows */
SCIP_RETCODE SCIPlpiGetBasisInd(
   SCIP_LPI*             lpi,                /**< LP interface structure */
   int*                  bind                /**< basic column n gives value n, basic row m gives value -1-m */
   )
{
   int retval;

   assert(cpxenv != NULL);
   assert(lpi != NULL);
   assert(lpi->cpxlp != NULL);

   SCIPdebugMessage("getting basis information\n");

   retval = CPXgetbhead(cpxenv, lpi->cpxlp, bind, NULL);
   if( retval == CPXERR_NO_SOLN || retval == CPXERR_NO_LU_FACTOR || retval == CPXERR_NO_BASIC_SOLN
      || retval == CPXERR_NO_BASIS )
   {
      /* modifying the LP, restoring the old LP, and loading the old basis is not enough for CPLEX to be able to
       * return the basis -> we have to resolve the LP (should be done in 0 iterations);
       * this may happen after manual strong branching on an integral variable, or after conflict analysis on
       * a strong branching conflict created a constraint that is not able to modify the LP but trigger the additional
       * call of the separators, in particular, the Gomory separator
       */
      CHECK_ZERO( CPXdualopt(cpxenv, lpi->cpxlp) );
      assert(CPXgetphase1cnt(cpxenv, lpi->cpxlp) == 0);
      assert(CPXgetitcnt(cpxenv, lpi->cpxlp) == 0);
      retval = CPXgetbhead(cpxenv, lpi->cpxlp, bind, NULL);
   }
   CHECK_ZERO( retval );

   return SCIP_OKAY;
}

/** get dense row of inverse basis matrix B^-1 */
SCIP_RETCODE SCIPlpiGetBInvRow(
   SCIP_LPI*             lpi,                /**< LP interface structure */
   int                   r,                  /**< row number */
   SCIP_Real*            coef                /**< pointer to store the coefficients of the row */
   )
{
   int retval;

   assert(cpxenv != NULL);
   assert(lpi != NULL);
   assert(lpi->cpxlp != NULL);

   SCIPdebugMessage("getting binv-row %d\n", r);

   retval = CPXbinvrow(cpxenv, lpi->cpxlp, r, coef);
   if( retval == CPXERR_NO_SOLN || retval == CPXERR_NO_LU_FACTOR || retval == CPXERR_NO_BASIC_SOLN
      || retval == CPXERR_NO_BASIS )
   {
      /* modifying the LP, restoring the old LP, and loading the old basis is not enough for CPLEX to be able to
       * return the basis -> we have to resolve the LP (should be done in 0 iterations);
       * this may happen after manual strong branching on an integral variable, or after conflict analysis on
       * a strong branching conflict created a constraint that is not able to modify the LP but trigger the additional
       * call of the separators, in particular, the Gomory separator
       */
      CHECK_ZERO( CPXdualopt(cpxenv, lpi->cpxlp) );
      assert(CPXgetphase1cnt(cpxenv, lpi->cpxlp) == 0);
      assert(CPXgetitcnt(cpxenv, lpi->cpxlp) == 0);
      retval = CPXbinvrow(cpxenv, lpi->cpxlp, r, coef);
   }
   CHECK_ZERO( retval );

   return SCIP_OKAY;
}

/** get dense column of inverse basis matrix B^-1 */
SCIP_RETCODE SCIPlpiGetBInvCol(
   SCIP_LPI*             lpi,                /**< LP interface structure */
   int                   c,                  /**< column number of B^-1; this is NOT the number of the column in the LP;
                                              *   you have to call SCIPlpiGetBasisInd() to get the array which links the
                                              *   B^-1 column numbers to the row and column numbers of the LP!
                                              *   c must be between 0 and nrows-1, since the basis has the size
                                              *   nrows * nrows */
   SCIP_Real*            coef                /**< pointer to store the coefficients of the column */
   )
{
   int retval;

   assert(cpxenv != NULL);
   assert(lpi != NULL);
   assert(lpi->cpxlp != NULL);

   SCIPdebugMessage("getting binv-col %d\n", c);

   retval = CPXbinvcol(cpxenv, lpi->cpxlp, c, coef);
   if( retval == CPXERR_NO_SOLN || retval == CPXERR_NO_LU_FACTOR || retval == CPXERR_NO_BASIC_SOLN
      || retval == CPXERR_NO_BASIS )
   {
      /* modifying the LP, restoring the old LP, and loading the old basis is not enough for CPLEX to be able to
       * return the basis -> we have to resolve the LP (should be done in 0 iterations);
       * this may happen after manual strong branching on an integral variable, or after conflict analysis on
       * a strong branching conflict created a constraint that is not able to modify the LP but trigger the additional
       * call of the separators, in particular, the Gomory separator
       */
      CHECK_ZERO( CPXdualopt(cpxenv, lpi->cpxlp) );
      assert(CPXgetphase1cnt(cpxenv, lpi->cpxlp) == 0);
      assert(CPXgetitcnt(cpxenv, lpi->cpxlp) == 0);
      retval = CPXbinvcol(cpxenv, lpi->cpxlp, c, coef);
   }
   CHECK_ZERO( retval );

   return SCIP_OKAY;
}

/** get dense row of inverse basis matrix times constraint matrix B^-1 * A */
SCIP_RETCODE SCIPlpiGetBInvARow(
   SCIP_LPI*             lpi,                /**< LP interface structure */
   int                   r,                  /**< row number */
   const SCIP_Real*      binvrow,            /**< row in (A_B)^-1 from prior call to SCIPlpiGetBInvRow(), or NULL */
   SCIP_Real*            coef                /**< vector to return coefficients */
   )
{  /*lint --e{715}*/
   int retval;

   assert(cpxenv != NULL);
   assert(lpi != NULL);
   assert(lpi->cpxlp != NULL);

   SCIPdebugMessage("getting binva-row %d\n", r);

   retval = CPXbinvarow(cpxenv, lpi->cpxlp, r, coef);
   if( retval == CPXERR_NO_SOLN || retval == CPXERR_NO_LU_FACTOR || retval == CPXERR_NO_BASIC_SOLN
      || retval == CPXERR_NO_BASIS )
   {
      /* modifying the LP, restoring the old LP, and loading the old basis is not enough for CPLEX to be able to
       * return the basis -> we have to resolve the LP (should be done in 0 iterations);
       * this may happen after manual strong branching on an integral variable, or after conflict analysis on
       * a strong branching conflict created a constraint that is not able to modify the LP but trigger the additional
       * call of the separators, in particular, the Gomory separator
       */
      CHECK_ZERO( CPXdualopt(cpxenv, lpi->cpxlp) );

      /* In a numerical perfect world, the 10 below should be zero. However, due to numerical inaccuracies after refactorization, 
       * it might be necessary to do one (or even a few) extra pivot steps, in particular if FASTMIP is used. */ 
      assert(CPXgetphase1cnt(cpxenv, lpi->cpxlp) <= 10);
      assert(CPXgetitcnt(cpxenv, lpi->cpxlp) <= 10);
      retval = CPXbinvarow(cpxenv, lpi->cpxlp, r, coef);
   }
   CHECK_ZERO( retval );

   return SCIP_OKAY;
}

/** get dense column of inverse basis matrix times constraint matrix B^-1 * A */
SCIP_RETCODE SCIPlpiGetBInvACol(
   SCIP_LPI*             lpi,                /**< LP interface structure */
   int                   c,                  /**< column number */
   SCIP_Real*            coef                /**< vector to return coefficients */
   )
{  /*lint --e{715}*/
   int retval;

   assert(cpxenv != NULL);
   assert(lpi != NULL);
   assert(lpi->cpxlp != NULL);

   SCIPdebugMessage("getting binva-col %d\n", c);

   retval = CPXbinvacol(cpxenv, lpi->cpxlp, c, coef);
   if( retval == CPXERR_NO_SOLN || retval == CPXERR_NO_LU_FACTOR || retval == CPXERR_NO_BASIC_SOLN
      || retval == CPXERR_NO_BASIS )
   {
      /* modifying the LP, restoring the old LP, and loading the old basis is not enough for CPLEX to be able to
       * return the basis -> we have to resolve the LP (should be done in 0 iterations);
       * this may happen after manual strong branching on an integral variable, or after conflict analysis on
       * a strong branching conflict created a constraint that is not able to modify the LP but trigger the additional
       * call of the separators, in particular, the Gomory separator
       */
      CHECK_ZERO( CPXdualopt(cpxenv, lpi->cpxlp) );

      /* In a numerical perfect world, the 10 below should be zero. However, due to numerical inaccuracies after refactorization, 
       * it might be necessary to do one (or even a few) extra pivot steps, in particular if FASTMIP is used. */ 
      assert(CPXgetphase1cnt(cpxenv, lpi->cpxlp) <= 10);
      assert(CPXgetitcnt(cpxenv, lpi->cpxlp) <= 10);
      retval = CPXbinvacol(cpxenv, lpi->cpxlp, c, coef);
   }
   CHECK_ZERO( retval );

   return SCIP_OKAY;
}

/**@} */




/*
 * LP State Methods
 */

/**@name LP State Methods */
/**@{ */

/** stores LPi state (like basis information) into lpistate object */
SCIP_RETCODE SCIPlpiGetState(
   SCIP_LPI*             lpi,                /**< LP interface structure */
   BMS_BLKMEM*           blkmem,             /**< block memory */
   SCIP_LPISTATE**       lpistate            /**< pointer to LPi state information (like basis information) */
   )
{
   int ncols;
   int nrows;

   assert(blkmem != NULL);
   assert(cpxenv != NULL);
   assert(lpi != NULL);
   assert(lpi->cpxlp != NULL);
   assert(lpistate != NULL);

   /* if there is no basis information available (e.g. after barrier without crossover), no state can be saved */
   if( !lpi->solisbasic )
   {
      *lpistate = NULL;
      return SCIP_OKAY;
   }

   ncols = CPXgetnumcols(cpxenv, lpi->cpxlp);
   nrows = CPXgetnumrows(cpxenv, lpi->cpxlp);
   assert(ncols >= 0);
   assert(nrows >= 0);
   
   /* allocate lpistate data */
   SCIP_CALL( lpistateCreate(lpistate, blkmem, ncols, nrows) );

   SCIPdebugMessage("storing CPLEX LPI state in %p (%d cols, %d rows)\n", *lpistate, ncols, nrows);

   /* get unpacked basis information from CPLEX */
   SCIP_CALL( getBase(lpi) );

   /* pack LPi state data */
   (*lpistate)->ncols = ncols;
   (*lpistate)->nrows = nrows;
   lpistatePack(*lpistate, lpi->cstat, lpi->rstat);

   return SCIP_OKAY;
}

/** loads LPi state (like basis information) into solver; note that the LP might have been extended with additional
 *  columns and rows since the state was stored with SCIPlpiGetState()
 */
SCIP_RETCODE SCIPlpiSetState(
   SCIP_LPI*             lpi,                /**< LP interface structure */
   BMS_BLKMEM*           blkmem,             /**< block memory */
   SCIP_LPISTATE*        lpistate            /**< LPi state information (like basis information) */
   )
{
   int lpncols;
   int lpnrows;
   int i;

   assert(blkmem != NULL);
   assert(cpxenv != NULL);
   assert(lpi != NULL);
   assert(lpi->cpxlp != NULL);

   /* if there was no basis information available, the LPI state was not stored */
   if( lpistate == NULL )
      return SCIP_OKAY;

   lpncols = CPXgetnumcols(cpxenv, lpi->cpxlp);
   lpnrows = CPXgetnumrows(cpxenv, lpi->cpxlp);
   assert(lpistate->ncols <= lpncols);
   assert(lpistate->nrows <= lpnrows);

   SCIPdebugMessage("loading LPI state %p (%d cols, %d rows) into CPLEX LP with %d cols and %d rows\n",
      lpistate, lpistate->ncols, lpistate->nrows, lpncols, lpnrows);

   if( lpistate->ncols == 0 || lpistate->nrows == 0 )
      return SCIP_OKAY;   

   /* allocate enough memory for storing uncompressed basis information */
   SCIP_CALL( ensureCstatMem(lpi, lpncols) );
   SCIP_CALL( ensureRstatMem(lpi, lpnrows) );

   /* unpack LPi state data */
   lpistateUnpack(lpistate, lpi->cstat, lpi->rstat);

   /* extend the basis to the current LP */
   for( i = lpistate->ncols; i < lpncols; ++i )
      lpi->cstat[i] = CPX_AT_LOWER; /**@todo this has to be corrected for lb = -infinity */
   for( i = lpistate->nrows; i < lpnrows; ++i )
      lpi->rstat[i] = CPX_BASIC;

   /* load basis information into CPLEX */
   SCIP_CALL( setBase(lpi) );

   return SCIP_OKAY;
}

/** frees LPi state information */
SCIP_RETCODE SCIPlpiFreeState(
   SCIP_LPI*             lpi,                /**< LP interface structure */
   BMS_BLKMEM*           blkmem,             /**< block memory */
   SCIP_LPISTATE**       lpistate            /**< pointer to LPi state information (like basis information) */
   )
{
   assert(lpi != NULL);
   assert(lpistate != NULL);

   if( *lpistate != NULL )
   {
      lpistateFree(lpistate, blkmem);
   }

   return SCIP_OKAY;
}

/** checks, whether the given LP state contains simplex basis information */
SCIP_Bool SCIPlpiHasStateBasis(
   SCIP_LPI*             lpi,                /**< LP interface structure */
   SCIP_LPISTATE*        lpistate            /**< LP state information (like basis information) */
   )
{  /*lint --e{715}*/
   return (lpistate != NULL);
}

/** reads LP state (like basis information from a file */
SCIP_RETCODE SCIPlpiReadState(
   SCIP_LPI*             lpi,                /**< LP interface structure */
   const char*           fname               /**< file name */
   )
{
   assert(cpxenv != NULL);
   assert(lpi != NULL);
   assert(lpi->cpxlp != NULL);

   SCIPdebugMessage("reading LP state from file <%s>\n", fname);

   CHECK_ZERO( CPXreadcopybase(cpxenv, lpi->cpxlp, fname) );

   return SCIP_OKAY;
}

/** writes LP state (like basis information) to a file */
SCIP_RETCODE SCIPlpiWriteState(
   SCIP_LPI*             lpi,                /**< LP interface structure */
   const char*           fname               /**< file name */
   )
{
   assert(cpxenv != NULL);
   assert(lpi != NULL);
   assert(lpi->cpxlp != NULL);

   SCIPdebugMessage("writing LP state to file <%s>\n", fname);

   CHECK_ZERO( CPXmbasewrite(cpxenv, lpi->cpxlp, fname) );

   return SCIP_OKAY;
}

/**@} */




/*
 * Parameter Methods
 */

/**@name Parameter Methods */
/**@{ */

/** gets integer parameter of LP */
SCIP_RETCODE SCIPlpiGetIntpar(
   SCIP_LPI*             lpi,                /**< LP interface structure */
   SCIP_LPPARAM          type,               /**< parameter number */
   int*                  ival                /**< buffer to store the parameter value */
   )
{
   assert(cpxenv != NULL);
   assert(lpi != NULL);
   assert(lpi->cpxlp != NULL);
   assert(ival != NULL);

   SCIPdebugMessage("getting int parameter %d\n", type);

   switch( type )
   {
   case SCIP_LPPAR_FROMSCRATCH:
      *ival = (getIntParam(lpi, CPX_PARAM_ADVIND) == CPX_OFF);
      break;
   case SCIP_LPPAR_FASTMIP:
      *ival = getIntParam(lpi, CPX_PARAM_FASTMIP);
      break;
   case SCIP_LPPAR_SCALING:
#if (CPX_VERSION <= 1100)
      if( lpi->rngfound )
         return SCIP_PARAMETERUNKNOWN;
#endif
      *ival = (getIntParam(lpi, CPX_PARAM_SCAIND) == 0);
      break;
   case SCIP_LPPAR_PRESOLVING:
      *ival = (getIntParam(lpi, CPX_PARAM_PREIND) == CPX_ON);
      break;
   case SCIP_LPPAR_PRICING:
      *ival = (int)lpi->pricing; /* store pricing method in LPI struct */
      break;
#if 0
   case SCIP_LPPAR_PRICING:
      switch( getIntParam(lpi, CPX_PARAM_PPRIIND) )
      {
      case CPX_PPRIIND_FULL:
         *ival = (int)SCIP_PRICING_FULL;
         break;
      case CPX_PPRIIND_PARTIAL:
         *ival = (int)SCIP_PRICING_PARTIAL;
         break;
      case CPX_PPRIIND_STEEP:
         *ival = (int)SCIP_PRICING_STEEP;
         break;
      case CPX_PPRIIND_STEEPQSTART:
         *ival = (int)SCIP_PRICING_STEEPQSTART;
         break;
#if (CPX_VERSION >= 900)
      case CPX_PPRIIND_DEVEX:
         *ival = (int)SCIP_PRICING_DEVEX;
         break;
#endif
      default:
         *ival = (int)SCIP_PRICING_AUTO;
         break;
      }
      break;
#endif
   case SCIP_LPPAR_LPINFO:
      *ival = (getIntParam(lpi, CPX_PARAM_SCRIND) == CPX_ON);
      break;
   case SCIP_LPPAR_LPITLIM:
      *ival = getIntParam(lpi, CPX_PARAM_ITLIM);
      if( *ival >= CPX_INT_MAX )
         *ival = INT_MAX;
      break;
   case SCIP_LPPAR_THREADS:
#if (CPX_VERSION == 1100 || CPX_VERSION == 1220)
      /**< Due to CPLEX bug, we always set the thread count to 1. In order to fulfill an assert in lp.c, we have to
       *   return the value set by SCIP and not the real thread count */
      *ival = lpi->pseudonthreads;
      assert(getIntParam(lpi, CPX_PARAM_THREADS) == 1);
#else
      *ival = getIntParam(lpi, CPX_PARAM_THREADS);
#endif
      break;
   default:
      return SCIP_PARAMETERUNKNOWN;
   }  /*lint !e788*/

   return SCIP_OKAY;
}

/** sets integer parameter of LP */
SCIP_RETCODE SCIPlpiSetIntpar(
   SCIP_LPI*             lpi,                /**< LP interface structure */
   SCIP_LPPARAM          type,               /**< parameter number */
   int                   ival                /**< parameter value */
   )
{
   assert(cpxenv != NULL);
   assert(lpi != NULL);
   assert(lpi->cpxlp != NULL);

   SCIPdebugMessage("setting int parameter %d to %d\n", type, ival);

   switch( type )
   {
   case SCIP_LPPAR_FROMSCRATCH:
      assert(ival == TRUE || ival == FALSE);
      setIntParam(lpi, CPX_PARAM_ADVIND, ival == FALSE ? CPX_ON : CPX_OFF);
      break;
   case SCIP_LPPAR_FASTMIP:
      assert(0 <= ival && ival <= 2);
      setIntParam(lpi, CPX_PARAM_FASTMIP, ival);
      break;
   case SCIP_LPPAR_SCALING:
      assert(ival == TRUE || ival == FALSE);
#if (CPX_VERSION <= 1100)
      if( lpi->rngfound )
         return SCIP_PARAMETERUNKNOWN;
#endif
      setIntParam(lpi, CPX_PARAM_SCAIND, ival == TRUE ? 0 : -1);
      break;
   case SCIP_LPPAR_PRESOLVING:
      assert(ival == TRUE || ival == FALSE);
      setIntParam(lpi, CPX_PARAM_PREIND, ival == TRUE ? CPX_ON : CPX_OFF);
      break;
   case SCIP_LPPAR_PRICING:
      lpi->pricing = (SCIP_PRICING)ival;
      switch( (SCIP_PRICING)ival )
      {
      case SCIP_PRICING_AUTO:
	 setIntParam(lpi, CPX_PARAM_PPRIIND, CPX_PPRIIND_AUTO);
	 setIntParam(lpi, CPX_PARAM_DPRIIND, CPX_DPRIIND_AUTO);
         break;
      case SCIP_PRICING_FULL:
	 setIntParam(lpi, CPX_PARAM_PPRIIND, CPX_PPRIIND_FULL);
	 setIntParam(lpi, CPX_PARAM_DPRIIND, CPX_DPRIIND_FULL);
         break;
      case SCIP_PRICING_PARTIAL:
	 setIntParam(lpi, CPX_PARAM_PPRIIND, CPX_PPRIIND_PARTIAL);
	 setIntParam(lpi, CPX_PARAM_DPRIIND, CPX_DPRIIND_AUTO);
         break;
      case SCIP_PRICING_STEEP:
	 setIntParam(lpi, CPX_PARAM_PPRIIND, CPX_PPRIIND_STEEP);
	 setIntParam(lpi, CPX_PARAM_DPRIIND, CPX_DPRIIND_STEEP);
	 break;
      case SCIP_PRICING_LPIDEFAULT:
      case SCIP_PRICING_STEEPQSTART:
	 setIntParam(lpi, CPX_PARAM_PPRIIND, CPX_PPRIIND_STEEPQSTART);
	 setIntParam(lpi, CPX_PARAM_DPRIIND, CPX_DPRIIND_STEEPQSTART);
	 break;
#if (CPX_VERSION >= 900)
      case SCIP_PRICING_DEVEX:
	 setIntParam(lpi, CPX_PARAM_PPRIIND, CPX_PPRIIND_DEVEX);
	 setIntParam(lpi, CPX_PARAM_DPRIIND, CPX_DPRIIND_DEVEX);
	 break;
#endif
      default:
         return SCIP_LPERROR;
      }
      break;
   case SCIP_LPPAR_LPINFO:
      assert(ival == TRUE || ival == FALSE);
      if( ival )
	 setIntParam(lpi, CPX_PARAM_SCRIND, CPX_ON);
      else 
	 setIntParam(lpi, CPX_PARAM_SCRIND, CPX_OFF);
      break;
   case SCIP_LPPAR_LPITLIM:
      ival = MIN(ival, CPX_INT_MAX);
      setIntParam(lpi, CPX_PARAM_ITLIM, ival);
      break;
   case SCIP_LPPAR_THREADS:
#if (CPX_VERSION == 1100 || CPX_VERSION == 1220)
      /**< Due to CPLEX bug, we always set the thread count to 1. In order to fulfill an assert in lp.c, we have to
       *   store the value set by SCIP and return it later instead of the real thread count */
      lpi->pseudonthreads = ival;
      ival = 1;
#else
      ival = MIN(ival, CPX_INT_MAX);
#endif
      setIntParam(lpi, CPX_PARAM_THREADS, ival);
      break;
   default:
      return SCIP_PARAMETERUNKNOWN;
   }  /*lint !e788*/

   return SCIP_OKAY;
}

/** gets floating point parameter of LP */
SCIP_RETCODE SCIPlpiGetRealpar(
   SCIP_LPI*             lpi,                /**< LP interface structure */
   SCIP_LPPARAM          type,               /**< parameter number */
   SCIP_Real*            dval                /**< buffer to store the parameter value */
   )
{
   assert(cpxenv != NULL);
   assert(lpi != NULL);
   assert(lpi->cpxlp != NULL);
   assert(dval != NULL);

   SCIPdebugMessage("getting real parameter %d\n", type);

   switch( type )
   {
   case SCIP_LPPAR_FEASTOL:
      *dval = getDblParam(lpi, CPX_PARAM_EPRHS);
      break;
   case SCIP_LPPAR_DUALFEASTOL:
      *dval = getDblParam(lpi, CPX_PARAM_EPOPT);
      break;
   case SCIP_LPPAR_BARRIERCONVTOL:
      *dval = getDblParam(lpi, CPX_PARAM_BAREPCOMP);
      break;
   case SCIP_LPPAR_LOBJLIM:
      *dval = getDblParam(lpi, CPX_PARAM_OBJLLIM);
      break;
   case SCIP_LPPAR_UOBJLIM:
      *dval = getDblParam(lpi, CPX_PARAM_OBJULIM);
      break;
   case SCIP_LPPAR_LPTILIM:
      *dval = getDblParam(lpi, CPX_PARAM_TILIM);
      break;
   case SCIP_LPPAR_MARKOWITZ:
      *dval = getDblParam(lpi, CPX_PARAM_EPMRK);
      break;
   default:
      return SCIP_PARAMETERUNKNOWN;
   }  /*lint !e788*/
   
   return SCIP_OKAY;
}

/** sets floating point parameter of LP */
SCIP_RETCODE SCIPlpiSetRealpar(
   SCIP_LPI*             lpi,                /**< LP interface structure */
   SCIP_LPPARAM          type,               /**< parameter number */
   SCIP_Real             dval                /**< parameter value */
   )
{
   assert(cpxenv != NULL);
   assert(lpi != NULL);
   assert(lpi->cpxlp != NULL);

   SCIPdebugMessage("setting real parameter %d to %.15g\n", type, dval);

   switch( type )
   {
   case SCIP_LPPAR_FEASTOL:
      setDblParam(lpi, CPX_PARAM_EPRHS, dval);
      break;
   case SCIP_LPPAR_DUALFEASTOL:
      setDblParam(lpi, CPX_PARAM_EPOPT, dval);
      break;
   case SCIP_LPPAR_BARRIERCONVTOL:
      setDblParam(lpi, CPX_PARAM_BAREPCOMP, dval);
      break;
   case SCIP_LPPAR_LOBJLIM:
      setDblParam(lpi, CPX_PARAM_OBJLLIM, dval);
      break;
   case SCIP_LPPAR_UOBJLIM:
      setDblParam(lpi, CPX_PARAM_OBJULIM, dval);
      break;
   case SCIP_LPPAR_LPTILIM:
      setDblParam(lpi, CPX_PARAM_TILIM, dval);
      break;
   case SCIP_LPPAR_MARKOWITZ:
      setDblParam(lpi, CPX_PARAM_EPMRK, dval);
      break;
   default:
      return SCIP_PARAMETERUNKNOWN;
   }  /*lint !e788*/

   return SCIP_OKAY;
}

/**@} */




/*
 * Numerical Methods
 */

/**@name Numerical Methods */
/**@{ */

/** returns value treated as infinity in the LP solver */
SCIP_Real SCIPlpiInfinity(
   SCIP_LPI*             lpi                 /**< LP interface structure */
   )
{  /*lint --e{715}*/
   return CPX_INFBOUND;
}

/** checks if given value is treated as infinity in the LP solver */
SCIP_Bool SCIPlpiIsInfinity(
   SCIP_LPI*             lpi,                /**< LP interface structure */
   SCIP_Real             val                 /**< value to be checked for infinity */
   )
{  /*lint --e{715}*/
   return (val >= CPX_INFBOUND);
}

/**@} */




/*
 * File Interface Methods
 */

/**@name File Interface Methods */
/**@{ */

/** reads LP from a file */
SCIP_RETCODE SCIPlpiReadLP(
   SCIP_LPI*             lpi,                /**< LP interface structure */
   const char*           fname               /**< file name */
   )
{
   int restat;

   assert(cpxenv != NULL);
   assert(lpi != NULL);
   assert(lpi->cpxlp != NULL);

   SCIPdebugMessage("reading LP from file <%s>\n", fname);

   restat = CPXreadcopyprob(cpxenv, lpi->cpxlp, fname, NULL);
   if ( restat != 0 )
   {
      SCIPerrorMessage("LP Error: CPLEX returned %d\n", restat);
      return SCIP_READERROR;
   }

   return SCIP_OKAY;
}

/** writes LP to a file */
SCIP_RETCODE SCIPlpiWriteLP(
   SCIP_LPI*             lpi,                /**< LP interface structure */
   const char*           fname               /**< file name */
   )
{
   int restat;

   assert(cpxenv != NULL);
   assert(lpi != NULL);
   assert(lpi->cpxlp != NULL);

   SCIPdebugMessage("writing LP to file <%s>\n", fname);

   restat = CPXwriteprob(cpxenv, lpi->cpxlp, fname, NULL);
   if ( restat != 0 )
   {
      SCIPerrorMessage("LP Error: CPLEX returned %d\n", restat);
      return SCIP_READERROR;
   }

   return SCIP_OKAY;
}

/**@} */


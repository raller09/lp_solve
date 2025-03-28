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
#pragma ident "@(#) $Id: reader_col.c,v 1.11 2010/09/17 17:02:52 bzfgamra Exp $"

/**@file   reader_col.c
 * @brief  COL file reader
 * @author Gerald Gamrath
 *
 * This file implements the reader for coloring files in DIMACS standard format.
 *
 * Additionally, it provides two sorting functions and a method, which ensures that all nodes in the
 * graph are covered by at least one stable set.
 *
 */

/*---+----1----+----2----+----3----+----4----+----5----+----6----+----7----+----8----+----9----+----0----+----1----+----2*/
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include "reader_col.h"


#define READER_NAME             "colreader"
#define READER_DESC             "file reader for a .col-file representing a graph that should be colored"
#define READER_EXTENSION        "col"

#define COL_MAX_LINELEN 1024


/*
 * Data structures
 */

/** data for col reader */
struct SCIP_ReaderData
{
};


/*
 * Local methods
 */

/** get next number from string s */
static
long getNextNumber(
   char**                s                   /**< pointer to the pointer of the current position in the string */
   )
{
  long tmp;
  /* skip whitespaces */
  while ( isspace(**s) )
    ++(*s);
  /* read number */
  tmp = atol(*s);
  /* skip whitespaces */
  while ( (**s != 0) && (!isspace(**s)) )
    ++(*s);
  return tmp;
}

/** read LP in "COL File Format" */  
static
SCIP_RETCODE readCol(
   SCIP*                 scip,               /**< SCIP data structure */   
   const char*           filename            /**< name of the input file */
   )
{
   SCIP_FILE* fp;               /* file-reader */
   char buf[COL_MAX_LINELEN];   /* maximal length of line */
   long nedges;
   long nnodes;
   int line_nr;
   char* char_p;
   char* probname;
   int** edges;
   int i;
   int j;
   int begin;
   int end;
   int nduplicateedges;
   SCIP_Bool duplicateedge;

   
   assert(scip != NULL);
   assert(filename != NULL);
   
   if (NULL == (fp = SCIPfopen(filename, "r")))
   {
      SCIPerrorMessage("cannot open file <%s> for reading\n", filename);
      perror(filename);
      return SCIP_NOFILE;
   }
   
   /* Get problem name from filename and save it */
   SCIPfgets(buf, sizeof(buf), fp);
   i = 1;
   while ( (filename[i] != '/') && (filename[i] != '\0') )
   {
      i++;
   }
   if ( filename[i] != '/' )
   {
      j = i;
      i = -1;
   }
   else
   {
      j = i+1;
      while ( filename[i] == '/' && filename[j] != '\0' )
      {
         j = i+1;
         while ( filename[j] != '\0' )
         {
            j++;
            if ( filename[j] == '/' )
            {
               i = j;
               break;
            }
         }
      }
   }
   
   SCIPallocMemoryArray(scip, &probname, j-i-4);
   strncpy(probname, &filename[i+1], j-i-5);
   probname[j-i-5]= '\0';

   /* Read until information about graph starts */
   while( !SCIPfeof(fp) && (buf[0] != 'p') )
   {
      SCIPfgets(buf, sizeof(buf), fp);
      line_nr++;
   } 
   /* no graph information in file! */
   if ( SCIPfeof(fp) )
   {
      SCIPerrorMessage("Error! Could not find line starting with 'p'.\n");
      return SCIP_READERROR;
   }
   /* wrong format of the line containig number of nodes and edges */
   if ( buf[2] != 'e' || buf[3] != 'd' || buf[4] != 'g' || buf[5] != 'e' )
   {
      SCIPerrorMessage("Line starting with 'p' must continue with 'edge'!\n");
      return SCIP_READERROR;
   }
   char_p = &buf[6];
   /* if line reads 'edges' (non-standard!), instead of 'edge'. */
   if ( *char_p == 's' )
      ++(char_p);

   /* read out number of nodes and edges, the pointer char_p will be changed */
   nduplicateedges = 0;
   nnodes = getNextNumber(&char_p);
   nedges = getNextNumber(&char_p);
   if ( nnodes <= 0 )
   {
      SCIPerrorMessage("Number of vertices must be positive!\n");
      return SCIP_READERROR;
   }
   if ( nedges < 0 )
   {	  
      SCIPerrorMessage("Number of edges must be nonnegative!\n");
      return SCIP_READERROR;
   }
   /* create array for edges */
   SCIP_CALL( SCIPallocMemoryArray(scip, &edges, nedges) );
   for( i = 0; i < nedges; i++)
   {
      SCIP_CALL( SCIPallocMemoryArray(scip, &(edges[i]), 2) );
   }
   /* fill array for edges */
   SCIPfgets(buf, sizeof(buf), fp);
   line_nr++;
   i = 0;
   while ( !SCIPfeof(fp) )
   {
      if ( buf[0] == 'e')
      {
         duplicateedge = FALSE;
         char_p = &buf[2];
         
         begin = getNextNumber(&char_p);
         end = getNextNumber(&char_p);
         for ( j = 0; j < i; j++)
         {
            if ( ((edges[j][0] == begin) && (edges[j][1] == end))
               || ((edges[j][1] == begin) && (edges[j][0] == end)) )
            {
               duplicateedge = TRUE;
               nduplicateedges++;
               break;
            }
         }
         if ( !duplicateedge )
         {
            edges[i][0] = begin;
            edges[i][1] = end;
            assert((edges[i][0] > 0) && (edges[i][0] <= nnodes));
            assert((edges[i][1] > 0) && (edges[i][1] <= nnodes));
            i++;
         }
      }
      SCIPfgets(buf, sizeof(buf), fp);
      line_nr++;
   }
   if ( nduplicateedges > 0 )
   {
      printf("%d duplicate edges!\n", nduplicateedges);
   }
   
   /* create problem data */
   SCIP_CALL( SCIPcreateProbColoring(scip, probname, nnodes, nedges-nduplicateedges, edges) );

   /* create LP */
   SCIPdebugMessage("Erstelle LP...\n");
   COLORprobSetUpArrayOfCons(scip);

   
   /* activate the pricer */
   SCIP_CALL( SCIPactivatePricer(scip, SCIPfindPricer(scip, "coloring")) );
   SCIP_CALL( SCIPsetObjIntegral(scip) );
   for ( i = nedges-1; i >= 0; i--)
   {
      SCIPfreeMemoryArray(scip, &(edges[i]));
   }
   SCIPfreeMemoryArray(scip, &edges);
   SCIPfreeMemoryArray(scip, &probname);
   SCIPfclose(fp);

   return SCIP_OKAY;
}




/*
 * Callback methods of reader
 */

/** copy method for reader plugins (called when SCIP copies plugins) */
static
SCIP_DECL_READERCOPY(readerCopyCol)
{  /*lint --e{715}*/
   assert(scip != NULL);
   assert(reader != NULL);
   assert(strcmp(SCIPreaderGetName(reader), READER_NAME) == 0);
 
   return SCIP_OKAY;
}

#define readerFreeCol NULL

/** problem reading method of reader */
static
SCIP_DECL_READERREAD(readerReadCol)
{  /*lint --e{715}*/
   assert(reader != NULL);
   assert(strcmp(SCIPreaderGetName(reader), READER_NAME) == 0);
   assert(scip != NULL);
   assert(result != NULL);
   
   SCIP_CALL( readCol(scip, filename) );
   
   *result = SCIP_SUCCESS;
   
   return SCIP_OKAY;
}




/*
 * col file reader specific interface methods
 */

/** includes the col file reader in SCIP */
SCIP_RETCODE SCIPincludeReaderCol(
   SCIP*                 scip                /**< SCIP data structure */
   )
{
  SCIP_READERDATA* readerdata;

  /* create col reader data */
  readerdata = NULL;

  /* include col reader */
  SCIP_CALL( SCIPincludeReader(scip, READER_NAME, READER_DESC, READER_EXTENSION,
        readerCopyCol,
        readerFreeCol, readerReadCol, NULL, readerdata) );


  return SCIP_OKAY;
}

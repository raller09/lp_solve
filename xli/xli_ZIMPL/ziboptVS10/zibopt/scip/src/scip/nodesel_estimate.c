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
#pragma ident "@(#) $Id: nodesel_estimate.c,v 1.12 2010/09/27 17:20:23 bzfheinz Exp $"

/**@file   nodesel_estimate.c
 * @ingroup NODESELECTORS
 * @brief  node selector for best estimate search
 * @author Tobias Achterberg
 */

/*---+----1----+----2----+----3----+----4----+----5----+----6----+----7----+----8----+----9----+----0----+----1----+----2*/

#include <assert.h>
#include <string.h>

#include "scip/nodesel_estimate.h"


#define NODESEL_NAME             "estimate"
#define NODESEL_DESC             "best estimate search"
#define NODESEL_STDPRIORITY      200000
#define NODESEL_MEMSAVEPRIORITY     100




/*
 * Default parameter settings
 */

#define MINPLUNGEDEPTH               -1 /**< minimal plunging depth, before new best node may be selected (-1 for dynamic setting) */
#define MAXPLUNGEDEPTH               -1 /**< maximal plunging depth, before new best node is forced to be selected (-1 for dynamic setting) */
#define MAXPLUNGEQUOT              0.25 /**< maximal quotient (curlowerbound - lowerbound)/(cutoffbound - lowerbound)
                                         *   where plunging is performed */
#define BESTNODEFREQ                 10 /**< frequency at which the best node instead of the best estimate is selected (0: never) */


/** node selector data for best estimate search node selection */
struct SCIP_NodeselData
{
   SCIP_Real             maxplungequot;      /**< maximal quotient (curlowerbound - lowerbound)/(cutoffbound - lowerbound)
                                              *   where plunging is performed */
   int                   minplungedepth;     /**< minimal plunging depth, before new best node may be selected
                                              *   (-1 for dynamic setting) */
   int                   maxplungedepth;     /**< maximal plunging depth, before new best node is forced to be selected
                                              *   (-1 for dynamic setting) */
   int                   bestnodefreq;       /**< frequency at which the best node instead of the best estimate is selected
                                              *   (0: never) */
};



/*
 * Callback methods
 */

/** copy method for node selector plugins (called when SCIP copies plugins) */
static
SCIP_DECL_NODESELCOPY(nodeselCopyEstimate)
{  /*lint --e{715}*/
   assert(scip != NULL);
   assert(nodesel != NULL);
   assert(strcmp(SCIPnodeselGetName(nodesel), NODESEL_NAME) == 0);

   /* call inclusion method of node selector */
   SCIP_CALL( SCIPincludeNodeselEstimate(scip) );

   return SCIP_OKAY;
}

/** destructor of node selector to free user data (called when SCIP is exiting) */
static
SCIP_DECL_NODESELFREE(nodeselFreeEstimate)
{  /*lint --e{715}*/
   SCIP_NODESELDATA* nodeseldata;

   assert(nodesel != NULL);
   assert(strcmp(SCIPnodeselGetName(nodesel), NODESEL_NAME) == 0);
   assert(scip != NULL);

   /* free user data of node selector */
   nodeseldata = SCIPnodeselGetData(nodesel);
   assert(nodeseldata != NULL);
   SCIPfreeMemory(scip, &nodeseldata);
   SCIPnodeselSetData(nodesel, nodeseldata);

   return SCIP_OKAY;
}

/** initialization method of node selector (called after problem was transformed) */
#define nodeselInitEstimate NULL


/** deinitialization method of node selector (called before transformed problem is freed) */
#define nodeselExitEstimate NULL


/** solving process initialization method of node selector (called when branch and bound process is about to begin) */
#define nodeselInitsolEstimate NULL


/** solving process deinitialization method of node selector (called before branch and bound process data is freed) */
#define nodeselExitsolEstimate NULL


/** node selection method of node selector */
static
SCIP_DECL_NODESELSELECT(nodeselSelectEstimate)
{  /*lint --e{715}*/
   SCIP_NODESELDATA* nodeseldata;
   int minplungedepth;
   int maxplungedepth;
   int plungedepth;
   int bestnodefreq;
   SCIP_Real maxplungequot;

   assert(nodesel != NULL);
   assert(strcmp(SCIPnodeselGetName(nodesel), NODESEL_NAME) == 0);
   assert(scip != NULL);
   assert(selnode != NULL);

   *selnode = NULL;

   /* get node selector user data */
   nodeseldata = SCIPnodeselGetData(nodesel);
   assert(nodeseldata != NULL);

   /* calculate minimal and maximal plunging depth */
   minplungedepth = nodeseldata->minplungedepth;
   maxplungedepth = nodeseldata->maxplungedepth;
   maxplungequot = nodeseldata->maxplungequot;
   if( minplungedepth == -1 )
   {
      minplungedepth = SCIPgetMaxDepth(scip)/10;
      if( SCIPgetNStrongbranchLPIterations(scip) > 2*SCIPgetNNodeLPIterations(scip) )
        minplungedepth += 10;
      if( maxplungedepth >= 0 )
         minplungedepth = MIN(minplungedepth, maxplungedepth);
   }
   if( maxplungedepth == -1 )
      maxplungedepth = SCIPgetMaxDepth(scip)/2;
   maxplungedepth = MAX(maxplungedepth, minplungedepth);
   bestnodefreq = (nodeseldata->bestnodefreq == 0 ? INT_MAX : nodeseldata->bestnodefreq);

   /* check, if we exceeded the maximal plunging depth */
   plungedepth = SCIPgetPlungeDepth(scip);
   if( plungedepth > maxplungedepth )
   {
      /* we don't want to plunge again: select best node from the tree */
      SCIPdebugMessage("plungedepth: [%d,%d], cur: %d -> abort plunging\n", minplungedepth, maxplungedepth, plungedepth);
      if( SCIPgetNNodes(scip) % bestnodefreq == 0 )
         *selnode = SCIPgetBestboundNode(scip);
      else
         *selnode = SCIPgetBestNode(scip);
      SCIPdebugMessage("  -> best node   : lower=%g\n",
         *selnode != NULL ? SCIPnodeGetLowerbound(*selnode) : SCIPinfinity(scip));
   }
   else
   {
      SCIP_NODE* node;
      SCIP_Real lowerbound;
      SCIP_Real cutoffbound;
      SCIP_Real maxbound;

      /* get global lower and cutoff bound */
      lowerbound = SCIPgetLowerbound(scip);
      cutoffbound = SCIPgetCutoffbound(scip);

      /* if we didn't find a solution yet, the cutoff bound is usually very bad:
       * use only 20% of the gap as cutoff bound
       */
      if( SCIPgetNSolsFound(scip) == 0 )
         cutoffbound = lowerbound + 0.2 * (cutoffbound - lowerbound);
         
      /* check, if plunging is forced at the current depth */
      if( plungedepth < minplungedepth )
         maxbound = SCIPinfinity(scip);
      else
      {
         /* calculate maximal plunging bound */
         maxbound = lowerbound + maxplungequot * (cutoffbound - lowerbound);
      }

      SCIPdebugMessage("plungedepth: [%d,%d], cur: %d, bounds: [%g,%g], maxbound: %g\n",
         minplungedepth, maxplungedepth, plungedepth, lowerbound, cutoffbound, maxbound);

      /* we want to plunge again: prefer children over siblings, and siblings over leaves,
       * but only select a child or sibling, if its estimate is small enough;
       * prefer using nodes with higher node selection priority assigned by the branching rule
       */
      node = SCIPgetPrioChild(scip);
      if( node != NULL && SCIPnodeGetEstimate(node) < maxbound )
      {
         *selnode = node;
         SCIPdebugMessage("  -> selected prio child: estimate=%g\n", SCIPnodeGetEstimate(*selnode));
      }
      else
      {
         node = SCIPgetBestChild(scip);
         if( node != NULL && SCIPnodeGetEstimate(node) < maxbound )
         {
            *selnode = node;
            SCIPdebugMessage("  -> selected best child: estimate=%g\n", SCIPnodeGetEstimate(*selnode));
         }
         else
         {
            node = SCIPgetPrioSibling(scip);
            if( node != NULL && SCIPnodeGetEstimate(node) < maxbound )
            {
               *selnode = node;
               SCIPdebugMessage("  -> selected prio sibling: estimate=%g\n", SCIPnodeGetEstimate(*selnode));
            }
            else
            {
               node = SCIPgetBestSibling(scip);
               if( node != NULL && SCIPnodeGetEstimate(node) < maxbound )
               {
                  *selnode = node;
                  SCIPdebugMessage("  -> selected best sibling: estimate=%g\n", SCIPnodeGetEstimate(*selnode));
               }
               else
               {
                  if( SCIPgetNNodes(scip) % bestnodefreq == 0 )
                     *selnode = SCIPgetBestboundNode(scip);
                  else
                     *selnode = SCIPgetBestNode(scip);
                  SCIPdebugMessage("  -> selected best leaf: estimate=%g\n",
                     *selnode != NULL ? SCIPnodeGetEstimate(*selnode) : SCIPinfinity(scip));
               }
            }
         }
      }
   }

   return SCIP_OKAY;
}


/** node comparison method of node selector */
static
SCIP_DECL_NODESELCOMP(nodeselCompEstimate)
{  /*lint --e{715}*/
   SCIP_Real estimate1;
   SCIP_Real estimate2;

   assert(nodesel != NULL);
   assert(strcmp(SCIPnodeselGetName(nodesel), NODESEL_NAME) == 0);
   assert(scip != NULL);

   estimate1 = SCIPnodeGetEstimate(node1);
   estimate2 = SCIPnodeGetEstimate(node2);
   if( SCIPisLT(scip, estimate1, estimate2) )
      return -1;
   else if( SCIPisGT(scip, estimate1, estimate2) )
      return +1;
   else
   {
      SCIP_Real lowerbound1;
      SCIP_Real lowerbound2;

      lowerbound1 = SCIPnodeGetLowerbound(node1);
      lowerbound2 = SCIPnodeGetLowerbound(node2);
      if( SCIPisLT(scip, lowerbound1, lowerbound2) )
         return -1;
      else if( SCIPisGT(scip, lowerbound1, lowerbound2) )
         return +1;
      else
      {
         SCIP_NODETYPE nodetype1;
         SCIP_NODETYPE nodetype2;

         nodetype1 = SCIPnodeGetType(node1);
         nodetype2 = SCIPnodeGetType(node2);
         if( nodetype1 == SCIP_NODETYPE_CHILD && nodetype2 != SCIP_NODETYPE_CHILD )
            return -1;
         else if( nodetype1 != SCIP_NODETYPE_CHILD && nodetype2 == SCIP_NODETYPE_CHILD )
            return +1;
         else if( nodetype1 == SCIP_NODETYPE_SIBLING && nodetype2 != SCIP_NODETYPE_SIBLING )
            return -1;
         else if( nodetype1 != SCIP_NODETYPE_SIBLING && nodetype2 == SCIP_NODETYPE_SIBLING )
            return +1;
         else
         {
            int depth1;
            int depth2;
         
            depth1 = SCIPnodeGetDepth(node1);
            depth2 = SCIPnodeGetDepth(node2);
            if( depth1 < depth2 )
               return -1;
            else if( depth1 > depth2 )
               return +1;
            else
               return 0;
         }
      }
   }
}





/*
 * estimate specific interface methods
 */

/** creates the node selector for best estimate search and includes it in SCIP */
SCIP_RETCODE SCIPincludeNodeselEstimate(
   SCIP*                 scip                /**< SCIP data structure */
   )
{
   SCIP_NODESELDATA* nodeseldata;

   /* allocate and initialize node selector data; this has to be freed in the destructor */
   SCIP_CALL( SCIPallocMemory(scip, &nodeseldata) );

   /* include node selector */
   SCIP_CALL( SCIPincludeNodesel(scip, NODESEL_NAME, NODESEL_DESC, NODESEL_STDPRIORITY, NODESEL_MEMSAVEPRIORITY,
         nodeselCopyEstimate,
         nodeselFreeEstimate, nodeselInitEstimate, nodeselExitEstimate, 
         nodeselInitsolEstimate, nodeselExitsolEstimate, nodeselSelectEstimate, nodeselCompEstimate,
         nodeseldata) );

   /* add node selector parameters */
   SCIP_CALL( SCIPaddIntParam(scip,
         "nodeselection/estimate/minplungedepth",
         "minimal plunging depth, before new best node may be selected (-1 for dynamic setting)",
         &nodeseldata->minplungedepth, TRUE, MINPLUNGEDEPTH, -1, INT_MAX, NULL, NULL) );
   SCIP_CALL( SCIPaddIntParam(scip,
         "nodeselection/estimate/maxplungedepth",
         "maximal plunging depth, before new best node is forced to be selected (-1 for dynamic setting)",
         &nodeseldata->maxplungedepth, TRUE, MAXPLUNGEDEPTH, -1, INT_MAX, NULL, NULL) );
   SCIP_CALL( SCIPaddRealParam(scip,
         "nodeselection/estimate/maxplungequot",
         "maximal quotient (estimate - lowerbound)/(cutoffbound - lowerbound) where plunging is performed",
         &nodeseldata->maxplungequot, TRUE, MAXPLUNGEQUOT, 0.0, SCIP_REAL_MAX, NULL, NULL) );
   SCIP_CALL( SCIPaddIntParam(scip,
         "nodeselection/estimate/bestnodefreq",
         "frequency at which the best node instead of the best estimate is selected (0: never)",
         &nodeseldata->bestnodefreq, FALSE, BESTNODEFREQ, 0, INT_MAX, NULL, NULL) );
   
   return SCIP_OKAY;
}


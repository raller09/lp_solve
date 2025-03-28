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
#pragma ident "@(#) $Id: cons_cumulative.c,v 1.18 2010/09/28 20:07:56 bzfheinz Exp $"

/**@file   cons_cumulative.c
 * @ingroup CONSHDLRS 
 * @brief  constraint handler for cumulative constraints
 * @author Timo Berthold
 * @author Stefan Heinz
 * @author Jens Schulz
 *
 * Given:
 * - a set of jobs, represented by their integer start time variables \f$S_j\f$, their array of processing times \f$p_j\f$ and of
 *   their demands \f$d_j\f$.
 * - an integer resource capacity \f$C\f$
 *
 * The cumulative constraint ensures that for each point in time \f$t\f$ \f$\sum_{j: S_j \leq t < S_j + p_j} d_j \leq C\f$ holds.
 *
 * Separation: 
 * - can be done using binary start time model, see Pritskers, Watters and Wolfe 
 * - or by just separating relatively weak cuts on the start time variables
 *
 * Propagation: 
 * - time tabling, Klein & Scholl (1999) 
 * - Edge-finding from Petr Vilim, adjusted and simplified for dynamic propagation
 *   (2009)
 * - energetic reasoning, see Baptiste, Le Pape, Nuijten (2001)
 */

/*---+----1----+----2----+----3----+----4----+----5----+----6----+----7----+----8----+----9----+----0----+----1----+----2*/

#include <assert.h>
#include <string.h>

#include "scip/cons_cumulative.h"
#include "scip/cons_linking.h"
#include "scip/cons_knapsack.h"

/* constraint handler properties */
#define CONSHDLR_NAME          "cumulative"
#define CONSHDLR_DESC          "cumulative constraint handler"
#define CONSHDLR_SEPAPRIORITY   2100000 /**< priority of the constraint handler for separation */
#define CONSHDLR_ENFOPRIORITY  -2040000 /**< priority of the constraint handler for constraint enforcing */
#define CONSHDLR_CHECKPRIORITY -3030000 /**< priority of the constraint handler for checking feasibility */
#define CONSHDLR_SEPAFREQ             1 /**< frequency for separating cuts; zero means to separate only in the root node */
#define CONSHDLR_PROPFREQ             5 /**< frequency for propagating domains; zero means only preprocessing propagation */
#define CONSHDLR_EAGERFREQ          100 /**< frequency for using all instead of only the useful constraints in separation,
                                         *   propagation and enforcement, -1 for no eager evaluations, 0 for first only */
#define CONSHDLR_MAXPREROUNDS        -1 /**< maximal number of presolving rounds the constraint handler participates in (-1: no limit) */
#define CONSHDLR_DELAYSEPA        FALSE /**< should separation method be delayed, if other separators found cuts? */
#define CONSHDLR_DELAYPROP        FALSE /**< should propagation method be delayed, if other propagators found reductions? */
#define CONSHDLR_DELAYPRESOL      FALSE /**< should presolving method be delayed, if other presolvers found reductions? */
#define CONSHDLR_NEEDSCONS         TRUE /**< should the constraint handler be skipped, if no constraints are available? */

/* default parameter values */
#define DEFAULT_USEBINVARS             FALSE /**< should the binary representation be used? */
#define DEFAULT_LOCALCUTS              FALSE /**< should cuts be added only locally? */
#define DEFAULT_USECOVERCUTS            TRUE /**< should covering cuts be added? */
#define DEFAULT_USECORETIMES            TRUE /**< should core-times be propagated? */
#define DEFAULT_USECORETIMESHOLES      FALSE /**< should core-times be propagated to detect holes? */
#define DEFAULT_USEEDGEFINDING         FALSE /**< should edge finding be used? */
#define DEFAULT_USEENERGETICREASONING  FALSE /**< should energetic reasoning be used? */
#define DEFAULT_CUTSASCONSS             TRUE /**< should the cumulative constraint create the cuts as knapsack constraints? */


/*
 * Data structures
 */

/** constraint data for cumulative constraints */
struct SCIP_ConsData
{
   SCIP_VAR**            vars;               /**< array of variable representing the start time of each job */
   
   SCIP_CONS**           linkingconss;       /**< array of linking constraints for the integer variables */
   SCIP_ROW**            demandrows;         /**< array of rows of linear relaxation of this problem */
   SCIP_ROW**            scoverrows;         /**< array of rows of small cover cuts of this problem */ 
   SCIP_ROW**            bcoverrows;         /**< array of rows of big cover cuts of this problem */ 
   int*                  demands;            /**< array containing corresponding demands */
   int*                  durations;          /**< array containing corresponding durations */
   int                   nvars;              /**< number of variables */
   int                   ndemandrows;        /**< number of rows of cumulative constrint for linear relaxation */
   int                   demandrowssize;     /**< size of array rows of demand rows */
   int                   nscoverrows;        /**< number of rows of small cover cuts */
   int                   scoverrowssize;     /**< size of array of small cover cuts */  
   int                   nbcoverrows;        /**< number of rows of big cover cuts */
   int                   bcoverrowssize;     /**< size of array of big cover cuts */  
   int                   capacity;           /**< available cumulative capacity */
   unsigned int          covercuts:1;        /**< cover cuts are created? */
};

/** constraint handler data */
struct SCIP_ConshdlrData
{
   SCIP_Bool             usebinvars;         /**< should the binary variables be used? */
   SCIP_Bool             cutsasconss;        /**< should the cumulative constraint create the cuts as knapsack constraints? */
   SCIP_Bool             usecoretimes;       /**< should core-times be propagated? */
   SCIP_Bool             usecoretimesholes;  /**< should core-times be propagated to detect holes? */
   SCIP_Bool             useedgefinding;     /**< should edge finding be used? */
   SCIP_Bool             useenergeticreasoning;/**< should energeticreasoning be used? */
   SCIP_Bool             localcuts;          /**< should cuts be added only locally? */
   SCIP_Bool             usecovercuts;          /**< should covering cuts be added? */

   SCIP_Longint          lastsepanode;       /**< last node in which separation took place */
};


/*
 * local structure for INFERINFO
 */

/*
 * Propagation rules
 */
enum Proprule
{
   PROPRULE_1_CORETIMES          = 1,        /**< core-time propagator */
   PROPRULE_2_CORETIMEHOLES      = 2,        /**< core-time propagator for holes */
   PROPRULE_3_EDGEFINDING        = 3,        /**< edge-finder */
   PROPRULE_4_ENERGETICREASONING = 4,        /**< energetic reasoning */
   PROPRULE_INVALID              = 0         /**< propagation was applied without a specific propagation rule */
};
typedef enum Proprule PROPRULE;

/** inference information */
struct InferInfo
{
   union
   {
      struct
      {
         unsigned int    proprule:4;         /**< propagation rule that was applied */
         unsigned int    est:13;             /**< earliest start time of all jobs in conflict set */
         unsigned int    lct:15;             /**< latest completion time of all jobs in conflict set */
      } asbits;
      int                asint;              /**< inference information as a single int value */
   } val;
};
typedef struct InferInfo INFERINFO;

/** converts an integer into an inference information */
static
INFERINFO intToInferInfo(
   int                   i                   /**< integer to convert */
   )
{
   INFERINFO inferinfo;

   inferinfo.val.asint = i;

   return inferinfo;
}

/** converts an inference information into an int */
static
int inferInfoToInt(
   INFERINFO             inferinfo           /**< inference information to convert */
   )
{
   return inferinfo.val.asint;
}

/** returns the propagation rule stored in the inference information */
static
PROPRULE inferInfoGetProprule(
   INFERINFO             inferinfo           /**< inference information to convert */
   )
{
   return (PROPRULE) inferinfo.val.asbits.proprule;
}

/** returns the earliest start time stored in the inference information */
static
int inferInfoGetEst(
   INFERINFO             inferinfo           /**< inference information to convert */
   )
{
   return inferinfo.val.asbits.est;
}

/** returns the latest completion time stored in the inference information */
static
int inferInfoGetLct(
   INFERINFO             inferinfo           /**< inference information to convert */
   )
{
   return inferinfo.val.asbits.lct;
}


/** constructs an inference information out of a propagation rule, an earliest start and a latest completion time */
static
INFERINFO getInferInfo(
   PROPRULE              proprule,           /**< propagation rule that deduced the value */
   int                   est,                /**< earliest start time of all jobs in conflict set */
   int                   lct                 /**< latest completion time of all jobs in conflict set */
   )
{
   INFERINFO inferinfo;

   inferinfo.val.asbits.proprule = proprule; /*lint !e641*/
   inferinfo.val.asbits.est = est; /*lint !e732*/
   inferinfo.val.asbits.lct = lct; /*lint !e732*/

   return inferinfo;
}

/*
 * local structure for THETATREE
 */

/** Theta tree node structure */
typedef struct ThetaTreeNode THETATREENODE;
struct ThetaTreeNode
{
   THETATREENODE*        parent;             /**< pointer to the parent node */
   THETATREENODE*        left;               /**< pointer to the left child node */
   THETATREENODE*        right;              /**< pointer to the right child node */
   SCIP_Real             value;              /**< value according to which the tree is ordered */
   SCIP_VAR*             var;                /**< pointer to the variable if node is a leaf or NULL */
   int                   energy;             /**< sum of energies from the leaves in this subtree */
   int                   envelop;            /**< envelop of this subtree */
};

/** Theta tree structure */
struct ThetaTree
{
   THETATREENODE*        superroot;          /**< pointer to the dummy super root node; root is left child */
};
typedef struct ThetaTree THETATREE;

/** returns whether the node is a leaf */
static
SCIP_Bool thetatreeIsLeaf(
   THETATREENODE*        node                /**< node to be evaluated */
   )
{
   assert(node != NULL);
   assert(node->parent != NULL);
   return node->left == NULL && node->right == NULL;
}

/** returns whether the tree is empty */
static
SCIP_Bool thetatreeIsEmpty(
   THETATREE*            tree                /**< tree to be evaluated */
   )
{
   assert(tree != NULL);
   assert(tree->superroot != NULL);

   return tree->superroot->left == NULL;
}

/** returns whether the node is a left child */
static
SCIP_Bool thetatreeIsLeftChild(
   THETATREENODE*        node                /**< node to be evaluated */
   )
{
   assert(node != NULL);
   assert(node->parent != NULL);
   return node->parent->left == node;
}

/** creates an empty theta tree node */
static
SCIP_RETCODE createThetaTreeNode(
   SCIP*                 scip,               /**< SCIP data structure */
   THETATREENODE**       node                /**< node to be created */
   )
{
   SCIP_CALL( SCIPallocMemory(scip, node) );

   (*node)->parent = NULL;
   (*node)->left = NULL;
   (*node)->right = NULL;
   (*node)->value = 0.0;
   (*node)->var = NULL;
   (*node)->energy = 0;
   (*node)->envelop = 0;
   
   return SCIP_OKAY;
}

/** returns the closest leaf to the given node or NULL if tree is empty */
static
THETATREENODE* findLeafNode(
   THETATREE*            tree,               /**< tree in which the node is searched */
   THETATREENODE*        node                /**< node to be searched for */
   )
{
   THETATREENODE* tmpnode;

   assert(tree != NULL);
   assert(node != NULL);

   if( thetatreeIsEmpty(tree) )
      return NULL;


   tmpnode = tree->superroot->left;
   
   while( !thetatreeIsLeaf(tmpnode) )
   {
      if( node->value <= tmpnode->value )
         tmpnode = tmpnode->left;
      else
         tmpnode = tmpnode->right;
   }
   
   return tmpnode;
}

/** updates the envelop and energy on trace */
static
void updateEnvelop(
   THETATREE*            tree,               /**< tree data structure */
   THETATREENODE*        node                /**< node to be updated and its parents */
   )
{
   while( node != tree->superroot )
   {
      assert(node != NULL);
      assert(node->left != NULL);
      assert(node->right != NULL);

      /* update envelop and energy */
      node->envelop = MAX( node->left->envelop + node->right->energy, node->right->envelop);
      node->energy = node->left->energy + node->right->energy;

      /* go to parent */
      node = node->parent;
   }
}


/* inserts the given node into the tree if it is not already inserted */
static
SCIP_RETCODE splitThetaTreeLeaf(
   SCIP*                 scip,               /**< SCIP data structure */
   THETATREE*            tree,               /**< tree data structure */
   THETATREENODE*        splitnode,          /**< node to be splitted */
   THETATREENODE*        node                /**< node to be inserted */
   )
{
   THETATREENODE* newnode;

   assert(scip != NULL);
   assert(node != NULL);
   
   /* create a new node as parent of the given ones */
   SCIP_CALL( createThetaTreeNode(scip, &newnode) );
   assert(newnode != NULL);

   newnode->parent = splitnode->parent;
   
   if( thetatreeIsLeftChild(splitnode) )
   {
      newnode->parent->left = newnode;
   }
   else 
   {
      newnode->parent->right = newnode;
   }

   if( node->value < splitnode->value )
   {
      /* node is on the left */
      newnode->left = node;
      newnode->right = splitnode;
      newnode->value = node->value;
   }
   else
   {
      /* split node is on the left */
      newnode->left = splitnode;
      newnode->right = node;
      newnode->value = splitnode->value;
   }
   
   splitnode->parent = newnode;
   node->parent = newnode;

   updateEnvelop(tree, newnode);

   return SCIP_OKAY;
}

/** creates a theta tree node wth variable and sorting value */
static
SCIP_RETCODE thetatreeCreateLeaf(
   SCIP*                 scip,               /**< SCIP data structure */
   THETATREENODE**       node,               /**< node to be created */
   SCIP_VAR*             var,                /**< variable to be stored */
   SCIP_Real             value,              /**< value to be stored */
   int                   energy,             /**< sum of energies from the leaves in this subtree */
   int                   envelop             /**< envelop of this subtree */
   )
{
   assert(var != NULL);
   
   SCIP_CALL( SCIPallocMemory(scip, node) );

   (*node)->parent = NULL;
   (*node)->left = NULL;
   (*node)->right = NULL;
   (*node)->value = value;
   (*node)->var = var;
   (*node)->energy = energy;
   (*node)->envelop = envelop;

   return SCIP_OKAY;
}


/** creates an empty theta tree */
static
SCIP_RETCODE createThetaTree(
   SCIP*                 scip,               /**< SCIP data structure */
   THETATREE**           tree                /**< tree to be created */
   )
{
   THETATREENODE* node;

   assert(scip != NULL);
   assert(tree != NULL);


   SCIP_CALL( SCIPallocMemory(scip, tree) );

   SCIP_CALL( createThetaTreeNode(scip, &node) );

   (*tree)->superroot = node;

   return SCIP_OKAY; 
}

/** frees the theta tree node datastructure */
static
SCIP_RETCODE freeThetaTreeNode(
   SCIP*                 scip,               /**< SCIP data structure */
   THETATREENODE**       node                /**< node to be freed */
   )
{
   assert(scip != NULL);
   assert(node != NULL);
   assert(*node != NULL);

   if( (*node)->left != NULL || (*node)->right != NULL )
   {
      
      if( (*node)->left != NULL )
      {
         SCIP_CALL( freeThetaTreeNode(scip, &((*node)->left) ) );
      }
      
      if( (*node)->right != NULL )
      {
         SCIP_CALL( freeThetaTreeNode(scip, &((*node)->right) ) );
      }
      
      
      (*node)->left = NULL;
      (*node)->right = NULL;
      (*node)->parent = NULL;
      (*node)->var = NULL;
      SCIPfreeMemory(scip, node);
   }

   return SCIP_OKAY;
}

/** frees the theta tree node datastructure */
static
SCIP_RETCODE freeThetaTreeLeaf(
   SCIP*                 scip,               /**< SCIP data structure */
   THETATREENODE**       node                /**< node to be freed */
   )
{
   assert(scip != NULL);
   assert(node != NULL);
   assert(*node != NULL);

   assert((*node)->left == NULL);
   assert((*node)->right == NULL);
   
   (*node)->var = NULL;

   SCIPfreeMemory(scip, node);

   return SCIP_OKAY;
}

/** frees the theta tree datastructure */
static
SCIP_RETCODE freeThetaTree(
   SCIP*                 scip,               /**< SCIP data structure */
   THETATREE**           tree                /**< tree to be freed */
   )
{
   assert(scip != NULL);
   assert(tree != NULL);

   if( (*tree)->superroot != NULL )
   {
      SCIP_CALL( freeThetaTreeNode(scip, &((*tree)->superroot) ) );
   }

   SCIPfreeMemory(scip, tree);

   return SCIP_OKAY;
}

/** inserts the given node into the tree if it is not already inserted */
static
SCIP_RETCODE thetatreeInsertLeaf(
   SCIP*                 scip,               /**< SCIP data structure */
   THETATREE*            tree,               /**< tree in which the node is inserted */
   THETATREENODE*        node,               /**< node to be inserted */
   SCIP_Bool*            inserted            /**< pointer to store whether the node could be inserted */
   )
{
   assert(scip != NULL);
   assert(tree != NULL);
   assert(node != NULL);
   assert(inserted != NULL);
   
   *inserted = FALSE;

   /* if the tree is empty the node will be the root node */
   if( thetatreeIsEmpty(tree) )
   {
      tree->superroot->left = node;
      node->parent = tree->superroot;
      *inserted = TRUE;
      return SCIP_OKAY;
   }
   else
   {
      THETATREENODE* splitleaf;
      
      /* otherwise find the position to insert the node! */
      splitleaf = findLeafNode(tree, node);
      
      /* node is already inserted */
      if( node == splitleaf )
         return SCIP_OKAY;
            
      /* split the 'splitnode' and insert 'node' */
      SCIP_CALL( splitThetaTreeLeaf(scip, tree, splitleaf, node) );
      *inserted = TRUE;
   }

   return SCIP_OKAY;
}

/** return the envelop of the theta tree: \f$max_{\Omega \subseteq \Theta} (C * est_{\Omega} + e_{\Omega})\f$ */
static
int thetaTreeGetEnvelop(
   THETATREE*            tree                /**< tree of which the envelop is returned */   
   )
{
   assert(tree != NULL);

   if( thetatreeIsEmpty(tree) )
      return 0;
   
   return tree->superroot->left->envelop;
}

/*
 * local structure for THETA LAMBDA TREE
 */

/** Theta Lambda tree node structure */
typedef struct TLTreeNode TLTREENODE;
struct TLTreeNode
{
   TLTREENODE*           parent;             /**< pointer to the parent node */
   TLTREENODE*           left;               /**< pointer to the left child node */
   TLTREENODE*           right;              /**< pointer to the right child node */
   SCIP_Real             value;              /**< value according to which the tree is ordered */
   SCIP_VAR*             var;                /**< pointer to the variable if node is a leaf or NULL */
   int                   energy;             /**< sum of energies from the theta-leaves in this subtree */
   int                   envelop;            /**< theta envelop of this subtree */
   int                   energyL;            /**< sum of energies from the lambda-leaves in this subtree */
   int                   envelopL;           /**< lambda envelop of this subtree */
   SCIP_Bool             inTheta;            /**< stores whether this node belongs to the set theta or to lambda */
};

/** Theta lambda tree structure */
struct TLTree
{
   TLTREENODE*           superroot;          /**< pointer to the dummy super root node; root is left child */
};
typedef struct TLTree TLTREE;

/** returns whether the node is a leaf */
static
SCIP_Bool tltreeIsLeaf(
   TLTREENODE*           node                /**< node to be evaluated */
   )
{
   assert(node != NULL);

   return node->left == NULL && node->right == NULL;
}

/** returns whether the node is root node */
static
SCIP_Bool tltreeIsRoot(
   TLTREE*               tree,               /**< tree to be evaluated */
   TLTREENODE*           node                /**< node to be evaluated */
   )
{
   assert(tree != NULL);
   assert(node != NULL);
   assert(tree->superroot != NULL);

   return tree->superroot->left == node;
}

/** returns whether the tree is empty */
static
SCIP_Bool tltreeIsEmpty(
   TLTREE*               tree                /**< tree to be evaluated */
   )
{
   assert(tree != NULL);
   assert(tree->superroot != NULL);

   return tree->superroot->left == NULL;
}

/** returns whether the node is a left child */
static
SCIP_Bool tltreeIsLeftChild(
   TLTREENODE*           node                /**< node to be evaluated */
   )
{
   assert(node != NULL);

   return node->parent->left == node;
}

/** returns whether the node is a right child */
static
SCIP_Bool tltreeIsRightChild(
   TLTREENODE*           node                /**< node to be evaluated */
   )
{
   assert(node != NULL);

   return node->parent->right == node;
}

/** returns the sibling of the node */
static
TLTREENODE* tltreeGetSibling(
   TLTREENODE*           node                /**< node to be evaluated */
   )
{
   assert(node != NULL);
   assert(node->parent != NULL);
   assert(node->parent->left != NULL);
   assert(node->parent->right != NULL);

   if( tltreeIsLeftChild(node) )
      return node->parent->right;

   return node->parent->left;
}

/** creates an empty tltree node */
static
SCIP_RETCODE tltreeCreateNode(
   SCIP*                 scip,               /**< SCIP data structure */
   TLTREENODE**          node                /**< node to be created */
   )
{
   SCIP_CALL( SCIPallocMemory(scip, node) );

   (*node)->parent = NULL;
   (*node)->left = NULL;
   (*node)->right = NULL;
   (*node)->value = 0.0;
   (*node)->var = NULL;
   (*node)->energy = 0;
   (*node)->envelop = 0;
   (*node)->energyL = 0;
   (*node)->envelopL = 0;
   (*node)->inTheta = TRUE;
   
   return SCIP_OKAY;
}

/** returns the closest leaf to the given node or NULL if tree is empty */
static
TLTREENODE* tltreeFindLeafNode(
   TLTREE*               tree,               /**< tree in which the node is searched */
   TLTREENODE*           node                /**< node to be searched for */
   )
{
   TLTREENODE* tmpnode;

   assert(tree != NULL);
   assert(node != NULL);

   if( tltreeIsEmpty(tree) )
      return NULL;
   
   tmpnode = tree->superroot->left;
   
   while( !tltreeIsLeaf(tmpnode) )
   {
      if( node->value <= tmpnode->value )
         tmpnode = tmpnode->left;
      else
         tmpnode = tmpnode->right;
   }
   
   return tmpnode;
}

/** updates the value of the first parent on the trace which comes from left  */
static
void tltreeUpdateValuesOnTrace(
   TLTREE*               tree,               /**< tree data structure */
   TLTREENODE*           node,               /**< node to be updated or one of its parents */
   SCIP_Real             value               /**< value to be set */
   )
{
   assert(node != NULL);

   while( !tltreeIsRoot(tree, node) )
   {
      if( tltreeIsLeftChild(node) )
      {
         SCIPdebugMessage("update on a trace from %g to %g", node->parent->value, value);
         node->parent->value = value; 
         return;
      }
      node = node->parent;
   }
}

/** updates the envelop and energy on trace */
static
void tltreeUpdateEnvelop(
   TLTREE*               tree,               /**< tree data structure */
   TLTREENODE*           node                /**< node to be updated and its parents */
   )
{
   while( node != NULL && node != tree->superroot )
   {
      assert(node != NULL);
      assert(node->left != NULL);
      assert(node->right != NULL);

      /* update envelop and energy */
      node->envelop = MAX( node->left->envelop + node->right->energy, node->right->envelop);
      node->energy = node->left->energy + node->right->energy;

      node->envelopL = MAX( node->left->envelopL + node->right->energy, node->right->envelopL );
      node->envelopL = MAX( node->envelopL , node->left->envelop + node->right->energyL );

      node->energyL = MAX( node->left->energyL + node->right->energy, node->left->energy + node->right->energyL );

      /* negativ values are integer min value */
      if( node->envelop < 0 )
         node->envelop = INT_MIN;
      if( node->envelopL < 0 )
         node->envelopL = INT_MIN;
      if( node->energyL < 0 )
         node->energyL = INT_MIN;
      if( node->energy < 0 )
         node->energy = INT_MIN;

      /* go to parent */
      node = node->parent;
   }
}

/** inserts the given node into the tree if it is not already inserted */
static
SCIP_RETCODE tltreeSplitLeaf(
   SCIP*                 scip,               /**< SCIP data structure */
   TLTREE*               tree,               /**< tree data structure */
   TLTREENODE*           splitnode,          /**< node to be splitted */
   TLTREENODE*           node                /**< node to be inserted */
   )
{
   TLTREENODE* newnode;

   assert(scip != NULL);
   assert(node != NULL);
   
   /* create a new node as parent of the given ones */
   SCIP_CALL( tltreeCreateNode(scip, &newnode) );
   assert(newnode != NULL);

   newnode->parent = splitnode->parent;
   
   if( tltreeIsLeftChild(splitnode) )
   {
      newnode->parent->left = newnode;
   }
   else
   {
      newnode->parent->right = newnode;
   }

   if( node->value <= splitnode->value )
   {
      /* node is on the left */
      newnode->left = node;
      newnode->right = splitnode;
      newnode->value = node->value;
   }
   else
   {
      /* split node is on the left */
      newnode->left = splitnode;
      newnode->right = node;
      newnode->value = splitnode->value;
   }
   
   splitnode->parent = newnode;
   node->parent = newnode;

   tltreeUpdateEnvelop(tree, newnode);

   return SCIP_OKAY;
}

/** creates a theta tree node with variable in theta */
static
SCIP_RETCODE tltreeCreateThetaLeaf(
   SCIP*                 scip,               /**< SCIP data structure */
   TLTREENODE**          node,               /**< node to be created */
   SCIP_VAR*             var,                /**< variable to be stored */
   SCIP_Real             value,              /**< value to be stored */
   int                   energy,             /**< sum of energies from the leaves in this subtree */
   int                   envelop             /**< envelop of this subtree */
   )
{
   assert(var != NULL);
   
   SCIP_CALL( SCIPallocMemory(scip, node) );

   (*node)->parent = NULL;
   (*node)->left = NULL;
   (*node)->right = NULL;
   (*node)->value = value;
   (*node)->var = var;
   (*node)->energy = energy;
   (*node)->envelop = envelop;
   (*node)->energy = energy;
   (*node)->envelop = envelop;
   (*node)->energyL = INT_MIN;
   (*node)->envelopL = INT_MIN;
   (*node)->inTheta = TRUE;

   return SCIP_OKAY;
}

/** creates an empty theta tree */
static
SCIP_RETCODE createTltree(
   SCIP*                 scip,               /**< SCIP data structure */
   TLTREE**              tree                /**< tree to be created */
   )
{
   TLTREENODE* node;

   assert(scip != NULL);
   assert(tree != NULL);


   SCIP_CALL( SCIPallocMemory(scip, tree) );

   SCIP_CALL( tltreeCreateNode(scip, &node) );

   (*tree)->superroot = node;

   return SCIP_OKAY; 
}

/** inserts the given node into the tree if it is not already inserted */
static
SCIP_RETCODE tltreeInsertLeaf(
   SCIP*                 scip,               /**< SCIP data structure */
   TLTREE*               tree,               /**< tree in which the node is inserted */
   TLTREENODE*           node,               /**< node to be inserted */
   SCIP_Bool*            inserted            /**< pointer to store whether the node could be inserted */
   )
{
   assert(scip != NULL);
   assert(tree != NULL);
   assert(node != NULL);
   assert(inserted != NULL);
   
   *inserted = FALSE;

   /* if the tree is empty the node will be the root node */
   if( tltreeIsEmpty(tree) )
   {
      tree->superroot->left = node;
      node->parent = tree->superroot;
      *inserted = TRUE;
      return SCIP_OKAY;
   }
   else
   {
      TLTREENODE* splitleaf;
      
      /* otherwise find the position to insert the node! */
      splitleaf = tltreeFindLeafNode(tree, node);
      assert(tltreeIsLeaf(splitleaf));
      assert(node != splitleaf);

      /* node is already inserted */
      if( node == splitleaf )
      {
         return SCIP_OKAY;
      }
      
      /* split the 'splitnode' and insert 'node' */
      SCIP_CALL( tltreeSplitLeaf(scip, tree, splitleaf, node) );
      *inserted = TRUE;
   }

   return SCIP_OKAY;
}

/** creates a full theta lambda tree */
static
SCIP_RETCODE tltreeCreateTree(
   SCIP*                 scip,               /**< SCIP data structure */
   TLTREE**              tree,               /**< pointer to the tree to be created */
   TLTREENODE**          nodes,              /**< leaf nodes to be inserted */
   int*                  perm,               /**< permutation of the nodes to be used */
   int                   nvars               /**< number of leaves */
   )
{
   int j;

   assert(scip != NULL);
   assert(tree != NULL);
   assert(nodes != NULL);
   assert(perm != NULL);
   
   /* create an empty tree */
   SCIP_CALL( createTltree(scip, tree) );

   for( j = 0; j < nvars; ++j )
   {
      SCIP_Bool inserted;

      SCIP_CALL( tltreeInsertLeaf(scip, *tree, nodes[j], &inserted) );
      assert(inserted);
   }
 
   return SCIP_OKAY;
}

/** frees the theta lambda tree node datastructure, all leaves have to be freed on their own */
static
SCIP_RETCODE freeTltreeNode(
   SCIP*                 scip,               /**< SCIP data structure */
   TLTREENODE**          node                /**< node to be freed */
   )
{
   assert(scip != NULL);
   assert(node != NULL);

   if( tltreeIsLeaf(*node) )
      return SCIP_OKAY;

   if( (*node)->left != NULL || (*node)->right != NULL )
   {
      if( (*node)->left != NULL )
      {
         SCIP_CALL( freeTltreeNode(scip, &((*node)->left) ) );
      }
      
      if( (*node)->right != NULL )
      {
         SCIP_CALL( freeTltreeNode(scip, &((*node)->right) ) );
      }
      
      (*node)->left = NULL;
      (*node)->right = NULL;
      (*node)->parent = NULL;
      (*node)->var = NULL;
      
      SCIPfreeMemory(scip, node);
   }

   return SCIP_OKAY;
}

/** frees the theta lambda tree leaf */
static
SCIP_RETCODE freeTltreeLeaf(
   SCIP*                 scip,               /**< SCIP data structure */
   TLTREENODE**          node                /**< node to be freed */
   )
{
   assert(scip != NULL);
   assert(node != NULL);
   
   (*node)->left = NULL;
   (*node)->right = NULL;
   (*node)->parent = NULL;
   (*node)->var = NULL;

   SCIPfreeMemory(scip, node);

   return SCIP_OKAY;
}

/** frees the theta tree datastructure, BUT: all leaves have to be freed on their own */
static
SCIP_RETCODE freeTltree(
   SCIP*                 scip,               /**< SCIP data structure */
   TLTREE**              tree                /**< tree to be freed */
   )
{
   assert(scip != NULL);
   assert(tree != NULL);

   if( (*tree)->superroot != NULL )
   {
      SCIP_CALL( freeTltreeNode(scip, &((*tree)->superroot) ) );
   }

   SCIPfreeMemory(scip, tree);

   return SCIP_OKAY;
}

/** deletes the given node */
static
SCIP_RETCODE tltreeDeleteLeaf(
   SCIP*                 scip,               /**< SCIP data structure */
   TLTREE*               tree,               /**< tree in which the node is deleted */
   TLTREENODE*           node                /**< node to be deleted */
   )
{

   TLTREENODE* sibling;
   TLTREENODE* parent;
   TLTREENODE* grandparent;

   assert(scip != NULL);
   assert(tree != NULL);
   assert(node != NULL);
   
   assert(tltreeIsLeaf(node));

   if( tltreeIsRoot(tree, node) )
   {
      node->parent = NULL;
      tree->superroot->left = NULL;
   }

   /* the node belongs to a real subtree */
   sibling = tltreeGetSibling(node);
   assert(sibling != NULL);
   
   parent = node->parent;
   assert(parent != NULL);

   grandparent = parent->parent;
   assert(grandparent != NULL);
      
   /* reset parent of sibling */
   sibling->parent = grandparent;

   /* reset child of grandparent to sibling */
   if( tltreeIsLeftChild(parent) )
   {
      grandparent->left = sibling;
   }
   else
   {
      grandparent->right = sibling;
      
      if( tltreeIsRightChild(parent) )
         tltreeUpdateValuesOnTrace(tree, grandparent, sibling->value);
   }
   tltreeUpdateEnvelop(tree, grandparent);

   SCIPfreeMemory(scip, &parent);

   return SCIP_OKAY;
}

/** return the envelop(theta,lambda) */
static
int tltreeGetEnvelopTL(
   TLTREE*               tree                /**< tree of which the envelop is returned */   
   )
{
   assert(tree != NULL);

   if( tltreeIsEmpty(tree) )
      return 0;
   
   return tree->superroot->left->envelopL;
}

/** transforms the leaf from a theta leaf into a lambda leave */
static
SCIP_RETCODE tltreeTransformLeafTtoL(
   SCIP*                 scip,               /**< SCIP data structure */
   TLTREE*               tree,               /**< tree in which the node is contained as leaf */
   TLTREENODE*           node                /**< node to be transformed */   
   )
{
   assert(scip != NULL);
   assert(tree != NULL);
   assert(node != NULL);
      
   node->envelopL = node->envelop;
   node->energyL = node->energy;

   node->envelop = INT_MIN;
   node->energy = 0;

   node->inTheta = FALSE;

   /* update the energy and envelop values on trace */
   tltreeUpdateEnvelop(tree, node->parent);

   return SCIP_OKAY;
}

/** returns the leaf responsible for the energyL */
static 
TLTREENODE* tltreeGetResponsibleLeafEnergyL(
   TLTREENODE*           node                /**< node where the search is continued */
   )
{
   assert(node != NULL);

   if( tltreeIsLeaf(node) )
   {
      assert(!node->inTheta);
      return node;
   }

   if( node->energyL == node->left->energyL + node->right->energy )
      return tltreeGetResponsibleLeafEnergyL(node->left);
   
   assert(node->energyL == node->left->energy + node->right->energyL);
   return tltreeGetResponsibleLeafEnergyL(node->right);
}

/** returns the leaf responsible for the envelopL */
static 
TLTREENODE* tltreeGetResponsibleLeafEnvelopL(
   TLTREENODE*           node                /**< node where the search is continued */
   )
{
   assert(node != NULL);

   if( tltreeIsLeaf(node) )
   {
      assert(!node->inTheta);
      return node;
   }

   if( node->envelopL == node->left->envelopL + node->right->energy )
   {
      return tltreeGetResponsibleLeafEnvelopL(node->left);
   } 
   else if( node->envelopL == node->left->envelop + node->right->energyL )
   {
      return tltreeGetResponsibleLeafEnergyL(node->right);
   } 

   assert(node->envelopL == node->right->envelopL);
   
   return tltreeGetResponsibleLeafEnvelopL(node->right);
}

/** returns the leaf responsible for the envelopL */
static
TLTREENODE* tltreeFindResponsibleLeaf(
   TLTREE*               tree                /**< tree to search for responsible leaf */
   )
{
   TLTREENODE* root;

   assert(tree != NULL);

   root = tree->superroot->left;
   assert(root != NULL);

   if( tltreeIsLeaf(root) )
      return NULL;

   return tltreeGetResponsibleLeafEnvelopL(root);
}

/** reports all elements from set theta to generate a conflicting set */
static
void reportSubtreeTheta(
   TLTREENODE*           node,               /**< node whose envelopL needs to be backtraced */
   TLTREENODE***         omegaset,           /**< set to be filled */
   int*                  nelements           /**< pointer to store the number of elements in omegaset */
   )
{
   if( !tltreeIsLeaf(node) )
   {
      reportSubtreeTheta(node->left, omegaset, nelements);
      reportSubtreeTheta(node->right, omegaset, nelements);
   }
   else
   {
      if( node->inTheta ) 
      {
         (*omegaset)[*nelements] = node;
         (*nelements)++;
      } 
   }
}
 
/** reports all elements from set theta to generate a conflicting set */
static
void reportEnvelop(
   TLTREENODE*           node,               /**< node whose envelopL needs to be backtraced */
   TLTREENODE***         omegaset,           /**< set to be filled */
   int*                  nelements           /**< pointer to store the number of elements in omegaset */
   )
{
   if( tltreeIsLeaf(node) )
   {
      reportSubtreeTheta(node, omegaset, nelements);
   }
   else if( node->envelop == node->left->envelop + node->right->energy )
   {
      reportEnvelop(node->left, omegaset, nelements);
      reportSubtreeTheta(node->right, omegaset, nelements);
   }
   else
   {
      assert(node->envelop == node->right->envelop);
      reportEnvelop(node->right, omegaset, nelements);
   }
}

/** reports all elements from set theta to generate a conflicting set */
static
void reportEnergyL(
   TLTREENODE*           node,               /**< node whose envelopL needs to be backtraced */
   TLTREENODE***         omegaset,           /**< set to be filled */
   int*                  nelements           /**< pointer to store the number of elements in omegaset */
   )
{
   if( tltreeIsLeaf(node) )
      return;
   
   if( node->energyL == node->left->energyL + node->right->energy )
   {
      reportEnergyL(node->left, omegaset, nelements);
      reportSubtreeTheta(node->right, omegaset, nelements);
   }
   else 
   { 
      assert(node->energyL == node->left->energy + node->right->energyL);
      
      reportSubtreeTheta(node->left, omegaset, nelements);
      reportEnergyL(node->right, omegaset, nelements);
   }
}

/** reports all elements from set theta to generate a conflicting set */
static
void reportEnvelopL(
   TLTREENODE*           node,               /**< node whose envelopL needs to be backtraced */
   TLTREENODE***         omegaset,           /**< set to be filled */
   int*                  nelements           /**< pointer to store the number of elements in omegaset */
   )
{
   
   /* in a leaf there is no lambda element! */
   if( tltreeIsLeaf(node) )
      return;
   
   if( node->envelopL == node->left->envelopL + node->right->energy )
   {
      reportEnvelopL(node->left, omegaset, nelements);
      reportSubtreeTheta(node->right, omegaset, nelements);
      return;
   }
   else if( node->envelopL == node->left->envelop + node->right->energyL )
   {
      reportEnvelop(node->left, omegaset, nelements);
      reportEnergyL(node->right, omegaset, nelements);
      return;
   }
   else
   {
      assert(node->envelopL == node->right->envelopL);
      
      reportEnvelopL(node->right, omegaset, nelements);
   }   
}

/** finds an omega set that leads to a violation
 *  user should take care that this method is only called if the envelop(T,L) > C * lct_j 
 *  during edgefinding detection
 *  the array omegaset already needs to be allocated with enough space! 
 *  it will be filled with the jobs in non-decreasing order of est_j
 */
static
SCIP_RETCODE tltreeReportOmegaSet(
   SCIP*                 scip,               /**< SCIP data structure */
   TLTREE*               tree,               /**< tree in which the node is contained as leaf */
   TLTREENODE***         omegaset,           /**< set to be filled */
   int*                  nelements           /**< pointer to store the number of elements in omegaset */
   )
{
   assert(scip != NULL);
   assert(tree != NULL);
   assert(omegaset != NULL);
   assert(*omegaset != NULL);
   assert(nelements != NULL);

   *nelements = 0;

   assert(tree->superroot->left->envelopL > 0);

   reportEnvelopL(tree->superroot->left, omegaset, nelements);

   return SCIP_OKAY;
}

/*
 * Local methods
 */

#ifndef NDEBUG
/** converts the given double bound which is integral to an int; in optimized mode the function gets inlined for
 *  performance; in debug mode we check some additional conditions 
 */
static
int convertBoundToInt(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_Real             bound               /**< double bound to convert */
   )
{  
   assert(SCIPisIntegral(scip, bound));
   assert(SCIPisEQ(scip, bound, (SCIP_Real)(int)(bound + 0.5)));
      
   return (int)(bound + 0.5);
}
#else
#define convertBoundToInt(x, y) ((int)((y) + 0.5))
#endif

/** creates constaint handler data for cumulative constraint handler */
static
SCIP_RETCODE conshdlrdataCreate(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_CONSHDLRDATA**   conshdlrdata        /**< pointer to store the constraint handler data */
   )
{
   /* create precedence constraint handler data */
   assert(conshdlrdata != NULL);
   SCIP_CALL( SCIPallocMemory(scip, conshdlrdata) );
 
   return SCIP_OKAY;
}

/** frees constraint handler data for logic or constraint handler */
static
void conshdlrdataFree(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_CONSHDLRDATA**   conshdlrdata        /**< pointer to the constraint handler data */
   )
{
   assert(conshdlrdata != NULL);
   assert(*conshdlrdata != NULL);

   SCIPfreeMemory(scip, conshdlrdata);
}

/** prints cumulative constraint to file stream */
static
void consdataPrint(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_CONSDATA*        consdata,           /**< cumulative constraint data */
   FILE*                 file                /**< output file (or NULL for standard output) */
   )
{
   int v;

   assert(consdata != NULL);

   /* print coefficients */
   SCIPinfoMessage( scip, file, "cumulative(");

   for( v = 0; v < consdata->nvars; ++v )
   {
      assert(consdata->vars[v] != NULL);
      if( v > 0 )
         SCIPinfoMessage(scip, file, ", ");
      SCIPinfoMessage(scip, file, "<%s>(%d)[%d]", SCIPvarGetName(consdata->vars[v]), 
         consdata->durations[v], consdata->demands[v]);
   }
   SCIPinfoMessage(scip, file, ") <= %d", consdata->capacity);
}

/** creates constraint data of cumulative constraint */
static
SCIP_RETCODE consdataCreate(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_CONSDATA**       consdata,           /**< pointer to consdata */
   SCIP_VAR**            vars,               /**< array of integer variables */
   SCIP_CONS**           linkingconss,       /**< array of linking constraints for the integer variables, or NULL */
   int*                  durations,          /**< array containing corresponding durations */
   int*                  demands,            /**< array containing corresponding demands */
   int                   nvars,              /**< number of variables */
   int                   capacity            /**< available cumulative capacity */
   )
{
   int v;

   assert(scip != NULL);
   assert(consdata != NULL);
   assert(vars != NULL || nvars > 0);
   assert(demands != NULL);
   assert(durations != NULL);
   assert(capacity >= 0);

   /* create constraint data */
   SCIP_CALL( SCIPallocBlockMemory(scip, consdata) );

   (*consdata)->capacity = capacity;
   (*consdata)->demandrows = NULL;
   (*consdata)->demandrowssize = 0;
   (*consdata)->ndemandrows = 0;
   (*consdata)->scoverrows = NULL;
   (*consdata)->nscoverrows = 0;
   (*consdata)->scoverrowssize = 0; 
   (*consdata)->bcoverrows = NULL; 
   (*consdata)->nbcoverrows = 0;
   (*consdata)->bcoverrowssize = 0;
   (*consdata)->nvars = nvars;
   (*consdata)->covercuts = FALSE;
   
   if( nvars > 0 )
   {
      assert(vars != NULL); /* for flexlint */

      SCIP_CALL( SCIPduplicateBlockMemoryArray(scip, &(*consdata)->vars, vars, nvars) );
      SCIP_CALL( SCIPduplicateBlockMemoryArray(scip, &(*consdata)->demands, demands, nvars) );
      SCIP_CALL( SCIPduplicateBlockMemoryArray(scip, &(*consdata)->durations, durations, nvars) );
      (*consdata)->linkingconss = NULL;

      if( linkingconss != NULL )
      {
         SCIP_CALL( SCIPduplicateBlockMemoryArray(scip, &(*consdata)->linkingconss, linkingconss, nvars) );
      }
      else
      {
         /* collect linking constraints for each integer variable */
         SCIP_CALL( SCIPallocBlockMemoryArray(scip, &(*consdata)->linkingconss, nvars) );
         for( v = 0; v < nvars; ++v )
         {
            SCIP_CONS* cons;         
	    SCIP_VAR* var;

	    var = vars[v];
	    assert(var != NULL);
            
            SCIPdebugMessage("linking constraint (%d of %d) for variable <%s>\n", v+1, nvars, SCIPvarGetName(vars[v]));

            /* create linking constraint if it does not exist yet */
            if( !SCIPexistsConsLinking(scip, var) )
            {
               char name[SCIP_MAXSTRLEN];            
               
               (void)SCIPsnprintf(name, SCIP_MAXSTRLEN, "link(%s)", SCIPvarGetName(var));
               
               /** creates and captures an linking constraint */
               SCIP_CALL( SCIPcreateConsLinking(scip, &cons, name, var, NULL, 0, 0, 
                     TRUE, TRUE, TRUE, TRUE, TRUE, FALSE, FALSE, FALSE, FALSE /*TRUE*/, FALSE) );
               SCIP_CALL( SCIPaddCons(scip, cons) );
               (*consdata)->linkingconss[v] = cons;
               
               SCIP_CALL( SCIPreleaseCons(scip, &cons) );
            }
            else 
            {
               (*consdata)->linkingconss[v] = SCIPgetConsLinking(scip, var);
            }
            
            assert(SCIPexistsConsLinking(scip, var));
            assert((*consdata)->linkingconss[v] != NULL);
            assert(strcmp(SCIPconshdlrGetName(SCIPconsGetHdlr((*consdata)->linkingconss[v])), "linking") == 0 );
	    assert(SCIPgetConsLinking(scip, var) == (*consdata)->linkingconss[v]);
         }
      }

      /* transform variables, if they are not yet transformed */
      if( SCIPisTransformed(scip) )
      {
         SCIPdebugMessage("get tranformed variables and constraints\n");

         /* get transformed variables and do NOT captures these */
         SCIP_CALL( SCIPgetTransformedVars(scip, (*consdata)->nvars, (*consdata)->vars, (*consdata)->vars) );

         /* get transformed constraints and captures these */
         SCIP_CALL( SCIPtransformConss(scip, (*consdata)->nvars, (*consdata)->linkingconss, (*consdata)->linkingconss) );

	 for( v = 0; v < nvars; ++v )
            assert(SCIPgetConsLinking(scip, (*consdata)->vars[v]) == (*consdata)->linkingconss[v]);
      }
   }
   else
   {
      (*consdata)->vars = NULL;
      (*consdata)->demands = NULL;
      (*consdata)->durations = NULL;
      (*consdata)->linkingconss = NULL;
   }

   return SCIP_OKAY;
}

/** removes rounding locks for the given variable in the given cumulative constraint */
static
SCIP_RETCODE unlockRounding(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_CONS*            cons,               /**< linking constraint */
   SCIP_VAR*             var                 /**< variables  */
   )
{
   SCIP_CALL( SCIPunlockVarCons(scip, var, cons, TRUE, TRUE) );
   
   return SCIP_OKAY;
}

#ifdef PROFILE_DEBUG
/** output of the given profile */
static
void SCIPprofilePrintOut(
   CUMULATIVEPROFILE*    profile             /**< profile to output */
   )
{
   int t;
   	
   for (t=0; t <profile->ntimepoints; t++)
   {
      SCIPdebugMessage("tp[%d]: %d -> fc=%d\n", t, profile->timepoints[t], profile-> freecapacities[t]); 
   }
}
#endif

/** releases LP rows of constraint data and frees rows array */
static
SCIP_RETCODE consdataFreeRows(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_CONSDATA**       consdata            /**< constraint data */
   )
{
   int r;

   assert(consdata != NULL);
   assert(*consdata != NULL);

   for( r = 0; r < (*consdata)->ndemandrows; ++r )
   {
      assert((*consdata)->demandrows[r] != NULL);
      SCIP_CALL( SCIPreleaseRow(scip, &(*consdata)->demandrows[r]) );
   }
   
   SCIPfreeBlockMemoryArrayNull(scip, &(*consdata)->demandrows, (*consdata)->demandrowssize);

   (*consdata)->ndemandrows = 0;
   (*consdata)->demandrowssize = 0;

   /* free rows of cover cuts */
   for( r = 0; r < (*consdata)->nscoverrows; ++r ) 
   { 
      assert((*consdata)->scoverrows[r] != NULL); 
      SCIP_CALL( SCIPreleaseRow(scip, &(*consdata)->scoverrows[r]) ); 
   } 
    
   SCIPfreeBlockMemoryArrayNull(scip, &(*consdata)->scoverrows, (*consdata)->scoverrowssize); 
 
   (*consdata)->nscoverrows = 0; 
   (*consdata)->scoverrowssize = 0;
  
   for( r = 0; r < (*consdata)->nbcoverrows; ++r )  
   {  
      assert((*consdata)->bcoverrows[r] != NULL);  
      SCIP_CALL( SCIPreleaseRow(scip, &(*consdata)->bcoverrows[r]) );  
   }  
     
   SCIPfreeBlockMemoryArrayNull(scip, &(*consdata)->bcoverrows, (*consdata)->bcoverrowssize);  
  
   (*consdata)->nbcoverrows = 0;  
   (*consdata)->bcoverrowssize = 0;

   (*consdata)->covercuts = FALSE;
 
   return SCIP_OKAY;
}

/** frees a cumulative constraint data */
static
SCIP_RETCODE consdataFree(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_CONSDATA**       consdata            /**< pointer to linear constraint data */
   )
{
   int nvars;

   assert(consdata != NULL);
   assert(*consdata != NULL);

   nvars =  (*consdata)->nvars;
   
   if( nvars > 0 )
   {
      int v;

      if( SCIPisTransformed(scip) )
      {
         /* release the linking constraints */
         for( v = 0; v < nvars; ++v )
         {
            assert((*consdata)->linkingconss[v] != NULL );
            SCIP_CALL( SCIPreleaseCons(scip, &(*consdata)->linkingconss[v]) );
         }
      }

      /* release and free the rows */
      SCIP_CALL( consdataFreeRows(scip, consdata) );

      /* free arrays */
      SCIPfreeBlockMemoryArray(scip, &(*consdata)->linkingconss, nvars);
      SCIPfreeBlockMemoryArray(scip, &(*consdata)->durations, nvars);
      SCIPfreeBlockMemoryArray(scip, &(*consdata)->demands, nvars);
      SCIPfreeBlockMemoryArray(scip, &(*consdata)->vars, nvars);
   }

   /* free memory */
   SCIPfreeBlockMemory(scip, consdata);

   return SCIP_OKAY;
}

/** check if the given constraiviont is valid; checks each starting point of a job whether the remaining capacity is at
 *  least zero or not. If not (*violated) is set to TRUE
 */
static
SCIP_RETCODE checkCons(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_CONS*            cons,               /**< constraint to be checked */
   SCIP_SOL*             sol,                /**< primal solution, or NULL for current LP/pseudo solution */
   SCIP_Bool*            violated,           /**< pointer to store if the constraint is violated */
   SCIP_Bool             printreason         /**< should the reason for the violation be printed? */
   )
{
   SCIP_CONSDATA* consdata;

   int* startsolvalues; /* stores when each job is starting */
   int* endsolvalues;   /* stores when each job ends */
   int* startindices;         /* we will sort the startsolvalues, thus we need to know which index of a job it corresponds to */
   int* endindices;           /* we will sort the endsolvalues, thus we need to know which index of a job it corresponds to */

   int nvars;

   int freecapacity;
   int curtime;            /* point in time which we are just checking */
   int endindex;           /* index of endsolvalues with: endsolvalues[endindex] > curtime */

   int j;

   assert(scip != NULL);
   assert(cons != NULL);
   assert(violated != NULL);

   SCIPdebugMessage("check cumulative constraints <%s>\n", SCIPconsGetName(cons));

   consdata = SCIPconsGetData(cons);
   assert(consdata != NULL);

   nvars = consdata->nvars;

   /* check if for each point in time the cumulative capacity is enforced  */
   if( nvars == 0 )
      return SCIP_OKAY;

   assert(consdata->vars != NULL);

   /* compute time points where we have to check whether capacity constraint is infeasible or not */
   SCIP_CALL( SCIPallocBufferArray(scip, &startsolvalues, nvars) );
   SCIP_CALL( SCIPallocBufferArray(scip, &endsolvalues, nvars) );
   SCIP_CALL( SCIPallocBufferArray(scip, &startindices, nvars) );
   SCIP_CALL( SCIPallocBufferArray(scip, &endindices, nvars) );

   /* assign variables, start and endpoints to arrays */
   for ( j = 0; j < nvars; ++j )
   {
      /* the constraint of the cumulative constraint handler should be called after the integrality check */
      assert(SCIPisFeasIntegral(scip, SCIPgetSolVal(scip, sol, consdata->vars[j])));
      
      startsolvalues[j] = convertBoundToInt(scip, SCIPgetSolVal(scip, sol, consdata->vars[j]));
      startindices[j] = j;
      
      endsolvalues[j] = startsolvalues[j] + consdata->durations[j];
      endindices[j] = j;
   }

   /* sort the arrays not-decreasing according to start solution values and end solution values (and sort the
    * corresponding indices in the same way) */
   SCIPsortIntInt(startsolvalues, startindices, nvars);
   SCIPsortIntInt(endsolvalues, endindices, nvars);

#ifndef NDEBUG
   /* check if the arrays are sorted correctly */
   SCIPdebugMessage("Checking solution <%p> with starting times:\n", (void*)sol);
   SCIPdebugMessage("%i | ", startsolvalues[0]);
   for ( j = 1; j < nvars; ++j )
   {
      assert ( startsolvalues[j-1] <= startsolvalues[j] );
      SCIPdebugPrintf("%i | ", startsolvalues[j]);
   }
   SCIPdebugPrintf("\nand end times:\n%i | ",endsolvalues[0]);
   for ( j = 1; j < nvars; ++j )
   {
      assert ( endsolvalues[j-1] <= endsolvalues[j] );
      SCIPdebugPrintf("%i | ",endsolvalues[j]);
   }
   SCIPdebugPrintf("\n");
#endif

   endindex = 0;
   freecapacity = consdata->capacity;

   /* check each start point of a job whether the capacity is kept or not */
   for( j = 0; j < nvars; ++j )
   {
      curtime = startsolvalues[j];

      /* subtract all capacity needed up to this point */
      freecapacity -= consdata->demands[startindices[j]];
      while( j+1 < nvars && startsolvalues[j+1] == curtime )
      {
         j++;
         freecapacity -= consdata->demands[startindices[j]];
      }

      /* free all capacity usages of jobs that are no longer running */
      while( endindex < nvars && curtime >= endsolvalues[endindex] )
      {
         freecapacity += consdata->demands[endindices[endindex]];
         ++endindex;
      }
      assert(freecapacity <= consdata->capacity);

      /* check freecapacity to be smaller than zero */
      if( freecapacity < 0 )
      {
         SCIPdebugMessage("freecapacity = %3d \n", freecapacity);
         (*violated) = TRUE;

         if( printreason )
         {
            int i;

            SCIP_CALL( SCIPprintCons(scip, cons, NULL) );
            SCIPinfoMessage(scip, NULL, 
               "violation: at time point %d available capacity = %d, needed capacity = %d\n",
               curtime, consdata->capacity, consdata->capacity - freecapacity);
            
            for(i = 0; i < j; ++i )
            {
               if( startsolvalues[i] + consdata->durations[startindices[i]] > curtime )
               { 
                  SCIPinfoMessage(scip, NULL, "activity %s, start = %i, duration = %d, demand = %d \n",
                     SCIPvarGetName(consdata->vars[startindices[i]]), startsolvalues[i], consdata->durations[startindices[i]],
                     consdata->demands[startindices[i]]);
               }
            }
         }
         break;
      }
   }
   
   /* free all buffer arrays */
   SCIPfreeBufferArray(scip, &endindices);
   SCIPfreeBufferArray(scip, &startindices);
   SCIPfreeBufferArray(scip, &endsolvalues);
   SCIPfreeBufferArray(scip, &startsolvalues);
   
   return SCIP_OKAY;
}

/** checks if the constraint is redundant; that is if its capacity can never be exceeded; therefore we check with
 *  respect to the lower and upper bounds of the integer variables the maximum capacity usage for all event points
 */
static
SCIP_RETCODE consCheckRedundancy(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_CONS*            cons,               /**< cumulative constraint  */
   SCIP_Bool*            redundant           /**< pointer to store whether this constraint is redundant */
   )
{

   SCIP_CONSDATA* consdata;
   SCIP_VAR* var;
   int* starttimes;              /* stores when each job is starting */
   int* endtimes;                /* stores when each job ends */
   int* startindices;            /* we will sort the startsolvalues, thus we need to know wich index of a job it corresponds to */
   int* endindices;              /* we will sort the endsolvalues, thus we need to know wich index of a job it corresponds to */

   int freecapacity;             /* remaining capacity */
   int curtime;                  /* point in time which we are just checking */
   int endindex;                 /* index of endsolvalues with: endsolvalues[endindex] > curtime */

   int nvars;
   int j;

   assert(scip != NULL);
   assert(cons != NULL);
   assert(redundant != NULL);

   consdata = SCIPconsGetData(cons);
   assert(consdata != NULL);

   (*redundant) = TRUE; 
   nvars = consdata->nvars;
   
   /* if no activities are associated with this cumulative then this constraint is redundant */
   if( nvars == 0 )
      return SCIP_OKAY;

   assert(consdata->vars != NULL);
   
   SCIP_CALL( SCIPallocBufferArray(scip, &starttimes, nvars) );
   SCIP_CALL( SCIPallocBufferArray(scip, &endtimes, nvars) );
   SCIP_CALL( SCIPallocBufferArray(scip, &startindices, nvars) );
   SCIP_CALL( SCIPallocBufferArray(scip, &endindices, nvars) );

   /* assign variables, start and endpoints to arrays */
   for( j = 0; j < nvars; ++j )
   {
      var = consdata->vars[j];
      
      starttimes[j] = convertBoundToInt(scip, SCIPvarGetLbLocal(var));
      startindices[j] = j;

      endtimes[j] =  convertBoundToInt(scip, SCIPvarGetUbLocal(var)) + consdata->durations[j];
      endindices[j] = j;
   }

   /* sort the arrays not-decreasing according to startsolvalues and endsolvalues (and sort the indices in the same way) */
   SCIPsortIntInt(starttimes, startindices, nvars);
   SCIPsortIntInt(endtimes, endindices, nvars);
   
   endindex = 0;
   freecapacity = consdata->capacity;
   
   /* check each start point of a job whether the capacity is violated or not */
   for( j = 0; j < nvars; ++j )
   {
      curtime = starttimes[j];

      /* subtract all capacity needed up to this point */
      freecapacity -= consdata->demands[startindices[j]];
      while( j+1 < nvars && starttimes[j+1] == curtime )
      {
         ++j;
         freecapacity -= consdata->demands[startindices[j]];
      }

      /* free all capacity usages of jobs the are no longer running */
      while( endtimes[endindex] <= curtime )
      {
         freecapacity += consdata->demands[endindices[endindex]];
         ++endindex;
      }
      assert(freecapacity <= consdata->capacity);

      /* check freecapacity to be smaller than zero */
      if( freecapacity < 0 )
      {
         (*redundant) = FALSE;
         break;
      }
   }
   
   /* free all buffer arrays */
   SCIPfreeBufferArray(scip, &endindices);
   SCIPfreeBufferArray(scip, &startindices);
   SCIPfreeBufferArray(scip, &endtimes);
   SCIPfreeBufferArray(scip, &starttimes);
   
   return SCIP_OKAY;
}

/** this method reports all jobs that are running during the given time window (left and right bound) and that exceed
 *  the remaining capacity 
 */
static
SCIP_RETCODE analyzeConflictCoreTimesCumulative(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_CONS*            cons,               /**< constraint to analyzed */
   SCIP_VAR*             infervar,           /**< inference variable */
   int                   leftbound,          /**< left bound of the responsible time window */
   int                   rightbound,         /**< right bound of the responsible time window */
   int                   inferduration,      /**< duration of the inference variable */
   int                   inferdemand,        /**< demand of the inference variable */
   SCIP_BOUNDTYPE        boundtype,          /**< the type of the changed bound (lower or upper bound) */
   SCIP_BDCHGIDX*        bdchgidx,           /**< the index of the bound change, representing the point of time where the change took place */
   SCIP_Bool*            success             /**< pointer to store if the conflict was resolved */
   )
{
   SCIP_VAR** corevars;
   SCIP_CONSDATA* consdata;
   SCIP_VAR* var;
   int* startvalues;      /* stores when core of each job is starting */
   int* endvalues;        /* stores when core of each job ends */
   int* startindices;     /* we will sort the startvalues, thus we need to know wich index of a job it corresponds to */
   int* endindices;       /* we will sort the endvalues, thus we need to know wich index of a job it corresponds to */
   int* demands;
   
   int* conflictids;      /* array where we store job indices of running jobs that are probably in a conflict */
   int nconflictids;

   int nvars;
   int j; 
   int i; 

   int corelb;
   int coreub;
   int freecapacity;       /* remaining capacity */
   int curtime;            /* point in time which we are just checking */
   int endindex;           /* index of endsolvalues with: endsolvalues[endindex] > curtime */
      
   int ncores;
   
   assert(cons != NULL);
   assert(leftbound < rightbound);
   assert(success != NULL);
   assert(inferdemand > 0);

   /* process constraint */
   consdata = SCIPconsGetData(cons);
   assert(consdata != NULL);
   
   SCIPdebugMessage("analyze reason of '%s' bound change of variable <%s>(%d)[%d], bounds [%d,%d], cap = %d \n",
      boundtype == SCIP_BOUNDTYPE_LOWER ? "lower" : "upper", SCIPvarGetName(infervar), 
      inferduration, inferdemand, leftbound, rightbound, consdata->capacity );

   *success = FALSE;
   
   nvars = consdata->nvars;
   SCIP_CALL( SCIPallocBufferArray(scip, &corevars, nvars) );
   SCIP_CALL( SCIPallocBufferArray(scip, &startvalues, nvars) );
   SCIP_CALL( SCIPallocBufferArray(scip, &endvalues, nvars) );
   SCIP_CALL( SCIPallocBufferArray(scip, &startindices, nvars) );
   SCIP_CALL( SCIPallocBufferArray(scip, &endindices, nvars) );
   SCIP_CALL( SCIPallocBufferArray(scip, &demands, nvars) );

   ncores = 0;

   /* compute all cores of the variables which lay in the considered time window except the inference variable */
   for ( j = 0; j < nvars; ++j )
   {
      var = consdata->vars[j];
      assert(var != NULL);

      if( var == infervar )
         continue;
      
      /* compute cores of jobs; if core overlaps interval of inference variable add this job to the array */
      assert(SCIPisFeasEQ(scip, SCIPvarGetUbAtIndex(var, bdchgidx, TRUE), SCIPvarGetUbAtIndex(var, bdchgidx, FALSE)));
      assert(SCIPisIntegral(scip, SCIPvarGetUbAtIndex(var, bdchgidx, TRUE)));
      assert(SCIPisFeasEQ(scip, SCIPvarGetLbAtIndex(var, bdchgidx, TRUE), SCIPvarGetLbAtIndex(var, bdchgidx, FALSE)));
      assert(SCIPisIntegral(scip, SCIPvarGetLbAtIndex(var, bdchgidx, TRUE)));
      
      corelb = convertBoundToInt(scip, SCIPvarGetUbAtIndex(var, bdchgidx, TRUE));
      coreub = convertBoundToInt(scip, SCIPvarGetLbAtIndex(var, bdchgidx, TRUE)) + consdata->durations[j];
      
      if( corelb < coreub && leftbound < coreub && rightbound > corelb )
      {
         SCIPdebugMessage("core bounds(%d):%s [%d; %d] <%d>\n", 
            j, SCIPvarGetName(var), corelb, coreub, consdata->demands[j]);
         
         corevars[ncores] = var;
         startvalues[ncores] = corelb;
         endvalues[ncores] = coreub;
         demands[ncores] = consdata->demands[j];
         startindices[ncores] = ncores;
         endindices[ncores] = ncores;
         ncores++;
      }
   }
 
   /* sort the arrays not-decreasing according to startvalues and endvalues (and sort the indices in the same way) */
   SCIPsortIntInt(startvalues, startindices, ncores);
   SCIPsortIntInt(endvalues, endindices, ncores);
   
   nconflictids = 0;
   endindex = 0;
   curtime = 0;
   freecapacity = consdata->capacity - inferdemand;

   SCIP_CALL( SCIPallocBufferArray(scip, &conflictids, ncores) );
   
   SCIPdebugMessage("find conflict vars\n");

   /* check each start point of a job whether the capacity is respected  or not */
   for( j = 0; endindex < ncores; ++j )
   {
      /* subtract all capacity needed up to this point */
      if( j < ncores )
      {
         curtime = startvalues[j];
         freecapacity -= demands[startindices[j]];
         conflictids[nconflictids] = startindices[j];
         ++nconflictids;
      
         SCIPdebugMessage("   start of %d\n", startindices[j]);
         while( j+1 < ncores && startvalues[j+1] <= curtime )
         {
            ++j;
            SCIPdebugMessage("   start of %d\n", startindices[j]);
            freecapacity -= demands[startindices[j]];
            conflictids[nconflictids] = startindices[j];
            ++nconflictids;
         }
      }
      else
         curtime = endvalues[endindex];
      
      SCIPdebugMessage("   endindex=%d, nconflictids=%d\n", endindex, nconflictids);

      /* free all capacity usages of jobs the are no longer running */
      while( endindex < ncores && curtime >= endvalues[endindex] )
      {
         SCIPdebugMessage("   end of %d\n", endindices[endindex]);
         freecapacity += demands[endindices[endindex]];
         
         for( i = 0; i < nconflictids; ++i )
         {
            if( conflictids[i] == endindices[endindex]  )
            {
               conflictids[i] = conflictids[nconflictids-1];
               --nconflictids;
               break;
            }
         }
         ++endindex;
      }
      assert(nconflictids >= 0);
      
      SCIPdebugMessage("   nconflictids=%d\n", nconflictids);
      SCIPdebugMessage("freecap = %d\n",freecapacity);
      
      /* check freecapacity to be smaller than zero */
      if( freecapacity < 0 )
      {
         SCIPdebugMessage("freecap = %d\n",freecapacity);
       
         /* figure out running jobs that are responsible and have to be reported to conflict analysis */
         for( i = 0; i < nconflictids; ++i )
         {
            assert(conflictids[i] < nvars);
            assert(corevars[conflictids[i]] != NULL);
            SCIPdebugMessage("report <%s> with demand %d\n", 
               SCIPvarGetName(corevars[conflictids[i]]), demands[conflictids[i]]);

            SCIP_CALL( SCIPaddConflictUb( scip, corevars[conflictids[i]], bdchgidx) );
            SCIP_CALL( SCIPaddConflictLb( scip, corevars[conflictids[i]], bdchgidx) );
            
            *success = TRUE;
         }
         nconflictids = 0;         
      }
   } /* end for each job j */

   assert(*success);

   /* free all buffer arrays */
   SCIPfreeBufferArray(scip, &conflictids);
   SCIPfreeBufferArray(scip, &demands);
   SCIPfreeBufferArray(scip, &endindices);
   SCIPfreeBufferArray(scip, &startindices);
   SCIPfreeBufferArray(scip, &endvalues);
   SCIPfreeBufferArray(scip, &startvalues);
   SCIPfreeBufferArray(scip, &corevars);

   return SCIP_OKAY;
}

/** initialize conflict analysis and analyze conflict */
static
SCIP_RETCODE initializeConflictAnalysisCoreTimes(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_CONS*            cons,               /**< constraint to analyzed */
   SCIP_VAR*             var,                /**< inference variable */
   int                   leftbound,          /**< left bound of the responsible time window */
   int                   rightbound,         /**< right bound of the responsible time window */
   int                   duration,           /**< duration of the inference variable */
   int                   demand,             /**< demand of the inference variable */
   SCIP_BOUNDTYPE        boundtype          /**< the type of the changed bound (lower or upper bound) */
   )
{
   SCIP_Bool success;

   assert(leftbound < rightbound);

   SCIPdebugMessage("initialize conflict analysis\n");

   if( SCIPgetStage(scip) != SCIP_STAGE_SOLVING )
      return SCIP_OKAY;
   
   SCIP_CALL( SCIPinitConflictAnalysis(scip) );

   /* add lower and upper bound of variable which leads to the infeasibilty */
   SCIP_CALL( SCIPaddConflictLb(scip, var, NULL ) );
   SCIP_CALL( SCIPaddConflictUb(scip, var, NULL ) );
 
   SCIPdebugMessage("add lower and upper bounds of variable <%s>\n", SCIPvarGetName(var));
   
   SCIP_CALL( analyzeConflictCoreTimesCumulative(scip, cons, var, leftbound, rightbound, duration, demand, boundtype, NULL, &success) );
   assert(success);

   SCIP_CALL( SCIPanalyzeConflictCons(scip, cons, NULL) );

   return SCIP_OKAY;
}

/** this method reports all jobs that are running at time 'timepoint' such that the capacity is exceeded
 *  remaining capacity 
 */
static
SCIP_RETCODE analyzeConflictCoreTimesBinvarsCumulative(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_CONS*            cons,               /**< constraint to analyzed */
   SCIP_VAR*             inferbinvar,        /**< inference variable */
   SCIP_VAR*             intvar,             /**< integer variable corresponding to binary variable */
   int                   timepoint,          /**< point in time, where capacity will be exceeded */    
   int                   inferdemand,        /**< demand of the inference variable */
   SCIP_BDCHGIDX*        bdchgidx,           /**< the index of the bound change, representing the point of time where the change took place */
   SCIP_Bool*            success             /**< pointer to store if the conflict was resolved */
   )
{
   SCIP_VAR** corevars;
   SCIP_CONSDATA* consdata;
   SCIP_VAR* var;
   int* indices;          /* we will sort the demands, thus we need to know wich index of a job it corresponds to */
   int* demands;
   
   int nvars;
   int j; 
   
   int corelb;
   int coreub;
   int freecapacity;       /* remaining capacity */
   int ncores;
   
   assert(cons != NULL );
   assert(success != NULL );
   assert(inferdemand > 0 );
   assert(SCIPvarGetType(inferbinvar) == SCIP_VARTYPE_BINARY);
   assert(SCIPvarGetType(intvar) == SCIP_VARTYPE_INTEGER);

   /* process constraint */
   consdata = SCIPconsGetData(cons);
   assert(consdata != NULL);
   
   SCIPdebugMessage("analyze reason of bound change of variable <%s>[%d], cap = %d because of capacity at time %d\n",
      SCIPvarGetName(inferbinvar), inferdemand, consdata->capacity, timepoint );

   *success = FALSE;
   
   nvars = consdata->nvars;
   SCIP_CALL( SCIPallocBufferArray(scip, &corevars, nvars) );
   SCIP_CALL( SCIPallocBufferArray(scip, &indices, nvars) );
   SCIP_CALL( SCIPallocBufferArray(scip, &demands, nvars) );

   ncores = 0;

   /* compute all cores of the variables which lay in the considered time window except the inference variable */
   for ( j = 0; j < nvars; ++j )
   {
      var = consdata->vars[j];
      assert(var != NULL);

      if( intvar == var )
         continue;
      
      /* compute cores of jobs; if core overlaps interval of
       * inference variable add this job to the array 
       */
      corelb = convertBoundToInt(scip, SCIPvarGetUbAtIndex(var, bdchgidx, TRUE));
      coreub = convertBoundToInt(scip, SCIPvarGetLbAtIndex(var, bdchgidx, TRUE)) + consdata->durations[j];
      
      if( corelb < coreub && timepoint < coreub && timepoint >= corelb )
      {
         SCIPdebugMessage("core bounds(%d):%s [%d; %d] <%d>\n", 
            j, SCIPvarGetName(var), corelb, coreub, consdata->demands[j]);
         
         corevars[ncores] = var;
         demands[ncores] = consdata->demands[j];
         indices[ncores] = ncores;
         ncores++;
      }
   }
 
   /* sort the arrays not-decreasing according to startvalues and endvalues (and sort the indices in the same way) */
   SCIPsortDownIntInt(demands, indices, ncores);
   
   freecapacity = consdata->capacity - inferdemand;
   
   /* check each start point of a job whether the capacity is respected  or not */
   for( j = 0; (j < ncores) && (freecapacity > 0); ++j )
   {
      freecapacity -= demands[j];
      
      /* add both bounds of variables with a core at 'timepoint' */
      SCIP_CALL( SCIPaddConflictUb( scip, corevars[indices[j]], bdchgidx) );
      SCIP_CALL( SCIPaddConflictLb( scip, corevars[indices[j]], bdchgidx) );
            
      *success = TRUE;      
   }

   assert(*success);

   /* free all buffer arrays */
   SCIPfreeBufferArray(scip, &demands);
   SCIPfreeBufferArray(scip, &indices);
   SCIPfreeBufferArray(scip, &corevars);

   return SCIP_OKAY;
}

/** initialize conflict analysis and analyze conflict */
static
SCIP_RETCODE initializeConflictAnalysisCoreTimesBinvars(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_CONS*            cons,               /**< constraint to analyzed */
   SCIP_VAR*             binvar,             /**< binary inference variable */
   SCIP_VAR*             intvar,             /**< corresponding starttime variable */
   int                   timepoint,          /**< point in time, where capacity will be exceeded */
   int                   demand              /**< demand of the inference variable */
   )
{
   SCIP_Bool success;

   SCIPdebugMessage("initialize conflict analysis\n");

   if( SCIPgetStage(scip) != SCIP_STAGE_SOLVING )
      return SCIP_OKAY;
   
   SCIP_CALL( SCIPinitConflictAnalysis(scip) );

   /* integer variable is not responsible with its bounds! */
   SCIPdebugMessage("add lower and upper bounds of variable <%s>\n", SCIPvarGetName(binvar));
   
   SCIP_CALL( analyzeConflictCoreTimesBinvarsCumulative(scip, cons, binvar, intvar, timepoint, demand, NULL, &success) );
   assert(success);

   SCIP_CALL( SCIPanalyzeConflictCons(scip, cons, NULL) );

   return SCIP_OKAY;
}

/** updates the bounds by avoiding core infeasibilty */
static
SCIP_RETCODE updateBounds(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_CONS*            cons,               /**< cumulative constraint that is propagated */ 
   CUMULATIVEPROFILE*    profile,            /**< profile to use */
   SCIP_VAR*             var,                /**< the variable the bounds should be updated */
   int                   duration,           /**< the duration of the given variable */
   int                   demand,             /**< the demand of the given variable */
   SCIP_Bool*            cutoff,             /**< pointer to store if the node is infeasible */
   int*                  nbdchgs             /**< pointer to store the number of bound changes */
   )
{
   SCIP_Bool infeasible; 
   SCIP_Bool tightened;

   INFERINFO inferinfo;

   int lb;
   int ub;
   int newlb;
   int newub;

   lb = convertBoundToInt(scip, SCIPvarGetLbLocal(var));
   ub = convertBoundToInt(scip, SCIPvarGetUbLocal(var));
   assert(lb <= ub); 

   /* try to improve the lower bound */
   newlb = SCIPprofileGetEarliestFeasibleStart(profile, lb, ub, duration, demand, &infeasible);
   assert(newlb <= ub || infeasible);

   if( infeasible )
   {
      SCIPdebugMessage("infeasibility detected during change of lower bound of <%s> from %d to %d\n", SCIPvarGetName(var), lb, newlb);
      
      /* initialize conflict analysis */
      SCIP_CALL( initializeConflictAnalysisCoreTimes(scip, cons, var, lb, ub+duration, duration, demand, SCIP_BOUNDTYPE_LOWER) );
      *cutoff = TRUE;
      return SCIP_OKAY;
   }
   
   assert(newlb <= ub);
   inferinfo = getInferInfo(PROPRULE_1_CORETIMES, 0, 0);

   SCIP_CALL( SCIPinferVarLbCons(scip, var, (SCIP_Real)newlb, cons, inferInfoToInt(inferinfo), TRUE, &infeasible, &tightened) );
   assert(!infeasible);

   if( tightened )
   {
      SCIPdebugMessage("variable <%s> changes lower bound <%d> -> <%d>\n", SCIPvarGetName(var), lb, newlb);
      ++(*nbdchgs);
   }
   
   /* adjsut lower bound */
   lb = MAX(lb,newlb);

   /* get latest start due to cores */
   newub = SCIPprofileGetLatestFeasibleStart(profile, lb, ub, duration, demand, &infeasible);
   assert(newub <= ub);
   
   /* check whether job fits or not */
   if( infeasible )
   {
      SCIPdebugMessage("infeasibility detected during change of upper bound of <%s> from %d to %d\n", SCIPvarGetName(var), ub, newub);
      /* initialize conflict analysis */
      SCIP_CALL( initializeConflictAnalysisCoreTimes(scip, cons, var, lb, ub+duration, duration, demand, SCIP_BOUNDTYPE_UPPER) );
      *cutoff = TRUE;
      return SCIP_OKAY;
   }

   assert(newub >= lb);
   inferinfo = getInferInfo(PROPRULE_1_CORETIMES, 0, 0);
 
   /* apply bound change */
   SCIP_CALL( SCIPinferVarUbCons(scip, var, (SCIP_Real)newub, cons, inferInfoToInt(inferinfo), TRUE, &infeasible, &tightened) );
   assert(!infeasible);

   if( tightened )
   {
      SCIPdebugMessage("variable <%s> changes upper bound <%d> -> <%d>\n", SCIPvarGetName(var), ub, newub);
      ++(*nbdchgs);
   }
  
   return SCIP_OKAY;
}

/** a cumulative constraint is infeasible if its capacity is exceeded at a time where jobs cannot be shifted (core)
 *  anymore we build up a cumulative profile of all cores of jobs and try to improve bounds of all jobs 
 */
static
SCIP_RETCODE propagateCores(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_CONS*            cons,               /**< constraint to be checked */
   SCIP_Bool*            cutoff,             /**< pointer to store if the constraint is infeasible */
   int*                  nbdchgs             /**< pointer to store the number of bound changes */
   )
{
   SCIP_CONSDATA* consdata;
   SCIP_VAR* var;
   CUMULATIVEPROFILE* profile;
   SCIP_Bool* cores;
   SCIP_Bool* fixeds;

   SCIP_Bool infeasible;
   
   int demand;
   int duration;
   int oldnbdchgs;
   int nvars;
   int ncores;
   int j;
   
   assert(scip != NULL);
   assert(cons != NULL);
   
   SCIPdebugMessage("check cores of cumulative constraint <%s>\n", SCIPconsGetName(cons));

   consdata = SCIPconsGetData(cons);
   assert(consdata != NULL);
	
   oldnbdchgs = *nbdchgs;

   nvars = consdata->nvars;
   SCIP_CALL(SCIPallocBufferArray(scip, &cores, nvars) );
   SCIP_CALL(SCIPallocBufferArray(scip, &fixeds, nvars) );
	
   *cutoff =  FALSE;
   infeasible = FALSE;
   ncores = 0;

   SCIP_CALL( SCIPprofileCreate(scip, &profile, consdata->capacity, 4*nvars) );
   
   /* insert all cores */
   for( j = 0; j < nvars; ++j )
   {
      var = consdata->vars[j];
      duration = consdata->durations[j];
      demand =  consdata->demands[j];
      assert(demand > 0);

      assert(SCIPisFeasIntegral(scip, SCIPvarGetLbLocal(var)));
      assert(SCIPisFeasIntegral(scip, SCIPvarGetUbLocal(var)));

      SCIPprofileInsertCore(scip, profile, var, duration, demand, &cores[j], &fixeds[j], &infeasible);

      if( infeasible )
      {
         SCIPdebugMessage("infeasibility due to cores\n");
         
         /* initialize the contflic analyze */
         SCIP_CALL( initializeConflictAnalysisCoreTimes(scip, cons, var, convertBoundToInt(scip, SCIPvarGetUbLocal(var)), 
               convertBoundToInt(scip, SCIPvarGetLbLocal(var)) + duration, duration, demand,  SCIP_BOUNDTYPE_LOWER) );
         *cutoff = TRUE;
         break;
      }

      if( cores[j] ) 
         ++ncores;
   }
   
   if( !(*cutoff) && ncores > 0 )
   {
      /* start checking each job whether bounds can be improved */
      for( j = 0; j < nvars; ++j )
      {
         var = consdata->vars[j];
         duration = consdata->durations[j];
         demand =  consdata->demands[j];
         assert(demand > 0);
         assert(duration > 0);

         if( fixeds[j] )
            continue;
         
         if( cores[j] )
         {
            SCIPprofileDeleteCore(scip, profile, var, duration, demand, NULL);
         }
         
         /* try to improve bounds */
         SCIP_CALL( updateBounds(scip, cons, profile, var, duration, demand, cutoff, nbdchgs) );
         
         if( *cutoff )
            break;
         	      
         /* after updating we might have a new core */
         if( cores[j] || SCIPvarGetLbLocal(var) + duration > convertBoundToInt(scip, SCIPvarGetUbLocal(var)) )
         {
            SCIPprofileInsertCore(scip, profile, var, duration, demand, &cores[j], &fixeds[j], &infeasible);
            assert(cores[j]);
            assert(!infeasible);
         }
      }
   }

   /* if successful, reset age of constraint */
   if( *cutoff || *nbdchgs > oldnbdchgs )
   {
      SCIP_CALL( SCIPresetConsAge(scip, cons) );
   }

   /* free allocated memory */
   SCIPprofileFree(scip, &profile);
   SCIPfreeBufferArray(scip, &fixeds);
   SCIPfreeBufferArray(scip, &cores);
	
   return SCIP_OKAY;
}

/** updates the binary variables by core-times */
static
SCIP_RETCODE checkForHoles(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_CONS*            cons,               /**< cumulative constraint that is propagated */ 
   CUMULATIVEPROFILE*    profile,            /**< profile to use */
   SCIP_VAR*             var,                /**< the variable whose binary variables should be updated */
   int                   duration,           /**< the duration of the given variable */
   int                   demand,             /**< the demand of the given variable */
   SCIP_Bool*            cutoff,             /**< pointer to store if the node is infeasible */
   int*                  nbdchgs             /**< pointer to store the number of bound changes */
   )
{
   SCIP_VAR** binvars;
   SCIP_CONSDATA* consdata;

   SCIP_Bool infeasible;
   SCIP_Bool tightened;

   int offset;
   int nbinvars;
   int lb;
   int ub;
   int t;
   int pos;

   consdata= SCIPconsGetData(cons);
   assert(consdata != NULL);
   
   lb = convertBoundToInt(scip, SCIPvarGetLbLocal(var));
   ub = convertBoundToInt(scip, SCIPvarGetUbLocal(var));
   assert(lb <= ub); 

   if( ! SCIPexistsConsLinking(scip, var) )
      return SCIP_OKAY;

   SCIP_CALL( SCIPgetBinvarsLinking(scip, SCIPgetConsLinking(scip, var), &binvars, &nbinvars) );
   assert(nbinvars > 0 || binvars == NULL);

   if( nbinvars <= 1 )
   {
      return SCIP_OKAY;
   }
   
   assert(binvars != NULL); /* for flexlint */

   /* check each point in time, whether job can be executed! */
   for( t = lb + 1; t < ub; ++t )
   {
      if( !SCIPprofileIsFeasibleStart(profile, t, duration, demand, &pos) )
      {
         INFERINFO inferinfo;
         
         offset = SCIPgetOffsetLinking(scip, SCIPgetConsLinking(scip, var));
         assert(binvars[t-offset] != NULL);
         
         inferinfo = getInferInfo(PROPRULE_2_CORETIMEHOLES, t-offset, profile->timepoints[pos]);
         /* apply bound change */
         SCIP_CALL( SCIPinferVarUbCons(scip, binvars[t-offset], 0.0, cons, inferInfoToInt(inferinfo), TRUE, &infeasible, &tightened) );

         if( infeasible )
         {
            SCIPdebugMessage("infeasibility detected during fixing to zero of var <%s> at time %d not scheduable at %d\n", SCIPvarGetName(binvars[t-offset]), t, profile->timepoints[pos]);
            
            assert(profile->freecapacities[pos] < consdata->capacity - demand);

            /* initialize conflict analysis */
            SCIP_CALL( initializeConflictAnalysisCoreTimesBinvars(scip, cons, binvars[t-offset], var, profile->timepoints[pos], demand) );
            *cutoff = TRUE;
            return SCIP_OKAY;
         }
    
         if( tightened )
            ++(*nbdchgs);
      }
   }
 
   return SCIP_OKAY;
}

/** propagates the cores and fixes binary variables, possibly creating holes in the domain */
static
SCIP_RETCODE propagateCoresForHoles(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_CONS*            cons,               /**< constraint to be checked */
   SCIP_Bool*            cutoff,             /**< pointer to store if the constraint is infeasible */
   int*                  nbdchgs             /**< pointer to store the number of bound changes */
   )
{
   SCIP_CONSDATA* consdata;
   SCIP_VAR* var;
   CUMULATIVEPROFILE* profile;
   SCIP_Bool* cores;
   SCIP_Bool* fixeds;

   SCIP_Bool infeasible;
   
   int demand;
   int duration;
   int oldnbdchgs;
   int nvars;
   int ncores;
   int j;
   
   assert(scip != NULL);
   assert(cons != NULL);
   
   SCIPdebugMessage("check cores of cumulative constraint <%s>\n", SCIPconsGetName(cons));

   consdata = SCIPconsGetData(cons);
   assert(consdata != NULL);
	
   oldnbdchgs = *nbdchgs;

   nvars = consdata->nvars;
   SCIP_CALL(SCIPallocBufferArray(scip, &cores, nvars) );
   SCIP_CALL(SCIPallocBufferArray(scip, &fixeds, nvars) );
	
   *cutoff =  FALSE;
   infeasible = FALSE;
   ncores = 0;

   SCIP_CALL( SCIPprofileCreate(scip, &profile, consdata->capacity, 4*nvars) );
   
   /* insert all cores */
   for( j = 0; j < nvars; ++j )
   {
      var = consdata->vars[j];
      duration = consdata->durations[j];
      demand =  consdata->demands[j];
      assert(demand > 0);

      assert(SCIPisFeasIntegral(scip, SCIPvarGetLbLocal(var)));
      assert(SCIPisFeasIntegral(scip, SCIPvarGetUbLocal(var)));

      SCIPprofileInsertCore(scip, profile, var, duration, demand, &cores[j], &fixeds[j], &infeasible);
      assert(!infeasible);
      
      if( cores[j] ) 
         ++ncores;
   }

   if( !(*cutoff) && ncores > 0 )
   {
      /* start checking each job whether bounds can be improved */
      for( j = 0; j < nvars; ++j )
      {
         var = consdata->vars[j];
         duration = consdata->durations[j];
         demand =  consdata->demands[j];
         assert(demand > 0);
         assert(duration > 0);

         if( fixeds[j] )
            continue;
         
         if( cores[j] )
         {
            SCIPprofileDeleteCore(scip, profile, var, duration, demand, NULL);
         }
         
         /* try to improve bounds */
         SCIP_CALL( checkForHoles(scip, cons, profile, var, duration, demand, cutoff, nbdchgs) );
         
         if( *cutoff )
            break;
         	      
         /* after updating we might have a new core */
         if( cores[j] )
         {
            SCIPprofileInsertCore(scip, profile, var, duration, demand, &cores[j], &fixeds[j], &infeasible);
            assert(cores[j]);
            assert(!infeasible); /* cannot be infeasible; otherwise cutoff in checkForHoles() */
         }
      }
   }

   /* if successful, reset age of constraint */
   if( *cutoff || *nbdchgs > oldnbdchgs )
   {
      SCIP_CALL( SCIPresetConsAge(scip, cons) );
   }

   /* free allocated memory */
   SCIPprofileFree(scip, &profile);
   SCIPfreeBufferArray(scip, &fixeds);
   SCIPfreeBufferArray(scip, &cores);
	
   return SCIP_OKAY;
}

/** returns TRUE if all demands are smaller than the capacity of the cumulative constraint */
static
SCIP_Bool checkDemands(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_CONS*            cons                /**< constraint to be checked */
   )
{
   SCIP_CONSDATA* consdata;
   int capacity;   
   int nvars;
   int j;

   assert(scip != NULL);
   assert(cons != NULL);

   consdata = SCIPconsGetData(cons);
   assert(consdata != NULL);

   nvars = consdata->nvars;
   
   /* if no activities are associated with this cumulative then this constraint is not infeasible, return */
   if( nvars == 0 )
      return TRUE;

   assert(consdata->vars != NULL);
   capacity = consdata->capacity;
  
   /* check each activity: if demand is larger than capacity the problem is infeasible */
   for ( j = 0; j < nvars; ++j )
   {
      if( consdata->demands[j] > capacity )
         return FALSE;
   }
   
   return TRUE;
}

/** creates covering cuts for jobs violating resource constraints */
static 
SCIP_RETCODE createCoverCutsTimepoint(
   SCIP*            scip,                 /**< SCIP data structure */
   SCIP_CONS*       cons,                 /**< constraint to be checked */
   int*             startvalues,          /**< upper bounds on finishing time per job for activities from 0,..., nactivities -1 */
   int              time                  /**< at this point in time covering constraints are valid */
   )
{
   SCIP_VAR** binvars;    /* binary variables of some integer variable */
   SCIP_CONSDATA* consdata;
   SCIP_ROW* row;
   int* flexibleids;
   int* demands;

   char rowname[SCIP_MAXSTRLEN];

   int remainingcap;
   int smallcoversize;    /* size of a small cover */ 
   int bigcoversize;    /* size of a big cover */ 
   int nbinvars;
   int offset;
   int nvars; 

   int nflexible;
   int D;            /* demand of all jobs up to a certain index */
   int j; 
   int i;
   
   assert(cons != NULL);

   /* get constraint data structure */
   consdata = SCIPconsGetData(cons);
   assert(consdata != NULL );

   nvars = consdata->nvars;

   /* sort jobs according to demands */
   SCIP_CALL( SCIPallocBufferArray(scip, &demands, nvars) );
   SCIP_CALL( SCIPallocBufferArray(scip, &flexibleids, nvars) );
   
   nflexible = 0;
   remainingcap = consdata->capacity;

   /* get all jobs intersecting point 'time' with their bounds */
   for( j = 0; j < nvars; ++j )
   {
      int ub;
      
      ub = convertBoundToInt(scip, SCIPvarGetUbLocal(consdata->vars[j]));
      
      /* only add jobs to array if they intersect with point 'time' */
      if( startvalues[j] <= time && ub + consdata->durations[j] > time )
      {
         /* if job is fixed, capacity has to be decreased */
         if( startvalues[j] == ub )
         {
            remainingcap -= consdata->demands[j];
         }
         else
         {
            demands[nflexible] = consdata->demands[j];
            flexibleids[nflexible] = j;
            ++nflexible;
         }
      }
   }
   assert(remainingcap >= 0);

   /* sort demands and job ids */
   SCIPsortIntInt(demands, flexibleids, nflexible);

   /*
    * version 1: 
    * D_j := sum_i=0,...,j  d_i, finde j maximal, so dass D_j <= remainingcap
    * erzeuge cover constraint
    * 
    */
   
   /* find maximum number of jobs that can run in parallel (-->coversize = j) */
   D = 0;
   j = 0;

   while( j < nflexible && D <= remainingcap )
   {
      D += demands[j];
      ++j;
   }

   /* j jobs form a conflict, set coversize to 'j - 1' */
   bigcoversize = j-1;
   assert(D > remainingcap);
   assert(bigcoversize < nflexible);   

   /* - create a row for all jobs and their binary variables. 
    * - at most coversize many binary variables of jobs can be set to one 
    */
   
   /* construct row name */
   (void)SCIPsnprintf(rowname, SCIP_MAXSTRLEN, "capacity_coverbig_%d", time);
   SCIP_CALL( SCIPcreateEmptyRow(scip, &row, rowname, -SCIPinfinity(scip), (SCIP_Real)bigcoversize, 
         SCIPconsIsLocal(cons), SCIPconsIsModifiable(cons), TRUE) );
   SCIP_CALL( SCIPcacheRowExtensions(scip, row) );

   for( j = 0; j < nflexible; ++j )
   {
      int idx; 
      int end;
      int start;
      int lb;
      int ub;

      idx = flexibleids[j];

      /* get and add binvars into var array */
      SCIP_CALL( SCIPgetBinvarsLinking(scip, consdata->linkingconss[idx], &binvars, &nbinvars) );
      assert(nbinvars != 0);
      offset = SCIPgetOffsetLinking(scip, consdata->linkingconss[idx]);

      lb = convertBoundToInt(scip, SCIPvarGetLbLocal(consdata->vars[idx]));
      ub = convertBoundToInt(scip, SCIPvarGetUbLocal(consdata->vars[idx]));
      /* compute start and finishing time */
      start = MAX(lb, time + 1 - consdata->durations[idx]) - offset;
      end =  MIN(time, ub) + 1 - offset;

      /* add all neccessary binary variables */
      for( i = start; i < end; ++i )
      {
         assert(i >= 0);
         assert(i < nbinvars);
         assert(binvars[i] != NULL);
         SCIP_CALL( SCIPaddVarToRow(scip, row, binvars[i], 1.0) );
      }
   }
   
   /* insert and release row */
   SCIP_CALL( SCIPflushRowExtensions(scip, row) );

   if( consdata->bcoverrowssize == 0 ) 
   { 
      consdata->bcoverrowssize = 10; 
      SCIP_CALL( SCIPallocBlockMemoryArray(scip, &consdata->bcoverrows, consdata->bcoverrowssize) ); 
   } 
   if( consdata->nbcoverrows == consdata->bcoverrowssize ) 
   { 
      consdata->bcoverrowssize *= 2; 
      SCIP_CALL( SCIPreallocBlockMemoryArray(scip, &consdata->bcoverrows, consdata->nbcoverrows, consdata->bcoverrowssize) ); 
   } 

   consdata->bcoverrows[consdata->nbcoverrows] = row;
   consdata->nbcoverrows++;
   
   /*
    * version 2: 
    * D_j := sum_i=j,...,0  d_i, finde j minimal, so dass D_j <= remainingcap
    * erzeuge cover constraint und fuege alle jobs i hinzu, mit d_i = d_largest
    */
   /* find maximum number of jobs that can run in parallel (= coversize -1) */
   D = 0;
   j = nflexible -1;
   while( D <= remainingcap )
   {
      assert(j >= 0);
      D += demands[j];
      --j;
   }
   
   smallcoversize = nflexible - (j + 1) - 1;
   while( j > 0 && demands[j] == demands[nflexible-1] )
      --j;
   
   assert(smallcoversize < nflexible); 

   if( smallcoversize != 1 || smallcoversize != nflexible - (j + 1) - 1 )   
   {
      /* construct row name */
      (void)SCIPsnprintf(rowname, SCIP_MAXSTRLEN, "capacity_coversmall_%d", time);
      SCIP_CALL( SCIPcreateEmptyRow(scip, &row, rowname, -SCIPinfinity(scip), (SCIP_Real)smallcoversize, 
            SCIPconsIsLocal(cons), SCIPconsIsModifiable(cons), TRUE) );
      SCIP_CALL( SCIPcacheRowExtensions(scip, row) );
      
      /* filter binary variables for each unfixed job */
      for( j = j + 1; j < nflexible; ++j )
      {
         int idx; 
         int end;
         int start;
         int lb;
         int ub;

         idx = flexibleids[j];
         
         /* get and add binvars into var array */
         SCIP_CALL( SCIPgetBinvarsLinking(scip, consdata->linkingconss[idx], &binvars, &nbinvars) );
         assert(nbinvars != 0);
         offset = SCIPgetOffsetLinking(scip, consdata->linkingconss[idx]);

         lb = convertBoundToInt(scip, SCIPvarGetLbLocal(consdata->vars[idx]));
         ub = convertBoundToInt(scip, SCIPvarGetUbLocal(consdata->vars[idx]));
         /* compute start and finishing time */
         start = MAX(lb, time + 1 - consdata->durations[idx]) - offset;
         end =  MIN(time, ub) + 1 - offset;
         
         /* add  all neccessary binary variables */
         for( i = start; i < end; ++i )
         {
            assert(i >= 0);
            assert(i < nbinvars);
            assert(binvars[i] != NULL);
            SCIP_CALL( SCIPaddVarToRow(scip, row, binvars[i], 1.0) );
         }
      }
      
      /* insert and release row */
      SCIP_CALL( SCIPflushRowExtensions(scip, row) );
      if( consdata->scoverrowssize == 0 )  
      {  
         consdata->scoverrowssize = 10;  
         SCIP_CALL( SCIPallocBlockMemoryArray(scip, &consdata->scoverrows, consdata->scoverrowssize) );  
      }  
      if( consdata->nscoverrows == consdata->scoverrowssize )  
      {  
         consdata->scoverrowssize *= 2;  
         SCIP_CALL( SCIPreallocBlockMemoryArray(scip, &consdata->scoverrows, consdata->nscoverrows, consdata->scoverrowssize) );  
      }  
      
      consdata->scoverrows[consdata->nscoverrows] = row; 
      consdata->nscoverrows++; 
   }
   
   /* free buffer arrays */
   SCIPfreeBufferArray(scip, &flexibleids);
   SCIPfreeBufferArray(scip, &demands);

   return SCIP_OKAY;
}

/** method to construct cover cuts for all points in time */
static 
SCIP_RETCODE createCoverCuts(
   SCIP*            scip,                      /**< SCIP data structure */
   SCIP_CONS*       cons                       /**< constraint to be separated */
   )
{
   SCIP_CONSDATA* consdata;
   
   int* startvalues;        /* stores when each job is starting */
   int* endvalues;          /* stores when each job ends */
   int* startvaluessorted;  /* stores when each job is starting */
   int* endvaluessorted;    /* stores when each job ends */
   int* startindices;     /* we sort the startvalues, so we need to know wich index of a job it corresponds to */
   int* endindices;       /* we sort the endvalues, so we need to know wich index of a job it corresponds to */
   
   int nvars;               /* number of jobs for this constraint */
   int freecapacity;        /* remaining capacity */
   int curtime;             /* point in time which we are just checking */
   int endidx;              /* index of endsolvalues with: endsolvalues[endindex] > curtime */
   
   int j;
   int t;
 
   assert(scip != NULL);
   assert(cons != NULL);

   consdata = SCIPconsGetData(cons);
   assert(consdata != NULL);

   /* if no activities are associated with this resource then this constraint is redundant */
   if( consdata->vars == NULL )
      return SCIP_OKAY;

   nvars = consdata->nvars;
   
   SCIP_CALL( SCIPallocBufferArray(scip, &startvalues, nvars) );
   SCIP_CALL( SCIPallocBufferArray(scip, &endvalues, nvars) );
   SCIP_CALL( SCIPallocBufferArray(scip, &startvaluessorted, nvars) );
   SCIP_CALL( SCIPallocBufferArray(scip, &endvaluessorted, nvars) );
   SCIP_CALL( SCIPallocBufferArray(scip, &startindices, nvars) );
   SCIP_CALL( SCIPallocBufferArray(scip, &endindices, nvars) );

   /* assign start and endpoints to arrays */
   for ( j = 0; j < nvars; ++j )
   {
      startvalues[j] = convertBoundToInt(scip, SCIPvarGetLbLocal(consdata->vars[j]));
      startvaluessorted[j] = startvalues[j];
      
      endvalues[j] = convertBoundToInt(scip, SCIPvarGetUbLocal(consdata->vars[j])) + consdata->durations[j];
      endvaluessorted[j] = endvalues[j];

      startindices[j] = j;
      endindices[j] = j;
   }

   /* sort the arrays not-decreasing according to startsolvalues and endsolvalues 
    * (and sort the indices in the same way) */
   SCIPsortIntInt(startvaluessorted, startindices, nvars);
   SCIPsortIntInt(endvaluessorted, endindices, nvars);

   endidx = 0;
   freecapacity = consdata->capacity;

   /* check each startpoint of a job whether the capacity is kept or not */
   for( j = 0; j < nvars; ++j )
   {
      curtime = startvaluessorted[j];

      /* subtract all capacity needed up to this point */  
      freecapacity -= consdata->demands[startindices[j]];
      
      while( j+1 < nvars && startvaluessorted[j+1] == curtime )
      {
         ++j;
         freecapacity -= consdata->demands[startindices[j]];
      }

      /* free all capacity usages of jobs the are no longer running */
      while( endidx < nvars && curtime >= endvaluessorted[endidx] )
      {
         freecapacity += consdata->demands[endindices[endidx]];
         ++endidx;
      }

      assert(freecapacity <= consdata->capacity);
      assert(endidx <= nvars);
      
      /* --> endindex - points to the next job which will finish
       *     j        - points to the last job that has been released 
       */
      
      
      /* check freecapacity to be smaller than zero 
       * then we will add cover constraints to the MIP
       */
      if( freecapacity < 0 )
      {
         int nextprofilechange;
         
         /* we can create covering constraints for each pint in time in interval [curtime; nextprofilechange[ */
         if( j < nvars-1 ) 
            nextprofilechange = MIN( startvaluessorted[j+1], endvaluessorted[endidx] );
         else 
            nextprofilechange = endvaluessorted[endidx];
         
         for( t = curtime; t < nextprofilechange; ++t )
         { 
            SCIPdebugMessage("add cover constraint for time %d\n", curtime);
            
            /* create covering constraint */
            SCIP_CALL( createCoverCutsTimepoint(scip, cons, startvalues, t)  );

         }
      } /* end if freecapacity > 0 */
      
   } /* end for each activityindex j */
   
   consdata->covercuts = TRUE;

   /* free all buffer arrays */
   SCIPfreeBufferArray(scip, &endindices);   
   SCIPfreeBufferArray(scip, &startindices);
   SCIPfreeBufferArray(scip, &endvaluessorted);
   SCIPfreeBufferArray(scip, &startvaluessorted);
   SCIPfreeBufferArray(scip, &endvalues);
   SCIPfreeBufferArray(scip, &startvalues);

   return SCIP_OKAY;
}

/** collects all necessary binary variables to represent the jobs which can be active at time point of interest */
static
SCIP_RETCODE collectBinaryVars(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_CONSDATA*        consdata,           /**< constraint data */
   SCIP_VAR***           vars,               /**< pointer to the array to store the binary variables */
   int**                 coefs,              /**< pointer to store the coefficients */
   int*                  nvars,              /**< number if collect binary variables */
   int*                  startindices,       /**< permutation with rspect to the start times */
   int                   curtime,            /**< current point in time */
   int                   nstarted,           /**< number of jobs that start before the curtime or at curtime */
   int                   nfinished           /**< number of jobs that finished before curtime or at curtime */
   )
{
   SCIP_VAR** binvars;
   SCIP_VAR* var;
   int nbinvars;
   int nrowvars;
   int startindex;
   int endtime;
   int duration;
   int demand;
   int varidx;
   int offset;
   int minub;
   int size;

   size = 10;
   nrowvars = 0;
   startindex = nstarted - 1;

   SCIP_CALL( SCIPallocBufferArray(scip, vars, size) );
   SCIP_CALL( SCIPallocBufferArray(scip, coefs, size) );

   /* search for the (nstarted - nfinished) jobs which are active at curtime */
   while( nstarted - nfinished > nrowvars )
   {
      /* collect job information */
      varidx = startindices[startindex];
      assert(varidx >= 0 && varidx < consdata->nvars);
      
      var = consdata->vars[varidx];
      duration = consdata->durations[varidx];
      demand = consdata->demands[varidx];
      assert(var != NULL);
      
      endtime = convertBoundToInt(scip, SCIPvarGetUbGlobal(var)) + duration;
      
      /* check the end time of this job is larger than the curtime; in this case the job is still running */
      if( endtime > curtime )
      {
         int tau;  /* counter from curtime - duration + 1 to curtime */

         /* check if the linking constraints exists */
         assert(SCIPexistsConsLinking(scip, var));
         assert(SCIPgetConsLinking(scip, var) != NULL);
	 assert(SCIPgetConsLinking(scip, var) == consdata->linkingconss[varidx]);
         
         /* collect linking constraint information */
         SCIP_CALL( SCIPgetBinvarsLinking(scip, consdata->linkingconss[varidx], &binvars, &nbinvars) );
         offset = SCIPgetOffsetLinking(scip, consdata->linkingconss[varidx]);
         
         minub = MIN(curtime, endtime - duration);
         
         for (tau = MAX(curtime - duration + 1, offset); tau <= minub; ++tau )
         {
            assert(tau >= offset && tau < nbinvars + offset);
            assert(binvars[tau-offset] != NULL);

            /* ensure array proper array size */
            if( size == *nvars )
            {
               size *= 2;
               SCIP_CALL( SCIPreallocBufferArray(scip, vars, size) );
               SCIP_CALL( SCIPreallocBufferArray(scip, coefs, size) );
            }
            
            (*vars)[*nvars] = binvars[tau-offset];
            (*coefs)[*nvars] = demand;
            (*nvars)++;
         }         
         nrowvars++;
      }
      
      startindex--;
   }

   return SCIP_OKAY;
}

/** this method creates a row for time point curtime which insures the capacity restriction of the cumulative constraint */
static
SCIP_RETCODE createCapacityRestriction(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_CONS*            cons,               /**< constraint to be checked */
   int*                  startindices,       /**< permutation with rspect to the start times */
   int                   curtime,            /**< current point in time */
   int                   nstarted,           /**< number of jobs that start before the curtime or at curtime */
   int                   nfinished,          /**< number of jobs that finished before curtime or at curtime */
   SCIP_Bool             cutsasconss         /**< should the cumulative constraint create the cuts as constraints? */
   )
{
   SCIP_CONSDATA* consdata;
   SCIP_VAR** binvars;
   int* coefs;
   int nbinvars;
   char name[SCIP_MAXSTRLEN];
   int capacity;
   int b;

   assert(nstarted > nfinished);
   
   consdata = SCIPconsGetData(cons);
   assert(consdata != NULL);
   assert(consdata->nvars > 0);

   capacity = consdata->capacity;
   assert(capacity > 0);

   nbinvars = 0;
   SCIP_CALL( collectBinaryVars(scip, consdata, &binvars, &coefs, &nbinvars, startindices, curtime, nstarted, nfinished) );

   /* construct row name */
   (void)SCIPsnprintf(name, SCIP_MAXSTRLEN, "%s_%d[%d]", SCIPconsGetName(cons), nstarted-1, curtime);
   
   if( cutsasconss )
   {
      SCIP_CONS* lincons;

      /* create linear constraint for the linking between the binary variables and the integer variable */
      SCIP_CALL( SCIPcreateConsKnapsack(scip, &lincons, name, 0, NULL, NULL, (SCIP_Longint)(capacity),
            TRUE, TRUE, TRUE, TRUE, TRUE, FALSE, FALSE, FALSE, TRUE, FALSE) );
      
      for( b = 0; b < nbinvars; ++b )
      {
         SCIP_CALL( SCIPaddCoefKnapsack(scip, lincons, binvars[b], (SCIP_Longint)coefs[b]) );
      }
      
      SCIP_CALL( SCIPaddCons(scip, lincons) );
      SCIP_CALL( SCIPreleaseCons(scip, &lincons) );
   }
   else
   {
      SCIP_ROW* row;
      
      SCIP_CALL( SCIPcreateEmptyRow(scip, &row, name, -SCIPinfinity(scip), (SCIP_Real)capacity, FALSE, FALSE, SCIPconsIsRemovable(cons)) );
      SCIP_CALL( SCIPcacheRowExtensions(scip, row) );

      for( b = 0; b < nbinvars; ++b )
      {
         SCIP_CALL( SCIPaddVarToRow(scip, row, binvars[b], (SCIP_Real)coefs[b]) );
      }

      SCIP_CALL( SCIPflushRowExtensions(scip, row) );
      SCIPdebug( SCIP_CALL(SCIPprintRow(scip, row, NULL)) );
   
      if( consdata->demandrowssize == 0 )
      {
         consdata->demandrowssize = 10;
         SCIP_CALL( SCIPallocBlockMemoryArray(scip, &consdata->demandrows, consdata->demandrowssize) );
      }
      if( consdata->ndemandrows == consdata->demandrowssize )
      {
         consdata->demandrowssize *= 2;
         SCIP_CALL( SCIPreallocBlockMemoryArray(scip, &consdata->demandrows, consdata->ndemandrows, consdata->demandrowssize) );
      }
      
      consdata->demandrows[consdata->ndemandrows] = row;
      consdata->ndemandrows++;
   }

   SCIPfreeBufferArrayNull(scip, &binvars);
   SCIPfreeBufferArrayNull(scip, &coefs);
   
   return SCIP_OKAY;
}

/** initialize the sorted event point arrays */
static 
void createSortedEventpoints(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_CONSDATA*        consdata,           /**< constraint data */
   int*                  starttimes,         /**< array to store sorted start events */
   int*                  endtimes,           /**< array to store sorted end events */
   int*                  startindices,       /**< permutation with rspect to the start times */
   int*                  endindices,         /**< permutation with rspect to the end times */
   SCIP_Bool             local               /**< shall local bounds be used */
   )
{
   SCIP_VAR* var;
   int nvars;
   int j;

   nvars = consdata->nvars;

   /* assign variables, start and endpoints to arrays */
   for ( j = 0; j < nvars; ++j )
   {
      var = consdata->vars[j];
      if( local )
         starttimes[j] = convertBoundToInt(scip, SCIPvarGetLbLocal(var));
      else 
         starttimes[j] = convertBoundToInt(scip, SCIPvarGetLbGlobal(var));
      
      startindices[j] = j;

      if( local )
         endtimes[j] = convertBoundToInt(scip, SCIPvarGetUbLocal(var)) + consdata->durations[j];
      else
         endtimes[j] = convertBoundToInt(scip, SCIPvarGetUbGlobal(var)) + consdata->durations[j];
   
      endindices[j] = j;
   }

   /* sort the arrays not-decreasing according to startsolvalues and endsolvalues (and sort the indices in the same way) */
   SCIPsortIntInt(starttimes, startindices, nvars);
   SCIPsortIntInt(endtimes, endindices, nvars);
}

/** remove the capacity requirments for all job which start at the curtime */
static
void subtractStartingJobDemands(
   SCIP_CONSDATA*        consdata,           /**< constraint data */
   int                   curtime,            /**< current point in time */
   int*                  starttimes,         /**< array of start times */
   int*                  startindices,       /**< permutation with respect to the start times */
   int*                  freecapacity,       /**< pointer to store the resulting free capacity */
   int*                  idx,                /**< pointer to index in start time array */
   int                   nvars               /**< number of vars in array of starttimes and startindices */
   )
{
   assert(idx != NULL);
#ifdef SCIP_DEBUG
   int oldidx;
   oldidx = *idx;
#endif
   assert(starttimes != NULL);
   assert(starttimes != NULL);
   assert(freecapacity != NULL);
   assert(starttimes[*idx] == curtime);
   assert(consdata->demands != NULL);
   assert(freecapacity != idx);
   
   /* subtract all capacity needed up to this point */
   (*freecapacity) -= consdata->demands[startindices[*idx]];

   while( (*idx)+1 < nvars && starttimes[(*idx)+1] == curtime )
   {
      ++(*idx);
      (*freecapacity) -= consdata->demands[startindices[(*idx)]];
      assert(freecapacity != idx);
   }
#ifdef SCIP_DEBUG
   assert(oldidx <= *idx);
#endif
}

/** add the capacity requirments for all job which end at the curtime */
static
void addEndingJobDemands(
   SCIP_CONSDATA*        consdata,           /**< constraint data */
   int                   curtime,            /**< current point in time */
   int*                  endtimes,           /**< array of end times */
   int*                  endindices,         /**< permutation with rspect to the end times */
   int*                  freecapacity,       /**< pointer to store the resulting free capacity */
   int*                  idx,                /**< pointer to index in end time array */
   int                   nvars               /**< number of vars in array of starttimes and startindices */
   )
{
#ifdef SCIP_DEBUG
   int oldidx;
   oldidx = *idx;
#endif

   /* free all capacity usages of jobs the are no longer running */
   while( endtimes[*idx] <= curtime && *idx < nvars)
   {
      (*freecapacity) += consdata->demands[endindices[*idx]];
      ++(*idx);
   }

#ifdef SCIP_DEBUG
   assert(oldidx <= *idx);
#endif
}
 
/** this method checks how many cumulatives can run at most at one time if this is greater than the capacity it creates
 *  row 
 */
static
SCIP_RETCODE consCapacityConstraintsFinder(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_CONS*            cons,               /**< constraint to be checked */
   SCIP_Bool             cutsasconss         /**< should the cumulative constraint create the cuts as constraints? */
   )
{
   SCIP_CONSDATA* consdata;

   int* starttimes;         /* stores when each job is starting */
   int* endtimes;           /* stores when each job ends */
   int* startindices;       /* we will sort the startsolvalues, thus we need to know wich index of a job it corresponds to */
   int* endindices;         /* we will sort the endsolvalues, thus we need to know wich index of a job it corresponds to */
   
   int nvars;               /* number of activities for this constraint */
   int freecapacity;        /* remaining capacity */
   int curtime;             /* point in time which we are just checking */
   int endindex;            /* index of endsolvalues with: endsolvalues[endindex] > curtime */
   
   int j;

   assert(scip != NULL);
   assert(cons != NULL);

   consdata = SCIPconsGetData(cons);
   assert(consdata != NULL);

   nvars = consdata->nvars;

   
   /* if no activities are associated with this cumulative then this constraint is redundant */
   if( nvars == 0 )
      return SCIP_OKAY;
   
   assert(consdata->vars != NULL);

   SCIP_CALL( SCIPallocBufferArray(scip, &starttimes, nvars) );
   SCIP_CALL( SCIPallocBufferArray(scip, &endtimes, nvars) );
   SCIP_CALL( SCIPallocBufferArray(scip, &startindices, nvars) );
   SCIP_CALL( SCIPallocBufferArray(scip, &endindices, nvars) );

   SCIPdebugMessage("create sorted event points for cumulative constraint <%s> with %d jobs\n",
      SCIPconsGetName(cons), nvars);

   /* create event point arrays */
   createSortedEventpoints(scip, consdata,starttimes, endtimes, startindices, endindices, FALSE);
   
   endindex = 0;
   freecapacity = consdata->capacity;

   /* check each startpoint of a job whether the capacity is kept or not */
   for( j = 0; j < nvars; ++j )
   {
      curtime = starttimes[j];
      SCIPdebugMessage("look at %d-th job with start %d\n", j, curtime);
      
      /* remove the capacity requirments for all job which start at the curtime */
      subtractStartingJobDemands(consdata, curtime, starttimes, startindices, &freecapacity, &j, nvars);

      /* add the capacity requirments for all job which end at the curtime */
      addEndingJobDemands(consdata, curtime, endtimes, endindices, &freecapacity, &endindex, nvars);

      assert(freecapacity <= consdata->capacity);
      assert(endindex <= nvars);

      /* endindex - points to the next job which will finish */
      /* j - points to the last job that has been released */

      /* if free capacity is smaller than zero, then add rows to the LP */
      if( freecapacity < 0 )
      {
         int nextstarttime;
         int t;
         
         /* step forward until next job is released and see whether capacity constraint is met or not */
         if( j < nvars-1 ) 
            nextstarttime = starttimes[j+1];
         else 
            nextstarttime = endtimes[nvars-1];
         
         /* create capacity restriction row for current event point */
         SCIP_CALL( createCapacityRestriction(scip, cons, startindices, curtime, j+1, endindex, cutsasconss) );
         
         /* create for all points in time between the current event point and next start event point a row if the free
          * capacity is still smaller than zero  */
         for( t = curtime+1 ; t < nextstarttime; ++t )
         {
            /* add the capacity requirments for all job which end at the curtime */
            addEndingJobDemands(consdata, t, endtimes, endindices, &freecapacity, &endindex, nvars);
            
            if( freecapacity < 0 )
            {
               /* add constraint */
               SCIPdebugMessage("add capacity constraint at time %d\n", t);

               /* create capacity restriction row */
               SCIP_CALL( createCapacityRestriction(scip, cons, startindices, t, j+1, endindex, cutsasconss) );
            }
            else
               break;
         }
      } 
   }

   /* free all buffer arrays */
   SCIPfreeBufferArray(scip, &endindices);   
   SCIPfreeBufferArray(scip, &startindices);
   SCIPfreeBufferArray(scip, &endtimes);
   SCIPfreeBufferArray(scip, &starttimes);

   return SCIP_OKAY;
}

/** creates LP rows corresponding to cumulative constraint; therefore, check each point in time if the maximal needed
 *  capacity is larger than the capacity of the cumulative constraint
 *  - for each necessary point in time: 
 *    
 *    sum_j sum_t demand_j * x_{j,t} <= capacity 
 *
 *    where x(j,t) is the binary variables of job j at time t 
 */
static 
SCIP_RETCODE createRelaxation(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_CONS*            cons,               /**< cumulative constraint */
   SCIP_Bool             cutsasconss         /**< should the cumulative constraint create the cuts as constraints? */
   )
{
   SCIP_CONSDATA* consdata;

   consdata = SCIPconsGetData(cons);
   assert(consdata != NULL);
   assert(consdata->demandrows == NULL);
   assert(consdata->ndemandrows == 0);

   SCIP_CALL( consCapacityConstraintsFinder(scip, cons, cutsasconss) ); 

   /* switch of separation for the cumulative constraint if linear constraints are add as cuts */
   if( cutsasconss )
   {      
      if( SCIPconsIsInitial(cons) )
      {
         SCIP_CALL( SCIPsetConsInitial(scip, cons, FALSE) );
      }
      if( SCIPconsIsSeparated(cons) )
      {
         SCIP_CALL( SCIPsetConsSeparated(scip, cons, FALSE) );
      }
      if( SCIPconsIsEnforced(cons) )
      {
         SCIP_CALL( SCIPsetConsEnforced(scip, cons, FALSE) );
      }
   }
   
   return SCIP_OKAY;
}  

/** adds linear relaxation of cumulative constraint to the LP */
static 
SCIP_RETCODE addRelaxation(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_CONS*            cons,               /**< cumulative constraint */
   SCIP_Bool             cutsasconss         /**< should the cumulative constraint create the cuts as constraints? */
   )
{
   SCIP_CONSDATA* consdata;
   int r;

   consdata = SCIPconsGetData(cons);
   assert(consdata != NULL);

   if( consdata->demandrows == NULL )
   {
      SCIP_CALL( createRelaxation(scip, cons, cutsasconss) );  
   }

   for( r = 0; r < consdata->ndemandrows; ++r )
   {
      if( !SCIProwIsInLP(consdata->demandrows[r]) )
      {
         assert(consdata->demandrows[r] != NULL);
         SCIP_CALL( SCIPaddCut(scip, NULL, consdata->demandrows[r], FALSE) );
      }
   }
   
   return SCIP_OKAY;
}

/** repropagation of energetic reasoning algorithm */
static
SCIP_RETCODE analyzeConflictEnergeticReasoning(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_CONS*            cons,               /**< constraint to propagate */
   SCIP_VAR*             infervar,           /**< variable whose bound change is to be explained */
   INFERINFO             inferinfo,          /**< inference info containing position of correct bdchgids */
   SCIP_BDCHGIDX*        bdchgidx,           /**< the index of the bound change, representing the point of time where the change took place */
   SCIP_Bool*            success             /**< pointer to store if we could explain the bound change */
   )
{
   SCIP_CONSDATA* consdata;
   
   int est;
   int lct;

   int nvars;
   int j;

   assert(scip != NULL);
   assert(cons != NULL);
   assert(inferInfoGetProprule(inferinfo) == PROPRULE_4_ENERGETICREASONING);
   
   SCIPdebugMessage("repropagate energetic reasoning for constraint <%s> and variable <%s>\n", 
      SCIPconsGetName(cons), infervar == NULL ? "null" : SCIPvarGetName(infervar));

   consdata = SCIPconsGetData(cons);
   assert(consdata != NULL);

   *success = FALSE;
   nvars = consdata->nvars;

   est = inferInfoGetEst(inferinfo);
   lct = inferInfoGetLct(inferinfo); 
   assert(est < lct);

   /* collect the current lower bound of the start variables */
   for( j = 0; j < nvars; ++j ) 
   {
      SCIP_VAR* var;
      var = consdata->vars[j];      
      
      if( var == infervar )
         continue;

      /* report all jobs with non-empty intersection with [est_omega; lct_omega]*/
      if( convertBoundToInt(scip, SCIPvarGetLbAtIndex(var, bdchgidx, FALSE)) + consdata->durations[j] >= est
         && convertBoundToInt(scip, SCIPvarGetUbAtIndex(var, bdchgidx, FALSE)) <= lct )
      {         
         SCIP_CALL( SCIPaddConflictUb(scip, var, bdchgidx) );
         SCIP_CALL( SCIPaddConflictLb(scip, var, bdchgidx) );     
         *success = TRUE;
      }  
   }

   if( !(*success) )
   {
      SCIPinfoMessage(scip, NULL, "could not resolve conflict from energetic reasoning\n");
      SCIPABORT();
   }

   return SCIP_OKAY;
}

/** initialize conflict analysis and analyze conflict */
static
SCIP_RETCODE initializeConflictAnalysisEnergeticReasoning(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_CONS*            cons,               /**< constraint to analyzed */
   SCIP_VAR*             infervar,           /**< inference variable */
   INFERINFO             inferinfo           /**< inference information */
   )
{
   SCIP_CONSDATA* consdata;
   SCIP_Bool success;
   
   SCIPdebugMessage("initialize conflict analysis for energetic reasoning\n");

   if( SCIPgetStage(scip) != SCIP_STAGE_SOLVING )
      return SCIP_OKAY;

   consdata = SCIPconsGetData(cons);
   assert(consdata != NULL);

   assert(inferInfoGetProprule(inferinfo) == PROPRULE_4_ENERGETICREASONING);
   
   SCIP_CALL( SCIPinitConflictAnalysis(scip) );

   /* add lower and upper bound of variable which lead to the infeasibilty */
   if( infervar != NULL )
   {
      SCIP_CALL( SCIPaddConflictLb(scip, infervar, NULL ) );
      SCIP_CALL( SCIPaddConflictUb(scip, infervar, NULL ) );
      
      SCIPdebugMessage("add lower and upper bounds of variable <%s>\n", SCIPvarGetName(infervar));
   }
   
   SCIP_CALL( analyzeConflictEnergeticReasoning(scip, cons, infervar, inferinfo, NULL, &success) );
   assert(success);

   SCIP_CALL( SCIPanalyzeConflictCons(scip, cons, NULL) );

   return SCIP_OKAY;
}

/** computes the energy in the interval [est,lct] of the given variable if the corresponding job is right shifted */
static
int getVarRightEnergy(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_CONS*            cons,               /**< constraint to be propagated */
   SCIP_HASHMAP*         varhashmap,         /**< hashmap from variables to their indices */
   SCIP_VAR*             var,                /**< variable whose energy is reported */
   int                   est,                /**< left bound of interval */
   int                   lct                 /**< right bound of interval */
   )
{
   SCIP_CONSDATA* consdata;
   int energy;
   int lst_j;
   int min;
   int j;
   
   assert(scip != NULL);
   assert(cons != NULL);
   assert(varhashmap != NULL);
   assert(var != NULL);
   assert(est < lct);

   SCIPdebugMessage("perform energetic reasoning\n");
   
   /* get constraint data */
   consdata = SCIPconsGetData(cons);
   assert(consdata != NULL);

   j = (int)(size_t)SCIPhashmapGetImage(varhashmap, var);
   assert(var == consdata->vars[j]);

   /* compute variable's bounds */
   lst_j = convertBoundToInt(scip, SCIPvarGetUbLocal(var));
   
   min = MIN(lct - est, lct - lst_j);

   energy =  MAX(0, MIN( min, consdata->durations[j] )) * consdata->demands[j];

   return energy;
}

/** computes the energy in the interval [est,lct] of the given variable if the corresponding job is left shifted */
static
int getVarLeftEnergy(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_CONS*            cons,               /**< constraint to be propagated */
   SCIP_HASHMAP*         varhashmap,         /**< hashmap from variables to their indices */
   SCIP_VAR*             var,                /**< variable whose energy is reported */
   int                   est,                /**< left bound of interval */
   int                   lct                 /**< right bound of interval */
   )
{
   SCIP_CONSDATA* consdata;
   int energy;
   int ect_j;
   int min;
   int j;
   
   assert(scip != NULL);
   assert(cons != NULL);
   assert(varhashmap != NULL);
   assert(var != NULL);
   assert(est < lct);

   /* get constraint data */
   consdata = SCIPconsGetData(cons);
   assert(consdata != NULL);

   j = (int)(size_t)SCIPhashmapGetImage(varhashmap, var);
   assert(var == consdata->vars[j]);

   /* compute variable's bounds */
   ect_j = convertBoundToInt(scip, SCIPvarGetLbLocal(var)) + consdata->durations[j];
   
   min = MIN( lct-est, ect_j-est );

   energy =  MAX(0, MIN( min, consdata->durations[j] )) * consdata->demands[j];

   return energy;
}

/** computes the energy in the interval [est,lct] of the given variable */
static
int getVarEnergy(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_CONS*            cons,               /**< constraint to be propagated */
   SCIP_HASHMAP*         varhashmap,         /**< hashmap from variables to their indices */
   SCIP_VAR*             var,                /**< variable whose energy is reported */
   int                   est,                /**< left bound of interval */
   int                   lct                 /**< right bound of interval */
   )
{
   SCIP_CONSDATA* consdata;
   int energy;
   int lst_j;
   int ect_j;
   int min;
   int j;
   
   assert(scip != NULL);
   assert(cons != NULL);
   assert(varhashmap != NULL);
   assert(var != NULL);
   assert(est < lct);

   SCIPdebugMessage("perform energetic reasoning\n");
   
   /* get constraint data */
   consdata = SCIPconsGetData(cons);
   assert(consdata != NULL);

   j = (int)(size_t)SCIPhashmapGetImage(varhashmap, var);
   assert(var == consdata->vars[j]);

   /* compute variable's bounds */
   ect_j = convertBoundToInt(scip, SCIPvarGetLbLocal(var)) + consdata->durations[j];
   lst_j = convertBoundToInt(scip, SCIPvarGetUbLocal(var));

   min = MIN3( lct-est, ect_j-est, lct-lst_j );
   
   energy =  MAX(0, MIN( min, consdata->durations[j] )) * consdata->demands[j];

   return energy;
}

/** computes the energy in the interval [est,lct] of all variable/jobs */
static
int computeEnergy(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_CONS*            cons,               /**< constraint to be propagated */
   int                   est,                /**< left bound of interval */
   int                   lct                 /**< right bound of interval */
   )
{

   SCIP_CONSDATA* consdata;
   int energy;
   int nvars;
   int j;
   
   assert(scip != NULL);
   assert(cons != NULL);
   assert(est < lct);

   SCIPdebugMessage("perform energetic reasoning\n");
   
   /* get constraint data */
   consdata = SCIPconsGetData(cons);
   assert(consdata != NULL);
   
   nvars = consdata->nvars;

   energy = 0;

   /* loop over all jobs to compute their energetic demand in [est, lct] */
   for( j = 0; j < nvars; ++j )
   {
      SCIP_VAR* var;
      int lst_j;
      int ect_j;
      int min;

      var = consdata->vars[j];
      ect_j = convertBoundToInt(scip, SCIPvarGetLbLocal(var)) + consdata->durations[j];
      lst_j = convertBoundToInt(scip, SCIPvarGetUbLocal(var));
      
      min = MIN3( lct-est, ect_j-est, lct-lst_j );

      energy += MAX(0, MIN(min, consdata->durations[j])) * consdata->demands[j];
   }

   return energy;
}

/** detects whether new edges should be added to the relaxation */
static
SCIP_RETCODE performEnergeticReasoning(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_CONS*            cons,               /**< constraint to be propagated */
   SCIP_Bool*            cutoff,             /**< pointer to store if the constraint is infeasible */
   int*                  nbdchgs             /**< pointer to store the number of bound changes */
   )
{
   SCIP_CONSDATA* consdata;
   SCIP_HASHMAP* varhashmap;
   
   int* ests;
   int* lcts;

   SCIP_Bool infeasible; 
   SCIP_Bool tightened;

   int est;
   int lct;

   int nvars;
   int ntimepoints;
   int ntimepointsest;
   int ntimepointslct;
   int capacity;
   int i;
   int j;

   assert(scip != NULL);
   assert(cons != NULL);

   SCIPdebugMessage("perform energetic reasoning\n");
   
   /* get constraint data */
   consdata = SCIPconsGetData(cons);
   assert(consdata != NULL);
   
   capacity = consdata->capacity;
   nvars = consdata->nvars;
   infeasible = FALSE;

   ntimepoints = 2*nvars;

   SCIP_CALL( SCIPallocBufferArray(scip, &ests, ntimepoints) );
   SCIP_CALL( SCIPallocBufferArray(scip, &lcts, ntimepoints) );
 
   /* insert all jobs into the hashmap to find the corresponding index of the variable */
   SCIP_CALL( SCIPhashmapCreate(&varhashmap, SCIPblkmem(scip), SCIPcalcHashtableSize(nvars)) );

   /* initializing est and lct */
   for( j = 0; j < nvars; ++j ) 
   {
      SCIP_VAR* var; 

      var = consdata->vars[j];

      assert(!SCIPhashmapExists(varhashmap, var));
      SCIP_CALL( SCIPhashmapInsert(varhashmap, var, (void*)(size_t)j) );

      lcts[2*j] = convertBoundToInt(scip, SCIPvarGetLbLocal(var)) + consdata->durations[j];
      lcts[2*j+1] = convertBoundToInt(scip, SCIPvarGetUbLocal(var)) + consdata->durations[j];
      
      ests[2*j] = convertBoundToInt(scip, SCIPvarGetLbLocal(var));
      ests[2*j+1] = convertBoundToInt(scip, SCIPvarGetUbLocal(var));
   }

   /* sort the latest completion times */
   SCIPsortInt(lcts, ntimepoints);
   SCIPsortInt(ests, ntimepoints);

   j = 0;
   for( i = 0; i < ntimepoints; ++i )
   {
      if( j == 0 || lcts[i] > lcts[j-1]  )
      {
         lcts[j] = lcts[i];
         j++;
      }
   }
   ntimepointslct = j;

   j = 0;
   for( i = 0; i < ntimepoints; ++i )
   {
      if( j == 0 || ests[i] > ests[j-1]  )
      {
         ests[j] = ests[i];
         j++;
      }
   }
   ntimepointsest = j;
   
   for( i = 0; i < ntimepointsest && !infeasible; ++i )
   {
      est = ests[i];

      for( j = ntimepointslct-1; j >= 0 && !infeasible; --j )
      {
         int energy;
         int k;
         
         lct = lcts[j];

         /* only check non-empty intervals */
         if( lct <= est )
            break;

         energy = computeEnergy(scip, cons, est, lct);

         /* check all jobs */
         for( k = 0; k < nvars && !infeasible; k++ )      
         {
            SCIP_VAR* var_k;
            int pos_k;
            int lst_k;
            int lct_k;
            int rightenergy_k;
            int energy_k;     /* energy that var_k contributes to [est,lct] */    
            INFERINFO inferinfo;    
            int demand_k; 
          
            var_k = consdata->vars[k];
            pos_k = (int)(size_t)SCIPhashmapGetImage(varhashmap, var_k);
            
            lst_k = convertBoundToInt(scip, SCIPvarGetUbLocal(var_k));
            lct_k = lst_k + consdata->durations[pos_k];

            /* don't do anything if job does not intersect before [est;lct] */
            if( lst_k >= lct || lct_k <=est ) 
               continue;

            demand_k = consdata->demands[pos_k];
            energy_k = getVarEnergy(scip, cons, varhashmap, var_k, est, lct) ;
            rightenergy_k = getVarRightEnergy(scip, cons, varhashmap, var_k, est, lct) ;

            if( energy - energy_k > (capacity - demand_k) * (lct - est) 
               && energy - energy_k + rightenergy_k > capacity * (lct - est) )
            {
               /* update lct_k */
               SCIP_Real diff;
               diff = (energy - energy_k - (capacity - demand_k) * (lct - est)) / (SCIP_Real)demand_k ;
               
               lst_k = lct - (int)SCIPfeasCeil(scip, diff) - consdata->durations[pos_k];

               /* check if problem is already infeasible (energy overload)*/
               if( lst_k + consdata->durations[pos_k] < est )
               {
                  SCIPdebugMessage("energetic reasoning detected overload in [%d,%d]\n", est, lct);
                  inferinfo = getInferInfo(PROPRULE_4_ENERGETICREASONING, est, lct);
                  SCIP_CALL( initializeConflictAnalysisEnergeticReasoning(scip, cons, NULL, inferinfo) );
                  infeasible = TRUE;
               }
               else  /* problem seems feasible --> update bound */
               { 
                  inferinfo = getInferInfo(PROPRULE_4_ENERGETICREASONING, est, lct);
                  
                  SCIPdebugMessage("energetic reasoning updates var <%s>[dur=%d, dem=%d] ub from %g to %d in interval [%d,%d]\n", 
                     SCIPvarGetName(var_k), consdata->durations[pos_k], demand_k, 
                     SCIPvarGetUbLocal(var_k), lst_k, est, lct);
                  SCIP_CALL( SCIPinferVarUbCons(scip, var_k, (SCIP_Real)lst_k, cons, 
                        inferInfoToInt(inferinfo), TRUE, &infeasible, &tightened) );
                  
                  if( tightened )
                     ++(*nbdchgs);
                  
                  if( infeasible )
                  {
                     SCIPdebugMessage("energetic reasoning detected infeasibility: ub-update\n");
                     SCIP_CALL( initializeConflictAnalysisEnergeticReasoning(scip, cons, var_k, inferinfo) );
                  }
               }
               /* recompute energy */
               energy = energy - energy_k + getVarEnergy(scip, cons, varhashmap, var_k, est, lct);

            }
         }

         /* check all jobs for lb update */ 
         for( k = 0; k < nvars && !infeasible; ++k)   
         {
            SCIP_VAR* var_k;
            int pos_k;
            int est_k;
            int ect_k;   
            int energy_k;     /* energy that var_k contributes to [est,lct] */        
            int leftenergy_k; 
            int demand_k;
            INFERINFO inferinfo;
            
            var_k = consdata->vars[k];
            pos_k = (int)(size_t)SCIPhashmapGetImage(varhashmap, var_k);
            
            est_k = convertBoundToInt(scip, SCIPvarGetLbLocal(var_k));
            ect_k = est_k + consdata->durations[pos_k];
            
            /* don't do anything if job does not intersect [est;lct] */
            if( ect_k <= est || est_k >= lct )
               continue;
            
            demand_k = consdata->demands[pos_k];
            energy_k = getVarEnergy(scip, cons, varhashmap, var_k, est, lct) ;
            leftenergy_k = getVarLeftEnergy(scip, cons, varhashmap, var_k, est, lct) ;

            if( energy - energy_k > (capacity - demand_k) * (lct - est) 
               && energy - energy_k + leftenergy_k > capacity * (lct - est) )
            {
               /* update est_k */
               SCIP_Real diff;
               diff = (energy - energy_k - (capacity - demand_k) * (lct - est))/(SCIP_Real)demand_k;
               
               est_k = est + (int)SCIPfeasCeil(scip, diff);

               if( est_k > lct )
               {
                  SCIPdebugMessage("energetic reasoning detected overload in [%d,%d]\n", est, lct);
                  inferinfo = getInferInfo(PROPRULE_4_ENERGETICREASONING, est, lct);
                  SCIP_CALL( initializeConflictAnalysisEnergeticReasoning(scip, cons, NULL, inferinfo) );
                  infeasible = TRUE;
               }
               else  /* problem seems feasible --> update bound */
               { 
                  inferinfo = getInferInfo(PROPRULE_4_ENERGETICREASONING, est, lct);
                  SCIPdebugMessage("energetic reasoning updates var <%s>[dur=%d, dem=%d] lb from %g to %d in interval [%d,%d]\n", 
                     SCIPvarGetName(var_k),  consdata->durations[pos_k], demand_k, 
                     SCIPvarGetLbLocal(var_k), est_k, est, lct);
                  SCIP_CALL( SCIPinferVarLbCons(scip, var_k, (SCIP_Real)est_k, cons, inferInfoToInt(inferinfo), TRUE, &infeasible, &tightened) );
                  
                  if( tightened )
                     ++(*nbdchgs);
                  
                  if( infeasible )
                  {
                     SCIPdebugMessage("energetic reasoning detected infeasibility in Node %lld: lb-update\n",
                        SCIPnodeGetNumber(SCIPgetCurrentNode(scip)));
                     
                     SCIP_CALL( initializeConflictAnalysisEnergeticReasoning(scip, cons, var_k, inferinfo) ); 
                  }
               }

               /* recompute energy */
               energy = energy - energy_k + getVarEnergy(scip, cons, varhashmap, var_k, est, lct);
            }
         }
         
         /* go to next change in lct */
         while( j > 0 && lcts[j-1] == lct )
            --j;
      }

      /* go to next change in est */
      while( i < ntimepointsest-1 && ests[i+1] == est )
         ++i;
   }

   if( infeasible )
   {
      SCIPdebugMessage("energetic reasoning detected infeasibility\n");
      *cutoff = TRUE;
   }

   /* free hashmap */
   SCIPhashmapFree(&varhashmap);
   
   /* free buffer arrays */
   SCIPfreeBufferArray(scip, &lcts);   
   SCIPfreeBufferArray(scip, &ests);   

   return SCIP_OKAY;
}

/** repropagation of Edge finding algorithm simplified version from Petr Vilim
 *  only a small subset is reported such that energy in total and for bound change is enough 
 */
static
SCIP_RETCODE analyzeShortConflictEdgeFinding(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_CONS*            cons,               /**< constraint to propagate */
   SCIP_VAR*             infervar,           /**< variable whose bound change is to be explained */
   INFERINFO             inferinfo,          /**< inference info containing position of correct bdchgids */
   int                   inferdemand,        /**< demand of inference variable */
   int                   inferduration,      /**< duration of inference variable */
   int                   inferdiff,          /**< difference that has to be reached by edge-finder for bound update */
   SCIP_BDCHGIDX*        bdchgidx,           /**< the index of the bound change, representing the point of time where the change took place */
   SCIP_Bool*            success             /**< pointer to store if we could explain the bound change */
   )
{
   SCIP_CONSDATA* consdata;
   
   SCIP_Real neededenergy;

   int* varids;          /* array of job indices that intersect the omega-interval */
   int* energies;        /* and their corresponding energies */
   int sizeenergies;

   int est_omega;
   int lct_omega;
   int delta_omega;

   int energy;
   int inferenergy;
   int nvars;
   int j;
   
   assert(scip != NULL);
   assert(cons != NULL);
   assert(inferInfoGetProprule(inferinfo) == PROPRULE_3_EDGEFINDING);
   
   SCIPdebugMessage("repropagate edge-finding with short reasons for constraint <%s> and variable <%s>\n", SCIPconsGetName(cons), SCIPvarGetName(infervar));

   consdata = SCIPconsGetData(cons);
   assert(consdata != NULL);

   *success = FALSE;
   nvars = consdata->nvars;
   sizeenergies = 0;

   est_omega = inferInfoGetEst(inferinfo);
   lct_omega = inferInfoGetLct(inferinfo);

   SCIP_CALL( SCIPallocBufferArray(scip, &energies, nvars) );
   SCIP_CALL( SCIPallocBufferArray(scip, &varids, nvars) );

   /* collect the energies of all variables in [est_omega, lct_omega] */
   for( j = 0; j < nvars; ++j ) 
   {
      SCIP_VAR* var;
      int lb;
      int ub;

      var = consdata->vars[j];      
      
      if( var == infervar )
         continue;

      lb = convertBoundToInt(scip, SCIPvarGetLbAtIndex(var, bdchgidx, FALSE));
      ub = convertBoundToInt(scip, SCIPvarGetUbAtIndex(var, bdchgidx, FALSE));

      /* report all jobs running in [est_omega; lct_omega] (these might be too much, but ok) */
      if( lb >= est_omega &&  ub + consdata->durations[j] <= lct_omega )
      {         
         energies[sizeenergies] = consdata->durations[j]*consdata->demands[j];
         varids[sizeenergies] = j;         
         sizeenergies++;
      }  
   }

   SCIPsortDownIntInt(energies, varids, sizeenergies);

   delta_omega = lct_omega - est_omega;
   neededenergy = (consdata->capacity - inferdemand) * delta_omega / (SCIP_Real)inferdemand;
   inferenergy = inferdemand * inferduration;

   /* report conflicting jobs until enough energy is reported */
   energy = 0;
   for( j = 0; j < sizeenergies; ++j )
   {
      SCIP_Real remaining;
      energy += energies[j];
      
      SCIP_CALL( SCIPaddConflictUb(scip, consdata->vars[varids[j]], bdchgidx) );
      SCIP_CALL( SCIPaddConflictLb(scip, consdata->vars[varids[j]], bdchgidx) );     

      remaining = SCIPfeasCeil(scip, (SCIP_Real)energy - neededenergy);

      if( remaining >= inferdiff && energy + inferenergy > consdata->capacity * delta_omega )
      {
         *success = TRUE;
         break; 
      }
      
#ifdef SCIP_DEBUG
      if( remaining >= inferdiff )
      {
         SCIPdebugMessage("enough energ for C-c_i\n");
      }
      if( energy + inferenergy > consdata->capacity * delta_omega )
      {
         SCIPdebugMessage("enough energy for C\n");
      }
#endif
   }
   
   /* free buffer array */
   SCIPfreeBufferArray(scip, &varids);
   SCIPfreeBufferArray(scip, &energies);

   if( !(*success) )
   {
      SCIPinfoMessage(scip, NULL, "could not resolve conflict from edgefinding\n");
      SCIPABORT();
   }


   return SCIP_OKAY;
}

/** repropagation of Edge finding algorithm simplified version from Petr Vilim*/
static
SCIP_RETCODE analyzeConflictEdgeFinding(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_CONS*            cons,               /**< constraint to propagate */
   SCIP_VAR*             infervar,           /**< variable whose bound change is to be explained */
   INFERINFO             inferinfo,          /**< inference info containing position of correct bdchgids */
   SCIP_BDCHGIDX*        bdchgidx,           /**< the index of the bound change, representing the point of time where the change took place */
   SCIP_Bool*            success             /**< pointer to store if we could explain the bound change */
   )
{
   SCIP_CONSDATA* consdata;
   
   int est_omega;
   int lct_omega;

   int nvars;
   int j;

   assert(scip != NULL);
   assert(cons != NULL);
   assert(inferInfoGetProprule(inferinfo) == PROPRULE_3_EDGEFINDING);
   
   SCIPdebugMessage("repropagate edge-finding for constraint <%s> and variable <%s>\n", SCIPconsGetName(cons), SCIPvarGetName(infervar));

   consdata = SCIPconsGetData(cons);
   assert(consdata != NULL);

   *success = FALSE;
   nvars = consdata->nvars;

   est_omega = inferInfoGetEst(inferinfo);
   lct_omega = inferInfoGetLct(inferinfo);
 
   /* collect the current lower bound of the start variables */
   for( j = 0; j < nvars; ++j ) 
   {
      SCIP_VAR* var;
      var = consdata->vars[j];      
      
      if( var == infervar )
         continue;

      /* report all jobs running in [est_omega; lct_omega] (these might be too much, but ok) */
      if( convertBoundToInt(scip, SCIPvarGetLbAtIndex(var, bdchgidx, FALSE)) >= est_omega
         && convertBoundToInt(scip, SCIPvarGetUbAtIndex(var, bdchgidx, FALSE)) + consdata->durations[j] <= lct_omega )
      {         
         SCIP_CALL( SCIPaddConflictUb(scip, var, bdchgidx) );
         SCIP_CALL( SCIPaddConflictLb(scip, var, bdchgidx) );     
         *success = TRUE;
      }  
   }

   if( !(*success) )
   {
      SCIPinfoMessage(scip, NULL, "could not resolve conflict from edgefinding\n");
      SCIPABORT();
   }

   return SCIP_OKAY;
}


/** initialize conflict analysis and analyze conflict */
static
SCIP_RETCODE initializeConflictAnalysisEdgeFinding(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_CONS*            cons,               /**< constraint to analyzed */
   SCIP_VAR*             infervar,           /**< inference variable */
   INFERINFO             inferinfo           /**< inference info */
   )
{
   SCIP_Bool success;
   
   SCIPdebugMessage("initialize conflict analysis\n");

   assert(inferInfoGetProprule(inferinfo) == PROPRULE_3_EDGEFINDING);

   if( SCIPgetStage(scip) != SCIP_STAGE_SOLVING )
      return SCIP_OKAY;

   SCIP_CALL( SCIPinitConflictAnalysis(scip) );

   /* add lower and upper bound of variable which leads to the infeasibilty */
   SCIP_CALL( SCIPaddConflictLb(scip, infervar, NULL ) );
   SCIP_CALL( SCIPaddConflictUb(scip, infervar, NULL ) );
 
   SCIPdebugMessage("add lower and upper bounds of variable <%s>\n", SCIPvarGetName(infervar));
   
   SCIP_CALL( analyzeConflictEdgeFinding(scip, cons, infervar, inferinfo, NULL, &success) );
   assert(success);

   SCIP_CALL( SCIPanalyzeConflictCons(scip, cons, NULL) );

   return SCIP_OKAY;
}

/** computes a new earliest starting time of the job in 'respleaf' due to the energy consumption and stores the
 *  responsible interval bounds in *est_omega and *lct_omega 
 */
static
int computeNewLstOmegaset(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_CONS*            cons,               /**< constraint to be propagated */
   SCIP_HASHMAP*         varhashmap,         /**< hashmap variable -> inndex */
   TLTREENODE*           respleaf,           /**< theta tree leaf whose variables lower bound will be updated */
   TLTREENODE**          omegaset,           /**< set of lambda nodes who determine new lower bound */
   int                   nelements,          /**< number of elements in omegaset */
   int                   lct_j,              /**< latest completion time from all nodes in theta lambda tree */
   int                   makespan,           /**< upper bound on lct of all vars */
   int*                  est_omega,          /**< pointer to store est of set omega */
   int*                  lct_omega           /**< pointer to store lct of set omega */
   )
{
   SCIP_CONSDATA* consdata;
   int newest;
   int newlst;
   int tmp;
   int j;
   int pos;
   int energy;

   int demand_pos;
   int duration_pos;
 
   assert(scip != NULL);
   assert(cons != NULL);

   /* get constraint data */
   consdata = SCIPconsGetData(cons);
   assert(consdata != NULL);

   energy = 0;
   newest = 0;
   *est_omega = INT_MAX;
   *lct_omega = 0;
   
   /* get position of responsible variable */
   pos = (int)(size_t)SCIPhashmapGetImage(varhashmap, respleaf->var);
   assert(pos < consdata->nvars);

   demand_pos = consdata->demands[pos];
   duration_pos = consdata->durations[pos];
   assert(demand_pos > 0);
   
   for( j = 0; j < nelements; ++j )
   {
      int idx;

      assert(omegaset[j]->inTheta);
      assert(omegaset[j]->var != NULL);
      idx = (int)(size_t)SCIPhashmapGetImage(varhashmap, omegaset[j]->var);
      assert(idx < consdata->nvars);

      tmp = (int) (makespan - SCIPvarGetUbLocal(omegaset[j]->var) - consdata->durations[idx]);
      *est_omega = MIN(*est_omega, tmp);
      tmp = (int) (makespan - SCIPvarGetLbLocal(omegaset[j]->var));
      *lct_omega = MAX(*lct_omega, tmp);

      assert(consdata->durations[idx] * consdata->demands[idx] == omegaset[j]->energy);
      assert(*lct_omega <= lct_j);
      energy += omegaset[j]->energy;
   }

   /* update est if enough energy */
   if( energy > (consdata->capacity - demand_pos) * (*lct_omega - *est_omega) )
   {
      if( energy+demand_pos*duration_pos > consdata->capacity * (*lct_omega - *est_omega) )
      {
         newest = (int)SCIPfeasCeil(scip, (energy - (consdata->capacity - demand_pos) * (*lct_omega - *est_omega)) / (SCIP_Real)demand_pos);
         newest += *est_omega;
      }

      assert(energy+demand_pos*duration_pos > consdata->capacity * (*lct_omega - *est_omega));

      /* recompute original values using 'makespan' */      
      tmp = makespan - *est_omega;
      *est_omega =  makespan - *lct_omega;
      *lct_omega = tmp;
   }

   newlst = makespan - newest - consdata->durations[pos];
   
   return newlst;
}

/** computes a new latest starting time of the job in 'respleaf' due to the energy consumption and stores the
 *  responsible interval bounds in *est_omega and *lct_omega 
 */
static
int computeNewEstOmegaset(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_CONS*            cons,               /**< constraint to be propagated */
   SCIP_HASHMAP*         varhashmap,         /**< hashmap variable -> inndex */
   TLTREENODE*           respleaf,           /**< theta tree leaf whose variables lower bound will be updated */
   TLTREENODE**          omegaset,           /**< set of lambda nodes who determine new lower bound */
   int                   nelements,          /**< number of elements in omegaset */
   int                   lct_j,              /**< latest completion time from all nodes in theta lambda tree */
   int*                  est_omega,          /**< pointer to store est of set omega */
   int*                  lct_omega           /**< pointer to store lct of set omega */
   )
{
   SCIP_CONSDATA* consdata;
   int newest;
   int j;
   int pos;
   int energy;

   int demand_pos;
   int duration_pos;
 
   assert(scip != NULL);
   assert(cons != NULL);

   /* get constraint data */
   consdata = SCIPconsGetData(cons);
   assert(consdata != NULL);

   energy = 0;
   newest = 0;
   *est_omega = INT_MAX;
   *lct_omega = 0;
   
   /* get position of responsible variable */
   pos = (int)(size_t)SCIPhashmapGetImage(varhashmap, respleaf->var);
   assert(pos < consdata->nvars);

   demand_pos = consdata->demands[pos];
   duration_pos = consdata->durations[pos];
   assert(demand_pos > 0);

   for( j = 0; j < nelements; ++j )
   {
      int idx ;
      int tmp;

      assert(omegaset[j]->var != NULL);
      assert(omegaset[j]->inTheta);
      idx = (int)(size_t)SCIPhashmapGetImage(varhashmap, omegaset[j]->var);
      assert(idx < consdata->nvars);

      tmp = convertBoundToInt(scip, SCIPvarGetLbLocal(omegaset[j]->var));
      *est_omega = MIN(*est_omega, tmp);
      tmp = convertBoundToInt(scip, SCIPvarGetUbLocal(omegaset[j]->var)) + consdata->durations[idx];
      *lct_omega = MAX(*lct_omega, tmp);

      assert(consdata->durations[idx] * consdata->demands[idx] == omegaset[j]->energy);
      assert(*lct_omega <= lct_j);

      energy += omegaset[j]->energy;
   }
  
   if( energy >  (consdata->capacity - demand_pos) * (*lct_omega - *est_omega) )
   {
      if( energy+demand_pos*duration_pos > consdata->capacity * (*lct_omega - *est_omega) )
      {
         newest =  (int)SCIPfeasCeil(scip, (energy - (consdata->capacity - demand_pos) * (*lct_omega - *est_omega)) / (SCIP_Real)demand_pos);
         newest += (*est_omega);
      }
      assert(energy + demand_pos * duration_pos > consdata->capacity * (*lct_omega - *est_omega));
   }

   return newest;
}

/** detects whether new edges should be added to the relaxation */
static
SCIP_RETCODE performEdgeFindingDetection(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_CONS*            cons,               /**< constraint to be propagated */
   SCIP_Bool             forward,            /**< shall lower bounds be updated? */
   SCIP_Bool*            cutoff,             /**< pointer to store if the constraint is infeasible */
   int*                  nbdchgs             /**< pointer to store the number of bound changes */
   )
{
   TLTREENODE** nodes;
   TLTREE* tltree;
   SCIP_CONSDATA* consdata;
   SCIP_HASHMAP* varhashmap;

   int* lcts;
   int* lct_ids;

   SCIP_Bool infeasible; 
   SCIP_Bool tightened;

   int makespan;             /* needed if we want to make the lct-updates instead of est */
   int nvars;
   int capacity;
   int j;

   assert(scip != NULL);
   assert(cons != NULL);

   SCIPdebugMessage("perform edge-finding detection\n");
   
   /* get constraint data */
   consdata = SCIPconsGetData(cons);
   assert(consdata != NULL);
   
   capacity = consdata->capacity;
   nvars = consdata->nvars;
   infeasible = FALSE;
   
   SCIP_CALL( SCIPallocBufferArray(scip, &lcts, nvars) );
   SCIP_CALL( SCIPallocBufferArray(scip, &lct_ids, nvars) );
   SCIP_CALL( SCIPallocBufferArray(scip, &nodes, nvars) );

   /* insert all jobs into the hashmap to find the corresponding index of the variable */
   SCIP_CALL( SCIPhashmapCreate(&varhashmap, SCIPblkmem(scip), SCIPcalcHashtableSize(nvars)) );

   /* compute makespan */
   makespan = 0;
   if( !forward )
   {
      for( j = 0; j < nvars; ++j )
      { 
         int tmp;

         tmp = convertBoundToInt(scip, SCIPvarGetUbLocal(consdata->vars[j])) + consdata->durations[j];
         makespan = MAX(makespan, tmp);
      }
   }

   /* initializing latest completion times and tree leaves */
   for( j = 0; j < nvars; ++j ) 
   {
      SCIP_VAR* var; 
      SCIP_Real est;
      int energy;

      var = consdata->vars[j];
      assert(var != NULL);
      assert(!SCIPhashmapExists(varhashmap, var));

      SCIP_CALL( SCIPhashmapInsert(varhashmap, var, (void*)(size_t)j) );

      if( forward )
      {
         lcts[j] = convertBoundToInt(scip, SCIPvarGetUbLocal(var)) + consdata->durations[j];
         lct_ids[j] = j;
         
         est = convertBoundToInt(scip, SCIPvarGetLbLocal(var) ) + j / (2.0 * nvars);
         energy = consdata->demands[j] * consdata->durations[j];
      } 
      else
      {
         lcts[j] = makespan - convertBoundToInt(scip, SCIPvarGetLbLocal(var));
         lct_ids[j] = j;
         
         est = makespan - convertBoundToInt(scip, SCIPvarGetUbLocal(var)) - consdata->durations[j] + j / (2.0 * nvars);
         energy = consdata->demands[j] * consdata->durations[j];
      }
         
      SCIP_CALL( tltreeCreateThetaLeaf(scip, &(nodes[j]), var, est, energy, consdata->capacity * (int)(est + 0.01) + energy) );
   }

   /* sort the latest completion times */
   SCIPsortIntInt(lcts, lct_ids, nvars);
   
   SCIP_CALL( tltreeCreateTree(scip, &tltree, nodes, lct_ids, nvars) );
   
   /* iterate over all jobs in non-decreasing order of lct_j */
   for( j = nvars-1; !infeasible && j >= 0; --j )
   {
      while( !infeasible && tltreeGetEnvelopTL(tltree) > capacity * lcts[j] )
      {
         TLTREENODE** omegaset;
         TLTREENODE* respleaf;
         
         int nelements;
         INFERINFO inferinfo;

         int est_omega;
         int lct_omega;

         int pos;
         int duration_pos;

         /* find out which variable var_i from Lambda is responsible */
         respleaf = tltreeFindResponsibleLeaf(tltree);

         assert(respleaf != NULL);
         assert(respleaf->left == NULL);
         assert(respleaf->right == NULL);
         assert(respleaf->var != NULL);
         assert(respleaf->energyL > 0);
         
         /* get position of responsible variable */
         pos = (int)(size_t)SCIPhashmapGetImage(varhashmap, respleaf->var);
         assert(pos < consdata->nvars);
         
         duration_pos = consdata->durations[pos];
         
         if( respleaf->value + duration_pos/*respleaf->energyL*/ >= lcts[j] )
         {
            SCIP_CALL( tltreeDeleteLeaf(scip, tltree, respleaf) );
            continue;
         }

         /* compute omega set */
         SCIP_CALL( SCIPallocBufferArray(scip, &omegaset, nvars - j) );         

         /* get omega set from tltree */
         SCIP_CALL( tltreeReportOmegaSet(scip, tltree, &omegaset, &nelements) );
         
         assert(nelements != 0);

         /* compute new earliest starting time */
         if( forward )
         {
            int newest;
            newest = computeNewEstOmegaset(scip, cons, varhashmap, respleaf, omegaset, nelements, lcts[j],
               &est_omega, &lct_omega);

            inferinfo = getInferInfo(PROPRULE_3_EDGEFINDING, est_omega, lct_omega);
            
            /* update variable's lower bound */
            SCIP_CALL( SCIPinferVarLbCons(scip, respleaf->var, (SCIP_Real)newest, 
                  cons, inferInfoToInt(inferinfo), TRUE, &infeasible, &tightened) );
         } 
         else 
         {
            int newlst;
            newlst = computeNewLstOmegaset(scip, cons, varhashmap, respleaf, omegaset, nelements, lcts[j], 
               makespan, &est_omega, &lct_omega);

            inferinfo = getInferInfo(PROPRULE_3_EDGEFINDING, est_omega, lct_omega);
            
            /* update variable's upper bound */
            SCIP_CALL( SCIPinferVarUbCons(scip, respleaf->var, (SCIP_Real)newlst, 
                  cons, inferInfoToInt(inferinfo), TRUE, &infeasible, &tightened) );
         }
         
         /* if node can be cutoff, start conflict analysis */
         if( infeasible )
         {
            SCIP_CALL( initializeConflictAnalysisEdgeFinding(scip, cons, respleaf->var, inferinfo) );
            *cutoff = TRUE;
         }

         /* count number of tightened bounds */
         if( tightened )
            ++(*nbdchgs);
         
         /* free omegaset array */
         SCIPfreeBufferArray(scip, &omegaset);

         /* delete responsible leaf from lambda */
         SCIP_CALL( tltreeDeleteLeaf(scip, tltree, respleaf) );
      }
      
      /* change set of job j from Theta to Lambda */
      SCIP_CALL( tltreeTransformLeafTtoL(scip, tltree, nodes[lct_ids[j]]) );
   }
   
   /* free theta tree */
   SCIP_CALL( freeTltree(scip, &tltree) );

   for( j = 0; j < nvars; ++j )
   {
      if( nodes[j] != NULL )
      {
         SCIP_CALL( freeTltreeLeaf(scip, &(nodes[j])) );
      }
   }

   /* free hashmap */
   SCIPhashmapFree(&varhashmap);
   
   /* free buffer arrays */
   SCIPfreeBufferArray(scip, &nodes);   
   SCIPfreeBufferArray(scip, &lct_ids);
   SCIPfreeBufferArray(scip, &lcts);   

   return SCIP_OKAY;
}

/** checks whether the instance is infeasible due to overload, 
 *  see Vilim: CPAIOR 2009: Max Energy Filtering Algorithm for Discrete Cumulative Resources 
 */
static
SCIP_RETCODE checkOverload(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_CONS*            cons,               /**< constraint to be separated */
   SCIP_Bool*            cutoff              /**< pointer to store whether node can be cutoff */
   )
{
   THETATREENODE** nodes;
   THETATREE* thetatree;
   SCIP_CONSDATA* consdata;

   int* lcts;
   int* lct_ids;
   int nvars;
   int capacity;

   int j;

   assert(scip != NULL);
   assert(cons != NULL);
   assert(cutoff != NULL);
   
   /* get constraint data */
   consdata = SCIPconsGetData(cons);
   assert(consdata != NULL);

   capacity = consdata->capacity;
   nvars = consdata->nvars;
   
   SCIP_CALL( SCIPallocBufferArray(scip, &lcts, nvars) );
   SCIP_CALL( SCIPallocBufferArray(scip, &lct_ids, nvars) );
   SCIP_CALL( SCIPallocBufferArray(scip, &nodes, nvars) );
   
   /* initializing latest completion times */
   for( j = 0; j < nvars; ++j ) 
   {
      SCIP_VAR* var; 
      SCIP_Real est;
      int energy;

      var = consdata->vars[j];
      lcts[j] = convertBoundToInt(scip, SCIPvarGetUbLocal(var)) + consdata->durations[j];
      lct_ids[j] = j;

      est = convertBoundToInt(scip, SCIPvarGetLbLocal(var)) + j / (2.0 * nvars);
      energy = consdata->demands[j] * consdata->durations[j];

      SCIP_CALL( thetatreeCreateLeaf(scip, &(nodes[j]), var, est, 
            energy, consdata->capacity * (int)(est + 0.01) + energy ) );
   }

   /* sort the latest completion times */
   SCIPsortIntInt(lcts, lct_ids, nvars);

   SCIP_CALL( createThetaTree(scip, &thetatree) );
   
   /* iterate over all jobs in non-decreasing order of lct_j */
   for( j = 0; j < nvars && !(*cutoff); ++j )
   {
      SCIP_Bool inserted;
      SCIP_CALL( thetatreeInsertLeaf(scip, thetatree, nodes[lct_ids[j]], &inserted) );
      assert(inserted);

      if( thetaTreeGetEnvelop(thetatree) > capacity * lcts[j] )
      {
         /*@todo: start conflict analysis, compute conflicting set */
         SCIPdebugMessage("Overload detected! Node can be cut off @todo: start conflict analysis\n");
         *cutoff = TRUE;
      }
   }

   /* free theta tree */
   SCIP_CALL( freeThetaTree(scip, &thetatree) );

   for( j = 0; j < nvars; ++j )
   {
      if( nodes[j] != NULL )
      {
         SCIP_CALL( freeThetaTreeLeaf(scip, &(nodes[j])) );
      }
   }
   
   /* free buffer arrays */
   SCIPfreeBufferArray(scip, &nodes);   
   SCIPfreeBufferArray(scip, &lct_ids);
   SCIPfreeBufferArray(scip, &lcts);   

   return SCIP_OKAY;
}

/** remove jobs which have a duration or demand of zero */
static
SCIP_RETCODE removeIrrelevantJobs(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_CONS*            cons                /**< constraint to propagate */
   )
{
   SCIP_CONSDATA* consdata;
   int j;
   
   assert(scip != NULL);
   assert(cons != NULL);
   
   SCIPdebugMessage("check cumulative constraint <%s> for irrelevant jobs\n", SCIPconsGetName(cons));

   consdata = SCIPconsGetData(cons);
   assert(consdata != NULL);

   j = 0;

   while( j < consdata->nvars )
   {
      if( consdata->demands[j] == 0 || consdata->durations[j] == 0 )
      {
         SCIP_CALL( unlockRounding(scip, cons, consdata->vars[j]) );
         SCIP_CALL( SCIPreleaseCons(scip, &consdata->linkingconss[j]) );
         consdata->nvars--;
         
         if( j < consdata->nvars )
         {
            consdata->vars[j] = consdata->vars[consdata->nvars];
            consdata->demands[j] = consdata->demands[consdata->nvars];
            consdata->durations[j]= consdata->durations[consdata->nvars];
            consdata->linkingconss[j]= consdata->linkingconss[consdata->nvars];
         }
      }
      else
         j++;
   }

   return SCIP_OKAY;
}

/** propagates the given constraint */
static
SCIP_RETCODE propagateCons(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_CONS*            cons,               /**< constraint to propagate */
   SCIP_Bool             usebinvars,         /**< is the binary representation used --> holes can be propagated */
   SCIP_Bool             usecoretimes,       /**< should core times be propagated */
   SCIP_Bool             usecoretimesholes,  /**< should core times be propagated to detect holes? */
   SCIP_Bool             useedgefinding,     /**< should edge finding be performed */
   SCIP_Bool             useenergeticreasoning,     /**< should energetic reasoning be performed */
   SCIP_Bool*            cutoff,             /**< pointer to store if we detected a cut off (infeasibility) */
   int*                  nchgbds,            /**< pointer to store the number of bound changes */
   int*                  ndelconss           /**< pointer to store the number of deleted constraints */
   )
{
   SCIP_Bool redundant;

   assert(ndelconss != NULL);

   redundant = FALSE;
   /**@todo avoid always sorting the variable array */
    
   /* check if the constraint is redundant */
   SCIP_CALL( consCheckRedundancy(scip, cons, &redundant) );

   if( redundant )
   {
      SCIPdebugMessage("%s deletes cumulative constraint <%s> since it is redundant\n", SCIPgetDepth(scip) == 0 ? "globally" : "locally", SCIPconsGetName(cons));

      SCIP_CALL( SCIPdelConsLocal(scip, cons) );
      (*ndelconss)++;

      return SCIP_OKAY;
   }

   
   /* propagate the job cores until nothing else can be detected */
   if( !(*cutoff) && usecoretimes )
   {
      SCIP_CALL( propagateCores(scip, cons, cutoff, nchgbds) );
   }
    
    
   /* check whether propagating the cores and creating holes helps */
   if( !(*cutoff) && usebinvars && usecoretimesholes )
   {
      /* experimentally inefficient, but possible to be turned on */
      SCIP_CALL( propagateCoresForHoles(scip, cons, cutoff, nchgbds) );
   }

   if( !(*cutoff) && useedgefinding )
   {
      /* check for overload, which may result in a cutoff */
      SCIP_CALL( checkOverload(scip, cons, cutoff) );

      if( !(*cutoff) )
      {
         /* perform edge-finding for lower bounds */
         SCIP_CALL( performEdgeFindingDetection(scip, cons, TRUE, cutoff, nchgbds) );
      }
      if( !(*cutoff) )
      {
         /* perform edge-finding for upper bounds */
         SCIP_CALL( performEdgeFindingDetection(scip, cons, FALSE, cutoff, nchgbds) );
      }
   }
    
   if( !(*cutoff) && useenergeticreasoning )
   {
      /* perform energetic reasoning */
      SCIP_CALL( performEnergeticReasoning(scip, cons, cutoff, nchgbds) );
   }
    
   return SCIP_OKAY;
}

/** checks constraint for violation, and adds it as a cut if possible */
static
SCIP_RETCODE separateCons(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_CONS*            cons,               /**< cumulative constraint to be separated */
   SCIP_SOL*             sol,                /**< primal CIP solution, NULL for current LP solution */
   SCIP_Bool*            cutoff,             /**< pointer to store TRUE, if a cutoff was found */
   SCIP_Bool*            reducedom,          /**< pointer to store TRUE, if a domain was reduced */
   SCIP_Bool*            separated           /**< pointer to store TRUE, if a cut was found */
   )
{  /*lint --e{715}*/
   SCIP_CONSDATA* consdata;
   SCIP_ROW* row;
   SCIP_Real minfeasibility;
   SCIP_Bool useall;
   int ncuts;
   int r;

   assert(scip != NULL);
   assert(cons != NULL);
   
   consdata = SCIPconsGetData(cons);
   assert(consdata != NULL);

   SCIPdebugMessage("separate cumulative constraint <%s>\n", SCIPconsGetName(cons));

   if( consdata->demandrows == NULL )
   {
      SCIP_CALL( createRelaxation(scip, cons, FALSE) );  
   }

   minfeasibility = SCIPinfinity(scip);
   row = NULL;
   useall = FALSE;
   ncuts = 0;
   
   /* check each row that is not contained in LP */
   for( r = 0; r < consdata->ndemandrows; ++r )
   {
      if( !SCIProwIsInLP(consdata->demandrows[r]) )
      {
         SCIP_Real feasibility;

         if( sol != NULL )
            feasibility = SCIPgetRowSolFeasibility(scip, consdata->demandrows[r], sol);
         else
            feasibility = SCIPgetRowLPFeasibility(scip, consdata->demandrows[r]);
         
         if( useall )
         {
            if( SCIPisFeasNegative(scip, feasibility) )
            {
               SCIP_CALL( SCIPaddCut(scip, sol,  consdata->demandrows[r], FALSE) );
               ncuts++;
            }
         }
         else
         {
            if( minfeasibility > feasibility )
            {
               minfeasibility = feasibility;
               row = consdata->demandrows[r];
            }
         }
      }
   }

   if( !useall && SCIPisFeasNegative(scip, minfeasibility)  ) 
   {
      SCIPdebugMessage("cumulative constraint <%s> separated cut with feasibility <%g>\n", 
         SCIPconsGetName(cons), minfeasibility);

      assert(row != NULL);
      SCIP_CALL( SCIPaddCut(scip, sol, row, FALSE) );
      
      /* if successful, reset age of constraint */
      SCIP_CALL( SCIPresetConsAge(scip, cons) );
      (*separated) = TRUE;
   }
   else if( ncuts > 0 )
   {
      /* if successful, reset age of constraint */
      SCIP_CALL( SCIPresetConsAge(scip, cons) );
      (*separated) = TRUE;
   }  
   
   return SCIP_OKAY;
}
 
/** checks constraint for violation, and adds it as a cut if possible */ 
static 
SCIP_RETCODE separateCoverCutsCons( 
   SCIP*                 scip,               /**< SCIP data structure */ 
   SCIP_CONS*            cons,               /**< logic or constraint to be separated */ 
   SCIP_SOL*             sol,                /**< primal CIP solution, NULL for current LP solution */ 
   SCIP_Bool*            separated           /**< pointer to store TRUE, if a cut was found */ 
   ) 
{ 
   SCIP_CONSDATA* consdata; 
   SCIP_ROW* row;
   SCIP_Real minfeasibility;
   int r; 
 
   assert(scip != NULL); 
   assert(cons != NULL); 
 
   consdata = SCIPconsGetData(cons); 
   assert(consdata != NULL); 
 
   SCIPdebugMessage("separate cumulative constraint <%s>\n", SCIPconsGetName(cons)); 
   
   if( !consdata->covercuts ) 
   { 
      SCIP_CALL( createCoverCuts(scip, cons) );   
   } 

   row = NULL;
   minfeasibility = SCIPinfinity(scip);

   /* check each row of small covers that is not contained in LP */ 
   for( r = 0; r < consdata->nscoverrows; ++r ) 
   { 
      if( !SCIProwIsInLP(consdata->scoverrows[r]) ) 
      { 
         SCIP_Real feasibility; 
          
         assert(consdata->scoverrows[r] != NULL); 
         if( sol != NULL ) 
            feasibility = SCIPgetRowSolFeasibility(scip, consdata->scoverrows[r], sol); 
         else 
            feasibility = SCIPgetRowLPFeasibility(scip, consdata->scoverrows[r]); 
          
         if( minfeasibility > feasibility )
         {
            minfeasibility = feasibility;
            row =  consdata->scoverrows[r];
         }
      } 
   } 
 
   if( SCIPisFeasNegative(scip, minfeasibility) )  
   { 
      SCIPdebugMessage("cumulative constraint <%s> separated 1 cover cut with feasibility %g\n", 
         SCIPconsGetName(cons), minfeasibility); 

      assert(row != NULL);
      SCIP_CALL( SCIPaddCut(scip, sol, row, FALSE) ); 
       
      /* if successful, reset age of constraint */ 
      SCIP_CALL( SCIPresetConsAge(scip, cons) ); 
      (*separated) = TRUE; 
   } 

   minfeasibility = SCIPinfinity(scip);
   row = NULL;
   
   /* check each row of small covers that is not contained in LP */  
   for( r = 0; r < consdata->nbcoverrows; ++r )  
   {  
      if( !SCIProwIsInLP(consdata->bcoverrows[r]) )  
      {  
         SCIP_Real feasibility;  
           
         assert(consdata->bcoverrows[r] != NULL); 
         if( sol != NULL )  
            feasibility = SCIPgetRowSolFeasibility(scip, consdata->bcoverrows[r], sol);  
         else  
            feasibility = SCIPgetRowLPFeasibility(scip, consdata->bcoverrows[r]);  
           
         if( minfeasibility > feasibility )
         {
            minfeasibility = feasibility;
            row =  consdata->bcoverrows[r];
         }
      }  
   }  

   if( SCIPisFeasNegative(scip, minfeasibility) )  
   { 
      SCIPdebugMessage("cumulative constraint <%s> separated 1 cover cut with feasibility %g\n", 
         SCIPconsGetName(cons), minfeasibility); 

      assert(row != NULL);
      SCIP_CALL( SCIPaddCut(scip, sol, row, FALSE) ); 
       
      /* if successful, reset age of constraint */ 
      SCIP_CALL( SCIPresetConsAge(scip, cons) ); 
      (*separated) = TRUE; 
   } 
    
   return SCIP_OKAY; 
} 

/** collect all integer variable which belong to jobs which can run at the point of interest */
static
SCIP_RETCODE collectIntVars(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_CONSDATA*        consdata,           /**< constraint data */
   SCIP_VAR***           activevars,         /**< jobs that are currently running */
   int*                  startindices,       /**< permutation with rspect to the start times */
   int                   curtime,            /**< current point in time */
   int                   nstarted,           /**< number of jobs that start before the curtime or at curtime */
   int                   nfinished,          /**< number of jobs that finished before curtime or at curtime */
   SCIP_Bool             lower,              /**< shall cuts be created due to lower or upper bounds? */
   int*                  lhs                 /**< lhs for the new row sum of lbs + minoffset */
   )
{
   SCIP_VAR* var;
   int startindex;
   int endtime;
   int duration;
   int demand;
   int starttime;

   int varidx;
   int sumofstarts;
   int mindelta;
   int counter;

   counter = 0;
   sumofstarts = 0;

   mindelta = INT_MAX; 

   startindex = nstarted - 1;

   /* search for the (nstarted - nfinished) jobs which are active at curtime */
   while( nstarted - nfinished > counter )
   {
      assert(startindex >= 0);

      /* collect job information */
      varidx = startindices[startindex];
      assert(varidx >= 0 && varidx < consdata->nvars);
      
      var = consdata->vars[varidx];
      duration = consdata->durations[varidx];
      assert(duration > 0);
      demand = consdata->demands[varidx];
      assert(demand > 0);
      assert(var != NULL);
      
      starttime = lower ? convertBoundToInt(scip, SCIPvarGetLbLocal(var)) : convertBoundToInt(scip, SCIPvarGetUbLocal(var));
      endtime = starttime + duration;
      
      /* check the end time of this job is larger than the curtime; in this case the job is still running */
      if( endtime > curtime )
      {
         (*activevars)[counter] = var;
         sumofstarts += starttime;
         mindelta = MIN(mindelta, endtime - curtime);
         counter++;
      }
      
      startindex--;
   }

   assert(mindelta > 0);
   *lhs = lower ? sumofstarts + mindelta : sumofstarts - mindelta;

   return SCIP_OKAY;
}

/** initialize the sorted event point arrays */
static 
void createSortedEventpointsSol(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_CONSDATA*        consdata,           /**< constraint data */
   SCIP_SOL*             sol,                /**< primal CIP solution, NULL for current LP solution */
   int*                  starttimes,         /**< array to store sorted start events */
   int*                  endtimes,           /**< array to store sorted end events */
   int*                  startindices,       /**< permutation with rspect to the start times */
   int*                  endindices,         /**< permutation with rspect to the end times */
   int*                  nvars,              /**< number of variables that are integral */
   SCIP_Bool             lower               /**< shall the constraints be derived for lower or upper bounds? */
   )
{
   SCIP_VAR* var;
   int tmpnvars;
   int j;

   tmpnvars = consdata->nvars;
   *nvars = 0;

   /* assign variables, start and endpoints to arrays */
   for ( j = 0; j < tmpnvars; ++j )
   {
      var = consdata->vars[j];
      assert(var != NULL);

      if( lower )
      {
         /* only consider jobs that are at their lower or upper bound */
         if( !SCIPisFeasIntegral(scip, SCIPgetSolVal(scip, sol, var))
            || !SCIPisFeasEQ(scip, SCIPgetSolVal(scip, sol, var), SCIPvarGetLbLocal(var)) )
            continue;
         
         if( consdata->durations[j] == 0 || consdata->demands[j] == 0 )
            continue;

         starttimes[*nvars] = convertBoundToInt(scip, SCIPgetSolVal(scip, sol, var));
         startindices[*nvars] = j;
         
         endtimes[*nvars] =  starttimes[*nvars] + consdata->durations[j];
         endindices[*nvars] = j;
         
         (*nvars) = *nvars + 1;

         SCIPdebugMessage("lower bounds are considered:\n");
         SCIPdebugMessage("%d: job[%d] starttime %d, endtime = %d, demand = %d\n ", *nvars-1, 
            startindices[*nvars-1], starttimes[*nvars-1], starttimes[*nvars-1] + consdata->durations[startindices[*nvars-1]],
            consdata->demands[startindices[*nvars-1]]);
      } 
      else
      {
         if( !SCIPisFeasIntegral(scip, SCIPgetSolVal(scip, sol, var))
            || !SCIPisFeasEQ(scip, SCIPgetSolVal(scip, sol, var), SCIPvarGetUbLocal(var)) )
            continue;
         
         starttimes[*nvars] = convertBoundToInt(scip, SCIPgetSolVal(scip, sol, var));
         startindices[*nvars] = j;
         
         endtimes[*nvars] =  starttimes[*nvars] + consdata->durations[j];
         endindices[*nvars] = j;
         
         (*nvars) = *nvars + 1;

         SCIPdebugMessage("upper bounds are considered:\n");
         SCIPdebugMessage("%d: job[%d] starttime %d, endtime = %d, demand = %d\n ", *nvars-1, 
            startindices[*nvars-1], starttimes[*nvars-1], starttimes[*nvars-1] + consdata->durations[startindices[*nvars-1]],
            consdata->demands[startindices[*nvars-1]]);
      }
   }
   
   /* sort the arrays not-decreasing according to startsolvalues and endsolvalues (and sort the indices in the same way) */
   SCIPsortIntInt(starttimes, startindices, *nvars);
   SCIPsortIntInt(endtimes, endindices, *nvars);

#ifdef SCIP_DEBUG
   SCIPdebugMessage("sorted output\n");
   for ( j = 0; j < *nvars; ++j )
   {
      SCIPdebugMessage("%d: job[%d] starttime %d, endtime = %d, demand = %d\n", j, 
         startindices[j], starttimes[j], starttimes[j] + consdata->durations[startindices[j]],
         consdata->demands[startindices[j]]);
   }
   
   for ( j = 0; j < *nvars; ++j )
   {  
      SCIPdebugMessage("%d: job[%d] endtime %d,  demand = %d\n", j, endindices[j], endtimes[j], 
         consdata->demands[endindices[j]]);
   }
   SCIPdebugMessage("capacity = %d\n", consdata->capacity);
#endif
}


/** this method creats a row for time point curtime which insurse the capacity restriction of the cumulative constraint */
static
SCIP_RETCODE createCapacityRestrictionIntvars(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_CONS*            cons,               /**< constraint to be checked */
   SCIP_SOL*             sol,                /**< primal CIP solution, NULL for current LP solution */
   int*                  startindices,       /**< permutation with rspect to the start times */
   int                   curtime,            /**< current point in time */
   int                   nstarted,           /**< number of jobs that start before the curtime or at curtime */
   int                   nfinished,          /**< number of jobs that finished before curtime or at curtime */
   SCIP_Bool             lower               /**< shall cuts be created due to lower or upper bounds? */
   )
{
   SCIP_CONSDATA* consdata;
   char name[SCIP_MAXSTRLEN];
   int capacity;
   int lhs; /* left hand side of constraint */

   SCIP_VAR** activevars;
   SCIP_ROW* row;

   int v;
   
   assert(nstarted > nfinished);
   
   consdata = SCIPconsGetData(cons);
   assert(consdata != NULL);
   assert(consdata->nvars > 0);

   capacity = consdata->capacity;
   assert(capacity > 0);

   
   SCIP_CALL( SCIPallocBufferArray(scip, &activevars, nstarted-nfinished) );

   SCIP_CALL( collectIntVars(scip, consdata, &activevars, 
         startindices, curtime, nstarted, nfinished, lower, &lhs ) );

   if( lower )
   {
      (void)SCIPsnprintf(name, SCIP_MAXSTRLEN, "lower(%d)", curtime);

      SCIP_CALL( SCIPcreateEmptyRow(scip, &row, name, (SCIP_Real) lhs, SCIPinfinity(scip),  TRUE, FALSE, SCIPconsIsRemovable(cons)) );
   } 
   else
   {
      (void)SCIPsnprintf(name, SCIP_MAXSTRLEN, "upper(%d)", curtime);
      SCIP_CALL( SCIPcreateEmptyRow(scip, &row, name, -SCIPinfinity(scip), (SCIP_Real) lhs, TRUE, FALSE, SCIPconsIsRemovable(cons)) );
   }
   SCIP_CALL( SCIPcacheRowExtensions(scip, row) );
   
   for( v = 0; v < nstarted - nfinished; ++v )
   {
      SCIP_CALL( SCIPaddVarToRow(scip, row, activevars[v], 1.) );
   }
   
   SCIP_CALL( SCIPflushRowExtensions(scip, row) );
   SCIPdebug( SCIP_CALL(SCIPprintRow(scip, row, NULL)) );
   
   SCIP_CALL( SCIPaddCut(scip, sol, row, TRUE) );
   
   SCIP_CALL( SCIPreleaseRow(scip, &row) );

   /* free buffers */
   SCIPfreeBufferArrayNull(scip, &activevars);

   return SCIP_OKAY;
}

/** checks constraint for violation, and adds it as a cut if possible */
static
SCIP_RETCODE separateConsOnIntegerVariables(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_CONS*            cons,               /**< cumulative constraint to be separated */
   SCIP_SOL*             sol,                /**< primal CIP solution, NULL for current LP solution */
   SCIP_Bool             lower,              /**< shall cuts be created according to lower bounds? */
   SCIP_Bool*            separated           /**< pointer to store TRUE, if a cut was found */
   )
{

   SCIP_CONSDATA* consdata;

   int* starttimes;         /* stores when each job is starting */
   int* endtimes;           /* stores when each job ends */
   int* startindices;       /* we will sort the startsolvalues, thus we need to know wich index of a job it corresponds to */
   int* endindices;         /* we will sort the endsolvalues, thus we need to know wich index of a job it corresponds to */
   
   int nvars;               /* number of activities for this constraint */
   int freecapacity;        /* remaining capacity */
   int curtime;             /* point in time which we are just checking */
   int endindex;            /* index of endsolvalues with: endsolvalues[endindex] > curtime */
   
   int j;

   assert(scip != NULL);
   assert(cons != NULL);

   consdata = SCIPconsGetData(cons);
   assert(consdata != NULL);

   nvars = consdata->nvars;

   /* if no activities are associated with this cumulative then this constraint is redundant */
   if( nvars == 0 )
      return SCIP_OKAY;
   
   assert(consdata->vars != NULL);

   SCIP_CALL( SCIPallocBufferArray(scip, &starttimes, nvars) );
   SCIP_CALL( SCIPallocBufferArray(scip, &endtimes, nvars) );
   SCIP_CALL( SCIPallocBufferArray(scip, &startindices, nvars) );
   SCIP_CALL( SCIPallocBufferArray(scip, &endindices, nvars) );

   SCIPdebugMessage("create sorted event points for cumulative constraint <%s> with %d jobs\n",
      SCIPconsGetName(cons), nvars);

   /* create event point arrays */
   createSortedEventpointsSol(scip, consdata, sol, starttimes, endtimes, startindices, endindices, &nvars, lower);

   endindex = 0;
   freecapacity = consdata->capacity;

   /* check each startpoint of a job whether the capacity is kept or not */
   /* only check those 'nvars' that are not fractional (only those were sorted in 'createsortedeventpointssol') */
   for( j = 0; j < nvars; ++j )
   {
      curtime = starttimes[j];
     
      /* remove the capacity requirments for all job which start at the curtime */
      subtractStartingJobDemands(consdata, curtime, starttimes, startindices, &freecapacity, &j, nvars);

      /* add the capacity requirments for all job which end at the curtime */
      addEndingJobDemands(consdata, curtime, endtimes, endindices, &freecapacity, &endindex, nvars);

      assert(freecapacity <= consdata->capacity);
      assert(endindex <= nvars);

      /* endindex - points to the next job which will finish */
      /* j - points to the last job that has been released */

      /* if free capacity is smaller than zero, then add rows to the LP */
      if( freecapacity < 0 )
      {
         /* create capacity restriction row for current event point */
         SCIP_CALL( createCapacityRestrictionIntvars(scip, cons, sol, startindices, curtime, j+1, endindex, lower) );
         *separated = TRUE;
      } 
   }

   /* free all buffer arrays */
   SCIPfreeBufferArray(scip, &endindices);   
   SCIPfreeBufferArray(scip, &startindices);
   SCIPfreeBufferArray(scip, &endtimes);
   SCIPfreeBufferArray(scip, &starttimes);
   
   return SCIP_OKAY;
}

/*
 * Callback methods of constraint handler
 */

/** copy method for constraint handler plugins (called when SCIP copies plugins) */
static
SCIP_DECL_CONSHDLRCOPY(conshdlrCopyCumulative)
{  /*lint --e{715}*/
   assert(scip != NULL);
   assert(conshdlr != NULL);
   assert(strcmp(SCIPconshdlrGetName(conshdlr), CONSHDLR_NAME) == 0);

   /* call inclusion method of constraint handler */
   SCIP_CALL( SCIPincludeConshdlrCumulative(scip) );
 
   *valid = TRUE;

   return SCIP_OKAY;
}

/** destructor of constraint handler to free constraint handler data (called when SCIP is exiting) */
static
SCIP_DECL_CONSFREE(consFreeCumulative)
{  /*lint --e{715}*/
   SCIP_CONSHDLRDATA* conshdlrdata;

   assert(conshdlr != NULL);
   assert(strcmp(SCIPconshdlrGetName(conshdlr), CONSHDLR_NAME) == 0);

   conshdlrdata = SCIPconshdlrGetData(conshdlr);
   assert(conshdlrdata != NULL);
   
   conshdlrdataFree(scip, &conshdlrdata);
   
   SCIPconshdlrSetData(conshdlr, NULL);

   return SCIP_OKAY;
}

/** initialization method of constraint handler (called after problem was transformed) */
#define consInitCumulative NULL

/** deinitialization method of constraint handler (called before transformed problem is freed) */
#define consExitCumulative NULL

/** presolving initialization method of constraint handler (called when presolving is about to begin) */
static
SCIP_DECL_CONSINITPRE(consInitpreCumulative)
{  
   SCIP_CONSHDLRDATA* conshdlrdata;
   SCIP_CONS* cons;
   int c; 

   assert(conshdlr != NULL);
   assert(strcmp(SCIPconshdlrGetName(conshdlr), CONSHDLR_NAME) == 0);
   assert(nconss == 0 || conss != NULL);
   assert(result != NULL);

   (*result) = SCIP_FEASIBLE;

   conshdlrdata = SCIPconshdlrGetData(conshdlr);
   assert(conshdlrdata != NULL);

   /* check all constraints for trivial feasibility  or infeasibility */
   for( c = 0; c < nconss; ++c )
   {
      cons = conss[c];
      assert(cons != NULL);
      
      if( !checkDemands(scip, cons) )
      {
         (*result) = SCIP_CUTOFF;
         break;
      }
   }   
   
   return SCIP_OKAY;
}

/** presolving deinitialization method of constraint handler (called after presolving has been finished) */
#define consExitpreCumulative NULL

/** solving process initialization method of constraint handler (called when branch and bound process is about to begin) */
#define consInitsolCumulative NULL

/** solving process deinitialization method of constraint handler (called before branch and bound process data is freed) */
static
SCIP_DECL_CONSEXITSOL(consExitsolCumulative)
{  /*lint --e{715}*/ 
   SCIP_CONSDATA* consdata;
   int c;
   
   assert(conshdlr != NULL);
   assert(strcmp(SCIPconshdlrGetName(conshdlr), CONSHDLR_NAME) == 0);

   /* release the rows of all constraints */
   for( c = 0; c < nconss; ++c )
   {
      consdata = SCIPconsGetData(conss[c]);
      assert(consdata != NULL);

      /* free rows */      
      SCIP_CALL( consdataFreeRows(scip, &consdata) );
   }
   
   return SCIP_OKAY;
}

/** frees specific constraint data */
static
SCIP_DECL_CONSDELETE(consDeleteCumulative)
{  /*lint --e{715}*/
   assert(conshdlr != NULL);
   assert(strcmp(SCIPconshdlrGetName(conshdlr), CONSHDLR_NAME) == 0);
   assert(consdata != NULL );
   assert(*consdata != NULL );

   /* free cumulative constraint data */
   SCIP_CALL( consdataFree(scip, consdata ) );

   return SCIP_OKAY;
}

/** transforms constraint data into data belonging to the transformed problem */
static
SCIP_DECL_CONSTRANS(consTransCumulative)
{  /*lint --e{715}*/
   SCIP_CONSDATA* sourcedata;
   SCIP_CONSDATA* targetdata;

   assert(conshdlr != NULL);
   assert(SCIPgetStage(scip) == SCIP_STAGE_TRANSFORMING);
   assert(sourcecons != NULL);
   assert(targetcons != NULL);

   sourcedata = SCIPconsGetData(sourcecons);
   assert(sourcedata != NULL);
   assert(sourcedata->demandrows == NULL);
   
   SCIPdebugMessage("transform cumulative constraint <%s>\n", SCIPconsGetName(sourcecons));
   
   /* create constraint data for target constraint */
   SCIP_CALL( consdataCreate(scip, &targetdata, sourcedata->vars, sourcedata->linkingconss, 
         sourcedata->durations, sourcedata->demands, sourcedata->nvars, sourcedata->capacity) );

   /* create target constraint */
   SCIP_CALL( SCIPcreateCons(scip, targetcons, SCIPconsGetName(sourcecons), conshdlr, targetdata,
         SCIPconsIsInitial(sourcecons), SCIPconsIsSeparated(sourcecons), SCIPconsIsEnforced(sourcecons),
         SCIPconsIsChecked(sourcecons), SCIPconsIsPropagated(sourcecons),
         SCIPconsIsLocal(sourcecons), SCIPconsIsModifiable(sourcecons),
         SCIPconsIsDynamic(sourcecons), SCIPconsIsRemovable(sourcecons), SCIPconsIsStickingAtNode(sourcecons)) );

   return SCIP_OKAY;
}

/** LP initialization method of constraint handler */
static
SCIP_DECL_CONSINITLP(consInitlpCumulative)
{ 
   SCIP_CONSHDLRDATA* conshdlrdata;
   int c;

   assert(strcmp(SCIPconshdlrGetName(conshdlr), CONSHDLR_NAME) == 0);
   assert(conshdlr != NULL);
   conshdlrdata = SCIPconshdlrGetData(conshdlr);
   assert(conshdlrdata != NULL);
   
   SCIPdebugMessage("initialize LP relaxation for %d cumulative constraints\n", nconss);

   if( conshdlrdata->usebinvars )
   {
      /* add rows to LP */
      for( c = 0; c < nconss; ++c )
      {
         assert(SCIPconsIsInitial(conss[c]));
         SCIP_CALL( addRelaxation(scip, conss[c], conshdlrdata->cutsasconss) );
         
         if( conshdlrdata->cutsasconss )
         {
            SCIP_CALL( SCIPrestartSolve(scip) );
         }
      }
   } 

   /**@todo if we want to use only the integer variables; only these will be in cuts 
    *       create some initial cuts */
   
   return SCIP_OKAY;
}

/** separation method of constraint handler for LP solutions */
static
SCIP_DECL_CONSSEPALP(consSepalpCumulative)
{  
   SCIP_CONSHDLRDATA* conshdlrdata;
   SCIP_Bool cutoff;
   SCIP_Bool reducedom;
   SCIP_Bool separated;
   int c;
   
   SCIPdebugMessage("consSepalpCumulative\n");
   
   assert(conshdlr != NULL);
   assert(strcmp(SCIPconshdlrGetName(conshdlr), CONSHDLR_NAME) == 0);
   assert(nconss == 0 || conss != NULL);
   assert(result != NULL);
   conshdlrdata = SCIPconshdlrGetData(conshdlr);
   assert(conshdlrdata != NULL);

   SCIPdebugMessage("separating %d/%d cumulative constraints\n", nusefulconss, nconss);

   cutoff = FALSE;
   reducedom = FALSE;
   separated = FALSE;
   (*result) = SCIP_DIDNOTFIND; 
   
   if( conshdlrdata->usebinvars )
   {      
      /* check all useful cumulative constraints for feasibility  */
      for( c = 0; c < nusefulconss && !reducedom && !cutoff; ++c )
      {
         SCIP_CALL( separateCons(scip, conss[c], NULL, &cutoff, &reducedom, &separated) );
      }
      
      if( !cutoff && !reducedom && conshdlrdata->usecovercuts ) 
      { 
         for( c = 0; c < nusefulconss; ++c )
         {
            SCIP_CALL( separateCoverCutsCons(scip, conss[c], NULL, &separated) ); 
         }
      }
   }
   else 
   {
      /* separate cuts containing only integer variables */
      for( c = 0; c < nusefulconss; ++c )
      {
         SCIP_CALL( separateConsOnIntegerVariables(scip, conss[c], NULL, TRUE, &separated) );
         SCIP_CALL( separateConsOnIntegerVariables(scip, conss[c], NULL, FALSE, &separated) );
      }
   }
   
   if( cutoff )
      *result = SCIP_CUTOFF;
   else if( reducedom )
      *result = SCIP_REDUCEDDOM;
   else if( separated )
      *result = SCIP_SEPARATED;

   return SCIP_OKAY;
}

/** separation method of constraint handler for arbitrary primal solutions */
static
SCIP_DECL_CONSSEPASOL(consSepasolCumulative)
{  /*lint --e{715}*/
   SCIP_CONSHDLRDATA* conshdlrdata;
   SCIP_Bool cutoff;
   SCIP_Bool reducedom;
   SCIP_Bool separated;
   int c;

   assert(conshdlr != NULL);
   assert(strcmp(SCIPconshdlrGetName(conshdlr), CONSHDLR_NAME) == 0);
   assert(nconss == 0 || conss != NULL);
   assert(result != NULL);

   SCIPdebugMessage("separating %d/%d cumulative constraints\n", nusefulconss, nconss);

   conshdlrdata = SCIPconshdlrGetData(conshdlr);
   assert(conshdlrdata != NULL);

   cutoff = FALSE;
   reducedom = FALSE;
   separated = FALSE;
   (*result) = SCIP_DIDNOTFIND;
   
   if( conshdlrdata->usebinvars )
   {
      /* check all useful cumulative constraints for feasibility  */
      for( c = 0; c < nusefulconss && !cutoff && !reducedom; ++c )
      {
         SCIP_CALL( separateCons(scip, conss[c], NULL, &cutoff, &reducedom, &separated) );
      } 
      
      if( !cutoff && !reducedom && conshdlrdata->usecovercuts )  
      {  
         for( c = 0; c < nusefulconss; ++c )
         {
            SCIP_CALL( separateCoverCutsCons(scip, conss[c], sol, &separated) );  
         }
      }
   } 
   else 
   {
      /* separate cuts containing only integer variables */
      for( c = 0; c < nusefulconss; ++c )
      {
         SCIP_CALL( separateConsOnIntegerVariables(scip, conss[c], sol, TRUE, &separated) );
         SCIP_CALL( separateConsOnIntegerVariables(scip, conss[c], sol, FALSE, &separated) );
      }
   }
   
   if( cutoff )
      *result = SCIP_CUTOFF;
   else if( reducedom )
      *result = SCIP_REDUCEDDOM;
   else if( separated )
      *result = SCIP_SEPARATED;
   
   return SCIP_OKAY;
}

/** constraint enforcing method of constraint handler for LP solutions */
static
SCIP_DECL_CONSENFOLP(consEnfolpCumulative)
{  /*lint --e{715}*/
   SCIP_CONSHDLRDATA* conshdlrdata;
   SCIP_Bool cutoff;
   SCIP_Bool reducedom;
   SCIP_Bool separated;
   int c;
   
   assert(conshdlr != NULL);
   assert(strcmp(SCIPconshdlrGetName(conshdlr), CONSHDLR_NAME) == 0);
   assert(nconss == 0 || conss != NULL);
   assert(result != NULL);
   conshdlrdata = SCIPconshdlrGetData(conshdlr);
   assert(conshdlrdata != NULL);

   if( solinfeasible )
   {
      *result = SCIP_INFEASIBLE;
      return SCIP_OKAY;
   }

   cutoff = FALSE;
   reducedom = FALSE;
   separated = FALSE;
   
   SCIPdebugMessage("LP enforcing %d useful resource constraints of %d constraints\n", nusefulconss, nconss);
   
   if( conshdlrdata->usebinvars )
   {
      
      /* check all useful cumulative constraints for feasibility */
      for( c = 0; c < nusefulconss && !cutoff && !reducedom; ++c )
      {
         SCIP_CALL( separateCons(scip, conss[c], NULL, &cutoff, &reducedom, &separated) );
      }
      
      /* check all obsolete cumulative constraints for feasibility */
      for( c = nusefulconss; c < nconss && !cutoff && !reducedom && !separated; ++c )
      {
         SCIP_CALL( separateCons(scip, conss[c], NULL, &cutoff, &reducedom, &separated) );
      }
      
      if( cutoff )
         *result = SCIP_CUTOFF;
      else if( reducedom )
         *result = SCIP_REDUCEDDOM;
      else if( separated )
         *result = SCIP_SEPARATED;
      else
         (*result) = SCIP_FEASIBLE;
   } 
   else
   {
      /* it is no longer clear how to forbid a solution by cuts on ibnteger variables -> only check solution */
      SCIP_Bool violated;

      violated = FALSE;
      
      for( c = 0; c < nconss && !violated; ++c )
      {
         SCIP_CALL( checkCons(scip, conss[c], NULL, &violated, FALSE) );
      }
      
      if( violated )
         *result = SCIP_INFEASIBLE;
      else
         *result = SCIP_FEASIBLE;
   }
   
   return SCIP_OKAY;
}

/** constraint enforcing method of constraint handler for pseudo solutions */
static
SCIP_DECL_CONSENFOPS(consEnfopsCumulative)
{  /*lint --e{715}*/
   SCIP_Bool violated;
   int c;

   SCIPdebugMessage("method: enforce pseudo solution\n");

   assert(conshdlr != NULL);
   assert(strcmp(SCIPconshdlrGetName(conshdlr), CONSHDLR_NAME) == 0);
   assert(nconss == 0 || conss != NULL);
   assert(result != NULL);

   if( objinfeasible )
   {
      *result = SCIP_DIDNOTRUN;
      return SCIP_OKAY;
   }

   violated = FALSE;

   (*result) = SCIP_FEASIBLE;
   for( c = 0; c < nconss && !violated; ++c )
   {
      SCIP_CALL( checkCons(scip, conss[c], NULL, &violated, FALSE) );
   }
   
   if( violated )
      *result = SCIP_INFEASIBLE;
   else
      *result = SCIP_FEASIBLE;
   
   return SCIP_OKAY;
}

/** feasibility check method of constraint handler for integral solutions */
static
SCIP_DECL_CONSCHECK(consCheckCumulative)
{  /*lint --e{715}*/
   SCIP_Bool violated;
   int c;

   assert(conshdlr != NULL);
   assert(strcmp(SCIPconshdlrGetName(conshdlr), CONSHDLR_NAME) == 0);
   assert(nconss == 0 || conss != NULL);
   assert(result != NULL);

   violated = FALSE;

   for( c = 0; c < nconss && !violated; ++c )
   {
      SCIP_CALL( checkCons(scip, conss[c], sol, &violated, printreason) );
   }
   
   if( violated )
      *result = SCIP_INFEASIBLE;
   else
      *result = SCIP_FEASIBLE;
   
   return SCIP_OKAY;
}

/** domain propagation method of constraint handler */
static
SCIP_DECL_CONSPROP(consPropCumulative)
{ 
   SCIP_CONSHDLRDATA* conshdlrdata;
   SCIP_Bool cutoff;   
   int nchgbds;
   int ndelconss;
   int c;
   
   SCIPdebugMessage("propagate cumulative constraints\n");
   
   assert(conshdlr != NULL);
   assert(strcmp(SCIPconshdlrGetName(conshdlr), CONSHDLR_NAME) == 0);
   assert(nconss == 0 || conss != NULL);
   assert(result != NULL);

   conshdlrdata = SCIPconshdlrGetData(conshdlr);
   assert(conshdlrdata != NULL);

   nchgbds = 0;
   ndelconss = 0;
   cutoff = FALSE;
   (*result) = SCIP_DIDNOTRUN;
   
   /* propgate all useful constraints */
   for( c = 0; c < nusefulconss && !cutoff; ++c )
   {
      SCIP_CALL( propagateCons(scip, conss[c], 
            conshdlrdata->usebinvars, conshdlrdata->usecoretimes, conshdlrdata->usecoretimesholes, 
            conshdlrdata->useedgefinding, conshdlrdata->useenergeticreasoning, 
            &cutoff, &nchgbds, &ndelconss) );
   }

   if( !cutoff && nchgbds == 0 )
   {
      /* propgate all other constraints */
      for( c = nusefulconss; c < nconss && !cutoff; ++c )
      {
         SCIP_CALL( propagateCons(scip, conss[c], 
               conshdlrdata->usebinvars, conshdlrdata->usecoretimes, conshdlrdata->usecoretimesholes, 
               conshdlrdata->useedgefinding, conshdlrdata->useenergeticreasoning, 
               &cutoff, &nchgbds, &ndelconss) );
      }
   }
      
   if( cutoff )
   {
      SCIPdebugMessage("detected infeasible\n");
      *result = SCIP_CUTOFF;
   }
   else if( nchgbds > 0 )
   {
      SCIPdebugMessage("delete (locally) %d constraints and changed %d variable bounds\n", ndelconss, nchgbds);
      *result = SCIP_REDUCEDDOM;
   }
   else 
      *result = SCIP_DIDNOTFIND;
   
   return SCIP_OKAY;
}

/** presolving method of constraint handler */
static
SCIP_DECL_CONSPRESOL(consPresolCumulative)
{  /*lint --e{715}*/
   SCIP_CONSHDLRDATA* conshdlrdata;
   SCIP_Bool cutoff;
   int noldchgbds;
   int nolddelconss;
   int c;

   assert(conshdlr != NULL);
   assert(strcmp(SCIPconshdlrGetName(conshdlr), CONSHDLR_NAME) == 0);
   assert(scip != NULL);
   assert(result != NULL);

   SCIPdebugMessage("presolve cumulative constraints\n");
     
   conshdlrdata = SCIPconshdlrGetData(conshdlr);
   assert(conshdlrdata != NULL);
   
   *result = SCIP_DIDNOTRUN;

   noldchgbds = *nchgbds;
   nolddelconss = *ndelconss;
   cutoff = FALSE;
   
   /* in the first presolving round do something */
   if( nrounds == 0 )
   {
      for( c = 0; c < nconss; ++c )
      {
         /* remove jobs which have a duration or demand of zero */
         SCIP_CALL( removeIrrelevantJobs(scip, conss[c]) );
      }
   }
   
   /* process constraints */
   for( c = 0; c < nconss && !cutoff; ++c )
   {
      SCIPdebugMessage("presolving  constraint <%s>\n", SCIPconsGetName(conss[c]));
      
      SCIP_CALL( propagateCons(scip, conss[c], 
            conshdlrdata->usebinvars, conshdlrdata->usecoretimes, conshdlrdata->usecoretimesholes, 
            conshdlrdata->useedgefinding, TRUE/*conshdlrdata->useenergeticreasoning*/, 
            &cutoff, nchgbds, ndelconss) );
   }

   SCIPdebugMessage("delete %d constraints and changed %d variable bounds\n", *ndelconss-nolddelconss, *nchgbds-noldchgbds);

   if( cutoff )
      *result = SCIP_CUTOFF;
   else if( *nchgbds > noldchgbds || *ndelconss > nolddelconss )
      *result = SCIP_SUCCESS;
   else      
      *result = SCIP_DIDNOTFIND;
   
   return SCIP_OKAY;
}

/** propagation conflict resolving method of constraint handler */
static
SCIP_DECL_CONSRESPROP(consRespropCumulative)
{  /*lint --e{715}*/
   SCIP_CONSDATA* consdata;
   SCIP_VAR* var;

   INFERINFO struct_inferinfo;

   SCIP_Bool success;

   int nvars;
   int inferdemand;
   int inferduration;  /* needed for upperbound resolve process */
   int j;
   
   assert(conshdlr != NULL);
   assert(strcmp(SCIPconshdlrGetName(conshdlr), CONSHDLR_NAME) == 0);
   assert(scip != NULL);
   assert(result != NULL);
   assert(infervar != NULL);
   assert(bdchgidx != NULL);
   
   struct_inferinfo = intToInferInfo(inferinfo);

   SCIPdebugMessage("resolve propagation for variable <%s> and cumulative constraint <%s> with rule %d\n", 
      SCIPvarGetName(infervar), SCIPconsGetName(cons), inferInfoGetProprule(struct_inferinfo));
   
   /* process constraint */
   assert(cons != NULL);
   
   consdata = SCIPconsGetData(cons);
   assert(consdata != NULL);

   nvars = consdata->nvars;
   *result = SCIP_DIDNOTFIND;
   
   if( SCIPvarGetType(infervar) == SCIP_VARTYPE_INTEGER )
   {
      /* get duration and demand of inference variable */
      /*@todo hashmap for variables and durations would speed this up */
      inferdemand = 0;
      inferduration = 0;

      for( j = 0; j < nvars; ++j )
      {
         var = consdata->vars[j];
         assert(var != NULL);
         
         if( var == infervar )
         {
            inferdemand = consdata->demands[j];
            inferduration = consdata->durations[j];
            break;
         }
      }
      
      SCIPdebugMessage("variable <%s> has duration = %d and demand = %d\n", 
         SCIPvarGetName(infervar), inferduration, inferdemand);
      
      /* repropagation for core-times */
      if( inferInfoGetProprule(struct_inferinfo) == PROPRULE_1_CORETIMES )   
      {
         int leftbound;    
         int rightbound;   
         
         if( boundtype == SCIP_BOUNDTYPE_UPPER )
         {
            SCIPdebugMessage("variable <%s> bound changed from %g to %g\n", 
               SCIPvarGetName(infervar), SCIPvarGetUbAtIndex(infervar, bdchgidx, FALSE), 
               SCIPvarGetUbAtIndex(infervar, bdchgidx, TRUE));
            
            rightbound = convertBoundToInt(scip, SCIPvarGetUbAtIndex(infervar, bdchgidx, FALSE)) + inferduration;
            leftbound = convertBoundToInt(scip, SCIPvarGetUbAtIndex(infervar, bdchgidx, TRUE)) + inferduration;
            
            /* old upper bound of variable itself is responsible */
            SCIP_CALL( SCIPaddConflictUb( scip, infervar, bdchgidx ) );
         } 
         else
         {    
            assert(boundtype == SCIP_BOUNDTYPE_LOWER);      
            leftbound = convertBoundToInt(scip, SCIPvarGetLbAtIndex(infervar, bdchgidx, FALSE));
            rightbound = convertBoundToInt(scip, SCIPvarGetLbAtIndex(infervar, bdchgidx, TRUE));
            
            /* old lower bound of variable itself is responsible */
            SCIP_CALL( SCIPaddConflictLb( scip, infervar, bdchgidx ) );
            
         }
         assert(leftbound < rightbound);
      
         SCIP_CALL( analyzeConflictCoreTimesCumulative(scip, cons, infervar, leftbound, rightbound, inferduration, inferdemand, 
               boundtype, bdchgidx, &success) );
         assert(success);      
      }
      else  
      {
         /* repropagation for edge-finding or energetic reasoning */
         int oldbound;
         int newbound;

         SCIPdebugMessage("repropagate edge-finder or energetic reasoning!\n");

         if( boundtype == SCIP_BOUNDTYPE_LOWER )
         {
            SCIPdebugMessage("variable <%s> lower bound changed from %g to %g\n", 
               SCIPvarGetName(infervar), SCIPvarGetLbAtIndex(infervar, bdchgidx, FALSE), 
               SCIPvarGetLbAtIndex(infervar, bdchgidx, TRUE));

            oldbound = convertBoundToInt(scip, SCIPvarGetLbAtIndex(infervar, bdchgidx, FALSE));
            newbound = convertBoundToInt(scip, SCIPvarGetLbAtIndex(infervar, bdchgidx, TRUE));
            assert(oldbound < newbound);

            SCIP_CALL( SCIPaddConflictLb(scip, infervar, bdchgidx) );
            
            /* analyze the conflict */
            if( inferInfoGetProprule(struct_inferinfo) == PROPRULE_3_EDGEFINDING )
            {
               /* can search for small clauses if earliest start is in the interval */
               if( oldbound >= inferInfoGetEst(struct_inferinfo) )
               {
                  int inferdiff;
                  inferdiff = newbound - inferInfoGetEst(struct_inferinfo);
                  assert(inferdiff > 0);
                  SCIP_CALL( analyzeShortConflictEdgeFinding(scip, cons, infervar, struct_inferinfo,
                        inferdemand, inferduration, inferdiff, bdchgidx, &success) );
               } 
               else 
               {
                  SCIP_CALL( analyzeConflictEdgeFinding(scip, cons, infervar, struct_inferinfo,
                        bdchgidx, &success) );
               }
            } 
            else
            {
               assert(inferInfoGetProprule(struct_inferinfo) == PROPRULE_4_ENERGETICREASONING);
               
               SCIP_CALL( analyzeConflictEnergeticReasoning(scip, cons, infervar, struct_inferinfo,
                     bdchgidx, &success) );
            }
         }
         else /* now consider upper bound changes */
         {
            SCIPdebugMessage("variable <%s> upper bound changed from %g to %g\n", 
               SCIPvarGetName(infervar), SCIPvarGetUbAtIndex(infervar, bdchgidx, FALSE), 
               SCIPvarGetUbAtIndex(infervar, bdchgidx, TRUE));

            oldbound = convertBoundToInt(scip, SCIPvarGetUbAtIndex(infervar, bdchgidx, FALSE));
            newbound = convertBoundToInt(scip, SCIPvarGetUbAtIndex(infervar, bdchgidx, TRUE));
            assert(oldbound > newbound);

            SCIP_CALL( SCIPaddConflictUb(scip, infervar, bdchgidx) );
            
            /* analyze the conflict */
            if( inferInfoGetProprule(struct_inferinfo) == PROPRULE_3_EDGEFINDING )
            {
               /* can search for small clauses if latest completion time is in the interval */
               if( oldbound + inferduration<= inferInfoGetLct(struct_inferinfo) )
               {
                  int inferdiff;
                  inferdiff = inferInfoGetLct(struct_inferinfo) - newbound - inferduration;
                  assert(inferdiff > 0);
                  SCIP_CALL( analyzeShortConflictEdgeFinding(scip, cons, infervar, struct_inferinfo,
                        inferdemand, inferduration, inferdiff, bdchgidx, &success) );
               } 
               else 
               {
                  SCIP_CALL( analyzeConflictEdgeFinding(scip, cons, infervar, struct_inferinfo,
                        bdchgidx, &success) );
               }               
            } 
            else /* upper bound conflict analysis for energetic reasoning */
            {
               assert(inferInfoGetProprule(struct_inferinfo) == PROPRULE_4_ENERGETICREASONING);
               
               SCIP_CALL( analyzeConflictEnergeticReasoning(scip, cons, infervar, struct_inferinfo,
                     bdchgidx, &success) );
            }
         }
         assert(success);
      }
   }
   else  
   {
      /* repropagation for binary variables set to zero; inferinfo == position in array and excluded timepoint */
      SCIP_VAR* intvar;
      SCIP_VAR** binvars;

      int pos;
      int nbinvars;

      assert(SCIPvarGetType(infervar) == SCIP_VARTYPE_BINARY);
      assert(inferInfoGetProprule(struct_inferinfo) == PROPRULE_2_CORETIMEHOLES); 

      intvar = NULL;
      inferdemand = 0;

      pos = inferInfoGetEst(struct_inferinfo);
      assert(pos >= 0);
 
      /* get demand and integer variable of given inference variable */
      for( j = 0; j < nvars; ++j )
      {
         var = consdata->vars[j];
         assert(var != NULL);
         
         SCIP_CALL( SCIPgetBinvarsLinking(scip, SCIPgetConsLinking(scip, var), &binvars, &nbinvars) );
         
         if( binvars[pos] == infervar )
         {
            intvar = var;
            inferdemand = consdata->demands[j];
            break;
         }
      }      
      assert(intvar != NULL);
      assert(inferdemand > 0);

      SCIP_CALL( analyzeConflictCoreTimesBinvarsCumulative(scip, cons, infervar, intvar, 
            inferInfoGetLct(struct_inferinfo), inferdemand, bdchgidx, &success) );      
   }

   if( success )
      *result = SCIP_SUCCESS;

   return SCIP_OKAY;
}

/** variable rounding lock method of constraint handler */
static
SCIP_DECL_CONSLOCK(consLockCumulative)
{  /*lint --e{715}*/
   SCIP_CONSDATA* consdata;
   int v;

   assert(scip != NULL);
   assert(cons != NULL);

   consdata = SCIPconsGetData(cons);
   assert(consdata != NULL);

   for( v = 0; v < consdata->nvars; ++v )
   {
      /* the integer start variable should not get rounded in both direction  */
      assert(consdata->vars[v] != NULL);
      SCIP_CALL( SCIPaddVarLocks(scip, consdata->vars[v], nlockspos + nlocksneg, nlockspos + nlocksneg) );
   }

   return SCIP_OKAY;
}

/** constraint activation notification method of constraint handler */
#define consActiveCumulative NULL

/** constraint deactivation notification method of constraint handler */
#define consDeactiveCumulative NULL

/** constraint enabling notification method of constraint handler */
#define consEnableCumulative NULL

/** constraint disabling notification method of constraint handler */
#define consDisableCumulative NULL

/** constraint display method of constraint handler */
static
SCIP_DECL_CONSPRINT(consPrintCumulative)
{  /*lint --e{715}*/
   assert(scip != NULL);
   assert(conshdlr != NULL);
   assert(cons != NULL);

   consdataPrint(scip, SCIPconsGetData(cons), file);

   return SCIP_OKAY;
}

/** constraint copying method of constraint handler */
static
SCIP_DECL_CONSCOPY(consCopyCumulative)
{  /*lint --e{715}*/
   SCIP_CONSDATA* sourceconsdata;
   SCIP_VAR** sourcevars;
   SCIP_VAR** vars;
   const char* consname;

   int nvars;
   int v;

   sourceconsdata = SCIPconsGetData(sourcecons);
   assert(sourceconsdata != NULL);
   
   /* get variables of the source constraint */
   nvars = sourceconsdata->nvars;
   sourcevars = sourceconsdata->vars;

   (*valid) = TRUE;
   
   if( nvars == 0 )
      return SCIP_OKAY;
   
   /* allocate buffer array */
   SCIP_CALL( SCIPallocBufferArray(scip, &vars, nvars) );

   for( v = 0; v < nvars; ++v )
   {
      SCIP_CALL( SCIPgetVarCopy(sourcescip, scip, sourcevars[v], &vars[v], varmap, consmap, global) );
   }
   
   if( name != NULL )
      consname = name;
   else
      consname = SCIPconsGetName(sourcecons);
   
   /* copy the logic using the linear constraint copy method */
   SCIP_CALL( SCIPcreateConsCumulative(scip, cons, consname, nvars, vars, 
         sourceconsdata->durations, sourceconsdata->demands, sourceconsdata->capacity,
         initial, separate, enforce, check, propagate, local, modifiable, dynamic, removable, stickingatnode) );
   
   /* free buffer array */
   SCIPfreeBufferArray(scip, &vars);

   return SCIP_OKAY;
}

/** constraint parsing method of constraint handler */
#define consParseCumulative NULL

/*
 * constraint specific interface methods
 */

/** creates the handler for cumulative constraints and includes it in SCIP */
SCIP_RETCODE SCIPincludeConshdlrCumulative(
   SCIP*                 scip                /**< SCIP data structure */
   )
{
   SCIP_CONSHDLRDATA* conshdlrdata;

   /* create cumulative constraint handler data */
   SCIP_CALL( conshdlrdataCreate(scip, &conshdlrdata) );
   
   /* include constraint handler */
   SCIP_CALL( SCIPincludeConshdlr(scip, CONSHDLR_NAME, CONSHDLR_DESC,
         CONSHDLR_SEPAPRIORITY, CONSHDLR_ENFOPRIORITY, CONSHDLR_CHECKPRIORITY,
         CONSHDLR_SEPAFREQ, CONSHDLR_PROPFREQ, CONSHDLR_EAGERFREQ, CONSHDLR_MAXPREROUNDS,
         CONSHDLR_DELAYSEPA, CONSHDLR_DELAYPROP, CONSHDLR_DELAYPRESOL, CONSHDLR_NEEDSCONS,
         conshdlrCopyCumulative,
         consFreeCumulative, consInitCumulative, consExitCumulative,
         consInitpreCumulative, consExitpreCumulative, consInitsolCumulative, consExitsolCumulative,
         consDeleteCumulative, consTransCumulative, consInitlpCumulative,
         consSepalpCumulative, consSepasolCumulative, consEnfolpCumulative, consEnfopsCumulative, consCheckCumulative,
         consPropCumulative, consPresolCumulative, consRespropCumulative, consLockCumulative,
         consActiveCumulative, consDeactiveCumulative,
         consEnableCumulative, consDisableCumulative,
         consPrintCumulative, consCopyCumulative, consParseCumulative,
         conshdlrdata) );

   /* set default values for constraint handler data */
   conshdlrdata->lastsepanode = -1;

   /* add cumulative constraint handler parameters */
   SCIP_CALL( SCIPaddBoolParam(scip,
         "constraints/"CONSHDLR_NAME"/usebinvars", "should the binary representation be used?",
         &conshdlrdata->usebinvars, FALSE, DEFAULT_USEBINVARS, NULL, NULL) );
   SCIP_CALL( SCIPaddBoolParam(scip,
         "constraints/"CONSHDLR_NAME"/usecoretimes", "should coretimes be propagated?",
         &conshdlrdata->usecoretimes, FALSE, DEFAULT_USECORETIMES, NULL, NULL) );
   SCIP_CALL( SCIPaddBoolParam(scip,
         "constraints/"CONSHDLR_NAME"/usecoretimesholes", "should coretimes be propagated to detect holes?",
         &conshdlrdata->usecoretimesholes, FALSE, DEFAULT_USECORETIMESHOLES, NULL, NULL) );
   SCIP_CALL( SCIPaddBoolParam(scip,
         "constraints/"CONSHDLR_NAME"/localcuts", "should cuts be added only locally?",
         &conshdlrdata->localcuts, FALSE, DEFAULT_LOCALCUTS, NULL, NULL) );
   SCIP_CALL( SCIPaddBoolParam(scip,
         "constraints/"CONSHDLR_NAME"/usecovercuts", "should covering cuts be added every node?",
         &conshdlrdata->usecovercuts, FALSE, DEFAULT_USECOVERCUTS, NULL, NULL) );
   SCIP_CALL( SCIPaddBoolParam(scip,
         "constraints/"CONSHDLR_NAME"/useedgefinding", "should edge finding be used?",
         &conshdlrdata->useedgefinding, FALSE, DEFAULT_USEEDGEFINDING, NULL, NULL) );
   SCIP_CALL( SCIPaddBoolParam(scip,
         "constraints/"CONSHDLR_NAME"/useenergeticreasoning", "should energetic reasoning be used?",
         &conshdlrdata->useenergeticreasoning, FALSE, DEFAULT_USEENERGETICREASONING, NULL, NULL) );
   SCIP_CALL( SCIPaddBoolParam(scip,
         "constraints/"CONSHDLR_NAME"/cutsasconss", 
         "should the cumulative constraint create cuts as knapsack constraints?",
         &conshdlrdata->cutsasconss, FALSE, DEFAULT_CUTSASCONSS, NULL, NULL) );

   return SCIP_OKAY;
}

/** creates and captures a cumulative constraint */
SCIP_RETCODE SCIPcreateConsCumulative(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_CONS**           cons,               /**< pointer to hold the created constraint */
   const char*           name,               /**< name of constraint */
   int                   nvars,              /**< number of variables (jobs) */
   SCIP_VAR**            vars,               /**< array of integer variable which corresponds to starting times for a job */
   int*                  durations,          /**< array containing corresponding durations */
   int*                  demands,            /**< array containing corresponding demands */
   int                   capacity,           /**< available cumulative capacity */
   SCIP_Bool             initial,            /**< should the LP relaxation of constraint be in the initial LP?
                                              *   Usually set to TRUE. Set to FALSE for 'lazy constraints'. */
   SCIP_Bool             separate,           /**< should the constraint be separated during LP processing?
                                              *   Usually set to TRUE. */
   SCIP_Bool             enforce,            /**< should the constraint be enforced during node processing?
                                              *   TRUE for model constraints, FALSE for additional, redundant constraints. */
   SCIP_Bool             check,              /**< should the constraint be checked for feasibility?
                                              *   TRUE for model constraints, FALSE for additional, redundant constraints. */
   SCIP_Bool             propagate,          /**< should the constraint be propagated during node processing?
                                              *   Usually set to TRUE. */
   SCIP_Bool             local,              /**< is constraint only valid locally?
                                              *   Usually set to FALSE. Has to be set to TRUE, e.g., for branching constraints. */
   SCIP_Bool             modifiable,         /**< is constraint modifiable (subject to column generation)?
                                              *   Usually set to FALSE. In column generation applications, set to TRUE if pricing
                                              *   adds coefficients to this constraint. */
   SCIP_Bool             dynamic,            /**< is constraint subject to aging?
                                              *   Usually set to FALSE. Set to TRUE for own cuts which 
                                              *   are seperated as constraints. */
   SCIP_Bool             removable,          /**< should the relaxation be removed from the LP due to aging or cleanup?
                                              *   Usually set to FALSE. Set to TRUE for 'lazy constraints' and 'user cuts'. */
   SCIP_Bool             stickingatnode      /**< should the constraint always be kept at the node where it was added, even
                                              *   if it may be moved to a more global node?
                                              *   Usually set to FALSE. Set to TRUE to for constraints that represent node data. */
   )
{
   SCIP_CONSHDLR* conshdlr;
   SCIP_CONSHDLRDATA* conshdlrdata;
   SCIP_CONSDATA* consdata;

   assert(scip != NULL);

   /* find the precedence constraint handler */
   conshdlr = SCIPfindConshdlr(scip, CONSHDLR_NAME);
   if( conshdlr == NULL )
   {
      SCIPerrorMessage(""CONSHDLR_NAME" constraint handler not found\n");
      return SCIP_PLUGINNOTFOUND;
   }

   SCIPdebugMessage("create cumulative constraint <%s> with %d jobs\n", name, nvars);

   conshdlrdata = SCIPconshdlrGetData(conshdlr);
   assert(conshdlrdata != NULL);

   /* create constraint data */
   SCIP_CALL( consdataCreate(scip, &consdata, vars, NULL, durations, demands, nvars, capacity) );

   /* create constraint */
   SCIP_CALL( SCIPcreateCons(scip, cons, name, conshdlr, consdata, 
         initial, separate, enforce, check, propagate, 
         local, modifiable, dynamic, removable, stickingatnode) );

   return SCIP_OKAY;
}

/** returns the activities of the cumulative constraint */
SCIP_VAR** SCIPgetVarsCumulative(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_CONS*            cons                /**< constraint data */
   )
{
   SCIP_CONSDATA* consdata;
   
   if( strcmp(SCIPconshdlrGetName(SCIPconsGetHdlr(cons)), CONSHDLR_NAME) != 0 )
   {
      SCIPerrorMessage("constraint is not a cumulative constraint\n");
      SCIPABORT();
   }

   consdata = SCIPconsGetData(cons);
   assert(consdata != NULL);

   return consdata->vars;
}

/** returns the activities of the cumulative constraint */
int SCIPgetNVarsCumulative(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_CONS*            cons                /**< constraint data */
   )
{
   SCIP_CONSDATA* consdata;

   if( strcmp(SCIPconshdlrGetName(SCIPconsGetHdlr(cons)), CONSHDLR_NAME) != 0 )
   {
      SCIPerrorMessage("constraint is not a cumulative constraint\n");
      SCIPABORT();
   }

   consdata = SCIPconsGetData(cons);
   assert(consdata != NULL);

   return consdata->nvars;
}

/** returns the capacity of the cumulative constraint */
int SCIPgetCapacityCumulative(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_CONS*            cons                /**< constraint data */
   )
{
   SCIP_CONSDATA* consdata;

   if( strcmp(SCIPconshdlrGetName(SCIPconsGetHdlr(cons)), CONSHDLR_NAME) != 0 )
   {
      SCIPerrorMessage("constraint is not a cumulative constraint\n");
      SCIPABORT();
   }

   consdata = SCIPconsGetData(cons);
   assert(consdata != NULL);

   return consdata->capacity;
}

/** returns the durations of the cumulative constraint */
int* SCIPgetDurationsCumulative(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_CONS*            cons                /**< constraint data */
   )
{
   SCIP_CONSDATA* consdata;

   if( strcmp(SCIPconshdlrGetName(SCIPconsGetHdlr(cons)), CONSHDLR_NAME) != 0 )
   {
      SCIPerrorMessage("constraint is not a cumulative constraint\n");
      SCIPABORT();
   }

   consdata = SCIPconsGetData(cons);
   assert(consdata != NULL);

   return consdata->durations;
}

/** returns the demands of the cumulative constraint */
int* SCIPgetDemandsCumulative(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_CONS*            cons                /**< constraint data */
   )
{
   SCIP_CONSDATA* consdata;

   if( strcmp(SCIPconshdlrGetName(SCIPconsGetHdlr(cons)), CONSHDLR_NAME) != 0 )
   {
      SCIPerrorMessage("constraint is not a cumulative constraint\n");
      SCIPABORT();
   }

   consdata = SCIPconsGetData(cons);
   assert(consdata != NULL);

   return consdata->demands;
}

/** create a new cumulative profile for the given capacity */
SCIP_RETCODE SCIPprofileCreate(
   SCIP*                 scip,               /**< SCIP data structure */
   CUMULATIVEPROFILE**   profile,            /**< pointer to store the create profile */
   int                   capacity,           /**< Capacity for this profile */
   int                   maxtimepoints       /**< maximium number time points */
   )
{
   assert(scip != NULL);
   assert(profile != NULL);
   assert(capacity > 0);
   assert(maxtimepoints > 0);

   /* Initialize memory */
   SCIP_CALL( SCIPallocMemory(scip, profile) );
   SCIP_CALL( SCIPallocMemoryArray(scip, &(*profile)->timepoints, maxtimepoints) );
   SCIP_CALL( SCIPallocMemoryArray(scip, &(*profile)->freecapacities, maxtimepoints) );

   /* Set up cumulative profile for use */
   (*profile)->ntimepoints = 2;
   (*profile)->timepoints[0] = 0;
   (*profile)->timepoints[1] = INT_MAX;
   (*profile)->freecapacities[0] = capacity;
   (*profile)->freecapacities[1] = 0;
   (*profile)->arraysize = maxtimepoints;

   return SCIP_OKAY;
}

/** frees given profile */
void SCIPprofileFree(
   SCIP*                 scip,               /**< SCIP data structure */
   CUMULATIVEPROFILE**   profile             /**< pointer to the profile */
   )
{
   assert(scip != NULL);
   assert(profile != NULL);

   /* free memory */
   SCIPfreeMemoryArray(scip, &(*profile)->timepoints);
   SCIPfreeMemoryArray(scip, &(*profile)->freecapacities);
   SCIPfreeMemory(scip, profile);
}

/** resizes the cumulative profile array */
SCIP_RETCODE SCIPprofileResize(
   SCIP*                 scip,               /**< SCIP data structure */
   CUMULATIVEPROFILE*    profile,            /**< cumulative profile to resize */
   int                   newminsize          /**< minimum size to ensure */
   )
{
   assert(scip != NULL);
   assert(profile != NULL);
   assert(newminsize >= 0);
   assert(profile->timepoints != NULL);
   assert(profile->freecapacities != NULL);
   
   if( profile->ntimepoints >= newminsize )
      return SCIP_OKAY;

   /* Grow arrays of times and free capacity */
   SCIP_CALL( SCIPreallocMemoryArray(scip, &profile->timepoints, newminsize) );
   SCIP_CALL( SCIPreallocMemoryArray(scip, &profile->freecapacities, newminsize) );
   profile->arraysize = newminsize;

   return SCIP_OKAY;
}

/** from the given job, the core time is computed. If core is non-empty the cumulative profile will be updated otherwise
 *  nothing happens
 */
void SCIPprofileInsertCore(
   SCIP*                 scip,               /**< SCIP data structure */
   CUMULATIVEPROFILE*    profile,            /**< profile to use */
   SCIP_VAR*             var,                /**< integer variable which corresponds to the starting point of the job */
   int                   duration,           /**< duration of the job */
   int                   demand,             /**< demand of the job */
   SCIP_Bool*            core,               /**< pointer to store if the corresponds job has a core */       
   SCIP_Bool*	         fixed,              /**< poiner to store if the job is fixed due to its bounds */ 
   SCIP_Bool*            infeasible          /**< pointer to store if the job does not fit due to capacity */
   )
{
   int begin;
   int end; 
   int lb;
   int ub;
	
   assert(core != NULL);
   assert(fixed != NULL);
   assert(infeasible != NULL);

   (*infeasible) = FALSE;
   (*fixed) = FALSE;
   (*core) = FALSE;
   
   lb = convertBoundToInt(scip, SCIPvarGetLbLocal(var));
   ub = convertBoundToInt(scip, SCIPvarGetUbLocal(var));
	
   if( ub - lb == 0 )
      (*fixed) = TRUE;

   begin = ub;
   end = lb + duration;
   
   /* check if a core exists */
   if( begin < end )
   {
      /* job has a nonempty core and will be inserted */
      (*core) = TRUE;

      /* insert core into the profile */
#ifdef PROFILE_DEBUG
      SCIPdebugMessage("before inserting: \n");
      SCIPprofilePrintOut(profile);
      SCIPdebugMessage("insert core from var <%s>: [%d,%d] [%d]\n", SCIPvarGetName(var), begin, end, demand);
#endif

      SCIPprofileUpdate(profile, begin, end, demand, infeasible);

#ifdef PROFILE_DEBUG
      {
         int i;
         SCIPdebugMessage("after inserting: %u\n", *infeasible);
         SCIPprofilePrintOut(profile);
         
         for( i =0; i < profile->ntimepoints-1; ++i )
         {
            assert(profile->timepoints[i] < profile->timepoints[i+1]);
         }
      }
#endif
   }
}

/** subtracts the demand from the profile during core time of the job */
void SCIPprofileDeleteCore(
   SCIP*                 scip,               /**< SCIP data structure */
   CUMULATIVEPROFILE*    profile,            /**< profile to use */
   SCIP_VAR*             var,                /**< integer variable which corresponds to the starting point of the job */
   int                   duration,           /**< duration of the job */
   int                   demand,             /**< demand of the job */
   SCIP_Bool*            core                /**< pointer to store if the corresponds job has a core, or NULL */       
   )
{
   int begin;
   int end; 
   SCIP_Bool infeasible;

   begin = convertBoundToInt(scip, SCIPvarGetUbLocal(var));
   end = convertBoundToInt(scip, SCIPvarGetLbLocal(var)) + duration;
   
   if( begin >= end )
   {
      if(core != NULL)
         *core = FALSE;

      return;
   }

   if( core != NULL )
      *core = TRUE;
	
#ifndef NDEBUG
   {
      /* check if the begin and end time points of the core correspond to a time point in the profile; this should be
       * the case since we added the core before to the profile */
      int pos;
      assert(SCIPprofileFindLowerBound(profile, begin, &pos));
      assert(SCIPprofileFindLowerBound(profile, end, &pos));
   }
#endif

   /* remove the core of the job from the current profile */
#ifdef PROFILE_DEBUG
   SCIPdebugMessage("before deleting:\n");
   SCIPprofilePrintOut(profile);

   SCIPdebugMessage("delete core from var <%s>: [%d,%d] [%d]\n", 
      SCIPvarGetName(var), begin, end, demand);
#endif

   SCIPprofileUpdate(profile, begin, end, -demand, &infeasible);

#ifdef PROFILE_DEBUG
   SCIPdebugMessage("after deleting: %u\n", infeasible);
   SCIPprofilePrintOut(profile);
#endif
   assert(!infeasible);
}


/** output of the given profile */
void SCIPprofilePrint(
   SCIP*                 scip,               /**< SCIP data structure */
   CUMULATIVEPROFILE*    profile,            /**< profile to output */
   FILE*                 file                /**< output file (or NULL for standard output) */
   )
{
   int t;
   
   for( t = 0; t < profile->ntimepoints; ++t )
   {
      SCIPinfoMessage(scip, file, "i: %d, tp: %d, fc: %d ;", t, profile->timepoints[t], profile-> freecapacities[t]); 
   }
   
   SCIPinfoMessage(scip, file,"\n");
}


/** return if the given time point exists in the profile and stores the position of the given time point if it exists;
 *  otherwise the position of the next smaller existing time point */
SCIP_Bool SCIPprofileFindLowerBound(
   CUMULATIVEPROFILE*    profile,              /**< profile to search in */
   int                   timepoint,            /**< time point to search for */
   int*                  pos                   /**< pointer to store the position */
   )
{
   assert(profile != NULL);
   assert(timepoint >= 0);
   assert(profile->ntimepoints > 0);
   assert(profile->timepoints[0] == 0);

   /* find the position of timepoint in the timepoints array via binary search */
   if( SCIPsortedvecFindInt(profile->timepoints, timepoint, profile->ntimepoints, pos) )
      return TRUE;
 
   assert(*pos > 0);
   (*pos)--;
   
   return FALSE;
}

/** inserts the given time point into the profile if it this time point does not exists yet; returns its position in the
 *  time point array */
int SCIPprofileInsertTimepoint(
   CUMULATIVEPROFILE*    profile,            /**< profile to insert the time point */
   int                   timepoint           /**< time point to insert */
   )
{
   int pos;
#ifndef NDEBUG
   int i;   
#endif

   assert(profile != NULL);
   assert(timepoint >= 0);
   assert(profile->arraysize >= profile->ntimepoints);

   if( timepoint == 0 )
      return 0;
      
   /* get the position of the given time point in the profile array if it exists; otherwise the position of the next
    * smaller existing time point */
   if( SCIPprofileFindLowerBound(profile, timepoint, &pos) )
   {
      /* if the time point exists return the corresponding position */
      assert(pos >= 0 && pos < profile->ntimepoints);
      return pos;
   }

   assert(pos >= 0 && pos < profile->ntimepoints);
   assert(timepoint >= profile->timepoints[pos]);
   assert(pos + 1 < profile->arraysize);

   /* insert new time point into the (sorted) profile */
   SCIPsortedvecInsertIntInt(profile->timepoints, profile->freecapacities, timepoint, profile->freecapacities[pos], 
      &profile->ntimepoints);

#ifndef NDEBUG
   /* check if the time points are sorted */
   for( i = 1; i < profile->ntimepoints; ++i )
      assert(profile->timepoints[i-1] < profile->timepoints[i]);
#endif
   
   return pos+1;
}

/** updates the profile due to inserting and removing a new job */
void SCIPprofileUpdate(
   CUMULATIVEPROFILE*    profile,            /**< profile to update */
   int                   starttime,          /**< time point to start */
   int                   endtime,            /**< time point to end */
   int                   demand,             /**< demand of the job */
   SCIP_Bool*            infeasible          /**< pointer to store if the update is infeasible */
   )
{
   int startpos;
   int endpos;

   assert(profile != NULL);
   assert(infeasible != NULL);
   assert(profile->arraysize >= profile->ntimepoints);
   assert(starttime >= 0 && endtime >= starttime);
   
   (*infeasible) = FALSE;
   
   if( starttime == endtime )
      return;

   /* get position of the starttime in profile */
   startpos = SCIPprofileInsertTimepoint(profile, starttime);
   assert(profile->timepoints[startpos] == starttime);

   /* get position of the endtime in profile */
   endpos = SCIPprofileInsertTimepoint(profile, endtime);
   assert(profile->timepoints[endpos] == endtime );

   assert(startpos < endpos);
   assert(profile->arraysize >= profile->ntimepoints);

   /* remove/add the given demand from the profile */
   for( ; startpos < endpos; ++startpos )
   {
      profile->freecapacities[startpos] -= demand;

      if( profile->freecapacities[startpos] < 0 )
      {
         *infeasible = TRUE;
         break;
      }      
   }
}

/** returns TRUE if the job (given by its  demand and duration) can be inserted at the given time point; otherwise FALSE */
SCIP_Bool SCIPprofileIsFeasibleStart(
   CUMULATIVEPROFILE*    profile,            /**< Cumulative profile to use */
   int                   timepoint,          /**< time point to start */
   int                   duration,           /**< duration of the job */
   int                   demand,             /**< the demand of the job */
   int*                  pos                 /**< pointer to store the earliest position where the job does not fit */
   )
{
   int endtime;
   int startpos;
   int endpos;
   int p;

   assert(profile != NULL);
   assert(timepoint >= 0);
   assert(demand >= 0);
   assert(pos != NULL);

   if( duration == 0 )
      return TRUE;
   
   endtime = timepoint + duration; 

   /* check if the activity fits at timepoint */
   (void)SCIPprofileFindLowerBound(profile, timepoint, &startpos);

   if( !SCIPprofileFindLowerBound(profile, endtime, &endpos) )
      endpos++;
   
   assert(profile->timepoints[startpos] <= timepoint);
   assert(profile->timepoints[endpos] >= endtime);
   
   for( p = startpos; p < endpos; ++p )
   {
      if( profile->freecapacities[p] < demand )
      {
         (*pos) = p;
         return FALSE;
      }
   }
   
   return TRUE;
}

/** return the earliest possible starting point within the time interval [lb,ub] for a given job (given by its duration
 *  and demand) */
int SCIPprofileGetEarliestFeasibleStart(
   CUMULATIVEPROFILE*    profile,            /**< profile to use */
   int                   lb,                 /**< earliest possible start point */
   int                   ub,                 /**< latest possible start point */
   int                   duration,           /**< duration of the job */
   int                   demand,             /**< demand of the job */
   SCIP_Bool*            infeasible          /**< pointer store if the job can not be scheduled */
   )
{
   int starttime;
   int pos;
	
   assert(profile != NULL);
   assert(lb >= 0);
   assert(duration >= 0);
   assert(demand >= 0);
   assert(infeasible != NULL);
   assert(profile->timepoints[profile->ntimepoints-1] > ub);

   if( lb > ub )
   {
      *infeasible = TRUE;
      return lb;
   }
   
   if( duration == 0 || demand == 0 ) 
   {
      *infeasible = FALSE;
      return lb;
   }

   starttime = lb;

   (void)SCIPprofileFindLowerBound(profile, starttime, &pos);
   assert(profile->timepoints[pos] <= starttime);
   
   (*infeasible) = TRUE;
	
   while( (*infeasible) && starttime <= ub )
   {
      if( SCIPprofileIsFeasibleStart(profile, starttime, duration, demand, &pos) )
      {
         (*infeasible) = FALSE;
         return starttime;
      }
    
      /* the job did not fit into the profile since at time point "pos" not enough capacity is available; therefore we
       * can proceed with the next time point  */
      assert(profile->freecapacities[pos] < demand);
      pos++;
      
      /* check if we exceed the time point array */
      if( pos >= profile->ntimepoints )
         break;
      
      starttime = profile->timepoints[pos];
   }
   
   assert(*infeasible || starttime <= ub);
   return starttime;
}

/** return the latest possible starting point within the time interval [lb,ub] for a given job (given by its duration
 *  and demand) */
int SCIPprofileGetLatestFeasibleStart(
   CUMULATIVEPROFILE*    profile,            /**< profile to use */
   int                   lb,                 /**< earliest possible start point */
   int                   ub,                 /**< latest possible start point */
   int                   duration,           /**< duration of the job */
   int                   demand,             /**< demand of the job */
   SCIP_Bool*            infeasible          /**< pointer store if the job can not be scheduled */
   )
{
   int starttime;
   int pos;
	
   assert(profile != NULL);
   assert(lb >= 0);
   assert(lb <= ub);
   assert(duration >= 0);
   assert(demand >= 0);
   assert(infeasible != NULL);
   assert(profile->timepoints[profile->ntimepoints-1] > ub);
   
   if( duration == 0 || demand == 0 ) 
      return ub;

   starttime = ub;   
   (void)SCIPprofileFindLowerBound(profile, starttime, &pos);
   assert(profile->timepoints[pos] <= starttime);
   
   (*infeasible) = TRUE;
	
   while( (*infeasible) && starttime >= lb )
   {
      if( SCIPprofileIsFeasibleStart(profile, starttime, duration, demand, &pos) )
      {
         (*infeasible) = FALSE;
         return starttime;
      }
    
      /* the job did not fit into the profile since at time point "pos" not enough capacity is available; 
       * therefore we can proceed with the next time point  */
      assert(profile->freecapacities[pos] < demand);
      
      /* check if we exceed the time point array */
      if( pos < 0  )
         break;
      
      starttime = profile->timepoints[pos] - duration;
   }

   assert(*infeasible || starttime >= lb);
  
   return starttime;
}

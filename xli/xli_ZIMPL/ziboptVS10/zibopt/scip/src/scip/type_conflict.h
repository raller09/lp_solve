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
#pragma ident "@(#) $Id: type_conflict.h,v 1.29 2010/09/27 17:20:25 bzfheinz Exp $"

/**@file   type_conflict.h
 * @ingroup TYPEDEFINITIONS
 * @brief  type definitions for conflict analysis
 * @author Tobias Achterberg
 */

/*---+----1----+----2----+----3----+----4----+----5----+----6----+----7----+----8----+----9----+----0----+----1----+----2*/

#ifndef __SCIP_TYPE_CONFLICT_H__
#define __SCIP_TYPE_CONFLICT_H__

#include "scip/def.h"
#include "scip/type_retcode.h"
#include "scip/type_result.h"
#include "scip/type_var.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SCIP_Conflicthdlr SCIP_CONFLICTHDLR; /**< conflict handler to process conflict sets */
typedef struct SCIP_ConflicthdlrData SCIP_CONFLICTHDLRDATA; /**< conflict handler data */
typedef struct SCIP_ConflictSet SCIP_CONFLICTSET; /**< set of conflicting bound changes */
typedef struct SCIP_Conflict SCIP_CONFLICT;       /**< conflict analysis data structure */


/** copy method for conflict handler plugins (called when SCIP copies plugins)
 *
 *  input:
 *  - scip            : SCIP main data structure
 *  - conflicthdlr    : the conflict handler itself
 */
#define SCIP_DECL_CONFLICTCOPY(x) SCIP_RETCODE x (SCIP* scip, SCIP_CONFLICTHDLR* conflicthdlr)

/** destructor of conflict handler to free conflict handler data (called when SCIP is exiting)
 *
 *  input:
 *  - scip            : SCIP main data structure
 *  - conflicthdlr    : the conflict handler itself
 */
#define SCIP_DECL_CONFLICTFREE(x) SCIP_RETCODE x (SCIP* scip, SCIP_CONFLICTHDLR* conflicthdlr)

/** initialization method of conflict handler (called after problem was transformed)
 *
 *  input:
 *  - scip            : SCIP main data structure
 *  - conflicthdlr    : the conflict handler itself
 */
#define SCIP_DECL_CONFLICTINIT(x) SCIP_RETCODE x (SCIP* scip, SCIP_CONFLICTHDLR* conflicthdlr)

/** deinitialization method of conflict handler (called before transformed problem is freed)
 *
 *  input:
 *  - scip            : SCIP main data structure
 *  - conflicthdlr    : the conflict handler itself
 */
#define SCIP_DECL_CONFLICTEXIT(x) SCIP_RETCODE x (SCIP* scip, SCIP_CONFLICTHDLR* conflicthdlr)

/** solving process initialization method of conflict handler (called when branch and bound process is about to begin)
 *
 *  This method is called when the presolving was finished and the branch and bound process is about to begin.
 *  The conflict handler may use this call to initialize its branch and bound specific data.
 *
 *  input:
 *  - scip            : SCIP main data structure
 *  - conflicthdlr    : the conflict handler itself
 */
#define SCIP_DECL_CONFLICTINITSOL(x) SCIP_RETCODE x (SCIP* scip, SCIP_CONFLICTHDLR* conflicthdlr)

/** solving process deinitialization method of conflict handler (called before branch and bound process data is freed)
 *
 *  This method is called before the branch and bound process is freed.
 *  The conflict handler should use this call to clean up its branch and bound data.
 *
 *  input:
 *  - scip            : SCIP main data structure
 *  - conflicthdlr    : the conflict handler itself
 */
#define SCIP_DECL_CONFLICTEXITSOL(x) SCIP_RETCODE x (SCIP* scip, SCIP_CONFLICTHDLR* conflicthdlr)

/** conflict processing method of conflict handler (called when conflict was found)
 *
 *  This method is called, when the conflict analysis found a conflict on variable bounds.
 *  The conflict handler may update its data accordingly and create a constraint out of the conflict set.
 *  If the parameter "resolved" is set, the conflict handler should not create a constraint, because
 *  a different conflict handler with higher priority already created a constraint.
 *  The bounds in the conflict set lead to a conflict (i.e. an infeasibility) when all enforced at the same time.
 *  Thus, a feasible conflict constraint must demand, that at least one of the variables in the conflict
 *  set violates its corresponding bound, i.e., fulfills the negation of the bound change in the conflict set.
 *  For continuous variables, the negation has to be defined in a relaxed way: if, e.g., the bound in the conflict
 *  set is "x <= u", the negation to be used has to be "x >= u", and not "x > u".
 *  The given "bdchginfos" array representing the conflict set is only a reference to an internal
 *  buffer, that may be modified at any time by SCIP. The user must copy the needed information from the
 *  "bdchginfos" array to its own data structures, if he wants to use the information later.
 *  He should not keep a pointer to the array or pointers to the single bdchginfos in the array, because these
 *  may get invalid afterwards.
 *
 *  input:
 *  - scip            : SCIP main data structure
 *  - conflicthdlr    : the conflict handler itself
 *  - node            : node to add resulting conflict constraint to (with SCIPaddConsNode())
 *  - validnode       : node at which the conflict constraint is valid (should be passed to SCIPaddConsNode())
 *  - bdchginfos      : array with bound changes that lead to a conflict
 *  - nbdchginfos     : number of bound changes in the conflict set
 *  - local           : is the conflict set only valid locally, i.e. should the constraint created as local constraint?
 *  - dynamic         : should the conflict constraint be made subject to aging?
 *  - removable       : should the conflict's relaxation be made subject to LP aging and cleanup?
 *  - resolved        : is the conflict set already used to create a constraint?
 *  - result          : pointer to store the result of the conflict processing call
 *
 *  possible return values for *result:
 *  - SCIP_CONSADDED  : the conflict handler created a constraint out of the conflict set
 *  - SCIP_DIDNOTFIND : the conflict handler could not create a constraint out of the conflict set
 *  - SCIP_DIDNOTRUN  : the conflict handler was skipped
 */
#define SCIP_DECL_CONFLICTEXEC(x) SCIP_RETCODE x (SCIP* scip, SCIP_CONFLICTHDLR* conflicthdlr, SCIP_NODE* node, \
      SCIP_NODE* validnode, SCIP_BDCHGINFO** bdchginfos, int nbdchginfos, \
      SCIP_Bool local, SCIP_Bool dynamic, SCIP_Bool removable, SCIP_Bool resolved, SCIP_RESULT* result)

#ifdef __cplusplus
}
#endif

#endif

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
#pragma ident "@(#) $Id: pub_nlp.h,v 1.7 2010/09/06 17:38:49 bzfviger Exp $"

/**@file   pub_nlp.h
 * @ingroup PUBLICMETHODS
 * @brief  public methods for NLP management
 * @author Thorsten Gellermann
 * @author Stefan Vigerske
 */

/*---+----1----+----2----+----3----+----4----+----5----+----6----+----7----+----8----+----9----+----0----+----1----+----2*/

#ifndef __SCIP_PUB_NLP_H__
#define __SCIP_PUB_NLP_H__

#include <stdio.h>

#include "scip/def.h"
#include "blockmemshell/memory.h"
#include "scip/type_set.h"
#include "scip/type_stat.h"
#include "scip/type_nlp.h"
#include "scip/type_var.h"
#include "scip/type_sol.h"
#include "nlpi/type_expression.h"
#include "nlpi/type_nlpi.h"

#ifdef __cplusplus
extern "C" {
#endif

/**@name Nonlinear row methods */
/**@{ */

/** output nonlinear row to file stream */
extern
SCIP_RETCODE SCIPnlrowPrint(
   SCIP_NLROW*           nlrow,              /**< NLP row */
   FILE*                 file                /**< output file (or NULL for standard output) */
   );

/** gets constant */
extern
SCIP_Real SCIPnlrowGetConstant(
   SCIP_NLROW*           nlrow               /**< NLP row */
   );

/** gets number of variables of linear part */
extern
int SCIPnlrowGetNLinearVars(
   SCIP_NLROW*           nlrow               /**< NLP row */
   );

/** gets array with variables of linear part */
extern
SCIP_VAR** SCIPnlrowGetLinearVars(
   SCIP_NLROW*           nlrow               /**< NLP row */
   );

/** gets array with coefficients in linear part */
extern
SCIP_Real* SCIPnlrowGetLinearCoefs(
   SCIP_NLROW*           nlrow               /**< NLP row */
   );

/** gets number of quadratic variables in quadratic part */
extern
int SCIPnlrowGetNQuadVars(
   SCIP_NLROW*           nlrow               /**< NLP row */
   );

/** gets quadratic variables in quadratic part */
extern
SCIP_VAR** SCIPnlrowGetQuadVars(
   SCIP_NLROW*           nlrow               /**< NLP row */
   );

/** gives position of variable in quadvars array of row, or -1 if not found */
extern
int SCIPnlrowSearchQuadVar(
   SCIP_NLROW*           nlrow,                /**< nonlinear row */
   SCIP_VAR*             var                   /**< variable to search for */
   );

/** gets number of quadratic elements in quadratic part */
extern
int SCIPnlrowGetNQuadElems(
   SCIP_NLROW*           nlrow               /**< NLP row */
   );

/** gets quadratic elements in quadratic part */
extern
SCIP_QUADELEM* SCIPnlrowGetQuadElems(
   SCIP_NLROW*           nlrow               /**< NLP row */
   );

/** gets array with coefficients in linear part */
extern
void SCIPnlrowGetQuadData(
   SCIP_NLROW*           nlrow,              /**< NLP row */
   int*                  nquadvars,          /**< buffer to store number of variables in quadratic term, or NULL if not of interest */
   SCIP_VAR***           quadvars,           /**< buffer to store pointer to array of variables in quadratic term, or NULL if not of interest */
   int*                  nquadelems,         /**< buffer to store number of entries in quadratic term, or NULL if not of interest */
   SCIP_QUADELEM**       quadelems           /**< buffer to store pointer to arrau of entries in quadratic term, or NULL if not of interest */
   );

/** gets expression tree */
extern
SCIP_EXPRTREE* SCIPnlrowGetExprtree(
   SCIP_NLROW*           nlrow               /**< NLP row */
   );

/** returns the left hand side of a nonlinear row */
extern
SCIP_Real SCIPnlrowGetLhs(
   SCIP_NLROW*           nlrow               /**< NLP row */
   );

/** returns the right hand side of a nonlinear row */
extern
SCIP_Real SCIPnlrowGetRhs(
   SCIP_NLROW*           nlrow               /**< NLP row */
   );

/** returns the name of a nonlinear row */
extern
const char* SCIPnlrowGetName(
   SCIP_NLROW*           nlrow               /**< NLP row */
   );

/** gets position of a nonlinear row in current NLP, or -1 if it is objective, or -2 if not in NLP */
extern
int SCIPnlrowGetNLPPos(
   SCIP_NLROW*           nlrow               /**< NLP row */
   );

/** returns TRUE iff row is member of current NLP */
extern
SCIP_Bool SCIPnlrowIsInNLP(
   SCIP_NLROW*           nlrow               /**< NLP row */
   );

/**@} */

/**@name Nonlinear problem (NLP) methods */
/**@{ */

#if 0
/** sets whether the current NLP is a convex problem, i.e., all restrictions are defined by convex functions w.r.t. current bounds */
extern
void SCIPnlpSetIsConvex(
   SCIP_NLP*             nlp,                /**< NLP data */
   SCIP_Bool             isconvex            /**< is the current NLP a convex problem? */
   );

/** returns whether the current NLP is a convex problem, i.e., all restrictions are defined by convex functions w.r.t. current bounds */
extern
SCIP_Bool SCIPnlpIsConvex(
   SCIP_NLP*             nlp                 /**< NLP data */
   );
#endif

/** gets array with variables of the NLP */
extern
SCIP_VAR** SCIPnlpGetVars(
   SCIP_NLP*             nlp                 /**< current NLP data */
   );

/** gets current number of variables in NLP */
extern
int SCIPnlpGetNVars(
   SCIP_NLP*             nlp                 /**< current NLP data */
   );

/** gets array with nonlinear rows of the NLP */
extern
SCIP_NLROW** SCIPnlpGetNlRows(
   SCIP_NLP*             nlp                 /**< current NLP data */
   );

/** gets current number of nonlinear rows in NLP */
extern
int SCIPnlpGetNNlRows(
   SCIP_NLP*             nlp                 /**< current NLP data */
   );

/** gets objective of the NLP
 * gives NULL if SCIP objective is used */
extern
SCIP_NLROW* SCIPnlpGetObjective(
   SCIP_NLP*             nlp                 /**< current NLP data */
   );

/** gets the NLP solver interface */
extern
SCIP_NLPI* SCIPnlpGetNLPI(
   SCIP_NLP*             nlp                 /**< current NLP data */
   );

/** gets the NLP problem in the solver interface */
extern
SCIP_NLPIPROBLEM* SCIPnlpGetNLPIProblem(
   SCIP_NLP*             nlp                 /**< current NLP data */
   );

/** indicates whether NLP is currently in diving mode */
extern
SCIP_Bool SCIPnlpIsDiving(
   SCIP_NLP*             nlp                 /**< current NLP data */
   );

/** gets solution status of current NLP */
extern
SCIP_NLPSOLSTAT SCIPnlpGetSolstat(
   SCIP_NLP*             nlp                 /**< current NLP data */
   );

/** gets termination status of last NLP solve */
extern
SCIP_NLPTERMSTAT SCIPnlpGetTermstat(
   SCIP_NLP*             nlp                 /**< current NLP data */
   );

/** gives statistics (number of iterations, solving time, ...) of last NLP solve */
extern
SCIP_RETCODE SCIPnlpGetStatistics(
   SCIP_NLP*             nlp,                /**< pointer to NLP datastructure */
   SCIP_NLPSTATISTICS*   statistics          /**< pointer to store statistics */
);

/** indicates whether a feasible solution for the current NLP is available
 * thus, returns whether the solution status <= feasible  */
extern
SCIP_Bool SCIPnlpHasSolution(
   SCIP_NLP*             nlp                 /**< current NLP data */
   );

/** gets values of current primal NLP solution
 * returns NULL if no solution available
 * use SCIPnlpGetSolstat to get information on whether solution is optimal or just feasible
 * use SCIPnlpGetVars to get variables corresponding to solution values */
extern
SCIP_Real* SCIPnlpGetSolVals(
   SCIP_NLP*             nlp                 /**< current NLP data */
   );

/** gets primal value of a single variable in current NLP solution */
extern
SCIP_RETCODE SCIPnlpGetVarSolVal(
   SCIP_NLP*             nlp,                /**< current NLP data */
   SCIP_VAR*             var,                /**< variable to get solution value for */
   SCIP_Real*            val                 /**< buffer to store value of variable in solution, or SCIP_INVALID if no solution available */
   );

/** gets integer parameter of NLP */
extern
SCIP_RETCODE SCIPnlpGetIntPar(
   SCIP_NLP*             nlp,                /**< pointer to NLP datastructure */
   SCIP_NLPPARAM         type,               /**< parameter number */
   int*                  ival                /**< pointer to store the parameter value */
);

/** sets integer parameter of NLP */
extern
SCIP_RETCODE SCIPnlpSetIntPar(
   SCIP_NLP*             nlp,                /**< pointer to NLP datastructure */
   SCIP_NLPPARAM         type,               /**< parameter number */
   int                   ival                /**< parameter value */
);

/** gets floating point parameter of NLP */
extern
SCIP_RETCODE SCIPnlpGetRealPar(
   SCIP_NLP*             nlp,                /**< pointer to NLP datastructure */
   SCIP_NLPPARAM         type,               /**< parameter number */
   SCIP_Real*            dval                /**< pointer to store the parameter value */
);

/** sets floating point parameter of NLP */
extern
SCIP_RETCODE SCIPnlpSetRealPar(
   SCIP_NLP*             nlp,                /**< pointer to NLP datastructure */
   SCIP_NLPPARAM         type,               /**< parameter number */
   SCIP_Real             dval                /**< parameter value */
);

/** gets string parameter of NLP */
extern
SCIP_RETCODE SCIPnlpGetStringPar(
   SCIP_NLP*             nlp,                /**< pointer to NLP datastructure */
   SCIP_NLPPARAM         type,               /**< parameter number */
   const char**          sval                /**< pointer to store the parameter value */
);

/** sets string parameter of NLP */
extern
SCIP_RETCODE SCIPnlpSetStringPar(
   SCIP_NLP*             nlp,                /**< pointer to NLP datastructure */
   SCIP_NLPPARAM         type,               /**< parameter number */
   const char*           sval                /**< parameter value */
);

/**@} */

#ifdef __cplusplus
}
#endif

#endif /* __SCIP_PUB_NLP_H__ */

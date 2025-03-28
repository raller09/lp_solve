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
#pragma ident "@(#) $Id: struct_lp.h,v 1.63 2010/09/03 14:50:16 bzfviger Exp $"

/**@file   struct_lp.h
 * @brief  datastructures for LP management
 * @author Tobias Achterberg
 *
 *  In SCIP, the LP is defined as follows:
 *
 *   min       obj * x
 *      lhs <=   A * x + const <= rhs
 *      lb  <=       x         <= ub
 *
 *  The row activities are defined as 
 *     activity = A * x + const
 *  and must therefore be in the range of [lhs,rhs].
 *
 *  The reduced costs are defined as
 *     redcost = obj - A^T * y
 *  and must be   nonnegative, if the corresponding lb is nonnegative,
 *                zero,        if the corresponging lb is negative.
 *
 *  The main datastructures for storing an LP are the rows and the columns.
 *  A row can live on its own (if it was created by a separator), or as SCIP_LP
 *  relaxation of a constraint. Thus, it has a nuses-counter, and is
 *  deleted, if not needed any more.
 *  A column cannot live on its own. It is always connected to a problem
 *  variable. Because pricing is always problem specific, it cannot create
 *  LP columns without introducing new variables. Thus, each column is
 *  connected to exactly one variable, and is deleted, if the variable
 *  is deleted.
 */

/*---+----1----+----2----+----3----+----4----+----5----+----6----+----7----+----8----+----9----+----0----+----1----+----2*/

#ifndef __SCIP_STRUCT_LP_H__
#define __SCIP_STRUCT_LP_H__


#include "scip/def.h"
#include "scip/type_lpi.h"
#include "scip/type_lp.h"
#include "scip/type_var.h"
#include "scip/type_event.h"

#ifdef __cplusplus
extern "C" {
#endif

/** LP column;
 *  The row vector of the LP column is partitioned into two parts: The first col->nlprows rows in the rows array
 *  are the ones that belong to the current LP (col->rows[j]->lppos >= 0) and that are linked to the column
 *  (col->linkpos[j] >= 0). The remaining col->len - col->nlprows rows in the rows array are the ones that
 *  don't belong to the current LP (col->rows[j]->lppos == -1) or that are not linked to the column
 *  (col->linkpos[j] == -1).
 */
struct SCIP_Col
{
   SCIP_Real             obj;                /**< current objective value of column in LP */
   SCIP_Real             lb;                 /**< current lower bound of column in LP */
   SCIP_Real             ub;                 /**< current upper bound of column in LP */
   SCIP_Real             lazylb;             /**< lazy lower bound of the column; if the current lower bound is not greater than 
                                              *   the lazy lower bound, then the lower bound has not to be added to the LP */
   SCIP_Real             lazyub;             /**< lazy upper bound of the column; if the current upper bound is not smaller than 
                                              *   the lazy upper bound, then the upper bound has not to be added to the LP */
   SCIP_Real             flushedobj;         /**< objective value of column already flushed to the LP solver */
   SCIP_Real             flushedlb;          /**< lower bound of column already flushed to the LP solver */
   SCIP_Real             flushedub;          /**< upper bound of column already flushed to the LP solver */
   SCIP_Real             primsol;            /**< primal solution value in LP, is 0 if col is not in LP */
   SCIP_Real             redcost;            /**< reduced cost value in LP, or SCIP_INVALID if not yet calculated */
   SCIP_Real             farkascoef;         /**< coefficient in dual farkas infeasibility proof (== dualfarkas^T A_c) */
   SCIP_Real             minprimsol;         /**< minimal LP solution value, this column ever assumed */
   SCIP_Real             maxprimsol;         /**< maximal LP solution value, this column ever assumed */
   SCIP_Real             sbdown;             /**< strong branching information for downwards branching */
   SCIP_Real             sbup;               /**< strong branching information for upwards branching */
   SCIP_Real             sbsolval;           /**< LP solution value of column at last strong branching call */
   SCIP_Real             sblpobjval;         /**< LP objective value at last strong branching call on the column */
   SCIP_Longint          sbnode;             /**< node number of the last strong branching call on this column */
   SCIP_Longint          obsoletenode;       /**< last node where this column was removed due to aging */
   SCIP_VAR*             var;                /**< variable, this column represents; there cannot be a column without variable */
   SCIP_ROW**            rows;               /**< rows of column entries, that may have a nonzero dual solution value */
   SCIP_Real*            vals;               /**< coefficients of column entries */
   int*                  linkpos;            /**< position of col in col vector of the row, or -1 if not yet linked */
   int                   index;              /**< consecutively numbered column identifier */
   int                   size;               /**< size of the row- and val-arrays */
   int                   len;                /**< number of nonzeros in column */
   int                   nlprows;            /**< number of linked rows in column, that belong to the current LP */
   int                   nunlinked;          /**< number of column entries, where the rows don't know about the column */
   int                   lppos;              /**< column position number in current LP, or -1 if not in current LP */
   int                   lpipos;             /**< column position number in LP solver, or -1 if not in LP solver */
   int                   lpdepth;            /**< depth level at which column entered the LP, or -1 if not in current LP */
   int                   validredcostlp;     /**< LP number for which reduced cost value is valid */
   int                   validfarkaslp;      /**< LP number for which farkas coefficient is valid */
   int                   validsblp;          /**< LP number for which strong branching values are valid */
   int                   sbitlim;            /**< strong branching iteration limit used to get strongbranch values, or -1 */
   int                   nsbcalls;           /**< number of times, strong branching was applied on the column */
   int                   age;                /**< number of successive times this variable was in LP and was 0.0 in solution */
   int                   var_probindex;      /**< copy of var->probindex for avoiding expensive dereferencing */
   unsigned int          basisstatus:2;      /**< basis status of column in last LP solution, invalid for non-LP columns */
   unsigned int          lprowssorted:1;     /**< are the linked LP rows in the rows array sorted by non-decreasing index? */
   unsigned int          nonlprowssorted:1;  /**< are the non-LP/not linked rows sorted by non-decreasing index? */
   unsigned int          objchanged:1;       /**< has objective value changed, and has data of LP solver to be updated? */
   unsigned int          lbchanged:1;        /**< has lower bound changed, and has data of LP solver to be updated? */
   unsigned int          ubchanged:1;        /**< has upper bound changed, and has data of LP solver to be updated? */
   unsigned int          coefchanged:1;      /**< has the coefficient vector changed, and has LP solver to be updated? */
   unsigned int          integral:1;         /**< is associated variable of integral type? */
   unsigned int          removable:1;        /**< is column removable from the LP (due to aging or cleanup)? */
   unsigned int          sbdownvalid:1;      /**< stores whether the stored strong branching down value is a valid dual bound;
                                              *   otherwise, it can only be used as an estimate value */
   unsigned int          sbupvalid:1;        /**< stores whether the stored strong branching up value is a valid dual bound;
                                              *   otherwise, it can only be used as an estimate value */
};

/** LP row
 *  The column vector of the LP row is partitioned into two parts: The first row->nlpcols columns in the cols array
 *  are the ones that belong to the current LP (row->cols[j]->lppos >= 0) and that are linked to the row   
 *  (row->linkpos[j] >= 0). The remaining row->len - row->nlpcols columns in the cols array are the ones that
 *  don't belong to the current LP (row->cols[j]->lppos == -1) or that are not linked to the row   
 *  (row->linkpos[j] == -1).
 */
struct SCIP_Row
{
   SCIP_Real             constant;           /**< constant shift c in row lhs <= ax + c <= rhs */
   SCIP_Real             lhs;                /**< left hand side of row */
   SCIP_Real             rhs;                /**< right hand side of row */
   SCIP_Real             flushedlhs;         /**< left hand side minus constant of row already flushed to the LP solver */
   SCIP_Real             flushedrhs;         /**< right hand side minus constant of row already flushed to the LP solver */
   SCIP_Real             sqrnorm;            /**< squared euclidean norm of row vector */
   SCIP_Real             sumnorm;            /**< sum norm of row vector (sum of absolute values of coefficients) */
   SCIP_Real             objprod;            /**< scalar product of row vector with objective function */
   SCIP_Real             maxval;             /**< maximal absolute value of row vector, only valid if nummaxval > 0 */
   SCIP_Real             minval;             /**< minimal absolute non-zero value of row vector, only valid if numminval > 0 */
   SCIP_Real             dualsol;            /**< dual solution value in LP, is 0 if row is not in LP */
   SCIP_Real             activity;           /**< row activity value in LP, or SCIP_INVALID if not yet calculated */
   SCIP_Real             dualfarkas;         /**< multiplier value in dual farkas infeasibility proof */
   SCIP_Real             pseudoactivity;     /**< row activity value in pseudo solution, or SCIP_INVALID if not yet calculated */
   SCIP_Real             minactivity;        /**< minimal activity value w.r.t. the column's bounds, or SCIP_INVALID */
   SCIP_Real             maxactivity;        /**< maximal activity value w.r.t. the column's bounds, or SCIP_INVALID */
   SCIP_Longint          validpsactivitydomchg; /**< domain change number for which pseudo activity value is valid */
   SCIP_Longint          validactivitybdsdomchg;/**< domain change number for which activity bound values are valid */
   SCIP_Longint          obsoletenode;       /**< last node where this row was removed due to aging */
   char*                 name;               /**< name of the row */
   SCIP_COL**            cols;               /**< columns of row entries, that may have a nonzero primal solution value */
   int*                  cols_index;         /**< copy of cols[i]->index for avoiding expensive dereferencing */
   SCIP_Real*            vals;               /**< coefficients of row entries */
   int*                  linkpos;            /**< position of row in row vector of the column, or -1 if not yet linked */
   SCIP_EVENTFILTER*     eventfilter;        /**< event filter for events concerning this row */
   int                   index;              /**< consecutively numbered row identifier */
   int                   size;               /**< size of the col- and val-arrays */
   int                   len;                /**< number of nonzeros in row */
   int                   nlpcols;            /**< number of linked columns in row, that belong to the current LP */
   int                   nunlinked;          /**< number of row entries, where the columns don't know about the row */
   int                   nuses;              /**< number of times, this row is referenced */
   int                   lppos;              /**< row position number in current LP, or -1 if not in current LP */
   int                   lpipos;             /**< row position number in LP solver, or -1 if not in LP solver */
   int                   lpdepth;            /**< depth level at which row entered the LP, or -1 if not in current LP */
   int                   minidx;             /**< minimal column index of row entries */
   int                   maxidx;             /**< maximal column index of row entries */
   int                   nummaxval;          /**< number of coefs with absolute value equal to maxval, zero if maxval invalid */
   int                   numminval;          /**< number of coefs with absolute value equal to minval, zero if minval invalid */
   int                   validactivitylp;    /**< LP number for which activity value is valid */
   int                   age;                /**< number of successive times this row was in LP and was not sharp in solution */
   unsigned int          basisstatus:2;      /**< basis status of row in last LP solution, invalid for non-LP rows */
   unsigned int          lpcolssorted:1;     /**< are the linked LP columns in the cols array sorted by non-decreasing index? */
   unsigned int          nonlpcolssorted:1;  /**< are the non-LP/not linked columns sorted by non-decreasing index? */
   unsigned int          delaysort:1;        /**< should the row sorting be delayed and done in a lazy fashion? */
   unsigned int          validminmaxidx:1;   /**< are minimal and maximal column index valid? */
   unsigned int          lhschanged:1;       /**< was left hand side or constant changed, and has LP solver to be updated? */
   unsigned int          rhschanged:1;       /**< was right hand side or constant changed, and has LP solver to be updated? */
   unsigned int          coefchanged:1;      /**< was the coefficient vector changed, and has LP solver to be updated? */
   unsigned int          integral:1;         /**< is activity (without constant) of row always integral in feasible solution? */
   unsigned int          local:1;            /**< is row only valid locally? */
   unsigned int          modifiable:1;       /**< is row modifiable during node processing (subject to column generation)? */
   unsigned int          removable:1;        /**< is row removable from the LP (due to aging or cleanup)? */
   unsigned int          inglobalcutpool:1;  /**< is row contained in the global cut pool? */
   unsigned int          nlocks:18;          /**< number of sealed locks of an unmodifiable row */
};

/** current LP data */
struct SCIP_Lp
{
   SCIP_Real             lpobjval;           /**< objective value of LP without loose variables, or SCIP_INVALID */
   SCIP_Real             looseobjval;        /**< current solution value of all loose variables set to their best bounds,
                                              *   ignoring variables, with infinite best bound */
   SCIP_Real             pseudoobjval;       /**< current pseudo solution value with all variables set to their best bounds,
                                              *   ignoring variables, with infinite best bound */
   SCIP_Real             rootlpobjval;       /**< objective value of root LP without loose variables, or SCIP_INVALID */
   SCIP_Real             rootlooseobjval;    /**< objective value of loose variables in root node, or SCIP_INVALID */
   SCIP_Real             cutoffbound;        /**< upper objective limit of LP (copy of primal->cutoffbound) */
   SCIP_Real             lpiuobjlim;         /**< current upper objective limit in LPI */
   SCIP_Real             lpifeastol;         /**< current feasibility tolerance in LPI */
   SCIP_Real             lpidualfeastol;     /**< current reduced costs feasibility tolerance in LPI */
   SCIP_Real             lpibarrierconvtol;  /**< current convergence tolerance used in barrier algorithm in LPI */
   SCIP_Real             objsqrnorm;         /**< squared euclidean norm of objective function vector of problem variables */
   SCIP_Real             objsumnorm;         /**< sum norm of objective function vector of problem variables */
   SCIP_LPI*             lpi;                /**< LP solver interface */
   SCIP_COL**            lpicols;            /**< array with columns currently stored in the LP solver */
   SCIP_ROW**            lpirows;            /**< array with rows currently stored in the LP solver */
   SCIP_COL**            chgcols;            /**< array of changed columns not yet applied to the LP solver */
   SCIP_ROW**            chgrows;            /**< array of changed rows not yet applied to the LP solver */
   SCIP_COL**            cols;               /**< array with current LP columns in correct order */
   SCIP_COL**            lazycols;           /**< array with current LP lazy columns */
   SCIP_ROW**            rows;               /**< array with current LP rows in correct order */
   SCIP_LPISTATE*        divelpistate;       /**< stores LPI state (basis information) before diving starts */
   int                   lpicolssize;        /**< available slots in lpicols vector */
   int                   nlpicols;           /**< number of columns in the LP solver */
   int                   lpifirstchgcol;     /**< first column of the LP which differs from the column in the LP solver */
   int                   lpirowssize;        /**< available slots in lpirows vector */
   int                   nlpirows;           /**< number of rows in the LP solver */
   int                   lpifirstchgrow;     /**< first row of the LP which differs from the row in the LP solver */
   int                   chgcolssize;        /**< available slots in chgcols vector */
   int                   nchgcols;           /**< current number of chgcols (number of used slots in chgcols vector) */
   int                   chgrowssize;        /**< available slots in chgrows vector */
   int                   nchgrows;           /**< current number of chgrows (number of used slots in chgrows vector) */
   int                   colssize;           /**< available slots in cols vector */
   int                   ncols;              /**< current number of LP columns (number of used slots in cols vector) */
   int                   lazycolssize;       /**< available slots in lazycols vector */
   int                   nlazycols;          /**< current number of LP lazy columns (number of used slots in lazycols vector) */
   int                   nremovablecols;     /**< number of removable columns in the LP */
   int                   firstnewcol;        /**< first column added at the current node */
   int                   rowssize;           /**< available slots in rows vector */
   int                   nrows;              /**< current number of LP rows (number of used slots in rows vector) */
   int                   nremovablerows;     /**< number of removable rows in the LP */
   int                   firstnewrow;        /**< first row added at the current node */
   int                   looseobjvalinf;     /**< number of loose variables with infinite best bound in current solution */
   int                   nloosevars;         /**< number of loose variables in LP */
   int                   pseudoobjvalinf;    /**< number of variables with infinite best bound in current pseudo solution */
   int                   validsollp;         /**< LP number for which the currently stored solution values are valid */
   int                   validfarkaslp;      /**< LP number for which the currently stored farkas row multipliers are valid */
   int                   lpiitlim;           /**< current iteration limit setting in LPI */
   int                   lpifastmip;         /**< current FASTMIP setting in LPI */
   int                   lpithreads;         /**< current THREADS setting in LPI */
   SCIP_PRICING          lpipricing;         /**< current pricing setting in LPI */
   SCIP_LPSOLSTAT        lpsolstat;          /**< solution status of last LP solution */
   SCIP_LPALGO           lastlpalgo;         /**< algorithm used for last LP solve */
   SCIP_Bool             objsqrnormunreliable;/**< is squared euclidean norm of objective function vector of problem
                                               *   variables unreliable and need recalculation? */
   SCIP_Bool             flushdeletedcols;   /**< have LPI-columns been deleted in the last lpFlush() call? */
   SCIP_Bool             flushaddedcols;     /**< have LPI-columns been added in the last lpFlush() call? */
   SCIP_Bool             flushdeletedrows;   /**< have LPI-rows been deleted in the last lpFlush() call? */
   SCIP_Bool             flushaddedrows;     /**< have LPI-rows been added in the last lpFlush() call? */
   SCIP_Bool             flushed;            /**< are all cached changes applied to the LP solver? */
   SCIP_Bool             solved;             /**< is current LP solved? */
   SCIP_Bool             primalfeasible;     /**< is current LP solution primal feasible? */
   SCIP_Bool             dualfeasible;       /**< is current LP solution dual feasible? */
   SCIP_Bool             solisbasic;         /**< is current LP solution a basic solution? */
   SCIP_Bool             rootlpisrelax;      /**< is root LP solution a relaxation of the problem and its value a valid global lower bound? */
   SCIP_Bool             isrelax;            /**< is current LP solution a relaxation of the current problem and its value a valid local lower bound? */
   SCIP_Bool             installing;         /**< whether the solution process is in stalling */
   SCIP_Bool             strongbranching;    /**< whether the lp is used for strong branching */
   SCIP_Bool             probing;            /**< are we currently in probing mode? */
   SCIP_Bool             diving;             /**< LP is used for diving: col bounds and obj don't corresond to variables */
   SCIP_Bool             divingobjchg;       /**< objective values were changed in diving: LP objective is invalid */
   SCIP_Bool             resolvelperror;     /**< an error occured during resolving the LP after diving or probing */
   SCIP_Bool             lpifromscratch;     /**< current FROMSCRATCH setting in LPI */
   SCIP_Bool             lpiscaling;         /**< current SCALING setting in LPI */
   SCIP_Bool             lpipresolving;      /**< current PRESOLVING setting in LPI */
   SCIP_Bool             lpilpinfo;          /**< current LPINFO setting in LPI */
   SCIP_Bool             lpihasfeastol;      /**< does the LPI support the FEASTOL parameter? */
   SCIP_Bool             lpihasdualfeastol;  /**< does the LPI support the DUALFEASTOL parameter? */
   SCIP_Bool             lpihasbarrierconvtol;/**< does the LPI support the BARRIERCONVTOL parameter? */
   SCIP_Bool             lpihasfastmip;      /**< does the LPI support the FASTMIP parameter? */
   SCIP_Bool             lpihasscaling;      /**< does the LPI support the SCALING parameter? */
   SCIP_Bool             lpihaspresolving;   /**< does the LPI support the PRESOLVING parameter? */
   SCIP_Bool             lpihasrowrep;       /**< does the LPI support row representation of a simplex basis? */
   SCIP_Real             lpirowrepswitch;    /**< simplex algorithm shall use row representation of the basis
                                              *   if number of rows divided by number of columns exceeds this value */
};

#ifdef __cplusplus
}
#endif

#endif

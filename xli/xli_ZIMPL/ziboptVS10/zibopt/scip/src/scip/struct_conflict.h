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
#pragma ident "@(#) $Id: struct_conflict.h,v 1.30 2010/03/12 14:54:30 bzfwinkm Exp $"

/**@file   struct_conflict.h
 * @brief  datastructures for conflict analysis
 * @author Tobias Achterberg
 */

/*---+----1----+----2----+----3----+----4----+----5----+----6----+----7----+----8----+----9----+----0----+----1----+----2*/

#ifndef __SCIP_STRUCT_CONFLICT_H__
#define __SCIP_STRUCT_CONFLICT_H__


#include "scip/def.h"
#include "scip/type_clock.h"
#include "scip/type_lpi.h"
#include "scip/type_misc.h"
#include "scip/type_var.h"
#include "scip/type_conflict.h"

#ifdef __cplusplus
extern "C" {
#endif

/** conflict handler */
struct SCIP_Conflicthdlr
{
   char*                 name;               /**< name of conflict handler */
   char*                 desc;               /**< description of conflict handler */
   SCIP_DECL_CONFLICTCOPY((*conflictcopy));  /**< copy method of conflict handler or NULL if you don't want to copy your plugin into subscips */
   SCIP_DECL_CONFLICTFREE((*conflictfree));  /**< destructor of conflict handler */
   SCIP_DECL_CONFLICTINIT((*conflictinit));  /**< initialize conflict handler */
   SCIP_DECL_CONFLICTEXIT((*conflictexit));  /**< deinitialize conflict handler */
   SCIP_DECL_CONFLICTINITSOL((*conflictinitsol));/**< solving process initialization method of conflict handler */
   SCIP_DECL_CONFLICTEXITSOL((*conflictexitsol));/**< solving process deinitialization method of conflict handler */
   SCIP_DECL_CONFLICTEXEC((*conflictexec));  /**< conflict processing method of conflict handler */
   SCIP_CONFLICTHDLRDATA* conflicthdlrdata;  /**< conflict handler data */
   int                   priority;           /**< priority of the conflict handler */
   SCIP_Bool             initialized;        /**< is conflict handler initialized? */
};

/** set of conflicting bound changes */
struct SCIP_ConflictSet
{
   SCIP_BDCHGINFO**      bdchginfos;         /**< bound change informations of the conflict set */
   int*                  sortvals;           /**< aggregated var index/bound type values for sorting */
   int                   bdchginfossize;     /**< size of bdchginfos array */
   int                   nbdchginfos;        /**< number of bound change informations in the conflict set */
   int                   validdepth;         /**< depth in the tree where the conflict set is valid */
   int                   insertdepth;        /**< depth level where constraint should be added */
   int                   conflictdepth;      /**< depth in the tree where the conflict set yields a conflict */
   int                   repropdepth;        /**< depth at which the conflict set triggers a deduction */
   SCIP_Bool             repropagate;        /**< should the conflict constraint trigger a repropagation? */
};

/** conflict analysis data structure */
struct SCIP_Conflict
{
   SCIP_Longint          nappliedglbconss;   /**< total number of conflict constraints added globally to the problem */
   SCIP_Longint          nappliedglbliterals;/**< total number of literals in globally applied conflict constraints */
   SCIP_Longint          nappliedlocconss;   /**< total number of conflict constraints added locally to the problem */
   SCIP_Longint          nappliedlocliterals;/**< total number of literals in locally applied conflict constraints */
   SCIP_Longint          npropcalls;         /**< number of calls to propagation conflict analysis */
   SCIP_Longint          npropsuccess;       /**< number of calls yielding at least one conflict constraint */
   SCIP_Longint          npropconfconss;     /**< number of valid conflict constraints detected in propagation conflict analysis */
   SCIP_Longint          npropconfliterals;  /**< total number of literals in valid propagation conflict constraints */
   SCIP_Longint          npropreconvconss;   /**< number of reconvergence constraints detected in propagation conflict analysis */
   SCIP_Longint          npropreconvliterals;/**< total number of literals in valid propagation reconvergence constraints */
   SCIP_Longint          ninflpcalls;        /**< number of calls to infeasible LP conflict analysis */
   SCIP_Longint          ninflpsuccess;      /**< number of calls yielding at least one conflict constraint */
   SCIP_Longint          ninflpconfconss;    /**< number of valid conflict constraints detected in infeasible LP conflict
                                              *   analysis */
   SCIP_Longint          ninflpconfliterals; /**< total number of literals in valid infeasible LP conflict constraints */
   SCIP_Longint          ninflpreconvconss;  /**< number of reconvergence constraints detected in infeasible LP conflict
                                              *   analysis */
   SCIP_Longint          ninflpreconvliterals; /**< total number of literals in valid infeasible LP reconvergence
                                                *   constraints */
   SCIP_Longint          ninflpiterations;   /**< total number of LP iterations used in infeasible LP conflict analysis */
   SCIP_Longint          nboundlpcalls;      /**< number of calls to bound exceeding LP conflict analysis */
   SCIP_Longint          nboundlpsuccess;    /**< number of calls yielding at least one conflict constraint */
   SCIP_Longint          nboundlpconfconss;  /**< number of valid conflict constraints detected in bound exceeding LP
                                              *   conflict analysis */
   SCIP_Longint          nboundlpconfliterals; /**< total number of literals in valid bound exceeding LP conflict
                                                *   constraints */
   SCIP_Longint          nboundlpreconvconss;/**< number of reconvergence constraints detected in bound exceeding LP
                                              *   conflict analysis */
   SCIP_Longint          nboundlpreconvliterals; /**< total number of literals in valid bound exceeding LP reconvergence
                                                  *   constraints */
   SCIP_Longint          nboundlpiterations; /**< total number of LP iterations used in bound exceeding LP conflict
                                              *   analysis */
   SCIP_Longint          nsbcalls;           /**< number of calls to infeasible strong branching conflict analysis */
   SCIP_Longint          nsbsuccess;         /**< number of calls yielding at least one conflict constraint */
   SCIP_Longint          nsbconfconss;       /**< number of conflict constraints detected in strong branching conflict analysis */
   SCIP_Longint          nsbconfliterals;    /**< total number of literals in valid strong branching conflict constraints */
   SCIP_Longint          nsbreconvconss;     /**< number of reconvergence constraints detected in strong branch conflict analysis */
   SCIP_Longint          nsbreconvliterals;  /**< total number of literals in valid strong branching reconvergence constraints */
   SCIP_Longint          nsbiterations;      /**< total number of LP iterations used in strong branching conflict analysis */
   SCIP_Longint          npseudocalls;       /**< number of calls to pseudo solution conflict analysis */
   SCIP_Longint          npseudosuccess;     /**< number of calls yielding at least one conflict constraint */
   SCIP_Longint          npseudoconfconss;   /**< number of valid conflict constraints detected in pseudo sol conflict analysis */
   SCIP_Longint          npseudoconfliterals;/**< total number of literals in valid pseudo solution conflict constraints */
   SCIP_Longint          npseudoreconvconss; /**< number of reconvergence constraints detected in pseudo sol conflict analysis */
   SCIP_Longint          npseudoreconvliterals;/**< total number of literals in valid pseudo solution reconvergence constraints */
   SCIP_CLOCK*           propanalyzetime;    /**< time used for propagation conflict analysis */
   SCIP_CLOCK*           inflpanalyzetime;   /**< time used for infeasible LP conflict analysis */
   SCIP_CLOCK*           boundlpanalyzetime; /**< time used for bound exceeding LP conflict analysis */
   SCIP_CLOCK*           sbanalyzetime;      /**< time used for strong branching LP conflict analysis */
   SCIP_CLOCK*           pseudoanalyzetime;  /**< time used for pseudo solution conflict analysis */
   SCIP_PQUEUE*          bdchgqueue;         /**< unprocessed conflict bound changes */
   SCIP_PQUEUE*          forcedbdchgqueue;   /**< unprocessed conflict bound changes that must be resolved */
   SCIP_CONFLICTSET*     conflictset;        /**< bound changes resembling the current conflict set */
   SCIP_CONFLICTSET**    conflictsets;       /**< conflict sets found at the current node */
   SCIP_Real*            conflictsetscores;  /**< score values of the conflict sets found at the current node */
   SCIP_BDCHGINFO**      tmpbdchginfos;      /**< temporarily created bound change information data */
   int                   conflictsetssize;   /**< size of conflictsets array */
   int                   nconflictsets;      /**< number of available conflict sets (used slots in conflictsets array) */
   int                   tmpbdchginfossize;  /**< size of tmpbdchginfos array */
   int                   ntmpbdchginfos;     /**< number of temporary created bound change information data */
   int                   count;              /**< conflict set counter to label binary conflict variables with */
};

#ifdef __cplusplus
}
#endif

#endif

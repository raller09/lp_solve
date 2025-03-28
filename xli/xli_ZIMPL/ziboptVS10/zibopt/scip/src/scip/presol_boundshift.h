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
#pragma ident "@(#) $Id: presol_boundshift.h,v 1.5 2010/01/04 20:35:45 bzfheinz Exp $"

/**@file   presol_boundshift.h
 * @brief  presolver that converts integer variables with domain [a,b] to integer variables with domain [0,b-a]
 * @author Tobias Achterberg
 * @author Michael Winkler
 */

/*---+----1----+----2----+----3----+----4----+----5----+----6----+----7----+----8----+----9----+----0----+----1----+----2*/

#ifndef __SCIP_PRESOL_BOUNDSHIFT_H__
#define __SCIP_PRESOL_BOUNDSHIFT_H__


#include "scip/scip.h"

#ifdef __cplusplus
extern "C" {
#endif

/** creates the boundshift presolver and includes it in SCIP */
extern
SCIP_RETCODE SCIPincludePresolBoundshift(
   SCIP*                 scip                /**< SCIP data structure */
   );

#ifdef __cplusplus
}
#endif

#endif

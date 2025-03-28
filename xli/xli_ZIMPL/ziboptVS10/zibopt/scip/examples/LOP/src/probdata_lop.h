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
#pragma ident "@(#) $Id: probdata_lop.h,v 1.6 2010/07/04 18:29:56 bzfpfets Exp $"

/**@file   probdata_lop.h
 * @brief  handling of data needed for solving linear ordering problems
 * @author Marc Pfetsch
 */

#ifndef __LOP_PROBDATA_LOP__
#define __LOP_PROBDATA_LOP__

#include <scip/scip.h>

#ifdef __cplusplus
extern "C" {
#endif

extern
SCIP_RETCODE LOPcreateProb(
   SCIP*                 scip,               /**< SCIP data structure */
   const char*           filename            /**< name of file to read */
   );

extern
SCIP_RETCODE LOPgenerateModel(
   SCIP*                 scip                /**< SCIP data structure */
   );

extern
SCIP_RETCODE LOPevalSolution(
   SCIP*                 scip                /**< SCIP data structure */
   );

extern
int LOPgetNElements(
   SCIP*                 scip                /**< SCIP data structure */
   );

#ifdef __cplusplus
}
#endif

#endif

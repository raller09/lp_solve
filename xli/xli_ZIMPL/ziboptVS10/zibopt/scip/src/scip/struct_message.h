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
#pragma ident "@(#) $Id: struct_message.h,v 1.9 2010/01/04 20:35:50 bzfheinz Exp $"

/**@file   struct_message.h
 * @brief  datastructures for problem statistics
 * @author Tobias Achterberg
 */

/*---+----1----+----2----+----3----+----4----+----5----+----6----+----7----+----8----+----9----+----0----+----1----+----2*/

#ifndef __SCIP_STRUCT_MESSAGE_H__
#define __SCIP_STRUCT_MESSAGE_H__


#include "scip/def.h"
#include "scip/type_message.h"

#ifdef __cplusplus
extern "C" {
#endif

/** message handler to redirect output */
struct SCIP_Messagehdlr
{
   SCIP_DECL_MESSAGEERROR((*messageerror));  /**< error message print method of message handler */
   SCIP_DECL_MESSAGEWARNING((*messagewarning));/**< warning message print method of message handler */
   SCIP_DECL_MESSAGEDIALOG((*messagedialog));/**< dialog message print method of message handler */
   SCIP_DECL_MESSAGEINFO ((*messageinfo));   /**< info message print method of message handler */
   SCIP_MESSAGEHDLRDATA* messagehdlrdata;    /**< message handler data */
   char*                 errorbuffer;        /**< buffer for constructing complete error output lines */
   char*                 warningbuffer;      /**< buffer for constructing complete warning output lines */
   char*                 dialogbuffer;       /**< buffer for constructing complete dialog output lines */
   char*                 infobuffer;         /**< buffer for constructing complete info output lines */
   int                   errorbufferlen;     /**< currently used space in the error buffer */
   int                   warningbufferlen;   /**< currently used space in the warning buffer */
   int                   dialogbufferlen;    /**< currently used space in the dialog buffer */
   int                   infobufferlen;      /**< currently used space in the info buffer */
};

#ifdef __cplusplus
}
#endif

#endif

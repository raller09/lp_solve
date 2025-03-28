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
/*  You should have received a copy of the ZIB Academic License.             */
/*  along with SCIP; see the file COPYING. If not email to scip@zib.de.      */
/*                                                                           */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#pragma ident "@(#) $Id: objdisp.h,v 1.11 2010/09/01 16:33:16 bzfheinz Exp $"

/**@file   objdisp.h
 * @brief  C++ wrapper for display column
 * @author Kati Wolter
 */

/*---+----1----+----2----+----3----+----4----+----5----+----6----+----7----+----8----+----9----+----0----+----1----+----2*/

#ifndef __SCIP_OBJDISP_H__
#define __SCIP_OBJDISP_H__

#include <cstring>

#include "scip/scip.h"
#include "objscip/objcloneable.h"

namespace scip
{

/** C++ wrapper object for display columns */
class ObjDisp : public ObjCloneable
{
public:
   /*lint --e{1540}*/
   
   /** SCIP data structure */
   SCIP* scip_;

   /** name of the display column */
   char* scip_name_;
   
   /** description of the display column */
   char* scip_desc_;
   
   /** head line of the display column */
   char* scip_header_;

   /** width of the display column (no. of chars used) */
   const int scip_width_;

   /** priority of the display column */
   const int scip_priority_;

   /** relative position of the display column */
   const int scip_position_;

   /** should the column be separated with a line from its right neighbour? */
   const SCIP_Bool scip_stripline_;

   /** default constructor */
   ObjDisp(
      SCIP*              scip,               /**< SCIP data structure */
      const char*        name,               /**< name of display column */
      const char*        desc,               /**< description of display column */
      const char*        header,             /**< head line of display column */
      int                width,              /**< width of display column (no. of chars used) */
      int                priority,           /**< priority of display column */
      int                position,           /**< relative position of display column */
      SCIP_Bool          stripline           /**< should the column be separated with a line from its right neighbour? */
      )
      : scip_(scip),
        scip_name_(0),
        scip_desc_(0),
        scip_header_(0),
        scip_width_(width),
        scip_priority_(priority),
        scip_position_(position),
        scip_stripline_(stripline)
   {
      /* the macro SCIPduplicateMemoryArray does not need the first argument: */
      SCIP_CALL_ABORT( SCIPduplicateMemoryArray(scip_, &scip_name_, name, std::strlen(name)+1) );
      SCIP_CALL_ABORT( SCIPduplicateMemoryArray(scip_, &scip_desc_, desc, std::strlen(desc)+1) );
      SCIP_CALL_ABORT( SCIPduplicateMemoryArray(scip_, &scip_header_, header, std::strlen(header)+1) );
   }

   /** destructor */
   virtual ~ObjDisp()
   {
      /* the macro SCIPfreeMemoryArray does not need the first argument: */
      /*lint --e{64}*/
      SCIPfreeMemoryArray(scip_, &scip_name_);
      SCIPfreeMemoryArray(scip_, &scip_desc_);
      SCIPfreeMemoryArray(scip_, &scip_header_);
   }

   /** destructor of display column to free user data (called when SCIP is exiting) */
   virtual SCIP_RETCODE scip_free(
      SCIP*              scip,               /**< SCIP data structure */
      SCIP_DISP*         disp                /**< the display column itself */
      )
   {  /*lint --e{715}*/
      return SCIP_OKAY;
   }
   
   /** initialization method of display column (called after problem was transformed) */
   virtual SCIP_RETCODE scip_init(
      SCIP*              scip,               /**< SCIP data structure */
      SCIP_DISP*         disp                /**< the display column itself */
      )
   {  /*lint --e{715}*/
      return SCIP_OKAY;
   }
   
   /** deinitialization method of display column (called before transformed problem is freed) */
   virtual SCIP_RETCODE scip_exit(
      SCIP*              scip,               /**< SCIP data structure */
      SCIP_DISP*         disp                /**< the display column itself */
      )
   {  /*lint --e{715}*/
      return SCIP_OKAY;
   }

   /** solving process initialization method of display column (called when branch and bound process is about to begin)
    *
    *  This method is called when the presolving was finished and the branch and bound process is about to begin.
    *  The display column may use this call to initialize its branch and bound specific data.
    */
   virtual SCIP_RETCODE scip_initsol(
      SCIP*              scip,               /**< SCIP data structure */
      SCIP_DISP*         disp                /**< the display column itself */
      )
   {  /*lint --e{715}*/
      return SCIP_OKAY;
   }
   
   /** solving process deinitialization method of display column (called before branch and bound process data is freed)
    *
    *  This method is called before the branch and bound process is freed.
    *  The display column should use this call to clean up its branch and bound data.
    */
   virtual SCIP_RETCODE scip_exitsol(
      SCIP*              scip,               /**< SCIP data structure */
      SCIP_DISP*         disp                /**< the display column itself */
      )
   {  /*lint --e{715}*/
      return SCIP_OKAY;
   }
   
   /** output method of display column to output file stream 'file'
    */
   virtual SCIP_RETCODE scip_output(
      SCIP*              scip,               /**< SCIP data structure */
      SCIP_DISP*         disp,               /**< the display column itself */
      FILE*              file                /**< file stream for output */    
      ) = 0;
};

} /* namespace scip */


   
/** creates the display column for the given display column object and includes it in SCIP
 *
 *  The method should be called in one of the following ways:
 *
 *   1. The user is resposible of deleting the object:
 *       SCIP_CALL( SCIPcreate(&scip) );
 *       ...
 *       MyDisp* mydisp = new MyDisp(...);
 *       SCIP_CALL( SCIPincludeObjDisp(scip, &mydisp, FALSE) );
 *       ...
 *       SCIP_CALL( SCIPfree(&scip) );
 *       delete mydisp;    // delete disp AFTER SCIPfree() !
 *
 *   2. The object pointer is passed to SCIP and deleted by SCIP in the SCIPfree() call:
 *       SCIP_CALL( SCIPcreate(&scip) );
 *       ...
 *       SCIP_CALL( SCIPincludeObjDisp(scip, new MyDisp(...), TRUE) );
 *       ...
 *       SCIP_CALL( SCIPfree(&scip) );  // destructor of MyDisp is called here
 */
extern
SCIP_RETCODE SCIPincludeObjDisp(
   SCIP*                 scip,               /**< SCIP data structure */
   scip::ObjDisp*        objdisp,            /**< display column object */
   SCIP_Bool             deleteobject        /**< should the display column object be deleted when display column is freed? */
   );

/** returns the display column object of the given name, or 0 if not existing */
extern
scip::ObjDisp* SCIPfindObjDisp(
   SCIP*                 scip,               /**< SCIP data structure */
   const char*           name                /**< name of display column */
   );

/** returns the display column object for the given display column */
extern
scip::ObjDisp* SCIPgetObjDisp(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_DISP*            disp                /**< display column */
   );

#endif

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                           */
/*                  This file is part of the class library                   */
/*       SoPlex --- the Sequential object-oriented simPlex.                  */
/*                                                                           */
/*    Copyright (C) 1996      Roland Wunderling                              */
/*                  1996-2010 Konrad-Zuse-Zentrum                            */
/*                            fuer Informationstechnik Berlin                */
/*                                                                           */
/*  SoPlex is distributed under the terms of the ZIB Academic Licence.       */
/*                                                                           */
/*  You should have received a copy of the ZIB Academic License              */
/*  along with SoPlex; see the file COPYING. If not email to soplex@zib.de.  */
/*                                                                           */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#pragma ident "@(#) $Id: spxdesc.cpp,v 1.22 2010/09/16 17:45:03 bzfgleix Exp $"

//#define DEBUGGING 1

#include <iostream>

#include "spxdefines.h"
#include "spxbasis.h"
#include "spxsolver.h"
#include "exceptions.h"

namespace soplex
{
 
SPxBasis::Desc::Desc(const SPxSolver& base)
{
   rowstat.reSize(base.nRows());
   colstat.reSize(base.nCols());

   if (base.rep() == SPxSolver::ROW)
   {
      stat   = &rowstat;
      costat = &colstat;
   }
   else
   {
      assert(base.rep() == SPxSolver::COLUMN);

      stat   = &colstat;
      costat = &rowstat;
   }

   assert(Desc::isConsistent());
}

SPxBasis::Desc::Desc(const Desc& old)
   : rowstat(old.rowstat)
   , colstat(old.colstat)
{
   if (old.stat == &old.rowstat)
   {
      assert(old.costat == &old.colstat);
      
      stat   = &rowstat;
      costat = &colstat;
   }
   else
   {
      assert(old.costat == &old.rowstat);
      
      stat   = &colstat;
      costat = &rowstat;
   }

   assert(Desc::isConsistent());
}

SPxBasis::Desc& SPxBasis::Desc::operator=(const SPxBasis::Desc& rhs)
{
   if (this != &rhs)
   {
      rowstat = rhs.rowstat;
      colstat = rhs.colstat;
      
      if (rhs.stat == &rhs.rowstat)
      {
         assert(rhs.costat == &rhs.colstat);
         
         stat   = &rowstat;
         costat = &colstat;
      }
      else
      {
         assert(rhs.costat == &rhs.rowstat);
         
         stat   = &colstat;
         costat = &rowstat;
      }

      assert(Desc::isConsistent());
   }
   return *this;
}

void SPxBasis::Desc::reSize(int rowDim, int colDim)
{
   METHOD( "SPxBasis::Desc::reSize()" );
   rowstat.reSize(rowDim);
   colstat.reSize(colDim);
}

void SPxBasis::Desc::dump() const
{
   METHOD( "SPxBasis::Desc::dump()" );
   int i;

   // Dump regardless of the verbosity level if this method is called.
   const SPxOut::Verbosity tmp_verbosity = spxout.getVerbosity();
   spxout.setVerbosity( SPxOut::ERROR );

   spxout << "DBDESC01 column status: ";
   for(i = 0; i < nCols(); i++)
      spxout << colStatus(i);
   spxout << std::endl;

   spxout << "DBDESC02 row status:    ";
   for(i = 0; i < nRows(); i++)
      spxout << rowStatus(i); 
   spxout << std::endl;
   spxout.setVerbosity( tmp_verbosity );
}

#ifndef NO_CONSISTENCY_CHECKS
bool SPxBasis::Desc::isConsistent() const
{
   METHOD( "SPxBasis::Desc::isConsistent()" );
   return rowstat.isConsistent() && colstat.isConsistent();
}
#endif

std::ostream& operator<<(std::ostream& os, const SPxBasis::Desc::Status& stat)
{
   char text;
   
   switch(stat)
   {
   case SPxBasis::Desc::P_ON_LOWER :
      text = 'L';
      break;
   case SPxBasis::Desc::P_ON_UPPER :
      text = 'U';
      break;
   case SPxBasis::Desc::P_FREE :
      text = 'F';
      break;
   case SPxBasis::Desc::P_FIXED :
      text = 'X';
      break;
   case SPxBasis::Desc::D_FREE :
      text = 'f';
      break;
   case SPxBasis::Desc::D_ON_UPPER :
      text = 'u';
      break;
   case SPxBasis::Desc::D_ON_LOWER :
      text = 'l';
      break;
   case SPxBasis::Desc::D_ON_BOTH :
      text = 'x';
      break;
   case SPxBasis::Desc::D_UNDEFINED :
      text = '.';
      break;
   default :
      os << std::endl << "Invalid status <" << int(stat) << ">" << std::endl;
      throw SPxInternalCodeException("XSPXDE01 This should never happen.");
   }
   os << text;

   return os;
}

} // namespace soplex

//-----------------------------------------------------------------------------
//Emacs Local Variables:
//Emacs mode:c++
//Emacs c-basic-offset:3
//Emacs tab-width:8
//Emacs indent-tabs-mode:nil
//Emacs End:
//-----------------------------------------------------------------------------

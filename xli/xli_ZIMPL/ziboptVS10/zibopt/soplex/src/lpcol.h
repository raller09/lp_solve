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
#pragma ident "@(#) $Id: lpcol.h,v 1.17 2010/09/16 17:45:02 bzfgleix Exp $"

/**@file  lpcol.h
 * @brief LP column.
 */
#ifndef _LPCOL_H_
#define _LPCOL_H_

#include <assert.h>

#include "spxdefines.h"
#include "dsvector.h"

namespace soplex
{
/**@brief   LP column.
   @ingroup Algo

   Class LPCol provides a datatype for storing the column of an LP a the
   form similar to
   \f[
      \begin{array}{rl}
         \hbox{max}  & c^T x         \\
         \hbox{s.t.} & Ax \le b      \\
                     & l \le x \le u
      \end{array}
   \f]
   Hence, an LPCol consists of an objective value, a column DSVector and
   an upper and lower bound to the corresponding variable, which may include
   \f$\pm\infty\f$. However, it depends on the LP code to use, what values are
   actually treated as \f$\infty\f$.
 */
class LPCol
{
private:

   //------------------------------------
   /**@name Data */
   //@{
   Real   up;           ///< upper bound
   Real   low;          ///< lower bound
   Real   object;       ///< objective value
   DSVector vec;        ///< the column vector
   //@}

public:

   //------------------------------------
   /**@name Construction / destruction */
   //@{
   /// default constructor.
   /** Construct LPCol with a column vector ready for taking \p defDim
    *  nonzeros.
    */
   explicit LPCol(int defDim = 0)
      : up(infinity), low(0), object(0), vec(defDim)
   {
      assert(isConsistent());  
   }

   /// initializing constructor.
   /*  Construct LPCol with the given objective value \p obj, a column
    *  %vector \p vec, upper bound \p upper and lower bound \p lower.
    */
   LPCol(Real p_obj, const SVector& p_vector, Real p_upper, Real p_lower)
      : up(p_upper), low(p_lower), object(p_obj), vec(p_vector)
   {
      assert(isConsistent());
   }

   /// copy constructor.
   LPCol(const LPCol& old)
      : up(old.up), low(old.low), object(old.object), vec(old.vec)
   {
      assert(isConsistent());
   }

   /// destructor
   ~LPCol()
   {}
   //@}

   //------------------------------------
   /**@name Access / modification */
   //@{
   /// get objective value.
   Real obj() const
   {
      return object;
   }
   /// access objective value.
   void setObj(Real p_object)
   {
      object = p_object;
   }

   /// get upper bound.
   Real upper() const
   {
      return up;
   }
   /// access upper bound.
   void setUpper(Real p_up)
   {
      up = p_up;
   }

   /// get lower bound.
   Real lower() const
   {
      return low;
   }
   /// access lower bound.
   void setLower(Real p_low)
   {
      low = p_low;
   }

   /// get constraint column vector.
   const SVector& colVector() const
   {
      return vec;
   }

   /// access constraint column vector.
   void setColVector(const SVector& p_vec)
   {
      vec = p_vec;
   }
   //@}

#ifndef NO_CONSISTENCY_CHECKS
   //------------------------------------
   /**@name Consistency check */
   //@{
   /// check consistency.
   bool isConsistent() const
   {
      return vec.isConsistent();
   }
   //@}
#endif
};
} // namespace soplex
#endif // _LPCOL_H_

//-----------------------------------------------------------------------------
//Emacs Local Variables:
//Emacs mode:c++
//Emacs c-basic-offset:3
//Emacs tab-width:8
//Emacs indent-tabs-mode:nil
//Emacs End:
//-----------------------------------------------------------------------------

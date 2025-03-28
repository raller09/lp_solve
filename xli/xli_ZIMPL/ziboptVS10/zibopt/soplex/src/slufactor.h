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
#pragma ident "@(#) $Id: slufactor.h,v 1.33 2010/10/01 19:30:47 bzfwinkm Exp $"

/**@file  slufactor.h
 * @brief Implementation of Sparse Linear Solver.
 */
#ifndef _SLUFACTOR_H_
#define _SLUFACTOR_H_

#include <assert.h>

#include "spxdefines.h"
#include "dvector.h"
#include "slinsolver.h"
#include "clufactor.h"

namespace soplex
{
/// maximum nr. of factorization updates allowed before refactorization.
#define MAXUPDATES      1000     

/**@brief   Implementation of Sparse Linear Solver.
 * @ingroup Algo
 * 
 * This class implements a #SLinSolver interface by using the sparse LU
 * factorization implementet in #CLUFactor.
 */
class SLUFactor : public SLinSolver, protected CLUFactor
{
public:

   //--------------------------------
   /**@name Types */
   //@{
   /// Specifies how to perform \ref soplex::SLUFactor::change "change" method.
   enum UpdateType
   {
      ETA = 0,       ///<
      FOREST_TOMLIN  ///<
   };
   /// for convenience
   typedef SLinSolver::Status Status;
   //@}

private:

   //--------------------------------
   /**@name Private data */
   //@{
   DVector    vec;           ///< Temporary vector
   SSVector   ssvec;         ///< Temporary semi-sparse vector
   //@}

protected:

   //--------------------------------
   /**@name Protected data */
   //@{
   bool       usetup;        ///< TRUE iff update vector has been setup
   UpdateType uptype;        ///< the current \ref soplex::SLUFactor::UpdateType "UpdateType".
   SSVector   eta;           ///< 
   SSVector   forest;        ///< ? Update vector set up by solveRight4update() and solve2right4update()
   Real       lastThreshold; ///< pivoting threshold of last factorization
   //@}

   //--------------------------------
   /**@name Control Parameters */
   //@{
   /// minimum threshold to use.
   Real minThreshold;
   /// minimum stability to achieve by setting threshold.
   Real minStability;
   /// |x| < epsililon is considered to be 0.
   Real epsilon;
   /// Time spent in solves
   Timer   solveTime; 
   /// Number of solves
   int     solveCount;
   //@}

protected:

   //--------------------------------
   /**@name Protected helpers */
   //@{
   ///
   void freeAll();
   ///
   void changeEta(int idx, SSVector& eta);
   //@}


public:

   //--------------------------------
   /**@name Update type */
   //@{
   /// returns the current update type uptype.
   UpdateType utype() const
   {
      return uptype;
   }

   /// sets update type.
   /** The new UpdateType becomes valid only after the next call to
       method load().
   */
   void setUtype(UpdateType tp)
   {
      uptype = tp;
   }
   //@}

   //--------------------------------
   /**@name Derived from SLinSolver
      See documentation of \ref soplex::SLinSolver "SLinSolver" for a 
      documentation of these methods.
   */
   //@{
   ///
   void clear();
   ///
   int dim() const
   {
      return thedim;
   }
   ///
   int memory() const
   {
      return nzCnt + l.start[l.firstUnused];
   }
   ///
   const char* getName() const
   {
      return (uptype == SLUFactor::ETA) ? "SLU-Eta" : "SLU-Forest-Tomlin";
   }
   ///
   Status status() const
   {
      return Status(stat);
   }
   ///
   Real stability() const;
   ///
   std::string statistics() const;
   ///
   Status load(const SVector* vec[], int dim);
   //@}

public:

   //--------------------------------
   /**@name Solve */
   //@{
   /// Solves \f$Ax=b\f$.
   void solveRight (Vector& x, const Vector& b);
   /// Solves \f$Ax=b\f$.
   void solveRight (SSVector& x, const SVector& b);
   /// Solves \f$Ax=b\f$.
   void solveRight4update(SSVector& x, const SVector& b);
   /// Solves \f$Ax=b\f$ and \f$Ay=d\f$.
   void solve2right4update(SSVector& x, Vector& y, const SVector& b, SSVector& d);
   /// Solves \f$Ax=b\f$.
   void solveLeft(Vector& x, const Vector& b);
   /// Solves \f$Ax=b\f$.
   void solveLeft(SSVector& x, const SVector& b);
   /// Solves \f$Ax=b\f$ and \f$Ay=d\f$.
   void solveLeft(SSVector& x, Vector& y, const SVector& b, SSVector& d);
   ///
   Status change(int idx, const SVector& subst, const SSVector* eta = 0);
   //@}

   //--------------------------------
   /**@name Miscellaneous */
   //@{
   /// time spent in factorizations
   Real getFactorTime() const
   {
      return factorTime.userTime();
   }
   /// number of factorizations performed
   int getFactorCount() const
   {
      return factorCount;
   }
   /// time spent in solves
   Real getSolveTime() const
   {
      return solveTime.userTime();
   }
   /// number of solves performed
   int getSolveCount() const
   {
      return solveCount;
   }
   /// prints the LU factorization to stdout.
   void dump() const;

#ifndef NO_CONSISTENCY_CHECKS
   /// consistency check.
   bool isConsistent() const;
#endif
   //@}

   //------------------------------------
   /**@name Constructors / Destructors */
   //@{
   /// default constructor.
   SLUFactor();
   /// assignment operator.
   SLUFactor& operator=(const SLUFactor& old);
   /// copy constructor.
   SLUFactor(const SLUFactor& old);
   /// destructor.
   virtual ~SLUFactor();
   /// clone function for polymorphism
   inline virtual SLinSolver* clone() const
   {
      return new SLUFactor(*this);
   }
   //@}

private:

   //------------------------------------
   /**@name Private helpers */
   //@{
   /// used to implement the assignment operator
   void assign(const SLUFactor& old);
   //@}
};

} // namespace soplex
#endif // _SLUFACTOR_H_

//-----------------------------------------------------------------------------
//Emacs Local Variables:
//Emacs mode:c++
//Emacs c-basic-offset:3
//Emacs tab-width:8
//Emacs indent-tabs-mode:nil
//Emacs End:
//-----------------------------------------------------------------------------

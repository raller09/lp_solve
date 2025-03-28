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
#pragma ident "@(#) $Id: ssvector.h,v 1.26 2010/09/16 17:45:04 bzfgleix Exp $"


/**@file  ssvector.h
 * @brief Semi sparse vector.
 */
#ifndef _SSVECTOR_H_
#define _SSVECTOR_H_

#include <assert.h>

#include "spxdefines.h"
#include "dvector.h"
#include "svector.h"
#include "didxset.h"
#include "spxalloc.h"

namespace soplex
{
class SVSet;

/**@brief   Semi sparse vector.
   @ingroup Algebra

   This class implements Semi Sparse Vectors. Such are
   #DVector%s where the indices of its nonzero elements can be stored in an
   extra IdxSet. Only elements with absolute value > #epsilon are considered
   to be nonzero. 
   Since really storing the nonzeros is not always convenient,
   an SSVector provides two different stati: setup and not setup.
   An SSVector being setup means that the nonzero indices are available,
   otherwise an SSVector is just an ordinary Vector with an empty IdxSet.
   Note that due to arithmetic operation, zeros can slip in, i.e., it is only
   guaranteed that at least every non-zero is in the IdxSet.
*/
class SSVector : protected DVector, protected IdxSet
{
private:

   friend class DVector;
   friend class Vector;
   friend class DSVector;

   //--------------------------------------------
   /**@name Data */
   //@{
   /// Is the SSVector set up?
   bool setupStatus;
   /// Allocates enough space to accommodate \p newmax values.
   void setMax(int newmax);
   /// A value x with |x| < epsilon is considered zero.
   Real epsilon;
   //@}

public:

   //--------------------------------------------
   /**@name Status of an SSVector
      An SSVector can be set up or not. In case it is set up, its IdxSet
      correctly contains all indices of nonzero elements of the SSVector.
      Otherwise, it does not contain any useful data. Whether or not an
      SSVector is setup can be determined with the method 
      \ref soplex::SSVector::isSetup() "isSetup()".
      
      There are three methods for directly affecting the setup status of an
      SSVector:
      - unSetup():     This method sets the status to ``not setup''.
      - setup():       This method initializes the IdxSet to the
                       SSVector's nonzero indices and sets the status
                       to ``setup''.
      - forceSetup():  This method sets the status to ``setup'' without
                       verifying that the IdxSet correctly contains
                       all nonzero indices. It may be used when the
                       nonzero indices have been computed externally.
   */
   //@{
   /// only used in slufactor.cpp
   Real* get_ptr()
   {
      return DVector::get_ptr();
   }
   /// returns the non-zero epsilon used.
   Real getEpsilon() const
   {
      return epsilon;
   }
   /// sets the non-zero epsilon.
   /** This invalidates the setup.
    */
   void setEpsilon(Real eps)
   {
      epsilon     = eps;
      setupStatus = false;
   }
   /// returns setup status.
   bool isSetup() const
   {
      return setupStatus;
   }

   /// makes SSVector not setup.
   void unSetup()
   {
      setupStatus = false;
   }

   /// initializes nonzero indices
   /** Initializes nonzero indices for all elements with absolute values
       greater than #epsilon and sets all other elements to 0.
   */
   void setup();
   
   /// forces setup status.
   void forceSetup()
   {
      setupStatus = true;
   }
   //@}


   //--------------------------------------------
   /**@name Methods for setup SSVectors */
   //@{
   /// returns index of the \p n 'th nonzero element.
   int index(int n) const
   {
      assert(isSetup());
      return IdxSet::index(n);
   }

   /// returns value of the \p n 'th nonzero element.
   Real value(int n) const
   {
      assert(isSetup());
      assert(n >= 0 && n < size());
      return val[idx[n]];
   }

   /// returns the position number of index \p i, or -1 if \p i doesn't exist.
   int number(int i) const
   {
      assert(isSetup());
      return IdxSet::number(i);
   }

   /// returns the number of nonzeros.
   int size() const
   {
      assert(isSetup());
      return IdxSet::size();
   }

   /// adds nonzero (\p i, \p x) to SSVector.
   /** No nonzero with index \p i must exist in the SSVector.
    */
   void add(int i, Real x)
   {
      assert(val[i] == 0);
      assert(number(i) < 0);
      addIdx(i);
      val[i] = x;
   }

   /// sets \p i 'th element to \p x.
   void setValue(int i, Real x);

   /// clears element \p i.
   void clearIdx(int i)
   {
      if (isSetup())
      {
         int n = number(i);
         if (n >= 0)
            remove(n);
      }
      val[i] = 0;

      assert(isConsistent());
   }

   /// sets \p n 'th nonzero element to 0 (index \p n must exist!).
   void clearNum(int n)
   {
      assert(isSetup());
      assert(index(n) >= 0);
      val[index(n)] = 0;
      remove(n);

      assert(isConsistent());
   }
   //@}


   //--------------------------------------------
   /**@name Methods independent of the Status */
   //@{
   /// returns \p i 'th value.
   Real operator[](int i) const
   {
      return val[i];
   }

   /// returns array indices.
   const int* indexMem() const
   {
      return idx;
   }

   /// returns array values.
   const Real* values() const
   {
      return val;
   }

   /// returns indices.
   const IdxSet& indices() const
   {
      return *this;
   }

   /// returns array indices.
   int* altIndexMem()
   {
      unSetup();
      return idx;
   }

   /// returns array values.
   Real* altValues()
   {
      unSetup();
      return val;
   }

   /// returns indices.
   IdxSet& altIndices()
   {
      unSetup();
      return *this;
   }
   //@}


   //------------------------------------
   /**@name Mathematical operations */
   //@{
   ///
   SSVector& operator+=(const Vector& vec);
   ///
   SSVector& operator+=(const SVector& vec);
   /// vector summation.
   SSVector& operator+=(const SSVector& vec);

   ///
   SSVector& operator-=(const Vector& vec);
   ///
   SSVector& operator-=(const SVector& vec);
   /// vector subtraction.
   SSVector& operator-=(const SSVector& vec);

   /// vector scaling.
   SSVector& operator*=(Real x);

   ///
   //SSVector& multAdd(Real x, const SSVector& vec);
   ///
   SSVector& multAdd(Real x, const SVector& vec);
   /// adds scaled vector (+= \p x * \p vec).
   SSVector& multAdd(Real x, const Vector& vec);
   /// assigns SSVector to \f$x^T \cdot A\f$.
   SSVector& assign2product(const SSVector& x, const SVSet& A);
   /// assigns SSVector to \f$A \cdot x\f$ for a setup \p x.
   SSVector& assign2product4setup(const SVSet& A, const SSVector& x);

public:

   /// assigns SSVector to \f$A \cdot x\f$ thereby setting up \p x.
   SSVector& assign2productAndSetup(const SVSet& A, SSVector& x);

   /// returns infinity norm of a Vector.
   Real maxAbs() const;
   /// returns euclidian norm of a Vector.
   Real length() const;
   /// returns squared norm of a Vector.
   Real length2() const;
   //@}


   //------------------------------------
   /**@name Miscellaneous */
   //@{
   /// returns dimension of Vector.
   int dim() const
   {
      return dimen;
   }

   /// resets dimension to \p newdim.
   void reDim (int newdim);

   /// sets number of nonzeros (thereby unSetup SSVector).
   void setSize(int n)
   {
      assert(n >= 0);
      assert(n <= IdxSet::max());
      unSetup();
      num = n;
   }

   /// resets memory consumption to \p newsize.
   void reMem(int newsize);

   /// clears vector.
   void clear ();

#ifndef NO_CONSISTENCY_CHECKS
   /// consistency check.
   bool isConsistent() const;
#endif
   //@}


   //------------------------------------
   /**@name Constructors / Destructors */
   //@{
   /// constructor.
   explicit SSVector(int p_dim, Real p_eps = Param::epsilon())
      : DVector (p_dim)
      , IdxSet  ()
      , setupStatus(true)
      , epsilon (p_eps)
   {
      len = (p_dim < 1) ? 1 : p_dim;
      spx_alloc(idx, len);

      Vector::clear();

      assert(isConsistent());
   }

   /// copy constructor.
   SSVector(const SSVector& vec)
      : DVector (vec)
      , IdxSet ()
      , setupStatus(vec.setupStatus)
      , epsilon (vec.epsilon)
   {
      len = (vec.dim() < 1) ? 1 : vec.dim();
      spx_alloc(idx, len);
      IdxSet::operator= ( vec );

      assert(isConsistent());
   }

   /// constructs nonsetup copy of \p vec.
   explicit SSVector(const Vector& vec, Real eps = Param::epsilon())
      : DVector (vec)
      , IdxSet ()
      , setupStatus(false)
      , epsilon (eps)
   { 
      len = (vec.dim() < 1) ? 1 : vec.dim();
      spx_alloc(idx, len);
      /// TODO: @todo Is there an IdxSet::operator=( vec ) missing here?

      assert(isConsistent());
   }

   /// sets up \p rhs vector, and assigns it.
   void setup_and_assign(SSVector& rhs);

   /// assign only the elements of \p rhs.
   SSVector& assign(const SVector& rhs);
   /// assignment operator
   SSVector& operator=(const SSVector& rhs);
   /// assignment operator
   SSVector& operator=(const SVector& rhs);
   /// assignment operator
   SSVector& operator=(const Vector& rhs)
   {
      unSetup();
      Vector::operator=(rhs);

      assert(isConsistent());
      return *this;
   }
   /// destructor
   ~SSVector()
   {
      if ( idx )
         spx_free(idx);
   }
   //@}

private:

   //----------------------------
   /**@name Private helpers */
   //@{
   ///
   SSVector& assign2product1(const SVSet& A, const SSVector& x);
   ///
   SSVector& assign2productShort(const SVSet& A, const SSVector& x);
   ///
   SSVector& assign2productFull(const SVSet& A, const SSVector& x);
   //@}
};


// ----------------------------------------------------------------------------
//   Vector operators involving SSVectors
// ----------------------------------------------------------------------------

inline Vector& Vector::multAdd(Real x, const SSVector& svec)
{
   assert(svec.dim() <= dim());

   if (svec.isSetup())
   {
      const int* idx = svec.indexMem();

      for(int i = 0; i < svec.size(); i++)
         val[idx[i]] += x * svec[idx[i]];
   }
   else
   {
      assert(svec.dim() == dim());

      for(int i = 0; i < dim(); i++)
         val[i] += x * svec.val[i];
   }
   //multAdd(x, static_cast<const Vector&>(svec));

   return *this;
}

inline Vector& Vector::assign(const SSVector& svec)
{
   assert(svec.dim() <= dim());

   if (svec.isSetup())
   {
      const int* idx = svec.indexMem();

      for(int i = svec.size(); i > 0; i--)
      {
         val[*idx] = svec.val[*idx];
         idx++;
      }
   }
   else
      operator= (static_cast<const Vector&>(svec));

   return *this;
}

inline Vector& Vector::operator=(const SSVector& vec)
{
   if (vec.isSetup())
   {
      clear ();
      assign(vec);
   }
   else
      operator= (static_cast<const Vector&>(vec));

   return *this;
}

inline Real Vector::operator*(const SSVector& v) const
{
   assert(dim() == v.dim());

   if (v.isSetup())
   {
      const int* idx = v.indexMem();
      Real     x   = 0;

      for(int i = v.size(); i > 0; i--)
      {
         x += val[*idx] * v.val[*idx];
         idx++;
      }
      return x;
   }
   else
      return operator*(static_cast<const Vector&>(v));
}
} // namespace soplex
#endif // _SSVECTOR_H_

//-----------------------------------------------------------------------------
//Emacs Local Variables:
//Emacs mode:c++
//Emacs c-basic-offset:3
//Emacs tab-width:8
//Emacs indent-tabs-mode:nil
//Emacs End:
//-----------------------------------------------------------------------------

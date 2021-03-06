/* This file is part of Into.
 * Copyright (C) Intopii 2013.
 * All rights reserved.
 *
 * Licensees holding a commercial Into license may use this file in
 * accordance with the commercial license agreement. Please see
 * LICENSE.commercial for commercial licensing terms.
 *
 * Alternatively, this file may be used under the terms of the GNU
 * Affero General Public License version 3 as published by the Free
 * Software Foundation. In addition, Intopii gives you special rights
 * to use Into as a part of open source software projects. Please
 * refer to LICENSE.AGPL3 for details.
 */

#ifndef _PIIHOUSEHOLDERTRANSFORM_H
#define _PIIHOUSEHOLDERTRANSFORM_H

#include "PiiMatrix.h"
#include "PiiAlgorithm.h"
#include "PiiMath.h"
#include "PiiTransposedMatrix.h"

namespace Pii
{
  /**
   * Generates an elementary reflector (a.k.a the Householder
   * transformation). The Householder transformation of an
   * n-dimensional vector *x* is an n-by-n matrix *H* for which the
   * following equation holds:
   *
   * \[
   * H \left( \begin{array}{c} x_0 \\ \vdots \\ x{n_1} \end{array} \right) =
   * \left( \begin{array}{c} \beta \\ 0 \\ 0 \end{array} \right)
   * \]
   *
   * In other words, *H* is a matrix that effectively transforms *x*
   * into a scalar. The transformation matrix H can be represented as
   *
   * \[
   * H = I - \tau v v^T
   * \]
   *
   * where v is an n-dimensional vector whose first component equals
   * one. (If complex numbers were involved, the transpose should be
   * replaced with a Hermitian transpose, but this routine only
   * supports real input.)
   *
   * @param x an n-dimensional input/output vector. This vector will
   * be modified by the algorithm so that it contains the vector v.
   *
   * @param n the length of *x*
   *
   * @param tau a return-value parameter that will store the value of
   * \(\tau\). If *x* is a null vector, *tau* will be
   * zero. Otherwise, \(1 \le \tau \le 2\).
   *
   * @param beta an optional return-value parameter that will store
   * the value of \(\beta\). If *x* is a null vector, *beta* will
   * be set to zero.
   *
   * @see reflectColumns()
   */
  template <class Iterator>
  void householderTransform(Iterator x, int n,
                            typename std::iterator_traits<Iterator>::value_type* tau,
                            typename std::iterator_traits<Iterator>::value_type* beta = 0)
  {
    typedef typename std::iterator_traits<Iterator>::value_type Real;

    if (n <= 0)
      {
        *tau = 0;
        return;
      }
    else if (n == 1)
      {
        *tau = 0;
        if (beta != 0)
          *beta = *x;
        *x = 1;
        return;
      }

    // Scaling may be needed to avoid overflow/underflow.
    Real dMaxAbs = abs(*findSpecialValue(x, x+n,
                                         std::greater<Real>(),
                                         Abs<Real>()));
    Real dScale = 1.0;
    if (dMaxAbs != 0)
      {
        dScale = qBound(Numeric<Real>::smallestPositive()/epsilon<Real>(1.0),
                        dMaxAbs,
                        Numeric<Real>::maxValue()*epsilon<Real>(1.0));

        if (dScale != 1.0)
          {
            mapN(x, n, std::bind2nd(std::multiplies<Real>(), 1.0/dScale));
            dMaxAbs /= dScale;
          }
      }

    Real dAlpha = *x;
    Real dScaledNorm = 0;
    if (dMaxAbs != 0)
      {
        for (int i=1; i < n; ++i)
          dScaledNorm += square(x[i]/dMaxAbs);
        dScaledNorm = sqrt(dScaledNorm) * dMaxAbs;
      }

    if (dScaledNorm == 0)
      {
        // H  =  I
        *tau = 0;
        x[0] *= dScale;
        if (beta != 0)
          *beta = 0;
        return;
      }

    dMaxAbs = qMax(abs(dAlpha), abs(dScaledNorm));
    Real dBeta = -dMaxAbs * sqrt(square(dAlpha/dMaxAbs) + square(dScaledNorm/dMaxAbs));
    if (dAlpha < 0)
      dBeta = -dBeta;

    *tau = (dBeta-dAlpha) / dBeta;
    mapN(x+1, n-1, std::bind2nd(std::multiplies<Real>(), 1.0/(dAlpha-dBeta)));
    x[0] = 1;

    if (beta != 0)
      *beta = dBeta * dScale;
  }


  /**
   * Applies a reflection transform to a rectangular matrix from the
   * left. This function uses the vector representation of a
   * reflection matrix (see [householderTransform()]) to speed up
   * calculations. The algorithm is functionally equivalent to \(A
   * \gets HA\), but the full reflection matrix H is never
   * calculated. Instead, since \(H = I - \tau v v^T\), we get
   *
   * \[
   * HA = (I - \tau v v^T) A = A - \tau v v^T A = A - \tau v (A^T v)^T .
   * \]
   *
   * @param A the input matrix storing the vectors to be reflected as
   * its columns. Will be modified in place.
   *
   * @param v an iterator to the beginning of the reflector vector. Must
   * have A.rows() elements.
   *
   * @param tau the \(tau\) parameter
   *
   * @param bfr a buffer of at least A.columns() elements for
   * temporary storage of calculation results.
   */
  template <class Matrix, class InputIterator>
  void reflectColumns(Matrix& A,
                      InputIterator v,
                      typename Matrix::value_type tau,
                      typename Matrix::value_type* bfr)
  {
    typedef typename Matrix::value_type Real;

    if (tau == 0)
      return;

    const int iRows = A.rows(), iCols = A.columns();

    // tmp = (A' * v)'
    for (int i=0; i<iCols; ++i)
      bfr[i] = innerProductN(A.columnBegin(i), iRows, v, Real(0));


    // A << A - tau * v * tmp
    for (int i=0; i<iRows; ++i)
    {
      mapN(A.rowBegin(i), iCols,
           bfr,
           binaryCompose(std::minus<Real>(),
                         Identity<Real>(),
                         std::bind2nd(std::multiplies<Real>(),
                                      v[i]*tau)));
    }
  }

  template <class Matrix, class InputIterator>
  inline void reflectColumns(Matrix& A,
                             InputIterator v,
                             typename Matrix::value_type tau)
  {
    typedef typename Matrix::value_type Real;
    Real* pBfr = new Real[A.columns()];
    reflectColumns(A, v, tau, pBfr);
    delete[] pBfr;
  }

  /**
   * Applies a reflection transform to a rectangular matrix from the
   * right. The algorithm is functionally equivalent to \(A \gets
   * AH^T\), where H is the reflection transform uniquely identified
   * by *v* and `tau`.
   *
   * @param A the input matrix storing the vectors to be reflected as
   * its rows. Will be modified in place.
   *
   * @param v an iterator to the beginning of the reflector vector. Must
   * have A.columns() elements.
   *
   * @param tau the \(tau\) parameter
   *
   * @param bfr a buffer of at least A.columns() elements for
   * temporary storage of calculation results.
   *
   * @see householderTransform()
   * @see reflectColumns()
   */
  template <class Matrix, class InputIterator>
  void reflectRows(Matrix& A,
                   InputIterator v,
                   typename Matrix::value_type tau)
  {
    typedef typename Matrix::value_type Real;

    if (tau == 0)
      return;

    const int iRows = A.rows(), iCols = A.columns();

    for (int r=0; r<iRows; ++r)
      // a_i = a_i - v_i * <A, V> * tau
      // Get it? In reality, C++ is just an ugly dialect of Lisp.
      mapN(A.rowBegin(r), iCols, v,
           binaryCompose(std::minus<Real>(),
                         Identity<Real>(),
                         std::bind2nd(std::multiplies<Real>(),
                                      innerProductN(A.rowBegin(r), iCols,  v,
                                                    Real(0)) * tau)));
  }

  /**
   * Unpacks a matrix in which a series of elementary reflections are
   * represented as row or column vectors. Algorithms such as the [QR
   * decomposition](qrDecompose) and [bidiagonal
   * decomposition](bdDecomponse()) represent the decomposition result
   * in a compact form by storing the elementary reflector vectors
   * only.  Given a set of such elementary reflectors in V, this
   * function calculates the components of the left-hand reflector
   * matrix Q in the form \(Q = I + V T V^T\) (*direction* =
   * `Vertically`), or the components of the right-hand reflector
   * matrix P in the form \(P = I + V^T T^T V\) (*direction* =
   * `Horizontally`). This function requires preallocated temporary
   * storage and is intended to be used in nested loops.
   *
   * ~~~
   * direction = Vertically
   * Main diagonal        Subdiagonal
   * diagonal = 0         diagonal = 1
   * Nr = min(m, n)       Nr = min(m-1, n)
   * ( 1   x   x   x  )   ( x   x   x   x )
   * ( v1  1   x   x  )   ( 1   x   x   x )
   * ( v1  v2  1   x  )   ( v1  1   x   x )
   * ( v1  v2  v3  1  )   ( v1  v2  1   x )
   *
   * direction = Horizontally
   * Main diagonal        Superdiagonal
   * diagonal = 0         diagonal = 1
   * Nr = min(m, n)       Nr = min(m, n-1)
   * ( 1   v1  v1  v1 )   ( x   1   v1  v1 )
   * ( x   1   v2  v2 )   ( x   x   1   v2 )
   * ( x   x   1   v3 )   ( x   x   x   1  )
   * ~~~
   *
   * Note that the number of reflector vectors (Nr) may be smaller
   * than the number of rows/columns as shown in the top right
   * illustration. Generally, the number of reflectors is
   * `min(m-diagonal, n)` for the case where *direction* =
   * `Vertically`, and `min(m, n-diagonal)` if *direction* =
   * `Horizontally`. Here, m and n denote the number of rows and columns
   * in *V*.
   *
   * @param direction specifies whether the reflector vectors are
   * stored as columns (`Vertically`) or as rows (`Horizontally`).
   *
   * @param V a set of elementary reflector vectors in a compact form
   * (see the illustration above). The diagonal entries must be ones.
   * Entries marked with an "x" in the illustration are ignored. Note
   * that this function does not modify V. To get V, you need to set
   * the entries marked with x to zero.
   *
   * @param tau a pointer to the beginning of the tau vector. The
   * number of elements in this array must equal to the number of
   * elementary reflectors considered.
   *
   * @param T an output-value matrix. This must be a N-by-N matrix,
   * where N is the number of rows (*direction* = Horizontally) or
   * columns (*direction* = Vertically) in *V*. The algorithm fills
   * in the upper triangle of T.
   *
   * @param gram a temporary storage for a Gram matrix. This must be a
   * M-by-M matrix, where M is the number of reflector vectors (Nr).
   *
   * @param diagonal the diagonal on which the reflector vectors
   * start. 0 means the main diagonal, 1 is the first
   * super/subdiagonal (see the illustration above) and so on.
   */
  template <class Matrix>
  void unpackReflectors(MatrixDirection direction,
                        const Matrix& V,
                        const typename Matrix::value_type* tau,
                        PiiMatrix<typename Matrix::value_type>& T,
                        PiiMatrix<typename Matrix::value_type>& gram,
                        int diagonal = 0)
  {
    typedef typename Matrix::value_type Real;
    const int iRows = V.rows(), iCols = V.columns();
    /* Calculate the Gram matrix of V.
       As a result, gram(i,j) = <v_i, v_j>

       Since <v_i, v_j> = <v_j, v_i>, only the lower triangle will be
       calculated. Furthermore, we don't need the diagonal entries
       <v_i, v_i>, which leaves the diagonal empty as well.
    */
    gram = 0;
    // Reflectors stored as column vectors
    if (direction == Vertically)
      {
        for (int r=diagonal; r<iRows; ++r)
          {
            // Entries on the upper triangle are ignored
            int iMax = qMin(iCols, r+1-diagonal);
            for (int c=1; c<iMax; ++c)
              {
                Real v = V(r,c);
                if (v != 0)
                  // G = G + V*v
                  mapN(gram.rowBegin(c), c, V.rowBegin(r),
                       binaryCompose(std::plus<Real>(),
                                     Identity<Real>(),
                                     std::bind2nd(std::multiplies<Real>(), v)));
              }
          }
      }
    // Reflectors stored as row vectors
    else
      {
        for (int r1=qMin(iRows, iCols-diagonal)-1; r1>0; --r1)
          {
            int iStartColumn = r1 + diagonal;
            Real* pGram = gram[r1];
            for (int r2=r1; r2--; )
              pGram[r2] = innerProduct(V.rowBegin(r1) + iStartColumn, V.rowEnd(r1),
                                       V.rowBegin(r2) + iStartColumn,
                                       Real(0.0));
          }
      }
    //std::cout << "Gram = ";
    //matlabPrint(std::cout, gram);

    for (int r1=0; r1<gram.rows(); ++r1)
      {
        for (int r2=0; r2<r1; ++r2)
          T(r2,r1) = -tau[r1] *
            innerProduct(T.rowBegin(r2) + r2, T.rowEnd(r2)-diagonal,
                         gram.rowBegin(r1) + r2,
                         Real(0.0));
        // The diagonal
        typename PiiMatrix<Real>::row_iterator row = T.rowBegin(r1) + r1;
        *row++ = -tau[r1];
        // The rest of the row is zeros
        fill(row, T.rowEnd(r1), 0);
      }
  }
}

#endif //_PIIHOUSEHOLDERTRANSFORM_H

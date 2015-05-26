/*******************************************************
 * Copyright (c) 2014, ArrayFire
 * All rights reserved.
 *
 * This file is distributed under 3-clause BSD license.
 * The complete license agreement can be obtained at:
 * http://arrayfire.com/licenses/BSD-3-Clause
 ********************************************************/

#pragma once
#include <af/array.h>
#include <af/defines.h>

#if __cplusplus
namespace af
{

    /**
       C++ Interface for LU decomposition in packed format

       \param[out] out is the output array containing the packed LU decomposition
       \param[out] pivot will contain the permutation indices to map the input to the decomposition
       \param[in] in is the input array

       \ingroup lapack_factor_func_lu
    */
    AFAPI void lu(array &out, array &pivot, const array &in);

    /**
       C++ Interface for LU decomposition

       \param[out] lower will contain the lower triangular matrix of the LU decomposition
       \param[out] upper will contain the upper triangular matrix of the LU decomposition
       \param[out] pivot will contain the permutation indices to map the input to the decomposition

       \ingroup lapack_factor_func_lu
    */
    AFAPI void lu(array &lower, array &upper, array &pivot, const array &in);

    /**
      C++ Interface for in place LU decomposition

      \param[out] pivot will contain the permutation indices to map the input to the decomposition
      \param[inout] in contains the input on entry, the packed LU decomposition on exit

      \ingroup lapack_factor_func_lu
    */
    AFAPI void luInPlace(array &pivot, array &in);

    /**
       C++ Interface for QR decomposition in packed format

       \param[out] out is the output array containing the packed QR decomposition
       \param[out] tau will contain additional information needed for unpacking the data
       \param[in] in is the input array

       \ingroup lapack_factor_func_qr
    */
    AFAPI void qr(array &out, array &tau, const array &in);

    /**
       C++ Interface for QR decomposition

       \param[out] q is the orthogonal matrix from QR decomposition
       \param[out] r is the upper triangular matrix from QR decomposition
       \param[out] tau will contain additional information needed for solving a least squares problem using \p q and \p r
       \param[in] in is the input array

       \ingroup lapack_factor_func_qr
    */
    AFAPI void qr(array &q, array &r, array &tau, const array &in);

    /**
       C++ Interface for QR decomposition

       \param[out] tau will contain additional information needed for unpacking the data
       \param[inout] in is the input array on entry. It contains packed QR decomposition on exit

       \ingroup lapack_factor_func_qr
    */
    AFAPI void qrInPlace(array &tau, array &in);

    /**
       C++ Interface for cholesky decomposition

       \param[out] out contains the triangular matrix. Multiply \p out with it conjugate transpose reproduces the input \p in.
       \param[in] in is the input matrix
       \param[in] is_upper a boolean determining if \p out is upper or lower triangular

       \returns \p 0 if cholesky decomposition passes. If not returns the rank at which the decomposition failed.

       \ingroup lapack_factor_func_cholesky
    */
    AFAPI int cholesky(array &out, const array &in, const bool is_upper = true);

    /**
       C++ Interface for in place cholesky decomposition

       \param[inout] in is the input matrix on entry. It contains the triangular matrix on exit.
       \param[in] is_upper a boolean determining if \p in is upper or lower triangular

       \returns \p 0 if cholesky decomposition passes. If not returns the rank at which the decomposition failed.

       \ingroup lapack_factor_func_cholesky
    */
    AFAPI int choleskyInPlace(array &in, const bool is_upper = true);

    /**
       C++ Interface for solving a system of equations

       \param[in] a is the coefficient matrix
       \param[in] b is the measured values
       \param[in] options determining various properties of matrix \p a
       \returns \p x, the matrix of unknown variables

       \ingroup lapack_solve_func_gen

       \note currently options needs to be \ref AF_MAT_NONE
    */
    AFAPI array solve(const array &a, const array &b, const matProp options = AF_MAT_NONE);


    /**
       C++ Invert a matrix

       \param[in] a is input matrix
       \returns \p x, the inverse of the input matrix

       \ingroup lapack_ops_func_inv

       \note currently options needs to be \ref AF_MAT_NONE
    */
    AFAPI array inverse(const array &in, const matProp options = AF_MAT_NONE);
}
#endif

#ifdef __cplusplus
extern "C" {
#endif

    /**
       C Interface for LU decomposition

       \param[out] lower will contain the lower triangular matrix of the LU decomposition
       \param[out] upper will contain the upper triangular matrix of the LU decomposition
       \param[out] pivot will contain the permutation indices to map the input to the decomposition
       \param[in] in is the input matrix

       \ingroup lapack_factor_func_lu
    */
    AFAPI af_err af_lu(af_array *lower, af_array *upper, af_array *pivot, const af_array in);

    /**
       C Interface for in place LU decomposition

       \param[out] pivot will contain the permutation indices to map the input to the decomposition
       \param[inout] in contains the input on entry, the packed LU decomposition on exit

       \ingroup lapack_factor_func_lu
    */
    AFAPI af_err af_lu_inplace(af_array *pivot, af_array in);

    /**
       C Interface for QR decomposition

       \param[out] q is the orthogonal matrix from QR decomposition
       \param[out] r is the upper triangular matrix from QR decomposition
       \param[out] tau will contain additional information needed for solving a least squares problem using \p q and \p r
       \param[in] in is the input array

       \ingroup lapack_factor_func_qr
    */
    AFAPI af_err af_qr(af_array *q, af_array *r, af_array *tau, const af_array in);

    /**
       C Interface for QR decomposition

       \param[out] tau will contain additional information needed for unpacking the data
       \param[inout] in is the input array on entry. It contains packed QR decomposition on exit

       \ingroup lapack_factor_func_qr
    */
    AFAPI af_err af_qr_inplace(af_array *tau, af_array in);

    /**
       C++ Interface for cholesky decomposition

       \param[out] out contains the triangular matrix. Multiply \p out with it conjugate transpose reproduces the input \p in.
       \param[out] info is \p 0 if cholesky decomposition passes. If not returns the rank at which the decomposition failed.
       \param[in] in is the input matrix
       \param[in] is_upper a boolean determining if \p out is upper or lower triangular

       \ingroup lapack_factor_func_cholesky
    */
    AFAPI af_err af_cholesky(af_array *out, int *info, const af_array in, const bool is_upper);

    /**
       C Interface for in place cholesky decomposition

       \param[out] info is \p 0 if cholesky decomposition passes. If not returns the rank at which the decomposition failed.
       \param[inout] in is the input matrix on entry. It contains the triangular matrix on exit.
       \param[in] is_upper a boolean determining if \p in is upper or lower triangular

       \ingroup lapack_factor_func_cholesky
    */
    AFAPI af_err af_cholesky_inplace(int *info, af_array in, const bool is_upper);

    /**
       C Interface for solving a system of equations

       \param[out] x, the matrix of unknown variables
       \param[in] a is the coefficient matrix
       \param[in] b is the measured values
       \param[in] options determining various properties of matrix \p a

       \ingroup lapack_solve_func_gen

       \note currently options needs to be \ref AF_MAT_NONE
    */
    AFAPI af_err af_solve(af_array *x, const af_array a, const af_array b,
                          const af_mat_prop options);

    /**
       C Invert a matrix

       \param[out] out will contain the inverse of matrix \p in
       \param[in] in is input matrix

       \ingroup lapack_ops_func_inv

       \note currently options needs to be \ref AF_MAT_NONE
    */
    AFAPI af_err af_inverse(af_array *out, const af_array in, const af_mat_prop options);

#ifdef __cplusplus
}
#endif

/*******************************************************
 * Copyright (c) 2014, ArrayFire
 * All rights reserved.
 *
 * This file is distributed under 3-clause BSD license.
 * The complete license agreement can be obtained at:
 * http://arrayfire.com/licenses/BSD-3-Clause
 ********************************************************/

#include <af/array.h>
#include <af/arith.h>
#include <af/blas.h>
#include <af/data.h>
#include <af/traits.hpp>
#include <af/util.h>
#include <af/index.h>
#include <af/device.h>
#include <af/gfor.h>
#include <af/algorithm.h>
#include "error.hpp"

namespace af
{
    static int gforDim(af_index_t *indices)
    {
        for (int i = 0; i < AF_MAX_DIMS; i++) {
            if (indices[i].isBatch) return i;
        }
        return -1;
    }

    static af_array gforReorder(const af_array in, unsigned dim)
    {
        // This is here to stop gcc from complaining
        if (dim > 3) AF_THROW_MSG("Invalid dimension", AF_ERR_INTERNAL);
        unsigned order[AF_MAX_DIMS] = {0, 1, 2, dim};
        order[dim] = 3;
        af_array out;
        AF_THROW(af_reorder(&out, in, order[0], order[1], order[2], order[3]));
        return out;
    }

    static af::dim4 seqToDims(af_index_t *indices, af::dim4 parentDims, bool reorder = true)
    {
        std::vector<af_seq> av(AF_MAX_DIMS);
        for (int i = 0; i < AF_MAX_DIMS; i++) av[i] = indices[i].mIndexer.seq;
        af::dim4 odims = toDims(av, parentDims);

        for (int i = 0; i < AF_MAX_DIMS; i++) {
            if (!indices[i].mIsSeq) {
                dim_type elems = 0;
                AF_THROW(af_get_elements(&elems, indices[i].mIndexer.arr));
                odims[i] = elems;
            }
        }

        // Change the dimensions if inside GFOR
        if (reorder) {
            for (int i = 0; i < AF_MAX_DIMS; i++) {
                if (indices[i].isBatch) {
                    int tmp = odims[i];
                    odims[i] = odims[3];
                    odims[3] = tmp;
                    break;
                }
            }
        }

        return odims;
    }

    static unsigned size_of(af::dtype type)
    {
        switch(type) {
        case f32: return sizeof(float);
        case f64: return sizeof(double);
        case s32: return sizeof(int);
        case u32: return sizeof(unsigned);
        case s64: return sizeof(intl);
        case u64: return sizeof(uintl);
        case u8 : return sizeof(unsigned char);
        case b8 : return sizeof(unsigned char);
        case c32: return sizeof(float) * 2;
        case c64: return sizeof(double) * 2;
        default: return sizeof(float);
        }
    }

    static unsigned numDims(const af_array arr)
    {
        unsigned nd;
        AF_THROW(af_get_numdims(&nd, arr));
        return nd;
    }

    static dim4 getDims(const af_array arr)
    {
        dim_type d0, d1, d2, d3;
        AF_THROW(af_get_dims(&d0, &d1, &d2, &d3, arr));
        return dim4(d0, d1, d2, d3);
    }

    array::array(const af_array handle): arr(handle)
    {
    }

    static void initEmptyArray(af_array *arr, af::dtype ty,
                               dim_type d0, dim_type d1=1, dim_type d2=1, dim_type d3=1)
    {
        dim_type my_dims[] = {d0, d1, d2, d3};
        AF_THROW(af_create_handle(arr, AF_MAX_DIMS, my_dims, ty));
    }

    template<typename T>
    static void initDataArray(af_array *arr, const T *ptr, af_source_t src,
                              dim_type d0, dim_type d1=1, dim_type d2=1, dim_type d3=1)
    {
        af::dtype ty = (af::dtype)dtype_traits<T>::af_type;
        dim_type my_dims[] = {d0, d1, d2, d3};
        switch (src) {
        case afHost:   AF_THROW(af_create_array(arr, (const void * const)ptr, AF_MAX_DIMS, my_dims, ty)); break;
        case afDevice: AF_THROW(af_device_array(arr, (const void *      )ptr, AF_MAX_DIMS, my_dims, ty)); break;
        default: AF_THROW_MSG("Can not create array from the requested source pointer",
                              AF_ERR_INVALID_ARG);
        }
    }

    array::array() : arr(0)
    {
        initEmptyArray(&arr, f32, 0, 0, 0, 0);
    }

    array::array(const dim4 &dims, af::dtype ty) : arr(0)
    {
        initEmptyArray(&arr, ty, dims[0], dims[1], dims[2], dims[3]);
    }

    array::array(dim_type d0, af::dtype ty) : arr(0)
    {
        initEmptyArray(&arr, ty, d0);
    }

    array::array(dim_type d0, dim_type d1, af::dtype ty) : arr(0)
    {
        initEmptyArray(&arr, ty, d0, d1);
    }

    array::array(dim_type d0, dim_type d1, dim_type d2, af::dtype ty) :
        arr(0)
    {
        initEmptyArray(&arr, ty, d0, d1, d2);
    }

    array::array(dim_type d0, dim_type d1, dim_type d2, dim_type d3, af::dtype ty) :
        arr(0)
    {
        initEmptyArray(&arr, ty, d0, d1, d2, d3);
    }

#define INSTANTIATE(T)                                                  \
    template<> AFAPI                                                    \
    array::array(const dim4 &dims, const T *ptr, af_source_t src, dim_type ngfor) \
        : arr(0)                                                        \
    {                                                                   \
        initDataArray<T>(&arr, ptr, src, dims[0], dims[1], dims[2], dims[3]); \
    }                                                                   \
    template<> AFAPI                                                    \
    array::array(dim_type d0, const T *ptr, af_source_t src, dim_type ngfor) \
        : arr(0)                                                        \
    {                                                                   \
        initDataArray<T>(&arr, ptr, src, d0);                           \
    }                                                                   \
    template<> AFAPI                                                    \
    array::array(dim_type d0, dim_type d1, const T *ptr, af_source_t src, \
                 dim_type ngfor) : arr(0)                               \
    {                                                                   \
        initDataArray<T>(&arr, ptr, src, d0, d1);                       \
    }                                                                   \
    template<> AFAPI                                                    \
    array::array(dim_type d0, dim_type d1, dim_type d2, const T *ptr,   \
                 af_source_t src, dim_type ngfor) :                     \
        arr(0)                                                          \
    {                                                                   \
        initDataArray<T>(&arr, ptr, src, d0, d1, d2);                   \
    }                                                                   \
    template<> AFAPI                                                    \
    array::array(dim_type d0, dim_type d1, dim_type d2, dim_type d3, const T *ptr, \
                 af_source_t src, dim_type ngfor) :                     \
        arr(0)                                                          \
                                                                        \
    {                                                                   \
        initDataArray<T>(&arr, ptr, src, d0, d1, d2, d3);               \
    }                                                                   \

    INSTANTIATE(cdouble)
    INSTANTIATE(cfloat)
    INSTANTIATE(double)
    INSTANTIATE(float)
    INSTANTIATE(unsigned)
    INSTANTIATE(int)
    INSTANTIATE(unsigned char)
    INSTANTIATE(char)
    INSTANTIATE(intl)
    INSTANTIATE(uintl)

#undef INSTANTIATE

    array::~array()
    {
        af_array tmp = get();
        // THOU SHALL NOT THROW IN DESTRUCTORS
        af_destroy_array(tmp);
    }

    af::dtype array::type() const
    {
        af::dtype my_type;
        AF_THROW(af_get_type(&my_type, arr));
        return my_type;
    }

    dim_type array::elements() const
    {
        dim_type elems;
        AF_THROW(af_get_elements(&elems, get()));
        return elems;
    }

    void array::host(void *data) const
    {
        AF_THROW(af_get_data_ptr(data, get()));
    }

    af_array array::get()
    {
        return arr;
    }

    af_array array::get() const
    {
        return ((array *)(this))->get();
    }

    // Helper functions
    dim4 array::dims() const
    {
        return getDims(get());
    }

    dim_type array::dims(unsigned dim) const
    {
        return dims()[dim];
    }

    unsigned array::numdims() const
    {
        return numDims(get());
    }

    size_t array::bytes() const
    {
        dim_type nElements;
        AF_THROW(af_get_elements(&nElements, get()));
        return nElements * size_of(type());
    }

    array array::copy() const
    {
        af_array other = 0;
        AF_THROW(af_copy_array(&other, get()));
        return array(other);
    }

#undef INSTANTIATE
#define INSTANTIATE(fn)                         \
    bool array::is##fn() const                  \
    {                                           \
        bool ret = false;                       \
        AF_THROW(af_is_##fn(&ret, get()));      \
        return ret;                             \
    }

    INSTANTIATE(empty)
    INSTANTIATE(scalar)
    INSTANTIATE(vector)
    INSTANTIATE(row)
    INSTANTIATE(column)
    INSTANTIATE(complex)
    INSTANTIATE(double)
    INSTANTIATE(single)
    INSTANTIATE(realfloating)
    INSTANTIATE(floating)
    INSTANTIATE(integer)
    INSTANTIATE(bool)

#undef INSTANTIATE

    template<typename Idx1, typename Idx2, typename Idx3, typename Idx4>
    static array::array_proxy gen_indexing(const array &ref, const Idx1 &s0, const Idx2 &s1, const Idx3 &s2, const Idx4 &s3)
    {
        ref.eval();
        af_index_t inds[AF_MAX_DIMS];
        inds[0] = s0.get();
        inds[1] = s1.get();
        inds[2] = s2.get();
        inds[3] = s3.get();

        af_array out = 0;
        //FIXME: check if this->s has same dimensions as numdims
        AF_THROW(af_weak_copy(&out, ref.get()));
        return array::array_proxy(const_cast<array&>(ref), inds);
    }


    array::array_proxy array::operator()(const indexer &s0, const indexer &s1, const indexer &s2, const indexer &s3)
    {
        return const_cast<const array*>(this)->operator()(s0, s1, s2, s3);
    }

    const array::array_proxy array::operator()(const indexer &s0, const indexer &s1, const indexer &s2, const indexer &s3) const
    {
        if(isvector()   && s1.isspan()
                        && s2.isspan()
                        && s3.isspan()) {
            int num_dims = numDims(this->arr);

            switch(num_dims) {
            case 1: return gen_indexing(*this, s0, s1, s2, s3);
            case 2: return gen_indexing(*this, s1, s0, s2, s3);
            case 3: return gen_indexing(*this, s1, s2, s0, s3);
            case 4: return gen_indexing(*this, s1, s2, s3, s0);
            default: AF_THROW_MSG("ArrayFire internal error", AF_ERR_INTERNAL);
            }
        }
        else {
            return gen_indexing(*this, s0, s1, s2, s3);
        }

    }

    array::array_proxy array::row(int index) const
    {
        seq idx(index, index, 1);
        return this->operator()(idx, span, span, span);
    }

    array::array_proxy array::col(int index) const
    {
        seq idx(index, index, 1);
        return this->operator()(span, idx, span, span);
    }

    array::array_proxy array::slice(int index) const
    {
        seq idx(index, index, 1);
        return this->operator()(span, span, idx, span);
    }

    array::array_proxy array::rows(int first, int last) const
    {
        seq idx(first, last, 1);
        return this->operator()(idx, span, span, span);
    }

    array::array_proxy array::cols(int first, int last) const
    {
        seq idx(first, last, 1);
        return this->operator()(span, idx, span, span);
    }

    array::array_proxy array::slices(int first, int last) const
    {
        seq idx(first, last, 1);
        return this->operator()(span, span, idx, span);
    }

    array array::as(af::dtype type) const
    {
        af_array out;
        AF_THROW(af_cast(&out, this->get(), type));
        return array(out);
    }

    array::array(const array& in) : arr(0)
    {
        AF_THROW(af_weak_copy(&arr, in.get()));
    }

    // Transpose and Conjugate Transpose
    array array::T() const
    {
        return transpose(*this);
    }

    array array::H() const
    {
        return transpose(*this, true);
    }

    void array::set(af_array tmp)
    {
        if (arr) AF_THROW(af_destroy_array(arr));
        arr = tmp;
    }

    array::array_proxy&
    af::array::array_proxy::operator=(const array &other)
    {
        unsigned nd = numDims(parent->get());
        const dim4 this_dims = getDims(parent->get());
        const dim4 other_dims = other.dims();
        int dim = gforDim(indices);
        af_array other_arr = other.get();

        bool batch_assign = false;
        bool is_reordered = false;
        if (dim >= 0) {
            batch_assign = true;
            for (int i = 0; i < AF_MAX_DIMS; i++) {
                if (this->indices[i].isBatch) batch_assign &= (other_dims[i] == 1);
                else                          batch_assign &= (other_dims[i] == this_dims[i]);
            }

            if (batch_assign) {
                //FIXME: Figure out a faster, cleaner way to do this
                dim4 out_dims = seqToDims(this->indices, this_dims, false);
                af_array out;
                AF_THROW(af_tile(&out, other_arr,
                                 out_dims[0] / other_dims[0],
                                 out_dims[1] / other_dims[1],
                                 out_dims[2] / other_dims[2],
                                 out_dims[3] / other_dims[3]));
                other_arr = out;

            } else if (this_dims != other_dims) {
                // HACK: This is a quick check to see if other has been reordered inside gfor
                // TODO: Figure out if this breaks and implement a cleaner method
                other_arr = gforReorder(other_arr, dim);
                is_reordered = true;
            }
        }

        af_array tmp = 0;
        AF_THROW(af_assign_gen(&tmp, parent->get(), nd, indices, other_arr));
        parent->set(tmp);

        if (dim >= 0 && (is_reordered || batch_assign)) {
            if (other_arr) AF_THROW(af_destroy_array(other_arr));
        }
        return *this;
    }

    array::array_proxy&
    af::array::array_proxy::operator=(const array::array_proxy &other)
    {
        array out = other;
        return *this = out;
    }

    af::array::array_proxy::array_proxy(array& par, af_index_t *ssss )
        : parent(&par)
        , indices()
    {
        std::copy(ssss, ssss + AF_MAX_DIMS, indices);
    }

    array array::array_proxy::as(dtype type) const
    {
        array out = *this;
        return out.as(type);
    }

    dim_type array::array_proxy::dims(unsigned dim) const
    {
        array out = *this;
        return out.dims(dim);
    }

    void array::array_proxy::host(void *ptr) const
    {
        array out = *this;
        return out.host(ptr);
    }

    af_array array::array_proxy::get()
    {
        array tmp = *this;
        af_array out = 0;
        af_weak_copy(&out, tmp.get());
        return out;
    }

#define MEM_FUNC(PREFIX, FUNC)                  \
    PREFIX array::array_proxy::FUNC() const     \
    {                                           \
        array out = *this;                      \
        return out.FUNC();                      \
    }

    MEM_FUNC(af_array               , get)
    MEM_FUNC(dim_type               , elements)
    MEM_FUNC(array                  , T)
    MEM_FUNC(array                  , H)
    MEM_FUNC(dtype                  , type)
    MEM_FUNC(dim4                   , dims)
    MEM_FUNC(unsigned               , numdims)
    MEM_FUNC(size_t                 , bytes)
    MEM_FUNC(array                  , copy)
    MEM_FUNC(bool                   , isempty)
    MEM_FUNC(bool                   , isscalar)
    MEM_FUNC(bool                   , isvector)
    MEM_FUNC(bool                   , isrow)
    MEM_FUNC(bool                   , iscolumn)
    MEM_FUNC(bool                   , iscomplex)
    MEM_FUNC(bool                   , isdouble)
    MEM_FUNC(bool                   , issingle)
    MEM_FUNC(bool                   , isrealfloating)
    MEM_FUNC(bool                   , isfloating)
    MEM_FUNC(bool                   , isinteger)
    MEM_FUNC(bool                   , isbool)
    MEM_FUNC(void                   , eval)
    //MEM_FUNC(void                   , unlock)
#undef MEM_FUNC


#define ASSIGN_TYPE(TY, OP)                                     \
    array::array_proxy&                                         \
    array::array_proxy::operator OP(const TY &value)            \
    {                                                           \
        dim4 dims = seqToDims(indices, getDims(parent->get())); \
        af::dtype ty = parent->type();                          \
        array cst = constant(value, dims, ty);                  \
        return this->operator OP(cst);                          \
    }                                                           \

#define ASSIGN_OP(OP, op1)                      \
    ASSIGN_TYPE(double             , OP)        \
    ASSIGN_TYPE(float              , OP)        \
    ASSIGN_TYPE(cdouble            , OP)        \
    ASSIGN_TYPE(cfloat             , OP)        \
    ASSIGN_TYPE(int                , OP)        \
    ASSIGN_TYPE(unsigned           , OP)        \
    ASSIGN_TYPE(long               , OP)        \
    ASSIGN_TYPE(unsigned long      , OP)        \
    ASSIGN_TYPE(long long          , OP)        \
    ASSIGN_TYPE(unsigned long long , OP)        \
    ASSIGN_TYPE(char               , OP)        \
    ASSIGN_TYPE(unsigned char      , OP)        \
    ASSIGN_TYPE(bool               , OP)        \

    ASSIGN_OP(= , =)
    ASSIGN_OP(+=, +)
    ASSIGN_OP(-=, -)
    ASSIGN_OP(*=, *)
    ASSIGN_OP(/=, /)
#undef ASSIGN_TYPE
#undef ASSIGN_OP

#define SELF_OP(OP, op1)                                                    \
    array::array_proxy& array::array_proxy::operator OP(const array &other) \
    {                                                                       \
        *this = *this op1 other;                                            \
        return *this;                                                       \
    }                                                                       \

    SELF_OP(+=, +)
    SELF_OP(-=, -)
    SELF_OP(*=, *)
    SELF_OP(/=, /)
#undef SELF_OP

    array::array_proxy::operator array() const
    {
        af_array tmp = 0;
        af_array arr = parent->get();
        af_index_gen(&tmp, arr, AF_MAX_DIMS, indices);

        return array(tmp);
    }

    array::array_proxy::operator array()
    {
        af_array tmp = 0;
        af_array arr = parent->get();
        af_index_gen(&tmp, arr, AF_MAX_DIMS, indices);

        int dim = gforDim(indices);
        if (tmp && dim >= 0) {
            arr = gforReorder(tmp, dim);
            if (tmp) AF_THROW(af_destroy_array(tmp));
        } else {
            arr = tmp;
        }

        return array(arr);
    }

    ///////////////////////////////////////////////////////////////////////////
    // Operator =
    ///////////////////////////////////////////////////////////////////////////
    array& array::operator=(const array &other)
    {
        if (this->get() == other.get()) {
            return *this;
        }
        //TODO: Unsafe. loses data if af_weak_copy fails
        if(this->arr != 0) {
            AF_THROW(af_destroy_array(this->arr));
        }

        af_array temp = 0;
        AF_THROW(af_weak_copy(&temp, other.get()));
        this->arr = temp;
        return *this;
    }
#define ASSIGN_TYPE(TY, OP)                             \
    array& array::operator OP(const TY &value)          \
    {                                                   \
        af::dim4 dims = this->dims();                   \
        af::dtype ty = this->type();                    \
        array cst = constant(value, dims, ty);          \
        return operator OP(cst);                        \
    }                                                   \

#define ASSIGN_OP(OP, op1)                              \
    array& array::operator OP(const array &other)       \
    {                                                   \
        af_array out = 0;                               \
        op1(&out, this->get(), other.get(), gforGet()); \
        this->set(out);                                 \
        return *this;                                   \
    }                                                   \
    ASSIGN_TYPE(double             , OP)                \
    ASSIGN_TYPE(float              , OP)                \
    ASSIGN_TYPE(cdouble            , OP)                \
    ASSIGN_TYPE(cfloat             , OP)                \
    ASSIGN_TYPE(int                , OP)                \
    ASSIGN_TYPE(unsigned           , OP)                \
    ASSIGN_TYPE(long               , OP)                \
    ASSIGN_TYPE(unsigned long      , OP)                \
    ASSIGN_TYPE(long long          , OP)                \
    ASSIGN_TYPE(unsigned long long , OP)                \
    ASSIGN_TYPE(char               , OP)                \
    ASSIGN_TYPE(unsigned char      , OP)                \
    ASSIGN_TYPE(bool               , OP)                \

    ASSIGN_OP(+=, af_add)
    ASSIGN_OP(-=, af_sub)
    ASSIGN_OP(*=, af_mul)
    ASSIGN_OP(/=, af_div)

#undef ASSIGN_OP
#undef ASSIGN_TYPE

#define ASSIGN_TYPE(TY, OP)                                     \
    array& array::operator OP(const TY &value)                  \
    {                                                           \
        af::dim4 dims = this->dims();                           \
        af::dtype ty = this->type();                            \
        array cst = constant(value, dims, ty);                  \
        return operator OP(cst);                                \
    }                                                           \

#define ASSIGN_OP(OP)                           \
    ASSIGN_TYPE(double             , OP)        \
    ASSIGN_TYPE(float              , OP)        \
    ASSIGN_TYPE(cdouble            , OP)        \
    ASSIGN_TYPE(cfloat             , OP)        \
    ASSIGN_TYPE(int                , OP)        \
    ASSIGN_TYPE(unsigned           , OP)        \
    ASSIGN_TYPE(long               , OP)        \
    ASSIGN_TYPE(unsigned long      , OP)        \
    ASSIGN_TYPE(long long          , OP)        \
    ASSIGN_TYPE(unsigned long long , OP)        \
    ASSIGN_TYPE(char               , OP)        \
    ASSIGN_TYPE(unsigned char      , OP)        \
    ASSIGN_TYPE(bool               , OP)        \

    ASSIGN_OP(= )

#undef ASSIGN_OP
#undef ASSIGN_TYPE

#define BINARY_TYPE(TY, OP, func, dty)                          \
    array operator OP(const array& plhs, const TY &value)       \
    {                                                           \
        af_array out;                                           \
        af::dtype ty = plhs.type();                             \
        af::dtype cty = plhs.isrealfloating() ? ty : dty;       \
        array cst = constant(value, plhs.dims(), cty);          \
        AF_THROW(func(&out, plhs.get(), cst.get(), gforGet())); \
        return array(out);                                      \
    }                                                           \
    array operator OP(const TY &value, const array &other)      \
    {                                                           \
        const af_array rhs = other.get();                         \
        af_array out;                                           \
        af::dtype ty = other.type();                            \
        af::dtype cty = other.isrealfloating() ? ty : dty;      \
        array cst = constant(value, other.dims(), cty);         \
        AF_THROW(func(&out, cst.get(), rhs, gforGet()));        \
        return array(out);                                      \
    }                                                           \

#define BINARY_OP(OP, func)                                     \
    array operator OP(const array &lhs, const array &rhs)       \
    {                                                           \
        af_array out;                                           \
        AF_THROW(func(&out, lhs.get(), rhs.get(), gforGet()));  \
        return array(out);                                      \
    }                                                           \
    BINARY_TYPE(double             , OP, func, f64)             \
    BINARY_TYPE(float              , OP, func, f32)             \
    BINARY_TYPE(cdouble            , OP, func, c64)             \
    BINARY_TYPE(cfloat             , OP, func, c32)             \
    BINARY_TYPE(int                , OP, func, s32)             \
    BINARY_TYPE(unsigned           , OP, func, u32)             \
    BINARY_TYPE(long               , OP, func, s64)             \
    BINARY_TYPE(unsigned long      , OP, func, u64)             \
    BINARY_TYPE(long long          , OP, func, s64)             \
    BINARY_TYPE(unsigned long long , OP, func, u64)             \
    BINARY_TYPE(char               , OP, func, b8)              \
    BINARY_TYPE(unsigned char      , OP, func, u8)              \
    BINARY_TYPE(bool               , OP, func, b8)              \

    BINARY_OP(+, af_add)
    BINARY_OP(-, af_sub)
    BINARY_OP(*, af_mul)
    BINARY_OP(/, af_div)
    BINARY_OP(==, af_eq)
    BINARY_OP(!=, af_neq)
    BINARY_OP(< , af_lt)
    BINARY_OP(<=, af_le)
    BINARY_OP(> , af_gt)
    BINARY_OP(>=, af_ge)
    BINARY_OP(&&, af_and)
    BINARY_OP(||, af_or)
    BINARY_OP(%, af_rem)
    BINARY_OP(&, af_bitand)
    BINARY_OP(|, af_bitor)
    BINARY_OP(^, af_bitxor)
    BINARY_OP(<<, af_bitshiftl)
    BINARY_OP(>>, af_bitshiftr)

#undef BINARY_TYPE
#undef BINARY_OP

    array array::operator-() const
    {
        af_array lhs = this->get();
        af_array out;
        array cst = constant(0, this->dims(), this->type());
        AF_THROW(af_sub(&out, cst.get(), lhs, gforGet()));
        return array(out);
    }

    array array::operator!() const
    {
        af_array lhs = this->get();
        af_array out;
        array cst = constant(0, this->dims(), this->type());
        AF_THROW(af_eq(&out, cst.get(), lhs, gforGet()));
        return array(out);
    }

    void array::eval() const
    {
        AF_THROW(af_eval(get()));
    }

// array instanciations
#define INSTANTIATE(T)                                              \
    template<> AFAPI T *array::host() const                         \
    {                                                               \
        if (type() != (af::dtype)dtype_traits<T>::af_type) {        \
            AF_THROW_MSG("Requested type doesn't match with array", \
                         AF_ERR_INVALID_TYPE);                      \
        }                                                           \
                                                                    \
        T *res = new T[elements()];                                 \
        AF_THROW(af_get_data_ptr((void *)res, get()));              \
                                                                    \
        return res;                                                 \
    }                                                               \
    template<> AFAPI T array::scalar() const                        \
    {                                                               \
        T *h_ptr = host<T>();                                       \
        T scalar = h_ptr[0];                                        \
        delete[] h_ptr;                                             \
        return scalar;                                              \
    }                                                               \
    template<> AFAPI T* array::device() const                       \
    {                                                               \
        void *ptr = NULL;                                           \
        AF_THROW(af_get_device_ptr(&ptr, get(), true));             \
        return (T *)ptr;                                            \
    }                                                               \
    template<> AFAPI void array::write(const T *ptr,                \
               const size_t bytes, af_source_t src)                 \
    {                                                               \
        if(src == afHost)   AF_THROW(af_write_array(get(), ptr, bytes, (af_source)afHost));             \
        if(src == afDevice) AF_THROW(af_write_array(get(), ptr, bytes, (af_source)afDevice));           \
    }                                                               \

    INSTANTIATE(cdouble)
    INSTANTIATE(cfloat)
    INSTANTIATE(double)
    INSTANTIATE(float)
    INSTANTIATE(unsigned)
    INSTANTIATE(int)
    INSTANTIATE(unsigned char)
    INSTANTIATE(char)
    INSTANTIATE(intl)
    INSTANTIATE(uintl)

#undef INSTANTIATE


// array_proxy instanciations
#define TEMPLATE_MEM_FUNC(TYPE, RETURN_TYPE, FUNC)      \
    template <>                                         \
    RETURN_TYPE array::array_proxy::FUNC() const        \
    {                                                   \
        array out = *this;                              \
        return out.FUNC<TYPE>();                        \
    }

#define INSTANTIATE(T)                                  \
    TEMPLATE_MEM_FUNC(T, T*, host)                      \
    TEMPLATE_MEM_FUNC(T, T , scalar)                    \
    TEMPLATE_MEM_FUNC(T, T*, device)                    \

    INSTANTIATE(cdouble)
    INSTANTIATE(cfloat)
    INSTANTIATE(double)
    INSTANTIATE(float)
    INSTANTIATE(unsigned)
    INSTANTIATE(int)
    INSTANTIATE(unsigned char)
    INSTANTIATE(char)
    INSTANTIATE(intl)
    INSTANTIATE(uintl)

#undef INSTANTIATE
#undef TEMPLATE_MEM_FUNC
}

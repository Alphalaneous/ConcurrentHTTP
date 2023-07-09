//////////////////////////////////////////////////////////////////////////////////////////
// A multi-platform support c++11 library with focus on asynchronous socket I/O for any
// client application.
//////////////////////////////////////////////////////////////////////////////////////////
/*
SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

Copyright (c) 2012-2023 HALX99

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 The pod_vector concepts:
   a. only accept pod types
   b. support release memory ownership with `release_pointer`
   c. support pod_allocater with C(realloc) or C++(new/delete)
Copy of https://github.com/microsoft/STL/blob/main/stl/inc/vector but for pod types and only provide
'emplace_back'
*/
#pragma once
#include <string.h>
#include <utility>
#include <new>
#include <type_traits>
#include <stdexcept>
#include <limits>
#include <stdint.h>

namespace yasio
{
template <typename _Ty, bool /*_use_crt_alloc*/ = false>
struct pod_allocator {
  static _Ty* allocate(size_t count) { return (_Ty*)new uint8_t[count * sizeof(_Ty)]; }
  static void deallocate(_Ty* pBlock) { delete[] (uint8_t*)pBlock; }
};

template <typename _Ty>
struct pod_allocator<_Ty, true> {
  static _Ty* allocate(size_t count) { return malloc(count * sizeof(_Ty)); }
  static void deallocate(_Ty* pBlock) { free(pBlock); }
};

template <typename _Ty, typename _Alty = pod_allocator<_Ty>, typename std::enable_if<std::is_trivially_copyable<_Ty>::value, int>::type = 0>
class pod_vector {
public:
  using pointer         = _Ty*;
  using const_pointer   = const _Ty*;
  using reference       = _Ty&;
  using const_reference = const _Ty&;
  using size_type       = size_t;
  using value_type      = _Ty;
  using iterator        = _Ty*; // transparent iterator
  using const_iterator  = const _Ty*;
  pod_vector()          = default;
  explicit pod_vector(size_t count) { resize_fit(count); }
  pod_vector(pod_vector const& rhs) { this->assign(rhs); }
  pod_vector& operator=(pod_vector const& rhs)
  {
    this->assign(rhs);
    return *this;
  }

  pod_vector(pod_vector&& o) YASIO__NOEXCEPT : _Myfirst(o._Myfirst), _Mylast(o._Mylast), _Myend(o._Myend)
  {
    o._Myfirst = nullptr;
    o._Mylast  = nullptr;
    o._Myend   = nullptr;
  }
  pod_vector& operator=(pod_vector&& o) YASIO__NOEXCEPT
  {
    this->swap(o);
    return *this;
  }

  void assign(pod_vector const& rhs)
  {
    resize_fit(rhs.size());
    if (!rhs.empty())
      std::copy(rhs._Myfirst, rhs._Mylast, _Myfirst);
  }

  void swap(pod_vector& rhs) YASIO__NOEXCEPT
  {
    std::swap(_Myfirst, rhs._Myfirst);
    std::swap(_Mylast, rhs._Mylast);
    std::swap(_Myend, rhs._Myend);
  }

  ~pod_vector() { _Tidy(); }

  template <typename... _Valty>
  _Ty& emplace_back(_Valty&&... _Val) YASIO__NOEXCEPT
  {
    if (_Mylast != _Myend)
      return *new (_Mylast++) _Ty(std::forward<_Valty>(_Val)...);

    return *_Emplace_back_reallocate(std::forward<_Valty>(_Val)...);
  }

  void reserve(size_t new_cap)
  {
    if (new_cap > this->capacity())
      _Reallocate_exactly(new_cap);
  }

  void resize(size_t new_size)
  {
    if (this->capacity() < new_size)
      _Reallocate_exactly(_Calculate_growth(new_size));
    _Mylast = _Myfirst + new_size;
  }

  void resize_fit(size_t new_size)
  {
    if (this->capacity() < new_size)
      _Reallocate_exactly(new_size);
    _Mylast = _Myfirst + new_size;
  }

  void reset(size_t new_size)
  {
    resize_fit(new_size);
    memset(_Myfirst, 0x0, size() * sizeof(_Ty));
  }

  static YASIO__CONSTEXPR size_t max_size() YASIO__NOEXCEPT { return (std::numeric_limits<ptrdiff_t>::max)(); }
  iterator begin() YASIO__NOEXCEPT { return _Myfirst; }
  iterator end() YASIO__NOEXCEPT { return _Mylast; }
  const_iterator begin() const YASIO__NOEXCEPT { return _Myfirst; }
  const_iterator end() const YASIO__NOEXCEPT { return _Mylast; }
  pointer data() YASIO__NOEXCEPT { return _Myfirst; }
  const_pointer data() const YASIO__NOEXCEPT { return _Myfirst; }
  size_t capacity() const YASIO__NOEXCEPT { return _Myend - _Myfirst; }
  size_t size() const YASIO__NOEXCEPT { return _Mylast - _Myfirst; }
  void clear() YASIO__NOEXCEPT { _Mylast = _Myfirst; }
  bool empty() const YASIO__NOEXCEPT { return _Mylast == _Myfirst; }
  const_reference operator[](size_t index) const { return _Myfirst[index]; }
  reference operator[](size_t index) { return _Myfirst[index]; }

  void shrink_to_fit()
  { // reduce capacity to size, provide strong guarantee
    const pointer _Oldlast = _Mylast;
    if (_Oldlast != _Myend)
    { // something to do
      const pointer _Oldfirst = _Myfirst;
      if (_Oldfirst == _Oldlast)
        _Tidy();
      else
        _Reallocate_exactly(static_cast<size_type>(_Oldlast - _Oldfirst));
    }
  }

  iterator erase(iterator _Where)
  {
    // _YASIO_VERIFY_RANGE(_Where >= _Myfirst && _Where < _Mylast, "byte_buffer: out of range!");
    _Mylast = std::move(_Where + 1, _Mylast, _Where);
    return _Where;
  }

  // release memmory ownership
  pointer release_pointer() YASIO__NOEXCEPT
  {
    auto ptr = _Myfirst;
    _Myfirst = nullptr;
    _Mylast  = nullptr;
    _Myend   = nullptr;
    return ptr;
  }

private:
  template <class... _Valty>
  pointer _Emplace_back_reallocate(_Valty&&... _Val)
  {
    const auto _Oldsize = static_cast<size_type>(_Mylast - _Myfirst);

    if (_Oldsize == max_size())
      throw std::length_error("pod_vector too long");

    const size_type _Newsize     = _Oldsize + 1;
    const size_type _Newcapacity = _Calculate_growth(_Newsize);

    const pointer _Newvec = _Alty::allocate(_Newcapacity);
    const pointer _Newptr = _Newvec + _Oldsize;
    new ((void*)_Newptr) _Ty(std::forward<_Valty>(_Val)...);

    // at back, provide strong guarantee
    std::move(_Myfirst, _Mylast, _Newvec);

    _Change_array(_Newvec, _Newsize, _Newcapacity);
    return _Newptr;
  }

  void _Reallocate_exactly(size_t _Newcapacity)
  {
    const auto _Size = static_cast<size_type>(_Mylast - _Myfirst);

    const pointer _Newvec = _Alty::allocate(_Newcapacity);

    std::copy(_Myfirst, _Mylast, _Newvec);

    _Change_array(_Newvec, _Size, _Newcapacity);
  }

  void _Change_array(const pointer _Newvec, const size_type _Newsize, const size_type _Newcapacity)
  {
    if (_Myfirst)
      _Alty::deallocate(_Myfirst /*, static_cast<size_type>(_Myend - _Myfirst)*/);

    _Myfirst = _Newvec;
    _Mylast  = _Newvec + _Newsize;
    _Myend   = _Newvec + _Newcapacity;
  }

  size_type _Calculate_growth(const size_type _Newsize) const
  {
    // given _Oldcapacity and _Newsize, calculate geometric growth
    const size_type _Oldcapacity = capacity();
    const auto _Max              = max_size();

    if (_Oldcapacity > _Max - _Oldcapacity / 2)
      return _Max; // geometric growth would overflow

    const size_type _Geometric = _Oldcapacity + (_Oldcapacity >> 1);

    if (_Geometric < _Newsize)
      return _Newsize; // geometric growth would be insufficient

    return _Geometric; // geometric growth is sufficient
  }

  void _Tidy() YASIO__NOEXCEPT
  { // free all storage
    if (_Myfirst)
    { // destroy and deallocate old array
      _Alty::deallocate(_Myfirst /*, static_cast<size_type>(_Myend - _Myfirst)*/);

      _Myfirst = nullptr;
      _Mylast  = nullptr;
      _Myend   = nullptr;
    }
  }

  pointer _Myfirst = nullptr;
  pointer _Mylast  = nullptr;
  pointer _Myend   = nullptr;
};
} // namespace yasio

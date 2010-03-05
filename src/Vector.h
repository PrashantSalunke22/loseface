// Copyright (C) 2008-2010 David Capello. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE.txt file.

#ifndef LOSEFACE_VECTOR_H
#define LOSEFACE_VECTOR_H

#include <algorithm>
#include <cassert>
#include <cmath>
#include <vector>
#include <iostream>
#include <fstream>

#include "approx_eq.h"

template<class T>
class Matrix;

template<class T>
class Vector
{
public:
  typedef typename std::vector<T>::iterator iterator;
  typedef typename std::vector<T>::const_iterator const_iterator;

private:
  std::vector<T> m_data;	        // values of each element in the vector

public:
  iterator begin() { return m_data.begin(); }
  iterator end() { return m_data.end(); }
  const_iterator begin() const { return m_data.begin(); }
  const_iterator end() const { return m_data.end(); }

  Vector() {
    m_data.resize(1);
  }

  explicit Vector(size_t n) {
    assert(n >= 1);
    m_data.resize(n);
  }

  Vector(const Vector<T>& u) {
    m_data = u.m_data;
  }

  T* getRaw() { return &m_data[0]; }
  const T* getRaw() const { return &m_data[0]; }

  size_t size() const { return m_data.size(); }

  T magnitude() const {
    T sq_sum = 0.0;

    for (size_t i=0; i<size(); ++i)
      sq_sum += m_data[i]*m_data[i];

    return std::sqrt(sq_sum);
  }

  T mean() const {
    T result = 0.0;

    for (size_t i=0; i<size(); ++i)
      result += m_data[i];

    return result / T(size());
  }

  Vector& resize(size_t n) {
    assert(n >= 1);
    m_data.resize(n);
    return *this;
  }

  Vector& zero() {
    std::fill(m_data.begin(), m_data.end(), T(0));
    return *this;
  }

  T getMin() const {
    T a = m_data[0];

    for (size_t i=1; i<size(); ++i)
      if (a > m_data[i])
	a = m_data[i];

    return a;
  }

  T getMax() const {
    T a = m_data[0];

    for (size_t i=1; i<size(); ++i)
      if (a < m_data[i])
	a = m_data[i];

    return a;
  }

  size_t getMinPos() const {
    size_t k = 0;
    T a = m_data[0];

    for (int i=1; i<size(); ++i)
      if (a > m_data[i])
	a = m_data[k=i];

    return k;
  }

  size_t getMaxPos() const {
    size_t k = 0;
    T a = m_data[0];

    for (size_t i=1; i<size(); ++i)
      if (a < m_data[i])
	a = m_data[k=i];

    return k;
  }

  Matrix<T> diagonalMatrix() const {
    Matrix<T> A(size(), size());
    A.zero();

    for (size_t i=0; i<size(); ++i)
      A(i, i) = m_data[i];

    return A;
  }

  Vector& operator=(const Vector& u) {
    m_data = u.m_data;
    return *this;
  }

  Vector& operator+=(const Vector& u) {
    assert(size() == u.size());

    for (size_t i=0; i<size(); ++i)
      m_data[i] += u.m_data[i];

    return *this;
  }

  Vector& operator-=(const Vector& u) {
    assert(size() == u.size());

    for (size_t i=0; i<size(); ++i)
      m_data[i] -= u.m_data[i];

    return *this;
  }

  Vector& operator*=(T s) {
    for (size_t i=0; i<size(); ++i)
      m_data[i] *= s;

    return *this;
  }

  Vector& operator/=(T s) {
    for (size_t i=0; i<size(); ++i)
      m_data[i] /= s;

    return *this;
  }

  Vector operator*(T s) const {
    Vector w(size());

    for (size_t i=0; i<size(); ++i)
      w.m_data[i] = s * m_data[i];

    return w;
  }

  Vector operator/(T s) const {
    Vector w(size());

    for (size_t i=0; i<size(); ++i)
      w.m_data[i] = m_data[i] / s;

    return w;
  }

  Vector operator+(const Vector& u) const {
    assert(size() == u.size());
    Vector w(size());

    for (size_t i=0; i<size(); ++i)
      w.m_data[i] = m_data[i] + u.m_data[i];

    return w;
  }

  Vector operator-(const Vector& u) const {
    assert(size() == u.size());
    Vector w(size());

    for (size_t i=0; i<size(); ++i)
      w.m_data[i] = m_data[i] - u.m_data[i];

    return w;
  }

  T operator*(const Vector& u) const {
    assert(size() == u.size());
    T dot_product = 0.0;

    for (size_t i=0; i<size(); ++i)
      dot_product += m_data[i] * u.m_data[i];

    return dot_product;
  }

  bool operator==(const Vector& u) const {
    if (size() != u.size())
      return false;

    for (size_t i=0; i<size(); ++i)
      if (m_data[i] != u.m_data[i])
	return false;

    return true;
  }

  bool operator!=(const Vector& u) const {
    return !operator==(u);
  }

  inline T& operator()(size_t i) {
    assert(i >= 0 && i < size());
    return m_data[i];
  }

  inline const T& operator()(size_t i) const {
    assert(i >= 0 && i < size());
    return m_data[i];
  }

  // inline T& operator[](size_t i) {
  //   assert(i >= 0 && i < size());
  //   return m_data[i];
  // }

  // inline const T& operator[](size_t i) const {
  //   assert(i >= 0 && i < size());
  //   return m_data[i];
  // }

  //////////////////////////////////////////////////////////////////////
  // Binary I/O
  //////////////////////////////////////////////////////////////////////
  
  void save(const char* filename) const
  {
    std::ofstream f(filename, std::ios::binary);
    write(f);
  }

  void load(const char* filename)
  {
    std::ifstream f(filename, std::ios::binary);
    read(f);
  }

  void write(std::ostream& s) const
  {
    size_t n = size();
    s.write((char*)&n, sizeof(size_t));
    s.write((char*)getRaw(), sizeof(T)*n);
  }

  void read(std::istream& s)
  {
    size_t n;
    s.read((char*)&n, sizeof(size_t));
    resize(n);
    s.read((char*)getRaw(), sizeof(T)*n);
  }

}; // class Vector

template<class T>
inline Vector<T> operator*(T s, const Vector<T>& u)
{
  return u.operator*(s);
}

template<class T>
bool approx_eq(const Vector<T>& u, const Vector<T>& v, unsigned precision)
{
  if (u.size() != v.size())
    return false;

  for (size_t i=0; i<u.size(); ++i) {
    if (!approx_eq(v(i), u(i), precision))
      return false;
  }
  return true;
}

//////////////////////////////////////////////////////////////////////
// Text I/O
//////////////////////////////////////////////////////////////////////

template<class T>
std::ostream& operator<<(std::ostream& o, const Vector<T>& v)
{
  o.precision(16);

  o << "(" << v.size() << ")";
  o << "[ ";

  for (size_t i=0; i<v.size(); ++i)
    o << v(i) << " ";

  o << "]";

  return o;
}

template<class T>
std::istream& operator>>(std::istream& s, Vector<T>& v)
{
  char c = 0;
  s >> c;
  if (c != '(') {
    s.clear(std::ios_base::badbit);
    return s;
  }

  int size = 0;
  s >> size >> c;
  if (c != ')') {
    s.clear(std::ios_base::badbit);
    return s;
  }

  s >> c;
  if (c != '[') {
    s.clear(std::ios_base::badbit);
    return s;
  }

  double elem;
  int i = 0;

  v.resize(size);
  while (s >> elem)
    v(i++) = elem;

  s >> c;
  if (c != ']')
    s.clear(std::ios_base::badbit);

  return s;
}

#endif // LOSEFACE_VECTOR_H

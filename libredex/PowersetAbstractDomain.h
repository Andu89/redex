/**
 * Copyright (c) 2016-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#pragma once

#include <cstddef>
#include <initializer_list>
#include <iostream>
#include <sstream>
#include <type_traits>

#include "AbstractDomain.h"
#include "Debug.h"

/*
 * The definition of an abstract value belonging to a powerset abstract domain.
 * The `Snapshot` parameter describes the type of the container returned by the
 * elements() method. It can be a reference to the actual underlying structure
 * or a completely different type.
 *
 * Note: the default constructor is expected to generate the empty set.
 */
template <typename Element, typename Snapshot, typename Derived>
class PowersetImplementation : public AbstractValue<Derived> {
 public:
  using Kind = typename AbstractValue<Derived>::Kind;

  virtual Snapshot elements() const = 0;

  virtual size_t size() const = 0;

  virtual bool contains(const Element& e) const = 0;

  virtual void add(const Element& e) = 0;

  virtual void remove(const Element& e) = 0;

  // We only consider finite powerset domains. Hence, we don't need to define a
  // widening or narrowing operator.

  Kind widen_with(const Derived& other) override {
    return this->join_with(other);
  }

  Kind narrow_with(const Derived& other) override {
    return this->meet_with(other);
  }
};

/*
 * A powerset abstract domain is the complete lattice made of all subsets of a
 * base set of elements. Note that in this abstract domain, Bottom is different
 * from the empty set. Bottom represents an unreachable program configuration,
 * whereas the empty set may have a perfectly valid semantics (like in liveness
 * analysis or pointer analysis, for example). Since in practice the base set of
 * elements is usually very large or infinite, it is implicitly represented by
 * Top. We use the AbstractDomainScaffolding template to build the domain. The
 * choice of the underlying implementation for sets is left as a parameter of
 * the abstract domain.
 */
template <typename Element,
          typename Powerset,
          typename Snapshot,
          typename Derived>
class PowersetAbstractDomain
    : public AbstractDomainScaffolding<Powerset, Derived> {
 public:
  using AbstractValueKind = typename AbstractValue<Powerset>::Kind;

  virtual ~PowersetAbstractDomain() {
    // The destructor is the only method that is guaranteed to be created when a
    // class template is instantiated. This is a good place to perform all the
    // sanity checks on the template parameters.
    static_assert(
        std::is_base_of<PowersetImplementation<Element, Snapshot, Powerset>,
                        Powerset>::value,
        "Powerset doesn't inherit from PowersetImplementation");
    static_assert(
        std::is_base_of<
            PowersetAbstractDomain<Element, Powerset, Snapshot, Derived>,
            Derived>::value,
        "Derived doesn't inherit from PowersetAbstractDomain");
  }

  /*
   * This constructor produces the empty set, which is distinct from Bottom.
   */
  PowersetAbstractDomain() : AbstractDomainScaffolding<Powerset, Derived>() {}

  explicit PowersetAbstractDomain(AbstractValueKind kind)
      : AbstractDomainScaffolding<Powerset, Derived>(kind) {}

  Snapshot elements() const {
    always_assert(this->kind() == AbstractValueKind::Value);
    return this->get_value()->elements();
  }

  size_t size() const {
    always_assert(this->kind() == AbstractValueKind::Value);
    return this->get_value()->size();
  }

  void add(const Element& e) {
    if (this->kind() == AbstractValueKind::Value) {
      this->get_value()->add(e);
    }
  }

  void add(std::initializer_list<Element> l) { add(l.begin(), l.end()); }

  template <typename InputIterator>
  void add(InputIterator first, InputIterator last) {
    if (this->kind() == AbstractValueKind::Value) {
      Powerset* powerset = this->get_value();
      for (InputIterator it = first; it != last; ++it) {
        powerset->add(*it);
      }
    }
  }

  void remove(const Element& e) {
    if (this->kind() == AbstractValueKind::Value) {
      this->get_value()->remove(e);
    }
  }

  void remove(std::initializer_list<Element> l) {
    if (this->kind() == AbstractValueKind::Value) {
      Powerset* powerset = this->get_value();
      for (const Element& e : l) {
        powerset->remove(e);
      }
    }
  }

  template <typename InputIterator>
  void remove(InputIterator first, InputIterator last) {
    if (this->kind() == AbstractValueKind::Value) {
      Powerset* powerset = this->get_value();
      for (InputIterator it = first; it != last; ++it) {
        powerset->remove(*it);
      }
    }
  }

  bool contains(const Element& e) const {
    switch (this->kind()) {
    case AbstractValueKind::Bottom: {
      return false;
    }
    case AbstractValueKind::Top: {
      return true;
    }
    case AbstractValueKind::Value: {
      return this->get_value()->contains(e);
    }
    }
  }
};

template <typename Element,
          typename Powerset,
          typename Snapshot,
          typename Derived>
inline std::ostream& operator<<(
    std::ostream& o,
    const PowersetAbstractDomain<Element, Powerset, Snapshot, Derived>& s) {
  using AbstractValueKind =
      typename PowersetAbstractDomain<Element, Powerset, Snapshot, Derived>::
          AbstractValueKind;
  switch (s.kind()) {
  case AbstractValueKind::Bottom: {
    o << "_|_";
    break;
  }
  case AbstractValueKind::Top: {
    o << "T";
    break;
  }
  case AbstractValueKind::Value: {
    o << "[#" << s.size() << "]";
    o << "{";
    auto& elements = s.elements();
    for (auto it = elements.begin(); it != elements.end();) {
      o << *it++;
      if (it != elements.end()) {
        o << ", ";
      }
    }
    o << "}";
    break;
  }
  }
  return o;
}
#pragma once

#include "core/CoreTypes.hpp"

#include <atomic>
#include <iostream>

class Object : NonCopyable {
  u64 id{ 0 };

  std::atomic<u32> refCount{ 0 };

protected:
  Object() = default;

public:
  virtual ~Object() = default;

  void addRef() {
    refCount.fetch_add(1, std::memory_order_relaxed);
  }

  void release() {
    if (refCount.fetch_sub(1, std::memory_order_relaxed) == 1) {
      delete this;
    }
  }

  u64 getId() const { return id; }
  void setId(u64 newId) { id = newId; };

  virtual void load(std::istream& i);
  virtual void save(std::ostream& o) const;
};

template<typename T>
class ObjectPtr {
  static_assert(std::is_base_of_v<Object, T>);

  T* ptr;

public:
  ~ObjectPtr() noexcept { reset(); }

  ObjectPtr() noexcept : ptr(nullptr) {}
  ObjectPtr(T* o) : ptr(o) {
    o.addRef();
  }
  ObjectPtr(ObjectPtr const& o) noexcept : ptr(o.ptr) {
    o.addRef();
  }
  ObjectPtr(ObjectPtr&& o) noexcept : ptr(o.ptr) {
    o.ptr = nullptr;
  }

  ObjectPtr operator=(T* o) noexcept {
    o->addRef();
    reset();
    ptr = o;
    return *this;
  }
  ObjectPtr operator=(ObjectPtr const& o) noexcept {
    o.ptr->addRef();
    reset();
    ptr = o.ptr;
    return *this;
  }
  ObjectPtr operator=(ObjectPtr&& o) noexcept {
    reset();
    ptr = o.ptr;
    o.ptr = nullptr;
    return *this;
  }

  operator bool () const noexcept { return ptr != nullptr; }
  bool operator!() const noexcept { return ptr == nullptr; }

  T& operator& () const noexcept { return *ptr; }
  T* operator* () const noexcept { return  ptr; }
  T* operator->() const noexcept { return  ptr; }

  void reset() {
    if (ptr) ptr->release();
  }
};

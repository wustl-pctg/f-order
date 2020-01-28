#pragma once

#include <cstdint> // uintptr_t

#include "future.hpp"

namespace cilk {

// Allows you to treat a variable as either a future or the
// result. When the result is touched it gets replaced so that get()
// is only called once.
template <typename T>
struct futwrapper {
  enum status { FUT, REAL };
  enum status m_stat;
  union {
    cilk::future<T>* m_fut;
    T m_data;
  };
  futwrapper& operator=(T other) {
    m_data = other;
    return *this;
  }
  futwrapper& operator=(cilk::future<T>* other) {
    m_fut = other;
    return *this;
  }

  T& get() {
    if (m_stat == FUT) {
      m_data = m_fut->get();
      m_stat = REAL;
    }
    return m_data;
  }

  operator T() { return get(); }

}; // class futwrapper

// Similar to the above, except you should use this when the result of
// the future is a pointer to something. Probably not that necessary,
// but it saves having the status flag, just setting the low-order bit
// of the pointer.
template <typename T>
struct futptr {
  union {
    cilk::future<T*>* m_fut;
    T* m_data;
  };
#define PTRAND(l,r) ((uintptr_t)(l) & (uintptr_t)(r))
#define PTROR(l,r) ((uintptr_t)(l) | (uintptr_t)(r))
  bool ready() const { return !(PTRAND(m_fut,0x1)); }
  void launch() { m_fut = (cilk::future<T*>*) PTROR(m_fut,0x1); }
  void finish() { m_data = (T*) PTRAND(m_fut,~0x1); }
  cilk::future<T*>* getfut() { return (cilk::future<T*>*) PTRAND(m_fut, ~0x1); }

  futptr(T* other) { *this = other; }

  futptr& operator=(T* other) {
    m_data = other;
    finish();
    return *this;
  }
  futptr& operator=(cilk::future<T*>* other) {
    m_fut = other;
    launch();
    return *this;
  }

  T* get() {
    if (!ready()) {
      m_data = getfut()->get();
    }
    return m_data;
  }

  operator T*() { return get(); }
  T& operator*() { return *get(); }
  T* operator->() { return get(); }

}; // class futptr

} // namespace cilk

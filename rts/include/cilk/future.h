#ifndef __CILK__FUTURE_H__
#define __CILK__FUTURE_H__

#include <assert.h>
#include <functional>
//#include <vector>
//#include <internal/abi.h>
#include <internal/future_base.h>
//#include <pthread.h>
#include <cilk/handcomp-macros.h>
#include "rd_future.h"

extern "C" {
extern void __cilkrts_set_future_parent(void);
extern void __cilkrts_unset_future_parent(void);
}
void noop() {}
#define FORCE_CILK_FUNC cilk_spawn noop()

namespace cilk {

// NOTE: This is the easy method for using futures,
//       but the handcomp version has less overhead.
//       Unfortunately, there is not compiler support
//       for futures yet.
//#define cilk_future_create(type,fut,func,args...)\
//  fut = new cilk::future<type>();\
//  __cilk_future_wrapper<decltype(func), func>(fut,##args);

// You have to nest the concatenation when passing in a macro
// as an argument, else the passed in macro will not be evaluated
#define CILK_CONCAT_MACRO1(A, B)   A ## B
#define CILK_CONCAT_MACRO(A, B)    CILK_CONCAT_MACRO1(A, B)

#define cilk_future_get(fut)              (fut)->get()

// NOTE: This macro is technically 1 line, so all references to __LINE__ are the same value
#define cilk_future_create1(fut,func,args...)\
    CilkFutureCreate();\
    __cilkrts_set_future_parent();\
    cilk_fiber *CILK_CONCAT_MACRO(initial_fiber, __LINE__) = cilk_fiber_get_current_fiber();\
    if (!CILK_SETJMP(cilk_fiber_get_resume_jmpbuf(CILK_CONCAT_MACRO(initial_fiber, __LINE__ )))) {\
      char *new_sp = __cilkrts_switch_fibers();\
      char *old_sp = NULL;\
      __asm__ volatile ("mov %%rsp, %0" : "=r" (old_sp));\
      __asm__ volatile ("mov %0, %%rsp" : : "r" (new_sp));\
      __cilk_future_create_helper<decltype(func),func>(fut,##args);\
      __asm__ volatile ("mov %0, %%rsp" : : "r" (old_sp));\
      __cilkrts_switch_fibers_back(CILK_CONCAT_MACRO(initial_fiber, __LINE__));\
    }\
    cilk_fiber_do_post_switch_actions(CILK_CONCAT_MACRO(initial_fiber, __LINE__));\
    __cilkrts_unset_future_parent();\

#define cilk_future_create(type,fut,func,args...)\
  fut = new cilk::future<type>();\
  cilk_future_create1(fut,func,##args);

template<typename T>
class future : public future_base {
private:
  volatile T m_result;

public:

  future() {
    tail = &head;
  };

  ~future() {
  }

  void __attribute__((always_inline)) put(T result) {
    // C++ and its lack of default volatile copy constructors is annoying
    *(const_cast<T*>(&m_result)) = result;
    __asm__ volatile ("" ::: "memory");

    resume_deques();
  };

  T __attribute__((always_inline)) get() {
    if (!this->ready()) {
      suspend_deque();
    } else {
      // TODO: replace with FOM
      CilkFutureGet(false, this->FOM);
    }

    assert(ready());
    // C++ and its lack of default volatile copy constructors is annoying
    return *(const_cast<T*>(&m_result));
  }
}; // class future

// future<void> specialization
template<>
class future<void> : public future_base {
public:
  future() {
     tail = &head;
  };

  ~future() {
  }

  void __attribute__((always_inline)) put(void) {
      resume_deques();
  };

  void __attribute__((always_inline)) get() {
    if (!this->ready()) {
      suspend_deque();
    } else {
      CilkFutureGet(false, this->FOM);
    }

    assert(ready());
  }
}; // class future<void>

} // namespace cilk

template <typename Func, Func func, typename T, typename... Args>
void __attribute__((noinline)) __cilk_future_create_helper(cilk::future<T> *fut, Args... args) {
  FUTURE_HELPER_PREAMBLE;
    fut->put(func(args...));
  FUTURE_HELPER_EPILOGUE;
}

template <typename Func, Func func, typename... Args>
void __attribute__((noinline)) __cilk_future_create_helper(cilk::future<void> *fut, Args... args) {
  FUTURE_HELPER_PREAMBLE;
    func(args...);
    __asm__ volatile ("" ::: "memory");\
    fut->put();
  FUTURE_HELPER_EPILOGUE;
}

template <typename Func, Func func, typename T, typename... Args>
void __attribute__((noinline)) __cilk_future_wrapper(cilk::future<T> *fut, Args... args) {
  CILK_FUNC_PREAMBLE;

  START_FIRST_FUTURE_SPAWN;
    __cilk_future_create_helper<Func, func, T>(fut,args...);
  END_FUTURE_SPAWN;

  CILK_FUNC_EPILOGUE;
}

// void "template specialization" via function overloading (you cannot specialize functions)
template <typename Func, Func func, typename... Args>
void __attribute__((noinline)) __cilk_future_wrapper(cilk::future<void> *fut, Args... args) {
  CILK_FUNC_PREAMBLE;

  START_FIRST_FUTURE_SPAWN;
    __cilk_future_create_helper<Func, func>(fut,args...);
  END_FUTURE_SPAWN;

  CILK_FUNC_EPILOGUE;
}

#endif // #ifndef __CILK__FUTURE_H__

#pragma once
// Template class for futures
/* In an ideal world, this file would not get instrumented by thread
   sanitizer. Unfortunately this is not an ideal world, so this file
   gets inlined by the preprocessor into the client code and then
   instrumented by TSAN. This causes some headaches, as seen below,
   but can be worked around. If someone has some time on their hands,
   they should merge our Cilk Plus branch of llvm/clang with a later
   version, which includes attributes for disabling thread sanitizer.
*/

#include <cassert>

#if REACH_MAINT == 1 || RACE_DETECT == 1
#include "rd.hpp"

// Convenience
#define __DC race_detector::disable_checking()
#define __EC race_detector::enable_checking()
#define __FS //cilk_future_start()
#define __FC //cilk_future_continuation();

extern "C" {
void cilk_future_create();
void cilk_future_get_begin(sfut_data *);
void cilk_future_get_end(sfut_data *);
void cilk_future_finish_begin(sfut_data *);
void cilk_future_finish_end(sfut_data *);
void cilk_future_helper_leave(sfut_data *);
void cilk_future_put_begin(sfut_data *);
void cilk_future_put_end(sfut_data *);
}

// Not supported in our version of clang...
//#define NOSANITIZE __attribute__((no_sanitizer("thread")))
#define NOSANITIZE
#else
#define __DC
#define __EC
#define __FS
#define __FC

/// @todo{ Put weak symbols for cilktool functions for futures in the runtime }
struct sfut_data {};
__attribute__((weak)) void cilk_future_create() {}
__attribute__((weak)) void cilk_future_get_begin(struct sfut_data *) {}
__attribute__((weak)) void cilk_future_get_end(struct sfut_data *) {}
__attribute__((weak)) void cilk_future_finish_begin(struct sfut_data *) {}
__attribute__((weak)) void cilk_future_finish_end(struct sfut_data *) {}
__attribute__((weak)) void cilk_future_helper_leave(sfut_data *) {}
__attribute__((weak)) void cilk_future_put_begin(struct sfut_data *) {}
__attribute__((weak)) void cilk_future_put_end(struct sfut_data *) {}
#define NOSANITIZE

#endif // RACE_MAINT || RACE_DETECT

// Without this, there are sometimes races on future handles. These
// are not real races, but due to the fact that checking is not
// disabled until we fully enter the future methods.
#define cilk_future_get(fut) __DC; (fut)->get(); __EC;
#define cilk_future_get_result(v, fut) __DC; (v) = (fut)->get(); __EC;

namespace cilk {

template<typename T>
class future {
private:
       enum class status { CREATED,  // memory allocated, initialized
                           //STARTED,
                           PUT,      // value is ready
                           DONE,     // strand has finished execution
                           };

  status m_stat;
  T m_val;

public:
  sfut_data m_rd_data;

  NOSANITIZE future() : m_stat(status::CREATED)
  { __DC; cilk_future_create(); __EC; }

  // NOSANITIZE void start()
  // { __DC; m_stat = status::STARTED; cilk_future_create(); __EC; }

    NOSANITIZE void put(T val) {
    __DC;
    cilk_future_put_begin(&m_rd_data);
    m_val = val;
    m_stat = status::PUT;

    // Make sure we make a new strand here.
    cilk_future_put_end(&m_rd_data);
    __EC;
  }

  NOSANITIZE void finish() {
    __DC;
    cilk_future_finish_begin(&m_rd_data);
    assert(m_stat == status::PUT);
    m_stat = status::DONE;
    cilk_future_finish_end(&m_rd_data);
    __EC;
  }

  NOSANITIZE void finish(T val) {
    /* I'd like to simply do this:
     *  put(val); finish();
     * But then clear_stack doesn't work correctly because all of
     * the functions in this file get instrumented. In other words,
     * the stack would get cleared at the end of finish(), instead of
     * the end of this finish(val). */

    __DC;
    //cilk_future_finish_begin(&m_rd_data); // disables checking
    m_val = val;
    m_stat = status::DONE;
    //cilk_future_finish_end(&m_rd_data); // enables checking
    __EC;
  }

  NOSANITIZE bool ready() { __DC; bool res = m_stat >= status::PUT; __EC; return res; }
  NOSANITIZE T get() {
    __DC;
    cilk_future_get_begin(&m_rd_data);
    assert(m_stat == status::DONE); // for sequential futures
    cilk_future_get_end(&m_rd_data);
    __EC;
    return m_val;
  }
}; // class future

// template<typename T>
// class future_ptr {
// public:
//   future<T>* m_fut;
//   future_ptr() = delete;
//   future_ptr(future<T>* f) : m_fut(f) {}
//   bool ready() { return (m_fut) ? m_fut->ready() : false; }
//   void touch() { assert(m_fut); m_fut->get(); }

//   bool operator==(const void* p) const { return m_fut == p; }


//   // Convenience, allows us to use futures in place of variables everywhere
//   // Do we really want to call get here? It may increase get_count
//   // even if we know it's already ready...
//   T* operator->() const { return m_fut->get(); }
//   T* operator*() const { return m_fut->get(); }
// }; // class future_ptr


} // namespace cilk

#ifndef __cilkfutures
#include "future_fake.hpp"
#endif
